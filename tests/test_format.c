#include "../include/chad.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

static void test_format_basic(void) {
    printf("\n-- Format Basic --\n");
    
    char *data[1][2] = {{"name", "chad"}};
    dictionary_t dict = createDictionary(1, data);
    
    char *result = replaceSubstrings(strdup("hello name"), dict);
    ASSERT_TRUE("replaceSubstrings basic", strcmp(result, "hello chad") == 0);
    free(result);
    
    dictionary_t tagged = convertKeysToTags(dict);
    ASSERT_TRUE("convertKeysToTags key", strcmp(tagged.key[0], "{name}") == 0);
    
    char *result2 = replaceSubstrings(strdup("hello {name}"), tagged);
    ASSERT_TRUE("replaceSubstrings tagged", strcmp(result2, "hello chad") == 0);
    free(result2);
    
    destroyDictionary(tagged);
}

static void test_format_macro(void) {
    printf("\n-- Format Macros --\n");
    
    int age = 25;
    char *name = "chad";
    
    char *formatted = format("hello {name}, you are {age} years old", name, age);
    ASSERT_TRUE("format macro result", strcmp(formatted, "hello chad, you are 25 years old") == 0);
    free(formatted);
}

static void test_positional_insert(void) {
    printf("\n-- Positional Insert --\n");
    
    char *data[2][2] = {{"0", "zero"}, {"1", "one"}};
    dictionary_t d = createDictionary(2, data);
    
    char *buf = strdup("val: {} and {}");
    buf = positionalInsert(buf, d);
    ASSERT_TRUE("positionalInsert empty braces", strcmp(buf, "val: zero and one") == 0);
    free(buf);
    
    char *buf2 = strdup("val: {1} and {0}");
    buf2 = positionalInsert(buf2, d);
    ASSERT_TRUE("positionalInsert numbered braces", strcmp(buf2, "val: one and zero") == 0);
    free(buf2);
    
    destroyDictionary(d);
}

int test_format(void) {
    test_format_basic();
    test_format_macro();
    test_positional_insert();
    
    printf("\n");
    if (failed == 0) {
        printf("\033[32mAll %d format tests passed.\033[0m\n", passed);
    } else {
        printf("\033[31m%d/%d format tests FAILED.\033[0m\n", failed, passed + failed);
    }
    return failed == 0 ? 0 : 1;
}
