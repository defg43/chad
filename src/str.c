// SPDX-License-Identifier: MIT
// Copyright (C) 2023-2025 defg43
// https://github.com/defg43/

#include "../include/chad/str.h"
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#ifndef container_of
#define container_of(ptr, type, member) ((type *)((size_t)ptr - offsetof(type, member)))
#endif

#ifndef containerof
#define containerof(ptr, type, member) container_of(ptr, type, member)
#endif

// eh

static char *my_strcpy(const char *src, char *dest) {
    if(src + (size_t)dest == src - (size_t)dest) return NULL;
    while((*dest++ = *src++));
    return dest;
}

static char *my_strncpy(char *dest, const char *src, size_t n) {
    char *orig = dest;
    if(src + (size_t)dest == src - (size_t)dest) return NULL;
    while(n-- && (*dest++ = *src++));
    return orig;
}

static size_t my_strlen(const char *str) {
    const char *orig_ptr = str;
    while(*str++);
    return (size_t)(str - orig_ptr - 1);
}

string stringFromCharPtr(const char *input) {
    size_t len = my_strlen(input);
    size_t to_allocate = sizeof(stringHeader_t) + len + 1; 
    stringHeader_t *result = malloc(to_allocate);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringFromCharPtr\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = to_allocate;
    result->length = len;
    my_strcpy(input, result->data);
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringFromString(string input) {
    stringHeader_t *header = containerof(input.data, stringHeader_t, data);
    #ifdef DEBUG
    if(header->allocated_bytes < header->length) {
        fprintf(stderr, 
            "string holds more characters than allocated, probably header corruption\n exiting\n");
        exit(EXIT_FAILURE);
    }
    if(header->length != my_strlen(header->data)) {
        fprintf(stderr, 
            "string length does not match header length, probably header corruption\n exiting\n");
        exit(EXIT_FAILURE);
    }
    #endif
    size_t to_alloctate = header->allocated_bytes;
    stringHeader_t *result = malloc(to_alloctate);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringFromString\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = to_alloctate;
    result->length = header->length;
    my_strncpy(result->data, header->data, header->length);
    result->data[header->length] = '\0';
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

void destroyString(string input) {
    free(containerof(input.data, stringHeader_t, data));
}

stringHeader_t *getHeaderPointer(string input) {
    return containerof(input.data, stringHeader_t, data);
}

size_t stringlen(string input) {
    return getHeaderPointer(input)->length;
}

size_t stringbytesalloced(string input) {
    return getHeaderPointer(input)->allocated_bytes;
}

int stringcmp(string a, string b) {
    stringHeader_t *a_header = getHeaderPointer(a);
    stringHeader_t *b_header = getHeaderPointer(b);
    if(a_header->length == 0 && b_header->length == 0) {
        return 0;
    }
    size_t min = a_header->length < b_header->length ? a_header->length : b_header->length;
    for(size_t i = 0; i < min; i++) {
        if(a_header->data[i] != b_header->data[i]) {
            return (int)(unsigned char)a_header->data[i] - (int)(unsigned char)b_header->data[i];
        }
    }
    return (int)a_header->length - (int)b_header->length;
}

int stringncmp(string a, string b, size_t n) {
    stringHeader_t *a_header = getHeaderPointer(a);
    stringHeader_t *b_header = getHeaderPointer(b);
    if(a_header->length == 0 && b_header->length == 0) {
        return 0;
    }
    size_t min = a_header->length < b_header->length ? a_header->length : b_header->length;
    if(min > n) min = n;
    for(size_t i = 0; i < min; i++) {
        if(a_header->data[i] != b_header->data[i]) {
            return (int)(unsigned char)a_header->data[i] - (int)(unsigned char)b_header->data[i];
        }
    }
    if(n >= a_header->length && n >= b_header->length) {
        return (int)a_header->length - (int)b_header->length;
    }
    return 0;
}

bool stringeql(string a, string b) {
    return stringcmp(a, b) == 0;
}

bool stringneql(string a, string b, size_t n) {
    return stringncmp(a, b, n) == 0;
}

bool streql(char *a, char *b) {
    size_t i = 0;
    while(a[i] != '\0' && b[i] != '\0') {
        if(a[i] == b[i]) {
            i++;
            continue;	
        }
        return false;
    }
    return a[i] == b[i];
}

bool strneql(char *a, char *b, size_t n) {
    for(size_t i = 0; i < n; i++) {
        if(a[i] == '\0' || b[i] == '\0' || a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

bool stringeqlidx(string haystack, size_t n, string needle) {
    if(n > stringlen(haystack)) {
        return false;
    }
    size_t remaining_haystack = stringlen(haystack) - n;
    if(remaining_haystack < stringlen(needle)) {
        return false;
    }
    for(size_t index = 0; index < stringlen(needle); index++) {
        if(haystack.at[n + index] != needle.at[index]) {
            return false;
        }
    }
    return true;
}

bool stringStartsWith(string str, string prefix) {
    return stringeqlidx(str, 0, prefix);
}

bool stringEndsWith(string str, string suffix) {
    size_t str_len = stringlen(str);
    size_t suffix_len = stringlen(suffix);
    if(suffix_len > str_len) {
        return false;
    }
    return stringeqlidx(str, str_len - suffix_len, suffix);
}

int stringcmpIgnoreCase(string a, string b) {
    stringHeader_t *a_header = getHeaderPointer(a);
    stringHeader_t *b_header = getHeaderPointer(b);
    size_t min = a_header->length < b_header->length ? a_header->length : b_header->length;
    for(size_t i = 0; i < min; i++) {
        int ca = tolower((unsigned char)a_header->data[i]);
        int cb = tolower((unsigned char)b_header->data[i]);
        if(ca != cb) {
            return ca - cb;
        }
    }
    return (int)a_header->length - (int)b_header->length;
}

int stringFind(string haystack, string needle) {
    return stringFindFrom(haystack, needle, 0);
}

int stringFindFrom(string haystack, string needle, size_t start) {
    size_t haystack_len = stringlen(haystack);
    size_t needle_len = stringlen(needle);
    if(needle_len == 0) return (int)start;
    if(needle_len > haystack_len || start > haystack_len - needle_len) return -1;
    
    for(size_t i = start; i <= haystack_len - needle_len; i++) {
        if(stringeqlidx(haystack, i, needle)) {
            return (int)i;
        }
    }
    return -1;
}

int stringFindLast(string haystack, string needle) {
    size_t haystack_len = stringlen(haystack);
    size_t needle_len = stringlen(needle);
    if(needle_len == 0) return (int)haystack_len;
    if(needle_len > haystack_len) return -1;
    
    for(size_t i = haystack_len - needle_len + 1; i > 0; i--) {
        if(stringeqlidx(haystack, i - 1, needle)) {
            return (int)(i - 1);
        }
    }
    return -1;
}

size_t stringCount(string haystack, string needle) {
    size_t count = 0;
    size_t needle_len = stringlen(needle);
    if(needle_len == 0) return 0;
    
    int pos = 0;
    while((pos = stringFindFrom(haystack, needle, pos)) != -1) {
        count++;
        pos += needle_len;
    }
    return count;
}


string stringConcat(string a, string b) {
    stringHeader_t *a_header = containerof(a.data, stringHeader_t, data);
    stringHeader_t *b_header = containerof(b.data, stringHeader_t, data);
    size_t to_alloctate = sizeof(stringHeader_t) + a_header->length + b_header->length + 1;
    stringHeader_t *result = malloc(to_alloctate);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in concat\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = to_alloctate;
    result->length = a_header->length + b_header->length;
    my_strncpy(result->data, a_header->data, a_header->length);
    my_strncpy(result->data + a_header->length, b_header->data, b_header->length);
    result->data[result->length] = '\0';
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringSliceFromString(string input, size_t start, size_t end) {
    bool needs_reversing = false;
    if(input.data == NULL) {
        return (string) { .data = NULL };
    }
    if(end > stringlen(input)) {
        if(start > stringlen(input)) {
            return (string) { .data = NULL };
        }
        end = stringlen(input);
    } else if(start > end) {
        needs_reversing = true;
        size_t temp = start;
        start = end;
        end = temp;
    }
    if(start > stringlen(input)) {
        if(!needs_reversing)   {
            return (string) { .data = NULL };
        } else {
            start = stringlen(input);
        }
    }
    size_t to_allocate = end - start;
    stringHeader_t *buf = malloc(sizeof(stringHeader_t) + to_allocate + 1);
    if(!buf) {
        fprintf(stderr, "failed to allocate memory in stringSliceFromString\n");
        exit(EXIT_FAILURE);
    }
    buf->allocated_bytes = sizeof(stringHeader_t) + to_allocate + 1;
    buf->length = to_allocate;
    for(size_t index = 0; index < to_allocate; index++) {
        buf->data[index] = input.at[start + index];
    }
    buf->data[to_allocate] = '\0';
    string ret = (string) { .data = (dataSegmentOfString_t *)buf->data };
    if(needs_reversing) {
        stringReverse(ret);
    }
    return ret;
}

string stringSliceFromCharPtr(const char *input, size_t start, size_t end) {
    size_t len = my_strlen(input);
    bool needs_reversing = false;
    if(input == NULL) {
        return (string) { .data = NULL };
    }
    if(end > len) {
        if(start > len) {
            return (string) { .data = NULL };
        }
        end = len;
    } else if(start > end) {
        needs_reversing = true;
        size_t temp = start;
        start = end;
        end = temp;
    }
    if(start > len) {
        if(!needs_reversing)   {
            return (string) { .data = NULL };
        } else {
            start = len;
        }
    }
    size_t to_allocate = end - start;
    stringHeader_t *buf = malloc(sizeof(stringHeader_t) + to_allocate + 1);
    if(!buf) {
        fprintf(stderr, "failed to allocate memory in stringSliceFromCharPtr\n");
        exit(EXIT_FAILURE);
    }
    buf->allocated_bytes = sizeof(stringHeader_t) + to_allocate + 1;
    buf->length = to_allocate;
    for(size_t index = 0; index < to_allocate; index++) {
        buf->data[index] = input[start + index];
    }
    buf->data[to_allocate] = '\0';
    string ret = (string) { .data = (dataSegmentOfString_t *)buf->data };
    if(needs_reversing) {
        stringReverse(ret);
    }
    return ret;
}

string stringToUpper(string str) {
    string result = stringFromString(str);
    for(size_t i = 0; i < stringlen(result); i++) {
        result.at[i] = toupper((unsigned char)result.at[i]);
    }
    return result;
}

string stringToLower(string str) {
    string result = stringFromString(str);
    for(size_t i = 0; i < stringlen(result); i++) {
        result.at[i] = tolower((unsigned char)result.at[i]);
    }
    return result;
}

string stringTrim(string str) {
    size_t len = stringlen(str);
    if(len == 0) return stringFromString(str);
    
    size_t start = 0;
    while(start < len && isspace((unsigned char)str.at[start])) {
        start++;
    }
    
    size_t end = len;
    while(end > start && isspace((unsigned char)str.at[end - 1])) {
        end--;
    }
    
    return stringSliceFromString(str, start, end);
}

string stringTrimLeft(string str) {
    size_t len = stringlen(str);
    size_t start = 0;
    while(start < len && isspace((unsigned char)str.at[start])) {
        start++;
    }
    return stringSliceFromString(str, start, len);
}

string stringTrimRight(string str) {
    size_t len = stringlen(str);
    size_t end = len;
    while(end > 0 && isspace((unsigned char)str.at[end - 1])) {
        end--;
    }
    return stringSliceFromString(str, 0, end);
}

string stringReplace(string str, string find, string replace) {
    return stringReplaceN(str, find, replace, (size_t)-1);
}

string stringReplaceN(string str, string find, string replace, size_t max) {
    if(stringlen(find) == 0) return stringFromString(str);
    
    size_t count = stringCount(str, find);
    if(count == 0 || max == 0) return stringFromString(str);
    if(count > max) count = max;
    
    size_t find_len = stringlen(find);
    size_t replace_len = stringlen(replace);
    size_t new_len = stringlen(str) + count * (replace_len - find_len);
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + new_len + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringReplaceN\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + new_len + 1;
    result->length = new_len;
    
    size_t src_pos = 0;
    size_t dst_pos = 0;
    size_t replacements = 0;
    
    while(src_pos < stringlen(str) && replacements < max) {
        if(stringeqlidx(str, src_pos, find)) {
            memcpy(result->data + dst_pos, replace.at, replace_len);
            dst_pos += replace_len;
            src_pos += find_len;
            replacements++;
        } else {
            result->data[dst_pos++] = str.at[src_pos++];
        }
    }
    
    while(src_pos < stringlen(str)) {
        result->data[dst_pos++] = str.at[src_pos++];
    }
    
    result->data[dst_pos] = '\0';
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

// utf-8 support

bool stringBinaryPrefix(unsigned char to_check, unsigned char prefix, size_t depth) {
    byte ones = 0b11111111;
    byte mask = ones << (8 - depth);
    byte result = to_check & mask;
    bool matches = result == prefix;
    #ifdef DEBUG
    printf("to_check: %08b\nprefix:   %08b\nmask:     %08b\nresult:   %08b\nmatches:  %s\n", 
        to_check, prefix, mask, result, matches ? "true" : "false");
    #endif
    return matches;
}

static void reverseUtf8_char(char *to_reverse) {
    size_t index = 0;
    while (to_reverse[index] != '\0') {
        if (stringBinaryPrefix(to_reverse[index], 0b00000000, 1)) {
            index++;
            continue;
        }
        if (stringBinaryPrefix(to_reverse[index], 0b11000000, 3) && to_reverse[index + 1] != '\0') {
            char temp = to_reverse[index];
            to_reverse[index] = to_reverse[index + 1];
            to_reverse[index + 1] = temp;
            index += 2;
            continue;
        }
        if (stringBinaryPrefix(to_reverse[index], 0b11100000, 4) && to_reverse[index + 2] != '\0') {
            char temp = to_reverse[index];
            to_reverse[index] = to_reverse[index + 2];
            to_reverse[index + 2] = temp;
            index += 3;
            continue;
        }
        if (stringBinaryPrefix(to_reverse[index], 0b11110000, 5) && to_reverse[index + 3] != '\0') {
            char temp = to_reverse[index];
            to_reverse[index] = to_reverse[index + 3];
            to_reverse[index + 3] = temp;
            temp = to_reverse[index + 1];
            to_reverse[index + 1] = to_reverse[index + 2];
            to_reverse[index + 2] = temp;
            index += 4;
            continue;
        }
        index++;
    }
}

string stringReverse(string input) {
    reverseUtf8_char(input.at);
    unsigned char temp;
    for(size_t i = 0, evil_i = stringlen(input); i < stringlen(input) / 2; i++, evil_i--) {
        temp = input.at[i];
        input.at[i] = input.at[evil_i - 1];
        input.at[evil_i - 1] = temp;
    }
    return input;
}

size_t stringUtf8Length(string str) {
    size_t count = 0;
    for(size_t i = 0; i < stringlen(str); ) {
        unsigned char c = str.at[i];
        if((c & 0x80) == 0) {
            i += 1;
        } else if((c & 0xE0) == 0xC0) {
            i += 2;
        } else if((c & 0xF0) == 0xE0) {
            i += 3;
        } else if((c & 0xF8) == 0xF0) {
            i += 4;
        } else {
            i += 1;
        }
        count++;
    }
    return count;
}

bool stringUtf8Validate(string str) {
    size_t len = stringlen(str);
    for(size_t i = 0; i < len; ) {
        unsigned char c = str.at[i];
        
        if((c & 0x80) == 0) {
            i += 1;
        } else if((c & 0xE0) == 0xC0) {
            if(i + 1 >= len || (str.at[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if((c & 0xF0) == 0xE0) {
            if(i + 2 >= len || (str.at[i + 1] & 0xC0) != 0x80 || (str.at[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if((c & 0xF8) == 0xF0) {
            if(i + 3 >= len || (str.at[i + 1] & 0xC0) != 0x80 || 
               (str.at[i + 2] & 0xC0) != 0x80 || (str.at[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

utf32_t stringUtf8DecodeAt(string str, size_t byte_index, size_t *bytes_read) {
    if(byte_index >= stringlen(str)) {
        if(bytes_read) *bytes_read = 0;
        return 0;
    }
    
    unsigned char c = str.at[byte_index];
    
    if((c & 0x80) == 0) {
        if(bytes_read) *bytes_read = 1;
        return c;
    } else if((c & 0xE0) == 0xC0) {
        if(byte_index + 1 >= stringlen(str)) {
            if(bytes_read) *bytes_read = 0;
            return 0;
        }
        if(bytes_read) *bytes_read = 2;
        return ((c & 0x1F) << 6) | (str.at[byte_index + 1] & 0x3F);
    } else if((c & 0xF0) == 0xE0) {
        if(byte_index + 2 >= stringlen(str)) {
            if(bytes_read) *bytes_read = 0;
            return 0;
        }
        if(bytes_read) *bytes_read = 3;
        return ((c & 0x0F) << 12) | ((str.at[byte_index + 1] & 0x3F) << 6) | (str.at[byte_index + 2] & 0x3F);
    } else if((c & 0xF8) == 0xF0) {
        if(byte_index + 3 >= stringlen(str)) {
            if(bytes_read) *bytes_read = 0;
            return 0;
        }
        if(bytes_read) *bytes_read = 4;
        return ((c & 0x07) << 18) | ((str.at[byte_index + 1] & 0x3F) << 12) | 
               ((str.at[byte_index + 2] & 0x3F) << 6) | (str.at[byte_index + 3] & 0x3F);
    }
    
    if(bytes_read) *bytes_read = 0;
    return 0;
}

string stringUtf8At(string str, size_t char_index) {
    size_t byte_pos = 0;
    size_t char_count = 0;
    
    while(byte_pos < stringlen(str) && char_count < char_index) {
        unsigned char c = str.at[byte_pos];
        if((c & 0x80) == 0) {
            byte_pos += 1;
        } else if((c & 0xE0) == 0xC0) {
            byte_pos += 2;
        } else if((c & 0xF0) == 0xE0) {
            byte_pos += 3;
        } else if((c & 0xF8) == 0xF0) {
            byte_pos += 4;
        } else {
            byte_pos += 1;
        }
        char_count++;
    }
    
    if(byte_pos >= stringlen(str)) {
        return (string) { .data = NULL };
    }
    
    size_t char_bytes = 1;
    unsigned char c = str.at[byte_pos];
    if((c & 0x80) == 0) {
        char_bytes = 1;
    } else if((c & 0xE0) == 0xC0) {
        char_bytes = 2;
    } else if((c & 0xF0) == 0xE0) {
        char_bytes = 3;
    } else if((c & 0xF8) == 0xF0) {
        char_bytes = 4;
    }
    
    return stringSliceFromString(str, byte_pos, byte_pos + char_bytes);
}

string stringUtf8Encode(utf32_t codepoint) {
    stringHeader_t *result;
    
    if(codepoint <= 0x7F) {
        result = malloc(sizeof(stringHeader_t) + 2);
        if(!result) {
            fprintf(stderr, "failed to allocate memory in stringUtf8Encode\n");
            exit(EXIT_FAILURE);
        }
        result->allocated_bytes = sizeof(stringHeader_t) + 2;
        result->length = 1;
        result->data[0] = (char)codepoint;
        result->data[1] = '\0';
    } else if(codepoint <= 0x7FF) {
        result = malloc(sizeof(stringHeader_t) + 3);
        if(!result) {
            fprintf(stderr, "failed to allocate memory in stringUtf8Encode\n");
            exit(EXIT_FAILURE);
        }
        result->allocated_bytes = sizeof(stringHeader_t) + 3;
        result->length = 2;
        result->data[0] = 0xC0 | (codepoint >> 6);
        result->data[1] = 0x80 | (codepoint & 0x3F);
        result->data[2] = '\0';
    } else if(codepoint <= 0xFFFF) {
        result = malloc(sizeof(stringHeader_t) + 4);
        if(!result) {
            fprintf(stderr, "failed to allocate memory in stringUtf8Encode\n");
            exit(EXIT_FAILURE);
        }
        result->allocated_bytes = sizeof(stringHeader_t) + 4;
        result->length = 3;
        result->data[0] = 0xE0 | (codepoint >> 12);
        result->data[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        result->data[2] = 0x80 | (codepoint & 0x3F);
        result->data[3] = '\0';
    } else if(codepoint <= 0x10FFFF) {
        result = malloc(sizeof(stringHeader_t) + 5);
        if(!result) {
            fprintf(stderr, "failed to allocate memory in stringUtf8Encode\n");
            exit(EXIT_FAILURE);
        }
        result->allocated_bytes = sizeof(stringHeader_t) + 5;
        result->length = 4;
        result->data[0] = 0xF0 | (codepoint >> 18);
        result->data[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        result->data[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        result->data[3] = 0x80 | (codepoint & 0x3F);
        result->data[4] = '\0';
    } else {
        result = malloc(sizeof(stringHeader_t) + 1);
        if(!result) {
            fprintf(stderr, "failed to allocate memory in stringUtf8Encode\n");
            exit(EXIT_FAILURE);
        }
        result->allocated_bytes = sizeof(stringHeader_t) + 1;
        result->length = 0;
        result->data[0] = '\0';
    }
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

dynarray(string) stringTokenize(char *input, char *delim) {
    if (!input || !delim) {
        return (dynarray(string)) { .at = NULL, .count = 0 };
    }
    size_t input_index = 0, delim_index = 0, marker = 0;
    bool found = false, delim_matches = true;
    string token = {};
    dynarray(string) ret = {};

    while (input[input_index]) {
        delim_matches = input[input_index] == delim[delim_index];
        found = !delim[delim_index];
        delim_index++;
        delim_index *= delim_matches;

        if (found) {
            token = stringSliceFromCharPtr(input, marker, input_index - my_strlen(delim));
            dynarray_append(ret, token);
            marker = input_index;
            delim_index = 0;
        }
        input_index++;
    }

    if (marker < input_index) {
        token = stringSliceFromCharPtr(input, marker, input_index);
        dynarray_append(ret, token);
    }

    return ret;
}

dynarray(string) stringTokenizePairwise(char *input, char *start_delimiter, char *end_delimiter) {
    if (!input || !start_delimiter || !end_delimiter) {
        return (dynarray(string)) { .at = NULL, .count = 0 };
    }
    size_t input_index = 0, start_delimiter_index = 0, end_delimiter_index = 0, token_start = 0;
    bool start_delimiter_matches = true, end_delimiter_matches = true, in_token = false;
    string token = {};
    dynarray(string) ret = {};
    
    while (input[input_index]) {
        if(!in_token) {
            start_delimiter_matches = input[input_index] == start_delimiter[start_delimiter_index];
            start_delimiter_index++;
            start_delimiter_index *= start_delimiter_matches;
            
            if (!start_delimiter[start_delimiter_index]) {
                token_start = input_index + 1;
                in_token = true;
                start_delimiter_index = 0;
            }
        } else {
            end_delimiter_matches = input[input_index] == end_delimiter[end_delimiter_index];
            end_delimiter_index++;
            end_delimiter_index *= end_delimiter_matches;
            
            if (!end_delimiter[end_delimiter_index]) {
                token = stringSliceFromCharPtr(input, token_start, input_index - my_strlen(end_delimiter) + 1);
                dynarray_append(ret, token);
                in_token = false;
                end_delimiter_index = 0;
            }
        }
        input_index++;
    }

    if (in_token && token_start < input_index) {
        token = stringSliceFromCharPtr(input, token_start, input_index);
        dynarray_append(ret, token);
    }

    return ret;
}

dynarray(string) stringSplit(string str, string delimiter) {
    dynarray(string) ret = {};
    if(stringlen(delimiter) == 0) {
        dynarray_append(ret, stringFromString(str));
        return ret;
    }
    
    size_t start = 0;
    int pos;
    while((pos = stringFindFrom(str, delimiter, start)) != -1) {
        string token = stringSliceFromString(str, start, pos);
        dynarray_append(ret, token);
        start = pos + stringlen(delimiter);
    }
    
    if(start <= stringlen(str)) {
        string token = stringSliceFromString(str, start, stringlen(str));
        dynarray_append(ret, token);
    }
    
    return ret;
}

string stringJoin(dynarray(string) parts, string separator) {
    if(parts.count == 0) {
        return string("");
    }
    
    size_t total_len = 0;
    for(size_t i = 0; i < parts.count; i++) {
        total_len += stringlen(parts.at[i]);
    }
    total_len += stringlen(separator) * (parts.count - 1);
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + total_len + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringJoin\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + total_len + 1;
    result->length = total_len;
    
    size_t pos = 0;
    for(size_t i = 0; i < parts.count; i++) {
        size_t part_len = stringlen(parts.at[i]);
        memcpy(result->data + pos, parts.at[i].at, part_len);
        pos += part_len;
        
        if(i < parts.count - 1) {
            size_t sep_len = stringlen(separator);
            memcpy(result->data + pos, separator.at, sep_len);
            pos += sep_len;
        }
    }
    
    result->data[pos] = '\0';
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringFormat(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    string result = stringFormatVa(fmt, args);
    va_end(args);
    return result;
}

string stringFormatVa(const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    
    if(len < 0) {
        fprintf(stderr, "vsnprintf failed in stringFormatVa\n");
        exit(EXIT_FAILURE);
    }
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + len + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringFormatVa\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + len + 1;
    result->length = len;
    
    vsnprintf(result->data, len + 1, fmt, args);
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

bool stringIsOnlyAlphNum(const char* input) {
    while (*input) {
        if (!isalnum((unsigned char)*input)) {
            return false;
        }
        input++;
    }
    return true;
}

bool stringIsAlpha(string str) {
    for(size_t i = 0; i < stringlen(str); i++) {
        if(!isalpha((unsigned char)str.at[i])) {
            return false;
        }
    }
    return stringlen(str) > 0;
}

bool stringIsDigit(string str) {
    for(size_t i = 0; i < stringlen(str); i++) {
        if(!isdigit((unsigned char)str.at[i])) {
            return false;
        }
    }
    return stringlen(str) > 0;
}

bool stringIsWhitespace(string str) {
    for(size_t i = 0; i < stringlen(str); i++) {
        if(!isspace((unsigned char)str.at[i])) {
            return false;
        }
    }
    return stringlen(str) > 0;
}

string stringGrowBuffer(string orig, size_t to_add) {
    if(orig.data == NULL) {
        orig = string("");
    }
    stringHeader_t *hdr = getHeaderPointer(orig);
    size_t old_bytes = stringbytesalloced(orig);
    stringHeader_t *new_hdr = realloc(hdr, old_bytes + to_add);
    if(new_hdr == NULL) {
        fprintf(stderr, "realloc failed in stringGrowBuffer\n");
        exit(EXIT_FAILURE);
    }
    new_hdr->allocated_bytes += to_add;
    new_hdr->data[new_hdr->length] = '\0';
    return (string) { .data = (dataSegmentOfString_t *)new_hdr->data };
}

string stringAppendCharPtr(string orig, const char *to_append) {
    size_t append_len = my_strlen(to_append);
    string ret = stringGrowBuffer(orig, append_len);
    my_strncpy(ret.at + stringlen(ret), to_append, append_len);
    getHeaderPointer(ret)->length += append_len;
    ret.at[stringlen(ret)] = '\0';
    assert(stringlen(ret) == my_strlen(ret.at));
    return ret;
}

string stringAppendString(string orig, string to_append) {
    string ret = stringGrowBuffer(orig, stringlen(to_append));
    my_strncpy(ret.at + stringlen(ret), to_append.at, stringlen(to_append));
    getHeaderPointer(ret)->length += stringlen(to_append);
    ret.at[stringlen(ret)] = '\0';
    assert(stringlen(ret) == my_strlen(ret.at));
    return ret;
}

string stringAppendChar(string orig, char c) {
    string ret = stringGrowBuffer(orig, 1);
    ret.at[stringlen(ret)] = c;
    getHeaderPointer(ret)->length++;
    ret.at[stringlen(ret)] = '\0';
    assert(stringlen(ret) == my_strlen(ret.at));
    return ret;
}

string stringPrependCharPtr(string orig, const char *to_prepend) {
    size_t prepend_len = my_strlen(to_prepend);
    size_t orig_len = stringlen(orig);
    string ret = stringGrowBuffer(orig, prepend_len);
    memmove(ret.at + prepend_len, ret.at, orig_len);
    memcpy(ret.at, to_prepend, prepend_len);
    getHeaderPointer(ret)->length += prepend_len;
    ret.at[stringlen(ret)] = '\0';
    assert(stringlen(ret) == my_strlen(ret.at));
    return ret;
}

string stringPrependString(string orig, string to_prepend) {
    size_t prepend_len = stringlen(to_prepend);
    size_t orig_len = stringlen(orig);
    string ret = stringGrowBuffer(orig, prepend_len);
    memmove(ret.at + prepend_len, ret.at, orig_len);
    memcpy(ret.at, to_prepend.at, prepend_len);
    getHeaderPointer(ret)->length += prepend_len;
    ret.at[stringlen(ret)] = '\0';
    assert(stringlen(ret) == my_strlen(ret.at));
    return ret;
}

string stringPrependChar(string orig, char c) {
    size_t orig_len = stringlen(orig);
    string ret = stringGrowBuffer(orig, 1);
    memmove(ret.at + 1, ret.at, orig_len);
    ret.at[0] = c;  
    getHeaderPointer(ret)->length++;
    ret.at[stringlen(ret)] = '\0';
    assert(stringlen(ret) == my_strlen(ret.at));
    return ret;
}

bool iterstringReset(iterstring_t *str) {
	str->index = str->previous;
	return true;	
}

bool iterstringAdvance(iterstring_t *str) {
	str->previous = str->index;
	return true;
}

// extra string operations

string stringRepeat(string str, size_t count) {
    if(count == 0) {
        return string("");
    }
    if(count == 1) {
        return stringFromString(str);
    }
    
    size_t str_len = stringlen(str);
    size_t total_len = str_len * count;
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + total_len + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringRepeat\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + total_len + 1;
    result->length = total_len;
    
    for(size_t i = 0; i < count; i++) {
        memcpy(result->data + (i * str_len), str.at, str_len);
    }
    result->data[total_len] = '\0';
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringPad(string str, size_t total_width, char pad_char, bool pad_left) {
    size_t str_len = stringlen(str);
    if(str_len >= total_width) {
        return stringFromString(str);
    }
    
    size_t pad_count = total_width - str_len;
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + total_width + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringPad\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + total_width + 1;
    result->length = total_width;
    
    if(pad_left) {
        memset(result->data, pad_char, pad_count);
        memcpy(result->data + pad_count, str.at, str_len);
    } else {
        memcpy(result->data, str.at, str_len);
        memset(result->data + str_len, pad_char, pad_count);
    }
    result->data[total_width] = '\0';
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringPadLeft(string str, size_t total_width, char pad_char) {
    return stringPad(str, total_width, pad_char, true);
}

string stringPadRight(string str, size_t total_width, char pad_char) {
    return stringPad(str, total_width, pad_char, false);
}

string stringCenter(string str, size_t total_width, char pad_char) {
    size_t str_len = stringlen(str);
    if(str_len >= total_width) {
        return stringFromString(str);
    }
    
    size_t total_padding = total_width - str_len;
    size_t left_padding = total_padding / 2;
    size_t right_padding = total_padding - left_padding;
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + total_width + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringCenter\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + total_width + 1;
    result->length = total_width;
    
    memset(result->data, pad_char, left_padding);
    memcpy(result->data + left_padding, str.at, str_len);
    memset(result->data + left_padding + str_len, pad_char, right_padding);
    result->data[total_width] = '\0';
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

bool stringContains(string haystack, string needle) {
    return stringFind(haystack, needle) != -1;
}

bool stringContainsChar(string str, char ch) {
    for(size_t i = 0; i < stringlen(str); i++) {
        if(str.at[i] == ch) {
            return true;
        }
    }
    return false;
}

int stringIndexOf(string haystack, char ch) {
    for(size_t i = 0; i < stringlen(haystack); i++) {
        if(haystack.at[i] == ch) {
            return (int)i;
        }
    }
    return -1;
}

int stringLastIndexOf(string haystack, char ch) {
    for(size_t i = stringlen(haystack); i > 0; i--) {
        if(haystack.at[i - 1] == ch) {
            return (int)(i - 1);
        }
    }
    return -1;
}

string stringSubstring(string str, size_t start, size_t length) {
    if(start >= stringlen(str)) {
        return string("");
    }
    size_t end = start + length;
    if(end > stringlen(str)) {
        end = stringlen(str);
    }
    return stringSliceFromString(str, start, end);
}

dynarray(string) stringSplitLines(string str) {
    dynarray(string) result = {};
    size_t line_start = 0;
    
    for(size_t i = 0; i < stringlen(str); i++) {
        if(str.at[i] == '\n') {
            string line = stringSliceFromString(str, line_start, i);
            dynarray_append(result, line);
            line_start = i + 1;
        } else if(str.at[i] == '\r') {
            if(i + 1 < stringlen(str) && str.at[i + 1] == '\n') {
                string line = stringSliceFromString(str, line_start, i);
                dynarray_append(result, line);
                line_start = i + 2;
                i++;
            } else {
                string line = stringSliceFromString(str, line_start, i);
                dynarray_append(result, line);
                line_start = i + 1;
            }
        }
    }
    
    if(line_start <= stringlen(str)) {
        string line = stringSliceFromString(str, line_start, stringlen(str));
        dynarray_append(result, line);
    }
    
    return result;
}

string stringReplaceChar(string str, char find, char replace) {
    string result = stringFromString(str);
    for(size_t i = 0; i < stringlen(result); i++) {
        if(result.at[i] == find) {
            result.at[i] = replace;
        }
    }
    return result;
}

string stringRemoveChar(string str, char to_remove) {
    size_t new_len = 0;
    for(size_t i = 0; i < stringlen(str); i++) {
        if(str.at[i] != to_remove) {
            new_len++;
        }
    }
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + new_len + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringRemoveChar\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + new_len + 1;
    result->length = new_len;
    
    size_t pos = 0;
    for(size_t i = 0; i < stringlen(str); i++) {
        if(str.at[i] != to_remove) {
            result->data[pos++] = str.at[i];
        }
    }
    result->data[new_len] = '\0';
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringTruncate(string str, size_t max_length, const char *suffix) {
    if(stringlen(str) <= max_length) {
        return stringFromString(str);
    }
    
    size_t suffix_len = suffix ? my_strlen(suffix) : 0;
    if(suffix_len >= max_length) {
        return stringSliceFromString(str, 0, max_length);
    }
    
    size_t truncate_at = max_length - suffix_len;
    string truncated = stringSliceFromString(str, 0, truncate_at);
    
    if(suffix) {
        string with_suffix = stringAppendCharPtr(truncated, suffix);
        return with_suffix;
    }
    
    return truncated;
}


bool stringToInt(string str, int *out) {
    if(stringlen(str) == 0) return false;
    
    char *endptr;
    long val = strtol(str.at, &endptr, 10);
    
    if(endptr == str.at || *endptr != '\0') {
        return false;
    }
    
    if(val > INT_MAX || val < INT_MIN) {
        return false;
    }
    
    if(out) *out = (int)val;
    return true;
}

bool stringToLong(string str, long *out) {
    if(stringlen(str) == 0) return false;
    
    char *endptr;
    long val = strtol(str.at, &endptr, 10);
    
    if(endptr == str.at || *endptr != '\0') {
        return false;
    }
    
    if(out) *out = val;
    return true;
}

bool stringToDouble(string str, double *out) {
    if(stringlen(str) == 0) return false;
    
    char *endptr;
    double val = strtod(str.at, &endptr);
    
    if(endptr == str.at || *endptr != '\0') {
        return false;
    }
    
    if(out) *out = val;
    return true;
}

bool stringToBool(string str, bool *out) {
    string lower = stringToLower(str);
    string trimmed = stringTrim(lower);
    
    bool result = false;
    bool valid = false;
    
    if(stringeql(trimmed, string("true")) || stringeql(trimmed, string("1")) || 
       stringeql(trimmed, string("yes")) || stringeql(trimmed, string("on"))) {
        result = true;
        valid = true;
    } else if(stringeql(trimmed, string("false")) || stringeql(trimmed, string("0")) || 
              stringeql(trimmed, string("no")) || stringeql(trimmed, string("off"))) {
        result = false;
        valid = true;
    }
    
    destroyString(lower);
    destroyString(trimmed);
    
    if(valid && out) *out = result;
    return valid;
}

string stringFromInt(int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return stringFromCharPtr(buffer);
}

string stringFromLong(long value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%ld", value);
    return stringFromCharPtr(buffer);
}

string stringFromDouble(double value, int precision) {
    char buffer[128];
    if(precision < 0) {
        snprintf(buffer, sizeof(buffer), "%f", value);
    } else {
        snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
    }
    return stringFromCharPtr(buffer);
}

string stringFromBool(bool value) {
    return value ? string("true") : string("false");
}

// =============================================================================
// STRING HASHING AND COMPARISON
// =============================================================================

uint32_t stringHash(string str) {
    // FNV-1a hash algorithm
    uint32_t hash = 2166136261u;
    for(size_t i = 0; i < stringlen(str); i++) {
        hash ^= (uint8_t)str.at[i];
        hash *= 16777619u;
    }
    return hash;
}

uint64_t stringHash64(string str) {
    // FNV-1a 64-bit hash algorithm
    uint64_t hash = 14695981039346656037ULL;
    for(size_t i = 0; i < stringlen(str); i++) {
        hash ^= (uint8_t)str.at[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

int stringCompareNatural(string a, string b) {
    size_t i = 0, j = 0;
    
    while(i < stringlen(a) && j < stringlen(b)) {
        if(isdigit((unsigned char)a.at[i]) && isdigit((unsigned char)b.at[j])) {
            // Extract numbers
            long num_a = 0, num_b = 0;
            while(i < stringlen(a) && isdigit((unsigned char)a.at[i])) {
                num_a = num_a * 10 + (a.at[i] - '0');
                i++;
            }
            while(j < stringlen(b) && isdigit((unsigned char)b.at[j])) {
                num_b = num_b * 10 + (b.at[j] - '0');
                j++;
            }
            if(num_a != num_b) {
                return num_a < num_b ? -1 : 1;
            }
        } else {
            if(a.at[i] != b.at[j]) {
                return (int)(unsigned char)a.at[i] - (int)(unsigned char)b.at[j];
            }
            i++;
            j++;
        }
    }
    
    return (int)stringlen(a) - (int)stringlen(b);
}

stringBuilder_t stringBuilderCreate(size_t initial_capacity) {
    if(initial_capacity == 0) initial_capacity = 256;
    
    stringHeader_t *buf = malloc(sizeof(stringHeader_t) + initial_capacity);
    if(!buf) {
        fprintf(stderr, "failed to allocate memory in stringBuilderCreate\n");
        exit(EXIT_FAILURE);
    }
    buf->allocated_bytes = sizeof(stringHeader_t) + initial_capacity;
    buf->length = 0;
    buf->data[0] = '\0';
    
    stringBuilder_t sb;
    sb.buffer = (string) { .data = (dataSegmentOfString_t *)buf->data };
    sb.capacity = initial_capacity;
    return sb;
}

void stringBuilderAppend(stringBuilder_t *sb, string str) {
    sb->buffer = stringAppendString(sb->buffer, str);
    sb->capacity = stringbytesalloced(sb->buffer) - sizeof(stringHeader_t);
}

void stringBuilderAppendCStr(stringBuilder_t *sb, const char *cstr) {
    sb->buffer = stringAppendCharPtr(sb->buffer, cstr);
    sb->capacity = stringbytesalloced(sb->buffer) - sizeof(stringHeader_t);
}

void stringBuilderAppendChar(stringBuilder_t *sb, char ch) {
    sb->buffer = stringAppendChar(sb->buffer, ch);
    sb->capacity = stringbytesalloced(sb->buffer) - sizeof(stringHeader_t);
}

void stringBuilderAppendFormat(stringBuilder_t *sb, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    string formatted = stringFormatVa(fmt, args);
    va_end(args);
    
    stringBuilderAppend(sb, formatted);
    destroyString(formatted);
}

string stringBuilderToString(stringBuilder_t *sb) {
    return stringFromString(sb->buffer);
}

void stringBuilderClear(stringBuilder_t *sb) {
    getHeaderPointer(sb->buffer)->length = 0;
    sb->buffer.at[0] = '\0';
}

void stringBuilderDestroy(stringBuilder_t *sb) {
    destroyString(sb->buffer);
    sb->capacity = 0;
}

string stringRemoveDuplicates(string str) {
    if(stringlen(str) <= 1) return stringFromString(str);
    
    bool seen[256] = {false};
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + stringlen(str) + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringRemoveDuplicates\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + stringlen(str) + 1;
    result->length = 0;
    
    for(size_t i = 0; i < stringlen(str); i++) {
        unsigned char ch = (unsigned char)str.at[i];
        if(!seen[ch]) {
            seen[ch] = true;
            result->data[result->length++] = str.at[i];
        }
    }
    result->data[result->length] = '\0';
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringIntersect(string a, string b) {
    bool in_b[256] = {false};
    bool added[256] = {false};
    
    for(size_t i = 0; i < stringlen(b); i++) {
        in_b[(unsigned char)b.at[i]] = true;
    }
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + stringlen(a) + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringIntersect\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + stringlen(a) + 1;
    result->length = 0;
    
    for(size_t i = 0; i < stringlen(a); i++) {
        unsigned char ch = (unsigned char)a.at[i];
        if(in_b[ch] && !added[ch]) {
            added[ch] = true;
            result->data[result->length++] = a.at[i];
        }
    }
    result->data[result->length] = '\0';
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringDifference(string a, string b) {
    bool in_b[256] = {false};
    
    for(size_t i = 0; i < stringlen(b); i++) {
        in_b[(unsigned char)b.at[i]] = true;
    }
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + stringlen(a) + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringDifference\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + stringlen(a) + 1;
    result->length = 0;
    
    for(size_t i = 0; i < stringlen(a); i++) {
        unsigned char ch = (unsigned char)a.at[i];
        if(!in_b[ch]) {
            result->data[result->length++] = a.at[i];
        }
    }
    result->data[result->length] = '\0';
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringEscapeC(string str) {
    stringBuilder_t sb = stringBuilderCreate(stringlen(str) * 2);
    
    for(size_t i = 0; i < stringlen(str); i++) {
        char ch = str.at[i];
        switch(ch) {
            case '\n': stringBuilderAppendCStr(&sb, "\\n"); break;
            case '\r': stringBuilderAppendCStr(&sb, "\\r"); break;
            case '\t': stringBuilderAppendCStr(&sb, "\\t"); break;
            case '\b': stringBuilderAppendCStr(&sb, "\\b"); break;
            case '\f': stringBuilderAppendCStr(&sb, "\\f"); break;
            case '\\': stringBuilderAppendCStr(&sb, "\\\\"); break;
            case '\"': stringBuilderAppendCStr(&sb, "\\\""); break;
            case '\'': stringBuilderAppendCStr(&sb, "\\'"); break;
            default:
                if(ch >= 32 && ch <= 126) {
                    stringBuilderAppendChar(&sb, ch);
                } else {
                    char hex[8];
                    snprintf(hex, sizeof(hex), "\\x%02x", (unsigned char)ch);
                    stringBuilderAppendCStr(&sb, hex);
                }
                break;
        }
    }
    
    string result = stringBuilderToString(&sb);
    stringBuilderDestroy(&sb);
    return result;
}

string stringUnescapeC(string str) {
    stringBuilder_t sb = stringBuilderCreate(stringlen(str));
    
    for(size_t i = 0; i < stringlen(str); i++) {
        if(str.at[i] == '\\' && i + 1 < stringlen(str)) {
            i++;
            switch(str.at[i]) {
                case 'n': stringBuilderAppendChar(&sb, '\n'); break;
                case 'r': stringBuilderAppendChar(&sb, '\r'); break;
                case 't': stringBuilderAppendChar(&sb, '\t'); break;
                case 'b': stringBuilderAppendChar(&sb, '\b'); break;
                case 'f': stringBuilderAppendChar(&sb, '\f'); break;
                case '\\': stringBuilderAppendChar(&sb, '\\'); break;
                case '\"': stringBuilderAppendChar(&sb, '\"'); break;
                case '\'': stringBuilderAppendChar(&sb, '\''); break;
                case 'x':
                    if(i + 2 < stringlen(str)) {
                        char hex[3] = {str.at[i+1], str.at[i+2], '\0'};
                        char *endptr;
                        long val = strtol(hex, &endptr, 16);
                        if(endptr != hex) {
                            stringBuilderAppendChar(&sb, (char)val);
                            i += 2;
                        } else {
                            stringBuilderAppendChar(&sb, str.at[i]);
                        }
                    } else {
                        stringBuilderAppendChar(&sb, str.at[i]);
                    }
                    break;
                default:
                    stringBuilderAppendChar(&sb, str.at[i]);
                    break;
            }
        } else {
            stringBuilderAppendChar(&sb, str.at[i]);
        }
    }
    
    string result = stringBuilderToString(&sb);
    stringBuilderDestroy(&sb);
    return result;
}

string stringToHex(string str) {
    stringBuilder_t sb = stringBuilderCreate(stringlen(str) * 2);
    
    for(size_t i = 0; i < stringlen(str); i++) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", (unsigned char)str.at[i]);
        stringBuilderAppendCStr(&sb, hex);
    }
    
    string result = stringBuilderToString(&sb);
    stringBuilderDestroy(&sb);
    return result;
}

string stringFromHex(string hex_str) {
    if(stringlen(hex_str) % 2 != 0) {
        return string("");
    }
    
    size_t result_len = stringlen(hex_str) / 2;
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + result_len + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringFromHex\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + result_len + 1;
    result->length = result_len;
    
    for(size_t i = 0; i < result_len; i++) {
        char hex[3] = {hex_str.at[i*2], hex_str.at[i*2+1], '\0'};
        char *endptr;
        long val = strtol(hex, &endptr, 16);
        if(endptr == hex || *endptr != '\0') {
            free(result);
            return string("");
        }
        result->data[i] = (char)val;
    }
    result->data[result_len] = '\0';
    
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringBase64Encode(string str) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    size_t in_len = stringlen(str);
    size_t out_len = 4 * ((in_len + 2) / 3);
    
    stringHeader_t *result = malloc(sizeof(stringHeader_t) + out_len + 1);
    if(!result) {
        fprintf(stderr, "failed to allocate memory in stringBase64Encode\n");
        exit(EXIT_FAILURE);
    }
    result->allocated_bytes = sizeof(stringHeader_t) + out_len + 1;
    result->length = out_len;
    
    size_t i = 0, j = 0;
    unsigned char array_3[3];
    unsigned char array_4[4];
    
    while(in_len--) {
        array_3[i++] = str.at[j++];
        if(i == 3) {
            array_4[0] = (array_3[0] & 0xfc) >> 2;
            array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
            array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
            array_4[3] = array_3[2] & 0x3f;
            
            for(i = 0; i < 4; i++) {
                result->data[j - 3 + i] = base64_chars[array_4[i]];
            }
            i = 0;
        }
    }
    
    if(i) {
        for(size_t k = i; k < 3; k++) {
            array_3[k] = '\0';
        }
        
        array_4[0] = (array_3[0] & 0xfc) >> 2;
        array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
        array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
        
        for(size_t k = 0; k < i + 1; k++) {
            result->data[j++] = base64_chars[array_4[k]];
        }
        
        while(i++ < 3) {
            result->data[j++] = '=';
        }
    }
    
    result->data[out_len] = '\0';
    return (string) { .data = (dataSegmentOfString_t *)result->data };
}

string stringUrlEncode(string str) {
    stringBuilder_t sb = stringBuilderCreate(stringlen(str) * 3);
    
    for(size_t i = 0; i < stringlen(str); i++) {
        unsigned char ch = (unsigned char)str.at[i];
        if(isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            stringBuilderAppendChar(&sb, ch);
        } else if(ch == ' ') {
            stringBuilderAppendChar(&sb, '+');
        } else {
            char encoded[4];
            snprintf(encoded, sizeof(encoded), "%%%02X", ch);
            stringBuilderAppendCStr(&sb, encoded);
        }
    }
    
    string result = stringBuilderToString(&sb);
    stringBuilderDestroy(&sb);
    return result;
}

bool stringMatchWildcard(string str, string pattern) {
    size_t s = 0, p = 0;
    size_t star_idx = (size_t)-1;
    size_t match_idx = 0;
    
    while(s < stringlen(str)) {
        if(p < stringlen(pattern) && 
           (pattern.at[p] == '?' || pattern.at[p] == str.at[s])) {
            s++;
            p++;
        } else if(p < stringlen(pattern) && pattern.at[p] == '*') {
            star_idx = p;
            match_idx = s;
            p++;
        } else if(star_idx != (size_t)-1) {
            p = star_idx + 1;
            match_idx++;
            s = match_idx;
        } else {
            return false;
        }
    }
    
    while(p < stringlen(pattern) && pattern.at[p] == '*') {
        p++;
    }
    
    return p == stringlen(pattern);
}

bool stringMatchGlob(string str, string pattern) {
    return stringMatchWildcard(str, pattern);
}

int stringLevenshteinDistance(string a, string b) {
    size_t len_a = stringlen(a);
    size_t len_b = stringlen(b);
    
    if(len_a == 0) return (int)len_b;
    if(len_b == 0) return (int)len_a;
    
    size_t *prev_row = malloc((len_b + 1) * sizeof(size_t));
    size_t *curr_row = malloc((len_b + 1) * sizeof(size_t));
    
    if(!prev_row || !curr_row) {
        fprintf(stderr, "failed to allocate memory in stringLevenshteinDistance\n");
        exit(EXIT_FAILURE);
    }
    
    for(size_t j = 0; j <= len_b; j++) {
        prev_row[j] = j;
    }
    
    for(size_t i = 0; i < len_a; i++) {
        curr_row[0] = i + 1;
        
        for(size_t j = 0; j < len_b; j++) {
            size_t cost = (a.at[i] == b.at[j]) ? 0 : 1;
            size_t deletion = prev_row[j + 1] + 1;
            size_t insertion = curr_row[j] + 1;
            size_t substitution = prev_row[j] + cost;
            
            curr_row[j + 1] = deletion < insertion ? deletion : insertion;
            curr_row[j + 1] = curr_row[j + 1] < substitution ? curr_row[j + 1] : substitution;
        }
        
        size_t *temp = prev_row;
        prev_row = curr_row;
        curr_row = temp;
    }
    
    int result = (int)prev_row[len_b];
    free(prev_row);
    free(curr_row);
    return result;
}

double stringSimilarity(string a, string b) {
    int distance = stringLevenshteinDistance(a, b);
    size_t max_len = stringlen(a) > stringlen(b) ? stringlen(a) : stringlen(b);
    if(max_len == 0) return 1.0;
    return 1.0 - ((double)distance / (double)max_len);
}

string stringToTitleCase(string str) {
    string result = stringFromString(str);
    bool new_word = true;
    
    for(size_t i = 0; i < stringlen(result); i++) {
        if(isspace((unsigned char)result.at[i])) {
            new_word = true;
        } else if(new_word) {
            result.at[i] = toupper((unsigned char)result.at[i]);
            new_word = false;
        } else {
            result.at[i] = tolower((unsigned char)result.at[i]);
        }
    }
    
    return result;
}

string stringToCamelCase(string str) {
    stringBuilder_t sb = stringBuilderCreate(stringlen(str));
    bool capitalize_next = false;
    
    for(size_t i = 0; i < stringlen(str); i++) {
        if(isspace((unsigned char)str.at[i]) || str.at[i] == '_' || str.at[i] == '-') {
            capitalize_next = true;
        } else if(capitalize_next) {
            stringBuilderAppendChar(&sb, toupper((unsigned char)str.at[i]));
            capitalize_next = false;
        } else {
            stringBuilderAppendChar(&sb, tolower((unsigned char)str.at[i]));
        }
    }
    
    string result = stringBuilderToString(&sb);
    stringBuilderDestroy(&sb);
    return result;
}

string stringToSnakeCase(string str) {
    stringBuilder_t sb = stringBuilderCreate(stringlen(str) * 2);
    
    for(size_t i = 0; i < stringlen(str); i++) {
        if(isupper((unsigned char)str.at[i])) {
            if(i > 0 && !isspace((unsigned char)str.at[i-1])) {
                stringBuilderAppendChar(&sb, '_');
            }
            stringBuilderAppendChar(&sb, tolower((unsigned char)str.at[i]));
        } else if(isspace((unsigned char)str.at[i]) || str.at[i] == '-') {
            stringBuilderAppendChar(&sb, '_');
        } else {
            stringBuilderAppendChar(&sb, str.at[i]);
        }
    }
    
    string result = stringBuilderToString(&sb);
    stringBuilderDestroy(&sb);
    return result;
}

string stringToKebabCase(string str) {
    stringBuilder_t sb = stringBuilderCreate(stringlen(str) * 2);
    
    for(size_t i = 0; i < stringlen(str); i++) {
        if(isupper((unsigned char)str.at[i])) {
            if(i > 0 && !isspace((unsigned char)str.at[i-1])) {
                stringBuilderAppendChar(&sb, '-');
            }
            stringBuilderAppendChar(&sb, tolower((unsigned char)str.at[i]));
        } else if(isspace((unsigned char)str.at[i]) || str.at[i] == '_') {
            stringBuilderAppendChar(&sb, '-');
        } else {
            stringBuilderAppendChar(&sb, str.at[i]);
        }
    }
    
    string result = stringBuilderToString(&sb);
    stringBuilderDestroy(&sb);
    return result;
}
