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
    // Включает оператор гонки корутин '||'
    using namespace asio::experimental::awaitable_operators;

    // Перечисление для понятного отображения статуса порта
    enum class PortStatus {
        Open,       //Порт открыт (получен SYN-ACK, соединение установлено)// SYN-ACK получен -> порт готов к обмену данными
        Closed,     // Порт закрыт (получен RST или ICMP ошибка)// RST получен -> хост активен, но служба не запущена
        Filtered    // Порт молчит (таймаут, пакет застрял в Firewall / ACL)// Таймаут/ICMP ошибка -> пакет заблокирован Firewall
    };

    class TcpScanner : public ISubnetScanner {
    private:
        // Ссылка на планировщик асинхронных событий (io_context), в потоке которого работают сокеты и таймеры сканера
        asio::any_io_executor executor_;

    public:
        // Конструктор принимает исполнитель (executor) io_context
        // Конструктор привязывает сканер к конкретному потоку ввода-вывода (io_context)
        explicit TcpScanner(asio::any_io_executor exec) : executor_(exec) {}

    /* @brief Проверка одного конкретного TCP-порта (Атомарная операция). Атомарная проверка одного TCP-порта (гонка connect || timer, низкоуровневая операция)
    * @param ip_address Целевой IP-адрес устройства
    * @param port Номер порта для проверки
    * @param timeout_ms Время ожидания ответа в миллисекундах (для локальных сетей рекомендуется 150-300)
    */
        asio::awaitable<PortStatus> scan_single_port(
                const std::string& ip_address,
                uint16_t port,
                std::chrono::milliseconds timeout_ms)
        {
            // На каждую проверку создаем СВЕЖИЙ сокет. Повторно использовать сокет без сброса нельзя.
            // РЕСУРСЫ: На каждый запрос создается новый сокет. Повторное использование без сброса запрещено ОС.
            asio::ip::tcp::socket socket(executor_);
            //Инициализируем таймер
            asio::steady_timer timer(executor_);
            // Код результата подключения; изначально ставим "операция в процессе" (would_block), чтобы не принять незавершённую попытку за успех
            boost::system::error_code connect_ec = asio::error::would_block;

            try {
                // Формируем конечную точку для TCP протокола
                asio::ip::tcp::endpoint target_ep(asio::ip::make_address(ip_address), port);
                //// Заводим таймер: фиксируем момент срабатывания (текущее время + timeout_ms), отсчёт пошёл
                timer.expires_after(timeout_ms);

                // Запускаем асинхронную гонку между подключением сокета и таймером таймаута.
                // asio::redirect_error подавляет выброс C++ исключений на закрытых портах (экономит CPU).
                // ГОНКА: Ждем, что наступит раньше: системное подключение или таймаут таймера.
                // redirect_error подавляет тяжелые C++ исключения на закрытых портах, критически экономя CPU.
                auto race_result = co_await (
                        socket.async_connect(target_ep, asio::redirect_error(asio::use_awaitable, connect_ec)) ||
                        timer.async_wait(asio::use_awaitable)
                );
                // Переменная-заглушка для перегрузки close(ec), которая не выбрасывает C++ исключения при ошибке закрытия сокета
                boost::system::error_code close_ec;
                // Обязательно закрываем сокет, освобождая дескриптор в ОС Windows/Linux!
                // ОСВОБОЖДЕНИЕ: Принудительно закрываем дескриптор сокета в ОС сразу после завершения гонки.
                // Закрываем сокет без исключений; код ошибки намеренно игнорируем, так как результат порта уже получен, а ресурс в любом случае освобождается
                socket.close(close_ec);

                // Индекс 0: Сокет проснулся первым (сеть ответила быстрее, чем прошел таймаут)
                // АНАЛИЗ: Индекс 0 означает, что сеть ответила быстрее, чем истек таймаут.
                if (race_result.index() == 0) {
                    if (!connect_ec) {
                        // Ошибок нет -> соединен -> Открыт// Соединение успешно
                        co_return PortStatus::Open;
                    }
                    // Пришел сброс (RST) -> Закрыт// Сеть вернула RST пакет
                    co_return PortStatus::Closed;
                }
                // Первым сработал таймер (тишина в сети)
                co_return PortStatus::Filtered;
            }
            catch (const std::exception&) {
                // На случай невалидного IP-адреса или сбоя инициализации сокета
                // Защита от некорректных IP или системных сбоев сокета
                // Страховочный перехватчик: в 99.99% случаев исключений нет (используется redirect_error),
                // но при сбое ОС или невалидном IP он не даст одной корутине обрушить весь io_context
                co_return PortStatus::Closed;
            }
        }

        /**
         * @brief Реализация ISubnetScanner: сканирует подсеть, возвращает живые хосты и свободный IP-маяк.
         * Сканирование диапазона IP и портов с ограничением конкурентности (Worker Pool). Оркестратор пула воркеров для параллельного сканирования подсети
         * @param ip_addresses Список целевых IP
         * @param ports Список портов для проверки каждого IP
         * @param timeout_ms Таймаут на попытку
         * @param max_concurrent_workers Максимальное число ОДНОВРЕМЕННЫХ сокетов в сети (размер пула)
         */
        asio::awaitable<SubnetScanResult> scan_subnet(
                const SubnetTarget& subnet,
                const std::vector<uint16_t>& target_ports,
                std::chrono::milliseconds timeout_ms,
                size_t max_concurrent_workers) override
        {
            // Результат сканирования подсети (исходная подсеть, список живых хостов с открытыми портами и свободный IP-маяк)
            SubnetScanResult result;
            //Запомнили в результате подсеть для сканирования
            result.subnet = subnet;

            //Вынести в другой модуль или класс
            //----------
            if (target_ports.empty() || subnet.mask == 0) {
                co_return result;
            }

            // Вычисляем длину префикса из маски (например, 0xFFFFFF00 -> 24)
            uint16_t prefix_len = static_cast<uint16_t>(std::popcount(subnet.mask));

            // Защита от зависания на гигантских сетях (/8, /16) и пустых диапазонах (> /30).
            // TODO: в будущем сохранять широкие подсети (< /20) в таблицу карантина для ручного подтверждения админом
            if (prefix_len < 20 || prefix_len > 30) {
                co_return result;
            }

            //Преобразуем 32-битное число в объект одиночного IP-адреса (например, 192.168.1.0 без маски)
            asio::ip::address_v4 net_addr(subnet.net_ip);
            // Объединяем адрес и длину префикса в объект подсети (192.168.1.0/24), чтобы получить диапазон хостов
            asio::ip::network_v4 network(net_addr, prefix_len);

            std::vector<std::string> ip_addresses;
            for (const auto& ip : network.hosts()) {
                ip_addresses.push_back(ip.to_string());
            }
            // Страховочная проверка на случай пустого диапазона хостов (например, для масок /31 или /32),
            // чтобы не создавать канал нулевой емкости.
            // TODO (оптимизация): передавать IP напрямую как uint32_t без конвертации в std::string и обратно
            if (ip_addresses.empty()) {
                co_return result;
            }//----------

            //IP и порт
            using ScanTask = std::pair<std::string, uint16_t>;
            // Задаем явную емкость буфера канала, чтобы вместить все задачи
            size_t total_tasks = ip_addresses.size() * target_ports.size();
            //Определить точно количество воркеров
            size_t effective_workers = std::min(max_concurrent_workers, total_tasks);
            // БУФЕР: Создаем канал емкостью под все задачи, чтобы try_send выполнялся мгновенно и без блокировок.
            asio::experimental::channel<void(boost::system::error_code, ScanTask)> task_channel(executor_, total_tasks);

            // 1. Наполняем канал задачами
            // НАПОЛНЕНИЕ: Формируем декартово произведение (каждый IP x каждый Порт) и пушим в канал.
            for (const auto& ip : ip_addresses) {
                for (uint16_t port : target_ports) {
                    task_channel.try_send(boost::system::error_code{}, ScanTask{ip, port});
                }
            }
            // Но самый надежный способ для Asio Channel —
            // это закрыть его СРАЗУ ПОСЛЕ наполнения, НО читать пока async_receive возвращает успех,
            // потому что async_receive умеет выгребать остатки из буфера закрытого канала!
            // ВАЖНО: Фиксируем очередь. Закрытый канал разрешает воркерам дочитать буфер до конца через async_receive.
            task_channel.close();

            // Агрегация результатов в едином потоке io_context (мьютексы не требуются)
            // Группировка открытых портов по IP-адресу: объединяет находки разных воркеров (например, 22 и 23) для одного хоста в единый список
            std::unordered_map<std::string, std::vector<uint16_t>> open_ports_by_ip;
            // Хосты, ответившие Open или Closed (RST)
            std::unordered_set<std::string> responsive_ips;

            // ЛЯМБДА ВОРКЕРА: Описывает поведение одного потока внутри пула.
            auto worker = [&]() -> asio::awaitable<void> {
                try {
                    // Бесконечный цикл разбора очереди, выход строго по ошибке закрытия пустого канала
                    for (;;) {
                        // Код результата подключения
                        boost::system::error_code ec;
                        // async_receive вытащит данные из буфера, ДАЖЕ если канал вызвал .close(),
                        // и вернет ошибку channel_closed ТОЛЬКО когда буфер полностью опустеет.
                        // Извлекаем задачу. Если буфер пуст И канал закрыт -> вернется channel_closed.
                        ScanTask task = co_await task_channel.async_receive(asio::redirect_error(asio::use_awaitable, ec));

                        if (ec == asio::experimental::channel_errc::channel_closed) {
                            // Вот теперь это честный выход: буфер пуст, канал закрыт. Честный выход из пула: задачи полностью исчерпаны
                            break;
                        }

                        // Выполняем сканирование
                        PortStatus status = co_await scan_single_port(task.first, task.second, timeout_ms);

                        if (status == PortStatus::Open) {
                            open_ports_by_ip[task.first].push_back(task.second);
                            responsive_ips.insert(task.first);
                        } else if (status == PortStatus::Closed) {
                            // Хост прислал RST — значит IP занят живым устройством
                            responsive_ips.insert(task.first);
                        }
                    }
                } catch (...) {
                    // Изолируем исключения. // Изолируем непредвиденные сбои воркера, защищая общую нить выполнения
                }
                co_return;
            };

            // 3. Запускаем воркеров через старый добрый барьер из вашей прошлой логики
            // БАРЬЕР СИНХРОНИЗАЦИИ (Latch): Управляет ожиданием группы асинхронных корутин.
            // Счётчик активных воркеров в пуле (сколько корутин ещё в работе);
            // shared_ptr продлевает жизнь счётчика до завершения самой последней фоновой корутины
            auto remaining_workers = std::make_shared<size_t>(effective_workers);
            // Асинхронный барьер ожидания (Latch): таймер ставится на бесконечность и принудительно отменяется (cancel) последним воркером;
            // shared_ptr гарантирует сохранность таймера до завершения всех фоновых корутин
            auto wait_timer = std::make_shared<asio::steady_timer>(executor_);
            //Ставим таймеру бесконечное время срабатывания. Сам по себе он никогда не «зазвонит» по времени
            wait_timer->expires_at(std::chrono::steady_clock::time_point::max());

            // СПАВН: Разворачиваем пул воркеров в контексте io_context.
            for (size_t i = 0; i < effective_workers; ++i) {
                asio::co_spawn(executor_, [&worker, remaining_workers, wait_timer]() -> asio::awaitable<void> {
                    co_await worker();
                    if (--(*remaining_workers) == 0) {
                        // Все воркеры завершились — будим оркестратор. // Последний воркер будит главный поток сканера
                        wait_timer->cancel();
                    }
                    co_return;
                }, asio::detached);
            }

            // Ожидаем, пока все воркеры отстреляются. // ОЖИДАНИЕ: Блокируем корутину оркестратора до тех пор, пока таймер не будет отменен воркерами.
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
