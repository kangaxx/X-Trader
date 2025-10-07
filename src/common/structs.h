#pragma once
#include <string>

struct DTBarDatas {
    std::string date_str;   // 日期字符串
    time_t datetime;        // 时间戳
    double open;            // 开盘价
    double high;            // 最高价
    double low;             // 最低价
    double close;           // 收盘价
    int volume;             // 成交量
};