#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include "mesh_api.h"



void test_api_task(void)
{
    printf("test_api_task run...\n");
    // 设置Mesh网络的SSID和密码
    if(mesh_init("FsrMesh", "12345678") != 0) {
        printf("mesh_init failed.\n");
    } else {
        printf("mesh_init successfully.\n");
    }
    while (1)
    {
        osDelay(1000);
        char src_mac[7] = {0};
        char data[512] = {0};
        if(mesh_recv_data(src_mac, data) == 0) {
            printf("Received data from client: %s, MAC: %s\n", data, src_mac);
        }else {
            printf("no data recv.\n");
        }
        if (mesh_broadcast("Hello, Mesh!") != 0) {
            printf("mesh_broadcast failed.\n");
        } else {
            printf("mesh_broadcast successfully.\n");
        }
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
        printf("Create test_api_task failed.\n");
    } else {
        printf("Create test_api_task successfully.\n");
    }
}

/* 启动任务 */
app_run(sta_sample_entry);
