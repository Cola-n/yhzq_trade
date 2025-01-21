
#ifndef TRADE_TOOL_H
#define TRADE_TOOL_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <unistd.h>

#include "TradeDataType.h"
#include "common_tool.h"


// std::map<std::string, std::string> get_target_holding_map(std::string holdingPath);
std::string get_target_holding_file_path(std::string fundname, std::string order_dir, std::string date);
std::string get_now_time();


// twap分单相关
//分单信号交易数量重排序
std::vector<int> rearrange(const std::vector<int>& parts);


#endif
