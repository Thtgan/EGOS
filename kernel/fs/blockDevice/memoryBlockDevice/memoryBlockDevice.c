#include<fs/blockDevice/memoryBlockDevice/memoryBlockDevice.h>

#include<fs/blockDevice/blockDevice.h>
#include<memory/malloc.h>
#include<memory/memory.h>
#include<stddef.h>
#include<string.h>
#include<structs/singlyLinkedList.h>

static size_t _deviceCnt = 0;
static char* _nameTemplate = "memory0";

static void __readBlocks(void* additionalData, size_t blockIndex, void* buffer, size_t n);

static void __writeBlocks(void* additionalData, size_t blockIndex, const void* buffer, size_t n);

BlockDevice* createMemoryBlockDevice(size_t size) {
    size_t blockSize = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    void* region = malloc(blockSize * BLOCK_SIZE);
    if (region == NULL) {
        return NULL;
    }

    memset(region, 0, size);

    BlockDevice* device = createBlockDevice(_nameTemplate, blockSize, BLOCK_DEVICE_TYPE_RAM);
    _nameTemplate[6] += _deviceCnt; //TODO: Replace this with sprintf

    device->additionalData = region;    //RAM block device's additional data is its memory region's begin
    device->readBlocks = __readBlocks;
    device->writeBlocks = __writeBlocks;

    ++_deviceCnt;
}

void deleteMemoryBlockDevice(BlockDevice* blockDevice) {
    free(blockDevice->additionalData);
}

static void __readBlocks(void* additionalData, block_index_t blockIndex, void* buffer, size_t n) {
    memcpy(buffer, additionalData + blockIndex * BLOCK_SIZE, n * BLOCK_SIZE);    //Just simple memcpy
}

static void __writeBlocks(void* additionalData, block_index_t blockIndex, const void* buffer, size_t n) {
    memcpy(additionalData + blockIndex * BLOCK_SIZE, buffer, n * BLOCK_SIZE);    //Just simple memcpy
}