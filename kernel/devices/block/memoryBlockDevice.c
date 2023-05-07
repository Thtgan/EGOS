#include<devices/block/memoryBlockDevice.h>

#include<devices/block/blockDevice.h>
#include<error.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>
#include<structs/singlyLinkedList.h>

static Result __readBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, size_t n);

static Result __writeBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, size_t n);

static BlockDeviceOperation _operations = {
    .readBlocks = __readBlocks,
    .writeBlocks = __writeBlocks
};

BlockDevice* createMemoryBlockDevice(void* region, size_t size, ConstCstring name) {
    if (region == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_ARGUMENT, ERROR_STATUS_IS_NULL);
        return NULL;
    }

    size_t blockSize = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    memset(region, 0, size);

    return createBlockDevice(name, RAM, blockSize, &_operations, (Object)region);   //RAM block device's additional data is its memory region's begin
}

static Result __readBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, size_t n) {
    memcpy(buffer, (void*)this->additionalData + blockIndex * BLOCK_SIZE, n * BLOCK_SIZE);  //Just simple memcpy
    return RESULT_SUCCESS;
}

static Result __writeBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, size_t n) {
    memcpy((void*)this->additionalData + blockIndex * BLOCK_SIZE, buffer, n * BLOCK_SIZE);  //Just simple memcpy
    return RESULT_SUCCESS;
}