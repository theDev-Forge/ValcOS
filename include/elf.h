#ifndef ELF_H
#define ELF_H

#include <stdint.h>

/**
 * ELF (Executable and Linkable Format) Loader
 * 
 * Supports 32-bit ELF binaries for x86
 */

// ELF magic number
#define ELF_MAGIC 0x464C457F  // "\x7FELF"

// ELF classes
#define ELFCLASS32 1
#define ELFCLASS64 2

// ELF data encoding
#define ELFDATA2LSB 1  // Little endian
#define ELFDATA2MSB 2  // Big endian

// ELF types
#define ET_NONE 0
#define ET_REL  1  // Relocatable
#define ET_EXEC 2  // Executable
#define ET_DYN  3  // Shared object
#define ET_CORE 4  // Core file

// ELF machine types
#define EM_386  3   // Intel 80386
#define EM_X86_64 62 // AMD x86-64

// Program header types
#define PT_NULL    0
#define PT_LOAD    1  // Loadable segment
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4

// Program header flags
#define PF_X 0x1  // Execute
#define PF_W 0x2  // Write
#define PF_R 0x4  // Read

// ELF header (32-bit)
typedef struct {
    uint8_t  e_ident[16];     // Magic number and other info
    uint16_t e_type;          // Object file type
    uint16_t e_machine;       // Architecture
    uint32_t e_version;       // Object file version
    uint32_t e_entry;         // Entry point virtual address
    uint32_t e_phoff;         // Program header table offset
    uint32_t e_shoff;         // Section header table offset
    uint32_t e_flags;         // Processor-specific flags
    uint16_t e_ehsize;        // ELF header size
    uint16_t e_phentsize;     // Program header entry size
    uint16_t e_phnum;         // Program header entry count
    uint16_t e_shentsize;     // Section header entry size
    uint16_t e_shnum;         // Section header entry count
    uint16_t e_shstrndx;      // Section header string table index
} __attribute__((packed)) elf32_ehdr_t;

// Program header (32-bit)
typedef struct {
    uint32_t p_type;          // Segment type
    uint32_t p_offset;        // Segment file offset
    uint32_t p_vaddr;         // Segment virtual address
    uint32_t p_paddr;         // Segment physical address
    uint32_t p_filesz;        // Segment size in file
    uint32_t p_memsz;         // Segment size in memory
    uint32_t p_flags;         // Segment flags
    uint32_t p_align;         // Segment alignment
} __attribute__((packed)) elf32_phdr_t;

/**
 * elf_validate - Validate ELF header
 * @ehdr: ELF header
 * Returns: 0 if valid, -1 if invalid
 */
int elf_validate(elf32_ehdr_t *ehdr);

/**
 * elf_load - Load ELF binary into memory
 * @data: ELF file data
 * @size: File size
 * Returns: Entry point address or 0 on error
 */
uint32_t elf_load(void *data, uint32_t size);

/**
 * elf_exec - Execute ELF binary
 * @path: Path to ELF file
 * Returns: 0 on success, -1 on error
 */
int elf_exec(const char *path);

#endif /* ELF_H */
