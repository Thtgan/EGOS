#include<fs/inode.h>

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
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
    return hashTable_delete(table, (Object)blockIndex) != NULL;
}

iNode* iNode_open(SuperBlock* superBlock, fsEntryDesc* desc) {
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Index64 key = desc->identifier.type == FS_ENTRY_TYPE_DEVICE ? desc->device + superBlockBlockDevice->device.capacity : desc->dataRange.begin >> superBlockBlockDevice->device.granularity;
    DEBUG_MARK_PRINT("%s %lX\n", superBlockBlockDevice->device.name, key);
    iNode* ret = iNode_openFromOpened(&superBlock->openedInode, key);
    if (ret == NULL) {
        ret = memory_allocate(sizeof(iNode));
        if (ret == NULL || superBlock_rawOpenInode(superBlock, ret, desc) == RESULT_FAIL) {
            DEBUG_MARK_PRINT("MARK\n");
            return NULL;
        }

        if (iNode_addToOpened(&superBlock->openedInode, ret, key) == RESULT_FAIL) {
            DEBUG_MARK_PRINT("MARK\n");
            memory_free(ret);
            return NULL;
        }
    } else {
        DEBUG_MARK_PRINT("MARK\n");
        ++ret->openCnt;
    }

    return ret;
}

Result iNode_close(iNode* iNode, fsEntryDesc* desc) {
    SuperBlock* superBlock = iNode->superBlock;
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Index64 key = desc->identifier.type == FS_ENTRY_TYPE_DEVICE ? desc->device + superBlockBlockDevice->device.capacity : desc->dataRange.begin >> superBlockBlockDevice->device.granularity;
    if (--iNode->openCnt == 0) {
        if (iNode_removeFromOpened(&superBlock->openedInode, key) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
        
        if (superBlock_rawCloseInode(superBlock, iNode) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
        memory_free(iNode);
    }

    return RESULT_SUCCESS;
}