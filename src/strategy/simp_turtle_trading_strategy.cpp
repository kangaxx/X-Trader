#include "simp_turtle_trading_strategy.h"
#include <sstream>
#include <deque>
#include "redis_client.h"

// �������߼���ÿ��tick���鴥��
void simp_turtle_trading_strategy::on_tick(const MarketData& tick)
{
    // ��̬��������ǰһ����
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

    // ��̬���㿪��ʱ��
    char begin_time[6] = {0};
    int begin_hour, begin_minute;
    sscanf(_begin_time.c_str(), "%d:%d", &begin_hour, &begin_minute);
    snprintf(begin_time, sizeof(begin_time), "%02d:%02d", begin_hour, begin_minute);

    // ���ÿ��̱�־
    if (strncmp(tick.update_time, begin_time, 5) == 0) { 
        _is_begin = true; 
        std::ostringstream oss;
        oss << "Strategy " << get_id() << " begin trading at " << tick.update_time;
        Logger::get_instance().info(oss.str());
    }

    // ��������ǰһ����ƽ�ֱ�־
    if (strncmp(tick.update_time, close_time, 5) == 0) { 
        _is_closing = true; 
        std::ostringstream oss;
        oss << "Strategy " << get_id() << " start closing positions at " << tick.update_time;
        Logger::get_instance().info(oss.str());
    }

    // ��ȡ��ǰ��Լ�ֲ���Ϣ
    const auto& posi = get_position(_contract);

    // �����������״̬������ƽ�����п�ƽ��λ
    if (_is_closing)
    {
        if (posi.long_.closeable)
        {
            // ƽ���
            sell_close_today(eOrderFlag::Limit, _contract, tick.bid_price[0], posi.long_.closeable);
        }
        if (posi.short_.closeable)
        {
            // ƽ�ղ�
            buy_close_today(eOrderFlag::Limit, _contract, tick.ask_price[0], posi.short_.closeable);
        }
        return;
    }

    // --- ���꽻�ײ��Ժ����߼� ---
    // ʹ��Redisά����ʷ��߼ۺ���ͼ۶���
    RedisClient& redis = RedisClient::getInstance();
    if (!redis.connect("127.0.0.1", 6379)) {
        Logger::get_instance().error("can not connect to redis server 127.0.0.1:6379!");
        return;
    }

    // Redis����key������Ϻ�Լ����ֹ��ͻ
    std::string high_key = "turtle:" + _contract + ":high_n1";
    std::string low_key  = "turtle:" + _contract + ":low_n2";

    // �������¼۸�
    redis.lpush(high_key, std::to_string(tick.highest_price));
    redis.lpush(low_key, std::to_string(tick.lowest_price));

    // ���ֶ��г��Ȳ�����n1/n2
    redis.ltrim(high_key, 0, _n1 - 1);
    redis.ltrim(low_key, 0, _n2 - 1);

    // ��ȡ��������Ԫ��
    std::vector<std::string> high_n1_strs = redis.lrange(high_key, 0, _n1 - 1);
    std::vector<std::string> low_n2_strs  = redis.lrange(low_key, 0, _n2 - 1);

    std::deque<double> high_n1, low_n2;
    for (const auto& s : high_n1_strs) {
        high_n1.push_back(std::stod(s));
    }
    for (const auto& s : low_n2_strs) {
        low_n2.push_back(std::stod(s));
    }

    // ����ͻ���ź�
    double breakout_high = *std::max_element(high_n1.begin(), high_n1.end());
    double breakout_low = *std::min_element(low_n2.begin(), low_n2.end());

    // �����źţ�ͻ��_n1��߼����࣬ͻ��_n2��ͼ�����
    if (_buy_orderref == null_orderref && posi.long_.position + posi.long_.open_no_trade < _position_limit)
    {
        if (tick.last_price > breakout_high)
        {
            // ͻ��_n1��߼ۣ������
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
            // ����_n2��ͼۣ����ղ�
            _sell_orderref = sell_open(eOrderFlag::Limit, _contract, tick.bid_price[0], _once_vol);
            std::ostringstream oss;
            oss << "Turtle breakout short: price " << tick.last_price << " < low_n2 " << breakout_low;
            Logger::get_instance().info(oss.str());
        }
    }

    // ƽ���źţ��س�_n2��ͼ�ƽ�࣬����_n1��߼�ƽ��
    if (posi.long_.closeable > 0 && tick.last_price < breakout_low)
    {
        // ��ͷֹ��
        sell_close_today(eOrderFlag::Limit, _contract, tick.bid_price[0], posi.long_.closeable);
        std::ostringstream oss;
        oss << "Turtle exit long: price " << tick.last_price << " < low_n2 " << breakout_low;
        Logger::get_instance().info(oss.str());
    }
    if (posi.short_.closeable > 0 && tick.last_price > breakout_high)
    {
        // ��ͷֹ��
        buy_close_today(eOrderFlag::Limit, _contract, tick.ask_price[0], posi.short_.closeable);
        std::ostringstream oss;
        oss << "Turtle exit short: price " << tick.last_price << " > high_n1 " << breakout_high;
        Logger::get_instance().info(oss.str());
    }
}

// �����ر��������ó�������
void simp_turtle_trading_strategy::on_order(const Order& order)
{
    if (order.order_ref == _buy_orderref || order.order_ref == _sell_orderref)
    {
        set_cancel_condition(order.order_ref, [this](orderref_t order_id)->bool {
            // ����ʱֱ�ӳ���
            if (_is_closing) { return true; }
            return false;
        });
    }
}

// �ɽ��ر������ɽ�һ�߳���һ�߹ҵ�
void simp_turtle_trading_strategy::on_trade(const Order& order)
{
    if (_buy_orderref == order.order_ref)
    {
        // �򵥳ɽ�����������
        cancel_order(_sell_orderref);
        _buy_orderref = null_orderref;
    }
    if (_sell_orderref == order.order_ref)
    {
        // �����ɽ���������
        cancel_order(_buy_orderref);
        _sell_orderref = null_orderref;
    }
}

// �����ر��������ùҵ�����
void simp_turtle_trading_strategy::on_cancel(const Order& order)
{
    if (_buy_orderref == order.order_ref) { _buy_orderref = null_orderref; }

    if (_sell_orderref == order.order_ref) { _sell_orderref = null_orderref; }
}

// ����ر��������ùҵ�����
void simp_turtle_trading_strategy::on_error(const Order& order)
{
    if (_buy_orderref == order.order_ref) { _buy_orderref = null_orderref; }

    if (_sell_orderref == order.order_ref) { _sell_orderref = null_orderref; }
}
