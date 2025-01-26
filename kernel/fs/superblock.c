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
#include<error.h>

void superBlock_initStruct(SuperBlock* superBlock, SuperBlockInitArgs* args) {
    superBlock->blockDevice     = args->blockDevice;
    superBlock->operations      = args->operations;
    superBlock->rootDirDesc     = args->rootDirDesc;
    superBlock->specificInfo    = args->specificInfo;
    hashTable_initStruct(&superBlock->openedInode, args->openedInodeBucket, args->openedInodeChains, hashTable_defaultHashFunc);
    hashTable_initStruct(&superBlock->mounted, args->mountedBucket, args->mountedChains, hashTable_defaultHashFunc);
    hashTable_initStruct(&superBlock->fsEntryDescs, args->fsEntryDescBucket, args->fsEntryDescChains, hashTable_defaultHashFunc);
}

void superBlock_getfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** finalSuperBlockOut, fsEntryDesc** descOut) {
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifier));
    
    fsEntryDesc* desc = NULL;
    SuperBlock* finalSuperBlock = NULL;
    fsEntryIdentifier finalIdentifier;

    if (identifier->name.length == 0 && identifier->parentPath.length == 0) {
        desc = superBlock->rootDirDesc;
        finalSuperBlock = superBlock;
    } else {
        fsEntryIdentifier_initStruct(&finalIdentifier, "", false);
        ERROR_GOTO_IF_ERROR(0);
        fsutil_loacateRealIdentifier(superBlock, identifier, &finalSuperBlock, &finalIdentifier);
        ERROR_GOTO_IF_ERROR(0);

        Object key = cstring_strhash(finalIdentifier.parentPath.data) + cstring_strhash(finalIdentifier.name.data);
        HashChainNode* found = hashTable_find(&finalSuperBlock->fsEntryDescs, key);

        if (found == NULL) {
            desc = memory_allocate(sizeof(fsEntryDesc));
            if (desc == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            fsutil_seekLocalFSentryDesc(finalSuperBlock, &finalIdentifier, &desc);
            ERROR_GOTO_IF_ERROR(0);
            hashTable_insert(&superBlock->fsEntryDescs, key, &desc->descNode);
            ERROR_GOTO_IF_ERROR(0);
        } else {
            desc = HOST_POINTER(found, fsEntryDesc, descNode);
        }

        fsEntryIdentifier_clearStruct(&finalIdentifier);
    }

    ++desc->descReferCnt;
    DEBUG_ASSERT_SILENT(descOut);
    *descOut = desc;
    DEBUG_ASSERT_SILENT(finalSuperBlockOut);
    *finalSuperBlockOut = finalSuperBlock;

    return;
    ERROR_FINAL_BEGIN(0);
    if (desc != NULL) {
        memory_free(desc);
    }

    if (fsEntryIdentifier_isActive(&finalIdentifier)) {
        fsEntryIdentifier_clearStruct(&finalIdentifier);
    }
}

void superBlock_releasefsEntryDesc(SuperBlock* superBlock, fsEntryDesc* entryDesc) {
    DEBUG_ASSERT_SILENT(entryDesc);
    if (entryDesc == superBlock->rootDirDesc) {
        return;
    }

    fsEntryIdentifier* identifier = &entryDesc->identifier;
    Object key = cstring_strhash(identifier->parentPath.data) + cstring_strhash(identifier->name.data);
    HashChainNode* found = hashTable_find(&superBlock->fsEntryDescs, key);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    DEBUG_ASSERT_SILENT(HOST_POINTER(found, fsEntryDesc, descNode) == entryDesc);

    if (--entryDesc->descReferCnt > 0) {
        return;
    }

    hashTable_delete(&superBlock->fsEntryDescs, key);
    ERROR_GOTO_IF_ERROR(0);

    fsEntryDesc_clearStruct(entryDesc);
    memory_free(entryDesc);

    return;
    ERROR_FINAL_BEGIN(0);
}

void superBlock_openfsEntry(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntry* entryOut, FCNTLopenFlags flags) {
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifier));

    SuperBlock* finalSuperBlock = NULL;
    fsEntryDesc* desc = NULL;

    bool needCreate = false;
    superBlock_getfsEntryDesc(superBlock, identifier, &finalSuperBlock, &desc);
    ERROR_CHECKPOINT(, 
        (ERROR_ID_NOT_FOUND, {
            needCreate = true;
            ERROR_CLEAR();
            break;
        })
    );

    if (TEST_FLAGS(flags, FCNTL_OPEN_EXCL) && TEST_FLAGS_FAIL(flags, FCNTL_OPEN_CREAT)) {
        ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
    }

    fsEntryIdentifier directoryIdentifier;
    if (needCreate) { //Not exist
        if (TEST_FLAGS_FAIL(flags, FCNTL_OPEN_CREAT)) {
            ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
        }

        fsEntryIdentifier_getParent(identifier, &directoryIdentifier);
        ERROR_GOTO_IF_ERROR(0);

        Directory directory;
        superBlock_openfsEntry(superBlock, &directoryIdentifier, &directory, FCNTL_OPEN_FILE_DEFAULT_FLAGS);
        ERROR_GOTO_IF_ERROR(0);

        fsEntryDesc newDesc;
        superBlock_rawCreate(superBlock, &newDesc, identifier->name.data, identifier->parentPath.data, FS_ENTRY_TYPE_FILE, DEVICE_INVALID_ID, EMPTY_FLAGS);
        ERROR_GOTO_IF_ERROR(0);

        fsEntry_rawDirAddEntry(&directory, &newDesc);
        ERROR_GOTO_IF_ERROR(0);

        fsEntryDesc_clearStruct(&newDesc);
        superBlock_closefsEntry(&directory);
        ERROR_GOTO_IF_ERROR(0);

        superBlock_rawFlush(superBlock);
        ERROR_GOTO_IF_ERROR(0);
        
        superBlock_getfsEntryDesc(superBlock, identifier, &finalSuperBlock, &desc);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        if (TEST_FLAGS(flags, FCNTL_OPEN_DIRECTORY) && desc->type != FS_ENTRY_TYPE_DIRECTORY) {
            ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
        }

        if (TEST_FLAGS(flags, FCNTL_OPEN_EXCL)) {
            ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
        }
    }

    superBlock_rawOpenfsEntry(finalSuperBlock, entryOut, desc, flags);
    ERROR_GOTO_IF_ERROR(0);

    if (TEST_FLAGS(flags, FCNTL_OPEN_TRUNC)) {
        fsEntry_genericResize(entryOut, 0);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
    if (finalSuperBlock != NULL) {
        superBlock_releasefsEntryDesc(finalSuperBlock, desc);
    }

    if (fsEntryIdentifier_isActive(&directoryIdentifier)) {
        fsEntryIdentifier_clearStruct(&directoryIdentifier);
    }
}

void superBlock_closefsEntry(fsEntry* entry) {
    SuperBlock* superBlock = entry->iNode->superBlock;
    fsEntryIdentifier* identifier = &entry->desc->identifier;
    SuperBlock* finalSuperBlock = superBlock;
    if (!identifier->isDirectory) { //TODO: Move directory update to somewhere else
        fsEntryIdentifier parentDirIdentifier;  //TODO: Release this if failed
        fsEntryIdentifier_getParent(identifier, &parentDirIdentifier);
        ERROR_GOTO_IF_ERROR(0);

        fsEntryDesc* parentEntryDesc;
        superBlock_getfsEntryDesc(superBlock, &parentDirIdentifier, &finalSuperBlock, &parentEntryDesc);
        ERROR_GOTO_IF_ERROR(0);

        fsEntryIdentifier_clearStruct(&parentDirIdentifier);

        fsEntry parentEntry;
        superBlock_rawOpenfsEntry(finalSuperBlock, &parentEntry, parentEntryDesc, FCNTL_OPEN_FILE_DEFAULT_FLAGS);
        ERROR_GOTO_IF_ERROR(0);

        fsEntry_rawDirUpdateEntry(&parentEntry, identifier, entry->desc);
        ERROR_GOTO_IF_ERROR(0);

        superBlock_rawClosefsEntry(finalSuperBlock, &parentEntry);
        ERROR_GOTO_IF_ERROR(0);

        superBlock_releasefsEntryDesc(finalSuperBlock, parentEntryDesc);
        ERROR_GOTO_IF_ERROR(0);
    }

    superBlock_rawClosefsEntry(finalSuperBlock, entry);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void mount_initStruct(Mount* mount, ConstCstring name, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) {
    string_initStruct(&mount->name, name);
    ERROR_GOTO_IF_ERROR(0);

    mount->superblock = targetSuperBlock;
    mount->targetDesc = targetDesc;
    linkedListNode_initStruct(&mount->node);

    return;
    ERROR_FINAL_BEGIN(0);
}

void mount_clearStruct(Mount* mount) {
    string_clearStruct(&mount->name);

    memory_memset(mount, 0, sizeof(Mount));
}

void superBlock_genericMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) { //TODO: Bad memory management
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifier));

    Mount* mount = NULL;
    DirMountList* mountList = NULL;
    
    mount = memory_allocate(sizeof(Mount));
    if (mount == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    mount_initStruct(mount, identifier->name.data, targetSuperBlock, targetDesc);
    ERROR_GOTO_IF_ERROR(0);

    Object pathKey = cstring_strhash(identifier->parentPath.data);
    HashChainNode* found = hashTable_find(&superBlock->mounted, pathKey);
    if (found == NULL) {
        mountList = memory_allocate(sizeof(DirMountList));
        if (mountList == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        linkedList_initStruct(&mountList->list);
        hashChainNode_initStruct(&mountList->node);

        hashTable_insert(&superBlock->mounted, pathKey, &mountList->node);
        ERROR_GOTO_IF_ERROR(0);
        
        found = &mountList->node;
    }

    LinkedList* list = &HOST_POINTER(found, DirMountList, node)->list;
    linkedListNode_insertFront(list, &mount->node);

    return;
    ERROR_FINAL_BEGIN(0);
    if (mount == NULL) {
        memory_free(mount);
    }

    if (mountList == NULL) {
        memory_free(list);
    }
}

void superBlock_genericUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier) {
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifier));
    
    Object pathKey = cstring_strhash(identifier->parentPath.data);
    HashChainNode* found = hashTable_find(&superBlock->mounted, pathKey);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
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
        ERROR_ASSERT_NONE();
        memory_free(list);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

DirMountList* superBlock_getDirMountList(SuperBlock* superBlock, ConstCstring directoryPath) {
    Object pathKey = cstring_strhash(directoryPath);
    HashChainNode* found = hashTable_find(&superBlock->mounted, pathKey);
    return found == NULL ? NULL : HOST_POINTER(found, DirMountList, node);
}

bool superBlock_stepSearchMount(SuperBlock* superBlock, String* searchPath, Index64* searchPathSplitRet, SuperBlock** newSuperBlockRet, fsEntryDesc** mountedToDescRet) {
    String subSearchPath;
    string_initStruct(&subSearchPath, "");
    ERROR_GOTO_IF_ERROR(0);

    bool ret = true;
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

                ret = false;
                break;
            }

            break;
        } else {
            if (nextSeperator == NULL) {
                break;
            }

            string_slice(&subSearchPath, searchPath, 0, ARRAY_POINTER_TO_INDEX(searchPath->data, nextSeperator));
            ERROR_GOTO_IF_ERROR(0);
        }
    }

    string_clearStruct(&subSearchPath);

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (STRING_IS_AVAILABLE(&subSearchPath)) {
        string_clearStruct(&subSearchPath);
    }

    return false;
}