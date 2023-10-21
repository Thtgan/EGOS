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

    Size blockNum = DIVIDE_ROUND_DOWN(size, DEFAULT_BLOCK_SIZE);
    memset(region, 0, blockNum * DEFAULT_BLOCK_SIZE);

    BlockDeviceArgs args = {
        .name               = name,
        .availableBlockNum  = blockNum,
        .bytePerBlockShift  = DEFAULT_BLOCK_SIZE_SHIFT,
        .parent             = NULL,
        .specificInfo       = (Object)region,
        .operations         = &_operations
    };

    return createBlockDevice(&args);    //RAM block device's additional data is its memory region's begin
}

static Result __readBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, Size n) {
    memcpy(buffer, (void*)this->specificInfo + (blockIndex << this->bytePerBlockShift), n << this->bytePerBlockShift);  //Just simple memcpy
    return RESULT_SUCCESS;
}

static Result __writeBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, Size n) {
    memcpy((void*)this->specificInfo + (blockIndex << this->bytePerBlockShift), buffer, n << this->bytePerBlockShift);  //Just simple memcpy
    return RESULT_SUCCESS;
}