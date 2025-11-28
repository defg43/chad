// SPDX-License-Identifier: MIT
// Copyright (C) 2023-2025 defg43
// https://github.com/defg43/

#ifndef STR_H
#define STR_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "cstl.h"

typedef uint8_t byte;
typedef uint32_t utf32_t;

typedef struct {
    char first_char;
} dataSegmentOfString_t;

typedef struct {
    union {
        dataSegmentOfString_t *data;
        char *at;
        utf32_t *as_utf32;
    };
} string;

typedef struct {
    size_t allocated_bytes;
    size_t length;
    char data[];
} stringHeader_t;

typedef struct {
    size_t previous;
    size_t index;
    string str;
} iterstring_t;

typedef struct {
    string buffer;
    size_t capacity;
} stringBuilder_t;

// construction and destruction

#define coerce(_expr, type) ({                                              \
    auto expr = (_expr);                                                    \
    union {                                                                 \
        typeof(expr) i_have_this;                                           \
        type but_i_want_this;                                               \
    } converter = { .i_have_this = expr };                                  \
    converter.but_i_want_this;                                              \
})

/** Create string from C string or another string */
#define string(input)                                                       \
    ({                                                                      \
        auto _input = (input);                                              \
        static_assert( _Generic((_input),                                   \
            char *: true,                                                   \
            const char *: true,                                             \
            string: true,                                                   \
            default: false                                                  \
        ), "input must be char * or string");                               \
        _Generic((_input),                                                  \
            char *: stringFromCharPtr(coerce(_input, char *)),              \
            const char *: stringFromCharPtr(coerce(_input, char *)),        \
            string: stringFromString(coerce(_input, string))                \
        );                                                                  \
    })

string stringFromCharPtr(const char *cstr);
string stringFromString(string src);
void destroyString(string str);

// properties

stringHeader_t *getHeaderPointer(string str);
size_t stringlen(string str);
size_t stringbytesalloced(string str);

/** Get UTF-8 character count (slower than byte length) */
size_t stringUtf8Length(string str);

/** Validate UTF-8 encoding */
bool stringUtf8Validate(string str);

// comparisons

int stringcmp(string strA, string strB);
int stringncmp(string strA, string strB, size_t maxLen);
bool stringeql(string strA, string strB);
bool stringneql(string strA, string strB, size_t maxLen);
bool streql(char *strA, char *strB);
bool strneql(char *strA, char *strB, size_t maxLen);

/** Check if string matches at given index */
bool stringeqlidx(string haystack, size_t startIdx, string needle);

/** Check if string starts with prefix */
bool stringStartsWith(string str, string prefix);

/** Check if string ends with suffix */
bool stringEndsWith(string str, string suffix);

/** Case-insensitive comparison */
int stringcmpIgnoreCase(string strA, string strB);

// searching

/** Find first occurrence of substring, returns index or -1 */
int stringFind(string haystack, string needle);

/** Find first occurrence starting from offset */
int stringFindFrom(string haystack, string needle, size_t startIdx);

/** Find last occurrence of substring, returns index or -1 */
int stringFindLast(string haystack, string needle);

/** Count occurrences of substring */
size_t stringCount(string haystack, string needle);

// modification (new string returned, argument unchanged)

string stringSliceFromString(string str, size_t startIdx, size_t endIdx);
string stringSliceFromCharPtr(const char *cstr, size_t startIdx, size_t endIdx);
string stringConcat(string strA, string strB);
string stringReverse(string str);

/** Convert to uppercase (allocates new string) */
string stringToUpper(string str);

/** Convert to lowercase (allocates new string) */
string stringToLower(string str);

/** Remove whitespace from both ends (allocates new string) */
string stringTrim(string str);

/** Remove whitespace from start (allocates new string) */
string stringTrimLeft(string str);

/** Remove whitespace from end (allocates new string) */
string stringTrimRight(string str);

/** Replace all occurrences (allocates new string) */
string stringReplace(string str, string findStr, string replaceStr);

/** Replace first N occurrences (allocates new string) */
string stringReplaceN(string str, string findStr, string replaceStr, size_t maxReplacements);

// in-place modifications (original string is modified, string is returned for chaining)
string stringGrowBuffer(string str, size_t additionalBytes);
string stringAppendCharPtr(string str, const char *cstr);
string stringAppendString(string str, string other);
string stringAppendChar(string str, char ch);
string stringPrependCharPtr(string str, const char *cstr);
string stringPrependString(string str, string other);
string stringPrependChar(string str, char ch);

#define stringAppend(str, toAppend)                                         \
    _Generic((toAppend),                                                    \
        const char *: stringAppendCharPtr(str, coerce(toAppend, char *)),   \
        char *:       stringAppendCharPtr(str, coerce(toAppend, char *)),   \
        string:       stringAppendString(str, coerce(toAppend, string)),    \
        char:         stringAppendChar(str, coerce(toAppend, char))         \
    )

#define stringPrepend(str, toPrepend)                                       \
    _Generic((toPrepend),                                                   \
        const char *: stringPrependCharPtr(str, coerce(toPrepend, char *)), \
        char *:       stringPrependCharPtr(str, coerce(toPrepend, char *)), \
        string:       stringPrependString(str, coerce(toPrepend, string)),  \
        char:         stringPrependChar(str, coerce(toPrepend, char))       \
    )


// splitting and joining

/** Split string by delimiter (returns dynarray) */
dynarray(string) stringTokenize(char *inputCstr, char *delimCstr);

/** Split string by start/end delimiters (returns dynarray) */
dynarray(string) stringTokenizePairwise(char *inputCstr, char *startDelim, char *endDelim);

/** Split string object by delimiter (returns dynarray) */
dynarray(string) stringSplit(string str, string delimiter);

/** Join array of strings with separator */
string stringJoin(dynarray(string) parts, string separator);

// Legacy tokenize macro
#define tokenize(str, delim) stringTokenize(str, delim)

// formating 

/** Format string like sprintf (allocates new string) */
string stringFormat(const char *fmt, ...);

/** Format string with va_list */
string stringFormatVa(const char *fmt, va_list args);

// utf-8

/** Check binary prefix for UTF-8 parsing */
bool stringBinaryPrefix(unsigned char byteToCheck, unsigned char prefix, size_t bitDepth);

/** Get nth UTF-8 character (returns string slice) */
string stringUtf8At(string str, size_t charIndex);

/** Decode UTF-8 character at byte position */
utf32_t stringUtf8DecodeAt(string str, size_t byteIndex, size_t *bytesRead);

/** Encode UTF-32 codepoint to UTF-8 string */
string stringUtf8Encode(utf32_t codepoint);

// classification

bool stringIsOnlyAlphNum(const char *inputCstr);

/** Check if string contains only alphabetic characters */
bool stringIsAlpha(string str);

/** Check if string contains only digits */
bool stringIsDigit(string str);

/** Check if string contains only whitespace */
bool stringIsWhitespace(string str);

/** Check if string is empty */
static inline bool stringIsEmpty(string str) {
    return stringlen(str) == 0;
}

// iterstring

bool iterstringReset(iterstring_t *iter);
bool iterstringAdvance(iterstring_t *iter);

// some functions are missing here

// string builder stuff

stringBuilder_t stringBuilderCreate(size_t initialSize);
void stringBuilderAppend(stringBuilder_t *sb, string str);
void stringBuilderAppendCStr(stringBuilder_t *sb, const char *cstr);
void stringBuilderAppendChar(stringBuilder_t *sb, char c);
void stringBuilderAppendFormat(stringBuilder_t *sb, const char *fmt, ...);
string stringBuilderToString(stringBuilder_t *sb);
void stringBuilderClear(stringBuilder_t *sb);
void stringBuilderDestroy(stringBuilder_t *sb);
string stringRemoveDuplicates(string str);
string stringIntersect(string str1, string str2);
string stringUnion(string str1, string str2);
string stringEscapeC(string str);
string stringUnescapeC(string str);
string stringBase64Encode(string str);
string stringBase64Decode(string str);
bool stringMatchWildcard(string str, string pattern);
bool stringMatchGlob(string str, string pattern);
size_t stringLevenshteinDistance(string str1, string str2);
string stringToSnakeCase(string str);
string stringToKebabCase(string str);

#endif // STR_H
