#ifndef __DATE_TIME_H__
#define __DATE_TIME_H__
#include "public.h"


// 定义常量 
#define DEFAULT_DATETIME_FORMAT "YYYY-MM-DD hh:mm:ss.nnn"  
#define NORMAL_DATETIME_FORMAT "YYYY-MM-DD hh:mm:ss" 
#define DEFAULT_INTERVAL_FORMAT "DD hh:mm:ss.nnn"  
#define RESET_HOUR_DEFAULT 5 

// 定义结构体 
typedef struct tm ds_tm;

// 异常信息结构体 
typedef struct {
	char* msg;
}dt_exception;

// datatime 结构体 
typedef struct {
	short _timeZone;
	short _showZone;
	int _totalDaySpan;
	long long _timeSpan;			// 时间戳，单位毫秒
	ds_tm* _tm;
}datetime;

// interval结构体 
typedef struct {
	long long _timeSpan;             // 时间间隔，单位毫秒
}interval;


// 全局静态变量 
datetime datetime_zero;
datetime datetime_forever;
interval interval_zero;
interval interval_oneMinute;
interval interval_oneHour;
interval interval_oneDay;
datetime datetime_lastDt;


#ifdef _WIN32
DWORD WINAPI datetime_thread_run(LPVOID arg);
#else
void* datetime_thread_run(void* arg);
#endif

void init_datetime();
bool datetime_init(datetime* dt, int year, int month, int day, int hour, int minute,
	int second, int millseconds, short timezone);
bool datetime_init_milliseconds(datetime* dt, long long millseconds, short timezone);
const datetime datetime_now();
void datetime_update(datetime* dt);
const ds_tm* datetime_getLocalTime(datetime* dt);
void datetime_parse(datetime* dt, const char* val, const char* format);
char* datetime_asstring(const datetime* dt, const char* format);
long datetime_getTimeZoneMills(datetime* dt);

int datetime_getYear(datetime* dt);
int datetime_getMonth(datetime* dt);
int datetime_getDay(datetime* dt);
int datetime_getHour(datetime* dt);
int datetime_getMinute(datetime* dt);
int datetime_getSecond(datetime* dt);
int datetime_getDayOfWeek(datetime* dt);
long long datetime_getMillSecond(datetime* dt);
void datetime_clearMillSecond(datetime* dt);
int datetime_getTotalDaySpan(datetime* dt);
int datetime_getTotalDay(datetime* dt);
long long datetime_getTotalMill(datetime* dt);
long datetime_getTotalSecond(datetime* dt);

interval datetime_sub(datetime* dt, datetime* other);
void datetime_sub_interval(datetime* dt, interval* val);
void datetime_add_interval(datetime* dt, interval* val);



/*
int datetime_equals(const datetime*dt, const datetime* other);
int datetime_less_than(const datetime*dt, const datetime* other);
int datetime_greater_than(const datetime*dt, const datetime* other);
*/

void datetime_set_timezone(datetime* dt, short timezone);
int get_local_timezone(datetime* dt);

bool datetime_less(datetime a, datetime b);



void interval_create(interval* val,int days, int hours, int minutes, int seconds, int millseconds);
void interval_create_milliseconds(interval* val, long long millseconds);
int interval_getdays(const interval* val);
int interval_gettotalseconds(const interval* val);


#endif  /*__DATE_TIME_H__*/