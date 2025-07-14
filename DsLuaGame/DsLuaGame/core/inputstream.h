#ifndef __INPUT_STREAM_H__
#define __INPUT_STREAM_H__
#include "public.h"

typedef struct _ClientSession ClientSession;

typedef struct _inputstream
{
    unsigned char* _pbuff;
    unsigned char* _rtag;
    unsigned char* _wtag;
    uint32_t _totalbufflen;
	ClientSession* _phandle;
}inputstream;


//初始化
void init_input_stream(inputstream* stream);
//销毁
void destory_input_stream(inputstream* stream);
//
void input_stream_inithandle(inputstream* stream, ClientSession* cl);
//处理数据
bool input_stream_analyse_data(inputstream* stream);
//查找消息头
void input_stream_findhead(inputstream* stream);
//写入数据
bool input_stream_write(inputstream* stream, const unsigned char* data, uint32_t len);
//获取空间
int32_t input_stream_getspace(inputstream* stream);

#endif  /*__INPUT_STREAM_H__*/