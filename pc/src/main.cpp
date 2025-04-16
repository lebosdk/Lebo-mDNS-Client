/**
 * @file main.cpp
 * @brief mDNS 设备发现服务示例程序
 * 
 * 程序逻辑:
 * 1. 设备发现服务初始化
 *    - 创建 DeviceDiscovery 实例
 *    - 注册设备发现回调函数
 *    - 指定要发现的服务类型
 * 
 * 2. 启动设备发现
 *    - 创建并配置 UDP 套接字
 *    - 加入 mDNS 多播组
 *    - 发送初始查询请求
 *    - 启动接收线程
 * 
 * 3. 设备发现过程
 *    - 接收线程监听 mDNS 响应
 *    - 解析收到的 DNS 消息
 *    - 提取设备信息(名称、IP、服务)
 *    - 解析 TXT 记录获取设备属性
 * 
 * 4. 设备列表管理
 *    - 检查设备是否已存在
 *    - 新设备加入列表并通知
 *    - 已有设备更新信息
 *    - 线程安全的列表访问
 * 
 * 5. 状态更新和通知
 *    - 设备首次发现时回调
 *    - 设备信息更新时回调
 *    - 日志记录设备变化
 * 
 * 6. 资源清理
 *    - 停止接收线程
 *    - 关闭网络连接
 *    - 释放系统资源
 */

#include "device_discovery.h"
#include "logger.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

// 辅助函数：打印设备信息
void printDeviceInfo(const DeviceDiscovery::DeviceInfo& device) {
    std::cout << "\nDevice Information:" << std::endl;
    std::cout << std::setfill('-') << std::setw(50) << "-" << std::endl;
    std::cout << std::setfill(' ');
    
    std::cout << "Name: " << device.name << std::endl;
    std::cout << "IP:   " << device.ip << std::endl;
    
    if (!device.txtRecords.empty()) {
        std::cout << "TXT Records:" << std::endl;
        for (const auto& txt : device.txtRecords) {
            std::cout << "  " << std::setw(20) << std::left << txt.first 
                     << ": " << txt.second << std::endl;
        }
    }
    
    std::cout << std::setfill('-') << std::setw(50) << "-" << std::endl;
}

/**
 * @brief 设备发现回调函数
 * 
 * 处理新设备发现和设备状态更新事件
 */
void onDeviceFound(const DeviceDiscovery::DeviceInfo& device) {
    LOG_INFO("发现设备:");
    LOG_INFO("  名称: " << device.name);
    LOG_INFO("  IP: " << device.ip);
    LOG_INFO("  TXT记录数: " << device.txtRecords.size());
    for (const auto& txt : device.txtRecords) {
        LOG_INFO("    " << txt.first << " = " << txt.second);
    }
}

int main() {
    try {
        // 生成日志文件名（使用当前时间）
        std::time_t now = std::time(nullptr);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));
        std::string logFile = "mdns_discovery_" + std::string(timestamp) + ".log";

        // 初始化日志系统，不输出到控制台
        if (!Logger::getInstance().init(logFile, false)) {
            std::cerr << "Failed to initialize logger" << std::endl;
            return 1;
        }

        LOG_INFO("mDNS Discovery application started");
        std::cout << "Starting mDNS discovery..." << std::endl;
        
        DeviceDiscovery discovery;
        
        // 启动设备发现
        if (!discovery.startDiscovery("_leboremote._tcp.local", onDeviceFound)) {
            LOG_ERROR("启动设备发现失败");
            return 1;
        }

        // 等待10秒
        LOG_INFO("等待10秒搜索设备...");
        std::cout << "Searching for devices (10 seconds)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));

        // 停止设备发现
        discovery.stopDiscovery();
        
        // 获取并打印所有发现的设备
        LOG_INFO("=== 设备列表 ===");
        auto devices = discovery.getDiscoveredDevices();
        
        std::cout << "\nDiscovered Devices:" << std::endl;
        if (devices.empty()) {
            LOG_INFO("未发现设备");
            std::cout << "No devices were found." << std::endl;
        } else {
            LOG_INFO("发现 " << devices.size() << " 个设备");
            for (const auto& device : devices) {
                printDeviceInfo(device);
                
                // 只记录到日志
                LOG_INFO("设备信息:");
                LOG_INFO("  名称: " << device.name);
                LOG_INFO("  IP: " << device.ip);
                for (const auto& txt : device.txtRecords) {
                    LOG_INFO("  TXT记录 - " << txt.first << ": " << txt.second);
                }
            }
        }

        LOG_INFO("mDNS设备发现程序结束");
        std::cout << "\nDiscovery completed." << std::endl;
    } catch (const std::exception& e) {
        LOG_ERROR("发生异常: " << e.what());
        return 1;
    }

    return 0;
} 