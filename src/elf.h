#ifndef DBG_ELF_H
#define DBG_ELF_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/// https://github.com/anza-xyz/sbpf/blob/main/src/elf_parser/consts.rs#L18-L19
/// https://github.com/anza-xyz/sbpf/blob/main/src/elf.rs#L841-L843
#define EM_BPF      247
#define EM_SBPF     263

#define EI_NIDENT   16

#define EI_CLASS    4
#define EI_DATA     5

#define ELFCLASS64  2
#define ELFDATA2LSB 1

#define SHN_UNDEF   0

/// https://codebrowser.dev/linux/include/elf.h.html
#define SHT_SYMTAB 2
#define STT_FUNC   2

/// yanked from `:Man elf`
typedef struct {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

/// yanked from `:Man elf`
typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

/// yanked from `:Man elf`
typedef struct {
    uint32_t st_name;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

_Static_assert(sizeof(Elf64_Ehdr) == 64, "ELF header must be 64 bytes");
_Static_assert(sizeof(Elf64_Shdr) == 64, "section header must be 64 bytes");
_Static_assert(sizeof(Elf64_Sym) == 24, "symbol must be 24 bytes");

int  elf_has_magic(const unsigned char *buf);

void elf_print_header(FILE *out, const Elf64_Ehdr *eh);

int  elf_check_header(const Elf64_Ehdr *eh, size_t len);

void elf_print_sections(FILE *out, const unsigned char *buf, size_t len,
                        const Elf64_Ehdr *eh);

void elf_print_symbols(FILE *out, const unsigned char *buf, size_t len,
                       const Elf64_Ehdr *eh);

int elf_find_section(const unsigned char *buf, size_t len, const Elf64_Ehdr *eh,
                     const char *name, Elf64_Shdr *out);

#endif
