#pragma once

#include <string>
#include <vector>
#include <esp_err.h>
#include "esp_netif.h"
#include "mdns.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


struct MDNSDevice {
    std::string name;
    std::string hostname;
    std::string ip;
    std::string uid;
    std::string appId;
    uint16_t port;
};

class MDNSDiscovery {
public:
    MDNSDiscovery();
    ~MDNSDiscovery();

    esp_err_t init();
    esp_err_t start_discovery(const char* service_type = "_lebo._tcp", int timeout = 3000);
    esp_err_t stop_discovery();
    std::vector<MDNSDevice> get_discovered_devices() const;
    bool is_init() const { return m_bIsInit; }
private:
    void parse_mdns_response(mdns_result_t* result);
    std::string resolve_mdns_host(const char * host_name);

    std::vector<MDNSDevice> discovered_devices_;
    bool is_discovery_running_;
    bool m_bIsInit;
    SemaphoreHandle_t mutex_; // 互斥锁
};
