//这是一个atr指标的实现文件,用于计算平均真实波幅（ATR）指标,该类包含计算ATR的完整逻辑,并提供了获取ATR值的接口.不需要其他文件的支持.
//
#ifndef ATR_INDICATOR_H
#define ATR_INDICATOR_H
#include <vector>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <utility>
#include <type_traits>
#include <iterator>
class ATRIndicator {
public:
    ATRIndicator(int period) : _period(period) {
        if (_period <= 0) {
            throw std::invalid_argument("Period must be a positive integer.");
        }
    }
    void addDataPoint(double high, double low, double close) {
        if (!_data_points.empty()) {
            double prev_close = _data_points.back().close;
            double tr = std::max({high - low, std::abs(high - prev_close), std::abs(low - prev_close)});
            _true_ranges.push_back(tr);
        }
        _data_points.push_back({high, low, close});
        if (_true_ranges.size() >= _period) {
            calculateATR();
        }
    }
    double getATR() const {
        if (_atr_values.empty()) {
            throw std::runtime_error("Not enough data to calculate ATR.");
        }
        return _atr_values.back();
    }

    bool isReady() const {
        return _atr_values.size() >= 1;
    }
private:
    struct DataPoint {
        double high;
        double low;
        double close;
    };
    int _period;
    std::vector<DataPoint> _data_points;
    std::vector<double> _true_ranges;
    std::vector<double> _atr_values;
    void calculateATR() {
        if (_atr_values.empty()) {
            double sum_tr = std::accumulate(_true_ranges.end() - _period, _true_ranges.end(), 0.0);
            _atr_values.push_back(sum_tr / _period);
        } else {
            double prev_atr = _atr_values.back();
            double current_tr = _true_ranges.back();
            double current_atr = (prev_atr * (_period - 1) + current_tr) / _period;
            _atr_values.push_back(current_atr);
        }
    }
};
#endif // ATR_INDICATOR_H
