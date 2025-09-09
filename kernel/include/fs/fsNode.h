#if !defined(__FS_FSNODE_H)
#define __FS_FSNODE_H

typedef struct fsNode fsNode;
typedef struct fsNodeDirPart fsNodeDirPart;
typedef struct DirFSnode DirFSnode;

#include<fs/fscore.h>
#include<kit/types.h>
#include<kit/util.h>
#include<multitask/locks/spinlock.h>
#include<structs/linkedList.h>
#include<structs/refCounter.h>
#include<structs/singlyLinkedList.h>
#include<structs/string.h>
#include<structs/hashTable.h>

typedef struct fsNode {
    String              name;
    fsEntryType         type;

    RefCounter32        refCounter; //How many instances requires this node to work

    LinkedListNode      childNode;
    HashChainNode       childHashNode;
    fsNode*             parent;

    Index64             physicalPosition;
    vNode*              vnode;
    vNode*              mount;

    RefCounter32        vNodeRefCounter;    //How many time is vnode referred
    
    Spinlock            lock;
} fsNode;

typedef struct fsNodeDirPart {
    LinkedList          children;
    HashTable           childrenHash;
#define FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM    (Uint32)-1
    Uint32              childrenNum;
    RefCounter32        livingChildNum; //Contained by refCounter
#define FSNODE_DIR_PART_CHILDREN_HASH_CHAIN_SIZE    11
    SinglyLinkedList    childrenHashChains[FSNODE_DIR_PART_CHILDREN_HASH_CHAIN_SIZE];
} fsNodeDirPart;

typedef struct DirFSnode {
    fsNode          node;
    fsNodeDirPart   dirPart;
} DirFSnode;

#define FSNODE_GET_DIRFSNODE(__NODE)    HOST_POINTER(__NODE, DirFSnode, node)

DEBUG_ASSERT_COMPILE(IS_POWER_2(sizeof(DirFSnode)));

fsNode* fsnode_create(ConstCstring name, fsEntryType type, fsNode* parent, Index64 physicalPosition);

// void fsnode_release(fsNode* node);

void fsnode_refer(fsNode* node);

bool fsnode_derefer(fsNode* node);

fsNode* fsnode_lookup(fsNode* node, ConstCstring name, bool isDirectory, bool autoRead);

void fsnode_setVnode(fsNode* node, vNode* vnode);

void fsnode_setMount(fsNode* node, vNode* mountVnode);

void fsnode_requestVnode(FScore* fscore, fsNode* node);

bool fsnode_releaseVnode(FScore* fscore, fsNode* node);

void fsnode_readDirectoryEntries(fsNode* node);

void fsnode_forgetAllDirectoryEntries(fsNode* node);

void fsnode_forgetDirectoryEntry(fsNode* node, ConstCstring name, bool isDirectory);

void fsnode_getAbsolutePath(fsNode* node, String* pathOut);

void fsnode_move(fsNode* node, fsNode* moveTo, ConstCstring newName);

#endif // __FS_FSNODE_H