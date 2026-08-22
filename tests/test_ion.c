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
    string name_key = string("name");
    ASSERT_TRUE("contains key 'name'", objcontains(obj, name_key));
    
    obj_t_value_t retrieved = objget(obj, name_key);
    destroyString(name_key);
    ASSERT_TRUE("retrieved value is string", retrieved.discriminant == obj_t_string);
    string chad_val = string("chad");
    ASSERT_TRUE("retrieved value matches", stringeql(retrieved.str, chad_val));
    destroyString(chad_val);
    
    string json = objectToJson(obj);
    ASSERT_TRUE("json matches", strstr(json.at, "\"name\" : \"chad\"") != NULL);
    
    destroyString(json);
    destroyObject(obj);
}

static void test_ion_nested_object(void) {
    printf("\n-- ION Nested Object --\n");
    
    object_t inner = createEmptyObject();
    string inner_key = string("inner_key");
    string inner_val = string("inner_val");
    inner = insertStringEntry(inner, inner_key, inner_val);
    
    object_t outer = createEmptyObject();
    string outer_key = string("outer_key");
    outer = insertSubobjectEntry(outer, outer_key, inner);
    
    ASSERT_TRUE("outer count is 1", outer.count == 1);
    
    obj_t_value_t retrieved = objget(outer, outer_key);
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
    string key_key = string("key");
    string num_key = string("num");
    ASSERT_TRUE("contains 'key'", objcontains(obj, key_key));
    ASSERT_TRUE("contains 'num'", objcontains(obj, num_key));
    destroyString(num_key);
    
    obj_t_value_t val = objget(obj, key_key);
    destroyString(key_key);
    string val_val = string("val");
    ASSERT_TRUE("key is 'val'", stringeql(val.str, val_val));
    destroyString(val_val);
    
    destroyString(json);
    destroyObject(obj);
}

static void test_ion_number_parsing(void) {
    printf("\n-- ION Number Parsing --\n");

    string num_key = string("num");

    string json1 = string("{\"num\" : 3.1}");
    object_t obj1 = jsonToObject(json1);
    obj_t_value_t val1 = objget(obj1, num_key);
    ASSERT_TRUE("3.1 parses as double", val1.num.number_discriminant == number_t_double);
    ASSERT_TRUE("3.1 parses to the correct value", val1.num.as_double > 3.09 && val1.num.as_double < 3.11);
    destroyString(json1);
    destroyObject(obj1);

    string json2 = string("{\"num\" : 3.10}");
    object_t obj2 = jsonToObject(json2);
    obj_t_value_t val2 = objget(obj2, num_key);
    ASSERT_TRUE("3.10 parses to the correct value", val2.num.as_double > 3.09 && val2.num.as_double < 3.11);
    destroyString(json2);
    destroyObject(obj2);

    string json3 = string("{\"num\" : 3.14}");
    object_t obj3 = jsonToObject(json3);
    obj_t_value_t val3 = objget(obj3, num_key);
    ASSERT_TRUE("3.14 parses to the correct value", val3.num.as_double > 3.13 && val3.num.as_double < 3.15);
    destroyString(json3);
    destroyObject(obj3);

    destroyString(num_key);
}

static void test_ion_valcmp(void) {
    printf("\n-- ION valcmp --\n");

    obj_t_value_t a = { .discriminant = obj_t_string, .str = string("same") };
    obj_t_value_t b = { .discriminant = obj_t_string, .str = string("same") };
    obj_t_value_t c = { .discriminant = obj_t_string, .str = string("different") };
    ASSERT_TRUE("equal strings compare equal", valcmp(a, b) == 0);
    ASSERT_TRUE("different strings do not compare equal", valcmp(a, c) != 0);
    destroyString(a.str);
    destroyString(b.str);
    destroyString(c.str);

    obj_t_value_t n1 = { .discriminant = obj_t_number, .num = { .number_discriminant = number_t_int64_t, .as_int64_t = 5 } };
    obj_t_value_t n2 = { .discriminant = obj_t_number, .num = { .number_discriminant = number_t_int64_t, .as_int64_t = 5 } };
    obj_t_value_t n3 = { .discriminant = obj_t_number, .num = { .number_discriminant = number_t_int64_t, .as_int64_t = 6 } };
    ASSERT_TRUE("equal numbers compare equal", valcmp(n1, n2) == 0);
    ASSERT_TRUE("different numbers do not compare equal", valcmp(n1, n3) != 0);

    ASSERT_TRUE("different discriminants do not compare equal", valcmp(a, n1) != 0);
}

static void test_ion_valncmp_valeql_valneql(void) {
    printf("\n-- ION valncmp/valeql/valneql --\n");

    obj_t_value_t a = { .discriminant = obj_t_string, .str = string("hello world") };
    obj_t_value_t b = { .discriminant = obj_t_string, .str = string("hello there") };
    ASSERT_TRUE("first 5 chars equal", valncmp(a, b, 5) == 0);
    ASSERT_TRUE("full strings not equal", valncmp(a, b, 100) != 0);
    ASSERT_TRUE("valeql true for identical values", valeql(a, a));
    ASSERT_TRUE("valeql false for different values", !valeql(a, b));
    ASSERT_TRUE("valneql true when first n chars equal", valneql(a, b, 5));
    ASSERT_TRUE("valneql false when full strings differ", !valneql(a, b, 100));
    destroyString(a.str);
    destroyString(b.str);
}

static void test_ion_objcopy_objremove(void) {
    printf("\n-- ION objcopy/objremove --\n");

    object_t obj = createEmptyObject();
    obj = insertStringEntry(obj, string("a"), string("1"));
    obj = insertNumberEntry(obj, string("b"), makeNumber((int64_t)2));

    object_t copy = objcopy(obj);
    ASSERT_TRUE("copy has same count", copy.count == obj.count);
    ASSERT_TRUE("copy is equal to original", objeql(copy, obj));

    string a_key = string("a");
    obj_t_value_t a_val = objget(copy, a_key);
    a_val.str.at[0] = 'x'; // mutate copy's string, should not affect original
    obj_t_value_t original_a = objget(obj, a_key);
    ASSERT_TRUE("copy is a deep copy (independent strings)", original_a.str.at[0] == '1');

    bool removed = objremove(&copy, a_key);
    ASSERT_TRUE("objremove reports success for existing key", removed);
    ASSERT_TRUE("objremove shrinks count", copy.count == 1);
    ASSERT_TRUE("objremove removed the right key", !objcontains(copy, a_key));
    string b_key = string("b");
    ASSERT_TRUE("objremove kept the other key", objcontains(copy, b_key));
    destroyString(b_key);

    bool removed_again = objremove(&copy, a_key);
    ASSERT_TRUE("objremove reports failure for missing key", !removed_again);

    destroyString(a_key);
    destroyObject(obj);
    destroyObject(copy);
}

static void test_ion_array_helpers(void) {
    printf("\n-- ION array helpers (cmp/eql/get/copy) --\n");

    array_t arr1 = createEmptyArray();
    arr1 = insertIntoArray(arr1, (obj_t_value_t){ .discriminant = obj_t_string, .str = string("a") });
    arr1 = insertIntoArray(arr1, (obj_t_value_t){ .discriminant = obj_t_string, .str = string("b") });

    array_t arr2 = createEmptyArray();
    arr2 = insertIntoArray(arr2, (obj_t_value_t){ .discriminant = obj_t_string, .str = string("a") });
    arr2 = insertIntoArray(arr2, (obj_t_value_t){ .discriminant = obj_t_string, .str = string("b") });

    array_t arr3 = createEmptyArray();
    arr3 = insertIntoArray(arr3, (obj_t_value_t){ .discriminant = obj_t_string, .str = string("a") });
    arr3 = insertIntoArray(arr3, (obj_t_value_t){ .discriminant = obj_t_string, .str = string("c") });

    ASSERT_TRUE("identical arrays compare equal", arraycmp(arr1, arr2) == 0);
    ASSERT_TRUE("differing arrays do not compare equal", arraycmp(arr1, arr3) != 0);
    ASSERT_TRUE("arrayeql true for identical arrays", arrayeql(arr1, arr2));
    ASSERT_TRUE("arrayeql false for differing arrays", !arrayeql(arr1, arr3));
    ASSERT_TRUE("arrayncmp with n=1 ignores second element diff", arrayncmp(arr1, arr3, 1) == 0);
    ASSERT_TRUE("arrayneql with n=1 reports equal (true)", arrayneql(arr1, arr3, 1));

    obj_t_value_t first = arrayget(arr1, 0);
    string a_key = string("a");
    ASSERT_TRUE("arrayget returns correct element", stringeql(first.str, a_key));
    destroyString(a_key);
    obj_t_value_t oob = arrayget(arr1, 99);
    ASSERT_TRUE("arrayget out-of-bounds returns null", oob.discriminant == obj_t_null);

    array_t copy = arraycopy(arr1);
    ASSERT_TRUE("arraycopy produces equal array", arrayeql(copy, arr1));
    copy.array[0].str.at[0] = 'z';
    ASSERT_TRUE("arraycopy is a deep copy", arr1.array[0].str.at[0] == 'a');

    destroyArray(arr1);
    destroyArray(arr2);
    destroyArray(arr3);
    destroyArray(copy);
}

static void test_ion_parse_string_escaped_backslash(void) {
    printf("\n-- ION parseString escaped-backslash-at-end bug --\n");

    // "hello\\" -- a literal trailing backslash, escaped, followed by the closing quote.
    // Must not be confused with an escaped closing quote (\").
    string json = string("\"hello\\\\\"");
    size_t pos = 0;
    obj_t_value_t result = {};
    bool ok = parseString(json, &pos, &result);
    ASSERT_TRUE("string ending in escaped backslash parses successfully", ok);
    if(ok) {
        ASSERT_TRUE("closing quote consumed, pos at end of input", pos == stringlen(json));
        destroyString(result.str);
    }
    destroyString(json);
}

static void test_ion_parse_object_key_leak_on_bad_value(void) {
    printf("\n-- ION parseObject key leak on invalid value (ASan-relevant) --\n");

    // valid key, but the value after ':' is malformed -- parseObject must free
    // the already-parsed key instead of leaking it on the syntax_error path.
    string json = string("{\"key\" : @@@}");
    size_t pos = 0;
    obj_t_value_t result = {};
    bool ok = parseObject(json, &pos, &result);
    ASSERT_TRUE("malformed value after valid key correctly fails to parse", !ok);
    destroyString(json);
}

static void test_ion_json_pretty_print(void) {
    printf("\n-- ION objectToJsonPretty/arrayToJsonPretty --\n");

    object_t obj = createEmptyObject();
    obj = insertStringEntry(obj, string("name"), string("chad"));

    array_t arr = createEmptyArray();
    arr = insertIntoArray(arr, (obj_t_value_t){ .discriminant = obj_t_number,
        .num = { .number_discriminant = number_t_int64_t, .as_int64_t = 1 } });
    arr = insertIntoArray(arr, (obj_t_value_t){ .discriminant = obj_t_number,
        .num = { .number_discriminant = number_t_int64_t, .as_int64_t = 2 } });
    obj = insertArrayEntry(obj, string("nums"), arr);

    string pretty = objectToJsonPretty(obj, 2);
    ASSERT_TRUE("pretty print contains newline", strstr(pretty.at, "\n") != NULL);
    ASSERT_TRUE("pretty print indents key with 2 spaces", strstr(pretty.at, "\n  \"name\"") != NULL);
    ASSERT_TRUE("pretty print contains nested array open bracket", strstr(pretty.at, "[\n") != NULL);
    ASSERT_TRUE("pretty print indents array elements with 4 spaces", strstr(pretty.at, "\n    1") != NULL);
    ASSERT_TRUE("pretty print closes with matching brace", strstr(pretty.at, "\n}") != NULL);
    destroyString(pretty);

    object_t empty = createEmptyObject();
    string empty_pretty = objectToJsonPretty(empty, 2);
    ASSERT_TRUE("empty object pretty prints as {}", strcmp(empty_pretty.at, "{}") == 0);
    destroyString(empty_pretty);
    destroyObject(empty);

    array_t empty_arr = createEmptyArray();
    string empty_arr_pretty = arrayToJsonPretty(empty_arr, 2);
    ASSERT_TRUE("empty array pretty prints as []", strcmp(empty_arr_pretty.at, "[]") == 0);
    destroyString(empty_arr_pretty);
    destroyArray(empty_arr);

    destroyObject(obj);
}

static void test_ion_objget_path(void) {
    printf("\n-- ION objgetPath --\n");

    object_t profile = createEmptyObject();
    profile = insertStringEntry(profile, string("name"), string("ada"));

    object_t user = createEmptyObject();
    user = insertSubobjectEntry(user, string("profile"), profile);

    object_t root = createEmptyObject();
    root = insertSubobjectEntry(root, string("user"), user);

    string path1 = string("user.profile.name");
    obj_t_value_t found = objgetPath(root, path1);
    destroyString(path1);
    ASSERT_TRUE("objgetPath finds deeply nested string", found.discriminant == obj_t_string);
    string ada = string("ada");
    ASSERT_TRUE("objgetPath returns correct value", stringeql(found.str, ada));
    destroyString(ada);

    string path2 = string("user.profile.missing");
    obj_t_value_t missing_leaf = objgetPath(root, path2);
    destroyString(path2);
    ASSERT_TRUE("objgetPath returns null for missing leaf", missing_leaf.discriminant == obj_t_null);

    string path3 = string("user.nope.name");
    obj_t_value_t missing_mid = objgetPath(root, path3);
    destroyString(path3);
    ASSERT_TRUE("objgetPath returns null for missing intermediate segment", missing_mid.discriminant == obj_t_null);

    string path4 = string("user.profile.name.extra");
    obj_t_value_t through_leaf = objgetPath(root, path4);
    destroyString(path4);
    ASSERT_TRUE("objgetPath returns null when indexing through a non-object", through_leaf.discriminant == obj_t_null);

    string path5 = string("user");
    obj_t_value_t single = objgetPath(root, path5);
    destroyString(path5);
    ASSERT_TRUE("objgetPath with single segment returns top-level subobject", single.discriminant == obj_t_obj);

    destroyObject(root);
}

static void test_ion_json_string_escaping(void) {
    printf("\n-- ION JSON String Escaping --\n");

    object_t obj = createEmptyObject();
    string key = string("msg");
    string value = string("line1\nline2\t\"quoted\"\\backslash");
    obj = insertStringEntry(obj, key, value);

    string json = objectToJson(obj);
    ASSERT_TRUE("serialized JSON contains escaped newline", strstr(json.at, "\\n") != NULL);
    ASSERT_TRUE("serialized JSON contains escaped quote", strstr(json.at, "\\\"") != NULL);
    ASSERT_TRUE("serialized JSON contains escaped backslash", strstr(json.at, "\\\\") != NULL);

    object_t roundtrip = jsonToObject(json);
    string msg_key = string("msg");
    obj_t_value_t roundtrip_val = objget(roundtrip, msg_key);
    string expected = string("line1\nline2\t\"quoted\"\\backslash");
    ASSERT_TRUE("round-tripped string matches original",
        stringeql(roundtrip_val.str, expected));
    destroyString(expected);
    destroyString(msg_key);

    destroyString(json);
    destroyObject(obj);
    destroyObject(roundtrip);
}

int test_ion(void) {
    test_ion_basic_object();
    test_ion_nested_object();
    test_ion_array();
    test_ion_json_parsing();
    test_ion_number_parsing();
    test_ion_valcmp();
    test_ion_valncmp_valeql_valneql();
    test_ion_objcopy_objremove();
    test_ion_array_helpers();
    test_ion_parse_string_escaped_backslash();
    test_ion_parse_object_key_leak_on_bad_value();
    test_ion_json_pretty_print();
    test_ion_objget_path();
    test_ion_json_string_escaping();
    
    printf("\n");
    if (failed == 0) {
        printf("\033[32mAll %d ION tests passed.\033[0m\n", passed);
    } else {
        printf("\033[31m%d/%d ION tests FAILED.\033[0m\n", failed, passed + failed);
    }
    return failed == 0 ? 0 : 1;
}
