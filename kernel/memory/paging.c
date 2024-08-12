#include<memory/paging.h>

#include<algorithms.h>
#include<debug.h>
#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<real/flags/cr0.h>
#include<real/flags/msr.h>
#include<real/simpleAsmLines.h>
#include<system/memoryLayout.h>
#include<system/pageTable.h>

ISR_FUNC_HEADER(__pageFaultHandler) { //TODO: This handler triggers double page faults for somehow
    void* v = (void*)readRegister_CR2_64();

    ExtendedPageTable* extendedPageTable = mm->extendedTable->extendedTable;
    for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
        PagingTable* pageTable = &extendedPageTable->table;
        ExtraPageTable* extraPageTable = &extendedPageTable->extraTable;
        Index16 index = PAGING_INDEX(level, v);
        PagingEntry entry = pageTable->tableEntries[index];

        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            break;
        }

        if (!PAGING_IS_LEAF(level, entry)) {
            extendedPageTable = paging_convertAddressP2V(HOST_POINTER(PAGING_TABLE_FROM_PAGING_ENTRY(entry), ExtendedPageTable, table));
            continue;
        }

        Result handlerRes = extendedPageTableRoot_pageFaultHandler(mm->extendedTable, level, extendedPageTable, index, v, handlerStackFrame, registers);
        if (handlerRes == RESULT_SUCCESS) {
            return;
        } else if (handlerRes == RESULT_FAIL) {
            print_printf(TERMINAL_LEVEL_DEBUG, "Page handler failed at level %u\n", level);
            break;
        }
    }

    print_printf(TERMINAL_LEVEL_DEBUG, "CURRENT STACK: %#018llX\n", readRegister_RSP_64());
    print_printf(TERMINAL_LEVEL_DEBUG, "FRAME: %#018llX\n", handlerStackFrame);
    print_printf(TERMINAL_LEVEL_DEBUG, "ERRORCODE: %#018llX RIP: %#018llX CS: %#018llX\n", handlerStackFrame->errorCode, handlerStackFrame->rip, handlerStackFrame->cs);
    print_printf(TERMINAL_LEVEL_DEBUG, "EFLAGS: %#018llX RSP: %#018llX SS: %#018llX\n", handlerStackFrame->eflags, handlerStackFrame->rsp, handlerStackFrame->ss);
    debug_dump_registers(registers);
    debug_blowup("Page fault: %#018llX access not allowed. Error code: %#X, RIP: %#llX", (Uint64)v, handlerStackFrame->errorCode, handlerStackFrame->rip); //Not allowed since malloc is implemented
}

static ExtendedPageTableRoot _extendedPageTableRoot;

Result paging_init() {
    if (extraPageTableContext_initStruct(&mm->extraPageTableContext) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    _extendedPageTableRoot.context = &mm->extraPageTableContext;

    if (
        extendedPageTableRoot_draw(
            &_extendedPageTableRoot,
            (void*)MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN + PAGE_SIZE, (void*)PAGE_SIZE,
            DIVIDE_ROUND_UP(algorithms_umin64(MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_END - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, (Uintptr)PHYSICAL_KERNEL_RANGE_END), PAGE_SIZE) - 1,
            extraPageTableContext_getPreset(&mm->extraPageTableContext, EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_KERNEL))
        ) == RESULT_FAIL
    ) {
        return RESULT_FAIL;
    }

    if (
        extendedPageTableRoot_draw(
            &_extendedPageTableRoot,
            (void*)MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN + PAGE_SIZE, (void*)PAGE_SIZE, 
            algorithms_umin64(DIVIDE_ROUND_UP(MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_END - MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN, PAGE_SIZE), mm->accessibleEnd) - 1,
            extraPageTableContext_getPreset(&mm->extraPageTableContext, EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE))
        ) == RESULT_FAIL
    ) {
        return RESULT_FAIL;
    }

    PAGING_SWITCH_TO_TABLE(&_extendedPageTableRoot);

    idt_registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_TRAP_GATE32); //Register default page fault handler

    Uint32 eax, edx;
    rdmsr(MSR_ADDR_EFER, &edx, &eax);
    SET_FLAG_BACK(eax, MSR_EFER_NXE);
    wrmsr(MSR_ADDR_EFER, edx, eax);

    writeRegister_CR0_64(readRegister_CR0_64() | CR0_WP); //Enable write protect

    return RESULT_SUCCESS;
}
