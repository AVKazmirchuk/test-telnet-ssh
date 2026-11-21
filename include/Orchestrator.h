//
// Created by Alexander on 20.11.2026.
//

#ifndef TEST_TELNET_SSH_SOLUTION_ORCHESTRATOR_H
#define TEST_TELNET_SSH_SOLUTION_ORCHESTRATOR_H

#pragma once
#include "DeviceWorkerTypes.h"
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

namespace net_discovery {

    namespace asio = boost::asio;

// ============================================================================
// 1. Вспомогательные структуры данных для конвейера разведки
// ============================================================================

    struct SubnetTarget {
        uint32_t net_ip = 0;       // Адрес сети в host byte order (например, 0x0A0A0A00 для 10.10.10.0)
        uint32_t mask = 0;         // Маска сети в host byte order (например, 0xFFFFFF00 для /24)
        std::string source_info;   // Откуда узнали о подсети ("LocalAdapter", "Gateway: 10.10.1.1")
    };

    struct DiscoveredHost {
        std::string ip;
        std::vector<uint16_t> open_ports; // Например: {22}, {23} или {22, 23}
    };

    // Результат сканирования подсети (исходная подсеть, список живых хостов с открытыми портами и свободный IP-маяк)
    struct SubnetScanResult {
        SubnetTarget subnet;
        std::vector<DiscoveredHost> alive_hosts;
        // Первый свободный IP-адрес в подсети (маяк для провокации ARP); std::nullopt, если все адреса заняты
        std::optional<std::string> free_beacon_ip;
    };

// Результат разбора конфигурации одного устройства (выход парсера в Фазе 1)
    struct ParsedDeviceConfig {
        std::string ip;
        uint64_t base_mac = 0;
        std::string vendor_profile;          // "cisco_ios", "huawei_vrp", "eltex", ...
        bool is_l3_router = false;           // Маркер L3-границы (2+ подсети / routing / routed-порты)
        std::vector<SubnetTarget> connected_subnets; // Новые локальные подсети (Connected / SVI)
        std::vector<std::string> next_hop_ips;       // Шлюзы из таблицы маршрутизации (для перехода за L3)
    };

// Запись FDB (выход парсера в Фазе 2)
    struct ParsedFdbEntry {
        uint16_t vlan_tag = 0;
        std::string port_name;
        uint64_t mac = 0;
    };

// ============================================================================
// 2. Интерфейсы внешних зависимостей (Сканер и Парсер вендоров)
// ============================================================================

    class ISubnetScanner {
    public:
        virtual ~ISubnetScanner() = default;
        virtual asio::awaitable<SubnetScanResult> scan_subnet(
                const SubnetTarget& subnet,
                const std::vector<uint16_t>& target_ports,
                std::chrono::milliseconds timeout_ms,
                size_t max_concurrent_workers) = 0;
    };

    class IDeviceOutputParser {
    public:
        virtual ~IDeviceOutputParser() = default;

        // Разбор конфигурации и интерфейсов (Фаза 1)
        virtual std::optional<ParsedDeviceConfig> parse_config_output(
                const std::string& ip,
                const std::string& raw_output) = 0;

        // Генерация вендор-зависимых команд для Фазы 2 (пинг маяка + чтение FDB)
        virtual std::string build_ping_command(const std::string& vendor, const std::string& target_ip) const = 0;
        virtual std::string build_fdb_command(const std::string& vendor) const = 0;

        // Разбор MAC-таблицы (Фаза 2)
        virtual std::vector<ParsedFdbEntry> parse_fdb_output(
                const std::string& vendor,
                const std::string& raw_output) = 0;
    };

    struct Credentials {
        std::string username;
        std::string password;
    };

// ============================================================================
// 3. Главный класс Оркестратора (Асинхронный конвейер BFS-разведки)
// ============================================================================

    class Orchestrator {
    private:
        asio::any_io_executor executor_;
        std::shared_ptr<ISubnetScanner> scanner_;
        std::shared_ptr<IDeviceWorkerPool> ssh_pool_;
        std::shared_ptr<IDeviceWorkerPool> telnet_pool_;
        std::shared_ptr<IDeviceOutputParser> parser_;
        Credentials creds_;

        // Защита от повторного сканирования и зацикливания (ключ = (uint64_t(net_ip) << 32) | mask)
        std::unordered_set<uint64_t> visited_subnets_;
        std::unordered_set<std::string> visited_ips_;

        // Кэш разведанных устройств по IP
        std::unordered_map<std::string, ParsedDeviceConfig> discovered_devices_;
        // Белый список собственных MAC-адресов всех найденных концентраторов
        std::unordered_set<uint64_t> concentrator_white_macs_;

        // Упаковывает 32-битный адрес сети и 32-битную маску в единый 64-битный ключ (старшие 32 бита — сеть, младшие — маска)
        // для мгновенного поиска уже посещённых подсетей в std::unordered_set
        static uint64_t make_subnet_key(const SubnetTarget& s) noexcept {
            return (static_cast<uint64_t>(s.net_ip) << 32) | static_cast<uint64_t>(s.mask);
        }

        // Разделение найденных хостов на задачи для SSH (приоритет) и Telnet
        // Распределяет найденные живые хосты между пулами SSH (приоритет) и Telnet,
        // отсекая уже опрошенные IP (visited_ips_) и подставляя учётные данные для подключения
        void dispatch_tasks_by_protocol(
                const std::vector<DiscoveredHost>& hosts,
                const std::string& command,
                std::vector<DeviceTask>& out_ssh_tasks,
                std::vector<DeviceTask>& out_telnet_tasks)
        {
            for (const auto& host : hosts) {
                if (visited_ips_.contains(host.ip)) {
                    continue;
                }
                visited_ips_.insert(host.ip);

                bool has_ssh = false;
                uint16_t ssh_port = 22;
                uint16_t telnet_port = 23;

                for (uint16_t p : host.open_ports) {
                    if (p == 22 || p == 2222) { has_ssh = true; ssh_port = p; }
                    if (p == 23) { telnet_port = p; }
                }

                DeviceTask task;
                task.ip = host.ip;
                task.username = creds_.username;
                task.password = creds_.password;
                task.command = command;

                if (has_ssh) {
                    task.port = ssh_port;
                    out_ssh_tasks.push_back(std::move(task));
                } else if (!host.open_ports.empty()) {
                    task.port = telnet_port;
                    out_telnet_tasks.push_back(std::move(task));
                }
            }
        }

    public:
        Orchestrator(
                asio::any_io_executor exec,
                std::shared_ptr<ISubnetScanner> scanner,
                std::shared_ptr<IDeviceWorkerPool> ssh_pool,
                std::shared_ptr<IDeviceWorkerPool> telnet_pool,
                std::shared_ptr<IDeviceOutputParser> parser,
                Credentials creds)
                : executor_(exec)
                , scanner_(std::move(scanner))
                , ssh_pool_(std::move(ssh_pool))
                , telnet_pool_(std::move(telnet_pool))
                , parser_(std::move(parser))
                , creds_(std::move(creds))
        {}

        /**
         * @brief Запуск полного цикла разведки сети (Фаза 1: BFS по L2-доменам -> Фаза 2: Сбор FDB)
         */
        asio::awaitable<void> run_discovery(const std::vector<SubnetTarget>& initial_subnets) {

            // Очередь (FIFO) подсетей, ожидающих сканирования в рамках алгоритма поиска в ширину (BFS);
            // пополняется по мере обнаружения новых локальных подсетей на L3-маршрутизаторах
            std::deque<SubnetTarget> subnet_queue;

            for (const auto& s : initial_subnets) {
                uint64_t key = make_subnet_key(s);
                if (visited_subnets_.insert(key).second) {
                    subnet_queue.push_back(s);
                }
            }

            // Список замкнутых L2-доменов для последующего прогрева и снятия FDB
            // Сформированный L2-домен с зафиксированными границами: содержит саму подсеть, свободный IP-маяк
            // для провокации ARP и полный список всех обнаруженных в этом сегменте коммутаторов для Фазы 2
            struct L2DomainBatch {
                SubnetTarget subnet;
                std::optional<std::string> beacon_ip;
                std::vector<std::string> switch_ips;
            };
            std::vector<L2DomainBatch> ready_domains;

            // ====================================================================
            // ФАЗА 1: Поиск в ширину (BFS) по подсетям и определение L3-границ
            // ====================================================================
            while (!subnet_queue.empty()) {
                SubnetTarget current_subnet = subnet_queue.front();
                subnet_queue.pop_front();

                // 1. Сканируем текущую подсеть на открытые порты 22 и 23
                SubnetScanResult scan_res = co_await scanner_->scan_subnet(
                        current_subnet, {22, 23}, std::chrono::milliseconds(800), 128);

                std::cout << "[TcpScanner] Подсеть " << asio::ip::address_v4(current_subnet.net_ip).to_string()
                          << ": найдено живых хостов (22/23) = " << scan_res.alive_hosts.size()
                          << ", маяк = " << scan_res.free_beacon_ip.value_or("нет") << "\n";

                if (scan_res.alive_hosts.empty()) {
                    continue;
                }

                // 2. Формируем параллельные задачи для SSH и Telnet пулов
                std::vector<DeviceTask> ssh_tasks;
                std::vector<DeviceTask> telnet_tasks;
                // Универсальная стартовая команда разведки (или определяется по баннеру)
                dispatch_tasks_by_protocol(scan_res.alive_hosts, "show running-config", ssh_tasks, telnet_tasks);

                // 3. Параллельно опрашиваем SSH и Telnet устройства в текущем сегменте
                std::vector<DeviceResult> all_results;
                if (!ssh_tasks.empty()) {
                    auto ssh_res = co_await ssh_pool_->execute_parallel_commands(
                            ssh_tasks, std::chrono::milliseconds(8000), 32);
                    all_results.insert(all_results.end(),
                                       std::make_move_iterator(ssh_res.begin()), std::make_move_iterator(ssh_res.end()));
                }

                if (!telnet_tasks.empty()) {
                    auto tel_res = co_await telnet_pool_->execute_parallel_commands(
                            telnet_tasks, std::chrono::milliseconds(10000), 32);
                    all_results.insert(all_results.end(),
                                       std::make_move_iterator(tel_res.begin()), std::make_move_iterator(tel_res.end()));
                }

                for (const auto& res : all_results) {
                    std::cout << "  [Фаза 1] " << res.ip << ":" << res.port
                              << " -> " << (res.success ? "OK (байт: " + std::to_string(res.raw_output.size()) + ")"
                                                        : "ОШИБКА: " + res.error_message) << "\n";
                }

                // 4. Разбираем ответы, пополняем белый список MAC и расширяем очередь подсетей
                L2DomainBatch domain_batch;
                domain_batch.subnet = current_subnet;
                domain_batch.beacon_ip = scan_res.free_beacon_ip;

                for (const auto& res : all_results) {
                    if (!res.success) {
                        continue; // Неответившие или чужие хосты пропускаем
                    }

                    auto parsed_opt = parser_->parse_config_output(res.ip, res.raw_output);
                    if (!parsed_opt.has_value()) {
                        continue;
                    }

                    ParsedDeviceConfig& cfg = *parsed_opt;
                    if (cfg.base_mac != 0) {
                        concentrator_white_macs_.insert(cfg.base_mac);
                    }

                    domain_batch.switch_ips.push_back(cfg.ip);
                    discovered_devices_[cfg.ip] = cfg;

                    // Если устройство является L3-границей (маршрутизатором),
                    // добавляем его новые локальные подсети в очередь BFS
                    // Если парсер определил устройство как L3-маршрутизатор (извлек из его конфига 2+ подсети или маршруты),
                    // перебираем все найденные на его интерфейсах локальные подсети (cfg.connected_subnets) и ставим ещё не посещённые в очередь BFS
                    if (cfg.is_l3_router) {
                        for (const auto& next_sub : cfg.connected_subnets) {

                                    uint64_t key = make_subnet_key(next_sub);
                            if (visited_subnets_.insert(key).second) {
                                std::cout << "      + Новая подсеть в очередь BFS: "
                                          << asio::ip::address_v4(next_sub.net_ip).to_string()
                                          << " / " << asio::ip::address_v4(next_sub.mask).to_string() << "\n";

                                subnet_queue.push_back(next_sub);
                            }
                        }
                    }

                    std::cout << "    -> Вендор: " << cfg.vendor_profile
                              << ", BaseMAC: 0x" << std::hex << cfg.base_mac << std::dec
                              << ", L3-маршрутизатор: " << (cfg.is_l3_router ? "ДА" : "НЕТ")
                              << ", новых подсетей: " << cfg.connected_subnets.size() << "\n";

                }

                if (!domain_batch.switch_ips.empty()) {
                    ready_domains.push_back(std::move(domain_batch));
                }
            }
// ====================================================================
// ФАЗА 2: Атомарный прогрев (ARP-маяк) и снятие FDB по L2-доменам
// ====================================================================
            for (const auto& domain : ready_domains) {
                std::vector<DeviceTask> fdb_ssh_tasks;
                std::vector<DeviceTask> fdb_telnet_tasks;
                for (const auto& sw_ip : domain.switch_ips) {
                    const auto& dev = discovered_devices_.at(sw_ip);
                    DeviceTask task;
                    task.ip = dev.ip;
                    task.username = creds_.username;
                    task.password = creds_.password;
                    task.command = parser_->build_fdb_command(dev.vendor_profile);
// Если в домене есть свободный IP-маяк — включаем шаг прогрева FDB
                    if (domain.beacon_ip.has_value()) {
                        task.warmup_command = parser_->build_ping_command(dev.vendor_profile, *domain.beacon_ip);
                        task.warmup_delay = std::chrono::milliseconds(2500);
                    }
// Распределяем по тому же протоколу, по которому устройство ответило в Фазе 1
                    fdb_ssh_tasks.push_back(std::move(task));
                }
// Запускаем массовый параллельный прогрев + чтение FDB в рамках L2-домена
                auto fdb_results = co_await ssh_pool_->execute_parallel_commands(
                        fdb_ssh_tasks, std::chrono::milliseconds(12000), 32);
// Контроль целостности (Сценарий 2: проверка, что все ожидаемые узлы ответили)
                size_t success_count = 0;
                for (const auto& r : fdb_results) {
                    if (r.success) ++success_count;
                }

                // 1. Логируем итог опроса домена (до проверки на неполноту)
                std::cout << "[Фаза 2] L2-домен " << asio::ip::address_v4(domain.subnet.net_ip).to_string()
                          << ": ответило " << success_count << "/" << domain.switch_ips.size() << " устройств.\n";

                if (success_count < domain.switch_ips.size()) {
// Зафиксирован обрыв части узлов во время снятия FDB:
// помечаем снимок неполным / отправляем на повторную проверку, чтобы не исказить линки
                    continue;
                }
// Фильтруем полученные FDB-таблицы строго по белому списку concentrator_white_macs_
// и записываем в накопительную таблицу ConcentratorsFdbTable (с текущим timestamp)
                size_t valid_fdb_links = 0;
                for (const auto& r : fdb_results) {
                    const auto& dev = discovered_devices_.at(r.ip);
                    auto entries = parser_->parse_fdb_output(dev.vendor_profile, r.raw_output);
                    for (const auto& entry : entries) {
                        if (concentrator_white_macs_.contains(entry.mac)) {
// Валидная связь: dev.ip (entry.port_name, entry.vlan_tag) -> entry.mac
                            ++valid_fdb_links;
                            // Валидная связь: dev.ip (entry.port_name, entry.vlan_tag) -> entry.mac

                        }
                    }
                }
                // 2. Логируем количество отфильтрованных связей между коммутаторами
                std::cout << "  -> Найдено валидных FDB-записей между коммутаторами: " << valid_fdb_links << "\n";
            }
            co_return;
        }

        std::function<void(const std::string&)> logger_ = [](const std::string& msg) {
            std::cout << msg << "\n";
        };
    };
} // namespace net_discovery

#endif //TEST_TELNET_SSH_SOLUTION_ORCHESTRATOR_H
