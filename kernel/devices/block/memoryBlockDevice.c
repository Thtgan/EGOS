#include<devices/block/memoryBlockDevice.h>

#include<devices/block/blockDevice.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/virtualMalloc.h>
#include<stdbool.h>
#include<stddef.h>
#include<string.h>
#include<structs/singlyLinkedList.h>

static bool _created = false;

static void __readBlocks(THIS_ARG_APPEND(BlockDevice, block_index_t blockIndex, void* buffer, size_t n));

static void __writeBlocks(THIS_ARG_APPEND(BlockDevice, block_index_t blockIndex, const void* buffer, size_t n));

static BlockDeviceOperation _operations = {
    .readBlocks = __readBlocks,
    .writeBlocks = __writeBlocks
};

BlockDevice* createMemoryBlockDevice(size_t size) {
    if (_created) {
        return NULL;
    }
    _created = true;

    size_t blockSize = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    void* region = vMalloc(blockSize * BLOCK_SIZE);
    if (region == NULL) {
        return NULL;
    }

    memset(region, 0, size);

    return createBlockDevice("memory", blockSize, &_operations, (Object)region); //RAM block device's additional data is its memory region's begin
}

void deleteMemoryBlockDevice(BlockDevice* blockDevice) {
    vFree((void*)blockDevice->additionalData);
    _created = false;
}

static void __readBlocks(THIS_ARG_APPEND(BlockDevice, block_index_t blockIndex, void* buffer, size_t n)) {
    memcpy(buffer, (void*)this->additionalData + blockIndex * BLOCK_SIZE, n * BLOCK_SIZE);  //Just simple memcpy
}

static void __writeBlocks(THIS_ARG_APPEND(BlockDevice, block_index_t blockIndex, const void* buffer, size_t n)) {
    memcpy((void*)this->additionalData + blockIndex * BLOCK_SIZE, buffer, n * BLOCK_SIZE);  //Just simple memcpy
}