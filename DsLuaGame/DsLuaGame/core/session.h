#ifndef __SESSION_H__
#define __SESSION_H__
#include "public.h"
#include "inputstream.h"
#include "timer.h"

typedef int(*event_cb)(void*);   //读数据回调函数

enum ESTaskType
{
	ESTaskTypeFirstPack = 1,
	ESTaskTypeKeepAlive = 2,
};

// 客户端连接结构
typedef struct{
	sock_t sock;                // 套接字描述符
	time_t last_active;       // 最后活动时间
	event_cb read_cb;   //读数据回调函数
	event_cb write_cb;  //写数据回调函数
	event_cb remove_cb;  //写数据回调函数
	timertask* timer_ev;
	inputstream _instream;
} CSession;

//创建
CSession* create_session(sock_t sock, event_cb rcb, event_cb wcb, event_cb ecb);
//删除session
void delete_session(void* node);
//定时器回调
void session_firstpack_callback(void* arg);
void session_keepalive_callback(void* arg);
//添加定时器
void session_addtimer(CSession* ss, int type);
void session_removetimer(CSession* ss, int type);

#endif