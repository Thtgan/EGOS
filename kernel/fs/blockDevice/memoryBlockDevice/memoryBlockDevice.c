#include<fs/blockDevice/memoryBlockDevice/memoryBlockDevice.h>

#include<fs/blockDevice/blockDevice.h>
#include<memory/malloc.h>
#include<memory/memory.h>
#include<stddef.h>
#include<string.h>
#include<structs/singlyLinkedList.h>

static size_t _deviceCnt = 0;
static char* _nameTemplate = "memory0";

static void __readBlock(void* additionalData, size_t block, void* buffer);

static void __writeBlock(void* additionalData, size_t block, const void* buffer);

void initMemoryBlockDevice(BlockDevice* device, size_t size) {
    void* region = malloc(size);
    if (region == NULL) {
        return;
    }

    memset(region, 0, size);

    initSinglyLinkedListNode(&device->node);

    device->availableBlockNum = size / BLOCK_SIZE;
    strcpy(device->name, _nameTemplate);    //TODO: Replace this with sprintf
    device->name[6] += _deviceCnt;
    device->type = BLOCK_DEVICE_TYPE_RAM;
    device->additionalData = region;    //RAM block device's additional data is its memory region's begin

    device->readBlock = __readBlock;
    device->writeBlock = __writeBlock;

    ++_deviceCnt;
}

static void __readBlock(void* additionalData, size_t block, void* buffer) {
    memcpy(buffer, additionalData + block * BLOCK_SIZE, BLOCK_SIZE);    //Just simple memcpy
}

static void __writeBlock(void* additionalData, size_t block, const void* buffer) {
    memcpy(additionalData + block * BLOCK_SIZE, buffer, BLOCK_SIZE);    //Just simple memcpy
}
