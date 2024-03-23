#include<fs/inode.h>

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>

iNode* iNode_openFromOpened(HashTable* table, Index64 blockIndex) {
    HashChainNode* found = hashTable_find(table, (Object)blockIndex);
    return found == NULL ? NULL : HOST_POINTER(found, iNode, openedNode);
}

Result iNode_addToOpened(HashTable* table, iNode* iNode, Index64 blockIndex) {
    return hashTable_insert(table, (Object)blockIndex, &iNode->openedNode);
}

Result iNode_removeFromOpened(HashTable* table, Index64 blockIndex) {
    return hashTable_delete(table, (Object)blockIndex) != NULL;
}

iNode* iNode_open(SuperBlock* superBlock, fsEntryDesc* desc) {
    iNode* ret = iNode_openFromOpened(&superBlock->openedInode, desc->dataRange.begin >> superBlock->device->bytePerBlockShift);
    if (ret == NULL) {
        ret = kMallocSpecific(sizeof(iNode), PHYSICAL_PAGE_ATTRIBUTE_PUBLIC, 16);
        if (ret == NULL || superBlock_rawOpenInode(superBlock, ret, desc) == RESULT_FAIL) {
            return NULL;
        }

        if (iNode_addToOpened(&superBlock->openedInode, ret, desc->dataRange.begin >> superBlock->device->bytePerBlockShift) == RESULT_FAIL) {
            kFree(ret);
            return NULL;
        }
    } else {
        ++ret->openCnt;
    }

    return ret;
}

Result iNode_close(iNode* iNode, fsEntryDesc* desc) {
    SuperBlock* superBlock = iNode->superBlock;
    if (--iNode->openCnt == 0) {
        if (iNode_removeFromOpened(&superBlock->openedInode, desc->dataRange.begin >> superBlock->device->bytePerBlockShift) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
        
        if (superBlock_rawCloseInode(superBlock, iNode) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
        kFree(iNode);
    }

    return RESULT_SUCCESS;
}