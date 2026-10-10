#ifndef DBG_FILE_H
#define DBG_FILE_H

#include <stddef.h>

unsigned char *read_file(const char *path, size_t *out_len);

#endif
