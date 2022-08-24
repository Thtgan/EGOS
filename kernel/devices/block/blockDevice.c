#include<devices/block/blockDevice.h>

#include<kit/types.h>
#include<memory/memory.h>
#include<memory/virtualMalloc.h>
#include<stdio.h>
#include<string.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static HashTable _hashTable;

static size_t __hashFunc(THIS_ARG_APPEND(HashTable, Object key)) {
    return (size_t)key % this->hashSize;
}

void initBlockDeviceManager() {
    initHashTable(&_hashTable, 16, __hashFunc);
}

BlockDevice* createBlockDevice(const char* name, size_t availableBlockNum, BlockDeviceOperation* operations, Object additionalData) {ID deviceID = strhash(name, 13, 65536);
    Object obj;
    if (hashTableFind(&_hashTable, (Object)deviceID, &obj)) {
        return NULL;
    }

    BlockDevice* ret = vMalloc(sizeof(BlockDevice));
    memset(ret, 0, sizeof(BlockDevice));

    initSinglyLinkedListNode(&ret->node);

    strcpy(ret->name, name);
    ret->deviceID = deviceID;
    ret->availableBlockNum = availableBlockNum;
    ret->operations = operations;
    ret->additionalData = additionalData;

    return ret;
}

void deleteBlockDevice(BlockDevice* device) {
    vFree(device);
}

BlockDevice* registerBlockDevice(BlockDevice* device) {
    size_t deviceID = strhash(device->name, 13, 65536);
    Object obj;
    if (hashTableFind(&_hashTable, (Object)deviceID, &obj)) {
        return NULL;
    }

    hashTableInsert(&_hashTable, (Object)deviceID, (Object)device);

    return device;
}

BlockDevice* unregisterBlockDeviceByName(const char* name) {
    size_t deviceID = strhash(name, 13, 65536);
    Object obj;
    hashTableDelete(&_hashTable, (Object)deviceID, &obj);

    return NULL;
}

BlockDevice* getBlockDeviceByName(const char* name) {
    size_t deviceID = strhash(name, 13, 65536);
    Object obj;
    hashTableDelete(&_hashTable, (Object)deviceID, &obj);

    return (BlockDevice*)obj;
}

BlockDevice* getBlockDeviceByID(ID id) {
    Object obj;
    hashTableDelete(&_hashTable, (Object)id, &obj);

    return (BlockDevice*)obj;
}