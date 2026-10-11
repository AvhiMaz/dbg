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

/// https://github.com/anza-xyz/sbpf/blob/main/doc/bytecode.md
/// https://github.com/anza-xyz/sbpf/blob/main/src/disassembler.rs
static const char *const OPC_NAMES[256] = {
    [0x00] = "lddw",   [0x04] = "add32",  [0x05] = "ja",     [0x07] = "add64",
    [0x0c] = "add32",  [0x0f] = "add64",  [0x14] = "sub32",  [0x15] = "jeq",
    [0x17] = "sub64",  [0x18] = "lddw",   [0x1c] = "sub32",  [0x1d] = "jeq",
    [0x1f] = "sub64",  [0x24] = "mul32",  [0x25] = "jgt",    [0x27] = "mul64",
    [0x2c] = "mul32",  [0x2d] = "jgt",    [0x2f] = "mul64",  [0x34] = "div32",
    [0x35] = "jge",    [0x37] = "div64",  [0x3c] = "div32",  [0x3d] = "jge",
    [0x3f] = "div64",  [0x44] = "or32",   [0x45] = "jset",   [0x47] = "or64",
    [0x4c] = "or32",   [0x4d] = "jset",   [0x4f] = "or64",   [0x54] = "and32",
    [0x55] = "jne",    [0x57] = "and64",  [0x5c] = "and32",  [0x5d] = "jne",
    [0x5f] = "and64",  [0x61] = "ldxw",   [0x62] = "stw",    [0x63] = "stxw",
    [0x64] = "lsh32",  [0x65] = "jsgt",   [0x67] = "lsh64",  [0x69] = "ldxh",
    [0x6a] = "sth",    [0x6b] = "stxh",   [0x6c] = "lsh32",  [0x6d] = "jsgt",
    [0x6f] = "lsh64",  [0x71] = "ldxb",   [0x72] = "stb",    [0x73] = "stxb",
    [0x74] = "rsh32",  [0x75] = "jsge",   [0x77] = "rsh64",  [0x79] = "ldxdw",
    [0x7a] = "stdw",   [0x7b] = "stxdw",  [0x7c] = "rsh32",  [0x7d] = "jsge",
    [0x7f] = "rsh64",  [0x84] = "neg32",  [0x85] = "call",   [0x87] = "neg64",
    [0x8d] = "callx",  [0x94] = "mod32",  [0x95] = "exit",   [0x97] = "mod64",
    [0x9c] = "mod32",  [0x9f] = "mod64",  [0xa4] = "xor32",  [0xa5] = "jlt",
    [0xa7] = "xor64",  [0xac] = "xor32",  [0xad] = "jlt",    [0xaf] = "xor64",
    [0xb4] = "mov32",  [0xb5] = "jle",    [0xb7] = "mov64",  [0xbc] = "mov32",
    [0xbd] = "jle",    [0xbf] = "mov64",  [0xc4] = "arsh32", [0xc5] = "jslt",
    [0xc7] = "arsh64", [0xcc] = "arsh32", [0xcd] = "jslt",   [0xcf] = "arsh64",
    [0xd4] = "le",     [0xd5] = "jsle",   [0xdc] = "be",     [0xdd] = "jsle",
};

static const char *opc_name(uint8_t opc) {
    return OPC_NAMES[opc] ? OPC_NAMES[opc] : "unknown";
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

        printf(" %-7s ptr=%lu opc=%02x dst=r%u src=r%u off=%d imm=%ld\n",
               opc_name(in.opc), in.ptr, in.opc, in.dst, in.src, in.off,
               in.imm);
    }
}
