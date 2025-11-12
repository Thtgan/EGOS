#if !defined(__FS_EXT2_EXT2_H)
#define __FS_EXT2_EXT2_H

typedef struct EXT2SuperBlock EXT2SuperBlock;
typedef struct EXT2fscore EXT2fscore;

#include<devices/blockDevice.h>
#include<fs/ext2/blockGroup.h>
#include<fs/fs.h>
#include<fs/fscore.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>

typedef struct EXT2SuperBlock {
    Uint32 totalInodeNum;
    Uint32 totalBlockNum;
    Uint32 superuserBlockNum;
    Uint32 unallocatedBlockNum;
    Uint32 unallocatedInodeNum;
    Index32 superBlockBlock;
#define EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE_SHIFT(__SHIFT)    ((__SHIFT) + 10)
#define EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(__SHIFT)          POWER_2(EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE_SHIFT(__SHIFT))
    Uint32 blockSizeShift;
#define EXT2_SUPERBLOCK_IN_STORAGE_GET_FRAGMENT_SIZE_SHIFT(__SHIFT) ((__SHIFT) + 10)
#define EXT2_SUPERBLOCK_IN_STORAGE_GET_FRAGMENT_SIZE(__SHIFT)       POWER_2(EXT2_SUPERBLOCK_IN_STORAGE_GET_FRAGMENT_SIZE_SHIFT(__SHIFT))
    Uint32 fragmentSizeShift;
    Uint32 blockGroupBlockNum;
    Uint32 blockGroupFragmentNum;
    Uint32 blockGroupInodeNum;
    Uint32 lastMountTime;
    Uint32 lastWrittenTime;
    Uint16 mountTimeSinceCheck;     //Since its last consistency check
    Uint16 mountTimeBeforeCheck;    //Before a consistency check (fsck) must be done
#define EXT2_SUPERBLOCK_IN_STORAGE_SIGNATURE    0xEF53
    Uint16 signature;
#define EXT2_SUPERBLOCK_IN_STORAGE_STATE_CLEAN  1
#define EXT2_SUPERBLOCK_IN_STORAGE_STATE_ERROR  2
    Uint16 fsState;
#define EXT2_SUPERBLOCK_IN_STORAGE_WHEN_ERROR_IGNORE            1
#define EXT2_SUPERBLOCK_IN_STORAGE_WHEN_ERROR_REMOUNT_READ_ONLY 2
#define EXT2_SUPERBLOCK_IN_STORAGE_WHEN_ERROR_PANIC             3
    Uint16 whenError;
    Uint16 versionMinor;
    Uint32 lastCheckTime;
    Uint32 checkInterval;
#define EXT2_SUPERBLOCK_IN_STORAGE_OS_ID_LINUX      0
#define EXT2_SUPERBLOCK_IN_STORAGE_OS_ID_GNU_HURD   1
#define EXT2_SUPERBLOCK_IN_STORAGE_OS_ID_MASIX      2
#define EXT2_SUPERBLOCK_IN_STORAGE_OS_ID_FREEBSD    3
#define EXT2_SUPERBLOCK_IN_STORAGE_OS_ID_OTHER      4
    Uint32 osID;
    Uint32 versionMajor;
    Uint16 reservedUserID;
    Uint16 reservedGroupID;
    //Beginning of extended fields, available only when versionMajor >= 1
    Uint32 firstInode;
    Uint16 iNodeSizeInByte;
    Uint16 SuperBlockGroup;
#define EXT2_SUPERBLOCK_IN_STORAGE_OPTIMAL_FEATURES_PREALLOCATE_DIRECTORY       FLAG32(0)
#define EXT2_SUPERBLOCK_IN_STORAGE_OPTIMAL_FEATURES_AFS_SERVER_INODE            FLAG32(1)
#define EXT2_SUPERBLOCK_IN_STORAGE_OPTIMAL_FEATURES_HAS_JOURNAL                 FLAG32(2)
#define EXT2_SUPERBLOCK_IN_STORAGE_OPTIMAL_FEATURES_INODE_EXTENDED_ATTRIBUTE    FLAG32(3)
#define EXT2_SUPERBLOCK_IN_STORAGE_OPTIMAL_FEATURES_RESIZE                      FLAG32(4)
#define EXT2_SUPERBLOCK_IN_STORAGE_OPTIMAL_FEATURES_DIRECTORY_HASH_INDEX        FLAG32(4)
    Flags32 optimalFeatures;
#define EXT2_SUPERBLOCK_IN_STORAGE_REUQUIRED_FEATURES_COMPRESSION           FLAG32(0)
#define EXT2_SUPERBLOCK_IN_STORAGE_REUQUIRED_FEATURES_DIRECTORY_ENTRY_TYPE  FLAG32(1)
#define EXT2_SUPERBLOCK_IN_STORAGE_REUQUIRED_FEATURES_REPLAY_JOURNAL        FLAG32(2)
#define EXT2_SUPERBLOCK_IN_STORAGE_REUQUIRED_FEATURES_JOURNAL_DEVICE        FLAG32(3)
    Flags32 requiredFeatures;
#define EXT2_SUPERBLOCK_IN_STORAGE_READONLY_IF_NOT_SUPPORTED_SPARSE_SUPERBLOCK_AND_GDT  FLAG32(0)
#define EXT2_SUPERBLOCK_IN_STORAGE_READONLY_IF_NOT_SUPPORTED_FILE_SIZE_64               FLAG32(1)
#define EXT2_SUPERBLOCK_IN_STORAGE_READONLY_IF_NOT_SUPPORTED_DINARY_TREE_DIRECTORY      FLAG32(2)
    Flags32 readOnlyIfNotSupported;
    char fsID[16];
    char volumeName[16];
    char lastMountPath[64];
    Uint32 compressionAlgorithm;
    Uint8 fileBlockPreallocate;
    Uint8 directoryBlockPreallocate;
    Uint16 unused1;
    Uint8 journalID[16];
    Uint32 journalInode;
    Uint32 journalDevice;
    Uint32 orphanInodeListHead;
    Uint8 unused2[276];
} __attribute__((packed)) EXT2SuperBlock;

DEBUG_ASSERT_COMPILE(sizeof(EXT2SuperBlock) == 512);

static inline Index32 ext2SuperBlock_inodeID2BlockGroupIndex(EXT2SuperBlock* superblock, Index32 inodeID) {
    return (inodeID - 1) / superblock->blockGroupInodeNum;
}

static inline Size ext2SuperBlock_getBlockGroupNum(EXT2SuperBlock* superblock) {
    return DIVIDE_ROUND_UP(superblock->totalBlockNum, superblock->blockGroupBlockNum);
}

static inline Index32 ext2SuperBlock_blockIndexDevice2fs(EXT2SuperBlock* superblock, Size deviceGranularity, Index32 deviceBlockIndex) {
    return (deviceBlockIndex << deviceGranularity) >> EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE_SHIFT(superblock->blockSizeShift);
}

static inline Index32 ext2SuperBlock_blockIndexFS2device(EXT2SuperBlock* superblock, Size deviceGranularity, Index32 blockIndex, Index32 inBlockOffset) {
    return ((blockIndex << EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE_SHIFT(superblock->blockSizeShift)) + inBlockOffset) >> deviceGranularity;
}

static inline Index32 ext2SuperBlock_blockOffsetFS2device(EXT2SuperBlock* superblock, Size deviceGranularity, Index32 blockIndex, Index32 inBlockOffset) {
    return TRIM_VAL_SIMPLE((blockIndex << EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE_SHIFT(superblock->blockSizeShift)) + inBlockOffset, 32, deviceGranularity);
}

typedef struct EXT2fscore {
    FScore                      fscore;
    EXT2SuperBlock*             superBlock;
    Size                        blockGroupNum;
    EXT2blockGroupDescriptor*   blockGroupTables;
} EXT2fscore;

Index32 ext2fscore_allocateBlock(EXT2fscore* fscore, Index32 preferredBlockGroup);

void ext2fscore_freeBlock(EXT2fscore* fscore, Index32 index);

Index32 ext2fscore_allocateInode(EXT2fscore* fscore, Index32 preferredBlockGroup, bool isDirectory);

void ext2fscore_freeInode(EXT2fscore* fscore, Index32 index);

void ext2_init();

bool ext2_checkType(BlockDevice* blockDevice);

void ext2_open(FS* fs, BlockDevice* blockDevice);

void ext2_close(FS* fs);

#endif // __FS_EXT2_EXT2_H
