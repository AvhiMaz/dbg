#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"
#include "file.h"

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.so>\n", argv[0]);
        return 1;
    }

    size_t         len;
    unsigned char *buf = read_file(argv[1], &len);

    if (buf == NULL) {
        return 1;
    }

    if (!elf_has_magic(buf)) {
        fprintf(stderr, "error: %s is not an ELF file\n", argv[1]);
        free(buf);
        return 1;
    }

    Elf64_Ehdr eh;

    memcpy(&eh, buf, sizeof(eh));

    if (!elf_check_header(&eh, len)) {
        free(buf);
        return 1;
    }

    printf("size of file: %zu\n", len);
    printf("%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

    elf_print_header(&eh);

    free(buf);
    return 0;
}
