//
// Created by Alexander on 20.11.2026.
//

#ifndef TEST_TELNET_SSH_SOLUTION_VENDORPARSER_H
#define TEST_TELNET_SSH_SOLUTION_VENDORPARSER_H

#pragma once
#include "Orchestrator.h"
#include <algorithm>
#include <boost/asio/ip/address_v4.hpp>
#include <cctype>
#include <cstdint>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace net_discovery {

// Профиль CLI-команд для конкретного производителя (легко сериализуется в JSON для Qt GUI)
    struct VendorProfile {
        std::string id;                 // "eltex_mes", "eltex_esr", "snr", "tfortis"
        std::string display_name;
        std::string config_command;     // Команда чтения конфига и собственного MAC (Фаза 1)
        std::string ping_template;      // Шаблон пинга с плейсхолдером {IP} (Фаза 2)
        std::string fdb_command;        // Команда чтения MAC-таблицы (Фаза 2)
    };

    // Универсальный парсер вывода CLI и менеджер профилей вендоров (Eltex, TFortis, SNR, UserGate).
    // Автоматически распознаёт производителя по тексту ответа, формирует вендор-зависимые команды
    // и преобразует сырые конфигурации и таблицы FDB/ARP в единые структуры данных Orchestrator
    class UniversalDeviceParser : public IDeviceOutputParser {
    private:

        // Словарь профилей оборудования: ключ — строковый ID вендора ("eltex_mes", "tfortis", "usergate" и т.д.),
        // значение — структура VendorProfile с набором CLI-команд для этого производителя
        std::unordered_map<std::string, VendorProfile> profiles_;

        //Перевести все буквы в нижний регистр
        static std::string to_lower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        // Парсинг MAC-адреса из любого формата (aa:bb:cc:dd:ee:ff, aa-bb-cc-dd-ee-ff, aabb.ccdd.eeff)
        static std::optional<uint64_t> parse_mac_to_u64(const std::string& raw_mac) {
            std::string hex_only;
            hex_only.reserve(12);
            for (char c : raw_mac) {
                if (std::isxdigit(static_cast<unsigned char>(c))) {
                    hex_only.push_back(c);
                }
            }
            if (hex_only.size() != 12) {
                return std::nullopt;
            }
            try {
                return std::stoull(hex_only, nullptr, 16);
            } catch (...) {
                return std::nullopt;
            }
        }

        // Преобразование длины префикса (например, 24) в 32-битную маску (0xFFFFFF00)
        static uint32_t prefix_to_mask(uint32_t prefix) noexcept {
            if (prefix == 0) return 0;
            if (prefix >= 32) return 0xFFFFFFFFu;
            return 0xFFFFFFFFu << (32 - prefix);
        }

        // Определение вендора по характерным маркерам в ответе CLI
        std::string detect_vendor(const std::string& raw_output) const {
            std::string lower = to_lower(raw_output);

            if (lower.find("usergate") != std::string::npos || lower.find("ugos") != std::string::npos) {
                return "usergate";
            }
            if (lower.find("tfortis") != std::string::npos || lower.find("psw-") != std::string::npos ||
                lower.find("fort-telecom") != std::string::npos) {
                return "tfortis";
            }
            if (lower.find("snr-") != std::string::npos || lower.find("ethernet1/0/") != std::string::npos) {
                return "snr";
            }
            if (lower.find("esr-") != std::string::npos || lower.find("eltex") != std::string::npos && lower.find("security gateway") != std::string::npos) {
                return "eltex_esr";
            }
            if (lower.find("cisco") != std::string::npos || lower.find("base ethernet mac address") != std::string::npos) {
                return "cisco";
            }
            // По умолчанию для вашего парка (90%) — коммутаторы Eltex MES
            return "mes";
        }

    public:
        UniversalDeviceParser() {
            // 1. Eltex MES (коммутаторы доступа и агрегации — 90% парка)
            profiles_["mes"] = VendorProfile{
                    "eltex_mes",
                    "Eltex MES Series",
                    "show system; show running-config",
                    "ping {IP} count 1",
                    "show mac address-table"
            };

            // 2. Eltex ESR (маршрутизаторы / межсетевые экраны)
            profiles_["eltex_esr"] = VendorProfile{
                    "eltex_esr",
                    "Eltex ESR Router",
                    "show version; show running-config",
                    "ping {IP} packets 1",
                    "show mac address-table"
            };

            // 3. TFortis PSW (уличные промышленные коммутаторы — Telnet)
            profiles_["tfortis"] = VendorProfile{
                    "tfortis",
                    "TFortis PSW",
                    "show all",
                    "ping {IP}",
                    "show fdb"
            };

            // 4. SNR (коммутаторы)
            profiles_["snr"] = VendorProfile{
                    "snr",
                    "SNR Switch",
                    "show version; show running-config",
                    "ping {IP} count 1",
                    "show mac-address-table"
            };

            // 5. UserGate NGFW / UTM (ядро маршрутизации и межсетевой экран)
            profiles_["usergate"] = VendorProfile{
                    "usergate",
                    "UserGate NGFW",
                    "show network interface",
                    "ping {IP} count 1",
                    "show network arp"
            };

            profiles_["cisco"] = VendorProfile{
                    "cisco",
                    "Cisco IOS",
                    "terminal length 0; show version; show running-config",
                    "ping {IP} repeat 1",
                    "show mac address-table"
            };
        }

        // Позволяет добавлять или переопределять профили вендоров из JSON / Qt GUI
        void register_profile(VendorProfile profile) {
            profiles_[profile.id] = std::move(profile);
        }

        /**
         * @brief Фаза 1: Разбор конфигурации и системной информации устройства
         */
        std::optional<ParsedDeviceConfig> parse_config_output(
                const std::string& ip,
                const std::string& raw_output) override
        {
            if (raw_output.empty()) {
                return std::nullopt;
            }

            ParsedDeviceConfig cfg;
            cfg.ip = ip;
            cfg.vendor_profile = detect_vendor(raw_output);

            // 1. Поиск собственного Base MAC (поддержка форматов xx:xx:xx:xx:xx:xx, xx-xx-..., xxxx.xxxx.xxxx)
            // Ищем строку рядом со словами "System MAC", "Base MAC", "MAC Address" или первый валидный MAC в шапке
            std::regex mac_regex(R"(([0-9a-fA-F]{2}[:-]){5}[0-9a-fA-F]{2}|([0-9a-fA-F]{4}\.){2}[0-9a-fA-F]{4})");
            std::smatch mac_match;
            if (std::regex_search(raw_output, mac_match, mac_regex)) {
                if (auto parsed_mac = parse_mac_to_u64(mac_match.str(0))) {
                    cfg.base_mac = *parsed_mac;
                }
            }

            // 2. Поиск настроенных IP-адресов и масок (форматы: "ip address 10.10.10.1 255.255.255.0" и "10.10.10.1/24")
            std::unordered_set<uint64_t> unique_subnets;

            // Вариант А: IP + классическая маска ("ip address 10.10.10.1 255.255.255.0")
            std::regex ip_mask_regex(R"(ip\s+address\s+(\d+\.\d+\.\d+\.\d+)\s+(\d+\.\d+\.\d+\.\d+))", std::regex::icase);
            for (std::sregex_iterator it(raw_output.begin(), raw_output.end(), ip_mask_regex), end; it != end; ++it) {
                boost::system::error_code ec1, ec2;
                auto addr = asio::ip::make_address_v4((*it)[1].str(), ec1);
                auto mask = asio::ip::make_address_v4((*it)[2].str(), ec2);
                if (!ec1 && !ec2 && mask.to_uint() != 0) {
                    uint32_t net_u32 = addr.to_uint() & mask.to_uint();
                    uint64_t key = (static_cast<uint64_t>(net_u32) << 32) | mask.to_uint();
                    if (unique_subnets.insert(key).second) {
                        cfg.connected_subnets.push_back(SubnetTarget{ net_u32, mask.to_uint(), "Device: " + ip });
                    }
                }
            }

            // Вариант Б: IP + CIDR префикс ("ip address 10.10.10.1/24" — характерно для Eltex ESR и TFortis)
            // Поддерживает и "ip address 10.10.10.1/24", и "ip: 10.10.10.1/24", и "ipv4: 10.10.10.1/24"
            std::regex ip_cidr_regex(R"((?:ip\s+address|ipv4|ip)\s*:?\s*(\d+\.\d+\.\d+\.\d+)/(\d{1,2}))", std::regex::icase);
            for (std::sregex_iterator it(raw_output.begin(), raw_output.end(), ip_cidr_regex), end; it != end; ++it) {
                boost::system::error_code ec;
                auto addr = asio::ip::make_address_v4((*it)[1].str(), ec);
                uint32_t pref = static_cast<uint32_t>(std::stoul((*it)[2].str()));
                if (!ec && pref > 0 && pref <= 32) {
                    uint32_t mask_u32 = prefix_to_mask(pref);
                    uint32_t net_u32 = addr.to_uint() & mask_u32;
                    uint64_t key = (static_cast<uint64_t>(net_u32) << 32) | mask_u32;
                    if (unique_subnets.insert(key).second) {
                        cfg.connected_subnets.push_back(SubnetTarget{ net_u32, mask_u32, "Device: " + ip });
                    }
                }
            }

            // 3. Универсальные маркеры L3-маршрутизатора:
            // Более 1 локальной подсети ИЛИ наличие маршрутов (кроме дефолтного 0.0.0.0) / включённый ip routing
            // Поиск признаков реальной L3-маршрутизации: статические маршруты в конкретные подсети (исключая обычный
            // шлюз по умолчанию "ip route 0.0.0.0 0.0.0.0", который есть почти на каждом L2-свитче) или протоколы OSPF/BGP
            bool has_routing_entries = (raw_output.find("ip route ") != std::string::npos &&
                                        raw_output.find("ip route 0.0.0.0 0.0.0.0") == std::string::npos) ||
                                       (raw_output.find("router ospf") != std::string::npos) ||
                                       (raw_output.find("router bgp") != std::string::npos);

            cfg.is_l3_router = (cfg.connected_subnets.size() >= 2) || has_routing_entries;

            return cfg;
        }

        // Формирует готовую команду ping для провокации ARP: берёт шаблон из профиля вендора
        // (или стандартный "ping {IP} count 1", если вендор не найден) и подставляет целевой IP-адрес вместо "{IP}"
        std::string build_ping_command(const std::string& vendor, const std::string& target_ip) const override {
            auto it = profiles_.find(vendor);
            std::string tpl = (it != profiles_.end()) ? it->second.ping_template : "ping {IP} count 1";
            size_t pos = tpl.find("{IP}");
            if (pos != std::string::npos) {
                tpl.replace(pos, 4, target_ip);
            }
            return tpl;
        }

        // Возвращает вендор-зависимую команду чтения таблицы MAC-адресов (FDB) или ARP из профиля производителя
        // (если профиль не найден, возвращает стандартную команду "show mac address-table")
        std::string build_fdb_command(const std::string& vendor) const override {
            auto it = profiles_.find(vendor);
            return (it != profiles_.end()) ? it->second.fdb_command : "show mac address-table";
        }

        /**
         * @brief Фаза 2: Универсальный построчный парсер MAC-таблиц (Eltex / SNR / TFortis)
         */
        std::vector<ParsedFdbEntry> parse_fdb_output(
                const std::string& vendor,
                const std::string& raw_output) override
        {
            std::vector<ParsedFdbEntry> entries;
            std::istringstream stream(raw_output);
            std::string line;

            if (vendor == "usergate") {
                // Парсинг строк ARP UserGate: IP | MAC | Interface (например, "port1.10" или "eth0")
                std::regex ug_arp_regex(
                        R"((\d+\.\d+\.\d+\.\d+)\s+(([0-9a-fA-F]{2}[:-]){5}[0-9a-fA-F]{2})\s+(\S+))"
                );
                while (std::getline(stream, line)) {
                    std::smatch m;
                    if (std::regex_search(line, m, ug_arp_regex)) {
                        auto mac_opt = parse_mac_to_u64(m.str());
                        if (!mac_opt.has_value()) continue;

                        std::string iface_name = m.str();
                        // Если интерфейс вида "port1.10" или "vlan10", можно извлечь тег VLAN после точки
                        uint16_t vlan = 0;
                        size_t dot_pos = iface_name.find('.');
                        if (dot_pos != std::string::npos) {
                            try {
                                vlan = static_cast<uint16_t>(std::stoul(iface_name.substr(dot_pos + 1)));
                            } catch (...) {}
                        }
                        entries.push_back(ParsedFdbEntry{ vlan, iface_name, *mac_opt });
                    }
                }
                return entries;
            }

            // Типичные строки таблиц FDB у Eltex, SNR и TFortis:
            // Eltex MES: "  10    68:13:e2:da:31:40    gi1/0/24     dynamic"
            // SNR:       "  10    68-13-e2-da-31-40    DYNAMIC      Ethernet1/0/24"
            // TFortis:   "  10    68:13:e2:da:31:40    Port 8       Dynamic"
            std::regex row_regex(
                    R"(^\s*(\d{1,4})\s+(([0-9a-fA-F]{2}[:-]){5}[0-9a-fA-F]{2}|([0-9a-fA-F]{4}\.){2}[0-9a-fA-F]{4})\s+(\S+)(?:\s+(\S+))?)"
            );

            while (std::getline(stream, line)) {
                std::smatch m;
                if (std::regex_search(line, m, row_regex)) {
                    uint16_t vlan = static_cast<uint16_t>(std::stoul(m[1].str()));
                    auto mac_opt = parse_mac_to_u64(m[2].str());
                    if (!mac_opt.has_value()) {
                        continue;
                    }

                    // Определяем, в какой колонке лежит имя порта (у Eltex сразу после MAC, у SNR — после слова DYNAMIC)
                    std::string col3 = m[5].str();
                    std::string col4 = m[6].matched ? m[6].str() : "";
                    std::string lower_col3 = to_lower(col3);
                    std::string port_name = col3;
                    if ((lower_col3 == "dynamic" || lower_col3 == "static" || lower_col3 == "secure") && !col4.empty()) {
                        port_name = col4;
                    }
// Игнорируем служебные CPU-записи самого коммутатора
                    std::string lower_port = to_lower(port_name);
                    if (lower_port == "cpu" || lower_port == "self" || lower_port == "0") {
                        continue;
                    }
                    entries.push_back(ParsedFdbEntry{ vlan, port_name, *mac_opt });
                }
            }
            return entries;
        }
    };
} // namespace net_discovery

#endif //TEST_TELNET_SSH_SOLUTION_VENDORPARSER_H
