#pragma once
#include <stddef.h>
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _iobuf FILE;
FILE *fopen(const char *, const char *);
int fclose(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
int fseek(FILE *, long, int);
int feof(FILE *);
int fopen_s(FILE **, const char *, const char *);
long ftell(FILE *);
int sprintf(char *, const char *, ...);
int snprintf(char *, size_t, const char *, ...);
// diagnostics printing of the bundled libraries is not wanted in the client
#define printf(...) ((void)0)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#ifdef __cplusplus
}
#endif
