#include<devices/block/blockDevice.h>

#include<devices/block/blockBuffer.h>
#include<devices/device.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<print.h>
#include<cstring.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static Result __blockDevice_doProbePartitions(BlockDevice* device, void* buffer);

static Result __blockDevice_partition_read(Device* device, Index64 index, void* buffer, Size n);
static Result __blockDevice_partition_write(Device* device, Index64 index, const void* buffer, Size n);
static Result __blockDevice_partition_flush(Device* device);

static DeviceOperations _blockDevice_partition_deviceOperations = {
    .read   = __blockDevice_partition_read,
    .write  = __blockDevice_partition_write,
    .flush  = __blockDevice_partition_flush
};

Result blockDevice_initStruct(BlockDevice* blockDevice, BlockDeviceInitArgs* args) {
    if (args->deviceInitArgs.granularity == 0) {
        return RESULT_FAIL;
    }

    Device* device = &blockDevice->device;
    device_initStruct(device, &args->deviceInitArgs);
    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        BlockBuffer* blockBuffer = memory_allocate(sizeof(BlockBuffer));
        if (blockBuffer == NULL || blockBuffer_initStruct(blockBuffer, BLOCK_BUFFER_DEFAULT_HASH_SIZE, BLOCK_BUFFER_DEFAULT_MAX_BLOCK_NUM, device->granularity) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        blockDevice->blockBuffer = blockBuffer;
    }

    return RESULT_SUCCESS;
}

Result blockDevice_readBlocks(BlockDevice* blockDevice, Index64 blockIndex, void* buffer, Size n) {
    Device* device = &blockDevice->device;
    if (device->operations->read == NULL || blockIndex >= device->capacity) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            Block* block = blockBuffer_pop(blockDevice->blockBuffer, index);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (TEST_FLAGS_FAIL(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT)) {
                // DEBUG_MARK_PRINT("MARK %s %X\n", device->device.name, index);
                if (device_rawRead(device, index, block->data, 1) == RESULT_FAIL) { //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }
            }

            memory_memcpy(buffer + ((Index64)i * POWER_2(device->granularity)), block->data, POWER_2(device->granularity));

            if (blockBuffer_push(blockDevice->blockBuffer, index, block) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }

        return RESULT_SUCCESS;
    }

    return device_rawRead(device, blockIndex, buffer, n);
} 

Result blockDevice_writeBlocks(BlockDevice* blockDevice, Index64 blockIndex, const void* buffer, Size n) {
    Device* device = &blockDevice->device;
    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_READONLY) || device->operations->write == NULL || blockIndex >= device->capacity) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            Block* block = blockBuffer_pop(blockDevice->blockBuffer, index);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (index != block->blockIndex && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                if (device_rawWrite(device, block->blockIndex, block->data, 1) == RESULT_FAIL) {    //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }
            }

            memory_memcpy(block->data, buffer + ((Index64)i * POWER_2(device->granularity)), POWER_2(device->granularity));
            SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);

            if (blockBuffer_push(blockDevice->blockBuffer, index, block) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }

        return RESULT_SUCCESS;
    }

    return device_rawWrite(device, blockIndex, buffer, n);
}

Result blockDevice_flush(BlockDevice* blockDevice) {
    Device* device = &blockDevice->device;
    if (device->operations->flush == NULL) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        BlockBuffer* blockBuffer = blockDevice->blockBuffer;
        for (int i = 0; i < blockBuffer->blockNum; ++i) {
            Block* block = blockBuffer_pop(blockBuffer, INVALID_INDEX);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (block->blockIndex != INVALID_INDEX && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                if (device_rawWrite(device, block->blockIndex, block->data, 1) == RESULT_FAIL) {    //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }

                CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);
            }

            if (blockBuffer_push(blockBuffer, block->blockIndex, block) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }
    }

    return device_rawFlush(device);
}

Result blockDevice_probePartitions(BlockDevice* blopckDevice) {
    void* buffer = memory_allocate(BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);
    if (buffer == NULL) {
        return RESULT_FAIL;
    }

    Result ret = __blockDevice_doProbePartitions(blopckDevice, buffer);

    memory_free(buffer);
    return ret;
}

typedef struct {
    Uint8   driveArttribute;
    Uint8   beginHead;
    Uint8   beginSector     : 6;
    Uint16  beginCylinder   : 10;
    Uint8   systemID;
    Uint8   endHead;
    Uint8   endSector       : 6;
    Uint16  endCylinder     : 10;
    Uint32  sectorBegin;
    Uint32  sectorNum;
} __attribute__((packed)) __MBRpartitionEntry;

BlockDevice* firstBootablePartition;    //TODO: Ugly, figure out a method to know which device we are booting from

static Result __blockDevice_doProbePartitions(BlockDevice* blockDevice, void* buffer) {
    if (blockDevice_readBlocks(blockDevice, 0, buffer, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Device* device = &blockDevice->device;
    char nameBuffer[DEVICE_NAME_MAX_LENGTH + 1];
    if (PTR_TO_VALUE(16, buffer + 0x1FE) == 0xAA55) {
        for (int i = 0; i < 4; ++i) {
            __MBRpartitionEntry* entry = (__MBRpartitionEntry*)(buffer + 0x1BE + i * sizeof(__MBRpartitionEntry));
            if (entry->systemID == 0) {
                continue;
            }

            print_sprintf(nameBuffer, "%s-p%d", device->name, i);    //TODO: sprintf not working correctly

            MajorDeviceID major = DEVICE_MAJOR_FROM_ID(device->id);
            MinorDeviceID minor = device_allocMinor(major);
            if (minor == DEVICE_INVALID_ID) {
                return RESULT_FAIL;
            }

            BlockDeviceInitArgs args = {
                .deviceInitArgs     = (DeviceInitArgs) {
                    .id             = DEVICE_BUILD_ID(major, minor),
                    .name           = nameBuffer,
                    .parent         = device,
                    .granularity    = device->granularity,
                    .capacity       = entry->sectorNum,
                    .flags          = CLEAR_FLAG(device->flags, DEVICE_FLAGS_BUFFERED),
                    .operations     = &_blockDevice_partition_deviceOperations,
                    .specificInfo   = entry->sectorBegin
                },
            };

            BlockDevice* partitionDevice = memory_allocate(sizeof(BlockDevice));
            if (partitionDevice == NULL || blockDevice_initStruct(partitionDevice, &args) == RESULT_FAIL) {
                continue;
            }
            
            if (firstBootablePartition == NULL) {
                // DEBUG_MARK_PRINT("MARK %X\n", entry->sectorBegin);
                firstBootablePartition = partitionDevice;
            }
        }
    }

    return RESULT_SUCCESS;
}

static Result __blockDevice_partition_read(Device* device, Index64 index, void* buffer, Size n) {
    if (index >= device->capacity) {
        return RESULT_FAIL;
    }
    // DEBUG_MARK_PRINT("MARK %lX %lX\n", (Index64)device->device.specificInfo, blockIndex);

    // if (blockIndex > 0x100) {
    //     debug_dump_stack((void*)readRegister_RBP_64(), INFINITE);
    //     debug_blowup("TEST\n");
    // }

    return blockDevice_readBlocks(HOST_POINTER(device->parent, BlockDevice, device), (Index64)device->specificInfo + index, buffer, n);
}

static Result __blockDevice_partition_write(Device* device, Index64 index, const void* buffer, Size n) {
    if (index >= device->capacity) {
        return RESULT_FAIL;
    }

    return blockDevice_writeBlocks(HOST_POINTER(device->parent, BlockDevice, device), (Index64)device->specificInfo + index, buffer, n);
}

static Result __blockDevice_partition_flush(Device* device) {
    return blockDevice_flush(HOST_POINTER(device->parent, BlockDevice, device));
}