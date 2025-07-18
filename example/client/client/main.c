#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") 
#else 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#endif
#include "netpub.pb-c.h"
#include "msgtest.pb-c.h"
#include "bytestream.h"


#define SERVER_IP "192.168.8.32"
#define PORT 9500
#define BUFFER_SIZE 1024

static const char magic_flag[4] = { 'C', 'D', 'E', '!' };
#define MESSAGE_TYPE_CDE 0x10
#define MESSAGE_TYPE_CROSS_DOMAIN 0x20

#define MESSAGE_STATUS_COMPRESSION 0x01
#define MESSAGE_STATUS_ENCRYPT 0x02

typedef struct _SProtocolHead
{
	byte_t magic;
	uint_t messageSize;
}SProtocolHead;

// 接收消息线程
#ifdef _WIN32
DWORD WINAPI recv_thread(LPVOID arg)
#else
void* recv_thread(void* arg) 
#endif
{
	const int32_t headSize = sizeof(SProtocolHead);
	int sockfd = *(int*)arg;
	char buffer[BUFFER_SIZE];
	while (1) {
		int bytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
		if (bytes <= 0) break;
// 		buffer[bytes] = '\0';
// 		printf("%s\n", buffer);

		bytestream* buffstream = bytestream_create(0);
		bytestream_append(buffstream, buffer, headSize);
		SProtocolHead phead;
		memcpy(&phead, bytestream_getdata(buffstream), headSize);

		int msgLen = phead.messageSize;
		bytestream_clear(buffstream);
		bytestream_append(buffstream, buffer + headSize, msgLen);

		int type = (int)bytestream_readbyte(buffstream);
		const char* data = bytestream_readstring(buffstream);

		printf("messageType is %d, message: %s\n", type, data);
	}
	return NULL;
}


static void make_protocol_head(SProtocolHead* head, int len, int sourceLen, bool compress, bool encrypt, byte_t encryptAddLength)
{
	head->messageSize = endian_int(len);
	head->magic = MESSAGE_TYPE_CDE;
	head->magic |= compress ? MESSAGE_STATUS_COMPRESSION : 0;
	head->magic |= encrypt ? MESSAGE_STATUS_ENCRYPT : 0;
}


int main() {
#ifdef _WIN32 
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif 

	bytestream* bt = bytestream_create(1024);
	//把消息头追加进去
	bytestream_writestring(bt, "aaaaaa");

	printf("str=%s\n", bytestream_readstring(bt));

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	addr.sin_port = htons(PORT);

	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("Connection failed");
		exit(EXIT_FAILURE);
	}
	printf("Connected to server.\n");

	// 启动接收线程 
#ifdef _WIN32 
	CreateThread(NULL, 0, recv_thread, &sockfd, 0, NULL);
#else 
	pthread_t tid;
	pthread_create(&tid, NULL, recv_thread, &sockfd);
#endif 

	int hs = sizeof(SProtocolHead);

	// 发送消息 
	char buffer[BUFFER_SIZE];
	while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
		//send(sockfd, buffer, strlen(buffer), 0);
		// 从尾部向前扫描，移除所有空白符（含换行）
		size_t len = strlen(buffer);
		while (len > 0 && isspace((unsigned char)buffer[len - 1])) {
			buffer[len - 1] = '\0';
			len--;
		}

		bytestream* buffstream = bytestream_create(1024);
		bytestream_writebyte(buffstream,1);
		bytestream_writestring(buffstream, buffer);
		len = bytestream_getdatasize(buffstream);

		bytestream* bystream = bytestream_create(1024);
		int compressLen = 0;
		bool compress = false;
		bool encrypt = false;
		byte_t addLength = 0;
		SProtocolHead head;
		make_protocol_head(&head, len, len, compress, encrypt, addLength);
		bytestream_append(bystream, &head, sizeof(SProtocolHead));
		if (encrypt)
		{
			bytestream_append(bystream, &addLength, sizeof(byte_t));
		}
		if (compress)
		{
			bytestream_append(bystream, &compressLen, sizeof(int));
		}
		bytestream_append(bystream, bytestream_getdata(buffstream), bytestream_getdatasize(buffstream));

		//把消息头追加进去
//		bytestream_writestring(buffstream, buffer);
// 		bytestream_popdata(bystream, sizeof(SProtocolHead));
// 		printf("str=%s\n", bytestream_readstring(bystream));
// 		int ds = bytestream_getdatasize(bystream);
		int sendSize = bytestream_getdatasize(bystream);
		send(sockfd, bytestream_getdata(bystream), bytestream_getdatasize(bystream), 0);

		//send(sockfd, "a", 1, 0);
	}

	close(sockfd);
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}