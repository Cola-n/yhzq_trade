#include <iostream>
#include <atomic>
#include <map>
#if defined _WIN32
#include <windows.h>
#define sleep Sleep
#else
#include <unistd.h>
#endif

#include "atp_trade_api.h"
#include "KDEncodeCli.h"
std::atomic_bool g_connected_flag(false);           // 已连接
std::atomic_bool g_cust_logined_flag(false);        // 已登录

std::atomic_bool g_waiting_flag(false);             // 等待响应

ATPClientSeqIDType g_client_seq_id = 1;             //  客户系统消息号


std::string  agw_ip_port="223.70.124.229:32001";
//std::string  agw_ip_port="10.4.127.2:32001";
std::string  agw_user = "SZTHR_JR1";
std::string  agw_passwd= "SZTHR#JR1";

std::string  cust_id = "10420014598";
std::string  fund_account_id = "10420014598";
std::string  branch_id = "0104";
std::string  account_id_sh = "B881159262";
std::string  account_id_sz = "0800046928";
std::string  password= "159357";
std::string m_strEncryptFundPassword;

char order_way = 'd';
// 终端识别码
ATPClientFeatureCodeType g_client_feature_code = "PC;IIP=119.36.8.132;IPORT=50031;LIP=192.168.1.62;MAC=FCF8GR5C34;\
HD=TU366AY91GER;PCN=ADMIN1;CPU=432eh12fe21ge1f;PI=NA;VOL=NA@atp_demo;V3.2.3.13";  

// 定义回报数据分区+序号的全局变量，在收到回报时更新此变量，在断线重连时传入该变量，指示客户端所收到的最大回报序号
std::map<int32_t, int32_t> report_sync;

// 回调类，继承自ATPTradeHandler，可以只实现部分回调函数
class CustHandler : public ATPTradeHandler
{
    // 登入回调
    virtual void OnLogin(const std::string& reason)
    {
        std::cout << "OnLogin Recv:" << reason << std::endl;
        g_connected_flag.store(true);
    }

    // 登出回调
    virtual void OnLogout(const std::string& reason)
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
    virtual void OnEndOfConnection(const std::string& reason)
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
    virtual void OnRspOrderStatusInternalAck(const ATPRspOrderStatusAckMsg& order_status_ack)
    {
        std::cout << "order_status_Internal_ack : " << std::endl;
        std::cout << "partition : " << (int32_t)order_status_ack.partition <<
            " index : " << order_status_ack.index <<
            " business_type : " << (int32_t)order_status_ack.business_type <<
            " cl_ord_no : " << order_status_ack.cl_ord_no <<
            " security_id : " << order_status_ack.security_id <<
            " market_id : " << order_status_ack.market_id <<
            " exec_type : " << order_status_ack.exec_type <<
            " ord_status : " << (int32_t)order_status_ack.ord_status <<
            " cust_id : " << order_status_ack.cust_id <<
            " fund_account_id : " << order_status_ack.fund_account_id <<
            " account_id : " << order_status_ack.account_id <<
            " price : " << order_status_ack.price <<
            " order_qty : " << order_status_ack.order_qty <<
            " leaves_qty : " << order_status_ack.leaves_qty <<
            " cum_qty : " << order_status_ack.cum_qty <<
            " side : " << order_status_ack.side <<
            " transact_time : " << order_status_ack.transact_time <<
            " user_info : " << order_status_ack.user_info <<
            " order_id : " << order_status_ack.order_id <<
            " cl_ord_id : " << order_status_ack.cl_ord_id <<
            " client_seq_id : " << order_status_ack.client_seq_id <<
            " orig_cl_ord_no : " << order_status_ack.orig_cl_ord_no <<
            " frozen_trade_value : " << order_status_ack.frozen_trade_value <<
            " frozen_fee : " << order_status_ack.frozen_fee <<
            " reject_reason_code : " << order_status_ack.reject_reason_code <<
            " ord_rej_reason : " << order_status_ack.ord_rej_reason <<
            " order_type : " << order_status_ack.order_type <<
            " time_in_force : " << order_status_ack.time_in_force <<
            " position_effect : " << order_status_ack.position_effect <<
            " covered_or_uncovered : " << (int32_t)order_status_ack.covered_or_uncovered <<
            " account_sub_code : " << order_status_ack.account_sub_code << std::endl;
        
        // 保存回报分区号、序号，用于断线重连时指定已收到最新回报序号
        report_sync[order_status_ack.partition] = order_status_ack.index;
    }

    // 订单下达交易所确认
    virtual void OnRspOrderStatusAck(const ATPRspOrderStatusAckMsg& order_status_ack)
    {
        std::cout << "order_status_ack : " << std::endl;
        std::cout << "partition : " << (int32_t)order_status_ack.partition <<
            " index : " << order_status_ack.index <<
            " business_type : " << (int32_t)order_status_ack.business_type <<
            " cl_ord_no : " << order_status_ack.cl_ord_no <<
            " security_id : " << order_status_ack.security_id <<
            " market_id : " << order_status_ack.market_id <<
            " exec_type : " << order_status_ack.exec_type <<
            " ord_status : " << (int32_t)order_status_ack.ord_status <<
            " cust_id : " << order_status_ack.cust_id <<
            " fund_account_id : " << order_status_ack.fund_account_id <<
            " account_id : " << order_status_ack.account_id <<
            " price : " << order_status_ack.price <<
            " order_qty : " << order_status_ack.order_qty <<
            " leaves_qty : " << order_status_ack.leaves_qty <<
            " cum_qty : " << order_status_ack.cum_qty <<
            " side : " << order_status_ack.side <<
            " transact_time : " << order_status_ack.transact_time <<
            " user_info : " << order_status_ack.user_info <<
            " order_id : " << order_status_ack.order_id <<
            " cl_ord_id : " << order_status_ack.cl_ord_id <<
            " client_seq_id : " << order_status_ack.client_seq_id <<
            " orig_cl_ord_no : " << order_status_ack.orig_cl_ord_no <<
            " frozen_trade_value : " << order_status_ack.frozen_trade_value <<
            " frozen_fee : " << order_status_ack.frozen_fee <<
            " reject_reason_code : " << order_status_ack.reject_reason_code <<
            " ord_rej_reason : " << order_status_ack.ord_rej_reason <<
            " order_type : " << order_status_ack.order_type <<
            " time_in_force : " << order_status_ack.time_in_force <<
            " position_effect : " << order_status_ack.position_effect <<
            " covered_or_uncovered : " << (int32_t)order_status_ack.covered_or_uncovered <<
            " account_sub_code : " << order_status_ack.account_sub_code << 
            " quote_flag:"<<(int32_t)order_status_ack.quote_flag<< std::endl;
        
        // 保存回报分区号、序号，用于断线重连时指定已收到最新回报序号
        report_sync[order_status_ack.partition] = order_status_ack.index;
    }

    // 成交回报
    virtual void OnRspCashAuctionTradeER(const ATPRspCashAuctionTradeERMsg& cash_auction_trade_er) 
    {
        std::cout << "cash_auction_trade_er : " << std::endl;
        std::cout << "partition : " << (int32_t)cash_auction_trade_er.partition <<
            " index : " << cash_auction_trade_er.index <<
            " business_type : " <<(int32_t)cash_auction_trade_er.business_type <<
            " cl_ord_no : " << cash_auction_trade_er.cl_ord_no <<
            " security_id : " << cash_auction_trade_er.security_id <<
            " market_id : " << cash_auction_trade_er.market_id <<
            " exec_type : " << cash_auction_trade_er.exec_type <<
            " ord_status : " << (int32_t)cash_auction_trade_er.ord_status <<
            " cust_id : " << cash_auction_trade_er.cust_id <<
            " fund_account_id : " << cash_auction_trade_er.fund_account_id <<
            " account_id : " << cash_auction_trade_er.account_id <<
            " price : " << cash_auction_trade_er.price <<
            " order_qty : " << cash_auction_trade_er.order_qty <<
            " leaves_qty : " << cash_auction_trade_er.leaves_qty <<
            " cum_qty : " << cash_auction_trade_er.cum_qty <<
            " side : " << cash_auction_trade_er.side <<
            " transact_time : " << cash_auction_trade_er.transact_time <<
            " user_info : " << cash_auction_trade_er.user_info <<
            " order_id : " << cash_auction_trade_er.order_id <<
            " cl_ord_id : " << cash_auction_trade_er.cl_ord_id <<
            " exec_id : " << cash_auction_trade_er.exec_id <<
            " last_px : " << cash_auction_trade_er.last_px <<
            " last_qty : " << cash_auction_trade_er.last_qty <<
            " total_value_traded : " << cash_auction_trade_er.total_value_traded <<
            " fee : " << cash_auction_trade_er.fee <<
            " cash_margin : " << cash_auction_trade_er.cash_margin << std::endl;
        
        // 保存回报分区号、序号，用于断线重连时指定已收到最新回报序号
        report_sync[cash_auction_trade_er.partition] = cash_auction_trade_er.index;
    }

    // 订单下达内部拒绝
    virtual void OnRspBizRejection(const ATPRspBizRejectionOtherMsg& biz_rejection)
    {
        std::cout << "biz_rejection : " << std::endl;
        std::cout << "transact_time : " << biz_rejection.transact_time <<
            " client_seq_id : " << biz_rejection.client_seq_id <<
            " msg_type : " << biz_rejection.api_msg_type <<
            " reject_reason_code : " << biz_rejection.reject_reason_code <<
            " business_reject_text : " << biz_rejection.business_reject_text <<
            " user_info : " << biz_rejection.user_info << std::endl;
    }
};

// 建立连接
ATPRetCodeType connect(ATPTradeAPI* client, ATPTradeHandler* handler)
{
    // 设置连接信息
    ATPConnectProperty prop;
    prop.user = agw_user;                                           // 网关用户名
    prop.password = agw_passwd;                                    // 网关用户密码
    prop.locations = { agw_ip_port };      // 网关主备节点的地址+端口
    prop.heartbeat_interval_milli = 5000;                           // 发送心跳的时间间隔，单位：毫秒
    prop.connect_timeout_milli = 5000;                              // 连接超时时间，单位：毫秒
    prop.reconnect_time = 10;                                       // 重试连接次数
    prop.client_name = "api_demo";                                  // 客户端程序名字
    prop.client_version = "V1.0.0";                                 // 客户端程序版本
    prop.report_sync = report_sync;                                 // 回报同步数据分区号+序号，首次是空，断线重连时填入的是接受到的最新分区号+序号
    prop.mode = 0;                                                  // 模式0-同步回报模式，模式1-快速登录模式，不同步回报

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
ATPRetCodeType close(ATPTradeAPI* client)
{
    g_waiting_flag.store(true);
    ATPRetCodeType ec = client->Close();
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke Close error:" << ec << std::endl;
        return ec;
    }

    while(g_waiting_flag.load())
    {
        sleep(0);
    }
    return ErrorCode::kSuccess;
}

// 登录
ATPRetCodeType login(ATPTradeAPI* client)
{
    // 设置登入消息
    ATPReqCustLoginOtherMsg login_msg;
    strncpy(login_msg.fund_account_id, fund_account_id.c_str(), 17);             // 资金账户ID
   
    char cipher_text[1024] = {0} ; 
    std::string secret_key = "410301" ; 
    KDEncode(KDCOMPLEX_ENCODE, (unsigned char*)password.data(), 6, (unsigned char*)cipher_text, sizeof(cipher_text)-1, (void *)secret_key.data(), secret_key.size()) ; 
  
    m_strEncryptFundPassword = cipher_text ;
    std::cout<<password<<" 密码加密后:  "<<m_strEncryptFundPassword<<std::endl ; 
    
    strncpy(login_msg.password, m_strEncryptFundPassword.data(), 129) ;

    login_msg.login_mode = ATPCustLoginModeType::kFundAccountIDMode;   // 登录模式，资金账号登录
    login_msg.client_seq_id = g_client_seq_id++;                // 客户系统消息号
    login_msg.order_way = order_way;                                  // 委托方式，自助委托
    login_msg.client_feature_code = g_client_feature_code;      // 终端识别码

    g_waiting_flag.store(true);
    ATPRetCodeType ec = client->ReqCustLoginOther(&login_msg);
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke CustLogin error:" << ec << std::endl;
        return ec;
    }

    while(g_waiting_flag.load())
    {
        sleep(0);
    }

    return ErrorCode::kSuccess;
}

// 登出
ATPRetCodeType logout(ATPTradeAPI* client)
{
    // 设置登出消息
    ATPReqCustLogoutOtherMsg logout_msg;
    strncpy(logout_msg.fund_account_id, fund_account_id.c_str(), 17);             // 资金账户ID
    logout_msg.client_seq_id = g_client_seq_id++;               // 客户系统消息号
    logout_msg.client_feature_code = g_client_feature_code;      // 终端识别码

    g_waiting_flag.store(true);
    ATPRetCodeType ec = client->ReqCustLogoutOther(&logout_msg);
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke CustLogout error:" << ec << std::endl;
        return ec;
    }

    while(g_waiting_flag.load())
    {
        sleep(0);
    }

    return ErrorCode::kSuccess;
}

// 发送订单
ATPRetCodeType send(ATPTradeAPI* client)
{
    // 上海市场股票限价委托
    ATPReqCashAuctionOrderMsg* p = new ATPReqCashAuctionOrderMsg;
    strncpy(p->security_id, "600031", 6);                   // 证券代码
    p->market_id = ATPMarketIDConst::kShangHai;             // 市场ID，上海
    strncpy(p->cust_id, cust_id.c_str(), 17);                    // 客户号ID
    strncpy(p->fund_account_id, fund_account_id.c_str(), 17);             // 资金账户ID
    strncpy(p->account_id, account_id_sh.c_str(), 13);                   // 账户ID
    p->side = ATPSideConst::kBuy;                           // 买卖方向，买
    p->order_type = ATPOrdTypeConst::kFixedNew;             // 订单类型，限价
    p->price = 136600;                                      // 委托价格 N13(4)，21.0000元
    p->order_qty = 100000;                                  // 申报数量N15(2)；股票为股、基金为份、上海债券默认为张（使用时请务必与券商确认），其他为张；1000.00股
    p->client_seq_id = g_client_seq_id++;                   // 用户系统消息序号
    p->order_way = order_way;                                     // 委托方式，自助委托
    p->client_feature_code = g_client_feature_code;         // 终端识别码

    ATPRetCodeType ec = client->ReqCashAuctionOrder(p);
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Invoke Send error:" << ec << std::endl;
    }

    delete p;
    return ec;
}

// 初始化连接并完成登录
bool init(ATPTradeAPI* client, ATPTradeHandler* handler)
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
void exit(ATPTradeAPI* client, ATPTradeHandler* handler)
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

int main(int argc, char* argv[])
{
    // 初始化API
    const std::string station_name = ""; // 站点信息，该字段已经不使用
    const std::string cfg_path=".";      // 配置文件路径
    const std::string log_dir_path = ""; // 日志路径
    bool record_all_flag = true;         // 是否记录所有委托信息
    std::unordered_map<std::string,std::string> encrypt_cfg; // 加密库配置
	//启用回话保持，可以断线重连
    bool connection_retention_flag=true;   // 是否启用会话保持


    ATPRetCodeType ec = ATPTradeAPI::Init(station_name,cfg_path,log_dir_path,record_all_flag,encrypt_cfg,connection_retention_flag);
    if (ec != ErrorCode::kSuccess)
    {
        std::cout << "Init failed: " << ec <<std::endl;
        return false;
    }
    // 获取tradeAPI客户端和回调句柄
    ATPTradeAPI* client = new ATPTradeAPI();
    CustHandler* handler = new CustHandler();
    
    if (init(client, handler))
    {
        for(int i=0;i<3 ;i++)
        {
            ATPRetCodeType ret =send(client);
            if (ret == ErrorCode::kSuccess)
            {
                std::cout <<"\n"<< "send order success : "<<i << std::endl;
            }
            else
                std::cout<<"send error"<< ATPTradeAPI::GetErrorInfo(ret,ATPErrorCodeTypeConst::kErrorCode)<<std::endl;

            sleep(1);
            
        }
    }

    exit(client, handler);
    delete client;
    delete handler;

    return 0;
}
