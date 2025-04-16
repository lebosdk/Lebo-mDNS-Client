/**
 * @file logger.h
 * @brief 日志系统实现
 *
 * 提供线程安全的日志记录功能:
 * - 支持文件和控制台输出
 * - 多个日志级别(DEBUG/INFO/WARN/ERROR)
 * - 自动添加时间戳
 * - 线程安全的日志写入
 *
 * 使用方式:
 * 1. 初始化日志系统
 *    Logger::getInstance().init("app.log", true);  // 同时输出到文件和控制台
 *
 * 2. 记录日志
 *    LOG_DEBUG("调试信息");
 *    LOG_INFO("普通信息");
 *    LOG_WARN("警告信息");
 *    LOG_ERROR("错误信息");
 *
 * 3. 日志格式
 *    [时间戳] [日志级别] [文件:行号] 日志内容
 *    例如: [2024-02-20 10:30:15] [INFO] [main.cpp:42] 程序启动
 */

#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <mutex>
#include <memory>

 /**
  * @brief 日志级别枚举
  */
enum class LogLevel {
    LOG_DEBUG,  ///< 调试信息
    LOG_INFO,   ///< 普通信息
    LOG_WARN,   ///< 警告信息
    LOG_ERROR   ///< 错误信息
};

/**
 * @brief 日志系统类
 *
 * 使用单例模式实现，确保全局只有一个日志实例
 */
class Logger {
public:
    /**
     * @brief 获取日志系统单例
     *
     * @return Logger& 日志系统实例
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    /**
     * @brief 初始化日志系统
     *
     * @param filename 日志文件名
     * @param console_output 是否同时输出到控制台
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool init(const std::string& filename, bool console_output = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        logFile_.open(filename, std::ios::out | std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
            return false;
        }
        consoleOutput_ = console_output;
        return true;
    }

    /**
     * @brief 写入日志
     *
     * @param level 日志级别
     * @param file 源文件名
     * @param line 行号
     * @param message 日志内容
     */
    void log(LogLevel level, const char* file, int line, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 获取当前时间
        std::time_t now = std::time(nullptr);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        // 获取日志级别字符串
        const char* levelStr;
        switch (level) {
        case LogLevel::LOG_DEBUG:   levelStr = "DEBUG"; break;
        case LogLevel::LOG_INFO:    levelStr = "INFO"; break;
        case LogLevel::LOG_WARN:    levelStr = "WARN"; break;
        case LogLevel::LOG_ERROR:   levelStr = "ERROR"; break;
        }

        // 构建日志消息
        std::ostringstream logMessage;
        logMessage << "[" << timestamp << "][" << levelStr << "][" << file << ":" << line << "] "
            << message << std::endl;

        // 写入文件
        if (logFile_.is_open()) {
            logFile_ << logMessage.str();
            logFile_.flush();
        }

        // 根据设置决定是否输出到控制台
        if (consoleOutput_) {
            std::cout << logMessage.str();
        }
    }

    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile_;
    std::mutex mutex_;
    bool consoleOutput_ = true;  // 控制是否输出到控制台
};

// 日志宏定义
#define LOG_DEBUG(msg) { \
    std::ostringstream oss; \
    oss << msg; \
    Logger::getInstance().log(LogLevel::LOG_DEBUG, __FILE__, __LINE__, oss.str()); \
}

#define LOG_INFO(msg) { \
    std::ostringstream oss; \
    oss << msg; \
    Logger::getInstance().log(LogLevel::LOG_INFO, __FILE__, __LINE__, oss.str()); \
}

#define LOG_WARN(msg) { \
    std::ostringstream oss; \
    oss << msg; \
    Logger::getInstance().log(LogLevel::LOG_WARN, __FILE__, __LINE__, oss.str()); \
}

#define LOG_ERROR(msg)  { \
    std::ostringstream oss; \
    oss << msg; \
    Logger::getInstance().log(LogLevel::LOG_ERROR, __FILE__, __LINE__, oss.str()); \
}