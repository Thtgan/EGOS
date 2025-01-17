#include<driver/disk.h>

#include<kit/types.h>
#include<lib/intn.h>
#include<real/flags/eflags.h>

typedef struct {
    Uint8 DAPsize;
    Uint8 reserved;
    Uint16 n;
    Uint16 offset;
    Uint16 segment;
    Uint64 lba;
} __attribute__((packed)) __DiskAddressPacket;

OldResult readDiskParams(int drive, DiskParams* params) {
    if (params == NULL) {
        return RESULT_ERROR;
    }

    params->paramSize = sizeof(DiskParams);

    IntRegisters regs;
    initRegs(&regs);

    regs.ah = 0x48;
    regs.dl = drive;
    regs.ds = ADDR_TO_SEGMENT(params);
    regs.si = ADDR_TO_OFFSET(params);

    intn(0x13, &regs, &regs);

    if (TEST_FLAGS(regs.eflags, EFLAGS_CF)) {
        return RESULT_ERROR;
    }
    
    return RESULT_SUCCESS;
}

OldResult rawDiskReadSectors(int drive, void* buffer, Index64 begin, Size n) {
    __DiskAddressPacket dap;
    dap.DAPsize = sizeof(__DiskAddressPacket);
    dap.n       = n;
    dap.segment = ADDR_TO_SEGMENT(buffer);
    dap.offset  = ADDR_TO_OFFSET(buffer);
    dap.lba     = begin;

    IntRegisters regs;
    initRegs(&regs);

    regs.ah = 0x42;
    regs.dl = drive;
    regs.ds = ADDR_TO_SEGMENT(&dap);
    regs.si = ADDR_TO_OFFSET(&dap);

    intn(0x13, &regs, &regs);

    if (TEST_FLAGS(regs.eflags, EFLAGS_CF)) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

OldResult rawDiskWriteSectors(int drive, const void* buffer, Index64 begin, Size n) {
    __DiskAddressPacket dap;
    dap.DAPsize = sizeof(__DiskAddressPacket);
    dap.n       = n;
    dap.segment = ADDR_TO_SEGMENT(buffer);
    dap.offset  = ADDR_TO_OFFSET(buffer);
    dap.lba     = begin;

    IntRegisters regs;
    initRegs(&regs);

    regs.ah = 0x43;
    regs.dl = drive;
    regs.ds = ADDR_TO_SEGMENT(&dap);
    regs.si = ADDR_TO_OFFSET(&dap);

    intn(0x13, &regs, &regs);

    if (TEST_FLAGS(regs.eflags, EFLAGS_CF)) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}