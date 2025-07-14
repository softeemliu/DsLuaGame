#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__
#include "public.h"
#include "utils/lightlock.h"
#include "netmsgblock.h"

typedef struct _queuenode
{
    netmsgblock* _block;
    struct _queuenode* next;
}queuenode;
//定义一个消息队列结构体
typedef struct
{
    light_lock _lock;
    queuenode* _head;
    queuenode* _tail;
    int _size;
}netmsgqueue;

//单例
netmsgqueue* netmsgqueue_getinstance();
// 初始化队列 
void netmsgqueue_init(netmsgqueue* que);
// 销毁队列（需配合内存释放）
void netmsgqueue_destory(netmsgqueue* que);
// 线程安全插入数据 
void netmsgqueue_pushback(netmsgqueue* que, netmsgblock* data);
// 线程安全获取并移除头元素 
netmsgblock* netmsgqueue_gethead(netmsgqueue* que);
// 获取队列长度 
size_t netmsgqueue_getsize(netmsgqueue* que);

#ifdef _WIN32
DWORD WINAPI netmsgqueue_thread_run(LPVOID arg);
#else
void* netmsgqueue_thread_run(void* arg);
#endif

#endif /*__MSG_QUEUE_H__*/