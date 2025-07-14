#ifndef __OBJECT_POOL_H__
#define __OBJECT_POOL_H__
#include "public.h"
#include "utils/lightlock.h"
#include "core/bytestream.h"
#include <stdarg.h>

// 基础对象池结构
typedef struct {
	void** pool;        // 对象指针数组
	size_t capacity;    // 池容量
	size_t index;       // 当前可用对象索引
	bool final;         // 是否已销毁
	light_lock lock; // 互斥锁
} objectpool;


// 初始化对象池
objectpool* objectpool_create(size_t initial_capacity);
// 销毁对象池
void objectpool_destroy(objectpool* pool, void(*destructor)(void*));
// 从池中获取对象
void* objectpool_acquire(objectpool* pool, void* (*constructor)(va_list args), ...);
// 释放对象回池中
void objectpool_release(objectpool* pool, void* obj, void(*reset)(void*), void(*destructor)(void*));

//创建对象池
objectpool* init_bytestream_pool(size_t initial_capacity);
bytestream* obtain_bytestream();
void giveup_bytestream(bytestream* btstream);
void fina_bytestream_pool();


#endif /*__OBJECT_POOL_H__*/