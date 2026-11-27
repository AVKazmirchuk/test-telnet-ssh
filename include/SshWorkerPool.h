//
// Created by Alexander on 20.11.2026.
//

#ifndef TEST_TELNET_SSH_SOLUTION_SSHWORKERPOOL_H
#define TEST_TELNET_SSH_SOLUTION_SSHWORKERPOOL_H

#pragma once
#include "DeviceWorkerTypes.h"
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <libssh2.h>
#include <memory>

namespace net_discovery {

    using asio::ip::tcp;
    using namespace asio::experimental::awaitable_operators;

    //Пул воркеров опроса узлов по SSH
    class SshWorkerPool : public IDeviceWorkerPool {
    private:
        // Ссылка на планировщик асинхронных событий (io_context), в потоке которого работают сокеты и таймеры сканера
        asio::any_io_executor executor_;

        asio::awaitable<void> wait_socket(tcp::socket& socket, LIBSSH2_SESSION* session) {
            int dir = libssh2_session_block_directions(session);
            if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND) {
                co_await socket.async_wait(tcp::socket::wait_write, asio::use_awaitable);
            } else {
                co_await socket.async_wait(tcp::socket::wait_read, asio::use_awaitable);
            }
        }



        // Вспомогательная корутина неблокирующего чтения всего вывода одного exec-канала
        asio::awaitable<std::pair<bool, std::string>> run_exec_channel(
                tcp::socket& socket,
                LIBSSH2_SESSION* session,
                const std::string& command,
                const std::optional<std::string>& warmup_cmd,
                std::chrono::milliseconds warmup_delay,
                std::string& out_error)
        {
            // Неблокирующее открытие логического канала внутри SSH-сессии для выполнения команды.
            // Если ответ от коммутатора ещё не пришёл (EAGAIN), корутина засыпает до появления данных в сокете, не блокируя поток
            LIBSSH2_CHANNEL* channel = nullptr;
            while ((channel = libssh2_channel_open_session(session)) == nullptr) {
                if (libssh2_session_last_errno(session) == LIBSSH2_ERROR_EAGAIN) {
                    co_await wait_socket(socket, session);
                } else {
                    out_error = "Failed to open SSH exec channel";
                    co_return std::make_pair(false, std::string{});
                }
            }

            // Гарантированное закрытие канала при выходе
            // Автоматический «охранник» ресурса (RAII): при любом выходе из корутины (успех, ошибка или отмена по таймауту)
            // гарантированно вызовет переданную функцию-очиститель и освободит память C-структуры канала libssh2
            std::unique_ptr<LIBSSH2_CHANNEL, decltype(&libssh2_channel_free)> channel_guard(channel, libssh2_channel_free);


            // 1. Запрашиваем PTY
            int rc = 0;
            while ((rc = libssh2_channel_request_pty(channel, "vt100")) == LIBSSH2_ERROR_EAGAIN) {
                co_await wait_socket(socket, session);
            }
            if (rc != 0) {
                out_error = "Failed to request PTY";
                co_return std::make_pair(false, std::string{});
            }

            while ((rc = libssh2_channel_shell(channel)) == LIBSSH2_ERROR_EAGAIN) {
                co_await wait_socket(socket, session);
            }
            if (rc != 0) {
                out_error = "Failed to start shell";
                co_return std::make_pair(false, std::string{});
            }

            // Вспомогательная лямбда: отправка одной строки в канал
            auto send_line = [&](const std::string& line) -> asio::awaitable<bool> {
                std::string payload = line + "\r\n";
                size_t sent = 0;
                while (sent < payload.size()) {
                    ssize_t w = libssh2_channel_write(channel, payload.data() + sent, payload.size() - sent);
                    if (w > 0) {
                        sent += static_cast<size_t>(w);
                    } else if (w == LIBSSH2_ERROR_EAGAIN) {
                        co_await wait_socket(socket, session);
                    } else {
                        co_return false;
                    }
                }
                co_return true;
            };

            // Вспомогательная лямбда: чтение вывода до появления приглашения CLI ('#' или '>')
            auto read_until_prompt = [&]() -> asio::awaitable<std::pair<bool, std::string>> {
                std::string chunk_out;
                char buf[4096];
                for (;;) {
                    ssize_t bytes = libssh2_channel_read(channel, buf, sizeof(buf));
                    if (bytes > 0) {
                        chunk_out.append(buf, static_cast<size_t>(bytes));
                        size_t tail_start = (chunk_out.size() > 32) ? (chunk_out.size() - 32) : 0;
                        if (chunk_out.find('#', tail_start) != std::string::npos ||
                            chunk_out.find('>', tail_start) != std::string::npos) {
                            co_return std::make_pair(true, std::move(chunk_out));
                        }
                    } else if (bytes == LIBSSH2_ERROR_EAGAIN) {
                        co_await wait_socket(socket, session);
                    } else if (bytes == 0) {
                        co_return std::make_pair(true, std::move(chunk_out));
                    } else {
                        co_return std::make_pair(false, std::move(chunk_out));
                    }
                }
            };

            // 3. Вычитываем стартовый баннер и первое приглашение свитча
            auto [banner_ok, banner] = co_await read_until_prompt();
            if (!banner_ok) {
                out_error = "Failed reading initial CLI prompt";
                co_return std::make_pair(false, std::string{});
            }

            if (!co_await send_line("terminal datadump")) {
                out_error = "Failed sending terminal datadump";
                co_return std::make_pair(false, std::string{});
            }
            co_await read_until_prompt();

            if (warmup_delay.count() > 0) {
                asio::steady_timer w_timer(executor_);
                w_timer.expires_after(warmup_delay);
                boost::system::error_code w_ec;
                co_await w_timer.async_wait(asio::redirect_error(asio::use_awaitable, w_ec));
            }

            // 5. Разбиваем command по ';' и выполняем каждую команду по очереди
            std::string full_output;
            size_t start = 0;
            while (start <= command.size()) {
                size_t sep = command.find(';', start);
                std::string single_cmd = (sep == std::string::npos)
                                         ? command.substr(start)
                                         : command.substr(start, sep - start);

                // Убираем ведущие и хвостовые пробелы
                size_t first = single_cmd.find_first_not_of(" \t\r\n");
                if (first != std::string::npos) {
                    size_t last = single_cmd.find_last_not_of(" \t\r\n");
                    single_cmd = single_cmd.substr(first, last - first + 1);

                    if (!co_await send_line(single_cmd)) {
                        out_error = "Failed sending command: " + single_cmd;
                        co_return std::make_pair(false, full_output);
                    }
                    auto [cmd_ok, cmd_out] = co_await read_until_prompt();
                    if (!cmd_ok) {
                        out_error = "Failed reading output for: " + single_cmd;
                        co_return std::make_pair(false, full_output);
                    }
                    full_output += cmd_out;
                    full_output += "\n";
                }

                if (sep == std::string::npos) break;
                start = sep + 1;
            }

            co_return std::make_pair(true, std::move(full_output));

        }

        // Основная логика одной SSH-транзакции (без внешнего таймера)
        // Полный сценарий работы с одним коммутатором в рамках одной SSH-сессии:
        // TCP-подключение -> крипто-handshake -> авторизация -> [опционально: ping прогрева + пауза] -> выполнение целевой команды.
        // Внешний таймаут накладывается уровнем выше в execute_single_ssh
        asio::awaitable<DeviceResult> execute_ssh_core(DeviceTask task, std::shared_ptr<tcp::socket> socket_ptr, std::shared_ptr<std::string> stage) {
            DeviceResult result;
            result.ip = task.ip;
            result.port = task.port;

            tcp::socket& socket = *socket_ptr;
            LIBSSH2_SESSION* session = nullptr;

            try {

                // Неблокирующее для потока ОС разрешение адреса и установка TCP-соединения с коммутатором:
                // текущая корутина приостанавливается на co_await до завершения рукопожатия TCP, уступая CPU другим воркерам
                *stage = "tcp_connect";
                tcp::endpoint ep(asio::ip::make_address(task.ip), task.port);
                co_await socket.async_connect(ep, asio::use_awaitable);

                // Создаём объект SSH-сессии и переводим её в неблокирующий режим (0):
                // вместо заморозки потока при ожидании сети функции будут возвращать EAGAIN для передачи управления корутине
                session = libssh2_session_init();
                if (!session) {
                    result.error_message = "Failed to init libssh2 session";
                    co_return result;
                }
                libssh2_session_set_blocking(session, 0);

                // Автоматическая очистка крипто-сессии при любом выходе (включая отмену по таймауту)
                auto session_guard = std::unique_ptr<LIBSSH2_SESSION, void(*)(LIBSSH2_SESSION*)>(
                        session, [](LIBSSH2_SESSION* s) {
                            if (s) {
                                libssh2_session_disconnect(s, "Normal Shutdown");
                                libssh2_session_free(s);
                            }
                        });

                // 1. Криптографический Handshake
                int rc = 0;
                *stage = "handshake";
                while ((rc = libssh2_session_handshake(session, socket.native_handle())) == LIBSSH2_ERROR_EAGAIN) {
                    co_await wait_socket(socket, session);
                }
                if (rc != 0) {
                    result.error_message = "SSH handshake failed";
                    co_return result;
                }

                *stage = "auth";
                // 2. Авторизация по логину и паролю
                while ((rc = libssh2_userauth_password(session, task.username.c_str(), task.password.c_str())) == LIBSSH2_ERROR_EAGAIN) {
                    co_await wait_socket(socket, session);
                }
                if (rc != 0) {
                    result.error_message = "SSH authentication failed";
                    co_return result;
                }

                // 3. Выполнение прогрева и основной команды в одном канале
                *stage = "open_channel";
                std::string err_msg;
                auto [exec_ok, output] = co_await run_exec_channel(
                        socket, session, task.command, task.warmup_command, task.warmup_delay, err_msg
                );
                if (!exec_ok) {
                    result.error_message = err_msg;
                    co_return result;
                }

                result.success = true;
                result.raw_output = std::move(output);
            }
            catch (const std::exception& e) {
                result.error_message = std::string("SSH exception: ") + e.what();
            }

            co_return result;
        }

        // Обёртка с жёстким общим таймаутом на всю сессию (гонка ||)
        // Обёртка-предохранитель с жёстким общим таймаутом на всю SSH-сессию (гонка execute_ssh_core || timer).
        // Если свитч завис на любом этапе, таймер принудительно закрывает сокет извне, сворачивает корутину и освобождает воркер
        asio::awaitable<DeviceResult> execute_single_ssh(DeviceTask task, std::chrono::milliseconds timeout_ms) {

            auto stage = std::make_shared<std::string>("init");

            auto socket_ptr = std::make_shared<tcp::socket>(executor_);
            asio::steady_timer timeout_timer(executor_);
            timeout_timer.expires_after(timeout_ms);

            auto race = co_await (
                    execute_ssh_core(task, socket_ptr, stage) ||
                    timeout_timer.async_wait(asio::use_awaitable)
            );

            if (race.index() == 1) {
                // Таймер сработал первым — принудительно закрываем сокет, чтобы разбудить и свернуть корутину
                boost::system::error_code ec;
                socket_ptr->close(ec);

                DeviceResult timeout_res;
                timeout_res.ip = task.ip;
                timeout_res.port = task.port;
                timeout_res.success = false;
                timeout_res.error_message = "SSH operation timed out";
                timeout_res.error_message = "SSH timed out at stage: " + *stage;
                co_return timeout_res;
            }

            boost::system::error_code ec;
            socket_ptr->shutdown(tcp::socket::shutdown_both, ec);
            socket_ptr->close(ec);

            co_return std::get<0>(std::move(race));
        }

    public:

        //Инициализирует библиотеку libssh
        explicit SshWorkerPool(asio::any_io_executor exec) : executor_(exec) {
            libssh2_init(0);
        }

        ~SshWorkerPool() override {
            libssh2_exit();
        }

        // Диспетчер параллельного опроса: помещает все задачи в закрытый канал Asio, запускает пул из
        // max_concurrent_workers корутин-воркеров для одновременного опроса устройств и ждёт завершения всей группы
        asio::awaitable<std::vector<DeviceResult>> execute_parallel_commands(
                const std::vector<DeviceTask>& tasks,
                std::chrono::milliseconds timeout_ms,
                size_t max_concurrent_workers) override
        {
            std::vector<DeviceResult> total_results;
            if (tasks.empty()) {
                co_return total_results;
            }

            total_results.reserve(tasks.size());
            size_t effective_workers = std::min(max_concurrent_workers, tasks.size());

            asio::experimental::channel<void(boost::system::error_code, DeviceTask)> task_channel(executor_, tasks.size());
            for (const auto& task : tasks) {
                task_channel.try_send(boost::system::error_code{}, task);
            }
            task_channel.close();

            auto worker = [&](std::vector<DeviceResult>& shared_results) -> asio::awaitable<void> {
                try {
                    for (;;) {
                        boost::system::error_code ec;
                        DeviceTask task = co_await task_channel.async_receive(asio::redirect_error(asio::use_awaitable, ec));
                        if (ec == asio::experimental::channel_errc::channel_closed) {
                            break;
                        }
                        DeviceResult res = co_await execute_single_ssh(std::move(task), timeout_ms);
                        shared_results.push_back(std::move(res));
                    }
                } catch (...) {}
                co_return;
            };

            auto remaining_workers = std::make_shared<size_t>(effective_workers);
            auto wait_timer = std::make_shared<asio::steady_timer>(executor_);
            wait_timer->expires_at(std::chrono::steady_clock::time_point::max());

            for (size_t i = 0; i < effective_workers; ++i) {
                asio::co_spawn(executor_, [this, &worker, &total_results, remaining_workers, wait_timer]() -> asio::awaitable<void> {
                    co_await worker(total_results);
                    if (--(*remaining_workers) == 0) {
                        wait_timer->cancel();
                    }
                    co_return;
                }, asio::detached);
            }

            if (*remaining_workers > 0) {
                boost::system::error_code ec;
                co_await wait_timer->async_wait(asio::redirect_error(asio::use_awaitable, ec));
            }

            co_return total_results;
        }
    };

} // namespace net_discovery

#endif //TEST_TELNET_SSH_SOLUTION_SSHWORKERPOOL_H
