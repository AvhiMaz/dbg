#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disasm.h"
#include "elf.h"
#include "file.h"
#include "tui.h"

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "usage: %s [-p] <file.so>\n", argv[0]);
        return 1;
    }

    int         print = strcmp(argv[1], "-p") == 0;
    const char *path = print ? argv[2] : argv[1];

    if (path == NULL) {
        fprintf(stderr, "usage: %s [-p] <file.so>\n", argv[0]);
        return 1;
    }

    size_t         len;
    unsigned char *buf = read_file(path, &len);

    if (buf == NULL) {
        return 1;
    }

    if (!elf_has_magic(buf)) {
        fprintf(stderr, "error: %s is not an ELF file\n", path);
        free(buf);
        return 1;
    }

    Elf64_Ehdr eh;

    memcpy(&eh, buf, sizeof(eh));

    if (!elf_check_header(&eh, len)) {
        free(buf);
        return 1;
    }

    if (!print) {
        int rc = tui_run(path, buf, len, &eh);
        free(buf);
        return rc;
    }

    printf("size of file: %zu\n", len);
    printf("%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

    elf_print_header(stdout, &eh);
    elf_print_sections(stdout, buf, len, &eh);
    elf_print_symbols(stdout, buf, len, &eh);
    disasm_print(stdout, buf, len, &eh);

    free(buf);
    return 0;
}
