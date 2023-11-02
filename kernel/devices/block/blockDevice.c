#include<devices/block/blockDevice.h>

#include<devices/block/blockBuffer.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<print.h>
#include<string.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static Result __doProbePartitions(BlockDevice* device, void* buffer);

static Result __partitionReadBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n);
static Result __partitionWriteBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n);

static BlockDeviceOperation _partitionOperations = {
    .readBlocks     = __partitionReadBlocks,
    .writeBlocks    = __partitionWriteBlocks
};

Result initBlockDevice(BlockDevice* device, BlockDeviceArgs* args) {
    memset(device, 0, sizeof(BlockDevice));

    strncpy(device->name, args->name, BLOCK_DEVICE_NAME_LENGTH);

    device->availableBlockNum   = args->availableBlockNum;
    Size bytePerBlockShift      = args->bytePerBlockShift == 0 ? DEFAULT_BLOCK_SIZE_SHIFT : args->bytePerBlockShift;
    device->bytePerBlockShift   = bytePerBlockShift;

    device->flags               = EMPTY_FLAGS;

    device->parent              = args->parent;
    device->childNum = 0;
    initSinglyLinkedList(&device->children);
    initSinglyLinkedListNode(&device->node);
    if (device->parent != NULL) {
        ++device->parent->childNum;
        singlyLinkedListInsertNext(&device->parent->children, &device->node);
    }

    if (args->buffered) {
        BlockBuffer* blockBuffer = kMalloc(sizeof(BlockBuffer));
        if (blockBuffer == NULL || initBlockBuffer(blockBuffer, BLOCK_BUFFER_DEFAULT_HASH_SIZE, BLOCK_BUFFER_DEFAULT_MAX_BLOCK_NUM, bytePerBlockShift) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        device->blockBuffer     = blockBuffer;
        SET_FLAG_BACK(device->flags, BLOCK_DEVICE_FLAGS_BUFFERED);
    }

    device->operations          = args->operations;
    device->specificInfo        = args->specificInfo;
    initHashChainNode(&device->hashChainNode);

    return RESULT_SUCCESS;
}

Result blockDeviceReadBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n) {
    if (device->operations->readBlocks == NULL || blockIndex >= device->availableBlockNum) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS(device->flags, BLOCK_DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            Block* block = blockBufferPop(device->blockBuffer, index);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (TEST_FLAGS_FAIL(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT)) {
                if (device->operations->readBlocks(device, index, block->data, 1) == RESULT_FAIL) { //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }
            }

            memcpy(buffer, block->data, 1 << device->bytePerBlockShift);

            if (blockBufferPush(device->blockBuffer, index, block) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }

        return RESULT_SUCCESS;
    }

    return device->operations->readBlocks(device, blockIndex, buffer, n);
} 

Result blockDeviceWriteBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n) {
    if (TEST_FLAGS(device->flags, BLOCK_DEVICE_FLAGS_READONLY) || device->operations->writeBlocks == NULL || blockIndex >= device->availableBlockNum) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS(device->flags, BLOCK_DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            Block* block = blockBufferPop(device->blockBuffer, index);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (index != block->blockIndex && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                if (device->operations->writeBlocks(device, index, block->data, 1) == RESULT_FAIL) {    //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }
            }

            memcpy(block->data, buffer, 1 << device->bytePerBlockShift);
            SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);

            if (blockBufferPush(device->blockBuffer, index, block)) {
                return RESULT_FAIL;
            }
        }

        return RESULT_SUCCESS;
    }

    return device->operations->writeBlocks(device, blockIndex, buffer, n);
}

Result blockDeviceSynchronize(BlockDevice* device) {
    if (TEST_FLAGS_FAIL(device->flags, BLOCK_DEVICE_FLAGS_BUFFERED)) {
        BlockBuffer* blockBuffer = device->blockBuffer;
        for (int i = 0; i < blockBuffer->blockNum; ++i) {
            Block* block = blockBufferPop(device->blockBuffer, INVALID_INDEX);
            if (block == NULL) {
                return RESULT_FAIL;
            }

            if (block->blockIndex != INVALID_INDEX && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                if (device->operations->writeBlocks(device, block->blockIndex, block->data, 1) == RESULT_FAIL) {    //TODO: May be this happens too frequently?
                    return RESULT_FAIL;
                }

                CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);
            }

            if (blockBufferPush(device->blockBuffer, block->blockIndex, block)) {
                return RESULT_FAIL;
            }
        }

        return RESULT_SUCCESS;
    }

    return RESULT_SUCCESS;
}

Result probePartitions(BlockDevice* device) {
    void* buffer = allocateBuffer(BUFFER_SIZE_512);
    if (buffer == NULL) {
        return RESULT_FAIL;
    }

    Result ret = __doProbePartitions(device, buffer);

    releaseBuffer(buffer, BUFFER_SIZE_512);
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

static Result __doProbePartitions(BlockDevice* device, void* buffer) {
    if (blockDeviceReadBlocks(device, 0, buffer, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    char nameBuffer[16];
    if (PTR_TO_VALUE(16, buffer + 0x1FE) == 0xAA55) {
        initSinglyLinkedList(&device->children);
        for (int i = 0; i < 4; ++i) {
            __MBRpartitionEntry* entry = (__MBRpartitionEntry*)(buffer + 0x1BE + i * sizeof(__MBRpartitionEntry));
            if (entry->systemID == 0) {
                continue;
            }

            sprintf(nameBuffer, "%s-p%d", device->name, i);

            BlockDeviceArgs args = {
                .name               = nameBuffer,
                .availableBlockNum  = entry->sectorNum,
                .bytePerBlockShift  = device->bytePerBlockShift,
                .parent             = device,
                .specificInfo       = entry->sectorBegin,
                .operations         = &_partitionOperations,
                .buffered           = true
            };

            BlockDevice* partitionDevice = kMalloc(sizeof(BlockDevice));
            if (partitionDevice == NULL || initBlockDevice(partitionDevice, &args) == RESULT_FAIL) {
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

static Result __partitionReadBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n) {
    if (blockIndex >= device->availableBlockNum) {
        return RESULT_FAIL;
    }

    return blockDeviceReadBlocks(device->parent, (Index64)device->specificInfo + blockIndex, buffer, n);
}

static Result __partitionWriteBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n) {
    if (blockIndex >= device->availableBlockNum) {
        return RESULT_FAIL;
    }

    return blockDeviceWriteBlocks(device->parent, (Index64)device->specificInfo + blockIndex, buffer, n);
}