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
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
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
    
    free(table->buckets);  // 释放桶数组
    free(table);           // 释放哈希表本身
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
        LOG("%d ", vertex);
        vertex = graph->parentArray[vertex];
    }
    LOG("\n");
}

// 打印图的邻接表
void printGraph(struct Graph* graph) {
    for (int v = 0; v < graph->numVertices; v++) {
        struct Node* temp = graph->adjLists[v];
        LOG("Node %d: ", v);
        while (temp) {
            LOG("%d -> ", temp->vertex);
            temp = temp->next;
        }
        LOG("NULL\n");
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


// 处理路由包
void process_route_packet(const char *mac, const char *data)
{
    LOG("Received route packet from MAC: %s, data: %s\n", mac, data);
    // 解析数据包
    // 状态1：节点原本是子节点，hashtable和图都是空的
    // 解析数据包，加入子节点的哈希表和图

    // 状态2：节点已经是父节点，hashtable和图都不为空
    // 解析数据包，更新子节点的哈希表和图

    // 状态3：节点是子节点，hashtable和图都不为空
    // 解析数据包，更新子节点的哈希表和图

    // 最后检查，从图中获取当前节点的所有子节点，从路由表中获取当前节点的所有子节点，如果不一致，以路由表为准，更新图。


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

    if (len_mac_list == 0 && g_mesh_config.tree_level != 0) {
        LOG("No child nodes.\n");
        char msg[10];
        sprintf(msg, "0\n1\n%s", my_mac);
        HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, msg, g_mesh_config.tree_level - 1);
        return;
    }

    // // 创建哈希表，存储MAC地址和索引的映射
    // HashTable* table = create_hash_table();
    // insert(table, my_mac, 0);  // 根节点的索引为0
    // for (int i = 1; i <= len_mac_list; i++) {
    //     insert(table, (unsigned char*)mac_list[i], i);
    // }

    // // 创建一个图，表示邻接表
    // struct Graph* graph = createGraph(len_mac_list + 1);  // 6个节点

    // // 添加边
    // for (int i = 0; i < len_mac_list; i++) {
    //     addEdge(graph, find(table, my_mac), find(table, (unsigned char*)mac_list[i]));  // 0与1相连
    // }

    // // 打印图的邻接表
    // printGraph(graph);
    // HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, "Hello, parent!", g_mesh_config.tree_level);

    // // 释放所有内存
    // free_hash_table(table);
    // free(graph->parentArray);
    // free(graph->adjLists);
    // free(graph);
    // for (int i = 0; i < len_mac_list; i++) {
    //     free(mac_list[i]);
    // }

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
        uint32_t flags = osEventFlagsWait(route_transport_event_flags, ROUTE_TRANSPORT_START_BIT | ROUTE_TRANSPORT_STOP_BIT, osFlagsWaitAny, 100);
        LOG("flag:0x%08X\n", flags);
        if (flags & ROUTE_TRANSPORT_STOP_BIT && flags != osFlagsErrorTimeout) {
            LOG("Stop route transport task.\n");
            // 清空哈希表和图
            free_hash_table(table);
            free_graph(graph);
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