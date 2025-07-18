#include "timer.h"
#include "logger/elog.h"
#include "mempool.h"
#include "utils/datetime.h"

// 静态定时器管理器
// 全局实例
timermanager* timermgr = NULL;

// 创建定时任务
timertask* timer_task_create(timer_callback func, void* arg) {
	timertask* task = (timertask*)mempool_allocate(mempool_getinstance(), sizeof(timertask));
	task->func = func;
	task->arg = arg;
	return task;
}

// 销毁定时任务
void timer_task_destroy(timertask* task) {
	mempool_deallocate(mempool_getinstance(), task, sizeof(timertask));
}

void timer_init()
{
	if (timermgr == NULL) 
	{
		timermgr = (timermanager*)mempool_allocate(mempool_getinstance(), sizeof(timermanager));
		AUTO_LOCK_INIT(&timermgr->lock);
		timermgr->event = *lightevent_create("tter");
		if (&timermgr->event == NULL) {
			log_e("Failed to create sync objects\n");
			return;
		}
		timermgr->running = true;
		timermgr->tasks = NULL;
#ifdef _WIN32
		timermgr->thread_handle = CreateThread(NULL, 0, timer_thread_run, timermgr, 0, NULL);
#else 
		pthread_create(&timermgr->thread_handle, NULL, timer_thread_run, timermgr);
#endif 
	}
}

void timer_destroy()
{
	if (timermgr != NULL)
	{
		AUTO_LOCK(&timermgr->lock);
		timermgr->running = false;
		lightevent_notify(&timermgr->event);
		AUTO_UNLOCK(&timermgr->lock);

#ifdef _WIN32
		WaitForSingleObject(timermgr->thread_handle, INFINITE);
		CloseHandle(timermgr->thread_handle);
#else
		pthread_join(timermgr->thread_handle, NULL);
#endif

		// 清理任务链表
		timertasknode* node = timermgr->tasks;
		while (node != NULL) {
			timertasknode* next = node->next;
			timer_task_destroy(node->task);
			mempool_deallocate(mempool_getinstance(), node, sizeof(timertasknode));
			node = next;
		}
		AUTO_LOCK_DESTORY(&timermgr->lock);
		lightevent_destroy(&timermgr->event);
		mempool_deallocate(mempool_getinstance(), timermgr, sizeof(timermanager));
		timermgr = NULL;
	}
}

// 添加定时任务（指定具体时间）
void timer_schedule(timertask* task, datetime futureTime, interval intervalVal) {
	AUTO_LOCK(&timermgr->lock);
	// 创建新节点
	timertasknode* newNode = (timertasknode*)mempool_allocate(mempool_getinstance(), sizeof(timertasknode));
	newNode->task = task;
	newNode->nextTime = futureTime;
	newNode->val = intervalVal;

	// 插入到有序链表中（按nextTime升序）
	timertasknode** pp = &timermgr->tasks;
	while (*pp != NULL && datetime_less((*pp)->nextTime, futureTime)) {
		pp = &((*pp)->next);
	}
	newNode->next = *pp;
	*pp = newNode;

	lightevent_notify(&timermgr->event);
	AUTO_UNLOCK(&timermgr->lock);
}

// 添加定时任务（指定延迟时间）
void timer_schedule_delay(timertask* task, interval delay, interval intervalVal) {
	datetime dt = datetime_now();
	datetime_add_interval(&dt, &delay);
	timer_schedule(task, dt, intervalVal);
}

// 取消定时任务
bool timer_cancel(timertask* task) {
	if (timermgr == NULL) return false;

	AUTO_LOCK(&timermgr->lock);

	timertasknode** pp = &timermgr->tasks;
	while (*pp != NULL) {
		if ((*pp)->task == task) {
			timertasknode* toRemove = *pp;
			*pp = toRemove->next;
			timer_task_destroy(toRemove->task);
			mempool_deallocate(mempool_getinstance(), toRemove, sizeof(timertasknode));
			AUTO_UNLOCK(&timermgr->lock);
			return true;
		}
		pp = &((*pp)->next);
	}
	AUTO_UNLOCK(&timermgr->lock);
	return false;
}


#ifdef _WIN32
DWORD WINAPI timer_thread_run(LPVOID arg)
#else
void* timer_thread_run(void* arg)
#endif
{
	while (1) {
		AUTO_LOCK(&timermgr->lock);

		// 检查是否退出
		if (!timermgr->running) {
			AUTO_UNLOCK(&timermgr->lock);
			break;
		}

		datetime now = datetime_now();
		long long waitMillis = 1000; // 默认等待1秒

		// 如果链表不为空，计算等待时间
		if (timermgr->tasks != NULL) {
			datetime nextTime = timermgr->tasks->nextTime;
			if (datetime_less(nextTime, now)) {
				waitMillis = 0;
			}
			else {
				interval diff = datetime_sub(&nextTime, &now);
				waitMillis = diff._timeSpan;
				// 最大等待时间限制为1秒
				if (waitMillis > 1000) 
					waitMillis = 1000;
				if (waitMillis < 0) 
					waitMillis = 0;
			}
		}
		AUTO_UNLOCK(&timermgr->lock);

		// 等待条件变量
		if (waitMillis > 0) {
			lightevent_wait(&timermgr->event, waitMillis);
		}

		AUTO_LOCK(&timermgr->lock);

		// 处理所有到期的任务
		while (timermgr->tasks != NULL && datetime_less(timermgr->tasks->nextTime , now)) {
			timertasknode* node = timermgr->tasks;
			timermgr->tasks = node->next;

			// 执行任务
			if (node->task->func != NULL) 
			{
				node->task->func(node->task->arg);
			}

			// 如果是周期性任务，则重新安排
			if (node->val._timeSpan > 0) {
				// 更新下一次执行时间
				datetime_add_interval(&node->nextTime, &node->val);
				// 如果执行时间已经过去，则跳过
				if (datetime_less(node->nextTime, now)) 
				{
					datetime dt = datetime_now();
					datetime_add_interval(&dt, &node->val);
					long long mils = datetime_getMillSecond(&dt);
					datetime_init_milliseconds(&node->nextTime, mils, get_local_timezone(&now));
				}

				// 重新插入链表
				timertasknode** pp = &timermgr->tasks;
				while (*pp != NULL && datetime_less((*pp)->nextTime, node->nextTime)) {
					pp = &((*pp)->next);
				}
				node->next = *pp;
				*pp = node;
			}
			else {
				// 一次性任务，释放资源
				timer_task_destroy(node->task);
				mempool_deallocate(mempool_getinstance(), node, sizeof(timertasknode));
			}
		}

		AUTO_UNLOCK(&timermgr->lock);
	}
	return 0;
}