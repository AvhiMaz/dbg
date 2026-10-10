#include "elf.h"

#include <stdio.h>

int elf_has_magic(const unsigned char *buf) {
    return buf[0] == 0x7f && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F';
}

void elf_print_header(const Elf64_Ehdr *eh) {
    printf("type:       %u\n", eh->e_type);
    printf("machine:    %u\n", eh->e_machine);
    printf("version:    %u\n", eh->e_version);
    printf("entry:      %#lx\n", eh->e_entry);
    printf("phoff:      %#lx\n", eh->e_phoff);
    printf("shoff:      %lu\n", eh->e_shoff);
    printf("flags:      %u\n", eh->e_flags);
    printf("ehsize:     %u\n", eh->e_ehsize);
    printf("phentsize:  %u\n", eh->e_phentsize);
    printf("phnum:      %u\n", eh->e_phnum);
    printf("shentsize:  %u\n", eh->e_shentsize);
    printf("shnum:      %u\n", eh->e_shnum);
    printf("shstrndx:   %u\n", eh->e_shstrndx);
}

int elf_check_header(const Elf64_Ehdr *eh, size_t len) {

    if (eh->e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "error: not a 64-bit ELF\n");
        return 0;
    }

    if (eh->e_ident[EI_DATA] != ELFDATA2LSB) {
        fprintf(stderr, "error: not a little-endian ELF\n");
        return 0;
    }

    if (eh->e_machine != EM_BPF && eh->e_machine != EM_SBPF) {
        fprintf(stderr, "error: not an sBPF program (machine %u)\n",
                eh->e_machine);
        return 0;
    }

    if (eh->e_shentsize != 64) {
        fprintf(stderr, "error: unexpected section header size %u\n",
                eh->e_shentsize);
        return 0;
    }

    uint64_t table_size = (uint64_t)eh->e_shnum * eh->e_shentsize;

    if (eh->e_shoff > len || table_size > len - eh->e_shoff) {
        fprintf(stderr, "error: section header table is outside the file\n");
        return 0;
    }

    return 1;
}
