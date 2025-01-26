#include<fs/inode.h>

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<error.h>

ID iNode_generateID(fsEntryDesc* desc) {
    return desc->type == FS_ENTRY_TYPE_DEVICE ? (FLAG64(63) | desc->device) : desc->dataRange.begin;    //TODO: Ugly method to mark device
}

iNode* iNode_openFromOpened(HashTable* table, Index64 blockIndex) {
    HashChainNode* found = hashTable_find(table, (Object)blockIndex);
    return found == NULL ? NULL : HOST_POINTER(found, iNode, openedNode);
}

void iNode_addToOpened(HashTable* table, iNode* iNode, Index64 blockIndex) {
    hashTable_insert(table, (Object)blockIndex, &iNode->openedNode);    //Error passthrough
}

void iNode_removeFromOpened(HashTable* table, Index64 blockIndex) {
    hashTable_delete(table, (Object)blockIndex);    //Error passthrough
}

iNode* iNode_open(SuperBlock* superBlock, fsEntryDesc* desc) {
    iNode* ret = NULL;
    
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Index64 key = iNode_generateID(desc);
    ret = iNode_openFromOpened(&superBlock->openedInode, key);
    if (ret == NULL) {
        ret = memory_allocate(sizeof(iNode));
        if (ret == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        superBlock_rawOpenInode(superBlock, ret, desc);
        ERROR_GOTO_IF_ERROR(0);

        iNode_addToOpened(&superBlock->openedInode, ret, key);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        ++ret->openCnt;
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (ret != NULL) {
        memory_free(ret);
    }

    return NULL;
}

void iNode_close(iNode* iNode) {
    SuperBlock* superBlock = iNode->superBlock;
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    if (--iNode->openCnt == 0) {
        iNode_removeFromOpened(&superBlock->openedInode, iNode->iNodeID);
        ERROR_GOTO_IF_ERROR(0);
        
        superBlock_rawCloseInode(superBlock, iNode);
        ERROR_GOTO_IF_ERROR(0);
        
        memory_free(iNode);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}