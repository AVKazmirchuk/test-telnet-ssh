#include <iostream>

#include "telnetpp/session.hpp"

#include "telnetpp/client_option.hpp"

#include "boost/asio.hpp"
#include "boost/array.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/ip/address.hpp"

#include "winsock2.h"


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

    boost::array<char, 128> buf5;
    boost::system::error_code error5;
    std::size_t ba5 = socket.read_some(boost::asio::buffer(buf5), error5);
    std::cout << "\nfrom buffer: "; std::cout.write(buf5.data(), ba5);

    std::string str{"admin\r\n"};
    boost::asio::write(socket, boost::asio::buffer(str));

    boost::array<char, 128> buf7;
    boost::system::error_code error7;
    std::size_t ba7 = socket.read_some(boost::asio::buffer(buf7), error7);
    std::cout << "\nfrom buffer: "; std::cout.write(buf7.data(), ba7);

    std::string str2{"cdtn0a0h\r\n"};
    boost::asio::write(socket, boost::asio::buffer(str2));

    boost::array<char, 128> buf8;
    boost::system::error_code error8;
    std::size_t ba8 = socket.read_some(boost::asio::buffer(buf8), error8);
    std::cout << "\nfrom buffer: "; std::cout.write(buf8.data(), ba8);

    std::string str3{"sh run\r\n"};
    boost::asio::write(socket, boost::asio::buffer(str3));

    boost::array<char, 128> buf6;
    boost::system::error_code error6;
    std::size_t ba6 = socket.read_some(boost::asio::buffer(buf6), error6);
    std::cout << "\nfrom buffer: "; std::cout.write(buf6.data(), ba6);

    /*TelnetPPChannel channel{socket};
    telnetpp::session session{channel};



    // пример чтения обработанных данных из сеанса/канала Telnet++
    session.async_read([](telnetpp::bytes data) {
        std::vector<unsigned char> ba(data.begin(), data.end());
        std::cout << '\n';
        // ... сделайте что-нибудь с полученными и обработанными по Telnet++ данными ...
        for (auto elem : ba)
            std::cout << elem;

        std::cout << " - from session.async_read";
    });

// пример получения данных из сокета и записи в сеанс/канал Telnet++
    boost::array<char, 128> buf5;
    boost::system::error_code error5;

    std::size_t ba5 = socket.read_some(boost::asio::buffer(buf5), error5);
    telnetpp::bytes content5(reinterpret_cast<telnetpp::byte*>(buf5.data()),
                            reinterpret_cast<telnetpp::byte*>(buf5.data() + buf5.size()));
    channel.receive(content5); // пересылайте все полученные от сокета данные в сеанс/канал Telnet++

    std::cout << "\nfrom buffer: "; std::cout.write(buf5.data(), ba5);

    std::string str{"admin\r\n"};
    telnetpp::bytes content2(reinterpret_cast<const telnetpp::byte*>(str.data()),
                            reinterpret_cast<const telnetpp::byte*>(str.data() + str.size()));
    session.write(content2);



    // пример получения данных из сокета и записи в сеанс/канал Telnet++
    boost::array<char, 128> buf;
    boost::system::error_code error;
    std::size_t ba = socket.read_some(boost::asio::buffer(buf), error);
    telnetpp::bytes content(reinterpret_cast<const telnetpp::byte*>(buf.data()),
                                reinterpret_cast<const telnetpp::byte*>(buf.data() + buf.size()));
    channel.receive(content); // пересылайте все полученные от сокета данные в сеанс/канал Telnet++



    /*std::string str2{"cdtn0a0h\n"};
    telnetpp::bytes content3(reinterpret_cast<const telnetpp::byte*>(str2.data()),
                             reinterpret_cast<const telnetpp::byte*>(str2.data() + str2.size()));
    session.write(content3);



    // пример получения данных из сокета и записи в сеанс/канал Telnet++
    boost::array<char, 128> buf2;
    boost::system::error_code error2;
    std::size_t ba2 = socket.read_some(boost::asio::buffer(buf2), error2);
    telnetpp::bytes content4(reinterpret_cast<const telnetpp::byte*>(buf2.data()),
                                reinterpret_cast<const telnetpp::byte*>(buf2.data() + buf2.size()));
    channel.receive(content4); // пересылайте все полученные от сокета данные в сеанс/канал Telnet++





    // пример чтения обработанных данных из сеанса/канала Telnet++
    /*session.async_read([](telnetpp::bytes data) {
        std::string ba(data.begin(), data.end());

        ba += '\n';
        // ... сделайте что-нибудь с полученными и обработанными по Telnet++ данными ...

        std::cout << ba;
    });*/

    /*std::string str2{"cdtn0a0h\n"};
    telnetpp::bytes content3(reinterpret_cast<const telnetpp::byte*>(str2.data()),
                             reinterpret_cast<const telnetpp::byte*>(str2.data() + str2.size()));
    session.write(content3);

    // пример чтения обработанных данных из сеанса/канала Telnet++
    /*session.async_read([](telnetpp::bytes data) {
        std::string ba(data.begin(), data.end());

        ba += '\n';
        // ... сделайте что-нибудь с полученными и обработанными по Telnet++ данными ...

        std::cout << ba;
    });*/











    // пример получения данных из сокета и записи в сеанс/канал Telnet++

    /*boost::array<char, 128> buf;
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
    /*std::string sampleData = "hello!";
    if(socket.is_open())
    {
        telnetpp::bytes content(reinterpret_cast<const telnetpp::byte*>(sampleData.data()),
                                reinterpret_cast<const telnetpp::byte*>(sampleData.data() + sampleData.size()));
        session.write(content);
    }*/

    // ...
}



int main()
{
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char *message , server_reply[2000];
    int recv_size;

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    //Create a socket
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
    }

    printf("Socket created.\n");


    server.sin_addr.s_addr = inet_addr("10.178.18.4");
    server.sin_family = AF_INET;
    server.sin_port = htons( 23 );

    //Connect to remote server
    if (connect(s , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        puts("connect error");
        return 1;
    }

    puts("Connected");

//Send some data
    message = "admin\r\n\r\n";
    if( send(s , message , strlen(message) , 0) < 0)
    {
        puts("Send failed");
        return 1;
    }
    puts("Data Send\n");

    //Receive a reply from the server
    if((recv_size = recv(s , server_reply , 2000 , 0)) == SOCKET_ERROR)
    {
        puts("recv failed");
    }

    puts("Reply received\n");

    //Add a NULL terminating character to make it a proper string before printing
    server_reply[recv_size] = '\0';
    puts(server_reply);

//Send some data
    char *message2 = "cdtn0a0h\r\n\r\n";
    if( send(s , message2 , strlen(message2) , 0) < 0)
    {
        puts("Send failed");
        return 1;
    }
    puts("Data Send\n");

    //Receive a reply from the server
    if((recv_size = recv(s , server_reply , 2000 , 0)) == SOCKET_ERROR)
    {
        puts("recv failed");
    }

    puts("Reply received\n");

    //Add a NULL terminating character to make it a proper string before printing
    server_reply[recv_size] = '\0';
    puts(server_reply);

//Send some data
    char *message3 = "sh run\r\n\r\n";
    if( send(s , message3 , strlen(message3) , 0) < 0)
    {
        puts("Send failed");
        return 1;
    }
    puts("Data Send\n");

    //Receive a reply from the server
    if((recv_size = recv(s , server_reply , 2000 , 0)) == SOCKET_ERROR)
    {
        puts("recv failed");
    }

    puts("Reply received\n");

    //Add a NULL terminating character to make it a proper string before printing
    server_reply[recv_size] = '\0';
    puts(server_reply);

    /*WSADATA wsaData;
    WORD version = MAKEWORD(2, 2);
    if (WSAStartup(version, &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(23); // порт для telnet
    server.sin_addr.s_addr = inet_addr("10.178.18.4"); // адрес коммутатора

    connect(sock, (struct sockaddr*)&server, sizeof(server));

    char buffer0[1024];

        int bytes0 = recv(sock, buffer0, 1024, 0);
        //if (bytes == 0) break;
        std::cout << buffer0 << std::endl;


    std::string query = "admin\r\n";
    send(sock, query.c_str(), query.size(), 0);

    char buffer[1024];

    int bytes = recv(sock, buffer, 1024, 0);
    //if (bytes == 0) break;
    std::cout << buffer << std::endl;

    std::string query2 = "cdtn0a0h\r\n";
    send(sock, query2.c_str(), query2.size(), 0);

    char buffer2[1024];

    int bytes2 = recv(sock, buffer2, 1024, 0);
    //if (bytes == 0) break;
    std::cout << buffer2 << std::endl;

    std::string query3 = "sh run\r\n";
    send(sock, query3.c_str(), query3.size(), 0);

    char buffer3[1024];
    //while (true) {
        int bytes3 = recv(sock, buffer3, 1024, 0);
        //if (bytes == 0) break;
        std::cout << buffer3 << std::endl;
    //}
    close(sock);
    WSACleanup();*/




    //TelnetPPSample();
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
