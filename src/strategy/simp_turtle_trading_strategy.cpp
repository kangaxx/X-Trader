#include "simp_turtle_trading_strategy.h"
#include <sstream>
#include <deque>
#include "redis_client.h"

// 策略主逻辑：每个tick行情触发
void simp_turtle_trading_strategy::on_tick(const MarketData& tick)
{
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
    RedisClient& redis = RedisClient::getInstance();
    if (!redis.connect("127.0.0.1", 6379)) {
        Logger::get_instance().error("can not connect to redis server 127.0.0.1:6379!");
        return;
    }

    // Redis队列key建议带上合约名防止冲突
    std::string high_key = "turtle:" + _contract + ":high_n1";
    std::string low_key  = "turtle:" + _contract + ":low_n2";

    // 推入最新价格
    redis.lpush(high_key, std::to_string(tick.highest_price));
    redis.lpush(low_key, std::to_string(tick.lowest_price));

    // 保持队列长度不超过n1/n2
    redis.ltrim(high_key, 0, _n1 - 1);
    redis.ltrim(low_key, 0, _n2 - 1);

    // 获取队列所有元素
    std::vector<std::string> high_n1_strs = redis.lrange(high_key, 0, _n1 - 1);
    std::vector<std::string> low_n2_strs  = redis.lrange(low_key, 0, _n2 - 1);

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
