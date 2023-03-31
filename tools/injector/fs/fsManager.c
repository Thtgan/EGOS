#include<fs/fsManager.h>

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

bool registerDeviceFS(FileSystem* fs) {
    if (hashTableFind(&_hashTable, (Object)fs->device) != NULL) {
        return false;
    }

    initHashChainNode(&fs->managerNode);
    hashTableInsert(&_hashTable, (Object)fs->device, &fs->managerNode);

    return true;
}

FileSystem* unregisterDeviceFS(ID device) {
    if (hashTableFind(&_hashTable, (Object)device) == NULL) {
        return NULL;
    }

    return HOST_POINTER(hashTableDelete(&_hashTable, (Object)device), FileSystem, managerNode);
}

FileSystem* getDeviceFS(ID device) {
    HashChainNode* found = hashTableDelete(&_hashTable, (Object)device);
    if (found == NULL) {
        return NULL;
    }

    return HOST_POINTER(found, FileSystem, managerNode);
}