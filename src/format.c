// #define DEBUG
#include "../include/chad/format.h"
#include "../include/chad/debug.h"

#undef substring
#undef printh
#undef format

dictionary_t createDictionary(size_t count, char *data[count][2]) {
    dictionary_t dictionary_to_return;
    dictionary_to_return.key = malloc(count * sizeof(char *));
    dictionary_to_return.value = malloc(count * sizeof(char *));
    dictionary_to_return.entry_count = count;
    
    for (size_t i = 0; i < count; i++) {
        dictionary_to_return.key[i] = data[i][0] ? strdup(data[i][0]) : NULL;
        dictionary_to_return.value[i] = data[i][1] ? strdup(data[i][1]) : NULL;
        
        if ((data[i][0] && !dictionary_to_return.key[i]) || 
            (data[i][1] && !dictionary_to_return.value[i])) {
            for (size_t j = 0; j < i; j++) {
                free(dictionary_to_return.key[j]);
                free(dictionary_to_return.value[j]);
            }
            free(dictionary_to_return.key);
            free(dictionary_to_return.value);
            dictionary_to_return.entry_count = 0;
            return dictionary_to_return;
        }
    }
    return dictionary_to_return;
}

dictionary_t createDictionaryOwned(size_t count, char *data[count][2]) {
    dictionary_t dictionary_to_return;
    dictionary_to_return.key = malloc(count * sizeof(char *));
    dictionary_to_return.value = malloc(count * sizeof(char *));
    dictionary_to_return.entry_count = count;

    for (size_t i = 0; i < count; i++) {
        dictionary_to_return.key[i] = data[i][0];
        dictionary_to_return.value[i] = data[i][1];
    }
    return dictionary_to_return;
}

void destroyDictionary(dictionary_t to_destroy) {
    for (size_t i = 0; i < to_destroy.entry_count; i++) {
        free(to_destroy.key[i]);
        free(to_destroy.value[i]);
    }
    free(to_destroy.key);
    free(to_destroy.value);
}

void printDictionary(dictionary_t dictionary) {    
    for (size_t i = 0; i < dictionary.entry_count; ++i) {
        printf("%s: ", dictionary.key[i]);
        printf("%s\n", dictionary.value[i]);
    }
}

char *substringStrchr(substring_t substr, char c) {
    if (substr.start == NULL || substr.end == NULL || substr.start >= substr.end) {
        return NULL; // Handle invalid input
    }

    for (char *ptr = substr.start; ptr < substr.end; ++ptr) {
        if (*ptr == c) {
            return ptr;
        }
    }

    return NULL; // Character not found
}

dynarray(string) tokenizeString(char_ptr_conv_t input, char_ptr_conv_t delim) {
    if(!input.as_char_ptr || !delim.as_char_ptr) {
        return create_dynarray(string);
    }
    size_t input_index = 0, delim_index = 0, marker = 0;
        bool found = false, delim_matches = true;
        string token = {};
        dynarray(string) ret = create_dynarray(string);
    
        while (input.as_char_ptr[input_index]) {
            delim_matches = input.as_char_ptr[input_index] == delim.as_char_ptr[delim_index];
            found = !delim.as_char_ptr[delim_index];
            delim_index++;
            delim_index *= delim_matches;
    
            if (found) {
                token = stringSliceFromCharPtr(input.as_char_ptr, marker, input_index);
                dynarray_append(ret, token);
                marker = input_index; 
            }
            input_index++;
        }
    
        if (marker < input_index) {
            token = stringSliceFromCharPtr(input.as_char_ptr, marker, input_index);
            dynarray_append(ret, token);
        }
    
        return ret;
}

dynarray(string) tokenizePairwiseString(char_ptr_conv_t input, 
    char_ptr_conv_t start_delim, char_ptr_conv_t end_delim) {
    if (!input.as_char_ptr || !start_delim.as_char_ptr || !end_delim.as_char_ptr) {
        return create_dynarray(string);
    }
    size_t input_index = 0, start_delimiter_index = 0, end_delimiter_index = 0, token_start = 0;
    bool start_delimiter_matches = true, end_delimiter_matches = true;
    string token = {};
    dynarray(string) ret = create_dynarray(string);
    while (input.as_char_ptr[input_index]) {
        start_delimiter_matches = input.as_char_ptr[input_index] == start_delim.as_char_ptr[start_delimiter_index];
        end_delimiter_matches = input.as_char_ptr[input_index] == end_delim.as_char_ptr[end_delimiter_index];
        start_delimiter_index += start_delimiter_matches;
        end_delimiter_index += end_delimiter_matches;
        if (!start_delim.as_char_ptr[start_delimiter_index] && !end_delim.as_char_ptr[end_delimiter_index]) {
            token = stringSliceFromCharPtr(input.as_char_ptr, token_start, input_index);
            token_start = input_index;
            dynarray_append(ret, token);
        }
        input_index++;
    }

    if (token_start < input_index) {
        token = stringSliceFromCharPtr(input.as_char_ptr, token_start, input_index);
        dynarray_append(ret, token);
    }
    return ret;
}


substring_t substringTrimWhitespace(substring_t substr) {
    if (substr.start == NULL || substr.end == NULL || substr.start >= substr.end) {
        return (substring_t){ substr.start, substr.end }; // Handle invalid input
    }

    char *start = substr.start;
    while (start < substr.end && isspace((unsigned char)*start)) {
        start++;
    }

    if (start >= substr.end) {
        // entire input was whitespace
        return (substring_t){ start, start };
    }

    char *end = substr.end - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }

    dbg("substringTrimWhitespace: ");
    printSubstring((substring_t){ start, end + 1 });

    return (substring_t){
        .start = start, 
        .end = end + 1, // .end is exclusive, so include the last non-whitespace char
    };
}         

char *strdupSubstring(substring_t substr) {
    if (substr.start == NULL || substr.end == NULL || substr.start >= substr.end) {
        return NULL; // Handle invalid input
    }
    size_t len = substr.end - substr.start;
    char *dup = malloc(len + 1);
    if (dup == NULL) {
        return NULL;
    }
    memcpy(dup, substr.start, len);
    dup[len] = '\0';
    dbg("strdupSubstring: %s", dup);
    return dup;
}

void printSubstring(substring_t substr) {
    if(substr.start == NULL) {
        printf("<start pointer null>");
        return;
    } else {
        char *ptr = substr.start;
        while(ptr != substr.end && *ptr != '\0') {
            putchar(*ptr);
            if(substr.end > substr.start) {
                ptr++;
            } else {
                ptr--;
            }
        }
    }
}

char *strdup(const char * s) {
  	size_t len = 1 + strlen(s);
  	char *p = malloc(len);

  	return p ? memcpy(p, s, len) : NULL;
}

char *surroundWithBraces_old(char *text) {
	dbg("called btw");
  	if (!text) {
    	return strdup("{}");
  	}
  	size_t len = strlen(text);
  	if (malloc_usable_size(text) < len + 3) {
    	memmove(text + 1, text, len);
  	} else {
    	char *nbuf = malloc(len + 3);
    	memcpy(nbuf + 1, text, len);
    	free(text);
    	text = nbuf;
  	}
  	text[0] = '{';
  	text[len + 1] = '}';
  	text[len + 2] = '\0';
  	return text;
}

char *surroundWithBraces(char *text) {
    if (!text) {
        return strdup("{}");
    }

    size_t len = strlen(text);
    char *new_text = realloc(text, len + 3);
    if (!new_text) {
        return NULL;
    }

    memmove(new_text + 1, new_text, len);
    new_text[0] = '{';
    new_text[len + 1] = '}';
    new_text[len + 2] = '\0';
	dbg("%s", new_text);
    return new_text;
}


dictionary_t convertKeysToTags(dictionary_t dictionary) {
    for(size_t index = 0; index < dictionary.entry_count; index++) {
        dictionary.key[index] = surroundWithBraces(dictionary.key[index]);
    }
    return dictionary;
}

char *replaceSubstrings_new(char *inputString, dictionary_t dictionary) {
    // Calculate the length of the modified string
    size_t inputLength = strlen(inputString);
    size_t outputLength = inputLength;

    for (size_t i = 0; i < dictionary.entry_count; i++) {
        char *substring = dictionary.key[i];
        char *replacement = dictionary.value[i];

        // Count occurrences of substring
        char *pos = inputString;
        while ((pos = strstr(pos, substring)) != NULL) {
            outputLength += strlen(replacement) - strlen(substring);
            pos += strlen(substring);
        }
    }

    // Allocate memory for the modified string
    char *outputString = malloc(outputLength + 1);
    if (outputString == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Copy and replace substrings
    size_t currentIndex = 0;
    for (size_t i = 0; i < inputLength; i++) {
        int match_found = 0;
        for (size_t j = 0; j < dictionary.entry_count; j++) {
            char *substring = dictionary.key[j];
            char *replacement = dictionary.value[j];

            if (strncmp(inputString + i, substring, strlen(substring)) == 0) {
                strcpy(outputString + currentIndex, replacement);
                currentIndex += strlen(replacement);
                i += strlen(substring) - 1;  // Move the index past the matched substring
                match_found = 1;
                break;
            }
        }

        if (!match_found) {
            outputString[currentIndex++] = inputString[i];
        }
    }

    outputString[currentIndex] = '\0'; // Null-terminate the output string
	free(inputString);
    return outputString;
}

size_t countSubstring(const char *str, const char *sub) {
  size_t count = 0;
  size_t len_sub = strlen(sub);
	for (size_t i = 0; str[i] != '\0'; i++) {
    	if (strncmp(str + i, sub, len_sub) == 0) {
      		count++;
      		i += len_sub - 1;
    	}
  	}
	return count;
}

dictionary_index_search_t sequenceMatchesDictionaryKey(char *str, size_t index, dictionary_t dictionary) {
    dictionary_index_search_t result;
    result.success = false;
    result.index = 0;
    
    for (size_t i = 0; i < dictionary.entry_count; i++) {
        size_t key_len = strlen(dictionary.key[i]);

        if (index + key_len <= strlen(str) && strncmp(str + index, dictionary.key[i], key_len) == 0) {
			dbg("str is %p, the string is %s", str, str);
			dbg("str is %p, the string is %s", str, str);
			dbg("hello?");
            result.success = true;
            result.index = i;
            return result;
        }
    }
    return result;
}

char *replaceSubstrings(char *input_string, dictionary_t dictionary) {
    if (!input_string || dictionary.entry_count == 0) {
        return input_string;
    }

    size_t input_len = strlen(input_string);
    size_t output_len = input_len;
    size_t replacements_count = 0;

    for (size_t i = 0; i < dictionary.entry_count; i++) {
        if (!dictionary.key[i] || !dictionary.value[i]) continue;

        const char *pos = input_string;
        const char *key = dictionary.key[i];
        const size_t key_len = strlen(key);
        const size_t val_len = strlen(dictionary.value[i]);

        while ((pos = strstr(pos, key)) != NULL) {
            output_len += val_len - key_len;
            replacements_count++;
            pos += key_len;
        }
    }

    if (replacements_count == 0) {
        return input_string;
    }

    char *output = malloc(output_len + 2);
    if (!output) {
        return NULL;
    }

    char *out_ptr = output;
    const char *in_ptr = input_string;

    while (*in_ptr) {
        bool replaced = false;
        
        for (size_t i = 0; i < dictionary.entry_count; i++) {
            if (!dictionary.key[i]) continue;

            const size_t key_len = strlen(dictionary.key[i]);
            if (strncmp(in_ptr, dictionary.key[i], key_len) == 0) {
                const size_t val_len = strlen(dictionary.value[i]);
                memcpy(out_ptr, dictionary.value[i], val_len);
                out_ptr += val_len;
                in_ptr += key_len;
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            *out_ptr++ = *in_ptr++;
        }
    }
    *out_ptr = '\0';
    return output;
}

substring_t substring(char *start, char *end) {
    return (substring_t) {
        start, end
    };
}

int asprintf (char **str, const char *fmt, ...) {
  int size = 0;
  va_list args;

  // init variadic argumens
  va_start(args, fmt);

  // format and get size
  size = vasprintf(str, fmt, args);

  // toss args
  va_end(args);

  return size;
}

int vasprintf (char **str, const char *fmt, va_list args) {
  	int size = 0;
  	va_list tmpa;
	
  	// copy
  	va_copy(tmpa, args);
	
  	// apply variadic arguments to
  	// sprintf with format to get size
  	size = vsnprintf(NULL, 0, fmt, tmpa);
	
  	// toss args
  	va_end(tmpa);
	
  	// return -1 to be compliant if
  	// size is less than 0
  	if (size < 0) { return -1; }
	
  	// alloc with size plus 1 for `\0'
  	*str = (char *) malloc(size + 1);
	
  	// return -1 to be compliant
  	// if pointer is `NULL'
  	if (NULL == *str) { return -1; }

	// format string with original
  	// variadic arguments and set new size
  	size = vsprintf(*str, fmt, args);
  	return size;
} // end of asprintf

int getIdentifierIndex(char *in, size_t index) {
	if(strlen(in) < index) {
        return -1;
    }
    int position = -1;
	for(size_t i = index; in[i] != 0; i++) 
        if(in[i] <= ' ' || in[i] == '=') {
	    	position = i;
		    break;
    	}
	return position;	
}

char *stringAfter(char *in, size_t index) {
	if(in == NULL) {
		return NULL;
	}
	size_t length = strlen(in);
	index = (length <= index) ? length: index;
	char *substring;
	(substring = strncpy(malloc(index + 1), in, index))[index] = '\0'; 
	return substring;
}

char *getIdentifier(char *in) {
	return stringAfter(in, getIdentifierIndex(in, 0));
}

// buf is going to be edited by 
char *applyPadding(const char *value, char align, char fill, size_t width) {
	size_t len = value ? strlen(value) : 0;
	if(len >= width) {
		return strdup(value ? value : "");
	}

	size_t pad = width - len;
	char *result = malloc(width + 1);
	if(result == NULL) {
		return NULL;
	}

	switch(align) {
		case '>': {
			memset(result, fill, pad);
			if(len) memcpy(result + pad, value, len);
			break;
		}
		case '^': {
			size_t left = pad / 2;
			size_t right = pad - left;
			memset(result, fill, left);
			if(len) memcpy(result + left, value, len);
			memset(result + left + len, fill, right);
			break;
		}
		case '<':
		default: {
			if(len) memcpy(result, value, len);
			memset(result + len, fill, pad);
			break;
		}
	}
	result[width] = '\0';
	return result;
}

// parses a format spec of the form [[fill]align][width][.precision][type]
// (type is one of d/x/X/o/f) and renders value accordingly, converting the
// stringified value back to a number when a numeric type or precision is
// requested. returned string is always malloc'd and must be freed by the caller.
char *applyFormatSpec(const char *value, const char *spec, size_t spec_len) {
	if(value == NULL) {
		value = "";
	}

	char fill = ' ';
	char align = '<';
	size_t width = 0;
	bool has_width = false;
	long precision = -1;
	char type = 0;
	size_t i = 0;

	if(i + 1 < spec_len && (spec[i + 1] == '<' || spec[i + 1] == '>' || spec[i + 1] == '^')) {
		fill = spec[i];
		align = spec[i + 1];
		i += 2;
	} else if(i < spec_len && (spec[i] == '<' || spec[i] == '>' || spec[i] == '^')) {
		align = spec[i];
		i++;
	}

	while(i < spec_len && spec[i] >= '0' && spec[i] <= '9') {
		has_width = true;
		width = width * 10 + (spec[i] - '0');
		i++;
	}

	if(i < spec_len && spec[i] == '.') {
		i++;
		precision = 0;
		while(i < spec_len && spec[i] >= '0' && spec[i] <= '9') {
			precision = precision * 10 + (spec[i] - '0');
			i++;
		}
	}

	if(i < spec_len && strchr("dxXof", spec[i]) != NULL) {
		type = spec[i];
		i++;
	}

	char *converted = NULL;
	const char *to_pad = value;

	switch(type) {
		case 'd': {
			long long v = strtoll(value, NULL, 10);
			asprintf(&converted, "%lld", v);
			to_pad = converted;
			break;
		}
		case 'x': {
			long long v = strtoll(value, NULL, 10);
			asprintf(&converted, "%llx", v);
			to_pad = converted;
			break;
		}
		case 'X': {
			long long v = strtoll(value, NULL, 10);
			asprintf(&converted, "%llX", v);
			to_pad = converted;
			break;
		}
		case 'o': {
			long long v = strtoll(value, NULL, 10);
			asprintf(&converted, "%llo", v);
			to_pad = converted;
			break;
		}
		case 'f': {
			double v = strtod(value, NULL);
			asprintf(&converted, "%.*f", (int)(precision >= 0 ? precision : 6), v);
			to_pad = converted;
			break;
		}
		default: {
			if(precision >= 0) {
				// no numeric type requested: precision truncates the string
				size_t vlen = strlen(value);
				size_t take = (size_t)precision < vlen ? (size_t)precision : vlen;
				converted = malloc(take + 1);
				memcpy(converted, value, take);
				converted[take] = '\0';
				to_pad = converted;
			}
			break;
		}
	}

	char *result = has_width ? applyPadding(to_pad, align, fill, width) : strdup(to_pad);
	free(converted);
	return result;
}

// scans buf for named tags with a format spec, e.g. "{name:>10}", and renders
// them using the raw dictionary value for "{name}" -- runs after positionalInsert
// (which only touches bare/numbered braces) and before replaceSubstrings (which
// only handles plain, spec-less named tags).
char *applyNamedFormatSpecs(char *buf, dictionary_t dictionary) {
	if(buf == NULL) {
		return NULL;
	}

	size_t len = strlen(buf);
	size_t index = 0;

	while(index < len) {
		if(buf[index] != '{' || buf[index + 1] == '{') {
			index++;
			continue;
		}

		size_t name_start = index + 1;
		size_t i = name_start;
		while(i < len && buf[i] != ':' && buf[i] != '}' && buf[i] != '{') {
			i++;
		}

		if(i >= len || buf[i] != ':' || i == name_start) {
			index++;
			continue;
		}

		size_t name_end = i;
		size_t spec_start = i + 1;
		size_t j = spec_start;
		while(j < len && buf[j] != '}') {
			j++;
		}

		if(j >= len || buf[j] != '}') {
			index++;
			continue;
		}

		size_t name_len = name_end - name_start;
		char *tag = malloc(name_len + 3);
		tag[0] = '{';
		memcpy(tag + 1, buf + name_start, name_len);
		tag[name_len + 1] = '}';
		tag[name_len + 2] = '\0';

		char *raw_value = NULL;
		for(size_t k = 0; k < dictionary.entry_count; k++) {
			if(strcmp(dictionary.key[k], tag) == 0) {
				raw_value = dictionary.value[k];
				break;
			}
		}
		free(tag);

		if(raw_value == NULL) {
			index = j + 1;
			continue;
		}

		char *rendered = applyFormatSpec(raw_value, buf + spec_start, j - spec_start);
		size_t val_len = rendered ? strlen(rendered) : 0;
		size_t tag_len = j - index + 1;
		size_t new_length = len + val_len - tag_len + 1;

		if(val_len > tag_len) {
			buf = realloc(buf, new_length);
		}
		memmove(buf + index + val_len, buf + j + 1, len - j);
		if(val_len < tag_len) {
			buf = realloc(buf, new_length);
		}
		len = new_length - 1;
		if(val_len != 0) {
			memcpy(buf + index, rendered, val_len);
		}
		buf[len] = '\0';
		free(rendered);

		index += val_len;
	}

	return buf;
}

// literal "{{" / "}}" are protected from tag parsing (positionalInsert,
// applyNamedFormatSpecs, replaceSubstrings) by collapsing them to these
// single-byte sentinels; restoreEscapedBraces() converts them back to '{'/'}'
// as the final step of format()/printh().
#define ESCAPED_OPEN_BRACE_SENTINEL '\x01'
#define ESCAPED_CLOSE_BRACE_SENTINEL '\x02'

char *protectEscapedBraces(char *buf) {
	if(buf == NULL) {
		return NULL;
	}

	size_t len = strlen(buf);
	size_t index = 0;
	while(index < len) {
		if(buf[index] == '{' && buf[index + 1] == '{') {
			buf[index] = ESCAPED_OPEN_BRACE_SENTINEL;
			memmove(buf + index + 1, buf + index + 2, len - index - 2 + 1);
			len--;
		} else if(buf[index] == '}' && buf[index + 1] == '}') {
			buf[index] = ESCAPED_CLOSE_BRACE_SENTINEL;
			memmove(buf + index + 1, buf + index + 2, len - index - 2 + 1);
			len--;
		}
		index++;
	}
	return buf;
}

char *restoreEscapedBraces(char *buf) {
	if(buf == NULL) {
		return NULL;
	}

	for(size_t i = 0; buf[i]; i++) {
		if(buf[i] == ESCAPED_OPEN_BRACE_SENTINEL) {
			buf[i] = '{';
		} else if(buf[i] == ESCAPED_CLOSE_BRACE_SENTINEL) {
			buf[i] = '}';
		}
	}
	return buf;
}

char *positionalInsert(char *buf, dictionary_t dictionary) {
	if(buf == NULL) {
		return NULL;
	}

	size_t len = strlen(buf);
	// keeps track of number of {} and indexes
	// into the dictionary correctly	
	size_t dictionary_index = 0; 
	
	size_t index = 0;
	char ch;
	while(ch = buf[index]) {
		dbg("main loop running, the index is %ld\n", index);
		dbgstr(buf, index);
		if(ch == '{') {
			dbg("the charcter ch is now \e[0;31m%c\e[0m (should be '{')", buf[index]);
			size_t run = index + 1;
			size_t temp_number = 0;
			dbgstr(buf, index);
			if(buf[index + 1] == '}') {
				dictionary_index++; // todo insert value from index
				dbg("dictionary index: %ld\n", dictionary_index);
				if(dictionary_index - 1 < dictionary.entry_count) {
					size_t val_len = strlen(dictionary.value[dictionary_index - 1]);
					size_t new_length = len + val_len + 1; // null terminator :)
					if(val_len > 2) {
						buf = realloc(buf, new_length);
					}
					// 								the + 1 copies the null terminator
					memmove(buf + index + val_len, buf + index + 2, len - index - 2 + 1);
					if(val_len < 2) {
						buf = realloc(buf, new_length);
					}
					len += val_len - 2;
					if(val_len != 0) {
						strncpy(buf + index, dictionary.value[dictionary_index - 1], 
							strlen(dictionary.value[dictionary_index - 1]));
					} 
					dbg("->%s\n", buf);
				}
			} else {
				dbg("entering number construction segment");
				while(buf[run] == ' ') run++;
				while('0' <= buf[run] && buf[run] <= '9') {
					temp_number *= 10;
					temp_number += buf[run] - '0';
					run++;
				} 
				while(buf[run] == ' ') run++;

				// optional format spec: ':' [[fill]align][width][.precision][type]
				bool has_spec = false;
				size_t spec_start = 0;
				size_t spec_end = 0;
				if(buf[run] == ':') {
					has_spec = true;
					run++;
					spec_start = run;
					while(buf[run] && buf[run] != '}' && buf[run] != ' ') run++;
					spec_end = run;
				}
				while(buf[run] == ' ') run++;

				if(buf[run] == '}') {
					// end found
					dictionary_index = temp_number;
					dbg("dictionary index: %ld\n", dictionary_index);
					
					if (dictionary_index < dictionary.entry_count) {
						char *rendered = NULL;
						const char *value_to_use = dictionary.value[dictionary_index];
						if(has_spec) {
							rendered = applyFormatSpec(value_to_use, buf + spec_start, spec_end - spec_start);
							value_to_use = rendered;
						}
						size_t val_len = value_to_use ? strlen(value_to_use) : 0;
						size_t tag_len = run - index + 1;
						size_t new_length = len + val_len - tag_len + 1;
						
						if (val_len > tag_len) {
							buf = realloc(buf, new_length);
						}
						memmove(buf + index + val_len, buf + run + 1, len - run);
						if (val_len < tag_len) {
							buf = realloc(buf, new_length);
						}
						len = new_length - 1;
						if (val_len != 0) {
							memcpy(buf + index, value_to_use, val_len);
						}
						buf[len] = '\0';
						index += val_len - 1; 
						free(rendered);
					}
				}
			}
		} else {
			dbg("skipping to next character");
		}
		index++;
	}
	dbg("->%s\n", buf);
	return buf;	
}
                                                                                          
char *format(char *buf, dictionary_t dictionary) {
    char *temp;
	temp = protectEscapedBraces(buf);
	temp = positionalInsert(temp, dictionary);
	temp = applyNamedFormatSpecs(temp, dictionary);
	char *output = replaceSubstrings(temp, dictionary);
    if(output != temp) {
        free(temp);
    }
    output = restoreEscapedBraces(output);
    return output;
}

int printh(char *fmt, __attribute_maybe_unused__ dictionary_t dictionary) {
	char *output = format(strdup(fmt), dictionary);
    int output_length = strlen(output);
	fputs(output, stdout);
	free(output);
	return output_length;
}
