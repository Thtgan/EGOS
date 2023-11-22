#include<fs/inode.h>

#include<fs/fileSystem.h>
#include<fs/fileSystemEntry.h>
#include<kit/types.h>

iNode* openInodeBuffered(SuperBlock* superBlock, FileSystemEntryDescriptor* entryDescriptor) {
    iNode* ret = openInodeFromOpened(superBlock, entryDescriptor);
    if (ret == NULL) {
        ret = kMalloc(sizeof(iNode));
        if (ret == NULL || rawSuperNodeOpenInode(superBlock, ret, entryDescriptor) == RESULT_FAIL) {
            return NULL;
        }

        if (registerInode(superBlock, ret, entryDescriptor) == RESULT_FAIL) {
            kFree(ret);
            return NULL;
        }
    } else {
        ++ret->openCnt;
    }

    return ret;
}

Result closeInodeBuffered(iNode* iNode, FileSystemEntryDescriptor* entryDescriptor) {
    SuperBlock* superBlock = iNode->superBlock;
    if (--iNode->openCnt == 0) {
        if (unregisterInode(superBlock, entryDescriptor) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
        
        if (rawSuperNodeCloseInode(superBlock, iNode) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
        kFree(iNode);
    }

    return RESULT_SUCCESS;
}