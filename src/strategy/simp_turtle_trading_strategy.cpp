#include "simp_turtle_trading_strategy.h"
#include <sstream>
#include <deque>
#include "redis_client.h"
#include <jsoncpp/json/json.h>


// ���Բ���
struct StrategyParams {
    int fast_length = 20;     // ��������(����)
    int slow_length = 60;     // ��������(����)
    int atr_length = 14;      // ATR����
    double risk_factor = 1.0; // ����ϵ��
    double exit_multiplier = 2.0; // �˳�����
    int start_hour = 9;       // ��ʼ����Сʱ
    int start_minute = 0;     // ��ʼ���׷���
    int end_hour = 14;        // ��������Сʱ
    int end_minute = 50;      // �������׷���
};

double simp_turtle_trading_strategy::get_balance() {
    // ����Ӧ�÷���ʵ�ʵ��˻����
    return _balance; // ʾ��ֵ
}

double simp_turtle_trading_strategy::calculate_commission(double price, int volume) {
    // ����Ӧ�ø���ʵ�ʵ��������ʼ���������
    return price * volume * _commission_rate; // ʾ������
}

bool simp_turtle_trading_strategy::execute_trade(bool is_open, int size, double price, TradeRecord& record) {
    double required_margin = price * size * _contract_multiplier * _margin;
    if (is_open) {
        if (_balance < required_margin) {
            return false; // ����
        }
        _balance -= required_margin;
    } else {
        _balance += required_margin; // �����ͷű�֤��
    }
    double commission = calculate_commission(price, size);
    _balance -= commission; // �۳�������
    record.is_open = is_open;
    record.size = size;
    record.price = price;
    record.commission = commission;
    return true;
}

// �������߼���ÿ��tick���鴥��
void simp_turtle_trading_strategy::on_tick(const MarketData& tick)
{
    // ��ȡ��ǰ�����ַ���
    std::string tick_minute = std::string(tick.update_time).substr(0, 5); // "HH:MM"

    // ��̬/��Ա����ά��BarData��ʷ
    static std::deque<BarData> bar_history; // ����ATR����
    static double last_close = 0.0;

    // �ж��Ƿ�Ϊ�·���
    if (cur_minute.empty() || tick_minute != cur_minute) {
        // ������ǵ�һ���ӣ��Ƚ���һ����BarData����Redis
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

            // --- ATR������洢 ---
            // 1. ����bar����ʷ
            bar_history.push_back(cur_bar);
            if (bar_history.size() > _n1) bar_history.pop_front();

            // 2. ����TR
            std::vector<double> tr_list;
            for (size_t i = 1; i < bar_history.size(); ++i) {
                double high = bar_history[i].high;
                double low = bar_history[i].low;
                double prev_close = bar_history[i-1].close;
                double tr = std::max({high - low, std::abs(high - prev_close), std::abs(low - prev_close)});
                tr_list.push_back(tr);
            }

            // 3. ����ATR��������ƽ����
            double atr = 0.0;
            if (!tr_list.empty()) {
                double sum = 0.0;
                for (double v : tr_list) sum += v;
                atr = sum / tr_list.size();
            }

            // 4. �洢ATR��Redis����
            std::string atr_key = "turtle:" + _contract + ":atr_n1";
            _redis.lpush(atr_key, std::to_string(atr));
            _redis.ltrim(atr_key, 0, _n1 - 1);
        }
        // �·��ӣ�����BarData
        cur_bar.datetime = std::time(nullptr); // �����tickʱ���
        cur_bar.open = tick.last_price;
        cur_bar.high = tick.last_price;
        cur_bar.low = tick.last_price;
        cur_bar.close = tick.last_price;
        cur_bar.volume = tick.volume;
        cur_minute = tick_minute;
    } else {
        // �ۺϵ�ǰtick��BarData
        cur_bar.high = std::max(cur_bar.high, tick.last_price);
        cur_bar.low = std::min(cur_bar.low, tick.last_price);
        cur_bar.close = tick.last_price;
        cur_bar.volume += tick.volume;
    }

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

    // Redis����key������Ϻ�Լ����ֹ��ͻ
    std::string high_key = "turtle:" + _contract + ":high_n1";
    std::string low_key  = "turtle:" + _contract + ":low_n2";
	//Logger::get_instance().info("tick.lastprice is:" + std::to_string(tick.last_price));
    // ������ǰtick�ķֺ���
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

    // ���ֶ��г��Ȳ�����n1/n2
    _redis.ltrim(high_key, 0, _n1 - 1);
    _redis.ltrim(low_key, 0, _n2 - 1);

    // ��ȡ��������Ԫ��
    std::vector<std::string> high_n1_strs = _redis.lrange(high_key, 0, _n1 - 1);
    std::vector<std::string> low_n2_strs  = _redis.lrange(low_key, 0, _n2 - 1);

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
