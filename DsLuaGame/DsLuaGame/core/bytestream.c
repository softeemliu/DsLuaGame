#include "bytestream.h"
#include "utils/endian.h"

// 错误处理宏（模拟C++异常）
#define THROW_EXCEPTION(msg) fprintf(stderr, "Error: %s\n", msg); abort()

uint16_t endian16(uint16_t value) {
	return (value >> 8) | (value << 8); // 交换高8位和低8位
}

uint32_t endian(uint32_t value) {
	return ((value >> 24) & 0x000000FF) |
		((value >> 8) & 0x0000FF00) |
		((value << 8) & 0x00FF0000) |
		((value << 24) & 0xFF000000);
}

uint64_t endian64(uint64_t value) {
	return ((value >> 56) & 0x00000000000000FF) |
		((value >> 40) & 0x000000000000FF00) |
		((value >> 24) & 0x0000000000FF0000) |
		((value >> 8) & 0x00000000FF000000) |
		((value << 8) & 0x000000FF00000000) |
		((value << 24) & 0x0000FF0000000000) |
		((value << 40) & 0x00FF000000000000) |
		((value << 56) & 0xFF00000000000000);
}

// 初始化函数 
bytestream* bytestream_create(int len) {
	bytestream* stream = (bytestream*)malloc(sizeof(bytestream));
	if (!stream) return NULL;

	stream->readOnly = false;
	stream->bufferSize = 0;
	stream->dataSize = 0;
	stream->buffer = NULL;
	stream->readIndex = 0;
	stream->seqDataStack = NULL;

	if (len != 0) {
		stream->buffer = (char*)malloc(len);
		if (!stream->buffer) {
			free(stream);
			return NULL;
		}
		stream->bufferSize = len;
	}
	return stream;
}

// 从现有缓冲区创建 
bytestream* bytestream_createfrombuffer(const void* buf, int len) {
	bytestream* stream = (bytestream*)malloc(sizeof(bytestream));
	if (!stream) return NULL;

	stream->readOnly = true;
	stream->buffer = (char*)buf;
	stream->bufferSize = len;
	stream->dataSize = len;
	stream->readIndex = 0;
	stream->seqDataStack = NULL;
	return stream;
}

// 销毁函数 
void bytestream_destroy(bytestream* stream) {
	if (!stream) return;

	if (!stream->readOnly && stream->buffer) {
		free(stream->buffer);
	}

	// 释放序列数据栈
	SeqData* current = stream->seqDataStack;
	while (current) {
		SeqData* next = current->previous;
		free(current);
		current = next;
	}

	free(stream);
}

// 内存扩展检查 
CDFErrorCode checkbufferoverflow(bytestream* stream, int addLength) {
	if (stream->readOnly) return CDF_ERROR_READONLY;

	int requiredSize = stream->dataSize + addLength;
	if (stream->bufferSize >= requiredSize) return CDF_SUCCESS;

	int incSize = stream->bufferSize;
	if (incSize == 0) incSize = CDF_BYTES_BUFFER_INIT_SIZE;

	while (incSize < requiredSize) {
		incSize += incSize >> 1; // 每次增加50%
	}

	void* newBuffer = realloc(stream->buffer, incSize);
	if (!newBuffer) return ERROR_OUT_OF_MEMORY;

	stream->buffer = (char*)newBuffer;
	stream->bufferSize = incSize;
	return CDF_SUCCESS;
}

// 追加数据 
CDFErrorCode bytestream_append(bytestream* stream, const void* data, int dataSize) {
	if (checkbufferoverflow(stream, dataSize) != CDF_SUCCESS)
		return ERROR_OUT_OF_MEMORY;

	memcpy(stream->buffer + stream->dataSize, data, dataSize);
	stream->dataSize += dataSize;
	return CDF_SUCCESS;
}

// 清空缓冲区
void bytestream_clear(bytestream* stream) {
	if (stream->readOnly) return;

	stream->dataSize = 0;
	stream->readIndex = 0;

	if (stream->bufferSize > BYTES_BUFFER_MAX_SIZE) {
		free(stream->buffer);
		stream->buffer = NULL;
		stream->bufferSize = 0;
	}
}

// 重置读取索引
void bytestream_reset(bytestream* stream) {
	stream->readIndex = 0;
}

// 读取剩余字节数 
int bytestream_getbytesleft(const bytestream* stream) {
	return stream->dataSize - stream->readIndex;
}

const char* bytestream_getdata(const bytestream* stream)
{
	return stream->buffer;
}

const int bytestream_getdatasize(const bytestream* stream)
{
	return stream->dataSize;
}

// 写入单字节
void bytestream_writebyte(bytestream* stream, byte value)
{
	if (checkbufferoverflow(stream, 1) != CDF_SUCCESS) {
		THROW_EXCEPTION("Failed to write byte: Out of memory");
	}
	stream->buffer[stream->dataSize] = (char)value;
	stream->dataSize++;
}

// 读取单字节
byte bytestream_readbyte(bytestream* stream)
{
	if (stream->readIndex >= stream->dataSize) {
		THROW_EXCEPTION("Failed to read byte: No more data");
	}
	byte value = (byte)stream->buffer[stream->readIndex];
	stream->readIndex++;
	return value;
}

// 写入字节数组
CDFErrorCode bytestream_writebytearray(bytestream* stream, const byte* data, int dataSize) {
	if (stream->readOnly) return CDF_ERROR_READONLY;
	if (checkbufferoverflow(stream, dataSize) != CDF_SUCCESS)
		return ERROR_OUT_OF_MEMORY;

	memcpy(stream->buffer + stream->dataSize, data, dataSize);  // 内存拷贝 
	stream->dataSize += dataSize;
	return CDF_SUCCESS;
}

// 读取字节数组 
byte* bytestream_readbytearray(bytestream* stream, int len) {
	if (len <= 0 || stream->readIndex + len > stream->dataSize)
		return NULL;

	byte* bytes = (byte*)malloc(len);  // 动态分配内存
	if (!bytes) return NULL;

	memcpy(bytes, stream->buffer + stream->readIndex, len);
	stream->readIndex += len;  // 移动读指针 
	return bytes;  // 需调用者释放内存
}

// 写入 bool 值（占用 1 字节）
void bytestream_writebool(bytestream* stream, bool value) {
	byte byteValue = value ? 1 : 0;  // 将 bool 转为字节（0=false, 1=true）
	bytestream_writebyte(stream, byteValue);
}

// 读取 bool 值 
bool bytestream_readbool(bytestream* stream) {
	byte value = bytestream_readbyte(stream);
	return (value != 0);  // 非 0 即为 true 
}

// 写入 bool 数组 
CDFErrorCode bytestream_writeboolarray(bytestream* stream, const bool* arr, int count) {
	// 写入数组长度 
	bytestream_writeint(stream, count);

	// 检查缓冲区空间 
	int totalBytes = count * sizeof(byte);  // 每个 bool 占 1 字节 
	if (checkbufferoverflow(stream, totalBytes) != CDF_SUCCESS)
		return CDF_ERROR_BUFFER_OVERFLOW;

	// 逐个写入元素 
	for (int i = 0; i < count; i++) {
		byte byteValue = arr[i] ? 1 : 0;
		bytestream_writebyte(stream, byteValue);
	}
	return CDF_SUCCESS;
}

// 读取 bool 数组 
bool* bytestream_readboolarray(bytestream* stream, int* outCount) {
	// 读取数组长度 
	int count = bytestream_readint(stream);
	if (count <= 0 || stream->readIndex + count > stream->dataSize)
		return NULL;

	// 分配内存 
	bool* arr = (bool*)malloc(count * sizeof(bool));
	if (!arr) return NULL;

	// 读取并转换每个字节为 bool 
	for (int i = 0; i < count; i++) {
		byte value = bytestream_readbyte(stream);
		arr[i] = (value != 0);
	}

	*outCount = count;
	return arr;
}


// 写入单个short（16位整数）
void bytestream_writeshort(bytestream* stream, short value) {
	if (stream->readOnly) {
		THROW_EXCEPTION("Attempt to write to read-only stream");
		return;
	}

	// 处理字节序（非小端系统需转换）
#ifndef DSLITTLE_ENDIAN 
	value = (short)endian16((uint16_t)value);
#endif 

	// 检查缓冲区是否足够（需2字节）
	if (checkbufferoverflow(stream, sizeof(short)) != CDF_SUCCESS) {
		THROW_EXCEPTION("Buffer overflow in writeshort");
		return;
	}

	// 将数据复制到缓冲区（保持二进制精度）
	memcpy(stream->buffer + stream->dataSize, &value, sizeof(short));
	stream->dataSize += sizeof(short);
}

// 读取单个short（16位整数）
short bytestream_readshort(bytestream* stream) {
	// 检查缓冲区是否有足够数据（需2字节）
	if (stream->readIndex + sizeof(short) > stream->dataSize) {
		THROW_EXCEPTION("Insufficient data for readshort");
		return 0;
	}

	short value;
	// 从缓冲区复制数据 
	memcpy(&value, stream->buffer + stream->readIndex, sizeof(short));
	stream->readIndex += sizeof(short); // 移动读指针 

	// 处理字节序（非小端系统需转换）
#ifndef DSLITTLE_ENDIAN 
	value = (short)endian16((uint16_t)value);
#endif 

	return value;
}

// 写入short数组（格式：[长度(int)] + [元素1(short)] + [元素2(short)] + ...）
CDFErrorCode bytestream_writeshortarray(bytestream* stream, const short* arr, int count) {
	if (stream->readOnly) {
		return CDF_ERROR_READONLY;
	}
	if (count <= 0) {
		return CDF_SUCCESS; // 空数组无需处理 
	}

	// 第一步：写入数组长度（int类型，占4字节）
	bytestream_writeint(stream, count);

	// 第二步：计算数组总字节数（count * 2字节）
	int totalBytes = count * sizeof(short);
	// 检查缓冲区是否足够 
	if (checkbufferoverflow(stream, totalBytes) != CDF_SUCCESS) {
		return CDF_ERROR_BUFFER_OVERFLOW;
	}

	// 第三步：逐个写入元素（处理字节序） 
	for (int i = 0; i < count; i++) {
		short value = arr[i];
#ifndef DSLITTLE_ENDIAN 
		value = (short)endian16((uint16_t)value);
#endif 
		memcpy(stream->buffer + stream->dataSize, &value, sizeof(short));
		stream->dataSize += sizeof(short);
	}

	return CDF_SUCCESS;
}

// 读取short数组（返回动态分配的数组，需调用者释放）
// outCount：输出数组长度（若失败则设为0）
short* bytestream_readshortarray(bytestream* stream, int* outCount) {
	if (!outCount) {
		THROW_EXCEPTION("outCount is NULL in readshortarray");
		return NULL;
	}
	*outCount = 0;

	// 第一步：读取数组长度（int类型）
	int count = bytestream_readint(stream);
	if (count <= 0) {
		return NULL; // 无效长度 
	}

	// 第二步：检查缓冲区是否有足够数据（count * 2字节）
	if (stream->readIndex + count * sizeof(short) > stream->dataSize) {
		THROW_EXCEPTION("Insufficient data for readshortarray");
		return NULL;
	}

	// 第三步：分配内存（动态分配，需调用者释放）
	short* arr = (short*)malloc(count * sizeof(short));
	if (!arr) {
		THROW_EXCEPTION("Out of memory in readshortarray");
		return NULL;
	}

	// 第四步：逐个读取元素（处理字节序） 
	for (int i = 0; i < count; i++) {
		memcpy(&arr[i], stream->buffer + stream->readIndex, sizeof(short));
		stream->readIndex += sizeof(short); // 移动读指针 
#ifndef DSLITTLE_ENDIAN 
		arr[i] = (short)endian16((uint16_t)arr[i]);
#endif 
	}

	*outCount = count;
	return arr;
}


//写入float
void bytestream_writefloat(bytestream* stream, float value) {
	if (stream->readOnly) {
		THROW_EXCEPTION("Attempt to write to read-only stream");
		return;
	}

	// 将 float 转为 4 字节整型处理字节序
	uint32_t tmp;
	memcpy(&tmp, &value, sizeof(float)); // 保持二进制精度[4]()
#ifndef DSLITTLE_ENDIAN 
	tmp = endian(tmp); // 大端系统需转换字节序
#endif 

	// 检查缓冲区并写入 
	if (checkbufferoverflow(stream, sizeof(float)) != CDF_SUCCESS) {
		THROW_EXCEPTION("Buffer overflow in writefloat");
		return;
	}
	memcpy(stream->buffer + stream->dataSize, &tmp, sizeof(float));
	stream->dataSize += sizeof(float);
}

//读取float
float bytestream_readfloat(bytestream* stream) {
	if (stream->readIndex + sizeof(float) > stream->dataSize) {
		THROW_EXCEPTION("Insufficient data for readfloat");
		return 0.0f;
	}

	uint32_t tmp;
	memcpy(&tmp, stream->buffer + stream->readIndex, sizeof(float));
	stream->readIndex += sizeof(float);

#ifndef DSLITTLE_ENDIAN 
	tmp = endian(tmp); // 大端系统需还原字节序
#endif 

	float value;
	memcpy(&value, &tmp, sizeof(float));
	return value;
}

//写入float数组
CDFErrorCode bytestream_writefloatarray(bytestream* stream, const float* arr, int count) {
	// 先写入数组长度（兼容历史代码规范）
	bytestream_writeint(stream, count);

	// 检查总空间需求 
	int totalBytes = count * sizeof(float);
	if (checkbufferoverflow(stream, totalBytes) != CDF_SUCCESS) {
		return CDF_ERROR_BUFFER_OVERFLOW;
	}

	// 批量处理字节序（优化性能）
	for (int i = 0; i < count; ++i) {
		uint32_t tmp;
		memcpy(&tmp, &arr[i], sizeof(float));
#ifndef DSLITTLE_ENDIAN 
		tmp = endian(tmp);
#endif 
		memcpy(stream->buffer + stream->dataSize, &tmp, sizeof(float));
		stream->dataSize += sizeof(float);
	}
	return CDF_SUCCESS;
}

//读取float数组
float* bytestream_readfloatarray(bytestream* stream, int* outCount) {
	// 读取数组长度 
	int count = bytestream_readint(stream);
	if (count <= 0 || stream->readIndex + count * sizeof(float) > stream->dataSize) {
		*outCount = 0;
		return NULL;
	}

	// 分配内存（使用 calloc 避免未初始化风险）
	float* arr = (float*)calloc(count, sizeof(float));
	if (!arr) return NULL;

	// 批量读取并转换字节序
	for (int i = 0; i < count; ++i) {
		uint32_t tmp;
		memcpy(&tmp, stream->buffer + stream->readIndex, sizeof(float));
		stream->readIndex += sizeof(float);

#ifndef DSLITTLE_ENDIAN 
		tmp = endian(tmp);
#endif 

		memcpy(&arr[i], &tmp, sizeof(float));
	}

	*outCount = count;
	return arr;
}

// 写入整数（处理字节序）
void bytestream_writeint(bytestream* stream, int value) {
#ifndef DSLITTLE_ENDIAN 
	value = endian(value);
#endif 
	bytestream_append(stream, &value, sizeof(int));
}

// 读取整数（处理字节序）
int bytestream_readint(bytestream* stream) {
	if (stream->readIndex + sizeof(int) > stream->dataSize) return 0;

	int value;
	memcpy(&value, stream->buffer + stream->readIndex, sizeof(int));
#ifndef DSLITTLE_ENDIAN 
	value = endian(value);
#endif 
	stream->readIndex += sizeof(int);
	return value;
}

CDFErrorCode bytestream_writeintarray(bytestream* stream, const int* arr, int count)
{
	// 先写入数组长度 
	bytestream_writeint(stream, count);
	// 计算总需空间（每个int占4字节）
	int totalBytes = count * sizeof(int);
	if (checkbufferoverflow(stream, totalBytes) != CDF_SUCCESS)
		return CDF_ERROR_BUFFER_OVERFLOW;

	// 逐个写入元素（带字节序转换）
	for (int i = 0; i < count; i++) {
		int value = arr[i];
#ifndef DSLITTLE_ENDIAN 
		value = endian(value);
#endif 
		memcpy(stream->buffer + stream->dataSize, &value, sizeof(int));
		stream->dataSize += sizeof(int);
	}
	return CDF_SUCCESS;
}

int* bytestream_readintarray(bytestream* stream, int* outCount) {
	// 读取数组长度 
	int count = bytestream_readint(stream);
	if (count <= 0 || stream->readIndex + count * sizeof(int) > stream->dataSize)
		return NULL;

	// 分配内存 
	int* arr = (int*)malloc(count * sizeof(int));
	for (int i = 0; i < count; i++) {
		memcpy(&arr[i], stream->buffer + stream->readIndex, sizeof(int));
#ifndef DSLITTLE_ENDIAN 
		arr[i] = endian(arr[i]);
#endif 
		stream->readIndex += sizeof(int);
	}

	*outCount = count;
	return arr;
}

// 写入 double 类型 
void bytestream_writedouble(bytestream* stream, double value) {
	if (checkbufferoverflow(stream, sizeof(double)) != CDF_SUCCESS)
		return;

	// 处理字节序问题 
	uint64_t temp;
	memcpy(&temp, &value, sizeof(double));
#ifndef DSLITTLE_ENDIAN 
	temp = endian64(temp); // 需要添加 64 位字节序转换函数 
#endif 

	memcpy(stream->buffer + stream->dataSize, &temp, sizeof(double));
	stream->dataSize += sizeof(double);
}

// 读取 double 类型 
double bytestream_readdouble(bytestream* stream) {
	if (stream->readIndex + sizeof(double) > stream->dataSize)
		return 0.0;

	uint64_t temp;
	memcpy(&temp, stream->buffer + stream->readIndex, sizeof(double));
	stream->readIndex += sizeof(double);

#ifndef DSLITTLE_ENDIAN 
	temp = endian64(temp);
#endif 

	double value;
	memcpy(&value, &temp, sizeof(double));
	return value;
}

// 写入 double 数组 
CDFErrorCode bytestream_writedoublearray(bytestream* stream, const double* arr, int count) {
	// 先写入数组长度 
	bytestream_writeint(stream, count);

	// 检查缓冲区空间 
	int totalBytes = count * sizeof(double);
	if (checkbufferoverflow(stream, totalBytes) != CDF_SUCCESS)
		return CDF_ERROR_BUFFER_OVERFLOW;

	// 逐个写入元素（带字节序转换）
	for (int i = 0; i < count; i++) {
		bytestream_writedouble(stream, arr[i]);
	}
	return CDF_SUCCESS;
}

// 读取 double 数组 
double* bytestream_readdoublearray(bytestream* stream, int* outCount) {
	// 读取数组长度 
	int count = bytestream_readint(stream);
	if (count <= 0 || stream->readIndex + count * sizeof(double) > stream->dataSize)
		return NULL;

	// 分配内存 
	double* arr = (double*)malloc(count * sizeof(double));
	if (!arr) return NULL;

	// 读取每个元素 
	for (int i = 0; i < count; i++) {
		arr[i] = bytestream_readdouble(stream);
	}

	*outCount = count;
	return arr;
}


// 写入 long64_t（处理字节序）
void bytestream_writelong64(bytestream* stream, long64_t value) {
	if (checkbufferoverflow(stream, sizeof(long64_t)) != CDF_SUCCESS) {
		THROW_EXCEPTION("Buffer overflow in writelong64");
		return;
	}

	// 处理字节序
	long64_t tmp = value;
#ifndef DSLITTLE_ENDIAN 
	tmp = endian64(tmp); // 使用已有的 64 位字节序转换函数 
#endif 

	memcpy(stream->buffer + stream->dataSize, &tmp, sizeof(long64_t));
	stream->dataSize += sizeof(long64_t);
}

// 读取 long64_t（处理字节序）
long64_t bytestream_readlong64(bytestream* stream) {
	if (stream->readIndex + sizeof(long64_t) > stream->dataSize) {
		THROW_EXCEPTION("Insufficient data for readlong64");
		return 0;
	}

	long64_t tmp;
	memcpy(&tmp, stream->buffer + stream->readIndex, sizeof(long64_t));
	stream->readIndex += sizeof(long64_t);

#ifndef DSLITTLE_ENDIAN 
	tmp = endian64(tmp);
#endif 

	return tmp;
}

// 写入 long64_t 数组 
CDFErrorCode bytestream_writelong64array(bytestream* stream, const long64_t* arr, int count) {
	// 写入数组长度（兼容现有规范）
	bytestream_writeint(stream, count);

	// 计算总空间需求（每个元素占 8 字节）
	int totalBytes = count * sizeof(long64_t);
	if (checkbufferoverflow(stream, totalBytes) != CDF_SUCCESS)
		return CDF_ERROR_BUFFER_OVERFLOW;

	// 批量处理字节序
	for (int i = 0; i < count; ++i) {
		long64_t tmp = arr[i];
#ifndef DSLITTLE_ENDIAN 
		tmp = endian64(tmp);
#endif 
		memcpy(stream->buffer + stream->dataSize, &tmp, sizeof(long64_t));
		stream->dataSize += sizeof(long64_t);
	}
	return CDF_SUCCESS;
}

// 读取 long64_t 数组 
long64_t* bytestream_readlong64array(bytestream* stream, int* outCount) {
	// 读取数组长度 
	int count = bytestream_readint(stream);
	if (count <= 0 || stream->readIndex + count * sizeof(long64_t) > stream->dataSize) {
		*outCount = 0;
		return NULL;
	}

	// 分配内存（使用 calloc 避免未初始化风险）
	long64_t* arr = (long64_t*)calloc(count, sizeof(long64_t));
	if (!arr) return NULL;

	// 批量读取并转换字节序
	for (int i = 0; i < count; ++i) {
		long64_t tmp;
		memcpy(&tmp, stream->buffer + stream->readIndex, sizeof(long64_t));
		stream->readIndex += sizeof(long64_t);

#ifndef DSLITTLE_ENDIAN 
		tmp = endian64(tmp);
#endif 

		arr[i] = tmp;
	}

	*outCount = count;
	return arr;
}


// 写入单个uint64_t 
void bytestream_writeuint64(bytestream* stream, uint64_t value) {
	if (stream->readOnly) {
		THROW_EXCEPTION("Attempt to write to read-only stream");
		return;
	}

	// 处理字节序 [2]()
#ifndef DSLITTLE_ENDIAN 
	value = endian64(value);
#endif 

	// 检查缓冲区空间 
	if (checkbufferoverflow(stream, sizeof(uint64_t)) != CDF_SUCCESS) {
		THROW_EXCEPTION("Buffer overflow in writeuint64");
		return;
	}

	// 写入数据（内存拷贝方式） [1]()
	memcpy(stream->buffer + stream->dataSize, &value, sizeof(uint64_t));
	stream->dataSize += sizeof(uint64_t);
}

// 读取单个uint64_t 
uint64_t bytestream_readuint64(bytestream* stream) {
	if (stream->readIndex + sizeof(uint64_t) > stream->dataSize) {
		THROW_EXCEPTION("Insufficient data for readuint64");
		return 0;
	}

	uint64_t value;
	memcpy(&value, stream->buffer + stream->readIndex, sizeof(uint64_t));
	stream->readIndex += sizeof(uint64_t);

	// 还原字节序
#ifndef DSLITTLE_ENDIAN 
	value = endian64(value);
#endif 

	return value;
}

// 写入uint64_t数组 
CDFErrorCode bytestream_writeuint64array(bytestream* stream, const uint64_t* arr, int count) {
	// 先写入数组长度（兼容现有规范）
	bytestream_writeint(stream, count);

	// 检查总空间需求 
	int totalBytes = count * sizeof(uint64_t);
	if (checkbufferoverflow(stream, totalBytes) != CDF_SUCCESS) {
		return CDF_ERROR_BUFFER_OVERFLOW;
	}

	// 批量写入（优化性能）
	for (int i = 0; i < count; ++i) {
		uint64_t tmp = arr[i];
#ifndef DSLITTLE_ENDIAN 
		tmp = endian64(tmp); // 字节序转换 
#endif 
		memcpy(stream->buffer + stream->dataSize, &tmp, sizeof(uint64_t));
		stream->dataSize += sizeof(uint64_t);
	}
	return CDF_SUCCESS;
}

// 读取uint64_t数组 
uint64_t* bytestream_readuint64array(bytestream* stream, int* outCount) {
	// 读取数组长度 
	int count = bytestream_readint(stream);
	if (count <= 0 || stream->readIndex + count * sizeof(uint64_t) > stream->dataSize) {
		*outCount = 0;
		return NULL;
	}

	// 分配内存 
	uint64_t* arr = (uint64_t*)malloc(count * sizeof(uint64_t));
	if (!arr) return NULL;

	// 批量读取 
	for (int i = 0; i < count; ++i) {
		memcpy(&arr[i], stream->buffer + stream->readIndex, sizeof(uint64_t));
		stream->readIndex += sizeof(uint64_t);

#ifndef DSLITTLE_ENDIAN 
		arr[i] = endian64(arr[i]); // 还原字节序
#endif 
	}

	*outCount = count;
	return arr; // 调用者需负责释放内存 
}


// 写入字符串（包含长度）
void bytestream_writestring(bytestream* stream, const char* str) {
	int len = (int)strlen(str);
	bytestream_writeint(stream, len);
	bytestream_append(stream, str, len);
}

// 读取字符串 
char* bytestream_readstring(bytestream* stream) {
	int len = bytestream_readint(stream);
	if (len <= 0 || stream->readIndex + len > stream->dataSize) return NULL;

	char* str = (char*)malloc(len + 1);
	memcpy(str, stream->buffer + stream->readIndex, len);
	str[len] = '\0';
	stream->readIndex += len;
	return str;
}

// 写入字符串数组
CDFErrorCode bytestream_writestringarray(bytestream* stream, const char** arr, int count) {
	// 先写入数组长度 
	bytestream_writeint(stream, count);

	// 遍历字符串数组 
	for (int i = 0; i < count; i++) {
		const char* str = arr[i];
		// 写入字符串 
		bytestream_writestring(stream, str);
	}

	return CDF_SUCCESS;
}

// 读取字符串数组 
char** bytestream_readstringarray(bytestream* stream, int* outCount) {
	// 读取数组长度 
	int count = bytestream_readint(stream);
	if (count <= 0) {
		*outCount = 0;
		return NULL;
	}

	// 分配内存 
	char** arr = (char**)malloc(count * sizeof(char*));
	if (!arr) {
		return NULL;
	}

	// 读取每个字符串 
	for (int i = 0; i < count; i++) {
		arr[i] = bytestream_readstring(stream);
		if (!arr[i]) {
			// 释放已分配的内存 
			for (int j = 0; j < i; j++) {
				free(arr[j]);
			}
			free(arr);
			return NULL;
		}
	}

	*outCount = count;
	return arr;
}

// 示例：序列化结构处理 
void bytestream_startseq(bytestream* stream, int numElements, int minSize) {
	SeqData* newSeq = (SeqData*)malloc(sizeof(SeqData));
	newSeq->numElements = numElements;
	newSeq->minSize = minSize;
	newSeq->previous = stream->seqDataStack;
	stream->seqDataStack = newSeq;
}

void bytestream_endseq(bytestream* stream) {
	if (!stream->seqDataStack) return;

	SeqData* oldSeq = stream->seqDataStack;
	stream->seqDataStack = oldSeq->previous;
	free(oldSeq);
}

// 错误处理示例 
void bytestream_setreadonly(bytestream* stream, bool readOnly) {
	stream->readOnly = readOnly;
}
