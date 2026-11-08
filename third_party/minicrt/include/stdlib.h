// Declarations of the UCRT functions the bundled C libraries (NanoVG, stb) use; the cross build has no Windows SDK.
#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
int abs(int);
double strtod(const char *, char **);
long strtol(const char *, char **, int);
#ifdef __cplusplus
}
#endif
