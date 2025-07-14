#ifndef _ENGINE_LIGHT_LOCK_H_ 
#define _ENGINE_LIGHT_LOCK_H_ 

#include <stdio.h> 
#include <stdlib.h> 

#ifdef _WIN32
#include <windows.h> 
#else
#include <pthread.h> 
#endif 

// 轻量级锁结构体 
typedef struct _light_lock {
#ifdef _WIN32 
	CRITICAL_SECTION _lock;
#else
	pthread_mutex_t _lock;
#endif 
} light_lock;

// 轻量级锁初始化函数 
void lightlock_init(light_lock* lock);
// 轻量级锁销毁函数 
void lightlock_destroy(light_lock* lock);

// 轻量级锁加锁函数 
void lightlock_lock(light_lock* lock);
// 轻量级锁解锁函数 
void lightlock_unlock(light_lock* lock);


#define AUTO_LOCK_INIT(lk)		lightlock_init(lk)
#define AUTO_LOCK(lk)			lightlock_lock(lk)
#define AUTO_UNLOCK(lk)			lightlock_unlock(lk)
#define AUTO_LOCK_DESTORY(lk)	lightlock_destroy(lk)

#endif 