#pragma once
#include "structs.h"
#include <deque>
#include <vector>
#include <string>

// ATR移动止损算法
// 参数说明：
// bars         - 输入的K线数据（最近N根）
// atr_period   - ATR计算周期
// atr_mult     - ATR乘数（回撤距离）
// entry_price  - 开仓价
// direction    - 1=多头，-1=空头
// 返回：当前移动止损价（多头为trailing_stop，空头为trailing_stop）
double calc_atr_trailing_stop(
    const std::deque<DTBarDatas>& bars,
    int atr_period,
    double atr_mult,
    double entry_price,
    int direction)
{
    if (bars.size() < static_cast<size_t>(atr_period + 1)) return 0.0;

    // 计算ATR
    double atr_sum = 0.0;
    for (size_t i = bars.size() - atr_period; i < bars.size(); ++i) {
        double high = bars[i].high;
        double low = bars[i].low;
        double prev_close = bars[i - 1].close;
        double tr = std::max({ high - low, std::abs(high - prev_close), std::abs(low - prev_close) });
        atr_sum += tr;
    }
    double atr = atr_sum / atr_period;

    // 计算持仓期间极值
    double trailing_extreme = entry_price;
    if (direction == 1) { // 多头
        for (size_t i = bars.size() - atr_period; i < bars.size(); ++i) {
            trailing_extreme = std::max(trailing_extreme, bars[i].high);
        }
        return trailing_extreme - atr_mult * atr;
    }
    else { // 空头
        for (size_t i = bars.size() - atr_period; i < bars.size(); ++i) {
            trailing_extreme = std::min(trailing_extreme, bars[i].low);
        }
        return trailing_extreme + atr_mult * atr;
    }
}