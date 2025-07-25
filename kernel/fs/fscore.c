#include<fs/fscore.h>

#include<cstring.h>
#include<fs/fcntl.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/locate.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<structs/hashTable.h>
#include<structs/linkedList.h>
#include<structs/refCounter.h>
#include<structs/string.h>
#include<error.h>

void fsCore_initStruct(FScore* fsCore, FScoreInitArgs* args) {
    ERROR_ASSERT_NONE();
    fsCore->blockDevice     = args->blockDevice;
    fsCore->operations      = args->operations;
    fsCore->rootVnode       = NULL;
    hashTable_initStruct(&fsCore->openedVnode, args->openedVnodeBucket, args->openedVnodeChains, hashTable_defaultHashFunc);
    linkedList_initStruct(&fsCore->mounted);
    fsCore->nextVnodeID     = 0;
    
    vNode* rootVnode = fsCore_rawOpenRootVnode(fsCore);
    if (rootVnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    fsCore->rootVnode       = rootVnode;
    hashTable_insert(&fsCore->openedVnode, rootVnode->vnodeID, &rootVnode->openedNode);
    ERROR_ASSERT_NONE();

    return;
    ERROR_FINAL_BEGIN(0);
}

ID fsCore_allocateVnodeID(FScore* fsCore) {
    ID ret = ATOMIC_FETCH_INC(&fsCore->nextVnodeID);
    DEBUG_ASSERT(ret != INVALID_ID, "vNode allocation overflow\n");
    return ret;
}

fsNode* fsCore_getFSnode(FScore* fsCore, ID vnodeID) {
    fsNode* ret = NULL;
    HashChainNode* found = hashTable_find(&fsCore->openedVnode, (Object)vnodeID);
    if (found != NULL) {
        ret = HOST_POINTER(found, vNode, openedNode)->fsNode;
        fsNode_refer(ret);  //Refer 'ret' once
    } else {
        ret = fsCore_rawGetFSnode(fsCore, vnodeID); //Refer 'ret' once
        ERROR_GOTO_IF_ERROR(0);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

vNode* fsCore_openVnode(FScore* fsCore, ID vnodeID) {
    //TODO: Lock
    vNode* ret = NULL;
    HashChainNode* found = hashTable_find(&fsCore->openedVnode, (Object)vnodeID);
    if (found != NULL) {
        ret = HOST_POINTER(found, vNode, openedNode);
    } else {
        ret = fsCore_rawOpenVnode(fsCore, vnodeID);
        ERROR_GOTO_IF_ERROR(0);
        hashTable_insert(&fsCore->openedVnode, vnodeID, &ret->openedNode);
        ERROR_ASSERT_NONE();
    }
    REF_COUNTER_REFER(ret->refCounter);
    ret->fsNode->isVnodeActive = true;  //TODO: Ugly code
    fsNode_refer(ret->fsNode);  //Refer 'node' once

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void fsCore_closeVnode(vNode* vnode) {
    //TODO: Lock
    DEBUG_ASSERT_SILENT(vnode->fsCore != NULL);

    FScore* fsCore = vnode->fsCore;
    if (REF_COUNTER_DEREFER(vnode->refCounter) == 0) {
        fsNode* node = vnode->fsNode;
        HashChainNode* deleted = hashTable_delete(&fsCore->openedVnode, vnode->vnodeID);
        if (deleted == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        
        fsCore_rawCloseVnode(fsCore, vnode);
        ERROR_GOTO_IF_ERROR(0);
        node->isVnodeActive = false;    //TODO: Ugly code
        fsNode_release(node);   //Release 'node' once (from openVnode)
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

fsEntry* fsCore_genericOpenFSentry(FScore* fsCore, vNode* vnode, FCNTLopenFlags flags) {
    //TODO: Lock
    fsEntry* ret = mm_allocate(sizeof(fsEntry));
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fsEntry_initStruct(ret, vnode, NULL, flags);

    return ret;
    ERROR_FINAL_BEGIN(0);
    
}

void fsCore_genericCloseFSentry(FScore* fsCore, fsEntry* entry) {
    //TODO: Lock
    memory_memset(entry, 0, sizeof(fsEntry));
    mm_free(entry);
}

void fsCore_genericMount(FScore* fsCore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags) {
    //TODO: flags not used yet
    //TODO: Lock
    fsNode* mountPointNode = NULL;
    vNode* parentDirVnode = NULL;
    Mount* mount = mm_allocate(sizeof(Mount));
    if (mount == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    string_initStruct(&mount->path);
    fsIdentifier_getAbsolutePath(mountPoint, &mount->path);
    ERROR_GOTO_IF_ERROR(0);
    linkedListNode_initStruct(&mount->node);
    mount->mountedVnode = mountVnode;

    linkedListNode_insertBack(&fsCore->mounted, &mount->node);

    FScore* finalFScore = NULL;
    mountPointNode = locate(mountPoint, FCNTL_OPEN_DIRECTORY_DEFAULT_FLAGS, &parentDirVnode, &finalFScore);   //Refer 'mountPointNode' once (if found), refer 'parentDirVnode->fsNode' once (if vNode opened)
    if (mountPointNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(fsCore == finalFScore);
    DEBUG_ASSERT_SILENT(mountPointNode->type == FS_ENTRY_TYPE_DIRECTORY);

    fsNode_setMount(mountPointNode, mountVnode);
    ERROR_GOTO_IF_ERROR(0);

    if (parentDirVnode != NULL) {
        fsCore_closeVnode(parentDirVnode);  //Release 'parentDirVnode->fsNode' once (if vNode opened in locate)
        parentDirVnode = NULL;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    if (mount != NULL) {
        if (string_isAvailable(&mount->path)) {
            string_clearStruct(&mount->path);
        }

        bool inserted = false;
        for (LinkedListNode* node = fsCore->mounted.next; node != &fsCore->mounted; node = node->next) {
            if (&mount->node == node) {
                inserted = true;
                break;
            }
        }

        if (inserted) {
            linkedListNode_delete(&mount->node);
        }

        if (mountPointNode != NULL && mountPointNode->mountOverwrite == mountVnode) {
            fsNode_setMount(mountPointNode, NULL);  //TODO: Is this correct?
        }

        mm_free(mount);
    }

    if (mountPointNode != NULL) {
        fsNode_release(mountPointNode); //Release 'mountPointNode' once (from locate)
    }

    if (parentDirVnode != NULL) {
        fsCore_closeVnode(parentDirVnode);  //Release 'parentDirVnode->fsNode' once (if vNode opened in locate)
    }
}

void fsCore_genericUnmount(FScore* fsCore, fsIdentifier* mountPoint) {
    //TODO: Lock
    fsNode* mountPointNode = NULL;
    vNode* parentDirVnode = NULL;
    String mountPointPath;
    string_initStruct(&mountPointPath);
    fsIdentifier_getAbsolutePath(mountPoint, &mountPointPath);
    ERROR_GOTO_IF_ERROR(0);

    Mount* mount = fsCore_lookupMount(fsCore, mountPointPath.data);
    DEBUG_ASSERT_SILENT(mount != NULL);

    string_clearStruct(&mount->path);
    linkedListNode_delete(&mount->node);

    mm_free(mount);

    FScore* finalFScore = NULL;
    mountPointNode = locate(mountPoint, FCNTL_OPEN_DIRECTORY_DEFAULT_FLAGS, &parentDirVnode, &finalFScore);   //Refer 'mountPointNode' once (if found), refer 'parentDirVnode->fsNode' once (if vNode opened)
    if (mountPointNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(mountPointNode->type == FS_ENTRY_TYPE_DIRECTORY);

    fsNode_setMount(mountPointNode, NULL);
    ERROR_GOTO_IF_ERROR(0);

    if (parentDirVnode != NULL) {
        fsCore_closeVnode(parentDirVnode);  //Release 'parentDirVnode->fsNode' once (if vNode opened in locate)
        parentDirVnode = NULL;
    }

    fsNode_release(mountPointNode); //Release 'mountPointNode' once (from locate)
    fsNode_release(mountPointNode); //Release 'mountPointNode' once (from mount)

    return;
    ERROR_FINAL_BEGIN(0);

    if (string_isAvailable(&mountPointPath)) {
        string_clearStruct(&mountPointPath);
    }

    if (mountPointNode != NULL) {
        fsNode_release(mountPointNode); //Release 'mountPointNode' once (from locate)
    }

    if (parentDirVnode != NULL) {
        fsCore_closeVnode(parentDirVnode);  //Release 'parentDirVnode->fsNode' once (if vNode opened in locate)
    }
}

Mount* fsCore_lookupMount(FScore* fsCore, ConstCstring path) {
    //TODO: Lock
    for (LinkedListNode* node = fsCore->mounted.next; node != &fsCore->mounted; node = node->next) {
        Mount* mount = HOST_POINTER(node, Mount, node);
        String* mountPath = &mount->path;

        Size prefixLen = cstring_prefixLen(path, mountPath->data);
        if (prefixLen == mountPath->length) {
            return mount;
        }
    }

    return NULL;
}