#include "hal_wifi.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include <stdlib.h>  // 引入动态内存分配

#define TEST_TARGET_SSID "FsrMesh_2B4277_0"
#define TEST_TARGET_PASSWORD "12345678"
#define MAX_SCAN_RESULTS 64

#define AP_SSID "FsrMesh_2B4276_1"
#define AP_PASSWORD "12345678"
#define AP_CHANNEL 6

#define WIFI_TASK_STACK_SIZE 0x1000
#define WIFI_TASK_PRIO       (osPriority_t)(13)
#define WIFI_RECONNECT_DELAY_MS 100  // 重连等待时间
#define WIFI_READ_STA_INTERVAL_MS 1000 // 读取连接STA信息的间隔时间

char ip[16];

// 任务的入口函数
void sta_sample_task(void *param)
{
    (void)param;
    int result;
    WiFiSTAConfig wifi_config;
    WiFiScanResult *scan_results = NULL;
    WiFiAPConfig ap_config;
    strcpy(wifi_config.ssid, TEST_TARGET_SSID);
    strcpy(wifi_config.password, TEST_TARGET_PASSWORD);
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
        result = HAL_WiFi_Scan(&wifi_config, scan_results, MAX_SCAN_RESULTS);
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
            printf("Failed to connect to Wi-Fi network: %s. Retrying in %d ms...\n", TEST_TARGET_SSID, WIFI_RECONNECT_DELAY_MS * 10);
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
    if (HAL_WiFi_AP_Enable(&ap_config, 1) != 0) {
        printf("Failed to start AP mode.\n");
    } else {
        printf("AP mode started successfully with SSID: %s\n", AP_SSID);
    }

    // 测试发送数据
    // const char *test_data = "Hello, World!";
    const char *test_data = "0\n1\n2B4277";
    char *server_ip = "192.168.0.1";
    uint16_t port = 9001;
    result = HAL_WiFi_Send_data(server_ip, port, test_data);
    if (result != 0) {
        printf("Failed to send data to server.\n");
    } else {
        printf("Data sent successfully to server.\n");
    }

    // 测试接收数据
    char buffer[128];
    char client_ip[16];
    server_ip = "192.168.1.1";
    result = HAL_WiFi_Receive_data(server_ip, port, buffer, sizeof(buffer), client_ip);
    if (result != 0) {
        printf("Failed to receive data from client.\n");
    } else {
        printf("Received data: %s from client: %s\n", buffer, client_ip);
    }

    // 获取AP模式的配置信息
    WiFiAPConfig get_config;
    if (HAL_WiFi_GetAPConfig(&get_config) == 0) {
        printf("AP mode config: SSID: %s, Password: %s, Channel: %d, Security: %d\n",
               get_config.ssid, get_config.password, get_config.channel, get_config.security);
    } else {
        printf("Failed to get AP mode config.\n");
    }

    // 循环读取连接到AP的STA设备信息
    WiFiSTAInfo sta_info[8];
    uint32_t sta_num = 8;

    while (1) {
        sta_num = 8;
        // 获取连接的STA信息
        if (HAL_WiFi_GetConnectedSTAInfo(sta_info, &sta_num) == 0) {
            printf("Connected STA devices: %d\n", sta_num);
            for (uint32_t i = 0; i < sta_num; i++) {
                printf("STA MAC: %02x:%02x:%02x:%02x:%02x:%02x, RSSI: %d, Rate: %u kbps\n",
                       sta_info[i].mac_addr[0], sta_info[i].mac_addr[1], sta_info[i].mac_addr[2],
                       sta_info[i].mac_addr[3], sta_info[i].mac_addr[4], sta_info[i].mac_addr[5],
                       sta_info[i].rssi, sta_info[i].best_rate);
            }
        } else {
            printf("Failed to get connected STA info.\n");
        }
        osDelay(WIFI_READ_STA_INTERVAL_MS / 10);  // 每隔WIFI_READ_STA_INTERVAL_MS读取一次STA信息
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
