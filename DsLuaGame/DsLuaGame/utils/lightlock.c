#include "lightlock.h"

// 轻量级锁初始化函数 
void lightlock_init(light_lock* lock) {
#ifdef _WIN32 
#if defined(_WIN32_WINNT) && (_WIN32_WINNT > 0x0403) 
	InitializeCriticalSectionAndSpinCount(&lock->_lock, 10);
#else 
	InitializeCriticalSection(&lock->_lock);
#endif 
#else
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	// 这里选择递归锁类型 
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&lock->_lock, &attr);
	pthread_mutexattr_destroy(&attr);
#endif 
}

// 轻量级锁销毁函数 
void lightlock_destroy(light_lock* lock) {
#ifdef _WIN32 
	DeleteCriticalSection(&lock->_lock);
#else
	pthread_mutex_destroy(&lock->_lock);
#endif 
}

// 轻量级锁加锁函数 
void lightlock_lock(light_lock* lock) {
#ifdef _WIN32 
	EnterCriticalSection(&lock->_lock);
#else
	if (0 != pthread_mutex_lock(&lock->_lock)) {
		// 这里可以添加错误处理代码 
	}
#endif 
}

// 轻量级锁解锁函数 
void lightlock_unlock(light_lock* lock) {
#ifdef _WIN32 
	LeaveCriticalSection(&lock->_lock);
#else 
	pthread_mutex_unlock(&lock->_lock);
#endif 
}