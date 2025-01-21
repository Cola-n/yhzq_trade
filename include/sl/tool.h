#pragma once

#include <string>
#include <vector>

#include "efh_lev2_define.h"
#include "sse_hpf_define.h"
#include "sze_hpf_define.h"

using namespace std;

bool string_split(const char *str_src, vector<string> &str_dst, const string &str_separator);
void lost_pkg_log(int msg_type, int64_t last_sequence, int64_t sequence);

int get_type_from_channel(enum_efh_lev2_type n);
string get_type_name(int type);

#ifdef WINDOWS
#define CLOCK_REALTIME (0)
int clock_gettime(int, struct timespec *spec);
#endif

struct pkg_info
{
    int64_t count = 0;
    int64_t last_seq = -1;
    int64_t count_lost = 0;
    bool b_rollback_flag = false;
    string multicast_ip = "x.x.x.x";
    int64_t multicast_port = 0;
};
bool operator==(const pkg_info &p1, const pkg_info &p2);

struct pkg_tick_info
{
    int64_t count = 0;
    int64_t exe_count = 0;
    int64_t order_count = 0;
    int64_t last_seq = -1;
    int64_t count_lost = 0;
    bool b_rollback_flag = false;
    string multicast_ip = "x.x.x.x";
    int64_t multicast_port = 0;
};
bool operator==(const pkg_tick_info &p1, const pkg_tick_info &p2);

std::string get_key(const sse_hpf_lev2 &data);
std::string get_key(const sse_hpf_idx &data);
std::string get_key(const sse_hpf_exe &data);
std::string get_key(const sse_hpf_order &data);
std::string get_key(const sse_hpf_tree &data);
std::string get_key(const sse_hpf_stock_option &data);
std::string get_key(const sse_hpf_bond_snap &data);
std::string get_key(const sse_hpf_bond_tick &data);
std::string get_key(const sse_hpf_tick_merge &data);
std::string get_key(const sse_hpf_etf &data);

std::string get_key(const sze_hpf_lev2 &data);
std::string get_key(const sze_hpf_idx &data);
std::string get_key(const sze_hpf_order &data);
std::string get_key(const sze_hpf_exe &data);
std::string get_key(const sze_hpf_tree &data);
std::string get_key(const sze_hpf_ibr_tree &data);
std::string get_key(const sze_hpf_turnover &data);
std::string get_key(const sze_hpf_after_close &data);
std::string get_key(const sze_hpf_bond_snap &data);
std::string get_key(const sze_hpf_bond_order &data);
std::string get_key(const sze_hpf_bond_exe &data);

std::string get_project_path();
bool isBefore(int hour, int minute, int second);
std::string format_date_yyyymmdd(int year, int month, int day);
