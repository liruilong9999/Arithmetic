#ifndef ASSURE_FIO_H
#define ASSURE_FIO_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE *as_fopen(const char *path, const char *mode);

size_t as_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t as_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int as_fseek(FILE *stream, long offset, int whence);
long as_ftell(FILE *stream);

int as_fflush(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
