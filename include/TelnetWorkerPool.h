//
// Created by Alexander on 20.11.2026.
//

#ifndef TEST_TELNET_SSH_SOLUTION_TELNETWORKERPOOL_H
#define TEST_TELNET_SSH_SOLUTION_TELNETWORKERPOOL_H

#pragma once
#include "DeviceWorkerTypes.h"
#include <algorithm>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <cctype>
#include <functional>
#include <memory>
#include <telnetpp/session.hpp>
#include <telnetpp/options/echo/client.hpp>

namespace net_discovery {

    using asio::ip::tcp;
    using namespace asio::experimental::awaitable_operators;

    // Адаптер канала (Channel) между Boost.Asio и telnetpp::session для автоматизации без внутреннего цикла чтения
    struct asio_script_channel {
        // Ссылка на активный TCP-сокет Boost.Asio, через который отправляются управляющие ответы протокола Telnet
        tcp::socket& socket;
        // Колбэк внутреннего парсера telnetpp: сохраняется при вызове async_read и вызывается вручную из нашей корутины при получении байтов из сокета
        std::function<void(telnetpp::bytes)> telnet_parser_callback = nullptr;

        // Колбэк исходящих данных для telnetpp: отправляет в сокет служебные пакеты согласования опций Telnet (WILL/WONT/DO/DONT)
        void write(telnetpp::bytes data) {
            if (!data.empty() && socket.is_open()) {
                boost::system::error_code ec;
                // Используем синхронную запись, так как telnetpp вызывает ее внутри логики сессии,
                // но сокет находится под управлением асинхронного цикла
                asio::write(socket, asio::buffer(data.data(), data.size()), ec);
            }
        }

        // Регистрация приёмного колбэка от telnetpp::session: вместо запуска фонового чтения сохраняет обработчик,
        // чтобы корутина wait_for_any_prompt сама передавала в него прочитанные из сокета байты
        void async_read(std::function<void(telnetpp::bytes)> callback) {
            telnet_parser_callback = std::move(callback);
        }

        // Проверка активности соединения (требуется концептом канала telnetpp): возвращает true, пока TCP-сокет открыт
        bool is_alive() const {
            return socket.is_open();
        }

        // Штатное закрытие канала со стороны telnetpp: корректно завершает приём/передачу (shutdown) и закрывает сокет без исключений
        void close() {
            if (socket.is_open()) {
                boost::system::error_code ec;
                socket.shutdown(tcp::socket::shutdown_both, ec);
                socket.close(ec);
            }
        }
    };

    //Пул воркеров опроса узлов по Telnet
    class TelnetWorkerPool : public IDeviceWorkerPool {
    private:
        asio::any_io_executor executor_;

        // Регистронезависимый поиск любого из ожидаемых промптов в конце/теле буфера
        // Регистронезависимый поиск хотя бы одного ожидаемого приглашения CLI (из списка prompts) в накопленном буфере ответа.
        // Позволяет одновременно ловить варианты разных вендоров ("Username:", "login:", "Password:", "#", ">") независимо от регистра букв
        static bool contains_any_prompt_icase(const std::string& buffer, const std::vector<std::string>& prompts) {
            std::string lower_buf = buffer;
            std::transform(lower_buf.begin(), lower_buf.end(), lower_buf.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            for (const auto& p : prompts) {
                std::string lower_p = p;
                std::transform(lower_p.begin(), lower_p.end(), lower_p.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (lower_buf.find(lower_p) != std::string::npos) {
                    return true;
                }
            }
            return false;
        }

        // Очистка и пролистывание постраничного вывода (--More--)
        // Обработчик постраничного вывода (пейджера): если коммутатор приостановил вывод маркером "--More--",
        // вырезает этот маркер из буфера (чтобы не портить парсинг таблиц) и отправляет пробел (" ") для загрузки следующей страницы
        static bool handle_pager_if_present(std::string& buffer, telnetpp::session& session) {
            // Ищем типичные маркеры пейджера Cisco/Huawei/Eltex/D-Link
            const std::vector<std::string> pagers = { "--More--", "-- More --", "---- More ----", "Next Page" };
            for (const auto& pager : pagers) {
                size_t pos = buffer.find(pager);
                if (pos != std::string::npos) {
                    // Вырезаем сам маркер пейджера, чтобы он не портил парсинг таблиц
                    buffer.erase(pos, pager.size());
                    // Отправляем пробел для получения следующей страницы
                    send_text(session, " ");
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Неблокирующее ожидание любого промпта из списка с автопролистыванием --More--
         */
        // Асинхронный цикл чтения вывода из Telnet-сессии до появления любого ожидаемого приглашения из target_prompts.
        // Фильтрует управляющие байты Telnet через telnetpp, автоматически пролистывает "--More--" и защищён общим таймаутом
        asio::awaitable<bool> wait_for_any_prompt(
                tcp::socket& socket,
                asio_script_channel& channel,
                telnetpp::session& session,
                std::string& session_buffer,
                const std::vector<std::string>& target_prompts,
                std::chrono::milliseconds timeout_ms)
        {
            std::vector<char> recv_buffer(4096);
            asio::steady_timer timer(executor_);
            timer.expires_after(timeout_ms);

            while (!contains_any_prompt_icase(session_buffer, target_prompts)) {
                boost::system::error_code ec;

                auto race_result = co_await (
                        socket.async_read_some(asio::buffer(recv_buffer), asio::redirect_error(asio::use_awaitable, ec)) ||
                        timer.async_wait(asio::use_awaitable)
                );

                if (race_result.index() == 1 || ec) {
                    co_return false; // Таймаут или обрыв сокета
                }

                size_t bytes_received = std::get<0>(race_result);
                if (bytes_received > 0 && channel.telnet_parser_callback) {
                    telnetpp::bytes view{ reinterpret_cast<uint8_t*>(recv_buffer.data()), bytes_received };
                    channel.telnet_parser_callback(view);

                    // Если свитч остановился на --More--, листаем дальше и продлеваем таймер
                    if (handle_pager_if_present(session_buffer, session)) {
                        timer.expires_after(timeout_ms);
                    }
                }
            }
            co_return true;
        }

        /**
         * @brief Скриптовый CLI-диалог с поддержкой прогрева FDB и нестандартных портов
         */
        // Полный сценарий работы с одним коммутатором в рамках одной Telnet-сессии:
        // TCP-подключение -> авторизация (Login/Password) -> [опционально: ping прогрева + пауза] -> выполнение целевой CLI-команды и сбор вывода
        asio::awaitable<DeviceResult> execute_single_telnet(DeviceTask task, std::chrono::milliseconds timeout_ms) {
            DeviceResult result;
            result.ip = task.ip;
            result.port = task.port;

            tcp::socket socket(executor_);
            asio_script_channel script_channel{ socket };
            telnetpp::session session(script_channel);



            std::string session_buffer;
            session.async_read([&](telnetpp::bytes data) {
                if (!data.empty()) {
                    session_buffer.append(reinterpret_cast<char const*>(data.data()), data.size());
                }
            });

            try {
                tcp::resolver resolver(executor_);
                auto endpoints = co_await resolver.async_resolve(task.ip, std::to_string(task.port), asio::use_awaitable);

                // Подключение с таймаутом
                asio::steady_timer connect_timer(executor_);
                connect_timer.expires_after(timeout_ms);
                auto conn_race = co_await (
                        asio::async_connect(socket, endpoints, asio::use_awaitable) ||
                        connect_timer.async_wait(asio::use_awaitable)
                );
                if (conn_race.index() == 1) {
                    result.error_message = "Telnet connect timed out";
                    co_return result;
                }

                // Активируем Echo ПОСЛЕ того, как TCP-сокет открыт, чтобы IAC DO ECHO реально ушёл в сеть!
                telnetpp::options::echo::client echo_client{ session };
                session.install(echo_client);
                echo_client.activate();

                auto auth_step_timeout = std::chrono::milliseconds(5000);
                const std::vector<std::string> login_prompts = { "user name>", "user", "login", "name:" };
                const std::vector<std::string> pass_prompts  = { "user password>", "word", "pass" };
                const std::vector<std::string> cli_prompts   = { "#", ">" };

                // 1. Ждём приглашения логина
                if (!co_await wait_for_any_prompt(socket, script_channel, session, session_buffer, login_prompts, auth_step_timeout)) {
                    send_text(session, "\r");
                    if (!co_await wait_for_any_prompt(socket, script_channel, session, session_buffer, login_prompts, auth_step_timeout)) {
                        result.error_message = "Timeout waiting for Login prompt";
                        co_return result;
                    }
                }

                // Отправляем логин (для TFortis критично использовать одиночный \r, чтобы \n не ушёл как пустой пароль)
                send_text(session, task.username + "\r");

                // 2. Ждём приглашения пароля
                if (!co_await wait_for_any_prompt(socket, script_channel, session, session_buffer, pass_prompts, auth_step_timeout)) {
                    std::string tail = session_buffer.size() > 60 ? session_buffer.substr(session_buffer.size() - 60) : session_buffer;
                    for (char& c : tail) if (c == '\r' || c == '\n') c = ' ';
                    result.error_message = "Timeout waiting for Password prompt. Tail: [" + tail + "]";
                    co_return result;
                }

                send_text(session, task.password + "\r");
                session_buffer.clear();

                // 3. Ждём оперативного приглашения CLI ("#" или ">")
                if (!co_await wait_for_any_prompt(socket, script_channel, session, session_buffer, cli_prompts, auth_step_timeout)) {
                    result.error_message = "Authentication failed or CLI prompt not found";
                    co_return result;
                }
                session_buffer.clear();



                // 4. Если задана команда прогрева FDB (Фаза 2: ping + пауза в той же сессии)
                if (task.warmup_command.has_value() && !task.warmup_command->empty()) {
                    //std::string ping_payload = *task.warmup_command + "\r\n";
                    send_text(session, *task.warmup_command + "\r\n");
                    co_await wait_for_any_prompt(socket, script_channel, session, session_buffer, cli_prompts, timeout_ms);
                    session_buffer.clear();

                    asio::steady_timer warmup_timer(executor_);
                    warmup_timer.expires_after(task.warmup_delay);
                    boost::system::error_code ec;
                    co_await warmup_timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
                }

                // 5. Отправляем целевую команду и ждём возврата любого CLI-промпта ("#" или ">")
                //std::string cmd_payload = task.command + "\r\n";
                // Разбиваем task.command по ';' и выполняем по очереди
                std::string full_output;
                size_t start = 0;
                while (start <= task.command.size()) {
                    size_t sep = task.command.find(';', start);
                    std::string single_cmd = (sep == std::string::npos)
                                             ? task.command.substr(start)
                                             : task.command.substr(start, sep - start);

                    size_t first = single_cmd.find_first_not_of(" \t\r\n");
                    if (first != std::string::npos) {
                        size_t last = single_cmd.find_last_not_of(" \t\r\n");
                        single_cmd = single_cmd.substr(first, last - first + 1);

                        session_buffer.clear();
                        send_text(session, single_cmd + "\r\n");

                        if (!co_await wait_for_any_prompt(socket, script_channel, session, session_buffer, cli_prompts, std::chrono::milliseconds(8000))) {
                            result.error_message = "Timeout waiting for output of: " + single_cmd;
                            co_return result;
                        }
                        full_output += session_buffer;
                        full_output += "\n";
                    }

                    if (sep == std::string::npos) break;
                    start = sep + 1;
                }

                result.success = true;
                result.raw_output = std::move(full_output);
                co_return result;

            }
            catch (const std::exception& e) {
                result.error_message = std::string("Telnet exception: ") + e.what();
            }

            boost::system::error_code ec;
            socket.shutdown(tcp::socket::shutdown_both, ec);
            socket.close(ec);

            co_return result;
        }

        // Вспомогательная отправка строки в сессию telnetpp (совместимо со std::span / gsl::span)
        static void send_text(telnetpp::session& session, std::string_view text) {
            const auto* ptr = reinterpret_cast<const unsigned char*>(text.data());
            session.write(telnetpp::bytes(ptr, text.size()));
        }


    public:
        explicit TelnetWorkerPool(asio::any_io_executor exec) : executor_(exec) {}

        // Диспетчер параллельного Telnet-опроса: помещает все задачи в закрытый канал Asio, запускает пул из
        // max_concurrent_workers корутин-воркеров для одновременного опроса коммутаторов и ждёт завершения всей группы
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
                        DeviceResult res = co_await execute_single_telnet(std::move(task), timeout_ms);
                        shared_results.push_back(std::move(res));
                    }
                } catch (...) {}
                co_return;
            };
            auto remaining_workers = std::make_shared<size_t>(effective_workers);
            auto wait_timer = std::make_shared<asio::steady_timer>(executor_);
            wait_timer->expires_at(std::chrono::steady_clock::time_point::max());
            for (size_t i = 0; i < effective_workers; ++i) {
                asio::co_spawn(executor_, [&worker, &total_results, remaining_workers, wait_timer] () -> asio::awaitable<void> {
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

#endif //TEST_TELNET_SSH_SOLUTION_TELNETWORKERPOOL_H
