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

    class SshWorkerPool : public IDeviceWorkerPool {
    private:
        // Ссылка на планировщик асинхронных событий (io_context), в потоке которого работают сокеты и таймеры сканера
        asio::any_io_executor executor_;

        // Вспомогательная корутина неблокирующего чтения всего вывода одного exec-канала
        asio::awaitable<std::pair<bool, std::string>> run_exec_channel(
                tcp::socket& socket,
                LIBSSH2_SESSION* session,
                const std::string& command,
                std::string& out_error)
        {
            // Неблокирующее открытие логического канала внутри SSH-сессии для выполнения команды.
            // Если ответ от коммутатора ещё не пришёл (EAGAIN), корутина засыпает до появления данных в сокете, не блокируя поток
            LIBSSH2_CHANNEL* channel = nullptr;
            while ((channel = libssh2_channel_open_session(session)) == nullptr) {
                if (libssh2_session_last_error(session, nullptr, nullptr, 0) == LIBSSH2_ERROR_EAGAIN) {
                    co_await socket.async_wait(tcp::socket::wait_read, asio::use_awaitable);
                } else {
                    out_error = "Failed to open SSH exec channel";
                    co_return std::make_pair(false, std::string{});
                }
            }

            // Гарантированное закрытие канала при выходе
            // Автоматический «охранник» ресурса (RAII): при любом выходе из корутины (успех, ошибка или отмена по таймауту)
            // гарантированно вызовет переданную функцию-очиститель и освободит память C-структуры канала libssh2
            auto channel_guard = std::unique_ptr<LIBSSH2_CHANNEL, void(*)(LIBSSH2_CHANNEL*)>(
                    channel, [](LIBSSH2_CHANNEL* ch) {
                        if (ch) {
                            libssh2_channel_close(ch);
                            libssh2_channel_free(ch);
                        }
                    });

            // Неблокирующая отправка команды в режиме exec (без интерактивного терминала и без пауз --More--).
            // Код 0 — коммутатор принял команду к выполнению; EAGAIN — ждём сетевого обмена
            int rc = 0;
            while ((rc = libssh2_channel_exec(channel, command.c_str())) == LIBSSH2_ERROR_EAGAIN) {
                co_await socket.async_wait(tcp::socket::wait_read, asio::use_awaitable);
            }
            if (rc != 0) {
                out_error = "SSH exec failed for command: " + command;
                co_return std::make_pair(false, std::string{});
            }

            // Неблокирующее вычитывание всего вывода команды порциями по 4 КБ до закрытия канала коммутатором (bytes == 0 / EOF).
            // Если очередная порция пакетов ещё в пути (EAGAIN), корутина засыпает на сокете, не блокируя поток
            std::string output;
            char buffer[4096];
            for (;;) {
                ssize_t bytes = libssh2_channel_read(channel, buffer, sizeof(buffer));
                if (bytes > 0) {
                    output.append(buffer, static_cast<size_t>(bytes));
                } else if (bytes == LIBSSH2_ERROR_EAGAIN) {
                    co_await socket.async_wait(tcp::socket::wait_read, asio::use_awaitable);
                } else if (bytes == 0) {
                    break; // EOF — вывод команды завершён
                } else {
                    out_error = "SSH channel read error";
                    co_return std::make_pair(false, output);
                }
            }

            co_return std::make_pair(true, std::move(output));
        }

        // Основная логика одной SSH-транзакции (без внешнего таймера)
        // Полный сценарий работы с одним коммутатором в рамках одной SSH-сессии:
        // TCP-подключение -> крипто-handshake -> авторизация -> [опционально: ping прогрева + пауза] -> выполнение целевой команды.
        // Внешний таймаут накладывается уровнем выше в execute_single_ssh
        asio::awaitable<DeviceResult> execute_ssh_core(DeviceTask task, std::shared_ptr<tcp::socket> socket_ptr) {
            DeviceResult result;
            result.ip = task.ip;
            result.port = task.port;

            tcp::socket& socket = *socket_ptr;
            LIBSSH2_SESSION* session = nullptr;

            try {

                // Неблокирующее для потока ОС разрешение адреса и установка TCP-соединения с коммутатором:
                // текущая корутина приостанавливается на co_await до завершения рукопожатия TCP, уступая CPU другим воркерам
                tcp::resolver resolver(executor_);
                auto endpoints = co_await resolver.async_resolve(task.ip, std::to_string(task.port), asio::use_awaitable);
                co_await asio::async_connect(socket, endpoints, asio::use_awaitable);

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
                while ((rc = libssh2_session_handshake(session, socket.native_handle())) == LIBSSH2_ERROR_EAGAIN) {
                    co_await socket.async_wait(tcp::socket::wait_read, asio::use_awaitable);
                }
                if (rc != 0) {
                    result.error_message = "SSH handshake failed";
                    co_return result;
                }

                // 2. Авторизация по логину и паролю
                while ((rc = libssh2_userauth_password(session, task.username.c_str(), task.password.c_str())) == LIBSSH2_ERROR_EAGAIN) {
                    co_await socket.async_wait(tcp::socket::wait_read, asio::use_awaitable);
                }
                if (rc != 0) {
                    result.error_message = "SSH authentication failed";
                    co_return result;
                }

                // 3. Если задана команда прогрева FDB (Фаза 2: ping + пауза)
                if (task.warmup_command.has_value() && !task.warmup_command->empty()) {
                    auto [ok_ping, ping_out] = co_await run_exec_channel(socket, session, *task.warmup_command, result.error_message);
                    if (!ok_ping) {
                        co_return result;
                    }

                    // Неблокирующая пауза внутри сессии, пока остальные воркеры тоже пингуют
                    asio::steady_timer warmup_timer(executor_);
                    warmup_timer.expires_after(task.warmup_delay);
                    boost::system::error_code ec;
                    co_await warmup_timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
                }

                // 4. Выполнение целевой команды (сбор конфигурации или MAC-таблицы)
                auto [ok_cmd, cmd_out] = co_await run_exec_channel(socket, session, task.command, result.error_message);
                if (!ok_cmd) {
                    co_return result;
                }

                result.raw_output = std::move(cmd_out);
                result.success = true;
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

            auto socket_ptr = std::make_shared<tcp::socket>(executor_);
            asio::steady_timer timeout_timer(executor_);
            timeout_timer.expires_after(timeout_ms);

            auto race = co_await (
                    execute_ssh_core(task, socket_ptr) ||
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
                co_return timeout_res;
            }

            boost::system::error_code ec;
            socket_ptr->shutdown(tcp::socket::shutdown_both, ec);
            socket_ptr->close(ec);

            co_return std::get<0>(std::move(race));
        }

    public:
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
