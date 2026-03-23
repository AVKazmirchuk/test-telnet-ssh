/*// AVOID ERROR BECAUSE OF swprintf
#undef __STRICT_ANSI__
// #define FOR WIN7. CAN'T FIND sdkddkver.h ON MINGW
#define _WIN32_WINNT 0x0601

#include "libssh2_config.h"
#include <libssh2.h>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind/protect.hpp>

#include <utility>

using boost::asio::ip::tcp;
using std::string;
using std::cout;
using std::endl;
using boost::asio::deadline_timer;
using boost::asio::io_service;
using boost::protect;

// Please, note: The class isn't called TestSSH because I lack imagination.
// The name comes from the fact that this hasn't yet seen anything
// beyond basic testing.
/*!
  * \todo Create exceptions, instead of using logic_error
 */
/*class TestSSH
{
public:
    TestSSH();
    ~TestSSH();
    void Connect(string const& hostName, string const& user,
                 string const& password);
    void ExecuteCommand(string const& command);
private:
    static const int PORT = 22;
    enum state { WORK_DONE, STILL_WORKING };

    void LoopAsync(boost::function<state()> DoWork, bool isRead);
    void LoopTimer(boost::function<state()> DoWork);

    void ConnectSocket(string const& hostName);
    void CreateSSHSession();
    state DoHandshake();
    state DoAuthenticate();
    state DoCreateChannel();
    state DoExecute();
    state DoGetResult();
    state DoCloseChannel();
    void Cleanup();

    string userName;
    string password;
    string command;
    io_service ios;
    tcp::socket sock;
    deadline_timer dtimer;
    LIBSSH2_SESSION* session;
    LIBSSH2_CHANNEL *channel;
};

const int TestSSH::PORT;

TestSSH::TestSSH() : userName("None"), password("None"), command("None"),
                     sock(ios), dtimer(ios), session(nullptr), channel(nullptr)
{
}

TestSSH::~TestSSH()
{
    if (channel != nullptr)
    {
        libssh2_channel_free(channel);
        channel = nullptr;
    }

    if (session != nullptr)
    {
        libssh2_session_set_blocking(session, 1);
        libssh2_session_disconnect(session, "Disconnecting");
        libssh2_session_free(session);
        session = nullptr;
    }

    if (sock.is_open())
    {
        try
        {
            boost::system::error_code ec;
            sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            sock.close(ec);
        }
        catch (std::exception& ex)
        {
            // NO EXCEPTIONS IN DTOR, LOG IT SOMEWHERE
        }
    }
}

void TestSSH::Connect(string const& hostName, string const& user,
                      string const& password)
{
    // THESE ARE BLOCKING
    ConnectSocket(hostName);
    CreateSSHSession();

    userName = user;
    this->password = std::move(password);

    // NOW WE ENTER THE IO SERVICE LOOP
    // FOR THE HANDSHAKE, READ EVENT IS ENOUGH
    sock.async_read_some(boost::asio::null_buffers(),
                         boost::bind(&TestSSH::LoopAsync, this,
                                     protect(boost::bind(&TestSSH::DoHandshake, this)), true));
    ios.run();

    // THEN, THE AUTHENTICATION. THIS ONE GOES WITH A TIMER, FOR NOW.
    // NEED MORE TESTING TO SEE IF I CAN GET ASYNC READ/WRITE TO WORK
    ios.reset(); // RESET THE EVENT LOOP, SO IT MAY BE REUSED
    dtimer.expires_from_now(boost::posix_time::milliseconds(500));
    dtimer.async_wait(boost::bind(&TestSSH::LoopTimer, this,
                                  protect(boost::bind(&TestSSH::DoAuthenticate, this))));
    ios.run();
}

/*!
 * \todo Turn this into a loop
 */
/*void TestSSH::ExecuteCommand(string const& command)
{
    this->command = command;

    ios.reset();
    sock.async_read_some(boost::asio::null_buffers(),
                         boost::bind(&TestSSH::LoopAsync, this,
                                     protect(boost::bind(&TestSSH::DoCreateChannel, this)), true));
    sock.async_write_some(boost::asio::null_buffers(),
                          boost::bind(&TestSSH::LoopAsync, this,
                                      protect(boost::bind(&TestSSH::DoCreateChannel, this)), false));
    ios.run();

    ios.reset();
    sock.async_read_some(boost::asio::null_buffers(),
                         boost::bind(&TestSSH::LoopAsync, this,
                                     protect(boost::bind(&TestSSH::DoExecute, this)), true));
    sock.async_write_some(boost::asio::null_buffers(),
                          boost::bind(&TestSSH::LoopAsync, this,
                                      protect(boost::bind(&TestSSH::DoExecute, this)), false));
    ios.run();

    ios.reset();
    sock.async_read_some(boost::asio::null_buffers(),
                         boost::bind(&TestSSH::LoopAsync, this,
                                     protect(boost::bind(&TestSSH::DoGetResult, this)), true));
    sock.async_write_some(boost::asio::null_buffers(),
                          boost::bind(&TestSSH::LoopAsync, this,
                                      protect(boost::bind(&TestSSH::DoGetResult, this)), false));
    ios.run();

    ios.reset();
    sock.async_read_some(boost::asio::null_buffers(),
                         boost::bind(&TestSSH::LoopAsync, this,
                                     protect(boost::bind(&TestSSH::DoCloseChannel, this)), true));
    sock.async_write_some(boost::asio::null_buffers(),
                          boost::bind(&TestSSH::LoopAsync, this,
                                      protect(boost::bind(&TestSSH::DoCloseChannel, this)), false));
    ios.run();

    // THIS IS BLOCKING
    Cleanup();
}

void TestSSH::LoopAsync(boost::function<state()> DoWork, bool isRead)
{
    if (sock.get_io_service().stopped())
    {
        return;
    }

    state st = DoWork();

    if (st == STILL_WORKING)
    {
        if (isRead)
        {
            sock.async_read_some(boost::asio::null_buffers(),
                                 boost::bind(&TestSSH::LoopAsync, this, DoWork, isRead));
        }
        else
        {
            sock.async_write_some(boost::asio::null_buffers(),
                                  boost::bind(&TestSSH::LoopAsync, this, DoWork, isRead));
        }
        return;
    }

    // OTHERWISE, WORK IS OVER
    sock.get_io_service().stop();
}

void TestSSH::LoopTimer(boost::function<state()> DoWork)
{
    state st = DoWork();

    if (st == STILL_WORKING)
    {
        dtimer.expires_from_now(boost::posix_time::milliseconds(500));
        dtimer.async_wait(boost::bind(&TestSSH::LoopTimer, this, DoWork));
    }
    // OTHERWISE, WORK IS OVER; NOTHING TO DO, AS THE TIMER IS NOT REARMED
}

void TestSSH::ConnectSocket(string const& hostName)
{
    tcp::resolver rsv(ios);
    tcp::resolver::query query(hostName,
                               boost::lexical_cast< std::string >(PORT));
    tcp::resolver::iterator iter = rsv.resolve(query);
    boost::asio::connect(sock, iter);
}

void TestSSH::CreateSSHSession()
{
    session = libssh2_session_init();
    if (!session)
    {
        throw std::logic_error("Error creating session");
    }

    libssh2_session_set_blocking(session, 0);
}

TestSSH::state TestSSH::DoHandshake()
{
    int rc = libssh2_session_handshake(session, sock.native_handle());

    if (rc == LIBSSH2_ERROR_EAGAIN)
    {
        return STILL_WORKING;
    }

    if (rc)
    {
        throw std::logic_error("Error in SSH handshake");
    }

    return WORK_DONE;
}

TestSSH::state TestSSH::DoAuthenticate()
{
    int rc = libssh2_userauth_password(session, userName.c_str(),
                                       password.c_str());

    if (rc == LIBSSH2_ERROR_EAGAIN)
    {
        return STILL_WORKING;
    }

    if (rc)
    {
        throw std::logic_error("Error in authentication");
    }

    return WORK_DONE;
}

TestSSH::state TestSSH::DoCreateChannel()
{
    channel = libssh2_channel_open_session(session);

    if (channel == nullptr)
    {
        int rc = libssh2_session_last_error(session, nullptr, nullptr, 0);

        if (rc == LIBSSH2_ERROR_EAGAIN)
        {
            return STILL_WORKING;
        }
        else
        {
            throw std::logic_error("Error opening channel");
        }
    }

    return WORK_DONE;
}

TestSSH::state TestSSH::DoExecute()
{
    int rc = libssh2_channel_exec(channel, command.c_str());

    if (rc == LIBSSH2_ERROR_EAGAIN)
    {
        return STILL_WORKING;
    }

    if (rc)
    {
        throw std::logic_error("Error executing command");
    }

    return WORK_DONE;
}

TestSSH::state TestSSH::DoGetResult()
{
    char buffer[0x4001];
    int rc = libssh2_channel_read(channel, buffer, sizeof(buffer)-1);

    if (rc > 0)
    {
        // BUFFER IS NOT AN SZ-STRING, SO WE TURN IT INTO ONE
        buffer[rc] = '\0';
        string str = buffer;
        cout << str << endl;

        // WE'RE NOT FINISHED UNTIL WE READ 0
        return STILL_WORKING;
    }

    if (rc == LIBSSH2_ERROR_EAGAIN)
    {
        return STILL_WORKING;
    }

    if (rc)
    {
        throw std::logic_error("Error getting execution result");
    }

    return WORK_DONE;
}

TestSSH::state TestSSH::DoCloseChannel()
{
    int rc = libssh2_channel_close(channel);

    if (rc == LIBSSH2_ERROR_EAGAIN)
    {
        return STILL_WORKING;
    }

    if (rc)
    {
        throw std::logic_error("Error closing channel");
    }

    return WORK_DONE;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Wunused-variable"
void TestSSH::Cleanup()
{
    int exitcode = libssh2_channel_get_exit_status( channel );
    char *buf =
            static_cast<char*>("A buffer for getting the exit signal, if any");
    libssh2_channel_get_exit_signal(channel, &buf, nullptr, nullptr,
                                    nullptr, nullptr, nullptr);

    if (buf == nullptr)
    {
        cout << "No signal" << endl;
    }
    else
    {
        string str = buf;
        cout << (str.length() == 0 ? "No signal description" : str) << endl;
    }

    libssh2_channel_free(channel);
    channel = nullptr;
}
#pragma GCC diagnostic pop

int main(int argc, char *argv[])
{
    string hostname("127.0.0.1");
    string commandline("pwd");
    string username("user");
    string password("********"); // STRONG PASSWORD :)

    if (argc > 1)
    {
        hostname = argv[1];
    }

    if (argc > 2)
    {
        username = argv[2];
    }

    if (argc > 3) {
        password = argv[3];
    }

    if (argc > 4) {
        commandline = argv[4];
    }

    int rc = libssh2_init (0);
    if (rc != 0) {
        fprintf (stderr, "libssh2 initialization failed (%d)\n", rc);
        return 1;
    }

    try
    {
        TestSSH tssh;
        tssh.Connect(hostname, username, password);
        tssh.ExecuteCommand(commandline);
    }
    catch (std::exception& ex)
    {
        cout << ex.what() << endl;
    }

    libssh2_exit();
    return 0;
}*/

























#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BUFFERSIZE 200

int decode_message(SOCKET mainSocket, char messageBuffer[BUFFERSIZE], char returnBuffer[BUFFERSIZE], int messageLength) {
    // Telnet protocol characters
    char IAC  = '\xff';   // "Interpret As Command"
    char DONT = '\xfe';
    char DO   = '\xfd';
    char WONT = '\xfc';
    char WILL = '\xfb';
    char theNULL = '\x00';
    char ECHO = '\x01';

    // special buffer to hold telnet commands received from server
    char commandBuffer[3];
    int commandBufferLocation = 0;

    // save location in return buffer
    int returnLocation = 0;

    for (int i = 0; i < messageLength; i++) {
        // read byte from buffer
        char c = messageBuffer[i];

        if (commandBufferLocation == 0) {
            if (c == theNULL) {
                // If null symbol, then do nothing
                continue;
            }
            else if (c != IAC) {
                // If it is no command, then add it to returnBuffer
                returnBuffer[returnLocation] = c;
                returnLocation++;
            }
            else {
                // If it is command start, then add it to buffer
                commandBuffer[commandBufferLocation] = c;
                commandBufferLocation++;
            }
        }
        else if (commandBufferLocation == 1) {
            // If there is second negotiation command
            if (c == DO || c == DONT || c == WILL || c == WONT) {
                // Add command to message buffer
                commandBuffer[commandBufferLocation] = c;
                commandBufferLocation++;
            }
            else {
                // Reset bufferLocation
                commandBufferLocation = 0;
            }
        }
        else if (commandBufferLocation == 2) {
            // response for DO and DONT
            if (commandBuffer[1] == DO || commandBuffer[1] == DONT) {
                // dont accept this command
                commandBuffer[1] = WONT;
                commandBuffer[2] = c;
                send(mainSocket, commandBuffer, 3, 0);
            }
            else if (commandBuffer[1] == WILL || commandBuffer[1] == WONT) {
                // dont accept this command
                commandBuffer[1] = DONT;
                commandBuffer[2] = c;
                send(mainSocket, commandBuffer, 3, 0);
            }

            // set bufferLocation to start
            commandBufferLocation = 0;
        }
    }

    // return message
    return returnLocation;
}

int main(int argc, char* argv[]) {
    // Arguments
    char IpAddress[15] = "192.168.4.13";    // 10.178.18.4 52.1.193.111    longest possible IPv4 address have only 15 characters
    unsigned short portNumber = 23;    // port number
    int recvTimeout = 500;  // how long client wait for server response
    int echoVerification = 1;   // echo verification on client side

    // Use Ip provided by user
    if (argc > 1) {
        if (strlen(argv[1]) > 15) {
            printf("Wrong IP address format \n");
            return 0;
        }
        else {
            strcpy(IpAddress, argv[1]);
        }
    }

    // check port number
    if (argc > 2) {
        portNumber = (unsigned short)atoi(argv[2]);
        if (portNumber == 0 || portNumber > 65535) {
            printf("Port number out of range. Use a number between 0 and 65535\n");
            return 1;
        }
    }

    // check recv timeout
    if (argc > 3) {
        recvTimeout = atoi(argv[3]);
        if (recvTimeout < 0 || recvTimeout > 600000) {
            printf("Recv timeout out of range. Use a number between 0 and 600000\n");
            return 1;
        }
    }

    // check echo verification
    if (argc > 4) {
        int temp = atoi(argv[4]);
        if (temp == 1) {
            echoVerification = 1;
        }
        else if (temp == 0) {
            echoVerification = 0;
        }
        else {
            printf("Out of range, use only 0 - inactive and 1 - active\n");
            return 1;
        }
    }

    // Initialize variables
    WSADATA wsaData;
    WORD LibVersion = MAKEWORD(2, 2);
    int comm_status = WSAStartup(LibVersion, &wsaData);

    if (comm_status == 0) {
        printf("Connection OK\n");
    }
    else {
        printf("ERROR\n");
        return 0;
    }

    // Create a socket
    SOCKET mainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (mainSocket == INVALID_SOCKET) {
        printf("Error creating socket: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Connect to a server
    struct sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(IpAddress);  // Replace with the server's IP address
    clientService.sin_port = htons(portNumber);  // Use the same port as the server

    // Set recv timeout
    comm_status = setsockopt(mainSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvTimeout, sizeof(int));
    if (comm_status == SOCKET_ERROR) {
        printf("setsockopt for SO_RCVTIMEO failed with error: %u\n", WSAGetLastError());
    } else {
        printf("Set SO_RCVTIMEO: ON\n");
    }

    // Use the connect function
    if (connect(mainSocket, (struct sockaddr *)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
        printf("Failed to connect: \n");
        WSACleanup();
        return 0;
    } else {
        printf("Client connected \n");
    }

    // communication buffers
    char receiveBuffer[BUFFERSIZE];
    int receiveBufferLen = 0;
    char senderBuffer[BUFFERSIZE];
    int senderBufferLen = 0;
    int echoFlag = 0;   // 1 when message is echo

    // Handle server connection
    while (1) {
        // empty the receiveBuffer
        memset(receiveBuffer, '\0', sizeof(receiveBuffer));

        // Read server message
        receiveBufferLen = recv(mainSocket, receiveBuffer, BUFFERSIZE, 0);

        // make sure that echo flag is 0
        echoFlag = 0;

        // Make sure, there is no echo. Some servers sends echo even if it is disable, so this is to make sure that there is no echo
        if (senderBufferLen > 1 && echoVerification) {
            if (strncmp(receiveBuffer, senderBuffer, senderBufferLen - 1) == 0) {
                echoFlag = 1;
            }
        }

        // empty the senderBuffer
        memset(senderBuffer, '\0', sizeof(senderBuffer));

        // If there are some bytes in buffer, then decode message
        if (receiveBufferLen > 0) {
            // Read server message
            senderBufferLen = decode_message(mainSocket, receiveBuffer, senderBuffer, receiveBufferLen);

            // Display server message
            if (echoFlag == 0) {
                printf("%s", senderBuffer);
            }
        }
        else {
            // Display start writing mark
            printf(": ");

            // Read user input
            fgets(senderBuffer, BUFFERSIZE, stdin);

            // check if termination command was send by user
            if (strcmp(senderBuffer, "exit\n") == 0) {
                break;
            }

            // Send message to a server
            senderBufferLen = send(mainSocket, senderBuffer, strlen(senderBuffer), 0);

            // Handle error
            if (senderBufferLen == SOCKET_ERROR) {
                printf("Server error: \n");
                break;
            }
        }
    }

    // Shutdown the connection
    shutdown(mainSocket, SD_SEND); // Shutdown the sending side

    // Close the socket
    closesocket(mainSocket);

    // Cleanup Winsock
    WSACleanup();

    printf("END \n");
    return 0;
}//*/