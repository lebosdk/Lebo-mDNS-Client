#include "mdns_browser.h"
#include <iostream>
#include <cstring>
#include <errno.h>
#include <string.h>

MDNSBrowser::MDNSBrowser() : dnssref(nullptr) {
}

MDNSBrowser::~MDNSBrowser() {
    stopBrowsing();
}

bool MDNSBrowser::startBrowsing(const std::string& serviceType, ServiceCallback callback) {
    serviceCallback = callback;
    running = true;

    std::cout << "Starting browse for service type: " << serviceType << std::endl;

    DNSServiceErrorType err = DNSServiceBrowse(
        &dnssref,
        0,
        kDNSServiceInterfaceIndexAny,
        serviceType.c_str(),
        "local.",
        browseCallback,
        this
    );

    if (err != kDNSServiceErr_NoError) {
        std::cerr << "DNSServiceBrowse failed with error: " << err << std::endl;
        return false;
    }

    // 处理事件
    int dns_sd_fd = DNSServiceRefSockFD(dnssref);
    if (dns_sd_fd < 0) {
        std::cerr << "Invalid DNS-SD socket" << std::endl;
        return false;
    }
    
    fd_set readfds;
    struct timeval tv;
    int timeoutCount = 0;
    const int MAX_TIMEOUTS = 33;  // 约10秒总时间 (300ms * 33 ≈ 10s)

    std::cout << "Searching for services (will timeout after 10 seconds)..." << std::endl;

    while (running && timeoutCount < MAX_TIMEOUTS) {
        // 每次循环重置fd_set
        FD_ZERO(&readfds);
        FD_SET(dns_sd_fd, &readfds);
        
        // 每次循环重置timeval
        tv.tv_sec = 0;
        tv.tv_usec = 300000;  // 300ms超时

        int result = select(dns_sd_fd + 1, &readfds, nullptr, nullptr, &tv);
        
        if (result < 0) {
            if (errno == EINTR) {
                // 如果是被信号中断，继续循环
                continue;
            }
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            break;
        } 
        else if (result == 0) {
            // 超时
            timeoutCount++;
            continue;
        }
        else if (result > 0 && FD_ISSET(dns_sd_fd, &readfds)) {
            DNSServiceErrorType processResult = DNSServiceProcessResult(dnssref);
            if (processResult != kDNSServiceErr_NoError) {
                std::cerr << "DNSServiceProcessResult failed: " << processResult << std::endl;
                break;
            }
        }
    }

    std::cout << "Search completed." << std::endl;
    return true;
}

void MDNSBrowser::stopBrowsing() {
    running = false;
    if (dnssref) {
        DNSServiceRefDeallocate(dnssref);
        dnssref = nullptr;
    }
}

std::map<std::string, std::string> MDNSBrowser::parseTXTRecord(
    const unsigned char* txtRecord,
    uint16_t txtLen
) {
    std::map<std::string, std::string> result;
    const unsigned char* end = txtRecord + txtLen;
    while (txtRecord < end) {
        uint8_t len = *txtRecord++;
        if (txtRecord + len <= end) {
            std::string record(reinterpret_cast<const char*>(txtRecord), len);
            size_t pos = record.find('=');
            if (pos != std::string::npos) {
                std::string key = record.substr(0, pos);
                std::string value = record.substr(pos + 1);
                result[key] = value;
            }
        }
        txtRecord += len;
    }
    return result;
}

std::string MDNSBrowser::getIPAddress(const char* hostname) {
    struct addrinfo hints;
    struct addrinfo* res = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;

    if (getaddrinfo(hostname, nullptr, &hints, &res) != 0) {
        return hostname;
    }

    char ipstr[INET_ADDRSTRLEN] = {0};
    if (res && res->ai_family == AF_INET) {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;
        inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, INET_ADDRSTRLEN);
    }

    if (res) {
        freeaddrinfo(res);
    }

    return ipstr[0] ? ipstr : hostname;
}

void DNSSD_API MDNSBrowser::browseCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char* serviceName,
    const char* regtype,
    const char* replyDomain,
    void* context
) {
    if (errorCode != kDNSServiceErr_NoError) return;

    auto browser = static_cast<MDNSBrowser*>(context);
    std::cout << "Found service: " << serviceName << std::endl;

    DNSServiceRef resolveRef;
    DNSServiceErrorType err = DNSServiceResolve(
        &resolveRef,
        0,
        interfaceIndex,
        serviceName,
        regtype,
        "local.",
        resolveCallback,
        browser
    );

    if (err == kDNSServiceErr_NoError) {
        DNSServiceProcessResult(resolveRef);
        DNSServiceRefDeallocate(resolveRef);
    }
}

void DNSSD_API MDNSBrowser::resolveCallback(
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
) {
    if (errorCode != kDNSServiceErr_NoError) return;

    auto browser = static_cast<MDNSBrowser*>(context);
    if (browser->serviceCallback) {
        auto txtRecords = parseTXTRecord(txtRecord, txtLen);
        std::string ipAddress = getIPAddress(hosttarget);
        port = ntohs(port);
        browser->serviceCallback(fullname, hosttarget, ipAddress, port, txtRecords);
    }
} 