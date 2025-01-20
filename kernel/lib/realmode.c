#include<realmode.h>

#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/memoryMap.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<interrupt/IDT.h>
#include<system/memoryMap.h>
#include<algorithms.h>
#include<carrier.h>
#include<debug.h>
#include<kernel.h>
#include<error.h>

typedef void (*realmodeExecFunc)(Uintptr realmodeCode, Uintptr stack, RealmodeRegs* inRegs, RealmodeRegs* outRegs);

static realmodeExecFunc _realmode_doExecFunc;

extern void realmode_doExec(Uintptr realmodeCode, Uintptr stack, RealmodeRegs* inRegs, RealmodeRegs* outRegs);

extern char realmode_begin, realmode_end;

extern CarrierMovMetadata* realmode_carryList;

extern Uint32 realmode_protectedModePageDirectory, realmode_compatabilityModePML4;

static void* _realMode_stack;

#define __REALMODE_MAX_FUNC_NUM 16
static void* _realMode_funcs[__REALMODE_MAX_FUNC_NUM];
static int _realMode_funcNum;

void* __realmode_getFunc(Index16 index);

void* __realmode_findHighestMemory(MemoryMap* mMap, Uintptr below, Size n);

void __realmode_setupPageTables(void* pageTableFrames);

void realmode_init() {
    void* realmode_beginPtr = &realmode_begin;
    void* realmode_endPtr = &realmode_end;
    Size codeSize = (Uintptr)&realmode_end - (Uintptr)&realmode_begin;

    MemoryMap* mMap = &mm->mMap;

    Uint32 requiredPageNum = DIVIDE_ROUND_UP(codeSize, PAGE_SIZE);
    void* copyTo = __realmode_findHighestMemory(mMap, 0x10000, requiredPageNum);

    extendedPageTableRoot_draw(
        mm->extendedTable,
        copyTo, copyTo, 
        requiredPageNum,
        extraPageTableContext_getPreset(&mm->extraPageTableContext, EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_KERNEL))
    );
    ERROR_GOTO_IF_ERROR(0);

    Uint32 stackNum = 1;
    Uint32 stackPageNum = stackNum;

    _realMode_stack = __realmode_findHighestMemory(mMap, 0x100000, requiredPageNum);
    extendedPageTableRoot_draw(
        mm->extendedTable,
        _realMode_stack, _realMode_stack, 
        stackPageNum,
        extraPageTableContext_getPreset(&mm->extraPageTableContext, EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_KERNEL))
    );
    ERROR_GOTO_IF_ERROR(0);

    _realMode_stack = _realMode_stack + stackPageNum * PAGE_SIZE;

    void* pageTableFrames = memory_allocateFrame(3);    //TODO: Make sure it allocates fromn lower 4GB
    if ((Uintptr)pageTableFrames + 3 * PAGE_SIZE > 0x100000000) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    __realmode_setupPageTables(pageTableFrames);

    _realmode_doExecFunc = (void*)realmode_doExec - realmode_beginPtr + copyTo;

    if (carrier_carry(realmode_beginPtr, copyTo, codeSize, &realmode_carryList) != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    _realMode_funcNum = 0;

    return;
    ERROR_FINAL_BEGIN(0);
}

OldResult realmode_exec(Index16 funcIndex, RealmodeRegs* inRegs, RealmodeRegs* outRegs) {
    void* func = __realmode_getFunc(funcIndex);
    if (func == NULL) {
        return RESULT_ERROR;
    }

    bool interrupt = idt_disableInterrupt();

    REGISTERS_SAVE();

    _realmode_doExecFunc((Uintptr)func, (Uintptr)_realMode_stack, inRegs, outRegs);

    REGISTERS_RESTORE();

    idt_setInterrupt(interrupt);

    return RESULT_SUCCESS;
}

void realmode_registerFuncs(void* codeBegin, Size codeSize, CarrierMovMetadata** carrierList, void** funcList, Size funcNum, int* indexRet) {
    DEBUG_ASSERT(_realMode_funcNum + funcNum <= __REALMODE_MAX_FUNC_NUM && indexRet != NULL, "");

    void* copyTo = __realmode_findHighestMemory(&mm->mMap, 0x10000, DIVIDE_ROUND_UP(codeSize, PAGE_SIZE));
    if (copyTo == NULL || carrier_carry(codeBegin, paging_convertAddressP2V(copyTo), codeSize, carrierList) != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);   //TODO: Temporary solution
    }

    for (int i = 0; i < funcNum; ++i, ++_realMode_funcNum) {
        _realMode_funcs[_realMode_funcNum] = (Uintptr)(funcList[i] - codeBegin) + copyTo;
        indexRet[i] = _realMode_funcNum;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void* __realmode_getFunc(Index16 index) {
    return index > _realMode_funcNum ? NULL : _realMode_funcs[index];
}

void* __realmode_findHighestMemory(MemoryMap* mMap, Uintptr below, Size n) {
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
        if (top - n * PAGE_SIZE >= e->base && (target == NULL || target->base < e->base)) {
            target = e;
        }
    }

    if (target == NULL) {
        return NULL;
    }

    void* ret = (void*)algorithms_umin64(ALIGN_DOWN(target->base + target->length, PAGE_SIZE), below) - n * PAGE_SIZE;

    memoryMap_splitEntryAndTidyup(mMap, target, (Uint64)ret - target->base, MEMORY_MAP_ENTRY_TYPE_RESERVED);
    ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
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