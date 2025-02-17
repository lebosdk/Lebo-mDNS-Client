## 一、功能介绍  

该项目是一个 **PC 局域网环境发现乐播服务的示例**，主要用于辅助乐播 Open 接口的使用。  

通过该项目，可以获取 **乐播设备名、UID、渠道号、是否支持会议** 等信息。以下是基于 **我的电视 M6p** 设备的示例数据：  

### 示例信息  
```plaintext
设备2
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
- **设备名称**：`Name` 字段 → **我的电视M6p**  
- **设备 UID**：`u` 字段 → **10000000172342**  
- **渠道号**：`a` 字段 → **10658**  
- **是否支持会议（文档投屏）**：
  - **`feature` 字段转二进制，判断第 13 位是否为 1**
  - `feature = 162303` → 二进制 **100111100111111111**
  - **第 13 位为 1**，表示 **支持会议** ✅  

---

## 二、编译方法  

该项目基于 **苹果 mDNSResponder** 实现设备搜索功能，**仅实现了客户端部分**，守护进程依赖 **mDNSResponder 提供的服务**。  

- **mDNSResponder 版本**：`mDNSResponder-2559.60.39.0.1`  
- **源码下载地址**：[mDNSResponder GitHub](https://github.com/apple-oss-distributions/mDNSResponder/tags)  

### 1. 编译方法  
#### **Windows 环境**
```sh
mkdir build
cd build
cmake ..
cmake --build .
```

#### **Linux & macOS 环境**
```sh
mkdir build
cd build
cmake ..
make -j4
```

---

## 三、为什么需要启动守护进程？  

**mDNS（Multicast DNS）** 是一种局域网内 **无中心化的设备发现协议**，可用于在没有 DNS 服务器的网络环境下，实现设备自动发现和解析。  

### **1. mDNS 的作用**
- 在局域网内自动发现 **支持 mDNS 的设备**（如智能电视、投屏设备、智能家居等）。
- 允许设备通过 **设备名称（如 `我的电视M6p.local`）解析 IP 地址**，而无需手动配置 IP。
- 提供 **服务发现**（Service Discovery），让应用程序能找到 **特定类型的服务**（如 AirPlay、打印机、投屏等）。

### **2. 为什么必须运行 mDNSResponder？**
- **mDNSResponder 是 mDNS 协议的具体实现**，用于在 PC 端发送和解析 mDNS 数据包。
- 该项目 **仅实现了客户端部分**，需要依赖 **mDNSResponder 守护进程** 来提供 mDNS 服务。
- **如果没有运行 mDNSResponder，PC 无法在局域网内发现乐播设备**，也无法解析设备名称。

---

## 四、启动守护进程  

#### **Windows**  
- **方式 1**：下载 `mDNSResponder`，编译完成后运行：  
  ```sh
  mDNSResponder.exe -server
  ```
- **方式 2**：将 `mDNSResponder.exe` **注册为服务**
- **方式 3**：直接安装 **Bonjour 服务**（[下载地址](https://support.apple.com/zh-cn/106380)）  

#### **Linux**  
- 下载 `mDNSResponder`，编译完成后执行：
  ```sh
  make install
  /etc/init.d/mdns start
  ```
- 参考 `mDNSResponder` 工程下 `mDNSPosix` 的 `readme.txt`  

#### **macOS**  
- **macOS 自带 mDNS 服务**，无需额外安装  