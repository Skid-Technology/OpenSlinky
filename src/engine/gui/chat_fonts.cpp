// 0xf000: chat command check, GUI font/style setup
#include "slinky/engine.h"
#include "slinky/jids.h"
FN(jstring, java_get_c14_packet_tab_complete, (Jvm *jvm, void *c14_packet_tab_complete));
// does the text of obj start with '!'?
extern "C" bool text_starts_with_bang(Jvm **c, void *obj) {
    JNIEnv *env = (*c)->env;
    jstring c14_packet_tab_complete = java_get_c14_packet_tab_complete(*c, obj);
    bool r;
    if ((*env)->GetStringLength(env, c14_packet_tab_complete)) {
        const jchar *p = (const jchar *)(*env)->GetStringCritical(env, c14_packet_tab_complete, 0);
        r = *p == '!';
        (*env)->ReleaseStringCritical(env, c14_packet_tab_complete, p);
    } else {
        r = false;
    }
    return r;
}

// ---- chat commands: "!word word arg arg" walks the command tree, parses up to 3 typed arguments and runs the
// node's handler; prints help (sub-commands) or usage on errors. "!!text" sends "!text" as a normal message. ----
struct CBuf { char *p; unsigned long long cap, len; };
union CVal { struct { const char *p; unsigned long long n; } s; unsigned long long i; unsigned char b; };
struct CArg { const char *name; int type; unsigned char opt; char padding_0d[3]; CVal dflt; };   // type 1 str, 2 uint, 3 bool
struct CNode { const char *name, *desc; CArg args[3]; void (*run)(Jvm **c, CVal *v); CNode *sub; };
extern "C" CNode chat_fonts_root;   // root
FN(void *, java_get_c01_packet_chat_message, (Jvm *jvm, void *msg));
FN(void, java_set_c01_packet_chat_message, (Jvm *jvm, void *msg, jstring s));
FN(void, buffer_init, (CBuf *b, char *p, unsigned long long cap));
FN(bool, buffer_printf, (CBuf *b, const char *fmt, ...));
FN(bool, buffer_push_char, (CBuf *b, char character));
FN(bool, buffer_terminate, (CBuf *b));
FN(unsigned long long, libc_strlen, (const char *str));
FN(void *, libc_memcpy_alt, (void *dest, const void *src, unsigned long long n));
FN(int, libc_strncmp_alt, (const void *left, const void *right, unsigned long long n));
extern "C" void jni_throw_format(Jvm *jvm, const char *fmt, ...);
extern "C" const char str_s_in_chat_fonts[], str_sect_7_s[], str_sect_f_s_s_sect_7_s[], str_sect_f_message_sect_7_send_sec[], str_sect_cmissing_argument_sect_o[], str_sect_cargument_sect_o_ssect_c[],
    str_sect_cargument_sect_o_ssect_c_in_chat_fonts[], str_sect_ctoo_many_arguments_provi[], str_sect_c_s[], str_csect_o_ssect_c_s_c[];
extern "C" const char *const ptr_to_str_string[];   // type names
extern "C" bool handle_chat_message(Jvm **c, void *msg) {
    JNIEnv *env = (*c)->env;
    jstring c01_packet_chat_message = (jstring)java_get_c01_packet_chat_message(*c, msg);
    if (!c01_packet_chat_message) return false;
    const char *utf = (*env)->GetStringUTFChars(env, c01_packet_chat_message, 0);
    if (!utf) return false;
    char buf[0x1a0];
    unsigned long long ulen = libc_strlen(utf);
    if (ulen >= sizeof buf) ulen = sizeof buf - 1;
    libc_memcpy_alt(buf, utf, ulen);
    buf[ulen] = 0;
    (*env)->ReleaseStringUTFChars(env, c01_packet_chat_message, utf);
    if (!buf[0] || buf[0] != '!') return false;
    char *p = buf + 1;
    if (buf[1] == '!') {
        java_set_c01_packet_chat_message(*c, msg, (*env)->NewStringUTF(env, p));
        return false;
    }
    char pb[0x100], ubuf[0x100];
    CBuf path;
    buffer_init(&path, pb, 0x100);
    buffer_push_char(&path, '!');
    unsigned long long L = libc_strlen(p), pos = 0;
    CNode *node = &chat_fonts_root;
    Jvm *jvm;
    const char *err;
    CVal v[3];
    unsigned long long i;
    // command words
    while (node->sub) {
        if (pos == L) goto help;
        unsigned long long we = pos;
        while (we < L && p[we] != ' ') we++;
        unsigned long long nx = we;
        while (nx < L && p[nx] == ' ') nx++;
        unsigned long long wl = we - pos;
        CNode *sub = node->sub;
        for (; sub->name; sub++)
            if (libc_strlen(sub->name) == wl && libc_strncmp_alt(sub->name, p + pos, wl) == 0) break;
        if (!sub->name) goto help;
        buffer_printf(&path, str_s_in_chat_fonts /* "%s " */, sub->name);
        pos = nx;
        node = sub;
    }
    // arguments
    jvm = *c;
    for (i = 0; i != 3; i++) {
        CArg *a = &node->args[i];
        if (!a->name) break;
        if (pos == L) {
            if (!(a->opt & 1)) {
                err = str_sect_cmissing_argument_sect_o;   // missing argument
                goto usage;
            }
            if (a->type == 3) v[i].b = a->dflt.b & 1;
            else if (a->type == 2) v[i].i = a->dflt.i;
            else if (a->type == 1) v[i] = a->dflt;
            continue;
        }
        unsigned long long we = pos;
        while (we < L && p[we] != ' ') we++;
        unsigned long long nx = we;
        while (nx < L && p[nx] == ' ') nx++;
        const char *w = p + pos;
        unsigned long long wl = we - pos;
        if (a->type == 3) {
            unsigned d = (unsigned char)w[0] - 0x46u;
            if (d > 0x33) goto badbool;
            if ((0x10100000101ull >> d) & 1) v[i].b = 0;            // F N f n
            else if ((0x8400000084000ull >> d) & 1) v[i].b = 1;     // T Y t y
            else goto badbool;
        } else if (a->type == 2) {
            unsigned long long x = 0;
            for (unsigned long long k = 0; k < wl; k++) {
                unsigned char ch = (unsigned char)w[k];
                if ((unsigned char)(ch - ':') < 0xf6) goto badint;
                unsigned long long y = x * 10 + (ch & 0xf);
                if (y < x) goto badint;
                x = y;
            }
            v[i].i = x;
        } else if (a->type == 1) {
            v[i].s.p = w;
            v[i].s.n = wl;
        }
        pos = nx;
    }
    if (pos < L) {
        jni_throw_format(jvm, str_sect_ctoo_many_arguments_provi);   // too many arguments
        goto usage_line;
    }
    node->run(c, v);
    return true;
badbool:
    err = str_sect_cargument_sect_o_ssect_c_in_chat_fonts;
    goto usage;
badint:
    err = str_sect_cargument_sect_o_ssect_c;
usage:
    jni_throw_format(jvm, err);
usage_line : {
    buffer_terminate(&path);
    CBuf u;
    buffer_init(&u, ubuf, 0x100);
    buffer_printf(&u, str_sect_c_s, path.p);
    for (int k = 0; k < 3; k++) {
        CArg *a = &node->args[k];
        if (!a->name) continue;
        bool o = a->opt & 1;
        buffer_printf(&u, str_csect_o_ssect_c_s_c, o ? '[' : '<', a->name, ptr_to_str_string[(unsigned)a->type], o ? ']' : '>');
    }
    buffer_terminate(&u);
    jni_throw_format(jvm, ubuf);
    return true;
}
help : {
    buffer_terminate(&path);
    jvm = *c;
    jni_throw_format(jvm, str_sect_7_s, node->desc);
    for (CNode *ch = node->sub; ch->name; ch++) jni_throw_format(jvm, str_sect_f_s_s_sect_7_s, path.p, ch->name, ch->desc);
    if (node == &chat_fonts_root) jni_throw_format(jvm, str_sect_f_message_sect_7_send_sec);
    return true;
}
}

// ---- GLFW key code -> client key code (0 = unmapped). Counterpart of windows_key_to_client_key (Windows virtual keys). ----
extern "C" int glfw_key_to_client_key(int key) {
    switch (key) {
    case 0x20: return 0x58;                                   // space
    case 0x27: return 0x3e;                                   // '
    case 0x2c: return 0x36;                                   // ,
    case 0x2d: case 0x14d: return 0x37;                       // - (also keypad -)
    case 0x2e: case 0x14a: return 0x38;                       // . (also keypad .)
    case 0x2f: case 0x14b: return 0x39;                       // / (also keypad /)
    case 0x30 ... 0x39: return key - 0x2a;                    // 0-9
    case 0x3b: return 0x34;                                   // ;
    case 0x3d: case 0x14e: case 0x150: return 0x35;           // = (also keypad)
    case 0x41 ... 0x5a: return key - 0x27;                    // A-Z
    case 0x5b: return 0x3b;
    case 0x5c: return 0x3c;
    case 0x5d: return 0x3d;
    case 0x60: return 0x3a;
    case 0x100: return 0x57;                                  // escape
    case 0x101: case 0x14f: return 0x55;                      // enter (also keypad)
    case 0x102: return 0x54;                                  // tab
    case 0x103: return 0x53;                                  // backspace
    case 0x104: return 0x5e;                                  // insert
    case 0x105: return 0x5f;                                  // delete
    case 0x106: return 0x41;                                  // right
    case 0x107: return 0x3f;                                  // left
    case 0x108: return 0x42;                                  // down
    case 0x109: return 0x40;                                  // up
    case 0x10a: return 0x59;                                  // page up
    case 0x10b: return 0x5a;                                  // page down
    case 0x10c: return 0x5c;                                  // home
    case 0x10d: return 0x5b;                                  // end
    case 0x118: return 0x56;                                  // caps lock
    case 0x119: return 0x61;                                  // scroll lock
    case 0x11a: return 0x60;                                  // num lock
    case 0x11b: return 0x5d;                                  // print screen
    case 0x122 ... 0x131: return key - 0xdf;                  // F1-F16
    case 0x140 ... 0x149: return key - 0x130;                 // keypad 0-9
    case 0x154: return 0x62;                                  // left shift
    case 0x155: return 0x64;                                  // left control
    case 0x156: return 0x66;                                  // left alt
    case 0x158: return 0x63;                                  // right shift
    case 0x159: return 0x65;                                  // right control
    case 0x15a: return 0x67;                                  // right alt
    default: return 0;
    }
}

// copy the two built-in chat commands (descriptors in .rdata) into the root command's sub-command array
extern "C" const CNode *ptr_to_rdata_1531c0, *ptr_to_rdata_151518;
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));
extern "C" int init_chat_commands() {
    CNode *cmds = (CNode *)libc_calloc(3, sizeof(CNode));   // two commands + a zero terminator
    if (!cmds) return 0x3ed;
    cmds[0] = *ptr_to_rdata_1531c0;
    cmds[1] = *ptr_to_rdata_151518;
    chat_fonts_root.sub = cmds;
    return 0;
}

// next token of a text reader: returns a pointer to it and its length in *len, then skips the delimiters after it
struct TextReader { const char *data; unsigned long long end, pos; };
extern "C" const char *text_reader_next(TextReader *r, char delim, unsigned long long *len) {
    if (r->pos == r->end) return 0;
    unsigned long long start = r->pos, p = start;
    const char *data = r->data;
    while (p < r->end && data[p] != delim) r->pos = ++p;
    *len = p - start;
    while (r->pos < r->end && data[r->pos] == delim) r->pos++;
    return data + start;
}

// load a font file into slot `slot` of the font table (0x1772 on failure)
FN(int, nvg_create_font_mem, (void *nvg, const char *path, const char *name, int size, int flags));
extern "C" int find_glfw_entrypoint(void **ctx, unsigned slot, const char *path, const char *name, int size) {
    int id = nvg_create_font_mem(ctx[0], path, name, size, 0);
    if (id == -1) return 0x1772;
    ((int *)ctx)[0x10 + slot] = id;
    return 0;
}

// ---- small helpers the first gap scan missed ----
extern "C" void *chat_font_context;
FN(void, libc_free, (void *ptr));
FN(unsigned long long, libc_strlen, (const char *text));
FN(void *, libc_memcpy_alt, (void *dest, const void *src, unsigned long long n));
extern "C" void destroy_chat_commands() {
    libc_free(chat_fonts_root.sub);
    chat_fonts_root.sub = 0;
}

// bounded memory cursor {data, size, pos}: the sink of the output helpers (fwrite-like mem_cursor_write)
struct MemCursor { char *data; unsigned long long size, pos; };
extern "C" MemCursor *mem_cursor_init(MemCursor *c, char *data, unsigned long long size) {
    c->data = data;
    c->size = size;
    c->pos = 0;
    return c;
}
extern "C" MemCursor *text_reader_init(MemCursor *c, char *text) {
    unsigned long long n = libc_strlen(text);
    c->data = text;
    c->size = n;
    c->pos = 0;
    return c;
}
extern "C" bool text_reader_has_more(const MemCursor *c) { return c->pos < c->size; }
// fwrite onto the cursor: append `count` records of `size` bytes if they all fit, returns the bytes written
extern "C" unsigned long long mem_cursor_write(const void *src, unsigned long long size, unsigned long long count, MemCursor *c) {
    unsigned long long n = size * count;
    if (c->pos + n > c->size) return 0;
    libc_memcpy_alt(c->data + c->pos, src, n);
    c->pos += n;
    return n;
}
