#ifndef NETWORK_FSM_H
#define NETWORK_FSM_H

#include <stdint.h>
#include <stdbool.h>

// 定义状态枚举类型
typedef enum {
    STATE_STARTUP,                   // 节点上电，启动 STA 模式
    STATE_SCANNING,                  // 扫描 Mesh 网络
    STATE_JOIN_EXISTING_NETWORK,     // 加入已有 Mesh 网络
    STATE_OPEN_AP,                   // 开启 AP 模式
    STATE_CHECK_ROOT_CONFLICT,       // 检查根节点冲突
    STATE_CREATE_ROOT,               // 创建根节点网络
    STATE_TERMINATE                  // 结束状态（完成所有操作）
} NetworkState;

// 定义 Mesh 网络配置信息的结构体
typedef struct {
    char mesh_ssid[16];           // Mesh 自定义 SSID，16 字节 + 1 结束符
    uint8_t root_mac[6];          // 根节点的 MAC 地址
    char password[65];            // Mesh 密码，最长 64 字节 + 1 结束符
    uint8_t tree_level;           // 父节点的树层级
} MeshNetworkConfig;

typedef struct {
    char ssid[33];          // 节点的 SSID
    uint8_t bssid[6];       // 节点的 BSSID (MAC 地址)
    int16_t rssi;           // 节点的信号强度 (RSSI)
    uint8_t channel;        // 节点所在的信道
    int tree_level;         // 节点的树层级（0 表示根节点）
} MeshNode;

// 定义状态机初始化函数
int network_fsm_init(const MeshNetworkConfig *config);

// 状态处理主循环函数，开始状态机流程
void network_fsm_run(void);

// 各状态处理函数的声明
NetworkState state_startup(void);                // 启动状态处理函数
NetworkState state_scanning(void);               // 扫描状态处理函数
NetworkState state_join_existing_network(void);  // 加入已有网络状态处理函数
NetworkState state_open_ap(void);                // 开启AP热点
NetworkState state_check_root_conflict(void);    // 检查根节点冲突状态处理函数
NetworkState state_create_root(void);            // 创建根节点状态处理函数
NetworkState state_terminate(void);              // 结束状态处理函数

// 网络连接状态
int network_connected(void);

#endif // NETWORK_FSM_H
