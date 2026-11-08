// 0x50000: cJSON (upstream source) - Duplicate, Is*, Compare
#include "slinky/engine.h"
#include "slinky/cjson.h"

extern "C" cJSON *cjson_duplicate(const cJSON *item, cJSON_bool recurse) {   // cJSON_Duplicate
    cJSON *newitem = NULL;
    cJSON *child = NULL;
    cJSON *next = NULL;
    cJSON *newchild = NULL;
    if (!item) goto fail;
    newitem = cJSON_New_Item(&global_hooks);
    if (!newitem) goto fail;
    newitem->type = item->type & (~cJSON_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring) {
        newitem->valuestring = (char *)cJSON_strdup((unsigned char *)item->valuestring, &global_hooks);
        if (!newitem->valuestring) goto fail;
    }
    if (item->string) {
        newitem->string = (item->type & cJSON_StringIsConst) ? item->string
                                                              : (char *)cJSON_strdup((unsigned char *)item->string, &global_hooks);
        if (!newitem->string) goto fail;
    }
    if (!recurse) return newitem;
    child = item->child;
    while (child != NULL) {
        newchild = cjson_duplicate(child, true);
        if (!newchild) goto fail;
        if (next != NULL) {
            next->next = newchild;
            newchild->prev = next;
            next = newchild;
        } else {
            newitem->child = newchild;
            next = newchild;
        }
        child = child->next;
    }
    if (newitem && newitem->child) newitem->child->prev = newchild;
    return newitem;
fail:
    if (newitem != NULL) cjson_delete(newitem);
    return NULL;
}

extern "C" cJSON_bool cjson_is_true(const cJSON *const item) {   // cJSON_IsTrue
    if (item == NULL) return false;
    return (item->type & 0xff) == cJSON_True;
}
extern "C" cJSON_bool cjson_is_bool(const cJSON *const item) {   // cJSON_IsBool
    if (item == NULL) return false;
    return (item->type & (cJSON_True | cJSON_False)) != 0;
}
extern "C" cJSON_bool cjson_is_array(const cJSON *const item) {   // cJSON_IsArray
    if (item == NULL) return false;
    return (item->type & 0xFF) == cJSON_Array;
}
extern "C" cJSON_bool cjson_is_object(const cJSON *const item) {   // cJSON_IsObject
    if (item == NULL) return false;
    return (item->type & 0xFF) == cJSON_Object;
}

typedef double v2d __attribute__((vector_size(16)));
extern "C" v2d cjson_duplicate_fabs_mask;    // fabs mask
extern "C" double double_epsilon; // DBL_EPSILON
static inline __attribute__((always_inline)) double cj_fabs(double x) {
    v2d v = {x, 0};
    v = (v2d)((__attribute__((vector_size(16))) long long)v & (__attribute__((vector_size(16))) long long)cjson_duplicate_fabs_mask);
    return v[0];
}
static inline __attribute__((always_inline)) cJSON_bool compare_double(double left, double right) {
    double maxVal = cj_fabs(left) > cj_fabs(right) ? cj_fabs(left) : cj_fabs(right);
    return (cj_fabs(left - right) <= maxVal * double_epsilon);
}
FN(cJSON *, cjson_get_object_item_case_option, (const cJSON *const object, const char *const name, const cJSON_bool case_sensitive));   // get_object_item

extern "C" cJSON_bool cjson_compare(const cJSON *const a, const cJSON *const b, const cJSON_bool case_sensitive) {   // cJSON_Compare
    if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF))) return false;
    switch (a->type & 0xFF) {
    case cJSON_False: case cJSON_True: case cJSON_NULL: case cJSON_Number: case cJSON_String: case cJSON_Raw:
    case cJSON_Array: case cJSON_Object:
        break;
    default:
        return false;
    }
    if (a == b) return true;
    switch (a->type & 0xFF) {
    case cJSON_False: case cJSON_True: case cJSON_NULL:
        return true;
    case cJSON_Number:
        if (compare_double(a->valuedouble, b->valuedouble)) return true;
        return false;
    case cJSON_String: case cJSON_Raw:
        if ((a->valuestring == NULL) || (b->valuestring == NULL)) return false;
        if (strcmp(a->valuestring, b->valuestring) == 0) return true;
        return false;
    case cJSON_Array: {
        cJSON *a_element = a->child;
        cJSON *b_element = b->child;
        for (; (a_element != NULL) && (b_element != NULL);) {
            if (!cjson_compare(a_element, b_element, case_sensitive)) return false;
            a_element = a_element->next;
            b_element = b_element->next;
        }
        if (a_element != b_element) return false;
        return true;
    }
    case cJSON_Object: {
        cJSON *a_element = NULL;
        cJSON *b_element = NULL;
        for (a_element = a->child; a_element != NULL; a_element = a_element->next) {
            b_element = cjson_get_object_item_case_option(b, a_element->string, case_sensitive);
            if (b_element == NULL) return false;
            if (!cjson_compare(a_element, b_element, case_sensitive)) return false;
        }
        for (b_element = b->child; b_element != NULL; b_element = b_element->next) {
            a_element = cjson_get_object_item_case_option(a, b_element->string, case_sensitive);
            if (a_element == NULL) return false;
            if (!cjson_compare(b_element, a_element, case_sensitive)) return false;
        }
        return true;
    }
    default:
        return false;
    }
}

FN(unsigned, cjson_parse_hex4, (const unsigned char *const input));   // parse_hex4
static inline __attribute__((always_inline)) unsigned char
utf16_literal_to_utf8(const unsigned char *const input_pointer, const unsigned char *const input_end, unsigned char **output_pointer) {
    long unsigned int codepoint = 0;
    unsigned int first_code = 0;
    const unsigned char *first_sequence = input_pointer;
    unsigned char utf8_length = 0;
    unsigned char utf8_position = 0;
    unsigned char sequence_length = 0;
    unsigned char first_byte_mark = 0;
    if ((input_end - first_sequence) < 6) goto fail;
    first_code = cjson_parse_hex4(first_sequence + 2);
    if (((first_code >= 0xDC00) && (first_code <= 0xDFFF))) goto fail;
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF)) {
        const unsigned char *second_sequence = first_sequence + 6;
        unsigned int second_code = 0;
        sequence_length = 12;
        if ((input_end - second_sequence) < 6) goto fail;
        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u')) goto fail;
        second_code = cjson_parse_hex4(second_sequence + 2);
        if ((second_code < 0xDC00) || (second_code > 0xDFFF)) goto fail;
        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    } else {
        sequence_length = 6;
        codepoint = first_code;
    }
    if (codepoint < 0x80) {
        utf8_length = 1;
    } else if (codepoint < 0x800) {
        utf8_length = 2;
        first_byte_mark = 0xC0;
    } else if (codepoint < 0x10000) {
        utf8_length = 3;
        first_byte_mark = 0xE0;
    } else if (codepoint <= 0x10FFFF) {
        utf8_length = 4;
        first_byte_mark = 0xF0;
    } else {
        goto fail;
    }
    for (utf8_position = (unsigned char)(utf8_length - 1); utf8_position > 0; utf8_position--) {
        (*output_pointer)[utf8_position] = (unsigned char)((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    if (utf8_length > 1) (*output_pointer)[0] = (unsigned char)((codepoint | first_byte_mark) & 0xFF);
    else (*output_pointer)[0] = (unsigned char)(codepoint & 0x7F);
    *output_pointer += utf8_length;
    return sequence_length;
fail:
    return 0;
}

extern "C" cJSON_bool cjson_replace_item_in_object(cJSON *const item, parse_buffer *const input_buffer) {   // parse_string
    const unsigned char *input_pointer = buffer_at_offset(input_buffer) + 1;
    const unsigned char *input_end = buffer_at_offset(input_buffer) + 1;
    unsigned char *output_pointer = NULL;
    unsigned char *output = NULL;
    if (buffer_at_offset(input_buffer)[0] != '\"') goto fail;
    {
        size_t allocation_length = 0;
        size_t skipped_bytes = 0;
        while (((size_t)(input_end - input_buffer->content) < input_buffer->length) && (*input_end != '\"')) {
            if (input_end[0] == '\\') {
                if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length) goto fail;
                skipped_bytes++;
                input_end++;
            }
            input_end++;
        }
        if (((size_t)(input_end - input_buffer->content) >= input_buffer->length) || (*input_end != '\"')) goto fail;
        allocation_length = (size_t)(input_end - buffer_at_offset(input_buffer)) - skipped_bytes;
        output = (unsigned char *)input_buffer->hooks.allocate(allocation_length + sizeof(""));
        if (output == NULL) goto fail;
    }
    output_pointer = output;
    while (input_pointer < input_end) {
        if (*input_pointer != '\\') {
            *output_pointer++ = *input_pointer++;
        } else {
            unsigned char sequence_length = 2;
            if ((input_end - input_pointer) < 1) goto fail;
            switch (input_pointer[1]) {
            case 'b': *output_pointer++ = '\b'; break;
            case 'f': *output_pointer++ = '\f'; break;
            case 'n': *output_pointer++ = '\n'; break;
            case 'r': *output_pointer++ = '\r'; break;
            case 't': *output_pointer++ = '\t'; break;
            case '\"': case '\\': case '/': *output_pointer++ = input_pointer[1]; break;
            case 'u':
                sequence_length = utf16_literal_to_utf8(input_pointer, input_end, &output_pointer);
                if (sequence_length == 0) goto fail;
                break;
            default:
                goto fail;
            }
            input_pointer += sequence_length;
        }
    }
    *output_pointer = '\0';
    item->type = cJSON_String;
    item->valuestring = (char *)output;
    input_buffer->offset = (size_t)(input_end - input_buffer->content);
    input_buffer->offset++;
    return true;
fail:
    if (output != NULL) input_buffer->hooks.deallocate(output);
    if (input_pointer != NULL) input_buffer->offset = (size_t)(input_pointer - input_buffer->content);
    return false;
}
