#if !defined(__ELF_H)
#define __ELF_H

#include<devices/terminal/terminalSwitch.h>
#include<fs/file.h>
#include<kit/types.h>
#include<returnValue.h>

#define ELF_IDENTIFICATION_MAGIC            0x464C457F  //0x7F + "ELF"

#define ELF_IDENTIFICATION_CLASS_32         1
#define ELF_IDENTIFICATION_CLASS_64         2

#define ELF_IDENTIFICATION_ENDIAN_LITTLE    1
#define ELF_IDENTIFICATION_ENDIAN_BIG       2

#define ELF_IDENTIFICATION_VERSION_CURRENT  1

#define ELF_IDENTIFICATION_OSABI_SYSTEMV    0
#define ELF_IDENTIFICATION_OSABI_HPUX       1
#define ELF_IDENTIFICATION_OSABI_STANDALONE 255

typedef struct {
    uint32_t magic;
    uint8_t class;
    uint8_t endian;
    uint8_t version;
    uint8_t osABI;
    uint8_t unused[8];
} __attribute__((packed)) ELFidentification;

#define ELF64_HEADER_TYPE_NONE          0
#define ELF64_HEADER_TYPE_RELOCATABLE   1
#define ELF64_HEADER_TYPE_EXECUTABLE    2
#define ELF64_HEADER_TYPE_SHARED        3
#define ELF64_HEADER_TYPE_CORE          4
#define ELF64_HEADER_TYPE_LOOS          0xFE00
#define ELF64_HEADER_TYPE_HIOS          0xFEFF
#define ELF64_HEADER_TYPE_LOPROC        0xFF00
#define ELF64_HEADER_TYPE_HIPROC        0xFFFF

#define ELF64_HEADER_MACHINE_NO_SPECIFIC    0x00
#define ELF64_HEADER_MACHINE_SPARC          0x02
#define ELF64_HEADER_MACHINE_X86            0x03
#define ELF64_HEADER_MACHINE_MIPS           0x08
#define ELF64_HEADER_MACHINE_POWERPC        0x14
#define ELF64_HEADER_MACHINE_ARM            0x28
#define ELF64_HEADER_MACHINE_SUPERH         0x2A
#define ELF64_HEADER_MACHINE_IA64           0x32
#define ELF64_HEADER_MACHINE_X86_64         0x3E
#define ELF64_HEADER_MACHINE_AARCH64        0xB7
#define ELF64_HEADER_MACHINE_RISCV          0xF3

typedef struct {
    ELFidentification identification;
    uint16_t type;
    uint16_t machine;
    uint32_t elfVersion;
    uint64_t entryVaddr;
    uint64_t programHeadersBegin;
    uint64_t sectionHeadersBegin;
    uint32_t flags;
    uint16_t headerSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderEntryNum;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderEntryNum;
    uint16_t nameSectionHeaderEntryIndex;
} __attribute__((packed)) ELF64Header;

#define ELF_SPECIAL_SECTION_INDEX_UNDEFINED 0
#define ELF_SPECIAL_SECTION_INDEX_LOPROC    0xFF00
#define ELF_SPECIAL_SECTION_INDEX_HIPROC    0xFF1F
#define ELF_SPECIAL_SECTION_INDEX_LOOS      0xFF20
#define ELF_SPECIAL_SECTION_INDEX_HIOS      0xFF3F
#define ELF_SPECIAL_SECTION_INDEX_ABS       0xFFF1
#define ELF_SPECIAL_SECTION_INDEX_COMMON    0xFFF2

#define ELF_SECTION_HEADER_TYPE_NULL            0
#define ELF_SECTION_HEADER_TYPE_PROGBITS        1
#define ELF_SECTION_HEADER_TYPE_SYMBOL_TABLE    2
#define ELF_SECTION_HEADER_TYPE_STRING_TABLE    3
#define ELF_SECTION_HEADER_TYPE_RELA            4
#define ELF_SECTION_HEADER_TYPE_HASH            5
#define ELF_SECTION_HEADER_TYPE_DYNAMIC         6
#define ELF_SECTION_HEADER_TYPE_NOTE            7
#define ELF_SECTION_HEADER_TYPE_NOBITS          8
#define ELF_SECTION_HEADER_TYPE_REL             9
#define ELF_SECTION_HEADER_TYPE_SHLIB           10
#define ELF_SECTION_HEADER_TYPE_DYNMIC_SYMBOL   11
#define ELF_SECTION_HEADER_TYPE_LOOS            0x60000000
#define ELF_SECTION_HEADER_TYPE_HIOS            0x6FFFFFFF
#define ELF_SECTION_HEADER_TYPE_LOPROC          0x70000000
#define ELF_SECTION_HEADER_TYPE_HIPROC          0x7FFFFFFF

#define ELF_SECTION_HEADER_FLAGS_WRITE          0x00000001
#define ELF_SECTION_HEADER_FLAGS_ALLOC          0x00000002
#define ELF_SECTION_HEADER_FLAGS_EXECUTABLE     0x00000004
#define ELF_SECTION_HEADER_FLAGS_MASKOS         0x0F000000
#define ELF_SECTION_HEADER_FLAGS_MASKPROC       0xF0000000

typedef struct {
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    uint64_t vAddr;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t align;
    uint64_t entriesSize;
} __attribute__((packed)) ELF64SectionHeader;

#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_NOTYPE    0
#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_OBJECT    1
#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_FUNC      2
#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_SECTION   3
#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_FILE      4
#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_LOOS      10
#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_HIOS      12
#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_LOPROC    13
#define ELF64_SYMBOL_TABLE_ENTRY_TYPE_HIPROC    15

#define ELF64_SYMBOL_TABLE_ENTRY_BINDING_LOCAL  0
#define ELF64_SYMBOL_TABLE_ENTRY_BINDING_GLOBAL 1
#define ELF64_SYMBOL_TABLE_ENTRY_BINDING_WEAK   2
#define ELF64_SYMBOL_TABLE_ENTRY_BINDING_LOOS   10
#define ELF64_SYMBOL_TABLE_ENTRY_BINDING_HIOS   12
#define ELF64_SYMBOL_TABLE_ENTRY_BINDING_LOPROC 13
#define ELF64_SYMBOL_TABLE_ENTRY_BINDING_HIPROC 15

typedef struct {
    uint32_t name;
    uint8_t type    : 4;
    uint8_t binding : 4;
    uint8_t reserved;
    uint16_t sectionTableIndex;
    uint64_t symbolValue;
    uint64_t objectSize;
} __attribute__((packed)) ELF64SymbolTableEntry;

typedef struct {
    uint64_t offset;
    uint32_t type;
    uint32_t symbolTableIndex;
} __attribute__((packed)) ELF64REL;

typedef struct {
    uint64_t offset;
    uint32_t type;
    uint32_t symbolTableIndex;
    uint64_t addend;
} __attribute__((packed)) ELF64RELA;

#define ELF64_PROGRAM_HEADER_TYPE_NULL      0
#define ELF64_PROGRAM_HEADER_TYPE_LOAD      1
#define ELF64_PROGRAM_HEADER_TYPE_DYNAMIC   2
#define ELF64_PROGRAM_HEADER_TYPE_INTERP    3
#define ELF64_PROGRAM_HEADER_TYPE_NOTE      4
#define ELF64_PROGRAM_HEADER_TYPE_SHLIB     5
#define ELF64_PROGRAM_HEADER_TYPE_PHDR      6
#define ELF64_PROGRAM_HEADER_TYPE_LOOS      0x60000000
#define ELF64_PROGRAM_HEADER_TYPE_HIOS      0x6FFFFFFF
#define ELF64_PROGRAM_HEADER_TYPE_LOPROC    0x70000000
#define ELF64_PROGRAM_HEADER_TYPE_HIPROC    0x7FFFFFFF

#define ELF64_PROGRAM_HEADER_FLAGS_EXECUTE  0x1
#define ELF64_PROGRAM_HEADER_FLAGS_WRITE    0x2
#define ELF64_PROGRAM_HEADER_FLAGS_READ     0x4
#define ELF64_PROGRAM_HEADER_FLAGS_MAKEOS   0x00FF0000
#define ELF64_PROGRAM_HEADER_FLAGS_MAKEPROC 0xFF000000

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vAddr;
    uint64_t pAddr;
    uint64_t segmentSizeInFile;
    uint64_t segmentSizeInMemory;
    uint64_t align;
} __attribute__((packed)) ELF64ProgramHeader;

ReturnValue readELF64Header(File* file, ELF64Header* header);

void printELF64Header(TerminalLevel level, ELF64Header* header);

ReturnValue readELF64ProgramHeader(File* file, ELF64Header* elfHeader, ELF64ProgramHeader* programHeader, Index16 index);

void printELF64ProgramHeader(TerminalLevel level, ELF64ProgramHeader* header);

bool checkELF64ProgramHeader(ELF64ProgramHeader* programHeader);

ReturnValue loadELF64Program(File* file, ELF64ProgramHeader* programHeader);

void unloadELF64Program(ELF64ProgramHeader* programHeader);

#endif // __ELF_H
