#define _POSIX_C_SOURCE 200809L
#include "../include/chad.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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

static void test_print_substring(void) {
    printf("\n-- printSubstring --\n");

    char text[] = "hello world";
    substring_t substr = substring(text, text + 5);

    FILE *tmp = tmpfile();
    int saved_stdout = dup(fileno(stdout));
    fflush(stdout);
    dup2(fileno(tmp), fileno(stdout));

    printSubstring(substr);

    fflush(stdout);
    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);

    rewind(tmp);
    char buf[32] = {0};
    fread(buf, 1, sizeof(buf) - 1, tmp);
    fclose(tmp);

    ASSERT_TRUE("printSubstring stops exactly at end, no over-read", strcmp(buf, "hello") == 0);
}

static void test_substring_trim_whitespace(void) {
    printf("\n-- substringTrimWhitespace --\n");

    char text1[] = "  hello  ";
    substring_t trimmed1 = substringTrimWhitespace(substring(text1, text1 + strlen(text1)));
    char *dup1 = strdupSubstring(trimmed1);
    ASSERT_TRUE("trims leading and trailing whitespace", dup1 && strcmp(dup1, "hello") == 0);
    free(dup1);

    char text2[] = "hello";
    substring_t trimmed2 = substringTrimWhitespace(substring(text2, text2 + strlen(text2)));
    char *dup2 = strdupSubstring(trimmed2);
    ASSERT_TRUE("no-whitespace input is unchanged", dup2 && strcmp(dup2, "hello") == 0);
    free(dup2);

    char text3[] = "   ";
    substring_t trimmed3 = substringTrimWhitespace(substring(text3, text3 + strlen(text3)));
    ASSERT_TRUE("all-whitespace input trims to zero-width", trimmed3.start == trimmed3.end);

    char text4[] = "  x";
    substring_t trimmed4 = substringTrimWhitespace(substring(text4, text4 + strlen(text4)));
    char *dup4 = strdupSubstring(trimmed4);
    ASSERT_TRUE("leading-only whitespace trims correctly", dup4 && strcmp(dup4, "x") == 0);
    free(dup4);

    char text5[] = "x  ";
    substring_t trimmed5 = substringTrimWhitespace(substring(text5, text5 + strlen(text5)));
    char *dup5 = strdupSubstring(trimmed5);
    ASSERT_TRUE("trailing-only whitespace trims correctly", dup5 && strcmp(dup5, "x") == 0);
    free(dup5);
}

int test_format(void) {
    test_format_basic();
    test_format_macro();
    test_positional_insert();
    test_print_substring();
    test_substring_trim_whitespace();
    
    printf("\n");
    if (failed == 0) {
        printf("\033[32mAll %d format tests passed.\033[0m\n", passed);
    } else {
        printf("\033[31m%d/%d format tests FAILED.\033[0m\n", failed, passed + failed);
    }
    return failed == 0 ? 0 : 1;
}
