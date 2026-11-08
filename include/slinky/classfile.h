// Java class-file constant pool access (the client patches classes in ClassFileLoadHook).
// Error codes: 0x2af8 truncated, 0x2afd bad index, 0x2afe wrong tag.
#pragma once
struct CP { const unsigned char **e; unsigned short n; char pad[6]; const unsigned char *end; };
struct Utf8 { const unsigned char *p; unsigned short len; };
struct NameType { Utf8 name, desc; };
struct MemberRef { Utf8 cls; NameType nt; };
static inline __attribute__((always_inline)) unsigned short be16(const unsigned char *p) {
    unsigned short v = *(const unsigned short *)p;
    return (unsigned short)((v << 8) | (v >> 8));
}
// entry `idx` (1-based) and the end of its bytes (start of the next entry, or the pool end)
#define CP_ENTRY(cp, idx, ent, lim, fail)                                            \
    do {                                                                            \
        if ((idx) == 0) goto fail;                                                  \
        unsigned short i_ = (unsigned short)((idx) - 1);                            \
        if (i_ >= (cp)->n) goto fail;                                               \
        ent = (cp)->e[i_];                                                          \
        if (!ent) goto fail;                                                        \
        const unsigned char **nx_ = &(cp)->e[i_];                                    \
                                                                \
        lim = *((idx) == (cp)->n ? &(cp)->end : nx_ + 1);                           \
    } while (0)
