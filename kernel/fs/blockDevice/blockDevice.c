#include<fs/blockDevice/blockDevice.h>

#include<memory/memory.h>
#include<memory/virtualMalloc.h>
#include<stdio.h>
#include<string.h>
#include<structs/singlyLinkedList.h>

static size_t _registeredBlockDeviceNum;
static SinglyLinkedList _blockDeviceList;

void initBlockDeviceManager() {
    _registeredBlockDeviceNum = 0;
    initSinglyLinkedList(&_blockDeviceList);
}

BlockDevice* createBlockDevice(const char* name, size_t availableBlockNum, BlockDeviceType type) {
    for (SinglyLinkedListNode* node = singlyLinkedListGetNext(&_blockDeviceList); node != &_blockDeviceList; node = singlyLinkedListGetNext(node)) {
        BlockDevice* device = HOST_POINTER(node, BlockDevice, node);
        if (strcmp(device->name, name) == 0) {  //Duplicated name not allowed
            return NULL;
        }
    }

    BlockDevice* ret = vMalloc(sizeof(BlockDevice));
    memset(ret, 0, sizeof(BlockDevice));

    initSinglyLinkedListNode(&ret->node);

    strcpy(ret->name, name);
    ret->availableBlockNum = availableBlockNum;
    ret->type = type;

    return ret;
}

void deleteBlockDevice(BlockDevice* device) {
    vFree(device);
}

BlockDevice* registerBlockDevice(BlockDevice* device) {
    SinglyLinkedListNode* node, * last = &_blockDeviceList;
    for (node = singlyLinkedListGetNext(&_blockDeviceList); node != &_blockDeviceList; node = singlyLinkedListGetNext(node)) {
        char* deviceName = HOST_POINTER(node, BlockDevice, node)->name;

        if (strcmp(device->name, deviceName) == 0) {    //Duplicated name not allowed
            return NULL;
        }

        last = node;
    }
    singlyLinkedListInsertNext(last, &device->node);    //Insert to the end of the list
    ++_registeredBlockDeviceNum;
    return device;
}

BlockDevice* unregisterBlockDeviceByName(const char* name) {
    SinglyLinkedListNode* node, * last = &_blockDeviceList;
    for (node = singlyLinkedListGetNext(&_blockDeviceList); node != &_blockDeviceList; node = singlyLinkedListGetNext(node)) {
        BlockDevice* device = HOST_POINTER(node, BlockDevice, node);

        if (strcmp(device->name, name) == 0) {      //Name is unique, so can be used to identify device
            singlyLinkedListDeleteNext(last);       //Remove from list
            --_registeredBlockDeviceNum;
            return device;
        }

        last = node;
    }

    return NULL;
}

BlockDevice* getBlockDeviceByName(const char* name) {
    for (
        SinglyLinkedListNode* node = singlyLinkedListGetNext(&_blockDeviceList);
        node != &_blockDeviceList;
        node = singlyLinkedListGetNext(node)
        ) {
        BlockDevice* device = HOST_POINTER(node, BlockDevice, node);

        if (strcmp(device->name, name) == 0) {
            return device;
        }
    }
    return NULL;
}

BlockDevice* getBlockDeviceByType(BlockDevice* begin, BlockDeviceType type) {
    for (SinglyLinkedListNode* node = singlyLinkedListGetNext(&_blockDeviceList); 
        node != NULL && node != &_blockDeviceList; 
        node = singlyLinkedListGetNext(node)
        ) {
        BlockDevice* device = HOST_POINTER(node, BlockDevice, node);

        if (device->type == type) {
            return device;
        }
    }
    return NULL;
}

void printBlockDevices() {
    printf("%u dlock device registered:\n", _registeredBlockDeviceNum);
    for (SinglyLinkedListNode* node = singlyLinkedListGetNext(&_blockDeviceList); node != &_blockDeviceList; node = singlyLinkedListGetNext(node)) {
        BlockDevice* device = HOST_POINTER(node, BlockDevice, node);
        printf("Name: %s, Capacity: %u\n", device->name, device->availableBlockNum);
    }
}