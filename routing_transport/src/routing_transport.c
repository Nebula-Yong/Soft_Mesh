#include <stdio.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include "hal_wireless.h"

// 定义宏开关，打开或关闭日志输出
#define ENABLE_LOG 1  // 1 表示开启日志，0 表示关闭日志

// 定义 LOG 宏，如果 ENABLE_LOG 为 1，则打印日志，并输出文件名、行号、日志内容
#if ENABLE_LOG
    #define LOG(fmt, ...) printf("LOG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...) // 空实现，日志输出被禁用
#endif

// 路由传输层开启标志位
extern osEventFlagsId_t route_transport_event_flags;
#define ROUTE_TRANSPORT_START_BIT (1 << 0)
#define ROUTE_TRANSPORT_STOP_BIT  (1 << 1)

void route_transport_task(void)
{
    int server_fd = HAL_Wireless_CreateServer(DEFAULT_WIRELESS_TYPE);
    if (server_fd < 0) {
        LOG("Failed to create server.\n");
        return;
    }
    LOG("Server created successfully.\n");
    int status = 1;
    while (1)
    {
        uint32_t flags = osEventFlagsWait(route_transport_event_flags, ROUTE_TRANSPORT_START_BIT | ROUTE_TRANSPORT_STOP_BIT, osFlagsWaitAny, 100);
        LOG("flag:0x%08X\n", flags);
        if (flags & ROUTE_TRANSPORT_STOP_BIT && flags != osFlagsErrorTimeout) {
            LOG("Stop route transport task.\n");
            status = 0;
        }else if (flags & ROUTE_TRANSPORT_START_BIT && flags != osFlagsErrorTimeout) {
            LOG("Start route transport task.\n");
            status = 1;
        }
        if (status == 0) {
            continue;
        }
        char mac[7] = {0};
        char buffer[50] = {0};
        int ret = HAL_Wireless_ReceiveDataFromClient(DEFAULT_WIRELESS_TYPE, server_fd, mac, buffer, sizeof(buffer));
        if (ret < 0) {
            LOG("nothing sent from client.\n");
            continue;
        }
        LOG("Received data from client: %s, MAC: %s\n", buffer, mac);
    }
    HAL_Wireless_CloseServer(DEFAULT_WIRELESS_TYPE, server_fd);
    
    return;
}

/* 创建任务 */
static void route_transport_entry(void)
{
    osThreadAttr_t attr;
    attr.name       = "route_transport_task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = 0x1000;
    attr.priority   = osPriorityLow4;

    if (osThreadNew((osThreadFunc_t)route_transport_task, NULL, &attr) == NULL) {
        printf("Create route_transport_task failed.\n");
    } else {
        printf("Create route_transport_task successfully.\n");
    }
}

/* 启动任务 */
app_run(route_transport_entry);