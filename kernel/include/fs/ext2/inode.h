#if !defined(__FS_EXT2_INODE_H)
#define __FS_EXT2_INODE_H

typedef struct EXT2inode EXT2inode;
typedef struct EXT2inodeOSspecific2Linux EXT2inodeOSspecific2Linux;

#include<fs/ext2/ext2.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<debug.h>

typedef struct EXT2inode {
#define EXT2_INODE_TYPE_MASK                    0xF000
#define EXT2_INDEO_GET_TYPE(__TYPE_PERMISSION)  TRIM_VAL(__TYPE_PERMISSION, EXT2_INODE_TYPE_MASK)
#define EXT2_INODE_TYPE_FIFO                    0x1000
#define EXT2_INODE_TYPE_CHARACTER_DEVICE        0x2000
#define EXT2_INODE_TYPE_DIRECTORY               0x4000
#define EXT2_INODE_TYPE_BLOCK_DEVICE            0x6000
#define EXT2_INODE_TYPE_REGULAR                 0x8000
#define EXT2_INODE_TYPE_SYMBOLIC_LINK           0xA000
#define EXT2_INODE_TYPE_SOCKET                  0xC000
#define EXT2_INODE_PERMISSION_LEVEL_OTHER       0
#define EXT2_INODE_PERMISSION_LEVEL_GROUP       1
#define EXT2_INODE_PERMISSION_LEVEL_USER        2
#define EXT2_INODE_PERMISSION_EXECUTE(__LEVEL)  FLAG16((3 * __LEVEL))
#define EXT2_INODE_PERMISSION_WRITE(__LEVEL)    FLAG16((3 * __LEVEL) + 1)
#define EXT2_INODE_PERMISSION_READ(__LEVEL)     FLAG16((3 * __LEVEL) + 2)
#define EXT2_INODE_PERMISSION_STICKY            FLAG16(9)
#define EXT2_INODE_PERMISSION_GROUP_ID_SET      FLAG16(10)
#define EXT2_INODE_PERMISSION_USER_ID_SET       FLAG16(11)
    Flags16 typePermission;
    Uint16  userID;
    Uint32  l32Size;
    Uint32  lastAccessTime;
    Uint32  createTime;
    Uint32  modificationTime;
    Uint32  deletionTime;
    Uint16  groupID;
    Uint16  hardLinkCnt;
    Uint32  sectorCnt;
#define EXT2_INODE_FALGS_SECURE_DELETION        FLAG32(0)   //Not used, for real
#define EXT2_INODE_FALGS_DELETION_KEEP_COPY     FLAG32(1)   //Not used, for real
#define EXT2_INODE_FALGS_FILE_COMPRESSION       FLAG32(2)   //Not used, for real
#define EXT2_INODE_FALGS_SYNC_UPDATE            FLAG32(3)
#define EXT2_INODE_FALGS_IMMUTABLE_FILE         FLAG32(4)   //Epuals to READ ONLY
#define EXT2_INODE_FALGS_APPEND_ONLY            FLAG32(5)
#define EXT2_INODE_FALGS_NOT_FOR_DUMP           FLAG32(6)
#define EXT2_INODE_FALGS_NO_LAST_ACCESS         FLAG32(7)
#define EXT2_INODE_FALGS_HASH_INDEX_DIRECTORY   FLAG32(16)
#define EXT2_INODE_FALGS_AFS_DIRECTORY          FLAG32(17)
#define EXT2_INODE_FALGS_JOURNAL_FILE_DATA      FLAG32(18)
    Flags32 flags;
    Uint8   osSpecific1[4]; //Reserved for linux implementation
    Index32 blockPtrL0[12];
    Index32 blockPtrL1;
    Index32 blockPtrL2;
    Index32 blockPtrL3;
    Uint32  generation;
    Flags32 extendedAttribute;
    Uint32  h32Size;
    Index32 fragmentIndex;
    Uint8   osSpecific2[12];
} __attribute__((packed)) EXT2inode;

DEBUG_ASSERT_COMPILE(sizeof(EXT2inode) == 128);

typedef struct EXT2inodeOSspecific2Linux {
    Uint8   fragmentNum;
    Uint8   fragmentSize;
    Uint16  reserved1;
    Uint16  h16userID;
    Uint16  h16groupID;
    Uint32  reserved2;
} __attribute__((packed)) EXT2inodeOSspecific2Linux;

DEBUG_ASSERT_COMPILE(sizeof(EXT2inodeOSspecific2Linux) == 12);

typedef void (*ext2InodeInterateFunc)(Index32 blockIndex, void* args);

void ext2Inode_iterateBlockRange(EXT2inode* inode, EXT2fscore* fscore, Index64 begin, Size n, ext2InodeInterateFunc func, void* args);

void ext2Inode_truncateTable(EXT2inode* inode, EXT2fscore* fscore, Index64 oldSize, Index64 newSize);

void ext2Inode_expandTable(EXT2inode* inode, ID inodeID, EXT2fscore* fscore, Index64 oldSize, Index64 newSize);

#endif // __FS_EXT2_INODE_H
