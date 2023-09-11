#if !defined(__VOLUME_H)
#define __VOLUME_H

#include<driver/disk.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<lib/singlyLinkedList.h>

#define VOLUME_FLAG_USED        FLAG8(0)    //Is this volume used
#define VOLUME_FLAG_BOOTABLE    FLAG8(1)    //Is this volume bootable
#define VOLUME_FLAG_PHYSICAL    FLAG8(2)    //Is this volume a whole physical driver(If not, this volume is a partition)(For now)

RECURSIVE_REFER_STRUCT(Volume) {
    int                         drive;

    Index64                     sectorBegin;
    Size                        sectorNum;
    Uint16                      bytePerSector;

    Flags8                      flags;

    Volume*                     parent;
    Uint8                       childNum;
    union {
        SinglyLinkedList        children;
        SinglyLinkedListNode    sibling;
    };

    void*                       fileSystem;
};

Volume* scanVolume(int drive);

Result rawVolumeReadSectors(Volume* v, void* buffer, Index64 begin, Size n);

Result rawVolumeWriteSectors(Volume* v, const void* buffer, Index64 begin, Size n);

Volume* volumeGetChild(Volume* v, Index8 childIndex);

#endif // __VOLUME_H
