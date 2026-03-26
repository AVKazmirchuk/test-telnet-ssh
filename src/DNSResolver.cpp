
#include "../include/DNSResolver.h"
#include "../include/Logger.h"
#include <iostream>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib") // Link to the Winsock library

//This constructor initializes Winsock by calling WSAStartup to load the necessary 
DNSResolver::DNSResolver(bool enableLogging) : loggingEnabled(enableLogging) 
{
    WSADATA wsaData;  // Structure for storing Winsock initialization data.
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        logError("WSAStartup failed"); 
    }
}

//This destructor calls WSACleanup() to clean up Winsock resources
DNSResolver::~DNSResolver() 
{
    WSACleanup(); 
}

//This function logs error messages
void DNSResolver::logError(const string& message) 
{
    if (loggingEnabled) 
    {
        Logger::logError(message);  
    }
}

//This function logs the resolved IP addresses for a given domain
void DNSResolver::logResults(const string& domain, const vector<string>& results) 
{
    if (loggingEnabled) 
    {
        Logger::log("Results for " + domain + ":");
        for (const auto& ip : results) 
        {
            Logger::log(ip); // Log each resolved IP address.
        }
    }
}

//This function converts the address stored in a sockaddr structure to a string. 
//It handles  both IPv4 and IPv6 address families
string DNSResolver::ipToString(const ::sockaddr* addr) 
{
    char ipStr[INET6_ADDRSTRLEN] = {0}; // Buffer to store the IP address.

    if (addr->sa_family == AF_INET) //IPv4.
    {
        const sockaddr_in* ipv4 = reinterpret_cast<const sockaddr_in*>(addr);
        if (inet_ntoa(ipv4->sin_addr)) //string conversion
        {
            return string(inet_ntoa(ipv4->sin_addr));
        }
    } 
    else if (addr->sa_family == AF_INET6) // IPv6.
    {
        const sockaddr_in6* ipv6 = reinterpret_cast<const sockaddr_in6*>(addr);
        DWORD ipSize = INET6_ADDRSTRLEN;
        WSAAddressToStringA((sockaddr*)ipv6, sizeof(sockaddr_in6), nullptr, ipStr, &ipSize); // Convert the IPv6 address to a string
        return string(ipStr);  
    }

    return "Unknown Address";
}

//  This function uses `getaddrinfo` to resolve a domain name into a list of address information.
//  It supports both IPv4 and IPv6 addresses.
vector<string> DNSResolver::resolveDomain(const string& domain) 
{
    vector<string> results;  // Vector to store the resolved IP addresses.
    addrinfo hints = {};  // Hints structure to specify the type of addresses we want.
    addrinfo* resultList = nullptr; // Pointer to the result list returned by getaddrinfo.

    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;  // We are resolving for a stream (TCP) socket type.

    // getaddrinfo to resolve the domain into addresses.
    if (getaddrinfo(domain.c_str(), nullptr, &hints, &resultList) != 0)
    {
        logError("Failed to resolve domain: " + domain); 
        return results;
    }

    // Iterate over the list of resolved addresses.
    for (addrinfo* ptr = resultList; ptr != nullptr; ptr = ptr->ai_next) 
    {
        string ip = ipToString(ptr->ai_addr);
        if (!ip.empty()) 
        {
            results.push_back(ip);  // Add the IP address to the results vector.
        }
    }

    freeaddrinfo(resultList);  // Free the memory allocated.
    return results;
}
