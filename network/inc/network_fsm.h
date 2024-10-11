#ifndef NETWORK_FSM_H
#define NETWORK_FSM_H

#include <stdint.h>
#include <stdbool.h>

// 定义状态枚举类型
typedef enum {
    STATE_STARTUP,                   // 节点上电，启动 STA 模式
    STATE_SCANNING,                  // 扫描 Mesh 网络
    STATE_JOIN_EXISTING_NETWORK,     // 加入已有 Mesh 网络
    STATE_CHECK_ROOT_COUNT,          // 检查根节点个数
    STATE_JOIN_NETWORK,              // 选择网络加入策略
    STATE_CONNECTED,                 // 成功接入 Mesh 网络
    STATE_CREATE_ROOT,               // 创建根节点网络
    STATE_CHECK_ROOT_CONFLICT,       // 检查根节点冲突
    STATE_HANDLE_ROOT_CONFLICT,      // 处理根节点冲突
    STATE_ROOT_ELECTION,             // 执行根节点选举算法
    STATE_TERMINATE                  // 结束状态（完成所有操作）
} NetworkState;

// 定义状态机初始化函数
void network_fsm_init(void);

// 状态处理主循环函数，开始状态机流程
void network_fsm_run(void);

// 各状态处理函数的声明
NetworkState state_startup(void);                // 启动状态处理函数
NetworkState state_scanning(void);               // 扫描状态处理函数
NetworkState state_join_existing_network(void);  // 加入已有网络状态处理函数
NetworkState state_check_root_count(void);       // 检查根节点个数状态处理函数
NetworkState state_join_network(void);           // 加入网络状态处理函数
NetworkState state_connected(void);              // 成功连接状态处理函数
NetworkState state_create_root(void);            // 创建根节点状态处理函数
NetworkState state_check_root_conflict(void);    // 检查根节点冲突状态处理函数
NetworkState state_handle_root_conflict(void);   // 处理根节点冲突状态处理函数
NetworkState state_root_election(void);          // 根节点选举状态处理函数

#endif // NETWORK_FSM_H
