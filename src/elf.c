#include "elf.h"

#include <stdio.h>
#include <string.h>

static Elf64_Shdr elf_section(const unsigned char *buf, const Elf64_Ehdr *eh,
                              int i) {
    Elf64_Shdr sh;
    memcpy(&sh, buf + eh->e_shoff + i * sizeof(sh), sizeof(sh));
    return sh;
}

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

    if (eh->e_shentsize != sizeof(Elf64_Shdr)) {
        fprintf(stderr, "error: unexpected section header size %u\n",
                eh->e_shentsize);
        return 0;
    }

    uint64_t table_size = (uint64_t)eh->e_shnum * eh->e_shentsize;

    if (eh->e_shoff > len || table_size > len - eh->e_shoff) {
        fprintf(stderr, "error: section header table is outside the file\n");
        return 0;
    }

    if (eh->e_shstrndx == SHN_UNDEF) {
        fprintf(stderr, "error: file has no section name table\n");
        return 0;
    }

    if (eh->e_shstrndx >= eh->e_shnum) {
        fprintf(stderr, "error: section name table index %u out of range\n",
                eh->e_shstrndx);
        return 0;
    }

    return 1;
}

void elf_print_sections(const unsigned char *buf, size_t len,
                        const Elf64_Ehdr *eh) {

    Elf64_Shdr shstrtab = elf_section(buf, eh, eh->e_shstrndx);

    if (shstrtab.sh_offset > len ||
        shstrtab.sh_size > len - shstrtab.sh_offset) {
        fprintf(stderr, "error: section name table is outside the file\n");
        return;
    }

    const char *names = (const char *)buf + shstrtab.sh_offset;

    if (shstrtab.sh_size == 0 || names[shstrtab.sh_size - 1] != '\0') {
        fprintf(stderr, "error: section name table is not NUL-terminated\n");
        return;
    }

    for (int i = 0; i < eh->e_shnum; i++) {
        Elf64_Shdr sh = elf_section(buf, eh, i);
        if (sh.sh_name >= shstrtab.sh_size) {
            printf("[%2d] <bad name>\n", i);
            continue;
        }
        printf("[%2d] %-16s type=%u offset=%#lx size=%#lx\n", i,
               names + sh.sh_name, sh.sh_type, sh.sh_offset, sh.sh_size);
    }
}
