#pragma once

#include "market_api.h"
#include "ThostFtdcMdApi.h"


class ctp_market : public market_api, public CThostFtdcMdSpi
{
public:
	ctp_market(std::map<std::string, std::string>& config, std::set<std::string>& contracts);
	virtual ~ctp_market();	
	virtual void release() override;

private:
	void req_user_login();
	void subscribe();
	void unsubscribe();

public:
	///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
	virtual void OnFrontConnected() override;
	///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
	///@param nReason ����ԭ��
	///        0x1001 �����ʧ��
	///        0x1002 ����дʧ��
	///        0x2001 ����������ʱ
	///        0x2002 ��������ʧ��
	///        0x2003 �յ�������
	virtual void OnFrontDisconnected(int nReason) override;
	///��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	///�ǳ�������Ӧ
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	///��������Ӧ��
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	///ȡ����������Ӧ��
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	///�������֪ͨ
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) override;

private:
	CThostFtdcMdApi* _md_api;
	std::string _broker_id;
	std::string _user_id;
	std::string _password;

	int _req_id;
	std::set<std::string> _contracts;
	std::unordered_map<std::string, CThostFtdcDepthMarketDataField> _previous_tick_map{};
};
