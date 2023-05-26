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

static Result __doExecute(ConstCstring path, iNode** iNodePtr, File** filePtr, int* retPtr);

Result initUsermode() {
    initSyscall();

    registerSyscallHandler(SYSCALL_EXIT, __syscallHandlerExit);

    return RESULT_SUCCESS;
}

int execute(ConstCstring path) {
    iNode* iNode = NULL;
    File* file = NULL;
    int ret = -1;

    Result res = __doExecute(path, &iNode, &file, &ret);

    if (file != NULL) {
        if (rawFileClose(file) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    if (iNode != NULL) {
        if (rawInodeClose(iNode) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return res == RESULT_FAIL ? -1 : ret;
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

static Result __doExecute(ConstCstring path, iNode** iNodePtr, File** filePtr, int* retPtr) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_FILE) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    iNode* iNode = NULL;
    *iNodePtr = iNode = rawInodeOpen(entry.iNodeID);
    if (iNode == NULL) {
        return RESULT_FAIL;
    }

    File* file = NULL;
    *filePtr = file = rawFileOpen(iNode, FILE_FLAG_READ_ONLY);
    if (file == NULL) {
        return RESULT_FAIL;
    }

    ELF64Header header;
    if (readELF64Header(file, &header) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    ELF64ProgramHeader programHeader;
    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        if (readELF64ProgramHeader(file, &header, &programHeader, i) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (checkELF64ProgramHeader(&programHeader) == RESULT_FAIL) {
            SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_VERIFIVCATION_FAIL);
            return RESULT_FAIL;
        }

        if (loadELF64Program(file, &programHeader) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    Process* process = schedulerGetCurrentProcess();
    PML4Table* pageTable = process->context.pageTable;
    for (uintptr_t i = PAGE_SIZE; i <= USER_STACK_SIZE; i += PAGE_SIZE) {
        if (translateVaddr(pageTable, (void*)USER_STACK_BOTTOM - i) == NULL) {
            void* pAddr = pageAlloc(1, PHYSICAL_PAGE_TYPE_USER_STACK);
            if (pAddr == NULL) {
                return RESULT_FAIL;
            }

            if (mapAddr(pageTable, (void*)USER_STACK_BOTTOM - i, pAddr) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }
    }

    asm volatile("pushq $0;");  //Reserved for return value
    
    SAVE_REGISTERS();
    
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
            return RESULT_FAIL;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (unloadELF64Program(&programHeader) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    for (uintptr_t i = PAGE_SIZE; i <= USER_STACK_SIZE; i += PAGE_SIZE) {
        void* pAddr = translateVaddr(pageTable, (void*)USER_STACK_BOTTOM - i);
        if (pAddr == NULL) {
            return RESULT_FAIL;
        }
        pageFree(pAddr, 1);
    }

    asm volatile(
        "pop %0"
        : "=m"(*retPtr)
    );

    return RESULT_SUCCESS;
}