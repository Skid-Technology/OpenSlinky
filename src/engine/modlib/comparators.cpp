// 0x3d000: comparators
#include "slinky/engine.h"
extern "C" int sort_key_table[];
extern "C" int compare_module_height(char *left, char *right) { return (int)(*(float *)(right + 0x48) - *(float *)(left + 0x48)); }
extern "C" int compare_key_table(short *left, short *right) { return sort_key_table[*left] - sort_key_table[*right]; }

// release the cached player records: their two global refs and five lists of global refs
#include "slinky/jids.h"
FN(void, libc_free, (void *ptr));
struct RefList { int padding_00; int n; jobject *refs; };
struct PRec { char padding_00[0x30]; jobject ref_primary, ref_secondary; char padding_40[0x10]; RefList lists[5]; };
extern "C" unsigned long long name_tags_count;
extern "C" PRec name_tags_table[];
extern "C" void entity_list_for_render(Jvm *jvm) {
    if (name_tags_count) {
        JNIEnv *env = jvm->env;
        for (unsigned long long i = 0; i < name_tags_count; i++) {
            PRec *p = &name_tags_table[i];
            if (p->ref_secondary) (*env)->DeleteGlobalRef(env, p->ref_secondary);
            if (p->ref_primary) (*env)->DeleteGlobalRef(env, p->ref_primary);
#pragma clang loop unroll(full)
            for (int k = 0; k < 5; k++) {
                RefList *l = &p->lists[k];
                if (!l->refs) continue;
                for (long long m = 0; m < l->n; m++) (*env)->DeleteGlobalRef(env, l->refs[m]);
                libc_free(l->refs);
                l->refs = 0;
            }
        }
    }
    name_tags_count = 0;
}
