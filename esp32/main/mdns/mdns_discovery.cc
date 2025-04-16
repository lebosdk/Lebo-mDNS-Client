#include "mdns_discovery.h"
#include <esp_log.h>
#include <string.h>
#define MAX_DISCOVERED_DEVICES 20
static const char* TAG = "MDNSDiscovery";


MDNSDiscovery::MDNSDiscovery() : is_discovery_running_(false) {}

MDNSDiscovery::~MDNSDiscovery() {
    stop_discovery();
    m_bIsInit = false;
}

esp_err_t MDNSDiscovery::init() {
    if(m_bIsInit){
        return ESP_OK;
    }
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS init failed: %s", esp_err_to_name(err));
        return err;
    }
    m_bIsInit = true;
    mutex_ = xSemaphoreCreateMutex();
    return ESP_OK;
}

esp_err_t MDNSDiscovery::start_discovery(const char* service_type, int timeout) {
    mdns_result_t *results = NULL;
    discovered_devices_.clear();
    xSemaphoreTake(mutex_, portMAX_DELAY);
    esp_err_t err = mdns_query_ptr(service_type, "_tcp", timeout, MAX_DISCOVERED_DEVICES, &results);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS query failed: %s", esp_err_to_name(err));
        xSemaphoreGive(mutex_);
        return err;
    }

    ESP_LOGI(TAG, "mdns query success");
    if (results) {
        ESP_LOGI(TAG, "found %s devices:", service_type);
        parse_mdns_response(results);
        mdns_query_results_free(results);
    } else {
        ESP_LOGI(TAG, "no %s devices found", service_type);
    }

    xSemaphoreGive(mutex_);
    ESP_LOGI(TAG, "started mdns discovery for service: %s", service_type);
    return ESP_OK;
}

esp_err_t MDNSDiscovery::stop_discovery() {
    ESP_LOGI(TAG, "stopped mdns discovery");
    return ESP_OK;
}

std::vector<MDNSDevice> MDNSDiscovery::get_discovered_devices() const {
    return discovered_devices_;
}


void MDNSDiscovery::parse_mdns_response(mdns_result_t* result) {
    if (!result) {
        return;
    }

    while(result){
        MDNSDevice device;
        if(result->hostname){
            device.hostname = result->hostname;
        }
        else{
            ESP_LOGE(TAG, "MDNS hostname is empty");
            continue;
        }
        if(result->instance_name){
            device.name = result->instance_name;
        }
        else{
            ESP_LOGE(TAG, "MDNS instance name is empty");
            continue;
        }

        
        ESP_LOGI(TAG, "hostname: %s", result->hostname);
        ESP_LOGI(TAG, "instance_name: %s", result->instance_name);

        for (int i = 0; i < result->txt_count; i++) {
            mdns_txt_item_t* txt = &result->txt[i];
            ESP_LOGI(TAG, "TXT: %s = %s", txt->key, txt->value);
            if(std::string(txt->key) == "u"){
                device.uid = txt->value;
            }
            else if(std::string(txt->key) == "a"){
                device.appId = txt->value;
            }
        }
        discovered_devices_.push_back(device);
        ESP_LOGI(TAG, "discovered_devices_.size(): %d \n\n", discovered_devices_.size());

        result = result->next;
    }
}

std::string MDNSDiscovery::resolve_mdns_host(const char * host_name) {
    std::string resolved_host = host_name;
    esp_ip4_addr_t addr;
    addr.addr = 0;

    esp_err_t err = mdns_query_a(host_name, 20000, &addr);
    if(err){
        if(err == ESP_ERR_NOT_FOUND){
            ESP_LOGE(TAG, "Host was not found: %s", host_name);
            return "";
        }
        ESP_LOGE(TAG, "Query Failed for host %s: %s", host_name, esp_err_to_name(err));
        return "";
    }

    char ip_str[16]; // 足够放下 IPv4 地址 (xxx.xxx.xxx.xxx\0)
    sprintf(ip_str, "%lu.%lu.%lu.%lu", 
            (addr.addr) & 0xff, 
            (addr.addr >> 8) & 0xff, 
            (addr.addr >> 16) & 0xff, 
            (addr.addr >> 24) & 0xff);
    resolved_host = ip_str;
    return resolved_host;
}
