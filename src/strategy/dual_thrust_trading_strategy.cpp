#include "dual_thrust_trading_strategy.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <set>
#include <iostream> // ���������ڽ���
#include <regex>

/**
 * Dual Thrust ���Թ��캯��ʵ��
 * ��ʼ������������ģʽ�¼�����ʷ���ݲ�׼������
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
        generate_base_bars(); // ����������_base_bars��K��
	}
    if (_is_sim && !_hist_file.empty()) {
        prepare_simulation();
        simulation_interactive();
    }
}

/**
 * Tick���ݻص�
 * 1. �ж��Ƿ�����ǰһ���ӣ�����ǿƽ��־
 * 2. ����ģʽ���ƽ�bar�α겢����bar
 * 3. ʵ��ģʽ�°����Ӿۺ�bar������
 */
void dual_thrust_trading_strategy::on_tick(const MarketData& tick) {
    // ��_end_timeǰһ��������_is_closing��־
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
        // ʵ��ģʽ�����Ӿۺ�bar
        if (_cur_minute.empty() || tick_minute != _cur_minute) {
            if (!_cur_minute.empty()) {
                on_bar(_cur_bar);
                // ʵ��ģʽ�£����һ����ǿ��ƽ��
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

// �����ر��������ó�������������ʱ��������δ�ɽ��ҵ�
void dual_thrust_trading_strategy::on_order(const Order& order)
{
    std::string str = "DualThrust on_order triggered";
    Logger::get_instance().info(str);

    // ��¼��/�����ҵ�
    if (order.dir_offset == eDirOffset::BuyOpen) {
        _buy_open_orders.insert(order.order_ref);
    } else if (order.dir_offset == eDirOffset::SellOpen) {
        _sell_open_orders.insert(order.order_ref);
    }

    // ����ʱ��������δ�ɽ�����/�����ҵ�
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

// �ɽ��ر������ɽ�ʱ�ӹҵ��б��Ƴ����ã���¼�ɽ���־
void dual_thrust_trading_strategy::on_trade(const Order& order)
{
    // �ɽ�ʱ�Ƴ��ҵ�����
    if (order.dir_offset == eDirOffset::BuyOpen) {
        _buy_open_orders.erase(order.order_ref);
    } else if (order.dir_offset == eDirOffset::SellOpen) {
        _sell_open_orders.erase(order.order_ref);
    }

    // ��ȡ�����ַ���
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

    // ��־���ݣ�ʱ�䡢�۸񡢷�������
    std::ostringstream oss;
    oss << "DualThrust order filled: "
        << "time=" << order.insert_time
        << ", price=" << order.limit_price
        << ", dir=" << dir_str
        << ", volume=" << order.volume_traded;

    Logger::get_instance().info(oss.str());
}

// �����ر��������ùҵ����ã���¼������־
void dual_thrust_trading_strategy::on_cancel(const Order& order)
{
    // ��ȡ�����ַ���
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

// ����ر�������¼������־
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
 * bar���ݴ���
 * 1. ά��bar��ʷ����
 * 2. ����Dual Thrust������
 * 3. ��ϸ��������/ʵ�̵Ŀ�ƽ���߼�
 */
void dual_thrust_trading_strategy::on_bar(const DTBarData& bar) {
    // �ϲ�����K��Ϊ��K��
    std::string bar_day = bar.date_str.substr(0, 10); // "YYYY-MM-DD"
    std::string today_day = _today_bar.date_str.empty() ? "" : _today_bar.date_str.substr(0, 10);

    // ��ȡbar��ʱ��
    std::string bar_time = (bar.date_str.size() >= 16) ? bar.date_str.substr(11, 5) : "";
    // _end_time��ʽΪ"HH:MM"
    bool is_after_end_time = (!bar_time.empty() && bar_time >= _end_time);

    if (_today_bar.date_str.empty() || bar_day == today_day) {
        // ͬһ�죬�ϲ���_today_bar
        if (_today_bar.date_str.empty()) {
            _today_bar = bar;
            _today_bar.date_str = bar_day; // ֻ�������ڲ���
        } else {
            _today_bar.high = std::max(_today_bar.high, bar.high);
            _today_bar.low = std::min(_today_bar.low, bar.low);
            _today_bar.close = bar.close;
            _today_bar.volume += bar.volume;
            _today_bar.datetime = bar.datetime;
        }
    } else {
        // ���ڱ�����Ƚ���һ���_today_bar����_base_bars
        if (_base_bars.size() >= static_cast<size_t>(_base_days)) {
            _base_bars.erase(_base_bars.begin());
        }
        _base_bars.push_back(_today_bar);
        // ����_base_bars���ݼ���range
        if (_base_bars.size() < static_cast<size_t>(_n)) {
            Logger::get_instance().error("DualThrust: not enough base bars to calculate range");
            _range = 0.0;
            return;
        }
        _range = calc_range_from_base_bars(_base_bars);
        // �µ�һ�죬����_today_bar
        _today_bar = bar;
        _today_bar.date_str = bar_day;
    }

    // �������򳬹�����ʱ�䣬ֹͣ������ǿ��ƽ��
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

    // ����ģʽ����ϸ��־�����ڵ���
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

        // ��ͷ����
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
        // ��ͷ����
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
        // ��ͷֹӯ
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
        // ��ͷֹӯ
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
        // ʵ��ģʽ�¿�ƽ���߼�
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
 * ����ģʽ������ǿ��ƽ��
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
 * ʵ��ģʽ������ǿ��ƽ��
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
 * ������ʷK������
 * @param file ��ʷ�����ļ�·��
 */
void dual_thrust_trading_strategy::load_history(const std::string& file) {
    std::ifstream ifs(file);
    if (!ifs.is_open()) {
        Logger::get_instance().error("DualThrust: cannot open history file : " + file);
        return;
    }
    std::string line;
    std::getline(ifs, line); // ������ͷ
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

// ������������"YYYY-MM-DD hh:mm:ss"����ȡ"YYYYMMDD"
static std::string extract_date(const std::string& datetime_str) {
    // �����ʽ�ϸ�Ϊ"YYYY-MM-DD hh:mm:ss"
    if (datetime_str.size() >= 10) {
        // ȥ��'-'
        return datetime_str.substr(0, 4) + datetime_str.substr(5, 2) + datetime_str.substr(8, 2);
    }
    return datetime_str;
}

// ������˽�к���������base_bars����range
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

    // ��ӡrange��debug��־
    std::ostringstream oss_range;
    oss_range << "[BASE_RANGE] range=" << range << ", hh=" << hh << ", ll=" << ll << ", hc=" << hc << ", lc=" << lc;
    Logger::get_instance().debug(oss_range.str());

    return range;
}

void dual_thrust_trading_strategy::generate_base_bars() {
    // 1. �Ȱ����ھۺϣ��õ�ÿ�������յ����з���K��
    std::map<std::string, std::vector<DTBarData>> date_to_bars;
    for (const auto& bar : _history) {
        std::string bar_date = extract_date(bar.date_str);
        std::string sim_start = extract_date(_sim_start_date);
        if (bar_date < sim_start) {
            date_to_bars[bar_date].push_back(bar);
        }
    }
    // 2. ����������ȡ���_base_days��
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
        // ��ԭΪ"YYYY-MM-DD"��ʽ
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
        // ��¼���յĵ�һ������K�ߵ�datetimeΪ��K�ߵ�datetime
        daily.datetime = bars.front().datetime;

        // ��ӡdaily��Ϣ��debug��־
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

    // 3. ��_base_bars���ݼ���range
    if (_base_bars.size() < static_cast<size_t>(_n)) {
        Logger::get_instance().error("DualThrust: not enough base bars to calculate range");
        _range = 0.0;
        return;
    }
    _range = calc_range_from_base_bars(_base_bars);
}

/**
 * ����׼�����ָ��׼����ͷ�������
 * _base_bars �� generate_base_bars ����
 */
void dual_thrust_trading_strategy::prepare_simulation() {
    // ����������Ȼ��ԭʼ����K��
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
 * ����ģʽ�½�������
 * ֧�� e-�˳���r-���ܣ�f-�����ļ���������
 */
void dual_thrust_trading_strategy::simulation_interactive()
{
    while (true) {
        // ����ģʽ�ƽ�bar�α�
        while (_bar_cursor < _sim_bars.size()) {
            const DTBarData& bar = _sim_bars[_bar_cursor++];
            on_bar(bar);
        }

        // ���������Ӣ����ʾ�û�����
        std::cout << "Simulation finished. Enter 'e' to exit, 'r' to rerun, or 'f' to change file and rerun: ";
        std::string cmd;
        std::getline(std::cin, cmd);

        if (cmd == "e" || cmd == "E") {
            std::cout << "Simulation ended. Program will exit." << std::endl;
            exit(0);
        } else if (cmd == "r" || cmd == "R") {
            // ���·��棬�����ʷ���α�
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
            // ֻ�滻�ļ������֣����ı�·��
            auto pos = _hist_file.find_last_of("/\\");
            if (pos != std::string::npos) {
                _hist_file = _hist_file.substr(0, pos + 1) + new_file;
            } else {
                _hist_file = new_file;
            }
            // ���·��棬�����ʷ���α�
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

// ����ֹӯ��ֵ
// ����˵����
// entry_price      ���ּ�
// volatility       �����ʣ���ATR�ȣ�
// current_price    ��ǰ�۸�
// trailing_extreme �ֲ��ڼ���߼ۣ���ͷ������ͼۣ���ͷ��
// fixed_value      �̶�����
// ratio_value      �̶���������0.02��ʾ2%��
// direction        1=��ͷ��-1=��ͷ
// ����ֹӯ��λ
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
        // �̶�����ֹӯ����ͷ=���ּ�+��������ͷ=���ּ�-����
        return entry_price + direction * fixed_value;
    case TakeProfitType::Ratio:
        // �̶�����ֹӯ����ͷ=���ּ�*(1+����)����ͷ=���ּ�*(1-����)
        return entry_price * (1.0 + direction * ratio_value);
    case TakeProfitType::Volatility:
        // ������ֹӯ����ͷ=���ּ�+N*volatility����ͷ=���ּ�-N*volatility
        return entry_price + direction * fixed_value * volatility;
    case TakeProfitType::Trailing:
        // �ƶ�ֹ��תֹӯ����ͷ=trailing_max-�س�����ͷ=trailing_min+�س�
        return trailing_extreme - direction * fixed_value;
    default:
        return 0.0;
    }
}
