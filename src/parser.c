#include "../include/chad/parser.h"
#include "../include/chad/cstl.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

type_modifier_t parseTypeModifier(iterstring_t *rule) {
	type_modifier_t ret = modifier_none;
		bool saw_optional = false;
		bool saw_array = false;
		int bracket_depth = 0;

		parseWhitespace(rule);

		while (1) {
		    char c = rule->str.at[rule->index];
		    switch (c) {
		        case '?':
		            if (saw_optional) return modifier_none;
		            saw_optional = true;
		            ret |= modifier_optional;
		            rule->index++;
		            break;

		        case '[':
		            if (bracket_depth > 0) return modifier_none;
		            bracket_depth++;
		            rule->index++;
		            break;

		        case ']':
		            if (bracket_depth != 1 || saw_array) return modifier_none;
		            bracket_depth--;
		            saw_array = true;
		            ret |= modifier_array;
		            rule->index++;
		            break;
		        default:
		            goto end;
		    }
		}
	end:
		if (bracket_depth != 0) return modifier_none;

		iterstringAdvance(rule);
		return ret;	
}

option(string) parseLiteral(iterstring_t *rule) {
	if(rule->str.data == NULL) {
	    return (option(string)) none;
	}

	if(rule->str.at[rule->index] != '\'') {
	    iterstringReset(rule);
	    return (option(string)) none;
	}

	rule->index++;
	string ret = string("");

	while (rule->str.at[rule->index] != '\0' && rule->str.at[rule->index] != '\'') {
	    if (rule->str.at[rule->index] == '\\' && rule->str.at[rule->index + 1] != '\0') {
	        // Handle escape character
	        rule->index++;
	    }
	    ret = stringAppendChar(ret, rule->str.at[rule->index]);
	    rule->index++;
	}

	if(rule->str.at[rule->index] != '\'') {
	    iterstringReset(rule);
	    destroyString(ret);
	    return (option(string)) none;
	}

	rule->index++;
	iterstringAdvance(rule);
	return (option(string)) some(ret);
}

// both key name and rule
option(string) parseIdentifier(iterstring_t *rule) {
	if(rule->str.data == NULL) {
	    return (option(string)) none;
	}

	string ret = string("");

	if((((rule->str.at[rule->index] >= 'A') && (rule->str.at[rule->index] <= 'Z')) ||
	       ((rule->str.at[rule->index] >= 'a') && (rule->str.at[rule->index] <= 'z')))
	       || (rule->str.at[rule->index] == '_')) {
	    ret = stringAppendChar(ret, rule->str.at[rule->index]);
	    rule->index++;    
	} else {
		destroyString(ret);
	    return (option(string)) none;
	}

	while((((rule->str.at[rule->index] >= 'A') && (rule->str.at[rule->index] <= 'Z')) ||
	       ((rule->str.at[rule->index] >= 'a') && (rule->str.at[rule->index] <= 'z'))) 
	    ||((rule->str.at[rule->index] >= '0') && (rule->str.at[rule->index] <= '9'))
	    || (rule->str.at[rule->index] == '_')) {
	    ret = stringAppendChar(ret, rule->str.at[rule->index]);
	    rule->index++;
	}

	if(rule->index == rule->previous) {
	    iterstringReset(rule);
	    destroyString(ret);
	    return (option(string)) none;
	}

	iterstringAdvance(rule);
	return (option(string)) some(ret);
}

bool parseWhitespace(iterstring_t *rule) {
	if(rule->str.data == NULL) {
		return false;
	}
	bool at_least_one_space_found = false;
	
	while(isspace(rule->str.at[rule->index])) {
		at_least_one_space_found = true;
	    rule->index++;
	}

	if(!at_least_one_space_found) {
		iterstringReset(rule);
	} else {
		iterstringAdvance(rule);
	}
	return at_least_one_space_found;	
}

bool parseSeperator(iterstring_t *rule) {
	parseWhitespace(rule);
	if(rule->str.at[rule->index] == ':') {
	    rule->index++;
	    iterstringAdvance(rule);
	} else {
	    iterstringReset(rule);
	    return false;
	}
	parseWhitespace(rule);
	return true;
}

bool isFollowedByAlternative(iterstring_t *rule) {
	parseWhitespace(rule);
	if(rule->str.at[rule->index] == '|') {
	    rule->index++;
	    iterstringAdvance(rule);
	} else {
	    iterstringReset(rule);
	    return false;
	}
	parseWhitespace(rule);
	return true;
}

static void destroyRule(rule_t *rule);
static void destroyRuleNode(rule_node_t *node);
static void destroyRuleBody(rule_node_seq_t body);

static bool ruleHasStorageKey(rule_t *rule) {
	return rule->storage_key.valid;
}

static bool ruleNodeHasStorageKey(rule_node_t *node) {
	if(node->alternative_or_regular == is_regular) {
		return ruleHasStorageKey(&node->rule);
	}
	for(size_t i = 0; i < node->alternative.count; i++) {
		rule_sequence_t seq = node->alternative.at[i];
		for(size_t j = 0; j < seq.count; j++) {
			if(ruleHasStorageKey(&seq.at[j])) {
				return true;
			}
		}
	}
	return false;
}

static bool bodyHasStorageKey(rule_node_seq_t body) {
	for(size_t i = 0; i < body.count; i++) {
		if(ruleNodeHasStorageKey(&body.at[i])) {
			return true;
		}
	}
	return false;
}

// parses one "a b c" run of rule_t elements, stopping as soon as
// compileRule() can no longer match (e.g. at `|`, `)`, or end of input)
static option(rule_sequence_t) compileRuleSequence(iterstring_t *rule) {
	rule_sequence_t seq = create_dynarray(rule_t);
	option(rule_t) r;

	parseWhitespace(rule);
	while((r = compileRule(rule)).valid) {
		dynarray_append(seq, r.value);
		parseWhitespace(rule);
	}

	if(seq.count == 0) {
		destroy_dynarray(seq);
		return (option(rule_sequence_t)) none;
	}
	return (option(rule_sequence_t)) some(seq);
}

// compiles the whole element list for a grammar entry or an inline rule
// body. Alternation (`|`) has low binding strength: everything accumulated
// since the start of the body (or the previous `|`) becomes one alternative
// branch, so a single top-level `|` collapses the entire remaining body into
// one has_alternative rule_node_t instead of a sequence of separate nodes.
option(rule_node_seq_t) compileRuleBody(iterstring_t *rule) {
	rule_node_seq_t body = create_dynarray(rule_node_t);

	parseWhitespace(rule);

	option(rule_sequence_t) first_seq = compileRuleSequence(rule);
	if(!first_seq.valid) {
		destroy_dynarray(body);
		return (option(rule_node_seq_t)) none;
	}

	if(isFollowedByAlternative(rule)) {
		dynarray(rule_sequence_t) alternatives = create_dynarray(rule_sequence_t);
		dynarray_append(alternatives, first_seq.value);

		bool has_more = true;
		while(has_more) {
			option(rule_sequence_t) next_seq = compileRuleSequence(rule);
			if(!next_seq.valid) {
				for(size_t i = 0; i < alternatives.count; i++) {
					destroy_dynarray(alternatives.at[i]);
				}
				destroy_dynarray(alternatives);
				destroy_dynarray(body);
				return (option(rule_node_seq_t)) none;
			}
			dynarray_append(alternatives, next_seq.value);
			has_more = isFollowedByAlternative(rule);
		}

		rule_node_t node = {
			.alternative_or_regular = has_alternative,
			.alternative = alternatives,
		};
		dynarray_append(body, node);
		return (option(rule_node_seq_t)) some(body);
	}

	// no top-level alternation: every rule_t parsed becomes its own
	// separate rule_node_t, preserving the existing sequential behavior
	for(size_t i = 0; i < first_seq.value.count; i++) {
		rule_node_t node = {
			.alternative_or_regular = is_regular,
			.rule = first_seq.value.at[i],
		};
		dynarray_append(body, node);
	}
	destroy_dynarray(first_seq.value);

	return (option(rule_node_seq_t)) some(body);
}

// parses the body of an inline anonymous rule; assumes the current
// position is at the opening '('
static option(inline_rule_t) compileInlineRule(iterstring_t *rule) {
	rule->index++;
	iterstringAdvance(rule);
	parseWhitespace(rule);

	option(rule_node_seq_t) body = compileRuleBody(rule);
	if(!body.valid) {
		return (option(inline_rule_t)) none;
	}

	parseWhitespace(rule);
	if(rule->str.at[rule->index] != ')') {
		destroyRuleBody(body.value);
		return (option(inline_rule_t)) none;
	}
	rule->index++;
	iterstringAdvance(rule);

	inline_rule_t ret = {
		.rule_type = bodyHasStorageKey(body.value) ? object_storage : implicit_storage,
		.element = body.value,
	};
	return (option(inline_rule_t)) some(ret);
}

option(rule_t) compileRule(iterstring_t *rule) {
    parseWhitespace(rule);
    
    option(string) res;
    
    if((res = parseLiteral(rule)).valid) {
        rule_t ret = {
            .storage_key = none,
            .literal_or_rule = is_literal,
            .literal = res.value,
            .type_mod = parseTypeModifier(rule),		
        };
        return (option(rule_t)) some(ret);
    }

    if(rule->str.at[rule->index] == '(') {
        option(inline_rule_t) inl = compileInlineRule(rule);
        if(!inl.valid) {
            return (option(rule_t)) none;
        }
        rule_t ret = {
            .storage_key = none,
            .literal_or_rule = is_inline,
            .inl = inl.value,
            .type_mod = parseTypeModifier(rule),
        };
        return (option(rule_t)) some(ret);
    }
    
    if((res = parseIdentifier(rule)).valid) {
        parseWhitespace(rule);
        
        if(rule->str.at[rule->index] == ':') {
            rule->index++;
            iterstringAdvance(rule);
            parseWhitespace(rule);

            if(rule->str.at[rule->index] == '(') {
                option(inline_rule_t) inl = compileInlineRule(rule);
                if(inl.valid) {
                    rule_t ret = {
                        .type_mod = parseTypeModifier(rule),
                        .storage_key = res,
                        .literal_or_rule = is_inline,
                        .inl = inl.value,
                    };
                    return (option(rule_t)) some(ret);
                }
                destroyString(res.value);
                return (option(rule_t)) none;
            }

			option(string) rule_name = parseIdentifier(rule);
			if(rule_name.valid) {
				rule_t ret = {
					.type_mod = parseTypeModifier(rule),
					.storage_key = res,
					.literal_or_rule = is_rule,
					.rule_name = rule_name.value,
					.ge = NULL,
				};
				return (option(rule_t)) some(ret);
			}

			option(string) literal = parseLiteral(rule);
			if(literal.valid) {
				rule_t ret = {
					.type_mod = parseTypeModifier(rule),
					.storage_key = res,
					.literal_or_rule = is_literal,
					.literal = literal.value,	
				};
				return (option(rule_t)) some(ret);
			}
			destroyString(res.value);
			return (option(rule_t)) none;
        } else {
            rule_t ret = {
                .storage_key = none,
                .literal_or_rule = is_rule,
                .rule_name = res.value,
                .ge = NULL,
                .type_mod = parseTypeModifier(rule),
            };
            return (option(rule_t)) some(ret);
        }
    }   
    return (option(rule_t)) none;
}

bool parseStringLiterally(iterstring_t *rule, const char *literal) {
    size_t len = strlen(literal);
    for(size_t i = 0; i < len; i++) {
        if(rule->str.at[rule->index + i] != literal[i]) {
            return false;
        }
    }
    rule->index += len;
    iterstringAdvance(rule);
    return true;
}

option(grammar_entry_t) compileGrammarEntry(string rule_definition) {
    iterstring_t it = { .previous = 0, .index = 0, .str = rule_definition };
    
    parseWhitespace(&it);
    option(string) name = parseIdentifier(&it);
    if(!name.valid) {
        fprintf(stderr, "Failed to parse rule name\n");
        return (option(grammar_entry_t)) none;
    }
    
    parseWhitespace(&it);
    
    grammar_entry_t ret = {
        .name = name.value,
        .rule_type = storage_type_not_set,
    };
    
    if(!parseStringLiterally(&it, "->")) {
        fprintf(stderr, "Expected '->' after rule name '%s'\n", name.value.at);
        destroyString(ret.name);
        return (option(grammar_entry_t)) none;
    }
    
    parseWhitespace(&it);
    
    option(rule_node_seq_t) body = compileRuleBody(&it);
    if(!body.valid) {
        fprintf(stderr, "Rule '%s' has no elements\n", ret.name.at);
        destroyString(ret.name);
        return (option(grammar_entry_t)) none;
    }
    
    ret.element = body.value;
    ret.rule_type = bodyHasStorageKey(ret.element) ? object_storage : implicit_storage;
    
    return (option(grammar_entry_t)) some(ret);
}

option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]) {
    grammar_t gram;
    gram.entry = create_dynarray(grammar_entry_t);
    
    for(size_t i = 0; i < count; i++) {
        option(grammar_entry_t) entry = compileGrammarEntry((*rules)[i]);
        if(!entry.valid) {
            fprintf(stderr, "Failed to compile grammar entry %zu: '%s'\n", 
                    i, (*rules)[i].at);
            destroy_dynarray(gram.entry);
            return (option(grammar_t)) none;
        }
        dynarray_append(gram.entry, entry.value);
    }
    
    return (option(grammar_t)) some(gram);
}

option(size_t) findGrammarEntry(grammar_t *gram, string *name) {
    for(size_t i = 0; i < gram->entry.count; i++) {
        if(stringeql(gram->entry.at[i].name, *name)) {
            return (option(size_t)) some(i);
        }
    }
    return (option(size_t)) none;
}

static bool linkRuleNode(rule_node_t *node, grammar_t *gram);

bool linkRule(rule_t *rule, grammar_t *gram) {
    if(rule->literal_or_rule == is_rule) {
        option(size_t) idx = findGrammarEntry(gram, &rule->rule_name);
        if(!idx.valid) {
            fprintf(stderr, "Linking failed: unknown rule '%s'\n", 
                    rule->rule_name.at);
            return false;
        }
        rule->ge = &gram->entry.at[idx.value];
    } else if(rule->literal_or_rule == is_inline) {
        for(size_t i = 0; i < rule->inl.element.count; i++) {
            if(!linkRuleNode(&rule->inl.element.at[i], gram)) {
                return false;
            }
        }
    }
    return true;
}

static bool linkRuleNode(rule_node_t *node, grammar_t *gram) {
    if(node->alternative_or_regular == is_regular) {
        return linkRule(&node->rule, gram);
    } else {
        for(size_t i = 0; i < node->alternative.count; i++) {
            rule_sequence_t *seq = &node->alternative.at[i];
            for(size_t j = 0; j < seq->count; j++) {
                if(!linkRule(&seq->at[j], gram)) {
                    return false;
                }
            }
        }
        return true;
    }
}

bool linkGrammar(grammar_t *gram) {
    if(!gram) return false;
    
    for(size_t i = 0; i < gram->entry.count; i++) {
        grammar_entry_t *entry = &gram->entry.at[i];
        
        for(size_t j = 0; j < entry->element.count; j++) {
            if(!linkRuleNode(&entry->element.at[j], gram)) {
                return false;
            }
        }
    }
    
    return true;
}

static void destroyRule(rule_t *rule) {
    if(rule->storage_key.valid) {
        destroyString(rule->storage_key.value);
    }
    if(rule->literal_or_rule == is_literal) {
        destroyString(rule->literal);
    } else if(rule->literal_or_rule == is_rule) {
        destroyString(rule->rule_name);
    } else {
        destroyRuleBody(rule->inl.element);
    }
}

static void destroyRuleNode(rule_node_t *node) {
    if(node->alternative_or_regular == has_alternative) {
        for(size_t i = 0; i < node->alternative.count; i++) {
            rule_sequence_t seq = node->alternative.at[i];
            for(size_t j = 0; j < seq.count; j++) {
                destroyRule(&seq.at[j]);
            }
            destroy_dynarray(seq);
        }
        destroy_dynarray(node->alternative);
    } else {
        destroyRule(&node->rule);
    }
}

static void destroyRuleBody(rule_node_seq_t body) {
    for(size_t i = 0; i < body.count; i++) {
        destroyRuleNode(&body.at[i]);
    }
    destroy_dynarray(body);
}

void destroyGrammar(grammar_t *gram) {
    if(!gram) return;
    
    for(size_t i = 0; i < gram->entry.count; i++) {
        grammar_entry_t *entry = &gram->entry.at[i];
        destroyString(entry->name);
        destroyRuleBody(entry->element);
    }
    destroy_dynarray(gram->entry);
}

static option(obj_t_value_t) executeRule(iterstring_t *is, rule_t *rule, grammar_t *gram);
static option(obj_t_value_t) executeGrammarEntry(iterstring_t *is, grammar_entry_t *entry, grammar_t *gram);

static bool parseLiteralFromInput(iterstring_t *is, string literal) {
    size_t lit_len = stringlen(literal);
    
    for(size_t i = 0; i < lit_len; i++) {
        if(is->str.at[is->index + i] == '\0' || 
           is->str.at[is->index + i] != literal.at[i]) {
            return false;
        }
    }
    
    is->index += lit_len;
    iterstringAdvance(is);
    return true;
}

static string flattenToString(obj_t_value_t val) {
    switch(val.discriminant) {
        case obj_t_string:
            return stringFromString(val.str);
            
        case obj_t_array: {
            string result = string("");
            for(size_t i = 0; i < val.arr.count; i++) {
                string part = flattenToString(val.arr.array[i]);
                result = stringAppendString(result, part);
                destroyString(part);
            }
            return result;
        }
        
        case obj_t_obj: {
            string result = string("");
            for(size_t i = 0; i < val.obj.count; i++) {
                string part = flattenToString(val.obj.value[i]);
                result = stringAppendString(result, part);
                destroyString(part);
            }
            return result;
        }
        
        case obj_t_null:
        case obj_t_true:
        case obj_t_false:
        case obj_t_number:
        default:
            return string("");
    }
}

#include "../include/chad/macros/foreach.h"

void printValue(obj_t_value_t val) {
    if (val.discriminant == obj_t_string) {
        printf("\"%s\"", val.str);
    } else if (val.discriminant == obj_t_array) {
        printf("[");
        for (size_t i = 0; i < val.arr.count; i++) {
            printValue(val.arr.array[i]); // Recursive call
            if (i < val.arr.count - 1) printf(", ");
        }
        printf("]");
    }
}

static option(obj_t_value_t) executeBody(iterstring_t *is, rule_type_t rule_type, rule_node_seq_t *body, grammar_t *gram);

static option(obj_t_value_t) executeRuleBodyWithModifiers(iterstring_t *is, type_modifier_t mod,
                                                           rule_type_t rule_type, rule_node_seq_t *body,
                                                           grammar_t *gram) {
    if(mod & modifier_array) {
        array_t arr = createEmptyArray();

        option(obj_t_value_t) first = executeBody(is, rule_type, body, gram);
        if(!first.valid) {
            if(mod & modifier_optional) {
                // []? : zero matches is fine, yield an empty array
                obj_t_value_t ret = {
                    .discriminant = obj_t_array,
                    .arr = arr
                };
                return (option(obj_t_value_t)) some(ret);
            }
            destroyArray(arr);
            return (option(obj_t_value_t)) none;
        }

        arr = insertIntoArray(arr, first.value);

        while(1) {
            size_t save_pos = is->index;

            option(obj_t_value_t) next = executeBody(is, rule_type, body, gram);
            if(!next.valid) {
                is->index = save_pos;
                break;
            }
            arr = insertIntoArray(arr, next.value);
        }

        obj_t_value_t ret = {
            .discriminant = obj_t_array,
            .arr = arr
        };
        return (option(obj_t_value_t)) some(ret);

    } else if(mod & modifier_optional) {
        size_t save_pos = is->index;
        option(obj_t_value_t) opt = executeBody(is, rule_type, body, gram);
        if(!opt.valid) {
            is->index = save_pos;
            obj_t_value_t ret = {
                .discriminant = obj_t_null
            };
            return (option(obj_t_value_t)) some(ret);
        }
        return opt;

    } else {
        return executeBody(is, rule_type, body, gram);
    }
}

static option(obj_t_value_t) executeRule(iterstring_t *is, rule_t *rule, grammar_t *gram) {
    if(rule->literal_or_rule == is_literal) {
        if(!parseLiteralFromInput(is, rule->literal)) {
            return (option(obj_t_value_t)) none;
        }
        
        obj_t_value_t ret = {
            .discriminant = obj_t_string,
            .str = stringFromString(rule->literal)
        };
        return (option(obj_t_value_t)) some(ret);
    } else if(rule->literal_or_rule == is_rule) {
        if(!rule->ge) {
            fprintf(stderr, "Unlinked rule reference\n");
            return (option(obj_t_value_t)) none;
        }
        return executeRuleBodyWithModifiers(is, rule->type_mod, rule->ge->rule_type, &rule->ge->element, gram);
    } else {
        return executeRuleBodyWithModifiers(is, rule->type_mod, rule->inl.rule_type, &rule->inl.element, gram);
    }
}

// executes a single rule_t within a body and folds its result into the
// running accumulator: concatenated (flattened) for implicit_storage bodies,
// or inserted under its storage key for object_storage bodies
static bool executeElementInto(iterstring_t *is, rule_t *rule, grammar_t *gram,
                                rule_type_t rule_type, string *str_acc, object_t *obj_acc) {
    option(obj_t_value_t) val = executeRule(is, rule, gram);
    if(!val.valid) {
        return false;
    }

    if(rule_type == implicit_storage) {
        string part = flattenToString(val.value);
        *str_acc = stringAppendString(*str_acc, part);

        switch(val.value.discriminant) {
            case obj_t_string:
                destroyString(val.value.str);
            break;
            case obj_t_obj:
                destroyObject(val.value.obj);
            break;
            case obj_t_array:
                destroyArray(val.value.arr);
            break;
            default:
            // dc
            break;
        }
        destroyString(part);
    } else if(rule->storage_key.valid) {
        string key_copy = stringFromString(rule->storage_key.value);
        *obj_acc = insertObjectEntry(*obj_acc, key_copy, val.value);
    }
    return true;
}

static bool executeSequenceInto(iterstring_t *is, rule_sequence_t *seq, grammar_t *gram,
                                 rule_type_t rule_type, string *str_acc, object_t *obj_acc) {
    for(size_t i = 0; i < seq->count; i++) {
        if(!executeElementInto(is, &seq->at[i], gram, rule_type, str_acc, obj_acc)) {
            return false;
        }
    }
    return true;
}

// executes one element of a body: a regular node folds directly into the
// accumulator; an alternative node tries each low-binding alternative
// sequence in order (first match wins) and folds the whole winning
// sequence's results into the accumulator
static bool executeNodeInto(iterstring_t *is, rule_node_t *node, grammar_t *gram,
                             rule_type_t rule_type, string *str_acc, object_t *obj_acc) {
    if(node->alternative_or_regular == is_regular) {
        return executeElementInto(is, &node->rule, gram, rule_type, str_acc, obj_acc);
    }

    for(size_t i = 0; i < node->alternative.count; i++) {
        size_t save_pos = is->index;
        rule_sequence_t *seq = &node->alternative.at[i];

        string local_str = string("");
        object_t local_obj = createEmptyObject();

        if(executeSequenceInto(is, seq, gram, rule_type, &local_str, &local_obj)) {
            if(rule_type == implicit_storage) {
                *str_acc = stringAppendString(*str_acc, local_str);
            } else {
                for(size_t k = 0; k < local_obj.count; k++) {
                    *obj_acc = insertObjectEntry(*obj_acc, string(local_obj.key[k]),
                        obj_t_value_t_copy(local_obj.value[k]));
                }
            }
            destroyString(local_str);
            destroyObject(local_obj);
            return true;
        }

        destroyString(local_str);
        destroyObject(local_obj);
        is->index = save_pos;
    }

    return false;
}

// executes an entire rule body (a named grammar_entry_t or an inline rule),
// producing either a concatenated string (implicit_storage) or an object
// (object_storage) depending on rule_type
static option(obj_t_value_t) executeBody(iterstring_t *is, rule_type_t rule_type, rule_node_seq_t *body, grammar_t *gram) {
    string str_acc = string("");
    object_t obj_acc = createEmptyObject();

    for(size_t i = 0; i < body->count; i++) {
        if(!executeNodeInto(is, &body->at[i], gram, rule_type, &str_acc, &obj_acc)) {
            destroyString(str_acc);
            destroyObject(obj_acc);
            return (option(obj_t_value_t)) none;
        }
    }

    if(rule_type == implicit_storage) {
        destroyObject(obj_acc);
        obj_t_value_t ret = {
            .discriminant = obj_t_string,
            .str = str_acc
        };
        return (option(obj_t_value_t)) some(ret);
    } else {
        destroyString(str_acc);
        obj_t_value_t ret = {
            .discriminant = obj_t_obj,
            .obj = obj_acc
        };
        return (option(obj_t_value_t)) some(ret);
    }
}

static option(obj_t_value_t) executeGrammarEntry(iterstring_t *is, grammar_entry_t *entry, grammar_t *gram) {
    return executeBody(is, entry->rule_type, &entry->element, gram);
}

object_t parseIntoObject(object_t obj, string input, grammar_t *gram, string start_rule) {
    iterstring_t is = { .str = input, .index = 0, .previous = 0 };
    option(size_t) start_idx = findGrammarEntry(gram, &start_rule);

    if(!start_idx.valid) {
        fprintf(stderr, "Start rule '%s' not found in grammar\n", start_rule.at);
        return obj;
    }
    
    grammar_entry_t *start_entry = &gram->entry.at[start_idx.value];

    option(obj_t_value_t) result = executeGrammarEntry(&is, start_entry, gram);

    if(!result.valid) {
        fprintf(stderr, "Failed to parse input\n");
        return obj;
    }
    
    if(is.str.at[is.index] != '\0') {
        fprintf(stderr, "Warning: parsing succeeded but %zu characters remain at position %zu\n",
                stringlen(is.str) - is.index, is.index);
    }
    
    if(result.value.discriminant == obj_t_obj) {
        for(size_t i = 0; i < result.value.obj.count; i++) {
        	obj = insertObjectEntry(obj, string(result.value.obj.key[i]), 
            	obj_t_value_t_copy(result.value.obj.value[i]));
        }
        destroyObject(result.value.obj);
    } else {
        // For non-object results, just use the value directly without copying
        // (the strings are already properly managed by the parser)
        obj = insertObjectEntry(obj, string(start_rule), result.value);
    }
    return obj;
}

void printParsingMessage(FILE *stream, char *msg, string source, 
                        const char *const color, size_t color_start, size_t color_stop) {
    fprintf(stream, "%s\n", msg);
    assert((color_start < color_stop) && 
        (color_start < stringlen(source)) && 
        (color_stop < stringlen(source)));
    
	for(size_t i = 0; i < stringlen(source); i++) {
        if(i == color_start) {
            fprintf(stream, "%s", color);
        }
        fputc(source.at[i], stream);
        if(i == color_stop - 1) {
            fprintf(stream, "\033[0m");
        }
    }
    fputc('\n', stream);
}

