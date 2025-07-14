#include "timer.h"
#include "public.h"
#include <memory.h>
#include "logger/elog.h"

// 静态定时器管理器
static TimerManager* mgr = NULL;

// 获取当前时间（微秒）
uint64_t get_current_time() {
#ifdef _WIN32 
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
#else 
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#endif 
}

// 堆操作函数
void heap_swap(Timer** heap, int i, int j) {
	Timer* tmp = heap[i];
	heap[i] = heap[j];
	heap[j] = tmp;
}

void heapify_up(TimerManager* mgr, int index) {
	while (index > 0) {
		int parent = (index - 1) / 2;
		if (mgr->heap[index]->trigger_time < mgr->heap[parent]->trigger_time) {
			heap_swap(mgr->heap, index, parent);
			index = parent;
		}
		else {
			break;
		}
	}
}

void heapify_down(TimerManager* mgr, int index) {
	int smallest = index;
	int left = 2 * index + 1;
	int right = 2 * index + 2;

	if (left < (int)mgr->count &&
		mgr->heap[left]->trigger_time < mgr->heap[smallest]->trigger_time) {
		smallest = left;
	}

	if (right < mgr->count &&
		mgr->heap[right]->trigger_time < mgr->heap[smallest]->trigger_time) {
		smallest = right;
	}

	if (smallest != index) {
		heap_swap(mgr->heap, index, smallest);
		heapify_down(mgr, smallest);
	}
}

// 初始化定时器管理器 
void timer_init(uint32_t initial_capacity) {
	mgr = (TimerManager*)malloc(sizeof(TimerManager));
	mgr->heap = (Timer**)malloc(initial_capacity * sizeof(Timer*));
	if (mgr->heap == NULL) {
		log_e("Memory allocation failed for timers\n");
		return;
	}
	mgr->capacity = initial_capacity;
	mgr->count = 0;
	mgr->next_id = 1;

#ifdef _WIN32 
	mgr->mutex = CreateMutex(NULL, FALSE, NULL);
	mgr->event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (mgr->mutex == NULL || mgr->event == NULL) {
		log_e("Failed to create sync objects\n");
		free(mgr->heap);
		return;
	}

	mgr->thread_handle = CreateThread(NULL, 0, timer_thread_run, mgr, 0, NULL);

#else 
	if (pthread_mutex_init(&mgr->mutex, NULL) != 0 ||
		pthread_cond_init(&mgr->cond, NULL) != 0) {
		log_e("Failed to initialize sync objects\n");
		free(mgr->heap);
		return;
	}

	pthread_create(&mgr->thread_handle, NULL, timer_thread_run, mgr);
#endif 
}

// 动态扩容堆
void resize_heap(TimerManager* mgr) {
	int new_capacity = mgr->capacity * 2;
	Timer** new_heap = (Timer**)realloc(mgr->heap, new_capacity * sizeof(Timer*));
	if (new_heap == NULL) {
		log_i("Heap resize failed\n");
		return;
	}
	mgr->heap = new_heap;
	mgr->capacity = new_capacity;
}

// 添加定时器（返回定时器ID）
uint32_t timer_add(uint32_t interval, bool repeat, TimerCallback cb, void* data, UserDataFreeFunc free_func) {
	Timer* new_timer = (Timer*)malloc(sizeof(Timer));
	if (!new_timer) {
		log_i("Failed to allocate timer\n");
		return 0;
	}

	new_timer->id = mgr->next_id++;
	new_timer->interval = interval;
	new_timer->is_repeat = repeat;
	new_timer->callback = cb;
	new_timer->user_data = data;
	new_timer->is_active = true;
	new_timer->trigger_time = get_current_time() + interval * 1000; // 转换为微秒
	new_timer->free_func = free_func; // 设置释放函数

#ifdef _WIN32 
	WaitForSingleObject(mgr->mutex, INFINITE);
#else 
	pthread_mutex_lock(&mgr->mutex);
#endif 
	// 动态扩容
	if (mgr->count >= mgr->capacity) {
		resize_heap(mgr);
	}
	// 添加到堆尾并上浮
	mgr->heap[mgr->count] = new_timer;
	mgr->count++;
	heapify_up(mgr, mgr->count - 1);

	// 如果是堆顶元素，唤醒工作线程
	if (mgr->heap[0] == new_timer) {
#ifdef _WIN32
		SetEvent(mgr->event);
#else
		pthread_cond_signal(&mgr->cond);
#endif
	}

#ifdef _WIN32 
	ReleaseMutex(mgr->mutex);
#else 
	pthread_mutex_unlock(&mgr->mutex);
#endif 

	return new_timer->id;
}

// 删除定时器（惰性删除）
void timer_remove(uint32_t timer_id) {
#ifdef _WIN32 
	WaitForSingleObject(mgr->mutex, INFINITE);
#else 
	pthread_mutex_lock(&mgr->mutex);
#endif 
	for (int i = 0; i < mgr->count; i++) {
		if (mgr->heap[i]->id == timer_id) {
			mgr->heap[i]->is_active = false;
			break;
		}
	}
#ifdef _WIN32 
	ReleaseMutex(mgr->mutex);
#else 
	pthread_mutex_unlock(&mgr->mutex);
#endif 
}

// 销毁定时器系统
void timer_destroy() {
	// 设置停止标志
#ifdef _WIN32 
	WaitForSingleObject(mgr->mutex, INFINITE);
#else 
	pthread_mutex_lock(&mgr->mutex);
#endif 
	mgr->running = false;
	// 唤醒工作线程
#ifdef _WIN32
	SetEvent(mgr->event);
#else
	pthread_cond_signal(&mgr->cond);
#endif

#ifdef _WIN32 
	ReleaseMutex(mgr->mutex);
#else 
	pthread_mutex_unlock(&mgr->mutex);
#endif 

	// 等待线程退出
#ifdef _WIN32
	WaitForSingleObject(mgr->thread_handle, INFINITE);
	CloseHandle(mgr->thread_handle);
	CloseHandle(mgr->mutex);
	CloseHandle(mgr->event);
#else
	pthread_join(mgr->thread_handle, NULL);
	pthread_mutex_destroy(&mgr->mutex);
	pthread_cond_destroy(&mgr->cond);
#endif

	// 释放所有定时器
	for (int i = 0; i < mgr->count; i++) {
		free(mgr->heap[i]);
	}
	free(mgr->heap);
	free(mgr);
}

#ifdef _WIN32
DWORD WINAPI timer_thread_run(LPVOID arg)
#else
void* timer_thread_run(void* arg)
#endif
{
	TimerManager* mgr = (TimerManager*)arg;
	while (mgr->running) {
#ifdef _WIN32 
		WaitForSingleObject(mgr->mutex, INFINITE);
#else 
		pthread_mutex_lock(&mgr->mutex);
#endif 

		uint64_t now = get_current_time();
		uint64_t next_trigger = 0;

		// 清理堆顶的不活跃定时器
		while (mgr->count > 0 && !mgr->heap[0]->is_active) {
			Timer* to_remove = mgr->heap[0];
			mgr->heap[0] = mgr->heap[mgr->count - 1];
			mgr->count--;
			heapify_down(mgr, 0);
			free(to_remove);
		}

		// 检查堆顶定时器是否需要触发
		Timer* trigger_timer = NULL;
		if (mgr->count > 0 && mgr->heap[0]->trigger_time <= now) {
			trigger_timer = mgr->heap[0];
			// 从堆中移除
			mgr->heap[0] = mgr->heap[mgr->count - 1];
			mgr->count--;
			heapify_down(mgr, 0);
		}

		// 计算下一个触发时间
		if (mgr->count > 0) {
			next_trigger = mgr->heap[0]->trigger_time;
		}

#ifdef _WIN32 
		ReleaseMutex(mgr->mutex);
#else 
		pthread_mutex_unlock(&mgr->mutex);
#endif 

		// 执行回调（无锁状态）
		if (trigger_timer) {
			trigger_timer->callback(trigger_timer->user_data);

#ifdef _WIN32 
			WaitForSingleObject(mgr->mutex, INFINITE);
#else 
			pthread_mutex_lock(&mgr->mutex);
#endif 

			// 处理重复定时器
			if (trigger_timer->is_repeat && trigger_timer->is_active) {
				trigger_timer->trigger_time = now + trigger_timer->interval * 1000;

				// 重新插入堆
				if (mgr->count >= mgr->capacity) {
					resize_heap(mgr);
				}
				mgr->heap[mgr->count] = trigger_timer;
				mgr->count++;
				heapify_up(mgr, mgr->count - 1);
			}
			else {
				free(trigger_timer);
			}

#ifdef _WIN32 
			ReleaseMutex(mgr->mutex);
#else 
			pthread_mutex_unlock(&mgr->mutex);
#endif 
		}

		// 计算等待时间
		uint64_t wait_time = 0;
		if (next_trigger > 0) {
			wait_time = (next_trigger > now) ? (next_trigger - now) : 0;
		}
		else {
			// 无定时器时的默认等待
			wait_time = 10000; // 10ms
		}

		// 精确等待
#ifdef _WIN32 
		WaitForSingleObject(mgr->mutex, INFINITE);
		if (mgr->count > 0) {
			// 转换为毫秒，Windows事件等待最大为INFINITE
			DWORD dwWait = (DWORD)(wait_time / 1000);
			SignalObjectAndWait(mgr->mutex, mgr->event, dwWait, FALSE);
		}
		else {
			ReleaseMutex(mgr->mutex);
			Sleep(10);
		}
#else 
		pthread_mutex_lock(&mgr->mutex);
		if (mgr->count > 0) {
			struct timespec ts;
			ts.tv_sec = wait_time / 1000000;
			ts.tv_nsec = (wait_time % 1000000) * 1000;
			pthread_cond_timedwait(&mgr->cond, &mgr->mutex, &ts);
		}
		pthread_mutex_unlock(&mgr->mutex);
#endif
	}
	return 0;
}