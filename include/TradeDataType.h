
#ifndef _TRADE_DATA_TYPE_H_
#define _TRADE_DATA_TYPE_H_

#include <string>
#include <vector>
#include <map>

// 交易相关结构体
struct PositionInfo
{
    char StockCode[7];
	int current_volume = 0;    // 当前数量
	int can_sell_volume = 0;   // 可卖数量

    int target_volume = 0;      //目标数量
    int real_volume = 0;        //实际数量
};

void print_position_Info(const PositionInfo& info);

struct AssetInfo
{
	double can_use_money = 0;         // 可用资金
    double stock_market_value = 0;    // 股票市值
};


//ut行情回执结构体
struct ut_code_Info
{
    char StockCode[7];           //֤证券代码
    std::string time;            //市场时间
    float real_price = 0.0;            //现价
    float bid_one_price = 0.0;            //买一价
    int bid_one = 0;                 //买一量
    int bid_two = 0;                 //买二量
    float yes_close = 0.0;             //昨日收盘价
    float open_price = 0.0;            //开盘价
    float riseup_price = 0.0;          //涨停价
    float downstop_price = 0.0;        //跌停价
};
extern std::map<std::string, ut_code_Info> code_info_vec_ptr;
//打印函数
void print_ut_code_Info(const ut_code_Info& info);

void addOrUpdateCodeInfo(const ut_code_Info& new_info);

// 请求结构体
struct LoginReqField
{
    int account;
    int password;
};

struct OrderReqField
{
    // char stock_code[7];
    std::string stock_code;
    int market;
    int direction;
    int order_quantity;
    double order_price;
    char order_way;  // 订单方式  1->限价 13->市价best5
};

void printOrderReq(const OrderReqField& info);
void writeOrdersToFile(const std::vector<OrderReqField>& orders, const std::string& filename);
struct ReqQryHoldField
{
    char ExchangeID[5];

    // 证券代码
    char StockCode[7];
};


// 应答消息结构体
// 客户登录应答信息
struct OnUserLoginField
{
    /// 营业部号
	int BranchID;
    /// 资产账号
	char AccountID[19];
    /// 客户姓名
    char UserName[32];
	/// 交易日
	int	TradingDay;
	///报单引用（返回会话上次最大委托引用）
	char OrderRef[33];
	///当前会话编号
	int SessionID;
	/// 客户编号
	char UserID[32];
};

struct OnOrderReqField
{
    // 报单分区
    int OrderPartition;
    // 经纪公司报单编码
    char BrokerOrderID[32];
    // 会话编号
    int SessionID;
    // 报单引用
    char OrderRef[33];
};

struct OnQryHoldField
{
    char StockCode[7];
    double CurrentVolume;
};



// 错误信息
struct ErrorInfo
{
    // 错误代码
    int error_code;
    // 错误信息
    char error_msg[256];
};

// 监控卖单涨停状态
struct SellInfo
{
	int total_sell_quantity = 0;
	int is_open_limit_up = 0;         // 是否开盘涨停
	int is_limit_up_change = 0;       // 涨停股票是否开盘
	int is_pre_limit_up_change = 0;   // 昨日涨停未处理股票
	int is_middle_limit_up = 0;       // 是否盘中涨停
	int is_deal_done = 0;             // 是否已处理
    std::string limit_up_time = "";             // 涨停时间
};

//报单信息结构体
struct ordertradeInfo
{
    char StockCode[7];           //证券代码
    char side;                   //买卖方向
    int order_qty;               //成交数量
};
//打印函数
void printOrderTradeInfo(const ordertradeInfo& info);


#endif
