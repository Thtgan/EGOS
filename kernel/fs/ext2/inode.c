#include<fs/ext2/inode.h>

#include<devices/blockDevice.h>
#include<fs/ext2/ext2.h>
#include<fs/ext2/vnode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<algorithms.h>
#include<debug.h>

void ext2Inode_iterateBlockRange(EXT2inode* inode, EXT2fscore* fscore, Index64 begin, Size n, ext2InodeInterateFunc func, void* args) {
    EXT2SuperBlock* superblock = fscore->superBlock;
    
    Index64 currentIndex = begin;
    Size remainingBlockN = n;
    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
    BlockDevice* blockDevice = fscore->fscore.blockDevice;
    Size blockDeviceSize = blockSize / POWER_2(blockDevice->device.granularity);
    Size mappingEntryNum = blockSize / sizeof(Index32);

    Size L0capacity = 12;
    Size L1tableRange = mappingEntryNum;
    Size L1capacity = L0capacity + L1tableRange;
    Size L2tableRange = L1tableRange * mappingEntryNum;
    Size L2capacity = L1capacity + L2tableRange;
    Size L3tableRange = L1tableRange * mappingEntryNum;
    Size L3capacity = L2capacity + L3tableRange;

    Index32 translatedIndex = 0;
    while (remainingBlockN > 0 && currentIndex < L0capacity) {
        translatedIndex = inode->blockPtrL0[currentIndex];

        func(translatedIndex, args);

        ++currentIndex;
        --remainingBlockN;
    }

    if (remainingBlockN == 0) {
        return;
    }

    Index32 mappingIndex1 = (begin - L0capacity) % L1tableRange, mappingIndex2 = (begin - L1capacity) % L2tableRange, mappingIndex3 = (begin - L2capacity) % L3tableRange;

    Uint8 mappingTableBuffer1[blockSize];
    Index32* mappingTable1 = (Index32*)mappingTableBuffer1;

    if (currentIndex < L1capacity) {
        blockDevice_readBlocks(blockDevice, inode->blockPtrL1 * blockDeviceSize, mappingTableBuffer1, blockDeviceSize);
    }

    while (remainingBlockN > 0 && currentIndex < L1capacity) {
        translatedIndex = mappingTable1[mappingIndex1];

        func(translatedIndex, args);
        
        ++currentIndex;
        --remainingBlockN;
        ++mappingIndex1;
    }

    if (remainingBlockN == 0) {
        return;
    }

    if (mappingIndex1 == mappingEntryNum) {
        mappingIndex1 = 0;
    }

    Uint8 mappingTableBuffer2[blockSize];
    Index32* mappingTable2 = (Index32*)mappingTableBuffer2;

    if (currentIndex < L2capacity) {
        blockDevice_readBlocks(blockDevice, inode->blockPtrL2 * blockDeviceSize, mappingTableBuffer2, blockDeviceSize);
    }

    while (remainingBlockN > 0 && currentIndex < L2capacity) {
        Index32 translatedIndex1 = mappingTable2[mappingIndex2] * blockDeviceSize;

        blockDevice_readBlocks(blockDevice, translatedIndex1, mappingTableBuffer1, blockDeviceSize);
        
        while (remainingBlockN > 0 && mappingIndex1 < mappingEntryNum) {
            translatedIndex = mappingTable1[mappingIndex1];

            func(translatedIndex, args);
            
            ++currentIndex;
            --remainingBlockN;
            ++mappingIndex1;
        }

        mappingIndex1 = 0;

        ++mappingIndex2;
    }

    if (remainingBlockN == 0) {
        return;
    }

    if (mappingIndex2 == mappingEntryNum) {
        mappingIndex2 = 0;
    }

    Uint8 mappingTableBuffer3[blockSize];
    Index32* mappingTable3 = (Index32*)mappingTableBuffer3;

    if (currentIndex < L3capacity) {
        blockDevice_readBlocks(blockDevice, inode->blockPtrL3 * blockDeviceSize, mappingTableBuffer3, blockDeviceSize);
    }

    while (remainingBlockN > 0 && currentIndex < L3capacity) {
        Index32 translatedIndex2 = mappingTable3[mappingIndex3] * blockDeviceSize;

        blockDevice_readBlocks(blockDevice, translatedIndex2, mappingTableBuffer2, blockDeviceSize);
        
        mappingIndex2 = 0;
        while (remainingBlockN > 0 && mappingIndex2 < mappingEntryNum) {
            Index32 translatedIndex1 = mappingTable2[mappingIndex2] * blockDeviceSize;

            blockDevice_readBlocks(blockDevice, translatedIndex1, mappingTableBuffer1, blockDeviceSize);
            
            while (remainingBlockN > 0 && mappingIndex1 < mappingEntryNum) {
                translatedIndex = mappingTable1[mappingIndex1];

                func(translatedIndex, args);
                
                ++currentIndex;
                --remainingBlockN;
                ++mappingIndex1;
            }

            mappingIndex1 = 0;

            ++mappingIndex2;
        }
        
        mappingIndex2 = 0;

        ++mappingIndex3;
    }
}

void ext2Inode_truncateTable(EXT2inode* inode, EXT2fscore* fscore, Index64 oldSize, Index64 newSize) {
    EXT2SuperBlock* superblock = fscore->superBlock;
    
    Index64 currentIndex = newSize;
    Size remainingBlockN = oldSize - newSize;
    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
    BlockDevice* blockDevice = fscore->fscore.blockDevice;
    Size blockDeviceSize = blockSize / POWER_2(blockDevice->device.granularity);
    Size mappingEntryNum = blockSize / sizeof(Index32);

    DEBUG_ASSERT_SILENT(oldSize > newSize && oldSize == DIVIDE_ROUND_UP(inode->sectorCnt, blockDeviceSize));

    Size L0capacity = 12;
    Size L1tableRange = mappingEntryNum;
    Size L1capacity = L0capacity + L1tableRange;
    Size L2tableRange = L1tableRange * mappingEntryNum;
    Size L2capacity = L1capacity + L2tableRange;
    Size L3tableRange = L1tableRange * mappingEntryNum;
    Size L3capacity = L2capacity + L3tableRange;

    if (currentIndex < L0capacity) {
        Index32 midIndex = currentIndex - L0capacity;

        Size L0truncated = algorithms_umin64(L0capacity - currentIndex, remainingBlockN);

        Index32 midIndexEnd = midIndex + L0truncated;
        while (midIndex < midIndexEnd) {
            ext2fscore_freeBlock(fscore, inode->blockPtrL0[midIndex]);
            inode->blockPtrL0[midIndex++] = 0;
        }

        currentIndex += L0truncated;
        remainingBlockN -= L0truncated;
    }

    if (remainingBlockN == 0) {
        return;
    }

    Uint8 L1mappingTableBuffer[blockSize];
    Index32* L1mappingTable = (Index32*)L1mappingTableBuffer;

    if (currentIndex < L1capacity) {
        DEBUG_ASSERT_SILENT(currentIndex >= L0capacity);
        blockDevice_readBlocks(blockDevice, inode->blockPtrL1 * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);
        Index32 midIndex = currentIndex - L0capacity;

        Size L1truncated = algorithms_umin64(L1tableRange - midIndex, remainingBlockN);

        Index32 midIndexEnd = midIndex + L1truncated;
        while (midIndex < midIndexEnd) {
            ext2fscore_freeBlock(fscore, L1mappingTable[midIndex]);
            L1mappingTable[midIndex++] = 0;
        }

        blockDevice_writeBlocks(blockDevice, inode->blockPtrL1 * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);
        
        if (L1truncated == L1tableRange) {
            ext2fscore_freeBlock(fscore, inode->blockPtrL1);
            inode->blockPtrL1 = 0;
        }

        currentIndex += L1truncated;
        remainingBlockN -= L1truncated;
    }

    if (remainingBlockN == 0) {
        return;
    }

    Uint8 L2mappingTableBuffer[blockSize];
    Index32* L2mappingTable = (Index32*)L2mappingTableBuffer;

    if (currentIndex < L2capacity) {
        DEBUG_ASSERT_SILENT(currentIndex >= L1capacity);
        blockDevice_readBlocks(blockDevice, inode->blockPtrL2 * blockDeviceSize, L2mappingTableBuffer, blockDeviceSize);

        Index32 subCurrentIndex = currentIndex - L1capacity;
        Index32 midIndex1 = subCurrentIndex / L1tableRange, midIndex2 = subCurrentIndex % L1tableRange;

        Size L2truncated = algorithms_umin64(L2tableRange - subCurrentIndex, remainingBlockN);
        bool modified = false;

        Index32 midIndexEnd1 = DIVIDE_ROUND_UP(subCurrentIndex + L2truncated, L1tableRange);
        while (midIndex1 < midIndexEnd1) {
            Index32 mappingTo = L2mappingTable[midIndex1];
            blockDevice_readBlocks(blockDevice, mappingTo * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);

            Size L1truncated = algorithms_umin64(L1tableRange - midIndex2, remainingBlockN);

            Index32 midIndexEnd2 = midIndex2 + L1truncated;

            while (midIndex2 < midIndexEnd2) {
                ext2fscore_freeBlock(fscore, L1mappingTable[midIndex2]);
                L1mappingTable[midIndex2++] = 0;
            }
            midIndex2 = 0;

            blockDevice_writeBlocks(blockDevice, mappingTo * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);
        
            if (L1truncated == L1tableRange) {
                ext2fscore_freeBlock(fscore, mappingTo);
                L2mappingTable[midIndex1] = 0;
                modified = true;
            }

            ++midIndex1;

            currentIndex += L1truncated;
            remainingBlockN -= L1truncated;
        }

        if (modified) {
            blockDevice_writeBlocks(blockDevice, inode->blockPtrL2 * blockDeviceSize, L2mappingTableBuffer, blockDeviceSize);
        }

        if (L2truncated == L2tableRange) {
            ext2fscore_freeBlock(fscore, inode->blockPtrL2);
            inode->blockPtrL2 = 0;
        }
    }


    if (remainingBlockN == 0) {
        return;
    }

    Uint8 L3mappingTableBuffer[blockSize];
    Index32* L3mappingTable = (Index32*)L3mappingTableBuffer;

    if (currentIndex < L3capacity) {
        DEBUG_ASSERT_SILENT(currentIndex >= L2capacity);
        blockDevice_readBlocks(blockDevice, inode->blockPtrL3 * blockDeviceSize, L3mappingTableBuffer, blockDeviceSize);

        Index32 subCurrentIndex = currentIndex - L2capacity;
        Index32 midIndex1 = subCurrentIndex / L2tableRange, midIndex2 = (subCurrentIndex / L1tableRange) % mappingEntryNum, midIndex3 = subCurrentIndex % L1tableRange;

        Size L3truncated = algorithms_umin64(L3tableRange - subCurrentIndex, remainingBlockN);
        bool modified1 = false;

        Index32 midIndexEnd1 = DIVIDE_ROUND_UP(subCurrentIndex + L3truncated, L2tableRange);

        while (midIndex1 < midIndexEnd1) {
            Index32 mappingTo1 = L3mappingTable[midIndex1];
            blockDevice_readBlocks(blockDevice, mappingTo1 * blockDeviceSize, L2mappingTableBuffer, blockDeviceSize);

            Index32 subSubCurrentIndex = midIndex2 * L1tableRange + midIndex3;
            Size L2truncated = algorithms_umin64(L2tableRange - subSubCurrentIndex, remainingBlockN);
            bool modified2 = false;

            Index32 midIndexEnd2 = DIVIDE_ROUND_UP(subSubCurrentIndex + L2truncated, L1tableRange);
            while (midIndex2 < midIndexEnd1) {
                Index32 mappingTo2 = L2mappingTable[midIndex1];
                blockDevice_readBlocks(blockDevice, mappingTo2 * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);

                Size L1truncated = algorithms_umin64(L1tableRange - midIndex3, remainingBlockN);

                Index32 midIndexEnd3 = midIndex3 + L1truncated;
                while (midIndex3 < midIndexEnd3) {
                    ext2fscore_freeBlock(fscore, L1mappingTable[midIndex3]);
                    L1mappingTable[midIndex3++] = 0;
                }
                midIndex3 = 0;

                blockDevice_writeBlocks(blockDevice, mappingTo2 * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);
        
                if (L1truncated == L1tableRange) {
                    ext2fscore_freeBlock(fscore, mappingTo2);
                    L2mappingTable[midIndex1] = 0;
                    modified2 = true;
                }
                
                ++midIndex2;

                currentIndex += L1truncated;
                remainingBlockN -= L1truncated;
            }
            midIndex2 = 0;

            if (modified2) {
                blockDevice_writeBlocks(blockDevice, mappingTo1 * blockDeviceSize, L2mappingTableBuffer, blockDeviceSize);
            }

            if (L2truncated == L2tableRange) {
                ext2fscore_freeBlock(fscore, mappingTo1);
                L3mappingTable[midIndex1] = 0;
                modified1 = true;
            }
        }

        if (modified1) {
            blockDevice_writeBlocks(blockDevice, inode->blockPtrL3 * blockDeviceSize, L3mappingTableBuffer, blockDeviceSize);
        }

        if (L3truncated == L3tableRange) {
            ext2fscore_freeBlock(fscore, inode->blockPtrL3);
            inode->blockPtrL3 = 0;
        }
    }
}

void ext2Inode_expandTable(EXT2inode* inode, ID inodeID, EXT2fscore* fscore, Index64 oldSize, Index64 newSize) {
    EXT2SuperBlock* superblock = fscore->superBlock;
    
    Index64 currentIndex = newSize;
    Size remainingBlockN = oldSize - newSize;
    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
    BlockDevice* blockDevice = fscore->fscore.blockDevice;
    Size blockDeviceSize = blockSize / POWER_2(blockDevice->device.granularity);
    Size mappingEntryNum = blockSize / sizeof(Index32);

    Index32 preferredBlockGroup = ext2SuperBlock_inodeID2BlockGroupIndex(superblock, inodeID);

    DEBUG_ASSERT_SILENT(oldSize > newSize && oldSize == DIVIDE_ROUND_UP(inode->sectorCnt, blockDeviceSize));

    Size L0capacity = 12;
    Size L1tableRange = mappingEntryNum;
    Size L1capacity = L0capacity + L1tableRange;
    Size L2tableRange = L1tableRange * mappingEntryNum;
    Size L2capacity = L1capacity + L2tableRange;
    Size L3tableRange = L1tableRange * mappingEntryNum;
    Size L3capacity = L2capacity + L3tableRange;

    if (currentIndex < L0capacity) {
        Index32 midIndex = currentIndex - L0capacity;

        Size L0expanded = algorithms_umin64(L0capacity - currentIndex, remainingBlockN);

        Index32 midIndexEnd = midIndex + L0expanded;
        while (midIndex < midIndexEnd) {
            inode->blockPtrL0[midIndex++] = ext2fscore_allocateBlock(fscore, preferredBlockGroup);
        }

        currentIndex += L0expanded;
        remainingBlockN -= L0expanded;
    }

    if (remainingBlockN == 0) {
        return;
    }

    Uint8 L1mappingTableBuffer[blockSize];
    Index32* L1mappingTable = (Index32*)L1mappingTableBuffer;

    if (currentIndex < L1capacity) {
        DEBUG_ASSERT_SILENT(currentIndex >= L0capacity);
        Index32 midIndex = currentIndex - L0capacity;

        Size L1expanded = algorithms_umin64(L1tableRange - midIndex, remainingBlockN);

        if (L1expanded == L1tableRange) {
            inode->blockPtrL1 = ext2fscore_allocateBlock(fscore, preferredBlockGroup);
            memory_memset(L1mappingTableBuffer, 0, blockSize);
        } else {
            blockDevice_readBlocks(blockDevice, inode->blockPtrL1 * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);
        }

        Index32 midIndexEnd = midIndex + L1expanded;
        while (midIndex < midIndexEnd) {
            ext2fscore_freeBlock(fscore, L1mappingTable[midIndex]);
            L1mappingTable[midIndex++] = 0;
        }

        blockDevice_writeBlocks(blockDevice, inode->blockPtrL1 * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);

        currentIndex += L1expanded;
        remainingBlockN -= L1expanded;
    }

    if (remainingBlockN == 0) {
        return;
    }

    Uint8 L2mappingTableBuffer[blockSize];
    Index32* L2mappingTable = (Index32*)L2mappingTableBuffer;

    if (currentIndex < L2capacity) {
        DEBUG_ASSERT_SILENT(currentIndex >= L1capacity);

        Index32 subCurrentIndex = currentIndex - L1capacity;
        Index32 midIndex1 = subCurrentIndex / L1tableRange, midIndex2 = subCurrentIndex % L1tableRange;

        Size L2expanded = algorithms_umin64(L2tableRange - subCurrentIndex, remainingBlockN);
        bool modified = false;

        if (L2expanded == L2tableRange) {
            inode->blockPtrL2 = ext2fscore_allocateBlock(fscore, preferredBlockGroup);
            memory_memset(L2mappingTableBuffer, 0, blockSize);
        } else {
            blockDevice_readBlocks(blockDevice, inode->blockPtrL2 * blockDeviceSize, L2mappingTableBuffer, blockDeviceSize);
        }

        Index32 midIndexEnd1 = DIVIDE_ROUND_UP(subCurrentIndex + L2expanded, L1tableRange);
        while (midIndex1 < midIndexEnd1) {
            Index32 mappingTo = L2mappingTable[midIndex1];

            Size L1expanded = algorithms_umin64(L1tableRange - midIndex2, remainingBlockN);

            if (L1expanded == L1tableRange) {
                L2mappingTable[midIndex1] = ext2fscore_allocateBlock(fscore, preferredBlockGroup);
                memory_memset(L1mappingTableBuffer, 0, blockSize);
                modified = true;
            } else {
                blockDevice_readBlocks(blockDevice, mappingTo * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);
            }

            Index32 midIndexEnd2 = midIndex2 + L1expanded;

            while (midIndex2 < midIndexEnd2) {
                L1mappingTable[midIndex2++] = ext2fscore_allocateBlock(fscore, preferredBlockGroup);
            }
            midIndex2 = 0;

            blockDevice_writeBlocks(blockDevice, mappingTo * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);

            ++midIndex1;

            currentIndex += L1expanded;
            remainingBlockN -= L1expanded;
        }

        if (modified) {
            blockDevice_writeBlocks(blockDevice, inode->blockPtrL2 * blockDeviceSize, L2mappingTableBuffer, blockDeviceSize);
        }
    }


    if (remainingBlockN == 0) {
        return;
    }

    Uint8 L3mappingTableBuffer[blockSize];
    Index32* L3mappingTable = (Index32*)L3mappingTableBuffer;

    if (currentIndex < L3capacity) {
        DEBUG_ASSERT_SILENT(currentIndex >= L2capacity);

        Index32 subCurrentIndex = currentIndex - L2capacity;
        Index32 midIndex1 = subCurrentIndex / L2tableRange, midIndex2 = (subCurrentIndex / L1tableRange) % mappingEntryNum, midIndex3 = subCurrentIndex % L1tableRange;

        Size L3expanded = algorithms_umin64(L3tableRange - subCurrentIndex, remainingBlockN);
        bool modified1 = false;

        if (L3expanded == L3tableRange) {
            inode->blockPtrL3 = ext2fscore_allocateBlock(fscore, preferredBlockGroup);
            memory_memset(L3mappingTableBuffer, 0, blockSize);
        } else {
            blockDevice_readBlocks(blockDevice, inode->blockPtrL3 * blockDeviceSize, L3mappingTableBuffer, blockDeviceSize);
        }

        Index32 midIndexEnd1 = DIVIDE_ROUND_UP(subCurrentIndex + L3expanded, L2tableRange);

        while (midIndex1 < midIndexEnd1) {
            Index32 mappingTo1 = L3mappingTable[midIndex1];
            
            Index32 subSubCurrentIndex = midIndex2 * L1tableRange + midIndex3;
            Size L2expanded = algorithms_umin64(L2tableRange - subSubCurrentIndex, remainingBlockN);
            bool modified2 = false;

            if (L2expanded == L2tableRange) {
                L3mappingTable[midIndex1] = ext2fscore_allocateBlock(fscore, preferredBlockGroup);
                memory_memset(L2mappingTableBuffer, 0, blockSize);
                modified1 = true;
            } else {
                blockDevice_readBlocks(blockDevice, mappingTo1 * blockDeviceSize, L2mappingTableBuffer, blockDeviceSize);
            }

            Index32 midIndexEnd2 = DIVIDE_ROUND_UP(subSubCurrentIndex + L2expanded, L1tableRange);
            while (midIndex2 < midIndexEnd1) {
                Index32 mappingTo2 = L2mappingTable[midIndex1];

                Size L1expanded = algorithms_umin64(L1tableRange - midIndex3, remainingBlockN);

                if (L1expanded == L1tableRange) {
                    L2mappingTable[midIndex1] = ext2fscore_allocateBlock(fscore, preferredBlockGroup);
                    memory_memset(L1mappingTableBuffer, 0, blockSize);
                    modified2 = true;
                } else {
                    blockDevice_readBlocks(blockDevice, mappingTo2 * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);
                }

                Index32 midIndexEnd3 = midIndex3 + L1expanded;
                while (midIndex3 < midIndexEnd3) {
                    L1mappingTable[midIndex3++] = ext2fscore_allocateBlock(fscore, preferredBlockGroup);;
                }
                midIndex3 = 0;

                blockDevice_writeBlocks(blockDevice, mappingTo2 * blockDeviceSize, L1mappingTableBuffer, blockDeviceSize);
                
                ++midIndex2;

                currentIndex += L1expanded;
                remainingBlockN -= L1expanded;
            }
            midIndex2 = 0;

            if (modified2) {
                blockDevice_writeBlocks(blockDevice, mappingTo1 * blockDeviceSize, L2mappingTableBuffer, blockDeviceSize);
            }
        }

        if (modified1) {
            blockDevice_writeBlocks(blockDevice, inode->blockPtrL3 * blockDeviceSize, L3mappingTableBuffer, blockDeviceSize);
        }
    }
}