#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <cmath>


// 交易记录结构
struct TradeRecord {
    time_t datetime;      // 时间
    bool is_buy;          // 买入/卖出
    int size;             // 手数
    double price;         // 价格
    double commission;    // 手续费
};

// 策略参数
struct StrategyParams {
    int fast_length = 20;     // 快速周期(分钟)
    int slow_length = 60;     // 慢速周期(分钟)
    int atr_length = 14;      // ATR周期
    double risk_factor = 1.0; // 风险系数
    double exit_multiplier = 2.0; // 退出乘数
    int start_hour = 9;       // 开始交易小时
    int start_minute = 0;     // 开始交易分钟
    int end_hour = 14;        // 结束交易小时
    int end_minute = 50;      // 结束交易分钟
};

// 交易账户类
class TradingAccount {
private:
    double balance;           // 账户余额
    double margin;            // 保证金比例
    double commission_rate;   // 手续费率
    int contract_multiplier;  // 合约乘数
    
public:
    TradingAccount(double initial_balance, double margin_ratio, 
                  double comm_rate, int multiplier) 
        : balance(initial_balance), margin(margin_ratio), 
          commission_rate(comm_rate), contract_multiplier(multiplier) {}
    
    // 获取账户余额
    double get_balance() const { return balance; }
    
    // 计算手续费
    double calculate_commission(double price, int size) const {
        return price * size * contract_multiplier * commission_rate;
    }
    
    // 执行交易
    bool execute_trade(bool is_buy, int size, double price, TradeRecord& record) {
        // 计算所需保证金
        double required_margin = price * size * contract_multiplier * margin;
        if (required_margin > balance) {
            return false; // 保证金不足
        }
        
        // 计算手续费
        double commission = calculate_commission(price, size);
        
        // 更新账户余额
        if (is_buy) {
            balance -= required_margin + commission;
        } else {
            balance += required_margin - commission; // 简化处理，实际应根据持仓计算
        }
        
        // 记录交易
        record.is_buy = is_buy;
        record.size = size;
        record.price = price;
        record.commission = commission;
        
        return true;
    }
};

// 海龟策略类
class IntradayTurtleStrategy {
private:
    StrategyParams params;
    TradingAccount account;
    std::vector<BarData> bars;
    std::vector<TradeRecord> trades;
    
    // 持仓信息
    int position = 0;        // 持仓手数，正数为多，负数为空
    double entry_price = 0;  // 入场价格
    
    // 指标值
    std::vector<double> atr_values;
    std::vector<double> high_values;
    std::vector<double> low_values;
    std::vector<double> slow_high_values;
    std::vector<double> slow_low_values;
    
public:
    IntradayTurtleStrategy(const StrategyParams& params, const TradingAccount& account)
        : params(params), account(account) {}
    
    // 加载K线数据
    bool load_data(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开数据文件: " << filename << std::endl;
            return false;
        }
        
        std::string line;
        // 跳过表头
        std::getline(file, line);
        
        while (std::getline(file, line)) {
            BarData bar;
            std::istringstream iss(line);
            std::string datetime_str;
            
            // 假设数据格式: 日期时间,开盘价,最高价,最低价,收盘价,成交量
            if (std::getline(iss, datetime_str, ',') &&
                iss >> bar.open && iss.ignore() &&
                iss >> bar.high && iss.ignore() &&
                iss >> bar.low && iss.ignore() &&
                iss >> bar.close && iss.ignore() &&
                iss >> bar.volume) {
                
                // 解析时间字符串 (假设格式: YYYY-MM-DD HH:MM)
                struct tm tm;
                strptime(datetime_str.c_str(), "%Y-%m-%d %H:%M", &tm);
                bar.datetime = mktime(&tm);
                
                bars.push_back(bar);
            }
        }
        
        std::cout << "加载数据完成，共 " << bars.size() << " 根K线" << std::endl;
        return true;
    }
    
    // 计算指标
    void calculate_indicators() {
        int n = bars.size();
        atr_values.resize(n, 0.0);
        high_values.resize(n, 0.0);
        low_values.resize(n, 0.0);
        slow_high_values.resize(n, 0.0);
        slow_low_values.resize(n, 0.0);
        
        // 计算ATR
        for (int i = 1; i < n; ++i) {
            double tr = std::max({bars[i].high - bars[i].low,
                                 std::fabs(bars[i].high - bars[i-1].close),
                                 std::fabs(bars[i].low - bars[i-1].close)});
                                 
            if (i < params.atr_length) {
                atr_values[i] = (atr_values[i-1] * i + tr) / (i + 1);
            } else {
                atr_values[i] = (atr_values[i-1] * (params.atr_length - 1) + tr) / params.atr_length;
            }
        }
        
        // 计算高低点
        for (int i = 0; i < n; ++i) {
            int start = std::max(0, i - params.fast_length + 1);
            double max_high = bars[start].high;
            double min_low = bars[start].low;
            
            for (int j = start; j <= i; ++j) {
                max_high = std::max(max_high, bars[j].high);
                min_low = std::min(min_low, bars[j].low);
            }
            
            high_values[i] = max_high;
            low_values[i] = min_low;
            
            // 计算慢速周期高低点
            start = std::max(0, i - params.slow_length + 1);
            max_high = bars[start].high;
            min_low = bars[start].low;
            
            for (int j = start; j <= i; ++j) {
                max_high = std::max(max_high, bars[j].high);
                min_low = std::min(min_low, bars[j].low);
            }
            
            slow_high_values[i] = max_high;
            slow_low_values[i] = min_low;
        }
    }
    
    // 检查是否在交易时间内
    bool is_in_trading_hours(time_t datetime) {
        struct tm* tm_info = localtime(&datetime);
        
        // 非交易日(周六周日)不交易
        if (tm_info->tm_wday == 0 || tm_info->tm_wday == 6) {
            return false;
        }
        
        // 组合成分钟数进行比较
        int current_time = tm_info->tm_hour * 60 + tm_info->tm_min;
        int start_time = params.start_hour * 60 + params.start_minute;
        int end_time = params.end_hour * 60 + params.end_minute;
        
        return current_time >= start_time && current_time <= end_time;
    }
    
    // 计算头寸大小
    int calculate_position_size(int index) {
        if (index < params.atr_length || atr_values[index] == 0) {
            return 1; // 最小1手
        }
        
        // 按账户1%风险计算头寸
        double account_value = account.get_balance();
        double risk_per_trade = account_value * 0.01;
        int contract_multiplier = 10; // 螺纹钢10吨/手
        
        double position_size = risk_per_trade / (atr_values[index] * contract_multiplier);
        
        // 取整数手数，最少1手
        return std::max(1, (int)std::round(position_size));
    }
    
    // 运行策略
    void run() {
        if (bars.empty()) {
            std::cerr << "没有数据可运行策略" << std::endl;
            return;
        }
        
        // 计算指标
        calculate_indicators();
        
        std::cout << "开始运行策略..." << std::endl;
        std::cout << "初始资金: " << std::fixed << std::setprecision(2) 
                  << account.get_balance() << std::endl;
        
        for (int i = params.slow_length; i < bars.size(); ++i) {
            const BarData& current_bar = bars[i];
            
            // 检查交易时间
            bool in_trading_hours = is_in_trading_hours(current_bar.datetime);
            
            // 非交易时间且有持仓则平仓
            if (!in_trading_hours && position != 0) {
                close_position(i);
                continue;
            }
            
            // 不在交易时间，跳过
            if (!in_trading_hours) {
                continue;
            }
            
            // 执行策略逻辑
            execute_strategy(i);
        }
        
        // 最后强制平仓
        if (position != 0) {
            close_position(bars.size() - 1);
        }
        
        std::cout << "策略运行完成" << std::endl;
        std::cout << "最终资金: " << std::fixed << std::setprecision(2) 
                  << account.get_balance() << std::endl;
        std::cout << "总交易次数: " << trades.size() << std::endl;
        
        // 输出交易记录
        output_trades();
    }
    
    // 执行策略逻辑
    void execute_strategy(int index) {
        const BarData& current_bar = bars[index];
        
        // 没有持仓时寻找入场信号
        if (position == 0) {
            // 突破快速高点做多
            if (current_bar.close > high_values[index - 1]) {
                int size = calculate_position_size(index);
                enter_position(true, size, current_bar.close, index);
            }
            // 突破快速低点做空
            else if (current_bar.close < low_values[index - 1]) {
                int size = calculate_position_size(index);
                enter_position(false, size, current_bar.close, index);
            }
        }
        // 有持仓时检查出场信号
        else {
            // 多头持仓
            if (position > 0) {
                // 多头止损：低于最近低点 - 2*ATR
                double stop_price = low_values[index - 1] - params.exit_multiplier * atr_values[index];
                if (current_bar.close < stop_price) {
                    close_position(index);
                }
            }
            // 空头持仓
            else {
                // 空头止损：高于最近高点 + 2*ATR
                double stop_price = high_values[index - 1] + params.exit_multiplier * atr_values[index];
                if (current_bar.close > stop_price) {
                    close_position(index);
                }
            }
        }
    }
    
    // 建立头寸
    void enter_position(bool is_buy, int size, double price, int index) {
        TradeRecord record;
        record.datetime = bars[index].datetime;
        
        if (account.execute_trade(is_buy, size, price, record)) {
            position = is_buy ? size : -size;
            entry_price = price;
            trades.push_back(record);
            
            struct tm* tm_info = localtime(&record.datetime);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
            
            std::cout << time_str << " 入场: " << (is_buy ? "买入" : "卖出") 
                      << " 价格: " << price << " 手数: " << size 
                      << " 手续费: " << std::fixed << std::setprecision(2) 
                      << record.commission << std::endl;
        }
    }
    
    // 平仓
    void close_position(int index) {
        if (position == 0) return;
        
        TradeRecord record;
        record.datetime = bars[index].datetime;
        bool is_buy = (position < 0); // 平仓方向与持仓相反
        int size = std::abs(position);
        
        if (account.execute_trade(is_buy, size, bars[index].close, record)) {
            trades.push_back(record);
            position = 0;
            
            struct tm* tm_info = localtime(&record.datetime);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
            
            std::cout << time_str << " 平仓: " << (is_buy ? "买入" : "卖出") 
                      << " 价格: " << bars[index].close << " 手数: " << size 
                      << " 手续费: " << std::fixed << std::setprecision(2) 
                      << record.commission << std::endl;
        }
    }
    
    // 输出交易记录
    void output_trades() {
        std::ofstream outfile("trades_record.csv");
        if (!outfile.is_open()) {
            std::cerr << "无法创建交易记录文件" << std::endl;
            return;
        }
        
        outfile << "时间,类型,价格,手数,手续费" << std::endl;
        
        for (const auto& trade : trades) {
            struct tm* tm_info = localtime(&trade.datetime);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
            
            outfile << time_str << ","
                    << (trade.is_buy ? "买入" : "卖出") << ","
                    << std::fixed << std::setprecision(2) << trade.price << ","
                    << trade.size << ","
                    << std::fixed << std::setprecision(2) << trade.commission << std::endl;
        }
        
        outfile.close();
        std::cout << "交易记录已保存至 trades_record.csv" << std::endl;
    }
};

int main() {
    // 设置策略参数
    StrategyParams params;
    params.fast_length = 20;    // 快速周期20分钟
    params.slow_length = 60;    // 慢速周期60分钟
    params.atr_length = 14;     // ATR周期14
    params.exit_multiplier = 2; // 2倍ATR止损
    
    // 创建交易账户：初始资金10万，保证金10%，手续费万分之一，合约乘数10
    TradingAccount account(100000.0, 0.1, 0.0001, 10);
    
    // 创建并运行策略
    IntradayTurtleStrategy strategy(params, account);
    
    // 加载数据（请替换为实际数据文件路径）
    if (strategy.load_data("rb_minute_data.csv")) {
        strategy.run();
    }
    
    return 0;
}
    