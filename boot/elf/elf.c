#include<elf/elf.h>

#include<fs/fileSystem.h>
#include<fs/volume.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<lib/memory.h>
#include<lib/print.h>
#include<system/E820.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

Result readELF64Header(FileSystemEntry* file, ELF64Header* header) {
    if (rawFileSeek(file, 0) == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    if (rawFileRead(file, header, sizeof(ELF64Header)) == RESULT_ERROR) {
        return RESULT_ERROR;
    }

    if (header->identification.magic != ELF_IDENTIFICATION_MAGIC) {
        return RESULT_ERROR;
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
        return RESULT_ERROR;
    }

    if (index >= elfHeader->programHeaderEntryNum) {
        return RESULT_ERROR;
    }

    if (rawFileSeek(file, elfHeader->programHeadersBegin + index * sizeof(ELF64ProgramHeader)) == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    if (rawFileRead(file, programHeader, sizeof(ELF64ProgramHeader)) == RESULT_ERROR) {
        return RESULT_ERROR;
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
        return RESULT_ERROR;
    }

    if (programHeader->segmentSizeInMemory == 0) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

Result loadELF64Program(FileSystemEntry* file, ELF64ProgramHeader* programHeader) {
    memset((void*)(Uintptr)programHeader->pAddr, 0, (Size)programHeader->segmentSizeInMemory);
    if (rawFileSeek(file, (Size)programHeader->offset) == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    return rawFileRead(file, (void*)(Uintptr)programHeader->pAddr, (Size)programHeader->segmentSizeInFile);
}

void* loadKernel(Volume* v, ConstCstring kernelPath, MemoryMap* mMap) {
    FileSystemEntry kernelFile;
    if (rawFileSystemEntryOpen(v, kernelPath, &kernelFile, FILE_SYSTEM_ENTRY_TYPE_FILE) == RESULT_ERROR) {
        return NULL;
    }

    ELF64Header header;
    if (readELF64Header(&kernelFile, &header) == RESULT_ERROR) {
        return NULL;
    }

    ELF64ProgramHeader programHeader;
    for (int i = 0; i < header.programHeaderEntryNum; ++i) {
        if (readELF64ProgramHeader(&kernelFile, &header, &programHeader, i) == RESULT_ERROR) {
            return NULL;
        }

        if (programHeader.type != ELF64_PROGRAM_HEADER_TYPE_LOAD) {
            continue;
        }

        if (checkELF64ProgramHeader(&programHeader) == RESULT_ERROR) {
            return NULL;
        }

        Uint32 l2 = ALIGN_DOWN((Uint32)programHeader.pAddr, PAGE_SIZE), r2 = ALIGN_UP((Uint32)programHeader.pAddr + (Uint32)programHeader.segmentSizeInMemory, PAGE_SIZE);
        
        MemoryMapEntry* entry = NULL;
        for (int i = 0; i < mMap->entryNum; ++i) {
            MemoryMapEntry* e = mMap->memoryMapEntries + i;

            if (RANGE_WITHIN((Uint32)e->base, (Uint32)e->base + (Uint32)e->length, l2, r2, <=, <=)) {
                entry = e;
            }
        }

        if (entry == NULL || entry->type != MEMORY_MAP_ENTRY_TYPE_RAM) {
            return NULL;
        }

        Uint32 l1 = (Uint32)entry->base, r1 = (Uint32)entry->base + (Uint32)entry->length;

        if (l1 < l2) {
            if (E820SplitEntry(mMap, entry, l2 - l1, &entry) == RESULT_ERROR) {
                return NULL;
            }
        }

        if (r2 < r1) {
            if (E820SplitEntry(mMap, entry, r2 - l2, NULL) == RESULT_ERROR) {
                return NULL;
            }
        }

        entry->type = MEMORY_MAP_ENTRY_TYPE_RESERVED;

        if (loadELF64Program(&kernelFile, &programHeader) == RESULT_ERROR) {
            return NULL;
        }
    }

    E820TidyUp(mMap);

    return (void*)(Uintptr)header.entryVaddr;
}