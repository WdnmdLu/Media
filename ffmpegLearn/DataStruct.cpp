#include <iostream>
#include <stdlib.h>
#include "main.h"

pktQueue::pktQueue(){
    head = (Node*)malloc(sizeof(Node));
    head->next =NULL;
    head->data = NULL;
    tail = head;
    this->Size = 0;
    pthread_mutex_init(&Mut,NULL);
}
pktQueue::~pktQueue(){
    Node *pMove = head->next;
    Node *pre = head->next;
    while(pMove != NULL){
        pMove = pMove->next;
        free(pre->data);
        free(pre);
        pre = pMove;
    }
    free(head);
}

void pktQueue::Push(void *data){
    //获取锁
    pthread_mutex_lock(&Mut);

    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = tail->next;
    tail->next = newNode;
    tail = newNode;
    Size++;
    // 释放锁
    pthread_mutex_unlock(&Mut);
}
void pktQueue::Pop(){
    if(Size == 0){
        return;
    }
    pthread_mutex_lock(&Mut);

    Node *temp = head->next;
    head->next = temp->next;
    free(temp->data);
    free(temp);
    Size--;
    if(Size == 0){
        tail = head;
    }

    pthread_mutex_unlock(&Mut);
    return;
}
void *pktQueue::Back(){
    if(Size == 0){
        return NULL;
    }
    AVPacket *pkt = (AVPacket*)head->next->data;
    printf("%p\n",pkt);
    return head->next->data;
}

int pktQueue::GetSize(){
   return Size;
}
