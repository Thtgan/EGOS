#include<usermode/elf.h>

#include<algorithms.h>
#include<devices/terminal/terminalSwitch.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/bit.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<multitask/schedule.h>
#include<print.h>
#include<system/pageTable.h>

OldResult elf_readELF64Header(File* file, ELF64Header* header) {
    if (fs_fileSeek(file, 0, FS_FILE_SEEK_BEGIN) == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    if (fs_fileRead(file, header, sizeof(ELF64Header)) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    if (header->identification.magic != ELF_IDENTIFICATION_MAGIC) {
        // ERROR_CODE_SET(ERROR_CODE_OBJECT_FILE, ERROR_CODE_STATUS_VERIFIVCATION_FAIL);
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

void elf_printELF64Header(TerminalLevel level, ELF64Header* header) {
    print_printf(level, "ELF File Header:\n");
    print_printf(level, "MAGIC:                       %X\n", header->identification.magic);
    print_printf(level, "CLASS:                       %u\n", header->identification.class);
    print_printf(level, "ENDIAN:                      %u\n", header->identification.endian);
    print_printf(level, "VERSION:                     %u\n", header->identification.version);
    print_printf(level, "OSABI:                       %u\n", header->identification.osABI);
    print_printf(level, "TYPE:                        %u\n", header->type);
    print_printf(level, "MACHINE:                     %u\n", header->machine);
    print_printf(level, "ELF VERSION:                 %u\n", header->elfVersion);
    print_printf(level, "ENTRY:                       %#018llX\n", header->entryVaddr);
    print_printf(level, "PROGRAM HEADER BEGIN:        %#018llX\n", header->programHeadersBegin);
    print_printf(level, "SECTION HEADER BEGIN:        %#018llX\n", header->sectionHeadersBegin);
    print_printf(level, "FLAGS:                       %#018llX\n", header->flags);
    print_printf(level, "HEADER SIZE:                 %u\n", header->headerSize);
    print_printf(level, "PROGRAM HEADER SIZE:         %u\n", header->programHeaderEntrySize);
    print_printf(level, "PROGRAM HEADER NUM:          %u\n", header->programHeaderEntryNum);
    print_printf(level, "SECTION HEADER SIZE:         %u\n", header->sectionHeaderEntrySize);
    print_printf(level, "SECTION HEADER NUM:          %u\n", header->sectionHeaderEntryNum);
    print_printf(level, "NANE SECTION HEADER INDEX:   %u\n", header->nameSectionHeaderEntryIndex);
}

OldResult elf_readELF64ProgramHeader(File* file, ELF64Header* elfHeader, ELF64ProgramHeader* programHeader, Index16 index) {
    if (elfHeader->programHeaderEntrySize != sizeof(ELF64ProgramHeader)) {
        // ERROR_CODE_SET(ERROR_CODE_OBJECT_DATA, ERROR_CODE_STATUS_VERIFIVCATION_FAIL);
        return RESULT_ERROR;
    }

    if (index >= elfHeader->programHeaderEntryNum) {
        // ERROR_CODE_SET(ERROR_CODE_OBJECT_INDEX, ERROR_CODE_STATUS_OUT_OF_BOUND);
        return RESULT_ERROR;
    }

    if (fs_fileSeek(file, elfHeader->programHeadersBegin + index * sizeof(ELF64ProgramHeader), FS_FILE_SEEK_BEGIN) == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    if (fs_fileRead(file, programHeader, sizeof(ELF64ProgramHeader)) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

void elf_printELF64ProgramHeader(TerminalLevel level, ELF64ProgramHeader* header) {
    print_printf(level, "ELF File Program Header:\n");
    print_printf(level, "TYPE:        %u\n", header->type);
    print_printf(level, "FLAGS:       %u\n", header->flags);
    print_printf(level, "OFFSET:      %#018llX\n", header->offset);
    print_printf(level, "VADDR:       %#018llX\n", header->vAddr);
    print_printf(level, "PADDR:       %#018llX\n", header->pAddr);
    print_printf(level, "MEMORY SIZE: %#018llX\n", header->segmentSizeInMemory);
    print_printf(level, "FILE SIZE:   %#018llX\n", header->segmentSizeInFile);
    print_printf(level, "ALIGN:       %#018llX\n", header->align);
}

OldResult elf_checkELF64ProgramHeader(ELF64ProgramHeader* programHeader) {
    if (programHeader->vAddr >= MEMORY_LAYOUT_KERNEL_BEGIN) {
        return RESULT_ERROR;
    }

    if (programHeader->segmentSizeInMemory < programHeader->segmentSizeInFile) {
        return RESULT_ERROR;
    }

    if (programHeader->segmentSizeInMemory == 0) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

OldResult elf_loadELF64Program(File* file, ELF64ProgramHeader* programHeader) {
    Uintptr
        pageBegin = CLEAR_VAL_SIMPLE(programHeader->vAddr, 64, PAGE_SIZE_SHIFT),
        fileBegin = CLEAR_VAL_SIMPLE(programHeader->offset, 64, PAGE_SIZE_SHIFT);
    
    Size
        readRemain = programHeader->segmentSizeInFile,
        memoryRemain = programHeader->segmentSizeInMemory;

    Scheduler* scheduler = schedule_getCurrentScheduler();
    ExtendedPageTableRoot* extendedTable = scheduler_getCurrentProcess(scheduler)->context.extendedTable;
    if (fs_fileSeek(file, fileBegin, FS_FILE_SEEK_BEGIN) == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    Uint64 flags = PAGING_ENTRY_FLAG_US | (TEST_FLAGS(programHeader->flags, ELF64_PROGRAM_HEADER_FLAGS_WRITE) ? PAGING_ENTRY_FLAG_RW : 0) | PAGING_ENTRY_FLAG_PRESENT;
    
    Uintptr from = programHeader->vAddr, to = algorithms_min64(programHeader->vAddr + programHeader->segmentSizeInMemory, from + PAGE_SIZE);
    void* base = (void*)pageBegin;
    while (memoryRemain > 0) {
        void* pAddr = NULL;
        if ((pAddr = extendedPageTableRoot_translate(extendedTable, base)) == NULL) {
            pAddr = memory_allocateFrame(1);
            if (pAddr == NULL) {
                return RESULT_ERROR;
            }

            //TODO: Set to USER_CODE when code load complete
            ERROR_TRY_CATCH_DIRECT(
                extendedPageTableRoot_draw(extendedTable, base, pAddr, 1, extendedTable->context->presets[EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(extendedTable->context, MEMORY_DEFAULT_PRESETS_TYPE_USER_DATA)]),
                ERROR_CATCH_DEFAULT_CODES_CRASH
            );
        }

        Size readN = algorithms_min64(readRemain, to - from);
        if (readN > 0) {
            if (fs_fileRead(file, (void*)from, readN) != RESULT_SUCCESS) {
                return RESULT_ERROR;
            }
            readRemain -= readN;
        }

        memoryRemain -= readN;

        if (to - from > readN) {
            memory_memset((void*)from + readN, 0, to - from - readN);
            memoryRemain -= (to - from - readN);
        }

        base += PAGE_SIZE;
        from = to;
        to = algorithms_min64(programHeader->vAddr + programHeader->segmentSizeInMemory, from + PAGE_SIZE);
    }

    return RESULT_SUCCESS;
}

OldResult elf_unloadELF64Program(ELF64ProgramHeader* programHeader) {
    Uintptr pageBegin = CLEAR_VAL_SIMPLE(programHeader->vAddr, 64, PAGE_SIZE_SHIFT);
    Size memoryRemain = programHeader->segmentSizeInMemory;

    Scheduler* scheduler = schedule_getCurrentScheduler();
    ExtendedPageTableRoot* extendedTable = scheduler_getCurrentProcess(scheduler)->context.extendedTable;
    Uintptr from = programHeader->vAddr, to = algorithms_min64(programHeader->vAddr + programHeader->segmentSizeInMemory, pageBegin + PAGE_SIZE);
    void* base = (void*)pageBegin;
    while (memoryRemain > 0) {
        void* pAddr = extendedPageTableRoot_translate(extendedTable, base);
        if (pAddr == NULL) {
            return RESULT_ERROR;
        }

        ERROR_TRY_CATCH_DIRECT(
            extendedPageTableRoot_erase(extendedTable, base, 1),
            ERROR_CATCH_DEFAULT_CODES_CRASH
        );

        memory_freeFrame(pAddr);
        base += PAGE_SIZE;
        memoryRemain -= (to - from);

        from = to;
        to = algorithms_min64(programHeader->vAddr + programHeader->segmentSizeInMemory, to + PAGE_SIZE);
    }

    return RESULT_SUCCESS;
}