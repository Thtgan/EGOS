#include<devices/block/memoryBlockDevice.h>

#include<devices/block/blockDevice.h>
#include<error.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>
#include<structs/singlyLinkedList.h>

static Result __readBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, Size n);

static Result __writeBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, Size n);

static BlockDeviceOperation _operations = {
    .readBlocks = __readBlocks,
    .writeBlocks = __writeBlocks
};

BlockDevice* createMemoryBlockDevice(void* region, Size size, ConstCstring name) {
    if (region == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_ARGUMENT, ERROR_STATUS_IS_NULL);
        return NULL;
    }

    Size blockSize = ALIGN_UP(size, BLOCK_SIZE);
    memset(region, 0, size);

    return createBlockDevice(name, blockSize, &_operations, (Object)region);   //RAM block device's additional data is its memory region's begin
}

static Result __readBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, Size n) {
    memcpy(buffer, (void*)this->handle + blockIndex * BLOCK_SIZE, n * BLOCK_SIZE);  //Just simple memcpy
    return RESULT_SUCCESS;
}

static Result __writeBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, Size n) {
    memcpy((void*)this->handle + blockIndex * BLOCK_SIZE, buffer, n * BLOCK_SIZE);  //Just simple memcpy
    return RESULT_SUCCESS;
}