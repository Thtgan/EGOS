#include<devices/device.h>

#include<devices/blockDevice.h>
#include<devices/charDevice.h>
#include<devices/pseudo.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<structs/RBtree.h>
#include<structs/singlyLinkedList.h>
#include<algorithms.h>
#include<cstring.h>
#include<error.h>

static RBtree _device_majorDeviceTree;

typedef struct {
    MajorDeviceID   major;
    Size            deviceNum;
    RBtree          minorTree;
    RBtreeNode      majorTreeNode;
} __DeviceMajorTreeNode;

static int __device_deviceMajorTreeCmpFunc(RBtreeNode* node1, RBtreeNode* node2);
static int __device_deviceMajorTreeSearchFunc(RBtreeNode* node, Object key);

static int __device_deviceMinorTreeCmpFunc(RBtreeNode* node1, RBtreeNode* node2);
static int __device_deviceMinorTreeSearchFunc(RBtreeNode* node, Object key);

void device_init() {
    RBtree_initStruct(&_device_majorDeviceTree, __device_deviceMajorTreeCmpFunc, __device_deviceMajorTreeSearchFunc);
    pseudoDevice_init();
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

MajorDeviceID device_allocMajor() {
    RBtreeNode* majorFirst = RBtree_getFirst(&_device_majorDeviceTree);
    MajorDeviceID ret = DEVICE_INVALID_ID;
    if (majorFirst == NULL || HOST_POINTER(majorFirst, __DeviceMajorTreeNode, majorTreeNode)->major != 0) {
        ret = 0;
    } else {
        RBtreeNode* current = majorFirst;
        for (MajorDeviceID i = 1; i < POWER_2(32 - DEVICE_ID_MAJOR_SHIFT) - 1; ++i) {
            current = RBtree_getSuccessor(&_device_majorDeviceTree, current);
            if (current == NULL || HOST_POINTER(current, __DeviceMajorTreeNode, majorTreeNode)->major != i) {
                ret = i;
                break;
            }
        }
    }

    if (ret != DEVICE_INVALID_ID) {
        __DeviceMajorTreeNode* newNode = mm_allocate(sizeof(__DeviceMajorTreeNode));
        if (newNode == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        newNode->major = ret;
        newNode->deviceNum = 0;
        RBtree_initStruct(&newNode->minorTree, __device_deviceMinorTreeCmpFunc, __device_deviceMinorTreeSearchFunc);
        RBtreeNode_initStruct(&_device_majorDeviceTree, &newNode->majorTreeNode);

        RBtree_insert(&_device_majorDeviceTree, &newNode->majorTreeNode);
        ERROR_GOTO_IF_ERROR(0);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return DEVICE_INVALID_ID;
}

void device_releaseMajor(MajorDeviceID major) {
    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    if (node->deviceNum > 0) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    RBtreeNode* deleted = RBtree_delete(&_device_majorDeviceTree, (Object)major);
    DEBUG_ASSERT_SILENT(deleted == found);

    mm_free(node);

    return;
    ERROR_FINAL_BEGIN(0);
}

MinorDeviceID device_allocMinor(MajorDeviceID major) {
    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    RBtreeNode* current = RBtree_getFirst(&node->minorTree);
    if (current == NULL) {
        return 0;
    }

    Device* device = HOST_POINTER(current, Device, deviceTreeNode);
    if (DEVICE_MINOR_FROM_ID(device->id) != 0) {
        return 0;
    }

    for (MinorDeviceID i = 1; i < POWER_2(DEVICE_ID_MAJOR_SHIFT) - 1; ++i) {
        current = RBtree_getSuccessor(&node->minorTree, current);
        device = HOST_POINTER(current, Device, deviceTreeNode);
        if (current != NULL && DEVICE_MINOR_FROM_ID(device->id) == i) {
            continue;
        }

        return i;
    }

    ERROR_THROW_NO_GOTO(ERROR_ID_OUT_OF_MEMORY);
    ERROR_FINAL_BEGIN(0);
    return DEVICE_INVALID_ID;
}

void device_initStruct(Device* device, DeviceInitArgs* args) {
    device->id              = args->id;
    cstring_strncpy(device->name, args->name, DEVICE_NAME_MAX_LENGTH);
    device->parent          = args->parent;
    device->granularity     = args->granularity;
    device->capacity        = args->capacity;
    device->flags           = args->flags;

    device->childNum        = 0;
    singlyLinkedList_initStruct(&device->children);
    singlyLinkedListNode_initStruct(&device->childNode);
    RBtreeNode_initStruct(&_device_majorDeviceTree, &device->deviceTreeNode);

    if (device->parent != NULL) {
        ++device->parent->childNum;
        singlyLinkedList_insertNext(&device->parent->children, &device->childNode);
    }

    device->operations      = args->operations;
}

void device_registerDevice(Device* device) {
    MajorDeviceID major = DEVICE_MAJOR_FROM_ID(device->id);
    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    RBtreeNode_initStruct(&node->minorTree, &device->deviceTreeNode);

    RBtree_insert(&node->minorTree, &device->deviceTreeNode);
    ERROR_GOTO_IF_ERROR(0);

    ++node->deviceNum;
    
    return;
    ERROR_FINAL_BEGIN(0);
}

void device_unregisterDevice(DeviceID id) {
    MajorDeviceID major = DEVICE_MAJOR_FROM_ID(id);
    MinorDeviceID minor = DEVICE_MINOR_FROM_ID(id);

    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    RBtreeNode* deleted = RBtree_delete(&node->minorTree, minor);
    if (deleted == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    --node->deviceNum;

    return;
    ERROR_FINAL_BEGIN(0);
}

Device* device_getDevice(DeviceID id) {
    MajorDeviceID major = DEVICE_MAJOR_FROM_ID(id);
    MinorDeviceID minor = DEVICE_MINOR_FROM_ID(id);

    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    found = RBtree_search(&node->minorTree, (Object)minor);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, Device, deviceTreeNode);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

MajorDeviceID device_iterateMajor(MajorDeviceID current) {
    RBtreeNode* next;
    if (current == DEVICE_INVALID_ID) {
        next = RBtree_getFirst(&_device_majorDeviceTree);
    } else {
        RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)current);
        if (found == NULL) {
            return DEVICE_INVALID_ID;
        }

        next = RBtree_getSuccessor(&_device_majorDeviceTree, found);
    }

    if (next == NULL) {
        return DEVICE_INVALID_ID;
    }

    return HOST_POINTER(next, __DeviceMajorTreeNode, majorTreeNode)->major;
}

Device* device_iterateMinor(MajorDeviceID major, MinorDeviceID current) {
    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        return NULL;
    }

    RBtree* minorTree = &HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode)->minorTree;
    RBtreeNode* next;
    if (current == DEVICE_INVALID_ID) {
        next = RBtree_getFirst(minorTree);
    } else {
        RBtreeNode* found = RBtree_search(minorTree, (Object)current);
        if (found == NULL) {
            return NULL;
        }

        next = RBtree_getSuccessor(minorTree, found);
    }

    if (next == NULL) {
        return NULL;
    }

    return HOST_POINTER(next, Device, deviceTreeNode);
}

void device_read(Device* device, Index64 begin, void* buffer, Size n) {
    if (!device_isBlockDevice(device)) {
        CharDevice* charDevice = HOST_POINTER(device, CharDevice, device);
        charDevice_read(charDevice, begin, buffer, n);
        return; //Error passthrough
    }

    BlockDevice* blockDevice = HOST_POINTER(device, BlockDevice, device);
    Size blockSize = POWER_2(device->granularity);
    char tmpBlock[blockSize];

    Index64 currentBlockIndex = begin / blockSize;
    void* currentBuffer = buffer;
    Size remaingByteNum = n;

    if (begin % blockSize != 0) {
        Index64 offsetInBlock = begin % blockSize;
        blockDevice_readBlocks(blockDevice, currentBlockIndex, tmpBlock, 1);
        ERROR_GOTO_IF_ERROR(0);

        Size byteReadN = algorithms_umin64(remaingByteNum, blockSize - offsetInBlock);
        memory_memcpy(buffer, tmpBlock + offsetInBlock, byteReadN);

        currentBlockIndex += 1;
        currentBuffer += byteReadN;
        remaingByteNum -= byteReadN;
    }

    if (remaingByteNum >= blockSize) {
        Size remainingFullBlockNum = remaingByteNum / blockSize;
        blockDevice_readBlocks(blockDevice, currentBlockIndex, currentBuffer, remainingFullBlockNum);
        ERROR_GOTO_IF_ERROR(0);

        currentBlockIndex += remainingFullBlockNum;
        currentBuffer += remainingFullBlockNum * blockSize;
        remaingByteNum %= blockSize;
    }

    if (remaingByteNum != 0) {
        blockDevice_readBlocks(blockDevice, currentBlockIndex, tmpBlock, 1);
        ERROR_GOTO_IF_ERROR(0);
        
        memory_memcpy(currentBuffer, tmpBlock, remaingByteNum);
        
        remaingByteNum = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;
}

void device_write(Device* device, Index64 begin, const void* buffer, Size n) {
    if (!device_isBlockDevice(device)) {
        CharDevice* charDevice = HOST_POINTER(device, CharDevice, device);
        charDevice_write(charDevice, begin, buffer, n);
        return; //Error passthrough
    }

    BlockDevice* blockDevice = HOST_POINTER(device, BlockDevice, device);
    Size blockSize = POWER_2(device->granularity);
    char tmpBlock[blockSize];

    Index64 currentBlockIndex = begin / blockSize;
    const void* currentBuffer = buffer;
    Size remaingByteNum = n;

    if (begin % blockSize != 0) {
        Index64 offsetInBlock = begin % blockSize;
        blockDevice_readBlocks(blockDevice, currentBlockIndex, tmpBlock, 1);
        ERROR_GOTO_IF_ERROR(0);
        
        Size byteWriteN = algorithms_umin64(remaingByteNum, blockSize - offsetInBlock);
        memory_memcpy(tmpBlock + offsetInBlock, buffer, byteWriteN);

        blockDevice_writeBlocks(blockDevice, currentBlockIndex, tmpBlock, 1);
        ERROR_GOTO_IF_ERROR(0);

        currentBlockIndex += 1;
        currentBuffer += byteWriteN;
        remaingByteNum -= byteWriteN;
    }

    if (remaingByteNum >= blockSize) {
        Size remainingFullBlockNum = remaingByteNum / blockSize;
        blockDevice_writeBlocks(blockDevice, currentBlockIndex, currentBuffer, remainingFullBlockNum);
        ERROR_GOTO_IF_ERROR(0);

        currentBlockIndex += remainingFullBlockNum;
        currentBuffer += remainingFullBlockNum * blockSize;
        remaingByteNum %= blockSize;
    }

    if (remaingByteNum != 0) {
        blockDevice_readBlocks(blockDevice, currentBlockIndex, tmpBlock, 1);
        ERROR_GOTO_IF_ERROR(0);
        
        memory_memcpy(tmpBlock, currentBuffer, remaingByteNum);

        blockDevice_writeBlocks(blockDevice, currentBlockIndex, tmpBlock, 1);
        ERROR_GOTO_IF_ERROR(0);
        
        remaingByteNum = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;
}

static int __device_deviceMajorTreeCmpFunc(RBtreeNode* node1, RBtreeNode* node2) {
    return (int)HOST_POINTER(node1, __DeviceMajorTreeNode, majorTreeNode)->major - (int)HOST_POINTER(node2, __DeviceMajorTreeNode, majorTreeNode)->major;
}

static int __device_deviceMajorTreeSearchFunc(RBtreeNode* node, Object key) {
    return (int)HOST_POINTER(node, __DeviceMajorTreeNode, majorTreeNode)->major - (int)key;
}

static int __device_deviceMinorTreeCmpFunc(RBtreeNode* node1, RBtreeNode* node2) {
    return (int)DEVICE_MINOR_FROM_ID(HOST_POINTER(node1, Device, deviceTreeNode)->id) - (int)DEVICE_MINOR_FROM_ID(HOST_POINTER(node2, Device, deviceTreeNode)->id);
}

static int __device_deviceMinorTreeSearchFunc(RBtreeNode* node, Object key) {
    return (int)DEVICE_MINOR_FROM_ID(HOST_POINTER(node, Device, deviceTreeNode)->id) - (int)key;
}