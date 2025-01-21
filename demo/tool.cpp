#include "tool.h"

#include <iostream>
#include <memory.h>
#include <string.h>
#include <iomanip>
#include <set>
#include <sstream>
#include <chrono>
#include <fstream>
#include <sys/stat.h>
void lost_pkg_log(int msg_type, int64_t last_sequence, int64_t sequence)
{
    // 打开文件，使用绝对路径或确保路径是可写的
    time_t now = time(NULL);
    tm *ltm = localtime(&now);
    std::string str_full_dir = get_project_path() + "/log/" + format_date_yyyymmdd(ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday) + 
                               "_lost_pkt_info.log";
    FILE *fp = fopen(str_full_dir.c_str(), "a");

    // 检查文件是否成功打开
    if (fp == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        return;
    }

    // 记录丢失的包信息
    for (int64_t i = last_sequence + 1; i < sequence; i++)
    {
        if (fprintf(fp, "%d,%ld\n", msg_type, i) < 0)
        {
            fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
            break;
        }
    }

    // 关闭文件
    if (fclose(fp) != 0)
    {
        fprintf(stderr, "Error closing file: %s\n", strerror(errno));
    }
}

bool isBefore(int hour, int minute, int second)
{
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);

    auto now_seconds = now_tm.tm_hour * 3600 + now_tm.tm_min * 60 + now_tm.tm_sec;
    int target_seconds = hour * 3600 + minute * 60 + second;

    // 判断当前时间是否早于目标时间
    // std::cout << "nowtime is " << now_tm.tm_hour << ":" << now_tm.tm_min << ":" << now_tm.tm_sec << std::endl;
    return now_seconds < target_seconds;
}

bool string_split(const char *str_src, vector<string> &str_dst, const string &str_separator)
{
    if (NULL == str_src)
    {
        return false;
    }

    // 注意，这里选择了清空输入的字符数组
    str_dst.clear();

    size_t sep_len = str_separator.length();
    if (0 == sep_len)
        return false;

    size_t src_len = strlen(str_src);
    if (0 == src_len)
        return true;

    if (src_len < sep_len) // 这种情况，认为只有一个元素而返回
    {
        str_dst.push_back(str_src);
        return true;
    }

    // 2010-08-05 Find Problem ls
    // 测试发现分割字符串，没有考虑最后一块，修改部分如下：
    //-----------------------------------------------------------
    /// add by zhou.you  review 2012/7/30
    /// 原修改不合理，应该先判断是否以分隔符结尾的。
    string srcstring = str_src;
    if (srcstring.substr(src_len - sep_len, sep_len) != str_separator)
        srcstring += str_separator;

    size_t lastIdx = 0;
    size_t idx = srcstring.find_first_of(str_separator);
    if (idx == string::npos) // 这种情况，认为只有一个元素而返回
    {
        str_dst.push_back(str_src);
        return true;
    }

    while (idx != string::npos)
    {
        string strTemp = srcstring.substr(lastIdx, idx - lastIdx); // 有可能空字符串，也作为结果加入。
        str_dst.push_back(strTemp);
        lastIdx = idx + sep_len;
        idx = srcstring.find_first_of(str_separator, lastIdx);
    }
    return true;
}

#ifdef WINDOWS
#include <Windows.h>
#include <time.h>
int clock_gettime(int, struct timespec *spec)
{
    __int64 wintime;
    GetSystemTimeAsFileTime((FILETIME *)&wintime);
    wintime -= 116444736000000000i64;
    spec->tv_sec = wintime / 10000000i64;
    spec->tv_nsec = wintime % 10000000i64 * 100;
    return 0;
}
#endif

int get_type_from_channel(enum_efh_lev2_type n)
{
    int ret = 0;

    switch (n)
    {
    case enum_efh_sze_lev2_snap:
        ret = SZE_LEV2_SNAP_MSG_TYPE;
        break;
    case enum_efh_sze_lev2_tick:
        ret = SZE_LEV2_ORDER_MSG_TYPE * 100 + SZE_LEV2_EXE_MSG_TYPE;
        break;
    case enum_efh_sze_lev2_idx:
        ret = SZE_LEV2_IDX_MSG_TYPE;
        break;
    case enum_efh_sze_lev2_tree:
        ret = SZE_LEV2_TREE_MSG_TYPE;
        break;
    case enum_efh_sze_lev2_after_close:
        ret = SZE_LEV2_AF_CLOSE_MSG_TYPE;
        break;
    case enum_efh_sze_lev2_ibr_tree:
        ret = SZE_LEV2_IBR_TREE_MSG_TYPE;
        break;
    case enum_efh_sze_lev2_turnover:
        ret = SZE_LEV2_TURNOVER_MSG_TYPE;
        break;
    case enum_efh_sze_lev2_bond_snap:
        ret = SZE_LEV2_BOND_SNAP_MSG_TYPE;
        break;
    case enum_efh_sze_lev2_bond_tick:
        ret = SZE_LEV2_BOND_ORDER_MSG_TYPE * 100 + SZE_LEV2_BOND_EXE_MSG_TYPE;
        break;

    case enum_efh_sse_lev2_snap:
        ret = SSE_LEV2_SNAP_MSG_TYPE;
        break;
    case enum_efh_sse_lev2_idx:
        ret = SSE_LEV2_IDX_MSG_TYPE;
        break;
    case enum_efh_sse_lev2_tick:
        ret = SSE_LEV2_ORDER_MSG_TYPE * 100 + SSE_LEV2_EXE_MSG_TYPE;
        break;
    case enum_efh_sse_lev2_opt:
        ret = SSE_LEV2_OPT_MSG_TYPE;
        break;
    case enum_efh_sse_lev2_tree:
        ret = SSE_LEV2_TREE_MSG_TYPE;
        break;
    case enum_efh_sse_lev2_bond_snap:
        ret = SSE_LEV2_BOND_SNAP_MSG_TYPE;
        break;
    case enum_efh_sse_lev2_bond_tick:
        ret = SSE_LEV2_BOND_TICK_MSG_TYPE;
        break;
    case enum_efh_sse_lev2_tick_merge:
        ret = SSE_LEV2_TICK_MERGE_MSG_TYPE;
        break;
    case enum_efh_sse_lev2_etf:
        ret = SSE_LEV2_ETF_MSG_TYPE;
        break;

    default:
        break;
    }
    return ret;
}

string get_type_name(int type)
{
    string ret;
    switch (type)
    {
    case SSE_LEV2_IDX_MSG_TYPE:
        ret = "SSE_idx       ";
        break;
    case SSE_LEV2_EXE_MSG_TYPE:
        ret = "SSE_tick      ";
        break;
    case SSE_LEV2_OPT_MSG_TYPE:
        ret = "SSE_opt       ";
        break;
    case SSE_LEV2_SNAP_MSG_TYPE:
        ret = "SSE_snap      ";
        break;
    case SSE_LEV2_ORDER_MSG_TYPE:
        ret = "SSE_tick      ";
        break;
    case SSE_LEV2_TREE_MSG_TYPE:
        ret = "SSE_tree      ";
        break;
    case SSE_LEV2_BOND_SNAP_MSG_TYPE:
        ret = "SSE_bond_snap ";
        break;
    case SSE_LEV2_BOND_TICK_MSG_TYPE:
        ret = "SSE_bond_tick ";
        break;
    case SSE_LEV2_TICK_MERGE_MSG_TYPE:
        ret = "SSE_tick_merge";
        break;
    case SSE_LEV2_ETF_MSG_TYPE:
        ret = "SSE_etf       ";
        break;
    case SZE_LEV2_SNAP_MSG_TYPE:
        ret = "SZE_snap     ";
        break;
    case SZE_LEV2_IDX_MSG_TYPE:
        ret = "SZE_idx      ";
        break;
    case SZE_LEV2_ORDER_MSG_TYPE:
        ret = "SZE_tick     ";
        break;
    case SZE_LEV2_EXE_MSG_TYPE:
        ret = "SZE_tick     ";
        break;
    case SZE_LEV2_AF_CLOSE_MSG_TYPE:
        ret = "SZE_after    ";
        break;
    case SZE_LEV2_TREE_MSG_TYPE:
        ret = "SZE_tree     ";
        break;
    case SZE_LEV2_IBR_TREE_MSG_TYPE:
        ret = "SZE_ibr_tree ";
        break;
    case SZE_LEV2_TURNOVER_MSG_TYPE:
        ret = "SZE_turnover ";
        break;
    case SZE_LEV2_BOND_SNAP_MSG_TYPE:
        ret = "SZE_bond_snap";
        break;
    case SZE_LEV2_BOND_ORDER_MSG_TYPE:
        ret = "SZE_bond_tick";
        break;
    case SZE_LEV2_BOND_EXE_MSG_TYPE:
        ret = "SZE_bond_tick";
        break;

    default:
        break;
    }
    return ret;
}
bool operator==(const pkg_tick_info &p1, const pkg_tick_info &p2)
{
    return p1.count == p2.count && p1.last_seq == p2.last_seq && p1.count_lost == p2.count_lost &&
           p1.b_rollback_flag == p2.b_rollback_flag && p1.order_count == p2.order_count && p1.exe_count == p2.exe_count;
}
bool operator==(const pkg_info &p1, const pkg_info &p2)
{
    return p1.count == p2.count && p1.last_seq == p2.last_seq && p1.count_lost == p2.count_lost &&
           p1.b_rollback_flag == p2.b_rollback_flag;
}

std::string get_key(const sse_hpf_lev2 &data)
{
    stringstream ss;

    ss << int(data.m_head.m_message_type) << "+" << data.m_quote_update_time << "+" << data.m_symbol;

    return ss.str();
}
std::string get_key(const sse_hpf_idx &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_quote_update_time << "+" << data.m_symbol;
    return ss.str();
}
std::string get_key(const sse_hpf_exe &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_trade_index << "+" << data.m_channel_num;
    return ss.str();
}
std::string get_key(const sse_hpf_order &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_order_index << "+" << data.m_channel_num;
    return ss.str();
}
std::string get_key(const sse_hpf_tree &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_channel_num << "+" << data.m_symbol << "+" << data.m_biz_index;
    return ss.str();
}
std::string get_key(const sse_hpf_stock_option &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_quote_update_time << "+" << data.m_symbol;
    return ss.str();
}
std::string get_key(const sse_hpf_bond_snap &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_quote_update_time << "+" << data.m_symbol;
    return ss.str();
}
std::string get_key(const sse_hpf_bond_tick &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_tick_index << "+" << data.m_channel_num;
    return ss.str();
}
std::string get_key(const sse_hpf_tick_merge &data)
{
    stringstream ss;
    ss << int(data.m_message_type) << "+" << data.m_tick_index << "+" << data.m_channel_num;
    return ss.str();
}
std::string get_key(const sse_hpf_etf &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_quote_update_time << "+" << data.m_symbol;
    return ss.str();
}

std::string get_key(const sze_hpf_lev2 &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_symbol << "+" << data.m_head.m_quote_update_time;
    return ss.str();
}
std::string get_key(const sze_hpf_idx &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_symbol << "+" << data.m_head.m_quote_update_time;
    return ss.str();
}
std::string get_key(const sze_hpf_order &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_channel_num << "+" << data.m_head.m_sequence_num;
    return ss.str();
}
std::string get_key(const sze_hpf_exe &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_channel_num << "+" << data.m_head.m_sequence_num;
    return ss.str();
}
std::string get_key(const sze_hpf_tree &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_symbol << "+" << data.m_head.m_channel_num << "+"
       << data.m_head.m_sequence_num;
    return ss.str();
}
std::string get_key(const sze_hpf_ibr_tree &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_symbol << "+" << data.m_head.m_channel_num << "+"
       << data.m_head.m_sequence_num;
    return ss.str();
}
std::string get_key(const sze_hpf_turnover &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_symbol << "+" << data.m_head.m_quote_update_time;
    return ss.str();
}
std::string get_key(const sze_hpf_after_close &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_symbol << "+" << data.m_head.m_quote_update_time;
    return ss.str();
}
std::string get_key(const sze_hpf_bond_snap &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_symbol << "+" << data.m_head.m_quote_update_time;
    return ss.str();
}
std::string get_key(const sze_hpf_bond_order &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_channel_num << "+" << data.m_head.m_sequence_num;
    return ss.str();
}
std::string get_key(const sze_hpf_bond_exe &data)
{
    stringstream ss;
    ss << int(data.m_head.m_message_type) << "+" << data.m_head.m_channel_num << "+" << data.m_head.m_sequence_num;
    return ss.str();
}

std::string get_project_path()
{
    std::string para_path = "../config/twap_para.txt";
    std::string project_path = "";
    std::ifstream file(para_path);

    std::string line;
    std::string key;
    std::string value;
    while (std::getline(file, line))
    {
        // 跳过空行和注释行
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        // 查找 '=' 位置
        size_t pos = line.find('=');
        if (pos == std::string::npos)
        {
            // std::cerr << "Invalid line format: " << line << std::endl;
            continue;
        }

        // 提取键和值
        key = line.substr(0, pos);
        value = line.substr(pos + 1);

        std::cout<<key<<value<<std::endl;

        // 根据键赋值给结构体字段
        if (key == "project_dir")
        {
            project_path = value;
            break;
        }
    }
    return project_path;
}

std::string format_date_yyyymmdd(int year, int month, int day)
{
    // 使用 std::setw 和 std::setfill 来格式化日期为 yyyymmdd 格式
    std::ostringstream oss;
    oss << year
        << std::setw(2) << std::setfill('0') << month
        << std::setw(2) << std::setfill('0') << day;
    return oss.str();
}