cmake_minimum_required(VERSION 3.10)
project(mdns_browser)

# 只生成64位Release版本
set(CMAKE_BUILD_TYPE Release)
if(WIN32)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# DNS-SD库路径
set(DNS_SD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third/dns_sd)
set(DNS_SD_INCLUDE_DIR ${DNS_SD_ROOT}/include)

# 添加源文件
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/mdns_browser.cpp
)

if(APPLE)
    # macOS 使用系统 DNS-SD 框架
    set(DNSSD_FRAMEWORK "/System/Library/Frameworks/CoreServices.framework")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${DNSSD_FRAMEWORK})
    
    # 设置编译选项
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -DUSE_DNSSD
    )
elseif(WIN32)
    # Windows 平台设置
    set(DNS_SD_LIB ${DNS_SD_ROOT}/lib/windows/dnssd.lib)
    set(DNS_SD_DLL ${DNS_SD_ROOT}/lib/windows/dnssd.dll)
    
    target_include_directories(${PROJECT_NAME} PRIVATE ${DNS_SD_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${DNS_SD_LIB})
    
    # 拷贝 DLL 到输出目录
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DNS_SD_DLL}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
else()
    # Linux 平台设置
    set(DNS_SD_LIB ${DNS_SD_ROOT}/lib/linux/libdns_sd.so)
    
    target_include_directories(${PROJECT_NAME} PRIVATE ${DNS_SD_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${DNS_SD_LIB})
    
    # 拷贝动态库到输出目录
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DNS_SD_LIB}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    
    # Linux 特定依赖
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif() 