#include<usermode/elf.h>

#include<algorithms.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/bit.h>
#include<memory/extendedPageTable.h>
#include<memory/frameReaper.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<multitask/schedule.h>
#include<multitask/process.h>
#include<print.h>
#include<system/pageTable.h>
#include<error.h>

void elf_readELF64Header(File* file, ELF64Header* header) {
    fs_fileSeek(file, 0, FS_FILE_SEEK_BEGIN);

    fs_fileRead(file, header, sizeof(ELF64Header));
    ERROR_GOTO_IF_ERROR(0);

    if (header->identification.magic != ELF_IDENTIFICATION_MAGIC) {
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void elf_printELF64Header(ELF64Header* header) {
    print_printf("ELF File Header:\n");
    print_printf("MAGIC:                       %X\n", header->identification.magic);
    print_printf("CLASS:                       %u\n", header->identification.class);
    print_printf("ENDIAN:                      %u\n", header->identification.endian);
    print_printf("VERSION:                     %u\n", header->identification.version);
    print_printf("OSABI:                       %u\n", header->identification.osABI);
    print_printf("TYPE:                        %u\n", header->type);
    print_printf("MACHINE:                     %u\n", header->machine);
    print_printf("ELF VERSION:                 %u\n", header->elfVersion);
    print_printf("ENTRY:                       %#018llX\n", header->entryVaddr);
    print_printf("PROGRAM HEADER BEGIN:        %#018llX\n", header->programHeadersBegin);
    print_printf("SECTION HEADER BEGIN:        %#018llX\n", header->sectionHeadersBegin);
    print_printf("FLAGS:                       %#018llX\n", header->flags);
    print_printf("HEADER SIZE:                 %u\n", header->headerSize);
    print_printf("PROGRAM HEADER SIZE:         %u\n", header->programHeaderEntrySize);
    print_printf("PROGRAM HEADER NUM:          %u\n", header->programHeaderEntryNum);
    print_printf("SECTION HEADER SIZE:         %u\n", header->sectionHeaderEntrySize);
    print_printf("SECTION HEADER NUM:          %u\n", header->sectionHeaderEntryNum);
    print_printf("NANE SECTION HEADER INDEX:   %u\n", header->nameSectionHeaderEntryIndex);
}

void elf_readELF64ProgramHeader(File* file, ELF64Header* elfHeader, ELF64ProgramHeader* programHeader, Index16 index) {
    if (elfHeader->programHeaderEntrySize != sizeof(ELF64ProgramHeader)) {
        ERROR_THROW(ERROR_ID_DATA_ERROR, 0);
    }

    if (index >= elfHeader->programHeaderEntryNum) {
        ERROR_THROW(ERROR_ID_DATA_ERROR, 0);
    }

    fs_fileSeek(file, elfHeader->programHeadersBegin + index * sizeof(ELF64ProgramHeader), FS_FILE_SEEK_BEGIN);

    fs_fileRead(file, programHeader, sizeof(ELF64ProgramHeader));
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void elf_printELF64ProgramHeader(ELF64ProgramHeader* header) {
    print_printf("ELF File Program Header:\n");
    print_printf("TYPE:        %u\n", header->type);
    print_printf("FLAGS:       %u\n", header->flags);
    print_printf("OFFSET:      %#018llX\n", header->offset);
    print_printf("VADDR:       %#018llX\n", header->vAddr);
    print_printf("PADDR:       %#018llX\n", header->pAddr);
    print_printf("MEMORY SIZE: %#018llX\n", header->segmentSizeInMemory);
    print_printf("FILE SIZE:   %#018llX\n", header->segmentSizeInFile);
    print_printf("ALIGN:       %#018llX\n", header->align);
}

bool elf_checkELF64ProgramHeader(ELF64ProgramHeader* programHeader) {
    if (programHeader->vAddr >= MEMORY_LAYOUT_KERNEL_BEGIN) {
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

void elf_loadELF64Program(File* file, ELF64ProgramHeader* programHeader) {
    void* frame = NULL;
    
    Uintptr
        pageBegin = CLEAR_VAL_SIMPLE(programHeader->vAddr, 64, PAGE_SIZE_SHIFT),
        fileBegin = CLEAR_VAL_SIMPLE(programHeader->offset, 64, PAGE_SIZE_SHIFT);
    
    Size
        readRemain = programHeader->segmentSizeInFile,
        memoryRemain = programHeader->segmentSizeInMemory;

    Process* currentProcess = schedule_getCurrentProcess();
    ExtendedPageTableRoot* extendedTable = currentProcess->extendedTable;
    fs_fileSeek(file, fileBegin, FS_FILE_SEEK_BEGIN);
    
    Uintptr from = programHeader->vAddr, to = algorithms_min64(programHeader->vAddr + programHeader->segmentSizeInMemory, from + PAGE_SIZE);
    void* base = (void*)pageBegin;
    while (memoryRemain > 0) {
        void* translated = extendedPageTableRoot_translate(extendedTable, base);
        ERROR_GOTO_IF_ERROR(0);
        DEBUG_ASSERT_SILENT(translated == NULL);

        frame = mm_allocateFrames(1);
        if (frame == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        //TODO: Set to USER_CODE when code load complete
        extendedPageTableRoot_draw(
            extendedTable, base, frame, 1,
            extraPageTableContext_getDefaultPreset(extendedTable->context, MEMORY_DEFAULT_PRESETS_TYPE_USER_DATA),
            EXTENDED_PAGE_TABLE_DRAW_FLAGS_PRESET_OVERWRITE
        );
        ERROR_GOTO_IF_ERROR(0);

        frame = NULL;

        Size readN = algorithms_min64(readRemain, to - from);
        if (readN > 0) {
            fs_fileRead(file, (void*)from, readN);
            ERROR_GOTO_IF_ERROR(0);
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

    return;
    ERROR_FINAL_BEGIN(0);
    if (frame != NULL) {
        mm_freeFrames(frame, 1);
    }

    void* releaseBase = (void*)pageBegin;
    ErrorRecord tmp;
    error_readRecord(&tmp);
    ERROR_CLEAR();  //TODO: Ugly solution
    while (releaseBase < base) {
        void* frame = extendedPageTableRoot_translate(extendedTable, base);
        ERROR_ASSERT_NONE();
        DEBUG_ASSERT_SILENT(frame != NULL);

        extendedPageTableRoot_erase(extendedTable, releaseBase, 1);
        ERROR_ASSERT_NONE();
        releaseBase += PAGE_SIZE;
    }
    frameReaper_reap(&extendedTable->reaper);
    error_writeRecord(&tmp);
}

void elf_unloadELF64Program(ELF64ProgramHeader* programHeader) {
    Uintptr pageBegin = CLEAR_VAL_SIMPLE(programHeader->vAddr, 64, PAGE_SIZE_SHIFT);
    Size memoryRemain = programHeader->segmentSizeInMemory;

    Process* currentProcess = schedule_getCurrentProcess();
    ExtendedPageTableRoot* extendedTable = currentProcess->extendedTable;
    Uintptr from = programHeader->vAddr, to = algorithms_min64(programHeader->vAddr + programHeader->segmentSizeInMemory, pageBegin + PAGE_SIZE);
    void* base = (void*)pageBegin;
    while (memoryRemain > 0) {
        void* frame = extendedPageTableRoot_translate(extendedTable, base);
        ERROR_GOTO_IF_ERROR(0);
        DEBUG_ASSERT_SILENT(frame != NULL);

        extendedPageTableRoot_erase(extendedTable, base, 1);
        ERROR_GOTO_IF_ERROR(0);

        base += PAGE_SIZE;
        memoryRemain -= (to - from);

        from = to;
        to = algorithms_min64(programHeader->vAddr + programHeader->segmentSizeInMemory, to + PAGE_SIZE);
    }

    frameReaper_reap(&extendedTable->reaper);

    return;
    ERROR_FINAL_BEGIN(0);
}