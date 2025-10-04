#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>

class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    static Logger& get_instance() {
        static Logger instance;
        return instance;
    }

    void setLevel(spdlog::level::level_enum level) {
        for (auto& l : loggers_) {
            l->set_level(level);
        }
    }


    // std::string重载
    void debug(const std::string& msg) { debug_logger_->debug(msg.c_str()); }
    void info(const std::string& msg) { info_logger_->info(msg.c_str()); }
    void warn(const std::string& msg) { warn_logger_->warn(msg.c_str()); }
    void error(const std::string& msg) { error_logger_->error(msg.c_str()); }
    void trace(const std::string& msg) { debug_logger_->trace(msg.c_str()); }
    void critical(const std::string& msg) { error_logger_->critical(msg.c_str()); }

private:
    Logger() {
        std::string log_dir = "logs";
        if (!std::filesystem::exists(log_dir)) {
            std::filesystem::create_directories(log_dir);
        }
        // 文件名
        std::string debug_file = log_dir + "/app_debug.log";
        std::string info_file  = log_dir + "/app_info.log";
        std::string warn_file  = log_dir + "/app_warn.log";
        std::string error_file = log_dir + "/app_error.log";

        // sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto debug_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(debug_file, 0, 0, false);
        auto info_sink  = std::make_shared<spdlog::sinks::daily_file_sink_mt>(info_file, 0, 0, false);
        auto warn_sink  = std::make_shared<spdlog::sinks::daily_file_sink_mt>(warn_file, 0, 0, false);
        auto error_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(error_file, 0, 0, false);

        debug_logger_ = std::make_shared<spdlog::logger>("debug_logger", spdlog::sinks_init_list{console_sink, debug_sink});
        info_logger_  = std::make_shared<spdlog::logger>("info_logger",  spdlog::sinks_init_list{console_sink, info_sink});
        warn_logger_  = std::make_shared<spdlog::logger>("warn_logger",  spdlog::sinks_init_list{console_sink, warn_sink});
        error_logger_ = std::make_shared<spdlog::logger>("error_logger", spdlog::sinks_init_list{console_sink, error_sink});

        for (auto& l : {debug_logger_, info_logger_, warn_logger_, error_logger_}) {
            l->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            l->set_level(spdlog::level::debug);
            l->flush_on(spdlog::level::warn);
        }
        loggers_ = {debug_logger_, info_logger_, warn_logger_, error_logger_};
        info_logger_->info("Logger initialized");
    }

    ~Logger() {
        spdlog::shutdown();
    }


    // 带格式参数的日志接口
    // 这批接口容易触发const expression警告，原因是spdlog的日志接口被声明为const成员函数
    // 所以改成private成员函数
    template<typename... Args>
    void debug(const char* fmt, const Args&... args) {
        debug_logger_->debug(fmt, args...);
    }

    template<typename... Args>
    void info(const char* fmt, const Args&... args) {
        info_logger_->info(fmt, args...);
    }

    template<typename... Args>
    void warn(const char* fmt, const Args&... args) {
        warn_logger_->warn(fmt, args...);
    }

    template<typename... Args>
    void error(const char* fmt, const Args&... args) {
        error_logger_->error(fmt, args...);
    }

    template<typename... Args>
    void trace(const char* fmt, const Args&... args) {
        debug_logger_->trace(fmt, args...);
    }

    template<typename... Args>
    void critical(const char* fmt, const Args&... args) {
        error_logger_->critical(fmt, args...);
    }


    std::vector<std::shared_ptr<spdlog::logger>> loggers_;
    std::shared_ptr<spdlog::logger> debug_logger_;
    std::shared_ptr<spdlog::logger> info_logger_;
    std::shared_ptr<spdlog::logger> warn_logger_;
    std::shared_ptr<spdlog::logger> error_logger_;
};