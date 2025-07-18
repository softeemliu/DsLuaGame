#ifndef __PUBLIC_H__
#define __PUBLIC_H__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#pragma warning(disable: 4819) // Disables "deprecated function" warning
#pragma execution_character_set("utf-8")  // 强制使用 UTF-8

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")  
typedef SOCKET sock_t;
#define BAD_SOCKET INVALID_SOCKET

typedef void * HANDLE;
typedef unsigned long DWORD;
typedef void* LPVOID;
#else

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
typedef int sock_t;
#define BAD_SOCKET -1
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#endif

#ifdef _WIN32
#pragma pack(push,4)
#else
#pragma pack(4)
#endif

#ifdef _WIN32 
#define strdup _strdup 
#endif

#if defined(_WIN32) || defined(__i386) || defined(__x86_64)
#define LITTLE_ENDIAN   1
#endif

#define  _USE_PROTOBUF_  0

#ifndef long64_t
#ifdef _WIN32
typedef unsigned __int64 ulong64_t;
typedef __int64 long64_t;
#else
typedef __int64_t long64_t;
typedef __uint64_t ulong64_t;
#endif
#endif

#ifndef uint_t
typedef unsigned int uint_t;
#endif

#ifndef byte_t
typedef unsigned char byte_t;
#endif

#endif /*__PUBLIC_H__*/