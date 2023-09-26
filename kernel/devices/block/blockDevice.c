#include<devices/block/blockDevice.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static HashTable _hashTable;
static SinglyLinkedList _hashChains[16];

Result initBlockDevice() {
    initHashTable(&_hashTable, 16, _hashChains, LAMBDA(Size, (HashTable* this, Object key) {
        return (Size)key % 13;
    }));

    return RESULT_SUCCESS;
}

BlockDevice* createBlockDevice(ConstCstring name, Size availableBlockNum, BlockDeviceOperation* operations, Object handle) {
    ID deviceID = (Uint16)strhash(name, 13, 65536);
    while (hashTableFind(&_hashTable, (Object)deviceID) != NULL) {
        ++deviceID;
    }

    BlockDevice* ret = kMalloc(sizeof(BlockDevice));
    memset(ret, 0, sizeof(BlockDevice));

    strcpy(ret->name, name);

    ret->deviceID = deviceID;
    ret->availableBlockNum = availableBlockNum;
    ret->operations = operations;
    ret->handle = handle;
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