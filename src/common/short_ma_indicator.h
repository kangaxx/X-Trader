#pragma once
#include <string>
#include <vector>

class ShortMAIndicator {
public:
    ShortMAIndicator(int period = 5);
    ~ShortMAIndicator();
    void addPrice(double price);
    double getShortMA() const;
    bool isReady() const;
    std::vector<double>& getDataPoints();
    int getPeriod() const;
private:
    int _period;
    std::vector<double> _prices;
    double _sum;
    double _shortMA;
    std::vector<double> _dataPoints;
    void clear();
};
