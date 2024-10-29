#ifndef ROUTING_TRANSPORT_H
#define ROUTING_TRANSPORT_H

#define MAC_SIZE 6
#define HASH_TABLE_SIZE 100     // 哈希表大小，选择适当大小避免冲突过多
#define MAX_NODES 100           // 最大节点数量

// 哈希表节点
typedef struct HashNode {
    unsigned char mac[MAC_SIZE];
    int index;
    struct HashNode* next;
} HashNode;

// 哈希表
typedef struct {
    HashNode* buckets[HASH_TABLE_SIZE];
    int num_nodes;
    unsigned char** indexToMac;  // 二维指针，用于动态存储每个index对应的MAC地址
} HashTable;

// 定义邻接表节点结构
struct Node {
    int vertex;
    struct Node* next;
};

// 定义图结构，表示邻接表
struct Graph {
    int numVertices;
    struct Node** adjLists;
    int* parentArray;  // 父节点数组
};

#endif