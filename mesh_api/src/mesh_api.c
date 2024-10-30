#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include "network_fsm.h"

// 定义宏开关，打开或关闭日志输出
#define ENABLE_LOG 1  // 1 表示开启日志，0 表示关闭日志

// 定义 LOG 宏，如果 ENABLE_LOG 为 1，则打印日志，并输出文件名、行号、日志内容
#if ENABLE_LOG
    #define LOG(fmt, ...) printf("LOG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...) // 空实现，日志输出被禁用
#endif

int mesh_init(const char *ssid, const char *password) {
    LOG("Initializing mesh network...\n");
    MeshNetworkConfig mesh_config;
    // 检查 SSID 和密码是否合法
    if (strlen(ssid) > 15 || strlen(password) > 64) {
        LOG("SSID or password is too long!\n");
        return -1;
    }
    // 配置Mesh网络设置
    strcpy(mesh_config.mesh_ssid, ssid);
    strcpy(mesh_config.password, password);
    if(network_fsm_init(&mesh_config) == 0) {
        network_fsm_run();
        return 0;
    }else {
        LOG("network_fsm init Failed.\n");
        return -1;
    }
}