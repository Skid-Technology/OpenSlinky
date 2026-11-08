// Notifications - pop-up messages in the top left corner.
//
// Shows a message box that slides in and fades out after 5 seconds for: profile loaded /
// saved, errors, friend added / removed, and modules switched on / off (several in one tick
// become "Enabled 3 mods"). The first time the GUI opens it shows "Press X to open GUI".
// Messages are kept in a ring buffer of 10 (the oldest is dropped).
//
// Callbacks:
//   notifications_profile_loaded_saved  a profile was loaded / saved
//   notifications_on_error_event        an error
//   notifications_on_friend_event       a friend was added / removed
//   notifications_module_toggled_gui    a module was switched on / off
//   notifications_tick_age_messages     every frame: age the messages, post the batched ones
//   notifications_draw_slide_boxes      every frame: draw
#include "slinky/engine.h"

struct Module { const char *name; char padding_08[8]; int category; int padding_14; int flags; int padding_1c; unsigned char hidden; };
struct ToggleEvt { char padding_00[0x10]; Module *mod; unsigned char on; };
struct Recent { const char *name; int count; };
struct Note { int kind; char text[0x68]; float age; };     // 0x70 bytes
struct ProfileEvt { char padding_00[8]; const char *name; unsigned status; };
struct ErrorEvt { char padding_00[8]; int code; int padding_0c; const char *msg; };
struct FriendEvt { char padding_00[8]; unsigned char added; };

S(fmt_str_str);   // "%s %s"
S(str_default_in_notifications);   // "Default"
S(str_s_in_notifications);   // "%s"
S(str_error_code_d);   // "Error code %d"
extern "C" const char *const ptr_to_str_loaded_profile[2];   // "Loaded profile", "Saved profile"
extern "C" char notifications_module_descriptor[];                 // GUI module descriptor
extern "C" unsigned char notifications_hide_flagged_hidden_modules, notifications_hide_flagged_hidden_modules_second;            // hide flagged / hidden modules
extern "C" unsigned char notifications_per_category, notifications_per_category_second, notifications_per_category_third, notifications_per_category_fourth, notifications_per_category_fifth;   // per category
extern "C" unsigned char notifications_gui_hint_shown;            // GUI hint shown
extern "C" Recent notifications_last_enabled;                   // last enabled
extern "C" Recent notifications_last_disabled;                   // last disabled
extern "C" int *const ptr_to_gui_module_descriptor;              // client config (+0x1c: GUI key)
S(str_press_s_to_open_gui);
S(str_enabled_in_notifications);   // "Enabled"
S(str_disabled);   // "Disabled"
S(str_s_u_mods);   // "%s %u mods"
S(str_slinky_in_notifications);   // "SLINKY"
struct Color { float r, g, b, a; };
struct Rect { float x, y, w, h; };
struct Ui { void *dl; };
struct TickHook { char padding_00[0x10]; unsigned flags; float dt; };
struct DrawHook { char padding_00[0x10]; Ui *ui; };
extern "C" unsigned char notifications_only_while_gui_open;    // only while the GUI is open
extern "C" const float threshold_5_in_notifications, font_size_14_in_notifications, notification_top_margin, alpha_1_in_notifications, factor_neg_100, colour_level_0_866667_in_notifications,
                       colour_level_0_278431_in_notifications, colour_level_0_54902, colour_level_0_92549, colour_level_0_168627, offset_102, colour_level_0_0784314_in_notifications,
                       corner_radius_20_in_notifications, rect_extent_8_in_notifications, rect_extent_neg_16_in_notifications, rect_extent_70, colour_alpha_0_15_in_notifications, corner_radius_10_in_notifications, factor_50_in_notifications;
extern "C" const double threshold_4_8, threshold_0_2_in_notifications, divisor_0_2_in_notifications_a, threshold_4_8_in_notifications, offset_5_in_notifications;
FN(void, nvg_FontSize, (void *nvg, float v));
FN(void, nvg_GlobalAlpha, (void *nvg, float alpha));
FN(float, ease_curve, (float t));
FN(Color, nvg_rgba_f, (float r, float g, float b, float a));
FN(Color, nvg_rgb_f, (float r, float g, float b));
FN(Color, get_speed_pair, ());
FN(Color, nvg_trans_rgba_f, (Color c, float mul));
FN(float, nvg_text_bounds, (void *nvg, float size, float wrap, const char *text, void *end, void *extra));
FN(Rect, make_rect, (float x, float y, float w, float h));
FN(void, nvg_FillColor, (void *nvg, Color c));
FN(void, draw_rect_rounded, (Ui *ui, Rect r, float rounding));
FN(void, draw_text, (Ui *ui, Rect r, int align, int flags, const char *text));   // "Press %s to open GUI"
FN(const char *, mouse_button_name, (int key));
extern "C" unsigned long long active_notification_count;      // count
extern "C" Note notification_queue[10];
extern "C" unsigned long long notifications_head;      // head

// Message kinds (colour of the box): 0 profile (green), 1 info (accent colour), 2 error (red).
enum { NOTE_PROFILE = 0, NOTE_INFO = 1, NOTE_ERROR = 2 };

// Add a message at the end of the ring buffer (dropping the oldest when full).
static Note *push(int kind) {
    unsigned long long head = notifications_head, n = active_notification_count;
    Note *note = &notification_queue[(n + head) % 10];
    note->kind = kind;
    note->age = 0;
    unsigned long long *slot = &active_notification_count, v = n;
    if (n == 10) {
        slot = &notifications_head;
        v = head;
    }
    *slot = v + 1;
    return note;
}

extern "C" void notifications_profile_loaded_saved(ProfileEvt *p) {
    unsigned st = p->status;
    if (st == 2)
        return;
    const char *label = ptr_to_str_loaded_profile[st];
    const char *name = p->name ? p->name : str_default_in_notifications;
    Note *note = push(NOTE_PROFILE);
    log_fmt(note->text, 0x65, fmt_str_str, label, name);
}

extern "C" void notifications_on_error_event(ErrorEvt *p) {
    Note *note = push(NOTE_ERROR);
    if (p->msg)
        log_fmt(note->text, 0x65, str_s_in_notifications, p->msg);
    else
        log_fmt(note->text, 0x65, str_error_code_d, p->code);
}

extern "C" void notifications_on_friend_event(FriendEvt *p) {
    Note *note = push(NOTE_INFO);
    if (p->added == 1)
        __builtin_memcpy(note->text, "Friend added", 13);
    else
        __builtin_memcpy(note->text, "Friend removed", 15);
}

// Should switching this module on / off be announced?
static bool announced(Module *module) {
    int flags = module->flags;
    if (flags & 0x10)
        return false;
    if ((flags & 2) && notifications_hide_flagged_hidden_modules)
        return false;
    if (module->hidden == 1 && notifications_hide_flagged_hidden_modules_second)
        return false;
    switch ((unsigned)module->category) {
    case 0: return notifications_per_category;          // combat
    case 1: return notifications_per_category_second;   // move
    case 2: return notifications_per_category_fourth;   // visual
    case 3: return notifications_per_category_third;    // block
    case 4: return notifications_per_category_fifth == 1;   // utility
    default: return true;
    }
}

// A module was switched on / off. The GUI itself: the first time, show the key hint.
// Others are counted, and announced together on the next tick.
extern "C" void notifications_module_toggled_gui(ToggleEvt *h) {
    Module *module = h->mod;
    if ((void *)module == notifications_module_descriptor) {
        if (!h->on) {
            active_notification_count = 0;
            return;
        }
        unsigned char expected = 0;
        if (!__atomic_compare_exchange_n(&notifications_gui_hint_shown, &expected, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
            return;
        Note *note = push(NOTE_INFO);
        log_fmt(note->text, 0x65, str_press_s_to_open_gui, mouse_button_name(ptr_to_gui_module_descriptor[0x1c / 4]));
        return;
    }
    if (!announced(module))
        return;
    Recent *recent = h->on ? &notifications_last_enabled : &notifications_last_disabled;
    recent->name = module->name;
    recent->count += 1;
}

// tick: age the messages, drop expired ones, post the batched enable / disable messages
extern "C" void notifications_tick_age_messages(TickHook *hook) {
    if (notifications_only_while_gui_open == 1 && !((unsigned char *)ptr_to_gui_module_descriptor)[0x14]) {
        active_notification_count = 0;
        notifications_last_enabled.count = 0;
        notifications_last_disabled.count = 0;
        return;
    }
    unsigned long long n = active_notification_count;
    while (n) {
        if (!(notification_queue[notifications_head % 10].age > threshold_5_in_notifications))
            break;
        notifications_head += 1;
        active_notification_count = --n;
    }
    if (notifications_last_enabled.count) {
        Note *note = push(NOTE_INFO);
        if (notifications_last_enabled.count == 1)
            log_fmt(note->text, 0x65, fmt_str_str, str_enabled_in_notifications, notifications_last_enabled.name);
        else
            log_fmt(note->text, 0x65, str_s_u_mods, str_enabled_in_notifications, notifications_last_enabled.count);
        notifications_last_enabled.count = 0;
        n = active_notification_count;
    }
    if (notifications_last_disabled.count) {
        Note *e = push(NOTE_INFO);
        if (notifications_last_disabled.count == 1)
            log_fmt(e->text, 0x65, fmt_str_str, str_disabled, notifications_last_disabled.name);
        else
            log_fmt(e->text, 0x65, str_s_u_mods, str_disabled, notifications_last_disabled.count);
        notifications_last_disabled.count = 0;
        n = active_notification_count;
    }
    if (!n)
        return;
    unsigned f = hook->flags;
    hook->flags = f | 1;
    float dt = hook->dt;
    unsigned char animating = 0;
    for (unsigned long long i = notifications_head; n; i++, n--) {
        Note *e = &notification_queue[i % 10];
        float age = e->age + dt;
        e->age = age;
        animating |= (threshold_4_8 < (double)age) | ((double)age < threshold_0_2_in_notifications);
    }
    if (animating & 1)
        hook->flags = f | 3;
}

// draw: slide-in boxes, one per message, coloured by kind
extern "C" void notifications_draw_slide_boxes(DrawHook *hook) {
    if (!active_notification_count)
        return;
    if (notifications_only_while_gui_open == 1 && !((unsigned char *)ptr_to_gui_module_descriptor)[0x14])
        return;
    Ui *window = hook->ui;
    void *nvg = window->dl;
    nvg_FontSize(nvg, font_size_14_in_notifications);
    float y = notification_top_margin;
    const float x0 = notification_top_margin;
    for (unsigned long long i = 0; i < active_notification_count; i++) {
        Note *note = &notification_queue[(notifications_head + i) % 10];
        double age = note->age;
        float a = alpha_1_in_notifications;
        if (divisor_0_2_in_notifications_a > age)
            a = ease_curve((float)(age / divisor_0_2_in_notifications_a));
        else if (age > threshold_4_8_in_notifications)
            a = ease_curve((float)((offset_5_in_notifications - age) / divisor_0_2_in_notifications_a));
        float alpha = alpha_1_in_notifications < a ? alpha_1_in_notifications : a;
        nvg_GlobalAlpha(nvg, alpha);
        float x = (alpha_1_in_notifications - alpha) * factor_neg_100 + x0;
        void *d2 = window->dl;
        Color c;
        switch (note->kind) {
        case 2: c = nvg_rgba_f(colour_level_0_866667_in_notifications, colour_level_0_278431_in_notifications, colour_level_0_278431_in_notifications, 1.0f); break;
        case 1: { Color acc = get_speed_pair(); c = nvg_rgb_f(acc.r, acc.g, acc.b); break; }
        case 0: c = nvg_rgba_f(colour_level_0_54902, colour_level_0_92549, colour_level_0_168627, 1.0f); break;
        default: c = nvg_rgba_f(colour_level_0_866667_in_notifications, colour_level_0_866667_in_notifications, colour_level_0_866667_in_notifications, 1.0f); break;
        }
        const char *text = note->text;
        float w = nvg_text_bounds(d2, 0.0f, 0.0f, text, nullptr, nullptr) + offset_102;
        Rect box = make_rect(x, y, w, 42.0f);
        nvg_FillColor(d2, nvg_rgba_f(colour_level_0_0784314_in_notifications, colour_level_0_0784314_in_notifications, colour_level_0_0784314_in_notifications, 1.0f));
        draw_rect_rounded(window, box, corner_radius_20_in_notifications);
        Rect bar = make_rect(box.x + rect_extent_8_in_notifications, box.y + rect_extent_8_in_notifications, rect_extent_70, box.h + rect_extent_neg_16_in_notifications);
        nvg_FillColor(d2, nvg_trans_rgba_f(c, colour_alpha_0_15_in_notifications));
        draw_rect_rounded(window, bar, corner_radius_10_in_notifications);
        nvg_FillColor(d2, c);
        draw_text(window, bar, 1, 0, str_slinky_in_notifications);
        Rect body = make_rect(bar.x + bar.w + rect_extent_8_in_notifications, box.y, box.w - bar.w + rect_extent_neg_16_in_notifications, box.h);
        nvg_FillColor(d2, nvg_rgba_f(colour_level_0_866667_in_notifications, colour_level_0_866667_in_notifications, colour_level_0_866667_in_notifications, 1.0f));
        draw_text(window, body, 0, 0, text);
        y += alpha * factor_50_in_notifications;
    }
    nvg_GlobalAlpha(nvg, alpha_1_in_notifications);
}
