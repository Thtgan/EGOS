#include<devices/block/memoryBlockDevice.h>

#include<devices/block/blockDevice.h>
#include<devices/block/blockDeviceTypes.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>
#include<structs/singlyLinkedList.h>

static bool _created = false;

static void __readBlocks(BlockDevice* this, block_index_t blockIndex, void* buffer, size_t n);

static void __writeBlocks(BlockDevice* this, block_index_t blockIndex, const void* buffer, size_t n);

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

    void* region = kMalloc(blockSize * BLOCK_SIZE, MEMORY_TYPE_NORMAL);
    if (region == NULL) {
        return NULL;
    }

    memset(region, 0, size);

    return createBlockDevice("memory", RAM, blockSize, &_operations, (Object)region); //RAM block device's additional data is its memory region's begin
}

void deleteMemoryBlockDevice(BlockDevice* blockDevice) {
    kFree((void*)blockDevice->additionalData);
    _created = false;
}

static void __readBlocks(BlockDevice* this, block_index_t blockIndex, void* buffer, size_t n) {
    memcpy(buffer, (void*)this->additionalData + blockIndex * BLOCK_SIZE, n * BLOCK_SIZE);  //Just simple memcpy
}

static void __writeBlocks(BlockDevice* this, block_index_t blockIndex, const void* buffer, size_t n) {
    memcpy((void*)this->additionalData + blockIndex * BLOCK_SIZE, buffer, n * BLOCK_SIZE);  //Just simple memcpy
}