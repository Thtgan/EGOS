#if !defined(__BLOCK_DEVICE_H)
#define __BLOCK_DEVICE_H

#include<devices/block/blockBuffer.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/hashTable.h>
#include<structs/linkedList.h>
#include<structs/singlyLinkedList.h>

#define DEFAULT_BLOCK_SIZE          512
#define DEFAULT_BLOCK_SIZE_SHIFT    9

#define BLOCK_DEVICE_NAME_LENGTH    31

STRUCT_PRE_DEFINE(BlockDeviceOperation);

STRUCT_PRE_DEFINE(BlockDevice);

typedef struct {
    ConstCstring            name;
    Size                    availableBlockNum;
    Uint8                   bytePerBlockShift;
    BlockDevice*            parent;
    Object                  specificInfo;
    BlockDeviceOperation*   operations;
    bool                    buffered;
} BlockDeviceArgs;

STRUCT_PRIVATE_DEFINE(BlockDevice) {
    char                        name[BLOCK_DEVICE_NAME_LENGTH + 1];
    Size                        availableBlockNum;
    
    Uint8                       bytePerBlockShift;

#define BLOCK_DEVICE_FLAGS_BOOTABLE FLAG8(0)
#define BLOCK_DEVICE_FLAGS_READONLY FLAG8(1)
#define BLOCK_DEVICE_FLAGS_BUFFERED FLAG8(2)
    Flags8                      flags;

    BlockDevice*                parent;
    Uint8                       childNum;
    SinglyLinkedList            children;
    SinglyLinkedListNode        node;
    BlockBuffer*                blockBuffer;

    Object                      specificInfo; //Something to assist the block device working, can be anything
    BlockDeviceOperation*       operations;
    HashChainNode               hashChainNode;
};

STRUCT_PRIVATE_DEFINE(BlockDeviceOperation) {
    /**
     * @brief Read blocks from the block device, sets errorcode to indicate error
     * 
     * @param additionalData Additional data of the block device
     * @param blockIndex Index of the first block to read from
     * @param buffer Buffer to storage the read data
     * @param n Num of blocks to read
     * @return Result Result of the operation
     */
    Result (*readBlocks)(BlockDevice* device, Index64 blockIndex, void* buffer, Size n);

    /**
     * @brief Write blocks to the block device, sets errorcode to indicate error
     * 
     * @param additionalData Additional data of the block device
     * @param blockIndex Index of the first block to write to
     * @param buffer Buffer contains the data to write
     * @param n Num of blocks to write
     * @return Result Result of the operation
     */
    Result (*writeBlocks)(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n);
};

Result initBlockDevice(BlockDevice* device, BlockDeviceArgs* args);

/**
 * @brief Packed function of block device operation, sets errorcode to indicate error
 * 
 * @param device Block device
 * @param blockIndex Index of block to read
 * @param buffer Buffer to write to
 * @param n Num of block(s) to read
 * @return Result Result of the operation
 */
Result blockDeviceReadBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n);

Result blockDeviceWriteBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n);

Result blockDeviceSynchronize(BlockDevice* device);

Result probePartitions(BlockDevice* device);

#endif // __BLOCK_DEVICE_H
