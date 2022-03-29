#include<fs/blockDevice/blockDevice.h>

#include<stdio.h>
#include<string.h>
#include<structs/singlyLinkedList.h>

SinglyLinkedList _blockDeviceList;

void initBlockDeviceManager() {
    initSinglyLinkedList(&_blockDeviceList);
}

BlockDevice* registerBlockDevice(BlockDevice* device) {
    SinglyLinkedListNode* node, * last = &_blockDeviceList;
    for (node = singleLinkedListGetNext(&_blockDeviceList); node != &_blockDeviceList; node = singleLinkedListGetNext(node)) {
        char* deviceName = HOST_POINTER(node, BlockDevice, node)->name;

        if (strcmp(device->name, deviceName) == 0) {    //Duplicated name not allowed
            return NULL;
        }

        last = node;
    }
    singleLinkedListInsertBack(last, &device->node);    //Insert to the end of the list
    return device;
}

BlockDevice* getBlockDeviceByName(const char* name) {
    for (
        SinglyLinkedListNode* node = singleLinkedListGetNext(&_blockDeviceList);
        node != &_blockDeviceList;
        node = singleLinkedListGetNext(node)
        ) {
        BlockDevice* device = HOST_POINTER(node, BlockDevice, node);

        if (strcmp(device->name, name) == 0) {
            return NULL;
        }
    }
    return NULL;
}

BlockDevice* getBlockDeviceByType(BlockDevice* begin, BlockDeviceType type) {
    for (SinglyLinkedListNode* node = singleLinkedListGetNext(&_blockDeviceList); 
        node != NULL && node != &_blockDeviceList; 
        node = singleLinkedListGetNext(node)
        ) {
        BlockDevice* device = HOST_POINTER(node, BlockDevice, node);

        if (device->type == type) {
            return device;
        }
    }
    return NULL;
}

void printBlockDevices() {
    printf("Block device list:\n");
    for (SinglyLinkedListNode* node = singleLinkedListGetNext(&_blockDeviceList); node != &_blockDeviceList; node = singleLinkedListGetNext(node)) {
        BlockDevice* device = HOST_POINTER(node, BlockDevice, node);
        printf("Name: %s, Capacity: %u\n", device->name, device->availableBlockNum);
    }
}