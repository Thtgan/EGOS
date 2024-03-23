#include<usermode/usermode.h>

#include<error.h>
#include<fs/fsutil.h>
#include<fs/inode.h>
#include<kit/types.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<multitask/context.h>
#include<multitask/schedule.h>
#include<real/simpleAsmLines.h>
#include<system/address.h>
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
void __jumpToUserMode(void* programBegin, void* stackBottom);

__attribute__((naked))
/**
 * @brief System call handler for exit
 * 
 * @param ret Return value of user program
 */
void __syscallHandlerExit(int ret);

static int __doExecute(ConstCstring path, File* file);

Result initUsermode() {
    initSyscall();

    registerSyscallHandler(SYSCALL_EXIT, __syscallHandlerExit);

    return RESULT_SUCCESS;
}

int execute(ConstCstring path) {    //TODO: Unstable code
    fsEntry entry;
    if (fsutil_openfsEntry(rootFS->superBlock, path, FS_ENTRY_TYPE_FILE, &entry) == RESULT_FAIL) {
        return -1;
    }

    int ret = __doExecute(path, &entry);

    fsutil_closefsEntry(&entry);

    return ret;
}

__attribute__((naked))
void __jumpToUserMode(void* programBegin, void* stackBottom) {
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
void __syscallHandlerExit(int ret) {
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
        : "g"(ret), "i"(SEGMENT_KERNEL_DATA), "g"(schedulerGetCurrentProcess()->userExitStackTop), "i"(SEGMENT_KERNEL_CODE), "g"(__execute_return)
    );
}

static int __doExecute(ConstCstring path, File* file) {
    Uint64 old = readRegister_RSP_64();
    ELF64Header header;
    if (readELF64Header(file, &header) == RESULT_FAIL) {
        return -1;
    }

    ELF64ProgramHeader programHeader;
    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        if (readELF64ProgramHeader(file, &header, &programHeader, i) == RESULT_FAIL) {
            return -1;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (checkELF64ProgramHeader(&programHeader) == RESULT_FAIL) {
            ERROR_CODE_SET(ERROR_CODE_OBJECT_FILE, ERROR_CODE_STATUS_VERIFIVCATION_FAIL);
            return -1;
        }

        if (loadELF64Program(file, &programHeader) == RESULT_FAIL) {
            return -1;
        }
    }

    PML4Table* pageTable = mm->currentPageTable;

    PagingLevel level;
    for (Uintptr i = PAGE_SIZE; i <= USER_STACK_SIZE; i += PAGE_SIZE) {
        PagingEntry* entry = pageTableGetEntry(pageTable, (void*)USER_STACK_BOTTOM - i, &level);
        if (entry != NULL) {
            return -1;
        }

        void* pAddr = physicalPage_alloc(1, PHYSICAL_PAGE_ATTRIBUTE_USER_STACK);
        if (pAddr == NULL || mapAddr(pageTable, (void*)USER_STACK_BOTTOM - i, pAddr, PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_US) == RESULT_FAIL) {
            return -1;
        }
    }

    pushq(0);   //Reserved for return value
    
    SAVE_REGISTERS();
    
    Process* process = schedulerGetCurrentProcess();
    process->userExitStackTop = (void*)readRegister_RSP_64();
    __jumpToUserMode((void*)header.entryVaddr, (void*)USER_STACK_BOTTOM);
    asm volatile (
        "__execute_return: mov %%rax, %P0(%%rsp);"   //Save return value immediately
        :
        : "i"(sizeof(Registers))
    );
    
    RESTORE_REGISTERS();    //Restore context

    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        if (readELF64ProgramHeader(file, &header, &programHeader, i) == RESULT_FAIL) {
            return -1;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (unloadELF64Program(&programHeader) == RESULT_FAIL) {
            return -1;
        }
    }

    for (Uintptr i = PAGE_SIZE; i <= USER_STACK_SIZE; i += PAGE_SIZE) {
        PagingEntry* entry = pageTableGetEntry(pageTable, (void*)USER_STACK_BOTTOM - i, &level);
        if (entry == NULL || level != PAGING_LEVEL_PAGE_TABLE) {
            return -1;
        }

        physicalPage_free(BASE_FROM_ENTRY_PS(PAGING_LEVEL_PAGE_TABLE, entry));
    }

    //TODO: Drop page entries about user program

    return popq();
}