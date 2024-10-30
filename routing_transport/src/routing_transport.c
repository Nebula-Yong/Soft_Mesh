#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include "hal_wireless.h"
#include "network_fsm.h"
#include "routing_transport.h"

extern MeshNetworkConfig g_mesh_config;

// 定义宏开关，打开或关闭日志输出
#define ENABLE_LOG 1  // 1 表示开启日志，0 表示关闭日志

// 定义 LOG 宏，如果 ENABLE_LOG 为 1，则打印日志，并输出文件名、行号、日志内容
#if ENABLE_LOG
    #define LOG(fmt, ...) printf("LOG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...) // 空实现，日志输出被禁用
#endif

// 路由传输层开启标志位
extern osEventFlagsId_t route_transport_event_flags;
#define ROUTE_TRANSPORT_START_BIT (1 << 0)
#define ROUTE_TRANSPORT_STOP_BIT  (1 << 1)

HashTable* table = NULL;  // 定义哈希表

struct Graph* graph = NULL;  // 定义图

void send_data_packet(const char *dest_mac, const char *data);

// 简单的哈希函数，将MAC地址转化为哈希值
unsigned int hash(unsigned char *mac) {
    unsigned int hash_value = 0;
    for (int i = 0; i < MAC_SIZE; i++) {
        hash_value = hash_value * 31 + mac[i];  // 31是常用的哈希基数
    }
    return hash_value % HASH_TABLE_SIZE;
}

// 创建哈希表
HashTable* create_hash_table(void) {
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    
    // 初始化哈希表的buckets
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }

    // 动态分配indexToMac数组，每个元素是一个MAC地址
    table->indexToMac = (unsigned char**)malloc(MAX_NODES * sizeof(unsigned char*));
    for (int i = 0; i < MAX_NODES; i++) {
        table->indexToMac[i] = NULL;  // 初始化为NULL
    }
    table->num_nodes = 0;
    return table;
}

// 向哈希表中插入MAC地址和对应的索引
void insert(HashTable* table, unsigned char *mac, int index) {
    unsigned int bucket_index = hash(mac);
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    memcpy(new_node->mac, mac, MAC_SIZE);
    new_node->index = index;
    new_node->next = table->buckets[bucket_index];
    table->buckets[bucket_index] = new_node;
    // 为indexToMac[index]动态分配空间并存储MAC地址
    if (table->indexToMac[index] == NULL) {
        table->indexToMac[index] = (unsigned char*)malloc(MAC_SIZE * sizeof(unsigned char));
    }
    memcpy(table->indexToMac[index], mac, MAC_SIZE);
    table->num_nodes++;
}

// 根据MAC地址查找索引
int find(HashTable* table, unsigned char *mac) {
    unsigned int bucket_index = hash(mac);
    HashNode* node = table->buckets[bucket_index];
    
    while (node != NULL) {
        if (memcmp(node->mac, mac, MAC_SIZE) == 0) {
            return node->index;  // 找到匹配的MAC地址
        }
        node = node->next;
    }
    return -1;  // 如果没有找到，返回-1
}

// 释放哈希表
void free_hash_table(HashTable* table) {
    if (table == NULL || table->buckets == NULL) {
        return;  // 防止传入空表或未初始化的桶
    }

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode* node = table->buckets[i];
        while (node != NULL) {
            HashNode* temp = node;
            node = node->next;
            free(temp);
        }
    }

    // 释放indexToMac中每个动态分配的MAC地址
    for (int i = 0; i < MAX_NODES; i++) {
        if (table->indexToMac[i] != NULL) {
            free(table->indexToMac[i]);
        }
    }
    free(table->indexToMac);
    free(table->buckets);
    free(table);
}

// 清除除0号节点的映射外的所有映射
void clean_hash_table(HashTable* table) {
    LOG("Cleaning hash table...");
    if (table == NULL || table->buckets == NULL) {
        return;  // 防止传入空表或未初始化的桶
    }
    unsigned char mac[MAC_SIZE];
    memcpy(mac, table->indexToMac[0], MAC_SIZE);  // 保存0号节点的MAC地址
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        table->indexToMac[i] = NULL;  // 清空indexToMac
        HashNode* node = table->buckets[i];
        while (node != NULL) {
            HashNode* temp = node;
            node = node->next;
            free(temp);
        }
        table->buckets[i] = NULL;  // 清空哈希桶
    }
    table->num_nodes = 0;  // 保留0号节点
    insert(table, mac, 0);  // 重新插入0号节点
}

// 创建一个图节点
struct Node* createNode(int v) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->vertex = v;
    newNode->next = NULL;
    return newNode;
}

// 创建图（邻接表）
struct Graph* createGraph(int vertices) {
    struct Graph* graph = (struct Graph*)malloc(sizeof(struct Graph));
    graph->numVertices = vertices;

    // 创建一个大小为numVertices的数组，每个元素是一个Node指针
    graph->adjLists = (struct Node**)malloc(vertices * sizeof(struct Node*));
    graph->parentArray = (int*)malloc(vertices * sizeof(int));  // 初始化父节点数组

    // 初始化每个邻接表为空（NULL），并将父节点设为-1（表示未定义）
    for (int i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
        graph->parentArray[i] = -1;  // 所有节点的父节点初始设为-1
    }
    return graph;
}

// 添加边，并记录父节点
void addEdge(struct Graph* graph, int src, int dest) {
    // 从src到dest添加一条边
    struct Node* newNode = createNode(dest);
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;

    // 记录dest的父节点为src
    graph->parentArray[dest] = src;

    // 因为是无向图，从dest到src也要添加一条边
    newNode = createNode(src);
    newNode->next = graph->adjLists[dest];
    graph->adjLists[dest] = newNode;
}

// 打印从某个节点到根节点的路径
void printPathToRoot(struct Graph* graph, int vertex) {
    while (vertex != -1) {  // 如果父节点为 -1，则到达根节点
        printf("%d ", vertex);
        vertex = graph->parentArray[vertex];
    }
    printf("\n");
}

// 打印图的邻接表
void printGraph(struct Graph* graph) {
    for (int v = 0; v < graph->numVertices; v++) {
        struct Node* temp = graph->adjLists[v];
        printf("Node %d: ", v);
        while (temp) {
            printf("%d -> ", temp->vertex);
            temp = temp->next;
        }
        printf("NULL\n");
    }
}

void free_graph(struct Graph* graph) {
    if (graph == NULL) {
        return;  // 如果图为空，则直接返回
    }
    
    // 释放邻接表中的每个链表节点
    if (graph->adjLists != NULL) {
        for (int i = 0; i < graph->numVertices; i++) {
            struct Node* temp = graph->adjLists[i];
            while (temp != NULL) {
                struct Node* next = temp->next;
                free(temp);
                temp = next;
            }
        }
        free(graph->adjLists);  // 释放邻接表数组
    }
    
    // 释放父节点数组
    if (graph->parentArray != NULL) {
        free(graph->parentArray);
    }
    
    free(graph);  // 释放图结构本身
}

// 将unsigned char类型（6字节）转为char类型（7字节）的MAC地址字符串
void uc2c(unsigned char *mac, char *str) {
    for (int i = 0; i < MAC_SIZE; i++) {
        str[i] = mac[i];
    }
    str[MAC_SIZE] = '\0';  // 添加字符串终止符
}

// 生成格式化字符串
void generateFormattedString(struct Graph* graph, HashTable* table, char *output) {
    // 在前面添加 "0\nX\n"，其中 X 是节点的数量
    sprintf(output, "0\n%d\n", table->num_nodes);
    for (int i = 0; i < table->num_nodes; i++)
    {
        char macStr[7]; // 存储MAC地址字符串（12个字符+终止符）
        uc2c(table->indexToMac[i], macStr);
        // 添加MAC地址和父节点信息
        sprintf(output + strlen(output), "%s %d", macStr, graph->parentArray[i]);
        // 添加\n作为节点之间的分隔符
        if (i < table->num_nodes - 1) {
            strcat(output, "\n");
        }
    }
}

void copy_graph(struct Graph* src, struct Graph* dest) {
    for (int i = 0; i < src->numVertices; i++) {
        if (src->parentArray[i] == -1) {
            continue;  // 跳过根节点
        }
        addEdge(dest, src->parentArray[i], i);
    }
}

void del_sub_tree(struct Graph* graph, int node) {
    struct Node* temp = graph->adjLists[node];
    while (temp != NULL) {
        if (temp->vertex == graph->parentArray[node]) {
            temp = temp->next;
            continue;
        }
        del_sub_tree(graph, temp->vertex);
        struct Node* next = temp->next;
        free(temp);
        temp = next;
    }
    graph->adjLists[node] = NULL;
    graph->parentArray[node] = -1;
}

// id表示当前节点，new_id表示新图中的当前节点
void regen_tree(struct Graph* graph, HashTable* table, struct Graph* new_graph, HashTable* new_table, int id, int new_id) {
    // 依次处理当前节点的子节点
    for (struct Node* temp = graph->adjLists[id]; temp != NULL; temp = temp->next) {
        if (temp->vertex == graph->parentArray[id]) {
            continue;  // 跳过父节点
        }
        // 添加边
        addEdge(new_graph, new_id, new_table->num_nodes);
        // 将MAC地址插入哈希表
        insert(new_table, table->indexToMac[temp->vertex], new_table->num_nodes);
        // 递归处理子节点
        regen_tree(graph, table, new_graph, new_table, temp->vertex, new_table->num_nodes - 1);
    }
}

void del_then_gen(struct Graph** p_graph, HashTable** p_table, int node) {
    struct Graph* graph = *p_graph;
    HashTable* table = *p_table;
    // 先删除父节点到该节点的边
    int parent = graph->parentArray[node];
    struct Node* temp = graph->adjLists[parent];
    struct Node* pre = NULL;
    while (temp != NULL) {
        if (temp->vertex == node) {
            if (pre == NULL) {
                graph->adjLists[parent] = temp->next;
            }else{
                pre->next = temp->next;
            }
            free(temp);
            break;
        }
        pre = temp;
        temp = temp->next;
    }

    // 删除该节点到子节点的边
    
    del_sub_tree(graph, node);
    int deleted_nodes = 0;
    for (int i = 1; i < table->num_nodes; i++) {
        if (graph->parentArray[i] == -1) {
            deleted_nodes++;
        }
    }

    struct Graph* new_graph = createGraph(table->num_nodes - deleted_nodes);
    HashTable* new_table = create_hash_table();

    insert(new_table, table->indexToMac[0], 0);
    regen_tree(graph, table, new_graph, new_table, 0, 0);

    free_graph(graph);
    free_hash_table(table);
    *p_graph = new_graph;
    *p_table = new_table;
}

// 从字符串中解析出节点信息并添加到图中
void add_tree_node(const char *mac, HashTable** p_table, struct Graph** p_graph, char* data) {
    HashTable* table = *p_table;
    struct Graph* graph = *p_graph;
    if (find(table, (unsigned char*)mac) != -1) {  // 说明该节点的子树已经存在
        LOG("Node %s already exists.\n", mac);
        del_then_gen(&graph, &table, find(table, (unsigned char*)mac));  // 先删除再生成
    }
    // 使用 strtok 解析出第一行和第二行
    char* token = strtok(data, "\n"); // 第一次调用，获取路由包类型 
    token = strtok(NULL, "\n");       // 第二次调用，获取节点数
    int num_nodes = atoi(token);
    struct Graph* new_graph = NULL;
    if (graph == NULL) {
        new_graph = createGraph(num_nodes + 1);  // 创建图，节点数为num_nodes + 1
    }else{
        new_graph = createGraph(graph->numVertices + num_nodes); 
        copy_graph(graph, new_graph);
    }
    int offset = table->num_nodes; 
    LOG("offset: %d\n", offset);
    // 使用 strtok 解析出每个节点的信息
    for (int i = 0; i < num_nodes; i++) {
        token = strtok(NULL, "\n");
        // 使用 sscanf 从每一行中解析数据
        int parent_index;
        char node_mac[7]; // 假设 MAC 地址不会超过 6 字符
        sscanf(token, "%s %d", node_mac, &parent_index);
        if (parent_index == -1) {  // 如果时根节点
            addEdge(new_graph, 0, table->num_nodes);
            insert(table, (unsigned char*)node_mac, table->num_nodes);
            continue;
        }
        parent_index = parent_index + offset; // 为了避免和已有节点冲突，将节点索引加上已有节点数
        // 添加边
        addEdge(new_graph, parent_index, table->num_nodes);
        // 将MAC地址插入哈希表
        insert(table, (unsigned char*)node_mac, table->num_nodes);
    }
    if (graph != NULL) {
        free_graph(graph);
    }
    *p_table = table;
    *p_graph = new_graph;
}

// 处理路由包
void process_route_packet(const char *mac, char *data)
{
    LOG("Received route packet from MAC: %s, data:\n%s\n", mac, data);
    if (table == NULL) {
        LOG("ERROR: hash Table is NULL.\n");
    }

    add_tree_node(mac, &table, &graph, data);
    LOG("add_tree_node success!");
    printGraph(graph);
    LOG("printGraph success!");

    char* output = (char*)malloc(11 * table->num_nodes * sizeof(char));
    generateFormattedString(graph, table, output);

    // 发送自己的路由表给父节点
    HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, output, g_mesh_config.tree_level - 1);

    free(output);
    
}

// 处理数据包
/*
| [0]:数据包类型 | [1-6]:源节点MAC地址 | [7-12]:目标节点MAC地址 | [13]:数据包状态 | [14-16]:数据包编号 | [17-18]:校验位（可选） | [19-512]数据位 |
| -------------- | ------------------- | ---------------------- | --------------- | ------------------ | ---------------------- | -------------- |
| 1:表示数据透传 | A1B2C3              | B2C3A1                 | 0：发送包       | 000~999            |                        |                |
*/
// 创建一个数据包的数据结构
typedef struct {
    char type;  // 数据包类型
    char src_mac[MAC_SIZE];  // 源节点MAC地址
    char dest_mac[MAC_SIZE];  // 目标节点MAC地址
    char status;  // 数据包状态
    char packet_num[3];  // 数据包编号
    char crc[2];        // 校验位
    char data[494];  // 数据位
} DataPacket;

DataPacket parse_data_packet(const char *data) {
    DataPacket packet;
    packet.type = data[0];
    memcpy(packet.src_mac, data + 1, MAC_SIZE);
    memcpy(packet.dest_mac, data + 7, MAC_SIZE);
    packet.status = data[13];
    memcpy(packet.packet_num, data + 14, 3);
    memcpy(packet.crc, data + 17, 2);
    memcpy(packet.data, data + 19, 494);
    return packet;
}

char* generate_data_packet(DataPacket packet) {
    char* data = (char*)malloc(513 * sizeof(char));
    data[0] = packet.type;
    memcpy(data + 1, packet.src_mac, MAC_SIZE);
    memcpy(data + 7, packet.dest_mac, MAC_SIZE);
    data[13] = packet.status;
    memcpy(data + 14, packet.packet_num, 3);
    packet.crc[0] = '0';
    packet.crc[1] = '0';
    memcpy(data + 17, packet.crc, 2);
    memcpy(data + 19, packet.data, 494);
    return data;
}

void broadcast_data_packet(DataPacket packet) {
    char* packet_data = generate_data_packet(packet);
    char** mac_list = NULL;
    int len_mac_list = HAL_Wireless_GetChildMACs(DEFAULT_WIRELESS_TYPE, &mac_list);
    for (int i = 0; i < len_mac_list; i++) {
        HAL_Wireless_SendData_to_child(DEFAULT_WIRELESS_TYPE, mac_list[i], packet_data);
        free(mac_list[i]);  // 顺便清理内存
    }
    free(packet_data);
    free(mac_list);
}

void send_ack_packet(char* my_mac, DataPacket packet) {
    memcpy(packet.dest_mac, packet.src_mac, MAC_SIZE);
    memcpy(packet.src_mac, my_mac, MAC_SIZE);
    packet.status = '1';
    strcpy(packet.data, "Received");
    char* ack_data = generate_data_packet(packet);
    HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, ack_data, g_mesh_config.tree_level - 1);
    free(ack_data);
}

void process_data_packet(const char *mac, char *data)
{
    LOG("Received data packet from MAC: %s, data: %s\n", mac, data);
    // 解析数据包
    DataPacket packet = parse_data_packet(data);

    // 如果是广播数据包，直接向下广播
    if (strncmp(packet.dest_mac, "FFFFFF", MAC_SIZE) == 0) {
        LOG("Broadcast data packet.\n");
        LOG("Broadcast data: %s", packet.data);
        broadcast_data_packet(packet);
        return;
    }

    // 如果是广播请求包，并且自己是根节点，则开始广播
    if (packet.status == '3' && g_mesh_config.tree_level == 0) {
        LOG("Received broadcast request.\n");
        strcpy(packet.dest_mac, "FFFFFF");
        packet.status = '4';  // 表示广播包
        broadcast_data_packet(packet);
        return;
    }
    // 获取自己的MAC地址
    char my_mac[MAC_SIZE + 1] = {0};
    if(HAL_Wireless_GetNodeMAC(DEFAULT_WIRELESS_TYPE, my_mac) != 0) {
        LOG("Failed to get MAC address.\n");
    }
    // 如果是目标节点，则处理数据包
    if (strncmp(packet.dest_mac, my_mac, MAC_SIZE) == 0) {
        LOG("Received data packet for me.\n");
        LOG("Data: %s\n", packet.data);
        // 回应收到, status = 1
        if (packet.status == '0') {
            send_ack_packet(my_mac, packet);
        }
    } else {
        // 如果不是目标节点，则转发数据包
        LOG("Forwarding data packet...\n");
        // 查找哈希表，分析节点是否在图中
        int dest_index = find(table, (unsigned char*)packet.dest_mac);
        if (dest_index == -1) {
            LOG("Forwarding data packet to parent node.\n");
            if (g_mesh_config.tree_level == 0) {
                LOG("target node not in mesh network\n");
                // 回应status = 2
                memcpy(packet.dest_mac, packet.src_mac, MAC_SIZE);
                memcpy(packet.src_mac, my_mac, MAC_SIZE);
                packet.status = '2';
                strcpy(packet.data, "Target node not in mesh network");
                char* ack_data = generate_data_packet(packet);
                HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, ack_data, g_mesh_config.tree_level - 1);
                free(ack_data);
                return;
            }
            HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, data, g_mesh_config.tree_level - 1);
            return;
        }else {
            LOG("Forwarding data packet to child node.\n");
            while (graph->parentArray[dest_index] != 0) 
            {
                dest_index = graph->parentArray[dest_index];
            }
            char dest_mac[7] = {0};
            uc2c(table->indexToMac[dest_index], dest_mac);
            HAL_Wireless_SendData_to_child(DEFAULT_WIRELESS_TYPE, dest_mac, data);
        }
    }
}

void send_data_packet(const char *dest_mac, const char *data) {
    // 创建数据包
    DataPacket packet;
    packet.type = '1';
    char my_mac[MAC_SIZE + 1] = {0};
    if(HAL_Wireless_GetNodeMAC(DEFAULT_WIRELESS_TYPE, my_mac) != 0) {
        LOG("Failed to get MAC address.\n");
    }
    memcpy(packet.src_mac, my_mac, MAC_SIZE);
    memcpy(packet.dest_mac, dest_mac, MAC_SIZE);
    packet.status = '0';
    memcpy(packet.packet_num, "000", 3);
    strcpy(packet.data, data);
    char* packet_data = generate_data_packet(packet);
    LOG("Sending data packet to MAC: %s, data: %s\n", dest_mac, packet_data);
    // 发送数据包
    if (find(table, (unsigned char*)dest_mac) == -1) {
        HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, packet_data, g_mesh_config.tree_level - 1);
        LOG("Forwarding data packet to parent node.\n");
    }else {
        HAL_Wireless_SendData_to_child(DEFAULT_WIRELESS_TYPE, dest_mac, packet_data);
        LOG("Forwarding data packet to child node.\n");
    }
    free(packet_data);
}

// 发送自己的路由表给父节点
void send_route_table_to_parent(void)
{
    // 获取子节点的MAC地址
    char** mac_list = NULL;
    int len_mac_list = HAL_Wireless_GetChildMACs(DEFAULT_WIRELESS_TYPE, &mac_list);
    for (int i = 0; i < len_mac_list; i++) {
        LOG("Child MAC: %s\n", mac_list[i]);
    }
    
    // 获取自己的MAC地址
    char my_mac[MAC_SIZE + 1] = {0};
    if(HAL_Wireless_GetNodeMAC(DEFAULT_WIRELESS_TYPE, my_mac) != 0) {
        LOG("Failed to get MAC address.\n");
    }
    // 创建哈希表，存储MAC地址和索引的映射
    table = create_hash_table();
    insert(table, (unsigned char*)my_mac, 0);  
    
    // 发送自己的路由表给父节点
    if (len_mac_list == 0 && g_mesh_config.tree_level != 0) {
        LOG("No child nodes.\n");
        char msg[10];
        sprintf(msg, "0\n1\n%s -1", my_mac);
        HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, msg, g_mesh_config.tree_level - 1);
        return;
    }

}

void del_overdue_nodes(void) {
    if (graph == NULL) {
        return;
    }
    printGraph(graph);
    // 获取子节点的MAC地址
    char** mac_list = NULL;
    int len_mac_list = HAL_Wireless_GetChildMACs(DEFAULT_WIRELESS_TYPE, &mac_list);
    if (len_mac_list == 0) {
        free_graph(graph);
        clean_hash_table(table);
        graph = NULL;
        char msg[10];
        sprintf(msg, "0\n1\n%s -1", table->indexToMac[0]);
        HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, msg, g_mesh_config.tree_level - 1);
        return;
    }

    // 从graph中获取根节点的子节点的MAC地址
    char ** graph_mac_list = (char**)malloc(15 * sizeof(char*));
    int graph_len_mac_list = 0;
    for (int i = 0; i < graph->numVertices; i++) {
        if (graph->parentArray[i] == 0) {
            graph_mac_list[graph_len_mac_list] = (char*)malloc(7 * sizeof(char));
            uc2c(table->indexToMac[i], graph_mac_list[graph_len_mac_list]);
            graph_len_mac_list++;
        }
    }
    // 比较两个MAC地址列表，删除过期的节点
    for (int i = 0; i < graph_len_mac_list; i++) {
        int found = 0;
        for (int j = 0; j < len_mac_list; j++) {
            if (strcmp(graph_mac_list[i], mac_list[j]) == 0) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            del_then_gen(&graph, &table, find(table, (unsigned char*)graph_mac_list[i]));
        }
    }

    // 清理分配的地址
    for (int i = 0; i < len_mac_list; i++) {
        free(mac_list[i]);
    }
    for (int i = 0; i < graph_len_mac_list; i++) {
        free(graph_mac_list[i]);
    }
    free(graph_mac_list);
    free(mac_list);
}

void route_transport_task(void)
{
    int server_fd = HAL_Wireless_CreateServer(DEFAULT_WIRELESS_TYPE);
    if (server_fd < 0) {
        LOG("Failed to create server.\n");
        return;
    }
    LOG("Server created successfully.\n");
    int status = 1;
    while (1)
    {
        uint32_t flags = osEventFlagsWait(route_transport_event_flags, ROUTE_TRANSPORT_START_BIT | ROUTE_TRANSPORT_STOP_BIT, osFlagsWaitAny, 200);
        del_overdue_nodes();
        LOG("flag:0x%08X\n", flags);
        if (flags & ROUTE_TRANSPORT_STOP_BIT && flags != osFlagsErrorTimeout) {
            LOG("Stop route transport task.\n");
            // 清空哈希表和图
            free_hash_table(table);
            free_graph(graph);
            table = NULL;
            graph = NULL;
            status = 0;
        }else if (flags & ROUTE_TRANSPORT_START_BIT && flags != osFlagsErrorTimeout) {
            LOG("Start route transport task.\n");
            send_route_table_to_parent();
            status = 1;
        }
        if (status == 0) {
            continue;
        }
        char mac[7] = {0};
        char buffer[50] = {0};
        int ret = HAL_Wireless_ReceiveDataFromClient(DEFAULT_WIRELESS_TYPE, server_fd, mac, buffer, sizeof(buffer));
        if (ret < 0) {
            LOG("nothing sent from client.\n");
            continue;
        }
        switch (buffer[0])
        {
        case'0':
            // 路由包
            process_route_packet(mac, buffer);
            break;
        case '1':
            // 数据包
            process_data_packet(mac, buffer);
            break;
        default:
            break;
        }
        LOG("Received data from client: %s, MAC: %s\n", buffer, mac);
    }
    HAL_Wireless_CloseServer(DEFAULT_WIRELESS_TYPE, server_fd);
    
    return;
}

/* 创建任务 */
static void route_transport_entry(void)
{
    osThreadAttr_t attr;
    attr.name       = "route_transport_task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = 0x1000;
    attr.priority   = osPriorityLow4;

    if (osThreadNew((osThreadFunc_t)route_transport_task, NULL, &attr) == NULL) {
        LOG("Create route_transport_task failed.\n");
    } else {
        LOG("Create route_transport_task successfully.\n");
    }
}

/* 启动任务 */
app_run(route_transport_entry);