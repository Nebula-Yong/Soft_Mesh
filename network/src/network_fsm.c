#include "network_fsm.h"
#include <stdio.h>
#include <stdbool.h>

// 定义当前状态变量
static NetworkState current_state;

// 初始化状态机
void network_fsm_init(void) {
    // 设置初始状态为 STATE_STARTUP
    current_state = STATE_STARTUP;
    printf("Network FSM initialized. Current state: STATE_STARTUP\n");
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
                printf("Unknown state, terminating...\n");
                current_state = STATE_TERMINATE;
                break;
        }
    }
    printf("State machine terminated.\n");
}

// 启动状态处理函数
NetworkState state_startup(void) {
    printf("Node powered on, initializing STA mode...\n");
    // 假设初始化完成后进入扫描状态
    return STATE_SCANNING;
}

// 扫描状态处理函数
NetworkState state_scanning(void) {
    printf("Scanning for existing Mesh networks...\n");
    // 扫描网络
    // 假设扫描完成后发现存在 Mesh 网络，则加入已有网络
    // 否则，创建根节点
    bool mesh_network_found = true;  // 假设找到网络（同步方式下的条件判断）
    if (mesh_network_found) {
        return STATE_JOIN_EXISTING_NETWORK;
    } else {
        return STATE_CREATE_ROOT;
    }
}

// 加入已有网络状态处理函数
NetworkState state_join_existing_network(void) {
    printf("Joining existing Mesh network...\n");
    // 假设加入网络成功，进入已连接状态
    return STATE_CONNECTED;
}

// 检查根节点个数状态处理函数
NetworkState state_check_root_count(void) {
    printf("Checking root node count...\n");
    // 假设根节点数量检查完成，进入下一状态
    return STATE_JOIN_NETWORK;
}

// 加入网络状态处理函数
NetworkState state_join_network(void) {
    printf("Joining network...\n");
    // 假设成功连接
    return STATE_CONNECTED;
}

// 成功连接状态处理函数
NetworkState state_connected(void) {
    printf("Successfully connected to Mesh network.\n");
    // 成功连接后可以继续扫描或处理其他逻辑
    // 假设此时终止状态机
    return STATE_TERMINATE;
}

// 创建根节点状态处理函数
NetworkState state_create_root(void) {
    printf("Creating new root node for Mesh network...\n");
    // 创建根节点成功后进入根节点冲突检查状态
    return STATE_CHECK_ROOT_CONFLICT;
}

// 检查根节点冲突状态处理函数
NetworkState state_check_root_conflict(void) {
    printf("Checking for root node conflicts...\n");
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
    printf("Handling root node conflict...\n");
    // 假设冲突处理完成后执行根节点选举
    return STATE_ROOT_ELECTION;
}

// 根节点选举状态处理函数
NetworkState state_root_election(void) {
    printf("Executing root node election...\n");
    // 假设选举完成，成功成为根节点
    return STATE_CONNECTED;
}
