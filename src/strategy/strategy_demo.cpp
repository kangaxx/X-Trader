#include "strategy_demo.h"
#include "frame.h"
#include "Logger.h"
#include <sstream>

strategy_demo::strategy_demo(stratid_t id, frame& frame, std::string contract)
	: strategy(id, frame), _contract(contract)
{
	get_contracts().insert(contract);
}

strategy_demo::~strategy_demo()
{
}

void strategy_demo::on_tick(const MarketData& tick)
{
  //printf("Strategy %s on tick : %.2f\n", tick.instrument_id, tick.last_price);
  std::ostringstream oss;
  oss << "Strategy " << tick.instrument_id << " , price : " << tick.last_price;
  Logger::get_instance().info(oss.str());
}

void strategy_demo::on_order(const Order& order)
{

}

void strategy_demo::on_trade(const Order& order)
{
}

void strategy_demo::on_cancel(const Order& order)
{
}

void strategy_demo::on_error(const Order& order)
{
}
