cmake_minimum_required(VERSION 3.10)
project(DeviceDiscovery VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加源文件
set(SOURCES
    src/device_discovery.cpp
    src/main.cpp
)

# 创建可执行文件
add_executable(device_discovery ${SOURCES})

# 添加头文件路径
target_include_directories(device_discovery PRIVATE include)

# 处理平台特定的依赖
if(WIN32)
    target_link_libraries(device_discovery PRIVATE ws2_32)
elseif(UNIX)
    target_link_libraries(device_discovery PRIVATE pthread)
endif()

# 设置输出目录
set_target_properties(device_discovery PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
) 