#include "../include/chad/str.h"
#include "../include/chad/macros/foreach.h"
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

static bool str_ok(string s, const char *expected) {
    if (s.data == NULL) return expected == NULL;
    if (expected == NULL) return false;
    stringHeader_t *h = getHeaderPointer(s);
    bool val_matches  = strcmp(s.at, expected) == 0;
    bool len_matches  = h->length == strlen(expected);
    bool null_term    = s.at[h->length] == '\0';
    bool alloc_sane   = h->allocated_bytes >= sizeof(stringHeader_t) + h->length + 1;
    if (!val_matches)  printf("    content: got '%s' want '%s'\n", s.at, expected);
    if (!len_matches)  printf("    length:  got %zu want %zu\n", h->length, strlen(expected));
    if (!null_term)    printf("    not null-terminated at index %zu\n", h->length);
    if (!alloc_sane)   printf("    alloc_bytes %zu < header + len + 1 = %zu\n",
                               h->allocated_bytes,
                               sizeof(stringHeader_t) + h->length + 1);
    return val_matches && len_matches && null_term && alloc_sane;
}

static void test_from_charptr(void) {
    printf("\n-- stringFromCharPtr --\n");

    string s = stringFromCharPtr("hello");
    ASSERT_TRUE("basic value",          str_ok(s, "hello"));
    ASSERT_TRUE("length 5",             stringlen(s) == 5);
    destroyString(s);

    string empty = stringFromCharPtr("");
    ASSERT_TRUE("empty value",          str_ok(empty, ""));
    ASSERT_TRUE("empty length 0",       stringlen(empty) == 0);
    destroyString(empty);

    string one = stringFromCharPtr("x");
    ASSERT_TRUE("single char",          str_ok(one, "x"));
    destroyString(one);
}

static void test_from_string(void) {
    printf("\n-- stringFromString --\n");

    string orig = stringFromCharPtr("copy me");
    string copy = stringFromString(orig);
    
    ASSERT_TRUE("copy equals original",     str_ok(copy, "copy me"));
    ASSERT_TRUE("copy is independent alloc", copy.data != orig.data);
    
    copy.at[0] = 'X';
    ASSERT_TRUE("original unaffected",      orig.at[0] == 'c');
    
    destroyString(orig);
    destroyString(copy);
}

static void test_concat(void) {
    printf("\n-- stringConcat --\n");

    string a = stringFromCharPtr("foo");
    string b = stringFromCharPtr("bar");
    string c = stringConcat(a, b);
    ASSERT_TRUE("concat value",     str_ok(c, "foobar"));
    ASSERT_TRUE("concat length",    stringlen(c) == 6);
    destroyString(c);

    string e = stringFromCharPtr("");
    string d = stringConcat(a, e);
    ASSERT_TRUE("concat with empty", str_ok(d, "foo"));
    destroyString(d);

    string f = stringConcat(e, a);
    ASSERT_TRUE("empty concat with", str_ok(f, "foo"));
    destroyString(f);

    destroyString(a);
    destroyString(b);
    destroyString(e);
}

static void test_append(void) {
    printf("\n-- stringAppend* --\n");

    string s = stringFromCharPtr("ab");
    s = stringAppendChar(s, 'c');
    ASSERT_TRUE("appendChar value",  str_ok(s, "abc"));
    ASSERT_TRUE("appendChar length", stringlen(s) == 3);

    s = stringAppendCharPtr(s, "de");
    ASSERT_TRUE("appendCharPtr value",  str_ok(s, "abcde"));
    ASSERT_TRUE("appendCharPtr length", stringlen(s) == 5);

    string tail = stringFromCharPtr("fg");
    s = stringAppendString(s, tail);
    ASSERT_TRUE("appendString value",  str_ok(s, "abcdefg"));
    ASSERT_TRUE("appendString length", stringlen(s) == 7);
    destroyString(tail);

    destroyString(s);

    string e = stringFromCharPtr("");
    e = stringAppendChar(e, 'z');
    ASSERT_TRUE("append to empty", str_ok(e, "z"));
    destroyString(e);
}

static void test_append_many(void) {
    printf("\n-- stringAppend (many reallocs) --\n");

    string s = stringFromCharPtr("");
    for (int i = 0; i < 128; i++) {
        s = stringAppendChar(s, (char)('a' + (i % 26)));
    }
    ASSERT_TRUE("length after 128 appends", stringlen(s) == 128);
    ASSERT_TRUE("header consistent",
        getHeaderPointer(s)->length == 128 && s.at[128] == '\0');
    destroyString(s);
}

static void test_slice(void) {
    printf("\n-- stringSliceFromString --\n");

    string src = stringFromCharPtr("hello world");
    
    string h = stringSliceFromString(src, 0, 5);
    ASSERT_TRUE("slice [0,5]",  str_ok(h, "hello"));
    destroyString(h);

    string w = stringSliceFromString(src, 6, 11);
    ASSERT_TRUE("slice [6,11]", str_ok(w, "world"));
    destroyString(w);

    string empty = stringSliceFromString(src, 3, 3);
    ASSERT_TRUE("zero-width slice", str_ok(empty, ""));
    destroyString(empty);

    string clamped = stringSliceFromString(src, 8, 9999);
    ASSERT_TRUE("clamped end slice", str_ok(clamped, "rld"));
    destroyString(clamped);

    destroyString(src);
}

static void test_cmp(void) {
    printf("\n-- stringcmp / stringeql --\n");

    string a = stringFromCharPtr("abc");
    string b = stringFromCharPtr("abc");
    string c = stringFromCharPtr("abd");
    string e = stringFromCharPtr("");
    string e2 = stringFromCharPtr("");

    ASSERT_TRUE("equal strings",      stringeql(a, b));
    ASSERT_TRUE("unequal strings",   !stringeql(a, c));
    ASSERT_TRUE("empty == empty",     stringeql(e, e2));
    ASSERT_TRUE("empty != non-empty",!stringeql(a, e));
    ASSERT_TRUE("cmp returns <0",     stringcmp(a, c) < 0);
    ASSERT_TRUE("cmp returns >0",     stringcmp(c, a) > 0);
    ASSERT_TRUE("cmp equal is 0",     stringcmp(a, b) == 0);

    destroyString(a); destroyString(b); destroyString(c);
    destroyString(e); destroyString(e2);
}

static void test_find(void) {
    printf("\n-- stringFind --\n");

    string hay = stringFromCharPtr("hello world");
    string needle_w = stringFromCharPtr("world");
    string needle_h = stringFromCharPtr("hello");
    string needle_x = stringFromCharPtr("xyz");
    string empty    = stringFromCharPtr("");

    ASSERT_TRUE("find at start",    stringFind(hay, needle_h) == 0);
    ASSERT_TRUE("find in middle",   stringFind(hay, needle_w) == 6);
    ASSERT_TRUE("find missing",     stringFind(hay, needle_x) == -1);
    ASSERT_TRUE("find empty needle",stringFind(hay, empty)    == 0);

    destroyString(hay);
    destroyString(needle_w); destroyString(needle_h);
    destroyString(needle_x); destroyString(empty);
}

static void test_tokenize(void) {
    printf("\n-- stringTokenize --\n");

    dynarray(string) parts = stringTokenize("one;two;three", ";");
    ASSERT_TRUE("tokenize count",    parts.count == 3);
    ASSERT_TRUE("tokenize [0]",      str_ok(parts.at[0], "one"));
    ASSERT_TRUE("tokenize [1]",      str_ok(parts.at[1], "two"));
    ASSERT_TRUE("tokenize [2]",      str_ok(parts.at[2], "three"));

    foreach(string s of parts) {
    	destroyString(s);
	}
    destroy_dynarray(parts);

    dynarray(string) single = stringTokenize("hello", ";");
    ASSERT_TRUE("single token count", single.count == 1);
    ASSERT_TRUE("single token value", str_ok(single.at[0], "hello"));
    foreach(string s of single) {
    	destroyString(s);
    }
    destroy_dynarray(single);

    dynarray(string) none_parts = stringTokenize("", ";");

    foreach(string s of none_parts) {
    	destroyString(s);
    }
	
    destroy_dynarray(none_parts);
    ASSERT_TRUE("empty tokenize no crash", true);
}

static void test_replace(void) {
    printf("\n-- stringReplace --\n");

    string src = stringFromCharPtr("aabbcc");
    string find = stringFromCharPtr("bb");
    string rep  = stringFromCharPtr("XX");

    string result = stringReplace(src, find, rep);
    ASSERT_TRUE("replace value",  str_ok(result, "aaXXcc"));
    ASSERT_TRUE("replace length", stringlen(result) == 6);
    destroyString(result);

    string shorter = stringFromCharPtr("b");
    string result2 = stringReplace(src, find, shorter);
    ASSERT_TRUE("replace shorter", str_ok(result2, "aabc c") || str_ok(result2, "aabcc"));

    string expected2 = stringFromCharPtr("aabcc");
    ASSERT_TRUE("replace shorter correct", stringeql(result2, expected2));
    destroyString(expected2);
    destroyString(result2);
    destroyString(shorter);

    string nomatch = stringFromCharPtr("zz");
    string result3 = stringReplace(src, nomatch, rep);
    ASSERT_TRUE("replace no match", str_ok(result3, "aabbcc"));
    destroyString(result3);
    destroyString(nomatch);

    destroyString(src); destroyString(find); destroyString(rep);
}

static void test_trim(void) {
    printf("\n-- stringTrim --\n");

    string s = stringFromCharPtr("  hello  ");
    string t = stringTrim(s);
    ASSERT_TRUE("trim both sides", str_ok(t, "hello"));
    destroyString(t); destroyString(s);

    string l = stringFromCharPtr("  left");
    string lt = stringTrimLeft(l);
    ASSERT_TRUE("trim left",  str_ok(lt, "left"));
    destroyString(lt); destroyString(l);

    string r = stringFromCharPtr("right  ");
    string rt = stringTrimRight(r);
    ASSERT_TRUE("trim right", str_ok(rt, "right"));
    destroyString(rt); destroyString(r);

    string e = stringFromCharPtr("   ");
    string et = stringTrim(e);
    ASSERT_TRUE("trim all-space gives empty", stringlen(et) == 0);
    destroyString(et); destroyString(e);
}

static void test_grow_buffer(void) {
    printf("\n-- stringGrowBuffer (realloc ownership) --\n");

    string s = stringFromCharPtr("abc");
    size_t old_alloc = stringbytesalloced(s);

    s = stringGrowBuffer(s, 4096);
    ASSERT_TRUE("grow increases alloc",  stringbytesalloced(s) >= old_alloc + 4096);
    ASSERT_TRUE("grow preserves content",str_ok(s, "abc"));
    ASSERT_TRUE("grow preserves length", stringlen(s) == 3);

    destroyString(s);
}


int test_str(void) {
    test_from_charptr();
    test_from_string();
    test_concat();
    test_append();
    test_append_many();
    test_slice();
    test_cmp();
    test_find();
    test_tokenize();
    test_replace();
    test_trim();
    test_grow_buffer();

    printf("\n");
    if (failed == 0) {
        printf("\033[32mAll %d string tests passed.\033[0m\n", passed);
    } else {
        printf("\033[31m%d/%d string tests FAILED.\033[0m\n", failed, passed + failed);
    }
    return failed == 0 ? 0 : 1;
}
