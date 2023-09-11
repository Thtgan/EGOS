#include<fs/volume.h>

#include<driver/disk.h>
#include<fs/fileSystem.h>
#include<kit/types.h>
#include<kit/util.h>
#include<mm/mm.h>

static Result __doScanVolume(int drive, Index64 sectorBegin, Size sectorNum, DiskParams* params, Volume* parent, Volume* ret);

Volume* scanVolume(int drive) {
    DiskParams params;
    if (readDiskParams(drive, &params) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Volume* ret = bMalloc(sizeof(Volume));

    if (ret == NULL || __doScanVolume(drive, 0, params.sectorNum, &params, NULL, ret) == RESULT_FAIL) {
        if (ret != NULL) {
            bFree(ret, sizeof(Volume));
        }

        ret = NULL;
    }

    return ret;
}

Result rawVolumeReadSectors(Volume* v, void* buffer, Index64 begin, Size n) {
    if (v == NULL || n > v->sectorNum) {
        return RESULT_FAIL;
    }

    return rawDiskReadSectors(v->drive, buffer, v->sectorBegin + begin, n);
}

Result rawVolumeWriteSectors(Volume* v, const void* buffer, Index64 begin, Size n) {
    if (v == NULL || n > v->sectorNum) {
        return RESULT_FAIL;
    }

    return rawDiskWriteSectors(v->drive, buffer, v->sectorBegin + begin, n);
}

typedef struct {
    Uint8   driveArttribute;
    Uint8   beginHead;
    Uint8   beginSector : 6;
    Uint16  beginCylinder : 10;
    Uint8   systemID;
    Uint8   endHead;
    Uint8   endSector : 6;
    Uint16  endCylinder : 10;
    Uint32  sectorBegin;
    Uint32  sectorNum;
} __attribute__((packed)) __MBRpartitionEntry;

static Result __doScanVolume(int drive, Index64 sectorBegin, Size sectorNum, DiskParams* params, Volume* parent, Volume* ret) {
    ret->drive          = drive;
    ret->sectorBegin    = sectorBegin;
    ret->sectorNum      = sectorNum;
    ret->bytePerSector  = params->bytePerSector;

    ret->parent         = parent;
    ret->childNum       = 0;

    Flags8 flags = EMPTY_FLAGS;
    if (sectorBegin == 0 && sectorNum == params->sectorNum) {
        SET_FLAG_BACK(flags, VOLUME_FLAG_PHYSICAL);
    }

    void* buffer = bMalloc(params->bytePerSector);
    if (buffer == NULL || rawDiskReadSectors(drive, buffer, sectorBegin, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (*(Uint16*)(buffer + 0x1FE) == 0xAA55) {
        SET_FLAG_BACK(flags, VOLUME_FLAG_USED);

        initSinglyLinkedList(&ret->children);
        for (int i = 0; i < 4; ++i) {
            __MBRpartitionEntry* entry = (__MBRpartitionEntry*)(buffer + 0x1BE + i * sizeof(__MBRpartitionEntry));
            if (entry->systemID == 0) {
                continue;
            }

            Volume* subVolume = bMalloc(sizeof(Volume));
            if (subVolume == NULL || __doScanVolume(drive, entry->sectorBegin, entry->sectorNum, params, ret, subVolume) == RESULT_FAIL) {
                if (subVolume != NULL) {
                    bFree(subVolume, sizeof(Volume));
                }

                return RESULT_FAIL;
            }
            
            if (entry->driveArttribute == 0x80) {
                SET_FLAG_BACK(subVolume->flags, VOLUME_FLAG_BOOTABLE);
                SET_FLAG_BACK(ret->flags, VOLUME_FLAG_BOOTABLE);
            }
        }
    }

    bFree(buffer, params->bytePerSector);

    ret->flags          = flags;

    if (openFileSystem(ret) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (parent != NULL) {
        initSinglyLinkedListNode(&ret->sibling);
        singlyLinkedListInsertNext(&parent->children, &ret->sibling);
        ++parent->childNum;
    }

    return RESULT_SUCCESS;
}

Volume* volumeGetChild(Volume* v, Index8 childIndex) {
    if (v == NULL || isSinglyListEmpty(&v->children) || childIndex >= v->childNum) {
        return NULL;
    }

    SinglyLinkedListNode* current = singlyLinkedListGetNext(&v->children);
    for (int i = 0; i < childIndex; ++i) {
        current = singlyLinkedListGetNext(current);
        if (current == &v->children) {
            return NULL;
        }
    }

    return HOST_POINTER(current, Volume, sibling);
}