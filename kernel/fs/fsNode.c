#include<fs/fsNode.h>

#include<fs/path.h>
#include<kit/util.h>
#include<memory/mm.h>
#include<multitask/locks/spinlock.h>
#include<structs/linkedList.h>
#include<structs/refCounter.h>
#include<structs/string.h>
#include<cstring.h>
#include<debug.h>
#include<error.h>

static inline bool __fsNode_isTypeMatchDirectory(fsNode* node, bool isDirectory) {
    return (node->type == FS_ENTRY_TYPE_DIRECTORY) == isDirectory;
}

static inline void __fsNode_addChildNode(fsNode* node, fsNode* child) {
    REF_COUNTER_REFER(node->refCounter);
    linkedListNode_insertBack(&node->children, &child->childNode);
}

static inline void __fsNode_removeChildNode(fsNode* child) {
    linkedListNode_delete(&child->childNode);
}

static void __fsNode_doGetAbsolutePath(fsNode* node, String* pathOut);

fsNode* fsNode_create(ConstCstring name, fsEntryType type, fsNode* parent, ID vnodeID) {
    DEBUG_ASSERT_SILENT(type != FS_ENTRY_TYPE_DUMMY);
    
    fsNode* ret = mm_allocate(sizeof(fsNode));
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    string_initStructStr(&ret->name, name);
    ERROR_GOTO_IF_ERROR(0);
    ret->type = type;
    REF_COUNTER_INIT(ret->refCounter, 0);
    linkedList_initStruct(&ret->children);
    linkedListNode_initStruct(&ret->childNode);

    if (parent != NULL) {
        spinlock_lock(&parent->lock);
        __fsNode_addChildNode(parent, ret);
        spinlock_unlock(&parent->lock);
    }

    ret->parent = parent;
    ret->vnodeID = vnodeID;
    ret->isVnodeActive = false;

    ret->lock = SPINLOCK_UNLOCKED;
    ret->mountOverwrite = NULL;
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

fsNode* fsNode_lookup(fsNode* node, ConstCstring name, bool isDirectory) {
    fsNode* ret = NULL;
    
    spinlock_lock(&node->lock);
    DEBUG_ASSERT_SILENT(!REF_COUNTER_CHECK(node->refCounter, 0));  //TODO: Not reliable
    for (LinkedListNode* childNode = node->children.next; childNode != &node->children; childNode = childNode->next) {
        fsNode* child = HOST_POINTER(childNode, fsNode, childNode);
        if (cstring_strcmp(name, child->name.data) == 0 && __fsNode_isTypeMatchDirectory(child, isDirectory)) {
            fsNode_refer(child);
            ret = child;
            break;
        }
    }
    spinlock_unlock(&node->lock);
    return ret;
}

void fsNode_remove(fsNode* node) {
    fsNode* parent = node->parent;
    DEBUG_ASSERT_SILENT(parent != NULL);
    spinlock_lock(&parent->lock);
    __fsNode_removeChildNode(node);
    spinlock_unlock(&parent->lock);
}

void fsNode_release(fsNode* node) {
    fsNode* currentNode = node;

    while (true) {  //Node is not supposed to be released when its being accessed(e.g. lookup), for it should be refered by a vNode
        if (!(REF_COUNTER_CHECK(currentNode->refCounter, 0) || REF_COUNTER_DEREFER(currentNode->refCounter) == 0)) {
            break;
        }

        DEBUG_ASSERT_SILENT(!currentNode->isVnodeActive);

        fsNode* parent = currentNode->parent;
        if (parent != NULL) {
            spinlock_lock(&parent->lock);
            DEBUG_ASSERT_SILENT(!REF_COUNTER_CHECK(parent->refCounter, 0));    //TODO: Not reliable
            __fsNode_removeChildNode(currentNode);
            spinlock_unlock(&parent->lock);
        } else {
            break;
        }

        mm_free(currentNode);
        currentNode = parent;
    }
}

vNode* fsNode_getVnode(fsNode* node, FScore* fscore) {
    if (node->mountOverwrite != NULL) {
        return node->mountOverwrite;
    }

    vNode* ret = fscore_openVnode(fscore, node->vnodeID);
    ERROR_GOTO_IF_ERROR(0);
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void fsNode_setMount(fsNode* node, vNode* mountVnode) {
    DEBUG_ASSERT_SILENT(node->type == FS_ENTRY_TYPE_DIRECTORY);
    if (node->mountOverwrite != NULL && mountVnode != NULL) {   //TODO: Remount support
        ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
    }
    
    node->mountOverwrite = mountVnode;
    return;
    ERROR_FINAL_BEGIN(0);
}

void fsNode_getAbsolutePath(fsNode* node, String* pathOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(pathOut));
    string_clear(pathOut);
    
    __fsNode_doGetAbsolutePath(node, pathOut);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsNode_transplant(fsNode* node, fsNode* transplantTo) {
    if (node->parent == transplantTo) {
        return;
    }
    
    for (fsNode* i = transplantTo; i != NULL; i = i->parent) {
        if (node == i) {
            ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
        }
    }
    
    fsNode* parent = node->parent;
    if (parent == NULL) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    spinlock_lock(&parent->lock);
    spinlock_lock(&transplantTo->lock);
    __fsNode_removeChildNode(node);
    __fsNode_addChildNode(transplantTo, node);
    spinlock_unlock(&transplantTo->lock);
    spinlock_unlock(&parent->lock);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __fsNode_doGetAbsolutePath(fsNode* node, String* pathOut) {
    if (node->parent == NULL) {
        string_initStructStr(pathOut, PATH_SEPERATOR_STR);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        __fsNode_doGetAbsolutePath(node->parent, pathOut);
        ERROR_GOTO_IF_ERROR(0);
        path_join(pathOut, pathOut, &node->name);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}
