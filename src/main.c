#include <stdio.h>
#include <stdlib.h>

int main() {

  FILE *file = fopen("/home/archlinux/dev/dbg/program/target/"
                     "sbpf-solana-solana/release/program.so",
                     "rb");

  if (file == NULL) {
    perror("fopen failed");
    return 1;
  };

  if (fseek(file, 0, SEEK_END)) {
    perror("fseek failed");
    fclose(file);
    return 1;
  };

  long sz = ftell(file);

  if (sz < 0) {
    perror("ftell failed");
    fclose(file);
    return 1;
  }

  rewind(file);

  printf("size of file: %ld\n", sz);

  unsigned char *buf = malloc(sz);

  if (buf == NULL) {
    perror("malloc failed");
    fclose(file);
    return 1;
  }

  size_t n = fread(buf, 1, sz, file);

  if (n != (size_t)sz) {
    fprintf(stderr, "error: failed to read entire file\n");
    free(buf);
    fclose(file);
    return 1;
  }

  fclose(file);

  if (n < 4) {
    fprintf(stderr, "error: file is smaller than 4 bytes\n");
    free(buf);
    return 1;
  }

  printf("%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

  free(buf);

  return 0;
}
