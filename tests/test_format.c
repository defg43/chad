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
    
    char *input1 = strdup("hello name");
    char *result = replaceSubstrings(input1, dict);
    ASSERT_TRUE("replaceSubstrings basic", strcmp(result, "hello chad") == 0);
    if(result != input1) free(input1);
    free(result);
    
    dictionary_t tagged = convertKeysToTags(dict);
    ASSERT_TRUE("convertKeysToTags key", strcmp(tagged.key[0], "{name}") == 0);
    
    char *input2 = strdup("hello {name}");
    char *result2 = replaceSubstrings(input2, tagged);
    ASSERT_TRUE("replaceSubstrings tagged", strcmp(result2, "hello chad") == 0);
    if(result2 != input2) free(input2);
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

static void test_format_macro_no_substitution(void) {
    printf("\n-- Format Macro No Substitution (use-after-free regression) --\n");

    // previously: format()'s underlying implementation freed `temp` even when
    // replaceSubstrings returned that same pointer unchanged (no tags matched),
    // causing the returned string to be a dangling/freed pointer. this exercises
    // that path directly (ASan/UBSan should catch a regression here).
    int unused = 7;
    char *result = format("no placeholders here", unused);
    ASSERT_TRUE("format with zero matching tags doesn't UAF",
        strcmp(result, "no placeholders here") == 0);
    free(result);
}

static void test_format_macro_named_spec(void) {
    printf("\n-- Format Macro Named Format Spec --\n");

    char *name = "ab";
    char *padded = format("[{name:>6}]", name);
    ASSERT_TRUE("named tag right-align width via format() macro",
        strcmp(padded, "[    ab]") == 0);
    free(padded);

    char *centered = format("[{name:^6}]", name);
    ASSERT_TRUE("named tag center-align width via format() macro",
        strcmp(centered, "[  ab  ]") == 0);
    free(centered);

    char *custom_fill = format("[{name:*<6}]", name);
    ASSERT_TRUE("named tag custom fill via format() macro",
        strcmp(custom_fill, "[ab****]") == 0);
    free(custom_fill);
}

static void test_format_macro_numeric_spec(void) {
    printf("\n-- Format Macro Numeric Type Specs --\n");

    int value = 255;
    char *hex = format("{value:x}", value);
    ASSERT_TRUE("named tag hex conversion", strcmp(hex, "ff") == 0);
    free(hex);

    char *upper_hex = format("{value:X}", value);
    ASSERT_TRUE("named tag uppercase hex conversion", strcmp(upper_hex, "FF") == 0);
    free(upper_hex);

    char *octal = format("{value:o}", value);
    ASSERT_TRUE("named tag octal conversion", strcmp(octal, "377") == 0);
    free(octal);

    char *decimal = format("{value:d}", value);
    ASSERT_TRUE("named tag explicit decimal conversion", strcmp(decimal, "255") == 0);
    free(decimal);

    char *padded_hex = format("[{value:>8x}]", value);
    ASSERT_TRUE("named tag hex conversion with width", strcmp(padded_hex, "[      ff]") == 0);
    free(padded_hex);
}

static void test_format_macro_float_precision(void) {
    printf("\n-- Format Macro Float Precision --\n");

    float pi = 3.14159f;
    char *rounded = format("{pi:.2f}", pi);
    ASSERT_TRUE("named tag float precision", strcmp(rounded, "3.14") == 0);
    free(rounded);

    double e = 2.718281828;
    char *default_precision = format("{e:f}", e);
    ASSERT_TRUE("named tag float default precision is 6 digits",
        strcmp(default_precision, "2.718282") == 0);
    free(default_precision);

    char *padded_float = format("[{pi:>10.1f}]", pi);
    ASSERT_TRUE("named tag float precision with width", strcmp(padded_float, "[       3.1]") == 0);
    free(padded_float);
}

static void test_format_macro_string_precision_truncation(void) {
    printf("\n-- Format Macro String Precision Truncation --\n");

    char *word = "verbose";
    char *truncated = format("{word:.3}", word);
    ASSERT_TRUE("named tag string precision truncates value", strcmp(truncated, "ver") == 0);
    free(truncated);

    char *padded_truncated = format("[{word:>6.3}]", word);
    ASSERT_TRUE("named tag string precision truncation combined with width",
        strcmp(padded_truncated, "[   ver]") == 0);
    free(padded_truncated);
}

static void test_format_macro_escaped_braces(void) {
    printf("\n-- Format Macro Escaped Braces --\n");

    int age = 25;
    char *escaped = format("{{age}} is literally {age}", age);
    ASSERT_TRUE("format macro honors {{ }} escaping alongside named substitution",
        strcmp(escaped, "{age} is literally 25") == 0);
    free(escaped);
}

static void test_printh_macro(void) {
    printf("\n-- printh Macro --\n");

    char *name = "chad";
    FILE *tmp = tmpfile();
    int saved_stdout = dup(fileno(stdout));
    fflush(stdout);
    dup2(fileno(tmp), fileno(stdout));

    int written = printh("hi {name:>8}!", name);

    fflush(stdout);
    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);

    rewind(tmp);
    char buf[64] = {0};
    fread(buf, 1, sizeof(buf) - 1, tmp);
    fclose(tmp);

    ASSERT_TRUE("printh applies named format spec and prints to stdout",
        strcmp(buf, "hi     chad!") == 0);
    ASSERT_TRUE("printh returns the printed length", written == (int)strlen(buf));
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

static void test_positional_insert_out_of_bounds(void) {
    printf("\n-- Positional Insert Out Of Bounds --\n");

    char *data[1][2] = {{"0", "zero"}};
    dictionary_t d = createDictionary(1, data);

    // more "{}" placeholders than dictionary entries must not crash
    // (previously an out-of-bounds array read)
    char *buf = strdup("{} {} {}");
    buf = positionalInsert(buf, d);
    ASSERT_TRUE("positionalInsert leaves excess bare braces untouched",
        strcmp(buf, "zero {} {}") == 0);
    free(buf);

    // out-of-range numbered index leaves the tag untouched
    char *buf2 = strdup("{5}");
    buf2 = positionalInsert(buf2, d);
    ASSERT_TRUE("positionalInsert leaves out-of-range numbered tag untouched",
        strcmp(buf2, "{5}") == 0);
    free(buf2);

    destroyDictionary(d);
}

static void test_positional_insert_escaped_braces(void) {
    printf("\n-- Escaped Braces (protect/restore) --\n");

    // escaping is handled by protectEscapedBraces()/restoreEscapedBraces(),
    // not positionalInsert itself -- this keeps escaped braces safe from
    // being reinterpreted as real tags by later passes (see
    // test_format_macro_escaped_braces for the full-pipeline version).
    char *data[1][2] = {{"0", "zero"}};
    dictionary_t d = createDictionary(1, data);

    char *buf = strdup("{{literal}} {0} {{another}}");
    buf = protectEscapedBraces(buf);
    buf = positionalInsert(buf, d);
    buf = restoreEscapedBraces(buf);
    ASSERT_TRUE("protect+restore unescapes {{ }} and substitutes {0}",
        strcmp(buf, "{literal} zero {another}") == 0);
    free(buf);

    char *buf2 = strdup("{{}}");
    buf2 = protectEscapedBraces(buf2);
    buf2 = positionalInsert(buf2, d);
    buf2 = restoreEscapedBraces(buf2);
    ASSERT_TRUE("protect+restore collapses {{}} to {}",
        strcmp(buf2, "{}") == 0);
    free(buf2);

    destroyDictionary(d);
}

static void test_positional_insert_format_spec(void) {
    printf("\n-- Positional Insert Format Spec (width/align/fill) --\n");

    char *data[2][2] = {{"0", "ab"}, {"1", "42"}};
    dictionary_t d = createDictionary(2, data);

    char *left = strdup("[{0:<6}]");
    left = positionalInsert(left, d);
    ASSERT_TRUE("left align pads on the right", strcmp(left, "[ab    ]") == 0);
    free(left);

    char *right = strdup("[{0:>6}]");
    right = positionalInsert(right, d);
    ASSERT_TRUE("right align pads on the left", strcmp(right, "[    ab]") == 0);
    free(right);

    char *center = strdup("[{0:^6}]");
    center = positionalInsert(center, d);
    ASSERT_TRUE("center align pads both sides", strcmp(center, "[  ab  ]") == 0);
    free(center);

    char *fill = strdup("[{1:*>6}]");
    fill = positionalInsert(fill, d);
    ASSERT_TRUE("custom fill character is honored", strcmp(fill, "[****42]") == 0);
    free(fill);

    char *narrow = strdup("[{0:>1}]");
    narrow = positionalInsert(narrow, d);
    ASSERT_TRUE("width smaller than value leaves value untruncated", strcmp(narrow, "[ab]") == 0);
    free(narrow);

    destroyDictionary(d);
}

static void test_apply_padding(void) {
    printf("\n-- applyPadding --\n");

    char *left = applyPadding("hi", '<', ' ', 5);
    ASSERT_TRUE("applyPadding left align", strcmp(left, "hi   ") == 0);
    free(left);

    char *right = applyPadding("hi", '>', ' ', 5);
    ASSERT_TRUE("applyPadding right align", strcmp(right, "   hi") == 0);
    free(right);

    char *center = applyPadding("hi", '^', ' ', 6);
    ASSERT_TRUE("applyPadding center align", strcmp(center, "  hi  ") == 0);
    free(center);

    char *unchanged = applyPadding("hello", '>', ' ', 3);
    ASSERT_TRUE("applyPadding no-op when width <= length", strcmp(unchanged, "hello") == 0);
    free(unchanged);
}

static void test_apply_format_spec(void) {
    printf("\n-- applyFormatSpec --\n");

    char *plain = applyFormatSpec("hello", "", 0);
    ASSERT_TRUE("applyFormatSpec empty spec leaves value unchanged", strcmp(plain, "hello") == 0);
    free(plain);

    char *right_aligned = applyFormatSpec("ab", ">6", 2);
    ASSERT_TRUE("applyFormatSpec width+align only", strcmp(right_aligned, "    ab") == 0);
    free(right_aligned);

    char *hex = applyFormatSpec("255", "x", 1);
    ASSERT_TRUE("applyFormatSpec hex type conversion", strcmp(hex, "ff") == 0);
    free(hex);

    char *hex_upper = applyFormatSpec("255", "X", 1);
    ASSERT_TRUE("applyFormatSpec uppercase hex type conversion", strcmp(hex_upper, "FF") == 0);
    free(hex_upper);

    char *octal = applyFormatSpec("8", "o", 1);
    ASSERT_TRUE("applyFormatSpec octal type conversion", strcmp(octal, "10") == 0);
    free(octal);

    char *decimal = applyFormatSpec("42", "d", 1);
    ASSERT_TRUE("applyFormatSpec explicit decimal type conversion", strcmp(decimal, "42") == 0);
    free(decimal);

    char *float_default = applyFormatSpec("3.14159", "f", 1);
    ASSERT_TRUE("applyFormatSpec float type with default precision",
        strcmp(float_default, "3.141590") == 0);
    free(float_default);

    char *float_precision = applyFormatSpec("3.14159", ".2f", 3);
    ASSERT_TRUE("applyFormatSpec float type with explicit precision",
        strcmp(float_precision, "3.14") == 0);
    free(float_precision);

    char *string_precision = applyFormatSpec("verbose", ".3", 2);
    ASSERT_TRUE("applyFormatSpec precision without type truncates string",
        strcmp(string_precision, "ver") == 0);
    free(string_precision);

    char *combined = applyFormatSpec("verbose", ">6.3", 4);
    ASSERT_TRUE("applyFormatSpec truncation combined with width/align",
        strcmp(combined, "   ver") == 0);
    free(combined);

    char *null_value = applyFormatSpec(NULL, ">4", 2);
    ASSERT_TRUE("applyFormatSpec handles NULL value gracefully",
        strcmp(null_value, "    ") == 0);
    free(null_value);
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
    test_format_macro_no_substitution();
    test_format_macro_named_spec();
    test_format_macro_numeric_spec();
    test_format_macro_float_precision();
    test_format_macro_string_precision_truncation();
    test_format_macro_escaped_braces();
    test_printh_macro();
    test_positional_insert();
    test_positional_insert_out_of_bounds();
    test_positional_insert_escaped_braces();
    test_positional_insert_format_spec();
    test_apply_padding();
    test_apply_format_spec();
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
