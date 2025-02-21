
```markdown
# 乐播服务发现示例程序

## 项目简介

乐播服务发现示例程序旨在帮助开发者通过乐播 Open API，轻松地发现局域网中的乐播设备，并解析其相关信息（如设备名称、UID、渠道号等）。通过使用此示例程序，开发者能够快速判断设备的功能支持情况（如是否支持会议投屏等），为后续的开发工作提供基础支持。

## 目录

1. [项目简介](#项目简介)
2. [功能介绍](#功能介绍)
3. [设备解析示例](#设备解析示例)
4. [项目结构](#项目结构)
5. [编译与运行](#编译与运行)
6. [依赖说明](#依赖说明)
7. [注意事项](#注意事项)

## 功能介绍

乐播服务发现示例程序能够通过解析局域网中乐播设备的信息，为开发者提供以下功能：

- 发现并列出所有可用的乐播设备。
- 提供设备的基本信息，包括设备名称、UID、渠道号等。
- 判断设备是否支持会议投屏等特定功能。

## 设备解析示例

以下是一个基于“我的电视T5p”设备的示例数据，展示了如何解析乐播设备的信息：

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

### 解析设备信息

- **设备名称**：`Name` 字段 → `我的电视T5p`
- **设备 UID**：`u` 字段 → `10000000138623`
- **渠道号**：`a` 字段 → `10658`
- **是否支持会议（文档投屏）**：
  - 将 `feature` 字段转为二进制形式：`162303` → `100111100111111111`
  - 判断第 13 位是否为 `1` → 支持会议功能 ✅

## 项目结构

以下是项目的目录结构示例：

```
project-root/
├── include/           # 头文件目录
│   ├── device_discovery.h       # UDP组播功能实现头文件
│   └── logger.h       # 日志实现头文件
├── src/               # 源代码目录
│   ├── main.cpp       # 主程序入口
│   └── device_discovery.cpp # UDP 组播搜索实现
├── CMakeLists.txt     # CMake 配置文件
└── README.md          # 项目说明文档
```

## 编译与运行

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

## 依赖说明

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

## 注意事项

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
