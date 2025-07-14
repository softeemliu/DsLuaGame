#ifndef __NET_SOCKET_H__
#define __NET_SOCKET_H__
#include "public.h"

// 安全关闭socket
void safe_close_socket(void *arg);
//设置为非阻塞模式
int set_non_blocking(sock_t sock, int bset);
//设置为端口重用
int set_reuse(sock_t sock, int buse);
//设置延时关闭
int set_linger(sock_t sock, int buse, int time);
//设置网络发送缓冲区大小
void set_net_send_buffsize(sock_t sock, int buffsize);
void set_net_recv_buffsize(sock_t sock, int buffsize);
//在send(),recv()过程中有时由于网络状况等原因，发收不能预期进行,而设置收发时限.
//设置发送时限
void set_send_time(sock_t sock, int time);
void set_recv_time(sock_t sock, int time);

int set_socket_options(sock_t sock, int is_server);

#endif /*__NET_SOCKET_H__*/