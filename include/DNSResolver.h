#pragma once
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

//@brief A class for resolving domain names into their corresponding IPv4/IPv6 addresses and logging the results.
//It uses the Windows Sockets API (WinSock2) for DNS resolution.
class DNSResolver 
{
public:
    explicit DNSResolver(bool enableLogging = true);

    //@brief Destructor to clean up the Windows Sockets API (WinSock2).
    ~DNSResolver();

    //@brief Resolves a given domain name into its corresponding IP addresses(IPv4 and/or IPv6).
    vector<string> resolveDomain(const string& domain);

    //@brief Logs the resolved IP addresses for a given domain.
    void logResults(const string& domain, const vector<string>& results);
    
    //@brief Converts a socket address structure (IPv4 or IPv6) to a human-readable IP string.
    string ipToString(const ::sockaddr* addr);

private:
    // Indicates whether logging is enabled for results and errors.
    bool loggingEnabled;

    //@brief Logs an error message if logging is enabled.
    void logError(const string& message);
};
