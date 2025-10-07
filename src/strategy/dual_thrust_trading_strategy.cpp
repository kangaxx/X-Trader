#include "dual_thrust_trading_strategy.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <set>
#include <iostream> // 新增：用于交互
#include <regex>

/**
 * Dual Thrust 策略构造函数实现
 * 初始化参数，仿真模式下加载历史数据并准备仿真
 */
dual_thrust_trading_strategy::dual_thrust_trading_strategy(stratid_t id, frame& frame, const std::string& contract,
    int n, double k1, double k2, int once_vol, bool is_sim, const std::string& hist_file,
    const std::string& sim_start_date, int base_days, const std::string& end_time)
    : strategy(id, frame), _contract(contract), _n(n), _k1(k1), _k2(k2), _once_vol(once_vol),
      _is_sim(is_sim), _hist_file(hist_file), _sim_start_date(sim_start_date), _base_days(base_days), _end_time(end_time)
{
    get_contracts().insert(contract);
    if (!_hist_file.empty()) {
        load_history(_hist_file);
        generate_base_bars(); // 新增：生成_base_bars日K线
	}
    if (_is_sim && !_hist_file.empty()) {
        prepare_simulation();
        simulation_interactive();
    }
}

/**
 * Tick数据回调
 * 1. 判断是否到收盘前一分钟，设置强平标志
 * 2. 仿真模式下推进bar游标并处理bar
 * 3. 实盘模式下按分钟聚合bar并处理
 */
void dual_thrust_trading_strategy::on_tick(const MarketData& tick) {
    // 在_end_time前一分钟设置_is_closing标志
    std::string tick_minute = std::string(tick.update_time).substr(0, 5);
    int end_hour = 0, end_min = 0;
    sscanf(_end_time.c_str(), "%d:%d", &end_hour, &end_min);
    int close_hour = end_hour, close_min = end_min - 1;
    if (close_min < 0) {
        close_min += 60;
        close_hour -= 1;
    }
    char close_time[6];
    snprintf(close_time, sizeof(close_time), "%02d:%02d", close_hour, close_min);
    if (tick_minute == close_time) {
        _is_closing = true;
        std::ostringstream oss;
        oss << "Strategy " << get_id() << " start closing positions at " << tick.update_time
            << " (triggered by close_time=" << close_time << ")";
        Logger::get_instance().info(oss.str());
    }

    if (_is_sim) {
		Logger::get_instance().warn("DualThrust strategy is in simulation mode, on_tick will not process ticks.");
    } else {
        // 实盘模式按分钟聚合bar
        if (_cur_minute.empty() || tick_minute != _cur_minute) {
            if (!_cur_minute.empty()) {
                on_bar(_cur_bar);
                // 实盘模式下，最后一分钟强制平仓
                if (_cur_minute == _end_time) {
                    force_close_realtime(_cur_bar);
                }
            }
            _cur_bar.datetime = std::time(nullptr);
            _cur_bar.open = tick.last_price;
            _cur_bar.high = tick.last_price;
            _cur_bar.low = tick.last_price;
            _cur_bar.close = tick.last_price;
            _cur_bar.volume = tick.volume;
            _cur_minute = tick_minute;
        } else {
            _cur_bar.high = std::max(_cur_bar.high, tick.last_price);
            _cur_bar.low = std::min(_cur_bar.low, tick.last_price);
            _cur_bar.close = tick.last_price;
            _cur_bar.volume += tick.volume;
        }
    }
}

// 订单回报处理：设置撤单条件，收盘时撤销所有未成交挂单
void dual_thrust_trading_strategy::on_order(const Order& order)
{
    std::string str = "DualThrust on_order triggered";
    Logger::get_instance().info(str);

    // 记录买开/卖开挂单
    if (order.dir_offset == eDirOffset::BuyOpen) {
        _buy_open_orders.insert(order.order_ref);
    } else if (order.dir_offset == eDirOffset::SellOpen) {
        _sell_open_orders.insert(order.order_ref);
    }

    // 收盘时撤销所有未成交的买开/卖开挂单
    if (_is_closing) {
        for (auto order_ref : _buy_open_orders) {
            cancel_order(order_ref);
        }
        for (auto order_ref : _sell_open_orders) {
            cancel_order(order_ref);
        }
        _buy_open_orders.clear();
        _sell_open_orders.clear();
    }
}

// 成交回报处理：成交时从挂单列表移除引用，记录成交日志
void dual_thrust_trading_strategy::on_trade(const Order& order)
{
    // 成交时移除挂单引用
    if (order.dir_offset == eDirOffset::BuyOpen) {
        _buy_open_orders.erase(order.order_ref);
    } else if (order.dir_offset == eDirOffset::SellOpen) {
        _sell_open_orders.erase(order.order_ref);
    }

    // 获取方向字符串
    std::string dir_str;
    if (order.dir_offset == eDirOffset::BuyOpen || order.dir_offset == eDirOffset::BuyClose ||
        order.dir_offset == eDirOffset::BuyCloseToday || order.dir_offset == eDirOffset::BuyCloseYesterday) {
        dir_str = "Buy";
    } else if (order.dir_offset == eDirOffset::SellOpen || order.dir_offset == eDirOffset::SellClose ||
               order.dir_offset == eDirOffset::SellCloseToday || order.dir_offset == eDirOffset::SellCloseYesterday) {
        dir_str = "Sell";
    } else {
        dir_str = "Unknown";
    }

    // 日志内容：时间、价格、方向、手数
    std::ostringstream oss;
    oss << "DualThrust order filled: "
        << "time=" << order.insert_time
        << ", price=" << order.limit_price
        << ", dir=" << dir_str
        << ", volume=" << order.volume_traded;

    Logger::get_instance().info(oss.str());
}

// 撤单回报处理：重置挂单引用，记录撤单日志
void dual_thrust_trading_strategy::on_cancel(const Order& order)
{
    // 获取方向字符串
    std::string dir_str;
    if (order.dir_offset == eDirOffset::BuyOpen || order.dir_offset == eDirOffset::BuyClose ||
        order.dir_offset == eDirOffset::BuyCloseToday || order.dir_offset == eDirOffset::BuyCloseYesterday) {
        dir_str = "Buy";
    } else if (order.dir_offset == eDirOffset::SellOpen || order.dir_offset == eDirOffset::SellClose ||
               order.dir_offset == eDirOffset::SellCloseToday || order.dir_offset == eDirOffset::SellCloseYesterday) {
        dir_str = "Sell";
    } else {
        dir_str = "Unknown";
    }

    std::ostringstream oss;
    oss << "DualThrust order canceled: "
        << "time=" << order.cancel_time
        << ", price=" << order.limit_price
        << ", dir=" << dir_str
        << ", volume=" << order.volume_total_original
        << ", order_ref=" << order.order_ref;
    Logger::get_instance().info(oss.str());
}

// 错误回报处理：记录错误日志
void dual_thrust_trading_strategy::on_error(const Order& order)
{
    std::ostringstream oss;
    oss << "DualThrust order error: "
        << "time=" << order.insert_time
        << ", price=" << order.limit_price
        << ", volume=" << order.volume_total_original
        << ", error_id=" << order.error_id
        << ", error_msg=" << order.error_msg;
    Logger::get_instance().error(oss.str());
}

/**
 * bar数据处理
 * 1. 维护bar历史队列
 * 2. 计算Dual Thrust买卖线
 * 3. 仔细分析仿真/实盘的开平仓逻辑
 */
void dual_thrust_trading_strategy::on_bar(const DTBarData& bar) {
    // 合并分钟K线为日K线
    std::string bar_day = bar.date_str.substr(0, 10); // "YYYY-MM-DD"
    std::string today_day = _today_bar.date_str.empty() ? "" : _today_bar.date_str.substr(0, 10);

    // 提取bar的时分
    std::string bar_time = (bar.date_str.size() >= 16) ? bar.date_str.substr(11, 5) : "";
    // _end_time格式为"HH:MM"
    bool is_after_end_time = (!bar_time.empty() && bar_time >= _end_time);

    if (_today_bar.date_str.empty() || bar_day == today_day) {
        // 同一天，合并到_today_bar
        if (_today_bar.date_str.empty()) {
            _today_bar = bar;
            _today_bar.date_str = bar_day; // 只保留日期部分
        } else {
            _today_bar.high = std::max(_today_bar.high, bar.high);
            _today_bar.low = std::min(_today_bar.low, bar.low);
            _today_bar.close = bar.close;
            _today_bar.volume += bar.volume;
            _today_bar.datetime = bar.datetime;
        }
    } else {
        // 日期变更，先将上一天的_today_bar加入_base_bars
        if (_base_bars.size() >= static_cast<size_t>(_base_days)) {
            _base_bars.erase(_base_bars.begin());
        }
        _base_bars.push_back(_today_bar);
        // 用新_base_bars数据计算range
        if (_base_bars.size() < static_cast<size_t>(_n)) {
            Logger::get_instance().error("DualThrust: not enough base bars to calculate range");
            _range = 0.0;
            return;
        }
        _range = calc_range_from_base_bars(_base_bars);
        // 新的一天，重置_today_bar
        _today_bar = bar;
        _today_bar.date_str = bar_day;
    }

    // 如果到达或超过收盘时间，停止开单并强制平仓
    if (is_after_end_time) {
        force_close_sim(bar);
        return;
    }

    _bar_history.push_back(bar);
    if (_bar_history.size() > _n + 1) _bar_history.pop_front();

    std::deque<DTBarData> ref_bars;
    if (_is_sim && _bar_cursor <= 1) {
        ref_bars.assign(_base_bars.begin(), _base_bars.end());
    } else {
        if (_bar_history.size() < _n + 1) return;
        for (int i = 0; i < _n; ++i) {
            ref_bars.push_back(_bar_history[i]);
        }
    }
    if (ref_bars.empty()) return;

    double buy_line = _today_bar.open + _k1 * _range;
    double sell_line = _today_bar.open - _k2 * _range;

    // 仿真模式下详细日志，便于调试
    if (_is_sim) {
        std::ostringstream oss_bar_info;
        oss_bar_info << "[SIM] Bar: date=" << bar.date_str
            << ", open=" << bar.open
            << ", high=" << bar.high
            << ", low=" << bar.low
            << ", close=" << bar.close
            << ", volume=" << bar.volume
            << ", k1=" << _k1
            << ", k2=" << _k2
            << ", range=" << _range
            << ", buy_line=" << buy_line
            << ", sell_line=" << sell_line
            << ", long_pos=" << _sim_pos.long_pos
            << ", short_pos=" << _sim_pos.short_pos
            << ", profit=" << _sim_pos.profit;
        Logger::get_instance().debug(oss_bar_info.str());

        // 多头开仓
        if (bar.close > buy_line && _sim_pos.long_pos == 0) {
            _sim_pos.long_pos = _once_vol;
            _sim_pos.long_entry = bar.close;
            _sim_pos.long_take_profit = calc_take_profit(
                _sim_pos.long_entry, _range, bar.close, bar.high, _range * 1.5, 0.02, 1
            );
            std::ostringstream oss2;
            oss2 << "[SIM] BUY OPEN: date=" << bar.date_str
                 << ", price=" << bar.close
                 << ", buy_line=" << buy_line
                 << ", long_pos=" << _sim_pos.long_pos
                 << ", take_profit=" << _sim_pos.long_take_profit;
            Logger::get_instance().info(oss2.str());
        }
        // 空头开仓
        if (bar.close < sell_line && _sim_pos.short_pos == 0) {
            _sim_pos.short_pos = _once_vol;
            _sim_pos.short_entry = bar.close;
            _sim_pos.short_take_profit = calc_take_profit(
                _sim_pos.short_entry, _range, bar.close, bar.low, _range * 1.5, 0.02, -1
            );
            std::ostringstream oss2;
            oss2 << "[SIM] SELL OPEN: date=" << bar.date_str
                 << ", price=" << bar.close
                 << ", sell_line=" << sell_line
                 << ", short_pos=" << _sim_pos.short_pos
                 << ", take_profit=" << _sim_pos.short_take_profit;
            Logger::get_instance().info(oss2.str());
        }
        // 多头止盈
        if (_sim_pos.long_pos > 0) {
            if (bar.close < sell_line) {
                double profit = (bar.close - _sim_pos.long_entry) * _sim_pos.long_pos;
                _sim_pos.profit += profit;
                std::ostringstream oss2;
                oss2 << "[SIM] CLOSE LONG: date=" << bar.date_str
                     << ", price=" << bar.close
                     << ", entry=" << _sim_pos.long_entry
                     << ", sell_line=" << sell_line
                     << ", profit=" << profit
                     << ", total_profit=" << _sim_pos.profit;
                Logger::get_instance().info(oss2.str());
                _sim_pos.long_pos = 0;
            } else if (bar.close >= _sim_pos.long_take_profit) {
                double profit = (_sim_pos.long_take_profit - _sim_pos.long_entry) * _sim_pos.long_pos;
                _sim_pos.profit += profit;
                std::ostringstream oss2;
                oss2 << "[SIM] TAKE PROFIT LONG: date=" << bar.date_str
                     << ", price=" << bar.close
                     << ", entry=" << _sim_pos.long_entry
                     << ", take_profit=" << _sim_pos.long_take_profit
                     << ", profit=" << profit
                     << ", total_profit=" << _sim_pos.profit;
                Logger::get_instance().info(oss2.str());
                _sim_pos.long_pos = 0;
            }
        }
        // 空头止盈
        if (_sim_pos.short_pos > 0) {
            if (bar.close > buy_line) {
                double profit = (_sim_pos.short_entry - bar.close) * _sim_pos.short_pos;
                _sim_pos.profit += profit;
                std::ostringstream oss2;
                oss2 << "[SIM] CLOSE SHORT: date=" << bar.date_str
                     << ", price=" << bar.close
                     << ", entry=" << _sim_pos.short_entry
                     << ", buy_line=" << buy_line
                     << ", profit=" << profit
                     << ", total_profit=" << _sim_pos.profit;
                Logger::get_instance().info(oss2.str());
                _sim_pos.short_pos = 0;
            } else if (bar.close <= _sim_pos.short_take_profit) {
                double profit = (_sim_pos.short_entry - _sim_pos.short_take_profit) * _sim_pos.short_pos;
                _sim_pos.profit += profit;
                std::ostringstream oss2;
                oss2 << "[SIM] TAKE PROFIT SHORT: date=" << bar.date_str
                     << ", price=" << bar.close
                     << ", entry=" << _sim_pos.short_entry
                     << ", take_profit=" << _sim_pos.short_take_profit
                     << ", profit=" << profit
                     << ", total_profit=" << _sim_pos.profit;
                Logger::get_instance().info(oss2.str());
                _sim_pos.short_pos = 0;
            }
        }
    } else {
        // 实盘模式下开平仓逻辑
        const auto& posi = get_position(_contract);
        if (bar.close > buy_line && posi.long_.position == 0) {
            buy_open(eOrderFlag::Limit, _contract, bar.close, _once_vol);
            std::ostringstream oss;
            oss << "DualThrust buy open, price=" << bar.close << ", buy_line=" << buy_line;
            Logger::get_instance().info(oss.str());
        }
        if (bar.close < sell_line && posi.short_.position == 0) {
            sell_open(eOrderFlag::Limit, _contract, bar.close, _once_vol);
            std::ostringstream oss;
            oss << "DualThrust sell open, price=" << bar.close << ", sell_line=" << sell_line;
            Logger::get_instance().info(oss.str());
        }
        if (posi.long_.position > 0 && bar.close < sell_line) {
            sell_close(eOrderFlag::Limit, _contract, bar.close, posi.long_.position);
            std::ostringstream oss;
            oss << "DualThrust close long, price=" << bar.close << ", sell_line=" << sell_line;
            Logger::get_instance().info(oss.str());
        }
        if (posi.short_.position > 0 && bar.close > buy_line) {
            buy_close(eOrderFlag::Limit, _contract, bar.close, posi.short_.position);
            std::ostringstream oss;
            oss << "DualThrust close short, price=" << bar.close << ", buy_line=" << buy_line;
            Logger::get_instance().info(oss.str());
        }
    }
}

/**
 * 仿真模式下收盘强制平仓
 */
void dual_thrust_trading_strategy::force_close_sim(const DTBarData& bar) {
    double profit = 0.0;
    if (_sim_pos.long_pos > 0) {
        profit = (bar.close - _sim_pos.long_entry) * _sim_pos.long_pos;
        _sim_pos.profit += profit;
        std::ostringstream oss;
        oss << "DualThrust SIM force close long, price=" << bar.close << ", profit=" << profit;
        Logger::get_instance().info(oss.str());
        _sim_pos.long_pos = 0;
    }
    if (_sim_pos.short_pos > 0) {
        profit = (_sim_pos.short_entry - bar.close) * _sim_pos.short_pos;
        _sim_pos.profit += profit;
        std::ostringstream oss;
        oss << "DualThrust SIM force close short, price=" << bar.close << ", profit=" << profit;
        Logger::get_instance().info(oss.str());
        _sim_pos.short_pos = 0;
    }
    std::ostringstream oss;
    oss << "DualThrust SIM total profit for the day: " << _sim_pos.profit;
    Logger::get_instance().info(oss.str());
}

/**
 * 实盘模式下收盘强制平仓
 */
void dual_thrust_trading_strategy::force_close_realtime(const DTBarData& bar) {
    const auto& posi = get_position(_contract);
    if (posi.long_.position > 0) {
        sell_close(eOrderFlag::Limit, _contract, bar.close, posi.long_.position);
        std::ostringstream oss;
        oss << "DualThrust REALTIME force close long, price=" << bar.close << ", volume=" << posi.long_.position;
        Logger::get_instance().info(oss.str());
    }
    if (posi.short_.position > 0) {
        buy_close(eOrderFlag::Limit, _contract, bar.close, posi.short_.position);
        std::ostringstream oss;
        oss << "DualThrust REALTIME force close short, price=" << bar.close << ", volume=" << posi.short_.position;
        Logger::get_instance().info(oss.str());
    }
}

/**
 * 加载历史K线数据
 * @param file 历史数据文件路径
 */
void dual_thrust_trading_strategy::load_history(const std::string& file) {
    std::ifstream ifs(file);
    if (!ifs.is_open()) {
        Logger::get_instance().error("DualThrust: cannot open history file : " + file);
        return;
    }
    std::string line;
    std::getline(ifs, line); // 跳过表头
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string token;
        DTBarData bar{};
        std::getline(iss, token, ','); bar.date_str = token;
        bar.datetime = 0;
        std::getline(iss, token, ','); bar.open = std::stod(token);
        std::getline(iss, token, ','); bar.high = std::stod(token);
        std::getline(iss, token, ','); bar.low = std::stod(token);
        std::getline(iss, token, ','); bar.close = std::stod(token);
        std::getline(iss, token, ','); bar.volume = std::stoi(token);
        _history.push_back(bar);
    }
    std::ostringstream oss;
    oss << "DualThrust: loaded " << _history.size() << " bars from " << file;
    Logger::get_instance().info(oss.str());
}

// 辅助函数：从"YYYY-MM-DD hh:mm:ss"中提取"YYYYMMDD"
static std::string extract_date(const std::string& datetime_str) {
    // 假设格式严格为"YYYY-MM-DD hh:mm:ss"
    if (datetime_str.size() >= 10) {
        // 去掉'-'
        return datetime_str.substr(0, 4) + datetime_str.substr(5, 2) + datetime_str.substr(8, 2);
    }
    return datetime_str;
}

// 新增：私有函数，基于base_bars计算range
double dual_thrust_trading_strategy::calc_range_from_base_bars(const std::vector<DTBarData>& bars) {
    if (bars.empty()) return 0.0;
    double hh = bars[0].high, ll = bars[0].low;
    double hc = bars[0].close, lc = bars[0].close;
    for (size_t i = 1; i < bars.size(); ++i) {
        hh = std::max(hh, bars[i].high);
        ll = std::min(ll, bars[i].low);
        hc = std::max(hc, bars[i].close);
        lc = std::min(lc, bars[i].close);
    }
    double range = std::max(hh - lc, hc - ll);

    // 打印range到debug日志
    std::ostringstream oss_range;
    oss_range << "[BASE_RANGE] range=" << range << ", hh=" << hh << ", ll=" << ll << ", hc=" << hc << ", lc=" << lc;
    Logger::get_instance().debug(oss_range.str());

    return range;
}

void dual_thrust_trading_strategy::generate_base_bars() {
    // 1. 先按日期聚合，得到每个交易日的所有分钟K线
    std::map<std::string, std::vector<DTBarData>> date_to_bars;
    for (const auto& bar : _history) {
        std::string bar_date = extract_date(bar.date_str);
        std::string sim_start = extract_date(_sim_start_date);
        if (bar_date < sim_start) {
            date_to_bars[bar_date].push_back(bar);
        }
    }
    // 2. 按日期排序，取最后_base_days天
    std::vector<std::string> all_dates;
    for (const auto& kv : date_to_bars) {
        all_dates.push_back(kv.first);
    }
    std::sort(all_dates.begin(), all_dates.end());
    if (all_dates.size() < static_cast<size_t>(_base_days)) {
        Logger::get_instance().error("DualThrust: not enough base days before sim_start_date");
        return;
    }
    _base_bars.clear();
    for (size_t i = all_dates.size() - _base_days; i < all_dates.size(); ++i) {
        const auto& bars = date_to_bars[all_dates[i]];
        if (bars.empty()) continue;
        DTBarData daily{};
        // 还原为"YYYY-MM-DD"格式
        daily.date_str = all_dates[i].substr(0, 4) + "-" + all_dates[i].substr(4, 2) + "-" + all_dates[i].substr(6, 2);
        daily.open = bars.front().open;
        daily.high = bars.front().high;
        daily.low = bars.front().low;
        daily.close = bars.back().close;
        daily.volume = 0;
        for (const auto& b : bars) {
            daily.high = std::max(daily.high, b.high);
            daily.low = std::min(daily.low, b.low);
            daily.volume += b.volume;
        }
        // 记录该日的第一个分钟K线的datetime为日K线的datetime
        daily.datetime = bars.front().datetime;

        // 打印daily信息到debug日志
        std::ostringstream oss;
        oss << "[BASE_DAILY] date=" << daily.date_str
            << ", open=" << daily.open
            << ", high=" << daily.high
            << ", low=" << daily.low
            << ", close=" << daily.close
            << ", volume=" << daily.volume
            << ", datetime=" << daily.datetime;
        Logger::get_instance().debug(oss.str());

        _base_bars.push_back(daily);
    }

    // 3. 用_base_bars数据计算range
    if (_base_bars.size() < static_cast<size_t>(_n)) {
        Logger::get_instance().error("DualThrust: not enough base bars to calculate range");
        _range = 0.0;
        return;
    }
    _range = calc_range_from_base_bars(_base_bars);
}

/**
 * 仿真准备，分割基准区间和仿真区间
 * _base_bars 由 generate_base_bars 生成
 */
void dual_thrust_trading_strategy::prepare_simulation() {
    // 仿真区间依然用原始分钟K线
    size_t sim_start_idx = 0;
    for (; sim_start_idx < _history.size(); ++sim_start_idx) {
        if (_history[sim_start_idx].date_str >= _sim_start_date) break;
    }
    _sim_bars.assign(_history.begin() + sim_start_idx, _history.end());
    _bar_cursor = 0;
    std::ostringstream oss;
    oss << "DualThrust: simulation start at " << _sim_start_date
        << ", base days: " << _base_days
        << ", sim bars: " << _sim_bars.size();
    Logger::get_instance().info(oss.str());
}

/**
 * 仿真模式下交互操作
 * 支持 e-退出，r-重跑，f-更换文件名后重跑
 */
void dual_thrust_trading_strategy::simulation_interactive()
{
    while (true) {
        // 仿真模式推进bar游标
        while (_bar_cursor < _sim_bars.size()) {
            const DTBarData& bar = _sim_bars[_bar_cursor++];
            on_bar(bar);
        }

        // 仿真结束后，英文提示用户操作
        std::cout << "Simulation finished. Enter 'e' to exit, 'r' to rerun, or 'f' to change file and rerun: ";
        std::string cmd;
        std::getline(std::cin, cmd);

        if (cmd == "e" || cmd == "E") {
            std::cout << "Simulation ended. Program will exit." << std::endl;
            exit(0);
        } else if (cmd == "r" || cmd == "R") {
            // 重新仿真，清空历史和游标
            _history.clear();
            _base_bars.clear();
            _sim_bars.clear();
            _bar_cursor = 0;
            _sim_pos = SimPosition();
            load_history(_hist_file);
            prepare_simulation();
            continue;
        } else if (cmd == "f" || cmd == "F") {
            std::cout << "Please input new file name (file name only, no path): ";
            std::string new_file;
            std::getline(std::cin, new_file);
            // 只替换文件名部分，不改变路径
            auto pos = _hist_file.find_last_of("/\\");
            if (pos != std::string::npos) {
                _hist_file = _hist_file.substr(0, pos + 1) + new_file;
            } else {
                _hist_file = new_file;
            }
            // 重新仿真，清空历史和游标
            _history.clear();
            _base_bars.clear();
            _sim_bars.clear();
            _bar_cursor = 0;
            _sim_pos = SimPosition();
            load_history(_hist_file);
            prepare_simulation();
            continue;
        } else {
            std::cout << "Invalid input, please try again." << std::endl;
        }
    }
}

// 计算止盈数值
// 参数说明：
// entry_price      开仓价
// volatility       波动率（如ATR等）
// current_price    当前价格
// trailing_extreme 持仓期间最高价（多头）或最低价（空头）
// fixed_value      固定点数
// ratio_value      固定比例（如0.02表示2%）
// direction        1=多头，-1=空头
// 返回止盈价位
double dual_thrust_trading_strategy::calc_take_profit(
    double entry_price,
    double volatility,
    double current_price,
    double trailing_extreme,
    double fixed_value,
    double ratio_value,
    int direction)
{
    switch (_take_profit_type) {
    case TakeProfitType::FixedPoint:
        // 固定点数止盈：多头=开仓价+点数，空头=开仓价-点数
        return entry_price + direction * fixed_value;
    case TakeProfitType::Ratio:
        // 固定比例止盈：多头=开仓价*(1+比例)，空头=开仓价*(1-比例)
        return entry_price * (1.0 + direction * ratio_value);
    case TakeProfitType::Volatility:
        // 波动率止盈：多头=开仓价+N*volatility，空头=开仓价-N*volatility
        return entry_price + direction * fixed_value * volatility;
    case TakeProfitType::Trailing:
        // 移动止损转止盈：多头=trailing_max-回撤，空头=trailing_min+回撤
        return trailing_extreme - direction * fixed_value;
    default:
        return 0.0;
    }
}
