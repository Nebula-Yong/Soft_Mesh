#include <stdio.h>
#include "cmsis_os2.h"  // CMSIS RTOS 库
#include "app_init.h"
#include "network_fsm.h"

#define NETWORK_TASK_STACK_SIZE 0x1500  // 定义任务栈大小
#define NETWORK_TASK_PRIO       (osPriority_t)(13)  // 定义任务优先级

void network_task(void *argument) {
    (void)argument;  // 忽略未使用的参数
    printf("start network_task!\n");
    osDelay(100);  // 延迟 1 秒，模拟任务运行（单位：毫秒）
    MeshNetworkConfig mesh_config = {
        .mesh_ssid = "FsrMesh",
        .password = "12345678"
    };
    if(network_fsm_init(&mesh_config) == 0) {
        network_fsm_run();
    }else {
        printf("network_fsm init Failed.\n");
    }
}

/* 创建任务 */
static void network_task_entry(void) {
    osThreadAttr_t attr;
    attr.name       = "network_task";         // 任务名
    attr.attr_bits  = 0U;                   // 任务属性位
    attr.cb_mem     = NULL;                 // 控制块内存
    attr.cb_size    = 0U;                   // 控制块大小
    attr.stack_mem  = NULL;                 // 栈内存
    attr.stack_size = NETWORK_TASK_STACK_SIZE;// 任务栈大小
    attr.priority   = NETWORK_TASK_PRIO;      // 任务优先级

    // 创建任务并检查是否成功
    if (osThreadNew((osThreadFunc_t)network_task, NULL, &attr) == NULL) {
        printf("Failed to create network task.\n");
    } else {
        printf("network task created successfully.\n");
    }
}
app_run(network_task_entry);
