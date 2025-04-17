/**
 * @file hello_world_main.cpp
 * @brief ESP32 mDNS设备发现应用程序 - C++实现版本
 * 
 * 这个应用程序演示了ESP32使用mDNS协议在局域网中发现设备的功能，
 * 特别是搜索_leboremote._tcp服务。使用C++类对WiFi和mDNS功能进行封装，
 * 提供更好的代码组织和资源管理。
 * 
 * 主要功能：
 * - 使用WiFiManager类管理WiFi连接
 * - 使用MDNSDiscovery类搜索网络设备
 * - 支持自动重连和错误处理
 * - 提供可靠的资源管理
 * 
 * @author Your Name
 * @date 2023
 */

#include <stdio.h>
#include <inttypes.h>
#include <string>
#include <memory>
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "mdns.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi/WiFiManager.hpp"
#include "mdns/mdns_discovery.h"

// 定义WiFi配置
#define WIFI_SSID      "LEBO_C-2.4G" //you wifi name
#define WIFI_PASSWORD  "********" //you wifi password
#define MAX_RETRY      5

static const char *TAG = "mdns_search";

// WiFi管理器共享指针
std::shared_ptr<WiFiManager> g_wifi_manager;

// 设备搜索任务句柄
TaskHandle_t device_search_task_handle = NULL;

/**
 * @brief 设备搜索任务函数
 * 
 * 定期搜索网络中的mDNS设备，特别关注_leboremote._tcp服务。
 * 此任务作为独立线程运行，完成以下工作：
 * 1. 等待WiFi连接稳定
 * 2. 初始化mDNS服务
 * 3. 定期搜索指定类型的mDNS设备
 * 4. 处理搜索结果
 * 
 * @param pvParameters FreeRTOS任务参数，本函数不使用此参数
 */
void device_search_task(void *pvParameters) {
    ESP_LOGI(TAG, "设备搜索任务已启动");
    
    // 等待WiFi连接
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // 检查WiFi是否已连接
    if (g_wifi_manager && !g_wifi_manager->isConnected()) {
        ESP_LOGE(TAG, "WiFi未连接，mDNS可能无法正常工作");
    } else if (g_wifi_manager) {
        // 打印WiFi信息
        g_wifi_manager->printInfo();
    }
    
    // 创建并初始化mDNS搜索实例
    MDNSDiscovery mdns_discovery;
    mdns_discovery.init();
    
    while (1) {
        // 确保WiFi连接状态
        if (g_wifi_manager && g_wifi_manager->isConnected()) {
            ESP_LOGI(TAG, "开始搜索设备...");
            mdns_discovery.start_discovery("_leboremote", 3000);
        } else {
            ESP_LOGW(TAG, "WiFi未连接，无法执行mDNS搜索");
        }
        
        // 5秒后再次搜索
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief 初始化NVS闪存
 * 
 * 初始化非易失性存储(NVS)，用于存储系统配置和应用数据。
 * 如果NVS分区已满或版本不匹配，会自动擦除并重新初始化。
 * 
 * @return esp_err_t 成功返回ESP_OK，失败返回对应错误码
 */
esp_err_t init_nvs() {
    ESP_LOGI(TAG, "初始化NVS Flash...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS需要擦除，正在重新初始化...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS初始化成功");
    } else {
        ESP_LOGE(TAG, "NVS初始化失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/**
 * @brief 主函数
 * 
 * 应用程序入口点，完成以下初始化工作：
 * 1. 初始化NVS存储
 * 2. 创建WiFi管理器实例并连接WiFi
 * 3. 创建设备搜索任务
 * 4. 进入主循环，保持系统运行
 * 
 * 使用extern "C"确保C++函数可以被ESP-IDF C代码调用
 */
extern "C" void app_main(void)
{
    printf("设备搜索应用启动\n");
    // 初始化NVS
    ESP_ERROR_CHECK(init_nvs());

    // 创建WiFi管理实例
    g_wifi_manager = WiFiManager::create(WIFI_SSID, WIFI_PASSWORD, MAX_RETRY);
    if (!g_wifi_manager) {
        ESP_LOGE(TAG, "WiFi管理器创建失败");
        return;
    }
    
    // 连接WiFi
    ESP_ERROR_CHECK(g_wifi_manager->connect());
    
    // 创建设备搜索任务
    ESP_LOGI(TAG, "创建设备搜索任务...");
    xTaskCreate(device_search_task, "device_search_task", 4096, NULL, 5, &device_search_task_handle);
    
    printf("主任务正在监控中...\n");
    
    while (1) {
        // 主循环 - 可以在这里添加更多功能
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
} 
