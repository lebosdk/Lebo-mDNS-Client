
# 乐播服务发现工具

乐播服务发现工具是一个用于在局域网环境中发现乐播设备的服务示例工具，帮助开发者快速获取设备信息并集成乐播 Open API。


## 目录
1. [简介](#简介)
2. [功能介绍](#功能介绍)
3. [项目结构](#项目结构)
4. [编译与运行](#编译与运行)
5. [为什么需要启动守护进程](#为什么需要启动守护进程)
6. [依赖说明](#依赖说明)


## 简介

乐播服务发现工具旨在辅助开发者使用乐播 Open API，提供一种简单的方式来发现局域网中的乐播设备，并解析其相关信息（如设备名、UID、渠道号等）。通过该项目，开发者可以轻松判断设备功能支持情况（如是否支持会议投屏），从而为后续的开发工作奠定基础。


## 功能介绍

以下是一个基于“我的电视 M6p”设备的示例数据，展示了如何解析乐播设备的信息：

### 示例信息

```plaintext
Found service: 我的电视M6p

Service Details:
  Name: 我的电视M6p._leboremote._tcp.local.
  Host: android-9d85d2f5c6c6d316.local.
  IP: android-9d85d2f5c6c6d316.local.
  Port: 52244
  TXT Records:
    a = 10658
    airplay = 52244
    appInfo = 0
    atv = 0
    channel = LEBO-APK--60093-35100
    devicemac = 58:ef:2a:78:9c:c0
    etv = 1
    feature = 162303
    h = 1080
    hmd = HappyCast3.2
    hstv = 320.00
    htv = 1
    lebofeature = 162303
    lelinkport = 52244
    mirror = 52645
    omd = Andlink
    packagename = com.hpplay.happyplay.aw
    port = 53388
    raop = 52244
    remote = 52244
    u = 10000000172342
    ver = 2.0
    version = 3.2
    vv = 2
    w = 1920
```

### 解析设备信息

- **设备名称**：`Name` 字段 → `我的电视M6p`
- **设备 UID**：`u` 字段 → `10000000172342`
- **渠道号**：`a` 字段 → `10658`
- **是否支持会议（文档投屏）**：
  - 将 `feature` 字段转为二进制形式：`162303` → `100111100111111111`
  - 判断第 13 位是否为 `1` → 支持会议功能 ✅


## 项目结构

以下是项目的目录结构示例：

```
project-root/
├── src/               # 源代码目录
│   ├── main.cpp       # 主程序入口
│   ├── mdns_browser.cpp # mDNS 浏览器实现
│   └── mdns_browser.h # mDNS 浏览器头文件
├── third/             # 第三方库目录
├── CMakeLists.txt     # CMake 配置文件
└── README.md          # 项目说明文档
```


## 编译与运行

该项目基于 mDNSResponder 实现设备搜索功能，仅实现了客户端部分，守护进程依赖 mDNSResponder 提供的服务。以下是跨平台的编译与运行方法：

### 1. 编译方法

#### Windows 环境
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

#### Linux & macOS 环境
```bash
mkdir build
cd build
cmake ..
make -j4
```

### 2. 启动守护进程

mDNSResponder 是 mDNS 协议的具体实现，用于在 PC 端发送和解析 mDNS 数据包。以下是启动守护进程的方法：

#### Windows
- 方式 1：下载并编译 mDNSResponder，运行：
  ```bash
  mDNSResponder.exe -server
  ```
- 方式 2：直接安装 [Bonjour 服务](https://support.apple.com/zh-cn/HT202043)。

#### Linux
```bash
sudo make install
sudo /etc/init.d/mdns start
```

#### macOS
macOS 自带 mDNS 服务，无需额外安装。


## 为什么需要启动守护进程？

mDNSResponder 守护进程是 mDNS 协议的核心组件，负责在网络中广播和监听设备的服务信息。具体来说：
- **广播设备信息**：当新的设备加入网络时，守护进程会自动广播该设备的服务信息，使其他设备能够发现它。
- **监听服务信息**：守护进程持续监听网络中的服务广播，将相关信息传递给客户端程序（如本工具），从而实现设备发现功能。
- **确保实时性**：通过守护进程的持续运行，可以保证设备发现过程的实时性和准确性，避免因网络延迟或中断导致的服务丢失问题。

因此，在运行本工具之前，必须确保 mDNSResponder 守护进程已正确启动，以支持局域网内的设备发现功能。


## 依赖说明

- **mDNSResponder**：mDNS 协议的具体实现，用于局域网设备发现。
- **CMake**：构建工具，用于跨平台编译。
- **Bonjour**（可选）：Apple 提供的 mDNS 实现，适用于 Windows 环境。
