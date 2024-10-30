#include "hal_wireless.h"
#include "hal_wifi.h"  // 包含 Wi-Fi 的 HAL 接口实现
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "std_def.h"

// 定义宏开关，打开或关闭日志输出
#define ENABLE_LOG 0  // 1 表示开启日志，0 表示关闭日志

// 定义 LOG 宏，如果 ENABLE_LOG 为 1，则打印日志，并输出文件名、行号、日志内容
#if ENABLE_LOG
    #define LOG(fmt, ...) printf("LOG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...) do { UNUSED(fmt); } while (0) 
#endif

osEventFlagsId_t wireless_event_flags;  // wifi连接事件标志对象
#define WIRELESS_CONNECT_BIT    (1 << 0)
#define WIRELESS_DISCONNECT_BIT (1 << 1)

/**
 * @brief 初始化指定类型的无线通信模块及相关资源
 * @param type 指定无线通信类型，如WIFI, BLUETOOTH, nearlink。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Init(WirelessType type)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Init();  // 调用Wi-Fi模块的初始化函数
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块初始化函数
            // ret = HAL_Bluetooth_Init();
            LOG("Bluetooth module initialization not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块初始化函数
            // ret = HAL_nearlink_Init();
            LOG("nearlink module initialization not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 去初始化指定类型的无线通信模块，释放相关资源
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Deinit(WirelessType type)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Deinit();  // 调用Wi-Fi模块的去初始化函数
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块去初始化函数
            // ret = HAL_Bluetooth_Deinit();
            LOG("Bluetooth module deinitialization not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块去初始化函数
            // ret = HAL_nearlink_Deinit();
            LOG("nearlink module deinitialization not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 启动指定类型的无线通信模块，如Wi-Fi STA模式、蓝牙连接模式等。
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Start(WirelessType type)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_STA_Enable();  // 启动Wi-Fi STA模式
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块启动函数
            // ret = HAL_Bluetooth_Enable();
            LOG("Bluetooth module start not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块启动函数
            // ret = HAL_nearlink_Enable();
            LOG("nearlink module start not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 停止指定类型的无线通信模块，如关闭Wi-Fi、蓝牙断连等。
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Stop(WirelessType type)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_STA_Disable();  // 停止Wi-Fi STA模式
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块停止函数
            // ret = HAL_Bluetooth_Disable();
            LOG("Bluetooth module stop not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块停止函数
            // ret = HAL_nearlink_Disable();
            LOG("nearlink module stop not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 启动指定类型的无线通信模块的AP模式，如Wi-Fi AP模式、蓝牙广播模式等。
 * @param type 指定无线通信类型。
 * @param config AP模式的配置参数（SSID、密码、信道等）
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_EnableAP(WirelessType type, const WirelessAPConfig *config, int tree_level)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI: {
            WiFiAPConfig wifi_config;
            strncpy(wifi_config.ssid, config->ssid, sizeof(wifi_config.ssid) - 1);
            strncpy(wifi_config.password, config->password, sizeof(wifi_config.password) - 1);
            wifi_config.channel = config->channel;
            wifi_config.security = config->security;  // 设置Wi-Fi安全类型
            ret = HAL_WiFi_AP_Enable(&wifi_config, tree_level);   // 启动Wi-Fi AP模式
            break;
        }
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块AP模式启动函数
            // ret = HAL_Bluetooth_EnableAP(config);
            LOG("Bluetooth AP mode start not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块AP模式启动函数
            // ret = HAL_nearlink_EnableAP(config);
            LOG("nearlink AP mode start not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 停止指定类型的无线通信模块的AP模式，如关闭Wi-Fi AP模式、关闭蓝牙广播等。
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_DisableAP(WirelessType type)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_AP_Disable();  // 停止Wi-Fi AP模式
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块AP模式停止函数
            // ret = HAL_Bluetooth_DisableAP();
            LOG("Bluetooth AP mode stop not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块AP模式停止函数
            // ret = HAL_nearlink_DisableAP();
            LOG("nearlink AP mode stop not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 扫描指定类型的无线网络
 * @param type 指定无线通信类型（如Wi-Fi、蓝牙）
 * @param wifi_config 用于匹配要连接的Wi-Fi网络的配置
 * @param results 用于存储扫描结果的数组，调用方需预先分配足够的空间。
 * @param max_results `results` 数组的最大长度，表示最多存储多少个扫描结果。
 * @return 扫描到的无线网络数量，或 < 0 表示失败
 */
int HAL_Wireless_Scan(WirelessType type, WirelessSTAConfig *wifi_config, WirelessScanResult *results, int max_results)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI: {
            WiFiSTAConfig wifi_sta_config;
            strncpy(wifi_sta_config.ssid, wifi_config->ssid, sizeof(wifi_sta_config.ssid) - 1);
            strncpy(wifi_sta_config.password, wifi_config->password, sizeof(wifi_sta_config.password) - 1);
            ret = HAL_WiFi_Scan(&wifi_sta_config, (WiFiScanResult *)results, max_results);
            memcpy(wifi_config->bssid, wifi_sta_config.bssid, sizeof(wifi_sta_config.bssid));
            wifi_config->security = wifi_sta_config.security_type;
            break;
        }
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块扫描函数
            // ret = HAL_Bluetooth_Scan(results, max_results);
            LOG("Bluetooth scan not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块扫描函数
            // ret = HAL_nearlink_Scan(results, max_results);
            LOG("nearlink scan not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 连接到指定的无线设备（如Wi-Fi AP或蓝牙设备）
 * @param config 连接配置信息，可以是不同通信类型的配置结构体（具体定义在各自HAL中）
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Connect(WirelessType type, WirelessSTAConfig *config)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:{
            WiFiSTAConfig wifi_config;
            strncpy(wifi_config.ssid, config->ssid, sizeof(wifi_config.ssid) - 1);
            strncpy(wifi_config.password, config->password, sizeof(wifi_config.password) - 1);
            memcpy(wifi_config.bssid, config->bssid, sizeof(wifi_config.bssid));
            wifi_config.security_type = config->security;  // 设置Wi-Fi安全类型
            ret = HAL_WiFi_Connect(&wifi_config);  // 连接Wi-Fi网络
            break;
        }
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块连接函数
            // ret = HAL_Bluetooth_Connect(config);
            LOG("Bluetooth connect not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块连接函数
            // ret = HAL_nearlink_Connect(config);
            LOG("nearlink connect not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 断开当前的无线连接
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Disconnect(WirelessType type)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Disconnect();  // 断开Wi-Fi连接
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // 蓝牙模块断开连接函数
            // ret = HAL_Bluetooth_Disconnect();
            LOG("Bluetooth disconnect not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // 星闪模块断开连接函数
            // ret = HAL_nearlink_Disconnect();
            LOG("nearlink disconnect not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 获取当前无线通信模块的IP地址（仅限Wi-Fi）
 * @param ip_buffer 存储IP地址的缓冲区，建议至少分配16字节以存储完整的IPv4地址。
 * @param buffer_len `ip_buffer` 的大小
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_GetIP(WirelessType type, char *ip_buffer, int buffer_len)
{
    if (type == WIRELESS_TYPE_WIFI) {
        return HAL_WiFi_GetIP(ip_buffer, buffer_len);  // 获取Wi-Fi IP地址
    } else {
        LOG("IP address retrieval only supported for Wi-Fi.\n");
        return -1;
    }
}

/**
 * @brief 获取已连接的无线设备信息，如Wi-Fi的STA或蓝牙的连接设备
 * @param type 指定无线通信类型。
 * @param[out] result 用于存储设备信息的数组
 * @param[in, out] size  输入为数组的最大长度，输出为已连接的设备数量
 * @return 0 表示成功，其他表示失败
 */
int HAL_Wireless_GetConnectedDeviceInfo(WirelessType type, WirelessConnectedInfo *result, uint32_t *size)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_GetConnectedSTAInfo((WiFiSTAInfo *)result, size);
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_GetConnectedDeviceInfo(result, size);
            LOG("Bluetooth connected device info retrieval not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_GetConnectedDeviceInfo(result, size);
            LOG("nearlink connected device info retrieval not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 获取AP模式下的MAC地址
 * @param type 指定无线通信类型。
 * @param[out] mac 存储MAC地址的缓冲区，WIFI的MAC长度为6字节
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_GetAPMacAddress(WirelessType type, uint8_t *mac)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_GetAPMacAddress(mac);
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_GetAPMacAddress(mac);
            LOG("Bluetooth AP MAC address retrieval not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_GetAPMacAddress(mac);
            LOG("nearlink AP MAC address retrieval not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 获取节点的MAC地址，AP MAC地址的后三位
 * @param type 指定无线通信类型。
 * @param[out] mac 存储MAC地址的缓冲区，至少需要7字节
 * @return 0 表示成功，非 0 表示失败
 * @note 6字节的MAC地址，最后一位为'\0'，共7字节
 */
int HAL_Wireless_GetNodeMAC(WirelessType type, char *mac) {
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_GetNodeMAC(mac);
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_GetNodeMAC(mac);
            LOG("Bluetooth node MAC address retrieval not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_GetNodeMAC(mac);
            LOG("nearlink node MAC address retrieval not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 通过无线通信模块发送数据
 * @param type 指定无线通信类型。
 * @param MAC 目标设备的MAC地址,7字节
 * @param data 要发送的字符串数据
 * @param tree_level 父节点的树级, 非负整数表示向上传输数据到父节点，负整数表示向下传输数据到子节点
 * @return 0 表示成功，非 0 表示失败
 * @note 当向上传递时，MAC地址为空字符串，当向下传递时，MAC地址为目标设备的MAC地址
 * @note 该接口仅支持Wi-Fi通信类型
 * @note 该接口仅支持发送TCP数据
 * 
 */
int HAL_Wireless_SendData_to_child(WirelessType type, const char *MAC, const char *data) {
    int ret = -1;
    char ip[16];
    memcmp(MAC, ip, sizeof(ip));
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Send_data_by_MAC(MAC, data);
            if(ret == 0) {
                LOG("Data sent successfully to %s.\n", ip);
            } else {
                LOG("Failed to send data to %s.\n", ip);
            }
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_SendData(MAC, data);
            LOG("Bluetooth data send not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_SendData(MAC, data);
            LOG("nearlink data send not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 通过无线通信模块发送数据给父节点
 * @param type 指定无线通信类型。
 * @param data 要发送的字符串数据
 * @param tree_level 父节点所在树的层数
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_SendData_to_parent(WirelessType type, const char *data, int tree_level) {
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Send_data_to_parent(data, tree_level);
            if(ret == 0) {
                LOG("Data sent successfully to parent node at level %d.\n", tree_level);
            } else {
                LOG("Failed to send data to parent node at level %d.\n", tree_level);
            }
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_SendData_to_parent(data, tree_level);
            LOG("Bluetooth data send to parent not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_SendData_to_parent(data, tree_level);
            LOG("nearlink data send to parent not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 创建无线接收服务器
 * @param type 指定无线通信类型。
 * @return 返回服务器资源描述符，或 < 0 表示失败
 */
int HAL_Wireless_CreateServer(WirelessType type) {
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Create_Server(9001);
            if(ret >= 0) {
                LOG("Wi-Fi server created successfully.\n");
            } else {
                LOG("Failed to create Wi-Fi server.\n");
            }
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_CreateServer(8080);
            LOG("Bluetooth server creation not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_CreateServer(8080);
            LOG("nearlink server creation not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 关闭无线接收服务器
 * @param type 指定无线通信类型。
 * @param server_fd 服务器资源描述符
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_CloseServer(WirelessType type, int server_fd) {
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Close_Server(server_fd);
            if(ret == 0) {
                LOG("Wi-Fi server closed successfully.\n");
            } else {
                LOG("Failed to close Wi-Fi server.\n");
            }
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_CloseServer(server_fd);
            LOG("Bluetooth server close not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_CloseServer(server_fd);
            LOG("nearlink server close not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 通过无线通信模块接收数据
 * @param type 指定无线通信类型。
 * @param server_fd 服务器资源描述符
 * @param[out] mac 存储发送数据的设备的MAC地址
 * @param[out] buffer 存储接收数据的缓冲区
 * @param buffer_len 缓冲区的大小
 * @return 接收到的数据长度，或 < 0 表示失败
 */
int HAL_Wireless_ReceiveDataFromClient(WirelessType type, int server_fd, char *mac, char *buffer, int buffer_len) {
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Server_Receive(server_fd, mac, buffer, buffer_len);
            if(ret >= 0) {
                // LOG("Data received from client %s: %s\n", mac, buffer);
            }
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_ReceiveDataFromClient(server_fd, mac, buffer, buffer_len);
            LOG("Bluetooth data receive from client not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_ReceiveDataFromClient(server_fd, mac, buffer, buffer_len);
            LOG("nearlink data receive from client not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/** 
 * @brief 获取所有子节点的MAC地址
 * @param[out] mac_list 存储MAC地址的指针数组
 * @return 返回子节点的数量，或 < 0 表示失败
 * @note 6字节的MAC地址，最后一位为'\0'，共7字节
 * @note 注意清理分配的地址
 */
int HAL_Wireless_GetChildMACs(WirelessType type, char ***mac_list) {
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_GetAllMAC(mac_list);
            if(ret >= 0) {
                LOG("Successfully retrieved %d child MAC addresses.\n", ret);
            } else {
                LOG("Failed to retrieve child MAC addresses.\n");
            }
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_GetChildMACs(mac_list);
            LOG("Bluetooth child MAC retrieval not implemented.\n");
            break;
        case WIRELESS_TYPE_NEARLINK:
            // ret = HAL_nearlink_GetChildMACs(mac_list);
            LOG("nearlink child MAC retrieval not implemented.\n");
            break;
        default:
            LOG("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}