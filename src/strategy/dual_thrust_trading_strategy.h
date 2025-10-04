#pragma once

#include "strategy.h"
#include "Logger.h"
#include <deque>
#include <vector>
#include <string>

// Bar数据结构
struct DTBarData {
    std::string date_str;
    time_t datetime;
    double open;
    double high;
    double low;
    double close;
    int volume;
};

// Dual Thrust 策略类声明
class dual_thrust_trading_strategy : public strategy
{
public:
    dual_thrust_trading_strategy(stratid_t id, frame& frame, const std::string& contract,
        int n = 4, double k1 = 0.5, double k2 = 0.5, int once_vol = 1,
        bool is_sim = false, const std::string& hist_file = "",
        const std::string& sim_start_date = "", int base_days = 4,
        const std::string& end_time = "14:59");
	~dual_thrust_trading_strategy() {}
    virtual void on_tick(const MarketData& tick) override;
    virtual void on_order(const Order& order) override;
    virtual void on_trade(const Order& order) override;
    virtual void on_cancel(const Order& order) override;
    virtual void on_error(const Order& order) override;

private:
    struct SimPosition {
        int long_pos = 0;
        int short_pos = 0;
        double long_entry = 0.0;
        double short_entry = 0.0;
        double profit = 0.0;
    };

    void on_bar(const DTBarData& bar);
    void force_close_sim(const DTBarData& bar);
    void force_close_realtime(const DTBarData& bar);
    void load_history(const std::string& file);
    void prepare_simulation();

    std::string _contract;
    int _n;
    double _k1, _k2;
    int _once_vol;
    bool _is_sim;
    std::string _hist_file;
    std::string _sim_start_date;
    int _base_days;
    std::string _end_time;

    std::string _cur_minute;
    DTBarData _cur_bar{};

    std::deque<DTBarData> _bar_history;
    std::vector<DTBarData> _history;
    std::vector<DTBarData> _base_bars;
    std::vector<DTBarData> _sim_bars;
    size_t _bar_cursor = 0;
    SimPosition _sim_pos;
    std::set<orderref_t> _buy_open_orders, _sell_open_orders;
    bool _is_closing = false;
};