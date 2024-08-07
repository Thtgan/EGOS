#include<devices/block/blockDevice.h>

#include<devices/block/blockBuffer.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<print.h>
#include<cstring.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static Result __blockDevice_doProbePartitions(BlockDevice* device, void* buffer);

static Result __blockDevice_partition_readBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n);
static Result __blockDevice_partition_writeBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n);

static BlockDeviceOperation _blockDevice_partition_blockDeviceOperations = {
    .readBlocks     = __blockDevice_partition_readBlocks,
    .writeBlocks    = __blockDevice_partition_writeBlocks
};

Result blockDevice_initStruct(BlockDevice* device, BlockDeviceArgs* args) {
    memory_memset(device, 0, sizeof(BlockDevice));

    cstring_strncpy(device->name, args->name, BLOCK_DEVICE_NAME_LENGTH);

    device->availableBlockNum   = args->availableBlockNum;
    Size bytePerBlockShift      = args->bytePerBlockShift == 0 ? BLOCK_DEVICE_DEFAULT_BLOCK_SIZE_SHIFT : args->bytePerBlockShift;
    device->bytePerBlockShift   = bytePerBlockShift;

    device->flags               = EMPTY_FLAGS;

    device->parent              = args->parent;
    device->childNum = 0;
    singlyLinkedList_initStruct(&device->children);
    singlyLinkedListNode_initStruct(&device->node);
    if (device->parent != NULL) {
        ++device->parent->childNum;
        singlyLinkedList_insertNext(&device->parent->children, &device->node);
    }

    if (args->buffered) {
        BlockBuffer* blockBuffer = memory_allocate(sizeof(BlockBuffer));
        if (blockBuffer == NULL || blockBuffer_initStruct(blockBuffer, BLOCK_BUFFER_DEFAULT_HASH_SIZE, BLOCK_BUFFER_DEFAULT_MAX_BLOCK_NUM, bytePerBlockShift) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        device->blockBuffer     = blockBuffer;
        SET_FLAG_BACK(device->flags, BLOCK_DEVICE_FLAGS_BUFFERED);
    }

    device->operations          = args->operations;
    device->specificInfo        = args->specificInfo;
    hashChainNode_initStruct(&device->hashChainNode);

    return RESULT_SUCCESS;
}

Result blockDevice_readBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n) {
    if (device->operations->readBlocks == NULL || blockIndex >= device->availableBlockNum) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS(device->flags, BLOCK_DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            Block* block = blockBuffer_pop(device->blockBuffer, index);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (TEST_FLAGS_FAIL(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT)) {
                if (device->operations->readBlocks(device, index, block->data, 1) == RESULT_FAIL) { //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }
            }

            memory_memcpy(buffer + (((Index64)i) << device->bytePerBlockShift), block->data, 1 << device->bytePerBlockShift);

            if (blockBuffer_push(device->blockBuffer, index, block) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }

        return RESULT_SUCCESS;
    }

    return device->operations->readBlocks(device, blockIndex, buffer, n);
} 

Result blockDevice_writeBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n) {
    if (TEST_FLAGS(device->flags, BLOCK_DEVICE_FLAGS_READONLY) || device->operations->writeBlocks == NULL || blockIndex >= device->availableBlockNum) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS(device->flags, BLOCK_DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            Block* block = blockBuffer_pop(device->blockBuffer, index);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (index != block->blockIndex && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                if (device->operations->writeBlocks(device, block->blockIndex, block->data, 1) == RESULT_FAIL) {    //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }
            }

            memory_memcpy(block->data, buffer + (((Index64)i) << device->bytePerBlockShift), 1 << device->bytePerBlockShift);
            SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);

            if (blockBuffer_push(device->blockBuffer, index, block) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }

        return RESULT_SUCCESS;
    }

    return device->operations->writeBlocks(device, blockIndex, buffer, n);
}

Result blockDevice_flush(BlockDevice* device) {
    if (TEST_FLAGS(device->flags, BLOCK_DEVICE_FLAGS_BUFFERED)) {
        BlockBuffer* blockBuffer = device->blockBuffer;
        for (int i = 0; i < blockBuffer->blockNum; ++i) {
            Block* block = blockBuffer_pop(device->blockBuffer, INVALID_INDEX);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (block->blockIndex != INVALID_INDEX && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                if (device->operations->writeBlocks(device, block->blockIndex, block->data, 1) == RESULT_FAIL) {    //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }

                CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);
            }

            if (blockBuffer_push(device->blockBuffer, block->blockIndex, block) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }
        return RESULT_SUCCESS;
    }

    return RESULT_SUCCESS;
}

Result blockDevice_probePartitions(BlockDevice* device) {
    void* buffer = memory_allocate(BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);
    if (buffer == NULL) {
        return RESULT_FAIL;
    }

    Result ret = __blockDevice_doProbePartitions(device, buffer);

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

static Result __blockDevice_doProbePartitions(BlockDevice* device, void* buffer) {
    if (blockDevice_readBlocks(device, 0, buffer, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    char nameBuffer[16];
    if (PTR_TO_VALUE(16, buffer + 0x1FE) == 0xAA55) {
        singlyLinkedList_initStruct(&device->children);
        for (int i = 0; i < 4; ++i) {
            __MBRpartitionEntry* entry = (__MBRpartitionEntry*)(buffer + 0x1BE + i * sizeof(__MBRpartitionEntry));
            if (entry->systemID == 0) {
                continue;
            }

            print_sprintf(nameBuffer, "%s-p%d", device->name, i);

            BlockDeviceArgs args = {
                .name               = nameBuffer,
                .availableBlockNum  = entry->sectorNum,
                .bytePerBlockShift  = device->bytePerBlockShift,
                .parent             = device,
                .specificInfo       = entry->sectorBegin,
                .operations         = &_blockDevice_partition_blockDeviceOperations,
                .buffered           = true
            };

            BlockDevice* partitionDevice = memory_allocate(sizeof(BlockDevice));
            if (partitionDevice == NULL || blockDevice_initStruct(partitionDevice, &args) == RESULT_FAIL) {
                continue;
            }
            
            if (entry->driveArttribute == 0x80) {
                SET_FLAG_BACK(partitionDevice->flags, BLOCK_DEVICE_FLAGS_BOOTABLE);
                if (firstBootablePartition == NULL) {
                    firstBootablePartition = partitionDevice;
                }
            }
        }
    }

    return RESULT_SUCCESS;
}

static Result __blockDevice_partition_readBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n) {
    if (blockIndex >= device->availableBlockNum) {
        return RESULT_FAIL;
    }

    return blockDevice_readBlocks(device->parent, (Index64)device->specificInfo + blockIndex, buffer, n);
}

static Result __blockDevice_partition_writeBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n) {
    if (blockIndex >= device->availableBlockNum) {
        return RESULT_FAIL;
    }

    return blockDevice_writeBlocks(device->parent, (Index64)device->specificInfo + blockIndex, buffer, n);
}