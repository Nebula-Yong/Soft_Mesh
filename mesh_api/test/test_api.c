#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include "mesh_api.h"

// 定义宏开关，打开或关闭日志输出
#define ENABLE_LOG 1  // 1 表示开启日志，0 表示关闭日志

// 定义 LOG 宏，如果 ENABLE_LOG 为 1，则打印日志，并输出文件名、行号、日志内容
#if ENABLE_LOG
    #define LOG(fmt, ...) printf("LOG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...) do { UNUSED(fmt); } while (0) 
#endif

void test_api_task(void)
{
    LOG("test_api_task run...\n");
    // 设置Mesh网络的SSID和密码
    if(mesh_init("FsrMesh", "12345678") != 0) {
        LOG("mesh_init failed.\n");
    } else {
        LOG("mesh_init successfully.\n");
    }
    while (1)
    {
        if (mesh_network_connected() == 0) {
            LOG("Mesh network not connected.\n");
            osDelay(100);
            continue;
        }
        osDelay(100);
        // if (mesh_broadcast("Hello, Mesh!") != 0) {
        //     LOG("mesh_broadcast failed.\n");
        // } else {
        //     LOG("mesh_broadcast successfully.\n");
        // }
        char src_mac[7] = {0};
        src_mac[6] = '\0';
        char data[512] = {0};
        if(mesh_recv_data(src_mac, data) == 0) {
            LOG("Received data from client: %s, MAC: %s\n", data, src_mac);
        }else {
            LOG("no data recv.\n");
            continue;
        }
        LOG("say hi to mac: %s\n", src_mac);
        mesh_send_data(src_mac, "Hi, Node!");
    }
    
}

/* 创建任务 */
static void sta_sample_entry(void)
{
    osThreadAttr_t attr;
    attr.name       = "test_api_task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = 0x1000;
    attr.priority   = osPriorityLow4;

    if (osThreadNew((osThreadFunc_t)test_api_task, NULL, &attr) == NULL) {
        LOG("Create test_api_task failed.\n");
    } else {
        LOG("Create test_api_task successfully.\n");
    }
}

/* 启动任务 */
app_run(sta_sample_entry);
