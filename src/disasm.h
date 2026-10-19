#ifndef DISASM_H
#define DISASM_H

#include <stdio.h>

#include "elf.h"

#define INSN_SIZE 8

/// https://github.com/anza-xyz/sbpf/blob/main/src/ebpf.rs#L535-L554
typedef struct {
    uint64_t ptr;
    uint8_t  opc;
    uint8_t  dst;
    uint8_t  src;
    int16_t  off;
    int64_t  imm;
} Insn;

void disasm_print(FILE *out, const unsigned char *buf, size_t len,
                  const Elf64_Ehdr *eh);

#endif
