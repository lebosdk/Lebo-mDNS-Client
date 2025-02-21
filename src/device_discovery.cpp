/**
 * @file device_discovery.cpp
 * @brief mDNS 设备发现服务实现
 *
 * 通过 mDNS 协议实现局域网内的设备发现功能:
 * - 加入 mDNS 多播组接收设备广播
 * - 发送 mDNS 查询请求搜索设备
 * - 解析 mDNS 响应获取设备信息
 * - 维护已发现设备的列表
 * - 通知设备状态变更事件
 *
 * mDNS 协议详解:
 *
 * 1. 消息格式
 * +------------+------------------+------------------+------------------+
 * | DNS Header | Question Section | Answer Section   | Additional Info  |
 * | (12 bytes) | (variable)      | (variable)       | (variable)       |
 * +------------+------------------+------------------+------------------+
 *
 * 2. DNS Header 格式
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                    ID                           |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                  QDCOUNT                        |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                  ANCOUNT                        |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                  NSCOUNT                        |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                  ARCOUNT                        |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * 3. 消息解析流程:
 * 1) 解析 DNS 头部
 *    - 验证消息长度 >= DNS头部长度(12字节)
 *    - 检查消息类型(QR位)是否为响应
 *    - 获取问题数(QDCOUNT)和应答数(ANCOUNT)
 *
 * 2) 跳过问题部分
 *    - 对每个问题记录:
 *      > 解析名称(按照长度+数据格式)
 *      > 跳过类型(QTYPE)和类(QCLASS)字段(共4字节)
 *
 * 3) 解析应答记录
 *    - 对每个应答记录:
 *      > 解析记录名称(支持压缩指针)
 *      > 读取类型(TYPE)、类(CLASS)、TTL和数据长度
 *      > 根据类型解析记录数据:
 *        * PTR(12): 服务实例名称
 *        * TXT(16): 键值对属性记录
 *        * SRV(33): 服务地址和端口
 *        * A(1)/AAAA(28): IP地址
 *
 * 4) 名称压缩处理
 *    - 检测高两位为11的长度字节(0xC0)
 *    - 获取14位偏移值
 *    - 跳转到偏移位置继续解析
 *    - 支持多级压缩引用
 *
 * 5) TXT记录解析
 *    - 按照 <length><key>=<value> 格式解析
 *    - 存储到设备信息的 txtRecords 映射中
 */

 /**
  * @brief mDNS 设备发现功能实现
  *
  * 通过 mDNS 协议发现局域网内的设备，支持:
  * - 发送 mDNS 查询
  * - 接收并解析 mDNS 响应
  * - 维护已发现设备列表
  * - 设备状态更新通知
  */

#include "device_discovery.h"
#include "logger.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <array>
#include <cstring>
#include <vector>
#include <mutex>
#include <algorithm>
#include <map>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

  // mDNS 协议常量定义
#define MDNS_PORT 5353
#define MDNS_GROUP "224.0.0.251"

/**
 * @brief DNS 消息头部结构
 *
 * 用于解析 DNS 消息的头部信息，包含:
 * - 消息标识
 * - 标志位
 * - 各类记录数量
 */
struct DNSHeader
{
    uint16_t id;      // 消息标识符
    uint16_t flags;   // 消息标志位
    uint16_t qdcount; // 问题记录数
    uint16_t ancount; // 应答记录数
    uint16_t nscount; // 授权记录数
    uint16_t arcount; // 附加记录数
};

/**
 * @brief DeviceDiscovery 的具体实现类
 *
 * 使用 PIMPL 模式实现设备发现功能，包含:
 * - mDNS 套接字管理
 * - 设备列表维护
 * - 消息收发处理
 */
class DeviceDiscovery::Impl
{
public:
    /**
     * @brief 构造函数
     *
     * 初始化设备发现服务，包括:
     * - 初始化网络环境
     * - 初始化内部状态
     */
    Impl() : running(false), socket_(INVALID_SOCKET)
    {
        LOG_INFO("初始化设备发现服务");
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            LOG_ERROR("WinSock初始化失败");
        }
#endif
    }

    ~Impl()
    {
        LOG_INFO("清理设备发现服务");
        stopDiscovery();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    /**
     * @brief 启动设备发现服务
     *
     * @param serviceType 要发现的服务类型
     * @param callback 设备发现回调函数
     * @return true 启动成功
     * @return false 启动失败
     */
    bool startDiscovery(const std::string& serviceType, const DeviceFoundCallback& callback)
    {
        LOG_INFO("开始mDNS服务发现，服务类型: " << serviceType);

        if (running)
        {
            LOG_WARN("发现服务已在运行");
            return false;
        }

        socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_ == INVALID_SOCKET)
        {
#ifdef _WIN32
            auto err = WSAGetLastError();
#else
            auto err = strerror(errno);
#endif

            LOG_ERROR("Failed to create socket, error: " << err);
            return false;
        }
        LOG_DEBUG("Socket created successfully");

        // 设置套接字选项
        int reuse = 1;
        if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
            (char*)&reuse, sizeof(reuse)) < 0)
        {
            LOG_ERROR("设置SO_REUSEADDR失败");
            closesocket(socket_);
            return false;
        }
        LOG_DEBUG("套接字选项SO_REUSEADDR设置成功");

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(MDNS_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            LOG_ERROR("Failed to bind socket to port " << MDNS_PORT);
            closesocket(socket_);
            return false;
        }
        LOG_DEBUG("Socket bound to port " << MDNS_PORT);

        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(MDNS_GROUP);
        mreq.imr_interface.s_addr = INADDR_ANY;
        if (setsockopt(socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP,
            (char*)&mreq, sizeof(mreq)) < 0)
        {
            LOG_ERROR("Failed to join multicast group " << MDNS_GROUP);
            closesocket(socket_);
            return false;
        }
        LOG_DEBUG("Joined multicast group " << MDNS_GROUP);

        // 发送初始查询
        if (!sendQuery(serviceType))
        {
            LOG_ERROR("发送初始查询失败");
            closesocket(socket_);
            return false;
        }
        LOG_DEBUG("初始查询已发送，服务类型: " << serviceType);

        running = true;
        receiveThread = std::thread([this, callback]()
            {
                LOG_INFO("Receive thread started");
                std::array<uint8_t, 1500> buffer;
                struct sockaddr_in sender;
                socklen_t senderLen = sizeof(sender);

                while (running) {
                    int bytes = recvfrom(socket_, (char*)buffer.data(), buffer.size(), 0,
                        (struct sockaddr*)&sender, &senderLen);
                    if (bytes < 0) {
#ifdef _WIN32
                        auto err = WSAGetLastError();
#else
                        auto err = strerror(errno);
#endif

                        LOG_ERROR("Error receiving data: " << err);
                        continue;
                    }
                    if (bytes > 0) {
                        LOG_DEBUG("Received " << bytes << " bytes from " <<
                            inet_ntoa(sender.sin_addr));
                        parseMDNSResponse(buffer.data(), bytes, sender, callback);
                    }
                }
                LOG_INFO("Receive thread stopped"); });

        LOG_INFO("Discovery started successfully");
        return true;
    }

    void stopDiscovery()
    {
        if (!running)
        {
            LOG_DEBUG("Discovery already stopped");
            return;
        }

        LOG_INFO("Stopping discovery");
        running = false;

        if (socket_ != INVALID_SOCKET)
        {
            LOG_DEBUG("Closing socket");
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }

        if (receiveThread.joinable())
        {
            LOG_DEBUG("Waiting for receive thread to finish");
            receiveThread.join();
        }

        LOG_INFO("Discovery stopped");
    }

    // 获取当前发现的所有设备
    std::vector<DeviceInfo> getDiscoveredDevices() const
    {
        std::lock_guard<std::mutex> lock(devicesMutex);
        return discoveredDevices;
    }

private:
    bool sendQuery(const std::string& serviceType)
    {
        LOG_DEBUG("Preparing mDNS query for " << serviceType);

        std::vector<uint8_t> query;
        DNSHeader header = { 0 };
        header.flags = htons(0x0100);
        header.qdcount = htons(1);

        query.insert(query.end(), (uint8_t*)&header, (uint8_t*)(&header + 1));
        addDNSName(query, serviceType);

        uint16_t type = htons(12);  // PTR
        uint16_t qclass = htons(1); // IN
        query.insert(query.end(), (uint8_t*)&type, (uint8_t*)(&type + 1));
        query.insert(query.end(), (uint8_t*)&qclass, (uint8_t*)(&qclass + 1));

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(MDNS_PORT);
        addr.sin_addr.s_addr = inet_addr(MDNS_GROUP);

        int sent = sendto(socket_, (char*)query.data(), query.size(), 0,
            (struct sockaddr*)&addr, sizeof(addr));

        if (sent < 0)
        {
#ifdef _WIN32
            auto err = WSAGetLastError();
#else
            auto err = strerror(errno);
#endif

            LOG_ERROR("Failed to send query: " << err);
            return false;
        }

        LOG_DEBUG("Query sent successfully, " << sent << " bytes");
        return true;
    }

    void addDNSName(std::vector<uint8_t>& packet, const std::string& name)
    {
        size_t start = 0;
        size_t end = name.find('.');

        while (end != std::string::npos)
        {
            std::string label = name.substr(start, end - start);
            packet.push_back(static_cast<uint8_t>(label.length()));
            packet.insert(packet.end(), label.begin(), label.end());

            start = end + 1;
            end = name.find('.', start);
        }

        std::string label = name.substr(start);
        if (!label.empty())
        {
            packet.push_back(static_cast<uint8_t>(label.length()));
            packet.insert(packet.end(), label.begin(), label.end());
        }

        packet.push_back(0); // 终止符
    }

    // 添加一个临时缓存来存储未完成的设备信息
    std::map<std::string, DeviceInfo> deviceCache;

    void parseMDNSResponse(const uint8_t* data, int size,
        const sockaddr_in& sender,
        const DeviceFoundCallback& callback)
    {
        LOG_DEBUG("Parsing mDNS response from " << inet_ntoa(sender.sin_addr)
            << ", size: " << size << " bytes");
        const uint8_t* basePtr = data;
        if (size < sizeof(DNSHeader))
        {
            LOG_ERROR("Response too small: " << size << " bytes (minimum "
                << sizeof(DNSHeader) << " bytes required)");
            return;
        }

        const DNSHeader* header = reinterpret_cast<const DNSHeader*>(data);
        uint16_t flags = ntohs(header->flags);
        bool isResponse = (flags & 0x8000) != 0;

        if (!isResponse)
        {
            LOG_DEBUG("Ignoring non-response packet (flags: 0x"
                << std::hex << flags << std::dec << ")");
            return;
        }

        uint16_t qdcount = ntohs(header->qdcount);
        uint16_t ancount = ntohs(header->ancount);

        LOG_DEBUG("Response contains " << qdcount << " questions and "
            << ancount << " answers");

        const uint8_t* ptr = data + sizeof(DNSHeader);
        const uint8_t* end = data + size;

        // Skip questions
        for (uint16_t i = 0; i < qdcount; i++)
        {
            std::string qname;
            if (parseDNSName(basePtr, ptr, end, qname))
            {
                ptr += 4; // Skip QTYPE and QCLASS
            }
        }

        try
        {
            DeviceInfo tempInfo;
            tempInfo.ip = inet_ntoa(sender.sin_addr);
            bool isTargetService = false;

            // Parse answer records
            for (uint16_t i = 0; i < ancount; i++)
            {
                std::string name;
                uint16_t type;
                uint16_t rdlength;
                const uint8_t* rdata;

                if (!parseRecordHeader(basePtr, ptr, end, name, type, rdlength, rdata))
                {
                    LOG_WARN("Failed to parse record header");
                    continue;
                }

                // 检查是否是我们感兴趣的服务
                if (name.find("_leboremote._tcp.local") != std::string::npos)
                {
                    isTargetService = true;
                    tempInfo.name = name;
                }

                switch (type)
                {
                case 16: // TXT
                {
                    if (isTargetService) {
                        auto tempPtr = rdata;
                        parseTXT(tempPtr, rdlength, tempInfo.txtRecords);
                        LOG_DEBUG("Found TXT records, count: " << tempInfo.txtRecords.size());
                        for (const auto& txt : tempInfo.txtRecords) {
                            LOG_DEBUG("  " << txt.first << " = " << txt.second);
                        }
                        //
                    }
                }
                break;
                }

                ptr = rdata + rdlength;
            }

            // 只有当设备信息完整且是目标服务时才通知
            if (!tempInfo.name.empty() && isTargetService)
            {
                std::lock_guard<std::mutex> lock(devicesMutex);
                auto it = std::find_if(discoveredDevices.begin(),
                    discoveredDevices.end(),
                    [&](const DeviceInfo& d) {
                        return d.name == tempInfo.name;
                    });

                if (it == discoveredDevices.end())
                {
                    discoveredDevices.push_back(tempInfo);
                    LOG_INFO("Device Information [" << discoveredDevices.size() - 1 << "]:");
                    LOG_INFO("  Name: " << tempInfo.name);
                    LOG_INFO("  IP: " << tempInfo.ip);
                    LOG_INFO("  TXT Records: " << tempInfo.txtRecords.size());
                    for (const auto& txt : tempInfo.txtRecords) {
                        LOG_INFO("    " << txt.first << " = " << txt.second);
                    }
                    callback(tempInfo);
                }
                else
                {
                    size_t index = std::distance(discoveredDevices.begin(), it);
                    if (!tempInfo.txtRecords.empty())
                    {
                        // 只在有新的TXT记录时更新
                        it->txtRecords = tempInfo.txtRecords;
                        it->ip = tempInfo.ip;
                        LOG_INFO("Device Updated [" << index << "]:");
                        LOG_INFO("  Name: " << it->name);
                        LOG_INFO("  IP: " << it->ip);
                        LOG_INFO("  TXT Records: " << it->txtRecords.size());
                        for (const auto& txt : it->txtRecords) {
                            LOG_INFO("    " << txt.first << " = " << txt.second);
                        }
                        callback(*it);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Exception while parsing response: " << e.what());
        }
    }

    bool parseRecordHeader(const uint8_t*& basePtr, const uint8_t*& ptr, const uint8_t* end,
        std::string& name, uint16_t& type,
        uint16_t& rdlength, const uint8_t*& rdata)
    {
        if (!parseDNSName(basePtr, ptr, end, name))
        {
            LOG_ERROR("Failed to parse DNS name in record header");
            return false;
        }

        if (ptr + 10 > end)
        {
            LOG_ERROR("Record header parse failed: insufficient data for header (need 10 bytes)");
            return false;
        }

        type = (ptr[0] << 8) | ptr[1];
        uint16_t rclass = (ptr[2] << 8) | ptr[3];
        uint32_t ttl = (ptr[4] << 24) | (ptr[5] << 16) | (ptr[6] << 8) | ptr[7];
        rdlength = (ptr[8] << 8) | ptr[9];
        ptr += 10;

        LOG_DEBUG("Record header - Type: " << type << ", Class: " << rclass
            << ", TTL: " << ttl << ", Length: " << rdlength);

        if (ptr + rdlength > end)
        {
            LOG_ERROR("Record data parse failed: data length " << rdlength << " exceeds buffer");
            return false;
        }

        rdata = ptr;
        return true;
    }

    /**
     * @brief 解析 DNS 名称
     *
     * 支持 DNS 消息压缩，处理以下情况:
     * - 普通名称标签序列
     * - 压缩指针引用
     * - 混合压缩形式
     *
     * @param basePtr 消息起始位置
     * @param ptr 当前解析位置
     * @param end 消息结束位置
     * @param name 解析得到的名称
     * @return true 解析成功
     * @return false 解析失败
     */
    bool parseDNSName(const uint8_t*& basePtr, const uint8_t*& ptr, const uint8_t* end, std::string& name)
    {
        name.clear();
        bool first = true;

        while (ptr < end)
        {
            uint8_t len = *ptr++;

            // Check for DNS name compression (high 2 bits set)
            if (len & 0xC0)
            {

                // Calculate offset from compression pointer
                uint16_t offset = ((len & 0x3F) << 8) | *ptr;
                const uint8_t* newPtr = basePtr + offset;

                if (newPtr >= end || newPtr < basePtr)
                {
                    LOG_ERROR("Invalid compression pointer offset: " << offset);
                    return false;
                }

                // Parse the rest of the name from the new location
                std::string remainingName;
                const uint8_t* tempPtr = newPtr;
                if (!parseDNSName(basePtr, tempPtr, end, remainingName))
                {
                    return false;
                }

                if (!first)
                {
                    name += '.';
                }
                name += remainingName;
                ptr++; // Move past the second byte of the compression pointer
                return true;
            }

            // Zero length means end of name
            if (len == 0)
            {
                break;
            }

            if (ptr + len > end)
            {
                LOG_ERROR("DNS name parse failed: length " << (int)len << " exceeds buffer");
                return false;
            }

            // Add dot between labels, except for the first one
            if (!first)
            {
                name += '.';
            }
            first = false;

            // Append this label
            name.append(reinterpret_cast<const char*>(ptr), len);
            ptr += len;
        }

        if (name.empty())
        {
            LOG_ERROR("DNS name parse failed: empty name");
            return false;
        }

        LOG_DEBUG("Parsed DNS name: " << name << ", total length: " << name.length());
        return true;
    }

    /**
     * @brief 解析 TXT 记录
     *
     * 解析 DNS TXT 记录中的键值对信息
     * 格式: <length><key>=<value>
     *
     * @param ptr 记录数据起始位置
     * @param length 记录总长度
     * @param txtRecords 解析得到的键值对
     */
    void parseTXT(const uint8_t* ptr, uint16_t length,
        std::map<std::string, std::string>& txtRecords)
    {
        const uint8_t* end = ptr + length;
        while (ptr < end)
        {
            uint8_t len = *ptr++;
            if (ptr + len > end)
            {
                LOG_ERROR("TXT record parse failed: length " << (int)len << " exceeds buffer");
                break;
            }

            std::string record(reinterpret_cast<const char*>(ptr), len);
            size_t sep = record.find('=');
            if (sep != std::string::npos)
            {
                std::string key = record.substr(0, sep);
                std::string value = record.substr(sep + 1);
                txtRecords[key] = value;
                LOG_DEBUG("Parsed TXT record - " << key << ": " << value);
            }
            else
            {
                LOG_WARN("Invalid TXT record format (missing '='): " << record);
            }
            ptr += len;
        }
    }

    // 添加设备列表相关成员
    mutable std::mutex devicesMutex;
    std::vector<DeviceInfo> discoveredDevices;

    // 添加设备到列表
    void addDevice(const DeviceInfo& device)
    {
        std::lock_guard<std::mutex> lock(devicesMutex);
        // 检查设备是否已存在
        auto it = std::find(discoveredDevices.begin(), discoveredDevices.end(), device);
        if (it == discoveredDevices.end())
        {
            LOG_INFO("添加新设备到列表: " << device.name << " 位于 " << device.ip);
            discoveredDevices.push_back(device);
        }
        else
        {
            LOG_DEBUG("设备已在列表中: " << device.name << " 位于 " << device.ip);
            // 更新TXT记录
            it->txtRecords = device.txtRecords;
        }
    }

    std::atomic<bool> running;
    SOCKET socket_;
    std::thread receiveThread;
};

// 实现接口方法
DeviceDiscovery::DeviceDiscovery() : pImpl(new Impl()) {}
DeviceDiscovery::~DeviceDiscovery() = default;

bool DeviceDiscovery::startDiscovery(const std::string& serviceType,
    const DeviceFoundCallback& callback)
{
    return pImpl->startDiscovery(serviceType, callback);
}

void DeviceDiscovery::stopDiscovery()
{
    pImpl->stopDiscovery();
}

// 实现获取设备列表的方法
std::vector<DeviceDiscovery::DeviceInfo> DeviceDiscovery::getDiscoveredDevices() const
{
    return pImpl->getDiscoveredDevices();
}