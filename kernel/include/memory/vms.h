#if !defined(__MEMORY_VMS_H)
#define __MEMORY_VMS_H

typedef struct VirtualMemoryRegion VirtualMemoryRegion;
typedef struct VirtualMemorySpace VirtualMemorySpace;

#include<kit/types.h>
#include<kit/bit.h>
#include<memory/extendedPageTable.h>
#include<memory/memoryOperations.h>
#include<structs/RBtree.h>

typedef struct VirtualMemoryRegion {
    RBtreeNode treeNode;
    Range range;
#define VIRTUAL_MEMORY_REGION_FLAGS_HOLE            FLAG16(0)
#define VIRTUAL_MEMORY_REGION_FLAGS_WRITABLE        FLAG16(1)
#define VIRTUAL_MEMORY_REGION_FLAGS_NOT_EXECUTABLE  FLAG16(2)
#define VIRTUAL_MEMORY_REGION_FLAGS_USER            FLAG16(3)
#define VIRTUAL_MEMORY_REGION_FLAGS_LAZY_LOAD       FLAG16(4)
    Flags16 flags;
    Uint8 memoryOperationsID;
} VirtualMemoryRegion;

void virtualMemoryRegion_initStruct(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID);

typedef struct VirtualMemorySpace {
    RBtree regionTree;
    Size regionNum;
    ExtendedPageTableRoot* pageTable;
    Range range;
} VirtualMemorySpace;

void virtualMemorySpace_initStruct(VirtualMemorySpace* vms, ExtendedPageTableRoot* pageTable, void* base, Size length);

void virtualMemorySpace_clearStruct(VirtualMemorySpace* vms);

void virtualMemorySpace_copy(VirtualMemorySpace* des, VirtualMemorySpace* src);

void virtualMemorySpace_draw(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID);

VirtualMemoryRegion* virtualMemorySpace_getRegion(VirtualMemorySpace* vms, void* ptr);

void* virtualMemorySpace_findFirstFitHole(VirtualMemorySpace* vms, void* prefer, Size length);

void virtualMemorySpace_erase(VirtualMemorySpace* vms, void* begin, Size length);

#endif // __MEMORY_VMS_H
