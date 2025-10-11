#include "backtest_engine.h"
#include <fstream>
#include <sstream>
#include <iostream>

//���캯��
BacktestEngine::BacktestEngine(const std::string& symbol, const std::string& start_date, const std::string& end_date) {
    // ��ʼ������
    _config["symbol"] = symbol;
    _config["start_date"] = start_date;
    _config["end_date"] = end_date;
    _current_plan.symbol = symbol;
    _current_plan.start_date = start_date;
    _current_plan.end_date = end_date;
}

// ��ȡ�����ļ�
bool BacktestEngine::LoadConfig(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            _config[key] = value;
        }
    }
    return true;
}

// ��ȡ�ز�ƻ�
bool BacktestEngine::LoadPlan(const std::string& plan_path, BacktestPlan& plan) {
    std::ifstream file(plan_path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            if (key == "symbol") plan.symbol = value;
            else if (key == "strategy") plan.strategy = value;
            else if (key == "start_date") plan.start_date = value;
            else if (key == "end_date") plan.end_date = value;
            else {
                try {
                    plan.parameters[key] = std::stod(value);
                } catch (...) {}
            }
        }
    }
    return true;
}

// ��ȡ��ʷ����
bool BacktestEngine::LoadHistoricalData(const std::string& csv_path, std::vector<HistoricalData>& data) {
    std::ifstream file(csv_path);
    if (!file.is_open()) return false;
    std::string line;
    // �����һ��Ϊ��ͷ
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        HistoricalData bar;
        std::string token;
        std::getline(ss, bar.date, ',');
        std::getline(ss, token, ','); bar.open = std::stod(token);
        std::getline(ss, token, ','); bar.high = std::stod(token);
        std::getline(ss, token, ','); bar.low = std::stod(token);
        std::getline(ss, token, ','); bar.close = std::stod(token);
        std::getline(ss, token, ','); bar.volume = std::stod(token);
        data.push_back(bar);
    }
	_is_data_loaded = true;
    return true;
}

// ִ�лز�
BacktestReport BacktestEngine::RunBacktest(const BacktestPlan& plan, const std::vector<HistoricalData>& data) {
    BacktestReport report;
    report.symbol = plan.symbol;
    report.strategy = plan.strategy;
    double equity = 0.0;
    double peak = 0.0;
    double max_drawdown = 0.0;
    std::vector<std::string> trade_logs;

    for (const auto& bar : data) {
        // ����MarketData����
        MarketData md{};
        md.instrument_id = plan.symbol;
        md.update_time = bar.date;
        md.open_price = bar.open;
        md.highest_price = bar.high;
        md.lowest_price = bar.low;
        md.last_price = bar.close;
        md.volume = static_cast<int>(bar.volume);

        // �ص�����
        if (_onBarCallback) {
            _onBarCallback(md);
        }

        // ʾ������¼ÿ��K�����̼�
        trade_logs.push_back("Date: " + bar.date + ", Close: " + std::to_string(bar.close));
        equity += bar.close; // ���ۼ�����
        if (equity > peak) peak = equity;
        double dd = peak - equity;
        if (dd > max_drawdown) max_drawdown = dd;
    }

    report.total_return = equity;
    report.max_drawdown = max_drawdown;
    report.sharpe_ratio = 0.0; // �����ʵ�����������м���
    report.trade_logs = std::move(trade_logs);
    return report;
}

// ���ɻزⱨ��
bool BacktestEngine::GenerateReport(const BacktestReport& report, const std::string& output_path) {
    std::ofstream file(output_path);
    if (!file.is_open()) return false;
    file << "Symbol: " << report.symbol << "\n";
    file << "Strategy: " << report.strategy << "\n";
    file << "Total Return: " << report.total_return << "\n";
    file << "Max Drawdown: " << report.max_drawdown << "\n";
    file << "Sharpe Ratio: " << report.sharpe_ratio << "\n";
    file << "Trade Logs:\n";
    for (const auto& log : report.trade_logs) {
        file << log << "\n";
    }
    return true;
}