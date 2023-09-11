#include<elf/elf.h>

#include<fs/fileSystem.h>
#include<fs/volume.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<lib/memory.h>
#include<lib/print.h>
#include<system/pageTable.h>

Result readELF64Header(FileSystemEntry* file, ELF64Header* header) {
    if (rawFileSeek(file, 0) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    if (rawFileRead(file, header, sizeof(ELF64Header)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (header->identification.magic != ELF_IDENTIFICATION_MAGIC) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

void printELF64Header(ELF64Header* header) {
    printf("ELF File Header:\n");
    printf("MAGIC:                       %X\n", header->identification.magic);
    printf("CLASS:                       %u\n", header->identification.class);
    printf("ENDIAN:                      %u\n", header->identification.endian);
    printf("VERSION:                     %u\n", header->identification.version);
    printf("OSABI:                       %u\n", header->identification.osABI);
    printf("TYPE:                        %u\n", header->type);
    printf("MACHINE:                     %u\n", header->machine);
    printf("ELF VERSION:                 %u\n", header->elfVersion);
    printf("ENTRY:                       %#010X\n", (Uint32)header->entryVaddr);
    printf("PROGRAM HEADER BEGIN:        %#010X\n", (Uint32)header->programHeadersBegin);
    printf("SECTION HEADER BEGIN:        %#010X\n", (Uint32)header->sectionHeadersBegin);
    printf("FLAGS:                       %#010X\n", header->flags);
    printf("HEADER SIZE:                 %u\n", header->headerSize);
    printf("PROGRAM HEADER SIZE:         %u\n", header->programHeaderEntrySize);
    printf("PROGRAM HEADER NUM:          %u\n", header->programHeaderEntryNum);
    printf("SECTION HEADER SIZE:         %u\n", header->sectionHeaderEntrySize);
    printf("SECTION HEADER NUM:          %u\n", header->sectionHeaderEntryNum);
    printf("NANE SECTION HEADER INDEX:   %u\n", header->nameSectionHeaderEntryIndex);
}

Result readELF64ProgramHeader(FileSystemEntry* file, ELF64Header* elfHeader, ELF64ProgramHeader* programHeader, Index16 index) {
    if (elfHeader->programHeaderEntrySize != sizeof(ELF64ProgramHeader)) {
        return RESULT_FAIL;
    }

    if (index >= elfHeader->programHeaderEntryNum) {
        return RESULT_FAIL;
    }

    if (rawFileSeek(file, elfHeader->programHeadersBegin + index * sizeof(ELF64ProgramHeader)) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    if (rawFileRead(file, programHeader, sizeof(ELF64ProgramHeader)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

void printELF64ProgramHeader(ELF64ProgramHeader* header) {
    printf("ELF File Program Header:\n");
    printf("TYPE:        %u\n", header->type);
    printf("FLAGS:       %u\n", header->flags);
    printf("OFFSET:      %#010X\n", (Uint32)header->offset);
    printf("VADDR:       %#010X\n", (Uint32)header->vAddr);
    printf("PADDR:       %#010X\n", (Uint32)header->pAddr);
    printf("MEMORY SIZE: %#010X\n", (Uint32)header->segmentSizeInMemory);
    printf("FILE SIZE:   %#010X\n", (Uint32)header->segmentSizeInFile);
    printf("ALIGN:       %#010X\n", (Uint32)header->align);
}

Result checkELF64ProgramHeader(ELF64ProgramHeader* programHeader) {
    if (programHeader->segmentSizeInMemory < programHeader->segmentSizeInFile) {
        return RESULT_FAIL;
    }

    if (programHeader->segmentSizeInMemory == 0) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result loadELF64Program(FileSystemEntry* file, ELF64ProgramHeader* programHeader, void* loadTo) {
    memset((void*)(Uintptr)programHeader->vAddr, 0, (Size)programHeader->segmentSizeInMemory);
    if (rawFileSeek(file, (Size)programHeader->offset) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    return rawFileRead(file, (void*)(Uintptr)programHeader->vAddr, (Size)programHeader->segmentSizeInFile);
}

void* loadKernel(Volume* v, ConstCstring kernelPath) {
    FileSystemEntry kernelFile;
    if (rawFileSystemEntryOpen(v, kernelPath, &kernelFile, FILE_SYSTEM_ENTRY_TYPE_FILE) == RESULT_FAIL) {
        return NULL;
    }

    ELF64Header header;
    if (readELF64Header(&kernelFile, &header) == RESULT_FAIL) {
        return NULL;
    }

    ELF64ProgramHeader programHeader;
    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        if (readELF64ProgramHeader(&kernelFile, &header, &programHeader, i) == RESULT_FAIL) {
            return NULL;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (checkELF64ProgramHeader(&programHeader) == RESULT_FAIL) {
            return NULL;
        }

        if (loadELF64Program(&kernelFile, &programHeader, (void*)(Uintptr)programHeader.vAddr) == RESULT_FAIL) {
            return NULL;
        }
    }

    return (void*)(Uintptr)header.entryVaddr;
}