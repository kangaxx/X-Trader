#pragma once
#include "structs.h"
#include <deque>
#include <vector>
#include <string>

// ATR�ƶ�ֹ���㷨
// ����˵����
// bars         - �����K�����ݣ����N����
// atr_period   - ATR��������
// atr_mult     - ATR�������س����룩
// entry_price  - ���ּ�
// direction    - 1=��ͷ��-1=��ͷ
// ���أ���ǰ�ƶ�ֹ��ۣ���ͷΪtrailing_stop����ͷΪtrailing_stop��
double calc_atr_trailing_stop(
    const std::deque<DTBarDatas>& bars,
    int atr_period,
    double atr_mult,
    double entry_price,
    int direction)
{
    if (bars.size() < static_cast<size_t>(atr_period + 1)) return 0.0;

    // ����ATR
    double atr_sum = 0.0;
    for (size_t i = bars.size() - atr_period; i < bars.size(); ++i) {
        double high = bars[i].high;
        double low = bars[i].low;
        double prev_close = bars[i - 1].close;
        double tr = std::max({ high - low, std::abs(high - prev_close), std::abs(low - prev_close) });
        atr_sum += tr;
    }
    double atr = atr_sum / atr_period;

    // ����ֲ��ڼ伫ֵ
    double trailing_extreme = entry_price;
    if (direction == 1) { // ��ͷ
        for (size_t i = bars.size() - atr_period; i < bars.size(); ++i) {
            trailing_extreme = std::max(trailing_extreme, bars[i].high);
        }
        return trailing_extreme - atr_mult * atr;
    }
    else { // ��ͷ
        for (size_t i = bars.size() - atr_period; i < bars.size(); ++i) {
            trailing_extreme = std::min(trailing_extreme, bars[i].low);
        }
        return trailing_extreme + atr_mult * atr;
    }
}