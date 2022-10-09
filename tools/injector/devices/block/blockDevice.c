#include<devices/block/blockDevice.h>

#include<kit/types.h>
#include<malloc.h>
#include<memory.h>
#include<stdio.h>
#include<string.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

static HashTable _hashTable;

static size_t __hashFunc(THIS_ARG_APPEND(HashTable, Object key));

static size_t __strhash(const char* str, size_t p, size_t mod);

void initBlockDeviceManager() {
    initHashTable(&_hashTable, 16, __hashFunc);
}

BlockDevice* createBlockDevice(const char* name, size_t availableBlockNum, BlockDeviceOperation* operations, Object additionalData) {
    ID deviceID = __strhash(name, 13, 65536);
    Object obj;
    if (hashTableFind(&_hashTable, (Object)deviceID, &obj)) {
        return (BlockDevice*)obj;
    }

    BlockDevice* ret = malloc(sizeof(BlockDevice));
    memset(ret, 0, sizeof(BlockDevice));

    strcpy(ret->name, name);
    ret->deviceID = deviceID;
    ret->availableBlockNum = availableBlockNum;
    ret->operations = operations;
    ret->additionalData = additionalData;

    return ret;
}

void deleteBlockDevice(BlockDevice* device) {
    free(device);
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
    size_t deviceID = __strhash(name, 13, 65536);
    Object obj;
    hashTableDelete(&_hashTable, (Object)deviceID, &obj);

    return (BlockDevice*)obj;
}

BlockDevice* getBlockDeviceByName(const char* name) {
    size_t deviceID = __strhash(name, 13, 65536);
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

static size_t __strhash(const char* str, size_t p, size_t mod) {
    size_t pp = 1, ret = 0;

    for (int i = 0; str[i] != '\0'; ++i) {
        ret = (ret + str[i] * pp) % mod;
        pp = (pp * p) % mod;
    }

    return ret;
}