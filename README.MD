
# 乐播投屏服务本地搜索发现示例

## 1. 项目简介

智能设备在调用乐播投屏服务时，可以通过投屏码、二维码调用。但如果用户电视上没有安装乐播投屏App，或者没有打开在前台，只有后台运行的投屏服务，此时通过设备本地局域网搜索发现机制找到可使用的投屏服务，即可通过 AI 交互引导用户选择连接服务，是最方便的。

实现这种方式，需要设备端参照本示例代码实现支持局域网本地搜索发现机制，在搜索到本地投屏服务后，解析出服务信息，结合用户选择结果，将对应投屏服务的 UID（即设备 ID）提交给乐播投屏 OpenAPI 相应接口，即可实现对这台电视设备的调用。

乐播投屏服务基于 mDNS 协议（多播IP地址224.0.0.251:5353进行请求和响应）实现局域网内设备的搜索发现过程。mDNS（Multicast DNS）是一种通过局域网（LAN）内的多播地址来执行DNS解析的协议，它允许设备在没有传统DNS服务器的情况下，基于名称在本地网络中进行解析和通信。它常用于设备自动发现和服务广告，尤其是在没有中央服务器的环境中。

## 2. 功能介绍

本示例程序能够通过搜索发现并解析局域网中乐播设备的信息，为开发者提供以下功能：

- 设备发现：发现并列出所有可用的投屏服务设备。
- 信息解析：提供设备的基本信息，包括设备名称、UID、渠道号等。
- 设备能力：判断设备是否支持会议投屏等特定功能。

## 3. 设备信息解析示例

以下一个搜索到的“我的电视T5p”设备的示例数据，展示了如何解析乐播投屏设备的信息：

```plaintext
Device Information:
--------------------------------------------------
Name: 我的电视T5p._leboremote._tcp.local
IP:   192.168.4.30
TXT Records:
  a                   : 10658
  airplay             : 52245
  appInfo             : 0
  atv                 : 0
  channel             : LEBO-APK--60085-33530
  devicemac           : 9a:f4:3a:38:9f:bf
  etv                 : 1
  feature             : 162303
  h                   : 1080
  hmd                 : HappyCast3.2
  hstv                : 320.00
  htv                 : 1
  lebofeature         : 162303
  lelinkport          : 52245
  mirror              : 40105
  omd                 : Andlink
  packagename         : com.hpplay.happyplay.aw
  port                : 53388
  raop                : 52245
  remote              : 52245
  u                   : 10000000138623
  ver                 : 2.0
  version             : 3.2
  vv                  : 2
  w                   : 1920
```

### 3.1解析设备信息

- **设备名称**：`Name` 字段 → `我的电视T5p`
- **设备 UID**：`u` 字段 → `10000000138623`
- **渠道号**：`a` 字段 → `10658`
- **是否支持会议（文档投屏）**：
  - 将 `feature` 字段转为二进制形式：`162303` → `100111100111111111`
  - 判断第 13 位是否为 `1` → 支持会议功能 ✅

## 4. 项目结构

以下是项目的目录结构示例：

```
project-root/
├── include/           # 头文件目录
│   ├── device_discovery.h       # UDP组播功能实现头文件
│   └── logger.h       # 日志实现头文件
├── src/               # 源代码目录
│   ├── main.cpp       # 主程序入口,演示如何调用device_discovery.cpp中的搜索发现功能
│   └── device_discovery.cpp # UDP 组播搜索实现
├── CMakeLists.txt     # CMake 配置文件
└── README.md          # 项目说明文档
```

## 5. 编译与运行

### 5.1 编译方法

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

## 6. 依赖说明

以下是项目所需的主要依赖和工具：

1. **CMake**
   - 构建工具，用于跨平台编译。
   - 可从 [CMake 官方网站](https://cmake.org/) 下载并安装。

2. **编译工具**
   - **Windows**: 
     - 推荐使用 [Visual Studio](https://visualstudio.microsoft.com/) 进行编译。
     - 需要安装 C++ 工具链（如 MSVC）。
   - **Linux**:
     - 使用 GCC 或 Clang 编译器。
     - 确保已安装 `build-essential`（对于 Ubuntu/Debian）或其他相关开发工具。
   - **macOS**:
     - 使用 Xcode 或命令行工具（Command Line Tools）进行编译。
     - 可通过 `xcode-select --install` 安装命令行工具。

## 7. 注意事项

- 运行程序时需要判断是否有其他搜索 mdns 服务启动。如果有，需要先关闭其他服务。
- 关闭其他服务的指令：
  - **macOS**: 可以使用以下命令关闭服务：
    ```bash
    sudo launchctl unload /System/Library/LaunchDaemons/com.apple.mDNSResponder.plist
    ```
  - **Linux**: 使用以下命令关闭服务：
    ```bash
    sudo systemctl stop avahi-daemon
    ```
  - **Windows**: 使用以下命令关闭服务：
    ```bash
    net stop "Bonjour Service"
    ``` 
