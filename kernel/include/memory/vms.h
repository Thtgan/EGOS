#if !defined(__MEMORY_VMS_H)
#define __MEMORY_VMS_H

typedef struct VirtualMemoryRegionSharedFrames VirtualMemoryRegionSharedFrames;
typedef struct VirtualMemoryRegionInfo VirtualMemoryRegionInfo;
typedef struct VirtualMemoryRegion VirtualMemoryRegion;
typedef struct VirtualMemorySpace VirtualMemorySpace;

#include<fs/fsEntry.h>
#include<kit/types.h>
#include<kit/bit.h>
#include<memory/extendedPageTable.h>
#include<memory/memoryOperations.h>
#include<structs/RBtree.h>
#include<structs/refCounter.h>
#include<structs/vector.h>

typedef struct VirtualMemoryRegionSharedFrames {
    Uintptr vBase;
    RefCounter referCnt;
    Vector frames;
} VirtualMemoryRegionSharedFrames;

typedef struct VirtualMemoryRegionInfo {
    Range range;
#define VIRTUAL_MEMORY_REGION_FLAGS_WRITABLE                FLAG16(0)
#define VIRTUAL_MEMORY_REGION_FLAGS_NOT_EXECUTABLE          FLAG16(1)
#define VIRTUAL_MEMORY_REGION_FLAGS_USER                    FLAG16(2)
#define VIRTUAL_MEMORY_REGION_FLAGS_EXTRACT_PROT(__FLAGS)   EXTRACT_VAL(__FLAGS, 16, 0, 3)
#define VIRTUAL_MEMORY_REGION_FLAGS_TYPE_HOLE               VAL_LEFT_SHIFT(0, 3)
#define VIRTUAL_MEMORY_REGION_FLAGS_TYPE_KERNEL             VAL_LEFT_SHIFT(1, 3)
#define VIRTUAL_MEMORY_REGION_FLAGS_TYPE_ANON               VAL_LEFT_SHIFT(2, 3)
#define VIRTUAL_MEMORY_REGION_FLAGS_TYPE_FILE               VAL_LEFT_SHIFT(3, 3)
#define VIRTUAL_MEMORY_REGION_FLAGS_EXTRACT_TYPE(__FLAGS)   TRIM_VAL_RANGE(__FLAGS, 16, 3, 5)
#define VIRTUAL_MEMORY_REGION_FLAGS_SHARED                  FLAG16(5)
#define VIRTUAL_MEMORY_REGION_FLAGS_LAZY_LOAD               FLAG16(6)
    Flags16 flags;
    Uint8 memoryOperationsID;
    File* file;
    Index64 offset;
} VirtualMemoryRegionInfo;

static inline Flags64 virtualMemoryRegionInfo_convertPagingProt(VirtualMemoryRegionInfo* info) {
    Flags64 flags = info->flags, ret = EMPTY_FLAGS;
    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_WRITABLE)) {
        SET_FLAG_BACK(ret, PAGING_ENTRY_FLAG_RW);
    }

    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_USER)) {
        SET_FLAG_BACK(ret, PAGING_ENTRY_FLAG_US);
    }
    
    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_NOT_EXECUTABLE)) {
        SET_FLAG_BACK(ret, PAGING_ENTRY_FLAG_XD);
    }

    return ret;
}

static inline void virtualMemoryRegionInfo_drawToExtendedTable(VirtualMemoryRegionInfo* info, ExtendedPageTableRoot* extendedTable, void* p) {
    Range* range = &info->range;
    Flags64 prot = virtualMemoryRegionInfo_convertPagingProt(info);
    Flags8 flags = EXTENDED_PAGE_TABLE_DRAW_FLAGS_ASSERT_DRAW_BLANK;
    if (TEST_FLAGS(info->flags, VIRTUAL_MEMORY_REGION_FLAGS_LAZY_LOAD)) {
        SET_FLAG_BACK(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_LAZY_MAP);
    }
    extendedPageTableRoot_draw(extendedTable, (void*)range->begin, p, range->length / PAGE_SIZE, info->memoryOperationsID, prot, flags);
}

typedef struct VirtualMemoryRegion {
    RBtreeNode treeNode;
    VirtualMemoryRegionInfo info;
    VirtualMemoryRegionSharedFrames* sharedFrames;
} VirtualMemoryRegion;

Index32 virtualMemoryRegion_getFrameIndex(VirtualMemoryRegion* vmr, void* v);

Index32 virtualMemoryRegion_setFrameIndex(VirtualMemoryRegion* vmr, void* v, Index32 frameIndex);

typedef struct VirtualMemorySpace {
    RBtree regionTree;
    Size regionNum;
    ExtendedPageTableRoot* pageTable;
    Range range;
} VirtualMemorySpace;

void virtualMemorySpace_initStruct(VirtualMemorySpace* vms, ExtendedPageTableRoot* pageTable, void* base, Size length);

void virtualMemorySpace_clearStruct(VirtualMemorySpace* vms);

void virtualMemorySpace_copy(VirtualMemorySpace* des, VirtualMemorySpace* src);

VirtualMemoryRegion* virtualMemorySpace_draw(VirtualMemorySpace* vms, VirtualMemoryRegionInfo* info);

VirtualMemoryRegion* virtualMemorySpace_getRegion(VirtualMemorySpace* vms, void* ptr);

void* virtualMemorySpace_findFirstFitHole(VirtualMemorySpace* vms, void* prefer, Size length);

void virtualMemorySpace_erase(VirtualMemorySpace* vms, void* begin, Size length);

#endif // __MEMORY_VMS_H
