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
//socket相关库文件
#include "lwip/sockets.h"
#include <sys/time.h>


// 定义宏开关，打开或关闭日志输出
#define ENABLE_LOG 0  // 1 表示开启日志，0 表示关闭日志

// 定义 LOG 宏，如果 ENABLE_LOG 为 1，则打印日志，并输出文件名、行号、日志内容
#if ENABLE_LOG
    #define LOG(fmt, ...) printf("LOG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...) do { UNUSED(fmt); } while (0) 
#endif

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

void remove_mac_ip_binding(const char *mac);
void delete_mac_ip_list(void);

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
        LOG("Failed to create Wi-Fi event flags.\r\n");
    }
}

static td_u8 g_wifi_state = WIFI_STA_SAMPLE_INIT;

// 链表的头节点（全局变量，初始化为空）
MAC_IP_Node *head = NULL;
uint32_t len_mac_ip_list = 0;

// 用于控制服务器是否继续运行的全局标志
volatile bool server_running = true;
volatile bool client_running = false;

osThreadId_t IP_MAC_thread_id;
osThreadId_t heart_beat_thread_id;
int tree_level = 0;

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
        LOG("Connect fail!. try agin !\r\n");
        g_wifi_state = WIFI_STA_SAMPLE_INIT;
        osEventFlagsSet(wireless_event_flags, WIRELESS_DISCONNECT_BIT);
        client_running = false;
    } else {
        g_wifi_state = WIFI_STA_SAMPLE_CONNECT_DONE;
        osEventFlagsSet(wireless_event_flags, WIRELESS_CONNECT_BIT);
        client_running = true;

    }
}

int HAL_WiFi_Init(void)
{
    CreateWirelessEventFlags();

    /* 注册事件回调 */
    if (wifi_register_event_cb(&wifi_event_cb) != 0) {
        LOG("wifi_event_cb register fail.\r\n");
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
        LOG("%s::STA DHCP timeout, try again !.\r\n");
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
        LOG("wifi_sta_enable fail.\r\n");
        return -1;
    }else{
        return 0;
    }
}

int HAL_WiFi_STA_Disable(void)
{
    if (wifi_sta_disable() != 0) {
        LOG("wifi_sta_disable fail.\r\n");
        return -1;
    }else{
        return 0;
    }
}

int HAL_WiFi_Scan(WiFiSTAConfig *wifi_config, WiFiScanResult *results, int max_results)
{
    if (wifi_config == NULL || results == NULL || max_results <= 0) {
        LOG("Invalid input.\r\n");
        return -1;
    }

    g_wifi_state = WIFI_STA_SAMPLE_SCANING;

    // 启动扫描
    if (wifi_sta_scan() != 0) {
        LOG("Wi-Fi scan failed to start.\r\n");
        return -1;
    }

    // 等待扫描完成或超时
    int elapsed_time = 0;
    while (g_wifi_state != WIFI_STA_SAMPLE_SCAN_DONE) {
        osDelay(SCAN_POLL_INTERVAL_MS); // 每次等待 10 毫秒
        elapsed_time += SCAN_POLL_INTERVAL_MS;

        if (elapsed_time >= SCAN_TIMEOUT_MS) {
            LOG("Wi-Fi scan timed out.\r\n");
            return -1;
        }
    }

    // 获取扫描结果
    td_u32 scan_len = sizeof(wifi_scan_info_stru) * WIFI_SCAN_AP_LIMIT;
    wifi_scan_info_stru *scan_results = osal_kmalloc(scan_len, OSAL_GFP_ATOMIC);
    if (scan_results == TD_NULL) {
        LOG("Failed to allocate memory for scan results.\r\n");
        return -1;
    }

    memset_s(scan_results, scan_len, 0, scan_len);
    uint32_t num_results = WIFI_SCAN_AP_LIMIT;
    if (wifi_sta_get_scan_info(scan_results, &num_results) != 0) {
        LOG("Failed to get Wi-Fi scan results.\r\n");
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
        LOG("Failed to find the target SSID: %s\r\n", wifi_config->ssid);
        return -1;
    }

    return num_results;
}

int HAL_WiFi_Connect(const WiFiSTAConfig *wifi_config)
{
    if (wifi_config == NULL) {
        LOG("Invalid input: wifi_config is NULL.\r\n");
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
        LOG("Wi-Fi connect failed.\r\n");
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
            LOG("Wi-Fi connection timed out.\r\n");
            return -1;
        }
    }

    // 获取网卡接口名
    const char *ifname = "wlan0";  // 假设网卡接口名为wlan0，根据实际调整
    struct netif *netif_p = netifapi_netif_find(ifname);
    if (netif_p == NULL) {
        LOG("Network interface not found: %s\n", ifname);
        return -1;
    }

    // 启动 DHCP 获取 IP 地址
    if (netifapi_dhcp_start(netif_p) != 0) {
        LOG("Failed to start DHCP on interface: %s\n", ifname);
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
            LOG("DHCP timeout on interface: %s\n", ifname);
            return -1;
        }
    }

    // 存储获取到的 IP 地址
    store_ip(netif_p);

    int ssid_len = strlen(wifi_config->ssid);
    char tree_level_char = wifi_config->ssid[ssid_len - 1];
    if (isdigit(tree_level_char)) {
        tree_level =  tree_level_char - '0';
    } else if (tree_level_char >= 'A' && tree_level_char <= 'Z') {
        tree_level = tree_level_char - 'A' + 10;
    } else if (tree_level_char >= 'a' && tree_level_char <= 'z') {
        tree_level = tree_level_char - 'a' + 36;
    }

    heart_beat_thread_id = osThreadNew((osThreadFunc_t)HAL_WiFi_CreateIPMACBindingClient, NULL, NULL);
    return 0;  
}

int HAL_WiFi_GetIP(char *ip_buffer, int buffer_len)
{
    // 检查输入参数是否合法
    if (ip_buffer == NULL || buffer_len <= 0) {
        LOG("Invalid input: ip_buffer is NULL or buffer_len is invalid.\n");
        return -1;
    }

    // 检查全局变量 g_sta_ip 中是否存有 IP 地址
    if (g_sta_ip[0] == '\0') {
        LOG("No IP address available.\n");
        return -1;
    }

    // 检查缓冲区大小是否足够
    if (buffer_len < 16) {
        LOG("Buffer size too small. Needs at least 16 bytes.\n");
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
        LOG("wifi disconnect fail.\r\n");
        return -1;
    }else{
        return 0;
    }
}

int HAL_WiFi_AP_Enable(WiFiAPConfig *config, int tree_level) {
    if (config == NULL) {
        LOG("Invalid input: config is NULL.\n");
        return -1;
    }

    // 创建 SoftAP 配置
    softap_config_stru hapd_conf = {0};
    softap_config_advance_stru advanced_conf = {0};
    td_char ifname[WIFI_IFNAME_MAX_SIZE + 1] = "ap0"; // SoftAp接口名
    struct netif *netif_p = NULL;
    ip4_addr_t st_gw, st_ipaddr, st_netmask;

    // 设置SoftAp的IP地址，子网掩码和网关
    IP4_ADDR(&st_ipaddr, 192, 168, tree_level, 1);  // IP地址：192.168.X.1
    IP4_ADDR(&st_netmask, 255, 255, 255, 0); // 子网掩码：255.255.255.0
    IP4_ADDR(&st_gw, 192, 168, tree_level, 255);      // 网关：192.168.X.255

    // 基本SoftAp配置
    strncpy((char *)hapd_conf.ssid, (const char *)config->ssid, sizeof(hapd_conf.ssid) - 1);
    strncpy((char *)hapd_conf.pre_shared_key, (const char *)config->password, sizeof(hapd_conf.pre_shared_key) - 1);
    hapd_conf.security_type = (td_u8)config->security;
    hapd_conf.channel_num = config->channel;
    hapd_conf.wifi_psk_type = 0;  // 假设PSK类型为0
    LOG("hapd_conf.channel_num:%d\r\n", hapd_conf.channel_num);

    // 高级SoftAp配置
    advanced_conf.beacon_interval = 100;     // Beacon周期设置为100ms
    advanced_conf.dtim_period = 2;           // DTIM周期设置为2
    advanced_conf.gi = 0;                    // Short GI默认关闭
    advanced_conf.group_rekey = 86400;       // 组播秘钥更新时间设置为1天
    advanced_conf.protocol_mode = 4;         // 802.11b/g/n/ax协议模式
    advanced_conf.hidden_ssid_flag = 1;      // SSID不隐藏

    // 设置SoftAp的高级配置
    if (wifi_set_softap_config_advance(&advanced_conf) != 0) {
        LOG("Failed to set advanced SoftAP configuration.\n");
        return -1;
    }

    // 启动SoftAp接口
    if (wifi_softap_enable(&hapd_conf) != 0) {
        LOG("Failed to enable SoftAP.\n");
        return -1;
    }

    // 获取网络接口
    netif_p = netif_find(ifname);
    if (netif_p == NULL) {
        LOG("Failed to find network interface: %s.\n", ifname);
        (void)wifi_softap_disable();
        return -1;
    }

    // 设置IP地址、子网掩码和网关
    if (netifapi_netif_set_addr(netif_p, &st_ipaddr, &st_netmask, &st_gw) != 0) {
        LOG("Failed to set network interface address.\n");
        (void)wifi_softap_disable();
        return -1;
    }

    // 启动DHCP服务器
    if (netifapi_dhcps_start(netif_p, NULL, 0) != 0) {
        LOG("Failed to start DHCP server.\n");
        (void)wifi_softap_disable();
        return -1;
    }

    // 创建线程启动MAC地址绑定表
    server_running = true;
    IP_MAC_thread_id = osThreadNew((osThreadFunc_t)HAL_WiFi_CreateIPMACBindingServer, NULL, NULL);
    if (IP_MAC_thread_id == NULL) {
        LOG("Failed to create MAC-IP binding table thread.\n");
        return -1;
    }

    LOG("SoftAP started successfully with SSID: %s\n", config->ssid);
    return 0;
}

int HAL_WiFi_AP_Disable(void) {
    // 停止 SoftAp 模式
    if (wifi_softap_disable() != 0) {
        LOG("Failed to disable SoftAP mode.\n");
        return -1;
    }
    server_running = false;
    delete_mac_ip_list();
    osDelay(200); // 等待2s，确保函数执行完成，释放掉socket资源
    osThreadTerminate(IP_MAC_thread_id);

    LOG("SoftAP mode disabled.\n");
    return 0;
}

int HAL_WiFi_GetConnectedSTAInfo(WiFiSTAInfo *result, uint32_t *size) {
    if (result == NULL || size == NULL || (*size) == 0) {
        LOG("Invalid input parameters.\n");
        return -1;
    }
    wifi_sta_info_stru* sta_info = (wifi_sta_info_stru*)malloc(sizeof(wifi_sta_info_stru) * (*size));
    if (sta_info == NULL) {
        LOG("Memory allocation failed.\n");
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
        LOG("Failed to get connected STA info.\n");
        return -1;
    }

    // 成功返回STA列表信息
    return 0;
}

int HAL_WiFi_GetAPMacAddress(uint8_t *mac_addr) {
    if (mac_addr == NULL) {
        LOG("Invalid input: mac_addr is NULL.\n");
        return -1;
    }
    int8_t temp_mac[6] = {0};
    // 获取 SoftAP MAC 地址
    if (wifi_softap_get_mac_addr(temp_mac, 6) != 0) {
        LOG("Failed to get SoftAP MAC address.\n");
        return -1;
    }
    // 将 MAC 地址拷贝到输出参数中
    memcpy(mac_addr, temp_mac, 6);
    return 0;
}

int HAL_WiFi_GetNodeMAC(char *mac){
    if (mac == NULL) {
        LOG("Invalid input: mac is NULL.\n");
        return -1;
    }
    // 获取节点的 MAC 地址
    uint8_t ap_mac[6];
    memset(ap_mac, 0, sizeof(ap_mac));
    HAL_WiFi_GetAPMacAddress(ap_mac);
    // 将mac后三位转换为字符写入g_mesh_config.root_mac
    char temp_mac[7];
    snprintf(temp_mac, sizeof(temp_mac), "%02X%02X%02X", ap_mac[3], ap_mac[4], ap_mac[5]);
    memcpy(mac, temp_mac, 7);
    return 0;
}

int HAL_WiFi_Send_data(const char *ip, uint16_t port, const char *data) {
    if (ip == NULL || data == NULL) {
        LOG("Invalid input: ip or data is NULL.\n");
        return -1;
    }

    // 创建一个 TCP 套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG("Failed to create socket.\n");
        return -1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // 将 IP 地址转换为网络字节序
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        LOG("IP address conversion failed.\n");
        closesocket(sockfd);
        return -1;
    }

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG("Failed to connect to server.\n");
        closesocket(sockfd);
        return -1;
    }

    // 发送数据
    int ret = send(sockfd, data, strlen(data), 0);
    if (ret < 0) {
        LOG("Failed to send data.\n");
        closesocket(sockfd);
        return -1;
    }

    // 关闭套接字
    closesocket(sockfd);
    return 0;
}

int HAL_WiFi_Receive_data(const char *ip, uint16_t port, char *buffer, int buffer_len, char *client_ip) {
    if (ip == NULL || buffer == NULL || buffer_len <= 0) {
        LOG("Invalid input: ip, buffer or buffer_len is invalid.\n");
        return -1;
    }

    // 创建一个 TCP 套接字
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        LOG("Failed to create socket.\n");
        return -1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // 将 IP 地址转换为网络字节序
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        LOG("IP address conversion failed.\n");
        closesocket(listen_sock);
        return -1;
    }

    // 绑定地址和端口
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        LOG("Failed to bind socket.\n");
        closesocket(listen_sock);
        return -1;
    }

    // 开始监听连接请求
    if (listen(listen_sock, 8) < 0) {
        LOG("Failed to listen on socket.\n");
        closesocket(listen_sock);
        return -1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        LOG("Failed to accept connection.\n");
        closesocket(listen_sock);
        return -1;
    }

    // 接收数据
    int ret = recv(client_sock, buffer, buffer_len - 1, 0);
    if (ret < 0) {
        LOG("Failed to receive data.\n");
        closesocket(client_sock);
        closesocket(listen_sock);
        return -1;
    }
    buffer[ret] = '\0';  // 确保字符串以 NULL 结尾

    // 获取客户端 IP 地址
    if (client_ip != NULL) {
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    }

    // 关闭套接字
    closesocket(client_sock);
    closesocket(listen_sock);
    return 0;
}

int HAL_WiFi_GetAPConfig(WiFiAPConfig *config) {
    if (config == NULL) {
        LOG("Invalid input: config is NULL.\n");
        return -1;
    }

    // 获取 SoftAP 配置
    softap_config_stru ap_config;
    if (wifi_get_softap_config(&ap_config) != 0) {
        LOG("Failed to get SoftAP configuration.\n");
        return -1;
    }

    // 将 SoftAP 配置拷贝到输出参数中
    strncpy(config->ssid, (char *)ap_config.ssid, sizeof(config->ssid) - 1);
    strncpy(config->password, (char *)ap_config.pre_shared_key, sizeof(config->password) - 1);
    config->channel = ap_config.channel_num;
    config->security = (WiFiSecurityType)ap_config.wifi_psk_type;

    return 0;
}

void add_mac_ip_binding(const char *mac, const char *ip) {
    MAC_IP_Node *current = head;

    // 遍历链表寻找MAC是否已经存在
    while (current != NULL) {
        if (strncmp(current->binding.mac, mac, sizeof(current->binding.mac)) == 0) {
            // 找到匹配的 MAC 地址，更新 IP 地址
            strcpy(current->binding.ip, ip);
            current->count = 0;
            return;
        }
        // 继续遍历链表
        current = current->next;
    }

    // 创建新的节点
    MAC_IP_Node *new_node = (MAC_IP_Node *)malloc(sizeof(MAC_IP_Node));
    if (new_node == NULL) {
        LOG("Memory allocation failed!\n");
        return;
    }

    // 初始化节点数据
    strcpy(new_node->binding.mac, mac);
    strcpy(new_node->binding.ip, ip);
    new_node->count = 0;
    new_node->next = NULL;

    // 插入到链表的开头（头插法）
    new_node->next = head;
    head = new_node;

    LOG("Added MAC: %s, IP: %s\n", new_node->binding.mac, new_node->binding.ip);
    len_mac_ip_list++;
}

void remove_mac_ip_binding(const char *mac) {
    MAC_IP_Node *current = head;
    MAC_IP_Node *previous = NULL;

    // 遍历链表寻找要删除的节点
    while (current != NULL) {
        if (strncmp(current->binding.mac, mac, sizeof(current->binding.mac)) == 0) {
            // 找到匹配的节点
            if (previous == NULL) {
                // 如果要删除的是头节点
                head = current->next;
            } else {
                // 不是头节点，将前一个节点指向当前节点的下一个节点
                previous->next = current->next;
            }

            // 释放节点的内存
            free(current);
            LOG("Removed MAC: %s\n", mac);
            return;
        }

        // 继续遍历链表
        previous = current;
        current = current->next;
    }

    LOG("MAC: %s not found.\n", mac);
    len_mac_ip_list--;
}

int find_mac_from_ip(const char *ip, char *mac) {
    MAC_IP_Node *current = head;

    // 遍历链表寻找匹配的 IP 地址
    while (current != NULL) {
        if (strncmp(current->binding.ip, ip, sizeof(current->binding.ip)) == 0) {
            // 找到匹配的 IP 地址
            strncpy(mac, current->binding.mac, sizeof(current->binding.mac) - 1);
            return 0;
        }

        // 继续遍历链表
        current = current->next;
    }

    // 没有找到匹配的 IP 地址
    return -1;
}

void add_mac_ip_bindings_counts(void) {
    MAC_IP_Node *current = head;

    // 遍历链表，为每个节点的计数器加 1
    while (current != NULL) {
        current->count++;
        if (current -> count > 30) {
            remove_mac_ip_binding(current->binding.mac);
        }
        current = current->next;
    }
}

void print_mac_ip_bindings(void) {
    MAC_IP_Node *current = head;

    LOG("Current MAC-IP Bindings:\n");
    while (current != NULL) {
        LOG("MAC: %s, IP: %s, count: %d\n", current->binding.mac, current->binding.ip, current->count);
        current = current->next;
    }
}

void delete_mac_ip_list(void) {
    MAC_IP_Node *current = head;
    MAC_IP_Node *next_node;

    while (current != NULL) {
        next_node = current->next;  // 保存下一个节点
        free(current);              // 释放当前节点
        current = next_node;        // 移动到下一个节点
    }

    head = NULL;  // 头指针置空，链表删除完成
    LOG("All MAC-IP bindings have been deleted.\n");
    len_mac_ip_list = 0;
}

void get_last_three_mac(char *mac, const uint8_t *full_mac) {
    if (mac == NULL || full_mac == NULL) {
        return;
    }
    // 将mac后三位转换为字符写入g_mesh_config.root_mac
    snprintf(mac, 7, "%02X%02X%02X", full_mac[3], full_mac[4] + 2, full_mac[5]);
    return;
}

void delete_leave_sta_mac_ip_list(WiFiSTAInfo *sta_info, uint32_t sta_num) {
    MAC_IP_Node *current = head;
    MAC_IP_Node *previous = NULL;
    WiFiSTAInfo *sta = sta_info;
    bool found = false;

    while (current != NULL) {
        found = false;
        for (uint32_t i = 0; i < sta_num; i++) {
            char sta_mac[7];
            get_last_three_mac(sta_mac, sta[i].mac_addr);
            LOG("sta_mac: %s\n", sta_mac);
            if (strncmp(current->binding.mac, sta_mac, 7) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            // 删除离开节点的mac-ip绑定
            if (previous == NULL) {
                // 如果要删除的是头节点
                head = current->next;
            } else {
                // 不是头节点，将前一个节点指向当前节点的下一个节点
                previous->next = current->next;
            }

            // 释放节点的内存
            free(current);
            len_mac_ip_list--;
        } else {
            previous = current;
        }
        current = current->next;
    }
}

int HAL_WiFi_GetAllMAC(char ***mac_list) {
    if (mac_list == NULL) {
        LOG("Invalid input parameters.\n");
        return -1;
    }

    MAC_IP_Node *current = head;
    uint32_t count = 0;

    if (len_mac_ip_list == 0) {
        return 0;
    }

    // 分配指针数组，大小为 len_mac_ip_list
    *mac_list = (char **)malloc(len_mac_ip_list * sizeof(char *));
    if (*mac_list == NULL) {
        LOG("Memory allocation failed.\n");
        return -1;
    }

    // 遍历链表，将 MAC 地址拷贝到 mac_list 中
    while (current != NULL) {
        (*mac_list)[count] = (char *)malloc(7 * sizeof(char));  // 分配空间给每个MAC地址
        if ((*mac_list)[count] == NULL) {
            LOG("Memory allocation failed.\n");
            return -1;
        }
        strcpy((*mac_list)[count], current->binding.mac);
        count++;
        current = current->next;
    }

    // 返回实际拷贝的 MAC 地址数量
    return count;
}



void HAL_WiFi_CreateIPMACBindingServer(void) {
    // 创建一个 TCP 套接字
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        LOG("Failed to create socket.\n");
        return;
    }

    // 设置 SO_REUSEADDR 套接字选项，允许端口复用
    int optval = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        LOG("Failed to set socket options.\n");
        closesocket(listen_sock);
        return;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定地址和端口
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG("Failed to bind socket.\n");
        closesocket(listen_sock);
        return;
    }

    // 开始监听连接请求
    if (listen(listen_sock, 8) < 0) {
        LOG("Failed to listen on socket.\n");
        closesocket(listen_sock);
        return;
    }

    // 设置 accept 的超时时间为1秒
    struct timeval timeout;
    timeout.tv_sec = 1;  // 1秒超时
    timeout.tv_usec = 0;
    setsockopt(listen_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    LOG("Server is listening on port 9000...\n");
    char mac[7];
    char ip[16];
    while (server_running) {  // 使用全局标志控制循环
        add_mac_ip_bindings_counts();
        print_mac_ip_bindings();
        // 接受连接请求
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            continue;
        }else{
            // 接收数据
            char buffer[10];
            int ret = recv(client_sock, buffer, sizeof(buffer) - 1, 0);  // 需要改成非阻塞接受
            if (ret < 0 || ret > 7) {
                // 超时或数据长度不符合要求
                closesocket(client_sock);
                continue;
            }
            buffer[ret] = '\0';  // 确保字符串以 NULL 结尾
            strcpy(mac, buffer);
            inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
            add_mac_ip_binding(mac, ip);
            LOG("Received data: %s\n", buffer);
        }
        closesocket(client_sock);
    }

    // 关闭监听套接字
    closesocket(listen_sock);
    LOG("Server stopped.\n");
}

void HAL_WiFi_CreateIPMACBindingClient(void) {
    while (client_running) {
        char ip[16];
        snprintf(ip, sizeof(ip), "192.168.%d.1", tree_level);
        char mac[7];
        HAL_WiFi_GetNodeMAC(mac);
        if(HAL_WiFi_Send_data(ip, 9000, mac) != 0)
        {
            LOG("send data fail.\r\n");
        }else{
            LOG("send data success.\r\n");
        }
        osDelay(100);
    }
}

int HAL_WiFi_Send_data_by_MAC(const char *MAC, const char *data) {
    if (MAC == NULL || data == NULL) {
        LOG("Invalid input: MAC or data is NULL.\n");
        return -1;
    }

    // 查找 MAC 对应的 IP 地址
    MAC_IP_Node *current = head;
    char ip[16];
    while (current != NULL) {
        if (strncmp(current->binding.mac, MAC, 7) == 0) {
            strncpy(ip, current->binding.ip, sizeof(ip) - 1);
            break;
        }
        current = current->next;
    }

    if (current == NULL) {
        LOG("MAC: %s not found.\n", MAC);
        return -1;
    }
    if(HAL_WiFi_Send_data(ip, 9001, data) != 0){
    
        LOG("send data fail.\r\n");
        return -2;
    }
    
    return 0;
}

int HAL_WiFi_Send_data_to_parent(const char *data, int tree_level) {
    if (data == NULL) {
        LOG("Invalid input: data is NULL.\n");
        return -1;
    }

    char ip[16];
    snprintf(ip, sizeof(ip), "192.168.%d.1", tree_level);

    if(HAL_WiFi_Send_data(ip, 9001, data) != 0){
        LOG("send data fail.\r\n");
        return -1;
    }
    return 0;
}

int HAL_WiFi_Create_Server(uint16_t port) {
    // 创建一个 TCP 套接字
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        LOG("Failed to create socket.\n");
        return -1;
    }

    // 设置 SO_REUSEADDR 套接字选项，允许端口复用
    int optval = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        LOG("Failed to set socket options.\n");
        closesocket(listen_sock);
        return -1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定地址和端口
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG("Failed to bind socket.\n");
        closesocket(listen_sock);
        return -1;
    }

    // 开始监听连接请求
    if (listen(listen_sock, 8) < 0) {
        LOG("Failed to listen on socket.\n");
        closesocket(listen_sock);
        return -1;
    }

    // 设置 accept 的超时时间为1秒
    struct timeval timeout;
    timeout.tv_sec = 1;  // 1秒超时
    timeout.tv_usec = 0;
    setsockopt(listen_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    return listen_sock;
}

int HAL_WiFi_Server_Receive(int server_fd, char *mac, char *buffer, int buffer_len) {
    if (server_fd < 0 || buffer == NULL || buffer_len <= 0) {
        LOG("Invalid input: server_fd, buffer or buffer_len is invalid.\n");
        return -1;
    }

    // 接受连接请求
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        return -1;
    }

    // 接收数据
    int ret = recv(client_sock, buffer, buffer_len - 1, 0);
    if (ret < 0) {
        LOG("Failed to receive data.\n");
        closesocket(client_sock);
        return -1;
    }
    buffer[ret] = '\0';  // 确保字符串以 NULL 结尾

    // 获取客户端 ip 地址
    char ip[16];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);

    // 查找 MAC 对应的 IP 地址
    find_mac_from_ip(ip, mac);

    // 关闭客户端套接字
    closesocket(client_sock);
    return ret;
}

int HAL_WiFi_Close_Server(int server_fd) {
    if (server_fd < 0) {
        LOG("Invalid input: server_fd is invalid.\n");
        return -1;
    }

    // 关闭监听套接字
    closesocket(server_fd);
    return 0;
}