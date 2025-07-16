#include "session.h"
#include "logger/elog.h"
#include "mempool.h"
#include "netsocket.h"
#include <time.h>
#include "timer.h"
#include "utils/datetime.h"
#include "utils/rbtree.h"

extern CMap _handleMap;

CSession* create_session(sock_t sock, event_cb rcb, event_cb wcb, event_cb ecb)
{
	CSession* session = (CSession*)mempool_allocate(mempool_getinstance(), sizeof(CSession));
	if (!session)
	{
		log_e("memory allocation failed for session");
		return NULL;
	}
	memset(session, 0, sizeof(CSession));
	session->sock = sock;
	session->last_active = time(NULL);
	session->read_cb = rcb;
	session->write_cb = wcb;
	session->remove_cb = ecb;
	session->timer_ev = NULL;

	input_stream_inithandle(&session->_instream, session);
	return session;
}

void delete_session(void* node)
{
	CSession* session = (CSession*)node;
	if (!session) return;
	// 安全移除定时器
	safe_close_socket(&session->sock);
	if (session->timer_ev)
	{
		timer_cancel(session->timer_ev);
	}
	mempool_deallocate(mempool_getinstance(), session, sizeof(CSession));
}

void session_firstpack_callback(void* arg)
{
	CSession* session = (CSession*)arg;
	if (!session)
	{
		log_e("session_firstpack_timer_callback session is null");
		return;
	}

	TreeNode* node = map_find(&_handleMap, session->sock);
	if (node && session->remove_cb)
	{
		char buf[128] = { 0 };
		sprintf(buf, "session_firstpack_timer_callback, sock:%d", session->sock);
		log_e(buf);

		session->remove_cb(node);
	}
}

void session_keepalive_callback(void* arg)
{
	CSession* session = (CSession*)arg;
	if (!session)
	{
		log_e("session_firstpack_timer_callback session is null");
	}
	//调用心跳处理
}

void session_addtimer(CSession* ss, int type)
{
	CSession* session = (CSession*)ss;
	if (NULL == session)
	{
		return;
	}

	timertask* task = (timertask*)mempool_allocate(mempool_getinstance(), sizeof(timertask));
	if (!task) {
		log_e("Timer task allocation failed");
		return;
	}

	datetime dt = datetime_now();
	interval val;
	interval_create_milliseconds(&val, 10 * 1000);
	datetime_add_interval(&dt, &val);
	switch (type) 
	{
		case ESTaskTypeFirstPack:
		{
			task->arg = session;
			task->func = session_firstpack_callback;
			timer_schedule(task, dt, interval_zero);
			ss->timer_ev = task;
		}
		break;
		case ESTaskTypeKeepAlive:
		{
			task->arg = session;
			task->func = session_keepalive_callback;
			timer_schedule(task, dt, val);  // 重复定时器
			ss->timer_ev = task;
		}
		break;
		default:	
		{
			char buf[128] = { 0 };
			sprintf(buf, "session_addtimer,type is not define, sock:%d", session->sock);
			log_e(buf);
			mempool_deallocate(mempool_getinstance(), task, sizeof(timertask));
		}
	}
}

void session_removetimer(CSession* ss, int type)
{
	CSession* session = (CSession*)ss;
	if (NULL == session)
	{
		return;
	}
	if (ss->timer_ev)
	{
		timer_cancel(ss->timer_ev);
	}
}
