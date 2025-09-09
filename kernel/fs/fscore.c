#include<fs/fscore.h>

#include<cstring.h>
#include<fs/fcntl.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/path.h>
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

static fsNode* __fscore_getLocalFSnode(FScore* fscore, fsNode* baseNode, String* pathFromBase, bool isDirectory);

void fscore_initStruct(FScore* fscore, FScoreInitArgs* args) {
    ERROR_ASSERT_NONE();
    fscore->blockDevice     = args->blockDevice;
    fscore->operations      = args->operations;
    fscore->rootFSnode      = fsnode_create("", FS_ENTRY_TYPE_DIRECTORY, NULL, args->rootFSnodePosition);
    linkedList_initStruct(&fscore->mounted);

    fsnode_requestVnode(fscore, fscore->rootFSnode);

    // fscore->nextVnodeID = 0;

    return;
    ERROR_FINAL_BEGIN(0);
}

// ID fscore_allocateVnodeID(FScore* fscore) {
//     ID ret = ATOMIC_FETCH_INC(&fscore->nextVnodeID);
//     DEBUG_ASSERT(ret != INVALID_ID, "vNode allocation overflow\n");
//     return ret;
// }

fsNode* fscore_getFSnode(FScore* fscore, fsIdentifier* identifier, FScore** finalFScoreOut, bool followMount) {
    DEBUG_ASSERT_SILENT(finalFScoreOut != NULL);

    String localAbsoluteDirPath, dirPathFromBase, basename;
    fsNode* ret = NULL;
    vNode* parentDirVnode = NULL;

    string_initStruct(&localAbsoluteDirPath);
    ERROR_GOTO_IF_ERROR(0);
    string_initStruct(&dirPathFromBase);
    ERROR_GOTO_IF_ERROR(0);
    string_initStruct(&basename);
    ERROR_GOTO_IF_ERROR(0);

    fsIdentifier_getAbsolutePath(identifier, &localAbsoluteDirPath);
    ERROR_GOTO_IF_ERROR(0);
    path_basename(&localAbsoluteDirPath, &basename);
    ERROR_GOTO_IF_ERROR(0);
    path_dirname(&localAbsoluteDirPath, &localAbsoluteDirPath);
    ERROR_GOTO_IF_ERROR(0);

    if (basename.length == 0) { //Identifier points to root
        DEBUG_ASSERT_SILENT(localAbsoluteDirPath.length == 0 || (localAbsoluteDirPath.length == 1 && localAbsoluteDirPath.data[0] == PATH_SEPERATOR));
        ret = identifier->baseVnode->fsNode;
        fsnode_refer(ret);  //Refer 'ret' once

        *finalFScoreOut = fscore;
    } else {
        vNode* currentBaseVnode = identifier->baseVnode;
        FScore* currentFScore = fscore;
        
        if (followMount) {
            while (true) {
                Mount* relayMount = fscore_lookupMount(currentFScore, localAbsoluteDirPath.data);
                if (relayMount == NULL) {
                    break;
                }
        
                currentBaseVnode = relayMount->mountedVnode;
                string_slice(&dirPathFromBase, &localAbsoluteDirPath, relayMount->path.length, localAbsoluteDirPath.length);
                ERROR_GOTO_IF_ERROR(0);
                fsnode_getAbsolutePath(currentBaseVnode->fsNode, &localAbsoluteDirPath);
                ERROR_GOTO_IF_ERROR(0);
                path_join(&localAbsoluteDirPath, &localAbsoluteDirPath, &dirPathFromBase);
                ERROR_GOTO_IF_ERROR(0);
                currentFScore = currentBaseVnode->fscore;
            }
        }

        fsNode* parentNode = __fscore_getLocalFSnode(currentFScore, currentBaseVnode->fsNode, &localAbsoluteDirPath, identifier->isDirectory);   //Refer 'ret' once
        ERROR_GOTO_IF_ERROR(0);

        fsnode_requestVnode(currentFScore, parentNode);
        
        ret = fsnode_lookup(parentNode, basename.data, identifier->isDirectory, true);

        fsnode_releaseVnode(currentFScore, parentNode);

        *finalFScoreOut = currentFScore;
    }

    string_clearStruct(&basename);
    string_clearStruct(&dirPathFromBase);
    string_clearStruct(&localAbsoluteDirPath);

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (string_isAvailable(&dirPathFromBase)) {
        string_clearStruct(&dirPathFromBase);
    }

    if (string_isAvailable(&basename)) {
        string_clearStruct(&basename);
    }

    if (string_isAvailable(&localAbsoluteDirPath)) {
        string_clearStruct(&localAbsoluteDirPath);
    }

    if (parentDirVnode != NULL) {
        fscore_releaseVnode(parentDirVnode);    //Release 'parentDirVnode->fsNode' (parentDirNode) once (if vNode opened)
    }

    if (ret != NULL) {
        fsnode_derefer(ret);    //Release 'ret' once (from root or __fscore_getLocalFSnode)
    }

    return NULL;
}

void fscore_releaseFSnode(fsNode* node) {
    fsnode_derefer(node);
}

vNode* fscore_getVnode(FScore* fscore, fsNode* node, bool followMount) {
    if (node->mount != NULL && followMount) {
        vNode* ret = node->mount;
        fsnode_requestVnode(ret->fscore, ret->fsNode);
        return ret;
    }

    fsnode_requestVnode(fscore, node);
    ERROR_GOTO_IF_ERROR(0);

    return node->vnode;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void fscore_releaseVnode(vNode* vnode) {
    fsnode_releaseVnode(vnode->fscore, vnode->fsNode);
}

fsEntry* fscore_genericOpenFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags) {
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

void fscore_genericCloseFSentry(FScore* fscore, fsEntry* entry) {
    //TODO: Lock
    memory_memset(entry, 0, sizeof(fsEntry));
    mm_free(entry);
}

void fscore_genericMount(FScore* fscore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags) {
    //TODO: flags not used yet
    //TODO: Lock
    fsNode* mountPointNode = NULL;
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

    linkedListNode_insertBack(&fscore->mounted, &mount->node);

    FScore* finalFScore = NULL;
    mountPointNode = fscore_getFSnode(fscore, mountPoint, &finalFScore, false); //Refer 'mountPointNode' once (if found)
    if (mountPointNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(fscore == finalFScore);
    DEBUG_ASSERT_SILENT(mountPointNode->type == FS_ENTRY_TYPE_DIRECTORY);

    fsnode_setMount(mountPointNode, mountVnode);
    ERROR_GOTO_IF_ERROR(0);

    fscore_releaseFSnode(mountPointNode);   //Release 'mountPointNode' once (from fscore_getFSnode)

    return;
    ERROR_FINAL_BEGIN(0);
    if (mount != NULL) {
        if (string_isAvailable(&mount->path)) {
            string_clearStruct(&mount->path);
        }

        bool inserted = false;
        for (LinkedListNode* node = fscore->mounted.next; node != &fscore->mounted; node = node->next) {
            if (&mount->node == node) {
                inserted = true;
                break;
            }
        }

        if (inserted) {
            linkedListNode_delete(&mount->node);
        }

        if (mountPointNode != NULL && mountPointNode->mount == mountVnode) {
            fsnode_setMount(mountPointNode, NULL);  //TODO: Is this correct?
        }

        mm_free(mount);
    }

    if (mountPointNode != NULL) {
        fscore_releaseFSnode(mountPointNode);   //Release 'mountPointNode' once (from fscore_getFSnode)
    }
}

void fscore_genericUnmount(FScore* fscore, fsIdentifier* mountPoint) {
    //TODO: Lock
    fsNode* mountPointNode = NULL;
    String mountPointPath;
    string_initStruct(&mountPointPath);
    fsIdentifier_getAbsolutePath(mountPoint, &mountPointPath);
    ERROR_GOTO_IF_ERROR(0);

    Mount* mount = fscore_lookupMount(fscore, mountPointPath.data);
    DEBUG_ASSERT_SILENT(mount != NULL);

    string_clearStruct(&mount->path);
    linkedListNode_delete(&mount->node);

    mm_free(mount);

    FScore* finalFScore = NULL;
    mountPointNode = fscore_getFSnode(fscore, mountPoint, &finalFScore, false);   //Refer 'mountPointNode' once (if found)
    if (mountPointNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(mountPointNode->type == FS_ENTRY_TYPE_DIRECTORY);

    fsnode_setMount(mountPointNode, NULL);
    ERROR_GOTO_IF_ERROR(0);

    fscore_releaseFSnode(mountPointNode);   //Release 'mountPointNode' once (from fscore_getFSnode)

    return;
    ERROR_FINAL_BEGIN(0);

    if (string_isAvailable(&mountPointPath)) {
        string_clearStruct(&mountPointPath);
    }

    if (mountPointNode != NULL) {
        fsnode_setMount(mountPointNode, NULL);
    }
}

Mount* fscore_lookupMount(FScore* fscore, ConstCstring path) {
    //TODO: Lock
    for (LinkedListNode* node = fscore->mounted.next; node != &fscore->mounted; node = node->next) {
        Mount* mount = HOST_POINTER(node, Mount, node);
        String* mountPath = &mount->path;

        Size prefixLen = cstring_prefixLen(path, mountPath->data);
        if (prefixLen == mountPath->length) {
            return mount;
        }
    }

    return NULL;
}

static fsNode* __fscore_getLocalFSnode(FScore* fscore, fsNode* baseNode, String* pathFromBase, bool isDirectory) {
    Index64 currentIndex = 0;
    String walked;
    fsNode* currentNode = baseNode;

    fsnode_refer(baseNode);
    if (pathFromBase->length == 0 || cstring_strcmp(pathFromBase->data, PATH_SEPERATOR_STR) == 0) {
        return baseNode;
    }

    string_initStruct(&walked);
    ERROR_GOTO_IF_ERROR(0);

    while (currentIndex != INVALID_INDEX64) {
        currentIndex = path_walk(pathFromBase, currentIndex, &walked);
        ERROR_GOTO_IF_ERROR(0);

        fsnode_requestVnode(fscore, currentNode);

        fsNode* lastNode = currentNode;
        currentNode = fsnode_lookup(currentNode, walked.data, (currentIndex == INVALID_INDEX64) ? isDirectory : true, true);    //Refer 'currentNode' once
        if (currentNode == NULL) {
            return NULL;
        }
        ERROR_GOTO_IF_ERROR(0);

        fsnode_derefer(lastNode);   //Refer the 'currentNode' from last round

        DEBUG_ASSERT_SILENT(currentNode->mount == NULL);
    }

    return currentNode;
    ERROR_FINAL_BEGIN(0);
    if (string_isAvailable(&walked)) {
        string_clearStruct(&walked);
    }

    if (currentNode != NULL) {
        fsnode_derefer(currentNode);
    }
    
    return NULL;
}