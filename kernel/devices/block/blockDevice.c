#include<devices/block/blockDevice.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<print.h>
#include<string.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static HashTable _hashTable;
static SinglyLinkedList _hashChains[16];

static Result __doProbePartitions(BlockDevice* device, void* buffer);

static Result __partitionReadBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n);
static Result __partitionWriteBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n);

static BlockDeviceOperation _partitionOperations = {
    .readBlocks = __partitionReadBlocks,
    .writeBlocks = __partitionWriteBlocks
};

Result initBlockDevice() {
    initHashTable(&_hashTable, 16, _hashChains, LAMBDA(Size, (HashTable* this, Object key) {
        return (Size)key % 13;
    }));

    return RESULT_SUCCESS;
}

BlockDevice* createBlockDevice(BlockDeviceArgs* args) {
    ID deviceID = (Uint16)strhash(args->name, 13, 65536);
    while (hashTableFind(&_hashTable, (Object)deviceID) != NULL) {
        ++deviceID;
    }

    BlockDevice* ret = kMalloc(sizeof(BlockDevice));
    memset(ret, 0, sizeof(BlockDevice));

    strncpy(ret->name, args->name, BLOCK_DEVICE_NAME_LENGTH);

    ret->deviceID           = deviceID;
    ret->availableBlockNum  = args->availableBlockNum;
    ret->bytePerBlockShift  = args->bytePerBlockShift == 0 ? DEFAULT_BLOCK_SIZE_SHIFT : args->bytePerBlockShift;

    ret->flags              = EMPTY_FLAGS;

    ret->parent             = args->parent;
    ret->childNum = 0;
    initSinglyLinkedList(&ret->children);
    initSinglyLinkedListNode(&ret->node);
    if (ret->parent != NULL) {
        ++ret->parent->childNum;
        singlyLinkedListInsertNext(&ret->parent->children, &ret->node);
    }

    ret->operations         = args->operations;
    ret->specificInfo       = args->specificInfo;
    initHashChainNode(&ret->hashChainNode);

    return ret;
}

void releaseBlockDevice(BlockDevice* device) {
    kFree(device);
}

Result registerBlockDevice(BlockDevice* device) {
    return hashTableInsert(&_hashTable, (Object)device->deviceID, &device->hashChainNode);
}

BlockDevice* unregisterBlockDevice(ID deviceID) {
    return HOST_POINTER(hashTableDelete(&_hashTable, (Object)deviceID), BlockDevice, hashChainNode);
}

BlockDevice* getBlockDeviceByName(ConstCstring name) {
    Size deviceID = (Uint16)strhash(name, 13, 65536);
    HashChainNode* node = NULL;
    if ((node = hashTableFind(&_hashTable, (Object)deviceID)) != NULL) {
        return HOST_POINTER(node, BlockDevice, hashChainNode);
    }

    return NULL;
}

BlockDevice* getBlockDeviceByID(ID id) {
    HashChainNode* node = NULL;
    if ((node = hashTableFind(&_hashTable, (Object)id)) != NULL) {
        return HOST_POINTER(node, BlockDevice, hashChainNode);
    }

    return NULL;
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
                .operations         = &_partitionOperations
            };

            BlockDevice* partitionDevice = createBlockDevice(&args);
            if (partitionDevice == NULL) {
                continue;
            }
            
            if (entry->driveArttribute == 0x80) {
                SET_FLAG_BACK(partitionDevice->flags, BLOCK_DEVICE_FLAGS_BOOTABLE);
            }

            registerBlockDevice(partitionDevice);
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