#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>

// 日志单例类，首字母大写
class Logger {
public:
    // 禁用拷贝构造和赋值运算符
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 禁用移动构造和移动赋值
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // 获取单例实例
    static Logger& get_instance() {
        static Logger instance;
        return instance;
    }

    // 设置日志级别
    void setLevel(spdlog::level::level_enum level) {
        logger_->set_level(level);
    }

    // 日志输出接口（const char* 格式字符串）
    template<typename... Args>
    void trace(const char* fmt, const Args&... args) {
        logger_->trace(fmt, args...);
    }
    template<typename... Args>
    void debug(const char* fmt, const Args&... args) {
        logger_->debug(fmt, args...);
    }
    template<typename... Args>
    void info(const char* fmt, const Args&... args) {
        logger_->info(fmt, args...);
    }
    template<typename... Args>
    void warn(const char* fmt, const Args&... args) {
        logger_->warn(fmt, args...);
    }
    template<typename... Args>
    void error(const char* fmt, const Args&... args) {
        logger_->error(fmt, args...);
    }
    template<typename... Args>
    void critical(const char* fmt, const Args&... args) {
        logger_->critical(fmt, args...);
    }

    // 日志输出接口（std::string 格式字符串重载，无Args参数）
    void trace(const std::string& msg) {
        logger_->trace(msg.c_str());
    }
    void debug(const std::string& msg) {
        logger_->debug(msg.c_str());
    }
    void info(const std::string& msg) {
        logger_->info(msg.c_str());
    }
    void warn(const std::string& msg) {
        logger_->warn(msg.c_str());
    }
    void error(const std::string& msg) {
        logger_->error(msg.c_str());
    }
    void critical(const std::string& msg) {
        logger_->critical(msg.c_str());
    }

private:
    // 私有构造函数
    Logger(const std::string& logger_name = "Logger",
        const std::string& file_pattern = "logs/%Y-%m-%d.log",
        int hour = 0, int minute = 0) {
        // 创建日志目录
        createLogDirectory(file_pattern);

        // 创建日志接收器
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            file_pattern, hour, minute, false  // 最后一个参数false表示不延迟创建
        );

        // 多接收器组合
        spdlog::sinks_init_list sink_list = { console_sink, file_sink };

        // 创建日志器
        logger_ = std::make_shared<spdlog::logger>(logger_name, sink_list);

        // 设置日志格式
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

        // 设置默认日志级别
        logger_->set_level(spdlog::level::info);

        // 强制刷新策略
        logger_->flush_on(spdlog::level::warn);

        // 写入一条初始化日志，确保文件被创建
        logger_->info("Logger initialized");
    }

    // 析构函数
    ~Logger() {
        spdlog::shutdown();
    }

    // 创建日志目录
    void createLogDirectory(const std::string& file_pattern) {
        std::filesystem::path log_path(file_pattern);
        std::filesystem::path log_dir = log_path.parent_path();

        if (!log_dir.empty() && !std::filesystem::exists(log_dir)) {
            std::filesystem::create_directories(log_dir);
        }
    }

    // 日志器对象
    std::shared_ptr<spdlog::logger> logger_;
};