#include<fs/superblock.h>

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
#include<structs/hashTable.h>
#include<structs/linkedList.h>
#include<structs/refCounter.h>
#include<structs/string.h>
#include<error.h>

void superBlock_initStruct(SuperBlock* superBlock, SuperBlockInitArgs* args) {
    ERROR_ASSERT_NONE();
    superBlock->blockDevice     = args->blockDevice;
    superBlock->operations      = args->operations;
    superBlock->rootInode       = NULL;
    hashTable_initStruct(&superBlock->openedInode, args->openedInodeBucket, args->openedInodeChains, hashTable_defaultHashFunc);
    linkedList_initStruct(&superBlock->mounted);
    superBlock->nextInodeID     = 0;
    
    iNode* rootInode = superBlock_rawOpenRootInode(superBlock);
    if (rootInode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    superBlock->rootInode       = rootInode;
    hashTable_insert(&superBlock->openedInode, rootInode->inodeID, &rootInode->openedNode);
    ERROR_ASSERT_NONE();

    return;
    ERROR_FINAL_BEGIN(0);
}

ID superBlock_allocateInodeID(SuperBlock* superBlock) {
    ID ret = ATOMIC_FETCH_INC(&superBlock->nextInodeID);
    DEBUG_ASSERT(ret != INVALID_ID, "iNode allocation overflow\n");
    return ret;
}

fsNode* superBlock_getFSnode(SuperBlock* superBlock, ID inodeID) {
    fsNode* ret = NULL;
    HashChainNode* found = hashTable_find(&superBlock->openedInode, (Object)inodeID);
    if (found != NULL) {
        ret = HOST_POINTER(found, iNode, openedNode)->fsNode;
        fsNode_refer(ret);  //Refer 'ret' once
    } else {
        ret = superBlock_rawGetFSnode(superBlock, inodeID); //Refer 'ret' once
        ERROR_GOTO_IF_ERROR(0);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

iNode* superBlock_openInode(SuperBlock* superBlock, ID inodeID) {
    //TODO: Lock
    iNode* ret = NULL;
    HashChainNode* found = hashTable_find(&superBlock->openedInode, (Object)inodeID);
    if (found != NULL) {
        ret = HOST_POINTER(found, iNode, openedNode);
    } else {
        ret = superBlock_rawOpenInode(superBlock, inodeID);
        ERROR_GOTO_IF_ERROR(0);
        hashTable_insert(&superBlock->openedInode, inodeID, &ret->openedNode);
        ERROR_ASSERT_NONE();
    }
    refCounter_refer(&ret->refCounter);
    ret->fsNode->isInodeActive = true;  //TODO: Ugly code
    fsNode_refer(ret->fsNode);  //Refer 'node' once

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void superBlock_closeInode(iNode* inode) {
    //TODO: Lock
    DEBUG_ASSERT_SILENT(inode->superBlock != NULL);

    SuperBlock* superBlock = inode->superBlock;
    if (refCounter_derefer(&inode->refCounter)) {
        fsNode* node = inode->fsNode;
        HashChainNode* deleted = hashTable_delete(&superBlock->openedInode, inode->inodeID);
        if (deleted == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        
        superBlock_rawCloseInode(superBlock, inode);
        ERROR_GOTO_IF_ERROR(0);
        node->isInodeActive = false;    //TODO: Ugly code
        fsNode_release(node);   //Release 'node' once (from openInode)
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

fsEntry* superBlock_genericOpenFSentry(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags) {
    //TODO: Lock
    fsEntry* ret = memory_allocate(sizeof(fsEntry));
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fsEntry_initStruct(ret, inode, NULL, flags);

    return ret;
    ERROR_FINAL_BEGIN(0);
    
}

void superBlock_genericCloseFSentry(SuperBlock* superBlock, fsEntry* entry) {
    //TODO: Lock
    memory_memset(entry, 0, sizeof(fsEntry));
    memory_free(entry);
}

void superBlock_genericMount(SuperBlock* superBlock, fsIdentifier* mountPoint, iNode* mountInode, Flags8 flags) {
    //TODO: flags not used yet
    //TODO: Lock
    fsNode* mountPointNode = NULL;
    iNode* parentDirInode = NULL;
    Mount* mount = memory_allocate(sizeof(Mount));
    if (mount == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    string_initStruct(&mount->path);
    fsIdentifier_getAbsolutePath(mountPoint, &mount->path);
    ERROR_GOTO_IF_ERROR(0);
    linkedListNode_initStruct(&mount->node);
    mount->mountedInode = mountInode;

    linkedListNode_insertBack(&superBlock->mounted, &mount->node);

    SuperBlock* finalSuperBlock = NULL;
    mountPointNode = locate(mountPoint, FCNTL_OPEN_DIRECTORY_DEFAULT_FLAGS, &parentDirInode, &finalSuperBlock);   //Refer 'mountPointNode' once (if found), refer 'parentDirInode->fsNode' once (if iNode opened)
    if (mountPointNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(superBlock == finalSuperBlock);
    DEBUG_ASSERT_SILENT(mountPointNode->type == FS_ENTRY_TYPE_DIRECTORY);

    fsNode_setMount(mountPointNode, mountInode);
    ERROR_GOTO_IF_ERROR(0);

    if (parentDirInode != NULL) {
        superBlock_closeInode(parentDirInode);  //Release 'parentDirInode->fsNode' once (if iNode opened in locate)
        parentDirInode = NULL;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    if (mount != NULL) {
        if (string_isAvailable(&mount->path)) {
            string_clearStruct(&mount->path);
        }

        bool inserted = false;
        for (LinkedListNode* node = superBlock->mounted.next; node != &superBlock->mounted; node = node->next) {
            if (&mount->node == node) {
                inserted = true;
                break;
            }
        }

        if (inserted) {
            linkedListNode_delete(&mount->node);
        }

        if (mountPointNode != NULL && mountPointNode->mountOverwrite == mountInode) {
            fsNode_setMount(mountPointNode, NULL);  //TODO: Is this correct?
        }

        memory_free(mount);
    }

    if (mountPointNode != NULL) {
        fsNode_release(mountPointNode); //Release 'mountPointNode' once (from locate)
    }

    if (parentDirInode != NULL) {
        superBlock_closeInode(parentDirInode);  //Release 'parentDirInode->fsNode' once (if iNode opened in locate)
    }
}

void superBlock_genericUnmount(SuperBlock* superBlock, fsIdentifier* mountPoint) {
    //TODO: Lock
    fsNode* mountPointNode = NULL;
    iNode* parentDirInode = NULL;
    String mountPointPath;
    string_initStruct(&mountPointPath);
    fsIdentifier_getAbsolutePath(mountPoint, &mountPointPath);
    ERROR_GOTO_IF_ERROR(0);

    Mount* mount = superBlock_lookupMount(superBlock, mountPointPath.data);
    DEBUG_ASSERT_SILENT(mount != NULL);

    string_clearStruct(&mount->path);
    linkedListNode_delete(&mount->node);

    memory_free(mount);

    SuperBlock* finalSuperBlock = NULL;
    mountPointNode = locate(mountPoint, FCNTL_OPEN_DIRECTORY_DEFAULT_FLAGS, &parentDirInode, &finalSuperBlock);   //Refer 'mountPointNode' once (if found), refer 'parentDirInode->fsNode' once (if iNode opened)
    if (mountPointNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(mountPointNode->type == FS_ENTRY_TYPE_DIRECTORY);

    fsNode_setMount(mountPointNode, NULL);
    ERROR_GOTO_IF_ERROR(0);

    if (parentDirInode != NULL) {
        superBlock_closeInode(parentDirInode);  //Release 'parentDirInode->fsNode' once (if iNode opened in locate)
        parentDirInode = NULL;
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

    if (parentDirInode != NULL) {
        superBlock_closeInode(parentDirInode);  //Release 'parentDirInode->fsNode' once (if iNode opened in locate)
    }
}

Mount* superBlock_lookupMount(SuperBlock* superBlock, ConstCstring path) {
    //TODO: Lock
    for (LinkedListNode* node = superBlock->mounted.next; node != &superBlock->mounted; node = node->next) {
        Mount* mount = HOST_POINTER(node, Mount, node);
        String* mountPath = &mount->path;

        Size prefixLen = cstring_prefixLen(path, mountPath->data);
        if (prefixLen == mountPath->length) {
            return mount;
        }
    }

    return NULL;
}