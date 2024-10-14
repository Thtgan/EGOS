#include<fs/inode.h>

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>

iNode* iNode_openFromOpened(HashTable* table, Index64 blockIndex) {
    HashChainNode* found = hashTable_find(table, (Object)blockIndex);
    return found == NULL ? NULL : HOST_POINTER(found, iNode, openedNode);
}

Result iNode_addToOpened(HashTable* table, iNode* iNode, Index64 blockIndex) {
    return hashTable_insert(table, (Object)blockIndex, &iNode->openedNode);
}

Result iNode_removeFromOpened(HashTable* table, Index64 blockIndex) {
    return hashTable_delete(table, (Object)blockIndex) != NULL ? RESULT_SUCCESS : RESULT_FAIL;
}

iNode* iNode_open(SuperBlock* superBlock, fsEntryDesc* desc) {
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Index64 key = desc->type == FS_ENTRY_TYPE_DEVICE ? desc->device + superBlockBlockDevice->device.capacity : desc->dataRange.begin >> superBlockBlockDevice->device.granularity;
    iNode* ret = iNode_openFromOpened(&superBlock->openedInode, key);
    if (ret == NULL) {
        ret = memory_allocate(sizeof(iNode));
        if (ret == NULL || superBlock_rawOpenInode(superBlock, ret, desc) != RESULT_SUCCESS) {
            return NULL;
        }

        if (iNode_addToOpened(&superBlock->openedInode, ret, key) != RESULT_SUCCESS) {
            memory_free(ret);
            return NULL;
        }
    } else {
        ++ret->openCnt;
    }

    return ret;
}

Result iNode_close(iNode* iNode, fsEntryDesc* desc) {
    SuperBlock* superBlock = iNode->superBlock;
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Index64 key = desc->type == FS_ENTRY_TYPE_DEVICE ? desc->device + superBlockBlockDevice->device.capacity : desc->dataRange.begin >> superBlockBlockDevice->device.granularity;
    if (--iNode->openCnt == 0) {
        if (iNode_removeFromOpened(&superBlock->openedInode, key) == RESULT_FAIL) {
            return RESULT_ERROR;
        }
        
        if (superBlock_rawCloseInode(superBlock, iNode) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }
        memory_free(iNode);
    }

    return RESULT_SUCCESS;
}