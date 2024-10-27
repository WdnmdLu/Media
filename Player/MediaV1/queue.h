#ifndef QUEUE_H
#define QUEUE_H
#include <pthread.h>

class Queue{
public:
    Queue(int Cap = 2);
    ~Queue();
    //放入
    void Push(void *data);
    // 队头的数据出队
    void Pop();
    // 获取到队头的数据
    void *Front();
    // 进行二倍扩容
    void expand();

    int GetSize();

    int GetCap();
    // 释放掉队列中的所有数据，将队列清空
    void ClearQueue();

private:
    struct Node{
        struct Node *next;
        // 通过void*用户可以指定任意的数据类型，可以是基本数据类型，int，float或者特殊结构体，AVPacket，AVFrame等等
        void* data;
    };
private:
    // 队列当前的数据量大小
    int size;
    // 队列的容量
    int Capacity;
    // 队头指针，用于表示整个队列
    Node *head;
    // 最新入队的数据
    Node *curr;
    // 指向最后一个位置的节点
    Node *tail;

    pthread_mutex_t Mut;
    pthread_cond_t Cond;
};
#endif // QUEUE_H
