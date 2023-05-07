#include<fs/fsManager.h>

#include<error.h>
#include<fs/fileSystem.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/hashTable.h>

static HashTable _hashTable;
static SinglyLinkedList _hashChains[32];

void initFSManager() {
    initHashTable(&_hashTable, 32, _hashChains, LAMBDA(size_t, (HashTable* this, Object key) {
        return (size_t)key % 31;
    }));
}

Result registerDeviceFS(FileSystem* fs) {
    if (hashTableFind(&_hashTable, (Object)fs->device) != NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
        return RESULT_FAIL;
    }

    initHashChainNode(&fs->managerNode);
    if (hashTableInsert(&_hashTable, (Object)fs->device, &fs->managerNode) == RESULT_FAIL) {
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

FileSystem* unregisterDeviceFS(ID device) {
    if (hashTableFind(&_hashTable, (Object)device) == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
        return NULL;
    }

    return HOST_POINTER(hashTableDelete(&_hashTable, (Object)device), FileSystem, managerNode);
}

FileSystem* getDeviceFS(ID device) {
    HashChainNode* found = hashTableFind(&_hashTable, (Object)device);
    if (found == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
        return NULL;
    }

    return HOST_POINTER(found, FileSystem, managerNode);
}