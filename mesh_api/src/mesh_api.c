#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include "network_fsm.h"
#include "routing_transport.h"
#include "hal_wireless.h"
#include "mesh_api.h"

// 定义宏开关，打开或关闭日志输出
#define ENABLE_LOG 0  // 1 表示开启日志，0 表示关闭日志

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
osThreadId_t route_transport_task_id;

extern osMessageQueueId_t dataPacketQueueId;

// Mesh配置全局变量
extern MeshNetworkConfig g_mesh_config;

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
    osThreadAttr_t attr1;
    attr1.name       = "network_task";         // 任务名
    attr1.attr_bits  = 0U;                   // 任务属性位
    attr1.cb_mem     = NULL;                 // 控制块内存
    attr1.cb_size    = 0U;                   // 控制块大小
    attr1.stack_mem  = NULL;                 // 栈内存
    attr1.stack_size = 0x1500;// 任务栈大小
    attr1.priority   = osPriorityLow4;      // 任务优先级

    network_task_id = osThreadNew((osThreadFunc_t)network_task, NULL, &attr1);
    // 创建任务并检查是否成功
    if (network_task_id == NULL) {
        LOG("Failed to create network task.\n");
        return -1;
    }

    // 创建路由构建和传输线程
    osThreadAttr_t attr2;
    attr2.name       = "route_transport_task";
    attr2.attr_bits  = 0U;
    attr2.cb_mem     = NULL;
    attr2.cb_size    = 0U;
    attr2.stack_mem  = NULL;
    attr2.stack_size = 0x1000;
    attr2.priority   = osPriorityLow4;

    route_transport_task_id = osThreadNew((osThreadFunc_t)route_transport_task, NULL, &attr2);
    if (route_transport_task_id == NULL) {
        LOG("Failed to create route transport task.\n");
        return -1;
    }

    return 0;
}

int mesh_send_data(const char *dest_mac, const char *data) {
    // 检查网络是否连接
    if (network_connected() != 1) {
        LOG("Network is not connected.\n");
        return -1;
    }
    // DataPacket packet;
    // 如果目标MAC地址是广播地址，则直接广播数据包
    if (strcmp(dest_mac, "FFFFFF") == 0) {
        mesh_broadcast(data);
    }else{
        // 如果目标MAC地址不是广播地址，则向目标节点发送数据包
        send_data_packet(dest_mac, data);
    }
    return 0;
}

int mesh_broadcast(const char *data) {
    // 检查网络是否连接
    if (network_connected() != 1) {
        LOG("Network is not connected.\n");
        return -1;
    }
    DataPacket packet;
    // 如果自己是根节点，则直接广播数据包
    if (g_mesh_config.tree_level == 0) {
        packet.type = '1';
        strcpy(packet.src_mac, "000000");
        strcpy(packet.dest_mac, "FFFFFF");
        packet.status = '4';
        memcpy(packet.packet_num, "000", 3);
        memcpy(packet.crc, "00", 2);
        strcpy(packet.data, data);
        broadcast_data_packet(packet);
    }else{
        // 如果不是根节点，则向根节点发送广播请求
        packet.type = '1';
        char my_mac[7] = {0};
        HAL_Wireless_GetNodeMAC(DEFAULT_WIRELESS_TYPE, my_mac);
        strcpy(packet.src_mac, my_mac);
        strcpy(packet.dest_mac, "000000");
        packet.status = '3';
        memcpy(packet.packet_num, "000", 3);
        memcpy(packet.crc, "00", 2);
        strcpy(packet.data, data);
        char *packet_data = generate_data_packet(packet);
        HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, packet_data, g_mesh_config.tree_level - 1);
    }
    return 0;
}

int mesh_recv_data(char *src_mac, char *data) {
    // 从队列中获取数据包
    DataPacket packet;
    osStatus_t status = osMessageQueueGet(dataPacketQueueId, &packet, NULL, 0);
    if (status != osOK) {
        LOG("no data in queue.\n");
        return -1;
    }
    if (packet.status == '1') {
        LOG("Received a ack packet.\n");
        return -1;
    }
    // 解析数据包
    strncpy(src_mac, packet.src_mac, MAC_SIZE);
    src_mac[MAC_SIZE] = '\0';
    strcpy(data, packet.data);
    return 0;
}

int mesh_network_connected(void) {
    return network_connected();
}