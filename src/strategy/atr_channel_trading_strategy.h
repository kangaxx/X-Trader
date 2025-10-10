//创建一个strategy的子类, 该类实现了一个基于ATR指标的交易策略
#ifndef ATR_CHANNEL_TRADING_STRATEGY_H
#define ATR_CHANNEL_TRADING_STRATEGY_H
#include "strategy.h"
#include "../common/atr_indicator.h"
#include "../common/long_ma_indicator.h"
#include "../common/short_ma_indicator.h"
//#include "../backtest/backtest_engine.h"
#include "Logger.h"
#include <string>
#include <memory>
#include <optional>
#include <variant>
#include <any>
    
class ATRChannelTradingStrategy : public strategy {
public:
    // 构造函数，初始化策略参数
    // 参数:
    // - symbol: 交易品种代码
    // - atrPeriod: ATR指标的计算周期
    // - atrMultiplier: ATR通道的倍数，用于计算通道宽度
    // - shortMAPeriod: 短期移动平均线的计算周期
    // - longMAPeriod: 长期移动平均线的计算周期
    // - is_simulation: 是否为模拟交易，默认为false
    // 返回值: 无
    // 示例:
    // ATRChannelTradingStrategy strategy("AAPL", 14, 2.0, 20, 50);
    // 注意事项:
    // - 确保传入的参数合理，例如周期应为正整数，倍数应为正数
    // - 该构造函数仅初始化参数
    ATRChannelTradingStrategy(stratid_t id, frame& frame, const std::string& symbol, int atrPeriod, double atrMultiplier
        , int shortMAPeriod, int longMAPeriod, bool is_simulation = true)
        : strategy(id, frame), _symbol(symbol), atrIndicator(atrPeriod), atrMultiplier(atrMultiplier),
          shortMAIndicator(shortMAPeriod), longMAIndicator(longMAPeriod), _is_simulation(is_simulation) 
    {
          std::string _startDate = "20220101"; // 回测起始日期
          std::string _endDate = "20221231";   // 回测结束日期
                                              // 根据is_simulation参数决定是否启用回测功能
          if (_is_simulation) {
              Logger::get_instance().info("Initialized ATRChannelTradingStrategy in simulation mode");
              //虚拟模式下，程序从历史数据中读取数据，按照指定的起始日期和结束日期进行回测
              //回测结果可以用来评估策略的有效性和稳定性
              //回测过程中，程序会模拟真实交易环境，包括手续费、滑点等因素
              //回测完成后，程序会生成一份详细的报告，包含策略的收益率、最大回撤、夏普比率等指标
              //
              //回测结果仅供参考，实际交易中可能会有较大差异
              //回测代码与实际程序的onTick方法一致，确保策略逻辑的一致性
              //回测过程中，程序会记录每笔交易的详细信息，包括买入卖出时间、价格、数量等
              //以下是回测代码
               BacktestEngine backtestEngine(symbol, _startDate, _endDate);
               backtestEngine.runBacktest([this](const TickData& tick) {
                   this->onTick(tick);
               });
               backtestEngine.generateReport("backtest_report.txt");
          } else {
              Logger::getInstance().info("Initialized ATRChannelTradingStrategy in live trading mode");
          } 
    }
    
    // 析构函数
    ~ATRChannelTradingStrategy() override {
        Logger::getInstance().info("ATRChannelTradingStrategy destroyed");
    }

    // 回测引擎
    class BacktestEngine {
    public:
        BacktestEngine(const std::string& symbol, const std::string& startDate, const std::string& endDate)
            : symbol(symbol), startDate(startDate), endDate(endDate) {}
        void runBacktest(std::function<void(const TickData&)> onTickCallback) {
            // 这里实现回测逻辑，读取历史数据并调用onTickCallback
        }
        void generateReport(const std::string& filename) {
            // 这里实现报告生成逻辑
        }
    private:
        std::string symbol;
        std::string _startDate;
        std::string _endDate;
        vector<TickData> historicalData; // 存储历史数据
                                         //将指定分钟级数据集中指定日期范围内的数据整合成日线数据并存储到dailyData中
        std::vector<TickData> dailyData; // 存储日线数据
        // 其他回测相关成员变量
        // 其他回测相关成员函数
        void loadHistoricalData() {
            // 这里实现加载历史数据的逻辑
            // 回测文件的文件名是期货产品代码前转换为两个大写字母加上9999加一个常量字符串  
            //  例如，"IF9999_historical_data.csv"  
            // 读取CSV文件并解析数据
            // 将数据存储在适当的数据结构中
            // 确保数据按时间顺序排列
            if (symbol.length() < 2) {
                throw std::invalid_argument("Symbol must have at least 2 characters");
            }
            if (islower(symbol[0]) || islower(symbol[1])) {
                // 转换为大写
                symbol[0] = toupper(symbol[0]);
                symbol[1] = toupper(symbol[1]);
            }
            std::string filename = symbol.substr(0, 2) + "9999.XSGE.csv"; 
            // 读取CSV文件并解析数据
            // 将数据存储在适当的数据结构中
            // 确保数据按时间顺序排列
            std::cout << "Loading historical data from " << filename << std::endl;
            historicalData.clear();
            // 这里添加实际的文件读取和数据解析代码
            // 例如，使用CSV库读取文件并填充historicalData向量
            // 确保数据按时间顺序排列
            ifream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            std::string line;
            // 读取文件头
            std::getline(file, line);
            while (std::getline(file, line)) {
                std::istringstream ss(line);
                std::string token;
                TickData tick;
                // 假设CSV文件的列顺序为: date, open, high, low, close, volume, open_interest, symbol
                std::getline(ss, token, ',');
                tick.date = token;
                std::getline(ss, token, ',');
                tick.open = std::stod(token);
                std::getline(ss, token, ',');
                tick.high = std::stod(token);
                std::getline(ss, token, ',');
                tick.low = std::stod(token);
                std::getline(ss, token, ',');
                tick.close = std::stod(token);
                std::getline(ss, token, ',');
                tick.volume = std::stoi(token);
                std::getline(ss, token, ',');
                tick.open_interest = std::stoi(token);
                std::getline(ss, token, ',');
                tick.symbol = token;
                //使用Logger::getInstance().info() 函数打印数据日志;
                Logger::getInstance().info("Loaded tick data: " + tick.date + " tick.open : " + tick.open + " tick.close : " + std::to_string(tick.close));
                // 过滤日期范围
                if (tick.date >= startDate && tick.date <= endDate) {
                    historicalData.push_back(tick); 
                }
            }
            file.close();
            // 确保数据按时间顺序排列
            std::sort(historicalData.begin(), historicalData.end(), [](const TickData& a, const TickData& b) {
                return a.date < b.date;
            });
            std::cout << "Loaded " << historicalData.size() << " ticks from " << filename << std::endl;
        }

        // historicalData成员变量存储了从CSV文件中读取的分钟级数据,其日期字段格式为"YYYY-MM-DD HH:MM:SS"，下面的函数将起始日期与结束日期之间的historicalData数据整合成日线数据并存储到dailyData中
        void aggregateToDaily(std::string startDate, std::string endDate) {
            if (historicalData.empty()) {
                std::cout << "No historical data to aggregate" << std::endl;
                return;
            }
            dailyData.clear();
            TickData currentDay;
            std::string currentDate = historicalData[0].date.substr(0, 10); // 提取日期部分"YYYY-MM-DD"
            if (currentDate < startDate || currentDate > endDate){
            // 如果第一条数据的日期不在范围内，找到第一个符合条件的日期
                size_t i = 0;
                while (i < historicalData.size() && (historicalData[i].date.substr(0, 10) < startDate || historicalData[i].date.substr(0, 10) > endDate)) {
                    ++i;
                }
                if (i == historicalData.size()) {
                    std::cout << "No data in the specified date range" << std::endl;
                    return; // 没有符合条件的数据
                }
            }
            currentDate = historicalData[i].date.substr(0, 10);
            currentDay.date = currentDate;
            currentDay.open = historicalData[0].open;
            currentDay.high = historicalData[0].high;
            currentDay.low = historicalData[0].low;
            currentDay.close = historicalData[0].close;
            currentDay.volume = historicalData[0].volume;
            currentDay.open_interest = historicalData[0].open_interest;
            currentDay.symbol = historicalData[0].symbol;
            for (const auto& tick : historicalData) {
                std::string tickDate = tick.date.substr(0, 10);
                if (tickDate != currentDate) {
                    // 新的一天，保存前一天的数据
                    dailyData.push_back(currentDay);
                    // 初始化新的一天
                    currentDate = tickDate;
                    currentDay.date = currentDate;
                    currentDay.open = tick.open;
                    currentDay.high = tick.high;
                    currentDay.low = tick.low;
                    currentDay.close = tick.close;
                    currentDay.volume = tick.volume;
                    currentDay.open_interest = tick.open_interest;  
                    currentDay.symbol = tick.symbol;
                } else {            
                    // 同一天，更新最高价、最低价和收盘价
                    if (tick.high > currentDay.high) {
                        currentDay.high = tick.high;
                    }
                    if (tick.low < currentDay.low) {
                        currentDay.low = tick.low;
                    }
                    currentDay.close = tick.close; // 收盘价为最后一个tick的价格
                    currentDay.volume += tick.volume; // 累加成交量
                    currentDay.open_interest = tick.open_interest; // 持仓量取最后一个tick的值
                                                                   // 使用Logger::getInstance().info() 函数打印数据日志;
                    Logger::getInstance().info("Aggregating tick data to daily: " + tick.date + " tick.open : " + std::to_string(tick.open) + " tick.close : " + std::to_string(tick.close));
                }
            }
            // 保存最后一天的数据
            dailyData.push_back(currentDay);
            std::cout << "Aggregated to " << dailyData.size() << " daily bars" << std::endl;
        }
    };

    // 每个Tick数据到来时调用该方法，交易逻辑写在on_bar方法中,本函数只是维护_priceData和计算均线
    // 参数:
    // - tick: 当前的Tick数据，包含价格等信息
    // 返回值: 无
    void onTick(const TickData& tick) override {
        // 价格数据维护
        _priceData.push_back(tick.close); // 假设tick包含收盘价
        if (_priceData.size() > std::max(shortMAIndicator.getPeriod(), longMAIndicator.getPeriod())) {
            _priceData.erase(_priceData.begin()); // 保持价格数据长度不超过最大周期
        } 
        // 更新短期和长期均线
        updateShortMA(tick);
        updateLongMA(tick); 
        if (_lastPrice == 0.0) {
            _lastPrice = tick.close;
            return; // 初始化上一个价格
        }
        // 如果价格没有变化，直接返回
        if (tick.close == _lastPrice) {
            return;
        }
        _lastPrice = tick.close;    
        onBar(tick); // 调用onBar方法处理交易逻辑   
        Logger::getInstance().info("onTick called with tick data");
    } 
    // 根据传入的实时Tick数据,实现基于atr指标的交易策略，后续代码不要写注释
    // 参数:
    // - tick: 当前的Tick数据，包含价格等信息
    // 返回值: 无
    void updateShortMA(const TickData& tick) {
        shortMAIndicator.update(tick);
        shortMA = shortMAIndicator.getValue();
        // 计算ATR通道上轨和下轨
        if (atrIndicator.isReady() && shortMAIndicator.isReady()) {
            double atr = atrIndicator.getValue();
            upperChannel = shortMA + atrMultiplier * atr;
            lowerChannel = shortMA - atrMultiplier * atr;
        }
    }
    void updateLongMA(const TickData& tick) {
        longMAIndicator.update(tick);
        longMA = longMAIndicator.getValue();
    }   

    void onBar(const TickData& tick ) override {
        // 确保ATR和均线指标已经准备好
        if (!atrIndicator.isReady() || !shortMAIndicator.isReady() || !longMAIndicator.isReady()) {
            return;
        }
        // 获取当前价格
        double currentPrice = tick.close; // 假设tick包含收盘价
        // 交易逻辑
        if (currentPrice > upperChannel && shortMA > longMA) {
            // 价格突破上轨且短期均线在长期均线上方，考虑做多
            if (position <= 0) { // 当前没有多头持仓
                placeOrder("BUY", 100, currentPrice); // 买入100股
            }
        } else if (currentPrice < lowerChannel && shortMA < longMA) {
            // 价格跌破下轨且短期均线在长期均线下方，考虑做空
            if (position >= 0) { // 当前没有空头持仓
                placeOrder("SELL", 100, currentPrice); // 卖出100股
            }
        } else {
            // 价格在通道内，考虑平仓
            if (position > 0) { // 当前有多头持仓
                placeOrder("SELL", position, currentPrice); // 平多头仓位
            } else if (position < 0) { // 当前有空头持仓
                placeOrder("BUY", -position, currentPrice); // 平空头仓位
            }
        }
        Logger::getInstance().info("onBar executed trading logic");
    }   
private:
	std::string _symbol;
    bool _is_simulation;
    ATRIndicator atrIndicator;
    LongMAIndicator longMAIndicator;
    ShortMAIndicator shortMAIndicator;
    double upperChannel = 0.0; // ATR通道上轨
    double lowerChannel = 0.0; // ATR通道下轨
    double shortMA = 0.0; // 短期移动平均线
    double longMA = 0.0; // 长期移动平均线
    double _sam = 0.0; // 短期移动平均线值
    double atrMultiplier;
    double _lastPrice = 0.0; // 需要在类中维护上一个价格
    std::vector<double>() _priceData; // 需要在类中维护价格数据
    int position = 0; // 当前持仓，正数表示多头，负数表示空头
    void placeOrder(const std::string& action, int quantity, double price) {
        // 这里实现下单逻辑，action为"BUY"或"SELL"
        std::cout << "Placing order: " << action << " " << quantity << " at " << price << std::endl;
        if (action == "BUY") {
            position += quantity;
        } else if (action == "SELL") {
            position -= quantity;
        }
    }
    //短期移动平均线算法, period为计算周期, 返回值为void,另外需要生成一个维护价格参数的相关方法
    // 这里假设使用简单移动平均线(SMA)作为短期移动平均线的计算方法
    // 参数:
    // - period: 计算周期，表示多少个数据点用于计算平均值
    // 返回值: 无
    // 示例:
    // shortTermMovingAverage(20); // 计算20周期的短期移动平均线
    // 注意事项:
    // - 确保传入的周期为正整数
    // - 需要在类中维护一个价格数据的数组或向量，以便计算平均值
    void shortTermMovingAverage() {

        // 这里实现短期移动平均线的计算逻辑 
        double sum = 0.0;
        int period = shortMAIndicator.getPeriod();
        // 假设priceData是一个包含价格数据的数组或向量
        if (_priceData.size() < period)   {
            return; // 数据点不足，无法计算 
                    // 计算平均值
        }
        for (int i = 0; i < period; ++i) {
            sum += _priceData[i]; // priceData为价格数据数组
        }   

        _sam = sum / period;
    }
    
    //长期移动平均线算法
    void longTermMovingAverage() {
        // 这里实现长期移动平均线的计算逻辑
        // 可以使用简单移动平均线(SMA)或指数移动平均线(EMA)等方法
        // 例如，计算SMA的伪代码如下：
        /*double sum = 0.0;
        for (int i = 0; i < period; ++i) {
            sum += priceData[i]; // priceData为价格数据数组
                                 // 确保priceData有足够的数据点
                                 // 计算平均值
                                 // double sma = sum / period;
        }*/
        double sum = 0.0;
        int period = longMAIndicator.getPeriod();
        // 假设priceData是一个包含价格数据的数组或向量
        std::vector<double> priceData; // 需要在类中维护价格数据
        if (priceData.size() < period) {
            return; // 数据点不足，无法计算 
                    // 计算平均值
        double sma = sum / period;  
    }
};
#endif // ATR_CHANNEL_TRADING_STRATEGY_H
