#include "network.h"
#include "netsocket.h"
#include "logger/elog.h"
#include "logger/elog.h"
#include "inputstream.h"
#include <time.h>
#include "utils/endian.h"
#include "objectpool/objectpool.h"
#include "tolua/bridge/luacallbridge.h"




#pragma warning(disable : 4013)  // 禁用 C4013 警告 

//网络错误码
#define	SOCKET_ERROR	(-1)    //网络错误标志
//全局变量，保存网络连接
CMap _handleMap;
static CNetServer* g_net_server = NULL;

// 获取当前时间（毫秒）
int64_t get_current_time_ms(void) {
#ifdef _WIN32
	// Windows 实现
	return (int64_t)GetTickCount64();
#else
	// Unix-like 系统实现 (Linux, macOS, BSD等)
	struct timespec ts;
#if defined(CLOCK_MONOTONIC_RAW)
	// 首选原始单调时钟（不受NTP调整影响）
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#elif defined(CLOCK_MONOTONIC)
	// 次选标准单调时钟
	clock_gettime(CLOCK_MONOTONIC, &ts);
#else
	// 最后备选：实时时钟（可能受系统时间调整影响）
	clock_gettime(CLOCK_REALTIME, &ts);
#endif
	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

void init_network(int size)
{
	g_net_server = (CNetServer*)malloc(sizeof(CNetServer));
	_handleMap.root = NULL;
	_handleMap.compare = compare_int;
	_handleMap.free_value = safe_close_socket;

	g_net_server->_time = time(NULL);
	g_net_server->running = true;
	AUTO_LOCK_INIT(&g_net_server->lock);
#ifdef _WIN32
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		log_e("WSAStartup failed");
		return;
	}

	FD_ZERO(&g_net_server->_fdsR[0]);
	FD_ZERO(&g_net_server->_fdsW[0]);
	FD_ZERO(&g_net_server->_fdsE[0]);
	FD_ZERO(&g_net_server->_fdsR[1]);
	FD_ZERO(&g_net_server->_fdsW[1]);
	FD_ZERO(&g_net_server->_fdsE[1]);
	g_net_server->_maxFD = 0;
#else
	g_net_server->_max_poll_size = size > 0 ? size : 1024;
	g_net_server->_epoll_fd = epoll_create(g_net_server->_max_poll_size);
	if (g_net_server->_epoll_fd == SOCKET_ERROR) {
		log_e("epoll_create failed");
		exit(EXIT_FAILURE);
	}
#endif

#ifdef _WIN32
	g_net_server->thread_handle = CreateThread(NULL, 0, network_thread_run, &g_net_server, 0, NULL);
#else
	pthread_create(&g_net_server->thread_handle, NULL, network_thread_run, &g_net_server);
#endif
}

void fina_network()
{
	// 1. 设置停止标志
	g_net_server->running = false;

	// 2. 等待网络线程退出
#ifdef _WIN32
	if (g_net_server->thread_handle) {
		WaitForSingleObject(g_net_server->thread_handle, 2000); // 等待2秒
		CloseHandle(g_net_server->thread_handle);
		g_net_server->thread_handle = NULL;
	}
#else
	pthread_join(g_net_server->thread_handle, NULL);
#endif

#ifdef _WIN32
	FD_ZERO(&g_net_server->_fdsR[0]);
	FD_ZERO(&g_net_server->_fdsW[0]);
	FD_ZERO(&g_net_server->_fdsE[0]);
	FD_ZERO(&g_net_server->_fdsR[1]);
	FD_ZERO(&g_net_server->_fdsW[1]);
	FD_ZERO(&g_net_server->_fdsE[1]);
	g_net_server->_maxFD = 0;
#else
	g_net_server->_epoll_fd = 0;
	g_net_server->_max_poll_size = 1024;
#endif

	CMapIterator it = map_iterator_begin(&_handleMap);
	while (!map_iterator_done(&it)) {
		TreeNode* node = map_iterator_current(&it);
		safe_close_socket(&node->key);
		map_iterator_next(&it);
	}
	map_destroy(&_handleMap);

#ifdef _WIN32
	WSACleanup();
#else
	if (g_net_server->_epoll_fd > 0) {
		close(g_net_server->_epoll_fd);
	}
#endif
	g_net_server->running = false;
	AUTO_LOCK_DESTORY(&g_net_server->lock);

	if (g_net_server)
	{
		free(g_net_server);
	}
}

void write_socket_error(const char* context, sock_t sock)
{
	char buff[256] = { 0 };
#ifdef _WIN32
	int err = WSAGetLastError();
	sprintf(buff, "%s (socket %d, error %d)", context, sock, err);
#else
	sprintf(buff, "%s (socket %d, error %d)", context, sock, errno);
#endif
	log_i(buff);
}

int handle_connect_error(sock_t sock)
{
#ifdef _WIN32
	int err = WSAGetLastError();
	switch (err) {
	case WSAEWOULDBLOCK:
	case WSAEINPROGRESS:
		return 0;  // 非阻塞连接中
	default:
		write_socket_error("connect failed", sock);
		return SOCKET_ERROR;
	}
#else
	if (errno == EINPROGRESS || errno == EALREADY) {
		return 0;  // 非阻塞连接中
	}
	write_socket_error("connect failed", sock);
	return SOCKET_ERROR;
#endif
}

ClientSession* create_session(sock_t sock, event_cb rcb, event_cb wcb, event_cb ecb)
{
	ClientSession* cc = (ClientSession*)malloc(sizeof(ClientSession));
	if (!cc)
	{
		log_e("memory allocation failed for session");
		return NULL;
	}
	memset(cc, 0, sizeof(ClientSession));
	cc->sock = sock;
	cc->last_active = time(NULL);
	cc->read_cb = rcb;
	cc->write_cb = wcb;
	cc->remove_cb = ecb;

	input_stream_inithandle(&cc->_instream, cc);
	return cc;
}

void delete_session(void* node)
{
	ClientSession* cc = (ClientSession*)node;
	if (cc)
	{
		safe_close_socket(&cc->sock);
		free(cc);
	}
}

// 网络线程（平台相关实现）
#ifdef _WIN32
DWORD WINAPI network_thread_run(LPVOID arg)
{
	struct timeval tv = { 0, 1000 };
	while (g_net_server->running) {
		if (!g_net_server->running) break;
		//睡眠10毫秒
		Sleep(10);
		AUTO_LOCK(&g_net_server->lock);

		memcpy(&g_net_server->_fdsR[0], &g_net_server->_fdsR[1], sizeof(g_net_server->_fdsR[1]));
		memcpy(&g_net_server->_fdsW[0], &g_net_server->_fdsW[1], sizeof(g_net_server->_fdsW[1]));
		memcpy(&g_net_server->_fdsE[0], &g_net_server->_fdsE[1], sizeof(g_net_server->_fdsE[1]));

		int ret = select(g_net_server->_maxFD + 1, &g_net_server->_fdsR[0], &g_net_server->_fdsW[0], &g_net_server->_fdsE[0], &tv);
		if (ret > 0) {
			// 处理活跃连接
			for (int i = 0; i < (int)g_net_server->_fdsR[0].fd_count; i++) {
				TreeNode* pNode = map_find(&_handleMap, (int)g_net_server->_fdsR[0].fd_array[i]);
				if (NULL != pNode && ((ClientSession*)pNode->value)->read_cb)
				{
					((ClientSession*)pNode->value)->read_cb(pNode);
				}
			}

			for (int i = 0; i < (int)g_net_server->_fdsW[0].fd_count; i++) {  //写数据
				sock_t tmpfd = g_net_server->_fdsW[0].fd_array[i];
				TreeNode* pNode = map_find(&_handleMap, (int)g_net_server->_fdsW[0].fd_array[i]);
				if (pNode && ((ClientSession*)pNode->value)->write_cb) {
					((ClientSession*)pNode->value)->write_cb(pNode);
				}
			}

			for (int i = 0; i < (int)g_net_server->_fdsE[0].fd_count; i++){  //异常数据
				TreeNode* pNode = map_find(&_handleMap, (int)g_net_server->_fdsE[0].fd_array[i]);
				if (pNode && ((ClientSession*)pNode->value)->remove_cb) {
					((ClientSession*)pNode->value)->remove_cb(pNode);
				}
			}
		}
		AUTO_UNLOCK(&g_net_server->lock);
	}
	return 0;
}
#else
void* network_thread_run(void* arg) {
	//忽略SIGPIPE信号
	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

	//epoll模型处理网络数据
	struct epoll_event events[g_net_server->_max_poll_size];
	int curfds = g_net_server->_max_poll_size;
	struct sockaddr_in their_addr;
	int len = sizeof(struct sockaddr_in);
	AUTO_LOCK_INIT(&g_net_server->lock);
	while (g_net_server->running) {
		//休眠10毫秒
		usleep(1000 * 10);
		int nfds = epoll_wait(g_net_server->_epoll_fd, events, g_net_server->_max_poll_size, 50);
		if (nfds == -1)
		{
			if (errno != EINTR) {
				write_socket_error("epoll_wait error", 0);
			}
			continue;
		}

		AUTO_LOCK(&g_net_server->lock);
		for (int i = 0; i < nfds; i++) {
			TreeNode* pNode = map_find(&_handleMap, events[i].data.fd);
			if (!pNode) continue;

			ClientSession* cc = (ClientSession*)pNode->value;
			uint32_t revents = events[i].events;

			if (revents & EPOLLIN && cc->read_cb)   //读数据
			{
				cc->read_cb(pNode);
			}
			if (revents & EPOLLOUT && cc->write_cb) //写数据
			{
				cc->write_cb(pNode);
			}
			if ((revents & (EPOLLHUP | EPOLLERR)) && cc->remove_cb)  // 异常数据
			{
				cc->remove_cb(pNode);
			}
		}
		AUTO_UNLOCK(&g_net_server->lock);
	}
	AUTO_LOCK_DESTORY(&g_net_server->lock);
	return NULL;
}
#endif


//创建socket
sock_t create_socket(int port)
{
	g_net_server->_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (g_net_server->_sock == SOCKET_ERROR)
	{
		write_socket_error("create socket failed", 0);
		return SOCKET_ERROR;
	}
	//设置为端口重用
	// 设置socket选项
	set_reuse(g_net_server->_sock, 1);
	set_linger(g_net_server->_sock, 0, 0);
	set_non_blocking(g_net_server->_sock, 1);
	set_net_recv_buffsize(g_net_server->_sock, 0);
	set_net_send_buffsize(g_net_server->_sock, 0);
	set_send_time(g_net_server->_sock, 0);
	set_recv_time(g_net_server->_sock, 0);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = PF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;;
	if (bind(g_net_server->_sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		write_socket_error("bind failed", g_net_server->_sock);
		safe_close_socket(&g_net_server->_sock);
		return SOCKET_ERROR;
	}

	if (listen(g_net_server->_sock, 1024) == SOCKET_ERROR)
	{
		write_socket_error("listen failed", g_net_server->_sock);
		safe_close_socket(&g_net_server->_sock);
		return SOCKET_ERROR;
	}
	// 添加监听socket到管理器
	ClientSession* cc = create_session(g_net_server->_sock, accept_socket, NULL, NULL);
	if (cc) {
		insert_handle(g_net_server->_sock, cc);
	}

	return g_net_server->_sock;
}
//连接
int connect_server(const char* host, int port)
{
	sock_t sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)  //创建socket失败
	{
		return -1;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(port);

	int ret = set_non_blocking(sock, 0);

	set_net_recv_buffsize(sock, 0);
	set_net_send_buffsize(sock, 0);

	set_send_time(sock, 0);
	set_recv_time(sock, 0);

	if (connect(sock, (const struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		int result = handle_connect_error(sock);
		safe_close_socket(&sock);
		return result;
	}

	ret = set_non_blocking(sock, 1);
	ret = set_linger(sock, 0, 0);
	set_reuse(sock, 1);

	// 创建并插入会话
	ClientSession* session = create_session(sock, read_socket, write_socket, remove_socket);
	if (session) {
		insert_handle(sock, session);
		return (int)sock;
	}

	safe_close_socket(&sock);
	return SOCKET_ERROR;
}

int socket_data(sock_t sock, const char* buff, int len)
{
	if (sock == INVALID_SOCKET || buff == NULL || len <= 0) {
		return SOCKET_ERROR;  // 参数检查
	}

	int total_sent = 0;
	const int64_t timeout_ms = 3000;  // 总超时时间 3 秒
	const int64_t start_time = get_current_time_ms();  // 获取当前时间

	// 仅用于 Linux 的 epoll 相关变量
#ifdef __linux__
	int epoll_fd = -1;
	struct epoll_event event, events[1];
#endif

	while (total_sent < len) {
		// 检查超时
		if (get_current_time_ms() - start_time > timeout_ms) {
			write_socket_error("Write operation timed out", sock);
#ifdef __linux__
			if (epoll_fd >= 0) close(epoll_fd);
#endif
			return SOCKET_ERROR;
		}

		int remaining = len - total_sent;
		int nSent = send(sock, buff + total_sent, remaining, 0);

		if (nSent > 0) {
			total_sent += nSent;
			continue;  // 成功发送部分数据，继续发送剩余部分
		}

		// 错误处理
		if (nSent == SOCKET_ERROR) {
#ifdef _WIN32
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) {
				// 使用 select 等待套接字可写
				fd_set writeSet;
				FD_ZERO(&writeSet);
				FD_SET(sock, &writeSet);

				struct timeval timeout = { 0, 10000 };  // 10ms 超时
				if (select(0, NULL, &writeSet, NULL, &timeout) > 0) {
					continue;  // 套接字可写，重试发送
				}
			}
			else if (err == WSAECONNRESET || err == WSAECONNABORTED) {
				return -1;  // 连接已重置
			}
#else
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				// 使用 epoll 替代 poll
				if (epoll_fd < 0) {
					// 首次创建 epoll 实例
					epoll_fd = epoll_create1(0);
					if (epoll_fd < 0) {
						write_socket_error("Failed to create epoll instance", sock);
						return SOCKET_ERROR;
					}

					event.events = EPOLLOUT;
					event.data.fd = sock;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event) < 0) {
						write_socket_error("Failed to add socket to epoll", sock);
						close(epoll_fd);
						return SOCKET_ERROR;
					}
				}

				// 等待套接字可写
				int ready = epoll_wait(epoll_fd, events, 1, 10); // 10ms 超时
				if (ready > 0 && (events[0].events & EPOLLOUT)) {
					continue;  // 套接字可写，重试发送
				}
			}
			else if (errno == EPIPE || errno == ECONNRESET) {
#ifdef __linux__
				if (epoll_fd >= 0) close(epoll_fd);
#endif
				return -1;  // 连接已重置
			}
#endif
			else {
				write_socket_error("Send operation failed", sock);
#ifdef __linux__
				if (epoll_fd >= 0) close(epoll_fd);
#endif
				return SOCKET_ERROR;
			}
		}
		else if (nSent == 0) {
#ifdef __linux__
			if (epoll_fd >= 0) close(epoll_fd);
#endif
			return -1;  // 连接关闭
		}
	}

#ifdef __linux__
	// 循环结束前清理 epoll 资源
	if (epoll_fd >= 0) close(epoll_fd);
#endif

	return total_sent == len ? 1 : SOCKET_ERROR;
}


void make_protocol_head(SProtocolHead* head, int len, int sourceLen, bool compress, bool encrypt, byte_t encryptAddLength)
{
	head->messageSize = endian_int(len);
	head->magic = MESSAGE_TYPE_CDE;
	head->magic |= compress ? MESSAGE_STATUS_COMPRESSION : 0;
	head->magic |= encrypt ? MESSAGE_STATUS_ENCRYPT : 0;
}

int socket_send_data(sock_t sock, bytestream* bystream)
{
	int len = bytestream_getdatasize(bystream);
	int compressLen = 0;
	bool compress = false;
	bool encrypt = false;
	byte_t addLength = 0;
	SProtocolHead head;
	make_protocol_head(&head, len, len, compress, encrypt, addLength);
	//获取一个bytestream对象
	bytestream* btstream = obtain_bytestream();
	bytestream_append(btstream, &head, sizeof(SProtocolHead));
	if (encrypt)
	{
		bytestream_append(btstream, &addLength, sizeof(byte_t));
	}
	if (compress)
	{
		bytestream_append(btstream, &compressLen, sizeof(int));
	}
	bytestream_append(btstream, bytestream_getdata(bystream), len);
	//通过socket发送
	// 发送数据
	int bytesSent = socket_data(sock, bytestream_getdata(btstream), bytestream_getdatasize(btstream));
	giveup_bytestream(btstream);

	return bytesSent;
}

void insert_handle(sock_t sock, ClientSession* cc)
{
	if (!cc || sock <= 0) return;

	AUTO_LOCK(&g_net_server->lock);
#ifdef _WIN32
	FD_SET(sock, &g_net_server->_fdsR[1]);
	FD_SET(sock, &g_net_server->_fdsW[1]);
	FD_SET(sock, &g_net_server->_fdsE[1]);
	g_net_server->_maxFD = max(g_net_server->_maxFD, (int)sock);
#else
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
	ev.data.fd = sock;
	if (epoll_ctl(g_net_server->_epoll_fd, EPOLL_CTL_ADD, sock, &ev) < 0)
	{
		write_socket_error("epoll_ctl add failed", sock);
	}
#endif
	map_insert(&_handleMap, (int)sock, cc);

	AUTO_UNLOCK(&g_net_server->lock);
}

int accept_socket(void* arg)
{
	TreeNode* pNode = (TreeNode*)arg;
	if (!pNode) return 0;

	ClientSession* server_cc = (ClientSession*)pNode->value;
	sock_t server_sock = server_cc->sock;

	while (1)
	{
		struct sockaddr_in their_addr;
		memset(&their_addr, 0, sizeof(struct sockaddr_in));
		int len = sizeof(struct sockaddr_in);
		sock_t new_fd = accept(g_net_server->_sock, (struct sockaddr *) &their_addr, &len);
		if (new_fd == SOCKET_ERROR)
		{
#ifdef _WIN32
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK || err == WSAEINTR) return 1;
#else
			if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) return 1;
#endif
			write_socket_error("accept failed", server_sock);
			return 0;
		}
		else
		{
			//设置为端口重用
			set_linger(new_fd, 0, 0);
			set_reuse(new_fd, 1);
			set_non_blocking(new_fd, 1);
			set_net_recv_buffsize(new_fd, 0);
			set_net_send_buffsize(new_fd, 0);

			set_send_time(new_fd, 0);
			set_recv_time(new_fd, 0);

			// 创建客户端连接
			ClientSession* client_cc = create_session(new_fd, read_socket, write_socket, remove_socket);
			if (client_cc)
			{
				insert_handle(new_fd, client_cc);

				// 日志和回调
				char ip_str[32] = { '\0' };
				inet_ntop(AF_INET, &their_addr.sin_addr, ip_str, sizeof(ip_str));

				char buff[128];
				snprintf(buff, sizeof(buff), "new connection from %s:%d", ip_str, ntohs(their_addr.sin_port));
				log_i(buff);

				//调用lua函数，把连接放入管理器中
				lua_handler("onNetworkAccept", new_fd, inet_ntoa(their_addr.sin_addr));
			}
			else
			{
				safe_close_socket(&new_fd);
			}
		}
	}
	return 1;
}

int read_socket(void* arg)
{
	TreeNode* pNode = (TreeNode*)arg;
	if (!pNode) return -1;

	ClientSession* cc = (ClientSession*)pNode->value;
	if (!cc) return -2;

	char buff[10 * 1024] = { 0 };
	int total_read = 0;
	int max_reads = 5; // 防止饿死其他连接

	while (max_reads-- > 0)
	{
		int nRec = recv(cc->sock, buff, sizeof(buff), 0);
		if (nRec == SOCKET_ERROR) {
#ifdef _WIN32
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK || err == WSAEINTR) break;
#else
			if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) break;
#endif
			// 其他错误视为连接断开
			remove_socket(pNode);
			return -1;
		}
		else if (nRec == 0)
		{
			// 对端关闭连接
			remove_socket(pNode);
			return 0;
		}
		else
		{
			input_stream_write(&cc->_instream, buff, nRec);
			input_stream_analyse_data(&cc->_instream);
			total_read += nRec;

			// 更新最后活动时间
			cc->last_active = time(NULL);
		}
	}
	return total_read;
}

int write_socket(void* arg)
{
	TreeNode* pNode = (TreeNode*)arg;
	if (!pNode) return -1;

	ClientSession* cc = (ClientSession*)pNode->value;
	if (!cc) return -2;
	//调用lua回调函数，通知发送数据成功
	//如果发送失败，则通知lua, 按照实际需求处理失败的情况
	return 1;
}

int remove_socket(void* arg)
{
	TreeNode* pNode = (TreeNode*)arg;
	if (NULL == pNode)
	{
		return -1;
	}
	ClientSession* cc = (ClientSession*)pNode->value;
	if (cc == NULL)
	{
		return -2;
	}
	AUTO_LOCK(&g_net_server->lock);
#ifdef _WIN32
	FD_CLR(cc->sock, &g_net_server->_fdsW[1]);
	FD_CLR(cc->sock, &g_net_server->_fdsE[1]);
	FD_CLR(cc->sock, &g_net_server->_fdsR[1]);
#else
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	if (epoll_ctl(g_net_server->_epoll_fd, EPOLL_CTL_DEL, cc->sock, &ev) == -1)
	{
		char buff[128] = { 0 };
		sprintf(buff, "remove_socket:%d", cc->sock);
		log_i(buff);
	}
#endif 

	//调用lua回调函数，通知发送数据成功
	lua_handler("onNetworkError", cc->sock, "client close");
	//删除
	map_delete(&_handleMap, (int)cc->sock);
	// 关闭socket并释放资源
	safe_close_socket(&cc->sock);
	free(cc);
	AUTO_UNLOCK(&g_net_server->lock);
	return 1;
}