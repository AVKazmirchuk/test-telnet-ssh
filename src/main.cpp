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

int __cdecl main()
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
#endif







































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
int main(int argc, char **argv)
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
    int status;
    char *req_str      = (char*) "get";
    //  char *dflt_req_oid = (char*) sysDescr;
    char *dflt_trp_oid = (char*) coldStart;
    char *genAddrStr   = (char*) "127.0.0.1" ;		  // localhost
    char *oid_str      = (char*) NULL;

        // Automatically frees addrinfo to prevent memory leaks and ensure exception safety
        std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)> res_ptr(res, freeaddrinfo);
    if (argc > 1) genAddrStr = argv[1];
    if (argc > 2) req_str    = argv[2];
    if (argc > 3) oid_str    = argv[3];

        // Buffer to store the resolved IP address
        char ipStr[NI_MAXHOST];
    Snmp::socket_startup();  // Initialize socket subsystem

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
    IpAddress ipAddr(genAddrStr);
    if (!ipAddr.valid()) {
        std::cout << "Invalid destination: " << genAddrStr << std::endl;
        return(1);
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
    // bind to any port and use IPv6 if needed
    Snmp snmp(status, 0, (ipAddr.get_ip_version() == Address::version_ipv6));
    if (status){
        std::cout << "Failed to create SNMP Session: " << status << std::endl;
        return(1);
    }
};

// This class handles user input, ensuring valid numerical input and choices
// It provides methods for selecting an option and choosing an address family
class UserInputHandler
{
public:
    std::cout << "Created session successfully" << std::endl;

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
    CTarget target(ipAddr);
    if (! target.valid()) {
        std::cout << "Invalid target" << std::endl;
        return(1);
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
    Pdu pdu;
    Vb vb;
    if ( strcmp(req_str, "get") == 0 ) {

                // Clear any extra input and return the corresponding address family
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (family == 1) return AF_INET;
                if (family == 2) return AF_INET6;
                return AF_UNSPEC;
        Vb vbl[NUM_SYS_VBS];
        vbl[0].set_oid(sysDescr);
        vbl[1].set_oid(sysObjectID);
        vbl[2].set_oid(sysUpTime);
        vbl[3].set_oid(sysContact);
        vbl[4].set_oid(sysName);
        vbl[5].set_oid(sysLocation);
//    vbl[6].set_oid(sysServices);

            }
                // Handle invalid input by resetting the input stream and displaying an error message
            catch (const std::exception& e)
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cerr << e.what() << "\n";
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
    else if ( strcmp(req_str, "trap") == 0 ) {
        std::cout << "Send a TRAP to: " << ipAddr.get_printable() << std::endl;

    return 0;
}
#endif




































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
        if ( ! oid_str )
            oid_str = dflt_trp_oid;

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
        Oid notify_oid(oid_str);
        if ( ! notify_oid.valid() ) {
            std::cout << "Notify oid constructor failed for:" << oid_str << std::endl;
            return(1);
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
        pdu.set_notify_id(notify_oid);

    /* Ultra basic "connect to port 22 on localhost".  Your code is
     * responsible for creating the socket establishing the connection
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == LIBSSH2_INVALID_SOCKET) {
        fprintf(stderr, "failed to create socket.\n");
        goto shutdown;
    }
        // Use a simple payload
        vb.set_oid(sysLocation);
        vb.set_value("This is a test");
        pdu += vb;

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
        status = snmp.trap(pdu, target);

    if(strlen(password) != 0) {
        /* We could authenticate via password */
        while((rc = libssh2_userauth_password(session, username, password)) ==
              LIBSSH2_ERROR_EAGAIN);
        if(rc) {
            fprintf(stderr, "Authentication by password failed.\n");
            goto shutdown;
        if (status){
            std::cout << "Failed to issue SNMP Trap: (" << status  << ") "
                      << snmp.error_msg(status) << std::endl;
            return(1);
        } else {
            std::cout << "Success" << std::endl;
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
        std::cout << "Invalid SNMP operation: " << req_str  << std::endl ;
        std::cout << "Usage: " << argv[0] << " hostname [get | trap]" << std::endl;
        return(1);
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
    Snmp::socket_cleanup();  // Shut down socket subsystem

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
    return(0);
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

#endif