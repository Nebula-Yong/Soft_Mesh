#ifndef HAL_WIFI_H
#define HAL_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASSWORD_MAX_LEN 64
#define WIFI_BSSID_LEN 6

/** Wi-Fi安全类型定义 */
typedef enum {
    WIFI_SECURITY_INVALID = -1,    // 无效安全类型
    WIFI_SECURITY_OPEN,            // Open
    WIFI_SECURITY_WEP,             // WEP
    WIFI_SECURITY_WPA2_PSK,        // WPA2 个人级
    WIFI_SECURITY_WPA2_WPA_PSK_MIX,// WPA/WPA2 混合
    WIFI_SECURITY_WPA_PSK,         // WPA 个人级
    WIFI_SECURITY_WPA,             // WPA 企业级
    WIFI_SECURITY_WPA2,            // WPA2 企业级
    WIFI_SECURITY_WPA3_SAE,        // WPA3 SAE
    WIFI_SECURITY_WPA3_WPA2_MIX,   // WPA2/WPA3 混合
    WIFI_SECURITY_WPA3,            // WPA3 企业级
    WIFI_SECURITY_OWE,             // OWE (Opportunistic Wireless Encryption)
    WIFI_SECURITY_WAPI_PSK,        // WAPI 个人级
    WIFI_SECURITY_WAPI_CERT,       // WAPI 企业级
    WIFI_SECURITY_UNKNOWN          // 其它认证类型
} WiFiSecurityType;

/** Wi-Fi接入点扫描结果结构体 */
typedef struct {
    char ssid[33];              // SSID (最多32字节 + 1 结束符)
    uint8_t bssid[6];           // BSSID (AP的MAC地址)
    int16_t rssi;               // 信号强度 (dBm)
    WiFiSecurityType security;  // 安全类型
    uint8_t channel;            // 信道
} WiFiScanResult;

typedef struct {
    char ssid[WIFI_SSID_MAX_LEN + 1];      // Wi-Fi SSID
    char password[WIFI_PASSWORD_MAX_LEN + 1]; // Wi-Fi 密码
    uint8_t bssid[WIFI_BSSID_LEN];         // Wi-Fi BSSID（接入点 MAC 地址）
    WiFiSecurityType security_type;        // Wi-Fi 安全类型
} WiFiSTAConfig;

/** Wi-Fi AP模式配置结构体 */
typedef struct {
    char ssid[33];              // AP模式的SSID
    char password[64];          // AP模式的密码 (最多64字节)
    uint8_t channel;            // AP的信道
    WiFiSecurityType security;  // 安全类型
} WiFiAPConfig;

/** Wi-Fi连接状态枚举 */
typedef enum {
    HAL_WIFI_DISCONNECTED,          // 断开连接
    HAL_WIFI_CONNECTED,             // 已连接
    HAL_WIFI_CONNECTING,            // 连接中
    HAL_WIFI_CONN_STATUS_BUTT,      // 枚举越界标记位
} WiFiConnectionStatus;

/** Wi-Fi已连接STA信息结构体 */
typedef struct {
    uint8_t mac_addr[WIFI_BSSID_LEN];       /*!< MAC地址 */
    int8_t rssi;                            /*!< 接收信号强度指示 (RSSI) */
    int8_t rsv;                            /*!< 保留字段 */
    uint32_t best_rate;                     /*!< 最佳发送速率 (kbps) */
} WiFiSTAInfo;

/**
 * @brief 初始化Wi-Fi硬件及相关资源
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_Init(void);

/**
 * @brief 去初始化Wi-Fi，释放相关资源
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_Deinit(void);

/**
 * @brief 启动Wi-Fi的STA模式
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_STA_Enable(void);

/**
 * @brief 关闭Wi-Fi STA模式
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_STA_Disable(void);

/**
 * @brief 启动Wi-Fi AP模式
 * @param config 包含AP模式的配置参数（SSID、密码、信道等）
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_AP_Enable(WiFiAPConfig *config);

/**
 * @brief 关闭Wi-Fi AP模式
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_AP_Disable(void);

/**
 * @brief 扫描附近的Wi-Fi网络
 * @param results 用于存储扫描结果的数组，调用方需预先分配足够的空间。
 * @param max_results `results` 数组的最大长度，表示最多存储多少个扫描结果。
 * @return 扫描到的Wi-Fi网络数量，或 < 0 表示失败
 */
int HAL_WiFi_Scan(const char *target_ssid, const char *target_password, WiFiSTAConfig *wifi_config, WiFiScanResult *results, int max_results);

/**
 * @brief 连接到指定的Wi-Fi AP
 * @param ssid 目标Wi-Fi网络的SSID
 * @param password 目标Wi-Fi网络的连接密码
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_Connect(const WiFiSTAConfig *wifi_config);

/**
 * @brief 获取当前STA模式下的IP地址
 * @param ip_buffer 存储IP地址的缓冲区，建议至少分配16字节以存储完整的IPv4地址。
 * @param buffer_len `ip_buffer` 的大小
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_GetIP(char *ip_buffer, int buffer_len);

/**
 * @brief 断开当前的Wi-Fi连接
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_Disconnect(void);

/**
 * @brief 获取已连接STA的信息
 * @param[out] result 用于存储STA信息的数组
 * @param[in, out] size  输入为数组的最大长度，输出为已连接的STA数量
 * @return 0表示成功，其他表示失败
 */
int HAL_WiFi_GetConnectedSTAInfo(WiFiSTAInfo *result, uint32_t *size);

/**
 * @brief 查询已连接 STA 的 MAC 和 IP 地址，并将其写入结构体数组。
 * 
 * @param[out] sta_info_array 用于存储已连接 STA 信息的结构体数组
 * @param[in,out] sta_count 传入结构体数组的长度，返回时为已连接的 STA 数量
 * 
 * @return 0 表示成功，其他值表示失败
 */

#ifdef __cplusplus
}
#endif

#endif // HAL_WIFI_H
