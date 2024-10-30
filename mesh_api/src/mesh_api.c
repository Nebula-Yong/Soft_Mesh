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



// 存储mesh网络配置信息
char mesh_ssid[16];
char mesh_password[65];

// 线程ID
osThreadId_t network_task_id;

void network_task(void) {
    MeshNetworkConfig mesh_config;
    strcpy(mesh_config.mesh_ssid, mesh_ssid);
    strcpy(mesh_config.password, mesh_password);
    if(network_fsm_init(&mesh_config) == 0) {
        network_fsm_run();
    }else {
        LOG("network_fsm init Failed.\n");
    }
}

int mesh_init(const char *ssid, const char *password) {
    LOG("Initializing mesh network...\n");
    // 检查 SSID 和密码是否合法
    if (strlen(ssid) > 15 || strlen(password) > 64) {
        LOG("SSID or password is too long!\n");
        return -1;
    }
    // 配置Mesh网络设置
    strcpy(mesh_ssid, ssid);
    strcpy(mesh_password, password);

    // 创建network线程
    osThreadAttr_t attr;
    attr.name       = "network_task";         // 任务名
    attr.attr_bits  = 0U;                   // 任务属性位
    attr.cb_mem     = NULL;                 // 控制块内存
    attr.cb_size    = 0U;                   // 控制块大小
    attr.stack_mem  = NULL;                 // 栈内存
    attr.stack_size = 0x1500;// 任务栈大小
    attr.priority   = osPriorityLow4;      // 任务优先级

    network_task_id = osThreadNew((osThreadFunc_t)network_task, NULL, &attr);
    // 创建任务并检查是否成功
    if (network_task_id == NULL) {
        LOG("Failed to create network task.\n");
        return -1;
    }
    return 0;
}