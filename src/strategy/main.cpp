#include "frame.h"
#include "market_making.h"
#include "statistical_arbitrage.h"
#include "order_flow.h"
#include "order_imbalance.h"
#include "high_low.h"
#include "market_correction.h"
#include "strategy_demo.h"
#include "dual_thrust_trading_strategy.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <ctime>
#include "Logger.h"
#include "redis_client.h"
#include "CurlSender.h"

//#include "decline_scalping.h"
//#include "decline_swing.h"

void start_running(const char* filename)
{
	std::string str = "Work server started!";
	Logger::get_instance().info(str);
	CurlSender sender("./ini/config.ini");
	sender.send("Work Server Start");
	RedisClient& redis = RedisClient::getInstance();
	if (!redis.connect("127.0.0.1", 6379)) {
		str = "can not connect to redis server 127.0.0.1:6379!";
		Logger::get_instance().error(str);
		return;
	}
	else
	{
		str = "connect to redis server 127.0.0.1:6379 success";
		Logger::get_instance().info(str);
	}
	frame run(filename);
	std::vector<std::shared_ptr<strategy>> strategys;
	//strategys.emplace_back(std::make_shared<market_making>(1, run, "rb2409", 2, 10, 1));
	//strategys.emplace_back(std::make_shared<market_making>(1, run, "MA505", 2, 5, 1));
	//strategys.emplace_back(std::make_shared<statistical_arbitrage>(2, run, "MA501", "MA505", 30, 1));
	//strategys.emplace_back(std::make_shared<order_flow>(3, run, "MA505", 1, 3, 3, 4, 1));
	//strategys.emplace_back(std::make_shared<order_imbalance>(4, run, "rb2409", 1));
	//strategys.emplace_back(std::make_shared<high_low>(5, run, "MA505"));
	//strategys.emplace_back(std::make_shared<market_correction>(6, run, "RM409", 1));
	//strategys.emplace_back(std::make_shared<decline_scalping>(7, run, "RM505", 1));
	//strategys.emplace_back(std::make_shared<decline_swing>(8, run, "RM505", 1));
	strategys.emplace_back(std::make_shared<strategy_demo>(9, run, "rb2511"));
    //strategys.emplace_back(std::make_shared<dual_thrust_trading_strategy>(1, run, "ag2512", 5, 0.3, 0.3, 1, true, "/root/tb_furture_data/AG9999.XSGE.csv", "2013-01-04", 5, "14:59"));
	run.run_until_close(strategys);
}


int main()
{
	start_running("./ini/gf/24_work.ini"); //guang fa work server
	//start_running("./ini/simnow/1_099008.ini");
	//start_running("./ini/simnow/2_083309.ini");
	//start_running("./ini/simnow/1_120585.ini");
	//start_running("./ini/simnow/3_117509.ini");
	//start_running("./ini/simnow/24_120585.ini"); //simnow new 24h , 16:00 to 9:00am
	//start_running("./ini/simnow/24_099008.ini");
	//start_running("./ini/mk_5.ini");
	//start_running("./ini/mk.ini");
	//start_running("./ini/5.ini");
	//start_running("./ini/gt1.ini");
	//start_running("./ini/gt2.ini");

	return 0;
}
