// Readable versions of the three protector-mutated cJSON functions of the image (upstream cJSON 1.7.x, MIT;
// the rest of cJSON is translated in modlib/cjson_*.cpp). The visible parts of the image's copies match
// upstream (literal checks with strncmp, ensure()/print_string_ptr() calls, strcmp lookups), so upstream is
// reproduced here, with parse_number/parse_array and print_number/print_array/print_object inlined into
// parse_value/print_value as the image does.
//   cjson_parse_value  parse_value        cjson_print_value  print_value        cjson_get_object_item_case_option  get_object_item
#include "slinky/cjson.h"

extern "C" cJSON_bool cjson_replace_item_in_object(cJSON *const item, parse_buffer *const input_buffer);   // parse_string
extern "C" cJSON_bool cjson_parse_array(cJSON *const item, parse_buffer *const input_buffer);   // parse_object
extern "C" unsigned char *cjson_print_string(printbuffer *const p, size_t needed);               // ensure
extern "C" cJSON_bool cjson_print_string_ptr(const unsigned char *const input, printbuffer *const output_buffer);   // print_string_ptr
extern "C" cJSON_bool cjson_parse_value(cJSON *const item, parse_buffer *const input_buffer);
extern "C" cJSON_bool cjson_print_value(const cJSON *const item, printbuffer *const output_buffer);

// C runtime
struct cjson_lconv { char *decimal_point; };
extern "C" cjson_lconv *localeconv(void);
extern "C" double strtod(const char *s, char **end);
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" int sscanf(const char *buf, const char *fmt, ...);
extern "C" int tolower(int c);

#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
static unsigned char get_decimal_point(void) { return (unsigned char)localeconv()->decimal_point[0]; }
static cJSON_bool compare_double(double left, double right) {
    double max_val = __builtin_fabs(left) > __builtin_fabs(right) ? __builtin_fabs(left) : __builtin_fabs(right);
    return __builtin_fabs(left - right) <= max_val * 2.220446049250313e-16;   // DBL_EPSILON
}
static parse_buffer *buffer_skip_whitespace(parse_buffer *const buffer) { return buffer_skip_whitespace_i(buffer); }

// ---------------------------------------------------------------- parsing
static cJSON_bool parse_number(cJSON *const item, parse_buffer *const input_buffer) {
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char number_c_string[64];
    unsigned char decimal_point = get_decimal_point();
    size_t i = 0;
    if ((input_buffer == NULL) || (input_buffer->content == NULL)) return false;
    // copy the number into a temporary buffer, replacing '.' with the locale's decimal point (strtod is locale aware)
    for (i = 0; (i < (sizeof(number_c_string) - 1)) && can_access_at_index(input_buffer, i); i++) {
        unsigned char c = buffer_at_offset(input_buffer)[i];
        if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == 'e' || c == 'E') number_c_string[i] = c;
        else if (c == '.') number_c_string[i] = decimal_point;
        else break;
    }
    number_c_string[i] = '\0';
    number = strtod((const char *)number_c_string, (char **)&after_end);
    if (number_c_string == after_end) return false;   // parse error
    item->valuedouble = number;
    if (number >= INT_MAX) item->valueint = INT_MAX;
    else if (number <= (double)INT_MIN) item->valueint = INT_MIN;
    else item->valueint = (int)number;
    item->type = cJSON_Number;
    input_buffer->offset += (size_t)(after_end - number_c_string);
    return true;
}

static cJSON_bool parse_array(cJSON *const item, parse_buffer *const input_buffer) {
    cJSON *head = NULL;   // head of the linked list
    cJSON *current_item = NULL;
    if (input_buffer->depth >= CJSON_NESTING_LIMIT) return false;   // too deeply nested
    input_buffer->depth++;
    if (buffer_at_offset(input_buffer)[0] != '[') goto fail;
    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ']')) goto success;   // empty
    if (cannot_access_at_index(input_buffer, 0)) {
        input_buffer->offset--;
        goto fail;
    }
    input_buffer->offset--;   // step back to the character in front of the first element
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
        buffer_skip_whitespace(input_buffer);
        if (!cjson_parse_value(current_item, input_buffer)) goto fail;
        buffer_skip_whitespace(input_buffer);
    } while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));
    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ']') goto fail;
success:
    input_buffer->depth--;
    if (head != NULL) head->prev = current_item;
    item->type = cJSON_Array;
    item->child = head;
    input_buffer->offset++;
    return true;
fail:
    if (head != NULL) cjson_delete(head);   // cJSON_Delete
    return false;
}

// parse_value: dispatch on the first character
extern "C" cJSON_bool cjson_parse_value(cJSON *const item, parse_buffer *const input_buffer) {
    if ((input_buffer == NULL) || (input_buffer->content == NULL)) return false;
    if (can_read(input_buffer, 4) && (strncmp((const char *)buffer_at_offset(input_buffer), "null", 4) == 0)) {
        item->type = cJSON_NULL;
        input_buffer->offset += 4;
        return true;
    }
    if (can_read(input_buffer, 5) && (strncmp((const char *)buffer_at_offset(input_buffer), "false", 5) == 0)) {
        item->type = cJSON_False;
        input_buffer->offset += 5;
        return true;
    }
    if (can_read(input_buffer, 4) && (strncmp((const char *)buffer_at_offset(input_buffer), "true", 4) == 0)) {
        item->type = cJSON_True;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    if (can_access_at_index(input_buffer, 0)) {
        unsigned char c = buffer_at_offset(input_buffer)[0];
        if (c == '\"') return cjson_replace_item_in_object(item, input_buffer);
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number(item, input_buffer);
        if (c == '[') return parse_array(item, input_buffer);
        if (c == '{') return cjson_parse_array(item, input_buffer);
    }
    return false;
}

// ---------------------------------------------------------------- printing
static cJSON_bool print_number(const cJSON *const item, printbuffer *const output_buffer) {
    unsigned char *output_pointer = NULL;
    double valuedouble = item->valuedouble;
    int length = 0;
    size_t i = 0;
    unsigned char number_buffer[26] = {0};   // temporary buffer to print the number into
    unsigned char decimal_point = get_decimal_point();
    double test = 0.0;
    if (output_buffer == NULL) return false;
    if (__builtin_isnan(valuedouble) || __builtin_isinf(valuedouble)) {
        length = sprintf((char *)number_buffer, "null");   // JSON has no NaN / Infinity
    } else if (valuedouble == (double)item->valueint) {
        length = sprintf((char *)number_buffer, "%d", item->valueint);
    } else {
        // 15 significant digits are usually enough; use 17 when that does not round-trip
        length = sprintf((char *)number_buffer, "%1.15g", valuedouble);
        if ((sscanf((char *)number_buffer, "%lg", &test) != 1) || !compare_double(test, valuedouble))
            length = sprintf((char *)number_buffer, "%1.17g", valuedouble);
    }
    if ((length < 0) || (length > (int)(sizeof(number_buffer) - 1))) return false;
    output_pointer = cjson_print_string(output_buffer, (size_t)length + sizeof(""));
    if (output_pointer == NULL) return false;
    // copy, writing the locale's decimal point as '.'
    for (i = 0; i < ((size_t)length); i++) {
        if (number_buffer[i] == decimal_point) {
            output_pointer[i] = '.';
            continue;
        }
        output_pointer[i] = number_buffer[i];
    }
    output_pointer[i] = '\0';
    output_buffer->offset += (size_t)length;
    return true;
}

static cJSON_bool print_array(const cJSON *const item, printbuffer *const output_buffer) {
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    cJSON *current_element = item->child;
    if (output_buffer == NULL) return false;
    output_pointer = cjson_print_string(output_buffer, 1);
    if (output_pointer == NULL) return false;
    *output_pointer = '[';
    output_buffer->offset++;
    output_buffer->depth++;
    while (current_element != NULL) {
        if (!cjson_print_value(current_element, output_buffer)) return false;
        update_offset(output_buffer);
        if (current_element->next) {
            length = (size_t)(output_buffer->format ? 2 : 1);
            output_pointer = cjson_print_string(output_buffer, length + 1);
            if (output_pointer == NULL) return false;
            *output_pointer++ = ',';
            if (output_buffer->format) *output_pointer++ = ' ';
            *output_pointer = '\0';
            output_buffer->offset += length;
        }
        current_element = current_element->next;
    }
    output_pointer = cjson_print_string(output_buffer, 2);
    if (output_pointer == NULL) return false;
    *output_pointer++ = ']';
    *output_pointer = '\0';
    output_buffer->depth--;
    return true;
}

static cJSON_bool print_object(const cJSON *const item, printbuffer *const output_buffer) {
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    cJSON *current_item = item->child;
    if (output_buffer == NULL) return false;
    length = (size_t)(output_buffer->format ? 2 : 1);   // "{" or "{\n"
    output_pointer = cjson_print_string(output_buffer, length + 1);
    if (output_pointer == NULL) return false;
    *output_pointer++ = '{';
    output_buffer->depth++;
    if (output_buffer->format) *output_pointer++ = '\n';
    output_buffer->offset += length;
    while (current_item) {
        if (output_buffer->format) {   // indent
            output_pointer = cjson_print_string(output_buffer, output_buffer->depth);
            if (output_pointer == NULL) return false;
            for (size_t i = 0; i < output_buffer->depth; i++) *output_pointer++ = '\t';
            output_buffer->offset += output_buffer->depth;
        }
        if (!cjson_print_string_ptr((unsigned char *)current_item->string, output_buffer)) return false;   // key
        update_offset(output_buffer);
        length = (size_t)(output_buffer->format ? 2 : 1);
        output_pointer = cjson_print_string(output_buffer, length);
        if (output_pointer == NULL) return false;
        *output_pointer++ = ':';
        if (output_buffer->format) *output_pointer++ = '\t';
        output_buffer->offset += length;
        if (!cjson_print_value(current_item, output_buffer)) return false;   // value
        update_offset(output_buffer);
        length = ((size_t)(output_buffer->format ? 1 : 0) + (size_t)(current_item->next ? 1 : 0));
        output_pointer = cjson_print_string(output_buffer, length + 1);
        if (output_pointer == NULL) return false;
        if (current_item->next) *output_pointer++ = ',';
        if (output_buffer->format) *output_pointer++ = '\n';
        *output_pointer = '\0';
        output_buffer->offset += length;
        current_item = current_item->next;
    }
    output_pointer = cjson_print_string(output_buffer, output_buffer->format ? (output_buffer->depth + 1) : 2);
    if (output_pointer == NULL) return false;
    if (output_buffer->format)
        for (size_t i = 0; i < (output_buffer->depth - 1); i++) *output_pointer++ = '\t';
    *output_pointer++ = '}';
    *output_pointer = '\0';
    output_buffer->depth--;
    return true;
}

// print_value: render an item into the print buffer
extern "C" cJSON_bool cjson_print_value(const cJSON *const item, printbuffer *const output_buffer) {
    unsigned char *output = NULL;
    if ((item == NULL) || (output_buffer == NULL)) return false;
    switch ((item->type) & 0xFF) {
    case cJSON_NULL:
        output = cjson_print_string(output_buffer, 5);
        if (output == NULL) return false;
        memcpy(output, "null", 5);
        return true;
    case cJSON_False:
        output = cjson_print_string(output_buffer, 6);
        if (output == NULL) return false;
        memcpy(output, "false", 6);
        return true;
    case cJSON_True:
        output = cjson_print_string(output_buffer, 5);
        if (output == NULL) return false;
        memcpy(output, "true", 5);
        return true;
    case cJSON_Number:
        return print_number(item, output_buffer);
    case cJSON_Raw: {
        size_t raw_length = 0;
        if (item->valuestring == NULL) return false;
        raw_length = strlen(item->valuestring) + sizeof("");
        output = cjson_print_string(output_buffer, raw_length);
        if (output == NULL) return false;
        memcpy(output, item->valuestring, raw_length);
        return true;
    }
    case cJSON_String:
        return cjson_print_string_ptr((unsigned char *)item->valuestring, output_buffer);
    case cJSON_Array:
        return print_array(item, output_buffer);
    case cJSON_Object:
        return print_object(item, output_buffer);
    default:
        return false;
    }
}

// ---------------------------------------------------------------- lookup
static int case_insensitive_strcmp(const unsigned char *string1, const unsigned char *string2) {
    if ((string1 == NULL) || (string2 == NULL)) return 1;
    if (string1 == string2) return 0;
    for (; tolower(*string1) == tolower(*string2); (void)string1++, string2++)
        if (*string1 == '\0') return 0;
    return tolower(*string1) - tolower(*string2);
}

// get_object_item: first member of object named `name`
extern "C" cJSON *cjson_get_object_item_case_option(const cJSON *const object, const char *const name, const cJSON_bool case_sensitive) {
    cJSON *current_element = NULL;
    if ((object == NULL) || (name == NULL)) return NULL;
    current_element = object->child;
    if (case_sensitive) {
        while ((current_element != NULL) && (current_element->string != NULL) &&
               (strcmp(name, current_element->string) != 0))
            current_element = current_element->next;
    } else {
        while ((current_element != NULL) &&
               (case_insensitive_strcmp((const unsigned char *)name, (const unsigned char *)(current_element->string)) != 0))
            current_element = current_element->next;
    }
    if ((current_element == NULL) || (current_element->string == NULL)) return NULL;
    return current_element;
}
