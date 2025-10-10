#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "structs.h"


// 回测计划结构体
struct BacktestPlan {
    std::string symbol;         // 合约代码
    std::string strategy;       // 策略名称
    std::string start_date;     // 回测开始日期
    std::string end_date;       // 回测结束日期
    std::map<std::string, double> parameters; // 策略参数
};

// 历史数据结构体
struct HistoricalData {
    std::string date;
    double open;
    double high;
    double low;
    double close;
    double volume;
    DataLevel level = DataLevel::Minute; // 数据级别，默认分钟级
};

// 回测结果结构体
struct BacktestReport {
    std::string symbol;
    std::string strategy;
    double total_return;
    double max_drawdown;
    double sharpe_ratio;
    std::vector<std::string> trade_logs;
};

class BacktestEngine {
public:
    // 读取配置文件
    bool LoadConfig(const std::string& config_path);

    // 读取回测计划
    bool LoadPlan(const std::string& plan_path, BacktestPlan& plan);

    // 读取历史数据
    bool LoadHistoricalData(const std::string& csv_path, std::vector<HistoricalData>& data);

    // 执行回测
    BacktestReport RunBacktest(const BacktestPlan& plan, const std::vector<HistoricalData>& data);

    // 生成回测报告
    bool GenerateReport(const BacktestReport& report, const std::string& output_path);

    // 设置onBar回调函数，参数类型与strategy::on_tick一致
    void SetOnBarCallback(std::function<void(const MarketData&)> callback) {
        onBarCallback_ = std::move(callback);
    }

    ~BacktestEngine() {}

private:
    std::map<std::string, std::string> config_;
    std::function<void(const MarketData&)> onBarCallback_; // onBar回调函数指针
};
