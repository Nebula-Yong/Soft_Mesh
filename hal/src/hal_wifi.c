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
#include "lwip/netifapi.h"
#include "wifi_hotspot.h"
#include "wifi_hotspot_config.h"
#include "lwip/etharp.h"


// 定义宏，用于自动传递 __FILE__, __func__, __LINE__ 给打印函数
#define perror printf
#define WIFI_NOT_AVALLIABLE              0
#define WIFI_AVALIABE                    1
#define WIFI_SCAN_AP_LIMIT               64
#define SCAN_TIMEOUT_MS                  5000      // 最大扫描等待时间（毫秒）
#define SCAN_POLL_INTERVAL_MS            10        // 每次轮询扫描状态的时间间隔（毫秒）
#define WIFI_GET_IP_MAX_COUNT            300

extern osEventFlagsId_t wireless_event_flags;
#define WIRELESS_CONNECT_BIT    (1 << 0)
#define WIRELESS_DISCONNECT_BIT (1 << 1)

static td_void wifi_scan_state_changed(td_s32 state, td_s32 size);
static td_void wifi_connection_changed(td_s32 state, const wifi_linked_info_stru *info, td_s32 reason_code);
static char g_sta_ip[16] = {0};  // 保存 STA 模式的 IP 地址

wifi_event_stru wifi_event_cb = {
    .wifi_event_connection_changed      = wifi_connection_changed,
    .wifi_event_scan_state_changed      = wifi_scan_state_changed,
};

enum {
    WIFI_STA_SAMPLE_INIT = 0,       /* 0:初始态 */
    WIFI_STA_SAMPLE_SCANING,        /* 1:扫描中 */
    WIFI_STA_SAMPLE_SCAN_DONE,      /* 2:扫描完成 */
    WIFI_STA_SAMPLE_CONNECT_DONE    /* 5:关联成功 */
} wifi_state_enum;

void CreateWirelessEventFlags(void) {
    wireless_event_flags = osEventFlagsNew(NULL);  // 使用默认属性创建事件标志
    if (wireless_event_flags == NULL) {
        perror("Failed to create Wi-Fi event flags.\r\n");
    }
}

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
        osEventFlagsSet(wireless_event_flags, WIRELESS_DISCONNECT_BIT);
    } else {
        g_wifi_state = WIFI_STA_SAMPLE_CONNECT_DONE;
        osEventFlagsSet(wireless_event_flags, WIRELESS_CONNECT_BIT);
    }
}

int HAL_WiFi_Init(void)
{
    CreateWirelessEventFlags();

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

/*****************************************************************************
  STA DHCP状态查询
*****************************************************************************/
td_bool example_check_dhcp_status(struct netif *netif_p, td_u32 *wait_count)
{
    if ((ip_addr_isany(&(netif_p->ip_addr)) == 0) && (*wait_count <= WIFI_GET_IP_MAX_COUNT)) {
        /* DHCP成功 */
        return 0;
    }

    if (*wait_count > WIFI_GET_IP_MAX_COUNT) {
        perror("%s::STA DHCP timeout, try again !.\r\n");
        *wait_count = 0;
        g_wifi_state = WIFI_STA_SAMPLE_INIT;
    }
    return -1;
}

/*****************************************************************************
 存储IP信息
*****************************************************************************/
void store_ip(struct netif *netif_p)
{
    #define WIFI_STA_IP_MAX_GET_TIMES 5
    for (uint8_t i = 0; i < WIFI_STA_IP_MAX_GET_TIMES; i++) {
        (void)osal_msleep(10);  /* 延时 10ms */
        if (netif_p->ip_addr.u_addr.ip4.addr != 0) {
            snprintf(g_sta_ip, sizeof(g_sta_ip), "%u.%u.%u.%u", 
                     (netif_p->ip_addr.u_addr.ip4.addr & 0x000000ff),
                     (netif_p->ip_addr.u_addr.ip4.addr & 0x0000ff00) >> 8,
                     (netif_p->ip_addr.u_addr.ip4.addr & 0x00ff0000) >> 16,
                     (netif_p->ip_addr.u_addr.ip4.addr & 0xff000000) >> 24);
            break;
        }
    }
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

int HAL_WiFi_Scan(WiFiSTAConfig *wifi_config, WiFiScanResult *results, int max_results)
{
    if (wifi_config == NULL || results == NULL || max_results <= 0) {
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
        if (strcmp(wifi_config->ssid, scan_results[i].ssid) == 0) {
            memcpy(wifi_config->bssid, scan_results[i].bssid, WIFI_BSSID_LEN);
            wifi_config->security_type = results[i].security;
            found_ap = TD_TRUE;
        }
    }

    osal_kfree(scan_results);

    if (!found_ap) {
        perror("Failed to find the target SSID: %s\r\n", wifi_config->ssid);
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
    hw_config.ip_type = 1; /* 1：IP类型为动态DHCP获取 */
    // 启动 STA 模式并连接
    if (wifi_sta_connect(&hw_config) != 0) {
        perror("Wi-Fi connect failed.\r\n");
        return -1;
    }

    // 定义连接超时时间（例如 10 秒）
    const int CONNECT_TIMEOUT_MS = 500;  // 10 秒超时
    const int POLL_INTERVAL_MS = 100;      // 每次等待 100 毫秒
    int elapsed_time = 0;

    // 等待 Wi-Fi 连接状态更新
    while (g_wifi_state != WIFI_STA_SAMPLE_CONNECT_DONE) {
        osDelay(POLL_INTERVAL_MS);  // 延迟 100ms
        elapsed_time += POLL_INTERVAL_MS;

        // 检查是否超时
        if (elapsed_time >= CONNECT_TIMEOUT_MS) {
            perror("Wi-Fi connection timed out.\r\n");
            return -1;
        }
    }

    // 获取网卡接口名
    const char *ifname = "wlan0";  // 假设网卡接口名为wlan0，根据实际调整
    struct netif *netif_p = netifapi_netif_find(ifname);
    if (netif_p == NULL) {
        perror("Network interface not found: %s\n", ifname);
        return -1;
    }

    // 启动 DHCP 获取 IP 地址
    if (netifapi_dhcp_start(netif_p) != 0) {
        perror("Failed to start DHCP on interface: %s\n", ifname);
        return -1;
    }

    // 等待获取 IP 地址
    td_u32 wait_count = 0;
    while (1) {
        osDelay(1);  // 10ms 延迟
        if (example_check_dhcp_status(netif_p, &wait_count) == 0) {
            break;
        }
        if (wait_count > WIFI_GET_IP_MAX_COUNT) {
            perror("DHCP timeout on interface: %s\n", ifname);
            return -1;
        }
    }

    // 存储获取到的 IP 地址
    store_ip(netif_p);
    return 0;  
}

int HAL_WiFi_GetIP(char *ip_buffer, int buffer_len)
{
    // 检查输入参数是否合法
    if (ip_buffer == NULL || buffer_len <= 0) {
        printf("Invalid input: ip_buffer is NULL or buffer_len is invalid.\n");
        return -1;
    }

    // 检查全局变量 g_sta_ip 中是否存有 IP 地址
    if (g_sta_ip[0] == '\0') {
        printf("No IP address available.\n");
        return -1;
    }

    // 检查缓冲区大小是否足够
    if (buffer_len < 16) {
        printf("Buffer size too small. Needs at least 16 bytes.\n");
        return -1;
    }

    // 将全局变量中的 IP 地址复制到传入的缓冲区中
    strncpy(ip_buffer, g_sta_ip, buffer_len - 1);
    ip_buffer[buffer_len - 1] = '\0';  // 确保字符串以 NULL 结尾

    return 0;  // 成功
}

int HAL_WiFi_Disconnect(void)
{
    if(wifi_sta_disconnect() != 0)
    {
        perror("wifi disconnect fail.\r\n");
        return -1;
    }else{
        return 0;
    }
}

int HAL_WiFi_AP_Enable(WiFiAPConfig *config, int tree_level) {
    if (config == NULL) {
        perror("Invalid input: config is NULL.\n");
        return -1;
    }

    // 创建 SoftAP 配置
    softap_config_stru hapd_conf = {0};
    softap_config_advance_stru advanced_conf = {0};
    td_char ifname[WIFI_IFNAME_MAX_SIZE + 1] = "ap0"; // SoftAp接口名
    struct netif *netif_p = NULL;
    ip4_addr_t st_gw, st_ipaddr, st_netmask;

    // 设置SoftAp的IP地址，子网掩码和网关
    IP4_ADDR(&st_ipaddr, 192, 168, 43, tree_level);  // IP地址：192.168.43.X
    IP4_ADDR(&st_netmask, 255, 255, 255, 0); // 子网掩码：255.255.255.0
    IP4_ADDR(&st_gw, 192, 168, 43, 1);      // 网关：192.168.43.1

    // 基本SoftAp配置
    strncpy((char *)hapd_conf.ssid, (const char *)config->ssid, sizeof(hapd_conf.ssid) - 1);
    strncpy((char *)hapd_conf.pre_shared_key, (const char *)config->password, sizeof(hapd_conf.pre_shared_key) - 1);
    hapd_conf.security_type = (td_u8)config->security;
    hapd_conf.channel_num = config->channel;
    hapd_conf.wifi_psk_type = 0;  // 假设PSK类型为0
    printf("hapd_conf.channel_num:%d\r\n", hapd_conf.channel_num);

    // 高级SoftAp配置
    advanced_conf.beacon_interval = 100;     // Beacon周期设置为100ms
    advanced_conf.dtim_period = 2;           // DTIM周期设置为2
    advanced_conf.gi = 0;                    // Short GI默认关闭
    advanced_conf.group_rekey = 86400;       // 组播秘钥更新时间设置为1天
    advanced_conf.protocol_mode = 4;         // 802.11b/g/n/ax协议模式
    advanced_conf.hidden_ssid_flag = 1;      // SSID不隐藏

    // 设置SoftAp的高级配置
    if (wifi_set_softap_config_advance(&advanced_conf) != 0) {
        perror("Failed to set advanced SoftAP configuration.\n");
        return -1;
    }

    // 启动SoftAp接口
    if (wifi_softap_enable(&hapd_conf) != 0) {
        perror("Failed to enable SoftAP.\n");
        return -1;
    }

    // 获取网络接口
    netif_p = netif_find(ifname);
    if (netif_p == NULL) {
        perror("Failed to find network interface: %s.\n", ifname);
        (void)wifi_softap_disable();
        return -1;
    }

    // 设置IP地址、子网掩码和网关
    if (netifapi_netif_set_addr(netif_p, &st_ipaddr, &st_netmask, &st_gw) != 0) {
        perror("Failed to set network interface address.\n");
        (void)wifi_softap_disable();
        return -1;
    }

    // 启动DHCP服务器
    if (netifapi_dhcps_start(netif_p, NULL, 0) != 0) {
        perror("Failed to start DHCP server.\n");
        (void)wifi_softap_disable();
        return -1;
    }

    printf("SoftAP started successfully with SSID: %s\n", config->ssid);
    return 0;
}

int HAL_WiFi_AP_Disable(void) {
    // 停止 SoftAp 模式
    if (wifi_softap_disable() != 0) {
        perror("Failed to disable SoftAP mode.\n");
        return -1;
    }

    perror("SoftAP mode disabled.\n");
    return 0;
}

int HAL_WiFi_GetConnectedSTAInfo(WiFiSTAInfo *result, uint32_t *size) {
    if (result == NULL || size == NULL || (*size) == 0) {
        printf("Invalid input parameters.\n");
        return -1;
    }
    wifi_sta_info_stru* sta_info = (wifi_sta_info_stru*)malloc(sizeof(wifi_sta_info_stru) * (*size));
    if (sta_info == NULL) {
        printf("Memory allocation failed.\n");
        return -1;
    }

    // 调用底层API获取已连接的STA信息
    int ret = wifi_softap_get_sta_list(sta_info, size);
    for(uint32_t i =0; i < (*size); i++)
    {
        result[i].best_rate = sta_info[i].best_rate;
        memcpy(result[i].mac_addr, sta_info[i].mac_addr, WIFI_BSSID_LEN);
        result[i].rssi = sta_info[i].rssi;
    }

    if (ret != 0) {
        printf("Failed to get connected STA info.\n");
        return -1;
    }

    // 成功返回STA列表信息
    return 0;
}

int HAL_WiFi_GetAPMacAddress(uint8_t *mac_addr) {
    if (mac_addr == NULL) {
        printf("Invalid input: mac_addr is NULL.\n");
        return -1;
    }
    int8_t temp_mac[6] = {0};
    // 获取 SoftAP MAC 地址
    if (wifi_softap_get_mac_addr(temp_mac, 6) != 0) {
        printf("Failed to get SoftAP MAC address.\n");
        return -1;
    }
    // 将 MAC 地址拷贝到输出参数中
    memcpy(mac_addr, temp_mac, 6);
    return 0;
}