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
    free(table);
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
    /*
    //多个节点的情况
    0          // 路由包类型，0表示路由表
    4        // 节点数
    3C:4D:5E -1   // MAC地址, 父节点
    3C:4D:5F 0    // 
    3C:4D:60 0    // 
    3C:4D:61 1    // 
    //单个节点的情况
    0          // 路由包类型，0表示路由表
    1        // 节点数
    1 3C:4D:5E 0  // 节点1, MAC地址, 邻居数量, 邻居节点编号列表
     */
    LOG("Received route packet from MAC: %s, data:\n %s\n", mac, data);
    if (table == NULL) {
        LOG("ERROR: hash Table is NULL.\n");
    }

    add_tree_node(mac, &table, &graph, data);
    printGraph(graph);

    char* output = (char*)malloc(11 * table->num_nodes * sizeof(char));
    generateFormattedString(graph, table, output);

    // 发送自己的路由表给父节点
    HAL_Wireless_SendData_to_parent(DEFAULT_WIRELESS_TYPE, output, g_mesh_config.tree_level - 1);

    free(output);


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
    return;
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
        del_overdue_nodes();
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