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


// ���ݼ���ö������
enum class DataLevel {
    Second,     // ���Ӽ�
    Minute,     // ���Ӽ�
    FiveMinute, // 5���Ӽ�
    Hour,       // Сʱ��
    Day         // ���߼�
};