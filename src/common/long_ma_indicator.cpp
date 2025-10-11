#include "long_ma_indicator.h"
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iostream>
LongMAIndicator::LongMAIndicator(int period) : _period(period) {
    if (_period <= 0) {
        throw std::invalid_argument("Period must be a positive integer");
    }
}

void LongMAIndicator::addDataPoint(double value) {
    _dataPoints.push_back(value);
    if (_dataPoints.size() > _period) {
        _dataPoints.erase(_dataPoints.begin());
    }
}
double LongMAIndicator::getValue() const {
    if (_dataPoints.size() < _period) {
        throw std::runtime_error("Not enough data points to calculate the moving average");
    }
    double sum = std::accumulate(_dataPoints.end() - _period, _dataPoints.end(), 0.0);
    return sum / _period;
}
bool LongMAIndicator::isReady() const {
    return _dataPoints.size() >= _period;
}

int LongMAIndicator::getPeriod() const { 
    return _period;
}

const std::vector<double>& LongMAIndicator::getDataPoints() const {
    return _dataPoints;
}

void LongMAIndicator::clear() {
    _dataPoints.clear();
}

void LongMAIndicator::printDataPoints() const {
    std::cout << "Data Points: ";
    for (const auto& point : _dataPoints) {
        std::cout << point << " ";
    }
    std::cout << std::endl;
}

double LongMAIndicator::calculateSMA(int period) const {
    if (_dataPoints.size() < period) {
        throw std::runtime_error("Not enough data points to calculate the SMA");
    }
    double sum = std::accumulate(_dataPoints.end() - period, _dataPoints.end(), 0.0);
    return sum / period;
}
