#if !defined(__DISK_H)
#define __DISK_H

#include<kit/types.h>

#define DISK_BUFFER_SIZE   (1 << 16)

typedef struct {
    Uint16  paramSize;
    Uint16  infoFlags;
    Uint32  cylinderNum;
    Uint32  headNum;
    Uint32  sectorNumPerTrack;
    Uint64  sectorNum;
    Uint16  bytePerSector;
    Uint32  EDD;
} __attribute__((packed)) DiskParams;

Result readDiskParams(int drive, DiskParams* params);

Result rawDiskReadSectors(int drive, void* buffer, Index64 begin, Size n);

Result rawDiskWriteSectors(int drive, const void* buffer, Index64 begin, Size n);

#endif // __DISK_H
