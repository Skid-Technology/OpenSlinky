// 0x4c000: Windows virtual-key -> key code, small CRT printf wrappers, string append
#include "slinky/engine.h"
#include "slinky/win32.h"
#include <stdarg.h>

extern "C" int windows_key_to_client_key(unsigned long long vk) {
    if (vk - 0x41 <= 0x19) return (int)vk - 0x27;
    if (vk - 0x30 <= 9) return (int)vk - 0x2a;
    if (vk - 0x60 <= 9) return (int)vk - 0x50;
    if ((vk & ~0xfull) == 0x70) return (int)vk - 0x2d;
    switch (vk) {
    case 0x8: return 0x53;
    case 0x9: return 0x54;
    case 0xd: return 0x55;
    case 0x14: return 0x56;
    case 0x1b: return 0x57;
    case 0x20: return 0x58;
    case 0x21: return 0x59;
    case 0x22: return 0x5a;
    case 0x23: return 0x5b;
    case 0x24: return 0x5c;
    case 0x25: return 0x3f;
    case 0x26: return 0x40;
    case 0x27: return 0x41;
    case 0x28: return 0x42;
    case 0x2a: return 0x5d;
    case 0x2d: return 0x5e;
    case 0x2e: return 0x5f;
    case 0x90: return 0x60;
    case 0x91: return 0x61;
    case 0xa0: return 0x62;
    case 0xa1: return 0x63;
    case 0xa2: return 0x64;
    case 0xa3: return 0x65;
    case 0xa4: return 0x66;
    case 0xa5: return 0x67;
    case 0xba: return 0x34;
    case 0xbb: return 0x35;
    case 0xbc: return 0x36;
    case 0xbd: return 0x37;
    case 0xbe: return 0x38;
    case 0xbf: return 0x39;
    case 0xc0: case 0xdf: return 0x3a;
    case 0xdb: return 0x3b;
    case 0xdc: return 0x3c;
    case 0xdd: return 0x3d;
    case 0xde: return 0x3e;
    default: return 0;
    }
}

FN(unsigned long long *, printf_options_ptr, ());   // __local_stdio_printf_options
extern "C" int libc_vswprintf(unsigned long long opts, char *buf, unsigned long long n, const char *fmt, void *locale, va_list ap);
extern "C" int libc_vswprintf_s(unsigned long long opts, wchar_t *buf, unsigned long long size, unsigned long long count,
                         const wchar_t *fmt, void *locale, va_list ap);
extern "C" const wchar_t ptr_to_rdata_2ba968[];   // L"%S"
extern "C" const char wide_path_format[];
FN(unsigned long long, libc_wcslen, (const char *str));
FN(unsigned long long, libc_strlen, (const char *str));
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));
FN(void, libc_free, (void *ptr));

extern "C" int format_string(char *buf, unsigned long long n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    return libc_vswprintf(*printf_options_ptr(), buf, n, fmt, 0, ap);
}
// _snwprintf_s(buf, 260, 260, L"%S", ...) with the constants propagated into the out-of-line copy
extern "C" int snwprintf_s(wchar_t *buf, unsigned long long, unsigned long long, const wchar_t *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    return libc_vswprintf_s(*printf_options_ptr(), buf, 0x104, 0x104, ptr_to_rdata_2ba968, 0, ap);
}
extern "C" int settings_path_append(char **pstr, const char *suffix) {
    char *old = *pstr;
    if (!old || !suffix) return 0x3ed;
    unsigned long long a = libc_wcslen(old);     // wcslen of the wide path
    unsigned long long b = libc_strlen(suffix);   // strlen of the narrow suffix
    // the buffer holds 261 wide chars: old + '\' + suffix + NUL
    if (a > 0x104 || b > 0x104 || a + b + 2 > 0x105) return 0x3ee;
    char *nb = (char *)libc_calloc(0x105, 2);    // 261 wide chars, as the image allocates
    if (!nb) return 0x3ed;
    wchar_t *w = (wchar_t *)nb;
    for (unsigned long long i = 0; i < a; i++) w[i] = ((const wchar_t *)old)[i];
    w[a] = L'\\';
    if (!MultiByteToWideChar(65001 /* CP_UTF8 */, 0, suffix, -1, w + a + 1, (int)(0x105 - (a + 1)))) {
        libc_free(nb);
        return 0x3ee;
    }
    libc_free(*pstr);
    *pstr = nb;
    return 0;
}
extern "C" void free_path(void **p) {
    libc_free(*p);
    *p = 0;
}

// ---- gap-scan functions ----
FN(unsigned char, crt_stub_returns_zero, ());
extern "C" bool crt_initialized() { return crt_stub_returns_zero() != 0; }
// cut a wide path at its last backslash (-> directory); no backslash: empty string
extern "C" void cut_wide_path_last_backslash(wchar_t **path) {
    wchar_t *s = *path;
    unsigned long long i = libc_wcslen((const char *)s);
    for (; i; i--)
        if (s[i] == L'\\') {
            s += i;
            break;
        }
    *s = 0;
}
