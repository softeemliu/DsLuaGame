#include "datetime.h"


// 全局常量 
datetime datetime_zero = { 0, 0, -1, 0, NULL };
datetime datetime_lastDt = { 0, 0, 0, -1, NULL };


#ifdef _WIN32
DWORD WINAPI datetime_thread_run(LPVOID arg)
#else
void* datetime_thread_run(void* arg)
#endif
{
	while (1)
	{
		datetime_update(&datetime_lastDt);
	}
}


bool parseYMD(struct tm* time, unsigned long* millis, const char ch, const char* value, int index, int length, int count) {
	char buf[5];
	int nvalue = 0;
	if (index + count > length)
		return false;
	switch (ch) {
	case 'Y':
		if (count == 2 || count == 4) {
			memcpy(buf, value + index, count);
			buf[count] = 0;
			nvalue = atoi(buf);
		}
		else
			return false;

		if (count == 2)
			nvalue += 2000;
		if (nvalue >= 1970)
			nvalue -= 1900;
		else
			return false;
		time->tm_year = nvalue;
		break;
	case 'M':
		if (count > 2)
			return false;
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue > 12 || nvalue <= 0)
			return false;
		time->tm_mon = nvalue - 1;
		break;
	case 'D':
		if (count > 2)
			return false;
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue > 31 || nvalue <= 0)
			return false;
		time->tm_mday = nvalue;
		break;
	case 'h':
		if (count > 2)
			return false;
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue > 23 || nvalue < 0)
			return false;
		time->tm_hour = nvalue;
		break;
	case 'm':
		if (count > 2)
			return false;
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue > 59 || nvalue < 0)
			return false;
		time->tm_min = nvalue;
		break;
	case 's':
		if (count > 2)
			return false;
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue > 59 || nvalue < 0)
			return false;
		time->tm_sec = nvalue;
		break;
	case 'n':
		if (count > 3)
			return false;
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue < 0)
			return false;
		*millis = nvalue;
		break;
	default:
		return false;
	}
	return true;
}

bool parseIntervalYMD(
	long64_t* time, const char ch, const char* value,
	int index, int length, int count
	)
{
	char buf[5];
	int nvalue = 0;
	if (index + count > length)
	{
		return false;
	}
	switch (ch)
	{
	case 'D':
	{
		if (count > 2)
			return false;
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue < 0)
			return false;
		*time += nvalue * 24 * 3600 * 1000;
	}
	break;
	case 'h':
	{
		if (count > 2)
		{
			return false;
		}
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue >= 24 || nvalue < 0)
			return false;
		*time += nvalue * 3600 * 1000;
	}
	break;
	case 'm':
	{
		if (count > 2)
		{
			return false;
		}
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue >= 60 || nvalue < 0)
			return false;
		*time += nvalue * 60 * 1000;
	}
	break;
	case 's':
	{
		if (count > 2)
		{
			return false;
		}
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue >= 60 || nvalue < 0)
		{
			return false;
		}
		*time += nvalue * 1000;
	}
	break;
	case 'n':
	{
		if (count > 3)
		{
			return false;
		}
		memcpy(buf, value + index, count);
		buf[count] = 0;
		nvalue = atoi(buf);
		if (nvalue < 0)
		{
			return false;
		}
		*time += nvalue;
	}
	break;
	default:
		return false;
	}
	return true;
}

bool asYMD(struct tm* time, unsigned long mills, const char ch,
	char* buf, int index, int count, int bufSize)
{
	if (index + count >= bufSize)
		return false;

	char change;
	char format[5] = {0};
	sprintf(format, "%%0%dd", count);
	change = buf[index + count];

	switch (ch) {
	case 'Y':
		if (count == 2) {
			sprintf(buf + index, format, time->tm_year % 100);
		}
		else if (count == 4) {
			sprintf(buf + index, format, time->tm_year + 1900);
		}
		else {
			return false;
		}
		break;
	case 'M':
		if (count > 2) return false;
		sprintf(buf + index, format, time->tm_mon + 1);
		break;
	case 'D':
		if (count > 2) return false;
		sprintf(buf + index, format, time->tm_mday);
		break;
	case 'h':
		if (count > 2) return false;
		sprintf(buf + index, format, time->tm_hour);
		break;
	case 'm':
		if (count > 2) return false;
		sprintf(buf + index, format, time->tm_min);
		break;
	case 's':
		if (count > 2) return false;
		sprintf(buf + index, format, time->tm_sec);
		break;
	case 'n':
		if (count > 3) return false;
		sprintf(buf + index, format, mills);
		break;
	default:
		return false;
	}
	buf[index + count] = change;
	return true;
}

bool datetime_init(datetime* dt, int year, int month, int day, int hour, int minute,
	int second, int millseconds, short timezone)
{
	if (dt->_tm) {
		free(dt->_tm);
		dt->_tm = NULL;
	}

	struct tm ltm;
	ltm.tm_year = year - 1900;
	ltm.tm_mon = month - 1;
	ltm.tm_mday = day;
	ltm.tm_hour = hour;
	ltm.tm_min = minute;
	ltm.tm_sec = second;
	time_t tmV = mktime(&ltm);

	if (millseconds < 0 || millseconds > 999 || tmV == -1) {
		dt->_timeZone = 0;
		dt->_showZone = 0;
		dt->_timeSpan = 0;
		dt->_totalDaySpan = -1;
		return false;
	}
	dt->_timeSpan = tmV;
	dt->_timeSpan = dt->_timeSpan * 1000 + millseconds;
	dt->_timeZone = dt->_showZone = timezone;
	dt->_totalDaySpan = -1;
	return true;
}

bool datetime_init_milliseconds(datetime* dt, long long millseconds, short timezone)
{
	if (dt->_tm) {
		free(dt->_tm);
		dt->_tm = NULL;
	}

	dt->_timeSpan = millseconds;
	dt->_timeZone = dt->_showZone = timezone;
	dt->_totalDaySpan = -1;
	return true;
}

void datetime_update(datetime* dt)
{
#ifdef _WIN32
	SYSTEMTIME time;
	GetLocalTime(&time);
	datetime_init(dt, time.wYear, time.wMonth, time.wDay
		, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, 0);
#else
	struct timespec tm1;
	clock_gettime(CLOCK_REALTIME, &tm1);
	long long millis = (long long)(1000) * (tm1.tv_sec) + (tm1.tv_nsec) / (1000 * 1000);
	//初始化时间
	datetime_init(dt, 0, 0, 0, 0, 0, 0, (int)(millis % 1000), 0);
	dt->_timeSpan = millis;
#endif
}

const ds_tm* datetime_getLocalTime(datetime* dt)
{
	if (dt->_tm) {
		return dt->_tm;
	}
	dt->_tm = (struct tm*)malloc(sizeof(struct tm));
	time_t tmV = dt->_timeSpan / 1000;
#ifndef _WIN32 
	localtime_r(&tmV, dt->_tm);
#else 
	*dt->_tm = *localtime(&tmV);
#endif 
	return dt->_tm;
}

void datetime_parse(datetime* dt, const char* val, const char* format)
{
	int len = (int)strlen(format);
	int vallen = (int)strlen(val);
	int index = 0;
	struct tm ltm;
	unsigned long millis = 0;
	for (index = 0; index < len;) {
		char ch = format[index];
		switch (ch) {
		case 'Y':
		case 'M':
		case 'D':
		case 'h':
		case 'm':
		case 's':
		case 'n': {
			int count = 1;
			while (format[++index] == ch)
				count++;

			if (!parseYMD(&ltm, &millis, ch, val, index - count, vallen, count)) {
				// 这里简单打印错误信息，可根据需要修改 
				printf("datetime Format Error! %s\n", val);
				return;
			}
		} break;
		default:
			index++;
		}
	}
	time_t timeV = mktime(&ltm);
	if (timeV == -1) {
		// 这里简单打印错误信息，可根据需要修改 
		printf("CDateTime Value Error! %s\n", val);
		return;
	}
	long64_t mill = timeV;
	mill = mill * 1000 + millis;
	datetime_init(dt, 0, 0, 0, 0, 0, 0, (int)(mill % 1000), 0);
	dt->_timeSpan = mill;
}

char* datetime_asstring(const datetime* dt, const char* format)
{
	static char buf[100];
	strcpy(buf, format);
	int len = (int)strlen(format);
	int index = 0;
	time_t showtime;
	if (dt->_timeZone != dt->_showZone) {
		showtime = dt->_timeSpan / 1000 + (dt->_showZone - dt->_timeZone) * 3600;
	}
	else {
		showtime = dt->_timeSpan / 1000;
	}

#ifdef _WIN32
	struct tm* timePtr = localtime(&showtime);
	if (!timePtr) {
		// 这里简单打印错误信息，可根据需要修改 
		printf("INVALID DATEIME, seconds: %ld\n", showtime); 
		return ""; 
	} 
	struct tm time = *timePtr;
#else 
	struct tm time;
	localtime_r(&showtime, &time);
#endif 
	unsigned long mills = (long)(dt->_timeSpan % 1000);
	for (index = 0; index < len;) {
		char ch = format[index];
		switch (ch) {
		case 'Y':
		case 'M':
		case 'D':
		case 'h':
		case 'm':
		case 's':
		case 'n': {
			int count = 1;
			while (format[++index] == ch)
				count++;
			// 这里需要实现 asYMD 函数 
			if (!asYMD(&time, mills, ch, buf, index - count, count, sizeof(buf))) { 
			    printf("CDateTime Format Error!\n"); 
			    return ""; 
			} 
		} break;
		default:
			index++;
		}
	}
	return buf;
}

long datetime_getTimeZoneMills(datetime* dt)
{
	return dt->_timeZone * 1000 * 3600;
}

int datetimee_getYear(datetime* dt)
{
	return datetime_getLocalTime(dt)->tm_year + 1900;
}

int datetimee_getMonth(datetime* dt)
{
	return datetime_getLocalTime(dt)->tm_mon + 1;
}

int datetimee_getDay(datetime* dt)
{
	return datetime_getLocalTime(dt)->tm_mday;
}

int datetimee_getHour(datetime* dt)
{
	return datetime_getLocalTime(dt)->tm_hour;
}

int datetimee_getMinute(datetime* dt)
{
	return datetime_getLocalTime(dt)->tm_min;
}

int datetimee_getSecond(datetime* dt)
{
	return datetime_getLocalTime(dt)->tm_sec;
}

int datetimee_getDayOfWeek(datetime* dt)
{
	return datetime_getLocalTime(dt)->tm_wday;
}

int datetimee_getMillSecond(datetime* dt)
{
	return  (int)(dt->_timeSpan % 1000);
}

void datetime_clearMillSecond(datetime* dt)
{
	dt->_timeSpan -= datetimee_getMillSecond(dt);
}

int datetime_getTotalDaySpan(datetime* dt)
{
	if ( -1 == dt->_totalDaySpan)
	{
		datetime_getLocalTime(dt);
#ifdef _WIN32
		dt->_totalDaySpan = (int)_mkgmtime(dt->_tm);
#else
		dt->_totalDaySpan = (int)timegm(dt->_tm);
#endif
	}
	return dt->_totalDaySpan;
}

int datetime_getTotalDay(datetime* dt)
{
	return datetime_getTotalDaySpan(dt) / (24 * 3600);
}

long long datetime_getTotalMill(datetime* dt)
{
	return dt->_timeSpan;
}

long datetime_getTotalSecond(datetime* dt)
{
	return (long)(dt->_timeSpan / 1000);
}

interval datetime_sub(datetime* dt, datetime* other)
{
	long64_t offval;
	if (dt->_timeZone == other->_timeZone)
	{
		offval = dt->_timeSpan - other->_timeSpan;
	}
	else
	{
		offval = dt->_timeSpan + datetime_getTimeZoneMills(dt)
			- other->_timeSpan - datetime_getTimeZoneMills(other);
	}
	interval val;
	interval_create_milliseconds(&val, offval);
	return val;
}

void datetime_sub_interval(datetime* dt, interval* val)
{
	dt->_timeSpan -= val->_timeSpan;
	dt->_totalDaySpan = -1;
	if (dt->_tm) {
		free(dt->_tm);
		dt->_tm = NULL;
	}
}

void datetime_add_interval(datetime* dt, interval* val)
{
	dt->_timeSpan += val->_timeSpan;
	dt->_totalDaySpan = -1;
	if (dt->_tm) {
		free(dt->_tm);
		dt->_tm = NULL;
	}
}

void datetime_set_timezone(datetime* dt, short timezone)
{
	dt->_timeZone = timezone;
}

int get_local_timezone(datetime* dt)
{
	return dt->_timeZone;
}


// 初始化 CInterval 
void interval_create(interval* val, int days, int hours, int minutes, int seconds, int millSeconds) {
	val->_timeSpan = millSeconds;
	val->_timeSpan += (long64_t)seconds * 1000;
	val->_timeSpan += (long64_t)minutes * 60 * 1000;
	val->_timeSpan += (long64_t)hours * 60 * 60 * 1000;
	val->_timeSpan += (long64_t)days * 24 * 60 * 60 * 1000;
}

void interval_create_milliseconds(interval* val, long long millseconds)
{
	val->_timeSpan = millseconds;
}