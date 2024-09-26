#include "hal_wifi.h"
#include "wifi_device.h"
#include "td_type.h"
#include "td_base.h"
#include "std_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cmsis_os2.h"
#include "soc_osal.h"
#include "securec.h"
#include "wifi_device_config.h"


// 定义宏，用于自动传递 __FILE__, __func__, __LINE__ 给打印函数
#define perror printf
#define WIFI_NOT_AVALLIABLE              0
#define WIFI_AVALIABE                    1
#define WIFI_SCAN_AP_LIMIT               64
#define SCAN_TIMEOUT_MS 5000            // 最大扫描等待时间（毫秒）
#define SCAN_POLL_INTERVAL_MS 10        // 每次轮询扫描状态的时间间隔（毫秒）

static td_void wifi_scan_state_changed(td_s32 state, td_s32 size);
static td_void wifi_connection_changed(td_s32 state, const wifi_linked_info_stru *info, td_s32 reason_code);

wifi_event_stru wifi_event_cb = {
    .wifi_event_connection_changed      = wifi_connection_changed,
    .wifi_event_scan_state_changed      = wifi_scan_state_changed,
};

enum {
    WIFI_STA_SAMPLE_INIT = 0,       /* 0:初始态 */
    WIFI_STA_SAMPLE_SCANING,        /* 1:扫描中 */
    WIFI_STA_SAMPLE_SCAN_DONE,      /* 2:扫描完成 */
    WIFI_STA_SAMPLE_FOUND_TARGET,   /* 3:匹配到目标AP */
    WIFI_STA_SAMPLE_CONNECTING,     /* 4:连接中 */
    WIFI_STA_SAMPLE_CONNECT_DONE,   /* 5:关联成功 */
    WIFI_STA_SAMPLE_GET_IP,         /* 6:获取IP */
} wifi_state_enum;

static td_u8 g_wifi_state = WIFI_STA_SAMPLE_INIT;

/*****************************************************************************
  STA 扫描事件回调函数
*****************************************************************************/
static td_void wifi_scan_state_changed(td_s32 state, td_s32 size)
{
    UNUSED(state);
    UNUSED(size);
    g_wifi_state = WIFI_STA_SAMPLE_SCAN_DONE;
    return;
}

/*****************************************************************************
  STA 关联事件回调函数
*****************************************************************************/
static td_void wifi_connection_changed(td_s32 state, const wifi_linked_info_stru *info, td_s32 reason_code)
{
    UNUSED(info);
    UNUSED(reason_code);

    if (state == WIFI_NOT_AVALLIABLE) {
        perror("Connect fail!. try agin !\r\n");
        g_wifi_state = WIFI_STA_SAMPLE_INIT;
    } else {
        g_wifi_state = WIFI_STA_SAMPLE_CONNECT_DONE;
    }
}

int HAL_WiFi_Init(void)
{

    /* 注册事件回调 */
    if (wifi_register_event_cb(&wifi_event_cb) != 0) {
        perror("wifi_event_cb register fail.\r\n");
        return -1;
    }

    /* 等待wifi初始化完成 */
    while (wifi_is_wifi_inited() == 0) {
        (void)osDelay(10); /* 等待100ms后判断状态 */
    }

    // SDK在其他地方调用了wifi_init,在此不再调用该函数
    return 0;
}

int HAL_WiFi_Deinit(void)
{
    // 一般不调用这个函数
    if(wifi_deinit()==ERRCODE_FAIL)
    {
        return -1;
    }else{
        return 0;
    }
}

int HAL_WiFi_STA_Enable(void)
{
    if (wifi_sta_enable() != 0) {
        perror("wifi_sta_enable fail.\r\n");
        return -1;
    }else{
        return 0;
    }
}

int HAL_WiFi_STA_Disable(void)
{
    if (wifi_sta_disable() != 0) {
        perror("wifi_sta_disable fail.\r\n");
        return -1;
    }else{
        return 0;
    }
}

int HAL_WiFi_Scan(const char *target_ssid, const char *target_password, WiFiSTAConfig *wifi_config, WiFiScanResult *results, int max_results)
{
    if (target_ssid == NULL || target_password == NULL || wifi_config == NULL || results == NULL || max_results <= 0) {
        perror("Invalid input.\r\n");
        return -1;
    }

    g_wifi_state = WIFI_STA_SAMPLE_SCANING;

    // 启动扫描
    if (wifi_sta_scan() != 0) {
        perror("Wi-Fi scan failed to start.\r\n");
        return -1;
    }

    // 等待扫描完成或超时
    int elapsed_time = 0;
    while (g_wifi_state != WIFI_STA_SAMPLE_SCAN_DONE) {
        osDelay(SCAN_POLL_INTERVAL_MS); // 每次等待 10 毫秒
        elapsed_time += SCAN_POLL_INTERVAL_MS;

        if (elapsed_time >= SCAN_TIMEOUT_MS) {
            perror("Wi-Fi scan timed out.\r\n");
            return -1;
        }
    }

    // 获取扫描结果
    td_u32 scan_len = sizeof(wifi_scan_info_stru) * WIFI_SCAN_AP_LIMIT;
    wifi_scan_info_stru *scan_results = osal_kmalloc(scan_len, OSAL_GFP_ATOMIC);
    if (scan_results == TD_NULL) {
        perror("Failed to allocate memory for scan results.\r\n");
        return -1;
    }

    memset_s(scan_results, scan_len, 0, scan_len);
    uint32_t num_results = WIFI_SCAN_AP_LIMIT;
    if (wifi_sta_get_scan_info(scan_results, &num_results) != 0) {
        perror("Failed to get Wi-Fi scan results.\r\n");
        osal_kfree(scan_results);
        return -1;
    }

    // 匹配目标 SSID 并设置 Wi-Fi 配置
    td_bool found_ap = TD_FALSE;
    for (uint32_t i = 0; i < num_results && i < (uint32_t)max_results; i++) {
        // 将扫描结果拷贝到 results 数组中
        strncpy(results[i].ssid, scan_results[i].ssid, sizeof(results[i].ssid) - 1);
        memcpy(results[i].bssid, scan_results[i].bssid, WIFI_BSSID_LEN);
        results[i].rssi = scan_results[i].rssi;
        
        // 将驱动中的安全类型转换为 WiFiSecurityType
        switch (scan_results[i].security_type) {
            case WIFI_SEC_TYPE_OPEN:
                results[i].security = WIFI_SECURITY_OPEN;
                break;
            case WIFI_SEC_TYPE_WEP:
                results[i].security = WIFI_SECURITY_WEP;
                break;
            case WIFI_SEC_TYPE_WPA2PSK:
                results[i].security = WIFI_SECURITY_WPA2_PSK;
                break;
            case WIFI_SEC_TYPE_WPA2_WPA_PSK_MIX:
                results[i].security = WIFI_SECURITY_WPA2_WPA_PSK_MIX;
                break;
            case WIFI_SEC_TYPE_WPAPSK:
                results[i].security = WIFI_SECURITY_WPA_PSK;
                break;
            case WIFI_SEC_TYPE_WPA:
                results[i].security = WIFI_SECURITY_WPA;
                break;
            case WIFI_SEC_TYPE_WPA2:
                results[i].security = WIFI_SECURITY_WPA2;
                break;
            case WIFI_SEC_TYPE_SAE:
                results[i].security = WIFI_SECURITY_WPA3_SAE;
                break;
            case WIFI_SEC_TYPE_WPA3_WPA2_PSK_MIX:
                results[i].security = WIFI_SECURITY_WPA3_WPA2_MIX;
                break;
            case WIFI_SEC_TYPE_WPA3:
                results[i].security = WIFI_SECURITY_WPA3;
                break;
            case WIFI_SEC_TYPE_OWE:
                results[i].security = WIFI_SECURITY_OWE;
                break;
            default:
                results[i].security = WIFI_SECURITY_UNKNOWN;
                break;
        }

        results[i].channel = scan_results[i].channel_num;

        // 匹配目标 SSID
        if (strcmp(target_ssid, scan_results[i].ssid) == 0) {
            strncpy(wifi_config->ssid, scan_results[i].ssid, sizeof(wifi_config->ssid) - 1);
            strncpy(wifi_config->password, target_password, sizeof(wifi_config->password) - 1);
            memcpy(wifi_config->bssid, scan_results[i].bssid, WIFI_BSSID_LEN);
            wifi_config->security_type = results[i].security;
            found_ap = TD_TRUE;
        }
    }

    osal_kfree(scan_results);

    if (!found_ap) {
        perror("Failed to find the target SSID: %s\r\n", target_ssid);
        return -1;
    }

    return num_results;
}

int HAL_WiFi_Connect(const WiFiSTAConfig *wifi_config)
{
    if (wifi_config == NULL) {
        perror("Invalid input: wifi_config is NULL.\r\n");
        return -1;
    }

    // 配置硬件相关的 Wi-Fi 连接参数
    wifi_sta_config_stru hw_config;
    memset(&hw_config, 0, sizeof(hw_config));
    strncpy((char *)hw_config.ssid, (const char *)wifi_config->ssid, sizeof(hw_config.ssid) - 1);
    strncpy((char *)hw_config.pre_shared_key, (const char *)wifi_config->password, sizeof(hw_config.pre_shared_key) - 1);
    memcpy(hw_config.bssid, wifi_config->bssid, WIFI_BSSID_LEN);
    hw_config.security_type = (wifi_security_enum)wifi_config->security_type;  // 转换为硬件驱动的安全类型
    hw_config.ip_type = 1;
    // 启动 STA 模式并连接
    if (wifi_sta_connect(&hw_config) != 0) {
        perror("Wi-Fi connect failed.\r\n");
        return -1;
    }

    return 0; // 成功连接
}