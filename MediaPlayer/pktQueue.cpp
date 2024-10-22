#include <mediacore.h>


Queue::Queue(int Cap){
    Capacity = Cap;
    size = 0;
    head = (Node*)malloc(sizeof(Node));
    head->next = NULL;
    head->data = NULL;
    curr = head;

    int i;
    // 建立Capacity个新的节点
    for(i = 0; i < Capacity ; i++){
        Node *newNode = (Node*)malloc(sizeof(Node));
        newNode->next = head->next;
        newNode->data = NULL;
        head->next = newNode;
        if(i == 0){
            tail= newNode;
        }
    }
    pthread_mutex_init(&Mut,NULL);
    pthread_cond_init(&Cond,NULL);
}
Queue::~Queue(){
    Node *pMove = head->next;
    Node *temp;
    while(pMove != NULL){
        temp = pMove;
        pMove = pMove->next;
        if(temp->data == NULL){
            free(temp->data);
        }
        free(temp);
    }
}
//放入
void Queue::Push(void *data){
    pthread_mutex_lock(&Mut);
    // 先判断当前的Size和Capacity是否相同
    if(size == Capacity){
        expand();
    }
    curr = curr->next;
    curr->data = data;
    size++;
    pthread_mutex_unlock(&Mut);
}
// 队头的数据出队
void Queue::Pop(){
    pthread_mutex_lock(&Mut);
    if(size == 0){
        return;
    }
    // 释放节点数据
    Node *temp = head->next;
    if(temp == curr){
        curr = head;
    }
    head->next = temp->next;
    temp->next = NULL;
    free(temp->data);
    temp->data = NULL;

    // 将节点插入到队尾从新进行利用
    tail->next = temp;
    tail = temp;
    size--;
    pthread_mutex_unlock(&Mut);
}
// 获取到队头的数据
void* Queue::Front(){
    if(size > 0){
        return head->next->data;
    }
    return NULL;
}
// 进行二倍扩容
void Queue::expand(){
    for(int i = 0; i < Capacity ; i++){
        Node *newNode = (Node*)malloc(sizeof(Node));
        newNode->next = tail->next;
        tail->next = newNode;
        tail = newNode;
    }
    Capacity *= 2;
}

int Queue::GetSize(){
    return size;
}

int Queue::GetCap(){
    return Capacity;
}

void Queue::ClearQueue(){
    if(size == 0){
        return;
    }
    Node* pMove = head->next;
    while(pMove != curr){
        free(pMove->data);
        pMove = pMove->next;
    }
    free(pMove->data);
    curr = head;
    return;
}
