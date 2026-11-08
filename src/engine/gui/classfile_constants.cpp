// 0x1f000: class-file constant pool getters (single exit, like the original)
#include "slinky/engine.h"
#include "slinky/classfile.h"
// CONSTANT_Utf8
extern "C" __attribute__((noinline)) int constant_utf8(CP *constant_pool, unsigned short idx, Utf8 *out) {
    const unsigned char *end, *lim;
    int error_code = 0x2afd;
    CP_ENTRY(constant_pool, idx, end, lim, out_);
    if (end + 1 > lim) goto trunc;
    error_code = 0x2afe;
    if (*end != 1) goto out_;
    {
        const unsigned char *cursor = end + 3;
        if (cursor > lim) goto trunc;
        unsigned short len = be16(end + 1);
        error_code = 0x2af8;
        if (cursor + len > lim) goto out_;
        error_code = 0;
        if (out) {
            out->len = len;
            out->p = cursor;
        }
    }
    goto out_;
trunc:
    error_code = 0x2af8;
out_:

    return error_code;
}
// CONSTANT_Class -> its name
extern "C" __attribute__((noinline)) int constant_class_name(CP *constant_pool, unsigned short idx, Utf8 *out) {
    const unsigned char *end, *lim, *u, *ulim;
    int error_code = 0x2afd;
    CP_ENTRY(constant_pool, idx, end, lim, out_);
    if (end + 1 > lim) goto trunc;
    if (*end != 7) goto tag;
    if (end + 3 > lim) goto trunc;
    {
        unsigned short ni = be16(end + 1);
        CP_ENTRY(constant_pool, ni, u, ulim, out_);
        if (u + 1 > ulim) goto trunc;
        error_code = 0x2afe;
        if (*u != 1) goto out_;
        const unsigned char *cursor = u + 3;
        if (cursor > ulim) goto trunc;
        unsigned short len = be16(u + 1);
        error_code = 0x2af8;
        if (cursor + len > ulim) goto out_;
        error_code = 0;
        if (out) {
            out->len = len;
            out->p = cursor;
        }
    }
    goto out_;
tag:
    error_code = 0x2afe;
    goto out_;
trunc:
    error_code = 0x2af8;
out_:

    return error_code;
}
// CONSTANT_NameAndType -> name + descriptor
extern "C" __attribute__((noinline)) int constant_name_and_type(CP *constant_pool, unsigned short idx, NameType *out) {
    const unsigned char *end, *lim, *u, *ulim;
    int error_code = 0x2afd;
    CP_ENTRY(constant_pool, idx, end, lim, out_);
    if (end + 1 > lim) goto trunc;
    if (*end != 0xc) goto tag;
    if (end + 3 > lim || end + 5 > lim) goto trunc;
    {
        unsigned short ni = be16(end + 1);
        CP_ENTRY(constant_pool, ni, u, ulim, out_);
        if (u + 1 > ulim) goto trunc;
        error_code = 0x2afe;
        if (*u != 1) goto out_;
        const unsigned char *cursor = u + 3;
        if (cursor > ulim) goto trunc;
        unsigned short len = be16(u + 1);
        error_code = 0x2af8;
        if (cursor + len > ulim) goto out_;
        unsigned short di = be16(end + 3);
        if (out) {
            out->name.len = len;
            out->name.p = cursor;
        }
        return constant_utf8(constant_pool, di, &out->desc);
    }
tag:
    error_code = 0x2afe;
    goto out_;
trunc:
    error_code = 0x2af8;
out_:

    return error_code;
}

struct WBuf { unsigned char *p, *end; };
extern "C" int classfile_write_u1(WBuf *w, unsigned char b) {
    unsigned char *cursor = w->p;
    if (cursor + 1 > w->end) return 0x2af9;
    *cursor = b;
    w->p = cursor + 1;
    return 0;
}

struct RBuf { const unsigned char *p, *end; };
extern "C" int classfile_read_u1(RBuf *r, unsigned char *out) {
    const unsigned char *cursor = r->p;
    if (cursor + 1 > r->end) return 0x2af8;
    if (out) *out = *cursor;
    r->p = cursor + 1;
    return 0;
}
extern "C" int classfile_read_u2(RBuf *r, unsigned short *out) {
    const unsigned char *cursor = r->p;
    if (cursor + 2 > r->end) return 0x2af8;
    if (out) *out = be16(cursor);
    r->p = cursor + 2;
    return 0;
}
extern "C" int classfile_write_u2(WBuf *w, unsigned short v) {
    unsigned char *cursor = w->p;
    if (cursor + 2 > w->end) return 0x2af9;
    cursor[0] = (unsigned char)(v >> 8);
    w->p[1] = (unsigned char)v;
    w->p = cursor + 2;
    return 0;
}

#include "slinky/jnienv.h"
extern "C" const char str_loadclass[], str_ljava_lang_string_ljava_lang_c[];   // "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"
struct LoaderRef { jobject loader; jmethodID loadClass; };
extern "C" int make_secure_class_loader(LoaderRef *o, JNIEnv *env, jobject loader) {
    jclass cls = (*env)->GetObjectClass(env, loader);
    jmethodID mid = (*env)->GetMethodID(env, cls, str_loadclass, str_ljava_lang_string_ljava_lang_c);
    if (!mid) return 0x7d4;
    jobject ref = (*env)->NewGlobalRef(env, loader);
    if (!ref) return 0x7d5;
    o->loader = ref;
    o->loadClass = mid;
    return 0;
}

FN(int, constant_class_name, (CP *cp, unsigned short idx, Utf8 *out));
// CONSTANT_Methodref / InterfaceMethodref (tags 10, 11) -> class, name, descriptor
extern "C" int constant_pool_member_ref2(CP *constant_pool, unsigned short idx, MemberRef *out) {
    const unsigned char *end, *lim;
    int error_code = 0x2afd;
    CP_ENTRY(constant_pool, idx, end, lim, out_);
    if (end + 1 > lim) goto trunc;
    error_code = 0x2afe;
    if ((*end & 0xfe) != 10) goto out_;
    if (end + 3 > lim) goto trunc;
    error_code = 0x2af8;
    if (end + 5 > lim) goto out_;
    {
        unsigned short ci = be16(end + 1);
        unsigned char hi = end[3], lo = end[4];
        error_code = constant_class_name(constant_pool, ci, &out->cls);
        if (error_code) goto out_;
        return constant_name_and_type(constant_pool, (unsigned short)((hi << 8) | lo), &out->nt);
    }
trunc:
    error_code = 0x2af8;
out_:
    return error_code;
}

FN(jclass, find_class, (jobject loader, JNIEnv *env, const char *name));
extern "C" const char str_net_minecraft_client_minecraft_in_classfile_constants[], str_field_71439_g_in_classfile_constants[], str_lnet_minecraft_client_entity_e_in_classfile_constants[], str_lnet_minecraft_client_entity_e_in_classfile_constants_a[], str_theplayer_in_classfile_constants[],
    str_bao_in_classfile_constants[], str_i_in_classfile_constants[], str_ljava_io_file[], str_ave_in_classfile_constants[], str_n_in_classfile_constants[];
struct Mapping { int player; int names; unsigned char padding_08; };   // player: 0 = ClientPlayerMP, 1 = PlayerSP; names: 0/1 obf, 1 MCP, 2 SRG
// detect which mapping set the game uses (SRG / MCP / obfuscated 1.7 / 1.8)
extern "C" int fill_jni_ids(Mapping *out, jobject loader, JNIEnv *env) {
    jclass mc = find_class(loader, env, str_net_minecraft_client_minecraft_in_classfile_constants);   // net.minecraft.client.Minecraft
    if (mc) {
        if ((*env)->GetFieldID(env, mc, str_field_71439_g_in_classfile_constants, str_lnet_minecraft_client_entity_e_in_classfile_constants)) { *(long long *)out = 0x200000000ll; return 0; }
        (*env)->ExceptionClear(env);
        if ((*env)->GetFieldID(env, mc, str_field_71439_g_in_classfile_constants, str_lnet_minecraft_client_entity_e_in_classfile_constants_a)) { *(long long *)out = 0x200000001ll; return 0; }
        (*env)->ExceptionClear(env);
        if ((*env)->GetFieldID(env, mc, str_theplayer_in_classfile_constants, str_lnet_minecraft_client_entity_e_in_classfile_constants)) { *(long long *)out = 0x100000000ll; out->padding_08 = 0; return 0; }
        (*env)->ExceptionClear(env);
        if ((*env)->GetFieldID(env, mc, str_theplayer_in_classfile_constants, str_lnet_minecraft_client_entity_e_in_classfile_constants_a)) { *(long long *)out = 0x100000001ll; out->padding_08 = 0; return 0; }
        (*env)->ExceptionClear(env);
        return 0x3eb;
    }
    (*env)->ExceptionClear(env);
    jclass c = find_class(loader, env, str_bao_in_classfile_constants);   // "bao" (1.7 obf)
    if (c && (*env)->GetFieldID(env, c, str_i_in_classfile_constants, str_ljava_io_file)) { *(long long *)out = 0; return 0; }
    (*env)->ExceptionClear(env);
    c = find_class(loader, env, str_ave_in_classfile_constants);          // "ave" (1.8 obf)
    if (c && (*env)->GetFieldID(env, c, str_n_in_classfile_constants, str_ljava_io_file)) { *(long long *)out = 1; return 0; }
    (*env)->ExceptionClear(env);
    return 0x3eb;
}

extern "C" const char str_java_lang_thread[], str_getallstacktraces[], str_ljava_util_map_in_classfile_constants[], str_java_util_map_in_classfile_constants[], str_keyset[], str_ljava_util_set[],
    str_java_util_collection[], str_toarray[], str_ljava_lang_object[], str_getname_in_classfile_constants[], str_ljava_lang_string_in_classfile_constants[], str_client_thread[], str_getcontextclassloader[], str_ljava_lang_classloader[];
extern "C" int libc_strcmp(const char *left, const char *right);   // strcmp
// find the game's "Client thread" and keep a global ref to its context class loader (+ loadClass)
extern "C" int find_game_class_loader(LoaderRef *out, JNIEnv *env) {
    jclass T = (*env)->FindClass(env, str_java_lang_thread);
    if (!T) return 0x7d0;
    jmethodID all = (*env)->GetStaticMethodID(env, T, str_getallstacktraces, str_ljava_util_map_in_classfile_constants);
    if (!all) return 0x7d0;
    jclass M = (*env)->FindClass(env, str_java_util_map_in_classfile_constants);
    if (!M) return 0x7d0;
    jmethodID keySet = (*env)->GetMethodID(env, M, str_keyset, str_ljava_util_set);
    if (!keySet) return 0x7d0;
    jobject map = (*env)->CallStaticObjectMethod(env, T, all);
    jobject set = (*env)->CallObjectMethod(env, map, keySet);
    jclass C = (*env)->FindClass(env, str_java_util_collection);
    if (!C) return 0x7d0;
    jmethodID toArray = (*env)->GetMethodID(env, C, str_toarray, str_ljava_lang_object);
    if (!toArray) return 0x7d0;
    jmethodID getName = (*env)->GetMethodID(env, T, str_getname_in_classfile_constants, str_ljava_lang_string_in_classfile_constants);
    if (!getName) return 0x7d0;
    jobjectArray arr = (jobjectArray)(*env)->CallObjectMethod(env, set, toArray);
    jsize n = (*env)->GetArrayLength(env, arr);
    if (n <= 0) return 0x3ea;
    jobject th;
    for (int i = 0;; i++) {
        th = (*env)->GetObjectArrayElement(env, arr, i);
        jstring name = (jstring)(*env)->CallObjectMethod(env, th, getName);
        const char *s = (*env)->GetStringUTFChars(env, name, 0);
        int c = libc_strcmp(s, str_client_thread);   // "Client thread"
        (*env)->ReleaseStringUTFChars(env, name, s);
        if (c == 0) break;
        if (n == i + 1) return 0x3ea;
    }
    jmethodID gccl = (*env)->GetMethodID(env, T, str_getcontextclassloader, str_ljava_lang_classloader);
    if (!gccl) return 0x7d0;
    jobject loader = (*env)->CallObjectMethod(env, th, gccl);
    jclass lc = (*env)->GetObjectClass(env, loader);
    jmethodID load = (*env)->GetMethodID(env, lc, str_loadclass, str_ljava_lang_string_ljava_lang_c);
    if (!load) return 0x7d4;
    jobject ref = (*env)->NewGlobalRef(env, loader);
    if (!ref) return 0x7d5;
    out->loader = ref;
    out->loadClass = load;
    return 0;
}
extern "C" void *class_loader_ref(void **p) { return *p; }
// load class `name` (slashes or dots) through the game's class loader: loader.loadClass(name with dots)
FN(unsigned long long, libc_strlen, (const char *str));
FN(void *, libc_malloc, (unsigned long long n));   // malloc
FN(void, libc_free, (void *ptr));                   // free
extern "C" __attribute__((noinline)) jclass find_class(jobject lr, JNIEnv *env, const char *name) {
    LoaderRef *o = (LoaderRef *)lr;
    unsigned long long n = libc_strlen(name);
    char *buf = (char *)libc_malloc(n + 1);
    if (!buf) return 0;
#pragma clang loop vectorize(disable) interleave(disable)
    for (unsigned long long i = 0; i < n; i++) buf[i] = name[i] == '/' ? '.' : name[i];
    buf[n] = 0;
    jstring s = (*env)->NewStringUTF(env, buf);
    libc_free(buf);
    if (!s) return 0;
    jclass c = (jclass)(*env)->CallObjectMethod(env, o->loader, o->loadClass, s);
    (*env)->DeleteLocalRef(env, s);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);   // class not found: the caller decides what to do
        return 0;
    }
    return c;
}

// ---- gap-scan functions of the class-file reader block ----
// release a global ref: DeleteGlobalRef(env, *ref); *ref = 0
extern "C" void class_file_scan(jobject *ref, JNIEnv *env) {
    if (*ref) {
        (*env)->DeleteGlobalRef(env, *ref);
        *ref = 0;
    }
}

// which Minecraft is it? the obfuscated main class "bao" (field I: File) or "ave" (field N: File)  ->  *kind 0 / 1
// (0x3eb: neither found)
extern "C" const char str_bao_in_classfile_constants[], str_i_in_classfile_constants[], str_ljava_io_file[], str_ave_in_classfile_constants[], str_n_in_classfile_constants[];
extern "C" int class_define(long long *kind, jobject loader, JNIEnv *env) {
    jclass c = find_class(loader, env, str_bao_in_classfile_constants /* "bao" */);
    if (c && (*env)->GetFieldID(env, c, str_i_in_classfile_constants /* "I" */, str_ljava_io_file /* "Ljava/io/File;" */)) {
        *kind = 0;
        return 0;
    }
    (*env)->ExceptionClear(env);
    c = find_class(loader, env, str_ave_in_classfile_constants /* "ave" */);
    if (c && (*env)->GetFieldID(env, c, str_n_in_classfile_constants /* "N" */, str_ljava_io_file)) {
        *kind = 1;
        return 0;
    }
    (*env)->ExceptionClear(env);
    return 0x3eb;
}

FN(void *, libc_memcpy_alt, (void *dest, const void *src, unsigned long long n));
// read n bytes into dst (may be NULL to skip) / a big-endian u32
extern "C" int classfile_read_bytes(RBuf *r, unsigned n, void *dst) {
    const unsigned char *cursor = r->p, *end = cursor + n;
    if (end > r->end) return 0x2af8;
    if (dst) libc_memcpy_alt(dst, cursor, n);
    r->p = end;
    return 0;
}
extern "C" int classfile_read_constant(RBuf *r, unsigned *out) {
    const unsigned char *cursor = r->p, *end = cursor + 4;
    if (end > r->end) return 0x2af8;
    if (out) *out = __builtin_bswap32(*(const unsigned *)cursor);
    r->p = end;
    return 0;
}

// Fieldref/Methodref-style entry whose tag satisfies `pred` -> class, name, descriptor (general form of constant_pool_member_ref/constant_pool_member_ref2)
FN(int, constant_name_and_type, (CP *cp, unsigned short idx, NameType *out));
extern "C" int constant_pool_find_if(CP *constant_pool, bool (*pred)(unsigned char), unsigned short idx, MemberRef *out) {
    const unsigned char *end, *lim;
    int error_code = 0x2afd;
    CP_ENTRY(constant_pool, idx, end, lim, out_);
    if (end + 1 > lim) goto trunc;
    {
        unsigned char tag = *end;
        error_code = 0x2afe;
        if (!pred(tag)) goto out_;
    }
    if (end + 3 > lim) goto trunc;
    error_code = 0x2af8;
    if (end + 5 > lim) goto out_;
    {
        unsigned short ci = be16(end + 1);
        unsigned char hi = end[3], lo = end[4];
        error_code = constant_class_name(constant_pool, ci, &out->cls);
        if (error_code) goto out_;
        return constant_name_and_type(constant_pool, (unsigned short)((hi << 8) | lo), &out->nt);
    }
trunc:
    error_code = 0x2af8;
out_:
    return error_code;
}
