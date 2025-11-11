#include<devices/blockDevice.h>

#include<devices/blockBuffer.h>
#include<devices/device.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<print.h>
#include<cstring.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<error.h>

void blockDevice_initStruct(BlockDevice* blockDevice, BlockDeviceInitArgs* args) {
    if (args->deviceInitArgs.granularity == 0) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    Device* device = &blockDevice->device;
    device_initStruct(device, &args->deviceInitArgs);

    BlockBuffer* blockBuffer = NULL;
    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        blockBuffer = mm_allocate(sizeof(BlockBuffer));
        if (blockBuffer == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        blockBuffer_initStruct(blockBuffer, BLOCK_BUFFER_DEFAULT_HASH_SIZE, BLOCK_BUFFER_DEFAULT_MAX_BLOCK_NUM, device->granularity);
        ERROR_GOTO_IF_ERROR(0);

        blockDevice->blockBuffer = blockBuffer;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        mm_free(blockBuffer);
    }
}

void blockDevice_readBlocks(BlockDevice* blockDevice, Index64 blockIndex, void* buffer, Size n) {
    Device* device = &blockDevice->device;
    DEBUG_ASSERT_SILENT(blockIndex != INVALID_INDEX64);
    if (device->operations->readUnits == NULL) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);   //TODO: Move this to raw call function?
    }

    if (blockIndex >= device->capacity) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            BlockBufferBlock* block = blockBuffer_pop(blockDevice->blockBuffer, index);
            if (block == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            if (TEST_FLAGS_FAIL(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT)) {
                device_rawReadUnits(device, index, block->data, 1);  //TODO: May be this happens too frequently?
                ERROR_GOTO_IF_ERROR(0);
            }

            memory_memcpy(buffer + ((Index64)i * POWER_2(device->granularity)), block->data, POWER_2(device->granularity));

            blockBuffer_push(blockDevice->blockBuffer, index, block);
            ERROR_GOTO_IF_ERROR(0);
        }

        return;
    }

    device_rawReadUnits(device, blockIndex, buffer, n);
    ERROR_GOTO_IF_ERROR(0);
    return;
    ERROR_FINAL_BEGIN(0);
} 

void blockDevice_writeBlocks(BlockDevice* blockDevice, Index64 blockIndex, const void* buffer, Size n) {
    Device* device = &blockDevice->device;
    DEBUG_ASSERT_SILENT(blockIndex != INVALID_INDEX64);
    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_READONLY)) {
        ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
    }

    if (device->operations->writeUnits == NULL) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);   //TODO: Move this to raw call function?
    }

    if (blockIndex >= device->capacity) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            BlockBufferBlock* block = blockBuffer_pop(blockDevice->blockBuffer, index);
            if (block == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            if (index != block->blockIndex && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                device_rawWriteUnits(device, block->blockIndex, block->data, 1); //TODO: May be this happens too frequently?
                ERROR_GOTO_IF_ERROR(0);
            }

            memory_memcpy(block->data, buffer + ((Index64)i * POWER_2(device->granularity)), POWER_2(device->granularity));
            SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);

            blockBuffer_push(blockDevice->blockBuffer, index, block);
            ERROR_GOTO_IF_ERROR(0);
        }

        return;
    }

    device_rawWriteUnits(device, blockIndex, buffer, n);
    ERROR_GOTO_IF_ERROR(0);
    return;
    ERROR_FINAL_BEGIN(0);
}

void blockDevice_flush(BlockDevice* blockDevice) {
    Device* device = &blockDevice->device;
    if (device->operations->flush == NULL) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);   //TODO: Move this to raw call function?
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        BlockBuffer* blockBuffer = blockDevice->blockBuffer;
        for (int i = 0; i < blockBuffer->blockNum; ++i) {
            BlockBufferBlock* block = blockBuffer_pop(blockBuffer, INVALID_INDEX64);
            if (block == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            if (block->blockIndex != INVALID_INDEX64 && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                device_rawWriteUnits(device, block->blockIndex, block->data, 1);
                ERROR_GOTO_IF_ERROR(0); //TODO: May be this happens too frequently?

                CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);
            }

            blockBuffer_push(blockBuffer, block->blockIndex, block);
            ERROR_GOTO_IF_ERROR(0);
        }
    }

    device_rawFlush(device);
    ERROR_GOTO_IF_ERROR(0);
    return;
    ERROR_FINAL_BEGIN(0);
}