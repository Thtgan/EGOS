#include<memory/stackUtility.h>

#include<blowup.h>
#include<memory/memory.h>
#include<real/simpleAsmLines.h>
#include<stddef.h>
#include<stdio.h>
#include<system/stackFrame.h>

#define STACK_GUARD 0x0000000000000000

//Current Stack Frame design in x86_64:
//             HighMemory
//#===============Main===============# <--Main's Header will use stack guard
//|          Return Address          |
//|       Last Stack Base (RBP)      |
//#==========Main's Header===========# <-+
//|    Main's Local variables...     |   |
//#============Function 1============#   |
//|          Return Address          |   |
//|       Last Stack Base (RBP)------+---+
//#=======Function 1's Header========# <-+
//|  Function 1's Local variables... |   |
//#============Function 2============#   |
//|          Return Address          |   |
//|       Last Stack Base (RBP)------+---+
//#=======Function 2's Header========# <- RBP(Point to header)
//|               ...                |
//|               ...                |
//|               ...                | <- RSP(Unknown position, but must in this function)
//|               ...                |
//             LowMemory

void setupKernelStack(void* kernelStackAddr, void* mainStackBase) {
    void* copyEnd = kernelStackAddr - sizeof(StackFrameHeader);
    memset(copyEnd, 0, sizeof(StackFrameHeader));
    ((StackFrameHeader*)copyEnd)->returnAddr    = 
    ((StackFrameHeader*)copyEnd)->lastStackBase = STACK_GUARD;

    void* oldStackTop = (void*)readRegister_RSP_64();
    size_t n = mainStackBase - oldStackTop;
    void * newStackTop = copyEnd - n;

    memcpy(newStackTop, oldStackTop, n);
    size_t offset = newStackTop - oldStackTop;
    void* stackBase = (void*)readRegister_RBP_64();
    for (
        StackFrameHeader* header = stackBase, * headerInNewStack = stackBase + offset;
        header != mainStackBase;
        header = header->lastStackBase, headerInNewStack = headerInNewStack->lastStackBase
        ) {
        headerInNewStack->lastStackBase = header->lastStackBase + offset;
    }

    if ((void*)readRegister_RSP_64() != oldStackTop) {
        blowup("Setting up kernel stack failed!\n");
    }

    writeRegister_RBP_64((uint64_t)(stackBase + offset));
    writeRegister_RSP_64((uint64_t)newStackTop);
}

void copyCurrentStack(void* oldStackTop, void* oldStackBase, void* newStackBottom) {
    
}