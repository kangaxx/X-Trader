#pragma once

#include "strategy.h"
#include "Logger.h"
#include <deque>
#include <vector>
#include <string>

// Bar数据结构，存储K线的基本信息
struct DTBarData {
    std::string date_str;   // 日期字符串
    time_t datetime;        // 时间戳
    double open;            // 开盘价
    double high;            // 最高价
    double low;             // 最低价
    double close;           // 收盘价
    int volume;             // 成交量
};

// 止盈类型枚举
enum class TakeProfitType {
    FixedPoint,     // 固定点数止盈
    Ratio,          // 固定比例止盈
    Volatility,     // 波动率止盈
    Trailing        // 移动止损转止盈
};

// Dual Thrust 策略类声明
class dual_thrust_trading_strategy : public strategy
{
public:
    /**
     * Dual Thrust 策略构造函数
     * @param id 策略ID
     * @param frame 框架引用
     * @param contract 合约代码
     * @param n 计算区间长度
     * @param k1 买入线系数
     * @param k2 卖出线系数
     * @param once_vol 每次下单手数
     * @param is_sim 是否为仿真模式
     * @param hist_file 历史数据文件
     * @param sim_start_date 仿真起始日期
     * @param base_days 仿真基准天数
     * @param end_time 收盘时间
     */
    dual_thrust_trading_strategy(stratid_t id, frame& frame, const std::string& contract,
        int n = 4, double k1 = 0.5, double k2 = 0.5, int once_vol = 1,
        bool is_sim = false, const std::string& hist_file = "",
        const std::string& sim_start_date = "", int base_days = 4,
        const std::string& end_time = "14:59");
    ~dual_thrust_trading_strategy() {}

    // Tick数据回调
    virtual void on_tick(const MarketData& tick) override;
    // 订单回报回调
    virtual void on_order(const Order& order) override;
    // 成交回报回调
    virtual void on_trade(const Order& order) override;
    // 撤单回报回调
    virtual void on_cancel(const Order& order) override;
    // 错误回报回调
    virtual void on_error(const Order& order) override;

private:
    // 仿真持仓结构体
    struct SimPosition {
        int long_pos = 0;        // 多头持仓
        int short_pos = 0;       // 空头持仓
        double long_entry = 0.0; // 多头开仓价
		double long_take_profit = 0.0; // 多头止盈价
        double short_entry = 0.0;// 空头开仓价
		double short_take_profit = 0.0; // 空头止盈价
        double profit = 0.0;     // 累计利润
    };

    // 处理K线bar数据
    void on_bar(const DTBarData& bar);
    // 仿真模式下强制平仓
    void force_close_sim(const DTBarData& bar);
    // 实盘模式下强制平仓
    void force_close_realtime(const DTBarData& bar);
    // 加载历史K线数据
    void load_history(const std::string& file);
	// 生成仿真基准区间（日K线）
	void generate_base_bars();
    // 仿真准备（分割基准区间和仿真区间）
    void prepare_simulation();
    // 仿真交互操作
    void simulation_interactive();

    std::string _contract;           // 合约代码
    int _n;                          // 区间长度
    double _k1, _k2;                 // Dual Thrust参数
    int _once_vol;                   // 每次下单手数
    bool _is_sim;                    // 是否仿真
    std::string _hist_file;          // 历史数据文件
    std::string _sim_start_date;     // 仿真起始日期
    int _base_days;                  // 仿真基准天数
    std::string _end_time;           // 收盘时间

    std::string _cur_minute;         // 当前分钟
    DTBarData _cur_bar{};            // 当前bar
	DTBarData _today_bar{};          // 今日日k线

    std::deque<DTBarData> _bar_history;      // bar历史队列
    std::vector<DTBarData> _history;         // 历史K线数据
    std::vector<DTBarData> _base_bars;       // 仿真基准区间
    std::vector<DTBarData> _sim_bars;        // 仿真区间
    size_t _bar_cursor = 0;                  // 仿真bar游标
    SimPosition _sim_pos;                    // 仿真持仓
    std::set<orderref_t> _buy_open_orders, _sell_open_orders; // 买开/卖开挂单引用
    bool _is_closing = false;                // 是否收盘强平标志
	double _range;                           // 价格区间
	double _sim_win_rate = 0.0;               // 仿真胜率
	double _sim_profit_loss_rate = 0.0;        // 仿真盈亏比
	int _sim_trades = 0;                     // 仿真总交易次数
    int _win_count = 0;
    double _total_win = 0.0, _total_loss = 0.0;

    TakeProfitType _take_profit_type = TakeProfitType::Volatility; // 止盈类型

    // 计算止盈数值
    // direction: 1=多头，-1=空头
    double calc_take_profit(double entry_price, double volatility, double current_price, double trailing_extreme, double fixed_value, double ratio_value, int direction);

    // 从基准区间计算波动率
    double calc_range_from_base_bars(const std::vector<DTBarData>& bars);
	// ATR移动止损算法
    double calc_atr_trailing_stop(const std::deque<DTBarData>& bars, int atr_period, double atr_mult,
        double entry_price, int direction, double cur_take_profit);
};