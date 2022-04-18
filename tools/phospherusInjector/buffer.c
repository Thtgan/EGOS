#include<buffer.h>

#include<malloc.h>
#include<memory.h>
#include<stddef.h>

#define PAGE_SIZE 4096

static size_t _bufferSizes[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};

static size_t _bufferNums[9];
static size_t _freeBufferNums[9];

size_t getTotalBufferNum(BufferSizes size) {
    return _bufferNums[size];
}

size_t getFreeBufferNum(BufferSizes size) {
    return _freeBufferNums[size];
}

void* allocateBuffer(BufferSizes size) {
    void* ret = malloc(_bufferSizes[size]);
    memset(ret, 0, sizeof(_bufferSizes[size]));
    return ret;
}

void releaseBuffer(void* buffer, BufferSizes size) {
    free(buffer);
}
