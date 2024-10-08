#include "hal_wireless.h"
#include "hal_wifi.h"
// #include "hal_bluetooth.h"
// #include "hal_nearlink.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 初始化指定类型的无线通信模块及相关资源
 * @param type 指定无线通信类型，如WIFI, BLUETOOTH, STARFLASH。
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
            // ret = HAL_Bluetooth_Init();  // 调用蓝牙模块的初始化函数
            break;
        case WIRELESS_TYPE_STARFLASH:
            // ret = HAL_StarFlash_Init();  // 调用星闪模块的初始化函数
            break;
        default:
            printf("Unknown wireless type!\n");
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
            // ret = HAL_Bluetooth_Deinit();  // 调用蓝牙模块的去初始化函数
            break;
        case WIRELESS_TYPE_STARFLASH:
            // ret = HAL_StarFlash_Deinit();  // 调用星闪模块的去初始化函数
            break;
        default:
            printf("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 启动指定类型的无线通信模块，如Wi-Fi STA模式、蓝牙连接模式等。
 * @param type 指定无线通信类型。
 * @param config 指向不同无线通信类型的配置结构体（如Wi-Fi的WiFiSTAConfig）。
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
            // ret = HAL_Bluetooth_Enable((BluetoothConfig *)config);  // 启动蓝牙连接模式
            break;
        case WIRELESS_TYPE_STARFLASH:
            // ret = HAL_StarFlash_Enable((StarFlashConfig *)config);  // 启动星闪连接模式
            break;
        default:
            printf("Unknown wireless type!\n");
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
            // ret = HAL_Bluetooth_Disable();  // 停止蓝牙连接模式
            break;
        case WIRELESS_TYPE_STARFLASH:
            // ret = HAL_StarFlash_Disable();  // 停止星闪连接模式
            break;
        default:
            printf("Unknown wireless type!\n");
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
int HAL_Wireless_Connect(WirelessType type, const void *config)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Connect((WiFiSTAConfig *)config);  // 连接Wi-Fi AP
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_Connect((BluetoothConfig *)config);  // 连接蓝牙设备
            break;
        case WIRELESS_TYPE_STARFLASH:
            // ret = HAL_StarFlash_Connect((StarFlashConfig *)config);  // 连接星闪设备
            break;
        default:
            printf("Unknown wireless type!\n");
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
            // ret = HAL_Bluetooth_Disconnect();  // 断开蓝牙连接
            break;
        case WIRELESS_TYPE_STARFLASH:
            // ret = HAL_StarFlash_Disconnect();  // 断开星闪连接
            break;
        default:
            printf("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}

/**
 * @brief 获取当前连接状态
 * @param type 指定无线通信类型。
 * @return 当前连接状态，如WIFI_CONNECTED, WIFI_DISCONNECTED等
 */
int HAL_Wireless_GetIP(WirelessType type, char *ip_buffer, int buffer_len){
    if(type != WIRELESS_TYPE_WIFI){
        printf("Only Wi-Fi supports getting IP address!\n");
        return -1;
    }
    HAL_WiFi_GetIP(ip_buffer, buffer_len);
    return 0;
}

/**
 * @brief 扫描指定类型的无线网络
 * @param type 指定无线通信类型（如Wi-Fi、蓝牙）
 * @param results 用于存储扫描结果的数组，调用方需预先分配足够的空间。
 * @param max_results `results` 数组的最大长度，表示最多存储多少个扫描结果。
 * @return 扫描到的无线网络数量，或 < 0 表示失败
 */
int HAL_Wireless_Scan(WirelessType type, WiFiSTAConfig *wifi_config, WiFiScanResult *results, int max_results)
{
    int ret = -1;
    switch (type) {
        case WIRELESS_TYPE_WIFI:
            ret = HAL_WiFi_Scan(wifi_config, results, max_results);  // 扫描Wi-Fi网络
            break;
        case WIRELESS_TYPE_BLUETOOTH:
            // ret = HAL_Bluetooth_Scan(results, max_results);  // 扫描蓝牙设备
            break;
        case WIRELESS_TYPE_STARFLASH:
            // ret = HAL_StarFlash_Scan(results, max_results);  // 扫描星闪设备
            break;
        default:
            printf("Unknown wireless type!\n");
            return -1;
    }
    return ret;
}
