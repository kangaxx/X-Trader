#pragma once

#include "strategy.h"
#include "Logger.h"

class simp_turtle_trading_strategy : public strategy
{
public:
	simp_turtle_trading_strategy(stratid_t id, frame& frame, std::string contract, double price_delta, uint32_t position_limit, int once_vol,
		std::string begin_time, std::string end_time) :
		strategy(id, frame), _contract(contract),
		_price_delta(price_delta), _position_limit(position_limit), _once_vol(once_vol),
		_buy_orderref(null_orderref), _sell_orderref(null_orderref), _is_closing(false),
		_begin_time(begin_time), _end_time(end_time)
	{
		get_contracts().insert(contract);
		if (!_redis.connect("127.0.0.1", 6379)) {
			Logger::get_instance().error("can not connect to redis server 127.0.0.1:6379!");
			exit(1);
		}
	}

	~simp_turtle_trading_strategy() { _redis.disconnect(); }

	virtual void on_tick(const MarketData& tick) override;
	virtual void on_order(const Order& order) override;
	virtual void on_trade(const Order& order) override;
	virtual void on_cancel(const Order& order) override;
	virtual void on_error(const Order& order) override;

private:
	std::string _contract;
	double _price_delta;
	uint32_t _position_limit;
	int _once_vol;
	orderref_t _buy_orderref;
	orderref_t _sell_orderref;
	bool _is_closing;
	bool _is_begin = false;
	std::string _begin_time = "09:00:00";
	std::string _end_time = "23:00:00";
	int _n1 = 20;
	int _n2 = 10;
	RedisClient& _redis = RedisClient::getInstance();
};