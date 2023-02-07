#include<devices/block/blockDevice.h>

#include<devices/block/blockDeviceTypes.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static HashTable _hashTable;
static SinglyLinkedList _hashChains[17];

static size_t __hashFunc(THIS_ARG_APPEND(HashTable, Object key));

void initBlockDeviceManager() {
    initHashTable(&_hashTable, 17, _hashChains, __hashFunc);
}

BlockDevice* createBlockDevice(ConstCstring name, BlockDeviceType type, size_t availableBlockNum, BlockDeviceOperation* operations, Object additionalData) {
    ID deviceID = strhash(name, 13, 65536);
    HashChainNode* node = NULL;
    if ((node = hashTableFind(&_hashTable, (Object)deviceID)) != NULL) {
        return HOST_POINTER(node, BlockDevice, hashChainNode);
    }

    BlockDevice* ret = kMalloc(sizeof(BlockDevice), MEMORY_TYPE_NORMAL);
    memset(ret, 0, sizeof(BlockDevice));

    strcpy(ret->name, name);

    ret->type = type;
    ret->deviceID = deviceID;
    ret->availableBlockNum = availableBlockNum;
    ret->operations = operations;
    ret->additionalData = additionalData;
    initHashChainNode(&ret->hashChainNode);

    return ret;
}

void releaseBlockDevice(BlockDevice* device) {
    kFree(device);
}

bool registerBlockDevice(BlockDevice* device) {
    if (hashTableFind(&_hashTable, (Object)device->deviceID) != NULL) {
        return false;
    }

    hashTableInsert(&_hashTable, (Object)device->deviceID, &device->hashChainNode);

    return true;
}

BlockDevice* unregisterBlockDeviceByName(ConstCstring name) {
    size_t deviceID = strhash(name, 13, 65536);
    return HOST_POINTER(hashTableDelete(&_hashTable, (Object)deviceID), BlockDevice, hashChainNode);
}

BlockDevice* getBlockDeviceByName(ConstCstring name) {
    size_t deviceID = strhash(name, 13, 65536);
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

static size_t __hashFunc(THIS_ARG_APPEND(HashTable, Object key)) {
    return (size_t)key % this->hashSize;
}