/**
 * @file WiFiManager.cpp
 * @brief WiFi管理类实现文件
 * 
 * 实现ESP32设备WiFi连接管理所需的各种功能，包括：
 * - WiFi初始化与配置
 * - 连接状态管理与监控
 * - 事件处理
 * - 网络信息获取
 * 
 * @author Your Name
 * @date 2023
 */

#include "WiFiManager.hpp"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// 日志标签
static const char* TAG = "WiFiManager";

/**
 * 构造函数实现
 * 初始化所有成员变量并记录创建日志
 */
WiFiManager::WiFiManager(const std::string& ssid, const std::string& password, int max_retry)
    : m_ssid(ssid)
    , m_password(password)
    , m_max_retry(max_retry)
    , m_initialized(false)
    , m_connected(false)
    , m_retry_num(0)
    , m_netif(nullptr) {
    ESP_LOGI(TAG, "WiFiManager创建，SSID: %s", m_ssid.c_str());
}

/**
 * 析构函数实现
 * 释放所有资源，确保WiFi相关资源被正确释放
 */
WiFiManager::~WiFiManager() {
    if (m_connected) {
        disconnect();
    }
    
    if (m_initialized) {
        // 清理WiFi资源
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler));
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler));
        
        esp_netif_destroy(m_netif);
        esp_wifi_stop();
        esp_wifi_deinit();
        
        ESP_LOGI(TAG, "WiFiManager已销毁");
    }
}

/**
 * 静态工厂方法实现
 * 创建并初始化WiFiManager实例
 */
std::shared_ptr<WiFiManager> WiFiManager::create(const std::string& ssid, 
                                              const std::string& password, 
                                              int max_retry) {
    auto manager = std::make_shared<WiFiManager>(ssid, password, max_retry);
    if (manager && manager->init() == ESP_OK) {
        return manager;
    }
    return nullptr;
}

/**
 * 初始化WiFi
 * 
 * 完成ESP32 WiFi系统的初始化，包括：
 * 1. 初始化网络接口
 * 2. 创建事件循环
 * 3. 初始化WiFi驱动
 * 4. 注册事件处理函数
 * 5. 设置工作模式
 */
esp_err_t WiFiManager::init() {
    if (m_initialized) {
        ESP_LOGW(TAG, "WiFi已初始化");
        return ESP_OK;
    }
    
    // 初始化TCP/IP适配器
    ESP_ERROR_CHECK(esp_netif_init());
    
    // 创建默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 初始化wifi站点
    m_netif = esp_netif_create_default_wifi_sta();
    
    // 默认WiFi配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 注册事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, this));
    
    // 设置WiFi工作模式为station模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    m_initialized = true;
    ESP_LOGI(TAG, "WiFi初始化成功");
    return ESP_OK;
}

/**
 * 连接到WiFi网络
 * 
 * 配置WiFi参数并启动连接过程
 * 注意：连接结果通过事件回调通知，本函数仅启动连接过程
 */
esp_err_t WiFiManager::connect() {
    if (!m_initialized) {
        ESP_LOGE(TAG, "WiFi未初始化，请先调用init()");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (m_connected) {
        ESP_LOGW(TAG, "已连接到WiFi");
        return ESP_OK;
    }
    
    // 配置连接参数
    wifi_config_t wifi_config = {};
    
    // 复制SSID和密码到配置
    strncpy((char*)wifi_config.sta.ssid, m_ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, m_password.c_str(), sizeof(wifi_config.sta.password) - 1);
    
    // 设置其他连接参数
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    // 设置WiFi配置
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // 启动WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "正在连接到 %s...", m_ssid.c_str());
    m_retry_num = 0;
    
    return ESP_OK;
}

/**
 * 断开WiFi连接
 * 
 * 主动断开当前WiFi连接并更新状态
 */
esp_err_t WiFiManager::disconnect() {
    if (!m_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "断开连接失败: %s", esp_err_to_name(err));
        return err;
    }
    
    m_connected = false;
    ESP_LOGI(TAG, "已断开WiFi连接");
    return ESP_OK;
}

/**
 * 打印网络接口信息
 * 
 * 获取并显示当前网络连接的详细信息
 */
void WiFiManager::printInfo() const {
    if (!m_connected) {
        ESP_LOGI(TAG, "未连接到WiFi");
        return;
    }
    
    // 获取IP信息
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(m_netif, &ip_info);
    
    // 打印IP相关信息
    ESP_LOGI(TAG, "已连接到 %s", m_ssid.c_str());
    ESP_LOGI(TAG, "IP地址: " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "子网掩码: " IPSTR, IP2STR(&ip_info.netmask));
    ESP_LOGI(TAG, "网关: " IPSTR, IP2STR(&ip_info.gw));
    
    // 打印WiFi信号强度
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        ESP_LOGI(TAG, "WiFi信号强度(RSSI): %d dBm", ap_info.rssi);
        ESP_LOGI(TAG, "WiFi通道: %d", ap_info.primary);
    }
}

/**
 * 事件处理函数实现
 * 
 * 处理各种WiFi和IP事件，更新连接状态
 * 主要处理以下事件：
 * - WIFI_EVENT_STA_START: WiFi启动，开始连接
 * - WIFI_EVENT_STA_DISCONNECTED: WiFi断开，尝试重连
 * - IP_EVENT_STA_GOT_IP: 获取IP地址，连接成功
 */
void WiFiManager::eventHandler(void* arg, esp_event_base_t event_base,
                           int32_t event_id, void* event_data) {
    // 获取WiFiManager实例指针
    WiFiManager* wifi = static_cast<WiFiManager*>(arg);
    
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            // WiFi启动后开始连接
            ESP_LOGI(TAG, "WiFi STA已启动，尝试连接到AP");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            wifi->m_connected = false;
            
            if (wifi->m_retry_num < wifi->m_max_retry) {
                esp_wifi_connect();
                wifi->m_retry_num++;
                ESP_LOGI(TAG, "WiFi连接失败，正在重试... (%d/%d)", 
                         wifi->m_retry_num, wifi->m_max_retry);
            } else {
                ESP_LOGE(TAG, "WiFi连接失败，已达到最大重试次数");
            }
        } else {
            ESP_LOGI(TAG, "收到WiFi事件: %ld", event_id);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // 获取IP事件数据
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        ESP_LOGI(TAG, "已获取IP地址: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi->m_connected = true;
        wifi->m_retry_num = 0;
        
        // 打印网关和子网掩码
        ESP_LOGI(TAG, "网关地址: " IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(TAG, "子网掩码: " IPSTR, IP2STR(&event->ip_info.netmask));
    }
} 