#include "hal_wireless.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include <stdlib.h>  // 引入动态内存分配

#define TEST_WIFI_SSID "FsrLab"            // Wi-Fi 测试SSID
#define TEST_WIFI_PASSWORD "12345678"        // Wi-Fi 测试密码
#define MAX_SCAN_RESULTS 64                  // 最大扫描结果数量

#define TEST_BT_TARGET_NAME "TestBTDevice"   // 蓝牙测试目标设备名称
#define TEST_STARFLASH_CHANNEL 11            // 星闪测试信道

#define TASK_STACK_SIZE 0x1000               // 任务栈大小
#define TASK_PRIORITY (osPriority_t)(13)     // 任务优先级
#define TASK_RECONNECT_DELAY_MS 100          // 重连等待时间

char ip[16];  // 用于存储Wi-Fi的IP地址

/**
 * @brief 无线通信测试任务
 * @param param 未使用的任务参数
 */
void wireless_test_task(void *param)
{
    (void)param;
    int result;
    WiFiScanResult *scan_results = NULL;
    WiFiSTAConfig wifi_config = {0};
    // WirelessConfig bluetooth_config = {0};

    // 初始化 Wi-Fi 模块
    result = HAL_Wireless_Init(WIRELESS_TYPE_WIFI);
    if (result != 0) {
        printf("Wi-Fi initialization failed.\n");
        return;
    }
    printf("Wi-Fi initialized successfully.\n");

    // 启动 Wi-Fi STA 模式
    result = HAL_Wireless_Start(WIRELESS_TYPE_WIFI);
    if (result != 0) {
        printf("Failed to start Wi-Fi.\n");
        return;
    }
    printf("Wi-Fi started successfully.\n");

    // 动态分配内存给扫描结果数组
    scan_results = (WiFiScanResult *)malloc(sizeof(WiFiScanResult) * MAX_SCAN_RESULTS);
    if (scan_results == NULL) {
        printf("Failed to allocate memory for scan results.\n");
        return;
    }

    // Wi-Fi 扫描
    printf("Scanning for Wi-Fi networks...\n");
    result = HAL_Wireless_Scan(WIRELESS_TYPE_WIFI, TEST_WIFI_SSID, TEST_WIFI_PASSWORD, &wifi_config, scan_results, MAX_SCAN_RESULTS);
    if (result < 0) {
        printf("Wi-Fi scan failed or no networks found.\n");
    } else {
        printf("Found %d Wi-Fi networks.\n", result);
        for (int i = 0; i < result; i++) {
            printf("SSID: %s, RSSI: %d, Channel: %d\n",
                   scan_results[i].ssid, scan_results[i].rssi, scan_results[i].channel);
        }
    }

    // 连接到指定 Wi-Fi 网络
    strncpy(wifi_config.ssid, TEST_WIFI_SSID, sizeof(wifi_config.ssid) - 1);
    strncpy(wifi_config.password, TEST_WIFI_PASSWORD, sizeof(wifi_config.password) - 1);
    printf("Connecting to Wi-Fi network: %s...\n", TEST_WIFI_SSID);
    result = HAL_Wireless_Connect(WIRELESS_TYPE_WIFI, &wifi_config);
    if (result != 0) {
        printf("Failed to connect to Wi-Fi network: %s.\n", TEST_WIFI_SSID);
    } else {
        printf("Successfully connected to Wi-Fi network: %s.\n", TEST_WIFI_SSID);
        HAL_Wireless_GetIP(WIRELESS_TYPE_WIFI, ip, sizeof(ip));
        printf("Wi-Fi IP is %s\n", ip);
    }

    // // 测试蓝牙连接
    // printf("Initializing Bluetooth...\n");
    // result = HAL_Wireless_Init(WIRELESS_TYPE_BLUETOOTH);
    // if (result != 0) {
    //     printf("Bluetooth initialization failed.\n");
    //     return;
    // }
    // printf("Bluetooth initialized successfully.\n");

    // strncpy(bluetooth_config.ssid, TEST_BT_TARGET_NAME, sizeof(bluetooth_config.ssid) - 1);  // 使用ssid字段存储蓝牙目标名称
    // printf("Connecting to Bluetooth device: %s...\n", TEST_BT_TARGET_NAME);
    // result = HAL_Wireless_Connect(WIRELESS_TYPE_BLUETOOTH, &bluetooth_config);
    // if (result != 0) {
    //     printf("Failed to connect to Bluetooth device: %s.\n", TEST_BT_TARGET_NAME);
    // } else {
    //     printf("Successfully connected to Bluetooth device: %s.\n", TEST_BT_TARGET_NAME);
    // }

    // // 测试星闪通信
    // printf("Initializing StarFlash...\n");
    // result = HAL_Wireless_Init(WIRELESS_TYPE_STARFLASH);
    // if (result != 0) {
    //     printf("StarFlash initialization failed.\n");
    //     return;
    // }
    // printf("StarFlash initialized successfully.\n");

    // WirelessConfig starflash_config = {0};
    // starflash_config.channel = TEST_STARFLASH_CHANNEL;
    // printf("Connecting to StarFlash network on channel: %d...\n", TEST_STARFLASH_CHANNEL);
    // result = HAL_Wireless_Connect(WIRELESS_TYPE_STARFLASH, &starflash_config);
    // if (result != 0) {
    //     printf("Failed to connect to StarFlash network.\n");
    // } else {
    //     printf("Successfully connected to StarFlash network on channel: %d.\n", TEST_STARFLASH_CHANNEL);
    // }

    // 清理资源
    HAL_Wireless_Stop(WIRELESS_TYPE_WIFI);
    HAL_Wireless_Deinit(WIRELESS_TYPE_WIFI);
    // HAL_Wireless_Stop(WIRELESS_TYPE_BLUETOOTH);
    // HAL_Wireless_Deinit(WIRELESS_TYPE_BLUETOOTH);
    // HAL_Wireless_Stop(WIRELESS_TYPE_STARFLASH);
    // HAL_Wireless_Deinit(WIRELESS_TYPE_STARFLASH);

    // 释放动态分配的内存
    free(scan_results);
}

/**
 * @brief 创建无线通信测试任务
 */
static void wireless_test_entry(void)
{
    osThreadAttr_t attr;
    attr.name       = "wireless_test_task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = TASK_STACK_SIZE;
    attr.priority   = TASK_PRIORITY;

    if (osThreadNew((osThreadFunc_t)wireless_test_task, NULL, &attr) == NULL) {
        printf("Create wireless_test_task failed.\n");
    } else {
        printf("Create wireless_test_task successfully.\n");
    }
}

/* 启动无线通信测试任务 */
app_run(wireless_test_entry);
