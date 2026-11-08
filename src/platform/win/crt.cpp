// C runtime entry points the client calls by their address in the original image (fn_<rva>), bound to the real
// Universal CRT (ucrtbase.dll). Every name was checked against the image: the import thunks (0x13cf50..) by how the
// code uses them, the statically linked CRT copies (0xab000..) by running them under Unicorn on test inputs.
#include <stddef.h>
#include <stdarg.h>

extern "C" {
// ---- the ucrt functions used below (no SDK headers in the cross build) ----
void *calloc(size_t, size_t);
void free(void *);
void *malloc(size_t);
void *realloc(void *, size_t);
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
void *memmove(void *, const void *, size_t);
size_t strlen(const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
size_t wcslen(const wchar_t *);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
__attribute__((noreturn)) void abort(void);
double acos(double);
double atan2(double, double);
double hypot(double, double);
float powf(float, float);
float truncf(float);
double sqrt(double);
float sqrtf(float);
double sin(double);
float sinf(float);
float roundf(float);
float logf(float);
double fmod(double, double);
float fmodf(float, float);
float ceilf(float);
double floor(double);
float floorf(float);
float cosf(float);
// ---- ucrt imports (declared here so no Windows SDK headers are needed) ----
int __stdio_common_vsprintf(unsigned long long opts, char *buf, size_t n, const char *fmt, void *locale, va_list ap);
int __stdio_common_vswprintf(unsigned long long opts, wchar_t *buf, size_t n, const wchar_t *fmt, void *locale,
                             va_list ap);
int __stdio_common_vsscanf(unsigned long long opts, const char *buf, size_t n, const char *fmt, void *locale,
                           va_list ap);
float _hypotf(float x, float y);
long long _time64(long long *t);
typedef void (*onexit_t)(void);
int _register_onexit_function(void *table, onexit_t fn);

// printf options word (vcruntime's __local_stdio_printf_options keeps it in a static)
static unsigned long long printf_options;
unsigned long long *printf_options_ptr() { return &printf_options; }

// ---- memory ----
void *libc_calloc(size_t n, size_t size) { return calloc(n, size); }
void libc_free(void *ptr) { free(ptr); }
void *libc_malloc(size_t n) { return malloc(n); }
void *libc_realloc(void *ptr, size_t n) { return realloc(ptr, n); }
void *libc_memcpy(void *dest, const void *src, size_t n) { return memcpy(dest, src, n); }
void *libc_memcpy_alt(void *dest, const void *src, size_t n) { return memcpy(dest, src, n); }
void *libc_memset(void *ptr, int c, size_t n) { return memset(ptr, c, n); }
void libc_memmove(void *dest, const void *src, size_t n) { memmove(dest, src, n); }

// ---- strings ----
size_t libc_strlen(const char *str) { return strlen(str); }
int libc_strncmp_alt(const char *left, const char *right, size_t n) { return strncmp(left, right, n); }
int libc_strcmp(const char *left, const char *right) { return strcmp(left, right); }
int libc_strncmp(const char *left, const char *right, size_t n) { return strncmp(left, right, n); }
size_t libc_wcslen(const wchar_t *str) { return wcslen(str); }

// ---- formatting ----
int libc_vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {   // vsnprintf
    int r = __stdio_common_vsprintf(printf_options | 2 /* standard snprintf behaviour */, buf, n, fmt, 0, ap);
    return r < 0 ? -1 : r;
}
int libc_snprintf(char *buf, size_t n, const char *fmt, ...) {          // snprintf
    va_list ap;
    va_start(ap, fmt);
    int r = libc_vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}
// the module code reaches snprintf through this name (include/slinky/api.h)
int log_fmt(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int result = libc_vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return result;
}
int libc_sprintf(char *buf, const char *fmt, ...) {                    // sprintf
    va_list ap;
    va_start(ap, fmt);
    int r = __stdio_common_vsprintf(printf_options, buf, (size_t)-1, fmt, 0, ap);
    va_end(ap);
    return r < 0 ? -1 : r;
}
int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int result = __stdio_common_vsprintf(printf_options, buf, (size_t)-1, fmt, 0, ap);
    va_end(ap);
    return result < 0 ? -1 : result;
}
int sscanf(const char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int result = __stdio_common_vsscanf(0, buf, (size_t)-1, fmt, 0, ap);
    va_end(ap);
    return result;
}
// the original calls these two ucrt functions directly (protected/files.cpp, modlib/e_4c.cpp)
// 13d080 is the wide one: the image's settings_path_append passes the UTF-16 format "%s\%S" (wide_path_format) and formats a
// wcslen()-measured path (0x4c610: wcslen(old) + strlen(suffix), calloc(0x105, 2)); 13d060 is the _s wide one
// (image snwprintf_s passes buffer/count 0x104 and ptr_to_rdata_2ba968 = L"%S").
int libc_vswprintf(unsigned long long opts, wchar_t *buf, size_t n, const wchar_t *fmt, void *locale, va_list ap) {
    return __stdio_common_vswprintf(opts, buf, n, fmt, locale, ap);
}
int libc_vswprintf_s(unsigned long long opts, wchar_t *buf, size_t size, size_t count, const wchar_t *fmt, void *locale,
              va_list ap) {
    (void)count;
    return __stdio_common_vswprintf(opts, buf, size, fmt, locale, ap);
}

// ---- sorting / searching / time / process ----
void libc_qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *)) { qsort(base, n, size, cmp); }
void *libc_bsearch(const void *key, const void *base, size_t n, size_t size, int (*cmp)(const void *, const void *)) {
    return bsearch(key, base, n, size, cmp);
}
long long libc_time64(long long *t) { return _time64(t); }
__attribute__((noreturn)) void libc_abort() { abort(); }
// _register_onexit_function(&table, fn): cJSON's atexit wrapper (cjson_strdup) passes the DLL's onexit table
int libc_register_onexit_function(void *table, onexit_t fn) { return _register_onexit_function(table, fn); }
// an import the image only reaches from the unused crt_initialized; its target is not identifiable (returns "no")
unsigned char crt_stub_returns_zero() { return 0; }

// ---- math ----
double libc_acos(double v) { return acos(v); }
double libc_atan2(double y, double x) { return atan2(y, x); }
float libc_hypotf(float a, float b) { return _hypotf(a, b); }
double libc_hypot(double x, double y) { return hypot(x, y); }
float libc_powf(float x, float y) { return powf(x, y); }
float libc_hypotf_alt(float x, float y) { return _hypotf(x, y); }
float libc_truncf(float v) { return truncf(v); }
double libc_sqrt(double x) { return sqrt(x); }
float libc_sqrtf(float x) { return sqrtf(x); }
double libc_sin(double x) { return sin(x); }
float libc_sinf(float v) { return sinf(v); }
float libc_roundf(float v) { return roundf(v); }
float libc_logf(float x) { return logf(x); }
double libc_fmod(double x, double y) { return fmod(x, y); }
float libc_fmodf(float x, float y) { return fmodf(x, y); }
float libc_ceilf(float v) { return ceilf(v); }
double libc_floor(double x) { return floor(x); }
float libc_floorf(float x) { return floorf(x); }
float libc_cosf(float v) { return cosf(v); }

// ---- compiler support ----
int _fltused = 0x9875;
}
