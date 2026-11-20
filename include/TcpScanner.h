//
// Created by Alexander on 20.11.2026.
//

#ifndef TEST_TELNET_SSH_SOLUTION_TCPSCANNER_H
#define TEST_TELNET_SSH_SOLUTION_TCPSCANNER_H

#pragma once
#include "Orchestrator.h"
#include <algorithm>
#include <bit>
#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace net_discovery {

    namespace asio = boost::asio;
    using namespace asio::experimental::awaitable_operators;

    enum class PortStatus {
        Open,       // Порт открыт (SYN-ACK)
        Closed,     // Порт закрыт (RST — хост живой, служба не запущена)
        Filtered    // Таймаут (хост молчит / свободный IP / Firewall)
    };

    class TcpScanner : public ISubnetScanner {
    private:
        asio::any_io_executor executor_;

    public:
        explicit TcpScanner(asio::any_io_executor exec) : executor_(exec) {}

        /**
         * @brief Атомарная проверка одного TCP-порта (гонка connect || timer)
         */
        asio::awaitable<PortStatus> scan_single_port(
                const std::string& ip_address,
                uint16_t port,
                std::chrono::milliseconds timeout_ms)
        {
            asio::ip::tcp::socket socket(executor_);
            asio::steady_timer timer(executor_);
            boost::system::error_code connect_ec = asio::error::would_block;

            try {
                asio::ip::tcp::endpoint target_ep(asio::ip::make_address(ip_address), port);
                timer.expires_after(timeout_ms);

                auto race_result = co_await (
                        socket.async_connect(target_ep, asio::redirect_error(asio::use_awaitable, connect_ec)) ||
                        timer.async_wait(asio::use_awaitable)
                );

                boost::system::error_code close_ec;
                socket.close(close_ec);

                if (race_result.index() == 0) {
                    if (!connect_ec) {
                        co_return PortStatus::Open;
                    }
                    co_return PortStatus::Closed;
                }
                co_return PortStatus::Filtered;
            }
            catch (const std::exception&) {
                co_return PortStatus::Closed;
            }
        }

        /**
         * @brief Реализация ISubnetScanner: сканирует подсеть, возвращает живые хосты и свободный IP-маяк
         */
        asio::awaitable<SubnetScanResult> scan_subnet(
                const SubnetTarget& subnet,
                const std::vector<uint16_t>& target_ports,
                std::chrono::milliseconds timeout_ms,
                size_t max_concurrent_workers) override
        {
            SubnetScanResult result;
            result.subnet = subnet;

            if (target_ports.empty() || subnet.mask == 0) {
                co_return result;
            }

            // Вычисляем длину префикса из маски (например, 0xFFFFFF00 -> 24)
            uint16_t prefix_len = static_cast<uint16_t>(std::popcount(subnet.mask));

            // Защита от случайного сканирования гигантских сетей (/8, /16)
            if (prefix_len < 20 || prefix_len > 30) {
                co_return result;
            }

            asio::ip::address_v4 net_addr(subnet.net_ip);
            asio::ip::network_v4 network(net_addr, prefix_len);

            std::vector<std::string> ip_addresses;
            for (const auto& ip : network.hosts()) {
                ip_addresses.push_back(ip.to_string());
            }

            if (ip_addresses.empty()) {
                co_return result;
            }

            using ScanTask = std::pair<std::string, uint16_t>;
            size_t total_tasks = ip_addresses.size() * target_ports.size();
            size_t effective_workers = std::min(max_concurrent_workers, total_tasks);

            asio::experimental::channel<void(boost::system::error_code, ScanTask)> task_channel(executor_, total_tasks);

            for (const auto& ip : ip_addresses) {
                for (uint16_t port : target_ports) {
                    task_channel.try_send(boost::system::error_code{}, ScanTask{ip, port});
                }
            }
            task_channel.close();

            // Агрегация результатов в едином потоке io_context (мьютексы не требуются)
            std::unordered_map<std::string, std::vector<uint16_t>> open_ports_by_ip;
            std::unordered_set<std::string> responsive_ips; // Хосты, ответившие Open или Closed (RST)

            auto worker = [&]() -> asio::awaitable<void> {
                try {
                    for (;;) {
                        boost::system::error_code ec;
                        ScanTask task = co_await task_channel.async_receive(asio::redirect_error(asio::use_awaitable, ec));

                        if (ec == asio::experimental::channel_errc::channel_closed) {
                            break;
                        }

                        PortStatus status = co_await scan_single_port(task.first, task.second, timeout_ms);

                        if (status == PortStatus::Open) {
                            open_ports_by_ip[task.first].push_back(task.second);
                            responsive_ips.insert(task.first);
                        } else if (status == PortStatus::Closed) {
                            // Хост прислал RST — значит IP занят живым устройством
                            responsive_ips.insert(task.first);
                        }
                    }
                } catch (...) {}
                co_return;
            };

            auto remaining_workers = std::make_shared<size_t>(effective_workers);
            auto wait_timer = std::make_shared<asio::steady_timer>(executor_);
            wait_timer->expires_at(std::chrono::steady_clock::time_point::max());

            for (size_t i = 0; i < effective_workers; ++i) {
                asio::co_spawn(executor_, [&worker, remaining_workers, wait_timer]() -> asio::awaitable<void> {
                    co_await worker();
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

            // 1. Формируем список живых хостов с открытыми портами управления
            result.alive_hosts.reserve(open_ports_by_ip.size());
            for (auto& [ip, ports] : open_ports_by_ip) {
                std::sort(ports.begin(), ports.end());
                result.alive_hosts.push_back(DiscoveredHost{ ip, std::move(ports) });
            }

            // 2. Выбираем первый «тихий» IP из подсети в качестве маяка для провокации Broadcast ARP
            for (const auto& candidate_ip : ip_addresses) {
                if (!responsive_ips.contains(candidate_ip)) {
                    result.free_beacon_ip = candidate_ip;
                    break;
                }
            }

            co_return result;
        }
    };

} // namespace net_discovery

#endif //TEST_TELNET_SSH_SOLUTION_TCPSCANNER_H
