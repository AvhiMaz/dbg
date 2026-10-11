#ifndef DISASM_H
#define DISASM_H

#include "elf.h"

#define INSN_SIZE 8

void disasm_print(const unsigned char *buf, size_t len, const Elf64_Ehdr *eh);

#endif
