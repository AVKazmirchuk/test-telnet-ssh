//
// Created by Alexander on 20.11.2026.
//

#ifndef TEST_TELNET_SSH_SOLUTION_DEVICEWORKERTYPES_H
#define TEST_TELNET_SSH_SOLUTION_DEVICEWORKERTYPES_H

#pragma once
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace net_discovery {

    namespace asio = boost::asio;

// Унифицированная задача для любого протокола (SSH или Telnet)
    struct DeviceTask {
        std::string ip;
        uint16_t port = 22;
        std::string username;
        std::string password;

        // Основная команда (или команда снятия FDB в Фазе 2)
        std::string command;

        // Опциональный шаг прогрева FDB (используется в Фазе 2):
        // Если задан — воркер сначала выполняет warmup_command (например, ping свободного IP),
        // затем асинхронно ждёт warmup_delay и только потом выполняет основную command.
        //Команда прогрева (ping)
        std::optional<std::string> warmup_command = std::nullopt;
        //Время ожидания после прогрева до выполнения основной команды
        std::chrono::milliseconds warmup_delay{2500};
    };

// Унифицированный результат опроса устройства
    struct DeviceResult {
        std::string ip;
        uint16_t port = 0;
        bool success = false;
        std::string raw_output;
        std::string error_message;
    };

// Единый интерфейс пула воркеров для Orchestrator
    class IDeviceWorkerPool {
    public:
        virtual ~IDeviceWorkerPool() = default;

        //Запустить параллельное выполнение команд
        virtual asio::awaitable<std::vector<DeviceResult>> execute_parallel_commands(
                const std::vector<DeviceTask>& tasks,
                std::chrono::milliseconds timeout_ms,
                size_t max_concurrent_workers) = 0;
    };

} // namespace net_discovery

#endif //TEST_TELNET_SSH_SOLUTION_DEVICEWORKERTYPES_H
