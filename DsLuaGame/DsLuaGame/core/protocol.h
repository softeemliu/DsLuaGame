#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__
#include "public.h"

#ifdef _WIN32
#pragma pack(push,4)
#else
#pragma pack(4)
#endif


#define PROTOCOL_VERSION			1
#define PROROCOL_STRING_VERSION		"1.0.0"
#define MAX_RECV_LENGTH				(1024*512)
#define MAX_SEND_LENGTH				(1024*512)
#define STREAM_MAX_LENGTH			(1024*128)
#define COMPRESS_LENGTH				(1024*4)

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

enum EMessageType
{
	MessageTypeMQ = 1,
	MessageTypeCall = 2,
	MessageTypeCallRet = 3,
};


#endif /*__PROTOCOL_H__*/