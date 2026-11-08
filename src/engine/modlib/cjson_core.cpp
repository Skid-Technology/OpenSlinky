// 0x4d000: cJSON (upstream source, MIT) - core: delete, parse entry, print entry, accessors.
#include "slinky/engine.h"
#include "slinky/cjson.h"

extern "C" char cjson_core_buffer[];
FN(void *, libc_register_onexit_function, (void *jvm, void *b));
extern "C" void *cjson_strdup(void *x) { return libc_register_onexit_function(cjson_core_buffer, x); }

extern "C" char *cjson_get_string_value(const cJSON *item) {     // cJSON_GetStringValue
    if (!cJSON_IsString(item)) return NULL;
    return item->valuestring;
}
extern "C" cJSON_bool cjson_is_string(const cJSON *item) { return cJSON_IsString(item); }
extern "C" double cjson_get_number_value(const cJSON *item) {    // cJSON_GetNumberValue
    double d = not_a_number;
    if (cJSON_IsNumber(item)) d = item->valuedouble;
    return d;
}
extern "C" cJSON_bool cjson_is_number(const cJSON *item) { return cJSON_IsNumber(item); }

extern "C" void cjson_delete(cJSON *item) {            // cJSON_Delete
    cJSON *next = NULL;
    while (item != NULL) {
        next = item->next;
        if (!(item->type & cJSON_IsReference) && (item->child != NULL)) cjson_delete(item->child);
        if (!(item->type & cJSON_IsReference) && (item->valuestring != NULL)) global_hooks.deallocate(item->valuestring);
        if (!(item->type & cJSON_StringIsConst) && (item->string != NULL)) global_hooks.deallocate(item->string);
        global_hooks.deallocate(item);
        item = next;
    }
}
extern "C" void cjson_free_string(void *object) { global_hooks.deallocate(object); }   // cJSON_free

static parse_buffer *buffer_skip_whitespace(parse_buffer *const buffer) {
    if ((buffer == NULL) || (buffer->content == NULL)) return NULL;
    if (cannot_access_at_index(buffer, 0)) return buffer;
    while (can_access_at_index(buffer, 0) && (buffer_at_offset(buffer)[0] <= 32)) buffer->offset++;
    if (buffer->offset == buffer->length) buffer->offset--;
    return buffer;
}
// buffer_skip_whitespace with its unused return value dropped (the original compiler did this for the
// out-of-line copy).
extern "C" void buffer_skip_whitespace_unused_return(parse_buffer *const buffer) {
    if ((buffer == NULL) || (buffer->content == NULL)) return;
    if (cannot_access_at_index(buffer, 0)) return;
    while (can_access_at_index(buffer, 0) && (buffer_at_offset(buffer)[0] <= 32)) buffer->offset++;
    if (buffer->offset == buffer->length) buffer->offset--;
}

static parse_buffer *skip_utf8_bom(parse_buffer *const buffer) {
    if ((buffer == NULL) || (buffer->content == NULL) || (buffer->offset != 0)) return NULL;
    if (can_access_at_index(buffer, 4) && (__builtin_strncmp((const char *)buffer_at_offset(buffer), "\xEF\xBB\xBF", 3) == 0))
        buffer->offset += 3;
    return buffer;
}

FN(cJSON_bool, cjson_parse_value, (cJSON *const item, parse_buffer *const input_buffer));   // parse_value

extern "C" cJSON *cjson_parse_with_options(const char *value, size_t buffer_length, const char **return_parse_end,
                           cJSON_bool require_null_terminated) {                  // cJSON_ParseWithLengthOpts
    parse_buffer buffer = {0, 0, 0, 0, {0, 0, 0}};
    cJSON *item = NULL;
    global_error.json = NULL;
    global_error.position = 0;
    if (value == NULL || 0 == buffer_length) goto fail;
    buffer.content = (const unsigned char *)value;
    buffer.length = buffer_length;
    buffer.offset = 0;
    buffer.hooks = global_hooks;
    item = cJSON_New_Item(&global_hooks);
    if (item == NULL) goto fail;
    if (!cjson_parse_value(item, buffer_skip_whitespace(skip_utf8_bom(&buffer)))) goto fail;
    if (require_null_terminated) {
        buffer_skip_whitespace(&buffer);
        if ((buffer.offset >= buffer.length) || buffer_at_offset(&buffer)[0] != '\0') goto fail;
    }
    if (return_parse_end) *return_parse_end = (const char *)buffer_at_offset(&buffer);
    return item;
fail:
    if (item != NULL) cjson_delete(item);
    if (value != NULL) {
        error local_error;
        local_error.json = (const unsigned char *)value;
        local_error.position = 0;
        if (buffer.offset < buffer.length) local_error.position = buffer.offset;
        else if (buffer.length > 0) local_error.position = buffer.length - 1;
        if (return_parse_end != NULL) *return_parse_end = (const char *)local_error.json + local_error.position;
        global_error = local_error;
    }
    return NULL;
}
extern "C" cJSON *cjson_parse(const char *value) {    // cJSON_Parse
    size_t buffer_length;
    if (NULL == value) return NULL;
    buffer_length = strlen(value) + sizeof("");
    return cjson_parse_with_options(value, buffer_length, 0, 0);
}

FN(cJSON_bool, cjson_print_value, (const cJSON *const item, printbuffer *const output_buffer));   // print_value

extern "C" unsigned char *cjson_print(const cJSON *const item, cJSON_bool format) {     // print (global hooks)
    static const size_t default_buffer_size = 256;
    printbuffer buffer[1];
    unsigned char *printed = NULL;
    const internal_hooks *const hooks = &global_hooks;
    memset(buffer, 0, sizeof(buffer));
    buffer->buffer = (unsigned char *)hooks->allocate(default_buffer_size);
    buffer->length = default_buffer_size;
    buffer->format = format;
    buffer->hooks = *hooks;
    if (buffer->buffer == NULL) goto fail;
    if (!cjson_print_value(item, buffer)) goto fail;
    update_offset(buffer);
    if (hooks->reallocate != NULL) {
        printed = (unsigned char *)hooks->reallocate(buffer->buffer, buffer->offset + 1);
        if (printed == NULL) goto fail;
        buffer->buffer = NULL;
    } else {
        printed = (unsigned char *)hooks->allocate(buffer->offset + 1);
        if (printed == NULL) goto fail;
        memcpy(printed, buffer->buffer, cjson_min(buffer->length, buffer->offset + 1));
        printed[buffer->offset] = '\0';
        hooks->deallocate(buffer->buffer);
    }
    return printed;
fail:
    if (buffer->buffer != NULL) hooks->deallocate(buffer->buffer);
    return NULL;
}
extern "C" char *cjson_print_unformatted(const cJSON *item) { return (char *)cjson_print(item, false); }   // cJSON_PrintUnformatted
