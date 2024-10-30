#ifndef MESH_API_H
#define MESH_API_H

/**
 * @brief 初始化Mesh网络
 * @param ssid Mesh网络的SSID
 * @param password Mesh网络的密码
 * @return 0表示成功，-1表示失败
 */
int mesh_init(const char *ssid, const char *password);



#endif

