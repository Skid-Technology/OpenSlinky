// 0x51000: cJSON (upstream source) - parse_hex4, ensure, print_string_ptr, parse_array/object
#include "slinky/engine.h"
#include "slinky/cjson.h"

extern "C" unsigned cjson_parse_hex4(const unsigned char *const input) {   // parse_hex4
    unsigned int h = 0;
    size_t i = 0;
    for (i = 0; i < 4; i++) {
        if ((input[i] >= '0') && (input[i] <= '9')) h += (unsigned int)input[i] - '0';
        else if ((input[i] >= 'A') && (input[i] <= 'F')) h += (unsigned int)10 + input[i] - 'A';
        else if ((input[i] >= 'a') && (input[i] <= 'f')) h += (unsigned int)10 + input[i] - 'a';
        else return 0;
        if (i < 3) h = h << 4;
    }
    return h;
}

extern "C" unsigned char *cjson_print_string(printbuffer *const p, size_t needed) {   // ensure
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;
    if ((p == NULL) || (p->buffer == NULL)) return NULL;
    if ((p->length > 0) && (p->offset >= p->length)) return NULL;
    if (needed > INT_MAX) return NULL;
    needed += p->offset + 1;
    if (needed <= p->length) return p->buffer + p->offset;
    if (p->noalloc) return NULL;
    if (needed > (INT_MAX / 2)) {
        if (needed <= INT_MAX) newsize = INT_MAX;
        else return NULL;
    } else {
        newsize = needed * 2;
    }
    if (p->hooks.reallocate != NULL) {
        newbuffer = (unsigned char *)p->hooks.reallocate(p->buffer, newsize);
        if (newbuffer == NULL) {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;
            return NULL;
        }
    } else {
        newbuffer = (unsigned char *)p->hooks.allocate(newsize);
        if (!newbuffer) {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = NULL;
            return NULL;
        }
        memcpy(newbuffer, p->buffer, p->offset + 1);
        p->hooks.deallocate(p->buffer);
    }
    p->length = newsize;
    p->buffer = newbuffer;
    return newbuffer + p->offset;
}

extern "C" int libc_sprintf(char *buf, const char *fmt, ...);   // sprintf
extern "C" const char fmt_u_hex4[];                       // "u%04x"
// print_string_ptr (the output_buffer NULL check was folded away by the original compiler: all callers pass one)
extern "C" cJSON_bool cjson_print_string_ptr(const unsigned char *const input, printbuffer *const output_buffer) {
    const unsigned char *input_pointer = NULL;
    unsigned char *output = NULL;
    unsigned char *output_pointer = NULL;
    size_t output_length = 0;
    size_t escape_characters = 0;
    if (input == NULL) {
        output = cjson_print_string(output_buffer, sizeof("\"\""));
        if (output == NULL) return false;
        output[0] = '"'; output[1] = '"'; output[2] = '\0';
        return true;
    }
    for (input_pointer = input; *input_pointer; input_pointer++) {
        switch (*input_pointer) {
        case '\"': case '\\': case '\b': case '\f': case '\n': case '\r': case '\t':
            escape_characters++;
            break;
        default:
            if (*input_pointer < 32) escape_characters += 5;
            break;
        }
    }
    output_length = (size_t)(input_pointer - input) + escape_characters;
    output = cjson_print_string(output_buffer, output_length + sizeof("\"\""));
    if (output == NULL) return false;
    if (escape_characters == 0) {
        output[0] = '\"';
        memcpy(output + 1, input, output_length);
        output[output_length + 1] = '\"';
        output[output_length + 2] = '\0';
        return true;
    }
    output[0] = '\"';
    output_pointer = output + 1;
    for (input_pointer = input; *input_pointer != '\0'; (void)input_pointer++, output_pointer++) {
        if ((*input_pointer > 31) && (*input_pointer != '\"') && (*input_pointer != '\\')) {
            *output_pointer = *input_pointer;
        } else {
            *output_pointer++ = '\\';
            switch (*input_pointer) {
            case '\\': *output_pointer = '\\'; break;
            case '\"': *output_pointer = '\"'; break;
            case '\b': *output_pointer = 'b'; break;
            case '\f': *output_pointer = 'f'; break;
            case '\n': *output_pointer = 'n'; break;
            case '\r': *output_pointer = 'r'; break;
            case '\t': *output_pointer = 't'; break;
            default:
                libc_sprintf((char *)output_pointer, fmt_u_hex4, *input_pointer);
                output_pointer += 4;
                break;
            }
        }
    }
    output[output_length + 1] = '\"';
    output[output_length + 2] = '\0';
    return true;
}

FN(cJSON_bool, cjson_replace_item_in_object, (cJSON *const item, parse_buffer *const input_buffer));   // parse_string
FN(cJSON_bool, cjson_parse_value, (cJSON *const item, parse_buffer *const input_buffer));   // parse_value

extern "C" cJSON_bool cjson_parse_array(cJSON *const item, parse_buffer *const input_buffer) {   // parse_object
    cJSON *head = NULL;
    cJSON *current_item = NULL;
    if (input_buffer->depth >= CJSON_NESTING_LIMIT) return false;
    input_buffer->depth++;
    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '{')) goto fail;
    input_buffer->offset++;
    buffer_skip_whitespace_i(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '}')) goto success;
    if (cannot_access_at_index(input_buffer, 0)) {
        input_buffer->offset--;
        goto fail;
    }
    input_buffer->offset--;
    do {
        cJSON *new_item = cJSON_New_Item(&(input_buffer->hooks));
        if (new_item == NULL) goto fail;
        if (head == NULL) {
            current_item = head = new_item;
        } else {
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }
        input_buffer->offset++;
        buffer_skip_whitespace_i(input_buffer);
        if (!cjson_replace_item_in_object(current_item, input_buffer)) goto fail;
        buffer_skip_whitespace_i(input_buffer);
        current_item->string = current_item->valuestring;
        current_item->valuestring = NULL;
        if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != ':')) goto fail;
        input_buffer->offset++;
        buffer_skip_whitespace_i(input_buffer);
        if (!cjson_parse_value(current_item, input_buffer)) goto fail;
        buffer_skip_whitespace_i(input_buffer);
    } while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));
    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '}')) goto fail;
success:
    input_buffer->depth--;
    if (head != NULL) head->prev = current_item;
    item->type = cJSON_Object;
    item->child = head;
    input_buffer->offset++;
    return true;
fail:
    if (head != NULL) cjson_delete(head);
    return false;
}
