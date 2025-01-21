#ifndef MEM_MAP_H
#define MEM_MAP_H

#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <cmath>

struct market_data
{    
    char code[7] = {0};

    double pre_close_price = 0;
    double upper_limit_price = 0;
    double low_limit_price = 0;
    double open_price = 0;
    double last_price = 0;
    double buy_price[10] = {0};
    int buy_vol[10] = {0};
    double sell_price[10] = {0};
    int sell_vol[10] = {0};

    double trade_price = 0;

    bool is_get_snap_data = false;
    bool is_get_trade_data = false;
};

void sys_err(char *str);
void get_today(char *today);
double get_upper_limit_price(std::string code, double pre_close_price);
double get_low_limit_price(std::string code, double pre_close_price);
int init_data_map(char *codepath, std::map<std::string, market_data> &data_map);
void memory_map(char *datapath, char *&mm, std::map<std::string, market_data> &data_map);
void update_data_map(char *&mm, std::map<std::string, market_data> &data_map);
void save_data_map(char *&mm, std::map<std::string, market_data> data_map);
void random_change_data_map(std::map<std::string, market_data> &data_map);
void read_data_map(char *&mm, std::map<std::string, market_data> &data_map);
void free_data_map(char *&mm, std::map<std::string, market_data> &data_map);

#endif
