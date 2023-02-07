#include<devices/block/blockDevice.h>

#include<devices/block/blockDeviceTypes.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static HashTable _hashTable;

static size_t __hashFunc(THIS_ARG_APPEND(HashTable, Object key));

void initBlockDeviceManager() {
    initHashTable(&_hashTable, 16, __hashFunc);
}

BlockDevice* createBlockDevice(const char* name, BlockDeviceType type, size_t availableBlockNum, BlockDeviceOperation* operations, Object additionalData) {
    ID deviceID = strhash(name, 13, 65536);
    Object obj;
    if (hashTableFind(&_hashTable, (Object)deviceID, &obj)) {
        return (BlockDevice*)obj;
    }

    BlockDevice* ret = kMalloc(sizeof(BlockDevice), MEMORY_TYPE_NORMAL);
    memset(ret, 0, sizeof(BlockDevice));

    strcpy(ret->name, name);

    ret->type = type;
    ret->deviceID = deviceID;
    ret->availableBlockNum = availableBlockNum;
    ret->operations = operations;
    ret->additionalData = additionalData;

    return ret;
}

void deleteBlockDevice(BlockDevice* device) {
    kFree(device);
}

BlockDevice* registerBlockDevice(BlockDevice* device) {
    Object obj;
    if (hashTableFind(&_hashTable, (Object)device->deviceID, &obj)) {
        return device;
    }

    hashTableInsert(&_hashTable, (Object)device->deviceID, (Object)device);

    return device;
}

BlockDevice* unregisterBlockDeviceByName(const char* name) {
    size_t deviceID = strhash(name, 13, 65536);
    Object obj;
    hashTableDelete(&_hashTable, (Object)deviceID, &obj);

    return (BlockDevice*)obj;
}

BlockDevice* getBlockDeviceByName(const char* name) {
    size_t deviceID = strhash(name, 13, 65536);
    Object obj;
    if (!hashTableFind(&_hashTable, (Object)deviceID, &obj)) {
        return NULL;
    }

    return (BlockDevice*)obj;
}

BlockDevice* getBlockDeviceByID(ID id) {
    Object obj;
    if (!hashTableFind(&_hashTable, (Object)id, &obj)) {
        return NULL;
    }

    return (BlockDevice*)obj;
}

static size_t __hashFunc(THIS_ARG_APPEND(HashTable, Object key)) {
    return (size_t)key % this->hashSize;
}