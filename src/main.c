#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char *read_file(const char *path, size_t *out_len) {

  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    perror("fopen failed");
    return NULL;
  };

  if (fseek(file, 0, SEEK_END)) {
    perror("fseek failed");
    fclose(file);
    return NULL;
  };

  long sz = ftell(file);

  if (sz < 0) {
    perror("ftell failed");
    fclose(file);
    return NULL;
  }

  if (sz < 64) {
    fprintf(stderr, "error: file too small to be an ELF\n");
    fclose(file);
    return NULL;
  }

  rewind(file);

  unsigned char *buf = malloc(sz);

  if (buf == NULL) {
    perror("malloc failed");
    fclose(file);
    return NULL;
  }

  size_t n = fread(buf, 1, sz, file);

  if (n != (size_t)sz) {
    fprintf(stderr, "error: failed to read entire file\n");
    free(buf);
    fclose(file);
    return NULL;
  }

  fclose(file);

  *out_len = n;
  return buf;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "usage: %s <file.so>\n", argv[0]);
    return 1;
  }

  size_t len;

  unsigned char *buf = read_file(argv[1], &len);

  if (buf == NULL) {
    return 1;
  }

  printf("size of file: %zu\n", len);
  printf("%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

  free(buf);
  return 0;
}
