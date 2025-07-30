#if !defined(__FS_FSNODE_H)
#define __FS_FSNODE_H

typedef struct fsNodeController fsNodeController;
typedef struct fsNode fsNode;

#include<fs/fscore.h>
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
    ID              vnodeID;
    RefCounter32    refCounter; //fsNode is only refered by: vnode (up to once), children
    fsEntryType     type;
    bool            isVnodeActive;
    Spinlock        lock;
    vNode*          mountOverwrite;
} fsNode;

fsNode* fsNode_create(ConstCstring name, fsEntryType type, fsNode* parent, ID vnodeID);

fsNode* fsNode_lookup(fsNode* node, ConstCstring name, bool isDirectory);

void fsNode_remove(fsNode* node);

void fsNode_release(fsNode* node);

static inline void fsNode_refer(fsNode* node) {    //Only for vnode openging
    REF_COUNTER_REFER(node->refCounter);
}

vNode* fsNode_getVnode(fsNode* node, FScore* fscore);

void fsNode_setMount(fsNode* node, vNode* mountVnode);

void fsNode_getAbsolutePath(fsNode* node, String* pathOut);

void fsNode_transplant(fsNode* node, fsNode* transplantTo);

#endif // __FS_FSNODE_H
 