#ifndef __TIMER_H__
#define __TIMER_H__
#include "public.h"

//定时器回调函数
typedef void(*TimerCallback)(void*);
// 新增：用户数据释放函数类型
typedef void(*UserDataFreeFunc)(void*);

// 修改：扩展 Timer 结构
typedef struct Timer {
	uint32_t id;
	uint32_t interval;
	bool is_repeat;
	TimerCallback callback;
	void* user_data;
	UserDataFreeFunc free_func;  // 用户数据释放函数
	bool is_active;
	uint64_t trigger_time;
} Timer;

typedef struct TimerManager {
    // 定时器堆（最小堆实现）
    Timer** heap;  
    // 堆的容量和当前元素数量
    uint32_t capacity;
    uint32_t count;  
    // 定时器ID生成器
    uint32_t next_id;   
    // 线程控制
#ifdef _WIN32
    HANDLE thread_handle;
#else
    pthread_t thread_handle;
#endif
    
    // 同步原语
#ifdef _WIN32
    HANDLE mutex;   // 互斥锁
    HANDLE event;   // 事件对象
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
    bool running;
} TimerManager;


// 获取当前时间（微秒）
uint64_t get_current_time();
// 堆操作函数
void heap_swap(Timer** heap, int i, int j);
void heapify_up(TimerManager* mgr, int index);
void heapify_down(TimerManager* mgr, int index);

// 初始化定时器管理器 
void timer_init(uint32_t initial_capacity);
// 动态扩容堆
void resize_heap(TimerManager* mgr);
// 添加定时器（返回定时器ID）
uint32_t timer_add(uint32_t interval, bool repeat, TimerCallback cb, void* data, UserDataFreeFunc free_func);
// 删除定时器（惰性删除）
void timer_remove(uint32_t timer_id);
// 销毁定时器系统
void timer_destroy();

#ifdef _WIN32
DWORD WINAPI timer_thread_run(LPVOID arg);
#else
void* timer_thread_run(void* arg);
#endif

#endif /*__TIMER_H__*/