#include<fs/locate.h>

#include<fs/fcntl.h>
#include<fs/fsIdentifier.h>
#include<fs/fsNode.h>
#include<fs/path.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<structs/linkedList.h>
#include<structs/string.h>
#include<debug.h>
#include<error.h>

static fsNode* locate_local(SuperBlock* superBlock, iNode* baseInode, String* pathFromBase, bool isDirectory, FCNTLopenFlags flags);

fsNode* locate(fsIdentifier* identifier, FCNTLopenFlags flags, iNode** parentDirInodeOut, SuperBlock** finalSuperBlockOut) {
    DEBUG_ASSERT_SILENT(parentDirInodeOut != NULL);
    DEBUG_ASSERT_SILENT(finalSuperBlockOut != NULL);

    String localAbsoluteDirPath, dirPathFromBase, basename;
    fsNode* ret = NULL;
    bool isDirectory = TEST_FLAGS(flags, FCNTL_OPEN_DIRECTORY);
    iNode* parentDirInode = NULL;

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
        DEBUG_ASSERT_SILENT(localAbsoluteDirPath.length == 1 && localAbsoluteDirPath.data[0] == PATH_SEPERATOR);
        ret = identifier->baseInode->fsNode;
        fsNode_refer(ret);  //Refer 'ret' once

        *parentDirInodeOut = NULL;
        *finalSuperBlockOut = identifier->baseInode->superBlock;
    } else {
        iNode* currentBaseInode = identifier->baseInode;
        SuperBlock* currentSuperBlock = currentBaseInode->superBlock;
        while (true) {
            Mount* relayMount = superBlock_lookupMount(currentSuperBlock, localAbsoluteDirPath.data);
            if (relayMount == NULL) {
                break;
            }
    
            currentBaseInode = relayMount->mountedInode;
            string_slice(&dirPathFromBase, &localAbsoluteDirPath, relayMount->path.length, localAbsoluteDirPath.length);
            ERROR_GOTO_IF_ERROR(0);
            fsNode_getAbsolutePath(currentBaseInode->fsNode, &localAbsoluteDirPath);
            ERROR_GOTO_IF_ERROR(0);
            path_join(&localAbsoluteDirPath, &localAbsoluteDirPath, &dirPathFromBase);
            ERROR_GOTO_IF_ERROR(0);
            currentSuperBlock = currentBaseInode->superBlock;
        }

        fsNode* parentDirNode = locate_local(currentSuperBlock, currentBaseInode, &localAbsoluteDirPath, identifier->isDirectory, flags);    //Refer 'parentDirNode' once
        ERROR_GOTO_IF_ERROR(0);
        parentDirInode = fsNode_getInode(parentDirNode, currentSuperBlock); //Refer 'parentDirNode' once (if iNode not opened)
        ERROR_GOTO_IF_ERROR(0);
        fsNode_release(parentDirNode);  //Release 'parentDirNode' once (from locate_local)

        ret = iNode_lookupDirectoryEntry(parentDirInode, basename.data, isDirectory);   //Refer 'ret' once (if found)
        ERROR_GOTO_IF_ERROR(0);
        
        if (ret->mountOverwrite != NULL) {
            currentSuperBlock = ret->mountOverwrite->superBlock;
        }

        *parentDirInodeOut = parentDirInode;
        *finalSuperBlockOut = currentSuperBlock;
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

    if (parentDirInode != NULL) {
        superBlock_closeInode(parentDirInode);  //Release 'parentDirInode->fsNode' (parentDirNode) once (if iNode opened)
    }

    if (ret != NULL) {
        fsNode_release(ret);    //Release 'ret' once (from root or iNode_lookupDirectoryEntry)
    }

    return NULL;
}

static fsNode* locate_local(SuperBlock* superBlock, iNode* baseInode, String* pathFromBase, bool isDirectory, FCNTLopenFlags flags) {
    Index64 currentIndex = 0;
    String walked;
    fsNode* currentNode = NULL;

    if (pathFromBase->length == 0 || cstring_strcmp(pathFromBase->data, PATH_SEPERATOR_STR) == 0) {
        refCounter_refer(&baseInode->fsNode->refCounter);
        return baseInode->fsNode;
    }

    string_initStruct(&walked);
    ERROR_GOTO_IF_ERROR(0);

    iNode* currentInode = baseInode;
    while (currentIndex != INVALID_INDEX) {
        currentIndex = path_walk(pathFromBase, currentIndex, &walked);
        ERROR_GOTO_IF_ERROR(0);

        fsEntryType type = (currentIndex == INVALID_INDEX) ? type : FS_ENTRY_TYPE_DIRECTORY;
        currentNode = iNode_lookupDirectoryEntry(currentInode, walked.data, type);  //Refer 'currentNode' once
        if (currentNode == NULL) {
            ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
        }

        DEBUG_ASSERT_SILENT(currentNode->mountOverwrite == NULL);

        ERROR_GOTO_IF_ERROR(0);
        if (currentInode != baseInode) {
            superBlock_closeInode(currentInode);    //Release 'currentInode->fsNode' (currentNode from last round) once (if iNode opened in last round)
            ERROR_GOTO_IF_ERROR(0);
            currentInode = NULL;
        }

        if (currentIndex == INVALID_INDEX) {
            break;
        }

        currentInode = fsNode_getInode(currentNode, superBlock);    //Refer 'currentNode' once (if iNode not opened)
        fsNode_release(currentNode);    //Release 'currentNode' once (from iNode_lookupDirectoryEntry)
        currentNode = NULL;
        ERROR_GOTO_IF_ERROR(0);
    }

    return currentNode;
    ERROR_FINAL_BEGIN(0);
    if (string_isAvailable(&walked)) {
        string_clearStruct(&walked);
    }

    if (currentNode != NULL) {
        fsNode_release(currentNode);
    }

    if (currentInode != baseInode && currentInode != NULL) {
        superBlock_closeInode(currentInode);
    }
    
    return NULL;
}