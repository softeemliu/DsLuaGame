#ifndef __BYTE_STREAM_H__
#define __BYTE_STREAM_H__
#include "public.h"

#define CDF_BYTES_BUFFER_INIT_SIZE (1024 * 2)
#define BYTES_BUFFER_MAX_SIZE (100 * 1024)
#define CDF_INC_BUFFER_SIZE 512 

// 错误码定义 
typedef enum {
	CDF_SUCCESS = 0,
	CDF_ERROR_READONLY,
	ERROR_OUT_OF_MEMORY,
	CDF_ERROR_BUFFER_OVERFLOW,
	CDF_ERROR_INVALID_DATA
} CDFErrorCode;

typedef struct SeqData {
	int numElements;
	int minSize;
	struct SeqData* previous;
} SeqData;

typedef struct {
	bool readOnly;
	char* buffer;
	int bufferSize;
	int dataSize;
	int readIndex;
	SeqData* seqDataStack;
} bytestream;



// 初始化函数 
bytestream* bytestream_create(int len);
// 从现有缓冲区创建 
bytestream* bytestream_createfrombuffer(const void* buf, int len);
// 销毁函数 
void bytestream_destroy(bytestream* stream);
// 内存扩展检查 
static CDFErrorCode checkbufferoverflow(bytestream* stream, int addLength);
// 追加数据 
CDFErrorCode bytestream_append(bytestream* stream, const void* data, int dataSize);
// 清空缓冲区
void bytestream_clear(bytestream* stream);
// 重置读取索引
void bytestream_reset(bytestream* stream);
// 读取剩余字节数 
int bytestream_getbytesleft(const bytestream* stream);
//获取数据
const char* bytestream_getdata(const bytestream* stream);
//获取长度
const int bytestream_getdatasize(const bytestream* stream);

// 写入单字节
void bytestream_writebyte(bytestream* stream, byte value);
// 读取单字节
byte bytestream_readbyte(bytestream* stream);
// 写入字节数组 
CDFErrorCode bytestream_writebytearray(bytestream* stream, const byte* data, int dataSize);
// 读取字节数组 
byte* bytestream_readbytearray(bytestream* stream, int len);

// 写入 bool 值（占用 1 字节）
void bytestream_writebool(bytestream* stream, bool value);
// 读取 bool 值 
bool bytestream_readbool(bytestream* stream);
// 写入 bool 数组 
CDFErrorCode bytestream_writeboolarray(bytestream* stream, const bool* arr, int count);
// 读取 bool 数组 
bool* bytestream_readboolarray(bytestream* stream, int* outCount);


// 写入单个short（16位整数）
void bytestream_writeshort(bytestream* stream, short value);
// 读取单个short（16位整数）
short bytestream_readshort(bytestream* stream);
// 写入short数组（格式：[长度(int)] + [元素1(short)] + [元素2(short)] + ...）
CDFErrorCode bytestream_writeshortarray(bytestream* stream, const short* arr, int count);
// 读取short数组（返回动态分配的数组，需调用者释放）
// outCount：输出数组长度（若失败则设为0）
short* bytestream_readshortarray(bytestream* stream, int* outCount);


//写入float
void bytestream_writefloat(bytestream* stream, float value);
//读取float
float bytestream_readfloat(bytestream* stream);
//写入float数组
CDFErrorCode bytestream_writefloatarray(bytestream* stream, const float* arr, int count);
//读取float数组
float* bytestream_readfloatarray(bytestream* stream, int* outCount);


// 写入整数（处理字节序）
void bytestream_writeint(bytestream* stream, int value);
// 读取整数（处理字节序）
int bytestream_readint(bytestream* stream);
//写入Int数组长度
CDFErrorCode bytestream_writeintarray(bytestream* stream, const int* arr, int count);
//读取Int数组
int* bytestream_readintarray(bytestream* stream, int* outCount);


// 写入 double 类型 
void bytestream_writedouble(bytestream* stream, double value);
// 读取 double 类型 
double bytestream_readdouble(bytestream* stream);
// 写入 double 数组 
CDFErrorCode bytestream_writedoublearray(bytestream* stream, const double* arr, int count);
// 读取 double 数组 
double* bytestream_readdoublearray(bytestream* stream, int* outCount);


// 写入 long64_t（处理字节序）
void bytestream_writelong64(bytestream* stream, long64_t value);
// 读取 long64_t（处理字节序）
long64_t bytestream_readlong64(bytestream* stream);
// 写入 long64_t 数组 
CDFErrorCode bytestream_writelong64array(bytestream* stream, const long64_t* arr, int count);
// 读取 long64_t 数组 
long64_t* bytestream_readlong64array(bytestream* stream, int* outCount);


// 写入单个uint64_t 
void bytestream_writeuint64(bytestream* stream, uint64_t value);
// 读取单个uint64_t 
uint64_t bytestream_readuint64(bytestream* stream);
// 写入uint64_t数组 
CDFErrorCode bytestream_writeuint64array(bytestream* stream, const uint64_t* arr, int count);
// 读取uint64_t数组 
uint64_t* bytestream_readuint64array(bytestream* stream, int* outCount);


// 写入字符串（包含长度）
void bytestream_writestring(bytestream* stream, const char* str);
// 读取字符串 
char* bytestream_readstring(bytestream* stream);
// 写入字符串数组
CDFErrorCode bytestream_writestringarray(bytestream* stream, const char** arr, int count);
// 读取字符串数组 
char** bytestream_readstringarray(bytestream* stream, int* outCount);


// 示例：序列化结构处理 
void bytestream_startseq(bytestream* stream, int numElements, int minSize);

void bytestream_endseq(bytestream* stream);
// 错误处理示例 
void bytestream_setreadonly(bytestream* stream, bool readOnly);


#endif  /*__BYTE_STREAM_H__*/