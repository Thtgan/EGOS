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

fsNode* iNode_lookupDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory) {
    fsNode* thisNode = inode->fsNode, * ret = NULL;
    DEBUG_ASSERT_SILENT(thisNode->isInodeActive);
    DEBUG_ASSERT_SILENT(thisNode->inodeID == inode->inodeID);
    DEBUG_ASSERT_SILENT(thisNode->type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&inode->lock);

    ret = fsNode_lookup(thisNode, name, isDirectory);    //Refer 'ret' once
    if (ret != NULL) {
        spinlock_unlock(&inode->lock);
        return ret;
    }

    ret = iNode_rawLookupDirectoryEntry(inode, name, isDirectory);
    ERROR_GOTO_IF_ERROR(0);

    if (ret != NULL) {
        DEBUG_ASSERT_SILENT(fsNode_isAlone(ret));
        fsNode_refer(ret);  //Refer 'ret' once
    }

    spinlock_unlock(&inode->lock);
    return ret;
    ERROR_FINAL_BEGIN(0);
    if (spinlock_isLocked(&inode->lock)) {
        spinlock_unlock(&inode->lock);
    }

    if (ret != NULL) {
        fsNode_release(ret);
    }
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