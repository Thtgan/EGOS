#include<memory/paging.h>

#include<algorithms.h>
#include<debug.h>
#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
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
#include<error.h>

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
            extendedPageTable = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(HOST_POINTER(PAGING_TABLE_FROM_PAGING_ENTRY(entry), ExtendedPageTable, table));
            continue;
        }

        extendedPageTableRoot_pageFaultHandler(mm->extendedTable, level, extendedPageTable, index, v, handlerStackFrame, registers);
        bool fixed = true;
        ERROR_CHECKPOINT({
            print_printf("Page handler failed at level %u\n", level);
            fixed = false;
        });

        if (fixed) {
            PAGING_FLUSH_TLB();
            return;
        }

        break;
    }
    ERROR_CLEAR();

    print_debugPrintf("CURRENT STACK: %#018llX\n", readRegister_RSP_64());
    print_debugPrintf("FRAME: %#018llX\n", handlerStackFrame);
    print_debugPrintf("ERRORCODE: %#018llX RIP: %#018llX CS: %#018llX\n", handlerStackFrame->errorCode, handlerStackFrame->rip, handlerStackFrame->cs);
    print_debugPrintf("EFLAGS: %#018llX RSP: %#018llX SS: %#018llX\n", handlerStackFrame->eflags, handlerStackFrame->rsp, handlerStackFrame->ss);
    debug_dump_registers(registers);
    debug_dump_stack((void*)registers->rbp, INFINITE);
    debug_blowup("Page fault: %#018llX access not allowed. Error code: %#X, RIP: %#llX", (Uint64)v, handlerStackFrame->errorCode, handlerStackFrame->rip); //Not allowed since malloc is implemented
}

static ExtendedPageTableRoot _extendedPageTableRoot;

void paging_init() {
    extraPageTableContext_initStruct(&mm->extraPageTableContext);
    ERROR_GOTO_IF_ERROR(0);

    _extendedPageTableRoot.context = &mm->extraPageTableContext;

    extendedPageTableRoot_draw(
        &_extendedPageTableRoot,
        (void*)MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN + PAGE_SIZE, (void*)PAGE_SIZE,
        DIVIDE_ROUND_UP(algorithms_umin64(MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_END - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, (Uintptr)PAGING_PHYSICAL_KERNEL_RANGE_END), PAGE_SIZE) - 1, //TODO: Maybe PHYSICAL_KERNEL_RANGE_END - PHYSICAL_KERNEL_RANGE_BEGIN?
        extraPageTableContext_getDefaultPreset(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_KERNEL),
        EMPTY_FLAGS
    );
    ERROR_GOTO_IF_ERROR(0);

    extendedPageTableRoot_draw(
        &_extendedPageTableRoot,
        (void*)MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN + PAGE_SIZE, (void*)PAGE_SIZE, 
        algorithms_umin64(DIVIDE_ROUND_UP(MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_END - MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN, PAGE_SIZE), mm->accessibleEnd) - 1,
        extraPageTableContext_getDefaultPreset(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_KERNEL),
        EMPTY_FLAGS
    );
    ERROR_GOTO_IF_ERROR(0);

    mm_switchPageTable(&_extendedPageTableRoot);

    idt_registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, 1, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_TRAP_GATE32); //Register default page fault handler, IST 1 in case of stack memory triggers error

    Uint32 eax, edx;
    rdmsr(MSR_ADDR_EFER, &edx, &eax);
    SET_FLAG_BACK(eax, MSR_EFER_NXE);
    wrmsr(MSR_ADDR_EFER, edx, eax);

    writeRegister_CR0_64(readRegister_CR0_64() | CR0_WP); //Enable write protect

    return;
    ERROR_FINAL_BEGIN(0);
}
