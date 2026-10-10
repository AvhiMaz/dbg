#include <stdio.h>

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

  long size = ftell(file);
  if (size < 0) {
    perror("ftell failed");
    fclose(file);
    return 1;
  }

  rewind(file);

  printf("size of file: %ld\n", size);

  fclose(file);

  return 0;
}
