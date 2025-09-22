#pragma once

#include "strategy.h"


class statistical_arbitrage : public strategy
{
	enum class eArbitrageStatus
	{
		Invalid,
		Buy,//��1��2
		Sell,//��1��2
	};

	enum class eTradeStatus
	{
		Invalid,
		BuySingleTrade,//�� ���ȳɽ�
		BuyDoubleTrade,//�� ˫�ȳɽ�
		SellSingleTrade,//�� ���ȳɽ�
		SellDoubleTrade,//�� ˫�ȳɽ�
	};

	enum
	{
		Buy1OrderRef,
		Sell1OrderRef,
		Buy2OrderRef,
		Sell2OrderRef,
		OrderRefCount
	};

public:
	statistical_arbitrage(stratid_t id, frame& frame, std::string contract1, std::string contract2, double spread, uint32_t once_vol)
		: strategy(id, frame), _contract1(contract1), _contract2(contract2), _spread(spread), _once_vol(once_vol),
		_price1(.0), _price2(.0), _is_closing(false)
	{
		get_contracts().insert(contract1);
		get_contracts().insert(contract2);
	}

	~statistical_arbitrage() {}

	virtual void on_tick(const MarketData& tick) override;
	virtual void on_order(const Order& order) override;
	virtual void on_trade(const Order& order) override;
	virtual void on_cancel(const Order& order) override;
	virtual void on_error(const Order& order) override;
	virtual void on_update() override;

	orderref_t try_buy(const std::string& contract);
	orderref_t try_sell(const std::string& contract);

private:
	std::string _contract1;
	std::string _contract2;
	double _price1;
	double _price2;
	double _spread;
	uint32_t _once_vol;
	
	bool _is_closing;
	
	orderref_t _orderref[OrderRefCount]{ null_orderref };
	
	eArbitrageStatus _arbitrage_status = eArbitrageStatus::Invalid;
	eTradeStatus _trade_status = eTradeStatus::Invalid;
	std::map<std::string, MarketData> _tick_map{};
};
