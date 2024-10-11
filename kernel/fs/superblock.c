#include<fs/superblock.h>

#include<cstring.h>
#include<fs/fcntl.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsutil.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<structs/linkedList.h>
#include<structs/string.h>

void superBlock_initStruct(SuperBlock* superBlock, SuperBlockInitArgs* args) {
    superBlock->blockDevice     = args->blockDevice;
    superBlock->operations      = args->operations;
    superBlock->rootDirDesc     = args->rootDirDesc;
    superBlock->specificInfo    = args->specificInfo;
    hashTable_initStruct(&superBlock->openedInode, args->openedInodeBucket, args->openedInodeChains, hashTable_defaultHashFunc);
    hashTable_initStruct(&superBlock->mounted, args->mountedBucket, args->mountedChains, hashTable_defaultHashFunc);
    hashTable_initStruct(&superBlock->fsEntryDescs, args->fsEntryDescBucket, args->fsEntryDescChains, hashTable_defaultHashFunc);
}

Result superBlock_getfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** finalSuperBlockOut, fsEntryDesc** descOut) {
    fsEntryDesc* desc = NULL;
    SuperBlock* finalSuperBlock = NULL;
    Result ret = RESULT_SUCCESS;
    if (identifier->name.length == 0 && identifier->parentPath.length == 0) {
        desc = superBlock->rootDirDesc;
        finalSuperBlock = superBlock;
    } else {
        fsEntryIdentifier finalIdentifier;
        if (fsutil_loacateRealIdentifier(superBlock, identifier, &finalSuperBlock, &finalIdentifier) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        Object key = cstring_strhash(finalIdentifier.parentPath.data) + cstring_strhash(finalIdentifier.name.data);
        HashChainNode* found = hashTable_find(&finalSuperBlock->fsEntryDescs, key);

        if (found == NULL) {
            desc = memory_allocate(sizeof(fsEntryDesc));
            if (desc == NULL) {
                return RESULT_ERROR;
            }

            Result seekRes;
            if ((seekRes = fsutil_seekLocalFSentryDesc(finalSuperBlock, &finalIdentifier, &desc)) == RESULT_ERROR) {
                memory_free(desc);
                return RESULT_ERROR;
            }

            fsEntryIdentifier_clearStruct(&finalIdentifier);
            hashTable_insert(&superBlock->fsEntryDescs, key, &desc->descNode);

            if (seekRes == RESULT_FAIL) {
                ret = RESULT_FAIL;
            }
        } else {
            desc = HOST_POINTER(found, fsEntryDesc, descNode);
        }
    }

    ++desc->descReferCnt;
    *descOut = desc;
    *finalSuperBlockOut = finalSuperBlock;

    return ret;
}

Result superBlock_releasefsEntryDesc(SuperBlock* superBlock, fsEntryDesc* entryDesc) {
    if (entryDesc == superBlock->rootDirDesc) {
        return RESULT_SUCCESS;
    }

    fsEntryIdentifier* identifier = &entryDesc->identifier;
    Object key = cstring_strhash(identifier->parentPath.data) + cstring_strhash(identifier->parentPath.data);
    HashChainNode* found = hashTable_find(&superBlock->fsEntryDescs, key);

    if (found == NULL || HOST_POINTER(found, fsEntryDesc, descNode) != entryDesc) {
        return RESULT_ERROR;
    }

    if (--entryDesc->descReferCnt > 0) {
        return RESULT_SUCCESS;
    }

    if (hashTable_delete(&superBlock->fsEntryDescs, key) == NULL) {
        return RESULT_ERROR;
    }

    fsEntryDesc_clearStruct(entryDesc);
    memory_free(entryDesc);

    return RESULT_SUCCESS;
}

Result superBlock_openfsEntry(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntry* entryOut, FCNTLopenFlags flags) {
    SuperBlock* finalSuperBlock = NULL;
    fsEntryDesc* desc = NULL;
    Result getRes;
    if ((getRes = superBlock_getfsEntryDesc(superBlock, identifier, &finalSuperBlock, &desc)) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    Result ret = superBlock_rawOpenfsEntry(finalSuperBlock, entryOut, desc, flags);
    if (ret != RESULT_SUCCESS) {
        superBlock_releasefsEntryDesc(finalSuperBlock, desc);  //TODO: What if release failed?
    }

    return ret;
}

Result superBlock_closefsEntry(fsEntry* entry) {
    SuperBlock* superBlock = entry->iNode->superBlock;
    fsEntryIdentifier* identifier = &entry->desc->identifier;
    SuperBlock* finalSuperBlock = superBlock;
    if (identifier->type != FS_ENTRY_TYPE_DIRECTORY) { //TODO: Move directory update to somewhere else
        fsEntryIdentifier parentDirIdentifier;
        if (fsEntryIdentifier_getParent(identifier, &parentDirIdentifier) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        fsEntryDesc* parentEntryDesc;
        if (superBlock_getfsEntryDesc(superBlock, &parentDirIdentifier, &finalSuperBlock, &parentEntryDesc) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        fsEntryIdentifier_clearStruct(&parentDirIdentifier);

        fsEntry parentEntry;
        if (superBlock_rawOpenfsEntry(finalSuperBlock, &parentEntry, parentEntryDesc, FCNTL_OPEN_DEFAULT_FLAGS) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        if (fsEntry_rawDirUpdateEntry(&parentEntry, identifier, entry->desc) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        if (superBlock_rawClosefsEntry(finalSuperBlock, &parentEntry) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        superBlock_releasefsEntryDesc(finalSuperBlock, parentEntryDesc);
    }

    return superBlock_rawClosefsEntry(finalSuperBlock, entry);
}

Result mount_initStruct(Mount* mount, ConstCstring name, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) {
    if (string_initStruct(&mount->name, name) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    mount->superblock = targetSuperBlock;
    mount->targetDesc = targetDesc;
    linkedListNode_initStruct(&mount->node);

    return RESULT_SUCCESS;
}

void mount_clearStruct(Mount* mount) {
    string_clearStruct(&mount->name);

    memory_memset(mount, 0, sizeof(Mount));
}

Result superBlock_genericMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) { //TODO: Bad memory management
    Mount* mount = memory_allocate(sizeof(Mount));
    if (mount == NULL || mount_initStruct(mount, identifier->name.data, targetSuperBlock, targetDesc) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    Object pathKey = cstring_strhash(identifier->parentPath.data);
    HashChainNode* found = hashTable_find(&superBlock->mounted, pathKey);
    if (found == NULL) {
        DirMountList* list = memory_allocate(sizeof(DirMountList));
        if (list == NULL) {
            return RESULT_ERROR;
        }

        linkedList_initStruct(&list->list);
        hashChainNode_initStruct(&list->node);

        if (hashTable_insert(&superBlock->mounted, pathKey, &list->node) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }
        found = &list->node;
    }

    LinkedList* list = &HOST_POINTER(found, DirMountList, node)->list;
    linkedListNode_insertFront(list, &mount->node);

    return RESULT_SUCCESS;
}

Result superBlock_genericUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier) {
    Object pathKey = cstring_strhash(identifier->parentPath.data);
    HashChainNode* found = hashTable_find(&superBlock->mounted, pathKey);
    if (found == NULL) {
        return RESULT_ERROR;
    }

    DirMountList* list = HOST_POINTER(found, DirMountList, node);
    for (LinkedListNode* node = list->list.next; node != &list->list; node = node->next) {
        Mount* mount = HOST_POINTER(node, Mount, node);
        if (cstring_strcmp(mount->name.data, identifier->name.data) != 0) {
            continue;
        }

        mount_clearStruct(mount);
        linkedListNode_delete(&mount->node);
        memory_free(mount);
        break;
    }

    if (linkedList_isEmpty(&list->list)) {
        hashTable_delete(&superBlock->mounted, pathKey);
        memory_free(list);
    }

    return RESULT_SUCCESS;
}

DirMountList* superBlock_getDirMountList(SuperBlock* superBlock, ConstCstring directoryPath) {
    Object pathKey = cstring_strhash(directoryPath);
    HashChainNode* found = hashTable_find(&superBlock->mounted, pathKey);
    return found == NULL ? NULL : HOST_POINTER(found, DirMountList, node);
}

Result superBlock_stepSearchMount(SuperBlock* superBlock, String* searchPath, Index64* searchPathSplitRet, SuperBlock** newSuperBlockRet, fsEntryDesc** mountedToDescRet) {
    String subSearchPath;
    if (string_initStruct(&subSearchPath, "") != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    Result ret = RESULT_SUCCESS;
    while (true) {
        DirMountList* dirMountList = superBlock_getDirMountList(superBlock, subSearchPath.data);
        char* nextSeperator = cstring_strchr(searchPath->data + subSearchPath.length + 1, FS_PATH_SEPERATOR);
        if (dirMountList != NULL) {
            for (LinkedListNode* node = dirMountList->list.next; node != &dirMountList->list; node = node->next) {
                Mount* mount = HOST_POINTER(node, Mount, node);
                if (cstring_strncmp(mount->name.data, searchPath->data + subSearchPath.length + 1, mount->name.length) != 0) {
                    continue;
                }

                *searchPathSplitRet = ARRAY_POINTER_TO_INDEX(searchPath->data, nextSeperator);
                *newSuperBlockRet = mount->superblock;
                *mountedToDescRet = mount->targetDesc;

                ret = RESULT_CONTINUE;
                break;
            }

            break;
        } else {
            if (nextSeperator == NULL) {
                break;
            }

            string_slice(&subSearchPath, searchPath, 0, ARRAY_POINTER_TO_INDEX(searchPath->data, nextSeperator));
        }
    }

    return ret;
}