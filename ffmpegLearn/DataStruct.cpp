#include <iostream>
#include <stdlib.h>
#include "main.h"

class pktQueue{
public:
    Queue(int Cap = 2){
        head = (Node*)malloc(sizeof(Node));
        head->next = NULL;
        head->data = NULL;
        size = 0;
        Capacity = Cap;
        for(int i = 0 ; i < Capacity ; i++){
            Node* newNode = (Node*)malloc(sizeof(Node));
            newNode->data = NULL;
            newNode->next = head->next;
            head->next = newNode;
            if(i == 0){
                tail = newNode;
            }
        }
        curr = head;
    }
    ~Queue(){ 
        Node *pMove = head->next;
        Node *temp;
        while(pMove != NULL){
            temp = pMove;
            pMove = pMove->next;
            if(temp->data != NULL){
                free(temp->data);
            }
            free(temp);
        }
    }

    void Expand(){
        Capacity *= 2;
        for(int i = size ; i < Capacity ; i++){
            Node* newNode = (Node*)malloc(sizeof(Node));
            newNode->data = NULL;
            newNode->next = curr->next;
            curr->next = newNode;
            if(i == size){
                tail = newNode;
            }
        }
        return;
    }
    // 数据入队
    void Push(void *data){
        if(size == Capacity){
            Expand();
        }
        curr = curr->next;
        curr->data = data;
        size++;
    }
    // 数据出队
    void Pop(){
        if(size == 0){
            return;
        }
        free(head->next->data);
        // 将这个节点的数据给释放，随后将这个节点放入到队尾
        Node *temp = head->next;
        head->next = temp->next;
        temp->data = NULL;
        temp->next = NULL;
        tail->next = temp;
        size--;
    }

    int GetSize() const {
        return size;
    }

    int GetCap() const {
        return Capacity;
    }

    void* Front(){
        if(size == 0){
            return NULL;
        }
        return head->next->data;
    }

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
    
    Node *curr;
    Node *head;
    Node *tail;
    

    pthread_mutex_t Mut;
    pthread_cond_t Cond;
};
