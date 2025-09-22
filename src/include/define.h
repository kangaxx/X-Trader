#pragma once

#include <stdint.h>
#include <set>
#include <vector>
#include <tuple>
#include <map>
#include<unordered_map>
#include <filesystem>
#include <atomic>
#include <string>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>


#if defined(WIN32)
#pragma warning(disable:4996)
#endif

#ifndef EXPORT_FLAG
#ifdef _MSC_VER
#define EXPORT_FLAG __declspec(dllexport)
#else
#define EXPORT_FLAG __attribute__((__visibility__("default")))
#endif
#endif

#ifndef PORTER_FLAG
#ifdef _MSC_VER
#define PORTER_FLAG _cdecl
#else
#define PORTER_FLAG 
#endif
#endif

using orderref_t = uint64_t;///�����������
constexpr orderref_t null_orderref = 0x0LLU;

using stratid_t = uint8_t;///���Ա������


using  DateType = char[9]; ///��������
using  TimeType = char[9]; ///ʱ������
using  ExchangeIDType = char[9]; ///��������������
using  InstrumentIDType = char[81]; ///��Լ��������
using  BrokerIDType = char[11]; ///���͹�˾��������
using  InvestorIDType = char[13]; ///Ͷ���ߴ�������
using  UserIDType = char[16]; ///�û���������
using  OrderRefType = char[13]; ///������������
using  CombOffsetFlagType = char[5]; ///��Ͽ�ƽ��־����
using  CombHedgeFlagType = char[5]; ///���Ͷ���ױ���־����
using  OrderLocalIDType = char[13]; ///���ر����������
using  OrderSysIDType = char[21]; ///�����������
using  ProductInfoType = char[11]; ///��Ʒ��Ϣ����
using  ErrorMsgType = char[81]; ///������Ϣ����
using  TradeIDType = char[21]; ///�ɽ��������
using  ClientIDType = char[11];///���ױ�������


inline void send2wecom(const char* text)
{
	char command[512];
	std::string curl = "curl ";
	std::string webhook = R"("����΢��Ⱥ������webhook��ַ")";
	std::string param = R"( -H "Content-Type: application/json" -d "{\"msgtype\": \"text\",\"text\": {\"content\": \"%s\"}}")";
	sprintf(command, (curl + webhook + param).c_str(), text);

	system(command);
}

inline const char* reqRtnReason(int rtn)
{
	switch (rtn)
	{
	case 0:///���ͳɹ�
		return "Sent successfully"; break;
	case -1:///������ԭ����ʧ��
		return "Failed to send"; break;
	case -2:///δ���������������������
		return "Unprocessed requests exceeded the allowed volume"; break;
	case -3:///ÿ�뷢��������������
		return "The number of requests sent per second exceeds the allowed volume"; break;
	default:///δ֪
		return "Unknown";
	}
}

inline const char* nReason2str(int nReason)
{
	switch (nReason)
	{
	case 0x1001:
		return "�����ʧ��";
	case 0x1002:
		return "����дʧ��";
	case 0x2001:
		return "����������ʱ";
	case 0x2002:
		return "��������ʧ��";
	case 0x2003:
		return "�յ�������";
	default:
		return "Unknown";
	}
}
