#include "dual_thrust_trading_strategy.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <set>

dual_thrust_trading_strategy::dual_thrust_trading_strategy(stratid_t id, frame& frame, const std::string& contract,
    int n, double k1, double k2, int once_vol, bool is_sim, const std::string& hist_file,
    const std::string& sim_start_date, int base_days, const std::string& end_time)
    : strategy(id, frame), _contract(contract), _n(n), _k1(k1), _k2(k2), _once_vol(once_vol),
      _is_sim(is_sim), _hist_file(hist_file), _sim_start_date(sim_start_date), _base_days(base_days), _end_time(end_time)
{
    get_contracts().insert(contract);
    if (_is_sim && !_hist_file.empty()) {
        load_history(_hist_file);
        prepare_simulation();
    }
}

void dual_thrust_trading_strategy::on_tick(const MarketData& tick) {
    // 设置收盘前一分钟平仓标志
    if (strncmp(tick.update_time, close_time, 5) == 0) {
        _is_closing = true;
        std::ostringstream oss;
        oss << "Strategy " << get_id() << " start closing positions at " << tick.update_time;
        Logger::get_instance().info(oss.str());
    }

    if (_is_sim) {
        if (_bar_cursor < _sim_bars.size()) {
            const DTBarData& bar = _sim_bars[_bar_cursor++];
            on_bar(bar);
            // 日内最后一分钟强制平仓
            if (_bar_cursor == _sim_bars.size()) {
                force_close_sim(bar);
            }
        }
    } else {
        std::string tick_minute = std::string(tick.update_time).substr(0, 5);
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

// 订单回报处理：设置撤单条件
void dual_thrust_trading_strategy::on_order(const Order& order)
{
    Logger::get_instance().info("DualThrust on_order triggered: order_ref={}", order.order_ref);

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

    // 设置撤单条件（原有逻辑）
    if (order.order_ref == _buy_orderref || order.order_ref == _sell_orderref)
    {
        set_cancel_condition(order.order_ref, [this](orderref_t order_id)->bool {
            if (_is_closing) { return true; }
            return false;
        });
    }
}

// 成交回报处理：成交时从列表移除挂单引用
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

// 撤单回报处理：重置挂单引用
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

// 错误回报处理：重置挂单引用
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

void dual_thrust_trading_strategy::on_bar(const DTBarData& bar) {
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

    double hh = ref_bars[0].high, ll = ref_bars[0].low;
    double hc = ref_bars[0].close, lc = ref_bars[0].close;
    for (size_t i = 1; i < ref_bars.size(); ++i) {
        hh = std::max(hh, ref_bars[i].high);
        ll = std::min(ll, ref_bars[i].low);
        hc = std::max(hc, ref_bars[i].close);
        lc = std::min(lc, ref_bars[i].close);
    }
    double range = std::max(hh - lc, hc - ll);
    double open = bar.open;
    double buy_line = open + _k1 * range;
    double sell_line = open - _k2 * range;

    if (_is_sim) {
        if (bar.close > buy_line && _sim_pos.long_pos == 0) {
            _sim_pos.long_pos = _once_vol;
            _sim_pos.long_entry = bar.close;
            Logger::get_instance().info("DualThrust SIM buy open, price={}, buy_line={}", bar.close, buy_line);
        }
        if (bar.close < sell_line && _sim_pos.short_pos == 0) {
            _sim_pos.short_pos = _once_vol;
            _sim_pos.short_entry = bar.close;
            Logger::get_instance().info("DualThrust SIM sell open, price={}, sell_line={}", bar.close, sell_line);
        }
        if (_sim_pos.long_pos > 0 && bar.close < sell_line) {
            double profit = (bar.close - _sim_pos.long_entry) * _sim_pos.long_pos;
            _sim_pos.profit += profit;
            Logger::get_instance().info("DualThrust SIM close long, price={}, sell_line={}, profit={}", bar.close, sell_line, profit);
            _sim_pos.long_pos = 0;
        }
        if (_sim_pos.short_pos > 0 && bar.close > buy_line) {
            double profit = (_sim_pos.short_entry - bar.close) * _sim_pos.short_pos;
            _sim_pos.profit += profit;
            Logger::get_instance().info("DualThrust SIM close short, price={}, buy_line={}, profit={}", bar.close, buy_line, profit);
            _sim_pos.short_pos = 0;
        }
    } else {
        const auto& posi = get_position(_contract);
        if (bar.close > buy_line && posi.long_.position == 0) {
            buy_open(eOrderFlag::Limit, _contract, bar.close, _once_vol);
            Logger::get_instance().info("DualThrust buy open, price={}, buy_line={}", bar.close, buy_line);
        }
        if (bar.close < sell_line && posi.short_.position == 0) {
            sell_open(eOrderFlag::Limit, _contract, bar.close, _once_vol);
            Logger::get_instance().info("DualThrust sell open, price={}, sell_line={}", bar.close, sell_line);
        }
        if (posi.long_.position > 0 && bar.close < sell_line) {
            sell_close(eOrderFlag::Limit, _contract, bar.close, posi.long_.position);
            Logger::get_instance().info("DualThrust close long, price={}, sell_line={}", bar.close, sell_line);
        }
        if (posi.short_.position > 0 && bar.close > buy_line) {
            buy_close(eOrderFlag::Limit, _contract, bar.close, posi.short_.position);
            Logger::get_instance().info("DualThrust close short, price={}, buy_line={}", bar.close, buy_line);
        }
    }
}

void dual_thrust_trading_strategy::force_close_sim(const DTBarData& bar) {
    double profit = 0.0;
    if (_sim_pos.long_pos > 0) {
        profit = (bar.close - _sim_pos.long_entry) * _sim_pos.long_pos;
        _sim_pos.profit += profit;
        Logger::get_instance().info("DualThrust SIM force close long, price={}, profit={}", bar.close, profit);
        _sim_pos.long_pos = 0;
    }
    if (_sim_pos.short_pos > 0) {
        profit = (_sim_pos.short_entry - bar.close) * _sim_pos.short_pos;
        _sim_pos.profit += profit;
        Logger::get_instance().info("DualThrust SIM force close short, price={}, profit={}", bar.close, profit);
        _sim_pos.short_pos = 0;
    }
    Logger::get_instance().info("DualThrust SIM total profit for the day: {}", _sim_pos.profit);
}

void dual_thrust_trading_strategy::force_close_realtime(const DTBarData& bar) {
    const auto& posi = get_position(_contract);
    if (posi.long_.position > 0) {
        sell_close(eOrderFlag::Limit, _contract, bar.close, posi.long_.position);
        Logger::get_instance().info("DualThrust REALTIME force close long, price={}, volume={}", bar.close, posi.long_.position);
    }
    if (posi.short_.position > 0) {
        buy_close(eOrderFlag::Limit, _contract, bar.close, posi.short_.position);
        Logger::get_instance().info("DualThrust REALTIME force close short, price={}, volume={}", bar.close, posi.short_.position);
    }
}

void dual_thrust_trading_strategy::load_history(const std::string& file) {
    std::ifstream ifs(file);
    if (!ifs.is_open()) {
        Logger::get_instance().error("DualThrust: cannot open history file {}", file);
        return;
    }
    std::string line;
    std::getline(ifs, line);
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
    Logger::get_instance().info("DualThrust: loaded {} bars from {}", _history.size(), file);
}

void dual_thrust_trading_strategy::prepare_simulation() {
    size_t start_idx = 0;
    for (; start_idx < _history.size(); ++start_idx) {
        if (_history[start_idx].date_str >= _sim_start_date) break;
    }
    if (start_idx < _base_days) {
        Logger::get_instance().error("DualThrust: not enough base days before sim_start_date");
        return;
    }
    _base_bars.assign(_history.begin() + (start_idx - _base_days), _history.begin() + start_idx);
    _sim_bars.assign(_history.begin() + start_idx, _history.end());
    _bar_cursor = 0;
    Logger::get_instance().info("DualThrust: simulation start at {}, base days: {}, sim bars: {}",
        _sim_start_date, _base_days, _sim_bars.size());
}