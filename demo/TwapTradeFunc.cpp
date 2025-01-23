/**
 * encodeing :gb2312
 *
 * **/

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <thread>
#include <map>
#include <sys/stat.h>

#include "trade_twap.h"

int read_twap_paras(std::string para_path, struct twap_para &para_obj)
{
    std::ifstream file(para_path);
    if (!file.is_open())
    {
        std::cerr << "Can't open the file: " << para_path << std::endl;
        return false;
    }

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

        // std::cout << key <<std::endl;

        // 根据键赋值给结构体字段
        if (key == "is_start")
        {
            para_obj.is_start = std::stoi(value);
        }
        else if (key == "project_dir")
        {
            para_obj.project_dir = trim(value);
        }
        else if (key == "fund_paras")
        {
            size_t colon_pos = value.find(':');
            size_t comma_pos = value.find(',');

            if (colon_pos != std::string::npos && comma_pos != std::string::npos)
            {

                std::string fundname = value.substr(0, colon_pos);
                std::string fundaccount = value.substr(colon_pos + 1);

                std::vector<std::string> values;
                std::stringstream ss(fundaccount);
                std::string item;

                while (std::getline(ss, item, ','))
                {
                    values.push_back(item);
                }
                para_obj.fund_paras[fundname] = values;
            }
        }
        else if (key == "fund_list")
        {
            std::string token;
            std::stringstream ss(trim(value));

            while (std::getline(ss, token, ','))
            {
                para_obj.fund_list.push_back(token);
            }
        }
        else if (key == "agw_list")
        {
            std::string token;
            std::stringstream ss(trim(value));
            std::vector<std::string> agw;

            while (std::getline(ss, token, ','))
            {
                agw.push_back(token);
            }
            para_obj.agw_list.push_back(agw);
        }
        else if (key == "mem_data_dir")
        {
            para_obj.mem_data_dir = trim(value);
        }
        else if (key == "mem_file_name")
        {
            para_obj.mem_file_name = trim(value);
        }
        else if (key == "twap_num")
        {
            para_obj.twap_num = std::stoi(value);
        }
        else if (key == "trade_days_path")
        {
            para_obj.trade_days_path = trim(value);
        }
        else if (key == "twap_trade_start_time")
        {
            para_obj.twap_trade_start_time = value;
        }
        else if (key == "twap_trade_end_time")
        {
            para_obj.twap_trade_end_time = value;
        }
        else if (key == "judge_end_time")
        {
            para_obj.judge_end_time = value;
        }
        else if (key == "twap_trade_times")
        {
            std::istringstream iss(value);
            std::string time;
            while (std::getline(iss, time, ','))
            {
                para_obj.twap_trade_times.push_back(time);
            }
        }
        else if (key == "real_time_trade_times")
        {
            std::istringstream iss(value);
            std::string time;
            while (std::getline(iss, time, ','))
            {
                para_obj.real_time_trade_times.push_back(time);
            }
        }
        else if (key == "wait_seconds")
        {
            para_obj.wait_seconds = std::stoi(value);
        }
        else
        {
            std::cerr << "unknown field: " << key << std::endl;
        }
    }

    file.close();
    return true;
}

// 读取行情配置文件
ut_para read_ut_para(std::string para_path)
{
    ut_para para_ut;
    std::ifstream file(para_path);

    if (!file.is_open())
    {
        std::cerr << "Can't open the file: " << para_path << std::endl;
    }

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

        // std::cout<<key<<std::endl;

        // 根据键赋值给结构体字段
        if (key == "account_number")
        {
            para_ut.account_number = trim(value);
        }
        else if (key == "username")
        {
            para_ut.username = trim(value);
        }
        else if (key == "password")
        {
            para_ut.password = std::to_string(std::stol(trim(value)) / 922519);
        }
        else if (key == "sh_address")
        {
            if (!value.empty() && value[0] == '\"')
            {
                value = value.substr(1);
            }
            if (!value.empty() && value[value.size() - 1] == '\"')
            {
                value = value.substr(0, value.size() - 1);
            }
            para_ut.sh_address = trim(value);
        }
        else if (key == "sz_address")
        {
            if (!value.empty() && value[0] == '\"')
            {
                value = value.substr(1);
            }
            if (!value.empty() && value[value.size() - 1] == '\"')
            {
                value = value.substr(0, value.size() - 1);
            }
            para_ut.sz_address = trim(value);
        }
        else
        {
            std::cerr << "unknown field: " << key << std::endl;
        }
    }
    file.close();
    return para_ut;
}

// 转换函数
int32_t stringToInt32(const std::string &str)
{
    try
    {
        int value = std::stoi(str); // 将字符串转换为整数
        if (value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max())
        {
            throw std::out_of_range("Value out of range for int8_t");
        }
        return static_cast<int32_t>(value);
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Invalid parameter: " << e.what() << std::endl;
        throw;
    }
    catch (const std::out_of_range &e)
    {
        std::cerr << "Number out of range: " << e.what() << std::endl;
        throw;
    }
}

int stringToInt(const std::string &str)
{
    try
    {
        int value = std::stoi(str); // 将字符串转换为整数
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::out_of_range("Value out of range for int8_t");
        }
        return static_cast<int>(value);
    }
    catch (const std::invalid_argument &e)
    {
        return 112233;
        // std::cerr << "Invalid parameter: " << e.what() << std::endl;
        // throw;
    }
    catch (const std::out_of_range &e)
    {
        std::cerr << "Number out of range: " << e.what() << std::endl;
        throw;
    }
}

// 转换函数
uint32_t stringToUInt32(const std::string &str)
{
    try
    {
        int value = std::stoi(str); // 将字符串转换为整数
        if (value < 0 || value > std::numeric_limits<uint32_t>::max())
        {
            throw std::out_of_range("Value out of range for uint32_t");
        }
        return static_cast<uint32_t>(value);
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Invalid parameter: " << e.what() << std::endl;
        throw;
    }
    catch (const std::out_of_range &e)
    {
        std::cerr << "Number out of range: " << e.what() << std::endl;
        throw;
    }
}

// 转换函数
bool stringToBool(const std::string &str)
{
    if (str == "true" || str == "1")
    {
        return true;
    }
    else if (str == "false" || str == "0")
    {
        return false;
    }
    else
    {
        throw std::invalid_argument("Invalid boolean string: " + str);
    }
}

// 定义 trim 函数
std::string trim(const std::string &str)
{
    if (str.empty())
    {
        return "";
    }
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
    {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// 定义 split 函数
std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

// 定义 isExistFile 函数
bool isExistFile(std::string filePath)
{
    std::ifstream file(filePath);
    return file.good();
}

void print_ut_code_Info(const ut_code_Info &info)
{
    std::cout << "StockCode: " << info.StockCode << std::endl;
    std::cout << "market_time: " << info.time << std::endl;
    std::cout << "real_price: " << std::fixed << std::setprecision(2) << info.real_price << std::endl;
    std::cout << "yes_close_price: " << std::fixed << std::setprecision(2) << info.yes_close << std::endl;
    std::cout << "open_price: " << std::fixed << std::setprecision(2) << info.open_price << std::endl;
    std::cout << "riseup_price: " << std::fixed << std::setprecision(2) << info.riseup_price << std::endl;
    std::cout << "downstop_price: " << std::fixed << std::setprecision(2) << info.downstop_price << std::endl;
    std::cout << "-----------------" << std::endl;
}

void printOrderReq(const OrderReqField &info)
{

    std::cout << info.stock_code << "  "
              << info.market << "  "
              << info.direction << "  "
              << info.order_quantity << "  "
              << info.order_price << "  "
              << info.order_way << std::endl;
}

std::string TradeTwapClass::get_current_time_ms()
{
    // 获取系统当前时间（微秒级）
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    // 获取微秒部分
    auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto us = now_us.time_since_epoch() % 1000000;

    // 转换为本地时间（注意：localtime 不是线程安全的）
    struct tm time_info;
#ifdef _WIN32
    localtime_s(&time_info, &now_time_t); // Windows 线程安全版本
#else
    localtime_r(&now_time_t, &time_info); // POSIX 线程安全版本
#endif

    // 格式化输出时间
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time_info);

    // 拼接结果，包含微秒
    char result[100];
    sprintf(result, "%s.%06ld", buffer, static_cast<long>(us.count()));

    return std::string(result);
}

// TWAP分单写入txt文件留存
void writeOrdersToFile(const std::vector<OrderReqField> &orders, const std::string &filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }
    file << "股票代码,市场,方向,订单数量,订单价格,订单方式" << std::endl;
    for (const auto &order : orders)
    {
        file << order.stock_code << ", "
             << order.market << ", "
             << order.direction << ", "
             << order.order_quantity << ", "
             << order.order_price << ", "
             << static_cast<int>(order.order_way) << std::endl;
    }

    file.close();
}

// 读取参数配置函数
int read_atp_para(std::string para_path, struct atp_para &para_atp, struct twap_para &para_twap, std::string fundname)
{
    std::ifstream file(para_path);
    if (!file.is_open())
    {
        std::cerr << "Can't open the file: " << para_path << std::endl;
        return false;
    }

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
        key = trim(key);
        value = trim(value);
        // std::cout << key << "  " << value << std::endl;
        // 1.终端识别码
        if (key == "terminal_feature_code")
            para_atp.terminal_feature_code = value;
        // 2.初始化api
        else if (key == "station_name")
            para_atp.station_name = value;
        else if (key == "cfg_path")
            para_atp.cfg_path = value;
        else if (key == "log_dir_path")
            para_atp.log_dir_path = value;
        else if (key == "record_all_flag")
            para_atp.record_all_flag = stringToBool(value);
        else if (key == "connection_retention_flag")
            para_atp.connection_retention_flag = stringToBool(value);
        // 3.加密库及公钥配置
        else if (key == "ENCRYPT_SCHEMA")
            para_atp.ENCRYPT_SCHEMA = value;
        else if (key == "ATP_LOGIN_ENCRYPT_PASSWORD")
            para_atp.ATP_LOGIN_ENCRYPT_PASSWORD = value;
        else if (key == "ATP_ENCRYPT_PASSWORD")
            para_atp.ATP_ENCRYPT_PASSWORD = value;
        else if (key == "GM_SM2_PUBLIC_KEY_PATH")
            para_atp.GM_SM2_PUBLIC_KEY_PATH = value;
        else if (key == "RSA_PUBLIC_KEY_PATH")
            para_atp.RSA_PUBLIC_KEY_PATH = value;
        // 4.连接信息
        else if (key == "user")
            para_atp.user.push_back(trim(value));
        else if (key == "connect_password")
            para_atp.connect_password.push_back(trim(value));
        else if (key == "locations")
            para_atp.locations.push_back(trim(value));
        else if (key == "heartbeat_interval_milli")
            para_atp.heartbeat_interval_milli = std::stoi(value);
        else if (key == "connect_timeout_milli")
            para_atp.connect_timeout_milli = std::stoi(value);
        else if (key == "reconnect_time")
            para_atp.reconnect_time = std::stoi(value);
        else if (key == "client_name")
            para_atp.client_name = trim(value);
        else if (key == "client_version")
            para_atp.client_version = trim(value);
        else if (key == "mode")
            para_atp.mode = std::stoi(value);
        // 5.登录信息
        // else if (key == "cust_id")
        //     para_atp.cust_id = value;
        // else if (key == "fund_account_id")
        //     para_atp.fund_account_id = value;
        // else if (key == "login_password")
        //     para_atp.login_password = value;
        else if (key == "login_mode")
            para_atp.login_mode = stringToUInt32(value);
        else if (key == "order_way")
            para_atp.order_way = trim(value)[0];
        // else if (key == "stock_account_sh")
        //     para_atp.stock_account_sh = value;
        // else if (key == "stock_account_sz")
        //     para_atp.stock_account_sz = value;

        else if (key == "max_order_qty")
            para_atp.max_order_qty = stringToUInt32(value);
        else if (key == "max_order_qty_sec")
            para_atp.max_order_qty_sec = stringToInt32(value);
        else if (key == "cancel_pro")
            para_atp.cancel_pro = std::stod(value);
        else if (key == "scrap_pro")
            para_atp.scrap_pro = std::stod(value);
        else
            continue;
    }

    file.close();
    para_atp.fundname = fundname;
    std::vector<std::string> login_value = para_twap.fund_paras[fundname];
    login_value[0] = trim(login_value[0]);
    login_value[1] = trim(login_value[1]);
    login_value[2] = trim(login_value[2]);
    login_value[3] = trim(login_value[3]);
    para_atp.cust_id = login_value[0];
    para_atp.fund_account_id = login_value[0];
    long int result = std::stol(login_value[1]) / 922519;
    para_atp.login_password = std::to_string(result);
    para_atp.stock_account_sh = login_value[2];
    para_atp.stock_account_sz = login_value[3];

    return 1;
}

// 使用 find 方法检查键是否存在
bool isKeyInMap(const std::map<std::string, std::vector<std::string>> &dictionary, const std::string &key)
{

    return dictionary.find(key) != dictionary.end();
}
// 转大写函数
std::string toUpperCase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c)
                   { return std::toupper(c); });
    return result;
}

bool TradeTwapClass::isBefore(int hour, int minute, int second)
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

std::string get_now_time()
{
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);

    // 使用stringstream来格式化时间字符串
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << now_tm.tm_hour
        << std::setw(2) << now_tm.tm_min
        << std::setw(2) << now_tm.tm_sec;

    std::string nowtime = oss.str();
    // std::cout << "nowtime is " << now_tm.tm_hour << ":" << now_tm.tm_min << ":" << now_tm.tm_sec << std::endl;
    return nowtime;
}

/***************************************************************
 *  @brief     TradeTwapClass类的构造函数
 *  @param     程序的路径，产品名称，监控路径，交易日文件路径
 *  @note
 **************************************************************/

TradeTwapClass::TradeTwapClass(twap_para twap_paras, std::string fund) : project_dir(twap_paras.project_dir),
                                                                         twap_num(twap_paras.twap_num),
                                                                         trade_day_path(twap_paras.trade_days_path),
                                                                         twap_trade_start_time(twap_paras.twap_trade_start_time),
                                                                         twap_trade_end_time(twap_paras.twap_trade_end_time),
                                                                         judge_end_time(twap_paras.judge_end_time),
                                                                         sell_list(6),
                                                                         wait_seconds(twap_paras.wait_seconds),
                                                                         buy_list(6)
{
    // 交易日日期相关
    get_trade_day();
    year = today.substr(0, 4);

    fundname = toUpperCase(fund);

    // 相关路径定义
    fund_dir = project_dir + fundname + "/";
    twap_orders_dir = fund_dir + fundname + "_Orders/";
    order_dir = fund_dir + "Sell_Buy_List_" + fundname + "/";
    create_directory(fund_dir + "/OrderRecord/" + today);
    order_done_record_path = fund_dir + "/OrderRecord/" + today + "/OrderRecord.csv";

    buy_signal_path = order_dir + "/BuyOrderList" + fundname + "_" + today + ".csv";
    sell_signal_path = order_dir + "/SellOrderList" + fundname + "_" + today + ".csv";
    yes_target_file_path = get_target_holding_file_path(fundname, order_dir, last_trade_day);

    position_diff_file_path = fund_dir + "/Diff";
    create_directory(position_diff_file_path);
    trade_flag_file_path = fund_dir + "tradeflag_" + fundname + ".txt";

    // 初始化分单下单时间
    trade_time_map = get_trade_time_points_vec(fundname, twap_paras);

    // 初始化卖单信息
    init_sell_map();

    limit_price_path = project_dir + "limit_price_file/" + today + "_limit_price.csv";

    // 创建文件存储路径
    time_t now = time(NULL);
    tm *ltm = localtime(&now);
    std::string str_full_dir = twap_paras.project_dir + "/data/" + today;
    create_directory(str_full_dir);

}

void create_directory(std::string path)
{
    struct stat info;

    // 检查文件夹是否存在
    if (stat(path.c_str(), &info) != 0)
    {
        // 检查错误是否是由于目录不存在
        if (errno == ENOENT)
        {
            // 文件夹不存在，尝试创建文件夹
            if (mkdir(path.c_str(), 0755) == 0)
            {
                // std::cout << "Directory created successfully: " << path << std::endl;
            }
            else
            {
                std::cerr << "Cannot create the directory: " << path << ". Error: " << strerror(errno) << std::endl;
                std::cin.get();
                std::cin.get();
            }
        }
        else
        {
            std::cerr << "Stat failed. Error: " << strerror(errno) << std::endl;
        }
    }
    else if (S_ISDIR(info.st_mode))
    {
        // std::cout << "Directory already exists: " << path << std::endl;
    }
    else
    {
        std::cerr << "Path exists but is not a directory: " << path << std::endl;
    }
}

std::map<std::string, std::string> get_trade_time_points_vec(std::string fundName, twap_para twap_paras)
{

    auto it = std::find(twap_paras.fund_list.begin(), twap_paras.fund_list.end(), fundName);
    int position = 0;
    if (it != twap_paras.fund_list.end())
    {
        position = std::distance(twap_paras.fund_list.begin(), it);
    }
    // std::cout<<"position::"<<position<<std::endl;
    std::map<std::string, std::string> trade_time_points;
    std::vector<std::string> twap_name = {"twap1", "twap2", "twap3", "twap4", "twap5", "twap6", "morning", "morning2Two", "afternoon", "afternoon2Two"};
    std::vector<std::string> temp_twap_times = twap_paras.twap_trade_times;
    std::vector<std::string> real_time_signal_times = twap_paras.real_time_trade_times;
    // 处理分单交易时间
    int index = 0;
    for (std::string &timeStr : temp_twap_times)
    {
        int hours = std::stoi(timeStr.substr(0, 2));
        int minutes = std::stoi(timeStr.substr(2, 2));
        int totalMinutes = hours * 60 + minutes;
        int totalseconds = totalMinutes * 60;
        totalseconds += position * 3;

        // 重新计算时
        totalMinutes = totalseconds / 60;
        int seconds = totalseconds % 60;
        hours = totalMinutes / 60;
        minutes = totalMinutes % 60;
        hours %= 24;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours
            << std::setw(2) << std::setfill('0') << minutes
            << std::setw(2) << std::setfill('0') << seconds;

        trade_time_points[twap_name[index]] = oss.str();
        index++;
    }
    // 处理实时信号交易时间
    // index = 0;
    for (std::string &timeStr2 : real_time_signal_times)
    {
        if (timeStr2.length() > 10)
        {
            continue;
        }
        int hours = stoi(timeStr2.substr(0, 2));
        int minutes = stoi(timeStr2.substr(2, 2));
        int totalMinutes = hours * 60 + minutes;
        // totalMinutes += position;      //未区分每个产品的实时信号下单时间
        // 重新计算时间
        hours = totalMinutes / 60;
        minutes = totalMinutes % 60;
        hours %= 24;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours
            << std::setw(2) << std::setfill('0') << minutes
            << "00";
        trade_time_points[twap_name[index]] = oss.str();
        index++;
    }
    // for (const auto &pair : trade_time_points)
    // {
    //     std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    // }

    return trade_time_points;
}

int countLines(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return -1;
    }

    int lineCount = 0;
    std::string line;
    while (std::getline(file, line))
    {
        ++lineCount;
    }

    file.close();
    return lineCount;
}



void TradeTwapClass::get_target_holding_map(std::string holdingPath)
{
    target_position.clear();

    std::ifstream file(holdingPath);

    // std::cout<<"holdingPath:"<<holdingPath<<std::endl;

    std::map<std::string, std::string> targetHolding;
    std::string line;

    // 跳过第一行（字段名）
    std::getline(file, line);
    int line_number = 1;
    int lines = countLines(holdingPath);

    while (std::getline(file, line))
    {
        // std::string key, value;

        // key = line.substr(1, 6);
        // value = line.substr(11);

        // 检查行是否不包含数字
        bool contains_digit = false;
        for (char ch : line) // 遍历字符串中的每个字符
        {
            if (ch >= '0' && ch <= '9') // 判断是否为数字
            {
                contains_digit = true;
                break;
            }
        }
        if (!contains_digit) // 如果不包含数字，停止读取
        {
            break;
        }
        std::string token;
        std::stringstream ss(line);
        std::vector<std::string> data;

        while (std::getline(ss, token, ','))
        {
            data.push_back(token);
            // std::cout<<token<<std::endl;
        }

        PositionInfo info;

        if (data[0][0] == '"')
            strncpy(info.StockCode, data[0].substr(1, 6).c_str(), 9);
        else
            strncpy(info.StockCode, data[0].substr(0, 6).c_str(), 9);

        // strncpy(info.StockCode, data[0].substr(1, 6).c_str(), 9);

        // std::cout<<data[0].substr(1,6).c_str()<<"  "<<data[1] <<std::endl;

        info.current_volume = stringToInt(data[1]);
        info.target_volume = stringToInt(data[1]);

        target_position[info.StockCode] = info;
        // targetHolding[key] = value;

        line_number++;
        if (line_number == lines - 3)
        {
            break;
        }
    }
}

void TradeTwapClass::update_target_holding_map(std::vector<struct OrderReqField> order_array)
{
    for (const auto &it : order_array)
    {
        std::string key = it.stock_code;
        // 该笔订单不存在于当前目标持仓中
        if ((target_position.find(key) == target_position.end()))
        {
            // std::cout << "   new target holding ..." << std::endl;
            PositionInfo info;
            strncpy(info.StockCode, key.c_str(), 9);
            info.target_volume = it.order_quantity;
            target_position[key.c_str()] = info;
        }
        // 该笔订单存在于当前目标持仓中
        else
        {
            // 买单数量增加
            if (it.direction == 1)
            {
                int new_holding = target_position[key].target_volume + it.order_quantity;
                target_position[key].target_volume = new_holding;
            }
            // 卖单数量减少
            else
            {
                int new_holding = target_position[key].target_volume - it.order_quantity;
                target_position[key.c_str()].target_volume = new_holding;
            }
        }
    }
}

void TradeTwapClass::update_target_holding_record_csv(std::vector<struct OrderReqField> order_array)
{
    struct stat buffer;
    if (stat(order_done_record_path.c_str(), &buffer) == 0)
    {
        std::ofstream file(order_done_record_path, std::ios::app);
        if (!file.is_open())
        {
            std::cerr << "无法打开文件: " << order_done_record_path << std::endl;
            return;
        }
        // 写入订单数据
        for (const OrderReqField &order : order_array)
        {
            file << order.stock_code << ","
                 << order.market << ","
                 << order.direction << ","
                 << order.order_quantity << ","
                 << order.order_price << ","
                 << order.order_way << std::endl;
        }

        file.close();
    }
    else
    {
        std::ofstream file(order_done_record_path, std::ios::app);
        if (!file.is_open())
        {
            std::cerr << "无法打开文件: " << order_done_record_path << std::endl;
            return;
        }
        // 写入CSV文件头
        file << "stock_code,market,direction,order_quantity,order_price,order_way" << std::endl;
        // 写入订单数据
        for (const OrderReqField &order : order_array)
        {
            file << order.stock_code << ","
                 << order.market << ","
                 << order.direction << ","
                 << order.order_quantity << ","
                 << order.order_price << ","
                 << order.order_way << std::endl;
        }

        file.close();
    }
}

std::vector<OrderReqField> TradeTwapClass::get_orderRecode_fromCSV()
{
    std::vector<OrderReqField> orders;
    std::ifstream file(order_done_record_path);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件: " << order_done_record_path << std::endl;
        return orders;
    }

    std::string line;
    // 跳过CSV文件头
    std::getline(file, line);

    // 读取订单数据
    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string field;
        OrderReqField order;

        std::getline(ss, field, ',');
        order.stock_code = field;

        std::getline(ss, field, ',');
        order.market = std::stoi(field);

        std::getline(ss, field, ',');
        order.direction = std::stoi(field);

        std::getline(ss, field, ',');
        order.order_quantity = std::stoi(field);

        std::getline(ss, field, ',');
        try
        {
            // 尝试将字段转换为 double
            order.order_price = std::stod(field);
        }
        catch (const std::invalid_argument &e) // 捕获无效格式异常
        {
            // 如果字段无法转换为 double，则设置为 0.0
            order.order_price = 0.0;
            std::cerr << "Invalid argument: " << e.what() << " - Setting order_price to 0.0" << std::endl;
        }
        catch (const std::out_of_range &e) // 捕获超出范围异常
        {
            // 如果字段超出范围，则设置为 0.0
            order.order_price = 0.0;
            std::cerr << "Out of range: " << e.what() << " - Setting order_price to 0.0" << std::endl;
        }

        std::getline(ss, field, ',');
        order.order_way = field[0];

        orders.push_back(order);
    }

    file.close();
    return orders;
}

std::string TradeTwapClass::get_target_holding_file_path(std::string fundname, std::string order_dir, std::string date)
{

    std::vector<std::string> file_names = {
        "JinRiChiCang" + fundname + "afternoon2Two_" + date + ".csv",
        "JinRiChiCang" + fundname + "afternoon_" + date + ".csv",
        "JinRiChiCang" + fundname + "morning2Two_" + date + ".csv",
        "JinRiChiCang" + fundname + "morning_" + date + ".csv",
        "JinRiChiCang" + fundname + "_" + date + ".csv"

    };
    std::string filePath;
    while (true)
    {
        bool found = false;
        for (const auto &file_name : file_names)
        {
            std::string tempPath = order_dir + file_name;
            std::ifstream file(tempPath);
            if (file.good())
            {
                return tempPath; // 找到文件后返回路径
            }

            // std::cout<<tempPath<<std::endl;
        }

        // 如果当前时间在9:30之前，等待10秒后再次检查
        if (isBefore(9, 30, 0))
        {
            std::cout << "Don't find " << date << "'s target holding file, wait for it's coming..." << std::endl;
            sleep(10);
        }
        else
        {
            // 如果当前时间已经过了9:30，提示用户手动检查文件
            std::cout << "Don't find " << date << "'s target holding file, please check it and press enter to continue:";
            std::cin.get();
            std::cin.get();
        }
    }
}

void TradeTwapClass::get_trade_day()
{
    /***************************************************************
     *  @brief     获取当天日期，以及上个交易日和下个交易日的日期
     *  @param     交易日文件路径，当天，上个交易日，下个交易日
     *  @note
     **************************************************************/
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);

    // std::ostringstream date;
    // date << std::put_time(&now_tm, "%Y%m%d");
    char date[20];
    std::strftime(date, sizeof(date), "%Y-%m-%d", &now_tm);

    std::string temptoday(date);
    temptoday = "2025-01-20";
    today = temptoday;
    
    std::string previous_trade_day;
    std::string temp_date;
    std::ifstream infile(trade_day_path);

    if (infile.is_open())
    {
        std::string line;
        std::cout << line << std::endl;
        while (std::getline(infile, line))
        {
            if (temp_date == temptoday)
            {
                next_trade_day = trim(line);
            }
            if (trim(line) == temptoday)
            {
                last_trade_day = temp_date;
            }
            temp_date = trim(line);
        }
        infile.close();
    }
    else
    {
        std::cerr << "Cannot open trade days file : " << strerror(errno) << std::endl
                  << trade_day_path << std::endl;
        previous_trade_day = "0";
    }

    last_trade_day = delCharInString(last_trade_day, '-');
    today = delCharInString(today, '-');
    next_trade_day = delCharInString(next_trade_day, '-');

    // std::cout<<last_trade_day<<std::endl;
    // std::cout<<today<<std::endl;
    // std::cout<<next_trade_day<<std::endl;
    std::cout << "Today is " << today << ", last trade day is " << last_trade_day << ", next_trade_day is " << next_trade_day << std::endl;
}

std::string delCharInString(std::string str, char cha)
{
    size_t pos;
    while ((pos = str.find(cha)) != std::string::npos)
    {
        str.erase(pos, 1); // 从 pos 位置开始删除 1 个字符
    }
    return str;
}

void TradeTwapClass::set_trade_flag(int num)
{
    std::ofstream file(trade_flag_file_path, std::ios::trunc);

    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << trade_flag_file_path << std::endl;
    }
    // 清空文件内容
    file.seekp(0);
    file.clear();

    // 写入新内容
    file << num;
    // std::cout << "Trade Flag = " << num << std::endl;
    // 关闭文件
    file.close();
}

void TradeTwapClass::get_trade_flag()
{
    std::ifstream file(trade_flag_file_path);

    if (!file.is_open())
    {
        std::cerr << "Can't open the file: " << trade_flag_file_path << std::endl;
    }
    std::string line;
    std::getline(file, line);
    int num = stringToInt32(line);
    trade_flag = num;

    // std::cout << "Trade Flag = " << trade_flag << std::endl;
    // 关闭文件
    file.close();
}

// 分单后重排序
std::vector<int> rearrange(const std::vector<int> &parts)
{
    std::vector<int> rearranged;
    size_t n = parts.size();

    // 遍历奇数索引的元素
    for (size_t i = 0; i < n; i += 2)
    {
        rearranged.push_back(parts[i]);
    }

    // 遍历偶数索引的元素
    for (size_t i = 1; i < n; i += 2)
    {
        rearranged.push_back(parts[i]);
    }

    return rearranged;
}

void TradeTwapClass::init_sell_map()
{
    std::ifstream infileSell(sell_signal_path);
    if (infileSell.is_open())
    {
        std::string line;
        std::getline(infileSell, line); // 跳过字段名
        while (std::getline(infileSell, line))
        {
            std::string token;
            std::stringstream ss(trim(line));
            // 检查行是否只包含双引号
            if (line.find_first_not_of('"') == std::string::npos)
            {
                break; // 如果是，停止读取
            }

            SellInfo sellInfo;
            int i = 1;
            std::string code;

            while (std::getline(ss, token, ','))
            {

                if (i == 1)
                    if (token[0] == '"')
                        code = token.substr(1, 6);
                    else
                        code = token.substr(0, 6);
                else if (i == 3)
                    sellInfo.total_sell_quantity = stringToInt(token);
                i++;
            }

            sellInfo.total_sell_quantity = 0;

            sell_map[code] = sellInfo;
        }
        infileSell.close();
    }
    else
    {
        std::cerr << "Cannot open sell signal file : " << strerror(errno) << std::endl
                  << sell_signal_path << std::endl;
    }
}

std::vector<struct OrderReqField> TradeTwapClass::read_real_time_signal(std::string file_path, bool is_sell_signal)
{
    std::vector<struct OrderReqField> siganl_array;
    std::ifstream signal_file(file_path);
    std::cout<<"is_sell_signal: "<<is_sell_signal<<std::endl;
    if (signal_file.is_open())
    {
        std::string line;
        std::getline(signal_file, line); // 跳过字段名
        while (std::getline(signal_file, line))
        {
            std::string token;
            std::stringstream ss(trim(line));
            if (line.empty())
                continue;
            OrderReqField order;
            std::vector<std::string> orderSignalInfo;
            while (std::getline(ss, token, ','))
            {
                orderSignalInfo.push_back(token);
                // std::cout << token << "  ";
            }

            if (orderSignalInfo[0][0] == '"')
                order.stock_code = orderSignalInfo[0].substr(1, 6);
            else
                order.stock_code = orderSignalInfo[0].substr(0, 6);

            order.direction = stringToInt(orderSignalInfo[1]);
            if (order.stock_code.substr(0, 1) == "6")
            {
                order.market = 1;
            }
            else
            {
                order.market = 2;
            }

            int total_quantity = stringToInt(orderSignalInfo[2]);
            order.order_quantity = total_quantity;
            order.order_price = (is_sell_signal ? limit_price_map[order.stock_code][1] : limit_price_map[order.stock_code][0]);
            siganl_array.push_back(order);
        }
        signal_file.close();
        return siganl_array;
    }
    else
    {
        std::cerr << "Cannot open the signal file : " << strerror(errno) << std::endl
                  << file_path << std::endl;
    }
}

std::string toCsvRow(const std::vector<struct OrderReqField> &orderList)
{
    std::stringstream ss;
    for (const auto &order : orderList)
    {
        ss << order.stock_code << ","
           << order.market << ","
           << order.direction << ","
           << order.order_quantity << ","
           << order.order_price << ","
           << order.order_way;
        ss << "\n"; // 每行结束时换行
    }
    return ss.str();
}

void savetTwapOrdersToCsv(std::vector<std::vector<struct OrderReqField>> ordersList, std::string filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }

    // 写入表头
    file << "stock_code,market,direction,order_quantity,order_price,order_way\n";

    // 遍历每一行订单
    for (const auto &orderList : ordersList)
    {
        file << toCsvRow(orderList); // 将每个子 vector 写入 CSV 行
    }

    file.close();
}

std::vector<struct OrderReqField> TradeTwapClass::get_diff_order()
{
    std::vector<struct OrderReqField> diff_order;
    for (const auto &row : diff_position)
    {
        // 有一字涨停、盘中涨停情况
        if (row[row.size() - 1] != "0")
        {
            continue;
        }
        OrderReqField order;
        std::vector<std::string> orderSignalInfo;
        for (const auto &item : row)
        {
            // std::cout<<item<<std::endl;
            orderSignalInfo.push_back(item);
        }
        order.stock_code = orderSignalInfo[0].substr(0, 6);

        if (order.stock_code.substr(0, 1) == "6")
        {
            order.market = 1;
        }
        else
        {
            order.market = 2;
        }

        int total_quantity = stringToInt(orderSignalInfo[3]);
        order.order_quantity = total_quantity;

        if (total_quantity > 0)
        {
            order.direction = 1;
        }
        else
        {
            order.direction = 2;
        }

        order.order_way = 13;
        order.order_price = order.direction == 1 ? limit_price_map[order.stock_code][0] : limit_price_map[order.stock_code][1];

        diff_order.push_back(order);
    }
    return diff_order;
}

std::vector<std::vector<OrderReqField>> TradeTwapClass::split_orders(const std::string &signal_path, bool is_buy_order)
{
    std::ifstream infile(signal_path);
    std::vector<std::vector<OrderReqField>> order_list(6); // 分成6个部分
    if (infile.is_open())
    {
        std::string line;
        std::getline(infile, line); // 跳过字段名

        while (std::getline(infile, line))
        {
            std::string token;
            std::stringstream ss(trim(line));
            if (line.empty())
                continue;
            OrderReqField order;
            std::vector<std::string> orderSignalInfo;

            while (std::getline(ss, token, ','))
            {
                orderSignalInfo.push_back(token);
            }

            if (stringToInt(orderSignalInfo[2]) == 0)
            {
                continue;
            }
            if (orderSignalInfo.empty())
            {
                std::cerr << "Empty order signal line: " << line << std::endl;
                continue;
            }

            if (orderSignalInfo[0][0] == '"')
                order.stock_code = orderSignalInfo[0].substr(1, 6);
            else
                order.stock_code = orderSignalInfo[0].substr(0, 6);
            order.direction = stringToInt(orderSignalInfo[1]);

            if (order.stock_code.substr(0, 1) == "6")
            {
                order.market = 1;
            }
            else
            {
                order.market = 2;
            }

            int total_quantity = stringToInt(orderSignalInfo[2]);
            int min_qty = (order.stock_code.substr(0, 3) == "688") ? 200 : 100;
            int k = 6;

            int largest_multiple_of_min_qty = (total_quantity / (min_qty * k)) * min_qty * k;
            int remainder = total_quantity - largest_multiple_of_min_qty;

            int base_size = 0;
            if (largest_multiple_of_min_qty >= (k * min_qty))
            {
                base_size = largest_multiple_of_min_qty / k;
            }

            std::vector<int> parts(k, base_size);
            int quotient = remainder / min_qty;      // 商
            int remainder_min = remainder % min_qty; // 余数

            for (int i = 0; i < quotient; ++i)
            {
                parts[i] += min_qty;
            }
            if (remainder_min != 0)
            {
                parts[0] += remainder_min;
            }
            parts = rearrange(parts);

            // 根据分配的每个部分数量生成订单
            for (size_t i = 0; i < 6; i++)
            {
                if (parts[i] != 0)
                {
                    order.order_quantity = parts[i];
                    order.order_way = 13;
                    order.order_price = is_buy_order ? limit_price_map[order.stock_code][0] : limit_price_map[order.stock_code][1];
                    (is_buy_order ? buy_total_cost : sell_total_cost) += order.order_quantity * order.order_price;
                    (is_buy_order ? buy_list : sell_list)[i].push_back(order);
                    (is_buy_order ? buy_total_cost : sell_total_cost) = (is_buy_order ? buy_total_cost : sell_total_cost) + order.order_price * order.order_quantity;
                }
            }
        }
        infile.close();
    }
    else
    {
        std::cerr << "Cannot open signal file: " << strerror(errno) << std::endl
                  << signal_path << std::endl;
    }
    return order_list;
}

void TradeTwapClass::order_split()
{
    // 买单分单
    split_orders(buy_signal_path, true);
    std::cout << "Buy orders split over..." << std::endl;
    // 卖单分单
    split_orders(sell_signal_path, false);
    std::cout << "Sell orders split over..." << std::endl;

    // 存储分单信号
    for (size_t i = 0; i < sell_list.size(); ++i)
    {
        std::string Sellfilename = twap_orders_dir + "SellOrderList" + std::to_string(i) + "_" + today + ".csv";
        writeOrdersToFile(sell_list[i], Sellfilename);
        std::string Buyfilename = twap_orders_dir + "BuyOrderList" + std::to_string(i) + "_" + today + ".csv";
        writeOrdersToFile(buy_list[i], Buyfilename);
    }
    // 打印分单后订单信息
    std::cout << "*************" << std::endl;
    std::cout << "Code,  market,  direction,  quantity,  price" << std::endl;
    for (size_t i = 0; i < sell_list.size(); ++i)
    {
        std::cout << " twap " << i + 1 << ":" << std::endl;
        for (const OrderReqField &item : sell_list[i])
        {
            printOrderReq(item);
        }
        for (const OrderReqField &item : buy_list[i])
        {
            printOrderReq(item);
        }
        std::cout << "-------------------------" << std::endl;
    }
}

void TradeTwapClass::compare_position()
{
    // 清空差异map
    diff_position.clear();

    // 遍历实际持仓
    for (std::map<std::string, PositionInfo>::iterator it = real_position.begin(); it != real_position.end(); ++it)
    {
        std::string key = it->first;
        PositionInfo value = it->second;
        auto target_it = target_position.find(key);

        // 目标持仓存在
        if (target_it != target_position.end())
        {
            const PositionInfo &target_info = target_it->second;
            // 实际持仓 != 目标持仓
            if (value.real_volume != target_info.target_volume)
            {
                diff_position.push_back({key, std::to_string(value.real_volume), std::to_string(target_info.target_volume), std::to_string(target_info.target_volume - value.real_volume)});
                // 检查是否为卖单，且是否涨停
                if (sell_map.find(key) != sell_map.end())
                {
                    std::cout << "Code '" << key << "' exists in the sell map." << std::endl;
                    // 开盘涨停
                    if (sell_map[key].is_open_limit_up == 1)
                    {
                        diff_position.back().push_back("open_limit_up");
                    }
                    // 盘中涨停
                    else if (sell_map[key].is_middle_limit_up == 1)
                    {
                        diff_position.back().push_back("middle_limit_up");
                    }
                    // 昨日涨停未处理
                    else if (sell_map[key].is_pre_limit_up_change == 1)
                    {
                        diff_position.back().push_back("yesterday_limit_up");
                    }
                    else
                    {
                        diff_position.back().push_back(std::to_string(0));
                    }
                }
                else
                {
                    diff_position.back().push_back(std::to_string(0));
                }
            }
        }
        // 目标持仓不存在
        else
        {
            diff_position.push_back({key, std::to_string(value.real_volume), "0", std::to_string(0 - value.real_volume)});
            // 检查是否为卖单，且是否涨停
            if (sell_map.find(key) != sell_map.end())
            {
                std::cout << "Code '" << key << "' exists in the sell map." << std::endl;
                // 开盘涨停
                if (sell_map[key].is_open_limit_up == 1)
                {
                    diff_position.back().push_back("open_limit_up");
                }
                // 盘中涨停
                else if (sell_map[key].is_middle_limit_up == 1)
                {
                    diff_position.back().push_back("middle_limit_up");
                }
                // 昨日涨停未处理
                else if (sell_map[key].is_pre_limit_up_change == 1)
                {
                    diff_position.back().push_back("yesterday_limit_up");
                }
                else
                {
                    diff_position.back().push_back(std::to_string(0));
                }
            }
            else
            {
                diff_position.back().push_back(std::to_string(0));
            }
        }
    }

    // 遍历目标持仓，找出目标持仓存在但实际不存在的股票
    for (std::map<std::string, PositionInfo>::iterator it = target_position.begin(); it != target_position.end(); ++it)
    {
        std::string key = it->first;
        PositionInfo value = it->second;

        // std::cout << key << "  " << value.target_volume << std::endl;
        if (value.target_volume != 0)
        {
            if (real_position.find(key) == real_position.end())
            {
                diff_position.push_back({key, "0", std::to_string(value.target_volume), std::to_string(value.target_volume - 0)});
                // 检查是否为卖单，且是否涨停
                if (sell_map.find(key) != sell_map.end())
                {
                    std::cout << "Code '" << key << "' exists in the sell map." << std::endl;
                    // 开盘涨停
                    if (sell_map[key].is_open_limit_up == 1)
                    {
                        diff_position.back().push_back("open_limit_up");
                    }
                    // 盘中涨停
                    else if (sell_map[key].is_middle_limit_up == 1)
                    {
                        diff_position.back().push_back("middle_limit_up");
                    }
                    // 昨日涨停未处理
                    else if (sell_map[key].is_pre_limit_up_change == 1)
                    {
                        diff_position.back().push_back("yesterday_limit_up");
                    }
                    else
                    {
                        diff_position.back().push_back(std::to_string(0));
                    }
                }
                else
                {
                    diff_position.back().push_back(std::to_string(0));
                }
            }
        }
    }
}

void TradeTwapClass::compare_position_before_trade()
{
    last_position_quantity = 0;
    query_position();
    sleep(1);
    while (true)
    {
        if (total_position_quantity == last_position_quantity + 1 || last_position_quantity == -1)
            break;
        query_position(last_position_quantity);
        sleep(1);
    }
    sleep(wait_seconds);
    std::cout << real_position.size() << " position records have been found " << std::endl;
    std::cout << std::endl
              << std::endl;
    std::cout << "==============================" << std::endl;
    std::string yes_holding_path = yes_target_file_path;
    get_target_holding_map(yes_holding_path);
    std::cout << "get_yes_target_holding end....... " << std::endl;
    compare_position();
    std::cout << "compare_position end....... " << std::endl;
    if (!diff_position.empty())
    {
        print_diff_position_Info(diff_position);

        // 打印最新实际持仓
        std::cout << "----------- real_position --------" << std::endl;
        std::cout << "   code  " << "target  " << "real" << std::endl;
        int i = 1;
        for (const auto &pair : real_position)
        {
            std::string code = pair.first;
            PositionInfo info = pair.second;
            std::cout << std::setw(5) << std::left << i;
            print_position_Info(info);
            i++;
        }

        // 打印最新目标持仓
        std::cout << "-------- target_position -----------" << std::endl;
        std::cout << "   code  " << "target  " << "real" << std::endl;
        i = 1;
        for (const auto &pair : target_position)
        {
            std::string code = pair.first;
            PositionInfo info = pair.second;
            std::cout << std::setw(5) << std::left << i;
            print_position_Info(info);
            i++;
        }

        char choice;

        std::cout << "Whether order the diff? (y/n): ";
        std::cin >> choice;
        choice = tolower(choice);
        while (choice != 'y' && choice != 'n')
        {
            std::cout << "Input error, please re-enter? (y/n): ";
            std::cin >> choice;
            choice = tolower(choice);
        }
        if (choice == 'y')
        {
            std::vector<struct OrderReqField> diff_order = get_diff_order();

            for (OrderReqField &it : diff_order)
            {
                // printOrderReq(it);
                order_insert(it);
            }
            sleep(3);
        }
    }
    else
    {
        std::cout << "==========target_position = real_position============" << std::endl
                  << std::endl;
    }
}

void TradeTwapClass::print_diff_position_Info(std::vector<std::vector<std::string>> info)
{
    const int col_width = 10;  // 列宽设置为12字符宽
    const int index_width = 4; // 行号宽度设置为3字符宽
    std::cout << "-------------------" << std::endl;
    std::cout << "    "
              << std::setw(col_width) << std::left << "code"
              << std::setw(col_width) << std::left << "real"
              << std::setw(col_width) << std::left << "target"
              << std::setw(col_width) << std::left << "diff"
              << std::setw(col_width) << std::left << "sell_flag"
              << std::endl;
    int i = 1;
    for (const auto &row : info)
    {
        std::cout << std::setw(index_width) << std::left << i;
        for (const auto &item : row)
        {
            std::cout << std::setw(col_width) << std::left << item;
        }
        std::cout << std::endl;
        i++;
    }
}

void print_position_Info(const PositionInfo &info)
{
    std::cout << std::setw(10) << std::left << info.StockCode;
    std::cout << std::setw(10) << std::left << info.target_volume;
    std::cout << info.real_volume << std::endl;
}

std::string TradeTwapClass::get_today_value()
{
}

void TradeTwapClass::StartTwapTrade()
{
    // 1.检测交易信号是否到达
    std::cout << "1.check if the trade signals exist......" << std::endl;
    today_target_file_path = get_target_holding_file_path(fundname, order_dir, today);
    std::vector<std::string> signal_paths = {
        buy_signal_path,
        sell_signal_path,
        today_target_file_path};

    struct stat buffer;
    for (const auto &signal_path : signal_paths)
    {
        if (stat(signal_path.c_str(), &buffer) == -1)
            std::cout << "File " << signal_path << " is not exists, wait for their arrival !! " << std::endl;
        while (stat(signal_path.c_str(), &buffer) == -1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        // std::cout << "file " << signal_path << " has be found" << std::endl;
    }
    // 2.订单分单
    std::cout << "2.start split the orders......" << std::endl;
    order_split();
    // 3.查询可用资金是否充足
    std::cout << "3.check if the available money is sufficient......" << std::endl;

    if (query_asset.can_use_money + sell_total_cost - buy_total_cost > 40000)
    {
        std::cout << "\033[32m available money is sufficient\033[0m" << std::endl;
    }
    else
    {
        std::cout << "\033[31m !!!!!available money is not sufficient!!!!!!(press enter to continue):\033[0m";
        std::cin.get();
        std::cin.get();
    }

    // 等待交易时间,判断是否为当日首次交易
    if (isBefore(9, 30, 0))
    {
        std::cout << "  now time is:" << get_now_time() << std::endl;
        trade_flag = 0;
        set_trade_flag(0);

        // 4.盘前持仓对比 昨日最新目标持仓对比今日盘前持仓
        std::cout << "4.compare the position before trade......" << std::endl;
        compare_position_before_trade();

        // 等待交易时间开启
        if (isBefore(9, 30, 0))
            std::cout << "Wait for trade time to begin..." << std::endl;
        while (isBefore(9, 30, 0))
        {
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
    else
    {
        char first_trade_flag;
        std::cout << std::endl;
        std::cout << "\033[31mWhether the first time to start trade?(y/n) :\033[0m";
        std::cin >> first_trade_flag;
        if (first_trade_flag == 'Y' || first_trade_flag == 'y')
        {
            // 4.盘前持仓对比 昨日最新目标持仓对比今日盘前持仓
            std::cout << "4.compare the position before trade......" << std::endl;
            compare_position_before_trade();
            trade_flag = 0;
            set_trade_flag(0);
        }
        else
        {
            get_trade_flag();
            std::vector<OrderReqField> orderArray = get_orderRecode_fromCSV();
            get_target_holding_map(yes_target_file_path);
            update_target_holding_map(orderArray);
        }
    }
    std::cout << std::endl;
    std::cout << "5.Now trade is begin..."
              << std::endl;
    std::cout << "=====================================" << std::endl;

    // 下单
    std::vector<std::vector<struct OrderReqField>> all_signal_array;

    while (trade_flag < 10)
    {
        sleep(1);
        std::string temp_trade_time = trade_time_map[twap_name_vec[trade_flag]];
        // std::cout << temp_trade_time << std::endl;
        int hour = stringToInt(temp_trade_time.substr(0, 2));
        int minute = stringToInt(temp_trade_time.substr(2, 2));
        int second = stringToInt(temp_trade_time.substr(4, 2));
        std::cout << "  now time is:" << get_now_time() << std::endl;
        std::cout << std::endl;
        std::cout << "======= Waiting to trade  " << twap_name_vec[trade_flag] << ", trade time: " << temp_trade_time << " ......" << std::endl;
        while (isBefore(hour, minute, second))
        {
            // 等待单笔交易时间到达
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        std::cout << "  now time is:" << get_now_time() << std::endl;
        std::cout << std::endl;
        std::cout << "=======trade_flag = " << trade_flag << " , " << twap_name_vec[trade_flag] << " trade begin......" << std::endl
                  << std::endl;
        bool real_signal = false;
        // morning 实时信号
        std::vector<struct OrderReqField> sell_buy_array;
        if (trade_flag == 2)
        {
            std::string morning_sell_file_path = order_dir + "SellOrderList" + fundname + "morning_" + today + ".csv";
            std::string no_signal_path = order_dir + "NoSellingMorning_" + today + ".txt";

            if (isExistFile(morning_sell_file_path) == false && isExistFile(no_signal_path) == false)
                std::cout << "***morning signal not arrive..." << std::endl;
            while (isExistFile(morning_sell_file_path) == false && isExistFile(no_signal_path) == false)
            {
                sleep(5);
            }
            if (isExistFile(no_signal_path))
            {
                std::cout << " no morning signal......" << std::endl;
            }
            else
            {
                real_signal = true;
                sell_buy_array.clear();
                std::cout << " now ordering morning selling signal  ";
                sell_buy_array = read_real_time_signal(morning_sell_file_path, true);
                // std::cout << "Code,  market,  direction,  quantity,  price" << std::endl;
                std::cout << " orders quantity:" << sell_buy_array.size() << std::endl;
                for (OrderReqField &it : sell_buy_array)
                {
                    // printOrderReq(it);
                    order_insert(it);
                }
            }
        }
        // morning2Two实时信号
        else if (trade_flag == 4)
        {
            std::string morning2_sell_file_path = order_dir + "SellOrderList" + fundname + "morning2Two_" + today + ".csv";
            std::string no_signal_path = order_dir + "NoSellingMorning2Two_" + today + ".txt";
            std::string no_signal_path2 = order_dir + "NoSellingMorning2two_" + today + ".txt";
            if (isExistFile(morning2_sell_file_path) == false && isExistFile(no_signal_path) == false && isExistFile(no_signal_path2) == false)
                std::cout << "***morning2Two signal not arrive..." << std::endl;
            while (isExistFile(morning2_sell_file_path) == false && isExistFile(no_signal_path) == false && isExistFile(no_signal_path2) == false)
            {
                sleep(5);
            }
            if (isExistFile(no_signal_path) || isExistFile(no_signal_path2))
            {
                std::cout << " no morning2Two signal......" << std::endl;
            }
            else
            {
                real_signal = true;
                sell_buy_array.clear();
                std::cout << " now ordering morning2Two selling signal  ";
                sell_buy_array = read_real_time_signal(morning2_sell_file_path, true);
                // std::cout << "Code,  market,  direction,  quantity,  price" << std::endl;
                std::cout << " orders quantity:" << sell_buy_array.size() << std::endl;
                for (OrderReqField &it : sell_buy_array)
                {
                    // printOrderReq(it);
                    order_insert(it);
                }
            }
        }
        // afternoon实时信号
        else if (trade_flag == 6)
        {
            std::string afternoon_sell_file_path = order_dir + "SellOrderList" + fundname + "afternoon_" + today + ".csv";
            std::string no_signal_path = order_dir + "NoSellingAfternoon_" + today + ".txt";
            if (isExistFile(afternoon_sell_file_path) == false && isExistFile(no_signal_path) == false)
                std::cout << "***afternoon signal not arrive..." << std::endl;
            while (isExistFile(afternoon_sell_file_path) == false && isExistFile(no_signal_path) == false)
            {
                sleep(5);
            }

            if (isExistFile(no_signal_path))
            {
                std::cout << " no afternoon signal......" << std::endl;
            }
            else
            {
                real_signal = true;
                sell_buy_array.clear();
                std::cout << " now ordering afternoon selling signal  ";
                sell_buy_array = read_real_time_signal(afternoon_sell_file_path, true);
                // std::cout << "Code,  market,  direction,  quantity,  price" << std::endl;
                std::cout << " orders quantity:" << sell_buy_array.size() << std::endl;
                for (OrderReqField &it : sell_buy_array)
                {
                    // printOrderReq(it);
                    order_insert(it);
                }
            }
        }
        // afternoon2Two实时信号
        else if (trade_flag == 9)
        {
            std::string afternoon2_buy_file_path = order_dir + "BuyOrderList" + fundname + "afternoon2Two_" + today + ".csv";
            std::string no_signal_path = order_dir + "NoBuyingAfternoon2Two_" + today + ".txt";

            if (isExistFile(afternoon2_buy_file_path) == false && isExistFile(no_signal_path) == false)
                std::cout << "***afternoon2Two signal not arrive..." << std::endl;
            while (isExistFile(afternoon2_buy_file_path) == false && isExistFile(no_signal_path) == false)
            {
                sleep(5);
            }

            if (isExistFile(no_signal_path))
            {
                std::cout << " no afternoon2Two signal......" << std::endl;
            }
            else
            {
                real_signal = true;
                sell_buy_array.clear();
                std::cout << " now ordering afternoon2Two buying signal  ";
                sell_buy_array = read_real_time_signal(afternoon2_buy_file_path, false);
                // std::cout << "Code,  market,  direction,  quantity,  price" << std::endl;
                std::cout << " orders quantity:" << sell_buy_array.size() << std::endl;
                for (OrderReqField &it : sell_buy_array)
                {
                    // printOrderReq(it);
                    order_insert(it);
                }
            }
        }
        // 分单信号
        else
        {
            int twap = stringToInt(twap_name_vec[trade_flag].substr(4, 1)) - 1;

            real_signal = true;
            sell_buy_array.clear();

            std::vector<struct OrderReqField> buy_signal = buy_list[twap];
            std::vector<struct OrderReqField> sell_aignal = sell_list[twap];

            std::cout << " orders quantity: " << sell_aignal.size() << " , sell orders......" << std::endl;
            // std::cout << "Code,  market,  direction,  quantity,  price" << std::endl;
            for (OrderReqField info : sell_aignal)
            {
                if (info.order_quantity == 0)
                {
                    continue;
                }
                sell_buy_array.push_back(info);
                // printOrderReq(info);
                order_insert(info);
            }
            std::cout << " orders quantity: " << buy_signal.size() << " , buy orders......" << std::endl;
            // std::cout << "Code,  market,  direction,  quantity,  price" << std::endl;
            for (OrderReqField info : buy_signal)
            {
                if (info.order_quantity == 0)
                {
                    continue;
                }
                sell_buy_array.push_back(info);
                // printOrderReq(info);
                order_insert(info);
            }
        }

        // 每次下单后持仓对比  昨日目标持仓+已报订单  ？=  现在实际持仓
        if (real_signal == true)
        {
            std::cout << std::endl;
            std::cout << "wait for 5 seconds to compare......" << std::endl;
            sleep(5);
            last_position_quantity = 0;
            query_position();
            sleep(1);
            while (true)
            {
                if (total_position_quantity == last_position_quantity + 1 || last_position_quantity == -1)
                    break;
                query_position(last_position_quantity);
                sleep(1);
            }
            sleep(wait_seconds);
            std::cout << real_position.size() << " position records have been found " << std::endl;
            std::cout << std::endl
                      << std::endl;
            update_target_holding_map(sell_buy_array);
            update_target_holding_record_csv(sell_buy_array);

            compare_position();

            if (!diff_position.empty())
            {
                print_diff_position_Info(diff_position);
                std::cout << std::endl;
                char choice;

                std::cout << "Whether order the diff? (y/n): ";
                std::cin >> choice;
                choice = tolower(choice);
                while (choice != 'y' && choice != 'n')
                {
                    std::cout << "Input error, please re-enter? (y/n): ";
                    std::cin >> choice;
                    choice = tolower(choice);
                }
                if (choice == 'y')
                {
                    std::vector<struct OrderReqField> diff_order = get_diff_order();

                    for (OrderReqField &it : diff_order)
                    {
                        // printOrderReq(it);
                        order_insert(it);
                    }
                }
            }
            else
            {
                std::cout << "     =====target_position = real_position=====" << std::endl;
            }
        }

        std::cout << "=======trade_flag = " << trade_flag << " , trade end......" << std::endl;

        // 更新trade_flag
        trade_flag++;
        set_trade_flag(trade_flag);
    }

    // 交易结束，进行盘后持仓对比    最新目标持仓文件 ？=  实际持仓
    std::cout << std::endl
              << std::endl;
    std::cout << "--------------last position compare--------------" << std::endl;
    while (true)
    {
        std::string newest_target_holding_path = get_target_holding_file_path(fundname, order_dir, today); // 获取最新目标持仓路径
        get_target_holding_map(newest_target_holding_path);
        std::cout << target_position.size() << "  target holding have been found " << std::endl;
        last_position_quantity = 0;
        query_position();
        sleep(1);
        while (true)
        {
            if (total_position_quantity == last_position_quantity + 1 || last_position_quantity == -1)
                break;
            query_position(last_position_quantity);
            sleep(1);
        }
        sleep(3); // 获取最新目标持仓map
        std::cout << real_position.size() << " position records have been found " << std::endl;
        std::cout << std::endl
                  << std::endl;
        compare_position();
        if (diff_position.size() != 0)
        {
            print_diff_position_Info(diff_position);

            char choice;

            std::cout << "Whether order the diff? (y/n): ";
            std::cin >> choice;
            choice = tolower(choice);
            while (choice != 'y' && choice != 'n')
            {
                std::cout << "Input error, please re-input? (y/n): ";
                std::cin >> choice;
                choice = tolower(choice);
            }
            if (choice == 'y')
            {
                std::vector<struct OrderReqField> diff_order = get_diff_order();
                // std::cout << "Code,  market,  direction,  quantity,  price" << std::endl;
                for (OrderReqField &it : diff_order)
                {
                    // printOrderReq(it);
                    order_insert(it);
                }
            }
            else
            {
                char choice2;
                std::cout << " Whether to again compare the position? (y/n): ";
                std::cin >> choice2;
                choice2 = tolower(choice2);
                while (choice2 != 'y' && choice2 != 'n')
                {
                    std::cout << "Input error, please re-input? (y/n): ";
                    std::cin >> choice2;
                    choice2 = tolower(choice2);
                }
                if (choice2 == 'y')
                {
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            std::cout << "     === target_position = real_position ===" << std::endl;
            break;
        }
    }

    std::cout << "=== Trades done, waiting for reverse repo... ===" << std::endl;
    // 交易程序结束, 等待15:00 退出程序
    while (isBefore(15, 0, 0))
    {
        sleep(5);
    }
    is_quit = 1;
}
