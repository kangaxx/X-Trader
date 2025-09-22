#pragma once

#include "trader_api.h"
#include "ThostFtdcTraderApi.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>


class ctp_trader : public trader_api, public CThostFtdcTraderSpi
{
public:
	ctp_trader(std::map<std::string, std::string>& config, std::set<std::string>& contracts);
	virtual ~ctp_trader();

public:
	virtual void release() override;
	virtual orderref_t insert_order(eOrderFlag order_flag, const std::string& contract, eDirOffset dir_offset, double price, int volume) override;
	virtual bool cancel_order(const orderref_t order_ref) override;
	virtual std::string get_trading_day() const override;
	virtual void get_trader_data(InstrumentMap& i_map, PositionMap& p_map, OrderMap& o_map) override;
	virtual void get_account() override;

private:
	void req_auth();
	void req_user_login();
	void req_user_logout();
	void req_settlement_confirm();
	void req_qry_instrument();
	void req_qry_position();
	void req_qry_order();
	inline uint32_t generate_reqid() { return _req_id.fetch_add(1); }

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
	///�ͻ�����֤��Ӧ
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///��¼������Ӧ
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///�ǳ�������Ӧ
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///Ͷ���߽�����ȷ����Ӧ
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///�����ѯ��Լ��Ӧ
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///�����ѯͶ���ֲ߳���Ӧ
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///�����ѯ�ʽ��˻���Ӧ
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///�����ѯ������Ӧ
	virtual void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///����¼��������Ӧ
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///��������������Ӧ
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequorder_id, bool bIsLast) override;
	///����֪ͨ
	virtual void OnRtnOrder(CThostFtdcOrderField* pOrder) override;
	///�ɽ�֪ͨ
	virtual void OnRtnTrade(CThostFtdcTradeField* pTrade) override;
	///����¼�����ر�
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo) override;
	///������������ر�
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo) override;


private:
	inline void to_ctp_dirOffset(const eDirOffset& local, CThostFtdcInputOrderField& ctp)
	{
		switch (local)
		{
		case eDirOffset::BuyOpen:
			ctp.Direction = THOST_FTDC_D_Buy;
			ctp.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
		case eDirOffset::SellOpen:
			ctp.Direction = THOST_FTDC_D_Sell;
			ctp.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
		case eDirOffset::BuyClose:
			ctp.Direction = THOST_FTDC_D_Buy;
			ctp.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
			break;
		case eDirOffset::SellClose:
			ctp.Direction = THOST_FTDC_D_Sell;
			ctp.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
			break;
		case eDirOffset::BuyCloseToday:
			ctp.Direction = THOST_FTDC_D_Buy;
			ctp.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
			break;
		case eDirOffset::SellCloseToday:
			ctp.Direction = THOST_FTDC_D_Sell;
			ctp.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
			break;
		case eDirOffset::BuyCloseYesterday:
			ctp.Direction = THOST_FTDC_D_Buy;
			ctp.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
			break;
		case eDirOffset::SellCloseYesterday:
			ctp.Direction = THOST_FTDC_D_Sell;
			ctp.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
			break;
		}
	}

	inline void to_local_dirOffset(const CThostFtdcOrderField& ctp, eDirOffset& local)
	{
		auto Direction = ctp.Direction;
		auto OffsetFlag = ctp.CombOffsetFlag[0];
		switch (Direction)
		{
		case THOST_FTDC_D_Buy:
			if (OffsetFlag == THOST_FTDC_OF_Open)
			{
				local = eDirOffset::BuyOpen;
			}
			else if (OffsetFlag == THOST_FTDC_OF_Close)
			{
				local = eDirOffset::BuyClose;
			}
			else if (OffsetFlag == THOST_FTDC_OF_CloseToday)
			{
				local = eDirOffset::BuyCloseToday;
			}
			else if (OffsetFlag == THOST_FTDC_OF_CloseYesterday)
			{
				local = eDirOffset::BuyCloseYesterday;
			}
			break;
		case THOST_FTDC_D_Sell:
			if (OffsetFlag == THOST_FTDC_OF_Open)
			{
				local = eDirOffset::SellOpen;
			}
			else if (OffsetFlag == THOST_FTDC_OF_Close)
			{
				local = eDirOffset::SellClose;
			}
			else if (OffsetFlag == THOST_FTDC_OF_CloseToday)
			{
				local = eDirOffset::SellCloseToday;
			}
			else if (OffsetFlag == THOST_FTDC_OF_CloseYesterday)
			{
				local = eDirOffset::SellCloseYesterday;
			}
			break;
		}
	}

	inline void to_local_orderSubmitStatus(const char& ctp, eOrderSubmitStatus& local)
	{
		switch (ctp)
		{
		case THOST_FTDC_OSS_InsertSubmitted:///�Ѿ��ύ
			local = eOrderSubmitStatus::InsertSubmitted; break;
		case THOST_FTDC_OSS_CancelSubmitted:///�����Ѿ��ύ
			local = eOrderSubmitStatus::CancelSubmitted; break;
		case THOST_FTDC_OSS_ModifySubmitted:///�޸��Ѿ��ύ
			local = eOrderSubmitStatus::ModifySubmitted; break;
		case THOST_FTDC_OSS_Accepted:///�Ѿ�����
			local = eOrderSubmitStatus::Accepted; break;
		case THOST_FTDC_OSS_InsertRejected:///�����Ѿ����ܾ�
			local = eOrderSubmitStatus::InsertRejected; break;
		case THOST_FTDC_OSS_CancelRejected:///�����Ѿ����ܾ�
			local = eOrderSubmitStatus::CancelRejected; break;
		case THOST_FTDC_OSS_ModifyRejected:///�ĵ��Ѿ����ܾ�
			local = eOrderSubmitStatus::ModifyRejected; break;
		}
	}

	inline void to_local_orderStatus(const char& ctp, eOrderStatus& local)
	{
		switch (ctp)
		{
		case THOST_FTDC_OST_AllTraded:///ȫ���ɽ�
			local = eOrderStatus::AllTraded; break;
		case THOST_FTDC_OST_PartTradedQueueing:///���ֳɽ����ڶ�����
			local = eOrderStatus::PartTradedQueueing; break;
		case THOST_FTDC_OST_PartTradedNotQueueing:///���ֳɽ����ڶ�����
			local = eOrderStatus::PartTradedNotQueueing; break;
		case THOST_FTDC_OST_NoTradeQueueing:///δ�ɽ����ڶ�����
			local = eOrderStatus::NoTradeQueueing; break;
		case THOST_FTDC_OST_NoTradeNotQueueing:///δ�ɽ����ڶ�����
			local = eOrderStatus::NoTradeNotQueueing; break;
		case THOST_FTDC_OST_Canceled:///����
			local = eOrderStatus::Canceled; break;
		case THOST_FTDC_OST_Unknown:///δ֪
			local = eOrderStatus::Unknown; break;
		case THOST_FTDC_OST_NotTouched:///��δ����
			local = eOrderStatus::NotTouched; break;
		case THOST_FTDC_OST_Touched:///�Ѵ���
			local = eOrderStatus::Touched; break;
		}
	}


private:
	CThostFtdcTraderApi* _td_api;
	std::atomic<uint32_t> _req_id;

	std::string _broker_id;
	std::string _user_id;
	std::string _password;
	std::string _app_id;
	std::string _auth_code;
	std::string _user_product_Info;

	uint32_t _front_id;
	uint32_t _session_id;
	std::atomic<uint32_t> _order_ref;

	InstrumentMap _instrument_map{};
	OrderMap _order_map{};
	PositionMap _position_map{};
	std::set<std::string> _contracts{};


private:
	inline void print(CThostFtdcRspInfoField* p)
	{
		printf("\n");
		printf("ErrorID = %d\n", p->ErrorID);///�������
		printf("ErrorMsg = %s\n", p->ErrorMsg);///������Ϣ
		printf("\n");
	}

	inline void print(CThostFtdcInvestorPositionField* p)
	{
		printf("\n");
		printf("PosiDirection = %s\n", getPosiDirectionString(p->PosiDirection).c_str());///�ֲֶ�շ���
		printf("PositionDate = %s\n", getPositionDateString(p->PositionDate).c_str());///�ֲ�����
		printf("YdPosition = %d\n", p->YdPosition);///���ճֲ�
		printf("Position = %d\n", p->Position);///���ճֲ�
		printf("LongFrozen = %d\n", p->LongFrozen);///��ͷ����
		printf("ShortFrozen = %d\n", p->ShortFrozen);///��ͷ����
		//printf("LongFrozenAmount = %.2f\n", p->LongFrozenAmount);///���ֶ�����
		//printf("ShortFrozenAmount = %.2f\n", p->ShortFrozenAmount);///���ֶ�����
		////printf("OpenVolume = %d\n", p->OpenVolume);///������
		//printf("CloseVolume = %d\n", p->CloseVolume);///ƽ����
		//printf("OpenAmount = %.2f\n", p->OpenAmount);///���ֽ��
		//printf("CloseAmount = %.2f\n", p->CloseAmount);///ƽ�ֽ��
		printf("PositionCost = %.2f\n", p->PositionCost);///�ֲֳɱ�
		//printf("PreMargin = %.2f\n", p->PreMargin);///�ϴ�ռ�õı�֤��
		//printf("UseMargin = %.2f\n", p->UseMargin);///ռ�õı�֤��
		//printf("FrozenMargin = %.2f\n", p->FrozenMargin);///����ı�֤��
		//printf("FrozenCash = %.2f\n", p->FrozenCash);///������ʽ�
		//printf("FrozenCommission = %.2f\n", p->FrozenCommission);///�����������
		//printf("CashIn = %.2f\n", p->CashIn);///�ʽ���
		printf("Commission = %.2f\n", p->Commission);///������
		printf("CloseProfit = %.2f\n", p->CloseProfit);///ƽ��ӯ��
		printf("PositionProfit = %.2f\n", p->PositionProfit);///�ֲ�ӯ��
		printf("PreSettlementPrice = %.2f\n", p->PreSettlementPrice);///�ϴν����
		printf("SettlementPrice = %.2f\n", p->SettlementPrice);///���ν����
		//printf("TradingDay = %s\n", p->TradingDay);///������
		printf("SettlementID = %d\n", p->SettlementID);///������
		printf("OpenCost = %.2f\n", p->OpenCost);///���ֳɱ�
		//printf("ExchangeMargin = %.2f\n", p->ExchangeMargin);///��������֤��
		//printf("CloseProfitByDate = %.2f\n", p->CloseProfitByDate);///���ն���ƽ��ӯ��
		//printf("CloseProfitByTrade = %.2f\n", p->CloseProfitByTrade);///��ʶԳ�ƽ��ӯ��
		printf("TodayPosition = %d\n", p->TodayPosition);///���ճֲ�
		//printf("MarginRateByMoney = %.2f\n", p->MarginRateByMoney);///��֤����
		//printf("MarginRateByVolume = %.2f\n", p->MarginRateByVolume);///��֤����(������)
		printf("ExchangeID = %s\n", p->ExchangeID);///����������
		printf("InstrumentID = %s\n", p->InstrumentID);///��Լ����
		printf("\n");
	}

	inline void print(CThostFtdcOrderField* p)
	{
		print_local_time(); printf("OnRtnOrder\n");
		//printf("BrokerID = %s\n", p->BrokerID);///���͹�˾����
		//printf("InvestorID = %s\n", p->InvestorID);///Ͷ���ߴ���
		printf("OrderRef = %s\n", p->OrderRef);///��������
		//printf("UserID = %s\n", p->UserID);///�û�����
		//printf("OrderPriceType = %s\n", getOrderPriceTypeString(p->OrderPriceType).c_str());///�����۸�����
		printf("Direction = %s\n", getDirectionString(p->Direction).c_str());///��������
		printf("CombOffsetFlag[0] = %s\n", getOffsetFlagString(p->CombOffsetFlag[0]).c_str());///��Ͽ�ƽ��־
		printf("LimitPrice = %.2f\n", p->LimitPrice);///�۸�
		
		//printf("TimeCondition = %s\n", getTimeConditionString(p->TimeCondition).c_str());///��Ч������
		//printf("GTDDate = %s\n", p->GTDDate);///GTD����
		//printf("VolumeCondition = %s\n", getVolumeConditionString(p->VolumeCondition).c_str());///�ɽ�������
		//printf("MinVolume = %d\n", p->MinVolume);///��С�ɽ���
		//printf("ContingentCondition = %s\n", getContingentConditionString(p->ContingentCondition).c_str());///��������
		//printf("StopPrice = %.2f\n", p->StopPrice);///ֹ���
		//printf("ForceCloseReason = %s\n", getForceCloseReasonString(p->ForceCloseReason).c_str());///ǿƽԭ��
		//printf("	IsAutoSuspend = %d\n", p->IsAutoSuspend);///�Զ������־
		printf("RequestID = %d\n", p->RequestID);///������
		printf("OrderLocalID = %s\n", p->OrderLocalID);///���ر������
		printf("OrderSysID = %s\n", p->OrderSysID);///�������
		printf("OrderSubmitStatus = %s\n", getOrderSubmitStatusString(p->OrderSubmitStatus).c_str());///�����ύ״̬
		//printf("TradingDay = %s\n", p->TradingDay);///������
		//printf("SettlementID = %d\n", p->SettlementID);///������
		printf("OrderStatus = %s\n", getOrderStatusString(p->OrderStatus).c_str());///����״̬
		printf("StatusMsg = %s\n", p->StatusMsg);///״̬��Ϣ
		printf("VolumeTotalOriginal = %d\n", p->VolumeTotalOriginal);///����
		printf("VolumeTraded = %d\n", p->VolumeTraded);///��ɽ�����
		printf("VolumeTotal = %d\n", p->VolumeTotal);///ʣ������
		//printf("	InsertDate = %s\n", p->InsertDate);///��������
		printf("InsertTime = %s\n", p->InsertTime);///ί��ʱ��
		//printf("ActiveTime = %s\n", p->ActiveTime);///����ʱ��
		//printf("SuspendTime = %s\n", p->SuspendTime);///����ʱ��
		//printf("UpdateTime = %s\n", p->UpdateTime);///����޸�ʱ��
		printf("CancelTime = %s\n", p->CancelTime);///����ʱ��
		printf("FrontID = %d\n", p->FrontID);///ǰ�ñ��
		printf("SessionID = %d\n", p->SessionID);///�Ự���
		printf("UserProductInfo = %s\n", p->UserProductInfo);///�û��˲�Ʒ��Ϣ		
		//printf("UserForceClose = %d\n", p->UserForceClose);///�û�ǿ����־
		printf("InstrumentID = %s\n", p->InstrumentID);///��Լ����
		printf("ExchangeID = %s\n", p->ExchangeID);///����������
		//printf("ExchangeInstID = %s\n", p->ExchangeInstID);///��Լ�ڽ������Ĵ���
		printf("\n");
	}

	inline void print(CThostFtdcTradeField* p)
	{
		print_local_time();
		printf("OnRtnTrade\n");
		//printf("BrokerID = %s\n", p->BrokerID);///���͹�˾����
		//printf("InvestorID = %s\n", p->InvestorID);///Ͷ���ߴ���
		printf("OrderRef = %s\n", p->OrderRef);///��������
		//printf("UserID = %s\n", p->UserID);///�û�����
		printf("TradeID = %s\n", p->TradeID);///�ɽ����
		printf("Direction = %s\n", getDirectionString(p->Direction).c_str());///��������
		printf("OrderSysID = %s\n", p->OrderSysID);///�������
		printf("OffsetFlag = %s\n", getOffsetFlagString(p->OffsetFlag).c_str());///��ƽ��־
		printf("Price = %.2f\n", p->Price);///�۸�
		printf("Volume = %d\n", p->Volume);///����
		//printf("TradeDate = %s\n", p->TradeDate);///�ɽ�����
		printf("TradeTime = %s\n", p->TradeTime);///�ɽ�ʱ��
		printf("PriceSource = %s\n", getPriceSourceString(p->PriceSource).c_str());///�ɽ�����Դ
		printf("OrderLocalID = %s\n", p->OrderLocalID);///���ر������
		//printf("TradingDay = %s\n", p->TradingDay);///������
		printf("SettlementID = %d\n", p->SettlementID);///������
		printf("InstrumentID = %s\n", p->InstrumentID);///��Լ����
		//printf("ExchangeInstID = %s\n", p->ExchangeInstID);///��Լ�ڽ������Ĵ���
		printf("\n");
	}

	void print_local_time()
	{
		// ��ȡ��ǰʱ���  
		auto now = std::chrono::system_clock::now();

		// ת��Ϊtime_t�Ի�ȡtm�ṹ��  
		std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
		std::tm* now_tm = std::localtime(&now_time_t);

		// ��ȡСʱ�����Ӻ���  
		int hours = now_tm->tm_hour;
		int minutes = now_tm->tm_min;
		int seconds = now_tm->tm_sec;

		// ��ȡ����  
		auto duration = now.time_since_epoch();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

		// ��ӡʱ��  
		std::cout << std::setfill('0')
			<< std::setw(2) << hours << ":"
			<< std::setw(2) << minutes << ":"
			<< std::setw(2) << seconds << ":"
			<< std::setw(3) << millis << " ";
	}

	std::map<char, std::string> PosiDirectionToString =
	{
		{THOST_FTDC_PD_Net, "Net"},
		{THOST_FTDC_PD_Long, "Long"},
		{THOST_FTDC_PD_Short, "Short"}
	};

	inline std::string getPosiDirectionString(char t)
	{
		return PosiDirectionToString[t];
	}

	std::map<char, std::string> PositionDateToString =
	{
		{THOST_FTDC_PSD_Today, "Today"},
		{THOST_FTDC_PSD_History, "History"}
	};

	inline std::string getPositionDateString(char t)
	{
		return PositionDateToString[t];
	}

	std::map<char, std::string> OrderPriceTypeToString =
	{
		{THOST_FTDC_OPT_AnyPrice, "AnyPrice"},
		{THOST_FTDC_OPT_LimitPrice, "LimitPrice"},
		{THOST_FTDC_OPT_BestPrice, "BestPrice"},
		{THOST_FTDC_OPT_LastPrice, "LastPrice"}
	};

	inline std::string getOrderPriceTypeString(char t)
	{
		return OrderPriceTypeToString[t];
	}

	std::map<char, std::string> DirectionToString =
	{
		{THOST_FTDC_D_Buy, "Buy"},
		{THOST_FTDC_D_Sell, "Sell"}
	};

	inline std::string getDirectionString(char t)
	{
		return DirectionToString[t];
	}

	std::map<char, std::string> OffsetFlagToString =
	{
		{THOST_FTDC_OF_Open, "Open"},
		{THOST_FTDC_OF_Close, "Close"},
		{THOST_FTDC_OF_ForceClose, "ForceClose"},
		{THOST_FTDC_OF_CloseToday, "CloseToday"},
		{THOST_FTDC_OF_CloseYesterday, "CloseYesterday"},
		{THOST_FTDC_OF_ForceOff, "ForceOff"},
		{THOST_FTDC_OF_LocalForceClose, "LocalForceClose"}
	};

	inline std::string getOffsetFlagString(char t)
	{
		return OffsetFlagToString[t];
	}

	std::map<char, std::string> TimeConditionToString =
	{
		{THOST_FTDC_TC_IOC, "IOC"},
		{THOST_FTDC_TC_GFS, "GFS"},
		{THOST_FTDC_TC_GFD, "GFD"},
		{THOST_FTDC_TC_GFA, "GFA"}
	};

	inline std::string getTimeConditionString(char t)
	{
		return TimeConditionToString[t];
	}

	std::map<char, std::string> VolumeConditionToString =
	{
		{THOST_FTDC_VC_AV, "AV"},
		{THOST_FTDC_VC_MV, "MV"},
		{THOST_FTDC_VC_CV, "CV"}
	};

	inline std::string getVolumeConditionString(char t)
	{
		return VolumeConditionToString[t];
	}

	std::map<char, std::string> ContingentConditionToString =
	{
		{THOST_FTDC_CC_Immediately, "Immediately"},
		{THOST_FTDC_CC_Touch, "Touch"},
		{THOST_FTDC_CC_TouchProfit, "TouchProfit"}
	};

	inline std::string getContingentConditionString(char t)
	{
		return ContingentConditionToString[t];
	}

	std::map<char, std::string> ForceCloseReasonToString =
	{
		{THOST_FTDC_FCC_NotForceClose, "NotForceClose"},
		{THOST_FTDC_FCC_LackDeposit, "LackDeposit"},
		{THOST_FTDC_FCC_Violation, "Violation"},
		{THOST_FTDC_FCC_Other, "Other"},
		{THOST_FTDC_FCC_PersonDeliv, "PersonDeliv"},
		{THOST_FTDC_FCC_Notverifycapital, "Notverifycapital"}
	};

	inline std::string getForceCloseReasonString(char t)
	{
		return ForceCloseReasonToString[t];
	}

	std::map<char, std::string> OrderSubmitStatusToString =
	{
		{THOST_FTDC_OSS_InsertSubmitted, "InsertSubmitted"},
		{THOST_FTDC_OSS_CancelSubmitted, "CancelSubmitted"},
		{THOST_FTDC_OSS_ModifySubmitted, "ModifySubmitted"},
		{THOST_FTDC_OSS_Accepted, "Accepted"},
		{THOST_FTDC_OSS_InsertRejected, "InsertRejected"},
		{THOST_FTDC_OSS_CancelRejected, "CancelRejected"},
		{THOST_FTDC_OSS_ModifyRejected, "ModifyRejected"}
	};

	inline std::string getOrderSubmitStatusString(char t)
	{
		return OrderSubmitStatusToString[t];
	}

	std::map<char, std::string> OrderStatusToString =
	{
		{THOST_FTDC_OST_AllTraded, "AllTraded"},
		{THOST_FTDC_OST_PartTradedQueueing, "PartTradedQueueing"},
		{THOST_FTDC_OST_PartTradedNotQueueing, "PartTradedNotQueueing"},
		{THOST_FTDC_OST_NoTradeQueueing, "NoTradeQueueing"},
		{THOST_FTDC_OST_NoTradeNotQueueing, "NoTradeNotQueueing"},
		{THOST_FTDC_OST_Canceled, "Canceled"},
		{THOST_FTDC_OST_Unknown, "Unknown"},
		{THOST_FTDC_OST_NotTouched, "NotTouched"},
		{THOST_FTDC_OST_Touched, "Touched"}
	};

	inline std::string getOrderStatusString(char t)
	{
		return OrderStatusToString[t];
	}

	std::map<char, std::string> PriceSourceToString =
	{
		{THOST_FTDC_PSRC_LastPrice, "LastPrice"},
		{THOST_FTDC_PSRC_Buy, "Buy"},
		{THOST_FTDC_PSRC_Sell, "Sell"},
		{THOST_FTDC_PSRC_OTC, "OTC"}
	};

	inline std::string getPriceSourceString(char t)
	{
		return PriceSourceToString[t];
	}

	inline void print_position()
	{
		printf("\n");
		for (auto& it : _position_map)
		{
			if (it.second.long_.position)
			{
				printf("instrument_id = %s\n", it.first.c_str());
				printf("long_.position = %d\n", it.second.long_.position);
				printf("long_.today_position = %d\n", it.second.long_.today_position);
				printf("long_.his_position = %d\n", it.second.long_.his_position);
				printf("long_.closeable = %d\n", it.second.long_.closeable);
				printf("long_.his_closeable = %d\n", it.second.long_.his_closeable);
				printf("long_.today_closeable = %d\n", it.second.long_.today_closeable);
				printf("long_.avg_open_cost = %.2f\n", it.second.long_.avg_open_cost);
				printf("long_.avg_posi_cost = %.2f\n", it.second.long_.avg_posi_cost);
				printf("long_.open_no_trade = %d\n", it.second.long_.open_no_trade);
				printf("\n");
			}
			
			if (it.second.short_.position)
			{
				printf("instrument_id = %s\n", it.first.c_str());
				printf("short_.position = %d\n", it.second.short_.position);
				printf("short_.today_position = %d\n", it.second.short_.today_position);
				printf("short_.his_position = %d\n", it.second.short_.his_position);
				printf("short_.closeable = %d\n", it.second.short_.closeable);
				printf("short_.his_closeable = %d\n", it.second.short_.his_closeable);
				printf("short_.today_closeable = %d\n", it.second.short_.today_closeable);
				printf("short_.avg_open_cost = %.2f\n", it.second.short_.avg_open_cost);
				printf("short_.avg_posi_cost = %.2f\n", it.second.short_.avg_posi_cost);
				printf("short_.open_no_trade = %d\n", it.second.short_.open_no_trade);
				printf("\n");
			}
		}
		printf("\n");
	}
};
