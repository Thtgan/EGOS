#if !defined(__FS_FS_H)
#define __FS_FS_H

typedef enum {
    FS_TYPE_FAT32,
    FS_TYPE_DEVFS,
    FS_TYPE_NUM,
    FS_TYPE_UNKNOWN
} FStype;

typedef struct FS FS;
typedef struct FS_fileStat FS_fileStat;

#include<devices/blockDevice.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<time/time.h>

/**
 * Basic structure of EGOS VFS
 * 
 *                                                       ┌─────┐    
 *                           ┌──────┐Root┌───────────┐   │Mount│┐   
 *                           │fsNode│◄───┤Super Block├──►└─────┘│┐  
 *                           └──┬───┘    └───────────┘    └─────┘│  
 *                              │              ▲           └─┬───┘  
 *                              │              │             │      
 *                              ▼              │             ▼      
 * ┌────────────┐ Points To  ┌──────┐       ┌──┴──┐      ┌───────┐  
 * │fsIdentifier├────────┬──►│fsNode│◄──────┤iNode│◄─────┤fsEntry│┐ 
 * └────────────┘        │   └──┬───┘       └─────┘      └───────┘│┐
 * ┌────────────┐        │      │                         └───────┘│
 * │fsIdentifier├────────┘      ▼                          └───────┘
 * └────────────┘           ┌──────┐                                
 *                          │fsNode│┐                               
 *                          └──────┘│┬─────►...                     
 *                           └──────┘│                              
 *                            └──────┘                              
 */

typedef struct FS {
    SuperBlock*     superBlock;
    ConstCstring    name;
    FStype          type;
} FS;

void fs_init();

/**
 * @brief Check type of file system on device
 * 
 * @param device Device to check
 * @return FileSystemTypes Type of file system
 */
FStype fs_checkType(BlockDevice* device);

/**
 * @brief Open file system on device, return from buffer if file system is open, sets errorcode to indicate error
 * 
 * @param device Device to open
 * @return FS* File system, NULL if error happens
 */
void fs_open(FS* fs, BlockDevice* device);

/**
 * @brief Close a opened file system, cannot close a closed file system, sets errorcode to indicate error
 * 
 * @param system File system
 */
void fs_close(FS* fs);

//Non-FS codes should use these functions

File* fs_fileOpen(ConstCstring path, FCNTLopenFlags flags);

void fs_fileClose(File* file);

void fs_fileRead(File* file, void* buffer, Size n);

void fs_fileWrite(File* file, const void* buffer, Size n);

#define FS_FILE_SEEK_BEGIN      0
#define FS_FILE_SEEK_CURRENT    1
#define FS_FILE_SEEK_END        2

Index64 fs_fileSeek(File* file, Int64 offset, Uint8 begin);

typedef struct FS_fileStat {
    Uint64 deviceID;
    Uint64 inodeID;
    Uint64 nLink;
    Uint32 mode;
#define FS_FILE_STAT_MODE_GET_TYPE(__MODE)          TRIM_VAL(EXTRACT_VAL(__MODE, 32, 16, 24), 0x17);
#define FS_FILE_STAT_MODE_SET_TYPE(__MODE, __TYPE)  ((__MODE) = (CLEAR_VAL_RANGE((__MODE), 32, 16, 24) | VAL_LEFT_SHIFT((Uint32)(__TYPE), 16)))
#define FS_FILE_STAT_MODE_TYPE_FIFO             0x01    //TODO: Not used
#define FS_FILE_STAT_MODE_TYPE_CHAR_DEVICE      0x02
#define FS_FILE_STAT_MODE_TYPE_DIRECTORY        0x04
#define FS_FILE_STAT_MODE_TYPE_BLOCK_DEVICE     0x06
#define FS_FILE_STAT_MODE_TYPE_REGULAR_FILE     0x10
#define FS_FILE_STAT_MODE_TYPE_SYMBOLIC_LINK    0x12    //TODO: Not used
#define FS_FILE_STAT_MODE_TYPE_SOCKET           0x14    //TODO: Not used
    Uint32 uid;
    Uint32 gid;
    Uint32 reserved0;
    Uint64 rDevice;
    Int64 size;
    Int64 blockSize;
    Int64 blocks;
    Timestamp accessTime;
    Timestamp modifyTime;
    Timestamp createTime;
    Uint64 reserved1[3];
} FS_fileStat;

void fs_fileStat(File* file, FS_fileStat* stat);

#endif // __FS_FS_H
