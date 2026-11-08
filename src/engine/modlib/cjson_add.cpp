// 0x4f000: cJSON (upstream source) - Add*ToObject, Create*
#include "slinky/engine.h"
#include "slinky/cjson.h"

extern "C" cJSON *cjson_add_string_to_object(cJSON *const object, const char *const name, const char *const string) {   // cJSON_AddStringToObject
    cJSON *string_item = cJSON_CreateString(string);
    if (add_item_to_object(object, name, string_item, &global_hooks, false)) return string_item;
    cjson_delete(string_item);
    return NULL;
}
extern "C" cJSON *cjson_create_string(const char *string) { return cJSON_CreateString(string); }   // cJSON_CreateString
extern "C" cJSON *cjson_add_bool_to_object(cJSON *const object, const char *const name) {   // cJSON_AddObjectToObject
    cJSON *object_item = cJSON_CreateObject();
    if (add_item_to_object(object, name, object_item, &global_hooks, false)) return object_item;
    cjson_delete(object_item);
    return NULL;
}
extern "C" cJSON *cjson_create_object(void) { return cJSON_CreateObject(); }   // cJSON_CreateObject
extern "C" cJSON *cjson_add_array_to_object(cJSON *const object, const char *const name) {   // cJSON_AddArrayToObject
    cJSON *array = cJSON_CreateArray();
    if (add_item_to_object(object, name, array, &global_hooks, false)) return array;
    cjson_delete(array);
    return NULL;
}
