#include<usermode/elf.h>

#include<algorithms.h>
#include<devices/terminal/terminalSwitch.h>
#include<fs/file.h>
#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/pageAlloc.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<multitask/process.h>
#include<print.h>
#include<system/address.h>
#include<system/pageTable.h>

int readELF64Header(File* file, ELF64Header* header) {
    fileSeek(file, 0);
    if (fileRead(file, header, sizeof(ELF64Header)) != 0 || header->identification.magic != ELF_IDENTIFICATION_MAGIC) {
        return -1;
    }

    return 0;
}

void printELF64Header(TerminalLevel level, ELF64Header* header) {
    printf(level, "ELF File Header:\n");
    printf(level, "MAGIC:                       %X\n", header->identification.magic);
    printf(level, "CLASS:                       %u\n", header->identification.class);
    printf(level, "ENDIAN:                      %u\n", header->identification.endian);
    printf(level, "VERSION:                     %u\n", header->identification.version);
    printf(level, "OSABI:                       %u\n", header->identification.osABI);
    printf(level, "TYPE:                        %u\n", header->type);
    printf(level, "MACHINE:                     %u\n", header->machine);
    printf(level, "ELF VERSION:                 %u\n", header->elfVersion);
    printf(level, "ENTRY:                       %#018llX\n", header->entryVaddr);
    printf(level, "PROGRAM HEADER BEGIN:        %#018llX\n", header->programHeadersBegin);
    printf(level, "SECTION HEADER BEGIN:        %#018llX\n", header->sectionHeadersBegin);
    printf(level, "FLAGS:                       %#018llX\n", header->flags);
    printf(level, "HEADER SIZE:                 %u\n", header->headerSize);
    printf(level, "PROGRAM HEADER SIZE:         %u\n", header->programHeaderEntrySize);
    printf(level, "PROGRAM HEADER NUM:          %u\n", header->programHeaderEntryNum);
    printf(level, "SECTION HEADER SIZE:         %u\n", header->sectionHeaderEntrySize);
    printf(level, "SECTION HEADER NUM:          %u\n", header->sectionHeaderEntryNum);
    printf(level, "NANE SECTION HEADER INDEX:   %u\n", header->nameSectionHeaderEntryIndex);
}

int readELF64ProgramHeader(File* file, ELF64Header* elfHeader, ELF64ProgramHeader* programHeader, Index16 index) {
    if (elfHeader->programHeaderEntrySize != sizeof(ELF64ProgramHeader) || index >= elfHeader->programHeaderEntryNum) {
        return -1;
    }

    fileSeek(file, elfHeader->programHeadersBegin + index * sizeof(ELF64ProgramHeader));
    if (fileRead(file, programHeader, sizeof(ELF64ProgramHeader)) != 0) {
        return -1;
    }

    return 0;
}

void printELF64ProgramHeader(TerminalLevel level, ELF64ProgramHeader* header) {
    printf(level, "ELF File Program Header:\n");
    printf(level, "TYPE:        %u\n", header->type);
    printf(level, "FLAGS:       %u\n", header->flags);
    printf(level, "OFFSET:      %#018llX\n", header->offset);
    printf(level, "VADDR:       %#018llX\n", header->vAddr);
    printf(level, "PADDR:       %#018llX\n", header->pAddr);
    printf(level, "MEMORY SIZE: %#018llX\n", header->segmentSizeInMemory);
    printf(level, "FILE SIZE:   %#018llX\n", header->segmentSizeInFile);
    printf(level, "ALIGN:       %#018llX\n", header->align);
}

bool checkELF64ProgramHeader(ELF64ProgramHeader* programHeader) {
    if (programHeader->vAddr >= KERNEL_VIRTUAL_BEGIN) {
        return false;
    }

    if (programHeader->segmentSizeInMemory < programHeader->segmentSizeInFile) {
        return false;
    }

    if (programHeader->segmentSizeInMemory == 0) {
        return false;
    }

    return true;
}

int loadELF64Program(File* file, ELF64ProgramHeader* programHeader) {
    uintptr_t
        pageBegin = CLEAR_VAL_SIMPLE(programHeader->vAddr, 64, PAGE_SIZE_SHIFT),
        fileBegin = CLEAR_VAL_SIMPLE(programHeader->offset, 64, PAGE_SIZE_SHIFT);
    
    size_t
        readRemain = programHeader->segmentSizeInFile,
        memoryRemain = programHeader->segmentSizeInMemory;

    PML4Table* pageTable = getCurrentProcess()->pageTable;
    fileSeek(file, fileBegin);
    uint64_t flags = PAGE_TABLE_ENTRY_FLAG_US | (TEST_FLAGS(programHeader->flags, ELF64_PROGRAM_HEADER_FLAGS_WRITE) ? PAGE_TABLE_ENTRY_FLAG_RW : 0) | PAGE_TABLE_ENTRY_FLAG_PRESENT;
    
    uintptr_t from = programHeader->vAddr, to = min64(programHeader->vAddr + programHeader->segmentSizeInMemory, pageBegin + PAGE_SIZE);
    void* base = (void*)pageBegin;
    while (memoryRemain > 0) {
        void* pAddr = NULL;
        if ((pAddr = translateVaddr(pageTable, base)) == NULL) {
            pAddr = pageAlloc(1, PHYSICAL_PAGE_TYPE_USER_PROGRAM);
            mapAddr(pageTable, base, pAddr);
        }
        pageTableSetFlag(pageTable, base, PAGING_LEVEL_PAGE_TABLE, flags);
        base += PAGE_SIZE;

        size_t readN = min64(readRemain, to - from);
        if (readN > 0) {
            fileRead(file, (void*)from, readN);
            readRemain -= readN;
        }

        memoryRemain -= readN;

        if (to - from > readN) {
            memset((void*)from + readN, 0, to - from - readN);
            memoryRemain -= (to - from - readN);
        }

        from = to;
        to = min64(programHeader->vAddr + programHeader->segmentSizeInMemory, to + PAGE_SIZE);
    }

    return 0;
}

void unloadELF64Program(ELF64ProgramHeader* programHeader) {
    uintptr_t pageBegin = CLEAR_VAL_SIMPLE(programHeader->vAddr, 64, PAGE_SIZE_SHIFT);
    size_t memoryRemain = programHeader->segmentSizeInMemory;

    PML4Table* pageTable = getCurrentProcess()->pageTable;
    uintptr_t from = programHeader->vAddr, to = min64(programHeader->vAddr + programHeader->segmentSizeInMemory, pageBegin + PAGE_SIZE);
    void* base = (void*)pageBegin;
    while (memoryRemain > 0) {
        pageFree(translateVaddr(pageTable, base), 1);
        base += PAGE_SIZE;
        memoryRemain -= (to - from);

        from = to;
        to = min64(programHeader->vAddr + programHeader->segmentSizeInMemory, to + PAGE_SIZE);
    }
}