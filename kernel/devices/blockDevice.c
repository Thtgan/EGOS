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
#include<lib/errorPosix.h>

void blockDevice_initStruct(BlockDevice* blockDevice, BlockDeviceInitArgs* args) {
    ERROR_THROW_NEW_IF(args->deviceInitArgs.granularity == 0, ERROR_INVALID_ARGUMENT, error_out);

    Device* device = &blockDevice->device;
    device_initStruct(device, &args->deviceInitArgs);

    BlockBuffer* blockBuffer = NULL;
    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        blockBuffer = mm_allocate(sizeof(BlockBuffer));
        ERROR_THROW_NEW_IF(blockBuffer == NULL, ERROR_OUT_OF_MEMORY, error_out);

        blockBuffer_initStruct(blockBuffer, BLOCK_BUFFER_DEFAULT_HASH_SIZE, BLOCK_BUFFER_DEFAULT_MAX_BLOCK_NUM, device->granularity);
        CHECK_ERROR(error_out);

        blockDevice->blockBuffer = blockBuffer;
    }

    return;
error_out:
    if (blockBuffer != NULL) {
        mm_free(blockBuffer);
    }
}

void blockDevice_readBlocks(BlockDevice* blockDevice, Index64 blockIndex, void* buffer, Size n) {
    Device* device = &blockDevice->device;
    DEBUG_ASSERT_SILENT(blockIndex != INVALID_INDEX64);
    ERROR_THROW_NEW_IF(device->operations->readUnits == NULL, ERROR_NOT_SUPPORTED, error_out);   //TODO: Move this to raw call function?

    ERROR_THROW_NEW_IF(blockIndex >= device->capacity, ERROR_INVALID_ARGUMENT, error_out);

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            BlockBufferBlock* block = blockBuffer_pop(blockDevice->blockBuffer, index);
            ERROR_THROW_NEW_IF(block == NULL, ERROR_INVALID_STATE, error_out);

            if (TEST_FLAGS_FAIL(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT)) {
                device_rawReadUnits(device, index, block->data, 1);  //TODO: May be this happens too frequently?
                CHECK_ERROR(error_out);
            }

            memory_memcpy(buffer + ((Index64)i * POWER_2(device->granularity)), block->data, POWER_2(device->granularity));

            blockBuffer_push(blockDevice->blockBuffer, index, block);
            CHECK_ERROR(error_out);
        }

        return;
    }

    device_rawReadUnits(device, blockIndex, buffer, n);
    CHECK_ERROR(error_out);
    return;
error_out:
}

void blockDevice_writeBlocks(BlockDevice* blockDevice, Index64 blockIndex, const void* buffer, Size n) {
    Device* device = &blockDevice->device;
    DEBUG_ASSERT_SILENT(blockIndex != INVALID_INDEX64);
    ERROR_THROW_NEW_IF(TEST_FLAGS(device->flags, DEVICE_FLAGS_READONLY), ERROR_PERMISSION_DENIED, error_out);

    ERROR_THROW_NEW_IF(device->operations->writeUnits == NULL, ERROR_NOT_SUPPORTED, error_out);   //TODO: Move this to raw call function?

    ERROR_THROW_NEW_IF(blockIndex >= device->capacity, ERROR_INVALID_ARGUMENT, error_out);

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            BlockBufferBlock* block = blockBuffer_pop(blockDevice->blockBuffer, index);
            ERROR_THROW_NEW_IF(block == NULL, ERROR_INVALID_STATE, error_out);

            if (index != block->blockIndex && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                device_rawWriteUnits(device, block->blockIndex, block->data, 1); //TODO: May be this happens too frequently?
                CHECK_ERROR(error_out);
            }

            memory_memcpy(block->data, buffer + ((Index64)i * POWER_2(device->granularity)), POWER_2(device->granularity));
            SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);

            blockBuffer_push(blockDevice->blockBuffer, index, block);
            CHECK_ERROR(error_out);
        }

        return;
    }

    device_rawWriteUnits(device, blockIndex, buffer, n);
    CHECK_ERROR(error_out);
    return;
error_out:
}

void blockDevice_flush(BlockDevice* blockDevice) {
    Device* device = &blockDevice->device;
    ERROR_THROW_NEW_IF(device->operations->flush == NULL, ERROR_NOT_SUPPORTED, error_out);   //TODO: Move this to raw call function?

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        BlockBuffer* blockBuffer = blockDevice->blockBuffer;
        for (int i = 0; i < blockBuffer->blockNum; ++i) {
            BlockBufferBlock* block = blockBuffer_pop(blockBuffer, INVALID_INDEX64);
            ERROR_THROW_NEW_IF(block == NULL, ERROR_INVALID_STATE, error_out);

            if (block->blockIndex != INVALID_INDEX64 && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                device_rawWriteUnits(device, block->blockIndex, block->data, 1);
                CHECK_ERROR(error_out); //TODO: May be this happens too frequently?

                CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);
            }

            blockBuffer_push(blockBuffer, block->blockIndex, block);
            CHECK_ERROR(error_out);
        }
    }

    device_rawFlush(device);
    CHECK_ERROR(error_out);
    return;
error_out:
}