/*!****************************************************************************
@node		Copyright (coffee), 2005-2020, Shengli Tech. Co., Ltd.
@file		i_efh_sze_lev2_api.h
@date		2020/12/14 08:30
@author		shengli

@brief		本接口实现深圳证券交易所盛立行情接收，支持缓存输出，支持三类网卡
                        init_sze失败后需调用close_sze，不得直接调用start_sze
******************************************************************************/
#pragma once
#include "efh_lev2_define.h"
#include "sze_hpf_define.h"

class efh_sze_lev2_api_event
{
public:
    virtual ~efh_sze_lev2_api_event() {}
    /*
    @brief 上报深交所盘后定价数据
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param p_after_close: 盘后定价结构
    */
    virtual void on_report_efh_sze_lev2_after_close(session_identity id, sze_hpf_after_close *p_after_close) = 0;
    /*
    @brief 上报lev2快照数据
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param p_snap: lev2快照结构
    */
    virtual void on_report_efh_sze_lev2_snap(session_identity id, sze_hpf_lev2 *p_snap) = 0;
    /*
    @brief 上报逐笔订单数据
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param msg_type: 消息类型
    @param p_order: 订单结构
    @param p_exe:	成交类型
    */
    virtual void on_report_efh_sze_lev2_tick(session_identity id, int msg_type, sze_hpf_order *p_order, sze_hpf_exe *p_exe) = 0;
    /*
    @brief 上报指数数据
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param p_idx:	指数数据结构
    */
    virtual void on_report_efh_sze_lev2_idx(session_identity id, sze_hpf_idx *p_idx) = 0;
    /*
    @brief 上报建树快照数据
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param p_tree:	建树快照结构
    */
    virtual void on_report_efh_sze_lev2_tree(session_identity id, sze_hpf_tree *p_tree) = 0;
    /*
    @brief 上报ibr建树快照数据
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param p_ibr_tree:	ibr建树快照结构
    */
    virtual void on_report_efh_sze_lev2_ibr_tree(session_identity id, sze_hpf_ibr_tree *p_ibr_tree) = 0;

    /*
    @brief 上报成交量统计快照数据
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param p_turnover:	成交量统计快照结构
    */
    virtual void on_report_efh_sze_lev2_turnover(session_identity id, sze_hpf_turnover *p_turnover) = 0;

    /*
    @brief 上报债券快照消息类型
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param p_bond_snap:	债券快照结构
    */
    virtual void on_report_efh_sze_lev2_bond_snap(session_identity id, sze_hpf_bond_snap *p_bond_snap) = 0;

    /*
    @brief 上报债券逐笔订单数据
    @param id:  id 通道信息, 用于指示当前回调的行情数据的行情源, 其成员变量 id.id 与 efh_channel_config.network_info_config 数组的下标一一对应, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param msg_type: 消息类型
    @param p_order: 债券订单结构
    @param p_exe:	债券成交类型
    */
    virtual void on_report_efh_sze_lev2_bond_tick(session_identity id, int msg_type, sze_hpf_bond_order *p_order, sze_hpf_bond_exe *p_exe) = 0;
};

class efh_sze_lev2_api_depend
{
public:
    virtual ~efh_sze_lev2_api_depend() {}
    /// 记录日志
    /// 上报调试信息
    virtual void efh_sze_lev2_debug(const char *msg, int len) {}
    /// 上报错误
    virtual void efh_sze_lev2_error(const char *msg, int len) {}
    /// 上报相关信息
    virtual void efh_sze_lev2_info(const char *msg, int len) {}
};

class i_efh_sze_lev2_api
{
public:
    virtual ~i_efh_sze_lev2_api(void) {}

    /*
    @brief 获取版本信息
    @param version:版本信息结构, 版本信息将被更新到此变量中
    */
    virtual void get_version(efh_version &version) = 0;
    /*
    @brief 初始化接口
    @param p_event:数据上报指针
    @param p_depend：日志指针
    @return 初始化操作是否成功, 成功: true , 失败: false
    */
    virtual bool init_sze(efh_sze_lev2_api_event *p_event, efh_sze_lev2_api_depend *p_depend = NULL) = 0;
    /*
    @brief 设置通道接收配置
    set_channel_config 相关函数应当在  start 前调用, 具体配置将以start 前最后一次调用为准
    @param p_recv_param: 通道配置数组
    @param num: 配置数组个数
    @return: 设置操作是否成功, 成功: true , 失败: false
    */
    virtual bool set_channel_config(efh_channel_config *p_recv_param, int num) = 0;
    /*
    @brief 使用 ats 服务设置通道接收配置
    ats 只提供自动配置 组播 ip 和组播端口, 所以需要提前配置网卡接收模式, 本地ip 本地端口和本地网卡名
    @param p_recv_param: 通道配置数组
    @param num: 配置数组个数
    @param is_keep_connection: 是否启用长链接
    @return: 设置操作, ats链接登录验证, 是否成功, 成功: true , 失败: false
    */
    virtual bool set_channel_config_with_ats(efh_channel_config *p_recv_param, int num, exchange_authorize_config ats, bool is_keep_connection) = 0;
    /*
    @brief 设置行情源自动切换功能
    @param b_switch: 使能开关, 默认不启用
    @param ll_timeout:  行情源自动切换功能异常检测超时, 单位秒, 推荐值范围 15秒-300秒
    @return: 设置操作是否成功, 成功: true , 失败: false
    */
    virtual bool set_auto_change_source_config(bool b_switch, unsigned long long ll_timeout) = 0;
    /*
    @brief 启动接口
    @return 是否成功, 成功: true , 失败: false
    */
    virtual bool start_sze() = 0;
    /*
    @brief 停止接口
    */
    virtual void stop_sze() = 0;
    /*
    @brief 关闭接口,释放资源
    */
    virtual void close_sze() = 0;
    /*
    @brief 设置合约订阅使能开关
    @note 合约订阅功能不启用时,对主路径接收性能无负面影响, 启用时, 存在轻微的性能开销, 理论上会增加穿越时延, 实验室测试确认开销不明显,几乎无影响
    @param flag: 使能开关标志, 默认不启用
    */
    virtual void set_symbol_filter_enable_switch(bool flag) = 0;
    /*
    @brief 重新设置合约订阅列表, 全量覆盖
    @param symbol_array: 合约数组
    @param num: 合约数组长度
    */
    virtual void reset_symbol_filter_list(symbol_item *symbol_array, int num) = 0;
    /*
    @brief 取消订阅全部合约列表, 清理合约订阅列表
    */
    virtual void unsubscribe_all_symbol_filter_list() = 0;
    /*
    @brief 获取合约订阅列表和使能开关
    @param symbol_array: 合约数组
    @param p_num: 合约数组长度指针, 指示合约数组可用大小
    @param p_enable_sw: 返回指示订阅开关
    @return 合约订阅列表总长度
    */
    virtual int get_symbol_filter_list_and_enable_switch(symbol_item *symbol_array, int *p_num, bool *p_enable_sw) = 0;
    /*
    @brief 批量新增合约订阅
    @param symbol_array: 合约数组
    @param num: 合约数组长度
    */
    virtual void subscribe_symbol_filter_items(symbol_item *symbol_array, int num) = 0;
    /*
    @brief 批量取消订阅合约订阅
    @param symbol_array: 合约数组
    @param num: 合约数组长度
    */
    virtual void unsubscribe_symbol_filter_items(symbol_item *symbol_array, int num) = 0;
    /*
    @brief ODP 行情重建接口
    @param ip: ODP 重建 server IP 地址
    @param port: ODP 重建 server 端口
    @param p_event: ODP 重建行情上报接口, 特别的, 对于从 ODP重建返回的行情, id.id 将被标记为 100
    @param channel_num: 频道代码
        201x: 股票
        202x: 基金
        203x: 可转债
        204x: 权证
        205x: 期权
        206x: 债券质押式回购
        207x: 债券现券匹配成交逐笔行情
    @param begin_seq: 开始序号
    @param end_seq: 结束序号，重建最大范围为 1000 条
    */
    virtual bool odp_rebuild_quote(
        char *ip,
        int port,
        efh_sze_lev2_api_event *p_event,
        int channel_num,
        long long begin_seq,
        long long end_seq) = 0;
};

#define CREATE_EFH_SZE_LEV2_API_FUNCTION "create_efh_sze_lev2_api_function"
#define DESTROY_EFH_SZE_LEV2_API_FUNCTION "destroy_efh_sze_lev2_api_function"

typedef i_efh_sze_lev2_api *(*func_create_efh_sze_lev2_api)();
typedef void (*func_destroy_efh_sze_lev2_api)(i_efh_sze_lev2_api *);
