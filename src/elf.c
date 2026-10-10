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
