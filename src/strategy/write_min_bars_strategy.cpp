#include "write_min_bars_strategy.h"
#include "frame.h"


write_min_bars_strategy::write_min_bars_strategy(stratid_t id, frame& frame, std::string contract)
	: strategy(id, frame), _contract(contract)
{
	get_contracts().insert(contract);
}

write_min_bars_strategy::~write_min_bars_strategy()
{
}

void write_min_bars_strategy::on_tick(const MarketData& tick)
{
	printf("Strategy %s on tick : %.2f\n", tick.instrument_id, tick.last_price);
}

void write_min_bars_strategy::on_order(const Order& order)
{

}

void write_min_bars_strategy::on_trade(const Order& order)
{
}

void write_min_bars_strategy::on_cancel(const Order& order)
{
}

void write_min_bars_strategy::on_error(const Order& order)
{
}
