#include<usermode/usermode.h>

#include<error.h>
#include<fs/directory.h>
#include<fs/file.h>
#include<fs/fsutil.h>
#include<fs/inode.h>
#include<kit/types.h>
#include<memory/physicalPages.h>
#include<memory/pageAlloc.h>
#include<memory/paging/paging.h>
#include<multitask/process.h>
#include<real/simpleAsmLines.h>
#include<structs/registerSet.h>
#include<system/address.h>
#include<system/GDT.h>
#include<system/pageTable.h>
#include<usermode/elf.h>
#include<usermode/syscall.h>

__attribute__((naked))
void __jumpToUserMode(void* programBegin, void* stackBottom);

__attribute__((naked))
void __syscallHandlerExit(int ret);

void initUsermode() {
    initSyscall();

    registerSyscallHandler(SYSCALL_TYPE_EXIT, __syscallHandlerExit);
}

int execute(ConstCstring path) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_FILE) == -1) {
        return -1;
    }

    iNode* iNode = iNodeOpen(entry.iNodeID);
    File* file = fileOpen(iNode);

    ELF64Header header;
    if (readELF64Header(file, &header) == -1) {
        return -1;
    }

    ELF64ProgramHeader programHeader;
    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        if (readELF64ProgramHeader(file, &header, &programHeader, i) == -1) {
            fileClose(file);
            iNodeClose(iNode);
            return -1;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (!checkELF64ProgramHeader(&programHeader)) {
            fileClose(file);
            iNodeClose(iNode);
            SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_VERIFIVCATION_FAIL);
            return -1;
        }

        if (loadELF64Program(file, &programHeader) == -1) {
            fileClose(file);
            iNodeClose(iNode);
            return -1;
        }
    }

    Process* process = getCurrentProcess();
    PML4Table* pageTable = process->pageTable;
    for (uintptr_t i = PAGE_SIZE; i <= USER_STACK_SIZE; i += PAGE_SIZE) {
        if (translateVaddr(pageTable, (void*)USER_STACK_BOTTOM - i) == NULL) {
            mapAddr(pageTable, (void*)USER_STACK_BOTTOM - i, pageAlloc(1, PHYSICAL_PAGE_TYPE_USER_STACK));
        }
    }

    asm volatile(
        "pushq $0;"         //Reserved for return value
        SAVE_ALL_NO_FRAME   //Save context
    );
    process->userExitStackTop = (void*)readRegister_RSP_64();
    __jumpToUserMode((void*)header.entryVaddr, (void*)USER_STACK_BOTTOM);
    asm volatile (
        "__execute_return: mov %%rax, %P0(%%rsp);"   //Save return value immediately
        :
        : "i"(sizeof(RegisterSet))
    );

    asm volatile(RESTORE_ALL);  //Restore context

    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        readELF64ProgramHeader(file, &header, &programHeader, i);
        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        unloadELF64Program(&programHeader);
    }

    for (uintptr_t i = PAGE_SIZE; i <= USER_STACK_SIZE; i += PAGE_SIZE) {
        pageFree(translateVaddr(pageTable, (void*)USER_STACK_BOTTOM - i), 1);
    }

    fileClose(file);
    iNodeClose(iNode);

    register int ret asm("rax");
    asm volatile("pop %rax");

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
        : "g"(ret), "i"(SEGMENT_KERNEL_DATA), "g"(getCurrentProcess()->userExitStackTop), "i"(SEGMENT_KERNEL_CODE), "g"(__execute_return)
    );
}