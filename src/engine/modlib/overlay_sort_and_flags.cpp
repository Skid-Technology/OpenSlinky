// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
extern "C" int libc_strcmp(const char *left, const char *right);
extern "C" unsigned char notifications_gui_hint_shown;
extern "C" int compare_overlay_entries_by_height_desc(char *left, char *right) {
    float fa = *(float *)(left + 0x3c), fb = *(float *)(right + 0x3c);
    if (fb > fa) return 1;
    if (fa > fb) return -1;
    char *pb = right + 8;
    char *pa = left + 8;
    return libc_strcmp(pa, pb);
}
extern "C" unsigned char take_and_clear_pending_flag() { return __atomic_exchange_n(&notifications_gui_hint_shown, 0, __ATOMIC_SEQ_CST); }
