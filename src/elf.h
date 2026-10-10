#ifndef DBG_ELF_H
#define DBG_ELF_H

#include <stddef.h>
#include <stdint.h>

#define EI_NIDENT   16

#define EI_CLASS    4
#define EI_DATA     5

#define ELFCLASS64  2
#define ELFDATA2LSB 1

/// https://github.com/anza-xyz/sbpf/blob/main/src/elf_parser/consts.rs#L18-L19
/// https://github.com/anza-xyz/sbpf/blob/main/src/elf.rs#L841-L843
#define EM_BPF  247
#define EM_SBPF 263

/// yanked from man 5 elf
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

_Static_assert(sizeof(Elf64_Ehdr) == 64, "ELF header must be 64 bytes");

int  elf_has_magic(const unsigned char *buf);

void elf_print_header(const Elf64_Ehdr *eh);

int  elf_check_header(const Elf64_Ehdr *eh, size_t len);

#endif
