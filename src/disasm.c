#include "disasm.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int16_t read_i16(const unsigned char *p) {
    int16_t v;
    memcpy(&v, p, sizeof v);
    return v;
}

static int32_t read_i32(const unsigned char *p) {
    int32_t v;
    memcpy(&v, p, sizeof v);
    return v;
}

/// https://github.com/anza-xyz/sbpf/blob/main/doc/bytecode.md#instruction-layout
static Insn decode(const unsigned char *p, uint64_t ptr) {
    return (Insn){
        .ptr = ptr,
        .opc = p[0],
        .dst = p[1] & 0x0f,
        .src = p[1] >> 4,
        .off = read_i16(p + 2),
        .imm = read_i32(p + 4),
    };
}

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

        Insn in = decode(code + off, off / INSN_SIZE);

        printf(" ptr=%lu opc=%02x dst=r%u src=r%u off=%d imm=%ld\n", in.ptr,
               in.opc, in.dst, in.src, in.off, in.imm);
    }
}
