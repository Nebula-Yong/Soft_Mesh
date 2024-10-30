#ifndef HAL_WIRELESS_H
#define HAL_WIRELESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** 支持的无线通信类型 */
typedef enum {
    WIRELESS_TYPE_WIFI = 0,          // Wi-Fi 通信
    WIRELESS_TYPE_BLUETOOTH,         // 蓝牙通信
    WIRELESS_TYPE_NEARLINK,          // 星闪通信
    WIRELESS_TYPE_MAX                // 最大类型，标识边界
} WirelessType;

#define DEFAULT_WIRELESS_TYPE WIRELESS_TYPE_WIFI
#define DEFAULT_WIRELESS_SECURITY 2

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
    uint8_t mac_addr[6];          /*!< MAC地址 */
    int8_t rssi;                  /*!< 接收信号强度指示 (RSSI) */
    uint8_t channel;              /*!< 连接信道 */
    WirelessType type;            /*!< 无线通信类型 (Wi-Fi、蓝牙、星闪) */
} WirelessConnectedInfo;

/** 通用无线 STA 模式配置结构体 */
typedef struct {
    char ssid[33];                // 要连接的 Wi-Fi SSID
    char password[64];            // 要连接的 Wi-Fi 密码
    uint8_t bssid[6];             // 要连接的 Wi-Fi AP 的 BSSID
    WirelessType type;            // 无线通信类型 (如 Wi-Fi)
    int security;                 // 安全类型（具体类型在各自 HAL 头文件中定义）
} WirelessSTAConfig;

/** 通用无线 AP 配置结构体 */
typedef struct {
    char ssid[33];                // AP模式的SSID
    char password[64];            // AP模式的密码 (最多64字节)
    uint8_t channel;              // AP的信道
    WirelessType type;            // 无线通信类型 (如 Wi-Fi)
    int security;                 // 安全类型（具体类型在各自 HAL 头文件中定义）
} WirelessAPConfig;

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
 * @brief 启动指定类型的无线通信模块的AP模式，如Wi-Fi AP模式、蓝牙广播模式等。
 * @param type 指定无线通信类型。
 * @param config AP模式的配置参数（SSID、密码、信道等）
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_EnableAP(WirelessType type, const WirelessAPConfig *config, int tree_level);

/**
 * @brief 停止指定类型的无线通信模块的AP模式，如关闭Wi-Fi AP模式、关闭蓝牙广播等。
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_DisableAP(WirelessType type);

/**
 * @brief 扫描指定类型的无线网络
 * @param type 指定无线通信类型（如Wi-Fi、蓝牙）
 * @param wifi_config 用于匹配要连接的Wi-Fi网络的配置
 * @param results 用于存储扫描结果的数组，调用方需预先分配足够的空间。
 * @param max_results `results` 数组的最大长度，表示最多存储多少个扫描结果。
 * @return 扫描到的无线网络数量，或 < 0 表示失败
 */
int HAL_Wireless_Scan(WirelessType type, WirelessSTAConfig *wifi_config, WirelessScanResult *results, int max_results);

/**
 * @brief 连接到指定的无线设备（如Wi-Fi AP或蓝牙设备）
 * @param config 连接配置信息，可以是不同通信类型的配置结构体（具体定义在各自HAL中）
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Connect(WirelessType type, WirelessSTAConfig *config);

/**
 * @brief 断开当前的无线连接
 * @param type 指定无线通信类型。
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_Disconnect(WirelessType type);

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

/**
 * @brief 获取无线通信模块作为AP时的MAC地址
 * @param type 指定无线通信类型。
 * @param[out] mac 存储MAC地址的缓冲区，至少需要6字节
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_GetAPMacAddress(WirelessType type, uint8_t *mac);

/**
 * @brief 获取节点的MAC地址，AP MAC地址的后三位
 * @param type 指定无线通信类型。
 * @param[out] mac 存储MAC地址的缓冲区，至少需要7字节
 * @return 0 表示成功，非 0 表示失败
 * @note 6字节的MAC地址，最后一位为'\0'，共7字节
 */
int HAL_Wireless_GetNodeMAC(WirelessType type, char *mac);

/**
 * @brief 通过无线通信模块发送数据给子节点
 * @param type 指定无线通信类型。
 * @param MAC 目标设备的MAC地址
 * @param data 要发送的字符串数据
 * @return 0 表示成功，非 0 表示失败
 * @note 当向上传递时，MAC地址为空字符串，当向下传递时，MAC地址为目标设备的MAC地址
 * @note 该接口仅支持Wi-Fi通信类型
 * @note 该接口仅支持发送TCP数据
 * 
 */
int HAL_Wireless_SendData_to_child(WirelessType type, const char *MAC, const char *data);

/**
 * @brief 通过无线通信模块发送数据给父节点
 * @param type 指定无线通信类型。
 * @param data 要发送的字符串数据
 * @param tree_level 父节点所在树的层数
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_SendData_to_parent(WirelessType type, const char *data, int tree_level);

/**
 * @brief 通过无线通信模块接收数据
 * @param type 指定无线通信类型。
 * @param[out] buffer 存储接收数据的缓冲区
 * @param buffer_len 缓冲区的大小
 * @return 接收到的数据长度，或 < 0 表示失败
 * @note 该接口仅支持Wi-Fi通信类型
 * @note 该接口仅支持接收TCP数据
 */
int HAL_Wireless_ReceiveData(WirelessType type, char *buffer, int buffer_len);

/**
 * @brief 创建无线接收服务器
 * @param type 指定无线通信类型。
 * @return 返回服务器资源描述符，或 < 0 表示失败
 */
int HAL_Wireless_CreateServer(WirelessType type);

/**
 * @brief 关闭无线接收服务器
 * @param type 指定无线通信类型。
 * @param server_fd 服务器资源描述符
 * @return 0 表示成功，非 0 表示失败
 */
int HAL_Wireless_CloseServer(WirelessType type, int server_fd);

/**
 * @brief 通过无线通信模块接收数据
 * @param type 指定无线通信类型。
 * @param server_fd 服务器资源描述符
 * @param[out] mac 存储发送数据的设备的MAC地址
 * @param[out] buffer 存储接收数据的缓冲区
 * @param buffer_len 缓冲区的大小
 * @return 接收到的数据长度，或 < 0 表示失败
 */
int HAL_Wireless_ReceiveDataFromClient(WirelessType type, int server_fd, char *mac, char *buffer, int buffer_len);

/** 
 * @brief 获取所有子节点的MAC地址
 * @param[out] mac_list 存储MAC地址的指针数组
 * @return 返回子节点的数量，或 < 0 表示失败
 * @note 6字节的MAC地址，最后一位为'\0'，共7字节
 */
int HAL_Wireless_GetChildMACs(WirelessType type, char ***mac_list);

#ifdef __cplusplus
}
#endif

#endif // HAL_WIRELESS_H
