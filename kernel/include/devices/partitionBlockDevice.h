#if !defined(__DEVICES_PARTITIONBLOCKDEVICE_H)
#define __DEVICES_PARTITIONBLOCKDEVICE_H

typedef struct PartitionBlockDevice PartitionBlockDevice;

#include<devices/blockDevice.h>
#include<kit/types.h>

typedef struct PartitionBlockDevice {
    BlockDevice blockDevice;
    Index64 parentBegin;
} PartitionBlockDevice;

void partitionBlockDevice_probePartitions(BlockDevice* parentDevice);

#endif // __DEVICES_PARTITIONBLOCKDEVICE_H
