//LocalNetworkConfig
#if 1

#include <string>
#include <iostream>
#include <vector>
#include "boost/asio.hpp"

namespace asio = boost::asio;

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>

#pragma comment(lib, "IPHLPAPI.lib")
#endif

// Структура теперь описывает ОДИН конкретный сетевой интерфейс
struct LocalNetworkConfig {
    ULONG interface_index;
    std::string interface_name;   // Описание адаптера (например, "Intel(R) Ethernet Connection")
    std::string ip_address;
    std::string subnet_mask;
    std::string gateway;
    std::string dhcp_server;
    std::vector<std::string> dns_servers;
};

struct NetworkRoute {
    std::string destination_subnet; // Целевая сеть (например, "10.10.0.0")
    ULONG prefix_length;            // Длина маски CIDR (например, 16 -> /16)
    std::string next_hop;           // Адрес шлюза для этого маршрута (Next Hop)
    ULONG interface_index;          // Индекс сетевой карты (для связки с адаптером)
    ULONG metric;                   // Метрика (приоритет) маршрута
};

/**
 * @brief Модуль сбора сетевых настроек для ВСЕХ активных интерфейсов текущего компьютера
 */
class LocalConfigProvider {
public:
    static std::vector<LocalNetworkConfig> get_current_config() {
        std::vector<LocalNetworkConfig> active_interfaces;
#ifdef _WIN32
        ULONG outBufLen = 15000; // Рекомендуемый размер буфера для WinAPI
        std::vector<BYTE> buffer(outBufLen);
        PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

        // Запрашиваем адреса, шлюзы и DNS-серверы
        ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX;
        ULONG dwRetVal = GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(outBufLen);
            pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
            dwRetVal = GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &outBufLen);
        }

        if (dwRetVal == NO_ERROR) {
            // Итерируемся по сетевым адаптерам
            for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses != NULL; pCurrAddresses = pCurrAddresses->Next) {
                // Нам нужны только активные интерфейсы (OperStatus == IfOperStatusUp)
                // Исключаем Loopback ( IfType == IF_TYPE_SOFTWARE_LOOPBACK )
                if (pCurrAddresses->OperStatus != IfOperStatusUp || pCurrAddresses->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                    continue;
                }

                LocalNetworkConfig config;

                // Извлекаем понятное имя адаптера (конвертируем wchar_t* описание в обычную строку)
                if (pCurrAddresses->Description) {
                    std::wstring wdesc(pCurrAddresses->Description);
                    config.interface_name = std::string(wdesc.begin(), wdesc.end());
                }

                // 1. Получаем IP и маску (извлекаем первый IPv4 адрес интерфейса)
                for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress; pUnicast != NULL; pUnicast = pUnicast->Next) {
                    if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                        config.ip_address = inet_ntoa(sa_in->sin_addr);

                        // Вычисляем маску на основе длины префикса (например, префикс 24 -> 255.255.255.0)
                        ULONG mask = (0xFFFFFFFF << (32 - pUnicast->OnLinkPrefixLength)) & 0xFFFFFFFF;
                        in_addr mask_addr;
                        mask_addr.S_un.S_addr = htonl(mask);
                        config.subnet_mask = inet_ntoa(mask_addr);
                        break;
                    }
                }

                // Если у интерфейса нет назначенного IPv4 адреса — он нам не интересен для сканирования
                if (config.ip_address.empty()) {
                    continue;
                }

                // 2. Получаем основной шлюз (Gateway) для ЭТОГО адаптера
                if (pCurrAddresses->FirstGatewayAddress && pCurrAddresses->FirstGatewayAddress->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pCurrAddresses->FirstGatewayAddress->Address.lpSockaddr);
                    config.gateway = inet_ntoa(sa_in->sin_addr);
                }

                // 3. Получаем DNS-серверы
                for (PIP_ADAPTER_DNS_SERVER_ADDRESS pDns = pCurrAddresses->FirstDnsServerAddress; pDns != NULL; pDns = pDns->Next) {
                    if (pDns->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pDns->Address.lpSockaddr);
                        config.dns_servers.push_back(inet_ntoa(sa_in->sin_addr));
                    }
                }

                // 4. Получаем DHCP-сервер (если DHCP включен)
                if (pCurrAddresses->Dhcpv4Enabled && pCurrAddresses->Dhcpv4Server.lpSockaddr) {
                    sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pCurrAddresses->Dhcpv4Server.lpSockaddr);
                    config.dhcp_server = inet_ntoa(sa_in->sin_addr);
                }

                config.interface_index = pCurrAddresses->IfIndex; // Сохраняем индекс интерфейса Windows
                // ИСПРАВЛЕНИЕ: Вместо break мы складываем конфигурацию адаптера в вектор и идем на следующий шаг цикла!
                active_interfaces.push_back(config);
            }
        }
#else
        // Заглушка для Linux
        LocalNetworkConfig eth, wlan;
        eth.interface_name = "eth0 (Кабель)"; eth.ip_address = "192.168.1.10"; eth.subnet_mask = "255.255.255.0"; eth.gateway = "192.168.1.1";
        wlan.interface_name = "wlan0 (Wi-Fi)"; wlan.ip_address = "172.16.0.50"; wlan.subnet_mask = "255.255.240.0"; wlan.gateway = "172.16.0.1";
        active_interfaces.push_back(eth);
        active_interfaces.push_back(wlan);
#endif
        return active_interfaces;
    }

    /**
     * @brief Получает полную таблицу маршрутизации IPv4 из операционной системы Windows
     */
    static std::vector<NetworkRoute> get_routing_table() {
        std::vector<NetworkRoute> routes;

#ifdef _WIN32
        PMIB_IPFORWARD_TABLE2 pForwardTable = nullptr;

        // Запрашиваем таблицу маршрутизации только для IPv4 (AF_INET)
        DWORD dwRetVal = GetIpForwardTable2(AF_INET, &pForwardTable);

        if (dwRetVal == NO_ERROR && pForwardTable != nullptr) {
            // Пробегаем по всем маршрутам в полученной таблице
            for (ULONG i = 0; i < pForwardTable->NumEntries; i++) {
                MIB_IPFORWARD_ROW2 row = pForwardTable->Table[i];

                // Нам интересны только постоянные или динамические маршруты,
                // исключаем петлевые (loopback) адреса хоста типа 127.0.0.1/32
                if (row.DestinationPrefix.Prefix.si_family != AF_INET) {
                    continue;
                }

                NetworkRoute route;
                route.interface_index = row.InterfaceIndex;
                route.metric = row.Metric;
                route.prefix_length = row.DestinationPrefix.PrefixLength;

                // 1. Извлекаем целевую подсеть (Destination Prefix)
                sockaddr_in* dest_sin = reinterpret_cast<sockaddr_in*>(&row.DestinationPrefix.Prefix.Ipv4);
                route.destination_subnet = inet_ntoa(dest_sin->sin_addr);

                // Исключаем локальный петлевой трафик для чистоты логов Оркестратора
                if (route.destination_subnet == "127.0.0.1" || route.destination_subnet == "127.0.0.0") {
                    continue;
                }

                // 2. Извлекаем шлюз (Next Hop)
                if (row.NextHop.si_family == AF_INET) {
                    sockaddr_in* next_hop_sin = reinterpret_cast<sockaddr_in*>(&row.NextHop.Ipv4);
                    route.next_hop = inet_ntoa(next_hop_sin->sin_addr);
                } else {
                    route.next_hop = "0.0.0.0"; // Прямой маршрут (On-link), шлюз не нужен
                }

                routes.push_back(route);
            }

            // ОБЯЗАТЕЛЬНО освобождаем память, выделенную WinAPI функцией!
            FreeMibTable(pForwardTable);
        } else {
            std::cerr << "[ConfigProvider] Ошибка получения таблицы маршрутов. Код: " << dwRetVal << "\n";
        }
#else
        // Заглушка для Linux
        NetworkRoute r1;
        r1.destination_subnet = "0.0.0.0"; r1.prefix_length = 0; r1.next_hop = "192.168.1.1"; r1.interface_index = 1; r1.metric = 100;
        routes.push_back(r1);
#endif
        return routes;
    }
};


asio::awaitable<void> run_app_workflow() {
    auto exec = co_await asio::this_coro::executor;

    std::cout << "=== 1. АКТИВНЫЕ СЕТЕВЫЕ ИНТЕРФЕЙСЫ ===\n";
    auto interfaces = LocalConfigProvider::get_current_config();
    for (const auto& iface : interfaces) {
        std::cout << "Карта [ID: " << iface.interface_index << "]: " << iface.interface_name << "\n"
                  << "  IP/Маска:   " << iface.ip_address << " / " << iface.subnet_mask << "\n"
                  << "  Шлюз по ум.: " << (iface.gateway.empty() ? "НЕТ" : iface.gateway) << "\n"
                  << "  DHCP:       " << (iface.dhcp_server.empty() ? "НЕТ (Static)" : iface.dhcp_server) << "\n"
                  << "--------------------------------------------------\n";

        for (const auto& dns : iface.dns_servers) {
            std::cout << "  DNS-Сервер: " << dns << "\n";
        }
        std::cout << "--------------------------------------------------\n";
    }

    std::cout << "\n=== 2. СИСТЕМНАЯ ТАБЛИЦА МАРШРУТИЗАЦИИ IPv4 ===\n";
    auto routes = LocalConfigProvider::get_routing_table();

    // Выводим красивую шапку таблицы
    std::printf("%-18s %-10s %-18s %-12s %-6s\n", "Сеть Назначения", "Маска CIDR", "Шлюз (Next Hop)", "Индекс Карты", "Метрика");
    std::cout << "=========================================================================\n";

    for (const auto& route : routes) {
        std::printf("%-18s /%-9d %-18s %-12d %-6d\n",
                    route.destination_subnet.c_str(),
                    route.prefix_length,
                    route.next_hop.c_str(),
                    route.interface_index,
                    route.metric);
    }
    std::cout << "\n";
}



    int main(int argc, char* argv[]) {

#ifdef _WIN32
        // Устанавливаем кодировку UTF-8 (код 65001) для вывода текста на экран консоли.
        // Это устраняет кракозябры при чтении русских букв, приходящих от сервера.
        SetConsoleOutputCP(65001);

        // Устанавливаем кодировку UTF-8 (код 65001) для ввода текста с клавиатуры.
        // Это гарантирует, что вводимые нами символы кодируются так же, как их ожидает сервер.
        SetConsoleCP(65001);

        // Получаем низкоуровневый дескриптор (хэндл) стандартного вывода текущей консоли.
        // Этот хэндл нужен операционной системе, чтобы изменить внутренние настройки экрана.
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        // Проверяем, что хэндл консоли успешно получен и система не вернула ошибку.
        if (hOut != INVALID_HANDLE_VALUE) {
            // Создаем переменную для хранения текущего флага-маски режимов консоли.
            DWORD dwMode = 0;
            // Запрашиваем у Windows текущие настройки консоли и записываем их в dwMode.
            // Если функция вернула true (успех), значит, мы можем их модифицировать.
            if (GetConsoleMode(hOut, &dwMode)) {
                // Включаем аппаратную обработку виртуального терминала + обработку вывода
                // Включаем обработку ANSI-последовательностей (Esc-кодов).
                // Благодаря этому консоль Windows аппаратно понимает команды сервера
                // о цвете текста, очистке экрана и перемещении курсора (история команд).
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                // Включаем обработку служебных управляющих символов вывода.
                // Этот флаг заставляет Windows корректно интерпретировать такие символы,
                // как Backspace (удаление), перевод строки (\n) и возврат каретки (\r).
                dwMode |= ENABLE_PROCESSED_OUTPUT;
                // Записываем обновленную маску режимов обратно в настройки ОС Windows.
                // С этого момента консоль начинает работать в режиме умного терминала.
                SetConsoleMode(hOut, dwMode);
            }
            else {
                std::cerr << "[Внимание Windows] Не удалось прочитать режимы консоли. Возможно искажение текста.\n";
            }
        }
        else {
            // ОБРАБОТКА ОШИБКИ: Сигнализируем, что стандартный вывод недоступен
            std::cerr << "[Внимание Windows] Стандартный вывод консоли недоступен (INVALID_HANDLE_VALUE).\n"
                      << "Программа запущена в фоне или вывод перенаправлен в файл.\n";
        }

#endif

        // Проверяем аргументы командной строки
        /*if (argc != 2) {
            std::cerr << "Использование: tcp_scanner.exe [IP-Адрес]\n";
            std::cerr << "Пример: tcp_scanner.exe 192.168.0.1\n";
            return 1;
        }

        std::string target_ip = argv[1];*/

        try {
            // Создаем главный асинхронный движок
            asio::io_context io_ctx;

            // Вбрасываем нашу корневую корутину в планировщик io_context
            asio::co_spawn(io_ctx, run_app_workflow(), asio::detached);

            // Запускаем цикл обработки событий.
            // Поток main заблокируется здесь и будет асинхронно обрабатывать пакеты
            std::cout << "[Main] Запуск планировщика Boost.Asio..." << std::endl;
            io_ctx.run();
            std::cout << "[Main] Работа завершена штатно." << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[Fatal Error]: " << e.what() << std::endl;
            return 1;
        }

        return 0;


}

/*
 *
Модуль 2: Локальная конфигурация (IP, маска, шлюз, DNS, DHCP)

Получение настроек сети (особенно шлюза, DHCP и DNS) сильно зависит от операционной системы. Boost.Asio предоставляет кроссплатформенный класс asio::ip::network_v4, с помощью которого можно вытащить IP и маску локальных интерфейсов, но для получения шлюза, DNS и DHCP требуются системные API (Windows API / Linux netlink).
Ниже представлена реализация для Windows (так как в вашем main используется SetConsoleOutputCP). Она использует функцию GetAdaptersAddresses, которая возвращает абсолютно всю конфигурацию интерфейсов в системе.

Разбор архитектуры LocalConfigProvider


1. Выделение памяти через std::vector<BYTE>

Использование конструкции std::vector<BYTE> buffer(outBufLen); — это лучшая практика. Старые примеры MSDN предлагают использовать malloc() и free(), что привело бы к утечке памяти, если бы внутри цикла произошел непредвиденный throw. Вектор гарантирует автоматическое освобождение памяти (RAII).

2. Трюк с вычислением маски подсети

Вы написали очень лаконичную и правильную битовую математику для конвертации префикса (CIDR) в стандартную маску:
cpp
ULONG mask = (0xFFFFFFFF << (32 - pUnicast->OnLinkPrefixLength)) & 0xFFFFFFFF;
Используйте код с осторожностью.
Например, если OnLinkPrefixLength равен 24 (типичный префикс подсети /24):
1. 32 - 24 = 8.
2. 0xFFFFFFFF << 8 сдвигает 32-битное число, заполненное единицами, на 8 бит влево. На месте младших битов появляются нули.
3. Получается 0xFFFFFF00 (в двоичном виде: 24 единицы и 8 нулей).
4. Функция htonl() правильно переворачивает байты в сетевой порядок (Big Endian), а inet_ntoa превращает это в "255.255.255.0". Всё работает как швейцарские часы.

Маленькие, но важные нюансы на будущее (Code Review)

Хотя код рабочий, в нем есть два момента, о которых нужно знать при интеграции в постоянный проект:

А. Потокобезопасность inet_ntoa

Функция inet_ntoa является устаревшей (deprecated) в современном сетевом программировании. Она возвращает указатель на статический буфер внутри системной библиотеки ws2_32.dll.
• Риск: Если вызвать inet_ntoa параллельно из двух корутин, они перепишут буфер друг друга.
• Решение: Поскольку данный метод get_current_config() вызывается один раз при старте Оркестратора и работает синхронно (без co_await), сейчас это полностью безопасно. Но если в будущем вы захотите вызывать WinAPI функции асинхронно, лучше заменить её на inet_ntop, как мы делали ранее.

Б. Ошибка в имени структуры DHCP

В коде получения DHCP-сервера есть опечатка (небольшое расхождение с синтаксисом WinAPI):
cpp
// У вас в коде:
if (pCurrAddresses->Dhcpv4Enabled && pCurrAddresses->Dhcpv4Server.lpSockaddr)
// В официальном WinAPI структура называется:
if (pCurrAddresses->Dhcpv4Enabled && pCurrAddresses->Dhcpv4Server.RemoteAddress.lpSockaddr)
Используйте код с осторожностью.
Уточнение: Если ваш компилятор (MSVC/MinGW) успешно пропустил Dhcpv4Server.lpSockaddr, возможно, используется специфическая версия заголовочных файлов SDK, но по стандарту поле Dhcpv4Server имеет тип SOCKET_ADDRESS, внутри которого лежит структура RemoteAddress. Если при сборке на другой машине возникнет ошибка, вы будете знать, куда смотреть.

Как Оркестратор применит этот модуль?

Паттерн использования модуля в Оркестраторе будет следующим:
1. Вызываем LocalConfigProvider::get_current_config().
2. Берем local_cfg.ip_address и local_cfg.subnet_mask.
3. Передаем их в наш сетевой генератор Asio (asio::ip::make_network_v4), который мы написали в прошлый раз.
4. Получаем чистый диапазон IP-адресов всей нашей подсети.
5. Загружаем этот диапазон в boost::asio::experimental::channel нашего пула воркеров для молниеносного сканирования.
Сбор локальной конфигурации закрыт полностью.
Мы верифицировали и разобрали логику LocalConfigProvider. По какому пути двинемся дальше:
• Перейдем к разбору следующего по плану модуля — TcpTraceroute (асинхронная трассировка маршрута по TTL)?
• Или у вас появились вопросы по битовым маскам и структурам WinAPI адаптеров?

вопрос. А все эти WinAPI есть в asio? Или только так можно получить?

Короткий ответ: нет, в Boost.Asio таких функций нет.
Сетевой движок Boost.Asio не предоставляет готовых кроссплатформенных методов для получения маски подсети, основного шлюза, адресов DHCP или детального списка физических адаптеров [ENTITY_PRECISION_RECALL].
Чтобы собрать эту информацию, приходится использовать именно системные WinAPI (или Netlink/getifaddrs в Linux).

Почему этого нет в Asio?

Разработчики Boost.Asio строго следуют философии разделения обязанностей:
1. Зона ответственности Asio: Асинхронный ввод-вывод. Это отправка и прием пакетов, управление сокетами, таймерами, корутинами и потоками.
2. Зона ответственности Операционной Системы (ОС): Управление сетевыми интерфейсами, таблицами маршрутизации, конфигурацией IP и драйверами.
Поскольку на разных ОС (Windows, Linux, macOS, FreeBSD) сетевые стеки утилитарно усложнены и работают совершенно по-разному, попытка написать единую универсальную абстракцию над настройками адаптеров раздула бы библиотеку Boost в разы.

Что Asio умеет делать из коробки?

Asio предоставляет только самый базовый механизм разрешения имен — класс asio::ip::tcp::resolver (который мы использовали для DnsResolver). Он работает поверх системной функции getaddrinfo. С его помощью можно узнать IP-адрес своего компьютера, если спросить локальное имя (Local Hostname):
cpp
asio::ip::tcp::resolver resolver(io_context);
// Вернет только локальные IP-адреса, привязанные к имени ПК:
auto results = resolver.resolve(asio::ip::host_name(), "");
Используйте код с осторожностью.
Минусы подхода Asio: Этот метод вернет только голые IP-адреса. Вы никогда не узнаете через него, какая у этого адреса маска, где находится шлюз и включен ли вообще этот адаптер прямо сейчас.

Как поступают в профессиональной разработке?

Именно так, как сделали вы в модуле LocalConfigProvider:
• Обертывают специфичные низкоуровневые системные вызовы (WinAPI GetAdaptersAddresses для Windows или getifaddrs для Linux) в свои изолированные C++ классы.
• Извлекают из них чистые строки и числа.
• Передают эти данные в Boost.Asio (например, в asio::ip::make_network_v4), чтобы запустить высокопроизводительное асинхронное сканирование.
Ваш подход с разделением WinAPI-конфигуратора и Asio-сканера является абсолютно правильным и стандартным для системного программирования.
Поскольку природа системных вызовов теперь ясна, мы готовы двигаться дальше по плану.
Что разберем на следующем шаге:
• Перейдем к разбору и проектированию асинхронного модуля TcpTraceroute (трассировка маршрута на базе изменения TTL сокета)?
• Или вы хотите уточнить еще какие-то детали по структурам сетевых интерфейсов?

я сейчас проштудирую код и возможно как всегда задам вопросы

Отличный подход. Глубокое понимание каждого шага в системном коде — залог того, что при интеграции в Оркестратор архитектура не «поплывет».
Делаем паузу. Спокойно и внимательно проштудируйте код LocalConfigProvider. Посмотрите на:
• То, как мы фильтруем адаптеры (проверка OperStatus != IfOperStatusUp и проверка наличия шлюза, чтобы не зацепить виртуальные хвосты от Docker или Loopback) [ENTITY_PRECISION_RECALL].
• То, как устроены вложенные циклы (pCurrAddresses->FirstUnicastAddress и далее), ведь это классические связные списки из мира Си [ENTITY_PRECISION_RECALL].
• То, как макросы вроде 0xFFFFFFFF << (32 - префикс) формируют маску в памяти [ENTITY_PRECISION_RECALL].
Я на связи и готов разбирать любые строчки, битовые операции или структуры WinAPI, которые вызовут у вас вопросы. Как будете готовы — выкладывайте их сюда!

честно сказать, особо не вникал, пробежал глазами. Это надо каждую функцию по справочнику прокомментировать - тогда будет всё ясно. Пока этого достаточно. Но есть вопросы. У меня включены две карты: с кабелем и вай-фай. В выводе только один адрес. Ещё вопрос. Если минимум две карты, активные, - должно быть шлюза: один по умолчанию для всех сетей, другой для определённых сетей в виде маршрута. Соответственно для большего количества сетевых карт их адреса и шлюзы (для маршрутов) увеличиваются в количестве. Насколько реально получить дополнительные адреса и шлюзы? Получить дополнительные адреса мне кажется не проблема. Получить шлюзы - читать маршруты.

Вы абсолютно правы в своих рассуждениях. Вы затронули фундаментальную проблему маршрутизации в операционных системах при наличии нескольких сетевых интерфейсов (Multi-homing).
Давайте разберем оба ваших вопроса: почему сейчас вывелась только одна карта и как правильно вытащить всю картину мира, включая таблицы маршрутизации.

Вопрос 1: Почему вывелась только одна карта (кабель или Wi-Fi)?

Посмотрите на самую последнюю строчку логики в цикле вашего LocalConfigProvider:
cpp
// Нашли основной активный адаптер — выходим
if (!config.ip_address.empty()) break; // <-- ВОТ ОН, ТОРМОЗ!
Используйте код с осторожностью.
Ваша структура LocalNetworkConfig сейчас спроектирована так, что она может хранить в себе только одну строку IP, один шлюз и один DHCP. Из-за этого break на первой же активной карте (например, на кабеле Ethernet) функция прекращает работу, а Wi-Fi адаптер просто игнорируется, хотя WinAPI честно прочитал его из системы.

Как это исправить?

Точно так же, как мы делали в DnsResolver или TcpScanner — перевести структуру на коллекции (вектора). Функция должна возвращать std::vector<LocalNetworkConfig>, где каждый элемент вектора — это отдельная физическая карта (интерфейс) со своим собственным IP, своей маской подсети и своим шлюзом.

Вопрос 2: Как получить дополнительные шлюзы и маршруты?

Вы абсолютно правы: шлюз по умолчанию (Default Gateway, маршрут 0.0.0.0/0) у операционной системы обычно один (или несколько с разной метрикой — приоритетом, например, кабель приоритетнее Wi-Fi). Но для специфичных промышленных сетей администраторы вручную прописывают статические маршруты (например: «в подсеть контроллеров 10.10.x.x ходить через шлюз 192.168.0.254»).
Чистый GetAdaptersAddresses умеет выводить шлюзы по умолчанию для конкретного адаптера через поле FirstGatewayAddress, но он ничего не знает про кастомные статические маршруты (например, добавленные через команду route add).
Чтобы получить полную карту шлюзов и маршрутов в Windows, нужно использовать другую функцию из того же IP Helper API — GetIpForwardTable2 (или её старый аналог GetIpForwardTable).

Как устроена таблица маршрутизации в Windows API?

Функция GetIpForwardTable2 возвращает структуру MIB_IPFORWARD_TABLE2, которая содержит массив всех маршрутов в системе. Каждый маршрут (MIB_IPFORWARD_ROW2) содержит:
1. DestinationPrefix — целевая сеть (например, 10.10.0.0/16).
2. NextHop — адрес шлюза, через который туда надо идти (например, 192.168.0.254).
3. InterfaceLuid / InterfaceIndex — ID сетевой карты, к которой привязан этот маршрут.
4. Metric — приоритет маршрута.

Рекомендация для Оркестратора

Для вашей задачи автоматизации (поиск коммутаторов TFortis через ядро сети) у нас есть два пути:
1. Путь "Максимум": Написать парсер таблицы маршрутизации через GetIpForwardTable2.
2. Путь "Практичный" (Рекомендуемый): Переписать LocalConfigProvider, чтобы он возвращал список всех активных адаптеров (убрав break). Если для Wi-Fi и кабеля в Windows прописаны свои шлюзы, GetAdaptersAddresses честно вернет их оба. Наш Оркестратор возьмет эти шлюзы и проверит их оба на наличие SSH-порта — так мы гарантированно найдем Ядро сети, к какому бы адаптеру оно ни было подключено, без сложного парсинга системных таблиц маршрутов.
Если логика с векторами адаптеров и таблицей маршрутов понятна, как поступим:
• Модифицируем LocalConfigProvider, чтобы он возвращал вектор всех активных карт (кабель + Wi-Fi) без преждевременного break?
• Или оставим этот модуль как есть и перейдем к проектированию TcpTraceroute?

лучше модифицировать LocalConfigProvider для возвращения всех активных карт

Отличное решение. Модификация LocalConfigProvider для возврата вектора всех активных карт — это самый надежный способ увидеть всю сеть целиком, не усложняя код парсингом системных таблиц маршрутизации.
Мы сделаем две вещи:
1. Изменим структуру LocalNetworkConfig, чтобы она представляла один конкретный интерфейс.
2. Уберем преждевременный break и перегрузку полей, чтобы функция get_current_config() возвращала std::vector<LocalNetworkConfig> со всеми активными картами (и кабелем, и Wi-Fi).


Что изменилось и почему это важно:

1. Никакой потери адаптеров: Убран деструктивный break. Теперь в векторе окажутся и Wi-Fi, и проводная карта, если они обе подняты (OperStatusUp).
2. Добавлено поле interface_name: Чтобы вы глазами в консоли сразу понимали, где настройки Realtek PCIe (кабель), а где Intel Wireless (Wi-Fi).
3. Исправлен WinAPI-вызов DHCP: Строка обновлена до pCurrAddresses->Dhcpv4Server.RemoteAddress.lpSockaddr для 100% совместимости с Microsoft SDK.
Теперь Оркестратор получит полный массив отправных точек для сканирования каждой доступной подсети.
Если логика расширения на вектор карт понятна, переходим к следующему этапу:
• Начнем разбирать архитектуру и теорию модуля TcpTraceroute (асинхронный трейс)?
• Или вы хотите сначала скомпилировать и протестировать этот обновленный вывод?

работает! Выводит два адаптера с понятными именами и со всеми адресами. Может тогда уж доделать парсинг маршрутов. Хотя бы получить эти маршруты. Этого уже будет достаточно. В таблице же есть привязка к интерфейсу

Отличные новости! Здорово, что LocalConfigProvider теперь видит всю систему целиком.
Раз вы решили идти до конца и сделать систему максимально надежной для сложных промышленных сетей, давайте допишем парсинг таблицы маршрутизации Windows.
Вы абсолютно правы: в системной таблице каждый маршрут жестко привязан к конкретному интерфейсу через его индекс (InterfaceIndex или InterfaceLuid). Наш Оркестратор сможет сопоставить эти данные и понять: «Ага, подсеть контроллеров 10.10.x.x доступна именно через Wi-Fi карту, а не через кабель, и идти туда надо через кастомный шлюз!»

Как устроена функция GetIpForwardTable2

Мы будем использовать современную функцию GetIpForwardTable2 (из той же iphlpapi.dll).
1. Она выделяет память под таблицу автоматически.
2. Возвращает указатель на структуру MIB_IPFORWARD_TABLE2.
3. Внутри этой структуры лежит массив строк Table. Каждая строка (MIB_IPFORWARD_ROW2) описывает один маршрут.
4. После окончания работы мы обязаны вызвать функцию FreeMibTable, чтобы вернуть память операционной системе.

Шаг 1: Добавляем структуру маршрута

Добавим в наш заголовочный файл структуру, в которую будем сохранять чистые данные:


Как связать адаптеры и маршруты? (Поле InterfaceIndex)

Чтобы вы увидели, как красиво связываются адаптеры и маршруты через их системные индексы, нам нужно сделать одну маленькую деталь: добавить сохранение индекса адаптера в структуру LocalNetworkConfig.
1. Добавим поле в LocalNetworkConfig:



 */
#endif



//Tracert
#if 0
/**
 * @brief Модуль асинхронной трассировки маршрута (Аналог tracert на базе TCP)
 */
class TcpTraceroute {
public:
    explicit TcpTraceroute(asio::any_io_executor exec) : executor_(exec) {}

    /**
     * @brief Запускает трассировку до целевого хоста
     * @param target_ip IP-адрес назначения
     * @param port Порт для отправки SYN (лучше использовать заведомо известный порт, например 80, 443 или любой открытый сканером)
     * @param max_hops Максимальное количество шагов (обычно 30)
     */
    asio::awaitable<void> trace(std::string target_ip, uint16_t port, int max_hops = 30) {
        std::cout << "[Tracert] Трассировка маршрута до " << target_ip << ":" << port << " (макс. " << max_hops << " улов):\n";

        asio::ip::tcp::endpoint target_ep(asio::ip::make_address(target_ip), port);

        for (int ttl = 1; ttl <= max_hops; ++ttl) {
            asio::ip::tcp::socket socket(executor_);
            asio::steady_timer timer(executor_);

            try {
                // Открываем сокет для протокола IPv4 до выполнения коннекта
                socket.open(asio::ip::tcp::v4());

                // КЛЮЧЕВАЯ СТРОКА: Ограничиваем время жизни пакета в шагах маршрутизаторов
                asio::ip::unicast::ttl ttl_option(ttl);
                socket.set_option(ttl_option);

                timer.expires_after(std::chrono::milliseconds(500)); // Полусекундный таймаут на хоп
                boost::system::error_code connect_ec = asio::error::would_block;

                // Запускаем гонку подключения
                auto race_result = co_await (
                    socket.async_connect(target_ep, asio::redirect_error(asio::use_awaitable, connect_ec)) ||
                    timer.async_wait(asio::use_awaitable)
                );

                boost::system::error_code close_ec;
                socket.close(close_ec);

                // Оцениваем результат хопа
                if (race_result.index() == 0) {
                    // Если мы получили ЛЮБОЙ ответ от сети (успех коннекта или сброс RST) — значит, мы ДОСТИГЛИ цели!
                    // Ошибки вроде "Connection refused" (RST) или успех говорят о том, что пакет долетел до финала.
                    if (!connect_ec || connect_ec == asio::error::connection_refused) {
                        std::cout << "  " << ttl << ".\t" << target_ip << " [Цель достигнута]\n";
                        break;
                    }
                    // Если вернулась специфическая ошибка сети или хоста (ICMP Time Exceeded трансформируется ОС в коды ошибок)
                    else {
                        // Примечание: Разные ОС транслируют ICMP Time Exceeded по-разному.
                        // Windows часто прячет адрес промежуточного узла при TCP-коннекте.
                        // Для вывода точного IP транзитного узла без raw-сокетов не обойтись,
                        // но этот метод гарантированно показывает *дистанцию* (количество хопов) до цели.
                        std::cout << "  " << ttl << ".\tУзел ответил кодом: " << connect_ec.message() << "\n";
                    }
                } else {
                    // Таймер сработал первым (узел промолчал)
                    std::cout << "  " << ttl << ".\t*\t*\t* [Таймаут]\n";
                }
            }
            catch (const std::exception& e) {
                std::cout << "  " << ttl << ".\t[Ошибка сокета: " << e.what() << "]\n";
            }
        }
        std::cout << "[Tracert] Трассировка завершена.\n\n";
    }

private:
    asio::any_io_executor executor_;
};

asio::awaitable<void> run_app_workflow() {
    auto exec = co_await asio::this_coro::executor;


    // --- Тест 3: TRACERT ---
    std::cout << "=== АСИНХРОННЫЙ TCP TRACEROUTE ===\n";
    TcpTraceroute tracer(exec);
    if (!ips.empty()) {
        // Берем первый разрешенный IP из nslookup и трассируем до 80-го веб-порта
        co_await tracer.trace(ips[0], 80, 15);
    }
}

int main()
{

    int main(int argc, char* argv[]) {

#ifdef _WIN32
        // Устанавливаем кодировку UTF-8 (код 65001) для вывода текста на экран консоли.
        // Это устраняет кракозябры при чтении русских букв, приходящих от сервера.
        SetConsoleOutputCP(65001);

        // Устанавливаем кодировку UTF-8 (код 65001) для ввода текста с клавиатуры.
        // Это гарантирует, что вводимые нами символы кодируются так же, как их ожидает сервер.
        SetConsoleCP(65001);

        // Получаем низкоуровневый дескриптор (хэндл) стандартного вывода текущей консоли.
        // Этот хэндл нужен операционной системе, чтобы изменить внутренние настройки экрана.
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        // Проверяем, что хэндл консоли успешно получен и система не вернула ошибку.
        if (hOut != INVALID_HANDLE_VALUE) {
            // Создаем переменную для хранения текущего флага-маски режимов консоли.
            DWORD dwMode = 0;
            // Запрашиваем у Windows текущие настройки консоли и записываем их в dwMode.
            // Если функция вернула true (успех), значит, мы можем их модифицировать.
            if (GetConsoleMode(hOut, &dwMode)) {
                // Включаем аппаратную обработку виртуального терминала + обработку вывода
                // Включаем обработку ANSI-последовательностей (Esc-кодов).
                // Благодаря этому консоль Windows аппаратно понимает команды сервера
                // о цвете текста, очистке экрана и перемещении курсора (история команд).
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                // Включаем обработку служебных управляющих символов вывода.
                // Этот флаг заставляет Windows корректно интерпретировать такие символы,
                // как Backspace (удаление), перевод строки (\n) и возврат каретки (\r).
                dwMode |= ENABLE_PROCESSED_OUTPUT;
                // Записываем обновленную маску режимов обратно в настройки ОС Windows.
                // С этого момента консоль начинает работать в режиме умного терминала.
                SetConsoleMode(hOut, dwMode);
            }
            else {
                std::cerr << "[Внимание Windows] Не удалось прочитать режимы консоли. Возможно искажение текста.\n";
            }
        }
        else {
            // ОБРАБОТКА ОШИБКИ: Сигнализируем, что стандартный вывод недоступен
            std::cerr << "[Внимание Windows] Стандартный вывод консоли недоступен (INVALID_HANDLE_VALUE).\n"
                      << "Программа запущена в фоне или вывод перенаправлен в файл.\n";
        }

#endif

        // Проверяем аргументы командной строки
        /*if (argc != 2) {
            std::cerr << "Использование: tcp_scanner.exe [IP-Адрес]\n";
            std::cerr << "Пример: tcp_scanner.exe 192.168.0.1\n";
            return 1;
        }

        std::string target_ip = argv[1];*/

        try {
            // Создаем главный асинхронный движок
            asio::io_context io_ctx;

            // Вбрасываем нашу корневую корутину в планировщик io_context
            asio::co_spawn(io_ctx, run_app_workflow(), asio::detached);

            // Запускаем цикл обработки событий.
            // Поток main заблокируется здесь и будет асинхронно обрабатывать пакеты
            std::cout << "[Main] Запуск планировщика Boost.Asio..." << std::endl;
            io_ctx.run();
            std::cout << "[Main] Работа завершена штатно." << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[Fatal Error]: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

}

/*
 *
Модуль 3: Асинхронный Трейсерут (tracert / traceroute)

Написание полноценного tracert на «сырых» ICMP сокетах (raw sockets) требует прав администратора (root/admin) в любой ОС и жестко блокируется современными брандмауэрами.
Однако в индустрии есть изящный, современный и абсолютно безопасный трюк — TCP Traceroute. Вместо отправки ICMP-пинов мы делаем async_connect на любой открытый порт цели (например, веб-порт 80 или 443), но принудительно выставляем сокету параметр TTL (Time To Live), последовательно увеличивая его от 1 до 30.
• Когда TTL падает до 0, промежуточный маршрутизатор обязан вернуть нам ICMP-пакет Time Exceeded.
• В Boost.Asio мы можем задавать TTL для TCP сокета штатными средствами:
asio::ip::unicast::ttl ttl_option(current_ttl); socket.set_option(ttl_option);


 */
#endif



//nslookup
#if 0

#include <iostream>
#include <boost/asio.hpp>

namespace asio = boost::asio;

/**
 * @brief Модуль асинхронного разрешения DNS-имен (Полноценный аналог nslookup)
 */
/**
 * @brief Модуль асинхронного разрешения DNS-имен (Полноценный аналог nslookup)
 */
class DnsResolver {
public:
    explicit DnsResolver(asio::any_io_executor exec) : executor_(exec) {}

    /**
     * @brief Прямой поиск: разрешает имя хоста в список IP-адресов (Forward Lookup)
     */
    asio::awaitable<std::vector<std::string>> resolve_name_to_ips(const std::string& hostname) {
        std::vector<std::string> resolved_ips;
        asio::ip::tcp::resolver resolver(executor_);
        boost::system::error_code ec;

        // Запрашиваем IP-адреса для хоста
        auto results = co_await resolver.async_resolve(hostname, "", asio::redirect_error(asio::use_awaitable, ec));

        if (!ec) {
            // Безопасный обход результатов через range-based for (обходит баги итераторов Boost)
            for (const auto& entry : results) {
                resolved_ips.push_back(entry.endpoint().address().to_string());
            }
        } else {
            std::cerr << "[DNS] [!] Ошибка прямого поиска для '" << hostname << "': " << ec.message() << "\n";
        }

        co_return resolved_ips;
    }

    /**
     * @brief Обратный поиск: определяет доменное имя по его IP-адресу (Reverse Lookup)
     * // ОПТИМИЗИРОВАНО: Обратный поиск (Системный адрес -> Все имена хоста)
     */
    asio::awaitable<std::vector<std::string>> resolve_ip_to_names(const asio::ip::address& address) {
        std::vector<std::string> resolved_names;
        asio::ip::tcp::resolver resolver(executor_);
        boost::system::error_code ec;

        try
        {


            // Создаем endpoint. Порт 0, он не важен для DNS PTR-запроса
            asio::ip::tcp::endpoint endpoint(address, 0);

            // Передаем endpoint. Asio понимает, что нужен обратный PTR-запрос
            auto results = co_await resolver.async_resolve(endpoint, asio::redirect_error(asio::use_awaitable, ec));

            if (!ec)
            {
                // ТЕПЕРЬ СОБИРАЕМ ВСЕ ИМЕНА БЕЗ ПРЕЖДЕВРЕМЕННОГО ВЫХОДА
                for (const auto &entry: results)
                {
                    resolved_names.push_back(entry.host_name());
                }
            }else {
                      std::cerr << "[DNS] [!] Обратный поиск не дал результатов для " << address << ": "
                              << ec.message() << "\n";
                }
                //co_return resolved_names;

        }
        catch (const std::exception& e) {
            std::cerr << "[DNS] [!] Ошибка выполнения обратного поиска: " << e.what() << "\n";
        }

        co_return resolved_names;
    }

    /**
     * @brief Автоматический nslookup: сам определяет тип строки (IP или имя) и выводит результат
     */
    // Главный метод автоматического определения
    asio::awaitable<void> resolve(const std::string& input) {
        boost::system::error_code ec;

        // 1. Парсим адрес один раз в корневом методе
        auto address = asio::ip::make_address(input, ec);

        if (!ec) {
            std::cout << "[nslookup] Запрос определен как IP-адрес. Запуск Reverse DNS...\n";
            // Передаем уже готовый объект адреса
            auto names = co_await resolve_ip_to_names(address);

            if (!names.empty()) {
                std::cout << "  [+] Результаты (Имена) для IP " << input << ":\n";
                for (const auto& name : names) {
                    std::cout << "    -> " << name << "\n";
                }
                std::cout << "\n";
            } else {
                std::cout << "  [-] Результат: Для IP " << input << " имен в DNS не найдено.\n\n";
            }
        }
        else {
            std::cout << "[nslookup] Запрос определен как имя хоста. Запуск Forward DNS...\n";
            auto ips = co_await resolve_name_to_ips(input);
            if (!ips.empty()) {
                std::cout << "  [+] Результаты (IP) для '" << input << "':\n";
                for (const auto& ip : ips) {
                    std::cout << "    -> " << ip << "\n";
                }
                std::cout << "\n";
            } else {
                std::cout << "  [-] Результат: Имя '" << input << "' не удалось разрешить.\n\n";
            }
        }
    }

private:
    asio::any_io_executor executor_;
};






asio::awaitable<void> run_app_workflow() {
    auto exec = co_await asio::this_coro::executor;
    DnsResolver nslookup(exec);

    // --- КЕЙС 1: Прямой поиск (Имя -> IP) ---
    co_await nslookup.resolve("google.com");

    // --- КЕЙС 2: Обратный поиск (IP -> Имя) ---
    // ВАЖНО: Для локальных сетей роутеры редко заполняют PTR-записи.
    // Для гарантированного теста обратного поиска используем публичный DNS-сервер Cloudflare (1.1.1.1) или Google (8.8.8.8)
    co_await nslookup.resolve("8.8.8.8");
}


int main(int argc, char* argv[])
{
#ifdef _WIN32
    // Устанавливаем кодировку UTF-8 (код 65001) для вывода текста на экран консоли.
    // Это устраняет кракозябры при чтении русских букв, приходящих от сервера.
    SetConsoleOutputCP(65001);

    // Устанавливаем кодировку UTF-8 (код 65001) для ввода текста с клавиатуры.
    // Это гарантирует, что вводимые нами символы кодируются так же, как их ожидает сервер.
    SetConsoleCP(65001);

    // Получаем низкоуровневый дескриптор (хэндл) стандартного вывода текущей консоли.
    // Этот хэндл нужен операционной системе, чтобы изменить внутренние настройки экрана.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // Проверяем, что хэндл консоли успешно получен и система не вернула ошибку.
    if (hOut != INVALID_HANDLE_VALUE)
    {
        // Создаем переменную для хранения текущего флага-маски режимов консоли.
        DWORD dwMode = 0;
        // Запрашиваем у Windows текущие настройки консоли и записываем их в dwMode.
        // Если функция вернула true (успех), значит, мы можем их модифицировать.
        if (GetConsoleMode(hOut, &dwMode))
        {
            // Включаем аппаратную обработку виртуального терминала + обработку вывода
            // Включаем обработку ANSI-последовательностей (Esc-кодов).
            // Благодаря этому консоль Windows аппаратно понимает команды сервера
            // о цвете текста, очистке экрана и перемещении курсора (история команд).
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            // Включаем обработку служебных управляющих символов вывода.
            // Этот флаг заставляет Windows корректно интерпретировать такие символы,
            // как Backspace (удаление), перевод строки (\n) и возврат каретки (\r).
            dwMode |= ENABLE_PROCESSED_OUTPUT;
            // Записываем обновленную маску режимов обратно в настройки ОС Windows.
            // С этого момента консоль начинает работать в режиме умного терминала.
            SetConsoleMode(hOut, dwMode);
        } else
        {
            std::cerr << "[Внимание Windows] Не удалось прочитать режимы консоли. Возможно искажение текста.\n";
        }
    } else
    {
        // ОБРАБОТКА ОШИБКИ: Сигнализируем, что стандартный вывод недоступен
        std::cerr << "[Внимание Windows] Стандартный вывод консоли недоступен (INVALID_HANDLE_VALUE).\n"
                  << "Программа запущена в фоне или вывод перенаправлен в файл.\n";
    }

#endif

    // Проверяем аргументы командной строки
    /*if (argc != 2) {
        std::cerr << "Использование: tcp_scanner.exe [IP-Адрес]\n";
        std::cerr << "Пример: tcp_scanner.exe 192.168.0.1\n";
        return 1;
    }

    std::string target_ip = argv[1];*/

    try
    {
        // Создаем главный асинхронный движок
        asio::io_context io_ctx;

        // Вбрасываем нашу корневую корутину в планировщик io_context
        asio::co_spawn(io_ctx, run_app_workflow(), asio::detached);

        // Запускаем цикл обработки событий.
        // Поток main заблокируется здесь и будет асинхронно обрабатывать пакеты
        std::cout << "[Main] Запуск планировщика Boost.Asio..." << std::endl;
        io_ctx.run();
        std::cout << "[Main] Работа завершена штатно." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Fatal Error]: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}



/*
 *
Модуль 1: Аналог nslookup (Асинхронный DNS-резолвер)

Для разрешения имен в IP-адреса в Boost.Asio есть встроенный класс asio::ip::tcp::resolver. Он работает асинхронно и позволяет получить весь список IPv4 и IPv6 адресов, привязанных к имени.

Действительно, классический nslookup работает в две стороны: Forward DNS (имя → IP) и Reverse DNS (IP → имя).
В текущей реализации метод resolver.async_resolve принимает имя хоста и пытается найти его IP. Чтобы заставить его делать обратное преобразование (Reverse Lookup), нам нужно передать ему готовый объект конечной точки (asio::ip::tcp::endpoint) вместо строки с именем, либо использовать специальный перегруженный метод.
Давайте доведем модуль DnsResolver до абсолютного идеала, разделив его на две четкие асинхронные операции.

Модернизированный DnsResolver (Прямой и Обратный поиск)

В сетевом стеке Boost.Asio для обратного резолвинга достаточно передать в async_resolve валидный объект tcp::endpoint. Порт при этом можно поставить в 0, так как для DNS-службы важен только сам IP-адрес.


Разбор механики по полочкам: как это работает изнутри?

1. Как операционная система понимает, что мы ищем имя по IP?
На низком сетевом уровне для этого используется специальный тип DNS-запроса — PTR (Pointer) запись. Когда вы передаете в async_resolve объект endpoint с IP-адресом (например, 8.8.8.8), Boost.Asio под капотом трансформирует его в реверсивный формат (для IPv4 это будет выглядеть как запрос зоны 8.8.8.8.in-addr.arpa.).
2. Почему метод возвращает results->host_name()?
Ответ от DNS-сервера приходит в виде итератора записей. Стрелочка -> (или разыменование) у итератора результатов results в Asio сразу смотрит на первый валидный элемент, у которого мы и забираем текстовое имя хоста.

Объект results — это итерируемый диапазон (range). Метод .begin() возвращает объект публичного типа basic_resolver_results::iterator. У этого итератора перегружен стандартный оператор ->, который предоставляет открытый и безопасный доступ к полям структуры basic_resolver_entry, включая метод .host_name().


В чем подвох проверки по первому символу?

1. Имена хостов могут начинаться с цифр: В промышленных сетях имена коммутаторов или серверов часто называют по номерам комнат, этажей или инвентарным номерам (например: 1floor-switch.local, 302-server, 8888.com). Если проверить только первую цифру, программа решит, что 1floor-switch — это IP-адрес, передаст его в обратный поиск и упадет с ошибкой.
2. IPv6 адреса: Они вообще начинаются с букв (например: fe80::1 или 2001:db8::). Проверка на цифру тут полностью сломается.

Как сделать это правильно и надежно?

Вместо ручного анализа строк, мы задействуем системный метод, который скрыт внутри самого сетевого стека: boost::asio::ip::make_address.
Этот метод — профессиональный валидатор. Он пытается распарсить строку по строгим правилам сетевых протоколов IPv4 и IPv6.
• Если строка является валидным IP-адресом (любой версии) — метод отработает успешно.
• Если строка — это доменное имя (или мусор) — метод выбросит исключение (или вернет код ошибки).
Давайте объединим обе функции в один универсальный метод resolve (прямо как в настоящем nslookup), который сам поймет, что перед ним:


Преимущества такого подхода:

1. 100% надежность: Никакое имя хоста (даже 8888.com или 123-server) не обманет программу. make_address четко знает, как выглядит структура IP.
2. Готовность к IPv6: Если завтра ваша сеть перейдет на IPv6, этот код продолжит работать без изменений.
3. Чистый интерфейс: Пользователю (или нашему будущему Оркестратору) больше не нужно думать, какой метод вызывать. Есть одна точка входа.


Разбор ошибок, которые мы победили:

1. Проблема с inaccessible / отсутствием .front(): Использование конструкции for (const auto& entry : results) полностью скрывает от нас внутреннее устройство итераторов Boost.Asio. Компилятор видит только чистую структуру entry, у которой публично доступен метод .host_name().
2. Почему раньше мог не выводиться результат: Если мы передавали в make_address имя хоста без указания переменной ошибки ec (make_address(input)), этот метод при сбое выбрасывал тяжелое C++ исключение std::runtime_error. Корутина падала до того, как переходила в блок прямого поиска. Теперь вызов make_address(input, ec) работает тихо, безопасно и просто возвращает статус.



 */
#endif





//ping, v.2
#if 0
#include <boost/asio.hpp>
// Подключаем стандартные операторы корутин для организации гонки (||)
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <variant>

namespace asio = boost::asio;
using namespace asio::experimental::awaitable_operators;// Включает оператор гонки корутин '||'

// Структура ICMP заголовка согласно спецификации RFC 792
struct icmp_header {
    uint8_t type;// Тип: 8 (Echo Request), 0 (Echo Reply)
    uint8_t code;// Код: всегда 0 для эхо-запросов/ответов
    uint16_t checksum;// Контрольная сумма всего пакета (заголовок + данные)
    uint16_t identifier;// Уникальный ID сессии (обычно PID процесса)
    uint16_t sequence;// Инкрементируемый порядковый номер пакета
};

// Подсчет контрольной суммы Internet Checksum (RFC 1071)
uint16_t calculate_checksum(const uint16_t* data, size_t length) {
    uint32_t sum = 0;
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }
    if (length > 0) {// Обработка нечетного остаточного байта
        sum += *reinterpret_cast<const uint8_t*>(data);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum);
}

class PingClient {
public:
    // Конструктор инициализирует raw-сокет для работы по протоколу ICMP IPv4
    explicit PingClient(asio::any_io_executor exec)
            : executor_(exec), socket_(exec, asio::ip::icmp::v4()), timer_(exec) {}

    // Верхнеуровневый сценарий пинга (аналог утилиты командной строки)
    asio::awaitable<void> ping_host(std::string host, int request_count = 4, size_t payload_size = 32) {
        try {
            // ШАГ 1: Асинхронное разрешение DNS-имени или IP-строки в сетевую точку (Endpoint)
            asio::ip::icmp::resolver resolver(executor_);
            auto endpoints = co_await resolver.async_resolve(host, "", asio::use_awaitable);
            asio::ip::icmp::endpoint destination = *endpoints.begin();

            std::cout << "\nОбмен пакетами с " << host << " [" << destination.address().to_string()
                      << "] с " << payload_size << " байтами данных:\n";

            uint16_t sequence = 0;

            // ШАГ 2: Получение ID текущего процесса для фильтрации "своих" ICMP пакетов в сети
#if defined(_WIN32)
            uint16_t pid = static_cast<uint16_t>(GetCurrentProcessId());
#else
            uint16_t pid = static_cast<uint16_t>(getpid());
#endif

            // ШАГ 3: Цикл отправки последовательности запросов (параметр -n)
            for (int i = 0; i < request_count; ++i) {
                co_await send_echo_request(destination, pid, ++sequence, payload_size);
                co_await receive_echo_reply(pid, sequence, payload_size);

                // Пауза 1 секунда между отправками пакетов (поток ОС НЕ блокируется)
                timer_.expires_after(std::chrono::seconds(1));
                co_await timer_.async_wait(asio::use_awaitable);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[Ping Error]: " << e.what() << "\n";
        }
    }

private:
    // Формирование и отправка ICMP пакета в сеть
    asio::awaitable<void> send_echo_request(asio::ip::icmp::endpoint dest, uint16_t id, uint16_t seq, size_t payload_size) {
        // Выделение буфера: 8 байт (заголовок) + N байт данных (параметр -l)
        std::vector<uint8_t> packet(sizeof(icmp_header) + payload_size);
        icmp_header* header = reinterpret_cast<icmp_header*>(packet.data());

        // Заполнение полей заголовка (в сетевом порядке байт - htons)
        header->type = 8;
        header->code = 0;
        header->checksum = 0;
        header->identifier = htons(id);
        header->sequence = htons(seq);

        // Заполнение полезной нагрузки алфавитным паттерном
        uint8_t* payload = packet.data() + sizeof(icmp_header);
        for (size_t i = 0; i < payload_size; ++i) {
            payload[i] = static_cast<uint8_t>('a' + (i % 23));
        }

        // Финальный расчет чексуммы от всего готового вектора байт
        header->checksum = calculate_checksum(reinterpret_cast<uint16_t*>(packet.data()), packet.size());

        // Фиксация времени отправки и неблокирующая передача в сокет
        start_time_ = std::chrono::steady_clock::now();
        co_await socket_.async_send_to(asio::buffer(packet), dest, asio::use_awaitable);
    }

    // Ожидание ответа с обработкой таймаута и фильтрацией трафика
    asio::awaitable<void> receive_echo_reply(uint16_t expected_id, uint16_t expected_seq, size_t expected_payload_size) {
        std::vector<uint8_t> reply_buffer(65536);// Большой буфер под входящий IP-пакет
        asio::ip::icmp::endpoint sender_ep;

        timer_.expires_after(std::chrono::milliseconds(2000));// Жесткий лимит ожидания ответа: 2 сек.

        // ШАГ 4: Организация неблокирующей гонки (race) "Чтение из сокета" VS "Таймаут"
        // Оператор '||' автоматически прервет и очистит проигравшую асинхронную операцию.

        // Избавляемся от experimental магии.
        // Используем оператор ||, который создает гонку (race) между двумя корутинами.
        // Результатом будет std::variant, содержащий результат той операции, которая завершилась ПЕРВОЙ.
        // При этом вторая операция автоматически и безопасно отменяется (Cancel).
        auto result = co_await (
                socket_.async_receive_from(asio::buffer(reply_buffer), sender_ep, asio::use_awaitable) ||
                timer_.async_wait(asio::use_awaitable)
        );

        // Вариант 0: Из сети успешно пришли байты до истечения таймаута
        // Индекс 0 в variant означает, что первой выполнилась async_receive_from
        if (result.index() == 0) {
            size_t bytes_recvd = std::get<0>(result);
            auto end_time = std::chrono::steady_clock::now();
            auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_).count();

            // ШАГ 5: Парсинг IPv4 заголовка (вычисляем длину IHL, извлекаем TTL)
            uint8_t ip_header_len = (reply_buffer[0] & 0x0F) * 4; // Исправлено: обращение по индексу к байту

            if (bytes_recvd >= ip_header_len + sizeof(icmp_header)) {
                // Сдвигаем указатель за пределы IP-заголовка, к началу ICMP данных
                icmp_header* icmp = reinterpret_cast<icmp_header*>(reply_buffer.data() + ip_header_len);

                // ШАГ 6: Валидация пакета. Проверяем, что это Echo Reply (0) именно для нашего PID и Sequence
                if (icmp->type == 0 && ntohs(icmp->identifier) == expected_id && ntohs(icmp->sequence) == expected_seq) {
                    uint8_t ttl = reply_buffer[8]; // Чтение поля TTL из 9-го байта IP-заголовка// Исправлено: TTL находится в 9-м байте (индекс 8) IP-заголовка
                    size_t icmp_data_bytes = bytes_recvd - ip_header_len - sizeof(icmp_header);

                    std::cout << "Ответ от " << sender_ep.address().to_string()
                              << ": число байт=" << icmp_data_bytes
                              << " время=" << rtt << "мс TTL=" << static_cast<int>(ttl) << "\n";
                    co_return;// Пакет успешно обработан, выходим из корутины ожидания
                }
            }
            // Если пакет прошел валидацию по чексумме сокета, но ID/Seq чужие —
            // рекурсивно возобновляем ожидание следующего пакета в рамках этого же шага.

            // Если пакет не наш, рекурсивно продолжаем ждать в рамках того же шага цикла
            co_await receive_echo_reply(expected_id, expected_seq, expected_payload_size);
        } else {
            // Вариант 1: Первым сработал таймер. Пакет не вернулся за 2 секунды.

            // Индекс 1 в variant означает, что первым завершился таймер (таймаут)
            std::cout << "Превышен интервал ожидания запроса.\n";
        }
    }

    asio::any_io_executor executor_;
    asio::ip::icmp::socket socket_;
    asio::steady_timer timer_;
    std::chrono::steady_clock::time_point start_time_;
};


int main() {

#ifdef _WIN32
    // Устанавливаем кодировку UTF-8 (код 65001) для вывода текста на экран консоли.
    // Это устраняет кракозябры при чтении русских букв, приходящих от сервера.
    SetConsoleOutputCP(65001);

    // Устанавливаем кодировку UTF-8 (код 65001) для ввода текста с клавиатуры.
    // Это гарантирует, что вводимые нами символы кодируются так же, как их ожидает сервер.
    SetConsoleCP(65001);

    // Получаем низкоуровневый дескриптор (хэндл) стандартного вывода текущей консоли.
    // Этот хэндл нужен операционной системе, чтобы изменить внутренние настройки экрана.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // Проверяем, что хэндл консоли успешно получен и система не вернула ошибку.
    if (hOut != INVALID_HANDLE_VALUE) {
        // Создаем переменную для хранения текущего флага-маски режимов консоли.
        DWORD dwMode = 0;
        // Запрашиваем у Windows текущие настройки консоли и записываем их в dwMode.
        // Если функция вернула true (успех), значит, мы можем их модифицировать.
        if (GetConsoleMode(hOut, &dwMode)) {
            // Включаем аппаратную обработку виртуального терминала + обработку вывода
            // Включаем обработку ANSI-последовательностей (Esc-кодов).
            // Благодаря этому консоль Windows аппаратно понимает команды сервера
            // о цвете текста, очистке экрана и перемещении курсора (история команд).
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            // Включаем обработку служебных управляющих символов вывода.
            // Этот флаг заставляет Windows корректно интерпретировать такие символы,
            // как Backspace (удаление), перевод строки (\n) и возврат каретки (\r).
            dwMode |= ENABLE_PROCESSED_OUTPUT;
            // Записываем обновленную маску режимов обратно в настройки ОС Windows.
            // С этого момента консоль начинает работать в режиме умного терминала.
            SetConsoleMode(hOut, dwMode);
        }
        else {
            std::cerr << "[Внимание Windows] Не удалось прочитать режимы консоли. Возможно искажение текста.\n";
        }
    }
    else {
        // ОБРАБОТКА ОШИБКИ: Сигнализируем, что стандартный вывод недоступен
        std::cerr << "[Внимание Windows] Стандартный вывод консоли недоступен (INVALID_HANDLE_VALUE).\n"
                  << "Программа запущена в фоне или вывод перенаправлен в файл.\n";
    }

#endif

    try {
        asio::io_context io_ctx;

        // Инициализируем клиент, передавая executor от io_context
        PingClient pinger(io_ctx.get_executor());

        // Запускаем корутину в контексте io_context
        asio::co_spawn(io_ctx,
                       pinger.ping_host("mail.ru", 3, 128),
                       asio::detached
        );

        // Запускаем цикл обработки событий
        io_ctx.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}

/*
 *
Архитектура асинхронного пинга на корутинах

1. Асинхронный Резолвер: gethostbyname заменяется на resolver.async_resolve.
2. Асинхронный Таймер и Сокет: Мы запускаем параллельно две операции — чтение из сокета ответа и таймер таймаута, объединяя их через экспериментальный (но уже стандартный для 1.87) asio::experimental::make_parallel_group или asio::race.

Полный код современного асинхронного модуля Ping

Ниже представлен готовый класс PingClient, который идеально впишется в ваш будущий оркестратор.


Главные улучшения по сравнению с вашим Си-кодом:

1. 0% Простоя CPU: Больше никаких Sleep() или блокирующих ожиданий ядерной функции pIcmpSendEcho. Пока пинг ждет ответа от коммутатора TFortis, ваш единственный поток io_context может параллельно обрабатывать SSH-команды или собирать SNMP-таблицы.
2. Изящный таймаут через make_parallel_group: В Asio 1.87 гонка асинхронных операций (сеть против таймера) пишется в несколько строк без развесистых колбэков.
3. Кроссплатформенность: Этот код скомпилируется и будет летать как на Windows, так и на Linux (внутри встроенных систем автоматизации).


Что изменилось и как это работает:

1. Динамический размер пакета: Строка std::vector<uint8_t> packet(sizeof(icmp_header) + payload_size) берет значение из параметра -l и выделяет ровно столько байт, сколько запросил пользователь.
2. Безопасный расчет чексуммы: Функция calculate_checksum теперь обрабатывает буферы произвольной длины (даже нечетное количество байт), корректно дополняя остаток, как требует стандарт RFC 1071.
3. Расчет чистых данных ответа: При выводе на экран icmp_data_bytes вычисляет размер полученного payload (вычитая заголовки IP и ICMP), чтобы выводить точное число байт, как оригинальная утилита.


Важные исправления:

1. Синтаксис Гонки (||): Код co_await (socket.async_receive_from(...) || timer.async_wait(...)) возвращает std::variant. Конструкция result.index() == 0 однозначно и без ошибок компиляции определяет, что из сети пришли данные, а не сработал таймаут.
2. Исправление парсинга буфера: Заменено ошибочное сырое приведение reply_buffer на явное обращение к элементам вектора: reply_buffer[0] для расчета длины заголовка IP и reply_buffer[8] для корректного чтения поля TTL.


Вариант 1. Запуск из другой корутины (Самый правильный путь)

Если вы вызываете PingClient внутри другой корутины, вы можете получить текущий валидный исполнитель через системный токен co_await asio::this_coro::executor:
cpp
#include <boost/asio.hpp>
#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> my_orchestrator_workflow() {
    // 1. Получаем executor текущей корутины
    asio::any_io_executor current_exec = co_await asio::this_coro::executor;

    // 2. Инициализируем PingClient
    PingClient pinger(current_exec);

    // 3. Запускаем пинг (передаем имя узла, -n 4, -l 64)
    co_await pinger.ping_host("192.168.0.1", 4, 64);
}


Вариант 2. Инициализация прямо в функции main()

Если вы хотите протестировать модуль изолированно в main(), вы забираете исполнитель у объекта io_context через метод .get_executor() и упаковываете корутину в asio::co_spawn:
cpp
int main() {
    try {
        asio::io_context io_ctx;

        // Инициализируем клиент, передавая executor от io_context
        PingClient pinger(io_ctx.get_executor());

        // Запускаем корутину в контексте io_context
        asio::co_spawn(io_ctx,
            pinger.ping_host("8.8.8.8", 4, 32),
            asio::detached
        );

        // Запускаем цикл обработки событий
        io_ctx.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}


Вариант 3. Инициализация как поля класса (Например, внутри Orchestrator)

Если PingClient будет долгоживущим объектом внутри будущего класса-оркестратора, передавайте any_io_executor в конструктор оркестратора:
cpp
class NetworkOrchestrator {
public:
    // Конструктор принимает общий executor для всех асинхронных подсистем
    explicit NetworkOrchestrator(asio::any_io_executor exec)
        : executor_(exec), pinger_(exec) {} // Инициализируем пингер тем же исполнителем

    void start_monitoring() {
        asio::co_spawn(executor_, [this]() -> asio::awaitable<void> {
            co_await pinger_.ping_host("192.168.1.1", 10, 32);
        }, asio::detached);
    }

private:
    asio::any_io_executor executor_;
    PingClient pinger_; // Объект живет внутри оркестратора
};


1. Зачем нужен asio::ip::icmp::resolver?

Он нужен для превращения человеческих имен в IP-адреса (DNS-резолвинг). Если пользователь передаст строку "google.com", сокет не сможет отправить туда пакет напрямую, ему нужен IP-адрес (например, 8.8.8.8). resolver обращается к DNS-серверу ОС и возвращает список IP-адресов, привязанных к этому имени. Если вы сразу передадите строку "192.168.0.1", резолвер просто быстро распарсит её в числовой IP без сетевых запросов.

2. Корутина resolver.async_resolve запускается отложенно?

Нет, она запускается сразу. Слово async_resolve и токен asio::use_awaitable заставляют функцию мгновенно отправить асинхронный запрос в ОС, а ключевое слово co_await приостанавливает (замораживает) выполнение вашей корутины ping_host. Поток процессора при этом свободен и может выполнять другие задачи (например, SSH или SNMP). Как только ОС получит ответ от DNS-сервера, Asio «разморозит» корутину, и результат запишется в переменную endpoints на следующей строке. Для программиста код выглядит линейным, но под капотом он асинхронный.

3. Что такое asio::ip::icmp::endpoint?

Это класс-контейнер, который объединяет в себе IP-адрес назначения (IPv4 или IPv6) и номер порта (хотя для протокола ICMP понятия портов нет, для унификации с TCP/UDP в Asio используется единая концепция эндпоинтов). Внутри destination хранится бинарный IP-адрес коммутатора, к которому мы шлем пакет.

4. Что такое htons?

Это системная функция, расшифровывающаяся как Host To Network Short (Перевод 16-битного целого числа из порядка байт процессора в сетевой порядок байт).
• Большинство процессоров (x86/x64 от Intel/AMD) используют порядок Little-Endian (младший байт пишется первым).
• Сетевые протоколы (IP, ICMP) по стандарту RFC требуют порядок Big-Endian (старший байт первым).
• htons гарантирует, что число 0x1234 на вашем компьютере превратится в 0x3412 внутри сетевого пакета, иначе другое оборудование (коммутатор TFortis) прочитает ваш ID процесса некорректно.

5. Означает ли asio::use_awaitable, что корутина send_echo_request не завершится, пока не отработает async_send_to?

Да, абсолютно верно. Токен asio::use_awaitable в связке с co_await заставляет корутину send_echo_request ждать, пока сетевая карта и драйвер ОС физически не заберут буфер пакета на отправку. Только после этого корутина пойдет дальше и завершится.

6. Почему такой большой буфер reply_buffer (65536 байт)?

Максимально возможный размер IP-пакета в сетях IPv4 составляет 65535 байт (включая все заголовки). Выделяя буфер максимального размера, мы на 100% гарантируем, что сокет сможет принять ответ любого объема (даже если коммутатор сойдет с ума или пришлет нам огромный кусок данных), избегая ошибки переполнения буфера (Buffer Overflow). В промышленном коде память под этот буфер выделяется один раз, чтобы не делать тяжелую аллокацию std::vector на каждый пинг.

7. Почему в таймауте ставим именно 2000 миллисекунд (2 секунды)?

Это стандартное дефолтное значение (RFC и утилиты Windows). Двух секунд более чем достаточно, чтобы пакет долетел до любого узла в мире и вернулся обратно. В локальных сетях с коммутаторами TFortis ответ обычно приходит за 1–5 мс. Если за 2 секунды ответа нет — сеть либо перегружена, либо узел мертв.

8. Блокирует ли оператор || дальнейшее выполнение кода?

Да. Оператор || объединяет две операции в одну общую "сверх-корутину". co_await замораживает текущий код на этой строчке до тех пор, пока хотя бы одна из двух операций (сеть или таймер) полностью не завершится. Как только кто-то «победил», код мгновенно идет на следующую строку.

9. Что возвращает std::get<0>(result)? Число байт, полученных нами, или полученных узлом?

Это число байт, полученных именно вашим сокетом из сети (размер пришедшего к нам IP-пакета). Ваш компьютер ничего не знает о том, сколько байт обработал коммутатор внутри себя. Сокет просто говорит: "Я достал из сетевого провода пакет размером Х байт".

10. Что по сути означает магия (reply_buffer[0] & 0x0F) * 4?

Это ручной парсинг структуры IPv4-заголовка согласно стандарту RFC 791:
• Любой входящий пакет на сыром сокете начинается с IP-заголовка. Самый первый его байт (reply_buffer[0]) разделен пополам: первые 4 бита — это версия (IPv4 = 4), а вторые 4 бита — IHL (Internet Header Length), длина заголовка в 32-битных словах.
• Операция маски & 0x0F отбрасывает версию и оставляет только значение IHL.
• Так как длина указана в "словах" (по 4 байта), мы умножаем полученное число на 4. Обычно там получается число 5. 5 * 4 = 20 байт. Это означает, что заголовок IP занимает первые 20 байт, а данные ICMP (наш ответ) начинаются ровно с 21-го байта (reply_buffer.data() + 20).

11. Что такое icmp->type и ntohs?

• icmp->type: Поле в ICMP-заголовке. Значение 8 означает "Я прошу ответить" (Echo Request), а значение 0 означает "Я отвечаю на твой запрос" (Echo Reply). Мы проверяем, что нам пришел именно ответ (== 0).
• ntohs: Функция, противоположная htons (Network To Host Short). Она переводит 16-битное число из сетевого порядка байт обратно в порядок вашего процессора, чтобы вы могли корректно сравнить identifier и sequence с вашими локальными переменными.

12. Где мы проверяем чек-сумму? Зачем мы её считаем, если проверки нет?

Вы подловили отличный нюанс!
1. Зачем мы её считаем при отправке: Операционная система и сетевая карта требуют, чтобы отправляемый ICMP-пакет имел валидную контрольную сумму. Если мы запишем туда 0 или мусор, первый же маршрутизатор или сам целевой коммутатор TFortis по дороге отбросит этот пакет как поврежденный, и мы никогда не получим ответ.
2. Почему мы не проверяем её при получении: Это делает за нас само ядро операционной системы Windows/Linux на уровне сетевого драйвера. Когда пакет поступает на сетевую карту, стек TCP/IP операционной системы автоматически считает контрольную сумму. Если пакет битый, ОС уничтожает его молча и даже не передает в наш сокет socket_.async_receive_from. Если наш сокет проснулся и получил байты — пакет гарантированно целостный и прошел аппаратную и системную проверку чек-суммы. Поэтому ручной код проверки в корутине не нужен.
Все ли моменты теперь прояснились? Если вы готовы двигаться дальше, дайте знать:
• Переходим ли мы к разработке следующего сетевого модуля (например, сканера TCP-портов), или хотите обсудить что-то еще по пингу?

про чек-сумму я не подумал. Мы же её помещаем в структуру  icmp_header и "запаковываем" в packet перед отправкой.

Абсолютно верно! Вы в самую точку разложили этот процесс.
Давайте зафиксируем, как это происходит поэтапно на физическом уровне (в памяти программы):
1. Вы создаете сырой вектор байт packet нужного размера.
2. Накладываете на его начало структуру заголовка: icmp_header* header = ....
3. Заполняете поля type, code, id, sequence, а поле checksum временно сбрасываете в 0.
4. Заполняете оставшуюся часть вектора (payload / полезную нагрузку) вашими данными ('a', 'b', 'c'...).
5. И вот тут наступает магия: вы вызываете функцию calculate_checksum, передавая ей весь пакет целиком (заголовок вместе с заполненными данными). Функция считает итоговое число.
6. Вы записываете полученный результат обратно в header->checksum. Теперь пакет «подписан» и готов.
7. Сетевая карта коммутатора TFortis, приняв этот пакет, выполнит точно такой же расчет. Если её результат совпадет с числом, которое вы туда записали, коммутатор поймет, что пакет не исказился в кабеле, и ответит на него.
Вы отлично разобрались в низкоуровневой сетевой логике и корутинах Boost.Asio! Этот модуль PingClient теперь полностью прозрачен, понятен и готов к работе.
Как мы поступим дальше в нашей сессии?
• Начнем проектировать новый асинхронный модуль (например, сканер TCP-портов для поиска активных сервисов на оборудовании)?
• Или вернемся к обсуждению глобальной архитектуры автоматизации, раз у нас теперь есть 4 рабочих инструмента (Telnet, SSH, SNMP и Ping)?

не совсем понял строку icmp_header* header = reinterpret_cast<icmp_header*>(packet.data());. Что в ней происходит. Получаем указатель массива packet типа std::vector<uint8_t>, приводим этот указатель к типу указателя на структуру icmp_header. Запоминаем этот новый указатель в header. Не понятно зачем мы так делаем. К заголовку (к его полям) мы и так можем обратиться. Каким же образом мы его упаковываем в packet?

Эта строка — классический прием низкоуровневого программирования, который называется маппингом (или наложением) структуры на сырой буфер памяти.
Давайте разберем, почему без этого не обойтись и как именно здесь происходит «упаковка».

В чем проблема «обратиться напрямую»?

Ваш std::vector<uint8_t> packet — это просто последовательный массив из N одиночных байт в памяти. Вектор ничего не знает про сетевые протоколы, для него это просто цепочка чисел от 0 до 255.
Если не использовать структуру icmp_header, то чтобы заполнить, например, двухбайтовое поле identifier, вам пришлось бы вручную заниматься битовыми сдвигами и распихивать байты по индексам вектора:
cpp
// Вот так выглядел бы ад без наложения структуры:
packet[0] = 8; // type
packet[1] = 0; // code
packet[4] = id >> 8;   // пихаем старший байт ID в 4-ю ячейку
packet[5] = id & 0xFF; // пихаем младший байт ID в 5-ю ячейку
Используйте код с осторожностью.
Это неудобно, в этом легко ошибиться и такой код сложно читать.

Что делает reinterpret_cast?

Строка icmp_header* header = reinterpret_cast<icmp_header*>(packet.data()); говорит компилятору C++ следующее:
«Вместо того чтобы смотреть на начало вектора packet как на массив отдельных байт, представь, что там в памяти уже лежит готовая структура icmp_header».
Память вектора 'packet':
[ ячейка 0 ] [ ячейка 1 ] [ ячейка 2 ] [ ячейка 3 ] [ ячейка 4 ] [ ячейка 5 ] ... [ ячейка N ]

|___________________________________________________________________________|
 \_ Благодаря reinterpret_cast сюда "накладывается" трафарет структуры:

   |-- type --|-- code --|------- checksum -------|------ identifier ------| ... |-- payload --|

Как происходит упаковка?

Вы правы: мы просто сохранили адрес в указатель header. Но магия в том, что header указывает на ту же самую область памяти, где живет packet.data(). У них один и тот же адрес!
Поэтому, когда на следующих строках вы пишете:
cpp
header->type = 8;
header->identifier = htons(id);
Используйте код с осторожностью.
Вы напрямую меняете байты внутри вектора packet. Структура header служит просто удобным «трафаретом» (или маской), чтобы компилятор знал, в какие именно байты вектора нужно записать type или identifier.

Итог:

Мы не копируем заголовок внутрь вектора. Мы создали вектор нужного размера, наложили на его начало структуру-трафарет, заполнили поля через эту структуру (тем самым изменив первые 8 байт вектора), а оставшуюся часть вектора заполнили данными. В этот момент пакет считается полностью упакованным и готовым к передаче в функцию async_send_to.

 Добрый день! Рад продолжить нашу работу.
Вы абсолютно верно описали первую часть алгоритма: мы разбиваем весь пакет на 16-битные «слова», складываем их и отдельно обрабатываем «хвост», если в пакете оказалось нечетное количество байт.
А вот финальный цикл действительно выглядит как зубодробительная математическая магия. На самом деле этот алгоритм стандартизирован в RFC 1071 и называется Internet Checksum. Он используется во всех низкоуровневых протоколах (IP, TCP, UDP, ICMP) с 1980-х годов.
Давайте «разберем этот механизм на шестеренки» с помощью простых аналогий, и вы увидите, что логика здесь очень изящная.

В чем главная проблема арифметики?

Мы складываем много 16-битных чисел (максимальное значение каждого — 65535 или 0xFFFF).
Когда мы складываем их в переменной uint32_t sum, сумма очень быстро начинает превышать 65535. Происходит переполнение, и лишние биты уходят «наверх» — в старшие 16 бит переменной sum.
По стандарту Интернет-контрольной суммы, мы не имеем права просто выбросить эти старшие биты. Мы обязаны применить арифметику дополнения до единицы (Ones' Complement). Это значит, что все биты, которые «вышли за разрядную сетку» 16 бит, должны быть возвращены обратно и прибавлены к младшим битам.

Разбор страшного цикла по строчкам

Давайте посмотрим на этот цикл:
cpp
while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
}
Используйте код с осторожностью.

Понятие 1: Что делает sum >> 16?

Эта операция сдвигает число на 16 бит вправо. Она как бы «отрезает» верхнюю половину 32-битного числа.
• Пример: Если в sum лежит число, у которого в старших битах что-то есть, то sum >> 16 вернет эти самые «лишние» биты (перенос).
• Условие цикла while (sum >> 16) означает: «Пока у нас в старших 16 битах есть хоть что-то (то есть число еще не помещается в рамки 16 бит), продолжай цикл».

Понятие 2: Что делает sum & 0xFFFF?

Маска 0xFFFF в двоичном виде — это шестнадцать единиц. Побитовое «И» (&) с этой маской обнуляет старшие 16 бит и оставляет только «чистые» младшие 16 бит нашего числа.

Понятие 3: Складываем их вместе

Строка sum = (sum & 0xFFFF) + (sum >> 16); по сути означает:
«Возьми нижние 16 бит числа, возьми верхние 16 бит (которые переполнились) и сложи их между собой».

Зачем нужен цикл while, а не просто один if?

В 99% случаев хватает одного сложения. Но если после сложения младшей и старшей части результат снова превысил 65535 (такое бывает при очень больших пакетах), то опять возникнет перенос в старшие биты. Цикл гарантирует, что мы будем «сворачивать» число до тех пор, пока оно железно не уложится в диапазон от 0 до 65535.

Последний штрих: Зачем нужна инверсия ~sum?

В самом конце функция делает return static_cast<uint16_t>(~sum); (оператор ~ — это побитовое НЕ, меняет все 0 на 1, а 1 на 0).
Это требование стандарта. По спецификации ICMP, в поле контрольной суммы должен лежать именно инвертированный результат.
Зачем это сделано?
Это гениальный трюк для принимающей стороны (коммутатора TFortis):
1. Когда коммутатор получает ваш пакет, его сетевой чип складывает вообще все 16-битные слова пакета, включая вашу контрольную сумму.
2. Поскольку вы заранее инвертировали сумму перед отправкой, при сложении «прямого» значения пакета и «инвертированного» значения чексуммы на стороне получателя, в идеальном состоянии (если кабель цел) должны получиться все единицы (0xFFFF).
3. Проверить, равен ли результат 0xFFFF, для процессора сетевой карты — это всего одна микропроцессорная инструкция. Это позволяет сетевому оборудованию проверять миллиарды пакетов в секунду без малейших задержек.

Наглядный пример "на пальцах"

Представим, что мы работаем не с 16-битными числами, а с обычными двузначными числами (от 00 до 99).
Все, что выше 99 — это переполнение (третий разряд).
1. Мы складывали данные и у нас получилось число 125. Оно трехзначное, не влезает в «двузначный» лимит.
2. Старший разряд (перенос) — это 1. Младший (остаток) — это 25.
3. Сворачиваем по нашему алгоритму: 25 + 1 = 26.
4. Теперь число 26 идеально влезает в двузначный лимит. Цикл завершен.
5. Инвертируем его (в нашей аналогии — вычитаем из максимума): 99 - 26 = 73.
6. Число 73 мы и отправляем в пакете.
Стала ли теперь понятна эта низкоуровневая математика с переносом разрядов?
Если да, то как планируете поступить дальше:
• Перейдем к проектированию асинхронного сканера TCP-портов на корутинах?
• Или хотите разобрать еще какие-то битовые маски в коде?

так коммутатор получит пакет с контрольной суммой 73. Он всё сложит и получит результат 198. Единички хватать не будет?

Вы правы, моя аналогия с вычитанием из 99 не совсем точно передала математику двоичного дополнения до единицы (Ones' Complement). Вы очень точно подметили, что простое сложение 26 + 73 дает 99, а не «круглый» ноль или перенос.
Давайте исправим аналогию на точную двоичную математику, и вы увидите, как получается тот самый красивый результат на стороне коммутатора.

Как это происходит в реальных битах (на примере 4-битных чисел)

Чтобы не запутаться в 16-битных числах, возьмем маленькие 4-битные числа.
В 4 битах максимальное число — это 1111 (в десятичной системе 15).
Всё, что больше 15 — это переполнение (выход в 5-й бит).

Шаг 1: Отправка (Ваш компьютер)

Допустим, у нас в пакете всего два числа, которые мы хотим отправить: 1010 (10) и 0101 (5).
1. Складываем их: 1010 + 0101 = 1111 (15).
2. Старших бит для «сворачивания» тут нет (число поместилось в 4 бита).
3. Инвертируем результат (меняем 0 на 1, а 1 на 0): Было 1111, стало 0000.
4. Записываем эту контрольную сумму (0000) в заголовок и отправляем пакет коммутатору.

Шаг 2: Получение и проверка (Коммутатор TFortis)

Коммутатор получает пакет и, согласно алгоритму RFC 1071, складывает вообще всё, что пришло, включая саму контрольную сумму:
• 1010 (первое число)
• 0101 (второе число)
• 0000 (ваша контрольная сумма)
Сетевой чип складывает: 1010 + 0101 + 0000 = 1111.
Результат — все единицы (1111)! Сетевая карта коммутатора видит, что все биты заполнены единицами, и мгновенно понимает: пакет целый.

А что если при сложении было переполнение?

Давайте возьмем другие числа, где при сложении на вашем компьютере вылезает лишний бит.
Складываем 1100 (12) и 0111 (7).

На вашем компьютере:

1. 1100 + 0111 = 10011 (19). Появился 5-й бит (единица слева)!
2. Работает наш страшный цикл («сворачивание»): отрезаем этот 5-й бит и прибавляем его к оставшимся 4 битам:
0011 + 1 = 0100 (4).
3. Теперь инвертируем результат 0100: получаем 1011. Это наша контрольная сумма. Отправляем пакет.

На коммутаторе TFortis:

Коммутатор складывает всё вместе: 1100 (12) + 0111 (7) + 1011 (наша чексумма).
1. 1100 + 0111 = 10011 (19)
2. Прибавляем чексумму: 10011 + 1011 = 11110 (30).
3. У сетевого чипа коммутатора тоже сработает «сворачивание» переполнения (5-й бит, который равен 1, отрезается и прибавляется к младшим 4 битам):
1110 + 1 = 1111.
И снова на выходе получаются все единицы (1111)!

В чем гениальность?

Вне зависимости от того, какие числа вы отправляете, и было ли при этом переполнение, благодаря правилу «инверсии» (~sum), на стороне получателя при сложении всего пакета всегда должен получаться результат, состоящий исключительно из битовых единиц (0xFFFF для 16 бит). Если в итоговом результате затесался хотя бы один ноль — пакет поврежден.
Теперь логика с «всеми единицами» встала на свои места?
Если всё понятно, мы можем:

 Вы абсолютно правы: пространство имен asio::experimental::awaitable_operators все еще содержит слово experimental. В версии Boost 1.87 разработчики Asio оставили его там для обратной совместимости, хотя сами операторы уже признаны стабильными.
Если ваша цель — полностью очистить код от слова experimental и использовать только 100% стабильное, стандартное API Boost.Asio без переписывания логики корутины, у нас есть идеальный инструмент: asio::race (или его близнец asio::any_cast / asio::make_race_group).
Начиная с последних версий Boost, функция asio::race официально стала частью основного стабильного API и решает задачу гонки корутин точно так же, как оператор ||.

Локальное решение: замена оператора || на asio::race

Мы меняем только внутренности метода receive_echo_reply. Вся остальная логика, рекурсия, типы данных и структура программы остаются нетронутыми.

 // СТАБИЛЬНЫЙ ВАРИАНТ БЕЗ EXPERIMENTAL:
    // Функция asio::race запускает обе корутины параллельно.
    // Она возвращает std::variant, где индекс (0 или 1) указывает на победителя.
    // Проигравшая корутина автоматически и безопасно отменяется.
    auto result = co_await asio::race(
        socket_.async_receive_from(asio::buffer(reply_buffer), sender_ep, asio::use_awaitable),
        timer_.async_wait(asio::use_awaitable)
    );

 */
#endif



//SSH, v.2
#if 0
#include <iostream>
#include <vector>
#include <string>
#include <regex>

#include <conio.h>
#include <boost/asio.hpp>
#include <libssh2.h>
#include <windows.h>



/**
 *
Архитектурный обзор: Как это работает «с высоты птичьего полета»

Наш асинхронный SSH-клиент держится на «трех китах»:
 * 1. Единственный поток ОС (io_context): Весь наш код (и чтение, и запись, и таймеры) выполняется строго в одном системном потоке. Никаких гонок данных (race conditions) или мьютексов.
2. Крипто-движок (libssh2): Он переведен в неблокирующий режим. Он не умеет сам работать с сетью — он лишь шифрует и дешифрует байты. Когда ему не хватает данных или сокет забит, он говорит: LIBSSH2_ERROR_EAGAIN.
3. Планировщик (Boost.Asio 1.87): Он управляет реальным системным сокетом. Когда libssh2 утыкается в EAGAIN, Boost.Asio берет сокет, засыпает (освобождая процессор) и просыпается ровно тогда, когда операционная система говорит: «Сокет готов к чтению/записи».

 */
namespace asio = boost::asio;
using asio::ip::tcp;

// Асинхронный канал SSH (адаптирован под скрытую аутентификацию)
class asio_ssh_channel {
private:
    tcp::socket& socket_;
    LIBSSH2_SESSION* session_ = nullptr;
    LIBSSH2_CHANNEL* channel_ = nullptr;
    bool is_alive_ = false;

/**
 *
Узел 1: Асингулятор сокета async_wait_socket
Это сердце нашей интеграции. Именно этот метод заменяет блокирующий Си-вызов select() из примера автора библиотеки.
- Как это понимает планировщик: Конструкция co_await socket_.async_wait мгновенно приостанавливает текущую корутину.
 Поток управления возвращается в io_context.run(). Процессор свободен — он может в этот момент обрабатывать нажатия
 клавиатуры. Как только сетевая карта получает пакет, Asio возобновляет корутину ровно с того места, где она заснула.
- Зачем нужен asio::as_tuple: По умолчанию Asio при ошибках или отменах (например, если связь оборвалась) выбрасывает
 тяжелое C++ исключение (throw). Обертка as_tuple заставляет метод вместо генерации исключения просто вернуть кортеж
 с кодом ошибки [ec]. Программа работает быстрее и стабильнее.
 * @return asio::awaitable<void>
 *         Возвращает асингую задачу (управляющий фрейм корутины) без возвращаемого значения (void).
 *         Для вызывающей корутины это означает:
 *         1. В процессе работы: приостанавливает текущую ветку выполнения (yield), освобождая поток ОС.
 *         2. По завершении: сигнализирует о том, что сокет стал готов к вводу/выводу (или произошла ошибка),
 *            разрешая вызывающему циклу продолжить работу со следующей строчки кода.

 */
    asio::awaitable<void> async_wait_socket() {
        // 1. Спрашиваем у libssh2: "В каком направлении ты сейчас забуксовала?"
        int dir = libssh2_session_block_directions(session_);
        boost::system::error_code active_ec; // Выносим переменную наружу, чтобы проверить её после wait


        if (dir & LIBSSH2_SESSION_BLOCK_INBOUND) {
            // 2. Если ей нужно читать, мы говорим Boost.Asio:
            // "Засни и жди, пока в TCP-сокет не прилетят новые байты от сервера"
            // as_tuple заставит async_wait вернуть [boost::system::error_code] вместо генерации throw
            std::tie(active_ec) = co_await socket_.async_wait(
                    tcp::socket::wait_read,
                    asio::as_tuple(asio::use_awaitable)
            );
        } else if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND) {
            // 3. Если ей нужно писать, мы говорим Boost.Asio:
            // "Засни и жди, пока сетевой буфер ОС не освободится для отправки пакета"
            std::tie(active_ec) = co_await socket_.async_wait(
                    tcp::socket::wait_write,
                    asio::as_tuple(asio::use_awaitable)
            );
        }

        // ОБРАБОТКА ОШИБКИ: Если Asio сообщает, что сокет упал или операция отменена таймером
        if (active_ec) {
            // Выставляем флаг, что канал мертв. Это мгновенно остановит
            // внешние циклы while(is_alive()) в корутинах чтения и записи!
            is_alive_ = false;
        }
    }


public:
    std::function<void(std::string_view)> on_data_callback = nullptr;

    asio_ssh_channel(tcp::socket& socket) : socket_(socket) {}
    ~asio_ssh_channel() { close(); }

    bool is_alive() const { return is_alive_ && socket_.is_open(); }


    /**
     * Именно здесь происходит магия превращения сырого TCP-соединения в защищенный, шифрованный крипто-канал libssh2.
     * @param user
     * @param pass
     * @return asio::awaitable<bool> Асинхронную задачу, которая при завершении выдает логическое значение true (успех) или false (ошибка).
     */
    asio::awaitable<bool> async_init(const std::string& user, const std::string& pass) {
        // 1. Создаем и инициализируем объект SSH-сессии внутри библиотеки libssh2.
        // Если память выделить не удалось, возвращаем false.
        session_ = libssh2_session_init();
        if (!session_) co_return false;

        // 2. КРИТИЧЕСКИ ВАЖНО: Переводим сессию libssh2 в неблокирующий режим.
        // Теперь функции библиотеки не будут замирать в ожидании сети, а сразу вернут EAGAIN.
        libssh2_session_set_blocking(session_, 0);

        int rc;
        // 3. Запускаем процедуру SSH-Handshake (обмен ключами шифрования и баннерами).
        // Так как режим неблокирующий, мы крутимся в цикле, пока библиотека возвращает EAGAIN.
        //Обратите внимание на шаг №3: socket_.native_handle(). Старая Си-библиотека libssh2 ничего не знает про
        // красивые C++ объекты Boost.Asio. Ей нужен классический низкоуровневый дескриптор сокета (число),
        // который понимает операционная система Windows/Linux. Метод .native_handle() как раз «достает» этот
        // системный числовой идентификатор из объекта Boost.Asio и передает его в libssh2. При этом управление
        // сокетом остается за Asio.
        while ((rc = libssh2_session_handshake(session_, socket_.native_handle())) == LIBSSH2_ERROR_EAGAIN) {
            // Уходим в асингую спячку через Boost.Asio, пока сокет не будет готов передавать данные.
            co_await async_wait_socket();
        }
        // Если handshake завершился с ошибкой (не 0), пишем в лог и выходим.
        if (rc) {
            std::cerr << "[SSH] Handshake failed: " << rc << "\n";
            co_return false;
        }

        // Проходим официальную SSH-аутентификацию по паролю до открытия Shell
        // 4. Проходим криптографическую SSH-аутентификацию по логину и паролю.
        // Процедура также полностью неблокирующая и ждет готовности сети через Asio.
        while ((rc = libssh2_userauth_password(session_, user.c_str(), pass.c_str())) == LIBSSH2_ERROR_EAGAIN) {
            // Уходим в асингую спячку через Boost.Asio, пока сокет не будет готов передавать данные.
            co_await async_wait_socket();
        }
        // Если оборудование отклонило логин или пароль, сообщаем об этом.
        if (rc) {
            std::cerr << "[SSH] Аутентификация отклонена оборудованием.\n";
            co_return false;
        }

        // 5. Открываем виртуальный канал (Channel) внутри установленной SSH-сессии.
        // Через этот канал будет идти наше CLI-взаимодействие с коммутатором.
        while (!(channel_ = libssh2_channel_open_session(session_))) {
            if (libssh2_session_last_error(session_, nullptr, nullptr, 0) != LIBSSH2_ERROR_EAGAIN) co_return false;
            // Уходим в асингую спячку через Boost.Asio, пока сокет не будет готов передавать данные.
            co_await async_wait_socket();
        }

        // Запрашиваем xterm с правильным размером 160x40.
// Замена nullptr на "" и явная передача размеров помогает серверной стороне
// инициализировать стандартные параметры строк/колонок (Tty modes).
// 6. Запрашиваем у удаленного сервера псевдотерминал (PTY) типа "xterm".
        // Явно передаем геометрию экрана 160x40 символов, чтобы таблицы выводились в ровные столбцы.
// Запрашиваем геометрию 160x40.
// ВНИМАНИЕ: Если вывод сервера превысит 160 символов, он принудительно перенесет строку.
// Для универсальности в интерактивном режиме здесь можно выставить ширину 500.
// При переходе к автоматизации (скриптам) этот PTY-блок будет полностью удален,
// так как режим 'libssh2_channel_exec' автоматически отключает любые ограничения ширины.

        while ((rc = libssh2_channel_request_pty_ex(
                channel_,       // Указатель на канал
                "xterm",        // Тип эмуляции
                5,              // Длина строки "xterm"
                nullptr,        // Modes (не используем кастомные)
                0,              // Modes length
                160,            // Ширина (в символах)
                40,             // Высота (в строках)
                0,              // Ширина в пикселях
                0               // Высота в пикселях
        )) == LIBSSH2_ERROR_EAGAIN)
        {
            // Уходим в асингую спячку через Boost.Asio, пока сокет не будет готов передавать данные.
            co_await async_wait_socket();
        }

        // 7. Запускаем интерактивную командную оболочку (Shell) на коммутаторе.
        // С этого момента сервер готов принимать наши команды и слать ответы.
        while ((rc = libssh2_channel_shell(channel_)) == LIBSSH2_ERROR_EAGAIN) co_await async_wait_socket();

        // Фиксируем, что канал успешно поднят и готов к работе.
        is_alive_ = true;
        co_return true;
    }

    /**
     *
Узел 3: Асинхронная корутина отправки async_write

Тот самый узел, который исправил зависание клавиш и стрелочек.
     * @param data
     * @return asio::awaitable<void> Асинхронную задачу без передачи данных. Она служит исключительно маркером
     * синхронизации времени — приостанавливает поток и возобновляет его, когда сетевое событие наступило.
     */
    // Новый асинхронный и безопасный метод отправки
    asio::awaitable<void> async_write(std::string_view data) {
        if (!is_alive() || data.empty()) co_return;

        size_t bytes_written = 0;
        const char* ptr = data.data();
        size_t remaining = data.size();

        while (remaining > 0 && is_alive()) {
            // 1. Пропихиваем зашифрованный символ в канал libssh2
            ssize_t rc = libssh2_channel_write(channel_, ptr + bytes_written, remaining);

            if (rc > 0) {
                // 2. Если сервер принял байты (или часть байт) — сдвигаем указатель
                bytes_written += rc;
                remaining -= rc;
            }
            else if (rc == LIBSSH2_ERROR_EAGAIN) {
                // 3. КЛЮЧЕВОЙ МОМЕНТ: буфер забит! Мы не блокируем программу,
                // а засыпаем, давая корутине чтения обработать входящий поток.
                // Если буфер отправки SSH забит — асинхронно ждем освобождения через Asio
                co_await async_wait_socket();
            }
            else {
                // Любая другая ошибка означает смерть канала
                is_alive_ = false;
                co_return;
            }
        }
    }

    // Метод для регистрации (сохранения) пользовательского коллбека.
// std::function<void(std::string_view)> — это универсальный контейнер для нашей лямбды.
// std::move перемещает лямбду во внутреннюю переменную on_data_callback без лишнего копирования.
    void async_read(std::function<void(std::string_view)> callback) {
        on_data_callback = std::move(callback); }

    /**
     *
Узел 2: Асинхронный цикл чтения process_read

Этот метод работает в бесконечном цикле, пока сессия жива.
     • Логика работы: Большую часть времени этот цикл проводит на строчке co_await async_wait_socket(). Программа
     не тратит ресурсы CPU на постоянный опрос сокета в холостом цикле while.
Главная асинхронная корутина чтения, работающая в фоновом цикле сессии.
     * @return
     */
    asio::awaitable<void> process_read() {
        // Выделяем временный буфер объемом 4 КБ под один сетевой пакет данных.
        std::vector<char> buffer(4096);
        // Крутимся в цикле, пока сессия SSH активна.
        while (is_alive()) {
            // 1. Пытаемся прочитать расшифрованные байты из канала SSH
            // Пытаемся считать расшифрованные сервером байты через API libssh2.
            ssize_t nread = libssh2_channel_read(channel_, buffer.data(), buffer.size());

            if (nread > 0) {
                // 2. Если что-то прочитали — мгновенно отдаем в std::cout через коллбек
                // Если данные успешно прочитаны (nread содержит количество байт)
                // и у нас зарегистрирован коллбек — вызываем его, передавая "взгляд" на буфер.
                if (on_data_callback) on_data_callback(std::string_view(buffer.data(), nread));
            } else if (nread == LIBSSH2_ERROR_EAGAIN) {
                // 3. Если читать пока нечего (сеть пуста) — уходим в асинхронную спячку!
                // Если сеть пуста и читать пока нечего (libssh2 вернула EAGAIN),
                // мы асинхронно засыпаем через Boost.Asio, освобождая поток ОС для других задач.
                co_await async_wait_socket();
            } else {
                // 4. Если вернулся 0 или отрицательный код (не EAGAIN) — сессия закрылась
                // Любой другой код (например, 0 или критическая ошибка сети)
                // означает, что удаленный сервер закрыл соединение. Останавливаем цикл.
                is_alive_ = false;
            }
        }
    }

    void close() {
        is_alive_ = false;
        if (channel_) { libssh2_channel_free(channel_); channel_ = nullptr; }
        if (session_) { libssh2_session_disconnect(session_, "Liveness Shutdown"); libssh2_session_free(session_); session_ = nullptr; }
    }
};

// Выделенная корутина чтения SSH
asio::awaitable<void> read_from_ssh_device(asio_ssh_channel& channel, asio::steady_timer& supervisor_timer) {
    try {
        co_await channel.process_read();
    } catch (...) {}
    std::cout << "\n[Система] SSH сессия завершена.\n";
    channel.close();
    supervisor_timer.cancel();
}

// Выделенная корутина клавиатуры для SSH
asio::awaitable<void> handle_ssh_keyboard(asio_ssh_channel& channel) {
    auto executor = co_await asio::this_coro::executor;
    asio::steady_timer timer(executor);
    std::cout << "[Режим интерактивного ввода SSH активирован]\n";

    try {
        while (channel.is_alive()) {
            if (_kbhit()) {
                int ch = _getch();
                std::string send_str = "";

                if (ch == 0 || ch == 224) {
                    int ext = _getch();
                    if (ext == 72) send_str = "\x1B[A";      // ВВЕРХ
                    else if (ext == 80) send_str = "\x1B[B"; // ВНИЗ
                    else if (ext == 77) send_str = "\x1B[C"; // ВПРАВО
                    else if (ext == 75) send_str = "\x1B[D"; // ВЛЕВО
                }
                else if (ch == '\r' || ch == '\n') send_str = "\r\n";
                else if (ch == 8) send_str = "\x7F"; // Backspace
                else send_str = static_cast<char>(ch);

                // Вызываем отправку асинхронно через co_await
                if (!send_str.empty()) {
                    co_await channel.async_write(send_str);
                }
            }
            timer.expires_after(std::chrono::milliseconds(10));
            co_await timer.async_wait(asio::use_awaitable);
        }
    } catch (...) {}
}

/**
 * // Главная управляющай корутина сессии. Логика управления сессией
 * @param socket
 * @param user
 * @param pass
 * @return
 */

asio::awaitable<void> ssh_session_logic(tcp::socket& socket, std::string user, std::string pass) {
    // 1. Получаем текущий экзекутор (планировщик потока) для корутины.
    //Экзекутор (Executor) — это внутренний менеджер потока от Boost.Asio. Он помнит, в каком именно потоке сейчас
    //выполняется эта корутина. Мы сохраняем его в переменную executor, чтобы в дальнейшем создавать асинхронные
    // таймеры сессии (например, keep_alive_timer) строго в этом же самом контексте. Конструкция
    // co_await asio::this_coro::executor асинхронно запрашивает этот маркер у планировщика.
    auto executor = co_await asio::this_coro::executor;
    // 2. Создаем экземпляр нашего SSH-моста, передавая ему ссылку на TCP-сокет.
    asio_ssh_channel channel{ socket };

    std::cout << "[SSH] Установка защищенного соединения...\n";

    //запускает скрытые механизмы шифрования libssh2
    /*
     * Ключевое слово co_await — это команда для компилятора C++20, которая говорит: «Эта операция выполняется
     * асинхронно по сети и может занять время. Не блокируй поток ОС! Если сервер еще не ответил, поставь эту
     * корутину на паузу, займись другими задачами, а когда ответ придет — вернись и продолжи с этого же места».
       Если убрать co_await, программа попытается пойти дальше, не дождавшись завершения авторизации. В итоге мы попытаемся
     читать из еще не созданного канала, и программа мгновенно упадет.
     */
    if (!co_await channel.async_init(user, pass)) {
        std::cerr << "[Ошибка] Не удалось авторизоваться или открыть Shell.\n";
        co_return;
    }

    // Регистрируем обработчик (коллбек) для входящих текстовых данных от SSH-сервера.
// Мы передаем в метод async_read лямбда-функцию [ ](std::string_view text).
// Как только из сети прилетит очередной кусок данных (чанк), наш канал автоматически
// вызовет эту лямбду, передав в нее чистый текст (text), который мы мгновенно выводим на экран.
    channel.async_read([](std::string_view text) {
        if (!text.empty()) {
            std::flush(std::cout);
            std::cout << text << std::flush;
        }
    });

// 1. Создаем асинхронный таймер-супервизор, привязанный к нашему потоку выполнения (executor).
    // Этот таймер будет удерживать главную корутину от досрочного завершения, пока идет сессия.
    asio::steady_timer keep_alive_timer(executor);

    // 2. Асинхронно разветвляем (co_spawn) выполнение: запускаем фоновую корутину чтения из сети.
    // read_from_ssh_device будет крутиться в своем цикле while, вылавливая байты от коммутатора.
    // Передаем ей ссылку на таймер keep_alive_timer, чтобы она могла "разбудить" нас при отключении.
    asio::co_spawn(executor, read_from_ssh_device(channel, keep_alive_timer), asio::detached);
    // 3. Асинхронно разветвляем выполнение вторично: запускаем фоновую корутину опроса клавиатуры.
    // handle_ssh_keyboard будет непрерывно сканировать нажатия кнопок и отправлять их в async_write.
    asio::co_spawn(executor, handle_ssh_keyboard(channel), asio::detached);

    try {
        // 4. Устанавливаем время ожидания таймера-супервизора на максимально возможное (бесконечность).
        keep_alive_timer.expires_at(std::chrono::steady_clock::time_point::max());
        // 5. КЛЮЧЕВАЯ ТОЧКА ЗАПИСИ: Главная корутина сессии засыпает на этой строчке.
        // Она освобождает поток и смиренно ждет, пока кто-нибудь принудительно не отменит этот таймер.
        co_await keep_alive_timer.async_wait(asio::use_awaitable);
    } catch (const boost::system::system_error& e) {
        // 6. Перехватываем событие отмены. Если код ошибки равен operation_aborted,
        // значит таймер был намеренно сброшен из корутины чтения (случился дисконнект).
        // Это штатная ситуация, мы просто пропускаем её и идем на корректное закрытие.
        if (e.code() != asio::error::operation_aborted) throw;
    }
    std::cout << "[Система] Главная SSH корутина завершена.\n";
}

int main() {

#ifdef _WIN32
    // Устанавливаем кодировку UTF-8 (код 65001) для вывода текста на экран консоли.
    // Это устраняет кракозябры при чтении русских букв, приходящих от сервера.
    SetConsoleOutputCP(65001);

    // Устанавливаем кодировку UTF-8 (код 65001) для ввода текста с клавиатуры.
    // Это гарантирует, что вводимые нами символы кодируются так же, как их ожидает сервер.
    SetConsoleCP(65001);

    // Получаем низкоуровневый дескриптор (хэндл) стандартного вывода текущей консоли.
    // Этот хэндл нужен операционной системе, чтобы изменить внутренние настройки экрана.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // Проверяем, что хэндл консоли успешно получен и система не вернула ошибку.
    if (hOut != INVALID_HANDLE_VALUE) {
        // Создаем переменную для хранения текущего флага-маски режимов консоли.
        DWORD dwMode = 0;
        // Запрашиваем у Windows текущие настройки консоли и записываем их в dwMode.
        // Если функция вернула true (успех), значит, мы можем их модифицировать.
        if (GetConsoleMode(hOut, &dwMode)) {
            // Включаем аппаратную обработку виртуального терминала + обработку вывода
            // Включаем обработку ANSI-последовательностей (Esc-кодов).
            // Благодаря этому консоль Windows аппаратно понимает команды сервера
            // о цвете текста, очистке экрана и перемещении курсора (история команд).
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            // Включаем обработку служебных управляющих символов вывода.
            // Этот флаг заставляет Windows корректно интерпретировать такие символы,
            // как Backspace (удаление), перевод строки (\n) и возврат каретки (\r).
            dwMode |= ENABLE_PROCESSED_OUTPUT;
            // Записываем обновленную маску режимов обратно в настройки ОС Windows.
            // С этого момента консоль начинает работать в режиме умного терминала.
            SetConsoleMode(hOut, dwMode);
        }
        else {
            std::cerr << "[Внимание Windows] Не удалось прочитать режимы консоли. Возможно искажение текста.\n";
        }
    }
    else {
        // ОБРАБОТКА ОШИБКИ: Сигнализируем, что стандартный вывод недоступен
        std::cerr << "[Внимание Windows] Стандартный вывод консоли недоступен (INVALID_HANDLE_VALUE).\n"
                  << "Программа запущена в фоне или вывод перенаправлен в файл.\n";
    }

#endif


    // 1. Запрашиваем данные до старта асинхронного движка
    std::string ip = "192.168.0.101"; // Укажите IP вашего TFortis
    std::string user, pass;
    std::cout << "Введите логин SSH: "; std::cin >> user;
    std::cout << "Введите пароль SSH: ";

    // Скрытие ввода символов пароля (опционально, для простоты обычный cin)
    std::cin >> pass;

    // 2. Инициализируем глобальный контекст libssh2
    // Инициализируем глобальный криптографический контекст библиотеки libssh2.
// Вызывается строго один раз при старте приложения до создания сокетов.
// Передача '0' означает запуск со стандартными системными настройками.
    if (libssh2_init(0)) {
        std::cerr << "Libssh2 init failed\n";
        return 1;
    }

    try {
        // Главное «сердце» и движок асинхронного приложения.
// Этот объект управляет циклом обработки всех сетевых событий, таймеров и корутин.
// Он распределяет задачи и следит за тем, чтобы программа работала в одном потоке ОС без блокировок.
        asio::io_context io_context;
        // Асинхронный преобразователь сетевых адресов (ДНС-резолвер).
// Он отвечает за перевод понятных человеку адресов (например, "tfortis.local" или "192.168.0.101")
// и текстовых имен портов (например, "ssh" или "22") в понятный для операционной системы цифровой IP-адрес.
        tcp::resolver resolver(io_context);
        // Объект сетевого TCP-сокета, привязанный к нашему движку io_context.
// Через него происходит реальный прием и отправка байтов по сети.
// В отличие от обычных сокетов, он не блокирует программу, а умеет засыпать и просыпаться асинхронно.
        tcp::socket socket(io_context);

        std::cout << "Подключение к " << ip << ":22...\n";

        // Синхронно подключаем TCP-сокет к удаленному серверу по найденному адресу и порту 22.
// Функция заблокирует поток выполнения только на этапе первоначального TCP-рукопожатия,
// гарантируя, что физическое соединение успешно установлено до старта асинхронного движка.
        asio::connect(socket, resolver.resolve(ip, "22"));
        std::cout << "TCP сокет подключен. Запуск корутин...\n";

        // Планируем и запускаем главную управляющую корутину SSH-сессии (ssh_session_logic).
// asio::co_spawn берет нашу корутину и регистрирует ее внутри движка io_context.
// asio::detached указывает, что мы запускаем корутину независимо и не ждем от нее возвращаемого значения.
        asio::co_spawn(io_context, ssh_session_logic(socket, user, pass), asio::detached);
        // Запускаем бесконечный цикл обработки событий асинхронного движка.
// Этот вызов блокирует текущий поток ОС, отдавая его под выполнение всех зарегистрированных корутин и таймеров.
// Функция завершит работу только тогда, когда закроются все сетевые сокеты и остановятся таймеры.
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << "\n";
    }

    libssh2_exit();
    return 0;
}

/**
 *
Физика процесса: Как это работает параллельно в одном потоке?

В обычной многопоточности (Threads) операционная система в любой случайный момент может прервать один поток и переключить процессор на другой. Из-за этого возникают гонки данных, и коду нужны тяжелые мьютексы (std::mutex).
В наших C++20 корутинах (кооперативная многозадачность) переключение происходит добровольно и строго в точках, где написано ключевое слово co_await.
Вот как выглядит их ежесекундный «танец» внутри io_context:
1. Шаг Главной коруниты: Она доходит до строки co_await keep_alive_timer.async_wait(...), регистрирует у планировщика намерение спать бесконечно, отдает управление потоком обратно движку io_context и полностью засыпает.
2. Шаг Корутины Клавиатуры: Планировщик видит, что активна корутина handle_ssh_keyboard. Она просыпается, проверяет функцию _kbhit(). Если вы ничего не нажали, она доходит до своей строчки co_await timer.async_wait(10ms). Она говорит планировщику: «Я засыпаю на 10 миллисекунд» — и добровольно возвращает поток движку.
3. Шаг Корутины Чтения: В этот же миг планировщик передает поток корутине read_from_ssh_device. Она делает вызов сети, видит, что коммутатор ничего не прислал (вернулся EAGAIN), доходит до строки co_await async_wait_socket() и говорит: «Я засыпаю, пока в сокете не появятся байты». Поток снова свободен!
4. Внезапное событие (Вы нажали клавишу): Проходит 10 мс. Планировщик будит клавиатуру. Она видит символ, вызывает co_await channel.async_write(). Байты улетают коммутатору. Клавиатура снова засыпает на 10 мс.
5. Внезапное событие (Сервер прислал ответ): Сетевая карта Windows получает пакет от коммутатора. Планировщик io_context мгновенно видит, что сокет готов к чтению. Он будит корутину чтения. Она просыпается со строчки async_wait_socket(), забирает текст, выводит его на экран через коллбек и снова засыпает в ожидании сети.

Как работает сценарий отключения (Supervisor Timer)?

Что произойдет, если коммутатор TFortis внезапно перезагрузится или сетевой кабель выдернут?
1. Корутина чтения read_from_ssh_device проснется, потому что сокет изменит состояние.
2. Метод libssh2_channel_read вернет не EAGAIN, а 0 (канал закрыт) или ошибку сети.
3. Цикл while(is_alive()) внутри метода чтения завершится. Выполнение выйдет из асинхронного метода наружу — в тело корутины read_from_ssh_device.
4. И там сработает спасительная строчка:cpp
supervisor_timer.cancel(); // Мгновенно будим супервизор
Используйте код с осторожностью.
5. Этот вызов мгновенно «взрывает» бесконечный сон главной корутины. Планировщик шлет ей сигнал отмены operation_aborted.
6. Главная корутина пулей вылетает из спячки keep_alive_timer.async_wait, проскакивает блок catch, выполняет деструктор класса ~asio_ssh_channel() (который чисто закрывает за собой сессию libssh2) и красиво завершает программу.
Никаких зависших процессов или зомби-потоков в системе не остается!
 */

/*
 *
Вопрос 1: Должны ли функции, которые запускают другие корутины, сами быть корутинами?

Да, обязательно. В C++20 действует «золотое правило» корутин: если внутри функции есть ключевое слово co_await (или co_return), эта функция автоматически обязана быть корутиной и возвращать специальный тип (в нашем случае asio::awaitable<...>).
Вы абсолютно правы в своей цепочке:
• process_read использует co_await async_wait_socket(), поэтому она обязана быть корутиной.
• read_from_ssh_device вызывает co_await channel.process_read(), поэтому она тоже обязана быть корутиной.
Если бы мы попытались сделать read_from_ssh_device обычной функцией, мы бы физически не смогли написать внутри неё co_await. Компилятор сразу выдал бы жесткую ошибку. co_await — это триггер, который заставляет компилятор полностью перестроить функцию в стейт-машину.

Вопрос 2: Почему мы регистрируем через co_spawn только три корутины, а не все их?

В этом кроется главное различие между «точками ветвления» (co_spawn) и «последовательным ожиданием» (co_await).
Когда мы пишем asio::co_spawn, мы создаем новую независимую параллельную задачу (в мире потоков это аналог создания нового потока std::thread). Эти задачи живут своей жизнью, планировщик переключается между ними, и они никак не зависят от судьбы друг друга (пока мы не свяжем их таймером). Мы зарегистрировали ровно три независимых ветки:
1. Контролер сессии (ssh_session_logic)
2. Приемщик сети (read_from_ssh_device)
3. Опросчик клавиатуры (handle_ssh_keyboard)
А вот когда корутина вызывает другую корутину через co_await (например, read_from_ssh_device вызывает process_read), новая параллельная ветка не создается!
Вместо этого текущая ветка просто «углубляется». Корутина read_from_ssh_device засыпает и передает управление корутине process_read. Планировщику io_context не нужно знать про process_read отдельно, потому что она является частью уже зарегистрированной ветки чтения! Планировщик управляет верхушкой этой ветки.
Аналогия: co_spawn — это покупка нового независимого станка на завод. А co_await — это когда рабочий на этом станке временно переключается на использование другого внутреннего инструмента. Заводу (io_context) нужно контролировать только количество станков, а не инструменты внутри них.

Вопрос 3: Правильно ли подсчитано количество постоянно работающих корутин?

Да, ваш подсчет идеален. Давайте зафиксируем эту структуру, чтобы вы могли добавить её в виде итогового комментария-схемы в свой код:
В процессе активной работы в контексте планировщика находятся ровно 4 постоянно удерживаемых в памяти фрейма корутин:
1. 🟢 ssh_session_logic — Постоянная. Спит на строчке co_await keep_alive_timer.async_wait(). Проснется только при закрытии программы.
2. 🟢 read_from_ssh_device — Постоянная. Спит на строчке co_await channel.process_read(), ожидая, пока внутренний метод завершит свой бесконечный цикл.
3. 🟢 process_read — Постоянная. Находится внутри read_from_ssh_device. Постоянно бегает по циклу while(is_alive) и большую часть времени спит на co_await async_wait_socket(), ожидая новые байты из сети.
4. 🟢 handle_ssh_keyboard — Постоянная. Бегает по своему циклу while(channel.is_alive()), просыпаясь каждые 10 миллисекунд для проверки нажатия клавиш.

А что с остальными?

• async_init — выполнилась один раз при старте, дошла до co_return true и полностью уничтожилась из памяти.
• async_wait_socket — Временная. Создается корутиной process_read (или async_write), засыпает на время ожидания физического пакета от сетевой карты, а как только пакет пришел — мгновенно уничтожается до следующего раза.
• async_write — Временная. Рождается только в ту миллисекунду, когда вы нажали клавишу, проталкивает байт в libssh2 и тут же исчезает.
 */

/*
 *
Почему жестко заданный размер — это ловушка?

Когда вы подключаетесь к устройству (коммутатору TFortis, Cisco, Linux-серверу) через SSH с запросом PTY (псевдотерминала), удаленная операционная система строго адаптирует свой вывод под переданные вами размеры.
• Если команда генерирует строку в 180 символов, а вы передали 160, сервер сам вставит символ переноса строки (\n) на 160-м символе.
• Хуже того, в интерактивном режиме с PTY сервер автоматически включает пагинацию — постраничный вывод. Дойдя до 40-й строки (размер высоты), коммутатор остановит вывод и напишет --More-- или [Press Space to continue]. Роботу-автоматизатору придется программно «нажимать пробел», чтобы читать дальше, что сильно усложняет код.

Стратегическое решение проблемы для автоматизации

Поскольку вы делаете подготовительную работу для написания робота, который будет автоматически собирать данные, у нас есть два абсолютно надежных инженерных пути:

Путь 1. Универсальный интерактивный вариант (Максимальный запас)

Если вам нужно оставить именно интерактивный Shell, то вместо 160 и 40 в функцию libssh2_channel_request_pty_ex передают огромный размер с запасом, который заведомо шире любой возможной таблицы, и бесконечную высоту:
cpp
// Выставляем ширину 500 символов (хватит для любой таблицы FDB/ARP)
// и высоту 10000 строк (чтобы сервер никогда не включал пагинацию --More--)
libssh2_channel_request_pty_ex(channel_, "xterm", 5, nullptr, 0, 500, 10000, 0, 0)
Используйте код с осторожностью.
Ни один коммутатор в мире не генерирует строки шире 500 символов, поэтому текст гарантированно прилетит в виде ровных «плоских» строк без искусственных переносов.

Путь 2. Идеальный робот-вариант (Вообще убрать PTY)

Как только вы закончите с интерактивным режимом и начнете писать код автоматического сбора, вы полностью уберете вызовы PTY и Shell из кода (libssh2_channel_request_pty_ex и libssh2_channel_shell будут просто удалены).
Вместо этого вы будете использовать прямой запуск команд через libssh2_channel_exec(channel_, "show fdb").
• Почему это решает проблему: Когда команда запускается напрямую без PTY, удаленный коммутатор понимает, что с ним общается не человек через терминал, а скрипт. В этом режиме коммутатор физически отключает любые ограничения по ширине и высоте. Он выдаст всю таблицу FDB целиком — одной сплошной простыней, где каждая строка будет идти до своего логического конца, будь она хоть 1000 символов в длину, и без всяких промежуточных --More--.

Резюме для комментариев в коде:

Зафиксируйте этот нюанс в коде рядом с вызовом libssh2_channel_request_pty_ex:
cpp
// Запрашиваем геометрию 160x40.
// ВНИМАНИЕ: Если вывод сервера превысит 160 символов, он принудительно перенесет строку.
// Для универсальности в интерактивном режиме здесь можно выставить ширину 500.
// При переходе к автоматизации (скриптам) этот PTY-блок будет полностью удален,
// так как режим 'libssh2_channel_exec' автоматически отключает любые ограничения ширины.

 */

/*
 *
Правильное техническое описание для комментариев

В C++20 корутинах тип asio::awaitable<T> возвращает не результат работы здесь и сейчас, а асинхронную задачу (промис).
Когда тип внутри указан как <void>, это означает, что корутина по окончании своей работы не передает обратно никаких данных (например, чисел или строк), но сам факт её завершения является сигналом для планировщика: «Задача выполнена, можно будить вызывающую корутину».
Вот эталонный блок описания, который вы можете вставить перед функцией async_wait_socket:
cpp
/**
 * @brief Асинхронное ожидание готовности системного сокета.
 *
 * @param[in]  (Отсутствуют) Метод использует внутренние приватные переменные класса: socket_ и session_.
 *
 * @return asio::awaitable<void>
 *         Возвращает асингую задачу (управляющий фрейм корутины) без возвращаемого значения (void).
 *         Для вызывающей корутины это означает:
 *         1. В процессе работы: приостанавливает текущую ветку выполнения (yield), освобождая поток ОС.
 *         2. По завершении: сигнализирует о том, что сокет стал готов к вводу/выводу (или произошла ошибка),
 *            разрешая вызывающему циклу продолжить работу со следующей строчки кода.
 */
/*asio::awaitable<void> async_wait_socket() {
    // ...
}
Используйте код с осторожностью.

Как это сопоставить с другими функциями для вашей документации?

Чтобы у вас была единая и стройная система описания всех функций в коде, используйте это простое правило:
1. Если функция возвращает asio::awaitable<bool> (как наш async_init):
• Возвращает: Асинхронную задачу, которая при завершении выдает логическое значение true (успех) или false (ошибка).
2. Если функция возвращает asio::awaitable<void> (как async_wait_socket или process_read):
• Возвращает: Асинхронную задачу без передачи данных. Она служит исключительно маркером синхронизации времени — приостанавливает поток и возобновляет его, когда сетевое событие наступило.
*/

#endif



//Telnet, v.3
#if 0

#include <iostream>
#include <vector>
#include <string>
#include <conio.h>

#include <boost/asio.hpp>
#include <telnetpp/session.hpp>
#include <telnetpp/options/echo/client.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

//Узел 1: Класс-мост asio_channel
//Этот класс адаптирует сокет Boost.Asio под требования библиотеки telnetpp (так называемый "концепт потока").
struct asio_channel {
    // Ссылка на сетевой TCP-сокет. Объект запрещено копировать, работаем напрямую.
    tcp::socket& socket;
    // Хранилище для указателя на парсер библиотеки Telnet++.
    // Сюда сохраняется функция, умеющая вырезать служебные Telnet-команды из потока байт.
    std::function<void(telnetpp::bytes)> telnet_parser_callback = nullptr;

    // Синхронная отправка (требуется концептом Telnet++). Вызывается внутренними механизмами Telnet++.
    void write(telnetpp::bytes data) {
        if (!data.empty() && is_alive()) {
            boost::system::error_code ec;
            // Аппаратно проталкиваем байты в сокет.
            // Перегрузка с 'ec' гасит исключения, если связь оборвалась в момент отправки.
            asio::write(socket, asio::buffer(data.data(), data.size()), ec);
            // Использование перегрузки с error_code предотвращает генерацию
            // исключений, если сокет оборвался прямо во время отправки
        }
    }

    // Регистрация парсера библиотеки. Сохраняем (регистрируем) обработчик парсера библиотеки для дальнейшего использования.
    void async_read(std::function<void(telnetpp::bytes)> callback) {
        telnet_parser_callback = std::move(callback);
    }

    // Проверка статуса сокета для концепта Telnet++. Проверяем, открыт ли системный сетевой сокет (жив ли канал связи).
    bool is_alive() const {
        return socket.is_open();
    }

    // Корректное закрытие сокета в стиле Boost.Asio
    void close() {
        if (socket.is_open()) {
            boost::system::error_code ec;
            // 1. Отключаем отправку и прием (Graceful Shutdown). Извещаем ОС, что прекращаем и прием, и передачу (Graceful Shutdown).
            socket.shutdown(tcp::socket::shutdown_both, ec);
            // 2. Освобождаем системный дескриптор. Полностью освобождаем дескриптор сокета в ОС.
            socket.close(ec);
        }
    }
};




namespace asio = boost::asio;
using asio::ip::tcp;

//Узел 2: Фоновая корутина чтения read_from_device
//Эта корутина постоянно работает в памяти и отвечает за «вычерпывание» байтов из сетевой карты.
// Корутина для асинхронного чтения из сокета оборудования. Фоновая корутина для асинхронного чтения сырых байтов из сокета оборудования.
asio::awaitable<void> read_from_device(tcp::socket& socket, asio_channel& channel, asio::steady_timer& supervisor_timer) {
    // Временное хранилище для одного пакета данных объемом 4 КБ.
    std::vector<char> recv_buffer(4096);

    try {
        while (channel.is_alive()) {
            // КЛЮЧЕВАЯ ТОЧКА: Корутина приостанавливает поток ОС и ждет появления байт в сети.
            std::size_t bytes_received = co_await socket.async_read_some(
                    asio::buffer(recv_buffer),
                    asio::use_awaitable
            );

            if (bytes_received > 0) {
                // Обертываем сырой буфер в тип 'bytes' для парсера Telnet++.
                telnetpp::bytes view{
                        reinterpret_cast<uint8_t*>(recv_buffer.data()),
                        bytes_received
                };

                // Отдаем пакет парсеру библиотеки. Он вырежет служебные опции
                // и перенаправит чистый текст в наш экранный коллбек.
                // Защитная проверка: вызываем парсер библиотеки только в том случае,
// если он был успешно зарегистрирован. Предотвращает аварийное завершение
// программы (crash), если данные из сети прилетели до завершения инициализации.
                if (channel.telnet_parser_callback) {
                    channel.telnet_parser_callback(view);
                }
            }
        }
    }
    catch (const boost::system::system_error& e) {
        // Проверяем код ошибки. Если это EOF (нормальное закрытие)
        // или connection_reset (принудительный выход через exit)
        // Перехватываем штатные сетевые коды отключения удаленного узла (коммутатора).
        if (e.code() == asio::error::eof || e.code() == asio::error::connection_reset) {
            std::cout << "\n[Система] Сессия Telnet завершена коммутатором.\n";
        }
        else {
            // Выводим только реальные, критические сетевые сбои
            // Фиксируем реальный критический сбой на физическом уровне сети.
            std::cerr << "\n[Критическая ошибка сети]: " << e.code().message() << " (Код: " << e.code().value() << ")\n";
        }
    }

    // Сокет закрылся. Мгновенно будим главную корутину-супервизор!
    // Соединение разорвано. Принудительно очищаем ресурсы канала.
    channel.close();
    // ВЗРЫВАЕМ СУПЕРВИЗОР: Будим главную корутину, выводя ее таймер из спячки.
    supervisor_timer.cancel(); // Этот вызов прервет сон таймера немедленно
}
//Узел 3: Корутина клавиатуры handle_keyboard
//Отвечает за посимвольный опрос кнопок и формирование ANSI-команд для стрелочек терминала.
// Корутина для обработки клавиатуры
// Фоновая корутина для неблокирующего сканирования нажатий кнопок.
asio::awaitable<void> handle_keyboard(tcp::socket& socket, telnetpp::session& session) {
    // Связываем локальный таймер корутины с текущим потоком исполнения.
    auto executor = co_await asio::this_coro::executor;
    asio::steady_timer timer(executor);

    std::cout << "[Режим интерактивного ввода активирован (Boost.Asio 1.87)]\n";

    try {
        while (session.is_alive()) {
            // Неблокирующая проверка WinAPI: нажата ли клавиша прямо сейчас?
            if (_kbhit()) {
                int ch = _getch();
                std::string send_str = "";

                // Проверка на расширенные клавиши Windows (стрелки имеют префикс 0 или 224)
                if (ch == 0 || ch == 224) {
                    int extended_ch = _getch();
                    if (extended_ch == 72)      send_str = "\x1B[A"; // ВВЕРХ
                    else if (extended_ch == 80) send_str = "\x1B[B"; // ВНИЗ
                    else if (extended_ch == 77) send_str = "\x1B[C"; // ВПРАВО
                    else if (extended_ch == 75) send_str = "\x1B[D"; // ВЛЕВО
                }
                else if (ch == '\r' || ch == '\n') {
                    send_str = "\r\n";// Принудительный перевод в сетевой стандарт конца строки
                }
                else if (ch == 8) {
                    send_str = "\x7F"; // Delete / Backspace // Код удаления символа (Backspace)
                }
                else {
                    send_str = static_cast<char>(ch);// Обычный печатный символ
                }

                if (!send_str.empty()) {
                    // Преобразуем строку в контейнер байт Telnet++
                    telnetpp::bytes input_view{
                            reinterpret_cast<uint8_t const*>(send_str.data()),
                            send_str.size()
                    };
                    // Отправляем байты в сессию (уйдет по сети на коммутатор)
                    session.write(input_view);
                }
            }

            // Небольшая асинхронная пауза, чтобы не грузить процессор в цикле _kbhit
            // МАКСИМАЛЬНО ВАЖНО: Асингуая пауза на 10 мс.
            // Предотвращает 100% загрузку ядра процессора бесконечным циклом опроса кнопок.
            timer.expires_after(std::chrono::milliseconds(10));
            co_await timer.async_wait(asio::use_awaitable);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\n[Ошибка ввода]: " << e.what() << "\n";
    }
}

//Узел 4: Главный диспетчер сессии telnet_session_logic
//Управляет бизнес-логикой сессии и удерживает контекст выполнения.

    // Главная корутина управления сессией (Ваш улучшенный вариант)
    // Управляющая корутина жизненного цикла Telnet-соединения.
asio::awaitable<void> telnet_session_logic(tcp::socket& socket) {
        // Связываем наш мост с TCP сокетом.
    asio_channel channel{ socket };
        // Инициализируем высокоуровневый движок сессии протокола Telnet.
    telnetpp::session session(channel);

        // Регистрируем лямбда-обработчик чистого текста (после фильтрации Telnet-опций)
        session.async_read([&](telnetpp::bytes data) {
        if (!data.empty()) {
            std::string clean_text(reinterpret_cast<char const*>(data.data()), data.size());
            std::cout << clean_text << std::flush; // Мгновенный вынос текста на консоль ПК
        }
    });

        // Настраиваем обязательный протокольный плагин (Echo option).
        // Он заставляет сервер TFortis пересылать нам обратно вводимые символы (эхо).
        telnetpp::options::echo::client echo_client{session};
    session.install(echo_client);
    echo_client.activate();

    std::cout << "Сессия Telnet инициирована. Ожидание TFortis...\n\n";

        // Получаем маркер текущего потока ОС.
    auto executor = co_await asio::this_coro::executor;
// Создаем бесконечный таймер-держатель (супервизор).
        asio::steady_timer keep_alive_timer(executor);

    // ТЕПЕРЬ МЫ РЕГИСТРИРУЕМ ОБЕ КОРУТИНЫ РАВНОПРАВНО!
    // Они обе уходят в планировщик io_context как независимые фоновые задачи
        // Запускаем чтение, передавая таймер по ссылке
        // Разветвляем выполнение: запускаем параллельные задачи чтения и ввода в планировщик.
        asio::co_spawn(executor, read_from_device(socket, channel, keep_alive_timer), asio::detached);
    asio::co_spawn(executor, handle_keyboard(socket, session), asio::detached);

    // А как теперь "заснуть" самой родительской корутине?
    // Мы просто асинхронно ждем, пока канал активен.
    // Для этого можно использовать простой асинхронный таймер, который проверяет liveness,
    // либо сделать бесконечный цикл ожидания, который не тратит CPU:
        try {
            // Выставляем таймер на "бесконечность"
            keep_alive_timer.expires_at(std::chrono::steady_clock::time_point::max());

            // Корутина засыпает навсегда. Она проснется ТОЛЬКО тогда,
            // когда read_from_device вызовет keep_alive_timer.cancel()
            // Уводим родительскую корутину в бесконечный сон, защищая сессию от досрочного вылета.
            co_await keep_alive_timer.async_wait(asio::use_awaitable);
        }
        catch (const boost::system::system_error& e) {
            // operation_aborted означает, что таймер был успешно отменен извне.
            // Для нас это зеленый свет: сессия завершена, выходим мгновенно!
            // Перехватываем сигнал принудительной отмены сна таймера из корутины чтения.
            if (e.code() != asio::error::operation_aborted) {
                throw;// Если это не отмена, пробрасываем ошибку дальше
            }
        }

        std::cout << "\n[Система] Главная управляющая корутина мгновенно завершила работу.\n";
}


int main() {
    try {
        std::string ip_address = "nethack.alt.org"; //10.178.18.11 nethack.alt.org
        std::string port = "23";

        // Главный управляющий контекст Boost.Asio
        asio::io_context io_context;

        // Резолвер и сокет в стиле Boost 1.87
        tcp::resolver resolver(io_context);
        tcp::socket socket(io_context);

        std::cout << "Подключение к оборудованию " << ip_address << "...\n";

        // Синхронное подключение для простоты старта (можно переписать на асинхронное)
        asio::connect(socket, resolver.resolve(ip_address, port));
        std::cout << "Успешно подключено к оборудованию!\n\n";

        // Запускаем главную корутину автоматизации/ввода
        asio::co_spawn(io_context, telnet_session_logic(socket), asio::detached);

        // Запускаем цикл обработки событий.
        // Теперь ВСЯ программа работает внутри этого вызова, используя ровно 1 поток!
        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << "\n";
    }

    return 0;
}

/*
 *
1. Защита от неопределенного поведения (Undefined Behavior)

Переменная telnet_parser_callback имеет тип std::function. Если вызвать пустой std::function (который равен nullptr), программа не просто выдаст ошибку, она мгновенно упадет с тяжелым системным исключением std::bad_function_call и аварийно завершится (crash).
Проверка if (channel.telnet_parser_callback) гарантирует, что даже при самом странном сбое вызова пустой функции не произойдет.

2. Асинхронная гонка при старте программы (Race Condition)

Поскольку корутины запускаются асинхронно, теоретически может возникнуть ситуация, когда операционная система переключит контекст на корутину чтения read_from_device до того, как внутренняя логика библиотеки telnetpp::session успеет полностью завершить инициализацию и прописать свой парсер в наш канал.
Если в этот микроскопический момент времени коммутатор уже успеет прислать в сокет какие-то байты (например, свой приветственный баннер), корутина чтения попытается вызвать еще не созданный коллбек. Проверка защищает программу на этапе старта.

3. Безопасность при закрытии сессии

В сложных асинхронных системах при уничтожении объектов или закрытии сессии указатели на функции часто принудительно сбрасываются в nullptr, чтобы разорвать циклические ссылки в памяти. Если сокет продолжит закрываться и выдаст последние байты буфера, проверка не даст программе упасть на этапе выхода.
 */
#endif



//Работа по SNMP, v.3
#if 0
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include "snmp_pp/snmp_pp.h"

#ifdef SNMP_PP_NAMESPACE
using namespace Snmp_pp;
#endif

#include "snmp_pp/snmp_pp.h"
#include <iostream>
#include <mutex>

class SnmpSubsystemManager {
public:
    // Конструктор автоматически инициализирует сокеты
    SnmpSubsystemManager() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ref_count_ == 0) {
#ifdef _WIN32
            std::cout << "[Система] Локальный запуск UDP-подсистемы WinSock...\n";
#endif
            Snmp_pp::Snmp::socket_startup(); // Вызов Си-библиотеки [1]
        }
        ++ref_count_;
    }

    // Деструктор автоматически очищает ресурсы, когда объект выходит из видимости
    ~SnmpSubsystemManager() {
        std::lock_guard<std::mutex> lock(mutex_);
        --ref_count_;
        if (ref_count_ == 0) {
#ifdef _WIN32
            std::cout << "[Система] Локальное освобождение ресурсов UDP-подсистемы...\n";
#endif
            Snmp_pp::Snmp::socket_cleanup(); // Вызов Си-библиотеки [1]
        }
    }

    // Запрещаем копирование, чтобы случайно не сломать счетчик ссылок
    SnmpSubsystemManager(const SnmpSubsystemManager&) = delete;
    SnmpSubsystemManager& operator=(const SnmpSubsystemManager&) = delete;

private:
    static inline int ref_count_ = 0;
    static inline std::mutex mutex_;
};

namespace asio = boost::asio;

// Глобальный или статический пул потоков для блокирующих вызовов Си-библиотек
// Глобальная точка доступа к изолированному пулу потоков операционной системы
inline asio::thread_pool& get_blocking_thread_pool() {
    static asio::thread_pool pool(4); // Выделяем ровно 4 физических потока для тяжелого Си-кода
    return pool;

}

// Полностью асинхронная, стабильная обертка для SNMP GET (Boost 1.87)
// Потокобезопасный асинхронный адаптер для одиночных GET-запросов
asio::awaitable<int> async_snmp_get(Snmp& snmp, Pdu& pdu, SnmpTarget& target) {

    // Лямбда-инициализатор, которую Asio вызовет внутри async_initiate
    auto initiate_get = [&snmp, &pdu, &target](auto&& handler) {

        // Отправляем задачу в наш честный пул потоков
        // Делегируем блокирующую задачу в физический поток пула
        asio::post(get_blocking_thread_pool(),
                   [&snmp, &pdu, &target, handler = std::forward<decltype(handler)>(handler)]() mutable {

                       // 1. Выполняем тяжелый блокирующий сетевой запрос в фоне
                       int status = snmp.get(pdu, target);// Вызов snmp++ (блокирует текущий фоновый поток)

                       // 2. Возвращаем результат в io_context.
                       // Это действие автоматически разбудит спящую корутину.
                       std::move(handler)(status);// Безопасный возврат результата в io_context
                   }
        );
    };

    // Магия C++20: Корутина мягко засыпает и просыпается ТОЛЬКО по факту вызова handler(status)
    // Замораживаем корутину без таймеров; пробуждение произойдет строго по вызову handler(status)
    co_return co_await asio::async_initiate<const asio::use_awaitable_t<>&, void(int)>(
            initiate_get, asio::use_awaitable
    );
}

// Асинхронная обертка над snmp.get_bulk
// Потокобезопасный асинхронный адаптер для пакетных GET-BULK запросов (выкачивание таблиц)
asio::awaitable<int> async_snmp_get_bulk(Snmp& snmp, Pdu& pdu, SnmpTarget& target,
                                         int non_repeaters, int max_repetitions) {
    auto initiate_bulk = [&snmp, &pdu, &target, non_repeaters, max_repetitions](auto&& handler) {
        asio::post(get_blocking_thread_pool(),
                   [&snmp, &pdu, &target, non_repeaters, max_repetitions, handler = std::forward<decltype(handler)>(handler)]() mutable {

                       // Вызываем bulk-запрос. Он «выстреливает» вперед от заданных OID-ов
                       int status = snmp.get_bulk(pdu, target, non_repeaters, max_repetitions);// Вызов snmp++

                       std::move(handler)(status);
                   }
        );
    };

    co_return co_await asio::async_initiate<const asio::use_awaitable_t<>&, void(int)>(
            initiate_bulk, asio::use_awaitable
    );
}

// Корутина сбора данных (логика осталась чистой и красивой)
asio::awaitable<void> snmp_collection_logic(std::string ip_address) {
    // Гарантируем, что пока работает эта корутина, подсистема SNMP инициализирована!
    // Как только корутина завершится (или упадет по исключению), деструктор локального объекта
    // сам безопасно вызовет socket_cleanup(), если это был последний активный сборщик.
    SnmpSubsystemManager snmp_lifetime_guard;

    std::cout << "[SNMP] Инициализация сессии для " << ip_address << "...\n";

    // 1. Создаем локальную SNMP сессию (0 — ОС сама выделит свободный UDP-порт)
    int status = 0;
    Snmp snmp(status, 0, false);
    if (status != SNMP_CLASS_SUCCESS) {
        std::cerr << "[Ошибка] Не удалось запустить SNMP сессию: " << status << "\n";
        co_return;
    }

    // 2. Валидируем и настраиваем параметры подключения к коммутатору (Target)
    IpAddress ip_target(ip_address.c_str());
    if (!ip_target.valid()) {
        std::cerr << "[Ошибка] Неверный IP адрес устройства.\n";
        co_return;
    }

    CTarget target(ip_target);
    target.set_version(version1);// Используем v1 (базовый вариант для тестов)
    target.set_readcommunity("public");// SNMP-пароль (community string)execur
    target.set_timeout(200);// Аппаратный таймаут: 2 секунды (200 * 10 мс)
    target.set_retry(1);// 1 повторная попытка при потере UDP-пакета

    if (!target.valid()) {
        std::cerr << "[Ошибка] Конфигурация Target невалидна.\n";
        co_return;
    }

    // 3. СБОР ОДИНОЧНЫХ МЕТРИК: Формируем PDU-конверт и наполняем его интересующими OID
    Pdu request_pdu;
    request_pdu += Vb("1.3.6.1.2.1.1.1.0"); // sysDescr
    request_pdu += Vb("1.3.6.1.2.1.1.3.0"); // sysUpTime
    request_pdu += Vb("1.3.6.1.2.1.1.5.0"); // sysName

    std::cout << "[SNMP] Отправка асинхронного GET-запроса через Thread Pool...\n";

    // Вызываем обновленную обертку. Поток io_context теперь РЕАЛЬНО свободен!
    // Асинхронно отправляем пакет. Корутина засыпает, io_context обрабатывает другие задачи
    int result_status = co_await async_snmp_get(snmp, request_pdu, target);

    if (result_status == SNMP_CLASS_SUCCESS) {
        std::cout << "[Успех] Данные от оборудования (" << ip_address << ") успешно получены:\n\n";

        // Парсим полученный контейнер PDU
        int vb_count = request_pdu.get_vb_count();
        Vb current_vb;

        for (int i = 0; i < vb_count; ++i) {
            request_pdu.get_vb(current_vb, i);
            std::cout << "  " << current_vb.get_printable_oid()
                      << " -> " << current_vb.get_printable_value() << "\n";
        }
    } else {
        std::cerr << "[Ошибка сбора] SNMP запрос завершился со статусом: " << result_status
                  << " (" << snmp.error_msg(result_status) << ")\n";
    }

    // 4. СБОР ТАБЛИЧНЫХ ДАННЫХ: Обход поддерева (Subtree Walk) на примере таблицы MAC-адресов
    // Базовый OID колонки портов в таблице MAC-адресов
    Oid base_oid("1.3.6.1.2.1.17.7.1.2.2");// Базовый OID колонки портов в FDB-таблице
    Oid current_oid = base_oid;
    bool has_more = true;

    while (has_more) {
        Pdu pdu;
        pdu += Vb(current_oid);// Загружаем в PDU стартовую точку для следующего шага

        // Выкачиваем по 20 строк за один сетевой запрос
        int status = co_await async_snmp_get_bulk(snmp, pdu, target, 0, 20); // качаем по 20 строк
        if (status != SNMP_CLASS_SUCCESS) break;

        int vb_count = pdu.get_vb_count();
        if (vb_count == 0) break;

        for (int i = 0; i < vb_count; ++i) {
            Vb vb;
            pdu.get_vb(vb, i);
            Oid res_oid;
            vb.get_oid(res_oid);

            // Проверяем, что полученный OID все еще принадлежит нашей таблице MAC-адресов
            // Проверяем, что мы всё еще внутри колонки dot1qTpFdbPort
            if (base_oid.nCompare(base_oid.len(), res_oid) == 0) {

                // 1. Получаем номер порта (он лежит в значении Vb)
                long port_number = 0;
                vb.get_value(port_number);// Достаем номер порта из значения

                // 2. А где же сам MAC-адрес? В SNMP индекс строки пристыковывается к OID!
                // Для FDB таблицы индекс — это последние 6 числовых под-идентификаторов (sub-ids) OID'а.
                // Пример res_oid: 1.3.6.1.2.1.17.7.1.2.2.1.2.0.26.114.243.5.12
                // Где последние 6 чисел: 0, 26, 114, 243, 5, 12 — это десятичные байты MAC-адреса!

                // Извлекаем MAC-адрес (он зашит в виде последних 6 числовых индексов самого OID)
                if (res_oid.len() >= 6) {
                    int len = res_oid.len();

                    // Печатаем красиво в HEX-формате, вытаскивая индекс из самого OID
                    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X -> на Порту: %ld\n",
                           res_oid[len - 6], res_oid[len - 5], res_oid[len - 4],
                           res_oid[len - 3], res_oid[len - 2], res_oid[len - 1],
                           port_number);
                }

                current_oid = res_oid;// Сдвигаем курсор вперед для следующей итерации цикла while
            } else {
                has_more = false;// Коммутатор начал отдавать данные из другой ветки MIB — таблица кончилась
                break;
            }
        }
    }
}

int main() {

#ifdef _WIN32
    // Устанавливаем кодировку UTF-8 (код 65001) для вывода текста на экран консоли.
    // Это устраняет кракозябры при чтении русских букв, приходящих от сервера.
    SetConsoleOutputCP(65001);

    // Устанавливаем кодировку UTF-8 (код 65001) для ввода текста с клавиатуры.
    // Это гарантирует, что вводимые нами символы кодируются так же, как их ожидает сервер.
    SetConsoleCP(65001);

    // Получаем низкоуровневый дескриптор (хэндл) стандартного вывода текущей консоли.
    // Этот хэндл нужен операционной системе, чтобы изменить внутренние настройки экрана.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // Проверяем, что хэндл консоли успешно получен и система не вернула ошибку.
    if (hOut != INVALID_HANDLE_VALUE) {
        // Создаем переменную для хранения текущего флага-маски режимов консоли.
        DWORD dwMode = 0;
        // Запрашиваем у Windows текущие настройки консоли и записываем их в dwMode.
        // Если функция вернула true (успех), значит, мы можем их модифицировать.
        if (GetConsoleMode(hOut, &dwMode)) {
            // Включаем аппаратную обработку виртуального терминала + обработку вывода
            // Включаем обработку ANSI-последовательностей (Esc-кодов).
            // Благодаря этому консоль Windows аппаратно понимает команды сервера
            // о цвете текста, очистке экрана и перемещении курсора (история команд).
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            // Включаем обработку служебных управляющих символов вывода.
            // Этот флаг заставляет Windows корректно интерпретировать такие символы,
            // как Backspace (удаление), перевод строки (\n) и возврат каретки (\r).
            dwMode |= ENABLE_PROCESSED_OUTPUT;
            // Записываем обновленную маску режимов обратно в настройки ОС Windows.
            // С этого момента консоль начинает работать в режиме умного терминала.
            SetConsoleMode(hOut, dwMode);
        }
        else {
            std::cerr << "[Внимание Windows] Не удалось прочитать режимы консоли. Возможно искажение текста.\n";
        }
    }
    else {
        // ОБРАБОТКА ОШИБКИ: Сигнализируем, что стандартный вывод недоступен
        std::cerr << "[Внимание Windows] Стандартный вывод консоли недоступен (INVALID_HANDLE_VALUE).\n"
                  << "Программа запущена в фоне или вывод перенаправлен в файл.\n";
    }

#endif

    // 1. Инициализируем системную UDP подсистему сокетов библиотеки
    //Snmp::socket_startup();

    try {
        asio::io_context io_context;
        std::string target_ip = "10.178.18.11"; // IP-адрес вашего TFortis для теста

        // Регистрируем корутину SNMP-сбора в планировщике Asio
        asio::co_spawn(io_context, snmp_collection_logic(target_ip), asio::detached);

        // Запускаем асинхронный движок
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Критическое исключение: " << e.what() << "\n";
    }

    // 2. Корректно освобождаем ресурсы UDP подсистемы перед выходом
    //Snmp::socket_cleanup();
    return 0;
}

/*
 *
1. Холостая загрузка процессора (Polling) — Полностью ликвидирована ❌ 🔄

В предыдущей версии микро-таймер просыпался каждые 5 миллисекунд. На 100 коммутаторах это создавало бы 20 000 просыпаний в секунду, заставляя ядро ОС постоянно переключать контекст потоков (Context Switch) и раскаляя процессор.
• Как сейчас: Нагрузка упала до абсолютного нуля (0% CPU) в режиме ожидания. Корутина вообще не выполняет никакой код, пока пакет летит по сети. io_context просто спит на системном вызове epoll/kqueue/GetQueuedCompletionStatus. Сигнал на пробуждение приходит аппаратно от ОС, когда фоновый поток завершает snmp.get() и пушит задачу в очередь.

2. Состояние гонки (Race Condition) и UB с памятью — Защищено на 100% 🛡️

Мы убрали временной зазор между завершением работы потока и пробуждением корутины.
• Как сейчас: Функция async_snmp_get гарантированно удерживает корутину «замороженной» в точке co_await. Локальные переменные request_pdu и target в функции snmp_collection_logic физически не могут выйти из области видимости и уничтожиться раньше, чем отработает фоновый поток. Падение 0xC0000005 из-за обращения к «мусорной» памяти здесь теперь невозможно.

⚠️ Единственный тонкий нюанс на будущее: Время жизни объекта Snmp snmp

В текущем коде объект Snmp snmp(status, 0, false); создан локально внутри корутины snmp_collection_logic.
Поскольку внутри async_snmp_get мы передаем snmp по ссылке:
cpp
auto initiate_get = [&snmp, &pdu, &target](auto&& handler)
Используйте код с осторожностью.
Это абсолютно безопасно, пока корутина работает последовательно. Но если в будущем вы решите оптимизировать код и запустить, например, параллельный сбор (когда одна корутина делает co_await одновременно для трех разных PDU через asio::make_parallel_group или boost::asio::collect), то один и тот же объект Snmp будет вызван из пула потоков параллельно. Внутри библиотеки snmp++ класс Snmp использует общие сокеты и структуры, поэтому он не является потокобезопасным для одновременных запросов.
Рекомендация на будущее: Правило «одна сессия Snmp — один последовательный запрос» (как у вас сейчас) — идеальное. Если захотите опрашивать 10 коммутаторов одновременно, просто запускайте для каждого свою независимую корутину snmp_collection_logic, где будет создаваться свой локальный экземпляр Snmp snmp
 */

/*
 *
1. Анатомия Vb (Variable Binding): Как извлечь тип данных?

Когда коммутатор возвращает ответ, внутри каждого Vb лежит не просто абстрактный текст, а строго типизированный объект. Если вы попытаетесь прочитать IP-адрес или счётчик трафика как обычную строку через get_printable_value(), вы получите текстовое представление, которое неудобно парсить программно.
Внутри Vb находится полиморфное значение, тип которого можно узнать методом current_vb.get_value().get_syntax().
Основные типы данных SNMP, с которыми вы столкнетесь на TFortis:

Имя типа в snmp++	Значение в SNMP	Что там лежит и как правильно забирать
sNMP_SYNTAX_INT32	Integer32	Целое число (например, номер порта, статус порта 1=up, 2=down).
long val; current_vb.get_value(val);
sNMP_SYNTAX_OCTETSTR	OctetString	Строка байт (sysName, MAC-адрес в бинарном виде, описание).
current_vb.get_value(octet_str_obj);
sNMP_SYNTAX_CNTR32 / CNTR64	Counter32 / 64	Счётчики трафика, ошибок, пакетов. Они только растут.
unsigned long c; current_vb.get_value(c);
sNMP_SYNTAX_TIMETICKS	TimeTicks	Время в сотых долях секунды (sysUpTime, время аптайма порта).
unsigned long t; current_vb.get_value(t);
sNMP_SYNTAX_IPADDRESS	IpAddress	IP-адрес устройства.
IpAddress ip; current_vb.get_value(ip);


2. Как устроена таблица в SNMP (Пример: Таблица портов MIB-II ifTable)

Каждая колонка таблицы имеет свой базовый OID. Строки таблицы определяются индексом (например, номером физического порта), который пристыковывается в конец OID.
Представим абстрактный коммутатор TFortis на 4 порта. Колонка описаний портов (ifDescr) имеет базовый OID 1.3.6.1.2.1.2.2.1.2. Колонка статусов (ifOperStatus) — 1.3.6.1.2.1.2.2.1.8.
Внутри SNMP они будут лежать последовательно:
• 1.3.6.1.2.1.2.2.1.2.1 -> "FastEthernet 0/1" (Строка 1, Колонка ifDescr)
• 1.3.6.1.2.1.2.2.1.2.2 -> "FastEthernet 0/2" (Строка 2, Колонка ifDescr)
• ...
• 1.3.6.1.2.1.2.2.1.8.1 -> 1 (up) (Строка 1, Колонка ifOperStatus)
• 1.3.6.1.2.1.2.2.1.8.2 -> 2 (down) (Строка 2, Колонка ifOperStatus)

3. Эффективный сбор таблиц через GetBulk (Асинхронный вариант)

Чтобы не запрашивать каждый порт отдельным UDP-запросом (что очень медленно), используется операция GetBulk. Вы передаете базовый OID колонки, а коммутатор за один сетевой пакет возвращает вам массив из следующих N элементов (задается через параметр max_repetitions).
Для выполнения этой операции в snmp++ вместо метода snmp.get() вызывается метод snmp.get_bulk(). Напишем для него асингую обертку и функцию обхода таблицы.

Шаг А: Стабильный асинхронный адаптер для GetBulk

cpp
// Асинхронная обертка над snmp.get_bulk
asio::awaitable<int> async_snmp_get_bulk(Snmp& snmp, Pdu& pdu, SnmpTarget& target,
                                         int non_repeaters, int max_repetitions) {
    auto initiate_bulk = [&snmp, &pdu, &target, non_repeaters, max_repetitions](auto&& handler) {
        asio::post(get_blocking_thread_pool(),
            [&snmp, &pdu, &target, non_repeaters, max_repetitions, handler = std::forward<decltype(handler)>(handler)]() mutable {

                // Вызываем bulk-запрос. Он «выстреливает» вперед от заданных OID-ов
                int status = snmp.get_bulk(pdu, target, non_repeaters, max_repetitions);

                std::move(handler)(status);
            }
        );
    };

    co_return co_await asio::async_initiate<const asio::use_awaitable_t<>&, void(int)>(
        initiate_bulk, asio::use_awaitable
    );
}
Используйте код с осторожностью.

Шаг Б: Корутина выкачивания таблицы портов (Пример логики)

Этот алгоритм называется Subtree Walk (обход поддерева). Мы запрашиваем пачку элементов и крутим цикл, пока возвращаемые OID-ы начинаются с базового OID нашей колонки.
cpp
asio::awaitable<void> snmp_walk_interfaces(Snmp& snmp, SnmpTarget& target) {
    std::cout << "[SNMP] Начинаем сбор таблицы интерфейсов...\n";

    // Базовый OID для имен портов (ifDescr)
    Oid base_oid("1.3.6.1.2.1.2.2.1.2");
    Oid current_oid = base_oid;

    bool has_more = true;

    while (has_more) {
        Pdu pdu;
        pdu += Vb(current_oid); // Запрашиваем элементы, следующие за current_oid

        // Запрашиваем по 10 строк за один раз (max_repetitions = 10)
        // non_repeaters = 0 (означает, что у нас нет фиксированных OID в начале пакета)
        int status = co_await async_snmp_get_bulk(snmp, pdu, target, 0, 10);

        if (status != SNMP_CLASS_SUCCESS) {
            std::cerr << "[Ошибка таблицы] " << snmp.error_msg(status) << "\n";
            break;
        }

        int vb_count = pdu.get_vb_count();
        if (vb_count == 0) break;

        for (int i = 0; i < vb_count; ++i) {
            Vb vb;
            pdu.get_vb(vb, i);
            Oid res_oid;
            vb.get_oid(res_oid);

            // Проверяем, находится ли еще возвращенный OID внутри нашей таблицы/колонки
            if (base_oid.n_compare(base_oid.len(), res_oid) == 0) {
                // Извлекаем строго типизированное значение (для ifDescr это строка)
                OctetStr val;
                vb.get_value(val);

                std::cout << "  Порт OID: " << res_oid.get_printable()
                          << " -> " << val.get_printable() << "\n";

                // Запоминаем последний полученный OID, чтобы следующий шаг цикла шел от него
                current_oid = res_oid;
            } else {
                // Мы вышли за пределы таблицы (коммутатор начал отдавать OID-ы из других веток MIB)
                has_more = false;
                break;
            }
        }
    }
    std::cout << "[SNMP] Сбор таблицы интерфейсов завершен.\n";
}
Используйте код с осторожностью.

Главные инсайты по структурам таблиц:

1. pdu.get_bulk — атомарно модифицирует Pdu, очищая ваш отправленный «затравочный» OID и заменяя его массивом Vb, который физически идет следом в базе MIB коммутатора.
2. base_oid.n_compare — это критически важный метод валидации. Без него ваш цикл уйдёт в бесконечное путешествие по всей памяти коммутатора, выкачивая данные вплоть до системных настроек.
3. Пул потоков спасает и тут: Обход таблицы может занимать 5-10 итераций сетевого обмена. Благодаря нашей архитектуре async_initiate, во время сбора таблицы все 10 итераций корутина будет тихо приостанавливаться, отдавая процессорное время соседним SSH/Telnet сессиям.

 */

/*
 *
Специфика таблицы MAC-адресов (FDB Table)

Для закрепления анатомии Vb и Pdu давайте разберем таблицу MAC-адресов. В стандартных MIB-II она называется dot1qTpFdbTable (или старая dot1dTpFdbTable).
В ней есть две ключевые колонки:
1. dot1qTpFdbPort (1.3.6.1.2.1.17.7.1.2.2.1.2) — возвращает Integer (номер порта, за которым увиден MAC).
2. dot1qTpFdbAddress (1.3.6.1.2.1.17.7.1.2.2.1.1) — возвращает сам MAC-адрес. Но на коммутаторах (включая TFortis) он возвращается не строкой "00:1A:2B...", а типом OctetString длиной строго 6 байт (сырые бинарные данные).
Поэтому при разборе этой таблицы вам нужно не просто вызвать .get_printable(), а правильно распарсить байты.

Пример адаптации логики парсинга FDB-таблицы:

cpp
// Базовый OID колонки портов в таблице MAC-адресов
Oid base_oid("1.3.6.1.2.1.17.7.1.2.2.1.2");
Oid current_oid = base_oid;
bool has_more = true;

while (has_more) {
    Pdu pdu;
    pdu += Vb(current_oid);

    int status = co_await async_snmp_get_bulk(snmp, pdu, target, 0, 20); // качаем по 20 строк
    if (status != SNMP_CLASS_SUCCESS) break;

    int vb_count = pdu.get_vb_count();
    if (vb_count == 0) break;

    for (int i = 0; i < vb_count; ++i) {
        Vb vb;
        pdu.get_vb(vb, i);
        Oid res_oid;
        vb.get_oid(res_oid);

        // Проверяем, что мы всё еще внутри колонки dot1qTpFdbPort
        if (base_oid.n_compare(base_oid.len(), res_oid) == 0) {

            // 1. Получаем номер порта (он лежит в значении Vb)
            long port_number = 0;
            vb.get_value(port_number);

            // 2. А где же сам MAC-адрес? В SNMP индекс строки пристыковывается к OID!
            // Для FDB таблицы индекс — это последние 6 числовых под-идентификаторов (sub-ids) OID'а.
            // Пример res_oid: 1.3.6.1.2.1.17.7.1.2.2.1.2.0.26.114.243.5.12
            // Где последние 6 чисел: 0, 26, 114, 243, 5, 12 — это десятичные байты MAC-адреса!

            if (res_oid.len() >= 6) {
                int len = res_oid.len();

                // Печатаем красиво в HEX-формате, вытаскивая индекс из самого OID
                printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X -> на Порту: %ld\n",
                       res_oid[len - 6], res_oid[len - 5], res_oid[len - 4],
                       res_oid[len - 3], res_oid[len - 2], res_oid[len - 1],
                       port_number);
            }

            current_oid = res_oid;
        } else {
            has_more = false;
            break;
        }
    }
}
Используйте код с осторожностью.

Какие еще таблицы вы будете собирать на TFortis по этой аналогии:

• Таблица статусов PoE (Power over Ethernet): 1.3.6.1.4.1.4526.11.16.1.1.1.3 (покажет потребляемую мощность камер на каждом порту).
• Таблица счетчиков ошибок (ifInErrors): 1.3.6.1.2.1.2.2.1.14 (поможет корутине-супервизору понять, что линк до камеры "флудит" или кабель поврежден).
Архитектура полностью масштабируема. Вы меняете только base_oid и логику распаковки (long, OctetStr или разбор суффикса Oid).
 */

/*
 *
Блок 1: Выделенный пул потоков (Thread Pool)

cpp
// Глобальная точка доступа к изолированному пулу потоков операционной системы
inline asio::thread_pool& get_blocking_thread_pool() {
    static asio::thread_pool pool(4); // Выделяем ровно 4 физических потока для тяжелого Си-кода
    return pool;
}
Используйте код с осторожностью.
• Зачем это нужно: Старая библиотека snmp++ выполняет блокирующие сетевые вызовы (ожидание UDP-ответа по таймауту). Пул потоков принимает эти задачи на себя.
• Результат: Главный цикл io_context остается абсолютно свободным и может без задержек крутить параллельные сессии SSH, Telnet или обслуживать GUI.


Блок 2: Асинхронные адаптеры (Get и GetBulk)

Этот блок — мост между синхронным кодом Си-библиотеки и асинхронными корутинами C++20.
cpp
// Потокобезопасный асинхронный адаптер для одиночных GET-запросов
asio::awaitable<int> async_snmp_get(Snmp& snmp, Pdu& pdu, SnmpTarget& target) {
    auto initiate_get = [&snmp, &pdu, &target](auto&& handler) {
        // Делегируем блокирующую задачу в физический поток пула
        asio::post(get_blocking_thread_pool(),
            [&snmp, &pdu, &target, handler = std::forward<decltype(handler)>(handler)]() mutable {
                int status = snmp.get(pdu, target); // Вызов snmp++ (блокирует текущий фоновый поток)
                std::move(handler)(status);         // Безопасный возврат результата в io_context
            }
        );
    };
    // Замораживаем корутину без таймеров; пробуждение произойдет строго по вызову handler(status)
    co_return co_await asio::async_initiate<const asio::use_awaitable_t<>&, void(int)>(
        initiate_get, asio::use_awaitable
    );
}

// Потокобезопасный асинхронный адаптер для пакетных GET-BULK запросов (выкачивание таблиц)
asio::awaitable<int> async_snmp_get_bulk(Snmp& snmp, Pdu& pdu, SnmpTarget& target,
                                         int non_repeaters, int max_repetitions) {
    auto initiate_bulk = [&snmp, &pdu, &target, non_repeaters, max_repetitions](auto&& handler) {
        asio::post(get_blocking_thread_pool(),
            [&snmp, &pdu, &target, non_repeaters, max_repetitions, handler = std::forward<decltype(handler)>(handler)]() mutable {
                int status = snmp.get_bulk(pdu, target, non_repeaters, max_repetitions); // Вызов snmp++
                std::move(handler)(status);
            }
        );
    };
    co_return co_await asio::async_initiate<const asio::use_awaitable_t<>&, void(int)>(
        initiate_bulk, asio::use_awaitable
    );
}
Используйте код с осторожностью.
• Механика asio::async_initiate: Корутина вызывает co_await и мгновенно «засыпает». Она не потребляет процессорное время, пока UDP-пакет летит до коммутатора TFortis.
• Безопасность памяти: Поскольку корутина гарантированно стоит на месте до выполнения лямбды в пуле, переданные по ссылкам объекты pdu и target физически не могут быть уничтожены. Падения 0xC0000005 исключены.


Блок 3: Корутина бизнес-логики сбора данных

cpp
asio::awaitable<void> snmp_collection_logic(std::string ip_address) {
    std::cout << "[SNMP] Инициализация сессии для " << ip_address << "...\n";

    // 1. Создаем локальную SNMP сессию (0 — ОС сама выделит свободный UDP-порт)
    int status = 0;
    Snmp snmp(status, 0, false);
    if (status != SNMP_CLASS_SUCCESS) { co_return; }

    // 2. Валидируем и настраиваем параметры подключения к коммутатору (Target)
    IpAddress ip_target(ip_address.c_str());
    if (!ip_target.valid()) { co_return; }

    CTarget target(ip_target);
    target.set_version(version1);         // Используем v1 (базовый вариант для тестов)
    target.set_readcommunity("public");   // SNMP-пароль (community string)
    target.set_timeout(200);              // Аппаратный таймаут: 2 секунды (200 * 10 мс)
    target.set_retry(1);                  // 1 повторная попытка при потере UDP-пакета

    if (!target.valid()) { co_return; }

    // 3. СБОР ОДИНОЧНЫХ МЕТРИК: Формируем PDU-конверт и наполняем его интересующими OID
    Pdu request_pdu;
    request_pdu += Vb("1.3.6.1.2.1.1.1.0"); // sysDescr (Описание системы)
    request_pdu += Vb("1.3.6.1.2.1.1.3.0"); // sysUpTime (Время работы устройства)
    request_pdu += Vb("1.3.6.1.2.1.1.5.0"); // sysName (Сетевое имя коммутатора)

    // Асинхронно отправляем пакет. Корутина засыпает, io_context обрабатывает другие задачи
    int result_status = co_await async_snmp_get(snmp, request_pdu, target);

    if (result_status == SNMP_CLASS_SUCCESS) {
        // Парсим полученный контейнер PDU
        int vb_count = request_pdu.get_vb_count();
        Vb current_vb;
        for (int i = 0; i < vb_count; ++i) {
            request_pdu.get_vb(current_vb, i);
            std::cout << "  " << current_vb.get_printable_oid() << " -> " << current_vb.get_printable_value() << "\n";
        }
    }

    // 4. СБОР ТАБЛИЧНЫХ ДАННЫХ: Обход поддерева (Subtree Walk) на примере таблицы MAC-адресов
    Oid base_oid("1.3.6.1.2.1.17.7.1.2.2.1.2"); // Базовый OID колонки портов в FDB-таблице
    Oid current_oid = base_oid;
    bool has_more = true;

    while (has_more) {
        Pdu bulk_pdu;
        bulk_pdu += Vb(current_oid); // Загружаем в PDU стартовую точку для следующего шага

        // Выкачиваем по 20 строк за один сетевой запрос
        int bulk_status = co_await async_snmp_get_bulk(snmp, bulk_pdu, target, 0, 20);
        if (bulk_status != SNMP_CLASS_SUCCESS) break;

        int vb_count = bulk_pdu.get_vb_count();
        if (vb_count == 0) break;

        for (int i = 0; i < vb_count; ++i) {
            Vb vb;
            bulk_pdu.get_vb(vb, i);
            Oid res_oid;
            vb.get_oid(res_oid);

            // Проверяем, что полученный OID все еще принадлежит нашей таблице MAC-адресов
            if (base_oid.n_compare(base_oid.len(), res_oid) == 0) {
                long port_number = 0;
                vb.get_value(port_number); // Достаем номер порта из значения

                // Извлекаем MAC-адрес (он зашит в виде последних 6 числовых индексов самого OID)
                if (res_oid.len() >= 6) {
                    int len = res_oid.len();
                    printf("  [Таблица FDB] MAC: %02X:%02X:%02X:%02X:%02X:%02X -> Порт: %ld\n",
                           res_oid[len - 6], res_oid[len - 5], res_oid[len - 4],
                           res_oid[len - 3], res_oid[len - 2], res_oid[len - 1], port_number);
                }
                current_oid = res_oid; // Сдвигаем курсор вперед для следующей итерации цикла while
            } else {
                has_more = false; // Коммутатор начал отдавать данные из другой ветки MIB — таблица кончилась
                break;
            }
        }
    }
    std::cout << "[SNMP] Корутина сбора успешно завершена.\n";
}


1. Что такое void(int)?

В строке:
cpp
asio::async_initiate<const asio::use_awaitable_t<>&, void(int)>(...)
Используйте код с осторожностью.
void(int) — это сигнатура функции-обработчика (Callback), которую ожидает получить асинхронная операция.
Она буквально говорит компилятору и Asio: «Когда фоновое действие завершится, корутину разбудит функция, которая принимает на вход ОДИН аргумент типа int (в нашем случае это статус операции) и ничего не возвращает (void)».

2. Откуда берется int внутри auto&& handler?

Когда вы вызываете asio::async_initiate и передаете туда asio::use_awaitable, внутри библиотеки Asio происходит следующее:
1. Asio смотрит на тип asio::use_awaitable и понимает, что вы хотите использовать C++20 корутины.
2. Автоматически генерирует специальный внутренний объект-обработчик (назовем его AsioCoroWaker). Сигнатура вызова этого объекта жестко привязана к переданному шаблону void(int). Это значит, что у этого объекта перегружен operator()(int status).
3. Этот сгенерированный объект Asio передает в вашу лямбду initiate_get в качестве аргумента handler.
Поэтому, когда в пуле потоков вы пишете:
cpp
int status = snmp.get(pdu, target);
std::move(handler)(status); // <--- Вот здесь передается int!
Используйте код с осторожностью.
Вы вызываете сгенерированный Asio функтор, передавая ему int.

3. Как int превращается в результат co_await?

Как только фоновый поток вызывает std::move(handler)(status):
1. Переданный status (тип int) сохраняется внутри контекста корутины.
2. Управляющий поток io_context получает сигнал: «Задача выполнена, корутину можно будить».
3. Корутина просыпается в главном потоке, а выражение co_await asio::async_initiate(...) возвращает этот самый int наружу, как результат обычной функции.

Простая аналогия:

Представьте, что вы приходите в МФЦ (это async_initiate) и просите сделать загранпаспорт через корутину (use_awaitable).
• В шаблоне void(int) вы заранее указываете: «Результатом услуги будет ОДИН бумажный паспорт (int)».
• МФЦ дает вам специальный пустой конверт (handler) и отправляет вас домой спать (co_await).
• Курьер в пуле потоков делает паспорт, кладет его в этот конверт (handler(status)) и отправляет в МФЦ.
• МФЦ будит вас, вы открываете конверт и получаете свой паспорт на руки (co_return co_await ...).
Стало ли понятнее, как связаны void(int) в шаблоне и вызов handler(status) внутри пула потоков? Если этот момент уложился, какой у вас следующий вопрос по коду или механике?

да, теперь понятно. Не понимаю выражение std::move(handler)(status);. Куда мы перемещаем, если не указана переменная?

Выражение std::move(handler)(status); выглядит необычно, потому что здесь совмещены две разные операции: приведение к rvalue-ссылке (перемещение) и немедленный вызов функции.
Давайте разложим эту синтаксическую конструкцию на две независимые строчки, чтобы понять, куда и что перемещается.

Разбор синтаксиса на две строки

Эквивалентный код без «магии» в одну строку выглядит так:
cpp
// 1. Принудительно перемещаем handler во временную rvalue-переменную moved_handler
auto&& moved_handler = std::move(handler);

// 2. Вызываем оператор круглых скобок, передавая статус
moved_handler(status);
Используйте код с осторожностью.
То есть, std::move сам по себе никуда физически не перемещает данные. Он является лишь указанием для компилятора: «Я разрешаю опустошить этот объект handler, так как он мне больше не нужен. Передай его внутренние ресурсы (указатели, контекст корутины) тому, кто будет вызван дальше».

Куда именно происходит перемещение?

Мы перемещаем внутреннее состояние объекта handler внутрь вызова самого оператора operator().
Зачем это нужно библиотеке Boost.Asio?
Внутри handler скрывается тяжелый контекст корутины C++20 (адрес кадра корутины, аллокаторы памяти, системные дескрипторы планировщика).
1. Если бы мы написали просто handler(status); (без std::move), компилятор выполнил бы копирование обработчика. Но handler от корутины часто является move-only объектом (его в принципе нельзя копировать, как std::unique_ptr). Будет ошибка компиляции.
2. Написав std::move(handler), мы говорим: «Вызывайся прямо на оригинальных ресурсах, забирай их и уничтожай handler в фоновом потоке, так как управление всё равно переходит обратно в корутину».

Наглядная аналогия с std::unique_ptr

Представьте, что handler — это std::unique_ptr<Window>. У вас есть функция, которая принимает его по значению, чтобы забрать владение окном: void close_window(std::unique_ptr<Window> w).
Вы не можете вызвать её так: close_window(my_ptr). Компилятор запретит копирование.
Вы вызываете её так: close_window(std::move(my_ptr)).
В нашем случае std::move(handler)(status) — это абсолютно то же самое, только вместо обычной функции вызывается перегруженный оператор круглых скобок у самого объекта handler, который «съедает» свои собственные ресурсы в момент активации.
Удалось ли теперь визуализировать, что std::move(handler)(status) — это просто вызов Move-Only функтора (объекта, ведущего себя как функция)? Если этот синтаксический барьер взят, какой следующий узел в коде вызывает у вас вопросы?

почему мы указываем в выражении static asio::thread_pool pool(4); 4 потока? просто для примера (на будущее)? Нам сейчас бы хватило и одного

Вы абсолютно правы: для текущего изолированного теста, где запускается всего одна корутина сбора данных, одного потока хватило бы за глаза.
Цифра 4 выбрана с прицелом на будущую архитектуру боевого приложения. Поскольку пул потоков объявлен как static, он создается один раз на всё время работы программы.
Давайте разберем, почему в реальной системе автоматизации (особенно при работе с коммутаторами TFortis) одного потока станет мало:

1. Одновременный опрос множества устройств (Параллелизм)

Когда программа перерастет тестовый режим, вам нужно будет опрашивать не один коммутатор, а, например, 30 или 50 устройств на объекте одновременно.
• Если в пуле будет всего 1 поток, все вызовы snmp.get() выстроятся в жесткую последовательную очередь. Если первый коммутатор завис и программа ждет его ответа по таймауту 2 секунды, то остальные 49 коммутаторов будут покорно ждать в очереди, пока этот поток освободится.
• При наличии 4 потоков пул сможет обрабатывать до 4 зависших сокетов параллельно, не тормозя обработку остальных «живых» устройств.

2. Специфика архитектуры Windows и Linux

В отличие от асинхронных сокетов Asio, которые работают через эффективные системные механизмы (epoll или IOCP), синхронная библиотека snmp++ внутри себя наглухо блокирует поток ОС вызовом select() или recvfrom().
Пока один поток спит в ожидании UDP-ответа, операционная система переводит его в режим ожидания и он практически не потребляет ресурсы процессора (0% CPU). Выделение 4 потоков под блокирующие задачи не нагружает процессор холостым ходом, но дает приложению необходимый запас «пропускной способности» при сетевых задержках.

3. Разделение обязанностей (Задел на будущее)

В вашем резюме проекта упоминалось, что этот пул потоков создается «для всех фоновых нужд интеграции со старыми Си-библиотеками». Если завтра вы решите добавить в проект фоновую запись логов на диск, тяжелую работу с базой данных (SQLite/PostgreSQL) или парсинг больших XML/JSON конфигураций — все эти блокирующие задачи смогут использовать этот же самый пул get_blocking_thread_pool(), не мешая друг другу.
Резюме: Прямо сейчас вы можете смело поменять значение на 1: static asio::thread_pool pool(1);. Код продолжит работать идеально. Но как только вы начнете масштабировать систему на всю сеть, верните 4 (или сделайте это значение настраиваемым из файла конфигурации в зависимости от количества коммутаторов).
Остались ли у вас еще вопросы по устройству пула потоков, асинхронной схеме или структурам Pdu/Vb? Если на этом этапе всё прозрачно, мы можем сделать следующий большой шаг — разобрать, как элегантно перевести этот рабочий каркас на защищенный протокол SNMP v3 с шифрованием (UTarget). Двигаемся дальше?

так в asio для создания пула потоков можно просто не напрягаясь использовать выражение asio::thread_pool pool(4), и затем добавлять в пул на запуск функции, используя asio::post. Так просто? Или есть какие-то ограничения?

Да, в Boost.Asio это действительно так просто! Именно за эту элегантность разработчики и любят Asio. Выражение asio::thread_pool pool(4) под капотом создаёт полноценный, кроссплатформенный пул потоков операционной системы (через std::thread или std::jthread), а asio::post — это универсальный диспетчер, который закидывает вашу задачу в общую очередь этого пула.
Однако, несмотря на внешнюю простоту, при работе с реальными пулами потоков (особенно в связке с Си-библиотеками) существует 4 критически важных ограничения и правила, о которых нужно знать:

1. Передача данных только по значению (или под строгим контролем времени жизни)

Это главное ограничение, на котором чаще всего спотыкаются. Как только вы сделали asio::post(pool, [...](){ ... });, лямбда улетает в другой поток. Если вы передали туда переменную по ссылке (&), а функция, которая вызывала asio::post, завершила работу — ссылка станет «битой» (Данные уничтожатся, а поток попытается их прочитать).
• Правило: Всё, что уходит в пул потоков через post, должно либо захватываться по значению (копированием/перемещением std::move), либо корутина на стороне главного потока должна быть жестко заблокирована через co_await до окончания работы потока (как мы сделали в нашей функции async_snmp_get).

2. Потокобезопасность вызываемого кода

Пул потоков параллелен. Если 4 потока одновременно начнут писать в одну и ту же глобальную переменную, std::cout, или вызывать методы одного некопируемого Си-объекта (например, общего класса Snmp), программа упадет или данные исказятся (Data Race).
• Правило: Код внутри лямбды для asio::post должен работать либо с полностью изолированными локальными данными, либо доступ к общим ресурсам должен быть защищен мьютексами (std::mutex или asio::strand).

3. "Застревание" потоков намертво

Если вы создали пул из 4 потоков, и отправили туда 4 задачи, внутри которых запущен бесконечный цикл while(true) или сетевой вызов без таймаута, пул заблокируется полностью. Пятая задача, отправленная через asio::post, встанет в очередь навсегда, так как свободных потоков больше нет.
• Правило: Для всех блокирующих операций (включая сокеты в snmp++) всегда жестко задавайте таймауты (например, ваши target.set_timeout(200)). Поток обязан возвращаться в пул, а не висеть там вечно.

4. Жизненный цикл самого пула

Если объект asio::thread_pool pool выйдет из области видимости (например, функция завершилась, а пул был локальным), его деструктор заблокирует текущий поток и будет послушно ждать, пока все задачи в очереди пула полностью завершатся.
• Правило: Именно поэтому мы объявили наш пул как static inline (или глобальный). Он должен жить ровно столько, сколько живет само приложение, и корректно уничтожаться только при выходе из main().

Краткий итог:

Инструмент asio::thread_pool + asio::post — это великолепный, невероятно легкий способ убрать любой тяжелый, старый или синхронный код с пути главного асинхронного движка io_context. Соблюдая правила передачи памяти и таймаутов, вы получаете абсолютно неубиваемую архитектуру.
 */

/*
 *
Архитектурный мост к SNMP v3

Этот патент на управление временем жизни подсистемы — идеальный фундамент для перехода на SNMP v3. Дело в том, что в SNMP v3 для работы шифрования (AES/DES) и аутентификации (SHA/MD5) требуется инициализировать глобальный класс безопасности — v3MP (v3 Message Processing Model).
И его тоже категорически нельзя инициализировать в main(). Мы упакуем его инициализацию прямо внутрь нашего созданного SnmpSubsystemManager!
Давайте посмотрим, как теперь будет выглядеть этот класс и как настроить защищённую цель UTarget вместо CTarget.

Шаг 1: Расширяем RAII-менеджер для поддержки SNMP v3

Библиотека snmp++ требует, чтобы для v3 мы создали объект v3MP и передали ему уникальный идентификатор нашего приложения (EngineID).
cpp
#include "snmp_pp/v3_mp.h" // Заголовок для SNMP v3

class SnmpSubsystemManager {
public:
    SnmpSubsystemManager() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ref_count_ == 0) {
            // 1. Инициализируем сокеты
            Snmp_pp::Snmp::socket_startup();

            // 2. Инициализируем движок безопасности SNMP v3 (v3MP)
            int status = 0;
            // Создаем локальный EngineID для нашего приложения
            Snmp_pp::OctetStr engine_id("MyAsioAppEngineID");

            // Инициализируем глобальный синглтон безопасности библиотеки
            v3_mp_ = new Snmp_pp::v3MP(engine_id, status);

            // Регистрируем стандартные алгоритмы шифрования (SHA, AES)
            Snmp_pp::AuthPriv* auth_priv = Snmp_pp::AuthPriv::get_instance();
            if (auth_priv) {
                // Включаем поддержку SHA и AES (важно для современных TFortis)
                auth_priv->add_auth(new Snmp_pp::AuthSHA());
                auth_priv->add_priv(new Snmp_pp::PrivAES128());
            }
        }
        ++ref_count_;
    }

    ~SnmpSubsystemManager() {
        std::lock_guard<std::mutex> lock(mutex_);
        --ref_count_;
        if (ref_count_ == 0) {
            // Освобождаем память движка v3
            delete v3_mp_;
            v3_mp_ = nullptr;

            // Закрываем сокеты
            Snmp_pp::Snmp::socket_cleanup();
        }
    }

private:
    static inline int ref_count_ = 0;
    static inline std::mutex mutex_;
    static inline Snmp_pp::v3MP* v3_mp_ = nullptr; // Храним указатель на модуль безопасности
};
Используйте код с осторожностью.

Шаг 2: Меняем CTarget на шифрованный UTarget в корутине

Теперь в корутине вместо простой настройки "public"-комьюнити мы настраиваем пользователя, тип аутентификации и ключ шифрования трафика.
cpp
asio::awaitable<void> snmp_v3_collection_logic(std::string ip_address) {
    SnmpSubsystemManager snmp_lifetime_guard; // Автоматически поднял сокеты и SHA/AES шифрование!

    int status = 0;
    Snmp snmp(status, 0, false);
    if (status != SNMP_CLASS_SUCCESS) { co_return; }

    IpAddress ip_target(ip_address.c_str());
    if (!ip_target.valid()) { co_return; }

    // ВМЕСТО CTarget создаем UTarget (User-Based Target)
    UTarget target(ip_target);
    target.set_version(version3); // Переключаем протокол на версию 3!
    target.set_timeout(200);      // 2 секунды
    target.set_retry(1);

    // Создаем учетные данные пользователя безопасности (USM)
    // Например: логин "admin", аутентификация по SHA с паролем "AuthPassword",
    // шифрование трафика по AES128 с паролем "PrivPassword"
    OctetStr security_name("admin");

    // Регистрируем этого пользователя в нашей сессии snmp
    snmp.get_usm()->add_usm_user(
        security_name,
        SNMP_AUTH_HMACSHA,       // Алгоритм проверки подлинности (SHA)
        SNMP_PRIV_AES128,        // Алгоритм шифрования пакетов (AES128)
        "AuthPassword123",       // Пароль для аутентификации
        "PrivPassword123"        // Пароль для шифрования данных
    );

    // Привязываем пользователя к нашей цели
    target.set_security_name(security_name);
    // Задаем уровень безопасности: Аутентификация + Шифрование (AuthPriv)
    target.set_security_level(SNMP_SECURITY_LEVEL_AUTHPRIV);

    if (!target.valid()) { co_return; }

    // ВСЁ! Вся остальная часть кода ПОЛНОСТЬЮ СОВМЕСТИМА!
    // Вы точно так же вызываете co_await async_snmp_get(snmp, request_pdu, target);
    // Но теперь пакеты в сети летят в полностью зашифрованном виде.
}
Используйте код с осторожностью.

В чём красота этой архитектуры:

Мы поменяли только блок конфигурации Target и добавили модуль безопасности в наш RAII-менеджер. Асинхронные обертки async_snmp_get и async_snmp_get_bulk, пул потоков, корутины и циклы обхода таблиц MAC-адресов остались нетронутыми. Программа стала безопасной, сохранив свою асинхронную монолитность.
 */

/*
 * Поскольку задачи по аудиту прототипов выполнены на отлично, какой будет наш следующий шаг?
• Начнем проектировать общий управляющий класс (Orchestrator/Manager), который будет координировать работу всех трех модулей одновременно?
• Или займемся созданием системы логирования / обработки ошибок для корутин, чтобы приложение умело само восстанавливать связь при сбоях в сети?

 */
#endif



//Сканер портов
#if 0
#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

namespace asio = boost::asio;
using namespace asio::experimental::awaitable_operators; // Включает оператор гонки корутин '||'

// Перечисление для понятного отображения статуса порта
enum class PortStatus {
    Open,       // Порт открыт (получен SYN-ACK, соединение установлено)// SYN-ACK получен -> порт готов к обмену данными
    Closed,     // Порт закрыт (получен RST или ICMP ошибка)// RST получен -> хост активен, но служба не запущена
    Filtered    // Порт молчит (таймаут, пакет застрял в Firewall / ACL)// Таймаут/ICMP ошибка -> пакет заблокирован Firewall
};

class TcpScanner {
public:
    // Конструктор принимает исполнитель (executor) io_context
    // Конструктор привязывает сканер к конкретному потоку ввода-вывода (io_context)
    explicit TcpScanner(asio::any_io_executor exec) : executor_(exec) {}

    /**
     * @brief Проверка одного конкретного TCP-порта (Атомарная операция). Атомарная проверка одного TCP-порта (низкоуровневая операция)
     * @param ip_address Целевой IP-адрес устройства
     * @param port Номер порта для проверки
     * @param timeout_ms Время ожидания ответа в миллисекундах (для локальных сетей рекомендуется 150-300)
     */
    asio::awaitable<PortStatus> scan_single_port(std::string ip_address, uint16_t port, std::chrono::milliseconds timeout_ms) {
        // На каждую проверку создаем СВЕЖИЙ сокет. Повторно использовать сокет без сброса нельзя.
        // РЕСУРСЫ: На каждый запрос создается новый сокет. Повторное использование без сброса запрещено ОС.
        asio::ip::tcp::socket socket(executor_);
        asio::steady_timer timer(executor_);

        boost::system::error_code connect_ec = asio::error::would_block;

        try {
            // Формируем конечную точку для TCP протокола
            asio::ip::tcp::endpoint target_ep(asio::ip::make_address(ip_address), port);

            timer.expires_after(timeout_ms);

            // Запускаем асинхронную гонку между подключением сокета и таймером таймаута.
            // asio::redirect_error подавляет выброс C++ исключений на закрытых портах (экономит CPU).
            // ГОНКА: Ждем, что наступит раньше: системное подключение или таймаут таймера.
            // redirect_error подавляет тяжелые C++ исключения на закрытых портах, критически экономя CPU.
            auto race_result = co_await (
                    socket.async_connect(target_ep, asio::redirect_error(asio::use_awaitable, connect_ec)) ||
                    timer.async_wait(asio::use_awaitable)
            );

            // Обязательно закрываем сокет, освобождая дескриптор в ОС Windows/Linux!
            // ОСВОБОЖДЕНИЕ: Принудительно закрываем дескриптор сокета в ОС сразу после завершения гонки.
            boost::system::error_code close_ec;
            socket.close(close_ec);

            // Индекс 0: Сокет проснулся первым (сеть ответила быстрее, чем прошел таймаут)
            // АНАЛИЗ: Индекс 0 означает, что сеть ответила быстрее, чем истек таймаут.
            if (race_result.index() == 0) {

                if (!connect_ec) {
                    co_return PortStatus::Open; // Ошибок нет -> соединен -> Открыт// Соединение успешно установлено

                } else {
                    co_return PortStatus::Closed; // Пришел сброс (RST) -> Закрыт// Сеть вернула RST пакет

                }
            }
                // Индекс 1: Первым сработал таймер таймаута (устройство промолчало)
            else {
                //boost::system::error_code close_ec;
                //socket.close(close_ec); // Принудительно разрываем зависшую попытку подключения
                co_return PortStatus::Filtered;// Первым сработал таймер (тишина в сети)

            }
        }
        catch (const std::exception&) {
            // На случай невалидного IP-адреса или сбоя инициализации сокета
            co_return PortStatus::Closed;// Защита от некорректных IP или системных сбоев сокета

        }
    }

    /**
     * @brief Сканирование диапазона IP и портов с ограничением конкурентности (Worker Pool). Оркестратор пула воркеров для параллельного сканирования подсети
     * @param ip_addresses Список целевых IP
     * @param ports Список портов для проверки каждого IP
     * @param timeout_ms Таймаут на попытку
     * @param max_concurrent_workers Максимальное число ОДНОВРЕМЕННЫХ сокетов в сети (размер пула)
     */
    asio::awaitable<void> scan_subnet(
            const std::vector<std::string>& ip_addresses,
            const std::vector<uint16_t>& ports,
            std::chrono::milliseconds timeout_ms,
            size_t max_concurrent_workers)
    {
        std::cout << "[Orchestrator] Старт. Задач в очереди: " << ip_addresses.size() * ports.size()
                  << ". Ограничение конкурентности: " << max_concurrent_workers << " сокетов.\n";


        using ScanTask = std::pair<std::string, uint16_t>;
        // Задаем явную емкость буфера канала, чтобы вместить все задачи
        size_t total_tasks = ip_addresses.size() * ports.size();
        // БУФЕР: Создаем канал емкостью под все задачи, чтобы try_send выполнялся мгновенно и без блокировок.
        asio::experimental::channel<void(boost::system::error_code, ScanTask)> task_channel(executor_, total_tasks);

        // 1. Наполняем канал задачами
        // НАПОЛНЕНИЕ: Формируем декартово произведение (каждый IP x каждый Порт) и пушим в канал.
        for (const auto& ip : ip_addresses) {
            for (uint16_t port : ports) {
                task_channel.try_send(boost::system::error_code{}, ScanTask{ip, port});
            }
        }

        // ВАЖНО: Мы НЕ закрываем канал здесь! Мы закроем его, когда поймем, что задачи кончились,
        // либо позволим воркерам выгребать их. Но самый надежный способ для Asio Channel —
        // это закрыть его СРАЗУ ПОСЛЕ наполнения, НО читать пока async_receive возвращает успех,
        // потому что async_receive умеет выгребать остатки из буфера закрытого канала!
        // ВАЖНО: Фиксируем очередь. Закрытый канал разрешает воркерам дочитать буфер до конца через async_receive.
        task_channel.close();

        // 2. Исправленный воркер
        // ЛЯМБДА ВОРКЕРА: Описывает поведение одного потока внутри пула.
        auto worker = [&](size_t worker_id) -> asio::awaitable<void> {
            try {
                // Бесконечный цикл, выход строго по ошибке закрытия пустого канала
                for (;;) {// Бесконечный цикл разбора очереди
                    boost::system::error_code ec;

                    // async_receive вытащит данные из буфера, ДАЖЕ если канал вызвал .close(),
                    // и вернет ошибку channel_closed ТОЛЬКО когда буфер полностью опустеет.
                    // Извлекаем задачу. Если буфер пуст И канал закрыт -> вернется channel_closed.
                    ScanTask task = co_await task_channel.async_receive(asio::redirect_error(asio::use_awaitable, ec));

                    if (ec == asio::experimental::channel_errc::channel_closed) {
                        break; // Вот теперь это честный выход: буфер пуст, канал закрыт. // Честный выход из пула: задачи полностью исчерпаны
                    }

                    // Выполняем сканирование
                    PortStatus status = co_await scan_single_port(task.first, task.second, timeout_ms);

                    if (status == PortStatus::Open) {
                        std::cout << "  [ Worker " << worker_id << " ] [+] ОТКРЫТ: "
                                  << task.first << ":" << task.second << " (" << guess_service(task.second) << ")\n";
                    }
                }
            } catch (...) {
                // Изолируем исключения. // Изолируем непредвиденные сбои воркера, защищая общую нить выполнения
            }
            co_return;
        };

        // 3. Запускаем воркеров через старый добрый барьер из вашей прошлой логики
        // БАРЬЕР СИНХРОНИЗАЦИИ (Latch): Управляет ожиданием группы асинхронных корутин.
        auto remaining_workers = std::make_shared<size_t>(max_concurrent_workers);
        auto wait_timer = std::make_shared<asio::steady_timer>(executor_);
        wait_timer->expires_at(std::chrono::steady_clock::time_point::max());

        // СПАВН: Разворачиваем пул воркеров в контексте io_context.
        for (size_t i = 0; i < max_concurrent_workers; ++i) {
            asio::co_spawn(executor_, [worker, i, remaining_workers, wait_timer]() -> asio::awaitable<void> {
                co_await worker(i);// Запуск жизненного цикла воркера

                (*remaining_workers)--;
                if (*remaining_workers == 0) {
                    wait_timer->cancel(); // Все воркеры завершились — будим оркестратор. // Последний воркер будит главный поток сканера
                }
                co_return;
            }, asio::detached);
        }

        // Ожидаем, пока все воркеры отстреляются. // ОЖИДАНИЕ: Блокируем корутину оркестратора до тех пор, пока таймер не будет отменен воркерами.
        if (*remaining_workers > 0) {
            boost::system::error_code ec;
            co_await wait_timer->async_wait(asio::redirect_error(asio::use_awaitable, ec));
        }

        std::cout << "[Orchestrator] Сканирование всей подсети успешно завершено.\n\n";
    }

    /**
     * @brief Параллельное сканирование вектора портов на одном хосте
     * @param ip_address Целевой IP-адрес
     * @param ports Вектор портов для проверки (например: {22, 23, 80, 445})
     * @param timeout_ms Таймаут на каждый порт
     */
    /**
     * @brief Истинно параллельное сканирование вектора портов на одном хосте
     */
    /**
     * @brief Истинно параллельный опрос вектора портов хоста
     */
    /*asio::awaitable<void> scan_host(std::string ip_address, const std::vector<uint16_t>& ports, std::chrono::milliseconds timeout_ms) {
        std::cout << "[Scanner] Начинаем ПАРАЛЛЕЛЬНЫЙ опрос хоста " << ip_address << " по " << ports.size() << " портам...\n";

        auto remaining_tasks = std::make_shared<size_t>(ports.size());
        auto wait_timer = std::make_shared<asio::steady_timer>(executor_);
        wait_timer->expires_at(std::chrono::steady_clock::time_point::max());

        for (uint16_t port : ports) {
            // ИСПРАВЛЕНИЕ: Вызываем именованную корутину-метод вместо лямбды.
            // Передача port по значению гарантирует изоляцию данных!
            asio::co_spawn(
                    executor_,
                    process_port_scan(ip_address, port, timeout_ms, remaining_tasks, wait_timer),
                    asio::detached
            );
        }

        if (*remaining_tasks > 0) {
            boost::system::error_code ec;
            co_await wait_timer->async_wait(asio::redirect_error(asio::use_awaitable, ec));
        }

        std::cout << "[Scanner] Параллельный опрос хоста " << ip_address << " завершен.\n\n";
    }*/

private:

    // НОВЫЙ МЕТОД: Изолированный рабочий воркер для одного порта
    /*asio::awaitable<void> process_port_scan(
            std::string ip_address,
            uint16_t port,
            std::chrono::milliseconds timeout_ms,
            std::shared_ptr<size_t> remaining_tasks,
            std::shared_ptr<asio::steady_timer> wait_timer)
    {
        // Здесь port равен ровно тому значению, которое было на итерации цикла!
        PortStatus status = co_await scan_single_port(ip_address, port, timeout_ms);

        if (status == PortStatus::Open) {
            std::string service_name = guess_service(port);
            std::cout << "  [+] ИНФО: " << ip_address << ":" << port << " ОТКРЫТ (" << service_name << ")\n";
        }

        (*remaining_tasks)--;
        if (*remaining_tasks == 0) {
            wait_timer->cancel();
        }
        co_return;
    }*/

    // Вспомогательный метод для красивого логирования
    std::string guess_service(uint16_t port) {
        switch (port) {
            case 21:   return "FTP / Firmware";
            case 22:   return "SSH / Management";
            case 23:   return "Telnet / Management";
            case 80:   return "HTTP / Web GUI";
            case 443:  return "HTTPS / Web GUI";
            case 135:  return "RPC / Windows PC маркер";
            case 445:  return "SMB / Windows PC маркер";
            case 3389: return "RDP / Windows Remote Desktop";
            default:   return "Unknown Service";
        }
    }

    asio::any_io_executor executor_;
};

/**
* @brief Генерирует список IP-адресов на основе CIDR строки подсети. Парсер CIDR масок на базе сетевого стека Boost.Asio
* @param cidr_str Строка вида "192.168.1.0/24" или "10.0.0.0/16"
* @return Вектор строк с IP-адресами хостов
*/
std::vector<std::string> generate_ips_from_cidr(const std::string& cidr_str) {
    std::vector<std::string> ips;
    try {
        // make_network сама распарсит строку, проверит корректность маски и IP. // Валидация строки и вычисление границ сети силами Boost
        asio::ip::network_v4 network = asio::ip::make_network_v4(cidr_str);

        // Получаем диапазон валидных хостов в подсети
        auto host_range = network.hosts();// Возвращает диапазон строго валидных хостов (без .0 и .255)


        // Итерируемся по нему. Asio сама переберет адреса от .1 до .254 (для /24)
        for (const auto& ip : host_range) {
            ips.push_back(ip.to_string());
        }

        std::cout << "[Parser] Подсеть " << cidr_str << " успешно распарсена. "
                  << "Обнаружено возможных хостов: " << ips.size() << "\n";
    }
    catch (const boost::system::system_error& e) {
        std::cerr << "[Parser] [!] Ошибка парсинга CIDR '" << cidr_str << "': " << e.what() << "\n";
    }
    return ips;
}

// Главный рабочий воркфлоу программы, оформленный как корутина
asio::awaitable<void> run_app_workflow() {
    auto exec = co_await asio::this_coro::executor;
    TcpScanner scanner(exec);

    // Вводим подсеть в промышленном формате CIDR
    std::string subnet_cidr = "10.182.16.0/22";

    // Генерируем хосты с помощью Boost.Asio
    std::vector<std::string> targets = generate_ips_from_cidr(subnet_cidr);

    if (targets.empty()) {
        std::cerr << "[Main] Список хостов пуст. Завершение работы.\n";
        co_return;
    }

    // Набор портов для фингерпринтинга (наш стандартный пул)
    std::vector<uint16_t> target_ports = {22, 23, 80, 135, 445, 3389};

    // Параметры сканирования
    auto timeout = std::chrono::milliseconds(200);
    size_t concurrent_sockets = 64;

    co_await scanner.scan_subnet(targets, target_ports, timeout, concurrent_sockets);
}

int main(int argc, char* argv[]) {

#ifdef _WIN32
    // Устанавливаем кодировку UTF-8 (код 65001) для вывода текста на экран консоли.
    // Это устраняет кракозябры при чтении русских букв, приходящих от сервера.
    SetConsoleOutputCP(65001);

    // Устанавливаем кодировку UTF-8 (код 65001) для ввода текста с клавиатуры.
    // Это гарантирует, что вводимые нами символы кодируются так же, как их ожидает сервер.
    SetConsoleCP(65001);

    // Получаем низкоуровневый дескриптор (хэндл) стандартного вывода текущей консоли.
    // Этот хэндл нужен операционной системе, чтобы изменить внутренние настройки экрана.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // Проверяем, что хэндл консоли успешно получен и система не вернула ошибку.
    if (hOut != INVALID_HANDLE_VALUE) {
        // Создаем переменную для хранения текущего флага-маски режимов консоли.
        DWORD dwMode = 0;
        // Запрашиваем у Windows текущие настройки консоли и записываем их в dwMode.
        // Если функция вернула true (успех), значит, мы можем их модифицировать.
        if (GetConsoleMode(hOut, &dwMode)) {
            // Включаем аппаратную обработку виртуального терминала + обработку вывода
            // Включаем обработку ANSI-последовательностей (Esc-кодов).
            // Благодаря этому консоль Windows аппаратно понимает команды сервера
            // о цвете текста, очистке экрана и перемещении курсора (история команд).
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            // Включаем обработку служебных управляющих символов вывода.
            // Этот флаг заставляет Windows корректно интерпретировать такие символы,
            // как Backspace (удаление), перевод строки (\n) и возврат каретки (\r).
            dwMode |= ENABLE_PROCESSED_OUTPUT;
            // Записываем обновленную маску режимов обратно в настройки ОС Windows.
            // С этого момента консоль начинает работать в режиме умного терминала.
            SetConsoleMode(hOut, dwMode);
        }
        else {
            std::cerr << "[Внимание Windows] Не удалось прочитать режимы консоли. Возможно искажение текста.\n";
        }
    }
    else {
        // ОБРАБОТКА ОШИБКИ: Сигнализируем, что стандартный вывод недоступен
        std::cerr << "[Внимание Windows] Стандартный вывод консоли недоступен (INVALID_HANDLE_VALUE).\n"
                  << "Программа запущена в фоне или вывод перенаправлен в файл.\n";
    }

#endif

    // Проверяем аргументы командной строки
    /*if (argc != 2) {
        std::cerr << "Использование: tcp_scanner.exe [IP-Адрес]\n";
        std::cerr << "Пример: tcp_scanner.exe 192.168.0.1\n";
        return 1;
    }

    std::string target_ip = argv[1];*/

    try {
        // Создаем главный асинхронный движок
        asio::io_context io_ctx;

        // Вбрасываем нашу корневую корутину в планировщик io_context
        asio::co_spawn(io_ctx, run_app_workflow(), asio::detached);

        // Запускаем цикл обработки событий.
        // Поток main заблокируется здесь и будет асинхронно обрабатывать пакеты
        std::cout << "[Main] Запуск планировщика Boost.Asio..." << std::endl;
        io_ctx.run();
        std::cout << "[Main] Работа завершена штатно." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal Error]: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/*
 * Да, абсолютно верно. На транспортном уровне сетевой модели (L4) порты разделяются на два абсолютно независимых типа: TCP (Transmission Control Protocol) и UDP (User Datagram Protocol).
Концептуально они работают совершенно по-разному, что напрямую влияет на то, как пишется код для их сканирования:

1. TCP порты (Ориентированные на соединение)

• Принцип: Перед обменом данными протокол TCP обязан установить сессию с помощью «трехэтапного рукопожатия» (3-way handshake: SYN \(\rightarrow \) SYN-ACK \(\rightarrow \) ACK).
• Специфика сканирования: Проверить TCP-порт относительно просто. Мы вызываем асингую функцию socket.async_connect. Если коммутатор ответил согласием — порт открыт. Если сбросил соединение (RST) — порт закрыт. Если промолчал — порт закрыт файрволом.
• Примеры: SSH (22), Telnet (23), HTTP (80), HTTPS (443).

2. UDP порты (Без установления соединения)

• Принцип: UDP просто «выстреливает» пакетами (датаграммами) в сеть. Ему не нужно подтверждение, готов ли получатель. Он не строит никаких сессий.
• Специфика сканирования: Сканировать UDP невероятно сложно и медленно. Если порт открыт, сервис на той стороне (например, SNMP) может просто промолчать, если наш пакет не содержал валидного запроса. А если порт закрыт, удаленная операционная система может прислать нам обратно ICMP-пакет с ошибкой "Destination Unreachable (Port Unreachable)". Из-за этого UDP-сканирование часто сводится к логике «если за 2 секунды не прилетела ошибка ICMP — возможно, порт открыт».
• Примеры: SNMP (161, 162), DNS (53), DHCP (67, 68).

Важный нюанс для кодовой базы:

Внутри одного коммутатора или сервера могут одновременно существовать TCP-порт 80 и UDP-порт 80. Они никак друг другу не мешают, не пересекаются, и обрабатываются операционной системой в разных очередях.

 Разбор трехэтапного рукопожатия TCP (3-way handshake) — это ключ к пониманию того, как работают сетевые сканеры. Нам нужно разобраться, какие именно флаги отправляются в сеть и как на них реагирует операционная система (или коммутатор TFortis), чтобы мы могли правильно интерпретировать ответы сокета в коде [ENTITY_PRECISION_RECALL].
Представьте, что два узла в сети хотят начать надежный разговор. Протокол TCP гарантирует доставку данных, поэтому перед отправкой любого полезного текста (например, команды по SSH) операционные системы обязаны «пожать друг другу руки» и согласовать начальные порядковые номера пакетов (Sequence Numbers).

Три шага классического рукопожатия

Внутри TCP-заголовка есть специальные битовые маркеры — флаги. В процессе рукопожатия участвуют два главных флага: SYN (Synchronize / Синхронизация) и ACK (Acknowledge / Подтверждение).
   Ваша программа (Клиент)                 Коммутатор TFortis (Сервер)

            |                                           |
            |   1. SYN (Привет, давай свяжемся!)         |
            |------------------------------------------>|
            |                                           |
            |   2. SYN-ACK (Привет! Давай. Я готов.)     |
            |<------------------------------------------|
            |                                           |
            |   3. ACK (Отлично, соединение установлено)|
            |------------------------------------------>|
            v                                           v
1. Шаг 1: Клиент отправляет SYN
Ваш сканер отправляет пустой TCP-пакет, в заголовке которого взведен флаг SYN. Этим мы говорим удаленному устройству: «Я хочу открыть с тобой сессию на порту Х».
2. Шаг 2: Сервер отвечает SYN-ACK
Если порт на коммутаторе открыт (например, там запущен и ждет подключений Telnet-сервер на 23 порту), операционная система коммутатора принимает наш пакет и отправляет в ответ пакет, где взведены сразу два флага: SYN и ACK. Это означает: «Твой запрос подтверждаю (ACK), со своей стороны тоже готов к синхронизации (SYN)».
3. Шаг 3: Клиент отвечает ACK
Получив SYN-ACK, ваш компьютер отправляет финальный пакет с флагом ACK («Понял, поехали!»). На этом этапе рукопожатие завершено, операционные системы выделили буферы под сессию, и можно передавать данные.

Как это использует сканер портов?

Когда мы пишем сканер портов, нам не обязательно слать финальный ACK и полноценно общаться с сервисом. Нам важно лишь понять — открыта «дверь» или закрыта.
В зависимости от состояния порта, на наш первый пакет SYN придут три совершенно разных сценария:

Ответ от порта	Что произошло под капотом	Что это значит для сканера	Как это увидит Boost.Asio
SYN-ACK	Порт открыт, служба работает и ждет клиентов.	Порт ОТКРЫТ	async_connect завершится успешно (без ошибок).
RST (Reset)	Порт закрыт. ОС коммутатора активна, но на этом порту никто не запущен.	Порт ЗАКРЫТ	async_connect мгновенно вернет ошибку (например, Connection refused).
Тишина (Таймаут)	Пакет либо потерялся, либо заблокирован брандмауэром (Firewall / ACL на коммутаторе).	Порт ФИЛЬТРУЕТСЯ	Наш асинхронный таймер сработает раньше, чем сокет что-то получит.

💡 Флаг RST (Сброс): Если вы стучитесь в закрытую дверь, сетевой стек коммутатора не будет молчать (если нет файрвола). Он вежливо, но жестко отправит вам пакет с флагом RST. Это защитный механизм, говорящий: «Хватит сюда слать пакеты, тут никого нет».

Наш подход в Boost.Asio

Мы будем использовать стратегию под названием TCP Connect Scan. Она использует стандартные системные вызовы операционной системы через socket.async_connect.
• Плюс этого метода в том, что операционная система сама берет на себя весь этот низкоуровневый танец с флагами SYN, SYN-ACK и ACK.
• Нам не придется вручную собирать TCP-заголовки, как мы делали в пинге. Нам достаточно дать команду сокету подключиться к IP:PORT и запустить параллельный таймер таймаута на случай, если порт «молчит» (фильтруется).

 Устройство TCP-сокета в Boost.Asio (asio::ip::tcp::socket) принципиально отличается от того raw-сокета, который мы использовали для пинга.
Если в ICMP-сокете мы вручную собирали каждый байт пакета, то TCP-сокет в Asio — это высокоуровневая абстракция над потоком данных (Stream). Он полностью инкапсулирует внутри себя всю сложную логику протокола TCP: трехэтапное рукопожатие, скользящие окна (sliding windows), повторную отправку потерянных пакетов и сборку потока байт в правильном порядке.
Разберем устройство и жизненный цикл TCP-сокета, которые нам понадобятся для написания сканера.

1. Архитектурные составляющие сокета

Чтобы сокет работал, ему всегда нужны три вещи:
1. Исполнитель (Executor) — контекст (io_context), в котором будут крутиться асинхронные системные вызовы.
2. Протокол (Protocol) — для TCP это объект asio::ip::tcp::v4() (или v6()).
3. Конечная точка (Endpoint) — структура asio::ip::tcp::endpoint, которая в отличие от ICMP обязательно содержит два параметра: IP-адрес и номер порта.

2. Как устроен asio::ip::tcp::endpoint

Создание эндпоинта для TCP выглядит так:
cpp
// Переводим строковый IP и номер порта в бинарную структуру для TCP
asio::ip::tcp::endpoint target_ep(asio::ip::make_address("192.168.0.1"), 80);
Используйте код с осторожностью.
Когда сокет начнет подключаться к этому эндпоинту, операционная система автоматически подставит порт 80 в поле Destination Port TCP-заголовка.

3. Жизненный цикл сокета при сканировании (Connect Scan)

Для сканера портов жизненный цикл сокета будет состоять из 4 быстрых шагов: Создание → Попытка подключения → Определение результата → Закрытие.
   [ ШАГ 1: Создание ]  -->  socket_(executor)  (чистый, закрытый сокет в памяти)
           |
           v
   [ ШАГ 2: Коннект ]   -->  co_await socket_.async_connect(target_ep, ...)
           |
           +-----------------------+-----------------------+

           | (Успех: SYN-ACK)      | (Ошибка: RST)         | (Таймаут: Тишина)
           v                       v                       v
   [ ШАГ 3: Результат ] --> Открыт! (0)    --> Закрыт! (ошибка)    --> Фильтруется! (таймер)

           |                       |                       |
           +-----------------------+-----------------------|
                                   v
   [ ШАГ 4: Очистка ]   -->  socket_.close() (ОБЯЗАТЕЛЬНО освобождаем дескриптор ОС)

Шаг 1: Создание сокета

Каждый раз, когда мы хотим проверить новый порт, мы должны создавать новый, чистый объект сокета.
cpp
asio::ip::tcp::socket socket(executor);
Используйте код с осторожностью.
Важно: Если сокет уже попытался куда-то подключиться (даже неудачно), его нельзя использовать повторно для другого порта без полного сброса. Легче и безопаснее всего просто создавать его в памяти на каждой итерации цикла сканирования.

Шаг 2: Асингую подключение (async_connect)

Мы вызываем функцию async_connect. Под капотом Asio дает команду операционной системе отправить SYN-пакет на указанный порт.
cpp
co_await socket.async_connect(target_ep, asio::use_awaitable);
Используйте код с осторожностью.

Шаг 3: Обработка результатов (Как сокет сообщает об ошибках)

В асинхронном C++ по умолчанию, если async_connect завершается неудачно (пришел флаг RST от закрытого порта), Asio выбрасывает исключение boost::system::system_error.
Для сканера портов выбрасывание исключений на каждый закрытый порт — это очень «дорого» и медленно для процессора (ведь закрытых портов будет 99%). Поэтому в Asio используют специальную перегрузку с переменной boost::system::error_code, которая возвращает ошибку как обычное число:
cpp
boost::system::error_code ec;
// Передаем ec в функцию. Теперь код не упадет с исключением, а запишет результат в ec
co_await socket.async_connect(target_ep, asio::redirect_error(asio::use_awaitable, ec));

if (!ec) {
    std::cout << "Порт открыт!\n";
} else if (ec == asio::error::connection_refused) {
    std::cout << "Порт закрыт (получен RST)!\n";
} else {
    std::cout << "Другая ошибка: " << ec.message() << "\n";
}
Используйте код с осторожностью.

Шаг 4: Обязательное закрытие (socket.close())

У операционной системы есть жесткий лимит на количество одновременно открытых сокетов (дескрипторов файлов). Если после проверки порта (неважно, открыт он или закрыт) не вызвать socket.close(), то при сканировании, например, 1000 портов у вас мгновенно закончатся ресурсы ОС, и программа аварийно завершится.

Как мы организуем логику сканера портов?

Так же, как и в пинге, мы будем запускать гонку двух корутин через оператор || (или asio::race в будущем):
1. Первая корутина делает socket.async_connect(...).
2. Вторая корутина делает timer.async_wait(...) (например, на 500 миллисекунд).
Если победил таймер — мы фиксируем состояние портов как «Заблокирован / Фильтруется файрволом» и принудительно уничтожаем/закрываем сокет.
Устройство TCP-сокета и механизм подавления исключений через redirect_error понятны?
Если да, то мы готовы двигаться дальше. Задавайте следующие вопросы, либо:
• Перейдем к проектированию структуры класса TcpScanner?
• Обсудим, как нам сканировать порты пачками (параллельно), чтобы не перегрузить сеть коммутатора TFortis?

 Это один из самых важных и зрелых вопросов при проектировании сетевого софта. Вы абсолютно правы: всего существует 65 535 TCP-портов.
Если сканировать каждый порт даже по 200 миллисекунд последовательно, проверка одного коммутатора займет около 3,5 часов. А если коммутаторов в сети TFortis несколько десятков, автоматизация потеряет всякий смысл.
В промышленной автоматизации и сетевом инжиниринге эту проблему решают тремя стратегиями: минимизацией списка, асинхронным параллелизмом и адаптивными таймаутами.

Стратегия 1: Сканирование только целевых (Top) портов

Вам не нужно сканировать все 65 тысяч портов. Сетевое оборудование TFortis и сопутствующие серверы автоматизации используют строго определенный стек технологий.
В коде сканера обычно жестко задают массив целевых портов (Target Ports). Их разделяют на логические группы:
• Порты управления (наш приоритет):
	• 21 — FTP (обновление прошивок)
	• 22 — SSH (безопасное управление)
	• 23 — Telnet (устаревшее управление)
	• 80 / 443 — HTTP / HTTPS (веб-интерфейс коммутатора)
• Порты мониторинга и автоматизации:
	• 161 / 162 — SNMP / SNMP Traps (хоть это и UDP, их часто держат в уме при общем аудите)
	• 514 — Syslog (сбор системных логов)
	• 123 — NTP (синхронизация времени)
• Промышленные и специфичные протоколы (если применимо):
	• 502 — Modbus TCP (часто встречается в контроллерах автоматизации)
Рекомендация: Для повседневного мониторинга оборудования ограничьтесь списком из 10–20 стандартных портов. Сканирование полного диапазона (1–65535) нужно запускать только раз в месяц в режиме глубокого аудита безопасности.

Стратегия 2: Асинхронный параллелизм (Batching)

Поскольку мы используем Boost.Asio и корутины, мы можем проверять порты пачками (батчами) параллельно, не создавая при этом 65 тысяч потоков ОС. Наш единственный поток io_context может запустить в сеть, например, 100 или 500 корутин одновременно.
В C++20 корутинах для этого идеально подходит концепция ограничения конкурентности (Semaphore / Конвейер):
[ Список портов: 22, 23, 80, 443, 8080... ]
                     |
                     v
   +-----------------------------------+

   | Ограничитель: максимум 50 портов  |  (Пул активных корутин)
   +-----------------------------------+
     /       |       |       |       \
  Port22  Port23  Port80  Port443  Port8080 ... (все работают асинхронно в одном потоке)
Как только один порт проверился (успешно или по таймауту), его корутина завершается, и конвейер тут же берет из списка следующий порт. Это позволяет держать скорость сканирования стабильно высокой, но не перегружать сетевой стек самого коммутатора.

Стратегия 3: Адаптивные таймауты

Если порт закрыт, коммутатор вернет флаг RST за 1–2 миллисекунды. Асинхронная корутина мгновенно закроется и перейдет к следующему порту.
Задержку создают только те порты, которые закрыты файрволом (они «молчат»). Если поставить таймаут ожидания в 2 секунды, то на фильтруемых портах сканер будет сильно спотыкаться.
Рекомендация: В локальной сети (где пинг до коммутатора TFortis составляет < 5 мс) таймаут на async_connect можно смело ставить в 200–300 миллисекунд. Если за треть секунды порт не ответил — для локальной автоматизации он считается недоступным.

С чего начнем проектирование?

Мы можем заложить в архитектуру нашего TcpScanner динамический массив портов, чтобы при вызове метода мы могли передать ему либо вектор конкретных портов std::vector<uint16_t>{22, 23, 80, 443}, либо команду «сканировать диапазон от Х до Y».
Какая схема работы кажется вам более удобной для вашего проекта?
• Передавать в сканер готовый список (вектор) портов?
• Задавать диапазон (например, от 1 до 1024)?
Давайте выберем, и я предложу структуру класса TcpScanner с учетом неблокирующей обработки ошибок, которую мы обсуждали ранее.

Вы абсолютно правы: передача std::vector<uint16_t> — это самый гибкий, профессиональный и экономичный с точки зрения трафика подход. Для сетевого оборудования (коммутаторов, концентраторов) порты 22 (SSH) и 23 (Telnet), наряду с 80/443 (HTTP/HTTPS) и 161 (SNMP), являются главными маркерами присутствия.
Что касается вашего вопроса — это классическая и очень интересная задача фингерпринтинга (OS/Device Fingerprinting), то есть определения типа устройства по косвенным сетевым признакам [ENTITY_PRECISION_RECALL].
Если у устройства (например, обычного офисного компьютера) закрыты порты управления, определить его тип можно с помощью следующих стратегий:

1. Анализ значения TTL (Time To Live) из нашего модуля Ping

Это самый простой и удивительно эффективный «пассивный» метод. Разные операционные системы по умолчанию выставляют разные начальные значения TTL для своих ICMP-ответов. Когда вы пингуете устройство в локальной сети (где пакет не проходит через маршрутизаторы и TTL не уменьшается), вы видите стартовое значение:
• TTL = 64 — Почти гарантированно Linux (а на его базе работают 95% коммутаторов, включая TFortis, роутеры, контроллеры автоматизации, Raspberry Pi, а также Android).
• TTL = 128 — Гарантированно машина под управлением Windows (компьютер инженера, сервер, рабочая станция).
• TTL = 255 — Часто встречается у сетевого оборудования Cisco или специфических промышленных микроконтроллеров/RTU без полноценной ОС.

2. Специфичный порт-маппинг (Маркеры Windows)

У обычных компьютеров на базе Windows практически всегда открыты порты для работы локальной сети, обмена файлами и удаленного администрирования. Если вы добавите в свой std::vector эти порты, вы сможете мгновенно отсечь ПК:
• 135 (RPC) / 445 (SMB) — Если эти порты открыты, перед вами 100% Windows-компьютер. На промышленном коммутаторе или контроллере автоматизации эти порты не появятся никогда.
• 3389 (RDP) — Удаленный рабочий стол Windows.

3. Запрос по SNMP (Если доступен)

Если на контроллере или компьютере включена служба SNMP (порт 161 UDP), то через ваш уже готовый SNMP-модуль можно запросить стандартный OID sysDescr (1.3.6.1.2.1.1.1.0).
• Коммутатор вернет: "TFortis PSW-2G+ HW v..." или "Cisco IOS...".
• Компьютер вернет: "Hardware: Intel... Software: Windows Version 10.0...".
• Контроллер вернет название своей промышленной прошивки.

4. Сканирование веб-портов (80 / 443)

Если порты 22/23 закрыты, но 80 или 443 открыты, можно сделать быстрый асинхронный HTTP-запрос (например, GET / HTTP/1.1).
• Обычный компьютер инженера вряд ли держит открытый веб-сайт.
• Промышленный контроллер или коммутатор сразу вернет в HTTP-заголовке Server: ... или в теле страницы характерный HTML-код (название бренда, форму ввода пароля для веб-интерфейса).

Архитектурный вывод для нашего сканера:

Чтобы ваш оркестратор мог легко отличать ПК от коммутатора, мы заложим в TcpScanner проверку вектора, содержащего как порты управления (22, 23), так и порты-маркеры ПК (135, 445).
Если логика распознавания устройств понятна, давайте сделаем следующий шаг:
• Спроектируем класс TcpScanner на корутинах с поддержкой таймаута и подавлением исключений через redirect_error?
• Или вы хотите подробнее остановиться на том, как мы организуем параллельный опрос этого вектора портов, чтобы они проверялись одновременно?

 Это великолепное, стратегически точное инженерное размышление. Вы сейчас описали классический промышленный алгоритм Network Discovery (сетевого обнаружения). Использование ARP-таблиц ядра сети (Core Switch) — это самый эффективный способ быстро построить карту сети, не сканируя миллиарды пустых IP-адресов вслепую.
Давайте разложим ваше стратегическое видение на технические этапы, которые мы учтем при проектировании нашего класса TcpScanner.

Фаза 1: Первичный широковещательный поиск (Быстрый горизонтальный скан)

Когда вы заходите на ядро сети по SSH и получаете списки подсетей (например, три подсети /24, а это около 750 потенциальных хостов), ваша задача — максимально быстро найти только коммутаторы.
• Как оптимизировать: На этом этапе мы настраиваем TcpScanner так, чтобы он проверял только 2-3 критических порта управления (например, 22 и 23) и ставим агрессивный таймаут (около 150–200 мс для локальной сети).
• При таком подходе 90% пустых IP-адресов или обычных компьютеров будут «отваливаться» по таймауту или сбросу (RST) мгновенно. Благодаря асинхронности Boost.Asio, мы сможем запустить опрос всей подсети /24 параллельно за пару секунд.

Фаза 2: Построение топологии и обогащение карты через ARP

Как только коммутаторы найдены, вы запрашиваете ARP-таблицу ядра сети. Она дает вам готовую пару: IP-адрес <---> MAC-адрес для всех активных устройств в сети.
• Магия MAC-адресов: Первые 3 байта MAC-адреса (OUI — Organizationally Unique Identifier) уникальны для каждого производителя. По ним ваш оркестратор сможет мгновенно, еще до всякого сканирования портов, понять:
	• Если MAC начинается на 00:E0:4C... (пример) — это может быть Realtek (типичная сетевая карта ПК).
	• Если MAC принадлежит пулу вендора промышленного оборудования или коммутаторов (например, TFortis) — это явный маркер.
• После этого вы запускаете TcpScanner точечно по тем IP из ARP-таблицы, которые не определились по MAC-адресу, используя расширенный вектор портов (135, 445, 80, 443), чтобы дорисовать на карте компьютеры и контроллеры.

Что это означает для проектирования класса TcpScanner?

Класс должен быть масштабируемым. Он не должен намертво привязываться к одному IP-адресу. Архитектурно правильно спроектировать его так, чтобы он умел:
1. Принимать задачу на проверку конкретной пары (IP, Port) как атомарную (минимальную) корутину.
2. Предоставлять высокоуровневый метод для параллельного обхода пула адресов (конвейер), где мы контролируем лимит одновременно открытых сокетов, чтобы программа не превысила лимиты дескрипторов операционной системы (например, максимум 200 параллельных соединений).
Мы полностью подготовили почву. Логика, ограничения ОС, сетевые флаги и стратегия обхода подсетей ясны.
Переходим к написанию кода класса TcpScanner на Boost.Asio 1.87 и C++20 корутинах?
Если готовы, я разверну код класса TcpScanner, который:
• Принимает std::vector<uint16_t> портов.
• Работает асинхронно через гонку || с таймером.
• Безопасно подавляет исключения закрытых портов через redirect_error.

 Отлично! Пишем чистый, современный и масштабируемый класс TcpScanner на Boost.Asio 1.87 и корутинах C++20.
Учитывая наши масштабные планы по сканированию целых подсетей, архитектура класса разделена на две части:
1. Атомарная корутина scan_single_port — проверяет ровно один порт на одном IP с жестким таймаутом и без генерации тяжелых исключений.
2. Пакетная корутина scan_host — принимает вектор портов и опрашивает их параллельно.


Разбор ключевых инженерных решений в коде:

1. Защита от утечки ресурсов (socket.close()): Обратите внимание, что при любом исходе гонки (и при успехе, и при таймауте) мы вызываем socket.close(). Если этого не сделать, дескриптор сокета зависнет в системе, и при массовом сканировании сети программа быстро упадет с ошибкой ОС "Too many open files".
2. asio::redirect_error вместо исключений: Мы передаем объект connect_ec прямо внутрь async_connect. Это заставляет Asio работать в стиле Си-функций — просто возвращать код ошибки. На закрытых портах (которых большинство) это экономит колоссальное количество ресурсов CPU, так как раскрутка стека исключений (stack unwinding) C++ очень медленная.
3. Параллельный запуск вектора: В методе scan_host мы собираем все корутины в вектор tasks. Они запускаются параллельно в рамках одного потока. Если вы передадите вектор из 10 портов и таймаут 200мс, весь хост проверится ровно за 200мс, а не за 2 секунды.
Класс TcpScanner спроектирован с учетом всех требований по оптимизации. Что сделаем дальше?
• Напишем пример функции main(), чтобы показать, как запустить этот сканер в работу?
• Или детально разберем, как масштабировать эту логику на сканирование целой подсети (массива IP-адресов), полученной из ARP-таблицы?

Вы абсолютно правы! Вы сформулировали фундаментальное правило работы с корутинами в Boost.Asio.
Вы всё поняли идеально:
1. Когда мы пишем co_await my_coroutine(), вызов функции возвращает временный безымянный объект (r-value). Он сразу же передает владение оператору co_await, корутина запускается, отрабатывает, и память очищается. Никакой std::move не нужен.
2. Но как только мы сохранили этот объект в переменную или положили в контейнер (как tasks.push_back(task())), он обрел имя и стал l-value. Теперь, чтобы запустить его, мы обязаны явно сказать компилятору: «Я отдаю владение» с помощью std::move().


Исправление scan_host для истинного параллелизма

Поскольку последовательный co_await в цикле (даже со std::move) заставит порты проверяться по очереди, для истинного параллельного запуска всей пачки мы применим стабильный механизм asio::co_spawn.
Функция co_spawn берет нашу корутину, забирает у нас владение (через тот же std::move) и мгновенно «вбрасывает» её в очередь планировщика io_context. Корутина начинает выполняться в фоне сама по себе, а co_spawn возвращает управление немедленно.
Чтобы scan_host не завершился раньше времени (пока фоновые задачи еще проверяют порты), мы будем использовать асинхронный счетчик завершения задач на базе asio::steady_timer или простой структуры-счетчика, но в Asio 1.87 для этого идеально подходит использование asio::barrier или фоновый запуск с ожиданием. Так как мы пишем чистый C++20, мы сделаем элегантный счетчик с использованием std::shared_ptr.
Вот переписанный, истинно параллельный метод scan_host:


Как это теперь работает под капотом:

1. Цикл for (uint16_t port : ports) пробегает по вектору мгновенно. Он не ждет ответа от сети.
2. На каждой итерации asio::co_spawn регистрирует фоновую задачу в io_context и сразу идет дальше. Все 10 или 20 сокетов отправляют свои SYN-пакеты в сеть практически одновременно.
3. Метод scan_host доходит до строки co_await wait_timer->async_wait и засыпает. Поток процессора полностью свободен.
4. Фоновые корутины портов просыпаются по мере ответов от коммутатора, выводят логи об открытых портах и уменьшают счетчик remaining_tasks.
5. Последний завершившийся порт «дергает за веревочку» — делает wait_timer->cancel(). Метод scan_host просыпается и завершает свою работу.
Время выполнения функции теперь всегда строго равно таймауту одного самого медленного порта, сколько бы портов в векторе ни было!
Конвейер параллельного сканирования теперь работает по правильному асинхронному принципу.
Что сделаем на следующем шаге?
• Напишем тестовый main.cpp, чтобы вы могли скомпилировать и проверить вживую работу сканера?
• Или обсудим, как этот метод адаптировать для сканирования целого диапазона IP-адресов (подсети) без перегрузки дескрипторов ОС?

Вы абсолютно правы, и ваш вопрос бьет в самую суть того, как работает кооперативная многозадачность (корутины) в однопоточном планировщике! Вы очень точно помните мои слова.
Давайте разберем этот парадокс: почему при последовательном переключении планировщика работа в сети получается физически параллельной, а не последовательной.

Физический смысл «параллельности» в асинхронном I/O

В сетевых задачах 99.9% времени корутина не производит вычисления на CPU, а ждет ответа от сетевой карты.
Если бы мы использовали последовательный цикл с co_await, конвейер выглядел бы так:
1. Корутина 1 посылает пакет на порт 22 \(\rightarrow \) засыпает.
2. Поток ОС ждет 200 мс, пока прилетит ответ от порта 22.
3. Корутина 1 просыпается, завершается.
4. Только теперь Корутина 2 посылает пакет на порт 23...
• Итог: Общее время для 10 портов = 2000 мс. Это последовательная работа.

Что происходит, когда мы использовали co_spawn?

Когда мы вызвали co_spawn для каждого порта, планировщик действительно запускает их на CPU по очереди (последовательно), но посмотрите, в какой момент происходит переключение:
Поток io_context (CPU)
  |
  |-- Запуск Порт_22 -> Вызов async_connect() -> Отправлен SYN-пакет #1 в провод!
  |   ↳ В этот же миг Порт_22 приостанавливается (co_await). CPU свободен!
  |
  |-- Планировщик мгновенно берет Порт_23 -> Вызов async_connect() -> Отправлен SYN-пакет #2 в провод!
  |   ↳ Порт_23 приостанавливается. CPU свободен!
  |
  |-- Планировщик мгновенно берет Порт_80 -> Вызов async_connect() -> Отправлен SYN-пакет #3 в провод!
  |   ↳ Порт_80 приостанавливается. CPU свободен!
  |
  v
(Всё это заняло менее 1 миллисекунды процессора!)

Итог в этот момент времени:

На CPU все три корутины сейчас приостановлены. Они не работают. Но в сетевом кабеле прямо сейчас одновременно (параллельно) летят три разных пакета к коммутатору TFortis!
С точки зрения сетевой карты и операционной системы, эти три соединения открываются параллельно.

Как происходит возврат ответов?

Через 5 миллисекунд сетевая карта компьютера получает сразу три ответа от коммутатора. Операционная система складывает их в буфер сокета.
1. Планировщик io_context просыпается, видит: «Пришел ответ для порта 80». Он будит корутину порта 80. Она обрабатывает результат за микросекунду и засыпает.
2. Планировщик видит: «Пришел ответ для порта 22». Будит корутину порта 22...
Общее время ожидания для всех портов составит те же самые 200 мс таймаута, потому что все пакеты ждали ответа в сети одновременно.

Подведем итог:

• На CPU (в процессоре): Корутины работают строго последовательно, по очереди сменяя друг друга в одном потоке, когда натыкаются на co_await.
• В сети (на сетевой карте и кабеле): Запросы выполняются параллельно, потому что мы успели «выстрелить» всеми пакетами в сеть до того, как начали ждать ответ от первого из них.
Именно поэтому co_spawn дает колоссальную скорость без необходимости создавать тяжелые потоки ОС (std::thread) под каждый порт.
Разложился ли теперь этот парадокс асинхронного параллелизма?
Если эта концепция теперь полностью прозрачна, как мы поступим:
• Напишем тестовый main.cpp для проверки сканера портов в действии?
• Или у вас остались еще вопросы по планировщику io_context?

Это невероятно важный симптом! Вы столкнулись с классической проблемой асинхронного захвата переменных в лямбда-функциях (лямбда-задачах) внутри циклов C++.
Давайте посмотрим на то, как сейчас написан цикл создания задач в scan_host:
cpp
for (uint16_t port : ports) { // <-- port живет на каждой итерации заново
    auto task = [this, ip_address, port, timeout_ms, remaining_tasks, wait_timer]() -> asio::awaitable<void> {
        PortStatus status = co_await scan_single_port(ip_address, port, timeout_ms);
        // ...
Используйте код с осторожностью.
Когда вы пишите захват port по значению, а потом сразу делаете asio::co_spawn(executor_, task(), asio::detached);, из-за особенностей ленивого выполнения корутин C++20 компилятор может оптимизировать цикл так, что к моменту реального физического старта корутины в планировщике, переменная port уже успевает измениться на следующую из вектора! В итоге несколько корутин «считывают» из памяти одно и то же последнее значение порта (в вашем случае это оказался 3389), а реальные порты (например, 80) просто теряются.
Вот почему на одном реальном открытом порту у вас дублируются записи, а нужный 80-й порт вообще пропал из вывода.

Как это исправить железно?

Чтобы разорвать эту связь, нужно передавать порт явно в качестве аргумента функции или жестко изолировать итератор цикла. Но самый надежный и чистый способ для корутин — отказаться от создания лямбд в цикле и вынести логику опроса одного порта в отдельный маленький именованный метод класса (например, process_port_scan). При вызове обычной функции C++ гарантирует, что значение port скопируется в стек задачи мгновенно и никогда не изменится.
Давайте локально перепишем класс TcpScanner. Мы добавим один приватный метод и изменим scan_host:


Почему теперь всё будет работать правильно:

В C++20 корутинах, когда вы вызываете функцию process_port_scan(...), компилятор мгновенно (прямо в точке вызова) выделяет под неё фрейм корутины и копирует туда аргументы. Даже если цикл пойдет дальше и переменная port в цикле изменится, внутри запущенной задачи копия порта останется строго неизменной. Дублирование логов исчезнет, а затерявшийся 80-й порт отобразится корректно.
Замените этот фрагмент в коде, и дублирование гарантированно пропадет.

План на сегодня выглядит логичным развитием системы. Нам нужно решить классическую проблему C10K/C100K в миниатюре: научить сканер «переваривать» подсети (например, /24 или /16), не упираясь в лимиты открытых файловых дескрипторов ОС (ulimit -n) и не забивая сетевой стек пачками полуоткрытых соединений.
Предлагаю начать с первой задачи: масштабирование на диапазоны IP с батчингом (пакетной обработкой).

Архитектура батчинга для подсетей

Чтобы эффективно сканировать диапазоны, нам нужен асинхронный ограничитель (семафор или конвейер с пулом воркеров). В мире Boost.Asio и корутин C++20 для контроля конкурентности идеально подходит батчинг фиксированного размера или использование asio::experimental::channel (или asio::awaitable_operators для параллельного ожидания).
Давайте набросаем шаблон функции, которая принимает диапазон IP-адресов и портов, бьет их на батчи (например, по 64 параллельных запроса) и дожидается завершения каждой пачки перед переходом к следующей.

Пример реализации батчинга на co_await

cpp
#include <boost/asio.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <iostream>
#include <vector>

namespace asio = boost::asio;

// Предположим, это наш отлаженный метод из прошлой сессии
asio::awaitable<void> process_port_scan(std::string ip, uint16_t port) {
    // Наша логика с async_connect и redirect_error
    co_return;
}

// Корутина для сканирования одного батча (пачки хостов/портов)
asio::awaitable<void> scan_batch(const std::vector<std::pair<std::string, uint16_t>>& targets) {
    // Используем executor текущей корутины
    auto executor = co_await asio::this_coro::executor;

    // Создаем группу корутин внутри батча
    // Для параллельного ожидания группы в Asio можно использовать разные подходы.
    // Самый простой для понимания — запустить N корутин через co_spawn и дождаться их завершения.

    // Но так как co_spawn возвращает void (или awaitable, если настроить),
    // для синхронизации группы часто используют счетчики или барьеры.
    // Рассмотрим вариант с простейшим пулом воркеров через канал Asio:
}

Однако более гибкий паттерн для сетевых сканеров — это не жесткие батчи (когда 63 порта уже проверились, а 1 «зависший» тормозит весь батч), а динамический пул воркеров (Паттерн Worker Pool).
Воркеры непрерывно берут задачи из общего асинхронного канала (очереди). Как только один порт проверен, воркер тут же берет следующий, удерживая планку строго в N одновременных соединений.
С какого подхода начнем масштабирование?
• Реализуем жесткий батчинг (разбиение на группы по N хостов, ждем всех, идем дальше) — это проще в отладке.
• Реализуем динамический пул воркеров через asio::experimental::channel (фиксированное число N активных корутин, которые непрерывно выгребают IP/порты из очереди) — это гораздо быстрее в реальной сети, так как нет просадок из-за медленных хостов.

Отличный, чистый код! Логика синхронизации через remaining_tasks и wait_timer->cancel() сделана очень изящно — вы фактически вручную собрали легковесный барьер синхронизации (Latch).
Давайте масштабировать этот код на сканирование подсетей/диапазонов IP. Так как вы выбрали путь динамического пула воркеров (Worker Pool), мы не будем плодить тысячи корутин одновременно (что могло бы вызвать отказ ОС выделять сокеты). Вместо этого мы запустим строго фиксированное количество воркеров (например, 32 или 64), которые будут параллельно «выкусывать» задачи из общего списка.
Для этого идеально подойдет инструмент boost::asio::experimental::channel. Канал — это потокобезопасная асинхронная очередь, встроенная прямо в Asio.

Пошаговый план модернизации

1. Добавим канал (asio::experimental::channel) в качестве очереди задач. Каждая задача — это пара (IP, Port).
2. Напишем метод scan_subnet, который:
	• Генерирует список IP-адресов подсети.
	• Заполняет канал задачами.
	• Закрывает канал на запись (чтобы воркеры знали, когда работа кончилась).
	• Запускает N воркеров и асинхронно ждет их завершения.
Вот как теперь будет выглядеть обновленный класс TcpScanner и функция main.

Модернизированный код сканера (с поддержкой подсетей)

Добавьте заголовочный файл для каналов:
cpp
#include <boost/asio/experimental/channel.hpp>
Используйте код с осторожностью.
Обновленный класс TcpScanner:


Генератор подсети для проверки

Чтобы передать в сканер список IP-адресов подсети (например, 192.168.1.1 – 192.168.1.254), напишем простую вспомогательную функцию для генерации вектора строк.


Преимущества этого подхода:

1. Жесткий лимит ресурсов: Сколько бы тысяч IP вы ни передали, в памяти и сети будут крутиться ровно concurrent_sockets задач. Операционная система не выбросит ошибку "Too many open files".
2. Эффективность по времени: Если 192.168.1.5 завис на брандмауэре (ждет 200мс), остальные 63 воркера продолжают быстро опрашивать живые хосты. "Зависшие" хосты больше не тормозят собутыльников по батчу.

Похоже, мы столкнулись с классической ловушкой при первом использовании boost::asio::experimental::channel. Вы абсолютно правы: 1270 задач физически не могут провериться мгновенно (даже при таймауте в 200 мс это заняло бы как минимум несколько секунд).
Причина такой «космической скорости» в том, что воркеры завершили работу сразу после старта, даже не начав сканирование.

В чем причина бага?

Посмотрим на код нашего цикла в воркере:
cpp
while (task_channel.is_open()) { ... }
Используйте код с осторожностью.
И сразу после наполнения канала мы вызвали:
cpp
task_channel.close();
Используйте код с осторожностью.
Дело в том, что метод channel::close() в Boost.Asio работает бескомпромиссно: он мгновенно переводит канал в состояние is_open() == false, даже если внутри буфера еще лежат отправленные задачи (try_send).
Когда воркеры стартовали, они проверили условие while(task_channel.is_open()), увидели false и сразу вышли из цикла. Вся очередь задач была просто уничтожена.

Правильное исправление

Канал не нужно закрывать снаружи до того, как воркеры всё прочитают. Правильный паттерн для каналов Asio: воркер пытается читать до тех пор, пока не получит специальную ошибку закрытия канала, а метод .close() вызывается только тогда, когда мы действительно хотим всё завершить, либо мы вообще не закрываем канал, а используем async_receive и проверяем ошибку окончания.
Давайте перепишем цикл воркера правильно. Мы уберем проверку is_open() из условия цикла, а выход сделаем строго по коду ошибки channel_closed.
Замените метод scan_subnet на этот исправленный вариант:


Что изменилось:

1. Мы убрали while (task_channel.is_open()), так как оно ломало логику при мгновенном закрытии.
2. Теперь используется бесконечный for (;;) внутри которого co_await task_channel.async_receive корректно дочитывает внутренний буфер до последнего элемента, и только потом выбрасывает channel_closed.
3. При инициализации канала мы явно передали емкость буфера total_tasks, чтобы try_send гарантированно не отбросил задачи из-за переполнения.

Отличный результат! 200+ живых хостов за несколько секунд — это как раз та скорость, которую должен выдавать настоящий асинхронный движок на корутинах без лишних накладных расходов.
Переходим к автоматизации: парсингу CIDR-нотации (например, 192.168.1.0/24). Вместо того чтобы писать парсеры вручную на строках, мы задействуем мощный встроенный инструмент boost::asio::ip::make_network. Он сам валидирует строку, вычисляет маску, адрес сети, широковещательный адрес и, что самое главное, предоставляет итераторы для обхода всех хостов.

1. Как устроен asio::ip::network_v4

Класс сети имеет встроенный диапазон hosts(), по которому можно пройтись обычным range-based for. Он автоматически исключает адрес самой сети (например, .0) и broadcast (например, .255), возвращая строго валидные IP-адреса устройств.

2. Обновление генератора хостов

Давайте заменим нашу старую временную функцию generate_subnet_ips на профессиональный инструмент.
Добавьте заголовочный файл (если еще не добавлен):


3. Модификация run_app_workflow

Теперь мы можем передавать в приложение любую подсеть (даже огромную /16 или нестандартную вроде /22), и движок прозрачно это обработит. Наш Worker Pool легко переварит любой объем, удерживая ровно 64 сокета в памяти.


Что это нам дает для архитектуры Orchestrator:

Теперь наша программа ведет себя как настоящий профессиональный инструмент (вроде nmap). Мы полностью защищены от ошибок ручного парсинга строк (вроде выхода за границы 255 или некорректных точек).


1. Про таймаут (200 мс) и поведение коммутаторов

• Почему именно 200 мс? Это эмпирический («золотой») стандарт для сканирования локальных сетей (LAN). Пакет внутри локального сегмента долетает до устройства и возвращается обратно за 1–5 миллисекунд (RTT). Запас в 200 мс дается на то, что устройство может быть слегка нагружено и ответит не мгновенно.
• Можно ли меньше? Можно (например, 50–100 мс), но возрастет количество ложноположительных результатов (Filtered). Если сеть или устройство на мгновение «задумаются», пакет опоздает всего на 1 мс, сканер закроет сокет и решит, что порт закрыт/фильтруется, хотя он был открыт.
• Как поведёт себя коммутатор?
	1. Сетевой стек коммутатора: Железо коммутатора (ASIC) пересылает транзитные пакеты на лету, ему всё равно. Но если вы сканируете управляющий IP-адрес самого коммутатора (его CPU), то слабый процессор старого коммутатора (например, какого-нибудь Eltex или Cisco) от лавины запросов может временно перегрузиться.
	2. Блокировки и безопасность: Современные коммутаторы корпоративного уровня (Cisco, Ядро, TFortis) часто имеют встроенные функции безопасности, такие как Storm Control, Rate Limiting или DoS Protection. Если они увидят, что с одного IP-адреса за миллисекунды «прилетает» сотня SYN-пакетов, они посчитают это атакой (SYN-flood) и внесут ваш IP в черный список (заблокируют) на уровне порта. Мягкий таймаут в 200 мс в сочетании с разумным количеством воркеров сглаживает этот поток.

2. Про количество воркеров (64 сокета)

• Почему именно 64? Это баланс между скоростью сканирования и нагрузкой на сетевой стек вашей ОС (и на сетевое оборудование).
• Можно ли больше или меньше?
	• Меньше (например, 10): Сканер будет работать медленнее. Если 10 хостов подряд окажутся «мертвыми», все 10 воркеров уснут на 200 мс, и сканирование замрет на этот период.
	• Больше (например, 512 или 1024): Скорость вырастет, но вы рискуете:
		1. Упереться в лимиты операционной системы на количество одновременно открытых сокетов (дескрипторов). В Linux по умолчанию лимит часто равен 1024 (ulimit -n).
		2. Спровоцировать те самые защитные блокировки на коммутаторах, о которых мы говорили выше.
• Надо ли проверять в тестах? Обязательно. На реальном объекте (в реальной промышленной сети) рекомендуется провести тесты, начав с 32 воркеров, и замерять: нагрузку на CPU, скорость и, главное, процент пропущенных (потерянных) устройств. Если при 128 воркерах устройства начинают «исчезать» из отчетов — сеть не справляется, нужно снижать планку.

3. Как устроен и работает asio::experimental::channel?

Представьте себе channel как трубу, у которой есть вход и выход.
• Это асинхронная, потокобезопасная очередь.
• Когда мы создавали его: channel<void(error_code, ScanTask)> task_channel(executor, total_tasks); мы сказали: «Это труба, внутри которой будут перемещаться объекты типа ScanTask (пара IP+порт), а само уведомление о доставке имеет формат подписки Boost.Asio — код ошибки + данные».
• Главная фишка канала в Asio — он умеет «будить» корутины. Если труба пустая, воркер не просто ждет в цикле CPU, он засыпает, отдавая поток другим задачам. Как только в трубу падает элемент, Asio мгновенно будит спящего воркера.

4. Что делает task_channel.try_send(...)?

Строка task_channel.try_send(boost::system::error_code{}, ScanTask{ip, port}); переводится так: «Попытаться немедленно затолкнуть задачу в трубу без блокировки потока».
• Почему try_send? Так как мы выделили каналу буфер размером ровно под все наши задачи (total_tasks), мы на 100% уверены, что буфер не переполнится. Поэтому нам не нужно использовать асинхронный co_await task_channel.async_send(...). Мы можем мгновенно, «в один присест», засыпать все задачи в память канала.
• Зачем там boost::system::error_code{}? Это требование архитектуры каналов Asio. Сигнатура канала может передавать ошибку вместе с данными. Передавая туда пустой (успешный) код ошибки, мы говорим воркеру: «Здесь лежат валидные данные, ошибки нет».

5. Почему мы закрываем канал сразу?

Мы вызываем task_channel.close(); сразу после заполнения. В обычных очередях (например, std::queue) закрытие уничтожило бы данные. Но в asio::channel логика другая:
• Метод close() переводит канал в режим: «Новые задачи принимать запрещено, но всё, что уже лежит внутри буфера, должно быть честно дочитано».
• Когда воркеры выгребут абсолютно все задачи из буфера, следующий вызов async_receive поймет: «Буфер пуст, а канал закрыт — значит, работы больше никогда не будет» — и вернет ошибку channel_closed. Для воркеров это идеальный сигнал: break; (выйти из цикла и завершиться).
• Если бы мы не закрыли канал, воркеры, выполнив все 1270 задач, просто зависли бы (уснули) навсегда на строке async_receive, ожидая, что кто-то еще закинет им работу. Программа бы никогда не завершилась.

6. Зачем коду засыпать на co_await task_channel.async_receive(...)?

Корутины спроектированы так, чтобы не тратить ресурсы процессора на пустое ожидание.
• Когда воркер делает co_await async_receive, он говорит планировщику: «Я хочу получить задачу. Если прямо в эту микросекунду операционная система или планировщик заняты, или я жду доступа к памяти — я умываю руки, засыпаю. Разбуди меня, когда задача будет у меня в руках».
• В этот момент поток ОС освобождается и может, например, обрабатывать сетевые пакеты async_connect от других воркеров. Это и обеспечивает "истинную асинхронность" — один поток ОС успевает обслуживать десятки воркеров.

7. Зачем нам канал, если можно было взять std::stack или std::queue?

Это самый фундаментальный вопрос! Почему нельзя использовать обычный std::queue<ScanTask> или std::stack?
• Причина 1: Потокобезопасность. Обычные контейнеры STL (std::queue, std::stack) не потокобезопасны. Если несколько воркеров одновременно попытаются вызвать .pop(), произойдет порча памяти (Data Race) и программа упадет в Segmentation Fault. Пришлось бы городить тяжелые синхронизации через std::mutex.
• Причина 2: Асинхронное ожидание (Самая важная). Допустим, мы защитили std::queue мьютексом. Что делать воркеру, если очередь временно опустела, но сканирование еще идет? В обычном коде вам пришлось бы писать while(queue.empty()) { sleep(1); }. Это заблокирует поток ОС!
• А asio::experimental::channel — это не просто контейнер, это инструмент синхронизации планировщика Asio. Он интегрирован с механизмом co_await. Он умеет приостанавливать выполнение самой корутины (освобождая CPU) и возобновлять её без блокировки системного потока. Ни один стандартный контейнер C++ (стек, очередь, вектор) так делать не умеет.

Результат потрясающий! Сеть с маской /22 — это 1022 возможных хоста, и найти их все за 10–15 секунд — чистая победа асинхронного подхода.
Вы абсолютно правы в своей инженерной оценке: классический ICMP Ping (эхо-запрос) для массового обнаружения часто оказывается самым медленным и капризным звеном. Вы точно подметили ключевые проблемы пинга:
1. Фактор таймаута (Потеря пакетов): В реальных сетях (особенно промышленных или Wi-Fi) один ICMP-пакет может легко потеряться из-за случайного всплеска трафика. Чтобы не получить ложный пропуск, приходится слать 2 или 3 пинга, закладывая таймаут около 1 секунды на каждый. Если хоста нет, мы гарантированно теряем секунды на ожидание «тишины».
2. Блокировки на уровне ОС (Firewall): Современные операционные системы (начиная с Windows 10/11 и серверных платформ) по умолчанию полностью игнорируют ICMP-эхо запросы (встроенный брандмауэр их просто дропает). Устройство может быть включено, активно работать, но на пинг оно не ответит никогда.

Почему ваш TCP-сканер портов оказался эффективнее?

Вместо того чтобы стучаться в «закрытую дверь» пинга, вы сразу проверяете прикладные маркеры. У любого живого сетевого устройства (будь то коммутатор, контроллер или ПК инженера) обязан быть открыт хотя бы один порт управления или сетевой службы.
• Если это Windows-машина — она выдаст себя по портам 135 или 445 (даже если пинг на ней отключен).
• Если это коммутатор — вы сразу увидите 22 (SSH), 23 (Telnet) или 80/443 (Web-интерфейс).
При этом асинхронный TCP-коннект работает на упреждение: как только хост живой и порт открыт, операционная система хоста мгновенно возвращает нам SYN-ACK (за пару миллисекунд). Мы тут же закрываем сокет и фиксируем: устройство существует. Нам не нужно ждать секунду, как в случае с пингом.

Следующий шаг: Переход к Orchestrator и сохранению результатов

Теперь, когда у нас есть сверхбыстрый движок, который умеет за секунды находить устройства по портам, нам нужно научить программу запоминать эти результаты, чтобы передать их в Orchestrator.
Оркестратор должен получить карту сети: какой IP-адрес какие порты открыл. На основании этого он будет принимать решения. Например:
• Нашел 192.168.1.50 с открытым 23 портом -> Передать этот IP в модуль Telnet++ для попытки авторизации и снятия логов.
• Нашел 192.168.1.100 с открытыми 135/445 портами -> Пометить как "Рабочая станция Windows" и не тратить время на Telnet.
Для этого нам нужно модифицировать scan_subnet, чтобы она не просто выводила данные в консоль, а собирала их в потокобезопасную структуру данных, например std::map<std::string, std::vector<uint16_t>>.
Давайте займемся логикой сбора данных. Подскажите:
• Хотите ли вы собирать все результаты (включая информацию о закрытых/фильтруемых портах) или для Оркестратора нам нужны строго открытые порты?
• Будем сразу набрасывать каркас класса Orchestrator, который примет эту карту сети и начнет распределять задачи по фингерпринтингу?



 */
#endif