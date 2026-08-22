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

static void test_reverse(void) {
    printf("\n-- stringReverse --\n");

    string s = stringFromCharPtr("abc");
    s = stringReverse(s);
    ASSERT_TRUE("reverse basic", str_ok(s, "cba"));
    destroyString(s);

    string s2 = stringFromCharPtr("racecar");
    s2 = stringReverse(s2);
    ASSERT_TRUE("reverse palindrome", str_ok(s2, "racecar"));
    destroyString(s2);
    
    // UTF-8 reverse test
    string u = stringFromCharPtr("こんにちは"); // "Hello" in Japanese
    u = stringReverse(u);
    // Reversed: "はちにんこ"
    ASSERT_TRUE("reverse utf-8", str_ok(u, "はちにんこ"));
    destroyString(u);
}

static void test_join(void) {
    printf("\n-- stringJoin --\n");

    dynarray(string) parts = create_dynarray(string);
    dynarray_append(parts, stringFromCharPtr("a"));
    dynarray_append(parts, stringFromCharPtr("b"));
    dynarray_append(parts, stringFromCharPtr("c"));

    string sep = stringFromCharPtr(",");
    string result = stringJoin(parts, sep);
    ASSERT_TRUE("join with comma", str_ok(result, "a,b,c"));
    
    foreach(string s of parts) destroyString(s);
    destroy_dynarray(parts);
    destroyString(sep);
    destroyString(result);
}

static void test_format(void) {
    printf("\n-- stringFormat --\n");

    string s = stringFormat("Hello %d %s", 123, "world");
    ASSERT_TRUE("format int and string", str_ok(s, "Hello 123 world"));
    destroyString(s);

    string s2 = stringFormat("%0.2f", 3.14159);
    ASSERT_TRUE("format float", str_ok(s2, "3.14"));
    destroyString(s2);
}

static void test_utf8(void) {
    printf("\n-- UTF-8 Support --\n");

    string s = stringFromCharPtr("hello");
    ASSERT_TRUE("utf8 validate ascii", stringUtf8Validate(s));
    ASSERT_TRUE("utf8 length ascii", stringUtf8Length(s) == 5);
    destroyString(s);

    string u = stringFromCharPtr("こんにち"); // 4 characters
    ASSERT_TRUE("utf8 validate japanese", stringUtf8Validate(u));
    ASSERT_TRUE("utf8 length japanese", stringUtf8Length(u) == 4);
    
    size_t bytes_read = 0;
    utf32_t cp = stringUtf8DecodeAt(u, 0, &bytes_read);
    ASSERT_TRUE("utf8 decode first char", bytes_read == 3);
    
    string first = stringUtf8At(u, 0);
    ASSERT_TRUE("utf8 at 0", str_ok(first, "こ"));
    destroyString(first);
    
    string third = stringUtf8At(u, 2);
    ASSERT_TRUE("utf8 at 2", str_ok(third, "に"));
    destroyString(third);

    destroyString(u);
}

static void test_base64(void) {
    printf("\n-- Base64 --\n");

    string s = string("hello world");
    string encoded = stringBase64Encode(s);
    ASSERT_TRUE("base64 encode", str_ok(encoded, "aGVsbG8gd29ybGQ="));

    string decoded = stringBase64Decode(encoded);
    ASSERT_TRUE("base64 decode round trip", str_ok(decoded, "hello world"));
    destroyString(decoded);
    destroyString(encoded);
    destroyString(s);

    string s2 = string("a");
    string encoded2 = stringBase64Encode(s2);
    ASSERT_TRUE("base64 encode 1 byte", str_ok(encoded2, "YQ=="));
    string decoded2 = stringBase64Decode(encoded2);
    ASSERT_TRUE("base64 decode 1 byte", str_ok(decoded2, "a"));
    destroyString(decoded2);
    destroyString(encoded2);
    destroyString(s2);

    string s3 = string("ab");
    string encoded3 = stringBase64Encode(s3);
    ASSERT_TRUE("base64 encode 2 bytes", str_ok(encoded3, "YWI="));
    string decoded3 = stringBase64Decode(encoded3);
    ASSERT_TRUE("base64 decode 2 bytes", str_ok(decoded3, "ab"));
    destroyString(decoded3);
    destroyString(encoded3);
    destroyString(s3);

    string empty = string("");
    string bad = stringBase64Decode(empty);
    ASSERT_TRUE("base64 decode empty input yields empty", str_ok(bad, ""));
    destroyString(bad);
    destroyString(empty);

    string malformed = string("not base64!");
    string bad2 = stringBase64Decode(malformed);
    ASSERT_TRUE("base64 decode malformed input yields empty", str_ok(bad2, ""));
    destroyString(bad2);
    destroyString(malformed);
}

static bool globMatchCStr(const char *s, const char *p) {
    string str = string((char *)s);
    string pattern = string((char *)p);
    bool result = stringMatchGlob(str, pattern);
    destroyString(str);
    destroyString(pattern);
    return result;
}

static void test_glob(void) {
    printf("\n-- Glob Matching --\n");

    ASSERT_TRUE("glob exact match", globMatchCStr("hello", "hello"));
    ASSERT_TRUE("glob star matches all", globMatchCStr("hello", "*"));
    ASSERT_TRUE("glob star prefix", globMatchCStr("hello.c", "*.c"));
    ASSERT_TRUE("glob star suffix mismatch", !globMatchCStr("hello.h", "*.c"));
    ASSERT_TRUE("glob question mark", globMatchCStr("cat", "c?t"));
    ASSERT_TRUE("glob question mark mismatch", !globMatchCStr("ct", "c?t"));
    ASSERT_TRUE("glob char class", globMatchCStr("cat", "[bc]at"));
    ASSERT_TRUE("glob char class mismatch", !globMatchCStr("hat", "[bc]at"));
    ASSERT_TRUE("glob char range", globMatchCStr("c3t", "c[0-9]t"));
    ASSERT_TRUE("glob negated char class", globMatchCStr("hat", "[!bc]at"));
    ASSERT_TRUE("glob negated char class mismatch", !globMatchCStr("bat", "[!bc]at"));
    ASSERT_TRUE("glob multiple stars", globMatchCStr("abcdef", "a*c*f"));
    ASSERT_TRUE("glob empty pattern needs empty string", globMatchCStr("", ""));
    ASSERT_TRUE("glob star matches empty", globMatchCStr("", "*"));
}

static void test_strip_chars(void) {
    printf("\n-- Strip Chars --\n");

    string s = string("###hello###");
    string stripped = stringStripChars(s, "#");
    ASSERT_TRUE("strip chars both sides", str_ok(stripped, "hello"));
    destroyString(stripped);

    string left = stringStripCharsLeft(s, "#");
    ASSERT_TRUE("strip chars left only", str_ok(left, "hello###"));
    destroyString(left);

    string right = stringStripCharsRight(s, "#");
    ASSERT_TRUE("strip chars right only", str_ok(right, "###hello"));
    destroyString(right);
    destroyString(s);

    string s2 = string("  ,,hi,,  ");
    string stripped2 = stringStripChars(s2, " ,");
    ASSERT_TRUE("strip chars multiple char set", str_ok(stripped2, "hi"));
    destroyString(stripped2);
    destroyString(s2);

    string plain = string("hello");
    string stripped3 = stringStripChars(plain, "#");
    ASSERT_TRUE("strip chars no matching chars leaves string unchanged", str_ok(stripped3, "hello"));
    destroyString(stripped3);
    destroyString(plain);

    string allStrip = string("###");
    string stripped4 = stringStripChars(allStrip, "#");
    ASSERT_TRUE("strip chars entirely stripped yields empty", str_ok(stripped4, ""));
    destroyString(stripped4);
    destroyString(allStrip);
}

static void test_json_escape(void) {
    printf("\n-- JSON Escape/Unescape --\n");

    string s1 = string("hello world");
    string e1 = stringEscapeJson(s1);
    ASSERT_TRUE("escape leaves plain text untouched", str_ok(e1, "hello world"));
    destroyString(e1);
    destroyString(s1);

    string s2 = string("a\"b\\c");
    string e2 = stringEscapeJson(s2);
    ASSERT_TRUE("escape handles quotes and backslashes", str_ok(e2, "a\\\"b\\\\c"));
    string u2 = stringUnescapeJson(e2);
    ASSERT_TRUE("unescape reverses quote/backslash escaping", str_ok(u2, "a\"b\\c"));
    destroyString(u2);
    destroyString(e2);
    destroyString(s2);

    string s3 = string("line1\nline2\ttab");
    string e3 = stringEscapeJson(s3);
    ASSERT_TRUE("escape handles newline and tab", str_ok(e3, "line1\\nline2\\ttab"));
    string u3 = stringUnescapeJson(e3);
    ASSERT_TRUE("unescape reverses newline/tab escaping", str_ok(u3, "line1\nline2\ttab"));
    destroyString(u3);
    destroyString(e3);
    destroyString(s3);

    string s4 = string("");
    string e4 = stringEscapeJson(s4);
    ASSERT_TRUE("escape of empty string is empty", str_ok(e4, ""));
    destroyString(e4);
    destroyString(s4);

    string e5 = string("caf\\u00e9");
    string u5 = stringUnescapeJson(e5);
    ASSERT_TRUE("unescape decodes \\u sequences below 0x80", str_ok(u5, "caf\xc3\xa9"));
    destroyString(u5);
    destroyString(e5);
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
    test_reverse();
    test_join();
    test_format();
    test_utf8();
    test_base64();
    test_glob();
    test_strip_chars();
    test_json_escape();

    printf("\n");
    if (failed == 0) {
        printf("\033[32mAll %d string tests passed.\033[0m\n", passed);
    } else {
        printf("\033[31m%d/%d string tests FAILED.\033[0m\n", failed, passed + failed);
    }
    return failed == 0 ? 0 : 1;
}
