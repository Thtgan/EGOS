#include<memory/stackUtility.h>

#include<blowup.h>
#include<memory/memory.h>
#include<memory/virtualMalloc.h>
#include<real/simpleAsmLines.h>
#include<stddef.h>
#include<system/address.h>
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
//|  Function 2's Local variables... |
//#============Function 3============#
//|          Return Address          |
//#=======Function 3's Return========# <- Seems GCC will not push RBP at the Function does not call other functions, so this is not a header
//|  Function 3's Local variables... | <- RSP(Unknown position, but must in this function)
//#==================================#
//             LowMemory

void setupKernelStack(void* mainStackBase) {
    void* newStackBottom = vMalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;

    void* copyEnd = newStackBottom - sizeof(StackFrameHeader);
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

size_t copyCurrentStack(void* oldStackTop, void* oldStackBase, void* newStackBottom) {
    StackFrameHeader* header = (StackFrameHeader*)oldStackBase;
    while (header->lastStackBase != STACK_GUARD) {
        header = (StackFrameHeader*)header->lastStackBase;
    }
    size_t size = (void*)header - oldStackTop + sizeof(StackFrameHeader);

    memcpy(newStackBottom - size, oldStackTop, size);

    size_t offset = newStackBottom - size - oldStackTop;
    header = (StackFrameHeader*)(oldStackBase + offset);
    while (header->lastStackBase != STACK_GUARD) {
        header->lastStackBase += offset;
        header = (StackFrameHeader*)header->lastStackBase;
    }

    return size;
}