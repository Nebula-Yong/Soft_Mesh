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

// 维护MAC与IP的绑定表
typedef struct {
    char mac[7];  // MAC地址
    char ip[16];   // IP地址
} MAC_IP_Binding;

// 链表节点
typedef struct MAC_IP_Node {
    MAC_IP_Binding binding;      // MAC-IP绑定信息
    struct MAC_IP_Node *next;    // 指向下一个节点的指针
    int count;                   // 未更新次数
} MAC_IP_Node;

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
int HAL_WiFi_AP_Enable(WiFiAPConfig *config, int tree_level);

/**
 * @brief 关闭Wi-Fi AP模式
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_AP_Disable(void);

/**
 * @brief 扫描附近的Wi-Fi网络
 * @param wifi_config 用于匹配要连接的Wi-Fi网络的配置
 * @param results 用于存储扫描结果的数组，调用方需预先分配足够的空间。
 * @param max_results `results` 数组的最大长度，表示最多存储多少个扫描结果。
 * @return 扫描到的Wi-Fi网络数量，或 < 0 表示失败
 */
int HAL_WiFi_Scan(WiFiSTAConfig *wifi_config, WiFiScanResult *results, int max_results);

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
 * @brief 获取开启AP模式的MAC地址
 * @param mac_addr 存储MAC地址的缓冲区，长度至少为6字节
 * @return 0表示成功，其他表示失败
 */
int HAL_WiFi_GetAPMacAddress(uint8_t *mac_addr);

/**
 * @brief 通过Wi-Fi发送数据
 * @param ip 目标IP地址
 * @param port 目标端口
 * @param data 要发送的数据
 * @return 0表示成功，其他表示失败
 */
int HAL_WiFi_Send_data(const char *ip, uint16_t port, const char *data);

/**
 * @brief 通过Wi-Fi接收数据
 * @param ip 发送数据的IP地址
 * @param port 发送数据的端口
 * @param buffer 存储接收数据的缓冲区
 * @param buffer_len 缓冲区的长度
 * @param client_ip 存储发送数据的客户端IP地址
 */
int HAL_WiFi_Receive_data(const char *ip, uint16_t port, char *buffer, int buffer_len, char *client_ip);

/**
 * @brief 获取AP的配置信息
 * @param[out] config 存储AP配置信息的结构体
 * @return 0表示成功，其他表示失败
 */
int HAL_WiFi_GetAPConfig(WiFiAPConfig *config);

/**
 * @brief 创建TCP服务端用于绑定MAC和IP地址
 */
void HAL_WiFi_CreateIPMACBindingServer(void);

/**
 * @brief 创建TCP客户端用于给绑定服务器定期发送MAC地址心跳包
 */
void HAL_WiFi_CreateIPMACBindingClient(void);

/**
 * @brief 获取节点的MAC地址，AP MAC地址的后三位
 * @param[out] mac 存储MAC地址的缓冲区，至少需要7字节
 * @return 0 表示成功，非 0 表示失败
 * @note 6字节的MAC地址，最后一位为'\0'，共7字节
 */
int HAL_WiFi_GetNodeMAC(char *mac);

/**
 * @brief 想指定的MAC地址发送数据
 * @param MAC 目标设备的MAC地址
 * @param data 要发送的字符串数据
 */
int HAL_WiFi_Send_data_by_MAC(const char *MAC, const char *data);

/**
 * @brief 通过Wi-Fi发送数据给父节点
 * @param data 要发送的字符串数据
 * @param tree_level 父节点所在树的层数
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_WiFi_Send_data_to_parent(const char *data, int tree_level);

/**
 * @brief 创建socket TCP服务端
 * @param port 服务端端口
 * @return 返回socket描述符
 * @note 返回值小于0表示失败
 */
int HAL_WiFi_Create_Server(uint16_t port);

/**
 * @brief 关闭socket TCP服务端
 * @param server_fd 服务端socket描述符
 * @return 0表示成功，其他表示失败
 * @note 关闭服务端后，服务端socket描述符将被置为-1
 */
int HAL_WiFi_Close_Server(int server_fd);

/**
 * @brief 接收TCP客户端的连接和数据
 * @param server_fd 服务端socket描述符
 * @param[out] mac 存储MAC地址的缓冲区，至少需要7字节
 * @param[out] buffer 存储接收数据的缓冲区
 * @param buffer_len 缓冲区的长度
 * @return 接收到的数据长度，或 < 0 表示失败
 */
int HAL_WiFi_Server_Receive(int server_fd, char *mac, char *buffer, int buffer_len);

/**
 * @brief 获取当前路由表的所有设备的MAC地址
 * @param[out] mac_list 存储MAC地址的指针数组
 * @return 返回MAC地址的数量，或 < 0 表示失败
 * @note 每个MAC地址占用6字节
 */
int HAL_WiFi_GetAllMAC(char ***mac_list);

#ifdef __cplusplus
}
#endif

#endif // HAL_WIFI_H
