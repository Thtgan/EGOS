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

void vNode_addDirectoryEntry(vNode* vnode, ConstCstring name, fsEntryType type, vNodeAttribute* attr, ID deviceID) {
    fsNode* fsnode = vnode->fsNode, * found = NULL;
    DEBUG_ASSERT_SILENT(fsnode->vnode == vnode);
    DEBUG_ASSERT_SILENT(fsnode->type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&vnode->lock);
    
    Index64 physicalPosition = vNode_rawAddDirectoryEntry(vnode, name, type, attr, deviceID);
    ERROR_GOTO_IF_ERROR(0);
    
    fsnode_create(name, type, fsnode, physicalPosition);
    ERROR_GOTO_IF_ERROR(0);
    
    spinlock_unlock(&vnode->lock);
    
    return;
    ERROR_FINAL_BEGIN(0);
    if (spinlock_isLocked(&vnode->lock)) {
        spinlock_unlock(&vnode->lock);
    }
}

void vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory) {
    fsNode* fsnode = vnode->fsNode, * found = NULL;
    DEBUG_ASSERT_SILENT(fsnode->vnode == vnode);
    DEBUG_ASSERT_SILENT(fsnode->type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&vnode->lock);
    
    fsnode_forgetDirectoryEntry(fsnode, name, isDirectory);

    vNode_rawRemoveDirectoryEntry(vnode, name, isDirectory);
    ERROR_GOTO_IF_ERROR(0);
    
    spinlock_unlock(&vnode->lock);
    
    return;
    ERROR_FINAL_BEGIN(0);
    if (spinlock_isLocked(&vnode->lock)) {
        spinlock_unlock(&vnode->lock);
    }
}

void vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName) {
    spinlock_lock(&vnode->lock);
    
    vNode_rawRenameDirectoryEntry(vnode, entry, moveTo, newName);
    ERROR_GOTO_IF_ERROR(0);
    
    fsnode_move(entry, moveTo->fsNode, newName);
    ERROR_GOTO_IF_ERROR(0);
    
    spinlock_unlock(&vnode->lock);

    return;
    ERROR_FINAL_BEGIN(0);
    if (spinlock_isLocked(&vnode->lock)) {
        spinlock_unlock(&vnode->lock);
    }
}

void vNode_genericReadAttr(vNode* vnode, vNodeAttribute* attribute) {
    memory_memcpy(attribute, &vnode->attribute, sizeof(vNodeAttribute));
}

void vNode_genericWriteAttr(vNode* vnode, vNodeAttribute* attribute) {
    memory_memcpy(&vnode->attribute, attribute, sizeof(vNodeAttribute));
}
