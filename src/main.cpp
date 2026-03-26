#if 0
#include "DNSResolver.h"
#include "ConfigManager.h"
#include <iostream>

using namespace std;

//@brief Main function that runs the DNS lookup tool.
int main(int argc, char* argv[])
{
    ConfigManager config(argc, argv);

    // Retrieve the domain name from the configuration
    string domain = config.getConfig("domain");
    if (domain.empty())
    {
        cerr << "Usage: main domain=<domain_name>" << endl;
        return 1;
    }

    DNSResolver dnsResolver;
    auto results = dnsResolver.resolveDomain(domain);  // Perform the domain resolution and store the results
    dnsResolver.logResults(domain, results);   // Log the IP's for the resolved domain

    return 0;
}
#endif
























#define _WIN32_WINNT 0x0600  // Ensure Windows 7+ API availability

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <limits>
#include <memory>

// Links the Winsock2 library for networking functions
#pragma comment(lib, "Ws2_32.lib")

// RAII wrapper to initialize and clean up Winsock automatically
class WinsockInitializer
{
public:
    // Constructor
    WinsockInitializer()
    {
        // Starts Winsock v2.2; throws an error if initialization fails
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            throw std::runtime_error("WSAStartup failed.");
        }
    }

    // Destructor
    ~WinsockInitializer()
    {
        WSACleanup();
    }

private:
    // Structure to hold Winsock data
    WSADATA wsaData;
};

// This class provides methods to resolve domain names to IP addresses and perform reverse DNS lookups
// It uses the Winsock API to fetch network information
class DNSResolver
{
public:

    /**
     * Resolves the given domain name to its IP addresses.
     * @param[in] domain The domain name to resolve (e.g., "google.com").
     * @param[in] family Address family (AF_INET for IPv4, AF_INET6 for IPv6, AF_UNSPEC for both).
     */
    void resolveDNS(const std::string& domain, int family = AF_UNSPEC)
    {
        std::cout << "\nResolving: " << domain << "\n";
        struct addrinfo hints = {}, *res = nullptr;

        // Specifies whether to resolve for IPv4, IPv6, or both
        hints.ai_family = family;

        // Sets the socket type to TCP
        hints.ai_socktype = SOCK_STREAM;

        // Perform DNS lookup
        int status = getaddrinfo(domain.c_str(), nullptr, &hints, &res);
        if (status != 0)
        {
            std::cerr << "Error: Could not resolve " << domain << ". " << gai_strerror(status) << "\n";
            return;
        }

        // Automatically frees addrinfo to prevent memory leaks and ensure exception safety
        std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)> res_ptr(res, freeaddrinfo);

        // Buffer to store the resolved IP address
        char ipStr[NI_MAXHOST];

        std::cout << "Addresses:\n";

        // Iterate through all resolved addresses and print them
        for (struct addrinfo* p = res_ptr.get(); p != nullptr; p = p->ai_next)
        {
            // Variable to hold the length of the IP string
            DWORD ipStrLen = NI_MAXHOST;

            // Convert the resolved address into a human-readable format
            if (WSAAddressToStringA(p->ai_addr, p->ai_addrlen, nullptr, ipStr, &ipStrLen) == 0)
            {
                std::cout << "  " << ipStr << "\n";
            }
            else
            {
                std::cerr << "Warning: Failed to convert address. Skipping...\n";
            }
        }
    }

    /**
     * Perform a reverse DNS lookup to find the hostname for a given IP address.
     * @param[in] ip The IPv4 address to resolve.
     */
    void reverseDNSLookup(const std::string& ip)
    {
        std::cout << "\nReverse Lookup: " << ip << "\n";

        // Structure to store the IP address information
        struct sockaddr_in sa;

        // Specify that the address belongs to the IPv4 family
        sa.sin_family = AF_INET;

        // Convert the IP string to a network address format
        sa.sin_addr.s_addr = inet_addr(ip.c_str());

        // Validate if the IP format is correct
        if (sa.sin_addr.s_addr == INADDR_NONE)
        {
            std::cerr << "Invalid IP format. Please enter a valid IPv4 address.\n";
            return;
        }

        char host[NI_MAXHOST];

        // Perform reverse DNS lookup to get the domain name
        if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, NI_MAXHOST, nullptr, 0, NI_NAMEREQD) == 0)
        {
            std::cout << "Resolved Hostname: " << host << "\n";
        }
        else
        {
            std::cerr << "Reverse lookup failed for " << ip << "\n";
        }
    }

    /**
     * Resolves multiple domain names sequentially.
     * @param[in] domains A list of domain names to resolve.
     * @param[in] family Address family (IPv4, IPv6, or both).
     */
    void resolveMultipleDomains(const std::vector<std::string>& domains, int family = AF_UNSPEC)
    {
        for (const auto& domain : domains)
        {
            resolveDNS(domain, family);
        }
    }
};

// This class handles user input, ensuring valid numerical input and choices
// It provides methods for selecting an option and choosing an address family
class UserInputHandler
{
public:

    /**
     * Prompts the user to enter a numerical choice.
     * Ensures valid integer input and prevents non-numeric or invalid characters.
     *
     * @return The validated integer choice entered by the user.
     */
    int getUserChoice()
    {
        int choice;
        while (true)
        {
            // Display Options and Prompt the user for input
            try
            {
                std::cin >> choice;

                if (std::cin.fail())
                {
                    throw std::invalid_argument("Invalid input. Please enter a number.");
                }

                // Clear any remaining input in the buffer to prevent unintended behavior
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return choice;
            }
                // Handle invalid input by resetting the input stream and displaying an error message
            catch (const std::exception& e)
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cerr << e.what() << "\n";
            }
        }
    }

    /**
     * Prompts the user to select an address family for network communication.
     * Valid options:
     *   1 - IPv4 only
     *   2 - IPv6 only
     *   3 - Both (default)
     *
     * @return The corresponding address family (AF_INET for IPv4, AF_INET6 for IPv6, AF_UNSPEC for both).
     */
    int getFamilyChoice()
    {
        int family;
        while (true)
        {
            // Display options and prompt the user to select an address family
            try
            {
                std::cout << "Select Address Family:\n";
                std::cout << "1. IPv4 only\n";
                std::cout << "2. IPv6 only\n";
                std::cout << "3. Both (default)\n";
                std::cout << "Enter choice: ";
                std::cin >> family;

                if (std::cin.fail() || (family < 1 || family > 3))
                {
                    throw std::invalid_argument("Invalid input. Enter 1, 2, or 3.");
                }

                // Clear any extra input and return the corresponding address family
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (family == 1) return AF_INET;
                if (family == 2) return AF_INET6;
                return AF_UNSPEC;

            }
                // Handle invalid input by resetting the input stream and displaying an error message
            catch (const std::exception& e)
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cerr << e.what() << "\n";
            }
        }
    }
};

int main()
{
    try
    {
        WinsockInitializer winsock;  // RAII ensures WSACleanup is called
        DNSResolver resolver;
        UserInputHandler inputHandler;

        // Display menu options for the user
        std::cout << "1. Resolve Domain\n";
        std::cout << "2. Reverse DNS Lookup\n";
        std::cout << "3. Resolve Multiple Domains\n";
        std::cout << "Choose an option: ";

        // Get user choice and validate input
        int choice = inputHandler.getUserChoice();

        // Get address family preference (IPv4, IPv6, or both)
        if (choice == 1)
        {
            // Resolve a single domain name
            std::string domain;
            std::cout << "Enter domain: ";
            std::getline(std::cin, domain);

            // Get address family preference (IPv4, IPv6, or both)
            int family = inputHandler.getFamilyChoice();
            resolver.resolveDNS(domain, family);
        }
        else if (choice == 2)
        {
            // Perform reverse DNS lookup for an IP address
            std::string ip;
            std::cout << "Enter IP address: ";
            std::getline(std::cin, ip);
            resolver.reverseDNSLookup(ip);
        }
        else if (choice == 3)
        {
            // Resolve multiple domain names
            int count;
            std::cout << "Enter number of domains: ";
            count = inputHandler.getUserChoice();

            std::vector<std::string> domains(count);

            // Collect domain names from the user
            for (int i = 0; i < count; ++i)
            {
                std::cout << "Enter domain " << (i + 1) << ": ";
                std::getline(std::cin, domains[i]);
            }

            // Get address family preference
            int family = inputHandler.getFamilyChoice();
            resolver.resolveMultipleDomains(domains, family);
        }
        else
        {
            std::cerr << "Invalid choice. Exiting.\n";
        }
    }
    catch (const std::exception& e)
    {
        // Handle any exceptions thrown during execution
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}





































#if 0
//#include "stdafx.h"

#include <stdio.h>
#include <WINSOCK2.H>
#include <stdlib.h>
#include <conio.h>
#include <iostream>



typedef struct tagIPINFO
{
    u_char Ttl; // Time To Live
    u_char Tos; // Type Of Service
    u_char IPFlags; // IP flags
    u_char OptSize; // Size of options data
    u_char FAR *Options; // Options data buffer
}IPINFO, *PIPINFO;


typedef struct tagICMPECHO
{
    u_long Source; // Source address
    u_long Status; // IP status
    u_long RTTime; // Round trip time in milliseconds
    u_short DataSize; // Reply data size
    u_short Reserved; // Unknown
    void FAR *pData; // Reply data buffer
    IPINFO ipInfo; // Reply options
}ICMPECHO, *PICMPECHO;




// ICMP.DLL Export Function Pointers
HANDLE (WINAPI *pIcmpCreateFile)(VOID);
BOOL (WINAPI *pIcmpCloseHandle)(HANDLE);
DWORD (WINAPI *pIcmpSendEcho)
        (HANDLE,DWORD,LPVOID,WORD,PIPINFO,LPVOID,DWORD,DWORD);


//
//
int main(int argc, char **argv)
{
    WSADATA wsaData; // WSADATA
    ICMPECHO icmpEcho; // ICMP Echo reply buffer
    HMODULE hndlIcmp; // LoadLibrary() handle to ICMP.DLL
    HANDLE hndlFile; // Handle for IcmpCreateFile()
    LPHOSTENT pHost; // Pointer to host entry structure
    struct in_addr iaDest; // Internet address structure
    DWORD dwAddress; // IP Address
    IPINFO ipInfo; // IP Options structure
    int nRet; // General use return code
    DWORD dwRet; // DWORD return code
    int x;


    // Check arguments
    /*if (argc != 2)
    {
        fprintf(stderr,"\nSyntax: ping.exe [HostName]Or[IPAddress]\n");
        return 1;
    }*/



    // Dynamically load the ICMP.DLL
    hndlIcmp = LoadLibrary("ICMP.DLL");
    if (hndlIcmp == NULL)
    {
        fprintf(stderr,"\nCould not load ICMP.DLL\n");
        return 1;
    }
    // Retrieve ICMP function pointers
    pIcmpCreateFile = (HANDLE (WINAPI *)(void))
            GetProcAddress(hndlIcmp,"IcmpCreateFile");
    pIcmpCloseHandle = (BOOL (WINAPI *)(HANDLE))
            GetProcAddress(hndlIcmp,"IcmpCloseHandle");
    pIcmpSendEcho = (DWORD (WINAPI *)
            (HANDLE,DWORD,LPVOID,WORD,PIPINFO,LPVOID,DWORD,DWORD))
            GetProcAddress(hndlIcmp,"IcmpSendEcho");
    // Check all the function pointers
    if (pIcmpCreateFile == NULL ||
        pIcmpCloseHandle == NULL ||
        pIcmpSendEcho == NULL)
    {
        fprintf(stderr,"\nError getting ICMP proc address\n");
        FreeLibrary(hndlIcmp);
        return 1;
    }

    printf("1. Dll loaded... ok \n");

    // Init WinSock
    nRet = WSAStartup(0x0101, &wsaData );
    if (nRet)
    {
        fprintf(stderr,"\nWSAStartup() error: %d\n", nRet);
        WSACleanup();
        FreeLibrary(hndlIcmp);
        return 1;
    }
    printf("2. Init WinSock  ...ok\n");

    // Check WinSock version
    if (0x0101 != wsaData.wVersion)
    {
        fprintf(stderr,"\nWinSock version 1.1 not supported\n");
        WSACleanup();
        FreeLibrary(hndlIcmp);
        return 1;
    }


    // Lookup destination
    // Use inet_addr() to determine if we're dealing with a name
    // or an address
    printf("3. Getting WinSock version 1.1 ...ok\n");

    iaDest.s_addr = inet_addr("169.254.0.1");
    dwAddress=inet_addr(argv[1]);
    printf("4. Getting ip address...\n");

    if (dwAddress == INADDR_NONE)
    {
        printf("5. No ip looking for hostname ... \n");
        pHost = gethostbyname(argv[1]);
        if (pHost==NULL)
        {
            printf("5. Warning! Can't find host name...ok\nPress any key to exit.\n");
            getch();
            return 1;
        }

        CopyMemory( &dwAddress,pHost->h_addr_list[0],pHost->h_length);
        if (dwAddress == INADDR_NONE)
        {
            printf("5. Warning! Can't find host name.\nPress any key to exit.\n");
            getch();
            return 1;
        }

    }
    else
    {    printf("5.1. Address ok)\n");

        //pHost = gethostbyaddr(inet_addr("169.254.0.1"),
        //              sizeof(struct in_addr), AF_INET);
    }
    printf("6. Host != NULL ...ok\n");
    if (pHost == NULL)
    {
        fprintf(stderr, "\n%s not found\n", argv[1]);
        WSACleanup();
        FreeLibrary(hndlIcmp);
        return 1;
    }
    printf("7. Data and functionst are ...ok \n");
    printf("8./////////////////PING//////////////// \n");

    // Tell the user what we're doing




    // Copy the IP address

    //CopyMemory( &dwAddress,pHost->h_addr_list[0],pHost->h_length); //inet_addr("169.254.0.3");
    //  dwAddress=inet_addr("169.254.0.9");

//         printf(" CopyMemory ok \n");

    // Get an ICMP echo request handle
    hndlFile = pIcmpCreateFile();
    printf("pIcmpCreateFile() ... ok \n");
    for (x = 0; x <5;x++)
    {      // Set some reasonable default values
        ipInfo.Ttl = 255;
        ipInfo.Tos = 0;
        ipInfo.IPFlags = 0;
        ipInfo.OptSize = 0;
        ipInfo.Options = NULL;
        //icmpEcho.ipInfo.Ttl = 256;
        // Reqest an ICMP echo

        char buf[32] = "qazwsxedcrfvtgbyhnujmik,ol.p;/[";


        dwRet = pIcmpSendEcho(
                hndlFile, // Handle from IcmpCreateFile()
                dwAddress, // Destination IP address
                NULL, // Pointer to buffer to send
                0, // Size of buffer in bytes
                &ipInfo, // Request options
                &icmpEcho, // Reply buffer
                sizeof(struct tagICMPECHO),
                5000); // Time to wait in milliseconds
        printf("pIcmpSendEcho ...ok\n");
        // Print the results
        iaDest.s_addr = icmpEcho.Source;
        printf("Reply from %s Time=%ldms TTL=%d\n",
               inet_ntoa(iaDest),
               icmpEcho.RTTime,
               icmpEcho.ipInfo.Ttl);
        std::cout << ", dataSize=" << icmpEcho.DataSize;
        if (icmpEcho.Status)
        {
            printf("\nError: icmpEcho.Status=%ld\n",icmpEcho.Status);
            //  return ;
            break;
        }
    }
    printf("9./////////////////PING//////////////// \n");
    printf("Press any key to exit.\n");
    // Close the echo request file handle
    pIcmpCloseHandle(hndlFile);
    FreeLibrary(hndlIcmp);
    WSACleanup();
    getch();

    return 0;
}
#endif

































#if 0
/* Copyright (C) The libssh2 project and its contributors.
 *
 * Sample showing how to use libssh2 to execute a command remotely.
 *
 * The sample code has fixed values for host name, user name, password
 * and command to run.
 *
 * $ ./ssh2_exec 127.0.0.1 user password "uptime"
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//#include <os400/libssh2_config.h>
#include <libssh2.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
//#include <winsock2.h>



static const char *hostname = "10.178.18.4"; //10.178.18.4
static const char *commandline = "sh run";
static const char *pubkey = "/home/username/.ssh/id_rsa.pub";
static const char *privkey = "/home/username/.ssh/id_rsa";
static const char *username = "admin";
static const char *password = "cdtn0a0h";

static int waitsocket(libssh2_socket_t socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO(&fd);

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
    FD_SET(socket_fd, &fd);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(session);

    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
        readfd = &fd;

    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
        writefd = &fd;

    rc = select((int)(socket_fd + 1), readfd, writefd, NULL, &timeout);

    return rc;
}

int main(int argc, char *argv[])
{

    uint32_t hostaddr;
    libssh2_socket_t sock;
    struct sockaddr_in sin;
    const char *fingerprint;
    int rc;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel;
    int exitcode;
    char *exitsignal = (char *)"none";
    ssize_t bytecount = 0;
    size_t len;
    LIBSSH2_KNOWNHOSTS *nh;
    int type;

#ifdef _WIN32
    WSADATA wsadata;

    rc = WSAStartup(MAKEWORD(2, 0), &wsadata);
    if(rc) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", rc);
        return 1;
    }
#endif

    if(argc > 1) {
        hostname = argv[1];  /* must be ip address only */
    }
    if(argc > 2) {
        username = argv[2];
    }
    if(argc > 3) {
        password = argv[3];
    }
    if(argc > 4) {
        commandline = argv[4];
    }

    rc = libssh2_init(0);
    if(rc) {
        fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
        return 1;
    }

    hostaddr = inet_addr(hostname);

    /* Ultra basic "connect to port 22 on localhost".  Your code is
     * responsible for creating the socket establishing the connection
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == LIBSSH2_INVALID_SOCKET) {
        fprintf(stderr, "failed to create socket.\n");
        goto shutdown;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = hostaddr;
    if(connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in))) {
        fprintf(stderr, "failed to connect.\n");
        goto shutdown;
    }

    /* Create a session instance */
    session = libssh2_session_init();
    if(!session) {
        fprintf(stderr, "Could not initialize SSH session.\n");
        goto shutdown;
    }

    /* tell libssh2 we want it all done non-blocking */
    libssh2_session_set_blocking(session, 0);

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    while((rc = libssh2_session_handshake(session, sock)) ==
          LIBSSH2_ERROR_EAGAIN);
    if(rc) {
        fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
        goto shutdown;
    }

    nh = libssh2_knownhost_init(session);
    if(!nh) {
        /* eeek, do cleanup here */
        return 2;
    }

    /* read all hosts from here */
    libssh2_knownhost_readfile(nh, "known_hosts",
                               LIBSSH2_KNOWNHOST_FILE_OPENSSH);

    /* store all known hosts to here */
    libssh2_knownhost_writefile(nh, "dumpfile",
                                LIBSSH2_KNOWNHOST_FILE_OPENSSH);

    fingerprint = libssh2_session_hostkey(session, &len, &type);
    if(fingerprint) {
        struct libssh2_knownhost *host;
        int check = libssh2_knownhost_checkp(nh, hostname, 22,
                                             fingerprint, len,
                                             LIBSSH2_KNOWNHOST_TYPE_PLAIN|
                                             LIBSSH2_KNOWNHOST_KEYENC_RAW,
                                             &host);

        fprintf(stderr, "Host check: %d, key: %s\n", check,
                (check <= LIBSSH2_KNOWNHOST_CHECK_MISMATCH) ?
                host->key : "<none>");

        /*****
         * At this point, we could verify that 'check' tells us the key is
         * fine or bail out.
         *****/
    }
    else {
        /* eeek, do cleanup here */
        return 3;
    }
    libssh2_knownhost_free(nh);

    if(strlen(password) != 0) {
        /* We could authenticate via password */
        while((rc = libssh2_userauth_password(session, username, password)) ==
              LIBSSH2_ERROR_EAGAIN);
        if(rc) {
            fprintf(stderr, "Authentication by password failed.\n");
            goto shutdown;
        }

    }
    else {
        /* Or by public key */
        while((rc = libssh2_userauth_publickey_fromfile(session, username,
                                                        pubkey, privkey,
                                                        password)) ==
              LIBSSH2_ERROR_EAGAIN);
        if(rc) {
            fprintf(stderr, "Authentication by public key failed.\n");
            goto shutdown;
        }
    }

#if 0
    libssh2_trace(session, ~0);
#endif

    /* Exec non-blocking on the remote host */
    do {
        channel = libssh2_channel_open_session(session);
        if(channel ||
           libssh2_session_last_error(session, NULL, NULL, 0) !=
           LIBSSH2_ERROR_EAGAIN)
            break;
        waitsocket(sock, session);
    } while(1);
    if(!channel) {
        fprintf(stderr, "Error\n");
        exit(1);
    }
    while((rc = libssh2_channel_exec(channel, commandline)) ==
          LIBSSH2_ERROR_EAGAIN) {
        waitsocket(sock, session);
    }
    if(rc) {
        fprintf(stderr, "exec error\n");
        exit(1);
    }
    for(;;) {
        ssize_t nread;
        /* loop until we block */
        do {
            char buffer[0x4000];
            nread = libssh2_channel_read(channel, buffer, sizeof(buffer));
            if(nread > 0) {
                ssize_t i;
                bytecount += nread;
                fprintf(stderr, "We read:\n");
                for(i = 0; i < nread; ++i)
                    fputc(buffer[i], stderr);
                fprintf(stderr, "\n");
            }
            else {
                if(nread != LIBSSH2_ERROR_EAGAIN)
                    /* no need to output this for the EAGAIN case */
                    fprintf(stderr, "libssh2_channel_read returned %ld\n",
                            (long)nread);
            }
        } while(nread > 0);

        /* this is due to blocking that would occur otherwise so we loop on
           this condition */
        if(nread == LIBSSH2_ERROR_EAGAIN) {
            waitsocket(sock, session);
        }
        else
            break;
    }
    exitcode = 127;
    while((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
        waitsocket(sock, session);

    if(rc == 0) {
        exitcode = libssh2_channel_get_exit_status(channel);
        libssh2_channel_get_exit_signal(channel, &exitsignal,
                                        NULL, NULL, NULL, NULL, NULL);
    }

    if(exitsignal)
        fprintf(stderr, "\nGot signal: %s\n", exitsignal);
    else
        fprintf(stderr, "\nEXIT: %d bytecount: %ld\n",
                exitcode, (long)bytecount);

    libssh2_channel_free(channel);
    channel = NULL;

    shutdown:

    if(session) {
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
    }

    if(sock != LIBSSH2_INVALID_SOCKET) {
        shutdown(sock, 2);
        LIBSSH2_SOCKET_CLOSE(sock);
    }

    fprintf(stderr, "all done\n");

    libssh2_exit();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
#endif





















































/*#include <stdio.h>
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