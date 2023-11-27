#include<usermode/elf.h>

#include<algorithms.h>
#include<devices/terminal/terminalSwitch.h>
#include<error.h>
#include<fs/fsutil.h>
#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<multitask/schedule.h>
#include<print.h>
#include<system/address.h>
#include<system/pageTable.h>

Result readELF64Header(File* file, ELF64Header* header) {
    if (fsutil_fileSeek(file, 0, FILE_SEEK_BEGIN) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    if (fsutil_fileRead(file, header, sizeof(ELF64Header)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (header->identification.magic != ELF_IDENTIFICATION_MAGIC) {
        SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
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

Result readELF64ProgramHeader(File* file, ELF64Header* elfHeader, ELF64ProgramHeader* programHeader, Index16 index) {
    if (elfHeader->programHeaderEntrySize != sizeof(ELF64ProgramHeader)) {
        SET_ERROR_CODE(ERROR_OBJECT_DATA, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    if (index >= elfHeader->programHeaderEntryNum) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }

    if (fsutil_fileSeek(file, elfHeader->programHeadersBegin + index * sizeof(ELF64ProgramHeader), FILE_SEEK_BEGIN) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    if (fsutil_fileRead(file, programHeader, sizeof(ELF64ProgramHeader)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
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

Result checkELF64ProgramHeader(ELF64ProgramHeader* programHeader) {
    if (programHeader->vAddr >= KERNEL_VIRTUAL_BEGIN) {
        return RESULT_FAIL;
    }

    if (programHeader->segmentSizeInMemory < programHeader->segmentSizeInFile) {
        return RESULT_FAIL;
    }

    if (programHeader->segmentSizeInMemory == 0) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result loadELF64Program(File* file, ELF64ProgramHeader* programHeader) {
    Uintptr
        pageBegin = CLEAR_VAL_SIMPLE(programHeader->vAddr, 64, PAGE_SIZE_SHIFT),
        fileBegin = CLEAR_VAL_SIMPLE(programHeader->offset, 64, PAGE_SIZE_SHIFT);
    
    Size
        readRemain = programHeader->segmentSizeInFile,
        memoryRemain = programHeader->segmentSizeInMemory;

    PML4Table* pageTable = schedulerGetCurrentProcess()->context.pageTable;
    if (fsutil_fileSeek(file, fileBegin, FILE_SEEK_BEGIN) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    Uint64 flags = PAGING_ENTRY_FLAG_US | (TEST_FLAGS(programHeader->flags, ELF64_PROGRAM_HEADER_FLAGS_WRITE) ? PAGING_ENTRY_FLAG_RW : 0) | PAGING_ENTRY_FLAG_PRESENT;
    
    Uintptr from = programHeader->vAddr, to = min64(programHeader->vAddr + programHeader->segmentSizeInMemory, pageBegin + PAGE_SIZE);
    void* base = (void*)pageBegin;
    while (memoryRemain > 0) {
        void* pAddr = NULL;
        if ((pAddr = translateVaddr(pageTable, base)) == NULL) {
            pAddr = pageAlloc(1, MEMORY_TYPE_USER_PROGRAM);
            if (pAddr == NULL || mapAddr(pageTable, base, pAddr, flags) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }

        PagingLevel level;
        PagingEntry* entry = pageTableGetEntry(pageTable, base, &level);
        if (entry == NULL || level != PAGING_LEVEL_PAGE_TABLE) {    //TODO: Ugly code
            return RESULT_FAIL;
        }

        *entry = BUILD_ENTRY_PS(PAGING_LEVEL_PAGE_TABLE, BASE_FROM_ENTRY_PS(PAGING_LEVEL_PAGE_TABLE, *entry), flags);

        base += PAGE_SIZE;

        Size readN = min64(readRemain, to - from);
        if (readN > 0) {
            if (fsutil_fileRead(file, (void*)from, readN) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
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

    return RESULT_SUCCESS;
}

Result unloadELF64Program(ELF64ProgramHeader* programHeader) {
    Uintptr pageBegin = CLEAR_VAL_SIMPLE(programHeader->vAddr, 64, PAGE_SIZE_SHIFT);
    Size memoryRemain = programHeader->segmentSizeInMemory;

    PML4Table* pageTable = schedulerGetCurrentProcess()->context.pageTable;
    Uintptr from = programHeader->vAddr, to = min64(programHeader->vAddr + programHeader->segmentSizeInMemory, pageBegin + PAGE_SIZE);
    void* base = (void*)pageBegin;
    while (memoryRemain > 0) {
        void* pAddr = translateVaddr(pageTable, base);
        if (pAddr == NULL) {
            return RESULT_FAIL;
        }
        pageFree(pAddr);
        base += PAGE_SIZE;
        memoryRemain -= (to - from);

        from = to;
        to = min64(programHeader->vAddr + programHeader->segmentSizeInMemory, to + PAGE_SIZE);
    }

    return RESULT_SUCCESS;
}