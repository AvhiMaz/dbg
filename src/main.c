#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

unsigned char *read_file(const char *path, size_t *out_len) {

  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    perror("fopen failed");
    return NULL;
  };

  struct stat st;

  if (fstat(fileno(file), &st) != 0) {
    perror("fstat failed");
    fclose(file);
    return NULL;
  }

  if (!S_ISREG(st.st_mode)) {
    fprintf(stderr, "error: %s is not a regular file\n", path);
    fclose(file);
    return NULL;
  }

  if (st.st_size < 64) {
    fprintf(stderr, "error: file too small to be an ELF\n");
    fclose(file);
    return NULL;
  }

  unsigned char *buf = malloc(st.st_size);

  if (buf == NULL) {
    perror("malloc failed");
    fclose(file);
    return NULL;
  }

  size_t n = fread(buf, 1, st.st_size, file);

  if (n != (size_t)st.st_size) {
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

  if (buf[0] != 0x7f || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {
    fprintf(stderr, "error: %s is not an ELF file\n", argv[1]);
    free(buf);
    return 1;
  }

  printf("size of file: %zu\n", len);
  printf("%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

  free(buf);
  return 0;
}
