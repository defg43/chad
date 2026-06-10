#include "../include/chad/ion.h"
#include "../include/chad/str.h"
#include <stdio.h>
#include <string.h>

static int passed = 0;
static int failed = 0;

#define ASSERT_TRUE(label, expr) do { \
    if (expr) { \
        printf("\033[32m✓\033[0m " label "\n"); \
        passed++; \
    } else { \
        printf("\033[31m✗\033[0m " label "\n"); \
        failed++; \
    } \
} while(0)

static void test_ion_basic_object(void) {
    printf("\n-- ION Basic Object --\n");
    
    object_t obj = createEmptyObject();
    string key = string("name");
    string val = string("chad");
    
    obj = insertStringEntry(obj, key, val);
    
    ASSERT_TRUE("object count is 1", obj.count == 1);
    ASSERT_TRUE("contains key 'name'", objcontains(obj, string("name")));
    
    obj_t_value_t retrieved = objget(obj, string("name"));
    ASSERT_TRUE("retrieved value is string", retrieved.discriminant == obj_t_string);
    ASSERT_TRUE("retrieved value matches", stringeql(retrieved.str, string("chad")));
    
    string json = objectToJson(obj);
    ASSERT_TRUE("json matches", strstr(json.at, "\"name\" : \"chad\"") != NULL);
    
    destroyString(json);
    destroyObject(obj);
}

static void test_ion_nested_object(void) {
    printf("\n-- ION Nested Object --\n");
    
    object_t inner = createEmptyObject();
    inner = insertStringEntry(inner, string("inner_key"), string("inner_val"));
    
    object_t outer = createEmptyObject();
    outer = insertSubobjectEntry(outer, string("outer_key"), inner);
    
    ASSERT_TRUE("outer count is 1", outer.count == 1);
    
    obj_t_value_t retrieved = objget(outer, string("outer_key"));
    ASSERT_TRUE("retrieved is subobject", retrieved.discriminant == obj_t_obj);
    ASSERT_TRUE("inner object count is 1", retrieved.obj.count == 1);
    
    string json = objectToJson(outer);
    ASSERT_TRUE("json contains inner key", strstr(json.at, "\"inner_key\"") != NULL);
    
    destroyString(json);
    destroyObject(outer);
}

static void test_ion_array(void) {
    printf("\n-- ION Array --\n");
    
    array_t arr = createEmptyArray();
    arr = insertIntoArray(arr, (obj_t_value_t){ .discriminant = obj_t_string, .str = string("first") });
    arr = insertIntoArray(arr, (obj_t_value_t){ .discriminant = obj_t_string, .str = string("second") });
    
    ASSERT_TRUE("array count is 2", arr.count == 2);
    
    string json = arrayToJson(arr);
    ASSERT_TRUE("array json matches", strcmp(json.at, "[\"first\", \"second\"]") == 0);
    
    destroyString(json);
    destroyArray(arr);
}

static void test_ion_json_parsing(void) {
    printf("\n-- ION JSON Parsing --\n");
    
    string json = string("{\"key\" : \"val\", \"num\" : 123}");
    object_t obj = jsonToObject(json);
    
    ASSERT_TRUE("parsed object count is 2", obj.count == 2);
    ASSERT_TRUE("contains 'key'", objcontains(obj, string("key")));
    ASSERT_TRUE("contains 'num'", objcontains(obj, string("num")));
    
    obj_t_value_t val = objget(obj, string("key"));
    ASSERT_TRUE("key is 'val'", stringeql(val.str, string("val")));
    
    destroyString(json);
    destroyObject(obj);
}

int test_ion(void) {
    test_ion_basic_object();
    test_ion_nested_object();
    test_ion_array();
    test_ion_json_parsing();
    
    printf("\n");
    if (failed == 0) {
        printf("\033[32mAll %d ION tests passed.\033[0m\n", passed);
    } else {
        printf("\033[31m%d/%d ION tests FAILED.\033[0m\n", failed, passed + failed);
    }
    return failed == 0 ? 0 : 1;
}
