#if !defined(__USERMODE_ELF_H)
#define __USERMODE_ELF_H

typedef struct ELFidentification ELFidentification;
typedef struct ELF64Header ELF64Header;
typedef struct ELF64SectionHeader ELF64SectionHeader;
typedef struct ELF64SymbolTableEntry ELF64SymbolTableEntry;
typedef struct ELF64REL ELF64REL;
typedef struct ELF64RELA ELF64RELA;
typedef struct ELF64ProgramHeader ELF64ProgramHeader;

#include<fs/fsEntry.h>
#include<kit/types.h>

#define ELF_IDENTIFICATION_MAGIC            0x464C457F  //0x7F + "ELF"

#define ELF_IDENTIFICATION_CLASS_32         1
#define ELF_IDENTIFICATION_CLASS_64         2

#define ELF_IDENTIFICATION_ENDIAN_LITTLE    1
#define ELF_IDENTIFICATION_ENDIAN_BIG       2

#define ELF_IDENTIFICATION_VERSION_CURRENT  1

#define ELF_IDENTIFICATION_OSABI_SYSTEMV    0
#define ELF_IDENTIFICATION_OSABI_HPUX       1
#define ELF_IDENTIFICATION_OSABI_STANDALONE 255

typedef struct ELFidentification {
    Uint32 magic;
    Uint8 class;
    Uint8 endian;
    Uint8 version;
    Uint8 osABI;
    Uint8 unused[8];
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

typedef struct ELF64Header {
    ELFidentification identification;
    Uint16 type;
    Uint16 machine;
    Uint32 elfVersion;
    Uint64 entryVaddr;
    Uint64 programHeadersBegin;
    Uint64 sectionHeadersBegin;
    Uint32 flags;
    Uint16 headerSize;
    Uint16 programHeaderEntrySize;
    Uint16 programHeaderEntryNum;
    Uint16 sectionHeaderEntrySize;
    Uint16 sectionHeaderEntryNum;
    Uint16 nameSectionHeaderEntryIndex;
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

typedef struct ELF64SectionHeader {
    Uint32 name;
    Uint32 type;
    Uint64 flags;
    Uint64 vAddr;
    Uint64 offset;
    Uint64 size;
    Uint32 link;
    Uint32 info;
    Uint64 align;
    Uint64 entriesSize;
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

typedef struct ELF64SymbolTableEntry {
    Uint32 name;
    Uint8 type    : 4;
    Uint8 binding : 4;
    Uint8 reserved;
    Uint16 sectionTableIndex;
    Uint64 symbolValue;
    Uint64 objectSize;
} __attribute__((packed)) ELF64SymbolTableEntry;

typedef struct ELF64REL {
    Uint64 offset;
    Uint32 type;
    Uint32 symbolTableIndex;
} __attribute__((packed)) ELF64REL;

typedef struct ELF64RELA {
    Uint64 offset;
    Uint32 type;
    Uint32 symbolTableIndex;
    Uint64 addend;
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

typedef struct ELF64ProgramHeader {
    Uint32 type;
    Uint32 flags;
    Uint64 offset;
    Uint64 vAddr;
    Uint64 pAddr;
    Uint64 segmentSizeInFile;
    Uint64 segmentSizeInMemory;
    Uint64 align;
} __attribute__((packed)) ELF64ProgramHeader;

/**
 * @brief Read ELF header from file, sets errorcode to indicate error
 * 
 * @param file ELF file
 * @param header Header struct
 */
void elf_readELF64Header(File* file, ELF64Header* header);

/**
 * @brief Print ELF header for debug 
 * 
 * @param level Terminal level to print to
 * @param header ELF header to print
 */
void elf_printELF64Header(ELF64Header* header);

/**
 * @brief Read ELF program header from file, sets errorcode to indicate error
 * 
 * @param file ELF file
 * @param elfHeader ELF header
 * @param programHeader Program header struct
 * @param index Index of program header
 */
void elf_readELF64ProgramHeader(File* file, ELF64Header* elfHeader, ELF64ProgramHeader* programHeader, Index16 index);

/**
 * @brief Print ELF program header for debug 
 * 
 * @param level Terminal level to print to
 * @param header ELF program header to print
 */
void elf_printELF64ProgramHeader(ELF64ProgramHeader* header);

/**
 * @brief Check is ELF program header leagel?
 * 
 * @param programHeader ELF program header
 * @return void True if header is leagal
 */
bool elf_checkELF64ProgramHeader(ELF64ProgramHeader* programHeader);

/**
 * @brief Load program corresponded to program header from ELF file, sets errorcode to indicate error
 * 
 * @param file ELF file
 * @param programHeader Program header describes program to load
 */
void elf_loadELF64Program(File* file, ELF64ProgramHeader* programHeader);

/**
 * @brief Unload ELF program loaded
 * 
 * @param programHeader Program header describes program loaded
 */
void elf_unloadELF64Program(ELF64ProgramHeader* programHeader);

#endif // __USERMODE_ELF_H
