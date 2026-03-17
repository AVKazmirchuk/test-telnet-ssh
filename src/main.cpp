#include <iostream>

#include "telnetpp/session.hpp"

#include "boost/asio.hpp"
#include "boost/asio/ip/tcp.hpp"



//Предоставляемый пользователем класс, моделирующий концепцию канала
struct Channel
{

    boost::asio::ip::tcp &socket;
    std::function<void (telnetpp::bytes)> read_callback;

    //вызывается внутренне из telnetpp::session::async_callback для пересылки данных результата из сокета в приложение
    void async_read(std::function<void (telnetpp::bytes)> const &callback) {
        read_callback = callback;
    }

    //вызывается внутренне из telnetpp::session::write для записи данных в сокет
    void write(telnetpp::bytes data) {
        socket.write(QByteArray(reinterpret_cast<const char*>(data.data()), data.size()));
    }

    //вызывается внутренне из telnetpp::session, чтобы проверить, все еще активен ли сокет
    bool is_alive() const {
        return socket.state() == QAbstractSocket::ConnectedState;
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
    boost::asio::io_service io_service;
    boost::asio::ip::tcp socket{io_service};
    Channel channel{socket};
    telnetpp::session session{channel};

    // пример получения данных из сокета и записи в сеанс/канал Telnet++
    QObject::connect(&socket, &QTcpSocket::readyRead, [&](){
        QByteArray  ba = socket.readAll();

        telnetpp::bytes content(reinterpret_cast<const telnetpp::byte*>(ba.constData()),
                                reinterpret_cast<const telnetpp::byte*>(ba.constData() + ba.size()));

        channel.receive(content); // пересылайте все полученные от сокета данные в сеанс/канал
    });

    // пример чтения обработанных данных из сеанса/канала Telnet++
    session.async_read([](telnetpp::bytes data) {
        QByteArray ba = QByteArray(reinterpret_cast<const char*>(data.data()), data.size());

        // ... сделайте что-нибудь с полученными и обработанными по Telnet++ данными ...
    });

    // подключиться к хосту
    socket.connectToHost("127.0.0.1", 23); // просто назначьте узел/порт telnet
    socket.waitForConnected();

    // пример отправки данных в сокет через предварительную обработку с помощью сеанса/канала Telnet++
    QByteArray sampleData = "hello!";
    if(socket.state() == QAbstractSocket::ConnectedState)
    {
        telnetpp::bytes content(reinterpret_cast<const telnetpp::byte*>(sampleData.constData()),
                                reinterpret_cast<const telnetpp::byte*>(sampleData.constData() + sampleData.size()));
        session.write(content);
    }

    // ...
}



int main()
{
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
