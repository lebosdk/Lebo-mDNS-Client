/**
 * @file device_discovery.h
 * @brief mDNS 设备发现服务接口
 * @details 提供基于 mDNS 协议的设备发现功能，支持:
 *  - 自动发现局域网内的设备
 *  - 解析设备信息和服务属性
 *  - 设备状态变更通知
 *  - 线程安全的设备列表管理
 *  - 支持设备广播和发现双重角色
 * 
 * 工作流程:
 * 1. 设备发现模式:
 *    - 创建 UDP 套接字并加入 mDNS 多播组
 *    - 发送 mDNS 查询请求
 *    - 接收并解析设备响应
 *    - 通过回调通知发现的设备
 * 
 * 2. 设备广播模式:
 *    - 创建 UDP 套接字
 *    - 定期发送 mDNS 广播
 *    - 响应其他设备的查询
 * 
 * 使用示例:
 * @code
 * // 设备发现模式
 * void onDeviceFound(const DeviceDiscovery::DeviceInfo& device) {
 *     std::cout << "Found device: " << device.name 
 *               << " at " << device.ip << std::endl;
 * }
 * 
 * DeviceDiscovery discovery;
 * discovery.startDiscovery("_leboremote._tcp.local", onDeviceFound);
 * 
 * // 设备广播模式
 * std::map<std::string, std::string> txtRecords;
 * txtRecords["version"] = "1.0";
 * txtRecords["type"] = "sensor";
 * discovery.startBroadcast("MyDevice", txtRecords);
 * @endcode
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>

/**
 * @brief 设备发现服务类
 * @details 使用 PIMPL 模式实现，主要功能:
 *  - 创建和管理 mDNS 套接字
 *  - 发送设备发现查询
 *  - 解析设备响应消息
 *  - 维护设备列表
 *  - 触发设备发现回调
 *  - 支持设备广播功能
 * 
 * 线程安全说明:
 *  - 所有公开方法都是线程安全的
 *  - 回调函数在接收线程中执行
 *  - 设备列表访问通过互斥锁保护
 */
class DeviceDiscovery {
public:
    /**
     * @brief 设备信息结构
     * @details 包含从 mDNS 响应中解析的设备信息:
     *  - name: 设备名称，格式为 "<instance>._<service>._<protocol>.local"
     *  - ip: 设备IP地址，支持 IPv4
     *  - txtRecords: 设备的 TXT 记录，包含设备属性
     * 
     * TXT 记录示例:
     *  - version: 设备版本号
     *  - type: 设备类型
     *  - id: 设备唯一标识
     *  - capabilities: 设备能力
     */
    struct DeviceInfo {
        std::string name;                              ///< 设备名称
        std::string ip;                                ///< 设备IP地址
        std::map<std::string, std::string> txtRecords; ///< 设备TXT记录

        bool operator==(const DeviceInfo& other) const {
            return name == other.name;  // 使用设备名称作为唯一标识
        }
    };

    /**
     * @brief 设备发现回调函数类型
     * @details 当发现新设备或设备状态更新时调用
     * 回调函数在接收线程中执行，注意线程安全
     * 
     * 触发时机:
     *  - 首次发现新设备
     *  - 设备 TXT 记录更新
     *  - 设备 IP 地址变更
     */
    using DeviceFoundCallback = std::function<void(const DeviceInfo&)>;

    DeviceDiscovery();
    ~DeviceDiscovery();

    /**
     * @brief 启动设备发现
     * @details 创建 mDNS 套接字，加入多播组，启动接收线程
     * 
     * 执行步骤:
     * 1. 创建 UDP 套接字
     * 2. 设置套接字选项(地址重用)
     * 3. 绑定到 mDNS 端口(5353)
     * 4. 加入多播组(224.0.0.251)
     * 5. 发送初始查询
     * 6. 启动接收线程
     * 
     * @param serviceType 要发现的服务类型，例如 "_leboremote._tcp.local"
     * @param callback 设备发现回调函数
     * @return true 启动成功
     * @return false 启动失败，可能原因:
     *  - 套接字创建失败
     *  - 端口绑定失败
     *  - 加入多播组失败
     *  - 发送查询失败
     *  - 内存分配失败
     */
    bool startDiscovery(const std::string& serviceType, 
                       const DeviceFoundCallback& callback);

    /**
     * @brief 停止设备发现
     * @details 停止接收线程，关闭套接字，清理资源
     * 
     * 执行步骤:
     * 1. 设置停止标志
     * 2. 等待接收线程结束
     * 3. 关闭套接字
     * 4. 清理设备列表
     */
    void stopDiscovery();

    /**
     * @brief 开始广播设备
     * @details 将本机作为设备广播到网络
     * 
     * @param deviceName 设备名称
     * @param txtRecords 设备属性记录
     * @return true 启动成功
     * @return false 启动失败
     */
    bool startBroadcast(const std::string& deviceName, 
                       const std::map<std::string, std::string>& txtRecords);

    /**
     * @brief 停止设备广播
     * @details 停止发送 mDNS 广播，关闭广播套接字
     */
    void stopBroadcast();

    /**
     * @brief 获取已发现的设备列表
     * @details 返回当前已发现的所有设备信息
     * 此方法是线程安全的，通过互斥锁保护
     * 
     * @note 返回的是设备列表的副本，调用时会有复制开销
     * @return std::vector<DeviceInfo> 设备列表的副本
     */
    std::vector<DeviceInfo> getDiscoveredDevices() const;

private:
    // PIMPL模式，隐藏实现细节
    class Impl;
    std::unique_ptr<Impl> pImpl;
}; 