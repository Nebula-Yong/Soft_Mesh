#include "hal_wifi.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include <stdlib.h>  // 引入动态内存分配

#define TEST_TARGET_SSID "FsrLab"
#define TEST_TARGET_PASSWORD "12345678"
#define MAX_SCAN_RESULTS 64

#define AP_SSID "My_AP"
#define AP_PASSWORD "ap_password"
#define AP_CHANNEL 6

#define WIFI_TASK_STACK_SIZE 0x1000
#define WIFI_TASK_PRIO       (osPriority_t)(13)
#define WIFI_RECONNECT_DELAY_MS 100  // 重连等待时间 (1秒)
#define WIFI_SHUTDOWN_DELAY_MS 1000 // 延迟10秒后关闭STA和AP

char ip[16];

// 任务的入口函数
void sta_sample_task(void *param)
{
    (void)param;
    int result;
    WiFiSTAConfig wifi_config;
    WiFiScanResult *scan_results = NULL;
    WiFiAPConfig ap_config;

    // 初始化 Wi-Fi
    result = HAL_WiFi_Init();
    if (result != 0) {
        printf("Wi-Fi initialization failed.\n");
        return;
    }
    printf("Wi-Fi initialized successfully.\n");

    // 启动 STA 模式
    result = HAL_WiFi_STA_Enable();
    if (result != 0) {
        printf("Failed to enable STA mode.\n");
        return;
    }
    printf("STA mode enabled successfully.\n");

    // 动态分配内存给扫描结果数组
    scan_results = (WiFiScanResult *)malloc(sizeof(WiFiScanResult) * MAX_SCAN_RESULTS);
    if (scan_results == NULL) {
        printf("Failed to allocate memory for scan results.\n");
        return;
    }

    while (1) {
        // 扫描 Wi-Fi 网络
        printf("Scanning for Wi-Fi networks...\n");
        result = HAL_WiFi_Scan(TEST_TARGET_SSID, TEST_TARGET_PASSWORD, &wifi_config, scan_results, MAX_SCAN_RESULTS);
        if (result < 0) {
            printf("Wi-Fi scan failed or target SSID not found.\n");
            osDelay(WIFI_RECONNECT_DELAY_MS);  // 等待一段时间再重试
            continue;  // 继续下一次扫描
        }

        printf("Found %d Wi-Fi networks.\n", result);
        for (int i = 0; i < result; i++) {
            printf("SSID: %s, RSSI: %d, Security: %d, Channel: %d\n",
                   scan_results[i].ssid, scan_results[i].rssi, scan_results[i].security, scan_results[i].channel);
        }

        // 尝试连接到目标 Wi-Fi 网络
        printf("Connecting to Wi-Fi network: %s...\n", TEST_TARGET_SSID);
        result = HAL_WiFi_Connect(&wifi_config);
        if (result != 0) {
            printf("Failed to connect to Wi-Fi network: %s. Retrying in %d ms...\n", TEST_TARGET_SSID, WIFI_RECONNECT_DELAY_MS*10);
            osDelay(WIFI_RECONNECT_DELAY_MS);  // 等待一段时间再重试
            continue;  // 继续下一次重试
        }

        printf("Successfully connected to Wi-Fi network: %s.\n", TEST_TARGET_SSID);
        
        HAL_WiFi_GetIP(ip, 16);  // 获取STA模式的IP地址
        printf("STA IP is %s\n", ip);
        break;  // 成功连接后退出循环
    }

    // 配置AP模式参数
    strncpy(ap_config.ssid, AP_SSID, sizeof(ap_config.ssid) - 1);
    strncpy(ap_config.password, AP_PASSWORD, sizeof(ap_config.password) - 1);
    ap_config.channel = AP_CHANNEL;
    ap_config.security = WIFI_SECURITY_WPA2_PSK;

    // 启动AP模式
    printf("Starting AP mode with SSID: %s\n", AP_SSID);
    if (HAL_WiFi_AP_Enable(&ap_config) != 0) {
        printf("Failed to start AP mode.\n");
    } else {
        printf("AP mode started successfully with SSID: %s\n", AP_SSID);
    }

    // 保持STA和AP模式一段时间后再关闭
    osDelay(WIFI_SHUTDOWN_DELAY_MS);  // 等待指定时间

    // 关闭STA和AP模式
    if (HAL_WiFi_Disconnect() != 0) {
        printf("Failed to disconnect STA.\n");
    } else {
        printf("STA disconnected successfully.\n");
    }

    if (HAL_WiFi_AP_Disable() != 0) {
        printf("Failed to disable AP mode.\n");
    } else {
        printf("AP mode disabled successfully.\n");
    }

    // 释放动态分配的内存
    free(scan_results);
}

/* 创建任务 */
static void sta_sample_entry(void)
{
    osThreadAttr_t attr;
    attr.name       = "sta_sample_task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = WIFI_TASK_STACK_SIZE;
    attr.priority   = WIFI_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)sta_sample_task, NULL, &attr) == NULL) {
        printf("Create sta_sample_task failed.\n");
    } else {
        printf("Create sta_sample_task successfully.\n");
    }
}

/* 启动任务 */
app_run(sta_sample_entry);
