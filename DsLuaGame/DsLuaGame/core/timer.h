#ifndef __TIMER_H__
#define __TIMER_H__
#include "public.h"
#include "utils/datetime.h"
#include "utils/lightlock.h"
#include "utils/lightevent.h"


// 定时任务函数指针
typedef void(*timer_callback)(void*);

// 定时任务结构体
typedef struct timertask {
	timer_callback func;
	void* arg; // 用户参数
} timertask;

// 定时器描述符
typedef struct timertasknode{
	datetime startTime;
	datetime nextTime;
	interval val;
	timertask* task;
	struct timertasknode* next;
} timertasknode;


// 定时器管理器
typedef struct timermanager {
	light_lock lock;
	light_event event;
	timertasknode* tasks;       // 主定时器链表
	bool running;
#ifdef _WIN32
	HANDLE thread_handle;
#else
	pthread_t thread_handle;
#endif
} timermanager;


// 创建定时任务
timertask* timer_task_create(timer_callback func, void* arg);
void timer_task_destroy(timertask* task);

void timer_init();
void timer_destroy();

void timer_schedule(timertask* task, datetime futureTime, interval intervalVal);
void timer_schedule_delay(timertask* task, interval delay, interval intervalVal);
bool timer_cancel(timertask* task);

#ifdef _WIN32
DWORD WINAPI timer_thread_run(LPVOID arg);
#else
void* timer_thread_run(void* arg);
#endif

#endif /*__TIMER_H__*/