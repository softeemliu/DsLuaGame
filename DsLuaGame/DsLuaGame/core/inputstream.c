#include "inputstream.h"
#include <string.h>
#include "netmsgblock.h"
#include "netmsgqueue.h"
#include "protocol.h"
#include "objectpool/objectpool.h"
#include "logger/elog.h"

//初始化
void init_input_stream(inputstream* stream)
{
    stream->_totalbufflen = 10 * 1024;
    stream->_pbuff = (unsigned char*)malloc(stream->_totalbufflen);
    stream->_rtag = stream->_wtag = stream->_pbuff;
    stream->_phandle = NULL;
}

//销毁
void destory_input_stream(inputstream* stream)
{
    free(stream->_pbuff);
    stream->_pbuff = NULL;
    stream->_rtag = stream->_wtag = NULL;
    stream->_totalbufflen = 0;
}

//
void input_stream_inithandle(inputstream* stream, void* cl)
{
	init_input_stream(stream);

    stream->_phandle = cl;
    stream->_rtag = stream->_wtag = stream->_pbuff;
}

//处理数据
bool input_stream_analyse_data(inputstream* stream)
{
	const int32_t headSize = sizeof(SProtocolHead);
	while (true) 
	{
		int32_t mbsize = (int32_t)(stream->_wtag - stream->_rtag);
		if (mbsize < headSize) {
			break; // 不足消息头长度
		}
		SProtocolHead phead;
		memcpy(&phead, stream->_rtag, headSize); // 直接复制头部

		if (phead.magic ^ MESSAGE_TYPE_CDE && phead.magic ^ MESSAGE_TYPE_CROSS_DOMAIN)
		{
			input_stream_findhead(stream);
			continue;
		}

		int32_t packageSize = phead.messageSize;
		int totalPackageSize = headSize + packageSize;
		if (mbsize < totalPackageSize) {
			break; // 不足完整包长度
		}
		// 提取有效载荷
		netinfo ni;
		ni._clsptr = stream->_phandle;
		ni._pbuf = stream->_rtag + headSize;
		ni._buflen = packageSize;

		// 创建消息块
		netmsgblock* pblk = netmsgblock_allocate(sizeof(netmsgblock));
		netmsgblock_init(pblk, &ni);
		netmsgblock_writenetinfo(pblk, &ni);
		netmsgqueue_pushback(netmsgqueue_getinstance(), pblk);

		// 关键修改：移动剩余数据
		stream->_rtag += totalPackageSize; // 仅跳过已处理包
		int32_t leftLen = stream->_wtag - stream->_rtag; // 剩余数据长度

		if (leftLen > 0) {
			// 使用 memmove 处理重叠内存
			memmove(stream->_pbuff, stream->_rtag, leftLen);
		}

		// 重置指针
		stream->_rtag = stream->_pbuff;
		stream->_wtag = stream->_pbuff + leftLen;

	}
	return true;
}

//查找消息头
void input_stream_findhead(inputstream* stream)
{
    stream->_rtag ++;
    unsigned char* p = stream->_rtag;
    if ( p > stream->_pbuff + stream->_totalbufflen)
    {
        stream->_rtag = stream->_pbuff;
        stream->_wtag = stream->_pbuff;
        return;
    }

    int32_t len = stream->_wtag - stream->_rtag;
    int32_t ipos = 0;
    while ( ipos < len && ipos < stream->_totalbufflen)
    {
        //if (memcmp(readPos, "MES_", 4) == 0)  以字符串"MES_"为消息头
        int flag;
        memcpy(&flag, p, sizeof(int));
        if ( flag == 9999)
        {
            stream->_rtag = p;
            return;
        }
        p ++;
        ipos++;
        if ( p >= stream->_pbuff + stream->_totalbufflen)
        {
            break;
        }
    }
    
    if ( ipos < len)
    {
        memcpy( stream->_pbuff, stream->_rtag, stream->_wtag - stream->_wtag);
        stream->_rtag = stream->_pbuff;
    }
    else
    {
        stream->_rtag = stream->_pbuff;
        stream->_wtag = stream->_pbuff;
    }
}

//写入数据
bool input_stream_write(inputstream* stream, const unsigned char* data, uint32_t len)
{
    uint32_t space = stream->_pbuff + stream->_totalbufflen - stream->_wtag;
    if ( space < len)
    {
        return false;
    }
    
    memcpy( stream->_wtag, data, len);
    stream->_wtag += len;
    return true;
}

//获取空间
int32_t input_stream_getspace(inputstream* stream)
{
	return (int32_t)(stream->_pbuff + stream->_totalbufflen - stream->_wtag);
}
