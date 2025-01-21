#ifndef READ_PARAS_TOOL_H
#define READ_PARAS_TOOL_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <string>
#include <limits>
#include <stdexcept>
#include <map>

// 读取产品信息
struct twap_para
{
    int is_start;
    std::string project_dir;
    std::map<std::string, std::vector<std::string>> fund_paras;
    std::vector<std::string> fund_list;

    std::vector<std::vector<std::string>> agw_list;

    std::string mem_data_dir;
    std::string mem_file_name;
    std::string trade_days_path;
    int twap_num;

    std::string twap_trade_start_time;
    std::string twap_trade_end_time;
    std::string judge_end_time;

    std::vector<std::string> twap_trade_times;
    std::vector<std::string> real_time_trade_times;
    
    int wait_seconds;
};

int read_twap_paras(std::string para_path, struct twap_para &para_obj);

// 读取连接配置文件
struct atp_para
{
    // 终端识别码
    std::string terminal_feature_code = "";

    // 初始化api
    std::string station_name = "";
    std::string cfg_path = ".";
    std::string log_dir_path = "";
    bool record_all_flag = true;
    bool connection_retention_flag = false;

    // 加密库及公钥配置
    // 对接兴业ATP柜台时，必须及时配置加密库及兴业ATP专用的公私钥文件
    std::string ENCRYPT_SCHEMA = "0";
    std::string ATP_LOGIN_ENCRYPT_PASSWORD = "";
    std::string ATP_ENCRYPT_PASSWORD = "";
    std::string GM_SM2_PUBLIC_KEY_PATH = "";
    std::string RSA_PUBLIC_KEY_PATH = "";

    // 连接信息
    std::vector<std::string> user;
    std::vector<std::string> connect_password;
    std::vector<std::string> locations;
    int32_t heartbeat_interval_milli;
    int32_t connect_timeout_milli;
    int32_t reconnect_time;
    std::string client_name = "";
    std::string client_version = "";
    int32_t mode;

    // 登录信息
    std::string branch_id = "";
    std::string cust_id = "";
    std::string fund_account_id = "";
    std::string login_password = "";
    uint32_t login_mode;
    char order_way;
    std::string fundname = "";

    // 委托信息
    std::string stock_account_sh = "";
    std::string stock_account_sz = "";
    // 风控要求
    int max_order_qty;
    int max_order_qty_sec;
    double cancel_pro;
    double scrap_pro;
};

// 转换函数
int32_t stringToInt32(const std::string &str);

int stringToInt(const std::string &str);

uint32_t stringToUInt32(const std::string &str);

bool stringToBool(const std::string &str);
// 定义 trim 函数
std::string trim(const std::string &str);

// 定义 split 函数
std::vector<std::string> split(const std::string &str, char delimiter);

int read_atp_para(std::string para_path, struct atp_para &para_atp, struct twap_para &para_twap, std::string fundname);

std::map<std::string, std::string> get_trade_time_points_vec(std::string fundName, twap_para twap_paras);

// 读取行情配置文件
struct ut_para
{
    std::string account_number;
    std::string username;
    std::string password;
    std::string sz_address;
    std::string sh_address;
};
ut_para read_ut_para(std::string para_path);

#endif