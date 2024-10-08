#ifndef HAL_WIRELESS_H
#define HAL_WIRELESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "hal_wifi.h"

/** 支持的无线通信类型 */
typedef enum {
    WIRELESS_TYPE_WIFI = 0,          // Wi-Fi 通信
    WIRELESS_TYPE_BLUETOOTH,         // 蓝牙通信
    WIRELESS_TYPE_STARFLASH,         // 星闪通信
    WIRELESS_TYPE_MAX                // 最大类型，标识边界
} WirelessType;

/** 无线连接状态 */
typedef enum {
    WIRELESS_DISCONNECTED,          // 断开连接
    WIRELESS_CONNECTED,             // 已连接
    WIRELESS_CONNECTING,            // 连接中
    WIRELESS_CONN_STATUS_BUTT,      // 枚举越界标记位
} WirelessConnectionStatus;

/** 通用无线接入点扫描结果结构体 */
typedef struct {
    char ssid[33];                // SSID (最多32字节 + 1 结束符)
    uint8_t bssid[6];             // BSSID (AP的MAC地址)
    int16_t rssi;                 // 信号强度 (dBm)
    uint8_t channel;              // 信道
    WirelessType type;            // 无线通信类型 (Wi-Fi、蓝牙、星闪)
} WirelessScanResult;

/** 通用无线已连接设备信息结构体 */
typedef struct {
    uint8_t mac_addr[6];      /*!< MAC地址 */
    int8_t rssi;              /*!< 接收信号强度指示 (RSSI) */
    uint8_t channel;          /*!< 连接信道 */
    WirelessType type;        /*!< 无线通信类型 (Wi-Fi、蓝牙、星闪) */
} WirelessConnectedInfo;

/**
 * @brief 初始化指定类型的无线通信模块及相关资源
 * @param type 指定无线通信类型，如WIFI, BLUETOOTH, STARFLASH。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Init(WirelessType type);

/**
 * @brief 去初始化指定类型的无线通信模块，释放相关资源
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Deinit(WirelessType type);

/**
 * @brief 启动指定类型的无线通信模块，如Wi-Fi STA模式、蓝牙连接模式等。
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Start(WirelessType type);

/**
 * @brief 停止指定类型的无线通信模块，如关闭Wi-Fi、蓝牙断连等。
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Stop(WirelessType type);

/**
 * @brief 扫描指定类型的无线网络
 * @param type 指定无线通信类型（如Wi-Fi、蓝牙）
 * @param results 用于存储扫描结果的数组，调用方需预先分配足够的空间。
 * @param max_results `results` 数组的最大长度，表示最多存储多少个扫描结果。
 * @return 扫描到的无线网络数量，或 < 0 表示失败
 */
int HAL_Wireless_Scan(WirelessType type, WiFiSTAConfig *wifi_config, WiFiScanResult *results, int max_results);

/**
 * @brief 连接到指定的无线设备（如Wi-Fi AP或蓝牙设备）
 * @param config 连接配置信息，可以是不同通信类型的配置结构体（具体定义在各自HAL中）
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Connect(WirelessType type, const void *config);

/**
 * @brief 断开当前的无线连接
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Disconnect(WirelessType type);

/**
 * @brief 获取当前连接状态
 * @param type 指定无线通信类型。
 * @return 当前连接状态，如WIFI_CONNECTED, WIFI_DISCONNECTED等
 */
WirelessConnectionStatus HAL_Wireless_GetConnectionStatus(WirelessType type);

/**
 * @brief 获取当前无线通信模块的IP地址（仅限Wi-Fi）
 * @param ip_buffer 存储IP地址的缓冲区，建议至少分配16字节以存储完整的IPv4地址。
 * @param buffer_len `ip_buffer` 的大小
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_GetIP(WirelessType type, char *ip_buffer, int buffer_len);

/**
 * @brief 获取已连接的无线设备信息，如Wi-Fi的STA或蓝牙的连接设备
 * @param type 指定无线通信类型。
 * @param[out] result 用于存储设备信息的数组
 * @param[in, out] size  输入为数组的最大长度，输出为已连接的设备数量
 * @return 0 表示成功，其他表示失败
 */
int HAL_Wireless_GetConnectedDeviceInfo(WirelessType type, WirelessConnectedInfo *result, uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif // HAL_WIRELESS_H
