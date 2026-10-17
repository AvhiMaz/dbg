#include "elf.h"

#include <stdio.h>
#include <string.h>

static Elf64_Shdr elf_section(const unsigned char *buf, const Elf64_Ehdr *eh,
                              int i) {
    Elf64_Shdr sh;
    memcpy(&sh, buf + eh->e_shoff + i * sizeof(sh), sizeof(sh));
    return sh;
}

static const char *elf_strtab(const unsigned char *buf, size_t len,
                              const Elf64_Shdr *sh, const char *what) {

    if (sh->sh_offset > len || sh->sh_size > len - sh->sh_offset) {
        fprintf(stderr, "error: %s is outside the file\n", what);
        return NULL;
    }

    const char *s = (const char *)buf + sh->sh_offset;

    if (sh->sh_size == 0 || s[sh->sh_size - 1] != '\0') {
        fprintf(stderr, "error: %s is not NUL-terminated\n", what);
        return NULL;
    }

    return s;
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

    if (eh->e_flags > 1) {
        if (eh->e_flags <= 4) {
            fprintf(stderr,
                    "error: sBPF v%u is not supported yet (only v0 and v1)\n",
                    eh->e_flags);
        } else {
            fprintf(stderr, "error: unknown sBPF version (e_flags = 0x%x)\n",
                    eh->e_flags);
        }
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

    Elf64_Shdr  shstrtab = elf_section(buf, eh, eh->e_shstrndx);

    const char *names = elf_strtab(buf, len, &shstrtab, "section name table");

    if (names == NULL) {
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

void elf_print_symbols(const unsigned char *buf, size_t len,
                       const Elf64_Ehdr *eh) {

    Elf64_Shdr symtab;
    int        found = 0;

    for (int i = 0; i < eh->e_shnum; i++) {
        Elf64_Shdr sh = elf_section(buf, eh, i);
        if (sh.sh_type == SHT_SYMTAB) {
            symtab = sh;
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("no symbol table\n");
        return;
    }

    if (symtab.sh_offset > len || symtab.sh_size > len - symtab.sh_offset) {
        fprintf(stderr, "error: symbol table is outside the file\n");
        return;
    }

    if (symtab.sh_entsize != sizeof(Elf64_Sym)) {
        fprintf(stderr, "error: unexpected symbol size %lu\n",
                symtab.sh_entsize);
        return;
    }

    printf("symbols: %lu\n", symtab.sh_size / symtab.sh_entsize);

    if (symtab.sh_link >= eh->e_shnum) {
        fprintf(stderr, "error: symbol name table index %u out of range\n",
                symtab.sh_link);
        return;
    }

    Elf64_Shdr  strtab = elf_section(buf, eh, symtab.sh_link);

    const char *sym_names = elf_strtab(buf, len, &strtab, "symbol name table");

    if (sym_names == NULL) {
        return;
    }

    uint64_t count = symtab.sh_size / symtab.sh_entsize;

    for (uint64_t i = 0; i < count; i++) {
        Elf64_Sym sym;
        memcpy(&sym, buf + symtab.sh_offset + i * sizeof(sym), sizeof(sym));

        if ((sym.st_info & 0xf) != STT_FUNC) {
            continue;
        }

        if (sym.st_name >= strtab.sh_size) {
            printf("0x%06lx %6lu <bad name>\n", sym.st_value, sym.st_size);
            continue;
        }

        printf("0x%06lx %6lu %s\n", sym.st_value, sym.st_size,
               sym_names + sym.st_name);
    }
}

int elf_find_section(const unsigned char *buf, size_t len, const Elf64_Ehdr *eh,
                     const char *name, Elf64_Shdr *out) {

    Elf64_Shdr  shstrtab = elf_section(buf, eh, eh->e_shstrndx);

    const char *names = elf_strtab(buf, len, &shstrtab, "section table name");

    if (names == NULL) {
        return 0;
    }

    for (int i = 0; i < eh->e_shnum; i++) {
        Elf64_Shdr sh = elf_section(buf, eh, i);

        if (sh.sh_name >= shstrtab.sh_size) {
            continue;
        }

        if (strcmp(names + sh.sh_name, name) == 0) {
            *out = sh;
            return 1;
        }
    }

    return 0;
}
