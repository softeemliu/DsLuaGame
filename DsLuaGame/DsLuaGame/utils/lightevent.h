#ifndef __LIGHT_EVENT_H__
#define __LIGHT_EVENT_H__
#include "public.h"

#ifdef _WIN32
#define LG_EVENT_NULL  NULL
#define LG_INFINITE    INFINITE
typedef HANDLE EVENT_T;
#else
#include <semaphore.h>

#define LG_EVENT_NULL  NULL
#define LG_INFINITE    -1
typedef sem_t* EVENT_T;
#endif

typedef struct _light_event {
	EVENT_T _event;
}light_event;


light_event* lightevent_create(const char* name);
void lightevent_destroy(light_event* obj);
int lightevent_wait(light_event* obj, int mill);
int lightevent_wait_infinite(light_event* obj);
int lightevent_notify(light_event* obj);


#endif /*__LIGHT_EVENT_H__*/