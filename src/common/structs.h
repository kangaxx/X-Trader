#pragma once
#include <string>

struct DTBarDatas {
    std::string date_str;   // �����ַ���
    time_t datetime;        // ʱ���
    double open;            // ���̼�
    double high;            // ��߼�
    double low;             // ��ͼ�
    double close;           // ���̼�
    int volume;             // �ɽ���
};