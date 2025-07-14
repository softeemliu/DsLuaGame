#include "netmsgqueue.h"
#include "objectpool/objectpool.h"
#include "protocol.h"
#include "core/network.h"
#include "logger/elog.h"

static netmsgqueue* queue_inst = NULL;

netmsgqueue* netmsgqueue_getinstance() {
    if (!queue_inst) {
        queue_inst = (netmsgqueue*)malloc(sizeof(netmsgqueue));
        netmsgqueue_init(queue_inst);
		//创建线程
#ifdef _WIN32
		CreateThread(NULL, 0, netmsgqueue_thread_run, NULL, 0, NULL);
#else
		pthread_t netmsgqueue_thread;
		pthread_create(&netmsgqueue_thread, NULL, netmsgqueue_thread_run, NULL);
#endif
    }
    return queue_inst;
}

// 初始化队列 
void netmsgqueue_init(netmsgqueue* que)
{
    AUTO_LOCK_INIT(&que->_lock);
    que->_head = NULL;
    que->_tail = NULL;
    que->_size = 0;
}

// 销毁队列（需配合内存释放）
void netmsgqueue_destory(netmsgqueue* que)
{
    AUTO_LOCK(&que->_lock);
    if ( queue_inst != NULL)
    {
        que->_head = NULL;
        que->_tail = NULL;
        que->_size = 0;
        while ( que->_head)
        {
            queuenode* node = que->_head;
            que->_head = node->next;
            free(node);
            node = NULL;
            que->_size --;
        }
    
        free(queue_inst);
        queue_inst = NULL;
    }
    AUTO_UNLOCK(&que->_lock);
    AUTO_LOCK_DESTORY(&que->_lock);
}

// 线程安全插入数据 
void netmsgqueue_pushback(netmsgqueue* que, netmsgblock* data)
{
    AUTO_LOCK(&que->_lock);
    queuenode* node = (queuenode*)malloc(sizeof(queuenode));
	if ( node == NULL)
	{
		AUTO_UNLOCK(&que->_lock);
		return;
	}
    node->_block = data;
    node->next = NULL;

    if ( que->_tail)
    {
        que->_tail->next = node;
        que->_tail = node;
    }
    else
    {
        que->_head = que->_tail = node;
    }
    que->_size++;

    AUTO_UNLOCK(&que->_lock);
}

// 线程安全获取并移除头元素 
netmsgblock* netmsgqueue_gethead(netmsgqueue* que)
{
    AUTO_LOCK(&que->_lock);
    netmsgblock* block = NULL;
    if ( que->_head)
    {
        queuenode* node = que->_head;
        block = node->_block;

        que->_head = node->next;
		if ( que->_head == NULL)
		{
			que->_tail = NULL;
		}
        free(node);
        que->_size--;
    }
    AUTO_UNLOCK(&que->_lock);
    return block;
}

// 获取队列长度 
size_t netmsgqueue_getsize(netmsgqueue* que)
{
    AUTO_LOCK(&que->_lock);
    size_t size = que->_size;
    AUTO_UNLOCK(&que->_lock);
    return size;
}

#ifdef _WIN32
DWORD WINAPI netmsgqueue_thread_run(LPVOID arg)
#else
void* netmsgqueue_thread_run(void* arg) 
#endif
{
	netmsgqueue_getinstance();
    //在此函数中处理数据，然后调用lua接口
    while( true)
    {
		netmsgblock* block = netmsgqueue_gethead(queue_inst);
        if ( block == NULL)
        {
            continue;
        }
        netinfo ni;
        netmsgblock_getnetdata(block, &ni);
		sock_t sock = ni._clsptr->sock;
		//构造一个
		bytestream* btstream = obtain_bytestream();
		bytestream_append(btstream, ni._pbuf, ni._buflen);
		//调用lua方法
		lua_call_mg("onNetworkMsg", sock, btstream);

		giveup_bytestream(btstream);
		netmsgblock_deallocate(block);
    }
}