// cJSON internals (upstream cJSON 1.7.x, MIT) mapped onto the image's globals.
#pragma once
#include <stddef.h>
typedef int cJSON_bool;
#define false 0
#define true 1
#define cJSON_Invalid (0)
#define cJSON_False  (1 << 0)
#define cJSON_True   (1 << 1)
#define cJSON_NULL   (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array  (1 << 5)
#define cJSON_Object (1 << 6)
#define cJSON_Raw    (1 << 7)
#define cJSON_IsReference 256
#define cJSON_StringIsConst 512
typedef struct cJSON {
    struct cJSON *next, *prev, *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;
typedef struct internal_hooks {
    void *(*allocate)(size_t size);
    void (*deallocate)(void *pointer);
    void *(*reallocate)(void *pointer, size_t size);
} internal_hooks;
typedef struct { const unsigned char *json; size_t position; } error;
typedef struct {
    const unsigned char *content; size_t length; size_t offset; size_t depth; internal_hooks hooks;
} parse_buffer;
typedef struct {
    unsigned char *buffer; size_t length; size_t offset; size_t depth; cJSON_bool noalloc; cJSON_bool format;
    internal_hooks hooks;
} printbuffer;
extern "C" internal_hooks block_in_speed_setting_field_0x38;
extern "C" error cjson_state;
extern "C" double not_a_number;   // NAN
#define global_hooks block_in_speed_setting_field_0x38
#define global_error cjson_state
#define can_access_at_index(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)
#define cjson_min(a, b) (((a) < (b)) ? (a) : (b))
extern "C" __attribute__((pure)) size_t libc_strlen(const char *s);           // strlen
extern "C" __attribute__((pure)) int libc_strncmp(const char *a, const char *b, size_t n);   // strncmp
extern "C" __attribute__((pure)) int libc_strcmp(const char *a, const char *b);             // strcmp
extern "C" void *libc_memcpy_alt(void *d, const void *s, size_t n);        // memcpy
#define strlen libc_strlen
#define strncmp libc_strncmp
#define strcmp libc_strcmp
#define memcpy libc_memcpy_alt
#define memset __builtin_memset
static inline __attribute__((always_inline)) cJSON_bool cJSON_IsString(const cJSON *const item) {
    if (item == NULL) return false;
    return (item->type & 0xFF) == cJSON_String;
}
static inline __attribute__((always_inline)) cJSON_bool cJSON_IsNumber(const cJSON *const item) {
    if (item == NULL) return false;
    return (item->type & 0xFF) == cJSON_Number;
}
static inline __attribute__((always_inline)) cJSON *cJSON_New_Item(const internal_hooks *const hooks) {
    cJSON *node = (cJSON *)hooks->allocate(sizeof(cJSON));
    if (node) memset(node, '\0', sizeof(cJSON));
    return node;
}
static inline __attribute__((always_inline)) void update_offset(printbuffer *const buffer) {
    const unsigned char *buffer_pointer = NULL;
    if ((buffer == NULL) || (buffer->buffer == NULL)) return;
    buffer_pointer = buffer->buffer + buffer->offset;
    buffer->offset += strlen((const char *)buffer_pointer);
}

// ---- shared static helpers (inlined into every public function, as in the original) ----
#define INLINE static inline __attribute__((always_inline))
extern "C" double threshold_2_14748e09, threshold_neg_2_14748e09;   // INT_MAX, INT_MIN as doubles
#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)
extern "C" void cjson_delete(cJSON *item);           // cJSON_Delete
INLINE unsigned char *cJSON_strdup(const unsigned char *string, const internal_hooks *const hooks) {
    size_t length = 0;
    unsigned char *copy = NULL;
    if (string == NULL) return NULL;
    length = strlen((const char *)string) + sizeof("");
    copy = (unsigned char *)hooks->allocate(length);
    if (copy == NULL) return NULL;
    memcpy(copy, string, length);
    return copy;
}
INLINE void suffix_object(cJSON *prev, cJSON *item) { prev->next = item; item->prev = prev; }
INLINE cJSON_bool add_item_to_array(cJSON *array, cJSON *item) {
    cJSON *child = NULL;
    if ((item == NULL) || (array == NULL) || (array == item)) return false;
    child = array->child;
    if (child == NULL) {
        array->child = item;
        item->prev = item;
        item->next = NULL;
    } else {
        if (child->prev) {
            suffix_object(child->prev, item);
            child->prev = item;
        }
    }
    return true;
}
INLINE cJSON_bool add_item_to_object(cJSON *const object, const char *const string, cJSON *const item,
                                     const internal_hooks *const hooks, const cJSON_bool constant_key) {
    char *new_key = NULL;
    int new_type = cJSON_Invalid;
    if ((object == NULL) || (string == NULL) || (item == NULL) || (object == item)) return false;
    if (constant_key) {
        new_key = (char *)string;
        new_type = item->type | cJSON_StringIsConst;
    } else {
        new_key = (char *)cJSON_strdup((const unsigned char *)string, hooks);
        if (new_key == NULL) return false;
        new_type = item->type & ~cJSON_StringIsConst;
    }
    if (!(item->type & cJSON_StringIsConst) && (item->string != NULL)) hooks->deallocate(item->string);
    item->string = new_key;
    item->type = new_type;
    return add_item_to_array(object, item);
}
INLINE cJSON *cJSON_CreateBool(cJSON_bool boolean) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) item->type = boolean ? cJSON_True : cJSON_False;
    return item;
}
INLINE cJSON *cJSON_CreateNumber(double num) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;
        if (num >= threshold_2_14748e09) item->valueint = INT_MAX;
        else if (num <= threshold_neg_2_14748e09) item->valueint = INT_MIN;
        else item->valueint = (int)num;
    }
    return item;
}
INLINE cJSON *cJSON_CreateString(const char *string) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_String;
        item->valuestring = (char *)cJSON_strdup((const unsigned char *)string, &global_hooks);
        if (!item->valuestring) {
            cjson_delete(item);
            return NULL;
        }
    }
    return item;
}
INLINE cJSON *cJSON_CreateObject(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) item->type = cJSON_Object;
    return item;
}
INLINE cJSON *cJSON_CreateArray(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) item->type = cJSON_Array;
    return item;
}
#define CJSON_NESTING_LIMIT 1000
extern "C" void buffer_skip_whitespace_unused_return(parse_buffer *const buffer);   // buffer_skip_whitespace (void-returning copy)
INLINE parse_buffer *buffer_skip_whitespace_i(parse_buffer *const buffer) {
    if ((buffer == NULL) || (buffer->content == NULL)) return NULL;
    if (cannot_access_at_index(buffer, 0)) return buffer;
    while (can_access_at_index(buffer, 0) && (buffer_at_offset(buffer)[0] <= 32)) buffer->offset++;
    if (buffer->offset == buffer->length) buffer->offset--;
    return buffer;
}
