// 0x21000: class-hook installer (JVMTI) and the table of native hook bodies
// Defines SlinkyHooks13 in the game's class loader, registers its natives and rewrites the nine hooked game classes
// through the ClassFileLoadHook (class_file_load_hook, see classfile_hooks.cpp). The tables of what gets rewritten are data
// (hook_tables.inc, generated from the image by tools/gen_hooks.py): the original assembled them on its stack.
#include "slinky/engine.h"
#include "slinky/jnienv.h"
#include "slinky/jids.h"
FN(void *, libc_memcpy_alt, (void *dest, const void *src, unsigned long long n));
extern "C" void slinky_logf(const char *fmt, ...);
static inline const char *cs(const void *p) { return p ? (const char *)p : "?"; }
extern "C" char hook_native_pointer_table[];
// Copies the 23 native pointers to hook_native_pointer_table (the original installer read them back from there; this
// implementation takes them from kNatives, so nothing calls this any more)
extern "C" void *copies_23_native_pointers_data(const void *natives) { return libc_memcpy_alt(hook_native_pointer_table, natives, 0xb8); }

// ---- run-time layouts shared with the ClassFileLoadHook code (classfile_hooks.cpp, classfile_patch_helpers.cpp) ----
struct MHook { const char *name, *desc; unsigned short ref; };                       // a native of SlinkyHooks13; ref = its Methodref
struct HookEnt { const char *owner, *name, *desc; long long kind; unsigned short *ref; };   // instruction to replace
struct MethodPat { const char *name, *desc; HookEnt *ents; unsigned long long n; };   // method to rewrite (-1, -1 = all)
struct HookRec { jclass cls; MHook *natives; unsigned long long nnat; MethodPat *pats; unsigned long long npats; };

// ---- compile-time description of the hooks ----
enum { INVOKE_VIRTUAL = 1, INVOKE_STATIC = 2, GETFIELD = 3, GETSTATIC = 4, PUTFIELD = 5, PUTSTATIC = 6 };
#define ANY nullptr
struct NativeRef { const char *name, *desc; };
struct Site { const char *owner, *name, *desc; int kind; int native; };   // `native` indexes the class's NativeRef list
struct Method { const char *name, *desc; const Site *sites; int nsites; };
struct HookedClass { jclass JIds::*cls; const NativeRef *natives; int nnatives; const Method *methods; int nmethods; };
#include "hook_tables.inc"

// ---- JVMTI (function table indices) ----
struct JvmtiTable { void *fn[160]; };
typedef int (*SetEventMode)(JvmtiTable **, int mode, int event, void *thread);
typedef int (*AddCaps)(JvmtiTable **, const void *caps);
typedef int (*SetCallbacks)(JvmtiTable **, const void *callbacks, int size);
typedef int (*Retransform)(JvmtiTable **, int count, jclass *classes);
#define JVMTI(env, idx, T) ((T)(*(env))->fn[idx])
enum { JVMTI_DISABLE = 0, JVMTI_ENABLE = 1, EV_CLASS_FILE_LOAD_HOOK = 0x36 };
enum { SET_EVENT_NOTIFICATION_MODE = 1, SET_EVENT_CALLBACKS = 121, ADD_CAPABILITIES = 141, RETRANSFORM_CLASSES = 151 };
enum { JVMTI_ERROR_BASE = 12000 };   // client error code = 12000 + jvmtiError

extern "C" unsigned char classfile_hooks_1_while_hooks_are_installed;   // 1 while the hooks are installed
extern "C" unsigned char classfile_hooks_world_client_pending;           // WorldClient retransform still to do (Java 9+)
extern "C" jobject classfile_hooks_global_ref_transformer_class;          // global ref of SlinkyHooks13
extern "C" int classfile_hooks_first_error;              // first error seen by the ClassFileLoadHook (sticky)
extern "C" HookRec hooked_class_minecraft[9];       // the hooked classes, read by the ClassFileLoadHook
extern "C" const char str_slinkyhooks13[];    // "SlinkyHooks13"
extern "C" const char ptr_to_hook_installer_class_file_slinkyhooks13[];    // jvmtiCapabilities: can_retransform_classes
extern "C" const char ptr_to_class_file_load_hook[];    // jvmtiEventCallbacks (0x138 bytes): ClassFileLoadHook = class_file_load_hook
extern "C" const jbyte hook_installer_class_file_slinkyhooks13[];   // class file of SlinkyHooks13
extern "C" const int hook_installer_its_length;       // its length
FN(int, define_class_with_natives, (JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len, const JNINativeMethod *methods, jint n,
               jclass *out));

// static storage for the records of the chosen mapping (the ClassFileLoadHook reads them for as long as the hooks
// are installed)
static MHook g_natives[9][6];
static MethodPat g_pats[9][3];
static HookEnt g_ents[32];
static unsigned long long g_nents;

// The original installer assembled parts of this table at run time from the JIds method name /
// signature slots (RendererLivingEntity/RenderPlayer doRender + canRenderName): tools/gen_hooks.py
// could not recover those values statically, because dump_hooks.py had filled every JIds pointer
// with a fake 0x10000000+offset when it decoded the table - the generated table therefore carries
// "BAD@<ids offset>" placeholders. Resolve them against the live JIds exactly like the original.
static const char *hook_string(JIds *ids, const char *s) {
    if (!s || s[0] != 'B' || s[1] != 'A' || s[2] != 'D' || s[3] != '@') return s;
    unsigned long long off = 0;
    for (const char *p = s + 4; *p; p++)
        off = off * 16 + (unsigned long long)(*p <= '9' ? *p - '0' : (*p | 0x20) - 'a' + 10);
    // The generator wrote the dumper's fake pointer base into the placeholder: "BAD@10000968" is
    // JIds+0x968, not JIds+0x10000968. Using the raw value read 256 MB past the table, which crashed
    // wherever that address was unmapped (vanilla 1.8.9: access violation in this function) and
    // silently produced garbage pointers where it happened to be mapped.
    if (off >= 0x10000000) off -= 0x10000000;
    if (off + sizeof(const char *) > sizeof(JIds)) return 0;   // malformed placeholder
    return *(const char **)((char *)ids + off);
}

static void build_records(JIds *ids, const HookedClass *tab) {
    g_nents = 0;
    for (int c = 0; c < 9; c++) {
        const HookedClass &hc = tab[c];
        HookRec &rec = hooked_class_minecraft[c];
        for (int i = 0; i < hc.nnatives; i++) g_natives[c][i] = MHook{hook_string(ids, hc.natives[i].name), hook_string(ids, hc.natives[i].desc), 0};
        for (int m = 0; m < hc.nmethods; m++) {
            const Method &meth = hc.methods[m];
            HookEnt *first = &g_ents[g_nents];
            for (int s = 0; s < meth.nsites; s++) {
                const Site &site = meth.sites[s];
                g_ents[g_nents++] = HookEnt{hook_string(ids, site.owner), hook_string(ids, site.name), hook_string(ids, site.desc), site.kind, &g_natives[c][site.native].ref};
            }
            const char *any = (const char *)-1;
            g_pats[c][m] = MethodPat{meth.name ? hook_string(ids, meth.name) : any, meth.desc ? hook_string(ids, meth.desc) : any, first,
                                     (unsigned long long)meth.nsites};
        }
        rec = HookRec{ids->*hc.cls, g_natives[c], (unsigned long long)hc.nnatives, g_pats[c], (unsigned long long)hc.nmethods};
    }
}

static bool java9_or_newer(JNIEnv *env) {
    if (!env) return false;
    return ((unsigned)(*env)->GetVersion(env) >> 16) >= 9;
}

// Install the hooks: 0 or an error (JNI/loader errors as the client defines them, 12000 + jvmtiError)
extern "C" int install_class_hooks(Jvm *jvm, JvmtiTable **jvmti) {
    if (!jvm || !jvm->ids) {
        slinky_logf("classhooks: install called with bad jvm=%p ids=%p", jvm, jvm ? jvm->ids : 0);
        return 0x7d0;
    }
    slinky_logf("classhooks: install jvm=%p ids=%p env=%p jvmti=%p installed=%d", jvm, jvm->ids, jvm->env, jvmti, (int)classfile_hooks_1_while_hooks_are_installed);
    if (classfile_hooks_1_while_hooks_are_installed) return 0;
    JNIEnv *env = jvm->env;
    JIds *ids = jvm->ids;
    // Build the per-class hook records before the ClassFileLoadHook is enabled: the callback matches a
    // load against these entries, and an entry that is still zero (or a class that failed to resolve)
    // would match JVMTI's class_being_redefined == NULL for a first-time load and rewrite every class.
    build_records(ids, kHooks[ids->variant][ids->names]);

    JNINativeMethod natives[23];
    for (int i = 0; i < 23; i++) natives[i] = JNINativeMethod{(char *)kNatives[i].name, (char *)kNatives[i].desc, kNatives[i].fn};
    jclass cls;
    int err = define_class_with_natives(env, str_slinkyhooks13, (jobject)((char *)ids + 0x10 /* game loader + loadClass */), hook_installer_class_file_slinkyhooks13,
                      hook_installer_its_length, natives, 23, &cls);
    if (err) return err;
    classfile_hooks_global_ref_transformer_class = (*env)->NewGlobalRef(env, cls);

    if ((err = JVMTI(jvmti, ADD_CAPABILITIES, AddCaps)(jvmti, ptr_to_hook_installer_class_file_slinkyhooks13))) return JVMTI_ERROR_BASE + err;
    char callbacks[0x138];
    __builtin_memcpy(callbacks, ptr_to_class_file_load_hook, sizeof callbacks);
    if ((err = JVMTI(jvmti, SET_EVENT_CALLBACKS, SetCallbacks)(jvmti, callbacks, sizeof callbacks)))
        return JVMTI_ERROR_BASE + err;
    if ((err = JVMTI(jvmti, SET_EVENT_NOTIFICATION_MODE, SetEventMode)(jvmti, JVMTI_ENABLE, EV_CLASS_FILE_LOAD_HOOK, 0)))
        return JVMTI_ERROR_BASE + err;

    slinky_logf("classhooks: variant=%d names=%d canRenderName=%s%s doRender=%s%s playerDoRender=%s%s",
                (int)ids->variant, (int)ids->names,
                cs(ids->RendererLivingEntity_canRenderNameMethodName), cs(ids->RendererLivingEntity_canRenderNameMethodSignature),
                cs(ids->RendererLivingEntity_doRenderMethodName), cs(ids->RendererLivingEntity_doRenderMethodSignature),
                cs(ids->RenderArrow_doRenderMethodName), cs(ids->RenderArrow_doRenderMethodSignature));
    for (unsigned long long k = 0; k < g_nents; k++)
        slinky_logf("ent[%llu] owner=%s kind=%d name=%p desc=%p", k, g_ents[k].owner, (int)g_ents[k].kind, (void *)g_ents[k].name, (void *)g_ents[k].desc);
    __atomic_exchange_n(&classfile_hooks_first_error, 0, __ATOMIC_SEQ_CST);
    // Retransform one class at a time: classes the game has not loaded yet make the
    // all-at-once call fail, and they will be patched later by the still-enabled
    // ClassFileLoadHook when the game loads them (RenderPlayer/WorldClient on world join).
    // Java 9+ class transformers (Lunar/Badlion) can deadlock inside
    // RetransformClasses(WorldClient) while their transformer is active. The
    // world hooks are the only ones affected; skip that one retransform so
    // the installer completes and the client stays responsive. The
    // ClassFileLoadHook remains enabled for classes loaded later.
    const bool java9plus = java9_or_newer(env);
    if (java9plus)
        slinky_logf("classhooks: Java 9+ detected; skipping WorldClient retransform");
    for (int i = 0; i < 9; i++) {
        jclass one = hooked_class_minecraft[i].cls;
        slinky_logf("classhooks: retransform[%d]=%p", i, one);
        if (!one) continue;
        if (java9plus && i == 6) {
            slinky_logf("classhooks: retransform[%d] skipped on Java 9+ (deferred)", i);
            __atomic_store_n(&classfile_hooks_world_client_pending, 1, __ATOMIC_SEQ_CST);
            continue;
        }
        err = JVMTI(jvmti, RETRANSFORM_CLASSES, Retransform)(jvmti, 1, &one);
        slinky_logf("classhooks: retransform[%d] err=%d first_error=%d", i, err, classfile_hooks_first_error);
        if (err) __sync_val_compare_and_swap(&classfile_hooks_first_error, 0, JVMTI_ERROR_BASE + err);
    }
    if (classfile_hooks_first_error) __atomic_store_n(&classfile_hooks_world_client_pending, 0, __ATOMIC_SEQ_CST);
    __atomic_exchange_n(&classfile_hooks_1_while_hooks_are_installed, 1, __ATOMIC_SEQ_CST);
    // Keep the ClassFileLoadHook enabled: classes that were not loaded yet (RenderPlayer,
    // WorldClient, ...) are only retransformed when the game loads them after joining a world.
    slinky_logf("classhooks: installed, hook stays enabled, first_error=%d", classfile_hooks_first_error);
    return classfile_hooks_first_error;
}
