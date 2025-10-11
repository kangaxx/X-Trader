#pragma once
//创建一个strategy的子类, 该类实现了一个基于ATR指标的交易策略
#include "strategy.h"
#include "../common/atr_indicator.h"
#include "../common/long_ma_indicator.h"
#include "../common/short_ma_indicator.h"
#include "../backtest/backtest_engine.h"
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
        : strategy(id, frame), _symbol(symbol), atrIndicator(atrPeriod), _atrMultiplier(atrMultiplier),
          shortMAIndicator(shortMAPeriod), longMAIndicator(longMAPeriod), _is_simulation(is_simulation) 
    {
          std::string _startDate = "20220101"; // 回测起始日期
          std::string _endDate = "20221231";   // 回测结束日期
                                              // 根据is_simulation参数决定是否启用回测功能
          if (_is_simulation) {
              Logger::get_instance().info("Initialized ATRChannelTradingStrategy in simulation mode");
              //以下是回测代码
               BacktestEngine backtestEngine(_symbol, _startDate, _endDate);
			   //在此通过回测引擎注册on_bar回调函数
			   backtestEngine.SetOnBarCallback([this](const MarketData& tick) { this->onBar(tick); });
               
               //backtestEngine.generateReport("backtest_report.txt");
          } else {
              Logger::get_instance().info("Initialized ATRChannelTradingStrategy in live trading mode");
          } 
    }
    
    // 析构函数
    ~ATRChannelTradingStrategy() override {
    }

    // 每个Tick数据到来时调用该方法，交易逻辑写在on_bar方法中,本函数只是维护_priceData和计算均线
    // 参数:
    // - tick: 当前的Tick数据，包含价格等信息
    // 返回值: 无
    void onTick(const MarketData& tick) {
        // 价格数据维护
        _priceData.push_back(tick.last_price); // 假设tick包含收盘价
        if (_priceData.size() > std::max(shortMAIndicator.getPeriod(), longMAIndicator.getPeriod())) {
            _priceData.erase(_priceData.begin()); // 保持价格数据长度不超过最大周期
        } 
        // 更新短期和长期均线
        updateShortMA(tick);
        updateLongMA(tick); 
        if (_lastPrice == 0.0) {
            _lastPrice = tick.last_price;
            return; // 初始化上一个价格
        }
        // 如果价格没有变化，直接返回
        if (tick.last_price == _lastPrice) {
            return;
        }
        _lastPrice = tick.last_price;    
        onBar(tick); // 调用onBar方法处理交易逻辑   
        Logger::get_instance().info("onTick called with tick data");
    } 
    // 根据传入的实时Tick数据,实现基于atr指标的交易策略，后续代码不要写注释
    // 参数:
    // - tick: 当前的Tick数据，包含价格等信息
    // 返回值: 无
    void updateShortMA(const MarketData& tick) {
        shortMAIndicator.addPrice(tick.last_price);
        _shortMA = shortMAIndicator.getShortMA();
        // 计算ATR通道上轨和下轨
        if (atrIndicator.isReady() && shortMAIndicator.isReady()) {
            double atr = atrIndicator.getATR();
            _upperChannel = _shortMA + _atrMultiplier * atr;
            _lowerChannel = _shortMA - _atrMultiplier * atr;
        }
    }
    void updateLongMA(const MarketData& tick) {
		longMAIndicator.addPrice(tick.last_price);
        _longMA = longMAIndicator.getLongMA();
    }   

    void onBar(const MarketData& tick) /* override */ {
        // 确保ATR和均线指标已经准备好
        if (!atrIndicator.isReady() || !shortMAIndicator.isReady() || !longMAIndicator.isReady()) {
            return;
        }
        double currentPrice = tick.last_price;
        if (currentPrice > _upperChannel && _shortMA > _longMA) {
            if (position <= 0) {
                placeOrder("BUY", 100, currentPrice);
            }
        } else if (currentPrice < _lowerChannel && _shortMA < _longMA) {
            if (position >= 0) {
                placeOrder("SELL", 100, currentPrice);
            }
        } else {
            if (position > 0) {
                placeOrder("SELL", position, currentPrice);
            } else if (position < 0) {
                placeOrder("BUY", -position, currentPrice);
            }
        }
        Logger::get_instance().info("onBar executed trading logic");
    }   
private:
	std::string _symbol;
    bool _is_simulation;
    ATRIndicator atrIndicator;
    LongMAIndicator longMAIndicator;
    ShortMAIndicator shortMAIndicator;
    double _upperChannel = 0.0; // ATR通道上轨
    double _lowerChannel = 0.0; // ATR通道下轨
    double _shortMA = 0.0; // 短期移动平均线
    double _longMA = 0.0; // 长期移动平均线
    double _sam = 0.0; // 短期移动平均线值
    double _atrMultiplier; // ATR通道倍数
    double _lastPrice = 0.0; // 需要在类中维护上一个价格
    std::vector<double> _priceData; // 需要在类中维护价格数据
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
    }
};
