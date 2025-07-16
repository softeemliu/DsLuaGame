#ifndef __NETWORK_H__
#define __NETWORK_H__
// 平台相关定义
#include "utils/lightlock.h"
#include "utils/rbtree.h"
#include "inputstream.h"
#include "protocol.h"
#include "bytestream.h"
#include "session.h"

//服务器网络信息结构体
typedef struct
{
	bool running;
	light_lock lock;
#ifdef _WIN32
	HANDLE thread_handle;  // 添加 Windows 线程句柄
#else
	pthread_t thread_handle;  // 添加 Linux 线程句柄
#endif
	volatile uint64_t _time;
	sock_t _sock;
#ifdef _WIN32
	struct fd_set _fdsR[2];
	struct fd_set _fdsW[2];
	struct fd_set _fdsE[2];
	int _maxFD;
#else
	sock_t _epoll_fd;
	int _max_poll_size;
#endif
}CNetServer;


//初始化网络
void init_network(int size);
//销毁
void fina_network();
// 记录socket错误
void write_socket_error(const char* context, sock_t sock);
int handle_connect_error(sock_t sock);

// 网络线程函数
#ifdef _WIN32
DWORD WINAPI network_thread_run(LPVOID arg);
#else
void* network_thread_run(void* arg);
#endif


//创建socket
sock_t create_socket(int port);
//连接
int connect_server(const char* host, int port);
//
int socket_data(sock_t sock, const char* buff, int len);
/*
len:origin len,原始长度
sourceLen:compress len,压缩长度
compress：是否压缩
encrypt：是否加密
encryptAddLength：加密增加的长度
*/
void make_protocol_head(SProtocolHead* head, int len,int sourceLen,bool compress,bool encrypt,byte_t encryptAddLength);

int socket_send_data(sock_t sock, bytestream* bystream);
void insert_handle(sock_t sock, CSession* cc);

//网络操作函数
int accept_socket(void* arg);
int read_socket(void* arg);
int write_socket(void* arg);
int remove_socket(void* arg);


#endif /*__NETWORK_H__*/