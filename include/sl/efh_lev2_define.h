/*!****************************************************************************
@node        Copyright (coffee), 2005-2020, Shengli Tech. Co., Ltd.
@file        efh_lev2_define.h
@date        2020/12/14 08:30
@author      shengli

@brief       本接口定义接收需要的公共数据接口
******************************************************************************/
#pragma once
#ifdef WINDOWS
#define DLL_EFH_LEV2_DLL_NAME "efh_lev2_api.dll" // api动态库名
#else
#define DLL_EFH_LEV2_DLL_NAME "libsl_efh_lev2_api.so" // api动态库名
#endif
#define EFH_API_VERSION "V2.3.9"                  // api版本号
#define SSE_LEV2_PROTOCOL_VERSION "EFH_SSE_4.2.5" // 上交所协议版本号
#define SZE_LEV2_PROTOCOL_VERSION "EFH_SZE_3.2.6" // 深交所协议版本号
#define DEFAULT_CACHE_SIZE (512 * 1024 * 1024)    // 缓存默认大小512M
#define IP_LEN (64)                               // IP字符串地址长度定义
#define ETH_NAME_LEN (64)                         // Eth字符串长度定义
#define VERSION_LEN (64)                          // 版本号字符串长度
#define NET_CHANNEL_NUM (4)
#define UNIVERSAL_SYMBOL_LEN (16)
#define AUTHORIZE_STR_LEN (128)

// 行情类型
enum enum_efh_lev2_type
{
    enum_efh_sze_lev2_snap = 1,        // 深交所lev2快照
    enum_efh_sze_lev2_tick = 2,        // 深交所逐笔信息
    enum_efh_sze_lev2_idx = 3,         // 深交所指数行情
    enum_efh_sze_lev2_tree = 4,        // 深交所lev2建树
    enum_efh_sze_lev2_after_close = 5, // 深交所盘后定价快照
    enum_efh_sze_lev2_ibr_tree = 6,    // 深交所ibr建树
    enum_efh_sze_lev2_turnover = 7,    // 深交所成交量统计快照

    enum_efh_sse_lev2_snap = 8,         // 上交所lev2快照
    enum_efh_sse_lev2_idx = 9,          // 上交所指数行情
    enum_efh_sse_lev2_tick = 10,        // 上交所逐笔信息
    enum_efh_sse_lev2_opt = 11,         // 上交所股票期权
    enum_efh_sse_lev2_tree = 12,        // 上交所建树快照
    enum_efh_sse_lev2_bond_snap = 13,   // 上交所债券快照
    enum_efh_sse_lev2_bond_tick = 14,   // 上交所债券逐笔信息
    enum_efh_sse_lev2_static_info = 15, // 上交所静态信息

    enum_efh_sze_lev2_bond_snap = 16, // 深交所债券 level-2 快照
    enum_efh_sze_lev2_bond_tick = 17, // 深交所债券逐笔信息

    enum_efh_sse_lev2_tick_merge = 18, // 上交所逐笔合并消息
    enum_efh_sse_lev2_etf = 19,        // 上交所 ETF 统计消息
};

// 接收方式
enum enum_efh_nic_type
{
    enum_nic_normal,               // 默认系统接收,操作系统协议栈
    enum_nic_solarflare_efvi,      // solarflare efvi接收
    enum_nic_exablaze_exanic,      // Exablaze exanic接收
    enum_nic_x710_win_speed,       // win高速接收，只适用于windows
    enum_nic_solarflare_win_speed, // win高速接收，目前只适用于windows
};

#pragma pack(push, 1)

// 网络接收配置信息
struct network_info_config
{
    enum_efh_nic_type m_nic_type;     // 接收模式
    char m_ch_dest_ip[IP_LEN];        // 目标ip地址
    unsigned short m_i_dest_port;     // 目标端口号
    char m_ch_src_ip[IP_LEN];         // 源ip地址
    unsigned short m_i_src_port;      // 源端口号
    char m_ch_eth_name[ETH_NAME_LEN]; // 网卡名
};

// 通道配置
struct efh_channel_config
{
    enum_efh_lev2_type m_efh_type;                       // 通道行情类型
    int m_i_cpu_id;                                      // 用于接收的cpu序号,当cpu赋值为-1时，则不绑定
    bool m_b_polling;                                    // 数据接收线程是否忙等待 true：等待 false：不等待，现在默认为false，功能未实现
    bool m_b_out_of_order_correction;                    // 乱序纠错功能开关, 默认为 false
    int m_i_channel_num;                                 // 行情源数量, 指示 m_channel_info 数组可用的行情源信息
    network_info_config m_channel_info[NET_CHANNEL_NUM]; // 行情源网络接收必要信息的配置
    unsigned long long m_ll_cache_size;                  // 设置缓存大小,设置为 0 表示不使用缓存,单位为字节
    unsigned long long m_ll_proc_data_wait_time;         // 缓存队列空闲时, 轮询间隔, 单位 us, 默认为 10 us
    unsigned long long m_ll_normal_socket_rxbuf;         // 设置 socket 模式 SO_RCVBUF 尺寸, 单位为字节, 默认为 10 Mb
};

// 版本
struct efh_version
{
    char m_ch_protocol_version[VERSION_LEN]; // 协议版本号
    char m_ch_api_version[VERSION_LEN];      // api版本号
};

// 合约股票标的结构, 主要用于合约订阅功能传参
struct symbol_item
{
    char symbol[UNIVERSAL_SYMBOL_LEN];
};

/// ats 登录必要的配置信息
struct exchange_authorize_config
{
    char m_ch_server_ip[IP_LEN];         // ats 服务IP地址
    unsigned short m_i_server_port;      // ats 服务端口号
    char m_ch_local_ip[IP_LEN];          // 本地网卡绑定 ip 地址
    unsigned short m_i_local_port;       // 本地网卡绑定端口号
    long long m_user_id;                 // 用户 ID
    char m_sys[AUTHORIZE_STR_LEN];       // 转发系统名称
    char m_machine[AUTHORIZE_STR_LEN];   // 转发机房名称
    char m_full_name[AUTHORIZE_STR_LEN]; // 转发用户全称
    char m_user_pwd[AUTHORIZE_STR_LEN];  // 用户密码
};

///  通道身份标识
struct session_identity
{
    int id;
};

#pragma pack(pop)
