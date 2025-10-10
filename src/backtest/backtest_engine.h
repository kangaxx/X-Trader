//这是一个通用的回测引擎接口，用户可以根据自己的需求实现不同的回测引擎
#ifndef BACKTEST_ENGINE_H
#define BACKTEST_ENGINE_H
#include <string>
#include <vector>
#include <map>
#include <memory>
/*
#include "market_data.h"
#include "order.h"
#include "trade.h"
#include "position.h"
#include "account.h"
#include "strategy.h
#include "performance.h"
#include "risk_management.h"
#include "event.h"
*/
#include "../strategy/Logger.h"
#include "config.h"
/*
#include "utils.h"
#include "data_handler.h"
#include "execution_handler.h"
#include "portfolio.h"
#include "statistics.h"
#include "report.h" 
#include "optimization.h"
#include "visualization.h"
#include "benchmark.h"
#include "slippage_model.h"
#include "commission_model.h"
#include "liquidity_model.h"
#include "market_impact_model.h"
#include "transaction_cost_model.h"
#include "order_sizing_model.h"
#include "position_sizing_model.h"
#include "risk_model.h"     
#include "drawdown_model.h"
#include "volatility_model.h"
#include "correlation_model.h"
#include "factor_model.h"
#include "alpha_model.h"
#include "beta_model.h"
#include "momentum_model.h"
#include "mean_reversion_model.h"
#include "seasonality_model.h"
#include "cycle_model.h"
#include "trend_model.h"
#include "pattern_model.h"
#include "signal_model.h"
#include "feature_model.h"
#include "machine_learning_model.h"
#include "deep_learning_model.h"
#include "reinforcement_learning_model.h"
#include "natural_language_processing_model.h"
#include "genetic_algorithm_model.h"
#include "swarm_intelligence_model.h"
#include "fuzzy_logic_model.h"
#include "expert_system_model.h"
#include "hybrid_model.h"
#include "custom_model.h"
#include "data_feed.h"
#include "data_source.h"
#include "data_storage.h"
#include "data_preprocessing.h"
#include "data_transformation.h"
#include "data_augmentation.h"
#include "data_cleaning.h"
#include "data_normalization.h"
#include "data_standardization.h"
#include "data_imputation.h"
#include "data_reduction.h"
#include "data_sampling.h"
#include "data_splitting.h"
#include "data_merging.h"
#include "data_aggregation.h"
#include "data_filtering.h"
#include "data_sorting.h"
#include "data_indexing.h"
#include "data_querying.h"
#include "data_visualization.h"
#include "data_analysis.h"
#include "data_statistics.h"
#include "data_machine_learning.h"
#include "data_deep_learning.h"
#include "data_reinforcement_learning.h"
#include "data_natural_language_processing.h"
#include "data_genetic_algorithm.h"
#include "data_swarm_intelligence.h"
#include "data_fuzzy_logic.h"
#include "data_expert_system.h"
#include "data_hybrid.h"
#include "data_custom.h"
#include "execution_strategy.h"
#include "execution_algorithm.h"
#include "execution_policy.h"
#include "execution_risk_management.h"
#include "execution_slippage_model.h"
#include "execution_commission_model.h"
#include "execution_liquidity_model.h"
#include "execution_market_impact_model.h"
#include "execution_transaction_cost_model.h"
#include "execution_order_sizing_model.h"
#include "execution_position_sizing_model.h"
#include "execution_risk_model.h"
#include "execution_drawdown_model.h"
#include "execution_volatility_model.h"
#include "execution_correlation_model.h"
#include "execution_factor_model.h"
#include "execution_alpha_model.h"
#include "execution_beta_model.h"
#include "execution_momentum_model.h"
#include "execution_mean_reversion_model.h"
#include "execution_seasonality_model.h"
#include "execution_cycle_model.h"
#include "execution_trend_model.h"
#include "execution_pattern_model.h"
#include "execution_signal_model.h"
#include "execution_feature_model.h"
#include "execution_machine_learning_model.h"
#include "execution_deep_learning_model.h"
#include "execution_reinforcement_learning_model.h"
#include "execution_natural_language_processing_model.h"
#include "execution_genetic_algorithm_model.h"
#include "execution_swarm_intelligence_model.h"
#include "execution_fuzzy_logic_model.h"
#include "execution_expert_system_model.h"
#include "execution_hybrid_model.h"
#include "execution_custom_model.h"
#include "backtest_report.h"
#include "backtest_visualization.h"
#include "backtest_optimization.h"
#include "backtest_statistics.h"
#include "backtest_benchmark.h"
#include "backtest_slippage_model.h"
#include "backtest_commission_model.h"
#include "backtest_liquidity_model.h"
#include "backtest_market_impact_model.h"
#include "backtest_transaction_cost_model.h"
#include "backtest_order_sizing_model.h"
#include "backtest_position_sizing_model.h"
#include "backtest_risk_model.h"
#include "backtest_drawdown_model.h"
#include "backtest_volatility_model.h"
#include "backtest_correlation_model.h"
#include "backtest_factor_model.h"
#include "backtest_alpha_model.h"
#include "backtest_beta_model.h"
#include "backtest_momentum_model.h"
#include "backtest_mean_reversion_model.h"
#include "backtest_seasonality_model.h"
#include "backtest_cycle_model.h"
#include "backtest_trend_model.h"
#include "backtest_pattern_model.h"
#include "backtest_signal_model.h"
#include "backtest_feature_model.h"
#include "backtest_machine_learning_model.h"
#include "backtest_deep_learning_model.h"
#include "backtest_reinforcement_learning_model.h"
#include "backtest_natural_language_processing_model.h"
#include "backtest_genetic_algorithm_model.h"
#include "backtest_swarm_intelligence_model.h"
#include "backtest_fuzzy_logic_model.h"
#include "backtest_expert_system_model.h"
#include "backtest_hybrid_model.h"
#include "backtest_custom_model.h"
#include "backtest_data_feed.h"
#include "backtest_data_source.h"
#include "backtest_data_storage.h"              
#include "backtest_data_preprocessing.h"
#include "backtest_data_transformation.h"
#include "backtest_data_augmentation.h"
#include "backtest_data_cleaning.h"
#include "backtest_data_normalization.h"
#include "backtest_data_standardization.h"
#include "backtest_data_imputation.h"
#include "backtest_data_reduction.h"
#include "backtest_data_sampling.h"
#include "backtest_data_splitting.h"
#include "backtest_data_merging.h"
#include "backtest_data_aggregation.h"
#include "backtest_data_filtering.h"
#include "backtest_data_sorting.h"
#include "backtest_data_indexing.h"
#include "backtest_data_querying.h"
#include "backtest_data_visualization.h"
#include "backtest_data_analysis.h"
#include "backtest_data_statistics.h"
#include "backtest_data_machine_learning.h"
#include "backtest_data_deep_learning.h"
#include "backtest_data_reinforcement_learning.h"
#include "backtest_data_natural_language_processing.h"
#include "backtest_data_genetic_algorithm.h"
#include "backtest_data_swarm_intelligence.h"
#include "backtest_data_fuzzy_logic.h"
#include "backtest_data_expert_system.h"
#include "backtest_data_hybrid.h"
#include "backtest_data_custom.h"
#include "backtest_execution_strategy.h"
#include "backtest_execution_algorithm.h"
#include "backtest_execution_policy.h"
#include "backtest_execution_risk_management.h"
#include "backtest_execution_slippage_model.h"
#include "backtest_execution_commission_model.h"
#include "backtest_execution_liquidity_model.h"
#include "backtest_execution_market_impact_model.h"
#include "backtest_execution_transaction_cost_model.h"
#include "backtest_execution_order_sizing_model.h"
#include "backtest_execution_position_sizing_model.h"           
#include "backtest_execution_risk_model.h"
#include "backtest_execution_drawdown_model.h"
#include "backtest_execution_volatility_model.h"
#include "backtest_execution_correlation_model.h"
#include "backtest_execution_factor_model.h"
#include "backtest_execution_alpha_model.h"
#include "backtest_execution_beta_model.h"
#include "backtest_execution_momentum_model.h"
#include "backtest_execution_mean_reversion_model.h"
#include "backtest_execution_seasonality_model.h"
#include "backtest_execution_cycle_model.h"
*/
class BacktestEngine {
public:
    BacktestEngine(const Config& config);
    virtual ~BacktestEngine() = default;    
    // 初始化回测引擎
    virtual void initialize() = 0;
    // 运行回测
    virtual void run() = 0;
    // 停止回测
    virtual void stop() = 0;
    // 获取回测结果
    virtual Performance get_performance() const = 0;
    // 获取账户信息
    virtual Account get_account() const = 0;
    // 获取持仓信息
    virtual std::map<std::string, Position> get_positions() const = 0;
    // 获取交易记录
    virtual std::vector<Trade> get_trades() const = 0;
    // 获取订单记录
    virtual std::vector<Order> get_orders() const = 0;
    // 获取市场数据
    virtual MarketData get_market_data(const std::string& symbol) const = 0;
    // 添加策略
    virtual void add_strategy(std::shared_ptr<Strategy> strategy) = 0;
    // 移除策略
    virtual void remove_strategy(const std::string& strategy_name) = 0;
    // 获取所有策略
    virtual std::vector<std::shared_ptr<Strategy>> get_strategies() const = 0;
    // 设置数据处理器
    virtual void set_data_handler(std::shared_ptr<DataHandler> data_handler) = 0;
    // 设置执行处理器
    virtual void set_execution_handler(std::shared_ptr<ExecutionHandler> execution_handler) = 0;
    // 设置投资组合
    virtual void set_portfolio(std::shared_ptr<Portfolio> portfolio) = 0;
    // 设置风险管理
    virtual void set_risk_management(std::shared_ptr<RiskManagement> risk_management) = 0;
    // 设置日志记录器
    virtual void set_logger(std::shared_ptr<Logger> logger) = 0;
    // 设置配置
    virtual void set_config(const Config& config) = 0;
    // 获取配置
    virtual Config get_config() const = 0;
    // 重置回测引擎
    virtual void reset() = 0;
protected:
    Config config_; // 配置
    std::shared_ptr<DataHandler> data_handler_; // 数据处理器
    std::shared_ptr<ExecutionHandler> execution_handler_; // 执行处理器
    std::shared_ptr<Portfolio> portfolio_; // 投资组合
    std::shared_ptr<RiskManagement> risk_management_; // 风险管理
    std::shared_ptr<Logger> logger_; // 日志记录器
    std::vector<std::shared_ptr<Strategy>> strategies_; // 策略列表
    Account account_; // 账户
    std::map<std::string, Position> positions_; // 持仓
    std::vector<Trade> trades_; // 交易记录
    std::vector<Order> orders_; // 订单记录
    std::map<std::string, MarketData> market_data_; // 市场数据
    Performance performance_; // 回测结果
};

// 示例实现一个简单的回测引擎
class SimpleBacktestEngine : public BacktestEngine {
public:
    SimpleBacktestEngine(const Config& config) : BacktestEngine(config) {}
    void initialize() override {
        Logger::get_instance().)info("Initializing SimpleBacktestEngine");
        // 初始化逻辑
    }
    void run() override {
        Logger::get_instance().info("Running SimpleBacktestEngine");
        // 运行逻辑
    }
    void stop() override {
        Logger::get_instance().info("Stopping SimpleBacktestEngine");
        // 停止逻辑
    }
    Performance get_performance() const override {
        return performance_;
    }
    Account get_account() const override {
        return account_;
    }
    std::map<std::string, Position> get_positions() const override {
        return positions_;
    }
    std::vector<Trade> get_trades() const override {
        return trades_;
    }
    std::vector<Order> get_orders() const override {
        return orders_;
    }
    MarketData get_market_data(const std::string& symbol) const override {
        auto it = market_data_.find(symbol);
        if (it != market_data_.end()) {
            return it->second;
        }
        return MarketData(); // 返回空的MarketData
    }
    void add_strategy(std::shared_ptr<Strategy> strategy) override {
        strategies_.push_back(strategy);
    }
    void remove_strategy(const std::string& strategy_name) override {
        strategies_.erase(std::remove_if(strategies_.begin(), strategies_.end(),
            [&strategy_name](const std::shared_ptr<Strategy>& strat) {
                return strat->get_name() == strategy_name;
            }), strategies_.end());     
    }
    std::vector<std::shared_ptr<Strategy>> get_strategies() const override {
        return strategies_;
    }
    void set_data_handler(std::shared_ptr<DataHandler> data_handler) override {
        data_handler_ = data_handler;
    }
    void set_execution_handler(std::shared_ptr<ExecutionHandler> execution_handler) override {
        execution_handler_ = execution_handler;         
    }
    void set_portfolio(std::shared_ptr<Portfolio> portfolio) override {
        portfolio_ = portfolio;
    }
    void set_risk_management(std::shared_ptr<RiskManagement> risk_management) override {
        risk_management_ = risk_management;
    }
    void set_logger(std::shared_ptr<Logger> logger) override {
        logger_ = logger;
    }
    void set_config(const Config& config) override {
        config_ = config;
    }
    Config get_config() const override {
        return config_;
    }
    void reset() override {         
        Logger::get_instance().info("Resetting SimpleBacktestEngine");
        // 重置逻辑
    }
};  
#endif // BACKTEST_ENGINE_H

