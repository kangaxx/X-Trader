#pragma once

#include "strategy.h"

class write_min_bars_strategy : public strategy
{
public:
	write_min_bars_strategy(stratid_t id, frame& frame, std::string contract);
	~write_min_bars_strategy();

	virtual void on_tick(const MarketData& tick) override;
	virtual void on_order(const Order& order) override;
	virtual void on_trade(const Order& order) override;
	virtual void on_cancel(const Order& order) override;
	virtual void on_error(const Order& order) override;

private:
	std::string _contract;
};