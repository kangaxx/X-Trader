#pragma once

#include "define.h"


///�̿ڷ���
enum class eTapeDir
{
	Up = 1,///����
	Flat = 0,///ƽ
	Down = -1,///����
};

///�̿�״̬
enum class eTapeStatus
{
	DoubleOpen,///˫��
	Open,///����
	Exchange,///����
	Close,//ƽ��
	DoubleClose,///˫ƽ
	Unknown,///δ֪
};

//�̿���Ϣ
struct Tape
{
	uint32_t last_volume;//����
	double last_open_interest;//����
	eTapeDir tape_dir;//�̿ڷ���

	Tape() : last_volume(0), last_open_interest(.0F), tape_dir(eTapeDir::Flat) {}

	eTapeStatus get_tape_status()
	{
		if (last_volume == last_open_interest && last_open_interest > 0)
		{
			return eTapeStatus::DoubleOpen;
		}
		else if (last_volume > last_open_interest && last_open_interest > 0)
		{
			return eTapeStatus::Open;
		}
		else if (last_volume > last_open_interest && last_open_interest == 0)
		{
			return eTapeStatus::Exchange;
		}
		else if (last_volume > -last_open_interest && -last_open_interest > 0)
		{
			return eTapeStatus::Close;
		}
		else if (last_volume == -last_open_interest && -last_open_interest > 0)
		{
			return eTapeStatus::DoubleClose;
		}
		else
		{
			return eTapeStatus::Unknown;
		}
	}
};


///K��
struct Sample
{
	std::string id;///��Լ����
	uint32_t period;///����
	std::string time;///ʱ��
	double open;/// ��
	double high;/// ��
	double low;/// ��
	double close;/// ��
	int volume;///�ɽ���
	bool is_new = true;///�Ƿ�Ϊ��
	
	double price_tick; //��С�䶯��λ
	double poc;///��������POC
	int32_t delta;///��������delta(active_buy_volume - active_sell_volume)
	//�������е���ϸ
	std::map<double, uint32_t> active_buy_volume;
	std::map<double, uint32_t> active_sell_volume;

	uint32_t get_buy_volume(double price) const
	{
		auto it = active_buy_volume.find(price);
		if (it == active_buy_volume.end()) { return 0; }
		return it->second;
	}

	uint32_t get_sell_volume(double price) const
	{
		auto it = active_sell_volume.find(price);
		if (it == active_sell_volume.end()) { return 0; }
		return it->second;
	}

	std::vector<std::tuple<double, uint32_t, uint32_t>> get_order_book() const
	{
		std::vector<std::tuple<double, uint32_t, uint32_t>> result;
		for (double price = low; price <= high; price += price_tick)
		{
			result.emplace_back(std::make_tuple(price, get_buy_volume(price), get_sell_volume(price)));
		}
		return result;
	}

	//��ȡ��ƽ�ⶩ��
	std::pair<std::shared_ptr<std::vector<double>>, std::shared_ptr<std::vector<double>>> get_imbalance(uint32_t multiple) const
	{
		//����ʧ��(��������)
		auto demand_imbalance = std::make_shared<std::vector<double>>();
		//����ʧ��(����Ӧ��)
		auto supply_imbalance = std::make_shared<std::vector<double>>();
		//����������
		auto order_book = get_order_book();

		for (size_t i = 0; i < order_book.size() - 1; i++)
		{
			auto demand_cell = order_book[i];
			auto supply_cell = order_book[i + 1];
			if (std::get<1>(demand_cell) * multiple > std::get<2>(supply_cell))
			{
				demand_imbalance->emplace_back(std::get<0>(demand_cell));
			}
			else if (std::get<2>(supply_cell) * multiple > std::get<1>(demand_cell))
			{
				supply_imbalance->emplace_back(std::get<0>(supply_cell));
			}
		}
		return std::make_pair(demand_imbalance, supply_imbalance);
	}

	Sample() : time(""), period(0U), open(0), close(0), high(0), low(0), volume(0), delta(0), poc(.0), price_tick(1.0) {}

	void clear()
	{
		period = 0U; open = .0; high = .0; low = .0; close = .0; volume = 0; delta = 0; poc = 0;
		active_buy_volume.clear(); active_sell_volume.clear();
	}
};


///�������
struct MarketData
{
	//DateType trading_day; ///������
	//ExchangeIDType exchange_id; ///����������
	InstrumentIDType instrument_id; ///��Լ����
	TimeType update_time; ///����޸�ʱ��
	int update_millisec; ///����޸ĺ���

	double pre_close_price; ///������
	//double pre_open_interest;  ///��ֲ���
	double pre_settlement_price; ///�ϴν����

	double last_price; ///���¼�
	int volume; ///����
	int last_volume; ///�ɽ�����
	//double turnover; ///�ɽ����
	double open_interest; ///�ֲ���
	double last_open_interest; ///����

	double open_price; ///����
	double highest_price; ///��߼�
	double lowest_price; ///��ͼ�
	double upper_limit_price; ///��ͣ���
	double lower_limit_price; ///��ͣ���

	//double close_price; ///������
	double settlement_price; ///���ν����

	double bid_price[5]; ///�����
	int bid_volume[5]; ///������
	double ask_price[5]; ///������
	int ask_volume[5]; ///������

	double average_price; ///���վ���
	eTapeDir tape_dir;//�̿ڷ���
};


///��Լ
struct Instrument
{
	ExchangeIDType exchange_id; ///����������
	InstrumentIDType instrument_id; ///��Լ����
	//int delivery_year; ///�������
	//int delivery_month; ///������
	//int max_market_order_volume; ///�м۵�����µ���
	//int min_market_order_volume; ///�м۵���С�µ���
	//int max_limit_order_volume; ///�޼۵�����µ���
	//int min_limit_order_volume; ///�޼۵���С�µ���
	int volume_multiple; ///��Լ��������
	double price_tick; ///��С�䶯��λ

	//DateType create_date; ///������
	//DateType open_date; ///������
	//DateType expire_date; ///������
	//DateType start_deliv_date; ///��ʼ������
	//DateType end_deliv_date; ///����������

	bool is_trading; ///��ǰ�Ƿ���
	bool is_use_history;///�Ƿ�ʹ����ʷ�ֲ�

	//double long_margin_ratio; ///��ͷ��֤����
	//double short_margin_ratio; ///��ͷ��֤����
};

const Instrument null_instrument{};


///�ֲ���ϸ
struct PosiDetail
{
	int position;///�ܲ�
	int today_position;///���(������/������Դר��)
	int his_position;///���(������/������Դר��)
	
	int closeable;///�ܿ�ƽ��
	int today_closeable;///���ƽ��(������/������Դר��)
	int his_closeable;///���ƽ��(������/������Դר��)
	int open_no_trade;///���ֻ�δ�ɽ�
	double avg_open_cost;///ƽ�����ֳɱ�
	double avg_posi_cost;///ƽ���ֲֳɱ�
};

///�ֲ�
struct Position
{
	std::string id;///��Լ����
	PosiDetail long_;///��
	PosiDetail short_;///��
};

const Position null_position{};


///������ƽ����
enum class eDirOffset : int8_t
{
	BuyOpen,///��
	SellOpen,///����
	BuyClose,///����������ƽ��    ����������ƽ(������ƽ��ƽ��)
	SellClose,///����������ƽ��    ����������ƽ(������ƽ��ƽ��)
	BuyCloseToday,///����������ƽ��    ����������ƽ(������ƽ��ƽ��)
	SellCloseToday,///����������ƽ��    ����������ƽ(������ƽ��ƽ��)
	BuyCloseYesterday,///����������ƽ��    ����������ƽ(������ƽ��ƽ��)
	SellCloseYesterday///����������ƽ��    ����������ƽ(������ƽ��ƽ��)
};

///�����ύ״̬����
enum class eOrderSubmitStatus : int8_t
{
	InsertSubmitted,///�Ѿ��ύ
	CancelSubmitted,///�����Ѿ��ύ
	ModifySubmitted,///�޸��Ѿ��ύ
	Accepted,///�Ѿ�����
	InsertRejected,///�����Ѿ����ܾ�
	CancelRejected,///�����Ѿ����ܾ�
	ModifyRejected///�ĵ��Ѿ����ܾ�
};

///����״̬����
enum class eOrderStatus : int8_t
{
	AllTraded,///ȫ���ɽ�
	PartTradedQueueing,///���ֳɽ����ڶ�����
	PartTradedNotQueueing,///���ֳɽ����ڶ�����
	NoTradeQueueing,///δ�ɽ����ڶ�����
	NoTradeNotQueueing,///δ�ɽ����ڶ�����
	Canceled,///����
	Unknown,///δ֪
	NotTouched,///��δ����
	Touched///�Ѵ���
};

///�¼�����
enum class eEventFlag : int8_t
{
	Order,///����
	Trade,///�ɽ�
	Cancel,///����
	ErrorInsert,///��������
	ErrorCancel///��������
};

///������־����
enum class eOrderFlag : int8_t
{
	Limit,///�޼۵�
	Market,///�м۵�
	FOK,///Fill or Kill
	FAK///Fill and Kill
};

///����
struct Order
{
	eEventFlag event_flag;///�¼�����
	orderref_t order_ref;///��������
	int front_id;///ǰ�ñ��
	int session_id;///�Ự���

	ExchangeIDType exchange_id; ///����������
	InstrumentIDType instrument_id;///��Լ����
	eDirOffset dir_offset;///������ƽ��־
	eOrderFlag order_flag;///������־
	double limit_price;///�۸�
	int volume_total_original;///����
	int volume_traded;///��ɽ�����
	int volume_total;///ʣ������

	int request_id;///������
	OrderLocalIDType order_local_id;///���ر������
	OrderSysIDType order_sys_id;///�������

	eOrderSubmitStatus order_submit_status;///�����ύ״̬
	eOrderStatus order_status;///����״̬
	ErrorMsgType status_msg;///״̬��Ϣ

	//DateType insert_date;///��������
	TimeType insert_time;///ί��ʱ��
	//TimeType	active_time;///����ʱ��
	//TimeType suspend_time;///����ʱ��
	//TimeType update_time;///����޸�ʱ��
	TimeType cancel_time;///����ʱ��

	int error_id;///�������
	ErrorMsgType error_msg;///������Ϣ

	//bool is_cancelable = true;///�Ƿ���Գ���
};

const Order null_order{};

struct tape_receiver
{
	virtual void on_tape(const Tape& tape) = 0;
};

struct bar_receiver
{
	virtual void on_bar(const Sample& bar) = 0;
};

extern "C"
{
	typedef void (PORTER_FLAG* tick_callback)(const MarketData&);
	typedef void (PORTER_FLAG* order_callback)(const Order&);
	typedef void (PORTER_FLAG* trade_callback)(const Order&);
	typedef void (PORTER_FLAG* cancel_callback)(const Order&);
	typedef void (PORTER_FLAG* error_callback)(const Order&);
	typedef void (PORTER_FLAG* update_callback)();
}

struct OrderEvent
{
	order_callback on_order;
	trade_callback on_trade;
	cancel_callback on_cancel;
	error_callback on_error;

	OrderEvent() = default;
	OrderEvent(order_callback order_cb, trade_callback trade_cb, cancel_callback cancel_cb, error_callback error_cb)
		: on_order(order_cb), on_trade(trade_cb), on_cancel(cancel_cb), on_error(error_cb) {}
};

using InstrumentMap = std::unordered_map<std::string, Instrument>;
using OrderMap = std::map<orderref_t, Order>;
using PositionMap = std::map<std::string, Position>;




inline std::map<eDirOffset, std::string> eDirOffsetToString =
{
	{eDirOffset::BuyOpen, "BuyOpen"},{eDirOffset::SellOpen, "SellOpen"},
	{eDirOffset::BuyClose, "BuyClose"},{eDirOffset::SellClose, "SellClose"},
	{eDirOffset::BuyCloseToday, "BuyCloseToday"},{eDirOffset::SellCloseToday, "SellCloseToday"},
	{eDirOffset::BuyCloseYesterday, "BuyCloseYesterday"},{eDirOffset::SellCloseYesterday, "SellCloseYesterday"}
};

inline std::string geteDirOffsetString(eDirOffset t)
{
	return eDirOffsetToString[t];
}

inline std::map<eOrderSubmitStatus, std::string> eOrderSubmitStatusToString =
{
	{eOrderSubmitStatus::InsertSubmitted, "InsertSubmitted"},
	{eOrderSubmitStatus::CancelSubmitted, "CancelSubmitted"},
	{eOrderSubmitStatus::ModifySubmitted, "ModifySubmitted"},
	{eOrderSubmitStatus::Accepted, "Accepted"},
	{eOrderSubmitStatus::InsertRejected, "InsertRejected"},
	{eOrderSubmitStatus::CancelRejected, "CancelRejected"},
	{eOrderSubmitStatus::ModifyRejected, "ModifyRejected"}
};

inline std::string geteOrderSubmitStatusString(eOrderSubmitStatus t)
{
	return eOrderSubmitStatusToString[t];
}

inline std::map<eOrderStatus, std::string> eOrderStatusToString =
{
	{eOrderStatus::AllTraded, "AllTraded"},
	{eOrderStatus::PartTradedQueueing, "PartTradedQueueing"},
	{eOrderStatus::PartTradedNotQueueing, "PartTradedNotQueueing"},
	{eOrderStatus::NoTradeQueueing, "NoTradeQueueing"},
	{eOrderStatus::NoTradeNotQueueing, "NoTradeNotQueueing"},
	{eOrderStatus::Canceled, "Canceled"},
	{eOrderStatus::Unknown, "Unknown"},
	{eOrderStatus::NotTouched, "NotTouched"},
	{eOrderStatus::Touched, "Touched"}
};

inline std::string geteOrderStatusString(eOrderStatus t)
{
	return eOrderStatusToString[t];
}

inline std::map<eOrderFlag, std::string> eOrderFlagToString =
{
	{eOrderFlag::Limit, "Limit"},
	{eOrderFlag::Market, "Market"},
	{eOrderFlag::FOK, "FOK"},
	{eOrderFlag::FAK, "FAK"}
};

inline std::string geteOrderFlagString(eOrderFlag t)
{
	return eOrderFlagToString[t];
}
