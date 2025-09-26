#include "simp_turtle_trading_strategy.h"
#include <sstream>
#include <deque>
#include "redis_client.h"
#include <jsoncpp/json/json.h>


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

double simp_turtle_trading_strategy::get_balance() {
    // 这里应该返回实际的账户余额
    return _balance; // 示例值
}

double simp_turtle_trading_strategy::calculate_commission(double price, int volume) {
    // 这里应该根据实际的手续费率计算手续费
    return price * volume * _commission_rate; // 示例计算
}

bool simp_turtle_trading_strategy::execute_trade(bool is_open, int size, double price, TradeRecord& record) {
    double required_margin = price * size * _contract_multiplier * _margin;
    if (is_open) {
        if (_balance < required_margin) {
            return false; // 余额不足
        }
        _balance -= required_margin;
    } else {
        _balance += required_margin; // 卖出释放保证金
    }
    double commission = calculate_commission(price, size);
    _balance -= commission; // 扣除手续费
    record.is_open = is_open;
    record.size = size;
    record.price = price;
    record.commission = commission;
    return true;
}

// 策略主逻辑：每个tick行情触发
void simp_turtle_trading_strategy::on_tick(const MarketData& tick)
{
    // 获取当前分钟字符串
    std::string tick_minute = std::string(tick.update_time).substr(0, 5); // "HH:MM"

    // 静态/成员变量维护BarData历史
    static std::deque<BarData> bar_history; // 用于ATR计算
    static double last_close = 0.0;

    // 判断是否为新分钟
    if (cur_minute.empty() || tick_minute != cur_minute) {
        // 如果不是第一分钟，先将上一分钟BarData存入Redis
        if (!cur_minute.empty()) {
            Json::Value bar_json;
            bar_json["datetime"] = (Json::Int64)cur_bar.datetime;
            bar_json["open"] = cur_bar.open;
            bar_json["high"] = cur_bar.high;
            bar_json["low"] = cur_bar.low;
            bar_json["close"] = cur_bar.close;
            bar_json["volume"] = cur_bar.volume;
            Json::StreamWriterBuilder builder;
            std::string bar_str = Json::writeString(builder, bar_json);

            std::string bar_key = "turtle:" + _contract + ":bar_n1";
            _redis.lpush(bar_key, bar_str);
            _redis.ltrim(bar_key, 0, _n1 - 1);

            // --- ATR计算与存储 ---
            // 1. 保存bar到历史
            bar_history.push_back(cur_bar);
            if (bar_history.size() > _n1) bar_history.pop_front();

            // 2. 计算TR
            std::vector<double> tr_list;
            for (size_t i = 1; i < bar_history.size(); ++i) {
                double high = bar_history[i].high;
                double low = bar_history[i].low;
                double prev_close = bar_history[i-1].close;
                double tr = std::max({high - low, std::abs(high - prev_close), std::abs(low - prev_close)});
                tr_list.push_back(tr);
            }

            // 3. 计算ATR（简单算术平均）
            double atr = 0.0;
            if (!tr_list.empty()) {
                double sum = 0.0;
                for (double v : tr_list) sum += v;
                atr = sum / tr_list.size();
            }

            // 4. 存储ATR到Redis队列
            std::string atr_key = "turtle:" + _contract + ":atr_n1";
            _redis.lpush(atr_key, std::to_string(atr));
            _redis.ltrim(atr_key, 0, _n1 - 1);
        }
        // 新分钟，重置BarData
        cur_bar.datetime = std::time(nullptr); // 或根据tick时间戳
        cur_bar.open = tick.last_price;
        cur_bar.high = tick.last_price;
        cur_bar.low = tick.last_price;
        cur_bar.close = tick.last_price;
        cur_bar.volume = tick.volume;
        cur_minute = tick_minute;
    } else {
        // 聚合当前tick到BarData
        cur_bar.high = std::max(cur_bar.high, tick.last_price);
        cur_bar.low = std::min(cur_bar.low, tick.last_price);
        cur_bar.close = tick.last_price;
        cur_bar.volume += tick.volume;
    }

    // 动态计算收盘前一分钟
    char close_time[6] = {0};
    int end_hour, end_minute;
    sscanf(_end_time.c_str(), "%d:%d", &end_hour, &end_minute);
    if (end_minute == 0) {
        end_hour -= 1;
        end_minute = 59;
    } else {
        end_minute -= 1;
    }
    snprintf(close_time, sizeof(close_time), "%02d:%02d", end_hour, end_minute);

    // 动态计算开盘时间
    char begin_time[6] = {0};
    int begin_hour, begin_minute;
    sscanf(_begin_time.c_str(), "%d:%d", &begin_hour, &begin_minute);
    snprintf(begin_time, sizeof(begin_time), "%02d:%02d", begin_hour, begin_minute);

    // 设置开盘标志
    if (strncmp(tick.update_time, begin_time, 5) == 0) { 
        _is_begin = true; 
        std::ostringstream oss;
        oss << "Strategy " << get_id() << " begin trading at " << tick.update_time;
        Logger::get_instance().info(oss.str());
    }

    // 设置收盘前一分钟平仓标志
    if (strncmp(tick.update_time, close_time, 5) == 0) { 
        _is_closing = true; 
        std::ostringstream oss;
        oss << "Strategy " << get_id() << " start closing positions at " << tick.update_time;
        Logger::get_instance().info(oss.str());
    }

    // 获取当前合约持仓信息
    const auto& posi = get_position(_contract);

    // 如果处于收盘状态，优先平掉所有可平仓位
    if (_is_closing)
    {
        if (posi.long_.closeable)
        {
            // 平多仓
            sell_close_today(eOrderFlag::Limit, _contract, tick.bid_price[0], posi.long_.closeable);
        }
        if (posi.short_.closeable)
        {
            // 平空仓
            buy_close_today(eOrderFlag::Limit, _contract, tick.ask_price[0], posi.short_.closeable);
        }
        return;
    }

    // --- 海龟交易策略核心逻辑 ---
    // 使用Redis维护历史最高价和最低价队列

    // Redis队列key建议带上合约名防止冲突
    std::string high_key = "turtle:" + _contract + ":high_n1";
    std::string low_key  = "turtle:" + _contract + ":low_n2";
	//Logger::get_instance().info("tick.lastprice is:" + std::to_string(tick.last_price));
    // 解析当前tick的分和秒
    std::string cur_min_sec = std::string(tick.update_time).substr(3, 5); // "mm:ss"
    bool do_lpush = false;
	//Logger::get_instance().info("cur_min_sec is: " + cur_min_sec + " and _last_lpush_time is: " + std::to_string(_last_lpush_time));
    if (_last_lpush_time == "00:00" || cur_min_sec != _last_lpush_time) {
        do_lpush = true;
        strcpy(_last_lpush_time, cur_min_sec.c_str());
    }

    if (do_lpush) {
        _redis.lpush(high_key, std::to_string(tick.last_price));
        _redis.lpush(low_key, std::to_string(tick.last_price));
    }

    // 保持队列长度不超过n1/n2
    _redis.ltrim(high_key, 0, _n1 - 1);
    _redis.ltrim(low_key, 0, _n2 - 1);

    // 获取队列所有元素
    std::vector<std::string> high_n1_strs = _redis.lrange(high_key, 0, _n1 - 1);
    std::vector<std::string> low_n2_strs  = _redis.lrange(low_key, 0, _n2 - 1);

    std::deque<double> high_n1, low_n2;
    for (const auto& s : high_n1_strs) {
        high_n1.push_back(std::stod(s));
    }
    for (const auto& s : low_n2_strs) {
        low_n2.push_back(std::stod(s));
    }

    // 计算突破信号
    double breakout_high = *std::max_element(high_n1.begin(), high_n1.end());
    double breakout_low = *std::min_element(low_n2.begin(), low_n2.end());

    // 开仓信号：突破_n1最高价做多，突破_n2最低价做空
    if (_buy_orderref == null_orderref && posi.long_.position + posi.long_.open_no_trade < _position_limit)
    {
        if (tick.last_price > breakout_high)
        {
            // 突破_n1最高价，开多仓
            _buy_orderref = buy_open(eOrderFlag::Limit, _contract, tick.ask_price[0], _once_vol);
            std::ostringstream oss;
            oss << "Turtle breakout long: price " << tick.last_price << " > high_n1 " << breakout_high;
            Logger::get_instance().info(oss.str());
        }
    }
    if (_sell_orderref == null_orderref && posi.short_.position + posi.short_.open_no_trade < _position_limit)
    {
        if (tick.last_price < breakout_low)
        {
            // 跌破_n2最低价，开空仓
            _sell_orderref = sell_open(eOrderFlag::Limit, _contract, tick.bid_price[0], _once_vol);
            std::ostringstream oss;
            oss << "Turtle breakout short: price " << tick.last_price << " < low_n2 " << breakout_low;
            Logger::get_instance().info(oss.str());
        }
    }

    // 平仓信号：回撤_n2最低价平多，回升_n1最高价平空
    if (posi.long_.closeable > 0 && tick.last_price < breakout_low)
    {
        // 多头止损
        sell_close_today(eOrderFlag::Limit, _contract, tick.bid_price[0], posi.long_.closeable);
        std::ostringstream oss;
        oss << "Turtle exit long: price " << tick.last_price << " < low_n2 " << breakout_low;
        Logger::get_instance().info(oss.str());
    }
    if (posi.short_.closeable > 0 && tick.last_price > breakout_high)
    {
        // 空头止损
        buy_close_today(eOrderFlag::Limit, _contract, tick.ask_price[0], posi.short_.closeable);
        std::ostringstream oss;
        oss << "Turtle exit short: price " << tick.last_price << " > high_n1 " << breakout_high;
        Logger::get_instance().info(oss.str());
    }
}

// 订单回报处理：设置撤单条件
void simp_turtle_trading_strategy::on_order(const Order& order)
{
    if (order.order_ref == _buy_orderref || order.order_ref == _sell_orderref)
    {
        set_cancel_condition(order.order_ref, [this](orderref_t order_id)->bool {
            // 收盘时直接撤单
            if (_is_closing) { return true; }
            return false;
        });
    }
}

// 成交回报处理：成交一边撤另一边挂单
void simp_turtle_trading_strategy::on_trade(const Order& order)
{
    if (_buy_orderref == order.order_ref)
    {
        // 买单成交，撤销卖单
        cancel_order(_sell_orderref);
        _buy_orderref = null_orderref;
    }
    if (_sell_orderref == order.order_ref)
    {
        // 卖单成交，撤销买单
        cancel_order(_buy_orderref);
        _sell_orderref = null_orderref;
    }
}

// 撤单回报处理：重置挂单引用
void simp_turtle_trading_strategy::on_cancel(const Order& order)
{
    if (_buy_orderref == order.order_ref) { _buy_orderref = null_orderref; }

    if (_sell_orderref == order.order_ref) { _sell_orderref = null_orderref; }
}

// 错误回报处理：重置挂单引用
void simp_turtle_trading_strategy::on_error(const Order& order)
{
    if (_buy_orderref == order.order_ref) { _buy_orderref = null_orderref; }

    if (_sell_orderref == order.order_ref) { _sell_orderref = null_orderref; }
}
