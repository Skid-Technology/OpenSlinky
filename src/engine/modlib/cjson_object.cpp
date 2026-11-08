// 0x4e000: cJSON (upstream source) - object lookup, array/object building
#include "slinky/engine.h"
#include "slinky/cjson.h"

extern "C" cJSON *cjson_get_object_item(const cJSON *const object, const char *const string) {   // cJSON_GetObjectItemCaseSensitive
    cJSON *current_element = NULL;
    if ((object == NULL) || (string == NULL)) return NULL;
    current_element = object->child;
    while ((current_element != NULL) && (current_element->string != NULL) &&
           (strcmp(string, current_element->string) != 0))
        current_element = current_element->next;
    if ((current_element == NULL) || (current_element->string == NULL)) return NULL;
    return current_element;
}
extern "C" cJSON_bool cjson_add_item_to_array(cJSON *array, cJSON *item) { return add_item_to_array(array, item); }   // cJSON_AddItemToArray

extern "C" cJSON *cjson_add_item_to_object(cJSON *const object, const char *const name, const cJSON_bool boolean) {   // cJSON_AddBoolToObject
    cJSON *bool_item = cJSON_CreateBool(boolean);
    if (add_item_to_object(object, name, bool_item, &global_hooks, false)) return bool_item;
    cjson_delete(bool_item);
    return NULL;
}
extern "C" cJSON *cjson_add_number_to_object(cJSON *const object, const char *const name, const double number) {   // cJSON_AddNumberToObject
    cJSON *number_item = cJSON_CreateNumber(number);
    if (add_item_to_object(object, name, number_item, &global_hooks, false)) return number_item;
    cjson_delete(number_item);
    return NULL;
}
