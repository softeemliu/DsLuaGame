#include "lightevent.h"
#include "core/mempool.h"


light_event* lightevent_create(const char* name)
{
	light_event* obj = (light_event*)mempool_allocate(mempool_getinstance(), sizeof(light_event));
	if (!obj) return NULL;

	obj->_event = LG_EVENT_NULL;

#ifdef _WIN32
	obj->_event = CreateEventA(NULL, FALSE, FALSE, name);
	if (!obj->_event) {
		mempool_deallocate(mempool_getinstance(), obj, sizeof(light_event));
		return NULL;
	}
#else
	obj->_event = (sem_t*)mempool_allocate(mempool_getinstance(),sizeof(sem_t));
	if (!obj->_event) {
		mempool_deallocate(mempool_getinstance(), obj, sizeof(light_event));
		return NULL;
	}
	if (0 != sem_init(obj->_event, 0, 0)) {
		mempool_deallocate(mempool_getinstance(), obj->_event, sizeof(sem_t));
		mempool_deallocate(mempool_getinstance(), obj, sizeof(light_event));
		return NULL;
	}
#endif
	return obj;
}

void lightevent_destroy(light_event* obj)
{
	if (!obj) return;

#ifdef WIN32
	if (obj->_event != LG_EVENT_NULL) {
		CloseHandle(obj->_event);
	}
#else
	if (obj->_event != CDF_EVENT_NULL) {
		sem_destroy(obj->_event);
		mempool_deallocate(mempool_getinstance(), obj->_event, sizeof(sem_t));
	}
#endif
	mempool_deallocate(mempool_getinstance(), obj, sizeof(light_event));
}

int lightevent_wait(light_event* obj, int mill)
{
	if (!obj || obj->_event == LG_EVENT_NULL) {
		return -1;
	}
#ifdef WIN32
	DWORD ret = WaitForSingleObject(obj->_event, (mill == LG_INFINITE) ? INFINITE : (DWORD)mill);
	if (ret == WAIT_TIMEOUT) return 1;
	if (ret == WAIT_OBJECT_0) return 0;
	return -1; 
#else
	if (mill == LG_INFINITE || mill < 0) {
        return (sem_wait(obj->_event) == 0 ? 0 : -1;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        return -1;
    }

    long long total_nsec = (long long)ts.tv_nsec + (long long)mill * 1000000LL;
    ts.tv_sec += total_nsec / 1000000000LL;
    ts.tv_nsec = total_nsec % 1000000000LL;

    int ret = sem_timedwait(obj->_event, &ts);
    if (ret == 0) return 0;
    if (errno == ETIMEDOUT) return 1;
    return -1;
#endif
}

int lightevent_wait_infinite(light_event* obj)
{
	return lightevent_wait(obj, LG_INFINITE);
}

int lightevent_notify(light_event* obj)
{
	if (!obj || obj->_event == LG_EVENT_NULL) {
		return -1;
	}

#ifdef WIN32
	return SetEvent(obj->_event) ? 0 : -1;
#else
	return sem_post(obj->_event);
#endif
}