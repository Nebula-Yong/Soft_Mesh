#include "network_fsm.h"
#include <stdio.h>
#include <stdbool.h>
#include "hal_wireless.h"       // 调用hal层接口
#include <stdlib.h>             // 引入动态内存分配
#include <string.h>
#include <ctype.h>

// 定义宏开关，打开或关闭日志输出
#define ENABLE_LOG 1  // 1 表示开启日志，0 表示关闭日志

// 定义 LOG 宏，如果 ENABLE_LOG 为 1，则打印日志，并输出文件名、行号、日志内容
#if ENABLE_LOG
    #define LOG(fmt, ...) printf("LOG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...) // 空实现，日志输出被禁用
#endif

#define MAX_SCAN_RESULTS 64 // 最大扫描数量
#define MAC_STR_LEN 18      // MAC 地址字符串长度 (17 个字符 + 1 个 '\0')
#define MAX_MESH_NODES 5    // 最大 Mesh 节点数量

// 定义当前状态变量
static NetworkState current_state;

// 定义全局 Mesh 网络配置
static MeshNetworkConfig g_mesh_config;

// 定义连接配置
static WirelessSTAConfig sta_config;

// 定义存储Mesh节点信息
static MeshNode mesh_nodes[MAX_MESH_NODES];

// 初始化状态机
int network_fsm_init(const MeshNetworkConfig *config) {
    // 保存传入的 Mesh 网络配置到全局变量中
    if (config != NULL) {
        strncpy(g_mesh_config.mesh_ssid, config->mesh_ssid, sizeof(g_mesh_config.mesh_ssid) - 1);
        strncpy(g_mesh_config.password, config->password, sizeof(g_mesh_config.password) - 1);
        g_mesh_config.mesh_ssid[sizeof(g_mesh_config.mesh_ssid) - 1] = '\0';  // 确保字符串以 '\0' 结尾
        g_mesh_config.password[sizeof(g_mesh_config.password) - 1] = '\0';    // 确保字符串以 '\0' 结尾

        LOG("Mesh Network Config: SSID = %s, Password = %s\n", g_mesh_config.mesh_ssid, g_mesh_config.password);
    }else
    {
        LOG("MeshNetworkConfig is NULL!\n");
        return -1;
    }
    // 设置初始状态为 STATE_STARTUP
    current_state = STATE_STARTUP;
    LOG("Network FSM initialized. Current state: STATE_STARTUP\n");
    return 0;
}

// 状态机的主循环函数
void network_fsm_run(void) {
    // 持续执行状态机，直到达到终止状态
    while (current_state != STATE_TERMINATE) {
        switch (current_state) {
            case STATE_STARTUP:
                current_state = state_startup();
                break;
            case STATE_SCANNING:
                current_state = state_scanning();
                break;
            case STATE_JOIN_EXISTING_NETWORK:
                current_state = state_join_existing_network();
                break;
            case STATE_CHECK_ROOT_COUNT:
                current_state = state_check_root_count();
                break;
            case STATE_JOIN_NETWORK:
                current_state = state_join_network();
                break;
            case STATE_CONNECTED:
                current_state = state_connected();
                break;
            case STATE_CREATE_ROOT:
                current_state = state_create_root();
                break;
            case STATE_CHECK_ROOT_CONFLICT:
                current_state = state_check_root_conflict();
                break;
            case STATE_HANDLE_ROOT_CONFLICT:
                current_state = state_handle_root_conflict();
                break;
            case STATE_ROOT_ELECTION:
                current_state = state_root_election();
                break;
            default:
                LOG("Unknown state, terminating...\n");
                current_state = STATE_TERMINATE;
                break;
        }
    }
    LOG("State machine terminated.\n");
}

// 启动状态处理函数
NetworkState state_startup(void) {
    LOG("Node powered on, initializing STA mode...\n");
    // 初始化 Wi-Fi
    if (HAL_Wireless_Init(DEFAULT_WIRELESS_TYPE) == 0) {
        LOG("Wi-Fi initialization successfully.\n");
    }else{
        LOG("Wi-Fi initialization failed.\n");
        return STATE_TERMINATE;
    }
    
    // 启动 Wi-Fi STA 模式
    if (HAL_Wireless_Start(DEFAULT_WIRELESS_TYPE) == 0) {
        LOG("Wi-Fi STA mode started successfully.\n");
    } else {
        LOG("Failed to start Wi-Fi STA mode.\n");
        return STATE_TERMINATE;
    }

    // 假设初始化完成后进入扫描状态
    return STATE_SCANNING;
}

// 判断是否是有效的树层级字符（0-9, A-Z, a-z）
static int is_valid_tree_level_char(char c) {
    return (isdigit(c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

// 将树层级转换为数字（0-9对应0-9，A-Z对应10-35，a-z对应36-61）
static int tree_level_to_int(char tree_level_char) {
    if (isdigit(tree_level_char)) {
        return tree_level_char - '0';
    } else if (tree_level_char >= 'A' && tree_level_char <= 'Z') {
        return tree_level_char - 'A' + 10;
    } else if (tree_level_char >= 'a' && tree_level_char <= 'z') {
        return tree_level_char - 'a' + 36;
    }
    return -1; // 无效字符
}

// 扫描状态处理函数
NetworkState state_scanning(void) {
    LOG("Scanning for existing Mesh networks...\n");

    WirelessScanResult *scan_results = (WirelessScanResult *)malloc(sizeof(WirelessScanResult) * MAX_SCAN_RESULTS);
    if (scan_results == NULL) {
        LOG("Memory allocation failed!\n");
        return STATE_TERMINATE;
    }

    // 扫描网络
    int scanned_count = HAL_Wireless_Scan(DEFAULT_WIRELESS_TYPE, &sta_config, scan_results, MAX_SCAN_RESULTS);
    LOG("Found %d networks:\n", scanned_count);

    const char *mesh_prefix = g_mesh_config.mesh_ssid; // Mesh 自定义 SSID 前缀
    size_t mesh_prefix_len = strlen(mesh_prefix);      // 计算 Mesh 自定义 SSID 的长度
    WirelessScanResult *best_result = NULL;            // 保存最佳网络
    int best_rssi = -9999;                             // 初始化为最小可能的 RSSI
    int best_tree_level = 62;                          // 初始化为最大的树层级（a-z，最大61）
    int best_mac[3] = {0};                             // 保存最大的 MAC 地址

    // 遍历所有扫描到的网络
    for (int i = 0; i < scanned_count; i++) {
        // 将 bssid 转换为 xx:xx:xx:xx:xx:xx 的格式
        char bssid_str[MAC_STR_LEN];
        snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 scan_results[i].bssid[0], scan_results[i].bssid[1], scan_results[i].bssid[2],
                 scan_results[i].bssid[3], scan_results[i].bssid[4], scan_results[i].bssid[5]);

        LOG("  - SSID: %s, RSSI: %d dBm, Channel: %d, BSSID: %s\n",
            scan_results[i].ssid, scan_results[i].rssi, scan_results[i].channel, bssid_str);
        
        int mesh_nodes_count = 0;
        // 只在 ssid_len 大于等于 mesh_prefix_len 时，才进行前缀比较
        if (strncmp(scan_results[i].ssid, mesh_prefix, mesh_prefix_len) == 0) {
            LOG("Matching Mesh SSID: %s\n", scan_results[i].ssid);
            
            size_t ssid_len = strlen(scan_results[i].ssid);
            // 提取树层级，假设树层级字符是 SSID 的最后一位
            char tree_level_char = scan_results[i].ssid[ssid_len - 1];

            // 从ssid提取根节点的mac地址
            uint8_t current_mac[3];
            current_mac[0] = scan_results[i].ssid[mesh_prefix_len];
            current_mac[1] = scan_results[i].ssid[mesh_prefix_len + 1];
            current_mac[2] = scan_results[i].ssid[mesh_prefix_len + 2];
            LOG("  Root node MAC (last 3 bytes) from SSID: %02X:%02X:%02X\n", current_mac[0], current_mac[1], current_mac[2]);

            // 判断是否为有效的树层级字符
            if (is_valid_tree_level_char(tree_level_char)) {
                int tree_level = tree_level_to_int(tree_level_char);
                LOG("  Tree level: %d, RSSI: %d\n", tree_level, scan_results[i].rssi);
                // 将扫描到的 Mesh 节点信息保存到数组中
                if (mesh_nodes_count < MAX_MESH_NODES) {
                    strncpy(mesh_nodes[mesh_nodes_count].ssid, scan_results[i].ssid, sizeof(mesh_nodes[mesh_nodes_count].ssid));
                    memcpy(mesh_nodes[mesh_nodes_count].bssid, scan_results[i].bssid, sizeof(mesh_nodes[mesh_nodes_count].bssid));
                    mesh_nodes[mesh_nodes_count].rssi = scan_results[i].rssi;
                    mesh_nodes[mesh_nodes_count].channel = scan_results[i].channel;
                    mesh_nodes[mesh_nodes_count].tree_level = tree_level;
                    mesh_nodes_count++;
                }
                // 先比较mac地址，再比较rssi，最后比较树层级
                if(memcmp(current_mac, best_mac, sizeof(current_mac)) > 0 || 
                    (memcmp(current_mac, best_mac, sizeof(current_mac)) == 0 && scan_results[i].rssi > best_rssi) ||
                    (memcmp(current_mac, best_mac, sizeof(current_mac)) == 0 && scan_results[i].rssi == best_rssi && tree_level < best_tree_level)) {
                    best_result = &scan_results[i];
                    best_rssi = scan_results[i].rssi;
                    best_tree_level = tree_level;
                    memcpy(best_mac, current_mac, 3);
                }
            }
        }
    }

    if (best_result) {
        LOG("Best matching Mesh network found: SSID = %s, RSSI = %d, Tree Level = %d\n",
            best_result->ssid, best_rssi, best_tree_level);

        // 设置连接的配置
        strncpy(sta_config.ssid, best_result->ssid, sizeof(sta_config.ssid));
        strcpy(sta_config.password, g_mesh_config.password);
        memcpy(sta_config.bssid, best_result->bssid, sizeof(sta_config.bssid));
        sta_config.type = DEFAULT_WIRELESS_TYPE;

        free(scan_results);  // 释放内存
        return STATE_JOIN_EXISTING_NETWORK;
    } else {
        LOG("No matching Mesh networks found, creating new root node...\n");
        free(scan_results);  // 释放内存
        return STATE_CREATE_ROOT;
    }
}

// 加入已有网络状态处理函数
NetworkState state_join_existing_network(void) {
    LOG("Joining existing Mesh network...\n");
    // 连接到扫描到的最佳网络
    if (HAL_Wireless_Connect(DEFAULT_WIRELESS_TYPE, &sta_config) == 0) {
        LOG("Successfully connected to Mesh network.\n");
        return STATE_CONNECTED;
    } else {
        LOG("Failed to connect to Mesh network.\n");
        return STATE_STARTUP;
    }
}

// 检查根节点个数状态处理函数
NetworkState state_check_root_count(void) {
    LOG("Checking root node count...\n");
    // 假设根节点数量检查完成，进入下一状态
    return STATE_JOIN_NETWORK;
}

// 加入网络状态处理函数
NetworkState state_join_network(void) {
    LOG("Joining network...\n");
    // 假设成功连接
    return STATE_CONNECTED;
}

// 成功连接状态处理函数
NetworkState state_connected(void) {
    LOG("Scaning other mesh network.\n");
    // 成功连接后可以继续扫描或处理其他逻辑
    // 假设此时终止状态机
    return STATE_TERMINATE;
}

// 创建根节点状态处理函数
NetworkState state_create_root(void) {
    LOG("Creating new root node for Mesh network...\n");
    // 创建根节点成功后进入根节点冲突检查状态
    return STATE_CHECK_ROOT_CONFLICT;
}

// 检查根节点冲突状态处理函数
NetworkState state_check_root_conflict(void) {
    LOG("Checking for root node conflicts...\n");
    // 假设检测到根节点冲突
    bool root_conflict_detected = true;
    if (root_conflict_detected) {
        return STATE_HANDLE_ROOT_CONFLICT;
    } else {
        return STATE_CONNECTED;
    }
}

// 处理根节点冲突状态处理函数
NetworkState state_handle_root_conflict(void) {
    LOG("Handling root node conflict...\n");
    // 假设冲突处理完成后执行根节点选举
    return STATE_ROOT_ELECTION;
}

// 根节点选举状态处理函数
NetworkState state_root_election(void) {
    LOG("Executing root node election...\n");
    // 假设选举完成，成功成为根节点
    return STATE_CONNECTED;
}
