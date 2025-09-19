#include<fs/fsNode.h>

#include<fs/directoryEntry.h>
#include<fs/path.h>
#include<fs/vnode.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<multitask/locks/spinlock.h>
#include<structs/linkedList.h>
#include<structs/refCounter.h>
#include<structs/string.h>
#include<cstring.h>
#include<debug.h>
#include<error.h>

static void __fsnode_initStruct(fsNode* node, DirectoryEntry* entry, FSnodeAttribute* attribute, fsNode* parent);

static void __fsnode_release(fsNode* node);

static inline bool __fsnode_isReadyToRelease(fsNode* node) {
    return REF_COUNTER_CHECK(node->refCounter, 0);
}

static inline Object __fsnode_nameHash(ConstCstring name, bool isDirectory) {
    return cstring_strhash(name) ^ VAL_LEFT_SHIFT((Uint64)isDirectory, 63);
}

static void __fsnode_doGetAbsolutePath(fsNode* node, String* pathOut);

static void __fsnodeDirPart_initStruct(fsNodeDirPart* part);

static void __dirFSnode_addLivingChild(DirFSnode* dirNode);

static void __dirFSnode_removeLivingChild(DirFSnode* dirNode);

static void __dirfsNode_addChildNode(DirFSnode* dirNode, fsNode* child);

static fsNode* __dirfsNode_removeChildNode(DirFSnode* dirNode, ConstCstring name, bool isDirectory);

void fsnodeAttribute_initDefault(FSnodeAttribute* attribute) {
    attribute->uid = 0;
    attribute->gid = 0;
    attribute->createTime = 0;  //Clock not initialized yet
    attribute->lastAccessTime = 0;
    attribute->lastModifyTime = 0;
}

fsNode* fsnode_create(DirectoryEntry* entry, FSnodeAttribute* attribute, fsNode* parent) {
    DEBUG_ASSERT_SILENT(directoryEntry_isDetailed(entry));
    DEBUG_ASSERT_SILENT(entry->type != FS_ENTRY_TYPE_DUMMY);
    
    fsNode* ret = NULL;
    if (entry->type == FS_ENTRY_TYPE_DIRECTORY) {
        DirFSnode* dirNode = mm_allocate(sizeof(DirFSnode));
        if (dirNode == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        __fsnodeDirPart_initStruct(&dirNode->dirPart);
        ret = &dirNode->node;
    } else {
        ret = mm_allocate(sizeof(fsNode));
        if (ret == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
    }
    
    DEBUG_ASSERT_SILENT(parent == NULL || parent->entry.type == FS_ENTRY_TYPE_DIRECTORY);
    __fsnode_initStruct(ret, entry, attribute, parent);

    DEBUG_ASSERT_SILENT(__fsnode_isReadyToRelease(ret));
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void fsnode_refer(fsNode* node) {
    if (REF_COUNTER_REFER(node->refCounter) == 1 && node->parent != NULL) {
        DirFSnode* parentFSnode = FSNODE_GET_DIRFSNODE(node->parent);
        __dirFSnode_addLivingChild(parentFSnode);
    }
}

bool fsnode_derefer(fsNode* node) {
    if (__fsnode_isReadyToRelease(node)) {
        __fsnode_release(node);
        return true;
    }

    if (REF_COUNTER_DEREFER(node->refCounter) == 0) {
        DirFSnode* parentFSnode = FSNODE_GET_DIRFSNODE(node->parent);
        __dirFSnode_removeLivingChild(parentFSnode);
    }
    
    return false;
}

fsNode* fsnode_lookup(fsNode* node, ConstCstring name, bool isDirectory, bool autoRead) {
    DEBUG_ASSERT_SILENT(node->entry.type == FS_ENTRY_TYPE_DIRECTORY);

    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(node);
    if (dirNode->dirPart.childrenNum == FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM) { //Children not read yet
        if (autoRead) {
            fsnode_readDirectoryEntries(node);
        } else {
            return NULL;
        }
    }

    fsNode* ret = NULL;
    spinlock_lock(&node->lock);
    Object hash = __fsnode_nameHash(name, isDirectory);
    HashChainNode* hashNode = hashTable_find(&dirNode->dirPart.childrenHash, hash);
    if (hashNode != NULL) {
        ret = HOST_POINTER(hashNode, fsNode, childHashNode);
    }

    spinlock_unlock(&node->lock);
    fsnode_refer(ret);  //Refer returned node
    return ret;
}

void fsnode_setVnode(fsNode* node, vNode* vnode) {
    if (node->vnode != NULL && vnode != NULL) {
        ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
    }

    if (node->vnode == NULL && vnode == NULL) {
        return;
    }

    node->vnode = vnode;
    if (vnode == NULL) {
        fsnode_derefer(node);
    } else {
        DEBUG_ASSERT_SILENT(!(node->entry.type == FS_ENTRY_TYPE_DIRECTORY && FSNODE_GET_DIRFSNODE(node)->dirPart.childrenNum != FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM));   //If vNode is not set, it's impossible to have any children
        fsnode_refer(node);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsnode_setMount(fsNode* node, vNode* mountVnode) {
    DEBUG_ASSERT_SILENT(node->entry.type == FS_ENTRY_TYPE_DIRECTORY);
    if (node->mount != NULL && mountVnode != NULL) {   //TODO: Remount support
        ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
    }

    if (node->mount == NULL && mountVnode == NULL) {
        return;
    }

    node->mount = mountVnode;
    if (mountVnode == NULL) {
        fsnode_derefer(node);
        fsnode_releaseVnode(mountVnode->fscore, mountVnode->fsNode);
    } else {
        fsnode_requestVnode(mountVnode->fscore, mountVnode->fsNode);
        fsnode_refer(node);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsnode_requestVnode(FScore* fscore, fsNode* node) {
    if (node->vnode != NULL) {
        DEBUG_ASSERT_SILENT(REF_COUNTER_GET(node->vNodeRefCounter) > 0);    //vnode must be referred at least once, as long as it exists
        fsnode_refer(node);
    } else {
        vNode* ret = fscore_rawOpenVnode(fscore, node);
        ERROR_GOTO_IF_ERROR(0);
        fsnode_setVnode(node, ret); //Refer node here
    }

    REF_COUNTER_REFER(node->vNodeRefCounter);

    return;
    ERROR_FINAL_BEGIN(0);
    return;
}

bool fsnode_releaseVnode(FScore* fscore, fsNode* node) {
    DEBUG_ASSERT_SILENT(node->vnode != NULL);
    DEBUG_ASSERT_SILENT(REF_COUNTER_GET(node->vNodeRefCounter) > 0);
    if (REF_COUNTER_DEREFER(node->vNodeRefCounter) > 0) {
        fsnode_derefer(node);
    } else {
        fscore_rawCloseVnode(fscore, node->vnode);
        fsnode_setVnode(node, NULL); //Derefer node here
    }
}

void fsnode_readDirectoryEntries(fsNode* node) {
    DEBUG_ASSERT_SILENT(node->entry.type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(node->vnode != NULL);
    DEBUG_ASSERT_SILENT(FSNODE_GET_DIRFSNODE(node)->dirPart.childrenNum == FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM);

    vNode_rawReadDirectoryEntries(node->vnode);   //Use fsnode_create to append children to node, refer node by number of child
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsnode_forgetAllDirectoryEntries(fsNode* node) {
    DEBUG_ASSERT_SILENT(node->entry.type == FS_ENTRY_TYPE_DIRECTORY);
    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(node);
    fsNodeDirPart* part = &dirNode->dirPart;

    DEBUG_ASSERT_SILENT(REF_COUNTER_CHECK(part->livingChildNum, 0));

    if (part->childrenNum == FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM) {
        return;
    }

    for (LinkedListNode* listNode = linkedListNode_getNext(&part->children); listNode != &part->children;) {
        LinkedListNode* next = linkedListNode_getNext(listNode);
        linkedListNode_delete(listNode);
        fsNode* childNode = HOST_POINTER(listNode, fsNode, childNode);
        DEBUG_ASSERT_SILENT(fsnode_derefer(childNode)); //Derefer node here
        listNode = next;
    }

    part->childrenNum = FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM;

    __fsnodeDirPart_initStruct(part);
}

void fsnode_forgetDirectoryEntry(fsNode* node, ConstCstring name, bool isDirectory) {
    DEBUG_ASSERT_SILENT(node->entry.type == FS_ENTRY_TYPE_DIRECTORY);
    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(node);
    fsNodeDirPart* part = &dirNode->dirPart;

    fsNode* removed = __dirfsNode_removeChildNode(dirNode, name, isDirectory);
    if (removed != NULL) {
        DEBUG_ASSERT_SILENT(fsnode_derefer(removed));   //Derefer node here
    }
}

void fsnode_getAbsolutePath(fsNode* node, String* pathOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(pathOut));
    string_clear(pathOut);
    
    __fsnode_doGetAbsolutePath(node, pathOut);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsnode_move(fsNode* node, fsNode* moveTo, ConstCstring newName) {
    DEBUG_ASSERT_SILENT(moveTo->entry.type == FS_ENTRY_TYPE_DIRECTORY);

    spinlock_lock(&node->lock);
    spinlock_lock(&node->parent->lock);
    spinlock_lock(&moveTo->lock);

    if (!__fsnode_isReadyToRelease(node)) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    DirFSnode* originParentNode = FSNODE_GET_DIRFSNODE(node->parent);
    DirFSnode* newParentNode = FSNODE_GET_DIRFSNODE(moveTo);
    
    __dirfsNode_removeChildNode(originParentNode, node->name.data, node->entry.type == FS_ENTRY_TYPE_DIRECTORY);
    if (newName != NULL) {
        string_clearStruct(&node->name);
        string_initStructStr(&node->name, newName);
        ERROR_GOTO_IF_ERROR(0);
    }
    __dirfsNode_addChildNode(newParentNode, node);
    
    spinlock_unlock(&moveTo->lock);
    spinlock_unlock(&node->parent->lock);
    spinlock_unlock(&node->lock);

    return;
    ERROR_FINAL_BEGIN(0);

    spinlock_unlock(&moveTo->lock);
    spinlock_unlock(&node->parent->lock);
    spinlock_unlock(&node->lock);
}

static void __fsnode_initStruct(fsNode* node, DirectoryEntry* entry, FSnodeAttribute* attribute, fsNode* parent) {    
    string_initStructStr(&node->name, entry->name);
    ERROR_GOTO_IF_ERROR(0);

    node->entry = (DirectoryEntry) {
        .name = node->name.data,
        .type = entry->type,
        .mode = entry->mode,
        .vnodeID = entry->vnodeID,
        .size = entry->size,
        .pointsTo = entry->pointsTo
    };

    memory_memcpy(&node->attribute, attribute, sizeof(FSnodeAttribute));

    REF_COUNTER_INIT(node->refCounter, 0);  //Release should not be invoked by internal operations

    linkedListNode_initStruct(&node->childNode);
    
    node->parent = parent;

    node->vnode = node->mount = NULL;
    REF_COUNTER_INIT(node->vNodeRefCounter, 0);
    
    node->lock = SPINLOCK_UNLOCKED;

    if (parent != NULL) {
        spinlock_lock(&parent->lock);
        fsnode_refer(parent);   //Refer parent here
        DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(parent);
        __dirfsNode_addChildNode(dirNode, node);
        spinlock_unlock(&parent->lock);
    }
        
    return;
    ERROR_FINAL_BEGIN(0);
}

static void __fsnode_release(fsNode* node) {
    DEBUG_ASSERT_SILENT(__fsnode_isReadyToRelease(node));

    mm_free(node);
}

static void __fsnode_doGetAbsolutePath(fsNode* node, String* pathOut) {
    if (node->parent == NULL) {
        string_initStructStr(pathOut, PATH_SEPERATOR_STR);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        __fsnode_doGetAbsolutePath(node->parent, pathOut);
        ERROR_GOTO_IF_ERROR(0);
        path_join(pathOut, pathOut, &node->name);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __fsnodeDirPart_initStruct(fsNodeDirPart* part) {
    linkedList_initStruct(&part->children);
    hashTable_initStruct(&part->childrenHash, FSNODE_DIR_PART_CHILDREN_HASH_CHAIN_SIZE, part->childrenHashChains, hashTable_defaultHashFunc);
    part->childrenNum = FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM;
    REF_COUNTER_INIT(part->livingChildNum, 0);
    for (int i = 0; i < FSNODE_DIR_PART_CHILDREN_HASH_CHAIN_SIZE; ++i) {
        singlyLinkedList_initStruct(&part->childrenHashChains[i]);
    }
}

static void __dirFSnode_addLivingChild(DirFSnode* dirNode) {
    fsNodeDirPart* part = &dirNode->dirPart;
    fsNode* node = &dirNode->node;

    DEBUG_ASSERT_SILENT(node->vnode != NULL && part->childrenNum != FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM);
    if (__fsnode_isReadyToRelease(node) && node->parent != NULL) {
        DirFSnode* parentDirNode = FSNODE_GET_DIRFSNODE(node->parent);
        __dirFSnode_addLivingChild(parentDirNode);
    }
    
    REF_COUNTER_REFER(part->livingChildNum);
    REF_COUNTER_REFER(node->refCounter);
}

static void __dirFSnode_removeLivingChild(DirFSnode* dirNode) {
    fsNodeDirPart* part = &dirNode->dirPart;
    fsNode* node = &dirNode->node;

    DEBUG_ASSERT_SILENT(REF_COUNTER_GET(part->livingChildNum) != 0);
    REF_COUNTER_DEREFER(part->livingChildNum);
    DEBUG_ASSERT_SILENT(REF_COUNTER_GET(node->refCounter) != 0);
    REF_COUNTER_DEREFER(node->refCounter);

    if (__fsnode_isReadyToRelease(node) && node->parent != NULL) {
        DirFSnode* parentDirNode = FSNODE_GET_DIRFSNODE(node->parent);
        __dirFSnode_removeLivingChild(parentDirNode);
    }
}

static void __dirfsNode_addChildNode(DirFSnode* dirNode, fsNode* child) {
    hashChainNode_initStruct(&child->childHashNode);
    Object hash = __fsnode_nameHash(child->name.data, child->entry.type == FS_ENTRY_TYPE_DIRECTORY);

    fsNodeDirPart* part = &dirNode->dirPart;
    hashTable_insert(&part->childrenHash, hash, &child->childHashNode);
    
    linkedList_initStruct(&child->childNode);
    linkedListNode_insertBack(&part->children, &child->childNode);
    ++part->childrenNum;
}

static fsNode* __dirfsNode_removeChildNode(DirFSnode* dirNode, ConstCstring name, bool isDirectory) {
    Object hash = __fsnode_nameHash(name, isDirectory);

    fsNodeDirPart* part = &dirNode->dirPart;
    HashChainNode* deleted = hashTable_delete(&part->childrenHash, hash);
    if (deleted == NULL) {
        return NULL;
    }

    fsNode* deletedNode = HOST_POINTER(deleted, fsNode, childHashNode);

    DEBUG_ASSERT_SILENT(deletedNode->parent == &dirNode->node);
    DEBUG_ASSERT_SILENT(__fsnode_isReadyToRelease(deletedNode));
    
    linkedListNode_delete(&deletedNode->childNode);
    --part->childrenNum;

    return deletedNode;
}
