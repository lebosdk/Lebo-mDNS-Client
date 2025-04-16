/**
 * @file WiFiManager.hpp
 * @brief WiFi管理类，用于ESP32设备WiFi连接管理
 * 
 * 该类封装了ESP32 WiFi功能，提供简单的接口进行WiFi初始化、连接和状态管理。
 * 使用面向对象方式设计，通过C++智能指针管理资源，避免内存泄漏。
 * 
 * @author Your Name
 * @date 2023
 */

#pragma once

#include <string>
#include <memory>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

/**
 * @class WiFiManager
 * @brief 管理ESP32的WiFi连接和状态
 * 
 * 此类封装了ESP32 WiFi连接相关的功能，包括初始化、连接、断开和状态监控。
 * 支持自动重连和连接状态跟踪，提供网络信息查询功能。
 */
class WiFiManager {
private:
    std::string m_ssid;             ///< WiFi SSID
    std::string m_password;         ///< WiFi 密码
    int m_max_retry;                ///< 最大重连次数
    bool m_initialized;             ///< 初始化状态
    bool m_connected;               ///< 连接状态
    int m_retry_num;                ///< 当前重试次数
    esp_netif_t* m_netif;           ///< 网络接口

    /**
     * @brief 事件处理函数
     * 
     * 处理WiFi和IP事件，如连接状态改变和IP地址获取。
     * 作为静态回调函数注册到ESP32事件系统，处理:
     * - WiFi连接/断开事件
     * - IP获取事件
     * - 自动重连逻辑
     * 
     * @param arg 事件参数，指向WiFiManager实例
     * @param event_base 事件基础类型（WIFI_EVENT或IP_EVENT）
     * @param event_id 事件ID
     * @param event_data 事件数据
     */
    static void eventHandler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

public:
    /**
     * @brief 构造函数
     * 
     * 创建WiFiManager实例，设置WiFi连接参数。
     * 注意：构造后需要调用init()和connect()方法完成初始化和连接。
     * 
     * @param ssid WiFi的SSID
     * @param password WiFi密码
     * @param max_retry 最大重连次数，默认为5
     */
    WiFiManager(const std::string& ssid, const std::string& password, int max_retry = 5);
    
    /**
     * @brief 析构函数
     * 
     * 确保在对象销毁时断开连接并释放资源。
     * 自动清理：
     * - 注销事件处理函数
     * - 断开WiFi连接
     * - 释放网络接口资源
     */
    ~WiFiManager();
    
    /**
     * @brief 初始化WiFi
     * 
     * 初始化WiFi驱动和网络接口，注册事件处理函数。
     * 必须在connect()之前调用。
     * 
     * @return ESP_OK成功, 否则返回错误代码
     */
    esp_err_t init();
    
    /**
     * @brief 连接到WiFi网络
     * 
     * 使用构造函数中提供的SSID和密码连接到WiFi网络。
     * 实际连接过程是异步的，状态通过事件回调更新。
     * 
     * @return ESP_OK成功启动连接过程, 否则返回错误代码
     */
    esp_err_t connect();
    
    /**
     * @brief 断开WiFi连接
     * 
     * 主动断开当前WiFi连接。
     * 
     * @return ESP_OK成功, 否则返回错误代码
     */
    esp_err_t disconnect();
    
    /**
     * @brief 检查是否已连接到WiFi
     * 
     * @return true已连接, false未连接
     */
    bool isConnected() const { return m_connected; }
    
    /**
     * @brief 获取当前重试次数
     * 
     * @return int 当前重试次数
     */
    int getRetryCount() const { return m_retry_num; }
    
    /**
     * @brief 重置重试计数器
     * 
     * 将重试计数器归零，用于手动重新开始连接尝试。
     */
    void resetRetryCount() { m_retry_num = 0; }
    
    /**
     * @brief 打印网络接口信息
     * 
     * 显示当前连接的详细信息，包括:
     * - IP地址
     * - 子网掩码
     * - 默认网关
     * - 信号强度(RSSI)
     * - WiFi通道
     */
    void printInfo() const;
    
    /**
     * @brief 创建WiFiManager实例的智能指针
     * 
     * 静态工厂方法，创建并初始化WiFiManager实例。
     * 推荐使用此方法代替直接构造，确保正确初始化。
     * 
     * @param ssid WiFi SSID
     * @param password WiFi密码
     * @param max_retry 最大重试次数，默认为5
     * @return std::shared_ptr<WiFiManager> 指向WiFiManager的共享指针，失败返回nullptr
     */
    static std::shared_ptr<WiFiManager> create(const std::string& ssid, 
                                            const std::string& password, 
                                            int max_retry = 5);
}; 