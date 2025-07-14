#ifndef __NET_MSG_BLOCK_H__
#define __NET_MSG_BLOCK_H__
#include "tolua/bridge/luacallbridge.h"

typedef struct _ClientSession ClientSession;

typedef struct
{
	unsigned char* _pbuf;
	int32_t _buflen;
	ClientSession* _clsptr;
}netinfo;

//定义netmsgblock结构体
typedef struct
{
	int _maxlen;
	int _len;
	char* _pbuff;
}netmsgblock;

// 初始化函数，替代构造函数 
void netmsgblock_init(netmsgblock* msgblock, netinfo* ni);

// 释放函数，替代析构函数 
void netmsgblock_destroy(netmsgblock* block);

// 向数据块里写网络数据 
void netmsgblock_writenetinfo(netmsgblock* block, const netinfo* ni);

// 得到网络数据 
void netmsgblock_getnetdata(netmsgblock* block, netinfo* ni);

// 设置标志 
void netmsgblock_setmsgtype(netmsgblock* block, const int msgtype);
// 内存分配函数，替代重载 new 
void* netmsgblock_allocate(size_t size);

// 内存释放函数，替代重载 delete 
void netmsgblock_deallocate(void* p);

// 向数据块里写 
void netmsgblock_write(netmsgblock* block, char* pBuf, int len);

#endif /*__NET_MSG_BLOCK_H__*/