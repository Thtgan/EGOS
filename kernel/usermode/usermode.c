#include<usermode/usermode.h>

#include<fs/fcntl.h>
#include<fs/fs.h>
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
#include<error.h>

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

void usermode_init() {
    syscall_init();
}

int usermode_execute(ConstCstring path) {
    File* file = fs_fileOpen(path, FCNTL_OPEN_READ_ONLY);
    ERROR_GOTO_IF_ERROR(0);

    int ret = __usermode_doExecute(path, file);
    ERROR_GOTO_IF_ERROR(0);

    fs_fileClose(file);
    ERROR_GOTO_IF_ERROR(0);

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (file != NULL) {
        fs_fileClose(file);
    }
    return -1;
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
        : "g"(ret), "i"(SEGMENT_KERNEL_DATA), "g"(schedule_getCurrentThread()->userExitStackTop), "i"(SEGMENT_KERNEL_CODE), "g"(__execute_return)
    );
}

#define __USERMODE_STACK_SIZE       (16ull * DATA_UNIT_KB)
#define __USERMODE_STACK_PAGE_NUM   DIVIDE_ROUND_UP(__USERMODE_STACK_SIZE, PAGE_SIZE)

static int __usermode_doExecute(ConstCstring path, File* file) {
    void* frame = NULL;
    Size loadedHeaderNum = 0;
    Size initedStackSize = 0;
    
    Uint64 old = readRegister_RSP_64();
    ELF64Header header;
    elf_readELF64Header(file, &header);
    ERROR_GOTO_IF_ERROR(0);

    ELF64ProgramHeader programHeader;
    for (int i = 0; i < header.programHeaderEntryNum; loadedHeaderNum = i++) {
        elf_readELF64ProgramHeader(file, &header, &programHeader, i);
        ERROR_GOTO_IF_ERROR(0);

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if(!elf_checkELF64ProgramHeader(&programHeader)) {
            ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
        }

        elf_loadELF64Program(file, &programHeader);
        ERROR_GOTO_IF_ERROR(0);
    }
    
    Thread* currentThread = schedule_getCurrentThread();
    thread_setupForUserProgram(currentThread);

    barrier();
    pushq(0);   //Reserved for return value
    
    REGISTERS_SAVE();
    
    currentThread->userExitStackTop = (void*)readRegister_RSP_64();
    __usermode_jumpToUserMode((void*)header.entryVaddr, (void*)currentThread->userStack.begin + currentThread->userStack.length);
    asm volatile (
        "__execute_return: mov %%rax, %P0(%%rsp);"   //Save return value immediately
        :
        : "i"(sizeof(Registers))
    );
    
    REGISTERS_RESTORE();    //Restore context

    Uint64 ret = popq();
    barrier();

    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        elf_readELF64ProgramHeader(file, &header, &programHeader, i);
        ERROR_GOTO_IF_ERROR(0);
        
        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }
        
        elf_unloadELF64Program(&programHeader);
        ERROR_GOTO_IF_ERROR(0);
    }

    //TODO: Drop page entries about user program

    return (int)ret;
    ERROR_FINAL_BEGIN(0);
    if (frame != NULL) {
        memory_freeFrames(frame);
    }

    ErrorRecord tmp;
    error_readRecord(&tmp);
    ERROR_CLEAR();  //TODO: Ugly solution

    for (int i = 0; i < loadedHeaderNum; ++i) {
        elf_readELF64ProgramHeader(file, &header, &programHeader, i);
        ERROR_ASSERT_NONE();

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        elf_unloadELF64Program(&programHeader);
        ERROR_ASSERT_NONE();
    }
    error_writeRecord(&tmp);

    return -1;
}

SYSCALL_TABLE_REGISTER(0xFE, __usermode_syscallHandlerExit);    //TODO: Move to process handler