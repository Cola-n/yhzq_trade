
#ifndef TRADE_TWAP_H
#define TRADE_TWAP_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <regex>

#include "TradeDataType.h"
#include "common_tool.h"
#include "read_paras_tool.h"
#include "trade_tool.h"
#include "mem_map.h"

class TradeTwapClass
{
public:
    TradeTwapClass(twap_para twap_paras, std::string fundname);

    std::string today;   //当天日期
    std::string last_trade_day;  //上个交易日日期
    std::string next_trade_day;  //下一个交易日日期
    std::string year; //今年年份

    std::string trade_day_path; //交易日文件的路径

    std::string project_dir;  //程序文件路径
    std::string fundname;  //产品名称
    std::string fund_dir;  //产品路径
    std::string order_dir;  //订单信号路径
    std::string twap_orders_dir;  //分单存储路径

// private:
   
    std::string buy_signal_path ;
    std::string sell_signal_path;
    std::string today_target_file_path;  // 今日最新的目标持仓文件
    std::string yes_target_file_path;  // 昨日最新的目标持仓文件
      
    std::map<std::string, PositionInfo> real_position;    // 实际持仓
    std::map<std::string, PositionInfo> target_position;  // 目标持仓
    std::vector<std::vector<std::string>> diff_position;  // 持仓差异
    std::string diff_white_list_path;  //持仓差异的白名单

    
    // twap和实时信号下单时间
    int twap_num;                                       //分单次数
    int trade_flag;
    std::vector<std::string> twap_name_vec = {"twap1", "twap2", "morning", "twap3", "morning2Two", "twap4", "afternoon", "twap5", "twap6", "afternoon2Two"};

    std::string trade_flag_file_path;                   //交易次数记录文件路径
    std::string twap_trade_start_time;                  //交易开始时间
    std::string twap_trade_end_time;                    //交易结束时间
    std::string judge_end_time;                         //持仓对比时间
    std::map <std::string, std::string> trade_time_map;

    int sleep_time;
    
    // 监控涨跌停相关变量
    bool is_inspect_rise_stop;
    char m_today[10];                                   // 日期
    char sell_list_path_today[128];                     // 需要监控的股票代码：今日卖单
    char sell_list_path_yes[128];                       // 需要监控的股票代码：昨日一字涨停未卖股票

    char mem_data_path[128];                            // 被映射的文件
    char mem_dir[128];                                  // 内存映射目录
    std::string mem_file_path;
    std::string mem_data_dir;
    char *data_p;                                       // 内存映射地址
    int is_deal_data_map;
    int is_init_sell_map;
    int start_read_data_to_judge;

    // 买卖单信息
    std::map<std::string, SellInfo> sell_map;
    std::vector<std::vector<struct OrderReqField>> sell_list;
    std::vector<std::vector<struct OrderReqField>> buy_list;
    std::vector<ut_code_Info> code_info_vec;

    // 监控记录文件路径
    std::string open_limit_up_record_path;
    std::string open_limit_up_change_record_path;
    std::string open_limit_up_not_change_record_path;
    std::string open_limit_up_pre_change_record_path;
    std::string deal_done_record_path;
    std::string middle_limit_up_record_path;

    // 交易记录文件路径
    std::string order_done_record_path;
    std::string send_order_error_record_path;
    std::string target_time_node_record_path;

    // 持仓对比差异记录文件
    std::string position_diff_file_path;

    // 资金相关
    double can_use_money;                               // 可用资金
    double can_out_money;                               // 可取资金

    //买卖单总花费
    double buy_total_cost = 0;
    double sell_total_cost = 0;

    int send_times = 0;        // 每秒报单数
    int total_send_times = 0;  // 总体报单数
    int cancel_order = 0;      // 撤单数
    int scrap_order = 0;       // 废单数

    int total_order_quantity = 0;      //总体成交数
    int last_order_quantity = 0;       // 查询成交返回最后索引
    int total_position_quantity = 0;   // 总体持仓数
    int last_position_quantity = 0;    // 查询持仓返回最后索引

    // 查询等待时间(s)
    int wait_seconds = 0;

public:
    int is_quit = 0;
    std::map<std::string, market_data> stock_data_map;  // 存储数据的map

    // 查询结果记录文件
    std::string query_position_record_path;
    std::string query_order_record_path;
    std::string query_trade_record_path;
    std::string query_money_record_path;
    std::string limit_price_path;                       //涨跌停文件路径

public:
    void get_trade_day();                               //获取交易日期等时间

public:
    void StartInspect();
    void get_market_data();
private:
    void init_sell_map();

public:
    void StartTwapTrade();
    void set_trade_flag(int num);
    void get_trade_flag();
    void StartUpdate();

    std::vector<struct OrderReqField> read_real_time_signal(std::string file_path, bool is_sell_signal);

    bool isBefore(int hour, int minute, int second);

    void order_split();

    std::vector<std::vector<OrderReqField>> split_orders(const std::string &signal_path, bool is_buy_order);

    void get_target_holding_map(std::string holdingPath);

    void update_target_holding_map(std::vector<struct OrderReqField> order_array);
    void update_target_holding_record_csv(std::vector<struct OrderReqField> order_array);
    std::string get_target_holding_file_path(std::string fundname, std::string order_dir, std::string date);

    std::vector<OrderReqField> get_orderRecode_fromCSV();

    void savetTwapOrdersToCsv(std::vector<std::vector<struct OrderReqField>> ordersList, std::string filename);

    void compare_position();
    std::string get_current_time_ms();
    void compare_position_before_trade();
    void print_diff_position_Info(std::vector<std::vector<std::string>> info);

    std::vector<struct OrderReqField> get_diff_order();
    
    std::string get_today_value();
public:
    virtual void query_position(int quert_index = 0) = 0;
    virtual void query_trade(int quert_index = 0) = 0;

    std::vector<ordertradeInfo> query_trade_array;
    AssetInfo query_asset;
    std::map<std::string, std::vector<double>> limit_price_map;
    virtual void order_insert(const OrderReqField &order_info) = 0;
};


#endif

