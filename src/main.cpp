#include <iostream>

#include "telnetpp/session.hpp"

#include "telnetpp/client_option.hpp"

#include "boost/asio.hpp"
#include "boost/array.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/ip/address.hpp"

#include "AsioTelnetClient.h"
#include "TelnetProtocol.h"


//Предоставляемый пользователем класс, моделирующий концепцию канала
struct TelnetPPChannel
{

    boost::asio::ip::tcp::socket &socket;
    std::function<void(telnetpp::bytes)> read_callback;

    //вызывается внутренне из telnetpp::session::async_callback для пересылки данных результата из сокета в приложение
    void async_read(std::function<void(telnetpp::bytes)> const &callback) {
        read_callback = callback;
    }

    //вызывается внутренне из telnetpp::session::write для записи данных в сокет
    void write(telnetpp::bytes data) {
        boost::asio::write(socket, boost::asio::buffer(std::string(data.begin(), data.end())));
        //socket.wr  .write(QByteArray(reinterpret_cast<const char*>(data.data()), data.size()));
    }

    //вызывается внутренне из telnetpp::session, чтобы проверить, все еще активен ли сокет
    bool is_alive() const {
        return socket.is_open();
    }

    //вызывается внутренне из telnetpp::session, может принудительно отключить сокет на основе протокола Telnet
    void close() {
        socket.close();
    }

    //Этой функции нет в документации. Вызывается из кода приложения для пересылки необработанных данных сокета в Telnet++
    void receive(telnetpp::bytes data) {
        if (read_callback)
            read_callback(data);
    }

};

void TelnetPPSample()
{



    // создайте все классы и объедините их вместе
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket{io_context};
    socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("10.178.18.4"), 23));

    TelnetPPChannel channel{socket};
    telnetpp::session session{channel};



    // пример чтения обработанных данных из сеанса/канала Telnet++
    session.async_read([](telnetpp::bytes data) {
        std::string ba(data.begin(), data.end());

        ba += '\n';
        // ... сделайте что-нибудь с полученными и обработанными по Telnet++ данными ...

        std::cout << ba << " - from session.async_read";
    });

    std::string str{"admin\n"};
    telnetpp::bytes content2(reinterpret_cast<const telnetpp::byte*>(str.data()),
                            reinterpret_cast<const telnetpp::byte*>(str.data() + str.size()));
    session.write(content2);
    // пример чтения обработанных данных из сеанса/канала Telnet++
    /*session.async_read([](telnetpp::bytes data) {
        std::string ba(data.begin(), data.end());

        ba += '\n';
        // ... сделайте что-нибудь с полученными и обработанными по Telnet++ данными ...

        std::cout << ba;
    });*/

    // пример чтения обработанных данных из сеанса/канала Telnet++
    /*session.async_read([](telnetpp::bytes data) {
        std::string ba(data.begin(), data.end());

        ba += '\n';
        // ... сделайте что-нибудь с полученными и обработанными по Telnet++ данными ...

        std::cout << ba;
    });*/











    // пример получения данных из сокета и записи в сеанс/канал Telnet++

    boost::array<char, 128> buf;
    boost::system::error_code error;
        std::size_t ba = socket.read_some(boost::asio::buffer(buf), error);

        telnetpp::bytes content(reinterpret_cast<const telnetpp::byte*>(buf.data()),
                                reinterpret_cast<const telnetpp::byte*>(buf.data() + buf.size()));

        channel.receive(content); // пересылайте все полученные от сокета данные в сеанс/канал Telnet++


    // пример чтения обработанных данных из сеанса/канала Telnet++
    /*session.async_read([](telnetpp::bytes data) {
        std::string ba(data.begin(), data.end());

        // ... сделайте что-нибудь с полученными и обработанными по Telnet++ данными ...

        std::cout << ba;
    });*/

    // подключиться к хосту
    //socket.connectToHost("127.0.0.1", 23); // просто назначьте узел/порт telnet
    //socket.waitForConnected();

    // пример отправки данных в сокет через предварительную обработку с помощью сеанса/канала Telnet++
    std::string sampleData = "hello!";
    if(socket.is_open())
    {
        telnetpp::bytes content(reinterpret_cast<const telnetpp::byte*>(sampleData.data()),
                                reinterpret_cast<const telnetpp::byte*>(sampleData.data() + sampleData.size()));
        session.write(content);
    }

    // ...
}



int main()
{
    TelnetPPSample();
    //std::getchar();

    /*std::string dest_ip;
    std::string dest_port;

    if (argc != 3)
    {
        std::cerr << "Usage: telnet <host> <port>\n";
        return 1;
    }
    else
    {
        dest_ip = argv[1];
        dest_port = argv[2];
    }

    try
    {
        std::cout << "SimpleTelnetClient is tring to connect " << dest_ip << ":" << dest_port << std::endl;

        boost::asio::io_service io_service;

        // resolve the host name and port number to an iterator that can be used to connect to the server
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(dest_ip, dest_port);
        tcp::resolver::iterator iterator = resolver.resolve(query);
        // define an instance of the main class of this program

        AsioTelnetClient telnet_client(io_service, iterator);

        // set a callback lambda function to process a message when it's received from telnet server.
        telnet_client.setReceivedSocketCallback([](const std::string& message) {
            std::cout << message;
        });

        // set a callback lambda function to realize an socket problem event.
        telnet_client.setClosedSocketCallback([]() {
            std::cout << " # disconnected" << std::endl;
        });

        telnet_client.write("admin");
        std::cout telnet_client.getCurrentLineBuffer();
        telnet_client.write("cdtn0a0h");
        std::cout telnet_client.getCurrentLineBuffer();

        /*while (1)
        {
            char ch;
            std::cin.get(ch); // blocking wait for standard input

            if (ch == 3) // ctrl-C to end program
                break;

            telnet_client.write(ch);
            telnet_client.getCurrentLineBuffer();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }*/

    return 0;


}
