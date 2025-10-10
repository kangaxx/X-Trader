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
#include <array>
#include <set>
#include <list>
#include <deque>        
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cfloat>
#include <climits>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cctype>       
#include <locale>
#include <codecvt>
#include <fstream>
#include <streambuf>
#include <regex>
class ATRIndicator {
public:
    ATRIndicator(int period) : period_(period) {
        if (period_ <= 0) {
            throw std::invalid_argument("Period must be a positive integer.");
        }
    }
    void addDataPoint(double high, double low, double close) {
        if (!data_points_.empty()) {
            double prev_close = data_points_.back().close;
            double tr = std::max({high - low, std::abs(high - prev_close), std::abs(low - prev_close)});
            true_ranges_.push_back(tr);
        }
        data_points_.push_back({high, low, close});
        if (true_ranges_.size() >= period_) {
            calculateATR();
        }
    }
    double getATR() const {
        if (atr_values_.empty()) {
            throw std::runtime_error("Not enough data to calculate ATR.");
        }
        return atr_values_.back();
    }
private:
    struct DataPoint {
        double high;
        double low;
        double close;
    };
    int period_;
    std::vector<DataPoint> data_points_;
    std::vector<double> true_ranges_;
    std::vector<double> atr_values_;
    void calculateATR() {
        if (atr_values_.empty()) {
            double sum_tr = std::accumulate(true_ranges_.end() - period_, true_ranges_.end(), 0.0);
            atr_values_.push_back(sum_tr / period_);
        } else {
            double prev_atr = atr_values_.back();
            double current_tr = true_ranges_.back();
            double current_atr = (prev_atr * (period_ - 1) + current_tr) / period_;
            atr_values_.push_back(current_atr);
        }
    }
};
#endif // ATR_INDICATOR_H
