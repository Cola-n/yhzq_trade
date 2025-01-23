#include <iostream>
#include <atomic>
#include <map>
#include <thread>

#if defined _WIN32
#include <windows.h>

#define sleep Sleep
#else
#include <unistd.h>
#endif

#include "atp_trade_api.h"
#include "KDEncodeCli.h"

////////////////////////////////twap交易相关/////////////////////////////////////////
ATPRetCodeType send_order(ATPTradeAPI *client, int market, std::string stock_code, int direction, int order_way, double order_price, double order_quantity);
ATPRetCodeType send_repo_order(ATPTradeAPI *client, std::string code, double price, int qty, int dir);

int query_position_atp(int quert_index = 0);

void query_order_atp(int quert_index = 0);
void query_trade_atp(int quert_index = 0);
void query_money_atp();
void order_insert_atp(int market, std::string stock_code, int direction, int order_way, double order_price, double order_quantity);

std::map<std::string, ut_code_Info> code_info_vec_ptr;
std::vector<std::string> mapKeys;
int agw_flag = 0;

class TwapTradeSample : public TradeTwapClass
{
public:
    // std::vector<APITradeOrderUnit> query_trade_array;// 查询当天成交记录
    // AssetInfo query_asset;// 资金情况
    // std::map<std::string, double> limit_price_map;
    // 查询委托单记录

    TwapTradeSample(twap_para twap_paras, std::string fundname) : TradeTwapClass(twap_paras, fundname) {}
    friend class CustHandler;

public:
    void query_position(int quert_index)
    {
        std::cout << "query real position..." << std::endl;
        query_position_atp(quert_index);
        // 更新  TradeTwapPointer->real_position
    }
    void query_trade(int quert_index)
    {
        std::cout << "query trade..." << std::endl;
        query_trade_atp(quert_index);
    }
    void order_insert(const OrderReqField &order_info)
    {
        std::cout << "order insert..." << std::endl;
        // 更新  TradeTwapPointer->real_position
        order_insert_atp(order_info.market, order_info.stock_code, order_info.direction, order_info.order_way, order_info.order_price, order_info.order_quantity);
    }
};

struct twap_para twap_paras;
struct atp_para atp_paras;
TwapTradeSample *TradeTwapPointer;

void GetMarketDataFunc()
{
    TradeTwapPointer->get_market_data();
}

void InspectFunc()
{
    TradeTwapPointer->StartInspect();
}

void TwapTradeFunc()
{
    TradeTwapPointer->StartTwapTrade();
}

// 定义回报数据分区+序号的全局变量，在收到回报时更新此变量，在断线重连时传入该变量，指示客户端所收到的最大回报序号
std::map<int32_t, int32_t> report_sync;

std::atomic_bool g_connected_flag(false);    // 已连接
std::atomic_bool g_cust_logined_flag(false); // 已登录

std::atomic_bool g_waiting_flag(false); // 等待响应

std::string orig_cl_ord_no;

ATPClientSeqIDType g_client_seq_id = 1; //  客户系统消息号
// ATPClientFeatureCodeType g_client_feature_code = "PC;IIP=NA;IPORT=NA;LIP=10.86.32.149;MAC=0050569376DC;HD=25DDA818-3E2E-4269-8A4F-A0E98EE9B7C1@zhongtian_xyzq_atp;v1.0.0";  // 终端识别码，由券商指定规则

// 回调类，继承自ATPTradeHandler，可以只实现部分回调函数
class CustHandler : public ATPTradeHandler
{
    // 登入回调
    virtual void OnLogin(const std::string &reason)
    {
        std::cout << "OnLogin Recv:" << reason << std::endl;
        g_connected_flag.store(true);
    }

    // 登出回调
    virtual void OnLogout(const std::string &reason)
    {
        std::cout << "OnLogout Recv:" << reason << std::endl;
        g_connected_flag.store(false);
        g_cust_logined_flag.store(false);
    }

    // 连接失败
    virtual void OnConnectFailure(const std::string &reason)
    {
        std::cout << "OnConnectFailure Recv:" << reason << std::endl;
        g_connected_flag.store(false);
        g_cust_logined_flag.store(false);
    }

    // 连接超时
    virtual void OnConnectTimeOut(const std::string &reason)
    {
        std::cout << "OnConnectTimeOut Recv:" << reason << std::endl;
        g_connected_flag.store(false);
        g_cust_logined_flag.store(false);
    }

    // 心跳超时
    virtual void OnHeartbeatTimeout(const std::string &reason)
    {
        std::cout << "OnHeartbeatTimeout Recv:" << reason << std::endl;
        g_connected_flag.store(false);
        g_cust_logined_flag.store(false);
    }

    // 连接关闭
    virtual void OnClosed(const std::string &reason)
    {
        std::cout << "OnClosed Recv:" << reason << std::endl;
        g_connected_flag.store(false);
        g_cust_logined_flag.store(false);
    }

    // 连接结束回调
    virtual void OnEndOfConnection(const std::string &reason)
    {
        std::cout << "OnEndOfConnection Recv:" << reason << std::endl;
        g_waiting_flag.store(false);
    }

    // 客户号登入回调
    virtual void OnRspCustLoginResp(const ATPRspCustLoginRespOtherMsg &cust_login_resp)
    {
        std::cout << "OnRspCustLoginResp Recv:" << static_cast<uint32_t>(cust_login_resp.permisson_error_code) << std::endl;
        if (cust_login_resp.permisson_error_code == 0)
        {
            g_cust_logined_flag.store(true);
            std::cout << "CustLogin Success!" << std::endl;
        }
        else
        {
            std::cout << "CustLogin Fail, permisson_error_code :" << static_cast<uint32_t>(cust_login_resp.permisson_error_code) << std::endl;
        }

        g_waiting_flag.store(false);
    }

    // 客户号登出回调
    virtual void OnRspCustLogoutResp(const ATPRspCustLogoutRespOtherMsg &cust_logout_resp)
    {
        std::cout << "OnRspCustLogoutResp Recv:" << static_cast<uint32_t>(cust_logout_resp.permisson_error_code) << std::endl;
        if (cust_logout_resp.permisson_error_code == 0)
        {
            g_cust_logined_flag.store(false);
            std::cout << "CustLogout Success!" << std::endl;
        }
        else
        {
            std::cout << "CustLogou Fail permisson_error_code :" << static_cast<uint32_t>(cust_logout_resp.permisson_error_code) << std::endl;
        }
        g_waiting_flag.store(false);
    }

    // 订单下达内部响应
    virtual void OnRspOrderStatusInternalAck(const ATPRspOrderStatusAckMsg &order_status_ack)
    {
        // std::cout << " ===order_status_Internal_ack: " << std::endl;
        // std::cout<<TradeTwapPointer->get_current_time_ms()<<std::endl;
        // std::cout << " order_status_Internal_ack : " << std::endl;
        // std::cout << " partition : " << (int32_t)order_status_ack.partition << std::endl;
        // std::cout << " index : " << order_status_ack.index << std::endl;
        // std::cout << " business_type : " << (int32_t)order_status_ack.business_type << std::endl;
        // std::cout << " cl_ord_no : " << order_status_ack.cl_ord_no << std::endl;
        // std::cout << " security_id : " << order_status_ack.security_id << std::endl;
        // std::cout << " market_id : " << order_status_ack.market_id << std::endl;
        // std::cout << " exec_type : " << order_status_ack.exec_type << std::endl;
        // std::cout << " ord_status : " << (int32_t)order_status_ack.ord_status << std::endl;
        // std::cout << " cust_id : " << order_status_ack.cust_id << std::endl;
        // std::cout << " fund_account_id : " << order_status_ack.fund_account_id << std::endl;
        // std::cout << " account_id : " << order_status_ack.account_id << std::endl;
        // std::cout << " price : " << order_status_ack.price / 10000 << std::endl;
        // std::cout << " order_qty : " << order_status_ack.order_qty / 100 << std::endl;
        // std::cout << " leaves_qty : " << order_status_ack.leaves_qty / 100 << std::endl;
        // std::cout << " cum_qty : " << order_status_ack.cum_qty / 100 << std::endl;
        // std::cout << " side : " << order_status_ack.side << std::endl;
        // std::cout << " transact_time : " << order_status_ack.transact_time << std::endl;
        // std::cout << " user_info : " << order_status_ack.user_info << std::endl;
        // std::cout << " order_id : " << order_status_ack.order_id << std::endl;
        // std::cout << " cl_ord_id : " << order_status_ack.cl_ord_id << std::endl;
        // std::cout << " client_seq_id : " << order_status_ack.client_seq_id << std::endl;
        // std::cout << " orig_cl_ord_no : " << order_status_ack.orig_cl_ord_no << std::endl;
        // std::cout << " frozen_trade_value : " << order_status_ack.frozen_trade_value << std::endl;
        // std::cout << " frozen_fee : " << order_status_ack.frozen_fee << std::endl;
        // std::cout << " reject_reason_code : " << order_status_ack.reject_reason_code << std::endl;
        // std::cout << " ord_rej_reason : " << order_status_ack.ord_rej_reason << std::endl;
        // std::cout << " order_type : " << order_status_ack.order_type << std::endl;
        // std::cout << " time_in_force : " << order_status_ack.time_in_force << std::endl;
        // std::cout << " position_effect : " << order_status_ack.position_effect << std::endl;
        // std::cout << " covered_or_uncovered : " << (int32_t)order_status_ack.covered_or_uncovered << std::endl;
        // std::cout << " account_sub_code : " << order_status_ack.account_sub_code << std::endl;

        if (order_status_ack.ord_status == '3' || order_status_ack.ord_status == '4' || order_status_ack.ord_status == '5')
        {
            TradeTwapPointer->cancel_order++;
        }
        else if (order_status_ack.ord_status == '8')
        {
            TradeTwapPointer->scrap_order++;
        }

        if (order_status_ack.reject_reason_code == 28)
        {
            std::cout << " =================================================" << std::endl;
            std::cout << " \033[31m Selling instruction error \033[0m" << std::endl;
            std::cout << " \033[31m the number of available shares for sale for stock" << order_status_ack.security_id << " is insufficient. Please check\033[0m" << std::endl;
            std::cout << " =================================================" << std::endl;
        }
        else if (order_status_ack.reject_reason_code == 27)
        {
            std::cout << " =================================================" << std::endl;
            std::cout << " \033[31m Purchase instruction error\033[0m" << std::endl;
            std::cout << " \033[31m insufficient available funds in account, please check!!!!\033[0m" << std::endl;
            std::cout << " =================================================" << std::endl;
        }
        else if (order_status_ack.reject_reason_code == 100)
        {
            std::cout << " security_id : " << order_status_ack.security_id << "  ";
            std::cout << " ord_rej_reason : " << order_status_ack.ord_rej_reason << std::endl;
        }

        // 保存回报分区号、序号，用于断线重连时指定已收到最新回报序号
        report_sync[order_status_ack.partition] = order_status_ack.index;
    }

    // 订单下达交易所确认
    virtual void OnRspOrderStatusAck(const ATPRspOrderStatusAckMsg &order_status_ack)
    {
        // std::cout << " ====order_status_ack:" << std::endl;
        // std::cout<<TradeTwapPointer->get_current_time_ms()<<std::endl;
        // std::cout << " order_status_ack : " << std::endl;
        // std::cout << " partition : " << (int32_t)order_status_ack.partition << std::endl;
        // std::cout << " index : " << order_status_ack.index << std::endl;
        // std::cout << " business_type : " << (int32_t)order_status_ack.business_type << std::endl;
        // std::cout << " cl_ord_no : " << order_status_ack.cl_ord_no << std::endl;
        // std::cout << " security_id : " << order_status_ack.security_id << std::endl;
        // std::cout << " market_id : " << order_status_ack.market_id << std::endl;
        // std::cout << " exec_type : " << order_status_ack.exec_type << std::endl;
        // std::cout << " ord_status : " << (int32_t)order_status_ack.ord_status << std::endl;
        // std::cout << " cust_id : " << order_status_ack.cust_id << std::endl;
        // std::cout << " fund_account_id : " << order_status_ack.fund_account_id << std::endl;
        // std::cout << " account_id : " << order_status_ack.account_id << std::endl;
        // std::cout << " price : " << order_status_ack.price / 10000 << std::endl;
        // std::cout << " order_qty : " << order_status_ack.order_qty / 100 << std::endl;
        // std::cout << " leaves_qty : " << order_status_ack.leaves_qty / 100 << std::endl;
        // std::cout << " cum_qty : " << order_status_ack.cum_qty / 100 << std::endl;
        // std::cout << " side : " << order_status_ack.side << std::endl;
        // std::cout << " transact_time : " << order_status_ack.transact_time << std::endl;
        // std::cout << " user_info : " << order_status_ack.user_info << std::endl;
        // std::cout << " order_id : " << order_status_ack.order_id << std::endl;
        // std::cout << " cl_ord_id : " << order_status_ack.cl_ord_id << std::endl;
        // std::cout << " client_seq_id : " << order_status_ack.client_seq_id << std::endl;
        // std::cout << " orig_cl_ord_no : " << order_status_ack.orig_cl_ord_no << std::endl;
        // std::cout << " frozen_trade_value : " << order_status_ack.frozen_trade_value << std::endl;
        // std::cout << " frozen_fee : " << order_status_ack.frozen_fee << std::endl;
        // std::cout << " reject_reason_code : " << order_status_ack.reject_reason_code << std::endl;
        // std::cout << " ord_rej_reason : " << order_status_ack.ord_rej_reason << std::endl;
        // std::cout << " order_type : " << order_status_ack.order_type << std::endl;
        // std::cout << " time_in_force : " << order_status_ack.time_in_force << std::endl;
        // std::cout << " position_effect : " << order_status_ack.position_effect << std::endl;
        // std::cout << " covered_or_uncovered : " << (int32_t)order_status_ack.covered_or_uncovered << std::endl;
        // std::cout << " account_sub_code : " << order_status_ack.account_sub_code << std::endl;
        // std::cout << " quote_flag:" << (int32_t)order_status_ack.quote_flag << std::endl;

        if (order_status_ack.reject_reason_code == 100)
        {
            std::cout << " security_id : " << order_status_ack.security_id << "  ";
            std::cout << " ord_rej_reason : " << order_status_ack.ord_rej_reason << std::endl;
        }

        orig_cl_ord_no = order_status_ack.cl_ord_id;

        // 保存回报分区号、序号，用于断线重连时指定已收到最新回报序号
        report_sync[order_status_ack.partition] = order_status_ack.index;
    }

    // 成交回报
    virtual void OnRspCashAuctionTradeER(const ATPRspCashAuctionTradeERMsg &cash_auction_trade_er)
    {
        // std::cout << " =======cash_auction_trade_er:" << std::endl;
        // std::cout << " cash_auction_trade_er : " << std::endl;
        // std::cout << " partition : " << (int32_t)cash_auction_trade_er.partition << std::endl;
        // std::cout << " index : " << cash_auction_trade_er.index << std::endl;
        // std::cout << " business_type : " << (int32_t)cash_auction_trade_er.business_type << std::endl;
        // std::cout << " cl_ord_no : " << cash_auction_trade_er.cl_ord_no << std::endl;
        // std::cout << " security_id : " << cash_auction_trade_er.security_id << std::endl;
        // std::cout << " market_id : " << cash_auction_trade_er.market_id << std::endl;
        // std::cout << " exec_type : " << cash_auction_trade_er.exec_type << std::endl;
        // std::cout << " ord_status : " << (int32_t)cash_auction_trade_er.ord_status << std::endl;
        // std::cout << " cust_id : " << cash_auction_trade_er.cust_id << std::endl;
        // std::cout << " fund_account_id : " << cash_auction_trade_er.fund_account_id << std::endl;
        // std::cout << " account_id : " << cash_auction_trade_er.account_id << std::endl;
        // std::cout << " price : " << cash_auction_trade_er.price / 10000 << std::endl;
        // std::cout << " order_qty : " << cash_auction_trade_er.order_qty / 100 << std::endl;
        // std::cout << " leaves_qty : " << cash_auction_trade_er.leaves_qty / 100 << std::endl;
        // std::cout << " cum_qty : " << cash_auction_trade_er.cum_qty / 100 << std::endl;
        // std::cout << " side : " << cash_auction_trade_er.side << std::endl;
        // std::cout << " transact_time : " << cash_auction_trade_er.transact_time << std::endl;
        // std::cout << " user_info : " << cash_auction_trade_er.user_info << std::endl;
        // std::cout << " order_id : " << cash_auction_trade_er.order_id << std::endl;
        // std::cout << " cl_ord_id : " << cash_auction_trade_er.cl_ord_id << std::endl;
        // std::cout << " exec_id : " << cash_auction_trade_er.exec_id << std::endl;
        // std::cout << " last_px : " << cash_auction_trade_er.last_px / 10000 << std::endl;
        // std::cout << " last_qty : " << cash_auction_trade_er.last_qty / 100 << std::endl;
        // std::cout << " total_value_traded : " << cash_auction_trade_er.total_value_traded / 1000000 << std::endl;
        // std::cout << " fee : " << cash_auction_trade_er.fee / 100000 << std::endl;
        // std::cout << " cash_margin : " << cash_auction_trade_er.cash_margin << std::endl;
        // std::cout << " reject_reason_code : " << cash_auction_trade_er.reject_reason_code << std::endl;
        // std::cout << " ord_rej_reason : " << cash_auction_trade_er.ord_rej_reason << std::endl;

        // 保存回报分区号、序号，用于断线重连时指定已收到最新回报序号
        report_sync[cash_auction_trade_er.partition] = cash_auction_trade_er.index;
    }

    // 订单下达内部拒绝
    virtual void OnRspBizRejection(const ATPRspBizRejectionOtherMsg &biz_rejection)
    {
        // std::cout << " =======biz_rejection==========" << std::endl;
        // std::cout << " biz_rejection : " << std::endl;
        // std::cout << " transact_time : " << biz_rejection.transact_time << std::endl;
        // std::cout << " client_seq_id : " << biz_rejection.client_seq_id << std::endl;
        // std::cout << " msg_type : " << biz_rejection.api_msg_type << std::endl;
        // std::cout << " reject_reason_code : " << biz_rejection.reject_reason_code << std::endl;
        // std::cout << " business_reject_text : " << biz_rejection.business_reject_text << std::endl;
        // std::cout << " user_info : " << biz_rejection.user_info << std::endl;
    }

    // 增强股份查询结果
    virtual void OnRspExtQueryResultShareEx(const ATPRspExtQueryResultShareExMsg &msg)
    {
        TradeTwapPointer->real_position.clear();
        // std::cout << "=======QueryResultShare==========" << std::endl;
        std::vector<ApiShareArrayEx> holding = msg.order_array;
        for (const ApiShareArrayEx &it : msg.order_array)
        {
            // std::cout << "=================" <<std::endl;
            // std::cout << "security_id: " << it.security_id << std::endl;
            // std::cout << "security_symbol: " << it.security_symbol << std::endl;
            // std::cout << "market_id: " << it.market_id << std::endl;
            // std::cout << "account_id: " << it.account_id  << std::endl;
            // std::cout << "init_qty: " << it.init_qty << std::endl;
            // std::cout << "leaves_qty: " << it.leaves_qty / 100 << std::endl;
            // std::cout << "available_qty: " << it.available_qty / 100 << std::endl;
            // std::cout << "profit_loss: " << it.profit_loss << std::endl;
            // std::cout << "market_value: " << it.market_value << std::endl;
            // std::cout << "cost_price: " << it.cost_price << std::endl;
            // std::cout << "last_price: " << it.last_price << std::endl;
            // std::cout << "security_type: " << it.security_type << std::endl;
            // std::cout << "stock_buy: " << it.stock_buy << std::endl;
            // std::cout << "stock_sale: " << it.stock_sale << std::endl;
            // std::cout << "fund_account_id: " << it.fund_account_id << std::endl;
            // std::cout << "branch_id: " << it.branch_id << std::endl;
            // std::cout << "etf_redemption_qty: " << it.etf_redemption_qty << std::endl;
            // std::cout << "first_day_pos: " << it.first_day_pos << std::endl;

            PositionInfo info;
            strncpy(info.StockCode, it.security_id, 9);
            info.can_sell_volume = it.available_qty / 100;
            info.current_volume = it.leaves_qty / 100;
            info.real_volume = it.leaves_qty / 100;
            if (it.leaves_qty == 0)
                continue;

            TradeTwapPointer->real_position[it.security_id] = info;
        }
    }

    // 股份查询结果
    virtual void OnRspShareQueryResult(const ATPRspShareQueryResultMsg &msg)
    {
        if (TradeTwapPointer->last_position_quantity == 0)
            TradeTwapPointer->real_position.clear();
        // std::cout << "===QueryResultShare:" << std::endl;
        std::vector<APIShareUnit> holding = msg.order_array;

        // std::cout << "last_index: " << msg.last_index << std::endl;
        // std::cout << "total_num: " << msg.total_num << std::endl;
        for (const APIShareUnit &it : msg.order_array)
        {
            // std::cout << "=====================" << std::endl;
            // std::cout << "security_id: " << it.security_id << std::endl;
            // std::cout << "security_symbol: " << it.security_symbol << std::endl;
            // std::cout << "market_id: " << it.market_id << std::endl;
            // std::cout<< "account_id: "<<it.account_id<<std::endl;
            // std::cout<< "fund_account_id: "<<it.fund_account_id<<std::endl;
            // std::cout << "branch_id: " << it.branch_id << std::endl;
            // std::cout << "init_qty: " << it.init_qty << std::endl;
            // std::cout << "leaves_qty: " << it.leaves_qty / 100 << std::endl;
            // std::cout << "available_qty: " << it.available_qty / 100 << std::endl;
            // std::cout << "profit_loss: " << it.profit_loss / 1000000 << std::endl;
            // std::cout << "market_value: " << it.market_value / 1000000 << std::endl;
            // std::cout << "cost_price: " << it.cost_price << std::endl;
            // std::cout << "currency: " << it.currency << std::endl;
            // std::cout << "init_crd_sell_buy_share_qt: " << it.init_crd_sell_buy_share_qty << std::endl;
            // std::cout << "init_crd_sell_occupied_amt: " << it.init_crd_sell_occupied_amt << std::endl;
            // std::cout << "cur_crd_sell_occupied_qty: " << it.cur_crd_sell_occupied_qty << std::endl;
            // std::cout << "cur_crd_sell_occupied_amt: " << it.cur_crd_sell_occupied_amt << std::endl;
            // std::cout << "security_type: " << it.security_type << std::endl;
            // std::cout << "etf_redemption_qty: " << it.etf_redemption_qty / 100 << std::endl;
            // std::cout << "first_day_pos: " << it.first_day_pos / 100 << std::endl;
            // std::cout << "extra_info: " << it.extra_info << std::endl;

            PositionInfo info;
            strncpy(info.StockCode, it.security_id, 9);
            info.can_sell_volume = it.available_qty / 100;
            info.current_volume = it.leaves_qty / 100;
            info.real_volume = it.leaves_qty / 100;
            if (it.leaves_qty == 0)
                continue;

            TradeTwapPointer->real_position[it.security_id] = info;
        }
        TradeTwapPointer->last_position_quantity = msg.last_index;
        TradeTwapPointer->total_position_quantity = msg.total_num;
    }

    // 资金查询结果
    virtual void OnRspFundQueryResult(const ATPRspFundQueryResultMsg &msg)
    {
        // std::cout << "===OnRspFundQueryResult:" << std::endl;

        std::cout << "leaves_value: " << msg.leaves_value / 10000 << std::endl;
        // std::cout << "init_leaves_value: " << msg.init_leaves_value / 10000 << std::endl;
        // std::cout << "available_t0: " << msg.available_t0 / 10000 << std::endl;
        // std::cout << "available_t1: " << msg.available_t1 / 10000 << std::endl;
        // std::cout << "available_t2: " << msg.available_t2 / 10000 << std::endl;
        // std::cout << "available_t3: " << msg.available_t3 / 10000 << std::endl;
        // std::cout << "available_tall: " << msg.available_tall / 10000 << std::endl;
        // std::cout << "frozen_all: " << msg.frozen_all / 10000 << std::endl;
        std::cout << "==========================" << std::endl;
        // std::cout << "leaves_data:" << msg.leaves_value / 10000 << std::endl;  // 资金余额
        // std::cout << "available_t0:" << msg.available_t0 / 10000 << std::endl; // 可用资金
        TradeTwapPointer->query_asset.can_use_money = msg.available_t0 / 10000.0;
        TradeTwapPointer->query_asset.stock_market_value = msg.frozen_all / 10000.0;
    }

    // 成交记录查询结果
    virtual void OnRspTradeOrderQueryResult(const ATPRspTradeOrderQueryResultMsg &msg)
    {
        TradeTwapPointer->query_trade_array.clear();
        // std::cout << "===OnRspTradeOrderQueryResult:" << std::endl;

        for (const APITradeOrderUnit &it : msg.order_array)
        {
            // std::cout << "cl_ord_no:" << it.cl_ord_no << std::endl;

            // std::cout << "security_id:" << it.security_id << std::endl;
            // std::cout << "side:" << it.side << std::endl;
            // std::cout << "last_px:" << it.last_px / 10000 << std::endl;
            // std::cout << "last_qty:" << it.last_qty / 100 << std::endl;
            // std::cout << "order_qty:" << it.order_qty / 100 << std::endl;
            // std::cout << "total_value_traded:" << it.total_value_traded / 1000000 << std::endl;
            // std::cout << "----------------------" << std::endl;

            ordertradeInfo info;

            strncpy(info.StockCode, it.security_id, 9);
            info.side = it.side;
            info.order_qty = it.last_qty;
            TradeTwapPointer->query_trade_array.push_back(info);
        }
        TradeTwapPointer->last_order_quantity += msg.last_index;
        TradeTwapPointer->total_send_times = msg.total_num;
        TradeTwapPointer->total_order_quantity = msg.total_num;
        // std::cout << "total_send_times:" << total_send_times << std::endl;
    }

    // 委托记录查询结果
    virtual void OnRspOrderQueryResult(const ATPRspOrderQueryResultMsg &msg)
    {
        // std::cout << "===OnRspOrderQueryResult:" << std::endl;
        // std::cout << "query_result_code: " << msg.query_result_code << std::endl;
        // for (const APIOrderUnit &it : msg.order_array)
        // {
        // std::cout << "cl_ord_no:" << it.cl_ord_no << std::endl;

        // std::cout << "side:" << it.side << std::endl;
        // std::cout << "ord_type:" << it.ord_type << std::endl;
        // std::cout << "ord_status:" << it.ord_status << std::endl;
        // std::cout << "order_qty:" << it.order_qty / 100 << std::endl;
        // std::cout << "order_price:" << it.order_price / 10000 << std::endl;
        // std::cout << "leaves_qty:" << it.leaves_qty / 100 << std::endl;
        // std::cout << "reject_reason_code:" << it.reject_reason_code << std::endl;
        // std::cout << "ord_rej_reason:" << it.ord_rej_reason << std::endl;
        // std::cout << "----------------------" << std::endl;
        // }
    }
};

ATPTradeAPI *client;
CustHandler *handler;

// 建立连接
ATPRetCodeType connect(ATPTradeAPI *client, ATPTradeHandler *handler)
{
    // 设置连接信息
    ATPConnectProperty prop;
    prop.user = atp_paras.user[agw_flag];                               // 网关用户名
    prop.password = atp_paras.connect_password[agw_flag];               // 网关用户密码
    prop.locations = atp_paras.locations;                               // 网关主备节点的地址+端口
    prop.heartbeat_interval_milli = atp_paras.heartbeat_interval_milli; // 发送心跳的时间间隔，单位：毫秒
    prop.connect_timeout_milli = atp_paras.connect_timeout_milli;       // 连接超时时间，单位：毫秒
    prop.reconnect_time = atp_paras.reconnect_time;                     // 重试连接次数
    prop.client_name = atp_paras.client_name;                           // 客户端程序名字
    prop.client_version = atp_paras.client_version;                     // 客户端程序版本
    prop.report_sync = report_sync;                                     // 回报同步数据分区号+序号，首次是空，断线重连时填入的是接受到的最新分区号+序号
    prop.mode = atp_paras.mode;                                         // 模式0-同步回报模式，模式1-快速登录模式，不同步回报

    // 建立连接
    while (!g_connected_flag.load())
    {
        // 在连接中
        if (g_waiting_flag.load())
        {
            sleep(0);
        }
        else
        {
            g_waiting_flag.store(true);
            // 建立连接
            ATPRetCodeType ec = client->Connect(prop, handler);
            if (ec != ErrorCode::kSuccess)
            {
                std::cout << "Invoke Connect error:" << ec << std::endl;
                return ec;
            }
            sleep(0);
        }
    }

    return kSuccess;
}

// 关闭连接
ATPRetCodeType close(ATPTradeAPI *client)
{
    g_waiting_flag.store(true);
    ATPRetCodeType ec = client->Close();
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke Close error:" << ec << std::endl;
        return ec;
    }

    while (g_waiting_flag.load())
    {
        sleep(0);
    }
    return ErrorCode::kSuccess;
}

// 登录
ATPRetCodeType login(ATPTradeAPI *client)
{
    // 设置登入消息
    ATPReqCustLoginOtherMsg login_msg;
    strncpy(login_msg.fund_account_id, atp_paras.fund_account_id.c_str(), 17); // 资金账户ID

    // strncpy(login_msg.password, atp_paras.login_password.c_str(), 129);        // 客户号密码
    char cipher_text[1024] = {0};
    std::string secret_key = "410301";
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char *)atp_paras.login_password.data(), 6, (unsigned char *)cipher_text, sizeof(cipher_text) - 1, (void *)secret_key.data(), secret_key.size());

    std::string m_strEncryptFundPassword = cipher_text;
    // std::cout << atp_paras.login_password << " 密码加密后:  " << m_strEncryptFundPassword << std::endl;

    strncpy(login_msg.password, m_strEncryptFundPassword.data(), 129);

    login_msg.login_mode = ATPCustLoginModeType::kFundAccountIDMode; // 登录模式，资金账号登录
    login_msg.client_seq_id = g_client_seq_id++;                     // 客户系统消息号
    login_msg.order_way = 'd';                                       // 委托方式，自助委托
    login_msg.client_feature_code = atp_paras.terminal_feature_code; // 终端识别码

    g_waiting_flag.store(true);
    ATPRetCodeType ec = client->ReqCustLoginOther(&login_msg);
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke CustLogin error:" << ec << std::endl;
        return ec;
    }

    while (g_waiting_flag.load())
    {
        sleep(0);
    }

    return ErrorCode::kSuccess;
}

// 登出
ATPRetCodeType logout(ATPTradeAPI *client)
{
    // 设置登出消息
    ATPReqCustLogoutOtherMsg logout_msg;
    strncpy(logout_msg.fund_account_id, atp_paras.fund_account_id.c_str(), 17); // 资金账户ID
    logout_msg.client_seq_id = g_client_seq_id++;                               // 客户系统消息号
    logout_msg.client_feature_code = atp_paras.terminal_feature_code;           // 终端识别码

    g_waiting_flag.store(true);
    ATPRetCodeType ec = client->ReqCustLogoutOther(&logout_msg);
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke CustLogout error:" << ec << std::endl;
        return ec;
    }

    while (g_waiting_flag.load())
    {
        sleep(0);
    }

    return ErrorCode::kSuccess;
}

// order 打印
void printATPReqCashAuctionOrderMsg(const ATPReqCashAuctionOrderMsg *msg)
{
    std::cout << "==========================" << std::endl;
    // std::cout << "  Customer ID: " << msg.cust_id << std::endl;
    // std::cout << "  Fund Account ID: " << msg.fund_account_id << std::endl;
    // std::cout << "  Branch ID: " << msg.branch_id << std::endl;
    // std::cout << "  Account ID: " << msg.account_id << std::endl;
    // std::cout << "  Client Seq ID: " << msg.client_seq_id << std::endl;
    // std::cout << "  User Info: " << msg.user_info << std::endl;
    // std::cout << "  Password: " << msg.password << std::endl;
    // std::cout << "  Extra Data: " << msg.extra_data << std::endl;
    // std::cout << "  Client Feature Code: " << msg.client_feature_code << std::endl;
    std::cout << "  Security ID: " << msg->security_id << std::endl;
    // std::cout << "  Market ID: " << msg->market_id << std::endl;
    std::cout << "  Side: " << msg->side << std::endl;
    std::cout << "  Order Quantity: " << msg->order_qty << std::endl;
    std::cout << "  Price: " << msg->price << std::endl;
    // std::cout << "  Order Way: " << msg->order_way << std::endl;
    // std::cout << "  Enforce Flag: " << msg.enforce_flag << std::endl;
    // std::cout << "  Stop Price: " << msg.stop_px << std::endl;
    // std::cout << "  Order Type: " << msg->order_type << std::endl;
    // std::cout << "  Min Quantity: " << msg.min_qty << std::endl;
    // std::cout << "  Max Price Levels: " << msg.max_price_levels << std::endl;
    // std::cout << "  Time In Force: " << msg.time_in_force << std::endl;
    // std::cout << "  Batch Client Order No: " << msg.batch_cl_ord_no << std::endl;
    // std::cout << "  Order Way Extension: " << msg.order_way_ext << std::endl;
}

// 发送订单
ATPRetCodeType send_order(ATPTradeAPI *client, int market, std::string stock_code, int direction, int order_way, double order_price, double order_quantity)
{
    // 上海市场股票限价委托
    ATPReqCashAuctionOrderMsg *p = new ATPReqCashAuctionOrderMsg;

    // 1.委托报单信息
    strncpy(p->security_id, stock_code.c_str(), 9); // 证券代码
    if (market == 1)
    {
        p->market_id = ATPMarketIDConst::kShangHai; // 市场ID，上海
    }
    else
    {
        p->market_id = ATPMarketIDConst::kShenZhen; // 市场ID，深圳
    }

    if (direction == 1)
    {
        p->side = ATPSideConst::kBuy; // 买卖方向，买
    }
    else if (direction == 2)
    {
        p->side = ATPSideConst::kSell; // 买卖方向，卖
    }
    else
    {
        p->side = ATPSideConst::kAntiRepo; // 买卖方向，逆回购
    }

    if (order_way == 13) // best5
    {
        // p->order_type = ATPOrdTypeConst::kImmediateDealTransferCancel;               // 立即成交剩余撤销（深圳）
        p->order_type = ATPOrdTypeConst::kOptimalFiveLevelFullDealTransferCancel; // 最优五档即时成交剩余撤销（上海、深圳）
    }
    else if (order_way == 1) // limit order
    {
        p->order_type = ATPOrdTypeConst::kFixedNew; // 订单类型，限价
    }
    else
    {
        // p->order_type = ATPOrdTypeConst::kImmediateDealTransferCancel;               // 立即成交剩余撤销（深圳）
        p->order_type = ATPOrdTypeConst::kOptimalFiveLevelFullDealTransferCancel; // 最优五档即时成交剩余撤销（上海、深圳）
    }

    // p->price = order_price * 10000;             // 委托价格 N13(4)，21.0000元

    // 市价报买单时用涨停价
    if (order_way == 13 && direction == 1)
    {
        std::string order_code = stock_code.substr(1, 6);
        if (TradeTwapPointer->limit_price_map.count(stock_code) != 0)
        {
            double multiplied = TradeTwapPointer->limit_price_map[stock_code][0] * 10000;
            p->price = static_cast<int>(std::round(multiplied / 100) * 100);
            // 委托价格 N13(4)，21.0000元
            // p->price = TradeTwapPointer->limit_price_map[stock_code][0] * 1000000 * 10000 / 1000000;
        }
        else
        {
            // 没有找到涨停价取50
            std::cout << "order_code=" << order_code << " not in buy_limit_price_map" << std::endl;
            p->price = 50 * 10000;
        }
        // std::cout << p->security_id << " " << p->price << std::endl;
    }
    // 市价报卖单时用跌停价
    else if (order_way == 13 && direction == 2)
    {
        std::string order_code = stock_code.substr(1, 6);
        if (TradeTwapPointer->limit_price_map.count(stock_code) != 0)
        {
            double multiplied = TradeTwapPointer->limit_price_map[stock_code][1] * 10000;
            p->price = static_cast<int>(std::round(multiplied / 100) * 100);
            // 委托价格 N13(4)，21.0000元
            // p->price = TradeTwapPointer->limit_price_map[stock_code][1] * 1000000 * 10000 / 1000000;
        }
        else
        {
            // 没有找到跌停价取0.1
            std::cout << "order_code=" << order_code << " not in buy_limit_price_map" << std::endl;
            p->price = 0.1 * 10000;
        }
    }
    // 限价报单
    else
    {
        double multiplied = order_price * 10000;
        p->price = static_cast<int>(std::round(multiplied / 100) * 100); // 委托价格 N13(4)，21.0000元
    }

    p->order_qty = abs(order_quantity) * 100; // 申报数量N15(2)；股票为股、基金为份、上海债券默认为张（使用时请务必与券商确认），其他为张；1000.00股

    // 2.客户固定信息
    strncpy(p->cust_id, atp_paras.cust_id.c_str(), 17);                 // 客户号ID
    strncpy(p->fund_account_id, atp_paras.fund_account_id.c_str(), 17); // 资金账户ID
    if (p->market_id == ATPMarketIDConst::kShangHai)
    {
        strncpy(p->account_id, atp_paras.stock_account_sh.c_str(), 13); // 上海账户ID
    }
    else
    {
        strncpy(p->account_id, atp_paras.stock_account_sz.c_str(), 13); // 深圳账户ID
    }
    p->order_way = '7'; // 委托方式，自助委托

    // strncpy(p->password, atp_paras.login_password.c_str(), 129);        // 客户号密码
    char cipher_text[1024] = {0};
    std::string secret_key = "410301";
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char *)atp_paras.login_password.data(), 6, (unsigned char *)cipher_text, sizeof(cipher_text) - 1, (void *)secret_key.data(), secret_key.size());

    std::string m_strEncryptFundPassword = cipher_text;
    // std::cout << atp_paras.login_password << " 密码加密后:  " << m_strEncryptFundPassword << std::endl;

    strncpy(p->password, m_strEncryptFundPassword.data(), 129);

    p->client_feature_code = atp_paras.terminal_feature_code; // 终端识别码

    p->client_seq_id = g_client_seq_id++; // 用户系统消息序号

    // 打印订单信息
    std::cout << "security_id: " << p->security_id << std::endl;
    // std::cout << "market_id: " << p->market_id << std::endl;
    // std::cout << "side: " << p->side << std::endl;
    // std::cout << "order_type: " << p->order_type << std::endl;
    std::cout << "price: " << p->price << std::endl;
    std::cout << "order_qty: " << p->order_qty << std::endl;
    // std::cout << "cust_id: " << p->cust_id << std::endl;
    // std::cout << "fund_account_id: " << p->fund_account_id << std::endl;
    // std::cout << "account_id: " << p->account_id << std::endl;
    // std::cout << "order_way: " << p->order_way << std::endl;
    // std::cout << "password: " << p->password << std::endl;
    // std::cout << "client_feature_code: " << p->client_feature_code << std::endl;
    // std::cout << "client_seq_id: " << p->client_seq_id << std::endl;

    // 调用下单函数
    // ATPRetCodeType ec = client->ReqCashAuctionOrder(p);

    // TradeTwapPointer->send_times++;
    // if (TradeTwapPointer->send_times >= atp_paras.max_order_qty_sec)
    // {
    //     std::cout << "The number of orders placed per second has exceeded the maximum limit" << std::endl;
    //     std::cout << "We will wait for 3 seconds to continue placing orders" << std::endl;
    //     sleep(3);
    //     TradeTwapPointer->send_times = 0;
    // }

    // TradeTwapPointer->total_send_times++;
    // if (TradeTwapPointer->total_send_times >= atp_paras.max_order_qty)
    // {
    //     std::cout << " The number of orders placed today has reached the top, and orders have now been stopped " << std::endl;
    //     std::cin.clear();
    //     int flag;
    //     std::cin >> flag;
    // }

    // if (TradeTwapPointer->cancel_order / TradeTwapPointer->total_send_times >= atp_paras.cancel_pro)
    // {
    //     std::cout << " The number of cancallation rate today has reached the top, and orders have now been stopped " << std::endl;
    //     std::cin.clear();
    //     int flag;
    //     std::cin >> flag;
    // }

    // if (TradeTwapPointer->scrap_order / TradeTwapPointer->total_send_times >= atp_paras.scrap_pro)
    // {
    //     std::cout << " The number of scrap rate today has reached the top, and orders have now been stopped " << std::endl;
    //     std::cin.clear();
    //     int flag;
    //     std::cin >> flag;
    // }

    // delete p;
    // return ec;
}

// 初始化连接并完成登录
bool init(ATPTradeAPI *client, ATPTradeHandler *handler)
{
    // 建立连接
    if (connect(client, handler) != ErrorCode::kSuccess)
    {
        return false;
    }

    // 登录
    if (login(client) != ErrorCode::kSuccess)
    {
        return false;
    }

    // 检查是否登录成功
    if (!g_cust_logined_flag.load())
    {
        return false;
    }

    return true;
}

// 关闭连接并退出
void exit(ATPTradeAPI *client, ATPTradeHandler *handler)
{
    if (g_cust_logined_flag.load())
    {
        logout(client);
    }

    if (g_connected_flag.load())
    {
        close(client);
    }

    ATPTradeAPI::Stop();
}

////////////////////////////////atp相关接口函数/////////////////////////////////////////

void init_limit_price_map(std::string limit_price_path)
{
    std::ifstream file(limit_price_path);

    if (file.is_open())
    {
        std::string line;
        std::getline(file, line); // 跳过字段名
        while (std::getline(file, line))
        {
            std::string token;
            std::stringstream ss(trim(line));

            int i = 1;
            std::string code;
            std::vector<double> data;

            while (std::getline(ss, token, ','))
            {

                if (i == 1)
                    code = token.substr(0, 6);
                else
                {
                    data.push_back(std::stod(token));
                }
                i++;
            }
            std::vector<double> parts(2, 0);
            parts[0] = data[1];
            parts[1] = data[2];
            TradeTwapPointer->limit_price_map[code] = parts;
        }
        file.close();
    }
    else
    {
        std::cerr << "Cannot open limit price file : " << strerror(errno) << std::endl
                  << limit_price_path << std::endl;
    }
}

int query_position_atp(int quert_index)
{
    // 查询持仓
    // ATPReqExtQueryShareExMsg *query_req = new ATPReqExtQueryShareExMsg;
    ATPReqShareQueryMsg *query_req = new ATPReqShareQueryMsg;
    strncpy(query_req->cust_id, atp_paras.cust_id.c_str(), 17);
    strncpy(query_req->fund_account_id, atp_paras.fund_account_id.c_str(), 17);

    // strncpy(query_req->password, atp_paras.login_password.c_str(), 129);        // 客户号密码
    char cipher_text[1024] = {0};
    std::string secret_key = "410301";
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char *)atp_paras.login_password.data(), 6, (unsigned char *)cipher_text, sizeof(cipher_text) - 1, (void *)secret_key.data(), secret_key.size());

    std::string m_strEncryptFundPassword = cipher_text;
    // std::cout << atp_paras.login_password << " 密码加密后:  " << m_strEncryptFundPassword << std::endl;

    strncpy(query_req->password, m_strEncryptFundPassword.data(), 129);

    query_req->client_feature_code = atp_paras.terminal_feature_code;
    query_req->query_index = quert_index;
    // std::cout<<"begin to query....... "<<std::endl;
    ATPRetCodeType ret;

    ret = client->ReqShareQuery(query_req);

    // try{
    //      ret = client->ReqExtQueryShareEx(query_req);
    // }catch(const std::runtime_error& e){
    //      std::cerr << "Caught an exception: " << e.what() << std::endl;
    // }

    if (ret != ErrorCode::kSuccess)
    {
        std::cout << "query_position error: ret" << ret << std::endl;
        return -1;
    }
    // std::cout<<"query end....... code_API: "<< ret << std::endl;
    delete query_req;
    return ret;
}

void query_trade_atp(int quert_index)
{
    // 查询成交订单
    ATPReqTradeOrderQueryMsg *query_req = new ATPReqTradeOrderQueryMsg;
    strncpy(query_req->cust_id, atp_paras.cust_id.c_str(), 17);
    strncpy(query_req->fund_account_id, atp_paras.fund_account_id.c_str(), 17);
    // strncpy(query_req->account_id, atp_paras.fund_account_id.c_str(), 17);
    query_req->client_seq_id = g_client_seq_id++;
    // strncpy(query_req->password, atp_paras.login_password.c_str(), 129);
    char cipher_text[1024] = {0};
    std::string secret_key = "410301";
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char *)atp_paras.login_password.data(), 6, (unsigned char *)cipher_text, sizeof(cipher_text) - 1, (void *)secret_key.data(), secret_key.size());

    std::string m_strEncryptFundPassword = cipher_text;
    // std::cout << atp_paras.login_password << " 密码加密后:  " << m_strEncryptFundPassword << std::endl;

    strncpy(query_req->password, m_strEncryptFundPassword.data(), 129);
    query_req->client_feature_code = atp_paras.terminal_feature_code;
    query_req->market_id = 0;
    query_req->query_index = quert_index;

    // std::cout<<"begin to query....... "<<std::endl;
    ATPRetCodeType ret;

    ret = client->ReqTradeOrderQuery(query_req);

    if (ret != ErrorCode::kSuccess)
    {
        std::cout << "query_position error: ret" << ret << std::endl;
    }
    // std::cout<<"query end....... code_API: "<< ret << std::endl;
    delete query_req;
}

void query_order_atp(int quert_index)
{
    // 查询委托信息
    ATPReqOrderQueryMsg *query_req = new ATPReqOrderQueryMsg;
    strncpy(query_req->cust_id, atp_paras.cust_id.c_str(), 17);
    strncpy(query_req->fund_account_id, atp_paras.fund_account_id.c_str(), 17);
    // strncpy(query_req->account_id, atp_paras.fund_account_id.c_str(), 17);
    query_req->client_seq_id = g_client_seq_id++;
    // strncpy(query_req->password, atp_paras.login_password.c_str(), 129);
    char cipher_text[1024] = {0};
    std::string secret_key = "410301";
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char *)atp_paras.login_password.data(), 6, (unsigned char *)cipher_text, sizeof(cipher_text) - 1, (void *)secret_key.data(), secret_key.size());

    std::string m_strEncryptFundPassword = cipher_text;
    // std::cout << atp_paras.login_password << " 密码加密后:  " << m_strEncryptFundPassword << std::endl;

    strncpy(query_req->password, m_strEncryptFundPassword.data(), 129);

    query_req->client_feature_code = atp_paras.terminal_feature_code;
    query_req->query_index = quert_index;

    // std::cout<<"begin to query....... "<<std::endl;
    ATPRetCodeType ret;

    ret = client->ReqOrderQuery(query_req);

    if (ret != ErrorCode::kSuccess)
    {
        std::cout << "query_position error: ret" << ret << std::endl;
    }
    // std::cout<<"query end....... code_API: "<< ret << std::endl;
    delete query_req;
}

void query_money_atp()
{
    // 查询资金情况

    ATPReqFundQueryMsg *query_req = new ATPReqFundQueryMsg;
    strncpy(query_req->cust_id, atp_paras.cust_id.c_str(), 17);
    strncpy(query_req->fund_account_id, atp_paras.fund_account_id.c_str(), 17);
    query_req->client_seq_id = g_client_seq_id++;
    // strncpy(query_req->password, atp_paras.login_password.c_str(), 129);
    char cipher_text[1024] = {0};
    std::string secret_key = "410301";
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char *)atp_paras.login_password.data(), 6, (unsigned char *)cipher_text, sizeof(cipher_text) - 1, (void *)secret_key.data(), secret_key.size());

    std::string m_strEncryptFundPassword = cipher_text;
    // std::cout << atp_paras.login_password << " 密码加密后:  " << m_strEncryptFundPassword << std::endl;

    strncpy(query_req->password, m_strEncryptFundPassword.data(), 129);
    query_req->client_feature_code = atp_paras.terminal_feature_code;

    // std::cout<<"begin to query....... "<<std::endl;
    ATPRetCodeType ret;

    ret = client->ReqFundQuery(query_req);

    if (ret != ErrorCode::kSuccess)
    {
        std::cout << "query_position error: ret" << ret << std::endl;
    }
    // std::cout<<"query end....... code_API: "<< ret << std::endl;
    delete query_req;
}

void order_insert_atp(int market, std::string stock_code, int direction, int order_way, double order_price, double order_quantity)
{
    /*
    在这里对接柜台的下单函数
    market: 1上交所  2深交所
    stock_code: 6位数字
    direction: 1买 2卖
    order_way: 13->best5   1->limit order
    */
    // 发送订单请求
    // sleep(0.5);
    send_order(client, market, stock_code, direction, order_way, order_price, order_quantity);
}

// 发送订单(demo)
ATPRetCodeType send(ATPTradeAPI *client, std::string code, double price, int qty, int dir)
{
    // 上海市场股票限价委托
    ATPReqCashAuctionOrderMsg *p = new ATPReqCashAuctionOrderMsg;
    strncpy(p->security_id, code.c_str(), 9);                           // 证券代码
    p->market_id = ATPMarketIDConst::kShangHai;                         // 市场ID，上海
    strncpy(p->cust_id, atp_paras.cust_id.c_str(), 17);                 // 客户号ID
    strncpy(p->fund_account_id, atp_paras.fund_account_id.c_str(), 17); // 资金账户ID
    strncpy(p->account_id, atp_paras.stock_account_sh.c_str(), 13);     // 账户ID
    if (dir == 1)
    {
        p->side = ATPSideConst::kBuy; // 买卖方向，买
    }
    else
    {
        p->side = ATPSideConst::kSell; // 买卖方向，卖
    }

    p->order_type = ATPOrdTypeConst::kOptimalFiveLevelFullDealTransferCancel; // 订单类型，五档即成剩撤
    p->price = price * 10000;                                                 // 委托价格 N13(4)，21.0000元
    p->order_qty = qty * 100;                                                 // 申报数量N15(2)；股票为股、基金为份、上海债券默认为张（使用时请务必与券商确认），其他为张；1000.00股
    p->client_seq_id = g_client_seq_id++;                                     // 用户系统消息序号
    p->order_way = '7';                                                       // 委托方式，自助委托
    // strncpy(p->password, atp_paras.login_password.c_str(), 129);              // 客户密码
    char cipher_text[1024] = {0};
    std::string secret_key = "410301";
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char *)atp_paras.login_password.data(), 6, (unsigned char *)cipher_text, sizeof(cipher_text) - 1, (void *)secret_key.data(), secret_key.size());

    std::string m_strEncryptFundPassword = cipher_text;
    // std::cout << atp_paras.login_password << " 密码加密后:  " << m_strEncryptFundPassword << std::endl;

    strncpy(p->password, m_strEncryptFundPassword.data(), 129);
    p->client_feature_code = atp_paras.terminal_feature_code; // 终端识别码

    ATPRetCodeType ec = client->ReqCashAuctionOrder(p);

    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke Send error:" << ec << std::endl;
    }

    delete p;
    return ec;
}

ATPRetCodeType send_repo_order(ATPTradeAPI *client, std::string code, double price, int qty, int dir)
{
    ATPReqRepoAuctionOrderMsg *p = new ATPReqRepoAuctionOrderMsg;
    strncpy(p->security_id, code.c_str(), 9);
    if (code[0] == '6')
        p->market_id = ATPMarketIDConst::kShangHai;
    else
        p->market_id = ATPMarketIDConst::kShenZhen;
    strncpy(p->cust_id, atp_paras.cust_id.c_str(), 17);
    strncpy(p->fund_account_id, atp_paras.fund_account_id.c_str(), 17);
    strncpy(p->account_id, atp_paras.stock_account_sh.c_str(), 13);
    p->client_seq_id = g_client_seq_id++;
    if (dir == 4)
    {
        p->side = ATPSideConst::kAntiRepo;
    }
    else
        p->side = ATPSideConst::kRepo;

    p->price = price * 100000;
    p->order_qty = qty * 100;

    p->order_way = '7';
    // strncpy(p->password, atp_paras.login_password.c_str(), 129);
    char cipher_text[1024] = {0};
    std::string secret_key = "410301";
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char *)atp_paras.login_password.data(), 6, (unsigned char *)cipher_text, sizeof(cipher_text) - 1, (void *)secret_key.data(), secret_key.size());

    std::string m_strEncryptFundPassword = cipher_text;
    std::cout << atp_paras.fund_account_id << std::endl;
    // std::cout << atp_paras.login_password << " 密码加密后:  " << m_strEncryptFundPassword << std::endl;

    p->client_feature_code = atp_paras.terminal_feature_code;

    ATPRetCodeType ec = client->ReqRepoAuctionOrder(p);

    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke Send error:" << ec << std::endl;
    }

    delete p;
    return ec;
}

// 撤销订单(demo)
ATPRetCodeType cancelOrder(ATPTradeAPI *client, std::string orig_cl_ord_no)
{
    // 上海市场股票限价委托
    ATPReqCancelOrderMsg *p = new ATPReqCancelOrderMsg;

    strncpy(p->cust_id, atp_paras.cust_id.c_str(), 17);                 // 客户号ID
    strncpy(p->fund_account_id, atp_paras.fund_account_id.c_str(), 17); // 资金账户ID
    strncpy(p->account_id, atp_paras.stock_account_sh.c_str(), 13);     // 账户ID
    strncpy(p->branch_id, atp_paras.branch_id.c_str(), 13);             // 营业部ID

    p->client_seq_id = g_client_seq_id++; // 用户系统消息序号
    p->orig_cl_ord_no = std::stoi(orig_cl_ord_no);

    ATPRetCodeType ec = client->ReqCancelOrder(p);

    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke Send error:" << ec << std::endl;
    }

    delete p;
    return ec;
}

int main(int argc, char *argv[])
{
    // 读取配置信息
    std::string twap_para_path = "../config/twap_para.txt";
    if (!isExistFile(twap_para_path))
    {
        std::cout << twap_para_path << " not exist " << std::endl;
        return -1;
    }
    read_twap_paras(twap_para_path, twap_paras);

    std::cout << " Please choose the fund to trade (";
    for (int i = 0; i < twap_paras.fund_list.size(); i++)
    {
        std::cout << twap_paras.fund_list[i] << ",";
    }
    std::cout << "):";
    std::string fundname;
    std::cin >> fundname;
    std::transform(fundname.begin(), fundname.end(), fundname.begin(), ::toupper);
    while (std::find(twap_paras.fund_list.begin(), twap_paras.fund_list.end(), fundname) == twap_paras.fund_list.end())
    {
        std::cout << "Invalid fund name. Please choose again: ";
        std::cin >> fundname;
        std::transform(fundname.begin(), fundname.end(), fundname.begin(), ::toupper);
    }
    std::cout << std::endl;

    // 不同产品分配不同网关，每个网关只能3个产品
    for (int i = 0; i < twap_paras.agw_list.size(); i++)
    {
        auto it_agw1 = std::find(twap_paras.agw_list[i].begin(), twap_paras.agw_list[i].end(), fundname);
        if (it_agw1 != twap_paras.agw_list[i].end())
            agw_flag = i;
    }

    std::string client_info_path = "../config/atp_para.txt";
    if (!isExistFile(client_info_path))
    {
        std::cout << client_info_path << " \033[31m not exist \033[0m" << std::endl;
        return -1;
    }
    std::cout << "the file " << twap_para_path << " and " << client_info_path << " is existed" << std::endl;
    read_atp_para(client_info_path, atp_paras, twap_paras, fundname);

    // std::cout << atp_paras.user[agw_flag] <<std::endl;
    // std::cout << atp_paras.connect_password[agw_flag] <<std::endl;

    std::cout << "Now begin to trade " << atp_paras.fundname;
    std::cout << ", the account_id is " << atp_paras.fund_account_id << std::endl;
    // std::cout << ", the password is " << atp_paras.login_password << std::endl;
    // std::cout << ", the stock_account_sh is " << atp_paras.stock_account_sh << std::endl;
    // std::cout << ", the stock_account_sz is " << atp_paras.stock_account_sz << std::endl;
    TradeTwapPointer = new TwapTradeSample(twap_paras, fundname);

    // 读取涨跌停价文件
    std::string limit_path = TradeTwapPointer->limit_price_path;
    if (!isExistFile(limit_path))
    {
        std::cout << limit_path << " \033[31m not exist \033[0m" << std::endl;
        return -1;
    }
    init_limit_price_map(limit_path);

    // 初始化API
    const std::string station_name = atp_paras.station_name; // 站点信息，该字段已经不使用
    const std::string cfg_path = atp_paras.cfg_path;         // 配置文件路径
    const std::string log_dir_path = atp_paras.log_dir_path; // 日志路径
    bool record_all_flag = true;                             // 是否记录所有委托信息
    bool connection_retention_flag = false;                  // 是否启用会话保持

    std::unordered_map<std::string, std::string> encrypt_cfg; // 加密库配置
    // encrypt_cfg参数填写：
    encrypt_cfg["ENCRYPT_SCHEMA"] = atp_paras.ENCRYPT_SCHEMA;                         // 字符 0 表示 不对消息中的所有 password 加密
    encrypt_cfg["ATP_ENCRYPT_PASSWORD"] = atp_paras.ATP_ENCRYPT_PASSWORD;             // 除登入及密码修改外其他消息的密码字段加密算法
    encrypt_cfg["ATP_LOGIN_ENCRYPT_PASSWORD"] = atp_paras.ATP_LOGIN_ENCRYPT_PASSWORD; // 登入及密码修改消息中密码字段的加密算法so路径
    encrypt_cfg["GM_SM2_PUBLIC_KEY_PATH"] = atp_paras.GM_SM2_PUBLIC_KEY_PATH;         // 采用国密算法时，通过该key配置 GM算法配置加密使用的公钥路径
    encrypt_cfg["RSA_PUBLIC_KEY_PATH"] = atp_paras.RSA_PUBLIC_KEY_PATH;               // 如果使用rsa算法加密，通过该key配置 rsa算法配置加密使用的公钥路径

    ATPRetCodeType ec = ATPTradeAPI::Init(station_name, cfg_path, log_dir_path, record_all_flag, encrypt_cfg, connection_retention_flag);
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Init failed: " << ec << std::endl;
        return false;
    }
    // 获取tradeAPI客户端和回调句柄
    client = new ATPTradeAPI();
    handler = new CustHandler();


    if (true){
        std::cout << "connect and login success" << std::endl;
        std::cout << "==============================" << std::endl;
        // 开启获取行情数据线程
        std::string inputline1;
        inputline1 = "y";
        if (inputline1 == "y")
        {
            std::thread GetMarketDataTrd(GetMarketDataFunc);
            GetMarketDataTrd.detach();
        }
        std::cout << "after thread get_market_data, sleep(5)" << std::endl;
        sleep(10);

        // 开启监控线程
        std::string inputline2;
        inputline2 = "y";
        if (inputline2 == "y")
        {
            std::thread InspectTrd(InspectFunc);
            InspectTrd.detach();
        }
    }
    
    // if (init(client, handler))
    // {
    //     // send(client, "601988", 4.59, 100, 1);
    //     // std::cout << "connect and login success" << std::endl;
    //     // std::cout << "==============================" << std::endl;
    //     // // 开启获取行情数据线程
    //     // std::string inputline1;
    //     // inputline1 = "y";
    //     // if (inputline1 == "y")
    //     // {
    //     //     std::thread GetMarketDataTrd(GetMarketDataFunc);
    //     //     GetMarketDataTrd.detach();
    //     // }
    //     // std::cout << "after thread get_market_data, sleep(5)" << std::endl;
    //     // sleep(10);

    //     // 开启交易线程
    //     // query_money_atp();
    //     // sleep(2);
    //     // std::string inputline3;
    //     // inputline3 = "y";
    //     // if (inputline3 == "y")
    //     // {
    //     //     std::thread StrategyTrd(TwapTradeFunc);
    //     //     StrategyTrd.detach();
    //     // }

    //     // 开启监控线程
    //     // sleep(2);
    //     // std::string inputline2;
    //     // inputline2 = "y";
    //     // if (inputline2 == "y")
    //     // {
    //     //     std::thread InspectTrd(InspectFunc);
    //     //     InspectTrd.detach();
    //     // }
    // }
    // else
    // {
    //     std::cout << "connect and login failed" << std::endl;
    //     TradeTwapPointer->is_quit = 1;
    // }
    // 循环等待退出程序
    while (TradeTwapPointer->is_quit != 1)
    {
        // std::cout << "Wait for quit: " << is_quit << std::endl;
    }
    exit(client, handler);
    delete client;
    delete handler;

    return 0;
}
