#ifndef __SESSION_H__
#define __SESSION_H__
#include "public.h"
#include "inputstream.h"
#include "timer.h"

typedef int(*event_cb)(void*);   //�����ݻص�����

enum ESTaskType
{
	ESTaskTypeFirstPack = 1,
	ESTaskTypeKeepAlive = 2,
};

// �ͻ������ӽṹ
typedef struct{
	sock_t sock;                // �׽���������
	time_t last_active;       // ���ʱ��
	event_cb read_cb;   //�����ݻص�����
	event_cb write_cb;  //д���ݻص�����
	event_cb remove_cb;  //д���ݻص�����
	timertask* timer_ev;
	inputstream _instream;
} CSession;

//����
CSession* create_session(sock_t sock, event_cb rcb, event_cb wcb, event_cb ecb);
//ɾ��session
void delete_session(void* node);
//��ʱ���ص�
void session_firstpack_callback(void* arg);
void session_keepalive_callback(void* arg);
//��Ӷ�ʱ��
void session_addtimer(CSession* ss, int type);
void session_removetimer(CSession* ss, int type);

#endif