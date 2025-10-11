#pragma once



#include <string>
#include <vector>
class LongMAIndicator {
public:
    LongMAIndicator(int period);
    ~LongMAIndicator();
    void addPrice(double price);
    double getLongMA() const;
    bool isReady() const;
    std::vector<double>& getDataPoints() const;
    int getPeriod() const;
private:
    int _period;
    std::vector<double> prices;
    double sum;
    int count;
    double longMA;
    void calculateLongMA();
    std::vector<double> _dataPoints;
    void clear();
};
