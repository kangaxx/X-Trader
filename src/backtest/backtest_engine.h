#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "structs.h"


// �ز�ƻ��ṹ��
struct BacktestPlan {
    std::string symbol;         // ��Լ����
    std::string strategy;       // ��������
    std::string start_date;     // �ز⿪ʼ����
    std::string end_date;       // �ز��������
    std::map<std::string, double> parameters; // ���Բ���
};

// ��ʷ���ݽṹ��
struct HistoricalData {
    std::string date;
    double open;
    double high;
    double low;
    double close;
    double volume;
    DataLevel level = DataLevel::Minute; // ���ݼ���Ĭ�Ϸ��Ӽ�
};

// �ز����ṹ��
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
    // ��ȡ�����ļ�
    bool LoadConfig(const std::string& config_path);

    // ��ȡ�ز�ƻ�
    bool LoadPlan(const std::string& plan_path, BacktestPlan& plan);

    // ��ȡ��ʷ����
    bool LoadHistoricalData(const std::string& csv_path, std::vector<HistoricalData>& data);

    // ִ�лز�
    BacktestReport RunBacktest(const BacktestPlan& plan, const std::vector<HistoricalData>& data);

    // ���ɻزⱨ��
    bool GenerateReport(const BacktestReport& report, const std::string& output_path);

    // ����onBar�ص�����������������strategy::on_tickһ��
    void SetOnBarCallback(std::function<void(const MarketData&)> callback) {
        onBarCallback_ = std::move(callback);
    }

    ~BacktestEngine() {}

private:
    std::map<std::string, std::string> config_;
    std::function<void(const MarketData&)> onBarCallback_; // onBar�ص�����ָ��
};
