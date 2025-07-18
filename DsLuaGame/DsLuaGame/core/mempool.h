#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#else
#include <pthread.h>
#endif

//引入一个锁
#include "utils/lightlock.h"
 
#define _ALIGN 8 
#define _MAX_BYTES 512 
#define _NFREELISTS 64
 
typedef struct _Obj {
    struct _Obj* _M_free_list_link;
    char _M_client_data[1];
} _Obj;
 
 
typedef struct _mempool {
    light_lock _light_lock;
    _Obj* _S_free_list[_NFREELISTS];
    char* _S_start_free;
    char* _S_end_free;
    size_t _S_heap_size;
} mempool;
 


mempool* mempool_getinstance();

// 初始化函数 
void mempool_init(mempool* self);

// 内存对齐计算 
size_t mempool_s_round_up(size_t bytes);

// 计算自由列表索引 
int mempool_s_freelist_index(size_t bytes);

// 分配新内存块
static char* mempool_s_chunk_alloc(mempool* self, size_t size, int* nobjs);

// 构建自由链表 
static void* mempool_s_refill(mempool* self, size_t n);

// 重新分配内存 
void* mempool_reallocate(mempool* self, void* p, size_t old_sz, size_t new_sz);

// 释放内存 
void mempool_deallocate(mempool* self, void* p, size_t n);

// 分配内存 
void* mempool_allocate(mempool* self, size_t n);

#endif