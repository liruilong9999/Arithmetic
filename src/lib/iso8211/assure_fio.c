#include <stdlib.h>
#include <errno.h>

#include "assure_fio.h"

FILE *as_fopen(const char *path, const char *mode)
{
  FILE *ret;
  errno = 0;
  ret = fopen(path, mode);
  if (ret == NULL || errno != 0) {
    perror("fopen");
    exit(1);
  }
  return ret;
}

size_t as_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t ret;
  errno = 0;
  ret = fread(ptr, size, nmemb, stream);
  if (errno != 0) {
    perror("fread");
    exit(1);
  }
  return ret;
}

size_t as_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t ret;
  errno = 0;
  ret = fwrite(ptr, size, nmemb, stream);
  if (errno != 0) {
    perror("fwrite");
    exit(1);
  }
  return ret;
}

int as_fseek(FILE *stream, long offset, int whence)
{
  int ret;
  errno = 0;
  ret = fseek(stream, offset, whence);
  if (errno != 0) {
    perror("fseek");
    exit(1);
  }
  return ret;
}

long as_ftell(FILE *stream)
{
  long ret;
  errno = 0;
  ret = ftell(stream);
  if (errno != 0) {
    perror("ftell");
    exit(1);
  }
  return ret;
}

int as_fflush(FILE *stream)
{
  int ret;
  errno = 0;
  ret = fflush(stream);
  if (errno != 0) {
    perror("fflush");
    exit(1);
  }
  return ret;
}
