#include "frame.h"
#include "../strategy/Logger.h"
#include <sstream>

/**
 * @brief Constructor, initialize frame object and create realtime instance
 * @param filename config file name
 */
frame::frame(const char* filename) : _filename(filename)
{
	_realtime = new realtime();
	frame::_self = this;
}

/**
 * @brief Destructor, release realtime instance
 */
frame::~frame()
{
	if (_realtime)
	{
		Logger::get_instance().info(std::string("frame::~frame delete realtime instance"));
		delete _realtime; _realtime = nullptr;
	}
}

/**
 * @brief Place order and associate order with strategy ID
 * @param sid strategy ID
 * @param order_flag order flag
 * @param contract contract code
 * @param dir_offset direction and offset
 * @param price order price
 * @param volume order volume
 * @return order reference
 */
orderref_t frame::insert_order(const stratid_t sid, eOrderFlag order_flag, const std::string& contract, eDirOffset dir_offset, double price, int volume)
{
	std::string msg = "frame::insert_order call _realtime->insert_order, contract=" + contract +
		", price=" + std::to_string(price) + ", volume=" + std::to_string(volume);
	Logger::get_instance().info(msg);
	auto id = _realtime->insert_order(order_flag, contract, dir_offset, price, volume);
	if (id != null_orderref) { _order_to_strategy_map[id] = sid; }
	return id;
}

/**
 * @brief Cancel specified order
 * @param order_ref order reference
 * @return true if cancel succeeded
 */
bool frame::cancel_order(const orderref_t order_ref)
{
	std::string msg = "frame::cancel_order call _realtime->cancel_order, order_ref=" + std::to_string(order_ref);
	Logger::get_instance().info(msg);
	return _realtime->cancel_order(order_ref);
}

/**
 * @brief Set order cancel condition callback
 * @param id order reference
 * @param callback cancel condition callback
 */
void frame::set_cancel_condition(orderref_t id, std::function<bool(orderref_t)> callback)
{
	_cancel_condition_map[id] = callback;
}

/**
 * @brief Remove order cancel condition
 * @param id order reference
 */
void frame::remove_cancel_condition(orderref_t id)
{
	auto it = _cancel_condition_map.find(id);
	if (it != _cancel_condition_map.end())
	{
		_cancel_condition_map.erase(it);
	}
}

/**
 * @brief Register tape receiver
 * @param contract contract code
 * @param receiver tape_receiver pointer
 */
void frame::register_tape_receiver(const std::string& contract, tape_receiver* receiver)
{
	_tape_receiver[contract].insert(receiver);
}

/**
 * @brief Register bar receiver
 * @param contract contract code
 * @param period bar period
 * @param receiver bar_receiver pointer
 */
void frame::register_bar_receiver(const std::string& contract, uint32_t period, bar_receiver* receiver)
{
	std::string msg = "frame::register_bar_receiver create resample, contract=" + contract +
		", period=" + std::to_string(period);
	Logger::get_instance().info(msg);
	auto iter = _resample[contract].find(period);
	if (iter == _resample[contract].end())
	{
		_resample[contract][period] = std::make_shared<resample>(period, _realtime->get_instrument(contract).price_tick);
	}
	_resample[contract][period]->add_receiver(receiver);
}

/**
 * @brief Register strategies and bind related event callbacks
 * @param strategys strategy object list
 */
void frame::register_strategy(const std::vector<std::shared_ptr<strategy>>& strategys)
{
	for (auto iter : strategys)
	{
		_strategy_map[iter->get_id()] = iter;
		for (auto it : iter->get_contracts())
		{
			_contracts.insert(it);
			_contract_to_strategy_map[it].push_back(iter);
		}
	}
	Logger::get_instance().info(std::string("frame::register_strategy bind tick, update, order events"));
	_realtime->bind_tick_event(_tick_callback);
	_realtime->bind_update_callback(_update_callback);
	_realtime->bind_order_event(OrderEvent{ _order_callback, _trade_callback, _cancel_callback, _error_callback });
}

/**
 * @brief Clear all strategies and related data
 */
void frame::clear_strategy()
{
	Logger::get_instance().info(std::string("frame::clear_strategy clear all strategies and related data"));
	_strategy_map.clear();
	_contracts.clear();
	_contract_to_strategy_map.clear();
	_order_to_strategy_map.clear();
	_cancel_condition_map.clear();
	_resample.clear(); _tape_receiver.clear();
}

/**
 * @brief Check and execute cancel operation for orders that meet the condition
 */
void frame::check_cancel_condition()
{
	for (auto it = _cancel_condition_map.begin(); it != _cancel_condition_map.end();)
	{
		if (it->second(it->first))
		{
			std::string msg = "frame::check_cancel_condition cancel condition met, order_ref=" + std::to_string(it->first);
			Logger::get_instance().info(msg);
			if (_realtime->cancel_order(it->first))
			{
				it = _cancel_condition_map.erase(it);
			}
			else { ++it; }
		}
		else { ++it; }
	}
}

/**
 * @brief Unregister order and strategy association
 * @param order_ref order reference
 */
void frame::unregister_orderref_strategy(orderref_t order_ref)
{
	auto it = _order_to_strategy_map.find(order_ref);
	if (it != _order_to_strategy_map.end())
	{
		_order_to_strategy_map.erase(it);
	}
}

/**
 * @brief Generate time_t close time for specified date and time
 * @param day date string (e.g. "20240101")
 * @param close close time string (e.g. "15:00:00"), default "15:00:00"
 * @return close time as time_t
 */
time_t frame::make_close_time(const std::string day, std::string close)
{
	std::string time_str = day + close;
	std::tm tm = {};
	std::stringstream ss(time_str);
	ss >> std::get_time(&tm, "%Y%m%d%H:%M:%S");
	std::time_t time = std::mktime(&tm);
	return time;
}

/**
 * @brief Run strategies until close time, then cleanup resources
 * @param strategys strategy object list
 */
void frame::run_until_close(const std::vector<std::shared_ptr<strategy>>& strategys)
{
	register_strategy(strategys);
	Logger::get_instance().info(std::string("frame::run_until_close call _realtime->init"));
	if (!_realtime->init(_filename, _contracts)) { return; }
	Logger::get_instance().info(std::string("frame::run_until_close call _realtime->start_service"));
	_realtime->start_service();
	_init_callback();
	time_t close_time = make_close_time(_realtime->get_trading_day(), "15:00:00");
	std::string close_time_str = std::to_string(close_time);
	time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::string now_time_str = std::to_string(now_time);
	time_t delta_seconds = close_time - now_time;
	std::string delta_seconds_str = std::to_string(delta_seconds);
	std::string msg = "frame::run_until_close close_time=" + close_time_str +
		", now_time=" + now_time_str + ", delta_seconds=" + delta_seconds_str;
	Logger::get_instance().info(msg);
	if (delta_seconds > 0)
	{
		std::this_thread::sleep_for(std::chrono::seconds(delta_seconds));
	}
	_realtime->get_account();
	std::this_thread::sleep_for(std::chrono::seconds(60));
	Logger::get_instance().info(std::string("frame::run_until_close call _realtime->stop_service"));
	_realtime->stop_service();
	clear_strategy();
	Logger::get_instance().info(std::string("frame::run_until_close call _realtime->release"));
	_realtime->release();
}