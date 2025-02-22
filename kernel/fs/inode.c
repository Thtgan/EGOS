#include<fs/inode.h>

#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/locks/spinlock.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<debug.h>
#include<error.h>

typedef struct __iNode_lookupIterateFuncArg {
    ConstCstring name;
    bool isDirectory;
} __iNode_lookupIterateFuncArg;

static bool __iNode_lookupIterateFunc(iNode* inode, DirectoryEntry* entry, Object arg, void* ret);

fsNode* iNode_lookupDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory) { //TODO: Shouldn't assume node has known its all children
    fsNode* thisNode = inode->fsNode, * ret = NULL;
    DEBUG_ASSERT_SILENT(thisNode->isInodeActive);
    DEBUG_ASSERT_SILENT(thisNode->inodeID == inode->inodeID);
    DEBUG_ASSERT_SILENT(thisNode->type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&inode->lock);

    ret = fsNode_lookup(thisNode, name, isDirectory);    //Refer 'ret' once (If found)
    if (ret != NULL) {
        spinlock_unlock(&inode->lock);
        return ret;
    }

    __iNode_lookupIterateFuncArg arg = (__iNode_lookupIterateFuncArg) {
        .name = name,
        .isDirectory = isDirectory
    };

    iNode_rawIterateDirectoryEntries(inode, __iNode_lookupIterateFunc, (Object)&arg, &ret); //Refer 'ret' once
    ERROR_GOTO_IF_ERROR(0);

    if (ret == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    spinlock_unlock(&inode->lock);
    return ret;
    ERROR_FINAL_BEGIN(0);
    spinlock_unlock(&inode->lock);
    return NULL;
}

void iNode_removeDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory) {
    fsNode* thisNode = inode->fsNode, * found = NULL;
    DEBUG_ASSERT_SILENT(thisNode->isInodeActive);
    DEBUG_ASSERT_SILENT(thisNode->inodeID == inode->inodeID);
    DEBUG_ASSERT_SILENT(thisNode->type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&inode->lock);
    
    found = fsNode_lookup(thisNode, name, isDirectory); //Refer 'found' once
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }
    fsNode_release(found);  //Release 'found' once (from lookup)
    found = NULL;

    iNode_rawRemoveDirectoryEntry(inode, name, isDirectory);
    ERROR_GOTO_IF_ERROR(0);

    spinlock_unlock(&inode->lock);
    return;
    ERROR_FINAL_BEGIN(0);
    if (spinlock_isLocked(&inode->lock)) {
        spinlock_unlock(&inode->lock);
    }

    if (found != NULL) {
        fsNode_release(found);
    }
}

void iNode_genericReadAttr(iNode* inode, iNodeAttribute* attribute) {
    memory_memcpy(attribute, &inode->attribute, sizeof(iNodeAttribute));
}

void iNode_genericWriteAttr(iNode* inode, iNodeAttribute* attribute) {
    memory_memcpy(&inode->attribute, attribute, sizeof(iNodeAttribute));
}

static bool __iNode_lookupIterateFunc(iNode* inode, DirectoryEntry* entry, Object arg, void* ret) {
    fsNode** nodeRet = (fsNode**)ret;
    
    __iNode_lookupIterateFuncArg* lookupArg = (__iNode_lookupIterateFuncArg*)arg;
    DirectoryEntry entryToMatch = (DirectoryEntry) {
        .name = lookupArg->name,
        .type = DIRECTORY_ENTRY_TYPE_ANY,
        .inodeID = DIRECTORY_ENTRY_INDOE_ID_ANY,
        .size = DIRECTORY_ENTRY_SIZE_ANY,
        .mode = DIRECTORY_ENTRY_MODE_ANY,
        .deviceID = DIRECTORY_ENTRY_DEVICE_ID_ANY
    };

    if (!directoryEntry_isMatch(entry, &entryToMatch, lookupArg->isDirectory)) {
        return false;
    }

    *nodeRet = superBlock_getFSnode(inode->superBlock, entry->inodeID); //Refer '*nodeRet' once
    if (*nodeRet == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    return true;
    ERROR_FINAL_BEGIN(0);
    return true;
}