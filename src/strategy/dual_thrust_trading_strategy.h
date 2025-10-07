#pragma once

#include "strategy.h"
#include "Logger.h"
#include <deque>
#include <vector>
#include <string>

// Bar���ݽṹ���洢K�ߵĻ�����Ϣ
struct DTBarData {
    std::string date_str;   // �����ַ���
    time_t datetime;        // ʱ���
    double open;            // ���̼�
    double high;            // ��߼�
    double low;             // ��ͼ�
    double close;           // ���̼�
    int volume;             // �ɽ���
};

// ֹӯ����ö��
enum class TakeProfitType {
    FixedPoint,     // �̶�����ֹӯ
    Ratio,          // �̶�����ֹӯ
    Volatility,     // ������ֹӯ
    Trailing        // �ƶ�ֹ��תֹӯ
};

// Dual Thrust ����������
class dual_thrust_trading_strategy : public strategy
{
public:
    /**
     * Dual Thrust ���Թ��캯��
     * @param id ����ID
     * @param frame �������
     * @param contract ��Լ����
     * @param n �������䳤��
     * @param k1 ������ϵ��
     * @param k2 ������ϵ��
     * @param once_vol ÿ���µ�����
     * @param is_sim �Ƿ�Ϊ����ģʽ
     * @param hist_file ��ʷ�����ļ�
     * @param sim_start_date ������ʼ����
     * @param base_days �����׼����
     * @param end_time ����ʱ��
     */
    dual_thrust_trading_strategy(stratid_t id, frame& frame, const std::string& contract,
        int n = 4, double k1 = 0.5, double k2 = 0.5, int once_vol = 1,
        bool is_sim = false, const std::string& hist_file = "",
        const std::string& sim_start_date = "", int base_days = 4,
        const std::string& end_time = "14:59");
    ~dual_thrust_trading_strategy() {}

    // Tick���ݻص�
    virtual void on_tick(const MarketData& tick) override;
    // �����ر��ص�
    virtual void on_order(const Order& order) override;
    // �ɽ��ر��ص�
    virtual void on_trade(const Order& order) override;
    // �����ر��ص�
    virtual void on_cancel(const Order& order) override;
    // ����ر��ص�
    virtual void on_error(const Order& order) override;

private:
    // ����ֲֽṹ��
    struct SimPosition {
        int long_pos = 0;        // ��ͷ�ֲ�
        int short_pos = 0;       // ��ͷ�ֲ�
        double long_entry = 0.0; // ��ͷ���ּ�
		double long_take_profit = 0.0; // ��ͷֹӯ��
        double short_entry = 0.0;// ��ͷ���ּ�
		double short_take_profit = 0.0; // ��ͷֹӯ��
        double profit = 0.0;     // �ۼ�����
    };

    // ����K��bar����
    void on_bar(const DTBarData& bar);
    // ����ģʽ��ǿ��ƽ��
    void force_close_sim(const DTBarData& bar);
    // ʵ��ģʽ��ǿ��ƽ��
    void force_close_realtime(const DTBarData& bar);
    // ������ʷK������
    void load_history(const std::string& file);
	// ���ɷ����׼���䣨��K�ߣ�
	void generate_base_bars();
    // ����׼�����ָ��׼����ͷ������䣩
    void prepare_simulation();
    // ���潻������
    void simulation_interactive();

    std::string _contract;           // ��Լ����
    int _n;                          // ���䳤��
    double _k1, _k2;                 // Dual Thrust����
    int _once_vol;                   // ÿ���µ�����
    bool _is_sim;                    // �Ƿ����
    std::string _hist_file;          // ��ʷ�����ļ�
    std::string _sim_start_date;     // ������ʼ����
    int _base_days;                  // �����׼����
    std::string _end_time;           // ����ʱ��

    std::string _cur_minute;         // ��ǰ����
    DTBarData _cur_bar{};            // ��ǰbar
	DTBarData _today_bar{};          // ������k��

    std::deque<DTBarData> _bar_history;      // bar��ʷ����
    std::vector<DTBarData> _history;         // ��ʷK������
    std::vector<DTBarData> _base_bars;       // �����׼����
    std::vector<DTBarData> _sim_bars;        // ��������
    size_t _bar_cursor = 0;                  // ����bar�α�
    SimPosition _sim_pos;                    // ����ֲ�
    std::set<orderref_t> _buy_open_orders, _sell_open_orders; // ��/�����ҵ�����
    bool _is_closing = false;                // �Ƿ�����ǿƽ��־
	double _range;                           // �۸�����
	double _sim_win_rate = 0.0;               // ����ʤ��
	double _sim_profit_loss_rate = 0.0;        // ����ӯ����
	int _sim_trades = 0;                     // �����ܽ��״���
    int _win_count = 0;
    double _total_win = 0.0, _total_loss = 0.0;

    TakeProfitType _take_profit_type = TakeProfitType::Volatility; // ֹӯ����

    // ����ֹӯ��ֵ
    // direction: 1=��ͷ��-1=��ͷ
    double calc_take_profit(double entry_price, double volatility, double current_price, double trailing_extreme, double fixed_value, double ratio_value, int direction);

    // �ӻ�׼������㲨����
    double calc_range_from_base_bars(const std::vector<DTBarData>& bars);
	// ATR�ƶ�ֹ���㷨
    double calc_atr_trailing_stop(const std::deque<DTBarData>& bars, int atr_period, double atr_mult,
        double entry_price, int direction, double cur_take_profit);
};