#include<devices/block/memoryBlockDevice.h>

#include<devices/block/blockDevice.h>
#include<error.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<cstring.h>
#include<structs/singlyLinkedList.h>

static Result __memoryBlockDevice_readBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, Size n);

static Result __memoryBlockDevice_writeBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, Size n);

static BlockDeviceOperation _memoryBlockDevice_blockDeviceOperations = {
    .readBlocks = __memoryBlockDevice_readBlocks,
    .writeBlocks = __memoryBlockDevice_writeBlocks
};

Result memoryBlockDevice_initStruct(BlockDevice* device, void* region, Size size, ConstCstring name) {
    if (region == NULL) {
        ERROR_CODE_SET(ERROR_CODE_OBJECT_ARGUMENT, ERROR_CODE_STATUS_IS_NULL);
        return RESULT_FAIL;
    }

    Size blockNum = DIVIDE_ROUND_DOWN(size, BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);
    memory_memset(region, 0, blockNum * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);

    BlockDeviceArgs args = {
        .name               = name,
        .availableBlockNum  = blockNum,
        .bytePerBlockShift  = BLOCK_DEVICE_DEFAULT_BLOCK_SIZE_SHIFT,
        .parent             = NULL,
        .specificInfo       = (Object)region,   //RAM block device's additional data is its memory region's begin
        .operations         = &_memoryBlockDevice_blockDeviceOperations
    };

    return blockDevice_initStruct(device, &args);
}

static Result __memoryBlockDevice_readBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, Size n) {
    memory_memcpy(buffer, (void*)this->specificInfo + (blockIndex << this->bytePerBlockShift), n << this->bytePerBlockShift);  //Just simple memcpy
    return RESULT_SUCCESS;
}

static Result __memoryBlockDevice_writeBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, Size n) {
    memory_memcpy((void*)this->specificInfo + (blockIndex << this->bytePerBlockShift), buffer, n << this->bytePerBlockShift);  //Just simple memcpy
    return RESULT_SUCCESS;
}