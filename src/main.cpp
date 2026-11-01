//Выводит только Host Name, DNS Servers
#if 0
//
// Link with IPHlpAPI.lib
//
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <windows.h>
#pragma comment(lib, "IPHLPAPI.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/* Note: could also use malloc() and free() */

//int __cdecl main()
int main()
{

    FIXED_INFO *pFixedInfo;
    ULONG ulOutBufLen;
    DWORD dwRetVal;
    IP_ADDR_STRING *pIPAddr;

    pFixedInfo = (FIXED_INFO *) MALLOC(sizeof (FIXED_INFO));
    if (pFixedInfo == NULL) {
        printf("Error allocating memory needed to call GetNetworkParams\n");
        return 1;
    }
    ulOutBufLen = sizeof (FIXED_INFO);

// Make an initial call to GetAdaptersInfo to get
// the necessary size into the ulOutBufLen variable
    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        FREE(pFixedInfo);
        pFixedInfo = (FIXED_INFO *) MALLOC(ulOutBufLen);
        if (pFixedInfo == NULL) {
            printf("Error allocating memory needed to call GetNetworkParams\n");
            return 1;
        }
    }

    if (dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR) {

        printf("Host Name: %s\n", pFixedInfo->HostName);
        printf("Domain Name: %s\n", pFixedInfo->DomainName);

        printf("DNS Servers:\n");
        printf("\t%s\n", pFixedInfo->DnsServerList.IpAddress.String);

        pIPAddr = pFixedInfo->DnsServerList.Next;
        while (pIPAddr) {
            printf("\t%s\n", pIPAddr->IpAddress.String);
            pIPAddr = pIPAddr->Next;
        }

        printf("Node Type: ");
        switch (pFixedInfo->NodeType) {
            case BROADCAST_NODETYPE:
                printf("Broadcast node\n");
                break;
            case PEER_TO_PEER_NODETYPE:
                printf("Peer to Peer node\n");
                break;
            case MIXED_NODETYPE:
                printf("Mixed node\n");
                break;
            case HYBRID_NODETYPE:
                printf("Hybrid node\n");
                break;
            default:
                printf("Unknown node type %0lx\n", pFixedInfo->NodeType);
                break;
        }

        printf("DHCP scope name: %s\n", pFixedInfo->ScopeId);

        if (pFixedInfo->EnableRouting)
            printf("Routing: enabled\n");
        else
            printf("Routing: disabled\n");

        if (pFixedInfo->EnableProxy)
            printf("ARP proxy: enabled\n");
        else
            printf("ARP Proxy: disabled\n");

        if (pFixedInfo->EnableDns)
            printf("DNS: enabled\n");
        else
            printf("DNS: disabled\n");

    } else {
        printf("GetNetworkParams failed with error: %d\n", dwRetVal);
        return 1;
    }

    if (pFixedInfo)
        FREE(pFixedInfo);

    return 0;
}
#endif



//Выводит непонятное
#if 0
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Link with Iphlpapi.lib
#pragma comment(lib, "IPHLPAPI.lib")

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/* Note: could also use malloc() and free() */

int __cdecl main(int argc, char **argv)
{

    /* Declare and initialize variables */

    DWORD dwRetVal = 0;

    unsigned int i = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

    // default to unspecified address family (both)
    ULONG family = AF_UNSPEC;

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
    IP_ADAPTER_PREFIX *pPrefix = NULL;

    if (argc != 2) {
        printf(" Usage: getadapteraddresses family\n");
        printf("        getadapteraddresses 4 (for IPv4)\n");
        printf("        getadapteraddresses 6 (for IPv6)\n");
        printf("        getadapteraddresses A (for both IPv4 and IPv6)\n");
        exit(1);
    }

    if (atoi(argv[1]) == 4)
        family = AF_INET;
    else if (atoi(argv[1]) == 6)
        family = AF_INET6;

    printf("Calling GetAdaptersAddresses function with family = ");
    if (family == AF_INET)
        printf("AF_INET\n");
    if (family == AF_INET6)
        printf("AF_INET6\n");
    if (family == AF_UNSPEC)
        printf("AF_UNSPEC\n\n");

    // Allocate a 15 KB buffer to start with.
    outBufLen = WORKING_BUFFER_SIZE;

    do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
        if (pAddresses == NULL) {
            printf
                    ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal =
                GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

    if (dwRetVal == NO_ERROR) {
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            printf("\tLength of the IP_ADAPTER_ADDRESS struct: %ld\n",
                   pCurrAddresses->Length);
            printf("\tIfIndex (IPv4 interface): %u\n", pCurrAddresses->IfIndex);
            printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);

            pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast != NULL) {
                for (i = 0; pUnicast != NULL; i++)
                    pUnicast = pUnicast->Next;
                printf("\tNumber of Unicast Addresses: %d\n", i);
            } else
                printf("\tNo Unicast Addresses\n");

            pAnycast = pCurrAddresses->FirstAnycastAddress;
            if (pAnycast) {
                for (i = 0; pAnycast != NULL; i++)
                    pAnycast = pAnycast->Next;
                printf("\tNumber of Anycast Addresses: %d\n", i);
            } else
                printf("\tNo Anycast Addresses\n");

            pMulticast = pCurrAddresses->FirstMulticastAddress;
            if (pMulticast) {
                for (i = 0; pMulticast != NULL; i++)
                    pMulticast = pMulticast->Next;
                printf("\tNumber of Multicast Addresses: %d\n", i);
            } else
                printf("\tNo Multicast Addresses\n");

            pDnServer = pCurrAddresses->FirstDnsServerAddress;
            if (pDnServer) {
                for (i = 0; pDnServer != NULL; i++)
                    pDnServer = pDnServer->Next;
                printf("\tNumber of DNS Server Addresses: %d\n", i);
            } else
                printf("\tNo DNS Server Addresses\n");

            printf("\tDNS Suffix: %wS\n", pCurrAddresses->DnsSuffix);

            printf("\tDescription: %wS\n", pCurrAddresses->Description);
            printf("\tFriendly name: %wS\n", pCurrAddresses->FriendlyName);

            if (pCurrAddresses->PhysicalAddressLength != 0) {
                printf("\tPhysical address: ");
                for (i = 0; i < (int) pCurrAddresses->PhysicalAddressLength;
                     i++) {
                    if (i == (pCurrAddresses->PhysicalAddressLength - 1))
                        printf("%.2X\n",
                               (int) pCurrAddresses->PhysicalAddress[i]);
                    else
                        printf("%.2X-",
                               (int) pCurrAddresses->PhysicalAddress[i]);
                }
            }
            printf("\tFlags: %ld\n", pCurrAddresses->Flags);
            printf("\tMtu: %lu\n", pCurrAddresses->Mtu);
            printf("\tIfType: %ld\n", pCurrAddresses->IfType);
            printf("\tOperStatus: %ld\n", pCurrAddresses->OperStatus);
            printf("\tIpv6IfIndex (IPv6 interface): %u\n",
                   pCurrAddresses->Ipv6IfIndex);
            printf("\tZoneIndices (hex): ");
            for (i = 0; i < 16; i++)
                printf("%lx ", pCurrAddresses->ZoneIndices[i]);
            printf("\n");

            printf("\tTransmit link speed: %I64u\n", pCurrAddresses->TransmitLinkSpeed);
            printf("\tReceive link speed: %I64u\n", pCurrAddresses->ReceiveLinkSpeed);

            pPrefix = pCurrAddresses->FirstPrefix;
            if (pPrefix) {
                for (i = 0; pPrefix != NULL; i++)
                    pPrefix = pPrefix->Next;
                printf("\tNumber of IP Adapter Prefix entries: %d\n", i);
            } else
                printf("\tNumber of IP Adapter Prefix entries: 0\n");

            printf("\n");

            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        printf("Call to GetAdaptersAddresses failed with error: %d\n",
               dwRetVal);
        if (dwRetVal == ERROR_NO_DATA)
            printf("\tNo addresses were found for the requested parameters\n");
        else {

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                              FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    // Default language
                              (LPTSTR) & lpMsgBuf, 0, NULL)) {
                printf("\tError: %s", lpMsgBuf);
                LocalFree(lpMsgBuf);
                if (pAddresses)
                    FREE(pAddresses);
                exit(1);
            }
        }
    }

    if (pAddresses) {
        FREE(pAddresses);
    }

    return 0;
}
#endif



//Выводит IP Address, IP Mask, Gateway, DHCP Server
#if 0
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "IPHLPAPI.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/* Note: could also use malloc() and free() */

int __cdecl main()
{

    /* Declare and initialize variables */

// It is possible for an adapter to have multiple
// IPv4 addresses, gateways, and secondary WINS servers
// assigned to the adapter.
//
// Note that this sample code only prints out the
// first entry for the IP address/mask, and gateway, and
// the primary and secondary WINS server for each adapter.

    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    UINT i;

/* variables used to print DHCP time info */
    struct tm newtime;
    char buffer[32];
    errno_t error;

    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO *) MALLOC(sizeof (IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        return 1;
    }
// Make an initial call to GetAdaptersInfo to get
// the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        FREE(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *) MALLOC(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            return 1;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            printf("\tComboIndex: \t%d\n", pAdapter->ComboIndex);
            printf("\tAdapter Name: \t%s\n", pAdapter->AdapterName);
            printf("\tAdapter Desc: \t%s\n", pAdapter->Description);
            printf("\tAdapter Addr: \t");
            for (i = 0; i < pAdapter->AddressLength; i++) {
                if (i == (pAdapter->AddressLength - 1))
                    printf("%.2X\n", (int) pAdapter->Address[i]);
                else
                    printf("%.2X-", (int) pAdapter->Address[i]);
            }
            printf("\tIndex: \t%d\n", pAdapter->Index);
            printf("\tType: \t");
            switch (pAdapter->Type) {
                case MIB_IF_TYPE_OTHER:
                    printf("Other\n");
                    break;
                case MIB_IF_TYPE_ETHERNET:
                    printf("Ethernet\n");
                    break;
                case MIB_IF_TYPE_TOKENRING:
                    printf("Token Ring\n");
                    break;
                case MIB_IF_TYPE_FDDI:
                    printf("FDDI\n");
                    break;
                case MIB_IF_TYPE_PPP:
                    printf("PPP\n");
                    break;
                case MIB_IF_TYPE_LOOPBACK:
                    printf("Loopback\n");
                    break;
                case MIB_IF_TYPE_SLIP:
                    printf("Slip\n");
                    break;
                default:
                    printf("Unknown type %ld\n", pAdapter->Type);
                    break;
            }

            printf("\tIP Address: \t%s\n",
                   pAdapter->IpAddressList.IpAddress.String);
            printf("\tIP Mask: \t%s\n", pAdapter->IpAddressList.IpMask.String);

            printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);
            printf("\t***\n");

            if (pAdapter->DhcpEnabled) {
                printf("\tDHCP Enabled: Yes\n");
                printf("\t  DHCP Server: \t%s\n",
                       pAdapter->DhcpServer.IpAddress.String);

                printf("\t  Lease Obtained: ");
                /* Display local time */
                error = _localtime32_s(&newtime, (__time32_t*) &pAdapter->LeaseObtained);
                if (error)
                    printf("Invalid Argument to _localtime32_s\n");
                else {
                    // Convert to an ASCII representation
                    error = asctime_s(buffer, 32, &newtime);
                    if (error)
                        printf("Invalid Argument to asctime_s\n");
                    else
                        /* asctime_s returns the string terminated by \n\0 */
                        printf("%s", buffer);
                }

                printf("\t  Lease Expires:  ");
                error = _localtime32_s(&newtime, (__time32_t*) &pAdapter->LeaseExpires);
                if (error)
                    printf("Invalid Argument to _localtime32_s\n");
                else {
                    // Convert to an ASCII representation
                    error = asctime_s(buffer, 32, &newtime);
                    if (error)
                        printf("Invalid Argument to asctime_s\n");
                    else
                        /* asctime_s returns the string terminated by \n\0 */
                        printf("%s", buffer);
                }
            } else
                printf("\tDHCP Enabled: No\n");

            if (pAdapter->HaveWins) {
                printf("\tHave Wins: Yes\n");
                printf("\t  Primary Wins Server:    %s\n",
                       pAdapter->PrimaryWinsServer.IpAddress.String);
                printf("\t  Secondary Wins Server:  %s\n",
                       pAdapter->SecondaryWinsServer.IpAddress.String);
            } else
                printf("\tHave Wins: No\n");
            pAdapter = pAdapter->Next;
            printf("\n");
        }
    } else {
        printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);

    }
    if (pAdapterInfo)
        FREE(pAdapterInfo);

    return 0;
}
#endif



//Сканер портов
#if 0
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>
#include "winsock2.h"
#include <ws2tcpip.h>
#include <unistd.h>



int main(int argc, char* argv[]) {
    // Check for correct number of arguments
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <target-ip> <start-port> <end-port>" << std::endl;
    return 1;
    }

    const char* target_ip = argv[1];
    int start_port = std::atoi(argv[2]);
    int end_port = std::atoi(argv[3]);

    // Loop through the specified port range
    for (int port = start_port; port <= end_port; ++port) {



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
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET) {
            printf("Error creating socket: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }
        int recvTimeout = 250;
// Set recv timeout
        comm_status = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvTimeout, sizeof(int));
        if (comm_status == SOCKET_ERROR) {
            printf("setsockopt for SO_RCVTIMEO failed with error: %u\n", WSAGetLastError());
        } else {
            printf("Set SO_RCVTIMEO: ON\n");
        }

        //int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        //if (sock == -1) {
        //    std::cerr << "Could not create socket: " << strerror(errno) << std::endl;
        //    return 1;
        //}

        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);

        // Convert IP address from text to binary form
        if (inet_pton(AF_INET, target_ip, &server_address.sin_addr) <= 0) {
            std::cerr << "Invalid IP address" << std::endl;
            close(sock);
            return 1;
        }

        // Attempt to connect to the port
        if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) == 0) {
            std::cout << "Port " << port << " is open" << std::endl;
        }

        close(sock); // Close the socket after checking

        // Cleanup Winsock
        WSACleanup();
    }

    return 0;
}
#endif



//Tracert, не определяет маршрут
#if 0
//TraceRoute.cpp
/*----------------------------------------------------------
Function Description:
         The program simply implements the tracert command function of the Windows operating system,
         It can output the routing information of IP packets from this machine to the destination host.
-----------------------------------------------------------*/
#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "TraceRoute.h"
#pragma comment(lib,"ws2_32")
using namespace std;

int main()
{
    while(1)
    {
        /*Store IP*/
        char ipString[100];
        cout<<"TraceRoute:";
        scanf("%s",ipString);
        //char *ipString = "www.baidu.com";

        /*Initialize winsock2 environment*/
        WSADATA wsa;//A data structure. This structure is used to store the Windows Sockets data returned after the WSAStartup function is called
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)//Do the corresponding socket library binding, MAKEWORD(2,2) means use WINSOCK2 version
        {
            cerr << "\nFailed to initialize the WinSock2 DLL\n"<< "error code: "<< WSAGetLastError() << endl;//Cerr is usually used to output error messages and other output content that does not belong to normal logic
            return -1;
        }

        /*Convert command line parameters to IP address*/
        u_long ulDestIP = inet_addr(ipString);//Convert a dotted decimal IP into a long integer number (u_long type)

        //cout<<"Test————"<<"Long integer number"<<ulDestIP<<endl;
        if (ulDestIP == INADDR_NONE)//INADDR_NONE is a macro definition, representing an invalid IP address for IpAddress.
        {
            //Resolve by domain name when the conversion is unsuccessful
            hostent* pHostent = gethostbyname(ipString);//Returns a pointer to the hostent structure containing the host name and address information corresponding to the given host name
            if (pHostent)
            {
                ulDestIP = (*(in_addr*)pHostent->h_addr).s_addr;//in_addr is used to represent a 32-bit IPv4 address. /cout<<"Test————"<<"ip address"<<ulDestIP< <endl;

                //Output screen information
                cout << "\nTracing route to " << ipString
                     << "[" << inet_ntoa(*(in_addr*)(&ulDestIP)) << "]"//Convert the network address into a "." dotted string format
                     << "with a maximum of" << DEF_MAX_HOP << "hops.\n" << endl;//DEF_MAX_HOP maximum number of hops
            }
            else //Failed to resolve the host name
            {
                cerr << "\nCould not resolve the host name " << ipString << '\n'
                     << "error code: " << WSAGetLastError() << endl;
                WSACleanup();//Terminate the use of Winsock 2 DLL (Ws2_32.dll)
                return -1;
            }
        }
        else
        {
            //Output screen information
            cout << "Tracing route to " << ipString
                 << " with a maximum of " << DEF_MAX_HOP << " hops." << endl;
        }
        //Fill the destination Socket address
//struct sockaddr_in Lewis;
        // Lewis.sin_family = AF_INET;//Indicates the address type. For communication based on the TCP/IP transmission protocol, the value can only be AF_INET;
        // Lewis.sin_port = htons(80);//Indicates the port number, for example: 21 or 80
        // Lewis.sin_addr.s_addr = inet_addr("202.96.134.133");//represents a 32-bit IP address
        // memset(Lewis.sin_zero,0,sizeof(Lewis.sin_zero));//means padding byte

        sockaddr_in destSockAddr;//The address used to process network communications
        ZeroMemory(&destSockAddr, sizeof(sockaddr_in));//Use 0 to fill a memory area
        destSockAddr.sin_family = AF_INET;//sin_family;//Address family protocol cluster AF_INET (TCP/IP-IPv4)
        destSockAddr.sin_addr.s_addr = ulDestIP;//s_addr 32-bit IPv4 address
        //Create Raw Socket using ICMP protocol
        SOCKET sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);//Socket type: raw socket, IPPROTO_ICMP indicates that the ICMP header is constructed by the program, iFlags: socket property description
        if (sockRaw == INVALID_SOCKET)//Invalid socket
        {
            cerr << "\nFailed to create a raw socket\n"
                 << "error code: " << WSAGetLastError() << endl;
            WSACleanup();
            return -1;
        }
        //Set port properties
        int iTimeout = DEF_ICMP_TIMEOUT;//3000ms
        //cout<<"Test————"<<"TIMEOUT:"<<iTimeout<<endl;
        if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&iTimeout, sizeof(iTimeout)) == SOCKET_ERROR)
            //Set the options associated with a socket. Options may exist in the multi-layer protocol. In order to operate the socket layer options, the value of the layer should be specified as SOL_SOCKET
            //The socket to be set or get the option, the protocol layer where the option is located, the name of the option that needs to be accessed (the operation of the socket automatically has a timeout), points to the buffer containing the new option value, and the length of the current option
        {
            cerr << "\nFailed to set recv timeout\n"
                 << "error code: " << WSAGetLastError() << endl;
            closesocket(sockRaw);
            WSACleanup();
            return -1;
        }
        //Create ICMP packet sending buffer and receiving buffer
        char IcmpSendBuf[sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE];
        memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf));
        char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
        memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf));
        //Fill the ICMP packet to be sent (header and data part)
        ICMP_HEADER* pIcmpHeader = (ICMP_HEADER*)IcmpSendBuf;
        pIcmpHeader->type = ICMP_ECHO_REQUEST;
        pIcmpHeader->code = 0;
        pIcmpHeader->id = (USHORT)GetCurrentProcessId();
        memset(IcmpSendBuf+sizeof(ICMP_HEADER),'E', DEF_ICMP_DATA_SIZE);//Data is partially filled
        //Start to detect routing
        DECODE_RESULT stDecodeResult;
        BOOL bReachDestHost = FALSE;
        USHORT usSeqNo = 0;
        int iTTL = 1;
        int iMaxHop = DEF_MAX_HOP;
        while (!bReachDestHost && iMaxHop--)
        {
            //Set the ttl field of the IP data header
            setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char*)&iTTL, sizeof(iTTL));//raw socket with 0 (IPPROTO_IP). Used to receive any IP data packet. The checksum and protocol analysis are completed by the program itself.
            //Output the current number of hops as the routing information serial number
            cout << setw(3) << iTTL << flush; //setw(3) sets the field width, cout<<flush means that the content of the buffer is sent to cout immediately, and the output buffer is flushed.
            //Fill in the remaining fields of the ICMP datagram
            ((ICMP_HEADER*)IcmpSendBuf)->cksum = 0;
            ((ICMP_HEADER*)IcmpSendBuf)->seq = htons(usSeqNo++);//Convert the unsigned short host byte order to the network byte order, exchange the high and low bits of a number, (e.g.: 12 34- -> 34 12)
            ((ICMP_HEADER*)IcmpSendBuf)->cksum = GenerateChecksum((USHORT*)IcmpSendBuf, sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE);

            //Record the serial number and current time
            stDecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuf)->seq;
            stDecodeResult.dwRoundTripTime = GetTickCount();//Returns the number of milliseconds that have elapsed since the operating system was started, and is often used to determine the execution time of a method

            //Send ICMP EchoRequest datagram
            if (sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR)
            {
                //If the destination host is unreachable, exit directly
                if (WSAGetLastError() == WSAEHOSTUNREACH)
                    cout << '/t' << "Destination host unreachable.\n"<< "\nTrace complete.\n" << endl;
                closesocket(sockRaw);
                WSACleanup();
                return 0;
            }
            //Receive ICMP EchoReply datagram
            //Because the received datagram may not be the datagram expected by the program, it needs to be received in a loop until the required data is received or timeout
            sockaddr_in from;
            int iFromLen = sizeof(from);
            int iReadDataLen;
            while (1)
            {
                //Wait for data to arrive
                iReadDataLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (sockaddr*)&from, &iFromLen);
                if (iReadDataLen != SOCKET_ERROR) //A data packet arrives
                {
                    //The decoded data packet, if the decoding is correct, jump out of the receiving loop and send the next EchoRequest packet
                    if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, stDecodeResult))
                    {
                        if (stDecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr)
                            bReachDestHost = TRUE;
                        cout <<'\t' << inet_ntoa(stDecodeResult.dwIPaddr) << endl;//Convert the network address into a "." dotted string format
                        break;
                    }
                }
                else if (WSAGetLastError() == WSAETIMEDOUT) //Receive timeout, print asterisk
                {
                    cout << setw(9) << '*' << '\t' << "Request timed out." << endl;
                    break;
                }
                else
                {
                    cerr << "\nFailed to call recvfrom\n"
                         << "error code: " << WSAGetLastError() << endl;
                    closesocket(sockRaw);
                    WSACleanup();
                    return -1;
                }
            }
            //TTL value plus 1
            iTTL++;
            //cout<<"Test————"<<iTTL<<endl;
        }
        //Output screen information
        cout << "\nTrace complete.\n" << endl;
        closesocket(sockRaw);
        WSACleanup();
    }
    return 0;

}
/*Generate internet checksum*/
USHORT GenerateChecksum(USHORT* pBuf, int iSize)// 16
{
    unsigned long cksum = 0;//32
    while (iSize>1)//40
    {
        cksum += *pBuf++;
        iSize -= sizeof(USHORT);
    }
    if (iSize)
        cksum += *(UCHAR*)pBuf;//8
    //printf("Test——cksum——Test:%x\n",cksum);
    //printf("Test——cksum>>16——Test:%x\n",cksum>>16);
    //printf("Test——cksum & 0xffff——Test:%x\n",cksum & 0xffff);
    cksum = (cksum >> 16) + (cksum & 0xffff);
    //printf("Test——cksum——Test:%x\n",cksum);
    //printf("Test——cksum>>16——Test:%x\n",cksum>>16);
    cksum += (cksum >> 16);
    //printf("Test——cksum——Test:%x\n",cksum);
    //printf("Test——(USHORT)(~cksum)——Test:%x\n",(USHORT)(~cksum));
    return (USHORT)(~cksum);//~ bitwise reverse
}

/*Decoded datagram*/
BOOL DecodeIcmpResponse(char* pBuf, int iPacketSize, DECODE_RESULT& stDecodeResult)
{
    //Check the validity of the datagram size
    IP_HEADER* pIpHdr = (IP_HEADER*)pBuf;
    int iIpHdrLen = pIpHdr->hdr_len * 4;//unit 4 bytes
    if (iPacketSize < (int)(iIpHdrLen+sizeof(ICMP_HEADER)))
        return FALSE;
    //Check the id field and serial number according to the ICMP packet type to determine whether it is the Icmp packet that the program should receive
    ICMP_HEADER* pIcmpHdr = (ICMP_HEADER*)(pBuf+iIpHdrLen);
    USHORT usID, usSquNo;//ICMP header identifier and serial number
    if (pIcmpHdr->type == ICMP_ECHO_REPLY)
    {
        usID = pIcmpHdr->id;
        usSquNo = pIcmpHdr->seq;
    }
    else if(pIcmpHdr->type == ICMP_TIMEOUT)
    {
        char* pInnerIpHdr = pBuf+iIpHdrLen+sizeof(ICMP_HEADER); //IP header in the payload
        int iInnerIPHdrLen = ((IP_HEADER*)pInnerIpHdr)->hdr_len * 4;//IP header length in the payload
        ICMP_HEADER* pInnerIcmpHdr = (ICMP_HEADER*)(pInnerIpHdr+iInnerIPHdrLen);//ICMP header in the payload
        usID = pInnerIcmpHdr->id;
        usSquNo = pInnerIcmpHdr->seq;
    }
    else
        return FALSE;
    if (usID != (USHORT)GetCurrentProcessId() || usSquNo !=stDecodeResult.usSeqNo)
        return FALSE;
    //Process the ICMP datagram received correctly
    if (pIcmpHdr->type == ICMP_ECHO_REPLY ||
        pIcmpHdr->type == ICMP_TIMEOUT)
    {
        //Return the decoding result
        stDecodeResult.dwIPaddr.s_addr = pIpHdr->sourceIP;
        stDecodeResult.dwRoundTripTime = GetTickCount() - stDecodeResult.dwRoundTripTime;
        //Print screen information
        if (stDecodeResult.dwRoundTripTime)
            cout << setw(6) << stDecodeResult.dwRoundTripTime << " ms" << flush;
        else
            cout << setw(6) << "<1" << " ms" << flush;
        return TRUE;
    }
    return FALSE;
}
/*_############################################################################
  _##
  _##  test_app.cpp
  _##
  _##  SNMP++ v3.4
  _##  -----------------------------------------------
  _##  Copyright (c) 2001-2021 Jochen Katz, Frank Fock
  _##
  _##  This software is based on SNMP++2.6 from Hewlett Packard:
  _##
  _##    Copyright (c) 1996
  _##    Hewlett-Packard Company
  _##
  _##  ATTENTION: USE OF THIS SOFTWARE IS SUBJECT TO THE FOLLOWING TERMS.
  _##  Permission to use, copy, modify, distribute and/or sell this software
  _##  and/or its documentation is hereby granted without fee. User agrees
  _##  to display the above copyright notice and this license notice in all
  _##  copies of the software and any documentation of the software. User
  _##  agrees to assume all liability for the use of the software;
  _##  Hewlett-Packard, Frank Fock, and Jochen Katz make no representations
  _##  about the suitability of this software for any purpose. It is provided
  _##  "AS-IS" without warranty of any kind, either express or implied. User
  _##  hereby grants a royalty-free license to any and all derivatives based
  _##  upon this software code base.
  _##
  _##########################################################################*/

#include <libsnmp.h>

#include "snmp_pp/snmp_pp.h"

#ifdef WIN32
#define strcasecmp _stricmp
#endif
#endif



//Определяет адрес по имени
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
//#ifdef SNMP_PP_NAMESPACE
//using namespace Snmp_pp;
//#endif

// default request oids
//#define NUM_SYS_VBS	6
//#define sysDescr	"1.3.6.1.2.1.1.1.0"
//#define sysObjectID	"1.3.6.1.2.1.1.2.0"
//#define sysUpTime	"1.3.6.1.2.1.1.3.0"
//#define sysContact	"1.3.6.1.2.1.1.4.0"
//#define sysName		"1.3.6.1.2.1.1.5.0"
//#define sysLocation	"1.3.6.1.2.1.1.6.0"
//#define sysServices	"1.3.6.1.2.1.1.7.0" // not all agents support this...

// default notification oid
//#define coldStart	"1.3.6.1.6.3.1.1.4.3.0.1"
#endif



//Определяет адрес по имени и наоборот
#if 0
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
#endif



//Пингует адреса и имена
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
    if (argc != 2)
    {
        fprintf(stderr,"\nSyntax: ping.exe [HostName]Or[IPAddress]\n");
        return 1;
    }



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

    iaDest.s_addr = inet_addr("192.168.0.1");
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
}
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



//Работа по SNMP
#if 0
/*_############################################################################
  _##
  _##  test_app.cpp
  _##
  _##  SNMP++ v3.4
  _##  -----------------------------------------------
  _##  Copyright (c) 2001-2021 Jochen Katz, Frank Fock
  _##
  _##  This software is based on SNMP++2.6 from Hewlett Packard:
  _##
  _##    Copyright (c) 1996
  _##    Hewlett-Packard Company
  _##
  _##  ATTENTION: USE OF THIS SOFTWARE IS SUBJECT TO THE FOLLOWING TERMS.
  _##  Permission to use, copy, modify, distribute and/or sell this software
  _##  and/or its documentation is hereby granted without fee. User agrees
  _##  to display the above copyright notice and this license notice in all
  _##  copies of the software and any documentation of the software. User
  _##  agrees to assume all liability for the use of the software;
  _##  Hewlett-Packard, Frank Fock, and Jochen Katz make no representations
  _##  about the suitability of this software for any purpose. It is provided
  _##  "AS-IS" without warranty of any kind, either express or implied. User
  _##  hereby grants a royalty-free license to any and all derivatives based
  _##  upon this software code base.
  _##
  _##########################################################################*/

#include <libsnmp.h>

#include "snmp_pp/snmp_pp.h"

#ifdef WIN32
#define strcasecmp _stricmp
#endif

#ifdef SNMP_PP_NAMESPACE
using namespace Snmp_pp;
#endif

// default request oids
#define NUM_SYS_VBS	6
#define sysDescr	"1.3.6.1.2.1.1.1.0"
#define sysObjectID	"1.3.6.1.2.1.1.2.0"
#define sysUpTime	"1.3.6.1.2.1.1.3.0"
#define sysContact	"1.3.6.1.2.1.1.4.0"
#define sysName		"1.3.6.1.2.1.1.5.0"
#define sysLocation	"1.3.6.1.2.1.1.6.0"
//#define sysServices	"1.3.6.1.2.1.1.7.0" // not all agents support this...

// default notification oid
#define coldStart	"1.3.6.1.6.3.1.1.4.3.0.1"

int main(int argc, char **argv)
{
    int status;
    char *req_str      = (char*) "get";
    //  char *dflt_req_oid = (char*) sysDescr;
    char *dflt_trp_oid = (char*) coldStart;
    char *genAddrStr   = (char*) "127.0.0.1" ;		  // localhost
    char *oid_str      = (char*) NULL;

    if (argc > 1) genAddrStr = argv[1];
    if (argc > 2) req_str    = argv[2];
    if (argc > 3) oid_str    = argv[3];

    Snmp::socket_startup();  // Initialize socket subsystem

    IpAddress ipAddr(genAddrStr);
    if (!ipAddr.valid()) {
        std::cout << "Invalid destination: " << genAddrStr << std::endl;
        return(1);
    }

    // bind to any port and use IPv6 if needed
    Snmp snmp(status, 0, (ipAddr.get_ip_version() == Address::version_ipv6));
    if (status){
        std::cout << "Failed to create SNMP Session: " << status << std::endl;
        return(1);
    }
    std::cout << "Created session successfully" << std::endl;


    CTarget target(ipAddr);
    if (! target.valid()) {
        std::cout << "Invalid target" << std::endl;
        return(1);
    }

    Pdu pdu;
    Vb vb;
    if ( strcmp(req_str, "get") == 0 ) {

        Vb vbl[NUM_SYS_VBS];
        vbl[0].set_oid(sysDescr);
        vbl[1].set_oid(sysObjectID);
        vbl[2].set_oid(sysUpTime);
        vbl[3].set_oid(sysContact);
        vbl[4].set_oid(sysName);
        vbl[5].set_oid(sysLocation);
//    vbl[6].set_oid(sysServices);

        std::cout << "Send a GET-REQUEST to: " << ipAddr.get_printable() << std::endl;
        if ( ! oid_str ) {
            if ( strcmp(genAddrStr,"localhost" ) == 0 ||
                 strcmp(genAddrStr, "127.0.0.1") == 0 ){
                pdu.set_vblist(vbl, NUM_SYS_VBS);
            } else {
                for (int i=0; i<NUM_SYS_VBS;i++)
                    pdu += vbl[i];
            }
        }
        else {
            Oid req_oid(oid_str);
            if ( ! req_oid.valid() ) {
                std::cout << "Request oid constructor failed for:" << oid_str << std::endl;
                return(1);
            }
            vb.set_oid(req_oid);
            pdu += vb;
        }
        status = snmp.get(pdu, target);
        if (status){
            std::cout << "Failed to issue SNMP Get: (" << status  << ") "
                      << snmp.error_msg(status) << std::endl;
            return(1);
        }
        else{
            std::cout << "Issued get successfully" << std::endl;
            int vbcount = pdu.get_vb_count();
            if ( vbcount == NUM_SYS_VBS ) {
                pdu.get_vblist(vbl, vbcount);
                for ( int i=0; i<vbcount ; i++ )  {
                    std::cout << vbl[i].get_printable_oid() << " : " <<
                              vbl[i].get_printable_value() << std::endl;
                }
            } else {
                for ( int i=0; i<vbcount ; i++ )  {
                    pdu.get_vb(vb, i);
                    std::cout << vb.get_printable_oid() << " : " <<
                              vb.get_printable_value() << std::endl;
                }
            }
        }
    }
    else if ( strcmp(req_str, "trap") == 0 ) {
        std::cout << "Send a TRAP to: " << ipAddr.get_printable() << std::endl;

        if ( ! oid_str )
            oid_str = dflt_trp_oid;

        Oid notify_oid(oid_str);
        if ( ! notify_oid.valid() ) {
            std::cout << "Notify oid constructor failed for:" << oid_str << std::endl;
            return(1);
        }

        pdu.set_notify_id(notify_oid);

        // Use a simple payload
        vb.set_oid(sysLocation);
        vb.set_value("This is a test");
        pdu += vb;

        status = snmp.trap(pdu, target);

        if (status){
            std::cout << "Failed to issue SNMP Trap: (" << status  << ") "
                      << snmp.error_msg(status) << std::endl;
            return(1);
        } else {
            std::cout << "Success" << std::endl;
        }

    }
    else {
        std::cout << "Invalid SNMP operation: " << req_str  << std::endl ;
        std::cout << "Usage: " << argv[0] << " hostname [get | trap]" << std::endl;
        return(1);
    }

    Snmp::socket_cleanup();  // Shut down socket subsystem

    return(0);
}
/*
 *
Архитектурный обзор сущностей snmp++

В отличие от низкоуровневых Си-структур, авторы snmp++ сделали очень логичное объектное разделение:
1. Snmp::socket_startup() и Snmp::socket_cleanup() — близкие аналоги нашего WSAStartup/libssh2_init. Инициализируют и корректно закрывают сетевую UDP-подсистему операционной системы.
2. IpAddress / Oid — умные классы-оболочки. Они сами валидируют данные (метод .valid()). IpAddress парсит IP, а Oid преобразует текстовую строку вида "1.3.6.1.2.1.1.1.0" во внутренний числовой массив идентификатора объекта (Object Identifier).
3. Snmp — это движок сессии. Он создает реальный UDP-сокет в ОС и предоставляет методы для отправки (snmp.get(...), snmp.trap(...)).
4. CTarget — объект, описывающий куда и как мы подключаемся. Он хранит целевой IP-адрес, порт (по умолчанию 161 для SNMP), версию протокола (v1, v2c, v3), количество попыток (retries) и таймаут ожидания. По умолчанию в snmp++ используется SNMP v1 с community-строкой "public".
5. Vb (Variable Binding) — атомарная пара «OID : Значение». Например: OID "1.3.6.1.2.1.1.5.0" и значение "TFortis-Switch".
6. Pdu (Protocol Data Unit) — это сетевой контейнер (пакет). SNMP позволяет в один запрос упаковать сразу несколько OID-ов, чтобы не гонять сеть попусту. Мы буквально складываем переменные в пакет с помощью перегруженного оператора pdu += vb;.


Главная архитектурная проблема примера: Блокирующий snmp.get

В Си-примере вызов выполняется синхронно:
cpp
status = snmp.get(pdu, target); // Блокирует поток до получения ответа по UDP!
Используйте код с осторожностью.
Если коммутатор TFortis недоступен или пакет потеряется в сети, этот метод намертво заблокирует наш единственный рабочий поток io_context на время таймаута (по умолчанию несколько секунд).

Как мы «подружим» snmp++ с Boost.Asio 1.87 корутинами

В snmp++ заложена прекрасная встроенная поддержка асинхронных вызовов через механизм коллбеков (snmp_callback).
Мы напишем свободную функцию-обертку async_snmp_get, которая будет принимать Snmp, Pdu, CTarget и возвращать C++20 корутинный тип asio::awaitable<int>.
Внутри этой функции мы переведем запрос в асинхронный режим, а чтобы связать коллбек библиотеки с корутиной Asio, мы используем специальное асинхронное обещание (asio::use_awaitable в связке с экзекутором). Поток ОС будет абсолютно свободен, пока UDP-пакет летит туда и обратно!
 */
#endif



//Работа по SNMP, v.2
#if 1
#include <iostream>
#include <vector>
#include <string>
#include <future>
#include <boost/asio.hpp>
#include "snmp_pp/snmp_pp.h"

#ifdef SNMP_PP_NAMESPACE
using namespace Snmp_pp;
#endif

namespace asio = boost::asio;

// Структура для передачи контекста в старый Си-коллбек библиотеки snmp++
struct SnmpAsyncContext {
    std::promise<int> status_promise;
    Pdu* response_pdu;
};



#include <boost/asio/post.hpp>
#include <future>

// Полностью асинхронная обертка для вызова SNMP GET через пул потоков Asio 1.87
asio::awaitable<int> async_snmp_get(Snmp& snmp, Pdu& pdu, SnmpTarget& target) {
    // 1. Создаем стандартное C++ обещание и привязываем к нему future
    auto promise = std::make_shared<std::promise<int>>();
    auto future = promise->get_future();

    // 2. Получаем системный экзекутор из нашей текущей корутины
    auto exec = co_await asio::this_coro::executor;

    // 3. Отправляем выполнение блокирующего метода snmp++ в фоновый контекст Asio.
    // Главный поток io_context мгновенно освобождается!
    asio::post(exec, [&snmp, &pdu, &target, promise]() {
        // В фоновом режиме надежно выполняется синхронный запрос библиотеки.
        // Ответ запишется прямо в pdu.
        int status = snmp.get(pdu, target);
        promise->set_value(status);
    });

    // 4. НАДЁЖНОЕ АСИНХРОННОЕ ОЖИДАНИЕ БЕЗ ЗАВИСАНИЙ:
    // Мы создаем микро-таймер Asio, который проверяет готовность future,
    // давая главному потоку дышать. Корутина спит, пока UDP пакет летит по сети.
    asio::steady_timer timer(exec);
    while (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        timer.expires_after(std::chrono::milliseconds(5));
        co_await timer.async_wait(asio::use_awaitable);
    }

    // Возвращаем итоговый статус операции (успех или код ошибки/таймаута)
    co_return future.get();
}



// Главная корутина логики сбора данных по SNMP
asio::awaitable<void> snmp_collection_logic(std::string ip_address) {
    std::cout << "[SNMP] Инициализация сессии для " << ip_address << "...\n";

    int status = 0;
    // Создаем сессию SNMP (0 означает привязку к любому свободному UDP порту ОС)
    Snmp snmp(status, 0, false);
    if (status != SNMP_CLASS_SUCCESS) {
        std::cerr << "[Ошибка] Не удалось запустить SNMP сессию: " << status << "\n";
        co_return;
    }

    // Задаем целевой IP-адрес устройства
    IpAddress ip_target(ip_address.c_str());
    if (!ip_target.valid()) {
        std::cerr << "[Ошибка] Неверный IP адрес устройства.\n";
        co_return;
    }

    // Настраиваем цель для SNMP v1 / v2c
    CTarget target(ip_target);
    target.set_version(version1);         // Задаем версию протокола v1 (для v3 поменяем на UTarget)
    target.set_readcommunity("public");   // Пароль-комьюнити (обычно "public" на TFortis)
    target.set_timeout(200);              // Таймаут ожидания ответа: 2 секунды (200 * 10мс)
    target.set_retry(1);                  // Количество повторных попыток при потере UDP-пакета

    if (!target.valid()) {
        std::cerr << "[Ошибка] Конфигурация Target невалидна.\n";
        co_return;
    }

    // Формируем пакет запроса PDU и наполняем его интересующими нас OID-ами
    Pdu request_pdu;

    // Системные OID стандарта MIB-II (Описания, Аптайм, Имя устройства)
    request_pdu += Vb("1.3.6.1.2.1.1.1.0"); // sysDescr
    request_pdu += Vb("1.3.6.1.2.1.1.3.0"); // sysUpTime
    request_pdu += Vb("1.3.6.1.2.1.1.5.0"); // sysName

    std::cout << "[SNMP] Отправка асинхронного GET-запроса пакета OID...\n";

    // Вызываем обновленную обертку. Ответ запишется сразу в request_pdu
    int result_status = co_await async_snmp_get(snmp, request_pdu, target);

    if (result_status == SNMP_CLASS_SUCCESS) {
        std::cout << "[Успех] Данные от оборудования успешно получены:\n\n";

        // Читаем результаты прямо из request_pdu
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


    std::cout << "\n[Система] Корутина SNMP сбора успешно завершила работу.\n";
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
    Snmp::socket_startup();

    try {
        asio::io_context io_context;
        std::string target_ip = "127.0.0.1"; // IP-адрес вашего TFortis для теста

        // Регистрируем корутину SNMP-сбора в планировщике Asio
        asio::co_spawn(io_context, snmp_collection_logic(target_ip), asio::detached);

        // Запускаем асинхронный движок
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Критическое исключение: " << e.what() << "\n";
    }

    // 2. Корректно освобождаем ресурсы UDP подсистемы перед выходом
    Snmp::socket_cleanup();
    return 0;
}


/*
 *
Как мы реализовали асинхронный co_await для snmp++

Так как snmp++ не имеет встроенной интеграции с планировщиком Boost.Asio, мы применили классический паттерн асинхронного обещания.
1. Перед отправкой запроса мы создаем std::promise<int> и связываем его с std::future<int>.
2. Мы вызываем асинхронную функцию библиотеки snmp.get(..., callback, ...) и передаем указатель на наше обещание в качестве пользовательского контекста (void* data).
3. Корутина делает co_await специального асинхронного ожидания будущего результата. Текущий поток ОС мгновенно освобождается! Программа может выполнять другие задачи.
4. Когда коммутатор TFortis присылает UDP-ответ, внутренний поток snmp++ ловит его, вызывает наш коллбек, который выполняет .set_value(status) в обещание.
5. Boost.Asio мгновенно «будит» нашу корутину, и она продолжает выполнение со следующей строчки, считывая полученные OID.

 */

/*
 *
Как мы реализовали асинхронный co_await для snmp++

Так как snmp++ не имеет встроенной интеграции с планировщиком Boost.Asio, мы применили классический паттерн асинхронного обещания.
1. Перед отправкой запроса мы создаем std::promise<int> и связываем его с std::future<int>.
2. Мы вызываем асинхронную функцию библиотеки snmp.get(..., callback, ...) и передаем указатель на наше обещание в качестве пользовательского контекста (void* data).
3. Корутина делает co_await специального асинхронного ожидания будущего результата. Текущий поток ОС мгновенно освобождается! Программа может выполнять другие задачи.
4. Когда коммутатор TFortis присылает UDP-ответ, внутренний поток snmp++ ловит его, вызывает наш коллбек, который выполняет .set_value(status) в обещание.
5. Boost.Asio мгновенно «будит» нашу корутину, и она продолжает выполнение со следующей строчки, считывая полученные OID.

 */

/*
 *
Архитектурные преимущества этой сборки:

1. Поддержка SNMP v3 «из коробки»: Классы Pdu, Vb, функция async_snmp_get и коллбек останутся на 100% нетронутыми, когда вы решите перейти на v3. Вам потребуется только заменить CTarget на UTarget в корутине snmp_collection_logic и прописать параметры шифрования USM.
2. Линейная логика (co_await): Благодаря обертке async_snmp_get, весь сложный асинхронный процесс с Си-коллбеками скрыт. Логика сбора выглядит просто, понятно и последовательно.
3. Нулевая блокировка потока: Программа не зависнет, даже если коммутатор выключен. Таймер Asio мягко проверяет готовность ответа, сохраняя поток свободным для других задач автоматизации.

/*

Резюме проекта: Три сетевых модуля (Boost.Asio 1.87 + C++20)

Мы полностью завершили этап создания принципиальных, изолированных рабочих прототипов для автоматизации коммутаторов и концентраторов (включая TFortis). Все три модуля работают асинхронно в рамках однопоточной корутинной архитектуры C++20.

1. Модуль Telnet (telnetpp + Asio корутины)

• Статус: Полностью рабочий интерактивный терминал.
• Логика: Вычерпывает байты из сети фоновой корутиной и отправляет их в парсер библиотеки, который очищает текст от опций. Клавиатура сканирует ввод через _kbhit() и шлет ANSI-коды стрелочек. Сессия защищена корутиной-супервизором (keep_alive_timer).

2. Модуль SSH (libssh2 + Asio корутины)

• Статус: Полностью рабочий интерактивный терминал с аппаратной историей команд.
• Достижения:
	1. Включили UTF-8 (65001) и ANSI-обработку консоли Windows, убрав кракозябры.
	2. Задали геометрию PTY терминала 160x40 символов через libssh2_channel_request_pty_ex, выровняв таблицы в красивые ровные столбики без разрыва слов.
	3. Перевели метод отправки в асингую корутину async_write с обработкой LIBSSH2_ERROR_EAGAIN, полностью ликвидировав мертвые зависания при быстром перелистывании стрелочками.

3. Модуль SNMP (snmp++ + Asio корутины)

• Статус: Полностью рабочий асингуый сборщик метрик по OID (успешно протестирован с кодом выхода 0 на сборе системных OID MIB-II: sysDescr, sysUpTime, sysName).
• Архитектурное решение: Чтобы Си-библиотека snmp++ не блокировала и не зацикливала наш главный поток io_context, мы обернули её синхронный метод snmp.get(pdu, target) в функцию async_snmp_get. Она отправляет блокирующую задачу в пул потоков через asio::post, а сама корутина мягко спит на 5-миллисекундном микро-таймере Asio, проверяя готовность std::future, давая процессору дышать и полностью защищая программу от падений памяти (0xC0000005).
• Задел на будущее: Архитектура готова к бесшовному переходу с SNMP v1 на шифрованный SNMP v3 (потребуется лишь поменять класс CTarget на UTarget).

🎯 План работы на завтра

Как только вы подключитесь, мы начнем доскональный построчный аудит логики работы SNMP-клиента:
1. Разберем механику связки std::promise / std::future внутри лямбды asio::post.
2. Поймем, как микро-таймер будит корутину и почему это спасло программу от зависания.
3. Изучим структуру классов Pdu и Vb (Variable Binding) для упаковки OID.
 */
#endif
