#include "netsocket.h"

// 安全关闭socket
void safe_close_socket(void *arg) {
	sock_t sock = *(sock_t*)arg;
	if (sock <= 0) return;
#ifdef _WIN32
	closesocket(sock);
#else
	close(sock);
#endif
}

//设置为非阻塞模式
int set_non_blocking(sock_t sock, int bset)
{
#ifdef _WIN32
	u_long nonBlock = bset;
	//获取与套接口相关的操作参数
	if (ioctlsocket(sock, FIONBIO, &nonBlock) == SOCKET_ERROR)
	{
		return -1;
	}
#else
	//如果采用ET模式,那么仅当状态发生变化时才会通知,而采用LT模式类似于原来的select/poll操作,只要还有没有处理的事件就会一直通知. 
	if (bset)
	{	//用来操作文件描述符的一些特性  施加强制锁 记录锁...
		if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0) | O_NONBLOCK) == -1)
		{
			return -2;
		}
	}
	else
	{	//用来操作文件描述符的一些特性  施加强制锁 记录锁...
		if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0)&~O_NONBLOCK) == -1)
		{
			return -3;
		}
	}
#endif
	return 1;
}

//设置为端口重用
int set_reuse(sock_t sock, int buse)
{
	int bReuseaddr = buse;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&bReuseaddr, sizeof(int)) != 0)
	{
		return -1;
	}
	return 1;
}

//设置延时关闭
int set_linger(sock_t sock, int buse, int time)
{
	struct linger lg;
	lg.l_onoff = buse;
	lg.l_linger = time;
	if (0 != setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&lg, sizeof(lg)))
	{
		return -1;
	}
	return 1;
}

//设置网络发送缓冲区大小
void set_net_send_buffsize(sock_t sock, int buffsize)
{
	int dwSize = 40 * 1024;
#ifdef _WIN32
	int dwLen = sizeof(int);
#else
	socklen_t dwLen = sizeof(dwSize);
#endif
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&dwSize, dwLen);
}

void set_net_recv_buffsize(sock_t sock, int buffsize)
{
	int dwSize = 60 * 1024;
#ifdef _WIN32
	int dwLen = sizeof(int);
#else
	socklen_t	dwLen = sizeof(dwSize);
#endif
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&dwSize, dwLen);
}
//在send(),recv()过程中有时由于网络状况等原因，发收不能预期进行,而设置收发时限.
//设置发送时限
void set_send_time(sock_t sock, int time)
{
	int nNetTimeout = 10000;
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&nNetTimeout, sizeof(int));
}

void set_recv_time(sock_t sock, int time)
{
	int nNetTimeout = 10000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&nNetTimeout, sizeof(int));
}

int set_socket_options(sock_t sock, int is_server)
{
	int ret = 0;
	ret |= set_reuse(sock, 1);
	ret |= set_linger(sock, 0, 0);

	if (is_server) {
		ret |= set_non_blocking(sock, 1);
	}
	else {
		// 客户端连接先阻塞，连接成功后设为非阻塞
		ret |= set_non_blocking(sock, 0);
	}
	set_net_send_buffsize(sock, 0);
	set_net_recv_buffsize(sock, 0);
	set_send_time(sock, 0);
	set_recv_time(sock, 0);
	return ret;
}