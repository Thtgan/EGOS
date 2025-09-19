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

void vNode_addDirectoryEntry(vNode* vnode, DirectoryEntry* entry, FSnodeAttribute* attr) {
    fsNode* node = vnode->fsNode, * found = NULL;
    DEBUG_ASSERT_SILENT(node->vnode == vnode);
    DEBUG_ASSERT_SILENT(node->entry.type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&vnode->lock);
    
    Index64 pointsTo = vNode_rawAddDirectoryEntry(vnode, entry, attr);
    ERROR_GOTO_IF_ERROR(0);

    if (FSNODE_GET_DIRFSNODE(node)->dirPart.childrenNum != FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM) {  //If node has read children, append new node dynamically
        fsnode_create(entry, attr, node);
        ERROR_GOTO_IF_ERROR(0);
    }
    
    spinlock_unlock(&vnode->lock);
    
    return;
    ERROR_FINAL_BEGIN(0);
    if (spinlock_isLocked(&vnode->lock)) {
        spinlock_unlock(&vnode->lock);
    }
}

void vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory) {
    fsNode* node = vnode->fsNode, * found = NULL;
    DEBUG_ASSERT_SILENT(node->vnode == vnode);
    DEBUG_ASSERT_SILENT(node->entry.type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&vnode->lock);
    
    if (FSNODE_GET_DIRFSNODE(node)->dirPart.childrenNum != FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM) {
        fsnode_forgetDirectoryEntry(node, name, isDirectory);
    }

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
