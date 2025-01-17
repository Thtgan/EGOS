#include<devices/device.h>

#include<cstring.h>
#include<devices/pseudo.h>
#include<kernel.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/RBtree.h>
#include<structs/singlyLinkedList.h>

RBtree _device_majorDeviceTree;

typedef struct {
    MajorDeviceID   major;
    Size            deviceNum;
    RBtree          minorTree;
    RBtreeNode      majorTreeNode;
} __DeviceMajorTreeNode;

int __device_deviceMajorTreeCmpFunc(RBtreeNode* node1, RBtreeNode* node2);
int __device_deviceMajorTreeSearchFunc(RBtreeNode* node, Object key);

int __device_deviceMinorTreeCmpFunc(RBtreeNode* node1, RBtreeNode* node2);
int __device_deviceMinorTreeSearchFunc(RBtreeNode* node, Object key);

Result* device_init() {
    RBtree_initStruct(&_device_majorDeviceTree, __device_deviceMajorTreeCmpFunc, __device_deviceMajorTreeSearchFunc);
    if (pseudoDevice_init() != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN);  //TODO: Temporary solution
    }

    ERROR_RETURN_OK();
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
        __DeviceMajorTreeNode* newNode = memory_allocate(sizeof(__DeviceMajorTreeNode));
        newNode->major = ret;
        newNode->deviceNum = 0;
        RBtree_initStruct(&newNode->minorTree, __device_deviceMinorTreeCmpFunc, __device_deviceMinorTreeSearchFunc);
        RBtreeNode_initStruct(&_device_majorDeviceTree, &newNode->majorTreeNode);

        RBtree_insert(&_device_majorDeviceTree, &newNode->majorTreeNode);
    }

    return ret;
}

OldResult device_releaseMajor(MajorDeviceID major) {
    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        return RESULT_ERROR;
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    if (node->deviceNum > 0 || RBtree_delete(&_device_majorDeviceTree, (Object)major) != found) {
        return RESULT_ERROR;
    }

    memory_free(node);

    return RESULT_SUCCESS;
}

MinorDeviceID device_allocMinor(MajorDeviceID major) {
    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        return 0;
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    RBtreeNode* current = RBtree_getFirst(&node->minorTree);
    if (current == 0) {
        return 0;
    }

    Device* device = HOST_POINTER(current, Device, deviceTreeNode);
    if (DEVICE_MINOR_FROM_ID(device->id) != 0) {
        return 0;
    }

    MajorDeviceID last = 0;
    for (MinorDeviceID i = 1; i < POWER_2(DEVICE_ID_MAJOR_SHIFT) - 1; ++i) {
        current = RBtree_getSuccessor(&node->minorTree, current);
        device = HOST_POINTER(current, Device, deviceTreeNode);
        if (current != NULL && DEVICE_MINOR_FROM_ID(device->id) == i) {
            continue;
        }

        return i;
    }

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

    device->specificInfo    = args->specificInfo;
}

OldResult device_registerDevice(Device* device) {
    MajorDeviceID major = DEVICE_MAJOR_FROM_ID(device->id);
    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        return RESULT_ERROR;
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    RBtreeNode_initStruct(&node->minorTree, &device->deviceTreeNode);
    OldResult res = RBtree_insert(&node->minorTree, &device->deviceTreeNode);
    if (res == RESULT_SUCCESS) {
        ++node->deviceNum;
    }
    return res;
}

OldResult device_unregisterDevice(DeviceID id) {
    MajorDeviceID major = DEVICE_MAJOR_FROM_ID(id);
    MinorDeviceID minor = DEVICE_MINOR_FROM_ID(id);

    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        return RESULT_ERROR;
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    OldResult res = (RBtree_delete(&node->minorTree, minor) != NULL);
    if (res == RESULT_SUCCESS) {
        --node->deviceNum;
    }

    return res;
}

Device* device_getDevice(DeviceID id) {
    MajorDeviceID major = DEVICE_MAJOR_FROM_ID(id);
    MinorDeviceID minor = DEVICE_MINOR_FROM_ID(id);

    RBtreeNode* found = RBtree_search(&_device_majorDeviceTree, (Object)major);
    if (found == NULL) {
        return NULL;
    }

    __DeviceMajorTreeNode* node = HOST_POINTER(found, __DeviceMajorTreeNode, majorTreeNode);
    found = RBtree_search(&node->minorTree, (Object)minor);
    if (found == NULL) {
        return NULL;
    }

    return HOST_POINTER(found, Device, deviceTreeNode);
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

int __device_deviceMajorTreeCmpFunc(RBtreeNode* node1, RBtreeNode* node2) {
    return (int)HOST_POINTER(node1, __DeviceMajorTreeNode, majorTreeNode)->major - (int)HOST_POINTER(node2, __DeviceMajorTreeNode, majorTreeNode)->major;
}

int __device_deviceMajorTreeSearchFunc(RBtreeNode* node, Object key) {
    return (int)HOST_POINTER(node, __DeviceMajorTreeNode, majorTreeNode)->major - (int)key;
}

int __device_deviceMinorTreeCmpFunc(RBtreeNode* node1, RBtreeNode* node2) {
    return (int)DEVICE_MINOR_FROM_ID(HOST_POINTER(node1, Device, deviceTreeNode)->id) - (int)DEVICE_MINOR_FROM_ID(HOST_POINTER(node2, Device, deviceTreeNode)->id);
}

int __device_deviceMinorTreeSearchFunc(RBtreeNode* node, Object key) {
    return (int)DEVICE_MINOR_FROM_ID(HOST_POINTER(node, Device, deviceTreeNode)->id) - (int)key;
}