#include "short_ma_indicator.h"
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <cmath>

//在这里实现ShortMAIndicator类的成员函数
ShortMAIndicator::ShortMAIndicator(int period) : _period(period) {
    if (period <= 0) {
        throw std::invalid_argument("Period must be a positive integer");
    }
    _prices.reserve(period);
}

ShortMAIndicator::~ShortMAIndicator() {
    // 析构函数
}   

void ShortMAIndicator::addPrice(double price) {
    if (_prices.size() == _period) {
        _prices.erase(_prices.begin()); // 移除最早的价格
    }
    _prices.push_back(price);
}


double ShortMaIndicator::getShortMA() const {
    if (!isReady()) {
        throw std::runtime_error("Not enough data to calculate Short MA");
    }
    double sum = std::accumulate(_prices.begin(), _prices.end(), 0.0);
    return sum / _period;
}   

bool ShortMAIndicator::isReady() const {
    return _prices.size() == _period;
}

std::vector<double>& ShortMAIndicator::getDataPoints()
{
    // TODO: 在此处插入 return 语句
	return _prices;
}

void ShortMAIndicator::clear() {
    _prices.clear();
}

std::vector<double> ShortMAIndicator::getDataPoints() const {
    return _prices;
}   

//getPeriod函数
int ShortMAIndicator::getPeriod() const {
    return _period;
}





