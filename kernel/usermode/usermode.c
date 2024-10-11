#include<usermode/usermode.h>

#include<error.h>
#include<fs/fcntl.h>
#include<fs/fsutil.h>
#include<fs/inode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<multitask/schedule.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>
#include<system/pageTable.h>
#include<usermode/elf.h>
#include<usermode/syscall.h>

__attribute__((naked))
/**
 * @brief Jump to user mode
 * 
 * @param programBegin Beginning of user mode program
 * @param stackBottom Stack bottom of user mode
 */
void __usermode_jumpToUserMode(void* programBegin, void* stackBottom);

__attribute__((naked))
/**
 * @brief System call handler for exit
 * 
 * @param ret Return value of user program
 */
void __usermode_syscallHandlerExit(int ret);

static int __usermode_doExecute(ConstCstring path, File* file);

Result usermode_init() {
    syscall_init();

    syscall_registerHandler(SYSCALL_EXIT, __usermode_syscallHandlerExit);

    return RESULT_SUCCESS;
}

int usermode_execute(ConstCstring path) {  //TODO: Unstable code
    fsEntry entry;
    if (fsutil_openfsEntry(rootFS->superBlock, path, FS_ENTRY_TYPE_FILE, &entry, FCNTL_OPEN_READ_ONLY) != RESULT_SUCCESS) {
        return -1;
    }

    int ret = __usermode_doExecute(path, &entry);

    fsutil_closefsEntry(&entry);

    return ret;
}

__attribute__((naked))
void __usermode_jumpToUserMode(void* programBegin, void* stackBottom) {
    asm volatile(
        "pushq %0;" //SS
        "pushq %1;" //RSP
        "pushfq;"   //EFLAGS
        "pushq %2;" //CS
        "pushq %3;" //RIP
        "mov 32(%%rsp), %%ds;"
        "mov 32(%%rsp), %%es;"
        "iretq;"
        :
        : "i" (SEGMENT_USER_DATA), "r" (stackBottom), "i"(SEGMENT_USER_CODE), "r"(programBegin)
    );
}

extern void* __execute_return;

__attribute__((naked))
void __usermode_syscallHandlerExit(int ret) {
    asm volatile(
        "pushq %1;"         //SS
        "pushq %2;"         //RSP
        "pushfq;"           //EFLAGS
        "pushq %3;"         //CS
        "lea %4, %%rax;"    //Inline assembly magic
        "pushq %%rax;"      //RIP
        "mov 32(%%rsp), %%ds;"
        "mov 32(%%rsp), %%es;"
        "mov %0, %%eax;"
        "iretq;"
        :
        : "g"(ret), "i"(SEGMENT_KERNEL_DATA), "g"(scheduler_getCurrentProcess()->userExitStackTop), "i"(SEGMENT_KERNEL_CODE), "g"(__execute_return)
    );
}

#define __USERMODE_STACK_SIZE       (16ull * DATA_UNIT_KB)
#define __USERMODE_STACK_PAGE_NUM   DIVIDE_ROUND_UP(__USERMODE_STACK_SIZE, PAGE_SIZE)

static int __usermode_doExecute(ConstCstring path, File* file) {
    Uint64 old = readRegister_RSP_64();
    ELF64Header header;
    if (elf_readELF64Header(file, &header) != RESULT_SUCCESS) {
        return -1;
    }

    ELF64ProgramHeader programHeader;
    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        if (elf_readELF64ProgramHeader(file, &header, &programHeader, i) != RESULT_SUCCESS) {
            return -1;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (elf_checkELF64ProgramHeader(&programHeader) != RESULT_SUCCESS) {
            ERROR_CODE_SET(ERROR_CODE_OBJECT_FILE, ERROR_CODE_STATUS_VERIFIVCATION_FAIL);
            return -1;
        }

        if (elf_loadELF64Program(file, &programHeader) != RESULT_SUCCESS) {
            return -1;
        }
    }

    ExtendedPageTableRoot* extendedTable = mm->extendedTable;

#define __USERMODE_USER_STACK_FRAME_NUM DIVIDE_ROUND_UP(USER_STACK_SIZE, PAGE_SIZE)
    for (Uintptr i = PAGE_SIZE; i <= __USERMODE_STACK_SIZE; i += PAGE_SIZE) {
        if (extendedPageTableRoot_translate(extendedTable, (void*)MEMORY_LAYOUT_USER_STACK_BOTTOM - i) != NULL) {
            return -1;
        }

        void* pAddr = memory_allocateFrame(1);
        if (pAddr == NULL || extendedPageTableRoot_draw(extendedTable, (void*)MEMORY_LAYOUT_USER_STACK_BOTTOM - i, pAddr, 1, extendedTable->context->presets[EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(extendedTable->context, MEMORY_DEFAULT_PRESETS_TYPE_USER_DATA)]) != RESULT_SUCCESS) {
            return -1;
        }
    }

    pushq(0);   //Reserved for return value
    
    REGISTERS_SAVE();
    
    Process* process = scheduler_getCurrentProcess();
    process->userExitStackTop = (void*)readRegister_RSP_64();
    __usermode_jumpToUserMode((void*)header.entryVaddr, (void*)MEMORY_LAYOUT_USER_STACK_BOTTOM);
    asm volatile (
        "__execute_return: mov %%rax, %P0(%%rsp);"   //Save return value immediately
        :
        : "i"(sizeof(Registers))
    );
    
    REGISTERS_RESTORE();    //Restore context

    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        if (elf_readELF64ProgramHeader(file, &header, &programHeader, i) != RESULT_SUCCESS) {
            return -1;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (elf_unloadELF64Program(&programHeader) != RESULT_SUCCESS) {
            return -1;
        }
    }

    for (Uintptr i = PAGE_SIZE; i <= __USERMODE_STACK_SIZE; i += PAGE_SIZE) {
        void* pAddr = extendedPageTableRoot_translate(extendedTable, (void*)MEMORY_LAYOUT_USER_STACK_BOTTOM - i);
        if (pAddr == NULL || extendedPageTableRoot_erase(extendedTable, (void*)MEMORY_LAYOUT_USER_STACK_BOTTOM - i, 1) != RESULT_SUCCESS) {
            return -1;
        }

        memory_freeFrame(pAddr);
    }

    //TODO: Drop page entries about user program

    return popq();
}