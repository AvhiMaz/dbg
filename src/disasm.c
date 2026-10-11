#include "disasm.h"

#include <stdint.h>
#include <stdio.h>

void disasm_print(const unsigned char *buf, size_t len, const Elf64_Ehdr *eh) {

    Elf64_Shdr text;

    if (!elf_find_section(buf, len, eh, ".text", &text)) {
        printf("no .text section\n");
        return;
    }

    if (text.sh_offset > len || text.sh_size > len - text.sh_offset) {
        fprintf(stderr, "error: .text is outside the file\n");
        return;
    }

    const unsigned char *code = buf + text.sh_offset;

    for (uint64_t off = 0; off + INSN_SIZE <= text.sh_size; off += INSN_SIZE) {
        printf("%6lx: ", text.sh_addr + off);

        for (int b = 0; b < INSN_SIZE; b++) {
            printf("%02x ", code[off + b]);
        }

        printf("\n");
    }
}
