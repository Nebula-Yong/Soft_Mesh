#ifndef MESH_API_H
#define MESH_API_H

/**
 * @brief 初始化Mesh网络
 * @param ssid Mesh网络的SSID
 * @param password Mesh网络的密码
 * @return 0表示成功，-1表示失败
 */
int mesh_init(const char *ssid, const char *password);

/**
 * @brief 发送数据给Mesh网络中的其他节点
 * @param dest_mac 目标节点的MAC地址
 * @param data 要发送的数据
 * @return 0表示成功，-1表示失败
 */
int mesh_send_data(const char *dest_mac, const char *data);

/**
 * @brief 广播数据给Mesh网络中的所有节点
 * @param data 要发送的数据
 * @return 0表示成功，-1表示失败
 */
int mesh_broadcast(const char *data);

#endif

