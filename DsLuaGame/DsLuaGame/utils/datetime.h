#ifndef __DATE_TIME_H__
#define __DATE_TIME_H__
#include <time.h> 
#include <string.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else

#endif

#ifndef long64_t
#ifdef _WIN32
typedef unsigned __int64 ulong64_t;
typedef __int64 long64_t;
#else
typedef __int64_t long64_t;
typedef __uint64_t ulong64_t;
#endif
#endif

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
	long long _timeSpan;
	ds_tm* _tm;
}datetime;

// interval结构体 
typedef struct {
	long long _timeSpan;
}interval;


// 全局静态变量 
extern datetime datetime_zero;
extern datetime datetime_forever;
extern interval interval_zero;
extern interval interval_oneMinute;
extern interval interval_oneHour;
extern interval interval_oneDay;
extern datetime datetime_lastDt;


#ifdef _WIN32
DWORD WINAPI datetime_thread_run(LPVOID arg);
#else
void* datetime_thread_run(void* arg);
#endif


bool datetime_init(datetime* dt, int year, int month, int day, int hour, int minute,
	int second, int millseconds, short timezone);
bool datetime_init_milliseconds(datetime* dt, long long millseconds, short timezone);
void datetime_update(datetime* dt);
const ds_tm* datetime_getLocalTime(datetime* dt);
void datetime_parse(datetime* dt, const char* val, const char* format);
char* datetime_asstring(const datetime* dt, const char* format);
long datetime_getTimeZoneMills(datetime* dt);

int datetimee_getYear(datetime* dt);
int datetimee_getMonth(datetime* dt);
int datetimee_getDay(datetime* dt);
int datetimee_getHour(datetime* dt);
int datetimee_getMinute(datetime* dt);
int datetimee_getSecond(datetime* dt);
int datetimee_getDayOfWeek(datetime* dt);
int datetimee_getMillSecond(datetime* dt);
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



void interval_create(interval* val,int days, int hours, int minutes, int seconds, int millseconds);
void interval_create_milliseconds(interval* val, long long millseconds);
int interval_getdays(const interval* val);
int interval_gettotalseconds(const interval* val);


#endif  /*__DATE_TIME_H__*/