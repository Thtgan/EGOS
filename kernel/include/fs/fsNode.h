#if !defined(__FS_FSNODE_H)
#define __FS_FSNODE_H

typedef struct fsNodeController fsNodeController;
typedef struct fsNode fsNode;

#include<fs/superblock.h>
#include<kit/types.h>
#include<multitask/locks/spinlock.h>
#include<structs/linkedList.h>
#include<structs/refCounter.h>
#include<structs/singlyLinkedList.h>
#include<structs/string.h>
#include<structs/hashTable.h>

typedef struct fsNode {
    String          name;
    LinkedList      children;
    LinkedListNode  childNode;
    fsNode*         parent;
    ID              inodeID;
    RefCounter      refCounter; //fsNode is only refered by: inode (up to once), children
    fsEntryType     type;
    bool            isInodeActive;
    Spinlock        lock;
    iNode*          mountOverwrite;
} fsNode;

fsNode* fsNode_create(ConstCstring name, fsEntryType type, fsNode* parent);

fsNode* fsNode_lookup(fsNode* node, ConstCstring name, bool isDirectory);

void fsNode_remove(fsNode* node);

void fsNode_release(fsNode* node);

static inline void fsNode_refer(fsNode* node) {    //Only for inode openging
    refCounter_refer(&node->refCounter);
}

ID fsNode_getInodeID(fsNode* node, SuperBlock* superBlock);

iNode* fsNode_getInode(fsNode* node, SuperBlock* superBlock);

void fsNode_setMount(fsNode* node, iNode* mountInode);

void fsNode_getAbsolutePath(fsNode* node, String* pathOut);

void fsNode_transplant(fsNode* node, fsNode* transplantTo);

#endif // __FS_FSNODE_H
 