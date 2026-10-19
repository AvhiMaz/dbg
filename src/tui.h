#ifndef TUI_H
#define TUI_H

#include <stddef.h>

#include "elf.h"

#define DARK   235
#define GRAY   245
#define GREEN  142
#define PURPLE 175
#define YELLOW 214
#define BLUE   109
#define TEXT   223

int tui_run(const char *path, const unsigned char *buf, size_t len,
            const Elf64_Ehdr *eh);

#endif
