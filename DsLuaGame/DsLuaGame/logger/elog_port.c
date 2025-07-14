/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */

#ifdef _WIN32
#include "elog.h"
#include <stdio.h>
#include <windows.h>
#else
#include "elog.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>
#endif

#ifdef ELOG_FILE_ENABLE
#include "elog_file.h"
#endif

#ifdef _WIN32
static HANDLE output_lock = NULL;
#else
static pthread_mutex_t output_lock;
#endif


/**
* EasyLogger port initialize
*
* @return result
*/
ElogErrCode elog_port_init(const char* path) {

	ElogErrCode result = ELOG_NO_ERR;
#ifdef _WIN32
	output_lock = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&output_lock, NULL);
#endif

#ifdef ELOG_FILE_ENABLE
	elog_file_init(path);
#endif


	return result;
}

/**
* EasyLogger port deinitialize
*
*/
void elog_port_deinit(void) {
#ifdef ELOG_FILE_ENABLE
	elog_file_deinit();
#endif

#ifdef _WIN32
	CloseHandle(output_lock);
#else
	pthread_mutex_destroy(&output_lock);
#endif
	
}

/**
* output log port interface
*
* @param log output of log
* @param size log size
*/
void elog_port_output(const char *log, size_t size) {
	/* output to terminal */
#ifdef _WIN32
	printf("%.*s", size, log);
#else
	printf("%.*s", (int)size, log);
#endif

#ifdef ELOG_FILE_ENABLE
	/* write the file */	
	elog_file_write(log, size);
#endif 
}

/**
* output lock
*/
void elog_port_output_lock(void) {
#ifdef _WIN32
	WaitForSingleObject(output_lock, INFINITE);
#else
	pthread_mutex_lock(&output_lock);
#endif
	
}

/**
* output unlock
*/
void elog_port_output_unlock(void) {
#ifdef _WIN32
	ReleaseMutex(output_lock);
#else
	pthread_mutex_unlock(&output_lock);
#endif
}


/**
* get current time interface
*
* @return current time
*/
const char *elog_port_get_time(void) {
	static char cur_system_time[24] = { 0 };
#ifdef _WIN32
	static SYSTEMTIME currTime;
	GetLocalTime(&currTime);
	snprintf(cur_system_time, 24, "%02d-%02d %02d:%02d:%02d.%03d", currTime.wMonth, currTime.wDay,
		currTime.wHour, currTime.wMinute, currTime.wSecond, currTime.wMilliseconds);
#else
	time_t cur_t;
	struct tm cur_tm;

	time(&cur_t);
	localtime_r(&cur_t, &cur_tm);

	strftime(cur_system_time, sizeof(cur_system_time), "%Y-%m-%d %T", &cur_tm);
#endif
	return cur_system_time;
}

void elog_port_time(int* year, int* mon, int* day) {
	static char cur_system_time[24] = { 0 };
#ifdef _WIN32
	static SYSTEMTIME currTime;
	GetLocalTime(&currTime);
	*year = (int)currTime.wYear;
	*mon = (int)currTime.wMonth;
	*day = (int)currTime.wDay;
#else
	time_t cur_t;
	struct tm cur_tm;

	time(&cur_t);
	localtime_r(&cur_t, &cur_tm);

	*year = cur_tm.tm_year  + 1900;  // 修正：tm_year是从1900开始的年数
    *mon = cur_tm.tm_mon  + 1;       // 修正：tm_mon范围0-11，需要加1
	*day = cur_tm.tm_mday;
#endif
}

bool elog_issame_day(int day)
{
#ifdef _WIN32
	static SYSTEMTIME currTime;
	GetLocalTime(&currTime);
	return day == currTime.wDay;
#else
	time_t cur_t;
	struct tm cur_tm;

	time(&cur_t);
	localtime_r(&cur_t, &cur_tm);
	return day == cur_tm.tm_mday;	
#endif
}

int elog_current_day()
{
#ifdef _WIN32
	static SYSTEMTIME currTime;
	GetLocalTime(&currTime);
	return currTime.wDay;
#else
	time_t cur_t;
	struct tm cur_tm;

	time(&cur_t);
	localtime_r(&cur_t, &cur_tm);
	return cur_tm.tm_mday;
#endif
}

/**
* get current process name interface
*
* @return current process name
*/
const char *elog_port_get_p_info(void) {
	static char cur_process_info[10] = { 0 };
#ifdef _WIN32
	snprintf(cur_process_info, 10, "pid:%04ld", GetCurrentProcessId());
#else
	snprintf(cur_process_info, 10, "pid:%04d", getpid());
#endif
	return cur_process_info;
}

/**
* get current thread name interface
*
* @return current thread name
*/
const char *elog_port_get_t_info(void) {
	static char cur_thread_info[10] = { 0 };
#ifdef _WIN32
	snprintf(cur_thread_info, 10, "tid:%04ld", GetCurrentThreadId());
#else
	snprintf(cur_thread_info, 10, "tid:%04d", syscall(SYS_gettid));
#endif

	return cur_thread_info;
}
