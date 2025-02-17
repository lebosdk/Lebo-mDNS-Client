#include "mdns_browser.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    MDNSBrowser browser;
    
    // 设置回调函数
    auto callback = [](
        const std::string& name,
        const std::string& host,
        const std::string& ip,
        uint16_t port,
        const std::map<std::string, std::string>& txtRecords
    ) {
        std::cout << "\nService Details:" << std::endl;
        std::cout << "  Name: " << name << std::endl;
        std::cout << "  Host: " << host << std::endl;
        std::cout << "  IP: " << ip << std::endl;
        std::cout << "  Port: " << port << std::endl;
        
        if (!txtRecords.empty()) {
            std::cout << "  TXT Records:" << std::endl;
            for (const auto& record : txtRecords) {
                std::cout << "    " << record.first << " = " << record.second << std::endl;
            }
        }
        std::cout << "------------------------" << std::endl;
    };

    std::cout << "Starting mDNS browser..." << std::endl;
    
    // 使用固定的服务类型
    const char* serviceType = "_leboremote._tcp";
    
    std::cout << "\nSearching for service type: " << serviceType << std::endl;
    if (browser.startBrowsing(serviceType, callback)) {
    }
    std::cout << "\n stopBrowsing for service type: " << serviceType << std::endl;
    browser.stopBrowsing();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
} 