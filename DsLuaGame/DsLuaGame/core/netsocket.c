#include "netsocket.h"

// ��ȫ�ر�socket
void safe_close_socket(void *arg) {
	sock_t sock = *(sock_t*)arg;
	if (sock <= 0) return;
#ifdef _WIN32
	closesocket(sock);
#else
	close(sock);
#endif
}

//����Ϊ������ģʽ
int set_non_blocking(sock_t sock, int bset)
{
#ifdef _WIN32
	u_long nonBlock = bset;
	//��ȡ���׽ӿ���صĲ�������
	if (ioctlsocket(sock, FIONBIO, &nonBlock) == SOCKET_ERROR)
	{
		return -1;
	}
#else
	//�������ETģʽ,��ô����״̬�����仯ʱ�Ż�֪ͨ,������LTģʽ������ԭ����select/poll����,ֻҪ����û�д�����¼��ͻ�һֱ֪ͨ. 
	if (bset)
	{	//���������ļ���������һЩ����  ʩ��ǿ���� ��¼��...
		if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0) | O_NONBLOCK) == -1)
		{
			return -2;
		}
	}
	else
	{	//���������ļ���������һЩ����  ʩ��ǿ���� ��¼��...
		if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0)&~O_NONBLOCK) == -1)
		{
			return -3;
		}
	}
#endif
	return 1;
}

//����Ϊ�˿�����
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

//������ʱ�ر�
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

//�������緢�ͻ�������С
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
//��send(),recv()��������ʱ��������״����ԭ�򣬷��ղ���Ԥ�ڽ���,�������շ�ʱ��.
//���÷���ʱ��
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
		// �ͻ������������������ӳɹ�����Ϊ������
		ret |= set_non_blocking(sock, 0);
	}
	set_net_send_buffsize(sock, 0);
	set_net_recv_buffsize(sock, 0);
	set_send_time(sock, 0);
	set_recv_time(sock, 0);
	return ret;
}