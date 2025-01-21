#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <chrono>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

#include <time.h>
#include <fstream>
#include <cstdarg>
#include <map>

#ifndef WINDOWS
#include <arpa/inet.h>
#include <dlfcn.h>
#endif

#include "profile.h"
#include "sse_report.h"
#include "sze_report.h"
#include "trade_twap.h"
#include "TradeDataType.h"
#include "read_paras_tool.h"
#include "trade_twap.h"
#include "trade_tool.h"

using namespace std;

#define INI_FILE_PATH "../config/efh_recv_conf.ini"
#define SZE_QUOTE_PARAM_NUM (9)
#define SSE_QUOTE_PARAM_NUM (10)

TIniFile ini;

extern std::map<std::string, ut_code_Info> code_info_vec_ptr;
extern std::vector<std::string> mapKeys;

std::chrono::system_clock::time_point parseTime(const std::string &timeStr)
{
    std::tm tm = {};
    tm.tm_hour = std::stoi(timeStr.substr(0, 2));
    tm.tm_min = std::stoi(timeStr.substr(2, 2));
    tm.tm_sec = std::stoi(timeStr.substr(4, 2));

    // 将std::tm转换为std::time_t
    std::time_t t = mktime(&tm);

    // 将std::time_t转换为std::chrono::system_clock::time_point
    return std::chrono::system_clock::from_time_t(t);
}

void TradeTwapClass::StartInspect()
{
    while (isBefore(9, 25, 0) && !sell_map.empty())
    {
        // std::cout << "wait for inspect time begin......" << std::endl;
        sleep(3);
    }
    // std::cout << get_now_time()<<std::endl;
    auto timePoint1 = parseTime(get_now_time()); // 涨停时间
    // while (code_info_vec_ptr.size() != sell_map.size()){
    //     sleep(1);
    // }

    // 检查是否开盘涨停, 统计剩余订单数量，开板后统一报单
    if (isBefore(15, 30, 0))
    {
        for (std::map<std::string, SellInfo>::iterator it = sell_map.begin(); it != sell_map.end(); ++it)
        {
            std::string key = it->first;
            SellInfo value = it->second;
            ut_code_Info code_info;
            if (code_info_vec_ptr.count(key) > 0)
            {
                code_info_vec_ptr[key].riseup_price = limit_price_map[key.substr(0, 6)][0];
                code_info_vec_ptr[key].downstop_price = limit_price_map[key.substr(0, 6)][1];
                code_info = code_info_vec_ptr[key];
            }
            else
            {
                // std::cerr << "Key " << key << " not found in code_info_vec_ptr" << std::endl;
                continue;
            }

            if (code_info.real_price >= code_info.riseup_price && code_info.bid_one_price >= code_info.riseup_price && code_info.riseup_price > 0)
            {
                // std::cout <<"real:" <<code_info.real_price<<"  rise:"<<code_info.riseup_price<<std::endl;
                std::cout << key << " rise top when open, now change rest orders to zero..." << std::endl;
                value.is_open_limit_up = 1;
                // 修改卖单中数量为0
                int reset_sell_qty = 0;
                for (std::vector<struct OrderReqField> &item : sell_list)
                {
                    for (OrderReqField &order : item)
                    {
                        if (order.stock_code.substr(0, 6) == key.substr(0, 6))
                        {
                            reset_sell_qty = order.order_quantity + reset_sell_qty;
                            order.order_quantity = 0;
                        }
                    }
                }
                it->second.total_sell_quantity = reset_sell_qty;
                it->second.is_open_limit_up = 1;
            }
        }
    }

    // std::cout << "inspect begin......" << std::endl;
    // std::cout << "=================================" << std::endl;
    while (isBefore(15, 57, 0) && !sell_map.empty())
    {
        for (std::map<std::string, SellInfo>::iterator it = sell_map.begin(); it != sell_map.end(); ++it)
        {
            std::string key = it->first;
            SellInfo value = it->second;
            // 监控未一字涨停股票是否盘中涨停, 修改、统计剩余委托量
            if (value.is_open_limit_up == 0 && value.is_deal_done == 0 && value.is_middle_limit_up == 0)
            {
                auto item = limit_price_map.find(key.substr(0, 6));
                if (item != limit_price_map.end() && item->second.size() >= 2)
                {
                    code_info_vec_ptr[key].riseup_price = item->second[0];
                    code_info_vec_ptr[key].downstop_price = item->second[1];
                }
                else
                {
                    // 处理不存在的情况
                    continue;
                }
                ut_code_Info code_info;
                if (code_info_vec_ptr.empty() || code_info_vec_ptr.find(key) == code_info_vec_ptr.end())
                {
                    continue;
                }
                // std::cout << key << std::endl;
                code_info = code_info_vec_ptr.at(key);
                if (code_info.real_price >= code_info.riseup_price && code_info.riseup_price > 0)
                {
                    std::cout << "real:" << code_info.real_price << "  rise:" << code_info.riseup_price << "    ";
                    std::cout << key << " rise top when middle, now change rest orders to zero..." << std::endl;
                    int reset_sell_qty = 0;
                    // 修改卖单中数量为0
                    int twap = stringToInt(twap_name_vec[trade_flag].substr(4, 1)) - 1;
                    if (twap > 6)
                    {
                        twap = stringToInt(twap_name_vec[trade_flag - 1].substr(4, 1)) - 1;
                    }
                    for (size_t i = twap; i < sell_list.size(); ++i)
                    {
                        for (OrderReqField &order : sell_list[i])
                        {
                            if (order.stock_code.substr(0, 6) == key.substr(0, 6))
                            {
                                reset_sell_qty = order.order_quantity + reset_sell_qty;
                                order.order_quantity = 0;
                            }
                        }
                    }
                    // 当天交易数量已全部执行
                    if (reset_sell_qty == 0)
                    {
                        it->second.is_deal_done == 1;
                    }
                    it->second.total_sell_quantity = reset_sell_qty;
                    it->second.is_middle_limit_up = 1;
                    it->second.limit_up_time = get_now_time(); // 记录涨停时间
                }
            }
            // 一字涨停是否开板
            else if (value.is_open_limit_up == 1 && value.is_deal_done == 0)
            {
                ut_code_Info code_info = code_info_vec_ptr[key];
                if (code_info.bid_one <= 1000000)
                {
                    std::cout << key << " Stock opens 1,  now all sold ... " << code_info.bid_one << std::endl;
                    OrderReqField order;
                    order.stock_code = key.substr(0, 6);
                    order.order_quantity = value.total_sell_quantity;
                    if (key[0] == '6')
                    {
                        order.market = 1;
                    }
                    else
                    {
                        order.market = 2;
                    }
                    order.direction = 2;
                    order.order_way = 1;
                    order.order_price = code_info.real_price * 0.99;
                    // order.order_price = code_info.real_price;
                    printOrderReq(order);
                    std::cout << get_current_time_ms() << std::endl;
                    order_insert(order);

                    std::vector<struct OrderReqField> open_sell_array;
                    open_sell_array.push_back(order);
                    update_target_holding_map(open_sell_array);
                    it->second.is_deal_done = 1;
                }
            }
            // 监控盘中涨停股票是否开板
            else if (value.is_middle_limit_up == 1 && value.is_deal_done == 0)
            {
                auto item = limit_price_map.find(key.substr(0, 6));
                if (item != limit_price_map.end() && item->second.size() >= 2)
                {
                    code_info_vec_ptr[key].riseup_price = item->second[0];
                    code_info_vec_ptr[key].downstop_price = item->second[1];
                }
                else
                {
                    // 处理不存在的情况
                    continue;
                }
                ut_code_Info code_info = code_info_vec_ptr[key];
                auto timePoint1 = parseTime(value.limit_up_time); // 涨停时间
                auto timePoint2 = parseTime(get_now_time());      // 现在时间

                // 计算时间差
                // auto duration = std::chrono::duration_cast<std::chrono::minutes>(timePoint2 - timePoint1);
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(timePoint2 - timePoint1);
                if (duration.count() < 60)
                {
                    // 股票仅短暂涨停，触发后立刻卖出剩余数量
                    // std::cout <<"real:" <<code_info.real_price<<"  rise:"<<code_info.riseup_price<<"    ";
                    if (code_info.real_price < code_info.riseup_price)
                    {
                        std::cout << key << " Stock opens 2, now all sold ... " << code_info.bid_one << std::endl;
                        // 下单剩余数量
                        OrderReqField order;
                        order.stock_code = key.substr(0, 6);
                        order.order_quantity = value.total_sell_quantity;
                        if (key[0] == '6')
                        {
                            order.market = 1;
                        }
                        else
                        {
                            order.market = 2;
                        }
                        order.direction = 2;
                        order.order_way = 1;
                        order.order_price = code_info.real_price * 0.99;
                        // order.order_price = code_info.real_price;
                        // printOrderReq(order);
                        std::cout << get_current_time_ms() << std::endl;
                        order_insert(order);
                        std::vector<struct OrderReqField> open_sell_array;
                        open_sell_array.push_back(order);
                        update_target_holding_map(open_sell_array);
                        it->second.is_deal_done = 1;
                    }
                }
                else
                {
                    // 涨停时间超过1分钟， 正式监控买一档委托量
                    if (code_info.bid_one <= 1000000 || code_info.real_price < code_info.riseup_price)
                    {
                        std::cout << key << " Stock opens 3, now all sold ... " << code_info.bid_one << std::endl;
                        // 下单剩余数量
                        OrderReqField order;
                        order.stock_code = key.substr(0, 6);
                        order.order_quantity = value.total_sell_quantity;
                        if (key[0] == '6')
                        {
                            order.market = 1;
                        }
                        else
                        {
                            order.market = 2;
                        }
                        order.direction = 2;
                        order.order_way = 1;
                        order.order_price = code_info.real_price * 0.99;
                        // order.order_price = code_info.real_price;
                        // printOrderReq(order);
                        std::cout << get_current_time_ms() << std::endl;
                        order_insert(order);
                        std::vector<struct OrderReqField> open_sell_array;
                        open_sell_array.push_back(order);
                        update_target_holding_map(open_sell_array);
                        it->second.is_deal_done = 1;
                    }
                }
            }
        }
        sleep(2.8);
    }
}

bool config_sock_udp_param(efh_channel_config &quote_param, const char *section)
{
    int enable = ini.ReadInt(section, "enable", 0);
    if (enable == 1)
    {
        char msg[4096];
        memset(msg, 0, sizeof(msg));

        quote_param.m_i_cpu_id = ini.ReadInt(section, "cpu_id", 0);

        int len = ini.ReadString(section, "multicast_ip", "", msg, sizeof(msg));
        if (len)
        {
            vector<string> str_vec;
            string_split(msg, str_vec, ";");
            quote_param.m_i_channel_num = str_vec.size() > NET_CHANNEL_NUM ? NET_CHANNEL_NUM : str_vec.size();
            for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++)
            {
                memset(quote_param.m_channel_info[i].m_ch_src_ip, 0, sizeof(quote_param.m_channel_info[i].m_ch_src_ip));
                memcpy(quote_param.m_channel_info[i].m_ch_src_ip, str_vec[i].c_str(), str_vec[i].length());
            }
        }
        len = ini.ReadString(section, "multicast_port", "", msg, sizeof(msg));
        if (len)
        {
            vector<string> str_vec;
            string_split(msg, str_vec, ";");
            for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++)
            {
                quote_param.m_channel_info[i].m_i_src_port = atoi(str_vec[i].c_str());
            }
        }
        len = ini.ReadString(section, "data_ip", "", msg, sizeof(msg));
        if (len)
        {
            vector<string> str_vec;
            string_split(msg, str_vec, ";");
            for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++)
            {
                memset(quote_param.m_channel_info[i].m_ch_dest_ip, 0, sizeof(quote_param.m_channel_info[i].m_ch_dest_ip));
                memcpy(quote_param.m_channel_info[i].m_ch_dest_ip, str_vec[i].c_str(), str_vec[i].length());
            }
        }
        len = ini.ReadString(section, "data_port", "", msg, sizeof(msg));
        if (len)
        {
            vector<string> str_vec;
            string_split(msg, str_vec, ";");
            for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++)
            {
                quote_param.m_channel_info[i].m_i_dest_port = atoi(str_vec[i].c_str());
            }
        }
        len = ini.ReadString(section, "eth_name", "", msg, sizeof(msg));
        if (len)
        {
            vector<string> str_vec;
            string_split(msg, str_vec, ";");
            for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++)
            {
                memset(quote_param.m_channel_info[i].m_ch_eth_name, 0, sizeof(quote_param.m_channel_info[i].m_ch_eth_name));
                memcpy(quote_param.m_channel_info[i].m_ch_eth_name, str_vec[i].c_str(), str_vec[i].length());
            }
        }
        len = ini.ReadString(section, "nic_type", "", msg, sizeof(msg));
        if (len)
        {
            vector<string> str_vec;
            string_split(msg, str_vec, ";");
            for (size_t i = 0; i < str_vec.size() && i < NET_CHANNEL_NUM; i++)
            {
                quote_param.m_channel_info[i].m_nic_type = (enum_efh_nic_type)atoi(str_vec[i].c_str());
            }
        }

        quote_param.m_ll_cache_size = ini.ReadInt(section, "cache_size", 0) * 1024ul * 1024ul;
        quote_param.m_ll_proc_data_wait_time = ini.ReadInt(section, "proc_data_wait_time", 10);
        quote_param.m_ll_normal_socket_rxbuf = ini.ReadInt(section, "normal_socket_rxbuf", 10) * 1024ul * 1024ul;
        quote_param.m_b_out_of_order_correction = (ini.ReadInt(section, "out_of_order_correction", 0) != 0);
        return true;
    }
    else
    {
        return false;
    }
}

exchange_authorize_config get_ats_config()
{
    exchange_authorize_config ret;
    ini.ReadString("ATS_SERVER", "server_ip", "127.0.0.1", ret.m_ch_server_ip, sizeof(ret.m_ch_server_ip));
    ret.m_i_server_port = ini.ReadInt("ATS_SERVER", "server_port", 0);
    ini.ReadString("ATS_SERVER", "local_ip", "127.0.0.1", ret.m_ch_local_ip, sizeof(ret.m_ch_local_ip));
    ret.m_i_local_port = ini.ReadInt("ATS_SERVER", "local_port", 0);

    char buf[4096];
    memset(buf, 0, sizeof(buf));
    ini.ReadString("ATS_SERVER", "user_id", "", buf, sizeof(buf));
    sscanf(buf, "%lld", &ret.m_user_id);

    ini.ReadString("ATS_SERVER", "sys", "", ret.m_sys, sizeof(ret.m_sys));
    ini.ReadString("ATS_SERVER", "machine", "", ret.m_machine, sizeof(ret.m_machine));
    ini.ReadString("ATS_SERVER", "full_name", "", ret.m_full_name, sizeof(ret.m_full_name));
    ini.ReadString("ATS_SERVER", "user_password", "", ret.m_user_pwd, sizeof(ret.m_user_pwd));
    return ret;
}

sze_report *run_sze()
{
    efh_channel_config quote_param[SZE_QUOTE_PARAM_NUM];
    int i = 0;

    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_TICK"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_tick;
        i++;
    }
    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_IDX"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_idx;
        i++;
    }
    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_SNAP"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_snap;
        i++;
    }
    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_AFTER_CLOSE"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_after_close;
        i++;
    }
    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_TREE"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_tree;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_IBR_TREE"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_ibr_tree;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_TURNOVER"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_turnover;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_BOND_SNAP"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_bond_snap;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SZE_LEV2_BOND_TICK"))
    {
        quote_param[i].m_efh_type = enum_efh_sze_lev2_bond_tick;
        i++;
    }
    int log_num = ini.ReadInt("REPORT", "auto_change_source_log_num", 0);
    sze_report *report = new sze_report();

    int ats_enable = ini.ReadInt("ATS_SERVER", "enable", 0);
    if (ats_enable)
    {
        exchange_authorize_config ats_config = get_ats_config();
        bool is_keep_connect = (0 != ini.ReadInt("ATS_SERVER", "is_keep_connect", 0));
        if (!report->init_with_ats(ats_config, quote_param, i, log_num, is_keep_connect))
        {
            printf("ATS connection login failed, unable to obtain multicast address information\n");
            return NULL;
        }
    }
    else
    {
        if (!report->init(quote_param, i, log_num))
        {
            return NULL;
        }
    }

    int report_quit = ini.ReadInt("REPORT", "report_when_quit", 0);
    int tick_detach_enable = ini.ReadInt("REPORT", "tick_detach_enable", 0);
    int symbol_filter = ini.ReadInt("REPORT", "enable_symbol_filter", 0);
    int64_t ll_auto_change_timeout = ini.ReadInt("REPORT", "auto_change_timeout", 15);
    bool b_auto_change_switch = (ini.ReadInt("REPORT", "auto_change_switch", 0) != 0);
    report->set_auto_change_source_config(b_auto_change_switch, ll_auto_change_timeout);
    report->set_tick_detach(tick_detach_enable);

    char msg[1024];
    memset(msg, 0, sizeof(msg));

    ini.ReadString("REPORT", "symbol", "", msg, sizeof(msg));
    report->run(report_quit, symbol_filter, msg);

    return report;
}

void release_sze(sze_report *report)
{
    report->close();
    delete report;
}

sse_report *run_sse()
{
    efh_channel_config quote_param[SSE_QUOTE_PARAM_NUM];
    int i = 0;

    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_TICK"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_tick;
        i++;
    }
    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_IDX"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_idx;
        i++;
    }
    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_SNAP"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_snap;
        i++;
    }
    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_OPTION"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_opt;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_TREE"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_tree;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_BOND_SNAP"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_bond_snap;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_BOND_TICK"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_bond_tick;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_TICK_MERGE"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_tick_merge;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SSE_LEV2_ETF"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_etf;
        i++;
    }

    if (config_sock_udp_param(quote_param[i], "EFH_SSE_STATIC_INFO"))
    {
        quote_param[i].m_efh_type = enum_efh_sse_lev2_static_info;
        i++;
    }
    int log_num = ini.ReadInt("REPORT", "auto_change_source_log_num", 0);
    sse_report *report = new sse_report();
    int ats_enable = ini.ReadInt("ATS_SERVER", "enable", 0);
    if (ats_enable)
    {
        exchange_authorize_config ats_config = get_ats_config();
        bool is_keep_connect = (0 != ini.ReadInt("ATS_SERVER", "is_keep_connect", 0));
        if (!report->init_with_ats(ats_config, quote_param, i, log_num, is_keep_connect))
        {
            printf("ATS connection login failed, unable to obtain multicast address information\n");
            return NULL;
        }
    }
    else
    {
        if (!report->init(quote_param, i, log_num))
        {
            return NULL;
        }
    }

    int report_quit = ini.ReadInt("REPORT", "report_when_quit", 0);
    int tick_detach_enable = ini.ReadInt("REPORT", "tick_detach_enable", 0);

    int symbol_filter = ini.ReadInt("REPORT", "enable_symbol_filter", 0);
    int64_t ll_auto_change_timeout = ini.ReadInt("REPORT", "auto_change_timeout", 15);
    bool b_auto_change_switch = (ini.ReadInt("REPORT", "auto_change_switch", 0) != 0);
    report->set_auto_change_source_config(b_auto_change_switch, ll_auto_change_timeout);
    report->set_tick_detach(tick_detach_enable);

    char msg[1024];
    memset(msg, 0, sizeof(msg));

    ini.ReadString("REPORT", "symbol", "", msg, sizeof(msg));
    report->run(report_quit, symbol_filter, msg);

    return report;
}

void release_sse(sse_report *report)
{
    report->close();
    delete report;
}

// bool string_split(const char* str_src, vector<string>& str_dst, const string& str_separator);
void help()
{
    printf(
        "%s%s%s%s%s%s%s%s%s%s",
        "------------------------------------------------------------------------\n",
        "quit                                                   Exit the program.\n",
        "show                                                 Display statistics.\n",
        "rdf <symbol>                                 Get symbol RDF information.\n",
        "ssequery <category_id> <trade_channel> <begin_seq> <end_seq>            \n",
        "szequery <channelNo> <begin_seq> <end_seq>                              \n",
        "get_symbol_sub\n",
        "symbol_sub <symbol_1> <symbol_2> <...> <symbol_N>\n",
        "symbol_unsub <symbol_1> <symbol_2> <...> <symbol_N>\n",
        "set_symbol_switch <[1 or 0]>\n");
}

void TradeTwapClass::get_market_data()
{
    char file[64] = INI_FILE_PATH;
    ini.Open(file);

    int sse_enable = ini.ReadInt("EFH_QUOTE_TYPE", "enable_sse", 0);
    int sze_enable = ini.ReadInt("EFH_QUOTE_TYPE", "enable_sze", 0);

    if (sse_enable == 0 && sze_enable == 0)
    {
        printf("sze enable and sse enable is all is 0\n");
        // return 0;
    }
    sse_report *sse = NULL;
    sze_report *sze = NULL;
    if (sze_enable == 1)
    {
        sze = run_sze();
        if (sze == NULL)
        {
            // return 0;
        }
    }
    if (sse_enable == 1)
    {
        sse = run_sse();
        if (sse == NULL)
        {
            // return 0;
        }
    }

    while (isBefore(15, 5, 0))
    {
        sleep(60);
    }

    std::cout << "  now time is over 15:00:00, project stopped..." << std::endl;
    if (sze_enable == 1)
    {
        release_sze(sze);
    }
    if (sse_enable == 1)
    {
        release_sse(sse);
    }
}

#define MAX_BUFFER_SIZE 1024

sse_report::sse_report()
{
    m_fp_lev2 = NULL;
    m_fp_idx = NULL;
    m_fp_option = NULL;
    m_fp_exe = NULL;
    m_fp_order = NULL;
    m_fp_tick = NULL;
    m_fp_tree = NULL;
    m_fp_bond = NULL;
    m_fp_bond_tick = NULL;
    m_fp_tick_merge = NULL;
    m_fp_etf = NULL;
    m_p_quote = NULL;

#ifdef TEST_SHENGLI_CODE_CONVERSION
    m_fp_it_code_lev2 = NULL;
    m_fp_it_code_option = NULL;
    m_fp_it_code_tree = NULL;
    m_fp_it_code_bond = NULL;
    m_fp_it_code_bond_tick = NULL;
#endif

    m_ll_idx_count = 0;
    m_ll_snap_count = 0;
    m_ll_option_count = 0;
    m_ll_exe_count = 0;
    m_ll_order_count = 0;
    m_ll_tree_count = 0;
    m_ll_bond_count = 0;
    m_ll_bond_tick_count = 0;
    m_ll_tick_merge_count = 0;
    m_ll_etf_count = 0;

    m_b_report_quit = false;
    m_b_tick_detach = false;

    m_snap = NULL;
    m_idx = NULL;
    m_option = NULL;
    m_exe = NULL;
    m_order = NULL;
    m_tree = NULL;
    m_bond_snap = NULL;
    m_bond_tick = NULL;
    m_tick_merge = NULL;
    m_etf = NULL;

#ifdef WINDOWS
    m_h_core = NULL;
#else
    m_h_core = NULL;
#endif
    /// map init
    for (size_t i = 0; i < 4; i++)
    {
        pkg_info tmp;

        m_map_pkg_info[i].insert(make_pair(SSE_LEV2_IDX_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SSE_LEV2_OPT_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SSE_LEV2_SNAP_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SSE_LEV2_TREE_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SSE_LEV2_BOND_SNAP_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SSE_LEV2_BOND_TICK_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SSE_LEV2_TICK_MERGE_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SSE_LEV2_ETF_MSG_TYPE, tmp));
    }
    m_i_auto_change_log_num = 0;
    m_last_id = 0;
    m_b_first = true;
    m_b_flag = false;
    m_i_after_write_count = 0;
    std::string str_full_dir = get_project_path() + "/log/sse_auto_change_source.log";
    m_auto_change_fp = fopen(str_full_dir.c_str(), "wb");
}

sse_report::~sse_report()
{
    fclose(m_auto_change_fp);
}

bool sse_report::init(efh_channel_config *p_param, int num, int auto_change_log_num)
{
    for (int i = 0; i < num; i++)
    {
        update_show_udp_session(p_param[i]);
    }

    m_i_auto_change_log_num = auto_change_log_num;
    const char *err_address = "operation::init:";
#ifdef WINDOWS
    m_h_core = LoadLibraryA(DLL_EFH_LEV2_DLL_NAME);
    if (m_h_core == NULL)
    {
        string msg = format_str("%s init: load dll:%s !\n", err_address, DLL_EFH_LEV2_DLL_NAME);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    func_create_efh_sse_lev2_api func_create =
        (func_create_efh_sse_lev2_api)GetProcAddress(m_h_core, CREATE_EFH_SSE_LEV2_API_FUNCTION);
    if (func_create == NULL)
    {
        string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }
#else
    m_h_core = dlopen(DLL_EFH_LEV2_DLL_NAME, RTLD_LAZY);
    if (m_h_core == NULL)
    {
        string msg = format_str("%s init: load dll:%s error: %s!\n", err_address, DLL_EFH_LEV2_DLL_NAME, dlerror());
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    func_create_efh_sse_lev2_api func_create =
        (func_create_efh_sse_lev2_api)dlsym(m_h_core, CREATE_EFH_SSE_LEV2_API_FUNCTION);
    if (func_create == NULL)
    {
        string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }
#endif
    m_p_quote = func_create();
    if (m_p_quote == NULL)
    {
        string msg = format_str("%s create sqs function ptr null.\n", err_address);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }
    m_p_quote->set_channel_config(p_param, num);
    if (!m_p_quote->init_sse(static_cast<efh_sse_lev2_api_event *>(this), static_cast<efh_sse_lev2_api_depend *>(this)))
    {
        string msg = format_str("%s init parse! error\n", err_address);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    return true;
}

bool sse_report::init_with_ats(
    exchange_authorize_config &ats_config,
    efh_channel_config *p_param,
    int num,
    int auto_change_log_num,
    bool is_keep_connection)
{
    m_i_auto_change_log_num = auto_change_log_num;
    const char *err_address = "operation::init:";
#ifdef WINDOWS
    m_h_core = LoadLibraryA(DLL_EFH_LEV2_DLL_NAME);
    if (m_h_core == NULL)
    {
        string msg = format_str("%s init: load dll:%s !\n", err_address, DLL_EFH_LEV2_DLL_NAME);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    func_create_efh_sse_lev2_api func_create =
        (func_create_efh_sse_lev2_api)GetProcAddress(m_h_core, CREATE_EFH_SSE_LEV2_API_FUNCTION);
    if (func_create == NULL)
    {
        string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }
#else
    m_h_core = dlopen(DLL_EFH_LEV2_DLL_NAME, RTLD_LAZY);
    if (m_h_core == NULL)
    {
        string msg = format_str("%s init: load dll:%s error: %s!\n", err_address, DLL_EFH_LEV2_DLL_NAME, dlerror());
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    func_create_efh_sse_lev2_api func_create =
        (func_create_efh_sse_lev2_api)dlsym(m_h_core, CREATE_EFH_SSE_LEV2_API_FUNCTION);
    if (func_create == NULL)
    {
        string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }
#endif
    m_p_quote = func_create();
    if (m_p_quote == NULL)
    {
        string msg = format_str("%s create sqs function ptr null.\n", err_address);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }
    bool ret = m_p_quote->set_channel_config_with_ats(p_param, num, ats_config, is_keep_connection);
    if (!ret)
    {
        return false;
    }
    if (!m_p_quote->init_sse(static_cast<efh_sse_lev2_api_event *>(this), static_cast<efh_sse_lev2_api_depend *>(this)))
    {
        string msg = format_str("%s init parse! error\n", err_address);
        efh_sse_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    return true;
}

void sse_report::set_tick_detach(bool enable)
{
    m_b_tick_detach = enable;
}

void sse_report::run(bool b_report_quit, bool symbol_filter, const char *symbol)
{
    m_b_report_quit = b_report_quit;

    if (m_b_report_quit)
    {
        m_snap = new sse_qt_node_snap[QT_SSE_QUOTE_COUNT];
        m_idx = new sse_qt_node_index[QT_SSE_QUOTE_COUNT];
        m_option = new sse_qt_node_stock_option[QT_SSE_QUOTE_COUNT];
        m_exe = new sse_qt_node_exe[QT_SSE_QUOTE_COUNT];
        m_order = new sse_qt_node_order[QT_SSE_QUOTE_COUNT];
        m_tree = new sse_qt_node_tree[QT_SSE_QUOTE_COUNT];
        m_bond_snap = new sse_qt_node_bond_snap[QT_SSE_QUOTE_COUNT];
        m_bond_tick = new sse_qt_node_bond_tick[QT_SSE_QUOTE_COUNT];
        m_tick_merge = new sse_qt_node_tick_merge[QT_SSE_QUOTE_COUNT];
        m_etf = new sse_qt_node_etf[QT_SSE_QUOTE_COUNT];

        memset(m_snap, 0, sizeof(sse_qt_node_snap) * QT_SSE_QUOTE_COUNT);
        memset(m_idx, 0, sizeof(sse_qt_node_index) * QT_SSE_QUOTE_COUNT);
        memset(m_option, 0, sizeof(sse_qt_node_stock_option) * QT_SSE_QUOTE_COUNT);
        memset(m_exe, 0, sizeof(sse_qt_node_exe) * QT_SSE_QUOTE_COUNT);
        memset(m_order, 0, sizeof(sse_qt_node_order) * QT_SSE_QUOTE_COUNT);
        memset(m_tree, 0, sizeof(sse_qt_node_tree) * QT_SSE_QUOTE_COUNT);
        memset(m_bond_snap, 0, sizeof(sse_qt_node_bond_snap) * QT_SSE_QUOTE_COUNT);
        memset(m_bond_tick, 0, sizeof(sse_qt_node_bond_tick) * QT_SSE_QUOTE_COUNT);
        memset(m_tick_merge, 0, sizeof(sse_qt_node_tick_merge) * QT_SSE_QUOTE_COUNT);
        memset(m_etf, 0, sizeof(sse_qt_node_etf) * QT_SSE_QUOTE_COUNT);
    }
    m_p_quote->set_symbol_filter_enable_switch(symbol_filter);
    string_split(symbol, m_vec_symbol, ",");
    if (symbol_filter && m_vec_symbol.empty())
    {
        printf("symbol filtering has been enabled, but the filter list is empty.\n");
    }
    if (m_vec_symbol.size())
    {
        for (int i = 0; i < (int)m_vec_symbol.size(); i++)
        {
            symbol_item tmp;
            strcpy(tmp.symbol, m_vec_symbol[i].c_str());
            m_p_quote->subscribe_symbol_filter_items(&tmp, 1);
        }
    }
    if (!m_p_quote->start_sse())
    {
        string msg = format_str("start parse error\n");
        efh_sse_lev2_error(msg.c_str(), msg.length());
    }
}

void sse_report::show()
{
    printf("--------------------< SSE Info >--------------------\n");
    efh_version version;
    m_p_quote->get_version(version);
    printf("version: %s\n", version.m_ch_api_version);
    for (size_t i = 0; i < 4; i++)
    {
        stringstream ss;
        ss << "--------------------< session " << i << " >--------------------\n";

        pkg_info tmp;
        pkg_tick_info tick_tmp;
        bool flag = (tick_tmp == m_tick_pkg_info[i]);
        for (auto iter = m_map_pkg_info[i].begin(); iter != m_map_pkg_info[i].end(); iter++)
        {
            flag = flag && (tmp == iter->second);

            ss << get_type_name(iter->first) << "\t[" << iter->second.multicast_ip << "]"
               << "[" << iter->second.multicast_port << "]\t"
               << " count: " << iter->second.count << ", lost_count: " << iter->second.count_lost
               << ", rollback_flag: " << iter->second.b_rollback_flag << "." << endl;
        }
        /// tick to string
        ss << "SSE_tick order count: " << m_tick_pkg_info[i].order_count << endl;
        ss << "SSE_tick exe count: " << m_tick_pkg_info[i].exe_count << endl;
        ss << "SSE_tick"
           << "\t[" << m_tick_pkg_info[i].multicast_ip << "]"
           << "[" << m_tick_pkg_info[i].multicast_port << "]\t"
           << " count: " << m_tick_pkg_info[i].count << ", lost_count: " << m_tick_pkg_info[i].count_lost
           << ", rollback_flag: " << m_tick_pkg_info[i].b_rollback_flag << "." << endl;

        if (!flag || i == 0)
        {
            printf("%s\n", ss.str().c_str());
        }
    }
    fflush(stdout);
}

void sse_report::show_rdp(const char *symbol)
{
    sse_static_msg_body msg;
    int ret = m_p_quote->get_static_info(symbol, msg);
    switch (ret)
    {
    case SSE_STATIC_INFO_OK:
        printf(
            "%u,%s,%u,%s,%d,%lf,%lf,%llu,%llu,%llu,%llu,%lf,%llu,%llu,%s,%u,%u,%d,%d,%s,%s\n",
            msg.m_exchange_id // 交易所id
            ,
            msg.m_symbol // 证券代码
            ,
            msg.m_send_time // 行情发送时间，时分秒毫秒
            ,
            msg.m_static_file_date // 静态文件日期，YYYYMMDD, 以'\0'结束
            ,
            msg.m_price_limit_type // 跌涨停限制类型
            ,
            msg.m_up_limit_price // 涨停价
            ,
            msg.m_down_limit_price // 跌停价
            ,
            msg.m_bid_qty_unit // 买数量单位
            ,
            msg.m_ask_qty_unit // 卖数量单位
            ,
            msg.m_limit_upper_qty // 限价申报数量上限
            ,
            msg.m_limit_lower_qty // 限价申报数量下限
            ,
            msg.m_price_changge_unit // 申报最小变价单位
            ,
            msg.m_market_upper_qty // 市价申报数量上限
            ,
            msg.m_market_lower_qty // 市价申报数量下限
            ,
            msg.m_security_name // 证券名称，以'\0'结束
            ,
            msg.m_ssecurity_type // 证券类型
            ,
            msg.m_sub_ssecurity_type // 证券子类型
            ,
            msg.m_finance_target_mark // 融资标的标志
            ,
            msg.m_ssecurity_target_mark // 融券标的标志
            ,
            msg.m_product_status // 产品状态, 以'\0'结束
            ,
            msg.m_listing_date // 上市日期，格式为YYYYMMDD, 以'\0'结束
        );
        break;
    case SSE_STATIC_INFO_SYMBOL_IS_INCORRECT:
        printf("symbol <%s> is incorrect\n", symbol);
        break;
    case SSE_STATIC_INFO_NOT_FOUND_SYMBOL:
        printf("symbol <%s> is not found\n", symbol);
        break;
    case SSE_STATIC_INFO_FOUND_SYMBOL_BUT_NO_VALUE:
        printf("symbol <%s> is found in map, but no value is available\n", symbol);
        break;
    default:
        break;
    }
    fflush(stdout);
}

void sse_report::close()
{
    if (m_p_quote == NULL)
    {
        return;
    }

    m_p_quote->stop_sse();
    m_p_quote->close_sse();
#ifdef WINDOWS
    func_destroy_efh_sse_lev2_api func_destroy =
        (func_destroy_efh_sse_lev2_api)GetProcAddress(m_h_core, DESTROY_EFH_SSE_LEV2_API_FUNCTION);
#else
    func_destroy_efh_sse_lev2_api func_destroy =
        (func_destroy_efh_sse_lev2_api)dlsym(m_h_core, DESTROY_EFH_SSE_LEV2_API_FUNCTION);
#endif
    if (func_destroy == NULL)
    {
        return;
    }

    if (m_b_report_quit)
    {
        report_efh_sse_lev2_snap();
        report_efh_sse_lev2_idx();
        report_efh_sse_lev2_option();
        report_efh_sse_lev2_exe();
        report_efh_sse_lev2_order();
        report_efh_sse_lev2_tree();
        report_efh_sse_lev2_bond_snap();
        report_efh_sse_lev2_bond_tick();
        report_efh_sse_lev2_tick_merge();
        report_efh_sse_lev2_etf();
    }

    if (m_fp_lev2)
    {
        fclose(m_fp_lev2);
        m_fp_lev2 = NULL;
    }

    if (m_fp_idx)
    {
        fclose(m_fp_idx);
        m_fp_idx = NULL;
    }

    if (m_fp_option)
    {
        fclose(m_fp_option);
        m_fp_option = NULL;
    }

    if (m_fp_exe)
    {
        fclose(m_fp_exe);
        m_fp_exe = NULL;
    }

    if (m_fp_order)
    {
        fclose(m_fp_order);
        m_fp_order = NULL;
    }

    if (m_fp_tick)
    {
        fclose(m_fp_tick);
        m_fp_tick = NULL;
    }

    if (m_fp_tree)
    {
        fclose(m_fp_tree);
        m_fp_tree = NULL;
    }

    if (m_fp_bond)
    {
        fclose(m_fp_bond);
        m_fp_bond = NULL;
    }

    if (m_fp_bond_tick)
    {
        fclose(m_fp_bond_tick);
        m_fp_bond_tick = NULL;
    }

    if (m_fp_tick_merge)
    {
        fclose(m_fp_tick_merge);
        m_fp_tick_merge = NULL;
    }
    if (m_fp_etf)
    {
        fclose(m_fp_etf);
        m_fp_etf = NULL;
    }

#ifdef TEST_SHENGLI_CODE_CONVERSION
    if (m_fp_it_code_lev2)
    {
        fclose(m_fp_it_code_lev2);
        m_fp_it_code_lev2 = NULL;
    }

    if (m_fp_it_code_option)
    {
        fclose(m_fp_it_code_option);
        m_fp_it_code_option = NULL;
    }

    if (m_fp_it_code_tree)
    {
        fclose(m_fp_it_code_tree);
        m_fp_it_code_tree = NULL;
    }

    if (m_fp_it_code_bond)
    {
        fclose(m_fp_it_code_bond);
        m_fp_it_code_bond = NULL;
    }

    if (m_fp_it_code_bond_tick)
    {
        fclose(m_fp_it_code_bond_tick);
        m_fp_it_code_bond_tick = NULL;
    }
#endif

    func_destroy(m_p_quote);
#ifdef WINDOWS
    FreeLibrary(m_h_core);
#else
    dlclose(m_h_core);
#endif
}

void sse_report::check_info_count(int id, int type, int64_t seq)
{
    if (id > 4)
    {
        return;
    }
    auto iter = m_map_pkg_info[id].find(type);
    if (iter == m_map_pkg_info[id].end())
    {
        return;
    }

    if (iter->second.last_seq >= 0)
    {
        auto num = seq - iter->second.last_seq - 1;
        if (num < 0)
        {
            iter->second.b_rollback_flag = true;
            printf("%s out of seq, last seq[%ld], cur seq[%ld].\n", get_type_name(type).c_str(), iter->second.last_seq, seq);
        }
        else if (num > 0)
        {
            iter->second.count_lost += num;
            lost_pkg_log(type, iter->second.last_seq, seq);
        }
    }

    iter->second.last_seq = seq;
    iter->second.count++;
}

void sse_report::check_tick_info_count(int type, int64_t seq, pkg_tick_info &info)
{
    if (info.last_seq >= 0)
    {
        auto num = seq - info.last_seq - 1;
        if (num < 0)
        {
            info.b_rollback_flag = true;
            printf("%s out of seq, last seq[%ld], cur seq[%ld].\n", get_type_name(type).c_str(), info.last_seq, seq);
        }
        else if (num > 0)
        {
            info.count_lost += num;
            lost_pkg_log(type, info.last_seq, seq);
        }
    }

    info.last_seq = seq;
    info.count++;
    switch (type)
    {
    case SSE_LEV2_ORDER_MSG_TYPE:
    case SZE_LEV2_ORDER_MSG_TYPE:
    case SZE_LEV2_BOND_ORDER_MSG_TYPE:
        info.order_count++;
        break;
    case SSE_LEV2_EXE_MSG_TYPE:
    case SZE_LEV2_EXE_MSG_TYPE:
    case SZE_LEV2_BOND_EXE_MSG_TYPE:
        info.exe_count++;
        break;
    default:
        break;
    }
}

void sse_report::on_report_efh_sse_lev2_idx(session_identity id, sse_hpf_idx *p_index) {}

void sse_report::on_report_efh_sse_lev2_snap(session_identity id, sse_hpf_lev2 *p_snap)
{
    check_info_count(id.id, SSE_LEV2_SNAP_MSG_TYPE, p_snap->m_head.m_sequence);
    // auto_change_log_info(id.id, get_key(*p_snap));
    int node = m_ll_snap_count % QT_SSE_QUOTE_COUNT;
    if (!m_b_report_quit)
    {
        if (p_snap == nullptr || p_snap->m_symbol == nullptr)
        {
            // 处理错误，可能是日志记录或直接返回
            return;
        }
        std::string code = p_snap->m_symbol;

        // 行情筛选 是否存在今日卖单中
        if (std::find(mapKeys.begin(), mapKeys.end(), code.substr(0, 6)) == mapKeys.end())
        {
            return;
        }
        // 原本数据
        ut_code_Info orig_code_info;
        if (code_info_vec_ptr.find(code) == code_info_vec_ptr.end())
        {
            orig_code_info = code_info_vec_ptr[code];
        }

        orig_code_info.time = std::to_string(static_cast<int>(std::floor(p_snap->m_quote_update_time)));
        orig_code_info.real_price = p_snap->m_last_price / 1000.0;
        orig_code_info.yes_close = p_snap->m_pre_close_price / 1000.0;
        orig_code_info.open_price = p_snap->m_open_price / 1000.0;
        orig_code_info.bid_one_price =p_snap-> m_bid_unit[0].m_price / 1000.0;
        orig_code_info.bid_one = p_snap-> m_bid_unit[0].m_quantity / 1000.0;
        orig_code_info.bid_two = p_snap-> m_bid_unit[1].m_quantity / 1000.0;
        code_info_vec_ptr[code] = orig_code_info;
    }
}

void sse_report::on_report_efh_sse_lev2_option(session_identity id, sse_hpf_stock_option *p_option) {}

void sse_report::on_report_efh_sse_lev2_tick(session_identity id, int msg_type, sse_hpf_order *p_order, sse_hpf_exe *p_exe) {}

void sse_report::on_report_efh_sse_lev2_tree(session_identity id, sse_hpf_tree *p_tree) {}

void sse_report::on_report_efh_sse_lev2_bond_snap(session_identity id, sse_hpf_bond_snap *p_bond) {}

void sse_report::on_report_efh_sse_lev2_bond_tick(session_identity id, sse_hpf_bond_tick *p_tick) {}

void sse_report::on_report_efh_sse_lev2_tick_merge(session_identity id, sse_hpf_tick_merge *p_tick) {}

void sse_report::on_report_efh_sse_lev2_etf(session_identity id, sse_hpf_etf *p_tick) {}

void sse_report::efh_sse_lev2_debug(const char *msg, int len)
{
    printf("[DEBUG] %s\n", msg);
}

void sse_report::efh_sse_lev2_error(const char *msg, int len)
{
    printf("[ERROR] %s\n", msg);
}

void sse_report::efh_sse_lev2_info(const char *msg, int len)
{
    printf("[INFO] %s\n", msg);
}

string sse_report::format_str(const char *pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    char buffer[40960];
    vsnprintf(buffer, 40960, pFormat, args);
    va_end(args);
    return string(buffer);
}

void sse_report::report_efh_sse_lev2_idx() {}

void sse_report::report_efh_sse_lev2_snap()
{
    if (m_fp_lev2 == NULL)
    {
        time_t now = time(NULL);
        tm *ltm = localtime(&now);

        char str_full_name[1024];
        memset(str_full_name, 0, sizeof(str_full_name));
        sprintf(str_full_name, "%04d%02d%02d_sse_snap.csv", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday);

        m_fp_lev2 = fopen(str_full_name, "wt+");
        if (m_fp_lev2 == NULL)
        {
            return;
        }
    }

    if (m_ll_snap_count > 0)
    {
        for (int i = 0; i < QT_SSE_QUOTE_COUNT; i++)
        {
            if (m_snap[i].m_local_time == 0)
            {
                return;
            }

            char ch_buffer[1024];
            memset(ch_buffer, 0, sizeof(ch_buffer));

            sprintf(
                ch_buffer,
                "%s, 0, %u, %lld, %u, %lld, %lld, %u, %lld, %u, %lld, %u, %lld, %u, %lld\n",
                m_snap[i].m_symbol,
                m_snap[i].m_quote_update_time,
                m_snap[i].m_local_time,
                m_snap[i].m_last_price,
                m_snap[i].m_total_quantity,
                m_snap[i].m_total_value,
                m_snap[i].m_bid_1_price,
                m_snap[i].m_bid_1_quantity,
                m_snap[i].m_ask_1_price,
                m_snap[i].m_ask_1_quantity,
                m_snap[i].m_bid_10_price,
                m_snap[i].m_bid_10_quantity,
                m_snap[i].m_ask_10_price,
                m_snap[i].m_ask10_quantity);

            int ret = fwrite(ch_buffer, strlen(ch_buffer), 1, m_fp_lev2);
            fflush(m_fp_lev2);
            if (ret <= 0)
            {
                printf("write sse_snap.csv error!\t msg : %s\n", ch_buffer);
            }
        }
    }
}

void sse_report::report_efh_sse_lev2_option() {}

void sse_report::report_efh_sse_lev2_exe() {}

void sse_report::report_efh_sse_lev2_order() {}

void sse_report::report_efh_sse_lev2_tree() {}

void sse_report::report_efh_sse_lev2_bond_snap() {}

void sse_report::report_efh_sse_lev2_bond_tick() {}

void sse_report::report_efh_sse_lev2_tick_merge() {}

void sse_report::report_efh_sse_lev2_etf() {}

string sse_report::get_sse_src_trading_phase_code(char ch_sl_trading_phase_code)
{
    char ch_buf[1024];
    memset(ch_buf, 0, sizeof(ch_buf));
    char ch_first = ' ';
    switch (ch_sl_trading_phase_code & 0xF0)
    {
    case 0x00:
        ch_first = 'S';
        break;
    case 0x10:
        ch_first = 'C';
        break;
    case 0x20:
        ch_first = 'T';
        break;
    case 0x30:
        ch_first = 'E';
        break;
    case 0x40:
        ch_first = 'P';
        break;
    case 0x50:
        ch_first = 'M';
        break;
    case 0x60:
        ch_first = 'N';
        break;
    case 0x70:
        ch_first = 'U';
        break;
    case 0x80:
        ch_first = 'V';
        break;
    default:
        break;
    }

    ch_buf[0] = ch_first;

    if ((ch_sl_trading_phase_code & 0x08) == 0x08)
    {
        ch_buf[1] = '1';
    }
    else if ((ch_sl_trading_phase_code & 0x08) == 0x00)
    {
        ch_buf[1] = '0';
    }

    if ((ch_sl_trading_phase_code & 0x04) == 0x04)
    {
        ch_buf[2] = '1';
    }
    else if ((ch_sl_trading_phase_code & 0x04) == 0x00)
    {
        ch_buf[2] = '0';
    }

    if ((ch_sl_trading_phase_code & 0x02) == 0x02)
    {
        ch_buf[3] = '1';
    }
    else if ((ch_sl_trading_phase_code & 0x02) == 0x00)
    {
        ch_buf[3] = '0';
    }

    string str_buf = ch_buf;
    return str_buf;
}

string sse_report::get_sse_src_instrument_status(char ch_sl_instrument_status)
{
    char ch_buf[1024];
    memset(ch_buf, 0, sizeof(ch_buf));
    switch (ch_sl_instrument_status)
    {
    case 0:
        strcpy(ch_buf, " ");
        break;
    case 1:
        strcpy(ch_buf, "START");
        break;
    case 2:
        strcpy(ch_buf, "OCALL");
        break;
    case 3:
        strcpy(ch_buf, "TRADE");
        break;
    case 4:
        strcpy(ch_buf, "SUSP");
        break;
    case 5:
        strcpy(ch_buf, "CCALL");
        break;
    case 6:
        strcpy(ch_buf, "CLOSE");
        break;
    case 7:
        strcpy(ch_buf, "ENDTR");
        break;
    case 8:
        strcpy(ch_buf, "ADD");
        break;
    default:
        strcpy(ch_buf, " ");
        break;
    }

    string str_buf = ch_buf;
    return str_buf;
}

string sse_report::get_sse_bond_trading_phase_code_by_instrument_status(char ch_sl_instrument_status)
{
    char ch_buf[9];
    memset(ch_buf, 0, sizeof(ch_buf));
    switch (ch_sl_instrument_status)
    {
    case 0:
        strcpy(ch_buf, "        ");
        break;
    case 1:
        strcpy(ch_buf, "S       ");
        break;
    case 2:
        strcpy(ch_buf, "C       ");
        break;
    case 3:
        strcpy(ch_buf, "T       ");
        break;
    case 4:
        strcpy(ch_buf, "P       ");
        break;
    case 6:
        strcpy(ch_buf, "E       ");
        break;
    case 7:
        strcpy(ch_buf, "E       ");
        break;
    case 8:
        strcpy(ch_buf, "        ");
        break;
    default:
        strcpy(ch_buf, "        ");
        break;
    }

    string str_buf = ch_buf;
    return str_buf;
}

void sse_report::odp_query(char *ip, int port, int category_id, int trade_channel, long long begin_seq, long long end_seq)
{
    if (m_p_quote)
    {
        odp_event.count = 0;
        bool ret = m_p_quote->odp_rebuild_quote(ip, port, &odp_event, category_id, trade_channel, begin_seq, end_seq);
        auto count = odp_event.count;
        printf("odp query finish ,result is [%s] count is [%d]\n", ret ? "true" : "false", count);
    }
}

bool sse_report::get_symbol_status(vector<symbol_item> &list)
{
    if (!m_p_quote)
    {
        return false;
    }

    int num = 1024 * 1024;
    symbol_item *buf = new symbol_item[num];

    bool enable;
    int symbol_num = m_p_quote->get_symbol_filter_list_and_enable_switch(buf, &num, &enable);
    if (symbol_num < num)
    {
        delete[] buf;
        buf = new symbol_item[symbol_num];
        num = symbol_num;
        m_p_quote->get_symbol_filter_list_and_enable_switch(buf, &num, &enable);
    }

    for (int i = 0; i < num; i++)
    {
        list.emplace_back(buf[i]);
    }
    delete[] buf;
    return enable;
}

void sse_report::sub_symbol(vector<symbol_item> &list)
{
    if (m_p_quote)
        m_p_quote->subscribe_symbol_filter_items(list.data(), list.size());
}
void sse_report::unsub_symbol(vector<symbol_item> &list)
{
    if (m_p_quote)
        m_p_quote->unsubscribe_symbol_filter_items(list.data(), list.size());
}
void sse_report::set_symbol_filter_enable_switch(bool flag)
{
    if (m_p_quote)
        m_p_quote->set_symbol_filter_enable_switch(flag);
}

void sse_report::auto_change_log_info(int id, const string &msg)
{
    if (m_i_auto_change_log_num == 0)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_log_mtx);
    if (m_b_first)
    {
        m_b_first = false;
        m_last_id = id;
        m_b_flag = false;
    }

    if (id == m_last_id && !m_b_flag)
    {
        m_auto_change_log_info.push_back(msg);
        while (m_auto_change_log_info.size() > m_i_auto_change_log_num)
        {
            m_auto_change_log_info.pop_front();
        }
        return;
    }
    /// 切换中,记录切换后前N条
    if (id == m_last_id && m_b_flag)
    {
        if (m_i_after_write_count >= m_i_auto_change_log_num)
        {
            m_b_flag = false;
            return;
        }
        if (m_auto_change_fp)
        {
            fprintf(m_auto_change_fp, "%s\n", msg.c_str());
            fflush(m_auto_change_fp);
        }
        m_i_after_write_count++;

        return;
    }

    /// 发生切换了
    m_i_after_write_count = 0;
    m_b_flag = true;
    m_last_id = id;
    if (m_auto_change_fp)
    {
        fprintf(m_auto_change_fp, "before switch source\n");
    }
    for (auto iter = m_auto_change_log_info.begin(); iter != m_auto_change_log_info.end(); iter++)
    {
        fprintf(m_auto_change_fp, "%s\n", iter->c_str());
    }
    m_auto_change_log_info.clear();
    if (m_auto_change_fp)
    {
        fprintf(m_auto_change_fp, "after switch source\n");
    }

    if (m_i_after_write_count >= m_i_auto_change_log_num)
    {
        m_b_flag = false;
        return;
    }
    if (m_auto_change_fp)
    {
        fprintf(m_auto_change_fp, "%s\n", msg.c_str());
        fflush(m_auto_change_fp);
    }
    m_i_after_write_count++;
}

void sse_report::update_show_udp_session(const efh_channel_config &cfg)
{
    auto key = get_type_from_channel(cfg.m_efh_type);
    for (int i = 0; i < cfg.m_i_channel_num; i++)
    {
        string mip = cfg.m_channel_info[i].m_ch_src_ip;
        int mport = cfg.m_channel_info[i].m_i_src_port;

        if (cfg.m_efh_type == enum_efh_sse_lev2_tick)
        {
            m_tick_pkg_info[i].multicast_ip = mip;
            m_tick_pkg_info[i].multicast_port = mport;
        }
        else
        {
            auto iter = m_map_pkg_info[i].find(key);
            if (iter == m_map_pkg_info[i].end())
            {
                continue;
            }
            iter->second.multicast_ip = mip;
            iter->second.multicast_port = mport;
        }
    }
}

bool sse_report::set_auto_change_source_config(bool b_flag, int64_t ll_time)
{
    return m_p_quote->set_auto_change_source_config(b_flag, ll_time);
}

sze_report::sze_report()
{
    m_fp_lev2 = NULL;
    m_fp_idx = NULL;
    m_fp_ord = NULL;
    m_fp_exe = NULL;
    m_fp_close_px = NULL;
    m_fp_tree = NULL;
    m_fp_ibr_tree = NULL;
    m_fp_turnover = NULL;
    m_fp_bond_snap = NULL;
    m_fp_bond_ord = NULL;
    m_fp_bond_exe = NULL;
    m_fp_tick = NULL;
    m_fp_bond_tick = NULL;
    m_p_quote = NULL;

    m_snap = NULL;
    m_idx = NULL;
    m_order = NULL;
    m_exe = NULL;
    m_after_close = NULL;
    m_tree = NULL;
    m_ibr_tree = NULL;
    m_turnover = NULL;
    m_bond_snap = NULL;
    m_bond_order = NULL;
    m_bond_exe = NULL;

    m_ll_snap_count = 0;
    m_ll_idx_count = 0;
    m_ll_order_count = 0;
    m_ll_exe_count = 0;
    m_ll_after_count = 0;
    m_ll_tree_count = 0;
    m_ll_ibr_tree_count = 0;
    m_ll_turnover_count = 0;
    m_ll_bond_snap_count = 0;
    m_ll_bond_order_count = 0;
    m_ll_bond_exe_count = 0;
    m_b_report_quit = false;
    m_b_tick_detach = false;

#ifdef WINDOWS
    m_h_core = NULL;
#else
    m_h_core = NULL;
#endif
    /// map init
    for (size_t i = 0; i < 4; i++)
    {
        pkg_info tmp;

        // m_map_pkg_info[i].insert(make_pair(SSE_LEV2_IDX_MSG_TYPE,tmp));
        // m_map_pkg_info[i].insert(make_pair(SSE_LEV2_OPT_MSG_TYPE,tmp));
        // m_map_pkg_info[i].insert(make_pair(SSE_LEV2_SNAP_MSG_TYPE,tmp));
        // m_map_pkg_info[i].insert(make_pair(SSE_LEV2_TREE_MSG_TYPE,tmp));
        // m_map_pkg_info[i].insert(make_pair(SSE_LEV2_BOND_SNAP_MSG_TYPE,tmp));
        // m_map_pkg_info[i].insert(make_pair(SSE_LEV2_BOND_TICK_MSG_TYPE,tmp));
        // m_map_pkg_info[i].insert(make_pair(SSE_LEV2_TICK_MERGE_MSG_TYPE,tmp));
        // m_map_pkg_info[i].insert(make_pair(SSE_LEV2_ETF_MSG_TYPE,tmp));

        m_map_pkg_info[i].insert(make_pair(SZE_LEV2_SNAP_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SZE_LEV2_IDX_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SZE_LEV2_AF_CLOSE_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SZE_LEV2_TREE_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SZE_LEV2_IBR_TREE_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SZE_LEV2_TURNOVER_MSG_TYPE, tmp));
        m_map_pkg_info[i].insert(make_pair(SZE_LEV2_BOND_SNAP_MSG_TYPE, tmp));
    }
    m_i_auto_change_log_num = 0;
    m_last_id = 0;
    m_b_first = true;
    m_b_flag = false;
    m_i_after_write_count = 0;
    std::string str_full_dir = get_project_path() + "/log/sze_auto_change_source.log";
    m_auto_change_fp = fopen(str_full_dir.c_str(), "wb");
}

sze_report::~sze_report()
{
    fclose(m_auto_change_fp);
}

bool sze_report::init(efh_channel_config *param, int num, int auto_change_log_num)
{
    for (int i = 0; i < num; i++)
    {
        update_show_udp_session(param[i]);
    }

    m_i_auto_change_log_num = auto_change_log_num;
    const char *err_address = "operation::init: ";
#ifdef WINDOWS
    m_h_core = LoadLibraryA(DLL_EFH_LEV2_DLL_NAME);
    if (m_h_core == NULL)
    {
        string msg = format_str("%s init: load dll:%s!\n", err_address, DLL_EFH_LEV2_DLL_NAME);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    func_create_efh_sze_lev2_api func_create =
        (func_create_efh_sze_lev2_api)GetProcAddress(m_h_core, CREATE_EFH_SZE_LEV2_API_FUNCTION);
    if (func_create == NULL)
    {
        string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }
#else
    m_h_core = dlopen(DLL_EFH_LEV2_DLL_NAME, RTLD_LAZY);
    if (m_h_core == NULL)
    {
        string msg = format_str("%s init: load dll:%s error:%s!\n", err_address, DLL_EFH_LEV2_DLL_NAME, dlerror());
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    func_create_efh_sze_lev2_api func_create =
        (func_create_efh_sze_lev2_api)dlsym(m_h_core, CREATE_EFH_SZE_LEV2_API_FUNCTION);
    if (func_create == NULL)
    {
        string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }
#endif

    m_p_quote = func_create();
    if (m_p_quote == NULL)
    {
        string msg = format_str("%s create sqs function ptr null.\n", err_address);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }
    m_p_quote->set_channel_config(param, num);
    if (!m_p_quote->init_sze(static_cast<efh_sze_lev2_api_event *>(this), static_cast<efh_sze_lev2_api_depend *>(this)))
    {
        string msg = format_str("%s init parse! error\n", err_address);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    return true;
}

bool sze_report::init_with_ats(
    exchange_authorize_config &ats_config,
    efh_channel_config *param,
    int num,
    int auto_change_log_num,
    bool is_keep_connection)
{
    m_i_auto_change_log_num = auto_change_log_num;
    const char *err_address = "operation::init: ";
#ifdef WINDOWS
    m_h_core = LoadLibraryA(DLL_EFH_LEV2_DLL_NAME);
    if (m_h_core == NULL)
    {
        string msg = format_str("%s init: load dll:%s!\n", err_address, DLL_EFH_LEV2_DLL_NAME);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    func_create_efh_sze_lev2_api func_create =
        (func_create_efh_sze_lev2_api)GetProcAddress(m_h_core, CREATE_EFH_SZE_LEV2_API_FUNCTION);
    if (func_create == NULL)
    {
        string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }
#else
    m_h_core = dlopen(DLL_EFH_LEV2_DLL_NAME, RTLD_LAZY);
    if (m_h_core == NULL)
    {
        string msg = format_str("%s init: load dll:%s error:%s!\n", err_address, DLL_EFH_LEV2_DLL_NAME, dlerror());
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    func_create_efh_sze_lev2_api func_create =
        (func_create_efh_sze_lev2_api)dlsym(m_h_core, CREATE_EFH_SZE_LEV2_API_FUNCTION);
    if (func_create == NULL)
    {
        string msg = format_str("%s get create sqs function ptr failed.\n", err_address);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }
#endif

    m_p_quote = func_create();
    if (m_p_quote == NULL)
    {
        string msg = format_str("%s create sqs function ptr null.\n", err_address);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }
    bool ret = m_p_quote->set_channel_config_with_ats(param, num, ats_config, is_keep_connection);
    if (!ret)
    {
        return false;
    }
    if (!m_p_quote->init_sze(static_cast<efh_sze_lev2_api_event *>(this), static_cast<efh_sze_lev2_api_depend *>(this)))
    {
        string msg = format_str("%s init parse! error\n", err_address);
        efh_sze_lev2_error(msg.c_str(), msg.length());
        return false;
    }

    return true;
}

void sze_report::set_tick_detach(bool enable)
{
    m_b_tick_detach = enable;
}

void sze_report::run(bool b_report_quit, bool symbol_filter, const char *symbol)
{
    m_b_report_quit = b_report_quit;

    if (m_b_report_quit)
    {
        m_snap = new sze_qt_node_snap[QT_SZE_QUOTE_COUNT];
        m_idx = new sze_qt_node_index[QT_SZE_QUOTE_COUNT];
        m_order = new sze_qt_node_order[QT_SZE_QUOTE_COUNT];
        m_exe = new sze_qt_node_exe[QT_SZE_QUOTE_COUNT];
        m_after_close = new sze_qt_node_after_close[QT_SZE_QUOTE_COUNT];
        m_tree = new sze_qt_node_tree[QT_SZE_QUOTE_COUNT];
        m_ibr_tree = new sze_qt_node_ibr_tree[QT_SZE_QUOTE_COUNT];
        m_turnover = new sze_qt_node_turnover[QT_SZE_QUOTE_COUNT];
        m_bond_snap = new sze_qt_node_snap[QT_SZE_QUOTE_COUNT];
        m_bond_order = new sze_qt_node_order[QT_SZE_QUOTE_COUNT];
        m_bond_exe = new sze_qt_node_exe[QT_SZE_QUOTE_COUNT];

        memset(m_snap, 0, sizeof(sze_qt_node_snap) * QT_SZE_QUOTE_COUNT);
        memset(m_idx, 0, sizeof(sze_qt_node_index) * QT_SZE_QUOTE_COUNT);
        memset(m_order, 0, sizeof(sze_qt_node_order) * QT_SZE_QUOTE_COUNT);
        memset(m_exe, 0, sizeof(sze_qt_node_exe) * QT_SZE_QUOTE_COUNT);
        memset(m_after_close, 0, sizeof(sze_qt_node_after_close) * QT_SZE_QUOTE_COUNT);
        memset(m_tree, 0, sizeof(sze_qt_node_tree) * QT_SZE_QUOTE_COUNT);
        memset(m_ibr_tree, 0, sizeof(sze_qt_node_ibr_tree) * QT_SZE_QUOTE_COUNT);
        memset(m_turnover, 0, sizeof(sze_qt_node_turnover) * QT_SZE_QUOTE_COUNT);
        memset(m_bond_snap, 0, sizeof(sze_qt_node_snap) * QT_SZE_QUOTE_COUNT);
        memset(m_bond_order, 0, sizeof(sze_qt_node_order) * QT_SZE_QUOTE_COUNT);
        memset(m_bond_exe, 0, sizeof(sze_qt_node_exe) * QT_SZE_QUOTE_COUNT);
    }
    m_p_quote->set_symbol_filter_enable_switch(symbol_filter);
    string_split(symbol, m_vec_symbol, ",");
    if (symbol_filter && m_vec_symbol.empty())
    {
        printf("symbol filtering has been enabled, but the filter list is empty.\n");
    }
    if (m_vec_symbol.size())
    {
        for (int i = 0; i < (int)m_vec_symbol.size(); i++)
        {
            symbol_item tmp;
            strcpy(tmp.symbol, m_vec_symbol[i].c_str());
            m_p_quote->subscribe_symbol_filter_items(&tmp, 1);
        }
    }
    if (!m_p_quote->start_sze())
    {
        string msg = format_str("start parse error\n");
        efh_sze_lev2_error(msg.c_str(), msg.length());
    }
}

void sze_report::show()
{
    printf("--------------------< SZE Info >--------------------\n");
    efh_version version;
    m_p_quote->get_version(version);
    printf("version: %s\n", version.m_ch_api_version);
    for (size_t i = 0; i < 4; i++)
    {
        stringstream ss;
        ss << "--------------------< session " << i << " >--------------------\n";

        pkg_info tmp;
        pkg_tick_info tick_tmp;
        bool flag = (tick_tmp == m_tick_pkg_info[i] && tick_tmp == m_bond_tick_pkg_info[i]);
        for (auto iter = m_map_pkg_info[i].begin(); iter != m_map_pkg_info[i].end(); iter++)
        {
            flag = flag && (tmp == iter->second);

            ss << get_type_name(iter->first) << "\t[" << iter->second.multicast_ip << "]"
               << "[" << iter->second.multicast_port << "]\t"
               << " count: " << iter->second.count << ", lost_count: " << iter->second.count_lost
               << ", rollback_flag: " << iter->second.b_rollback_flag << "." << endl;
        }
        /// tick to string
        ss << "SZE_tick order count: " << m_tick_pkg_info[i].order_count << endl;
        ss << "SZE_tick exe count: " << m_tick_pkg_info[i].exe_count << endl;
        ss << "SZE_tick"
           << "\t[" << m_tick_pkg_info[i].multicast_ip << "]"
           << "[" << m_tick_pkg_info[i].multicast_port << "]\t"
           << " count: " << m_tick_pkg_info[i].count << ", lost_count: " << m_tick_pkg_info[i].count_lost
           << ", rollback_flag: " << m_tick_pkg_info[i].b_rollback_flag << "." << endl;

        ss << "SZE_bond_tick order count: " << m_bond_tick_pkg_info[i].order_count << endl;
        ss << "SZE_bond_tick exe count: " << m_bond_tick_pkg_info[i].exe_count << endl;
        ss << "SZE_bond_tick"
           << "\t[" << m_bond_tick_pkg_info[i].multicast_ip << "]"
           << "[" << m_bond_tick_pkg_info[i].multicast_port << "]\t"
           << " count: " << m_bond_tick_pkg_info[i].count << ", lost_count: " << m_bond_tick_pkg_info[i].count_lost
           << ", rollback_flag: " << m_bond_tick_pkg_info[i].b_rollback_flag << "." << endl;

        if (!flag || i == 0)
        {
            printf("%s\n", ss.str().c_str());
        }
    }

    fflush(stdout);
}

void sze_report::close()
{
    if (m_p_quote == NULL)
    {
        return;
    }

    m_p_quote->stop_sze();
    m_p_quote->close_sze();
#ifdef WINDOWS
    func_destroy_efh_sze_lev2_api func_destroy =
        (func_destroy_efh_sze_lev2_api)GetProcAddress(m_h_core, DESTROY_EFH_SZE_LEV2_API_FUNCTION);
#else
    func_destroy_efh_sze_lev2_api func_destroy =
        (func_destroy_efh_sze_lev2_api)dlsym(m_h_core, DESTROY_EFH_SZE_LEV2_API_FUNCTION);
#endif
    if (func_destroy == NULL)
    {
        return;
    }
    if (m_b_report_quit)
    {
        report_efh_sze_lev2_snap();
        report_efh_sze_lev2_order();
        report_efh_sze_lev2_exe();
        report_efh_sze_lev2_after_close();
        report_efh_sze_lev2_idx();
        report_efh_sze_lev2_tree();
        report_efh_sze_lev2_ibr_tree();
        report_efh_sze_lev2_turnover();
        report_efh_sze_lev2_bond_snap();
        report_efh_sze_lev2_bond_order();
        report_efh_sze_lev2_bond_exe();
    }
    if (m_fp_lev2)
    {
        fclose(m_fp_lev2);
        m_fp_lev2 = NULL;
    }
    if (m_fp_idx)
    {
        fclose(m_fp_idx);
        m_fp_idx = NULL;
    }
    if (m_fp_ord)
    {
        fclose(m_fp_ord);
        m_fp_ord = NULL;
    }
    if (m_fp_exe)
    {
        fclose(m_fp_exe);
        m_fp_exe = NULL;
    }
    if (m_fp_close_px)
    {
        fclose(m_fp_close_px);
        m_fp_close_px = NULL;
    }
    if (m_fp_tree)
    {
        fclose(m_fp_tree);
        m_fp_tree = NULL;
    }
    if (m_fp_ibr_tree)
    {
        fclose(m_fp_ibr_tree);
        m_fp_ibr_tree = NULL;
    }
    if (m_fp_turnover)
    {
        fclose(m_fp_turnover);
        m_fp_turnover = NULL;
    }
    if (m_fp_bond_snap)
    {
        fclose(m_fp_bond_snap);
        m_fp_bond_snap = NULL;
    }
    if (m_fp_bond_ord)
    {
        fclose(m_fp_bond_ord);
        m_fp_bond_ord = NULL;
    }
    if (m_fp_bond_exe)
    {
        fclose(m_fp_bond_exe);
        m_fp_bond_exe = NULL;
    }
    if (m_fp_tick)
    {
        fclose(m_fp_tick);
        m_fp_tick = NULL;
    }
    if (m_fp_bond_tick)
    {
        fclose(m_fp_bond_tick);
        m_fp_bond_tick = NULL;
    }
    if (m_snap)
    {
        delete[] m_snap;
        m_snap = NULL;
    }
    if (m_idx)
    {
        delete[] m_idx;
        m_idx = NULL;
    }
    if (m_order)
    {
        delete[] m_order;
        m_order = NULL;
    }
    if (m_exe)
    {
        delete[] m_exe;
        m_exe = NULL;
    }
    if (m_after_close)
    {
        delete[] m_after_close;
        m_after_close = NULL;
    }
    if (m_tree)
    {
        delete[] m_tree;
        m_tree = NULL;
    }
    if (m_ibr_tree)
    {
        delete[] m_ibr_tree;
        m_ibr_tree = NULL;
    }
    if (m_turnover)
    {
        delete[] m_turnover;
        m_turnover = NULL;
    }
    if (m_bond_snap)
    {
        delete[] m_bond_snap;
        m_bond_snap = NULL;
    }
    if (m_bond_order)
    {
        delete[] m_bond_order;
        m_bond_order = NULL;
    }
    if (m_bond_exe)
    {
        delete[] m_bond_exe;
        m_bond_exe = NULL;
    }

    func_destroy(m_p_quote);
#ifdef WINDOWS
    FreeLibrary(m_h_core);
#else
    dlclose(m_h_core);
#endif
}

void sze_report::check_info_count(int id, int type, int64_t seq)
{
    if (id > 4)
    {
        return;
    }
    auto iter = m_map_pkg_info[id].find(type);
    if (iter == m_map_pkg_info[id].end())
    {
        return;
    }

    if (iter->second.last_seq >= 0)
    {
        auto num = seq - iter->second.last_seq - 1;
        if (num < 0)
        {
            iter->second.b_rollback_flag = true;
            printf("%s out of seq, last seq[%ld], cur seq[%ld].\n", get_type_name(type).c_str(), iter->second.last_seq, seq);
        }
        else if (num > 0)
        {
            iter->second.count_lost += num;
            lost_pkg_log(type, iter->second.last_seq, seq);
        }
    }

    iter->second.last_seq = seq;
    iter->second.count++;
}

void sze_report::check_tick_info_count(int type, int64_t seq, pkg_tick_info &info)
{
    if (info.last_seq >= 0)
    {
        auto num = seq - info.last_seq - 1;
        if (num < 0)
        {
            info.b_rollback_flag = true;
            printf("%s out of seq, last seq[%ld], cur seq[%ld].\n", get_type_name(type).c_str(), info.last_seq, seq);
        }
        else if (num > 0)
        {
            info.count_lost += num;
            lost_pkg_log(type, info.last_seq, seq);
        }
    }

    info.last_seq = seq;
    info.count++;
    switch (type)
    {
    case SSE_LEV2_ORDER_MSG_TYPE:
    case SZE_LEV2_ORDER_MSG_TYPE:
    case SZE_LEV2_BOND_ORDER_MSG_TYPE:
        info.order_count++;
        break;
    case SSE_LEV2_EXE_MSG_TYPE:
    case SZE_LEV2_EXE_MSG_TYPE:
    case SZE_LEV2_BOND_EXE_MSG_TYPE:
        info.exe_count++;
        break;
    default:
        break;
    }
}

void sze_report::on_report_efh_sze_lev2_after_close(session_identity id, sze_hpf_after_close *p_close_px) {}

void sze_report::on_report_efh_sze_lev2_snap(session_identity id, sze_hpf_lev2 *p_snap)
{
    check_info_count(id.id, SZE_LEV2_SNAP_MSG_TYPE, p_snap->m_head.m_sequence);
    // auto_change_log_info(id.id, get_key(*p_snap));
    if (!m_b_report_quit)
    {
        if (p_snap == nullptr || p_snap->m_head.m_symbol == nullptr)
        {
            // 处理错误，可能是日志记录或直接返回
            return;
        }
        std::string code(reinterpret_cast<char*>(p_snap->m_head.m_symbol), 9);

        // 行情筛选 是否存在今日卖单中
        if (std::find(mapKeys.begin(), mapKeys.end(), code.substr(0, 6)) == mapKeys.end())
        {
            return;
        }
        // 原本数据
        ut_code_Info orig_code_info;
        if (code_info_vec_ptr.find(code) == code_info_vec_ptr.end())
        { 
            orig_code_info = code_info_vec_ptr[code];
        }

        orig_code_info.time = std::to_string(static_cast<int>(std::floor(p_snap->m_head.m_quote_update_time)));
        orig_code_info.real_price = p_snap->m_last_price / 1000.0;
        orig_code_info.yes_close = p_snap->m_pre_close_price / 1000.0;
        orig_code_info.open_price = p_snap->m_open_price / 1000.0;
        orig_code_info.bid_one_price =p_snap-> m_bid_unit[0].m_price / 1000.0;
        orig_code_info.bid_one = p_snap-> m_bid_unit[0].m_quantity / 1000.0;
        orig_code_info.bid_two = p_snap-> m_bid_unit[1].m_quantity / 1000.0;
        code_info_vec_ptr[code] = orig_code_info;
    }
}

void sze_report::on_report_efh_sze_lev2_tick(session_identity id, int msg_type, sze_hpf_order *p_order, sze_hpf_exe *p_exe) {}

void sze_report::on_report_efh_sze_lev2_idx(session_identity id, sze_hpf_idx *p_idx) {}

void sze_report::on_report_efh_sze_lev2_tree(session_identity id, sze_hpf_tree *p_tree) {}

void sze_report::on_report_efh_sze_lev2_ibr_tree(session_identity id, sze_hpf_ibr_tree *p_ibr_tree) {}

void sze_report::on_report_efh_sze_lev2_turnover(session_identity id, sze_hpf_turnover *p_turnover) {}

void sze_report::on_report_efh_sze_lev2_bond_snap(session_identity id, sze_hpf_bond_snap *p_snap) {}

void sze_report::on_report_efh_sze_lev2_bond_tick(
    session_identity id,
    int msg_type,
    sze_hpf_bond_order *p_order,
    sze_hpf_bond_exe *p_exe) {}

void sze_report::efh_sze_lev2_debug(const char *msg, int len)
{
    printf("[DEBUG] %s\n", msg);
}

void sze_report::efh_sze_lev2_error(const char *msg, int len)
{
    printf("[ERROR] %s\n", msg);
}

void sze_report::efh_sze_lev2_info(const char *msg, int len)
{
    printf("[INFO] %s\n", msg);
}

string sze_report::format_str(const char *pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    char buffer[40960];
    vsnprintf(buffer, 40960, pFormat, args);
    va_end(args);
    return string(buffer);
}

void sze_report::report_efh_sze_lev2_order() {}

void sze_report::report_efh_sze_lev2_exe() {}

void sze_report::report_efh_sze_lev2_after_close() {}

void sze_report::report_efh_sze_lev2_snap()
{
    if (m_fp_lev2 == NULL)
    {
        time_t now = time(NULL);
        tm *ltm = localtime(&now);

        char str_full_name[1024];
        memset(str_full_name, 0, sizeof(str_full_name));
        sprintf(str_full_name, "%04d%02d%02d_sze_snap.csv", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday);

        m_fp_lev2 = fopen(str_full_name, "wt+");
        if (m_fp_lev2 == NULL)
        {
            return;
        }
    }

    if (m_ll_snap_count > 0)
    {
        for (int i = 0; i < QT_SZE_QUOTE_COUNT; i++)
        {
            if (m_snap[i].m_local_time == 0)
            {
                return;
            }

            char ch_buffer[1024];
            memset(ch_buffer, 0, sizeof(ch_buffer));
            sprintf(
                ch_buffer,
                "%s, %lld, %llu, %u, %llu, %llu, %u, %lld, %u, %lld, %u, %lld, %u, %lld,%u, %lld, %u, %lld,\n",
                m_snap[i].m_symbol,
                m_snap[i].m_quote_update_time,
                m_snap[i].m_local_time,
                m_snap[i].m_last_price,
                m_snap[i].m_total_quantity,
                m_snap[i].m_total_value,
                m_snap[i].m_bid_lev_1.m_price,
                m_snap[i].m_bid_lev_1.m_quantity,
                m_snap[i].m_ask_lev_1.m_price,
                m_snap[i].m_ask_lev_1.m_quantity,
                m_snap[i].m_bid_lev_10.m_price,
                m_snap[i].m_bid_lev_10.m_quantity,
                m_snap[i].m_ask_lev_10.m_price,
                m_snap[i].m_ask_lev_10.m_quantity,
                m_snap[i].m_bid_lev_5.m_price,
                m_snap[i].m_bid_lev_5.m_quantity,
                m_snap[i].m_ask_lev_5.m_price,
                m_snap[i].m_ask_lev_5.m_quantity);

            int ret = fwrite(ch_buffer, 1, strlen(ch_buffer), m_fp_lev2);
            fflush(m_fp_lev2);
            if (ret <= 0)
            {
                printf("write sze_snap.csv error!\t msg : %s\n", ch_buffer);
            }
        }
    }
}

void sze_report::report_efh_sze_lev2_idx() {}

void sze_report::report_efh_sze_lev2_tree() {}

void sze_report::report_efh_sze_lev2_ibr_tree() {}

void sze_report::report_efh_sze_lev2_turnover() {}

void sze_report::report_efh_sze_lev2_bond_order() {}

void sze_report::report_efh_sze_lev2_bond_exe() {}

void sze_report::report_efh_sze_lev2_bond_snap() {}

string sze_report::get_sze_src_trading_phase_code(char ch_sl_trading_phase_code)
{
    char ch_buf[8];
    memset(ch_buf, 0, sizeof(ch_buf));
    char ch_first = ' ';
    switch (ch_sl_trading_phase_code & 0xF0)
    {
    case 0x00:
        ch_first = 'S';
        break;
    case 0x10:
        ch_first = 'O';
        break;
    case 0x20:
        ch_first = 'T';
        break;
    case 0x30:
        ch_first = 'B';
        break;
    case 0x40:
        ch_first = 'C';
        break;
    case 0x50:
        ch_first = 'E';
        break;
    case 0x60:
        ch_first = 'H';
        break;
    case 0x70:
        ch_first = 'A';
        break;
    case 0x80:
        ch_first = 'V';
        break;
    default:
        break;
    }

    ch_buf[0] = ch_first;

    if ((ch_sl_trading_phase_code & 0x08) == 0x08)
    {
        ch_buf[1] = '1';
    }
    else if ((ch_sl_trading_phase_code & 0x08) == 0x00)
    {
        ch_buf[1] = '0';
    }

    string str_buf = ch_buf;
    return str_buf;
}

void sze_report::odp_query(char *ip, int port, int channel_num, long long begin_seq, long long end_seq)
{
    if (m_p_quote)
    {
        odp_event.count = 0;
        bool ret = m_p_quote->odp_rebuild_quote(ip, port, &odp_event, channel_num, begin_seq, end_seq);
        auto count = odp_event.count;
        printf("odp query finish ,result is [%s] count is [%d]\n", ret ? "true" : "false", count);
    }
}

bool sze_report::get_symbol_status(vector<symbol_item> &list)
{
    if (!m_p_quote)
    {
        return false;
    }

    int num = 1024 * 1024;
    symbol_item *buf = new symbol_item[num];

    bool enable;
    int symbol_num = m_p_quote->get_symbol_filter_list_and_enable_switch(buf, &num, &enable);
    if (symbol_num < num)
    {
        delete[] buf;
        buf = new symbol_item[symbol_num];
        num = symbol_num;
        m_p_quote->get_symbol_filter_list_and_enable_switch(buf, &num, &enable);
    }

    for (int i = 0; i < num; i++)
    {
        list.emplace_back(buf[i]);
    }
    delete[] buf;
    return enable;
}

void sze_report::sub_symbol(vector<symbol_item> &list)
{
    if (m_p_quote)
        m_p_quote->subscribe_symbol_filter_items(list.data(), list.size());
}
void sze_report::unsub_symbol(vector<symbol_item> &list)
{
    if (m_p_quote)
        m_p_quote->unsubscribe_symbol_filter_items(list.data(), list.size());
}
void sze_report::set_symbol_filter_enable_switch(bool flag)
{
    if (m_p_quote)
        m_p_quote->set_symbol_filter_enable_switch(flag);
}

void sze_report::auto_change_log_info(int id, const string &msg)
{
    if (m_i_auto_change_log_num == 0)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_log_mtx);
    if (m_b_first)
    {
        m_b_first = false;
        m_last_id = id;
        m_b_flag = false;
    }

    if (id == m_last_id && !m_b_flag)
    {
        m_auto_change_log_info.push_back(msg);
        while (m_auto_change_log_info.size() > m_i_auto_change_log_num)
        {
            m_auto_change_log_info.pop_front();
        }
        return;
    }
    /// 切换中,记录切换后前N条
    if (id == m_last_id && m_b_flag)
    {
        if (m_i_after_write_count >= m_i_auto_change_log_num)
        {
            m_b_flag = false;
            return;
        }
        if (m_auto_change_fp)
        {
            fprintf(m_auto_change_fp, "%s\n", msg.c_str());
            fflush(m_auto_change_fp);
        }
        m_i_after_write_count++;

        return;
    }

    /// 发生切换了
    m_i_after_write_count = 0;
    m_b_flag = true;
    m_last_id = id;
    if (m_auto_change_fp)
    {
        fprintf(m_auto_change_fp, "before switch source\n");
    }
    for (auto iter = m_auto_change_log_info.begin(); iter != m_auto_change_log_info.end(); iter++)
    {
        fprintf(m_auto_change_fp, "%s\n", iter->c_str());
    }
    m_auto_change_log_info.clear();
    if (m_auto_change_fp)
    {
        fprintf(m_auto_change_fp, "after switch source\n");
    }

    if (m_i_after_write_count >= m_i_auto_change_log_num)
    {
        m_b_flag = false;
        return;
    }
    if (m_auto_change_fp)
    {
        fprintf(m_auto_change_fp, "%s\n", msg.c_str());
        fflush(m_auto_change_fp);
    }
    m_i_after_write_count++;
}

void sze_report::update_show_udp_session(const efh_channel_config &cfg)
{
    auto key = get_type_from_channel(cfg.m_efh_type);
    for (int i = 0; i < cfg.m_i_channel_num; i++)
    {
        string mip = cfg.m_channel_info[i].m_ch_src_ip;
        int mport = cfg.m_channel_info[i].m_i_src_port;

        if (cfg.m_efh_type == enum_efh_sze_lev2_bond_tick)
        {
            m_bond_tick_pkg_info[i].multicast_ip = mip;
            m_bond_tick_pkg_info[i].multicast_port = mport;
        }
        else if (cfg.m_efh_type == enum_efh_sze_lev2_tick)
        {
            m_tick_pkg_info[i].multicast_ip = mip;
            m_tick_pkg_info[i].multicast_port = mport;
        }
        else
        {
            auto iter = m_map_pkg_info[i].find(key);
            if (iter == m_map_pkg_info[i].end())
            {
                continue;
            }
            iter->second.multicast_ip = mip;
            iter->second.multicast_port = mport;
        }
    }
}

bool sze_report::set_auto_change_source_config(bool b_flag, int64_t ll_time)
{
    return m_p_quote->set_auto_change_source_config(b_flag, ll_time);
}
