#include "netmsgblock.h"
#include "mempool.h"

// 初始化函数，替代构造函数 
void netmsgblock_init(netmsgblock* msgblock, netinfo* ni)
{
	msgblock->_maxlen = ni->_buflen + sizeof(ni->_clsptr);
    msgblock->_len = 0;
	msgblock->_pbuff = mempool_allocate(mempool_getinstance(), msgblock->_maxlen);
}

// 释放函数，替代析构函数 
void netmsgblock_destroy(netmsgblock* block)
{
    if( block->_maxlen != 0)
    {
        mempool_deallocate(mempool_getinstance(), block->_pbuff,block->_maxlen);
    }
    block->_pbuff = 0;
    block->_len = 0;
    block->_maxlen = 0;
}

// 向数据块里写网络数据 
void netmsgblock_writenetinfo(netmsgblock* block, const netinfo* ni)
{
	int32_t ptrlen = sizeof(ni->_clsptr);
	//前8个字节保存了网络handle的指针
	memcpy(block->_pbuff, &(ni->_clsptr), ptrlen);
    if ( ni->_buflen > 0)
    {
        memcpy(block->_pbuff+ptrlen , ni->_pbuf, ni->_buflen);
    }
    block->_len = ptrlen + ni->_buflen;
}

// 得到网络数据 
void netmsgblock_getnetdata(netmsgblock* block, netinfo* ni)
{
	memcpy(&(ni->_clsptr), block->_pbuff, sizeof(netinfo*));
	if ((int)(block->_len - sizeof(ni->_clsptr)) > 0)
    {
		ni->_pbuf = block->_pbuff + sizeof(ni->_clsptr);
		ni->_buflen = block->_len - sizeof(ni->_clsptr);
    }
    else
    {
        ni->_pbuf = 0;
        ni->_buflen = 0;
    }
}

// 设置标志 
void netmsgblock_setmsgtype(netmsgblock* block, const int msgtype) { 
   
} 

// 内存分配函数，替代重载 new 
void* netmsgblock_allocate(size_t size)
{
    return mempool_allocate(mempool_getinstance(), sizeof(netmsgblock));
}

// 内存释放函数，替代重载 delete 
void netmsgblock_deallocate(void* p)
{
    mempool_deallocate(mempool_getinstance(),p, sizeof(netmsgblock));
}

// 向数据块里写 
void netmsgblock_write(netmsgblock* block, char* pBuf, int len)
{
    memcpy(block->_pbuff, pBuf, len);
    block->_len = len;
}