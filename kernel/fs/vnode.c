#include<fs/vnode.h>

#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/locks/spinlock.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<debug.h>
#include<error.h>

typedef struct __vNode_lookupIterateFuncArg {
    ConstCstring name;
    bool isDirectory;
} __vNode_lookupIterateFuncArg;

static bool __vNode_lookupIterateFunc(vNode* vnode, DirectoryEntry* entry, Object arg, void* ret);

fsNode* vNode_lookupDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory) { //TODO: Shouldn't assume node has known its all children
    fsNode* thisNode = vnode->fsNode, * ret = NULL;
    DEBUG_ASSERT_SILENT(thisNode->isVnodeActive);
    DEBUG_ASSERT_SILENT(thisNode->vnodeID == vnode->vnodeID);
    DEBUG_ASSERT_SILENT(thisNode->type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&vnode->lock);

    ret = fsNode_lookup(thisNode, name, isDirectory);    //Refer 'ret' once (If found)
    if (ret != NULL) {
        spinlock_unlock(&vnode->lock);
        return ret;
    }

    __vNode_lookupIterateFuncArg arg = (__vNode_lookupIterateFuncArg) {
        .name = name,
        .isDirectory = isDirectory
    };

    vNode_rawIterateDirectoryEntries(vnode, __vNode_lookupIterateFunc, (Object)&arg, &ret); //Refer 'ret' once
    ERROR_GOTO_IF_ERROR(0);

    if (ret == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    spinlock_unlock(&vnode->lock);
    return ret;
    ERROR_FINAL_BEGIN(0);
    spinlock_unlock(&vnode->lock);
    return NULL;
}

void vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory) {
    fsNode* thisNode = vnode->fsNode, * found = NULL;
    DEBUG_ASSERT_SILENT(thisNode->isVnodeActive);
    DEBUG_ASSERT_SILENT(thisNode->vnodeID == vnode->vnodeID);
    DEBUG_ASSERT_SILENT(thisNode->type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&vnode->lock);
    
    found = fsNode_lookup(thisNode, name, isDirectory); //Refer 'found' once
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }
    fsNode_release(found);  //Release 'found' once (from lookup)
    found = NULL;

    vNode_rawRemoveDirectoryEntry(vnode, name, isDirectory);
    ERROR_GOTO_IF_ERROR(0);

    spinlock_unlock(&vnode->lock);
    return;
    ERROR_FINAL_BEGIN(0);
    if (spinlock_isLocked(&vnode->lock)) {
        spinlock_unlock(&vnode->lock);
    }

    if (found != NULL) {
        fsNode_release(found);
    }
}

void vNode_genericReadAttr(vNode* vnode, vNodeAttribute* attribute) {
    memory_memcpy(attribute, &vnode->attribute, sizeof(vNodeAttribute));
}

void vNode_genericWriteAttr(vNode* vnode, vNodeAttribute* attribute) {
    memory_memcpy(&vnode->attribute, attribute, sizeof(vNodeAttribute));
}

static bool __vNode_lookupIterateFunc(vNode* vnode, DirectoryEntry* entry, Object arg, void* ret) {
    fsNode** nodeRet = (fsNode**)ret;
    
    __vNode_lookupIterateFuncArg* lookupArg = (__vNode_lookupIterateFuncArg*)arg;
    DirectoryEntry entryToMatch = (DirectoryEntry) {
        .name = lookupArg->name,
        .type = DIRECTORY_ENTRY_TYPE_ANY,
        .vnodeID = DIRECTORY_ENTRY_INDOE_ID_ANY,
        .size = DIRECTORY_ENTRY_SIZE_ANY,
        .mode = DIRECTORY_ENTRY_MODE_ANY,
        .deviceID = DIRECTORY_ENTRY_DEVICE_ID_ANY
    };

    if (!directoryEntry_isMatch(entry, &entryToMatch, lookupArg->isDirectory)) {
        return false;
    }

    *nodeRet = fsCore_getFSnode(vnode->fsCore, entry->vnodeID); //Refer '*nodeRet' once
    if (*nodeRet == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    return true;
    ERROR_FINAL_BEGIN(0);
    return true;
}