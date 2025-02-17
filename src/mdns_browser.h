#pragma once

#include <dns_sd.h>
#include <string>
#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#endif

class MDNSBrowser {
public:
    using ServiceCallback = std::function<void(
        const std::string& name,
        const std::string& host,
        const std::string& ip,
        uint16_t port,
        const std::map<std::string, std::string>& txtRecords
    )>;

    MDNSBrowser();
    ~MDNSBrowser();

    bool startBrowsing(const std::string& serviceType, ServiceCallback callback);
    void stopBrowsing();

private:
    DNSServiceRef dnssref;
    ServiceCallback serviceCallback;
    std::atomic<bool> running;

    static std::map<std::string, std::string> parseTXTRecord(
        const unsigned char* txtRecord,
        uint16_t txtLen
    );
    
    static std::string getIPAddress(const char* hostname);

    static void DNSSD_API browseCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* serviceName,
        const char* regtype,
        const char* replyDomain,
        void* context
    );

    static void DNSSD_API resolveCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* fullname,
        const char* hosttarget,
        uint16_t port,
        uint16_t txtLen,
        const unsigned char* txtRecord,
        void* context
    );
}; 