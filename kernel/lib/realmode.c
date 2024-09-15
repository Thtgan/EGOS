#include<realmode.h>

#include<algorithms.h>
#include<carrier.h>
#include<kernel.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<interrupt/IDT.h>
#include<system/memoryMap.h>

typedef void (*realmodeExecFunc)(Uintptr realmodeCode, Uintptr stack, RealmodeRegs* inRegs, RealmodeRegs* outRegs);

static realmodeExecFunc _realmode_doExecFunc;

extern void realmode_doExec(Uintptr realmodeCode, Uintptr stack, RealmodeRegs* inRegs, RealmodeRegs* outRegs);

extern char realmode_begin, realmode_end;
extern char realmodeFuncs_begin, realmodeFuncs_end;

extern CarrierMovMetadata* realmode_carryList;
extern CarrierMovMetadata* realmodeFuncs_carryList;

extern Uint32 realmode_protectedModePageDirectory, realmode_compatabilityModePML4;

static void* _realMode_stack;

static void* _realMode_funcs[16];
static int _realMode_funcNum;

extern char realmodeFuncs_test;

void* __realmode_getFunc(Index16 index);

void* __realmode_findHighestMemory(MemoryMap* mMap, Uintptr below, Size requiredPageNum);

void __realmode_setupPageTables(void* pageTableFrames);

Result realmode_init() {
    void* realmode_beginPtr = &realmode_begin;
    void* realmode_endPtr = &realmode_end;
    Size codeSize = (Uintptr)&realmode_end - (Uintptr)&realmode_begin;

    MemoryMap* mMap = &mm->mMap;

    Uint32 requiredPageNum = DIVIDE_ROUND_UP(codeSize, PAGE_SIZE);
    void* copyTo = __realmode_findHighestMemory(mMap, 0x10000, requiredPageNum);
    print_printf(TERMINAL_LEVEL_OUTPUT, "%p\n", copyTo);

    void* realmodeFuncs_beginPtr = &realmodeFuncs_begin;
    void* realmodeFuncs_endPtr = &realmodeFuncs_end;
    Size funcsCodeSize = realmodeFuncs_endPtr - realmodeFuncs_beginPtr;

    Uint32 stackNum = 1;
    requiredPageNum = stackNum + DIVIDE_ROUND_UP(funcsCodeSize, PAGE_SIZE);

    void* funcsCopyTo = __realmode_findHighestMemory(mMap, 0x100000, requiredPageNum);
    _realMode_stack = funcsCopyTo + ALIGN_UP(funcsCodeSize, PAGE_SIZE);

    void* pageTableFrames = memory_allocateFrame(3);    //TODO: Make sure it allocates fromn lower 4GB
    if ((Uintptr)pageTableFrames + 3 * PAGE_SIZE > 0x100000000) {
        return RESULT_FAIL;
    }

    __realmode_setupPageTables(pageTableFrames);

    _realmode_doExecFunc = (void*)realmode_doExec - realmode_beginPtr + copyTo;

    if (carrier_carry(realmode_beginPtr, copyTo, codeSize, &realmode_carryList) == RESULT_FAIL || carrier_carry(realmodeFuncs_beginPtr, funcsCopyTo, funcsCodeSize, &realmodeFuncs_carryList) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    _realMode_funcNum = 0;

    if (realmode_registerFunc((void*)((Uintptr)&realmodeFuncs_test - (Uintptr)realmodeFuncs_beginPtr + (Uintptr)funcsCopyTo)) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result realmode_exec(Index16 funcIndex, RealmodeRegs* inRegs, RealmodeRegs* outRegs) {
    void* func = __realmode_getFunc(funcIndex);
    if (func == NULL) {
        return RESULT_FAIL;
    }

    bool interrupt = idt_disableInterrupt();

    REGISTERS_SAVE();

    _realmode_doExecFunc((Uintptr)func, (Uintptr)_realMode_stack, inRegs, outRegs);

    REGISTERS_RESTORE();

    idt_setInterrupt(interrupt);

    return RESULT_SUCCESS;
}

Index16 realmode_registerFunc(void* func) {
    if (_realMode_funcNum == 16 || (Uintptr)func >= 0x100000) {
        return INVALID_INDEX;
    }

    _realMode_funcs[_realMode_funcNum] = func;

    return _realMode_funcNum++;
}

void* __realmode_getFunc(Index16 index) {
    return index > _realMode_funcNum ? NULL : _realMode_funcs[index];
}

void* __realmode_findHighestMemory(MemoryMap* mMap, Uintptr below, Size requiredPageNum) {
    MemoryMapEntry* target = NULL;
    for (int i = 0; i < mMap->entryNum; ++i) {  //TODO: Pack this into a E820 function
        MemoryMapEntry* e = mMap->memoryMapEntries + i;

        if (e->type != MEMORY_MAP_ENTRY_TYPE_RAM) {
            continue;
        }

        if (e->base >= below) {
            continue;
        }

        Uintptr top = algorithms_umin64(ALIGN_DOWN(e->base + e->length, PAGE_SIZE), below);
        if (top - requiredPageNum * PAGE_SIZE >= e->base && (target == NULL || target->base < e->base)) {
            target = e;
        }
    }

    if (target == NULL) {
        return NULL;
    }

    void* ret = (void*)algorithms_umin64(ALIGN_DOWN(target->base + target->length, PAGE_SIZE), below) - requiredPageNum * PAGE_SIZE;

    if (
        extendedPageTableRoot_draw(
            mm->extendedTable,
            ret, ret, 
            requiredPageNum,
            extraPageTableContext_getPreset(&mm->extraPageTableContext, EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_KERNEL))
        ) == RESULT_FAIL
    ) {
        return NULL;
    }

    return ret;
}

void __realmode_setupPageTables(void* pageTableFrames) {
    Uint32* protectedModePageDirectory = (Uint32*)paging_convertAddressP2V(pageTableFrames);
    memory_memset(protectedModePageDirectory, 0, PAGE_SIZE);

    for (int i = 0; i < 1024; ++i) {
        protectedModePageDirectory[i] = ((Uint32)i * 0x400000) | 0x83;
    }

    Uint64* compatabilityModePML4 = (Uint64*)paging_convertAddressP2V((void*)protectedModePageDirectory + PAGE_SIZE);
    memory_memset(compatabilityModePML4, 0, PAGE_SIZE);
    Uint64* compatabilityModePDPT = (Uint64*)paging_convertAddressP2V((void*)compatabilityModePML4 + PAGE_SIZE);
    memory_memset(compatabilityModePDPT, 0, PAGE_SIZE);


    for (int i = 0; i < 512; ++i) {
        compatabilityModePDPT[i] = ((Uint64)i * 0x40000000) | 0x83;
    }

    compatabilityModePML4[0] = (Uint64)paging_convertAddressV2P(compatabilityModePDPT) | 0x3;

    realmode_protectedModePageDirectory = (Uint32)paging_convertAddressV2P(protectedModePageDirectory);
    realmode_compatabilityModePML4 = (Uint32)paging_convertAddressV2P(compatabilityModePML4);
}