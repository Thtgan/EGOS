#include<fs/superblock.h>

#include<cstring.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsStructs.h>
#include<fs/fsutil.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<structs/linkedList.h>
#include<structs/string.h>

void superBlock_initStruct(SuperBlock* superBlock, SuperBlockInitArgs* args) {
    superBlock->device          = args->device;
    superBlock->operations      = args->operations;
    superBlock->rootDirDesc     = args->rootDirDesc;
    superBlock->specificInfo    = args->specificInfo;
    hashTable_initStruct(&superBlock->openedInode, args->openedInodeBucket, args->openedInodeChains, hashTable_defaultHashFunc);
    hashTable_initStruct(&superBlock->mounted, args->mountedBucket, args->mountedChains, hashTable_defaultHashFunc);
    hashTable_initStruct(&superBlock->fsEntryDescs, args->fsEntryDescBucket, args->fsEntryDescChains, hashTable_defaultHashFunc);
}

static Result __superBlock_doReadfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntryDesc* descOut, String* fullPath, fsEntryIdentifier* tmpIdentifier);

Result superBlock_readfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntryDesc* descOut) {
    String fullPath;
    fsEntryIdentifier tmpIdentifier;

    Result ret = __superBlock_doReadfsEntryDesc(superBlock, identifier, descOut, &fullPath, &tmpIdentifier);

    if (ret == RESULT_FAIL) {
        string_clearStruct(&fullPath);
    }

    return ret;
}

static Result __superBlock_doReadfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntryDesc* descOut, String* fullPath, fsEntryIdentifier* tmpIdentifier) {
    if (identifier->name.length == 0 && identifier->parentPath.length == 0) {
        memcpy(descOut, &superBlock->rootDirDesc, sizeof(fsEntryDesc));
        return RESULT_SUCCESS;
    }

    fsEntry currentDirectory;
    fsEntryDesc currentEntryDesc, nextEntryDesc;
    memcpy(&currentEntryDesc, superBlock->rootDirDesc, sizeof(fsEntryDesc));

    if (string_append(fullPath, &identifier->parentPath, FS_PATH_SEPERATOR) == RESULT_FAIL || string_concat(fullPath, fullPath, &identifier->name) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    ConstCstring cFullPath = fullPath->data, name = cFullPath;

    while (true) {
        if (*name != FS_PATH_SEPERATOR) {
            return RESULT_FAIL;
        }
        ++name;

        ConstCstring next = name;
        while (*next != FS_PATH_SEPERATOR && *next != '\0') {
            ++next;
        }
        
        if (
            string_initStructN(&tmpIdentifier->name, name, ARRAY_POINTER_TO_INDEX(name, next)) == RESULT_FAIL ||
            string_initStructN(&tmpIdentifier->parentPath, cFullPath, ARRAY_POINTER_TO_INDEX(cFullPath, name) - 1) == RESULT_FAIL
        ) {
            return RESULT_FAIL;
        }
        tmpIdentifier->type = *next == '\0' ? identifier->type : FS_ENTRY_TYPE_DIRECTORY;

        if (superBlock_rawOpenfsEntry(superBlock, &currentDirectory, &currentEntryDesc) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (fsutil_lookupEntryDesc(&currentDirectory, tmpIdentifier, &nextEntryDesc, NULL) != RESULT_SUCCESS) {
            return RESULT_FAIL;
        }

        fsEntryIdentifier_clearStruct(tmpIdentifier);
        if (superBlock->operations->closefsEntry(superBlock, &currentDirectory) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (*next == '\0') {
            memcpy(descOut, &nextEntryDesc, sizeof(fsEntryDesc));
            break;
        }
        
        name = next;
        memcpy(&currentEntryDesc, &nextEntryDesc, sizeof(fsEntryDesc));
    }

    return RESULT_SUCCESS;
}

//TODO: Rework this
fsEntryDesc* superBlock_getfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** superBlockOut) {
    if (identifier->name.length == 0 && identifier->parentPath.length == 0) {
        ++superBlock->rootDirDesc->descReferCnt;
        if (superBlockOut != NULL) {
            *superBlockOut = superBlock;
        }
        return superBlock->rootDirDesc;
    }

    SuperBlock* currentSuperBlock = superBlock;
    fsEntryIdentifier finalIdentifier;
    fsEntryIdentifier_initStructSep(&finalIdentifier, identifier->parentPath.data, identifier->name.data, identifier->type);
    String fullPath, parentPath;
    if (string_append(&fullPath, &identifier->parentPath, FS_PATH_SEPERATOR) == RESULT_FAIL || string_concat(&fullPath, &fullPath, &identifier->name) == RESULT_FAIL) {
        if (superBlockOut != NULL) {
            *superBlockOut = superBlock;
        }
        return NULL;
    }
    string_initStruct(&parentPath, "");

    while (true) {
        Object pathKey = strhash(parentPath.data);
        HashChainNode* found = hashTable_find(&currentSuperBlock->mounted, pathKey);
        if (found != NULL) {
            DirMountList* list = HOST_POINTER(found, DirMountList, node);
            for (LinkedListNode* node = list->list.next; node != &list->list; node = node->next) {
                Mount* mount = HOST_POINTER(node, Mount, node);
                if (strncmp(mount->name.data, fullPath.data + parentPath.length + 1, mount->name.length) == 0) {
                    fsEntryDesc* desc = mount->targetDesc;

                    string_slice(&fullPath, &fullPath, parentPath.length + 1 + mount->name.length, -1);

                    string_clearStruct(&parentPath);
                    if (desc != mount->superblock->rootDirDesc) {
                        string_append(&parentPath, &desc->identifier.parentPath, FS_PATH_SEPERATOR);
                        string_concat(&parentPath, &parentPath, &desc->identifier.name);
                    }
                    string_concat(&fullPath, &parentPath, &fullPath);

                    currentSuperBlock = mount->superblock;
                    fsEntryIdentifier_clearStruct(&finalIdentifier);
                    fsEntryIdentifier_initStruct(&finalIdentifier, fullPath.data, identifier->type);
                }
            }
        }

        char* nextSeperator = strchr(fullPath.data + parentPath.length + 1, FS_PATH_SEPERATOR);
        if (nextSeperator == NULL) {
            break;
        }

        string_slice(&parentPath, &fullPath, 0, ARRAY_POINTER_TO_INDEX(fullPath.data, nextSeperator));
    }

    string_clearStruct(&parentPath);
    string_clearStruct(&fullPath);
    
    Object key = strhash(finalIdentifier.parentPath.data) + strhash(finalIdentifier.parentPath.data);
    HashChainNode* found = hashTable_find(&currentSuperBlock->fsEntryDescs, key);

    fsEntryDesc* ret = NULL;
    if (found == NULL) {
        ret = kMalloc(sizeof(fsEntryDesc));
        if (ret == NULL) {
            if (superBlockOut != NULL) {
                *superBlockOut = NULL;
            }

            fsEntryIdentifier_clearStruct(&finalIdentifier);
            return NULL;
        }

        if (superBlock_readfsEntryDesc(currentSuperBlock, & finalIdentifier, ret) == RESULT_FAIL) {
            kFree(ret);
            if (superBlockOut != NULL) {
                *superBlockOut = NULL;
            }
            fsEntryIdentifier_clearStruct(&finalIdentifier);
            return NULL;
        }

        hashTable_insert(&currentSuperBlock->fsEntryDescs, key, &ret->descNode);
    } else {
        ret = HOST_POINTER(found, fsEntryDesc, descNode);
    }

    ++ret->descReferCnt;

    if (superBlockOut != NULL) {
        *superBlockOut = currentSuperBlock;
    }

    fsEntryIdentifier_clearStruct(&finalIdentifier);
    return ret;
}

Result superBlock_releasefsEntryDesc(SuperBlock* superBlock, fsEntryDesc* entryDesc) {
    bool local = !(entryDesc == superBlock->rootDirDesc || TEST_FLAGS(entryDesc->flags, FS_ENTRY_DESC_FLAGS_MOUNTED));

    Object key;
    if (local) {
        fsEntryIdentifier* identifier = &entryDesc->identifier;
        key = strhash(identifier->parentPath.data) + strhash(identifier->parentPath.data);
        HashChainNode* found = hashTable_find(&superBlock->fsEntryDescs, key);
        if (found == NULL || HOST_POINTER(found, fsEntryDesc, descNode) != entryDesc) {
            return RESULT_FAIL;
        }
    }

    if (--entryDesc->descReferCnt > 0) {
        return RESULT_SUCCESS;
    }

    if (local) {
        if (hashTable_delete(&superBlock->fsEntryDescs, key) == NULL) {
            return RESULT_FAIL;
        }

        kFree(entryDesc);
    }

    return RESULT_SUCCESS;
}

Result superBlock_openfsEntry(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntry* entryOut) {
    SuperBlock* finalSuperBlock;
    fsEntryDesc* desc = superBlock_getfsEntryDesc(superBlock, identifier, &finalSuperBlock);
    if (desc == NULL) {
        return RESULT_FAIL;
    }

    Result ret = superBlock_rawOpenfsEntry(finalSuperBlock, entryOut, desc);
    if (ret == RESULT_FAIL) {
        superBlock_releasefsEntryDesc(superBlock, desc);  //TODO: What if release failed?
    }

    return ret;
}

Result superBlock_closefsEntry(fsEntry* entry) {
    SuperBlock* superBlock = entry->iNode->superBlock;
    fsEntryIdentifier* identifier = &entry->desc->identifier;
    if (identifier->type != FS_ENTRY_TYPE_DIRECTORY) { //TODO: Move directory update to somewhere else
        fsEntryIdentifier parentDirIdentifier;
        if (fsEntryIdentifier_getParent(identifier, &parentDirIdentifier) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        SuperBlock* finalSuperBlock;
        fsEntryDesc* parentEntryDesc = superBlock_getfsEntryDesc(superBlock, &parentDirIdentifier, &finalSuperBlock);
        if (parentEntryDesc == NULL) {
            return RESULT_FAIL;
        }

        fsEntryIdentifier_clearStruct(&parentDirIdentifier);

        fsEntry parentEntry;
        if (superBlock_rawOpenfsEntry(finalSuperBlock, &parentEntry, parentEntryDesc) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (fsEntry_rawDirUpdateEntry(&parentEntry, identifier, entry->desc) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (superBlock_rawClosefsEntry(finalSuperBlock, &parentEntry) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        superBlock_releasefsEntryDesc(finalSuperBlock, parentEntryDesc);
    }

    return superBlock_rawClosefsEntry(superBlock, entry);
}

Result mount_initStruct(Mount* mount, ConstCstring name, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) {
    if (string_initStruct(&mount->name, name) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    mount->superblock = targetSuperBlock;
    mount->targetDesc = targetDesc;
    linkedListNode_initStruct(&mount->node);

    return RESULT_SUCCESS;
}

void mount_clearStruct(Mount* mount) {
    string_clearStruct(&mount->name);

    memset(mount, 0, sizeof(Mount));
}

Result superBlock_genericMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) { //TODO: Bad memory management
    Mount* mount = kMallocSpecific(sizeof(Mount), PHYSICAL_PAGE_ATTRIBUTE_PUBLIC, 16);
    if (mount == NULL || mount_initStruct(mount, identifier->name.data, targetSuperBlock, targetDesc) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Object pathKey = strhash(identifier->parentPath.data);
    HashChainNode* found = hashTable_find(&superBlock->mounted, pathKey);
    if (found == NULL) {
        DirMountList* list = kMallocSpecific(sizeof(DirMountList), PHYSICAL_PAGE_ATTRIBUTE_PUBLIC, 16);
        if (list == NULL) {
            return RESULT_FAIL;
        }

        linkedList_initStruct(&list->list);
        hashChainNode_initStruct(&list->node);

        if (hashTable_insert(&superBlock->mounted, pathKey, &list->node) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
        found = &list->node;
    }

    LinkedList* list = &HOST_POINTER(found, DirMountList, node)->list;
    linkedListNode_insertFront(list, &mount->node);

    return RESULT_SUCCESS;
}

Result superBlock_genericUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier) {
    Object pathKey = strhash(identifier->parentPath.data);
    HashChainNode* found = hashTable_find(&superBlock->mounted, pathKey);
    if (found == NULL) {
        return RESULT_FAIL;
    }

    DirMountList* list = HOST_POINTER(found, DirMountList, node);
    for (LinkedListNode* node = list->list.next; node != &list->list; node = node->next) {
        Mount* mount = HOST_POINTER(node, Mount, node);
        if (strcmp(mount->name.data, identifier->name.data) != 0) {
            continue;
        }

        mount_clearStruct(mount);
        linkedListNode_delete(&mount->node);
        kFree(mount);
        break;
    }

    if (linkedList_isEmpty(&list->list)) {
        hashTable_delete(&superBlock->mounted, pathKey);
        kFree(list);
    }

    return RESULT_SUCCESS;
}
