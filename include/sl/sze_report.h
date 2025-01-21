/*!*************************************************************************
@node		Copyright (coffee), 2005-2020, Shengli Tech. Co., Ltd.
@file   	sze_report.h
@date		2020/12/14 08:30
@author		shengli

@brief		接收、上报深交所行情
******************************************************************************/
#pragma once
#include <stdio.h>

#include <deque>
#include <iostream>
#include <map>
#include <mutex>

#include "i_efh_sze_lev2_api.h"
#include "tool.h"

using namespace std;

#ifdef WINDOWS
#define QT_SZE_QUOTE_COUNT (100) /// 根据实际情况进行调整
#else
#define QT_SZE_QUOTE_COUNT (200) /// 根据实际情况进行调整
#endif

#ifdef WINDOWS
#include <Windows.h>
typedef HMODULE dll_handle_sl;
#else
typedef void *dll_handle_sl;
#endif

#pragma pack(push, 1)
/// 记录需求字段
struct qt_price_quantity_unit
{
    unsigned int m_price;
    long long m_quantity;
};

struct sze_qt_node_after_close
{
    char m_symbol[9];                    /// 合约
    short m_channel_num;                 /// 频道号
    long long m_sequence_num;            /// 消息记录号
    long long m_quote_update_time;       /// 行情时间
    unsigned long long m_local_time;     /// 本地接收时间
    unsigned char m_trading_status;      /// 交易阶段
    long long m_total_trade_num;         /// 成交笔数
    unsigned long long m_total_quantity; /// 成交总量
    unsigned long long m_total_value;    /// 成交总金额
    unsigned int m_pre_close_price;      /// 昨收价
    unsigned int
        m_exe_price;                   /// 执行价格，如果NoMDEntries=1 ，取MDEntryPx ；如果NoMDEntries>1，取所有 透传 MDEntryPx 的最大值
    unsigned long long m_bid_quantity; /// 买方数量
    unsigned long long m_ask_quantity; /// 卖方数量
};

struct sze_qt_node_snap
{
    char m_symbol[9];                /// 合约
    long long m_quote_update_time;   /// 行情更新时间
    unsigned long long m_local_time; /// 本地接收时间

    unsigned int m_last_price;           ///	最新价
    unsigned long long m_total_quantity; ///	总成交量
    unsigned long long m_total_value;    /// 总成交额

    // snp
    qt_price_quantity_unit m_bid_lev_1;  /// 买向第一档
    qt_price_quantity_unit m_bid_lev_10; /// 买向第十档
    qt_price_quantity_unit m_ask_lev_1;  /// 卖向第一档
    qt_price_quantity_unit m_ask_lev_10; /// 卖向第十档
    /// opt
    qt_price_quantity_unit m_bid_lev_5; /// 买向第五档
    qt_price_quantity_unit m_ask_lev_5; /// 买向第五档
};

struct sze_qt_node_order
{
    char m_symbol[9];                /// 合约
    short m_channel_num;             /// 频道号
    long long m_sequence_num;        /// 消息记录号
    long long m_quote_update_time;   /// 行情更新时间
    unsigned long long m_local_time; ///	本地接收时间
    int m_order_price;               ///	价格
    long long m_order_quantity;      /// 数量
    char m_order_type;               /// 订单类型
};

struct sze_qt_node_exe
{
    char m_symbol[9];                    /// 合约
    short m_channel_num;                 /// 频道号
    long long m_sequence_num;            /// 消息记录号
    long long m_quote_update_time;       /// 行情时间
    unsigned long long m_local_time;     /// 本地接收时间
    unsigned int m_trade_price;          /// 成交价格
    unsigned long long m_trade_quantity; /// 成交量
    char m_trade_type;                   /// 成交类型
};

struct sze_qt_node_index
{
    char m_symbol[9];                    /// 合约
    long long m_quote_update_time;       /// 原始行情时间
    unsigned long long m_local_time;     /// 本地接收时间
    unsigned long long m_total_quantity; /// 总成交量
    unsigned int m_last_price;           /// 最新价
    unsigned int m_pre_close_price;      /// 开盘价
    unsigned int m_open_price;           /// 成交值
    unsigned int m_day_high_price;       /// 最高价
    unsigned int m_day_low_price;        /// 最低价
};

struct sze_qt_node_tree
{
    unsigned int m_sequence;                     /// 盛立行情序号
    char m_symbol[9];                            /// 合约
    long long m_quote_update_time;               /// 行情更新时间
    unsigned long long m_local_time;             /// 本地接收时间
    qt_price_quantity_unit m_bid_lev_1;          /// 买向第一档
    qt_price_quantity_unit m_bid_lev_10;         /// 买向第十档
    qt_price_quantity_unit m_ask_lev_1;          /// 卖向第一档
    qt_price_quantity_unit m_ask_lev_10;         /// 卖向第十档
    unsigned int m_pre_close_price;              /// 昨收价
    unsigned int m_open_price;                   ///	开盘价
    unsigned int m_day_high_price;               /// 最高价
    unsigned int m_day_low_price;                ///	最低价
    unsigned int m_last_price;                   ///	最新价
    unsigned long long m_total_quantity;         ///	总成交量
    unsigned long long m_total_value;            /// 总成交额
    unsigned long long m_total_bid_quantity;     /// 买总量
    unsigned long long m_total_ask_quantity;     /// 卖总量
    unsigned int m_total_bid_weighted_avg_price; /// 买边挂单均价
    unsigned int m_total_ask_weighted_avg_price; /// 卖边挂单均价
    unsigned int m_bid_depth;                    /// 买边总档位数
    unsigned int m_ask_depth;                    /// 卖边总档位数
    unsigned long long m_market_open_total_bid;  /// 买边市价单挂单总量
    unsigned long long m_market_open_total_ask;  /// 卖边市价单挂单总量
};

struct sze_qt_node_ibr_tree
{
    unsigned int m_sequence;            /// 盛立行情序号
    char m_symbol[9];                   /// 合约
    long long m_quote_update_time;      /// 行情更新时间
    unsigned long long m_local_time;    /// 本地接收时间
    qt_price_quantity_unit m_bid_lev_1; /// 买向第一档
    qt_price_quantity_unit m_bid_lev_5; /// 买向第五档
    qt_price_quantity_unit m_ask_lev_1; /// 卖向第一档
    qt_price_quantity_unit m_ask_lev_5; /// 卖向第五档
    unsigned long long m_bid_depth;     /// 委托买入总量，放大100倍
    unsigned long long m_ask_depth;     /// 委托卖总量，放大100倍
};

struct sze_qt_node_turnover
{
    unsigned int m_sequence;             /// 盛立行情序号
    char m_symbol[9];                    /// 合约
    long long m_quote_update_time;       /// 行情更新时间
    unsigned long long m_local_time;     /// 本地接收时间
    long long m_total_trade_num;         /// 成交笔数
    unsigned long long m_total_quantity; /// 成交量，放大100倍
    unsigned long long m_total_value;    /// 成交金额，放大1000000倍
};

#pragma pack(pop)
struct sze_odp_event : public efh_sze_lev2_api_event
{
    FILE *m_fp = NULL;
    sze_odp_event()
    {
        std::string str_full_dir = get_project_path() +"/log/sze_odp_quote.log";
        m_fp = fopen(str_full_dir.c_str(), "wb");
    }
    ~sze_odp_event()
    {
        fclose(m_fp);
    }
    void on_report_efh_sze_lev2_after_close(session_identity id, sze_hpf_after_close *p_close_px) override
    {
        count++;
        if (m_fp)
        {
            fwrite(p_close_px, 1, sizeof(*p_close_px), m_fp);
            fflush(m_fp);
        }
    }
    void on_report_efh_sze_lev2_snap(session_identity id, sze_hpf_lev2 *p_snap) override
    {
        count++;
        if (m_fp)
        {
            fwrite(p_snap, 1, sizeof(*p_snap), m_fp);
            fflush(m_fp);
        }
    }
    void on_report_efh_sze_lev2_tick(session_identity id, int msg_type, sze_hpf_order *p_order, sze_hpf_exe *p_exe) override
    {
        count++;
        switch (msg_type)
        {
        case SZE_LEV2_ORDER_MSG_TYPE:
            if (m_fp)
            {
                fwrite(p_order, 1, sizeof(*p_order), m_fp);
                fflush(m_fp);
            }

            break;
        case SZE_LEV2_EXE_MSG_TYPE:
            if (m_fp)
            {
                fwrite(p_exe, 1, sizeof(*p_exe), m_fp);
                fflush(m_fp);
            }

            break;

        default:
            break;
        }
    }
    void on_report_efh_sze_lev2_idx(session_identity id, sze_hpf_idx *p_idx) override
    {
        count++;
        if (m_fp)
        {
            fwrite(p_idx, 1, sizeof(*p_idx), m_fp);
            fflush(m_fp);
        }
    }
    void on_report_efh_sze_lev2_tree(session_identity id, sze_hpf_tree *p_tree) override
    {
        count++;
        if (m_fp)
        {
            fwrite(p_tree, 1, sizeof(*p_tree), m_fp);
            fflush(m_fp);
        }
    }
    void on_report_efh_sze_lev2_ibr_tree(session_identity id, sze_hpf_ibr_tree *p_ibr_tree) override
    {
        count++;
        if (m_fp)
        {
            fwrite(p_ibr_tree, 1, sizeof(*p_ibr_tree), m_fp);
            fflush(m_fp);
        }
    }
    void on_report_efh_sze_lev2_turnover(session_identity id, sze_hpf_turnover *p_turnover) override
    {
        count++;
        if (m_fp)
        {
            fwrite(p_turnover, 1, sizeof(*p_turnover), m_fp);
            fflush(m_fp);
        }
    }
    void on_report_efh_sze_lev2_bond_snap(session_identity id, sze_hpf_bond_snap *p_bond) override
    {
        count++;
        if (m_fp)
        {
            fwrite(p_bond, 1, sizeof(*p_bond), m_fp);
            fflush(m_fp);
        }
    }
    void on_report_efh_sze_lev2_bond_tick(
        session_identity id,
        int msg_type,
        sze_hpf_bond_order *p_order,
        sze_hpf_bond_exe *p_exe) override
    {
        count++;
        switch (msg_type)
        {
        case SZE_LEV2_BOND_ORDER_MSG_TYPE:
            if (m_fp)
            {
                fwrite(p_order, 1, sizeof(*p_order), m_fp);
                fflush(m_fp);
            }

            break;
        case SZE_LEV2_BOND_EXE_MSG_TYPE:
            if (m_fp)
            {
                fwrite(p_exe, 1, sizeof(*p_exe), m_fp);
                fflush(m_fp);
            }

            break;

        default:
            break;
        }
    }

    int count = 0;
};

class sze_report : public efh_sze_lev2_api_event, public efh_sze_lev2_api_depend
{
public:
    sze_report();
    ~sze_report();
    bool init(efh_channel_config *param, int num, int auto_change_log_num);
    bool init_with_ats(
        exchange_authorize_config &ats_config,
        efh_channel_config *param,
        int num,
        int auto_change_log_num,
        bool is_keep_connection);
    void set_tick_detach(bool enable);
    void run(bool b_report_quit = false, bool symbol_filter = false, const char *symbol = "");
    void show();
    void close();
    void odp_query(char *ip, int port, int channel_num, long long begin_seq, long long end_seq);
    bool get_symbol_status(vector<symbol_item> &list);
    void sub_symbol(vector<symbol_item> &list);
    void unsub_symbol(vector<symbol_item> &list);
    void set_symbol_filter_enable_switch(bool flag);
    bool set_auto_change_source_config(bool b_flag, int64_t ll_time);

protected:
    void on_report_efh_sze_lev2_after_close(session_identity id, sze_hpf_after_close *p_close_px) override;
    void on_report_efh_sze_lev2_snap(session_identity id, sze_hpf_lev2 *p_snap) override;
    void on_report_efh_sze_lev2_tick(session_identity id, int msg_type, sze_hpf_order *p_order, sze_hpf_exe *p_exe) override;
    void on_report_efh_sze_lev2_idx(session_identity id, sze_hpf_idx *p_idx) override;
    void on_report_efh_sze_lev2_tree(session_identity id, sze_hpf_tree *p_tree) override;
    void on_report_efh_sze_lev2_ibr_tree(session_identity id, sze_hpf_ibr_tree *p_ibr_tree) override;
    void on_report_efh_sze_lev2_turnover(session_identity id, sze_hpf_turnover *p_turnover) override;
    void on_report_efh_sze_lev2_bond_snap(session_identity id, sze_hpf_bond_snap *p_bond) override;
    void on_report_efh_sze_lev2_bond_tick(
        session_identity id,
        int msg_type,
        sze_hpf_bond_order *p_order,
        sze_hpf_bond_exe *p_exe) override;

    virtual void efh_sze_lev2_debug(const char *msg, int len);
    virtual void efh_sze_lev2_error(const char *msg, int len);
    virtual void efh_sze_lev2_info(const char *msg, int len);

    string format_str(const char *pFormat, ...);
    string get_sze_src_trading_phase_code(char ch_sl_trading_phase_code);

private:
    void report_efh_sze_lev2_order();
    void report_efh_sze_lev2_exe();
    void report_efh_sze_lev2_after_close();
    void report_efh_sze_lev2_snap();
    void report_efh_sze_lev2_idx();
    void report_efh_sze_lev2_tree();
    void report_efh_sze_lev2_ibr_tree();
    void report_efh_sze_lev2_turnover();
    void report_efh_sze_lev2_bond_snap();
    void report_efh_sze_lev2_bond_order();
    void report_efh_sze_lev2_bond_exe();

    void check_info_count(int id, int type, int64_t seq);

    void check_tick_info_count(int type, int64_t seq, pkg_tick_info &info);
    void auto_change_log_info(int id, const string &msg);

    void update_show_udp_session(const efh_channel_config &cfg);

private:
    bool m_b_report_quit;
    bool m_b_tick_detach;
    dll_handle_sl m_h_core;
    i_efh_sze_lev2_api *m_p_quote;
    FILE *m_fp_lev2;
    FILE *m_fp_idx;
    FILE *m_fp_ord;
    FILE *m_fp_exe;
    FILE *m_fp_tick;
    FILE *m_fp_close_px;
    FILE *m_fp_tree;
    FILE *m_fp_ibr_tree;
    FILE *m_fp_turnover;
    FILE *m_fp_bond_snap;
    FILE *m_fp_bond_ord;
    FILE *m_fp_bond_exe;
    FILE *m_fp_bond_tick;
    vector<string> m_vec_symbol;

    long long m_ll_snap_count;
    long long m_ll_idx_count;
    long long m_ll_order_count;
    long long m_ll_exe_count;
    long long m_ll_after_count;
    long long m_ll_tree_count;
    long long m_ll_ibr_tree_count;
    long long m_ll_turnover_count;
    long long m_ll_bond_snap_count;
    long long m_ll_bond_order_count;
    long long m_ll_bond_exe_count;

    sze_qt_node_snap *m_snap;
    sze_qt_node_index *m_idx;
    sze_qt_node_order *m_order;
    sze_qt_node_exe *m_exe;
    sze_qt_node_after_close *m_after_close;
    sze_qt_node_tree *m_tree;
    sze_qt_node_ibr_tree *m_ibr_tree;
    sze_qt_node_turnover *m_turnover;
    sze_qt_node_snap *m_bond_snap;
    sze_qt_node_order *m_bond_order;
    sze_qt_node_exe *m_bond_exe;
    sze_odp_event odp_event;
    map<int, pkg_info> m_map_pkg_info[4];
    pkg_tick_info m_tick_pkg_info[4];
    pkg_tick_info m_bond_tick_pkg_info[4];

    size_t m_i_auto_change_log_num;
    deque<string> m_auto_change_log_info;
    mutex m_log_mtx;
    int m_last_id;
    bool m_b_first;
    bool m_b_flag;
    size_t m_i_after_write_count;
    FILE *m_auto_change_fp;
};
