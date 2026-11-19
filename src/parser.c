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

// Phase 6 detour: parses an optional '>tablename' suffix, tried right after
// the type modifier (so 'rule[]>table', 'rule?>table', and plain
// 'rule>table' all work). Returns none if no '>' is present (leaves the
// position untouched) or if '>' isn't followed by a valid identifier
// (treated as "no suffix", not a hard parse error - matches this file's
// general lenient-backtracking style for optional suffixes).
static option(string) parseStoreTableSuffix(iterstring_t *rule) {
	if(rule->str.at[rule->index] != '>') {
		iterstringReset(rule);
		return (option(string)) none;
	}
	rule->index++;
	iterstringAdvance(rule);

	option(string) name = parseIdentifier(rule);
	if(!name.valid) {
		iterstringReset(rule);
		return (option(string)) none;
	}
	iterstringAdvance(rule);
	return name;
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
	size_t start = rule->index;

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

	// nothing was read. (this used to compare with rule->previous, which still holds the
	// end of the last identifier after a backtrack, so reading the same identifier again failed)
	if(rule->index == start) {
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

    // '!element': whatever follows is compiled as usual and then marked
    if(rule->str.at[rule->index] == '!') {
        rule->index++;
        iterstringAdvance(rule);
        option(rule_t) negated = compileRule(rule);
        if(negated.valid) {
            negated.value.negated = true;
        }
        return negated;
    }

    option(string) res;

    // Phase 6 detour: '{table}' - a new atomic primitive (same binding
    // tightness as a literal or bare rule reference), matches iff the next
    // input token is currently visible in symbol table `table`. Tried
    // before parseLiteral()/'(' so it doesn't need to disambiguate against
    // them (neither starts with '{').
    if(rule->str.at[rule->index] == '{') {
        size_t save = rule->index;
        rule->index++;
        option(string) table_name = parseIdentifier(rule);
        parseWhitespace(rule);
        if(table_name.valid && rule->str.at[rule->index] == '}') {
            rule->index++;
            iterstringAdvance(rule);
            rule_t ret = {
                .storage_key = none,
                .store_table = none,
                .literal_or_rule = is_table_lookup,
                .table_lookup_name = table_name.value,
                .type_mod = parseTypeModifier(rule),
            };
            return (option(rule_t)) some(ret);
        }
        if(table_name.valid) destroyString(table_name.value);
        rule->index = save;
    }
    
    if((res = parseLiteral(rule)).valid) {
        type_modifier_t mod = parseTypeModifier(rule);
        option(string) store_table = parseStoreTableSuffix(rule);
        rule_t ret = {
            .storage_key = none,
            .store_table = store_table,
            .literal_or_rule = is_literal,
            .literal = res.value,
            .type_mod = mod,		
        };
        return (option(rule_t)) some(ret);
    }

    if(rule->str.at[rule->index] == '(') {
        option(inline_rule_t) inl = compileInlineRule(rule);
        if(!inl.valid) {
            return (option(rule_t)) none;
        }
        type_modifier_t mod = parseTypeModifier(rule);
        option(string) store_table = parseStoreTableSuffix(rule);
        rule_t ret = {
            .storage_key = none,
            .store_table = store_table,
            .literal_or_rule = is_inline,
            .inl = inl.value,
            .type_mod = mod,
        };
        return (option(rule_t)) some(ret);
    }
    
    if((res = parseIdentifier(rule)).valid) {
        parseWhitespace(rule);
        
        if(rule->str.at[rule->index] == ':') {
            rule->index++;
            iterstringAdvance(rule);
            parseWhitespace(rule);

            // Phase 6 detour: 'key:{table}' - a keyed lookup, e.g.
            // typename:{generics}.
            if(rule->str.at[rule->index] == '{') {
                size_t save = rule->index;
                rule->index++;
                option(string) table_name = parseIdentifier(rule);
                parseWhitespace(rule);
                if(table_name.valid && rule->str.at[rule->index] == '}') {
                    rule->index++;
                    iterstringAdvance(rule);
                    rule_t ret = {
                        .type_mod = parseTypeModifier(rule),
                        .storage_key = res,
                        .store_table = none,
                        .literal_or_rule = is_table_lookup,
                        .table_lookup_name = table_name.value,
                    };
                    return (option(rule_t)) some(ret);
                }
                if(table_name.valid) destroyString(table_name.value);
                rule->index = save;
            }

            if(rule->str.at[rule->index] == '(') {
                option(inline_rule_t) inl = compileInlineRule(rule);
                if(inl.valid) {
                    type_modifier_t mod = parseTypeModifier(rule);
                    option(string) store_table = parseStoreTableSuffix(rule);
                    rule_t ret = {
                        .type_mod = mod,
                        .storage_key = res,
                        .store_table = store_table,
                        .literal_or_rule = is_inline,
                        .inl = inl.value,
                    };
                    return (option(rule_t)) some(ret);
                }
                destroyString(res.value);
                return (option(rule_t)) none;
            }

			// 'key:@' stores where in the input this point is, as a number
			if(rule->str.at[rule->index] == '@') {
				rule->index++;
				iterstringAdvance(rule);
				rule_t ret = {
					.type_mod = modifier_none,
					.storage_key = res,
					.store_table = none,
					.literal_or_rule = is_position,
				};
				return (option(rule_t)) some(ret);
			}

			option(string) rule_name = parseIdentifier(rule);
			if(rule_name.valid) {
				type_modifier_t mod = parseTypeModifier(rule);
				option(string) store_table = parseStoreTableSuffix(rule);
				rule_t ret = {
					.type_mod = mod,
					.storage_key = res,
					.store_table = store_table,
					.literal_or_rule = is_rule,
					.rule_name = rule_name.value,
					.ge = NULL,
				};
				return (option(rule_t)) some(ret);
			}

			option(string) literal = parseLiteral(rule);
			if(literal.valid) {
				type_modifier_t mod = parseTypeModifier(rule);
				option(string) store_table = parseStoreTableSuffix(rule);
				rule_t ret = {
					.type_mod = mod,
					.storage_key = res,
					.store_table = store_table,
					.literal_or_rule = is_literal,
					.literal = literal.value,	
				};
				return (option(rule_t)) some(ret);
			}
			destroyString(res.value);
			return (option(rule_t)) none;
        } else {
            type_modifier_t mod = parseTypeModifier(rule);
            option(string) store_table = parseStoreTableSuffix(rule);
            rule_t ret = {
                .storage_key = none,
                .store_table = store_table,
                .literal_or_rule = is_rule,
                .rule_name = res.value,
                .ge = NULL,
                .type_mod = mod,
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
    grammar_t gram = { 0 };
    gram.entry = create_dynarray(grammar_entry_t);
    // Phase 6 detour: one root frame always present while a grammar_t is
    // valid, so top-level '>table'/'{table}' usage (outside any
    // alternative/repetition) has somewhere to write/read immediately.
    gram.symbol_frames = create_dynarray(symbol_frame_t);
    symbol_frame_t root_frame = create_dynarray(symbol_entry_t);
    dynarray_append(gram.symbol_frames, root_frame);
    
    for(size_t i = 0; i < count; i++) {
        option(grammar_entry_t) entry = compileGrammarEntry((*rules)[i]);
        if(!entry.valid) {
            fprintf(stderr, "Failed to compile grammar entry %zu: '%s'\n", 
                    i, (*rules)[i].at);
            destroy_dynarray(gram.entry);
            destroy_dynarray(gram.symbol_frames);
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
    if(rule->store_table.valid) {
        destroyString(rule->store_table.value);
    }
    if(rule->literal_or_rule == is_literal) {
        destroyString(rule->literal);
    } else if(rule->literal_or_rule == is_rule) {
        destroyString(rule->rule_name);
    } else if(rule->literal_or_rule == is_table_lookup) {
        destroyString(rule->table_lookup_name);
    } else if(rule->literal_or_rule == is_position) {
        // nothing to free
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

    // Phase 6 detour: tear down whatever frames remain (normally just the
    // one root frame, reset empty by the last parseIntoObject() call).
    for(size_t i = 0; i < gram->symbol_frames.count; i++) {
        symbol_frame_t *frame = &gram->symbol_frames.at[i];
        for(size_t j = 0; j < frame->count; j++) {
            destroyString(frame->at[j].table);
            destroyString(frame->at[j].key);
        }
        destroy_dynarray((*frame));
    }
    destroy_dynarray(gram->symbol_frames);
    destroy_dynarray(gram->trace_stack);
    if(gram->trace_scratch.at != NULL) {
        destroyString(gram->trace_scratch);
    }
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
        printf("\"%s\"", val.str.at);
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

// Phase 6 detour: transactional scoped symbol table helpers (symbol_frame_t
// declared in parser.h). Called in lockstep with the 4 existing is->index
// save/restore sites below, so a failed PEG attempt's '>table' stores are
// discarded exactly when its position rollback happens ("vanish"), and a
// successful attempt's stores become visible to its immediate parent
// attempt exactly when its position "commit" (no rollback) happens
// ("materialize" - append onto the new top frame, NOT the root, so nested
// alternatives only become permanently visible once every enclosing
// attempt, up to the top-level rule invocation, has also succeeded).
// --- the trace ---
//
// see parserTraceTo() in parser.h for what a line looks like. everything
// here only exists with -DENABLE_PARSER_TRACE. without it, trace(...) is
// nothing at all.

#ifdef ENABLE_PARSER_TRACE

#define TRACE_LOOKAHEAD 24

static void traceJsonString(FILE *stream, const char *text, size_t max_length) {
    size_t length = 0;
    while(length < max_length && text[length] != '\0') {
        length++;
    }
    string raw = stringSliceFromCharPtr(text, 0, length);
    string escaped = stringEscapeJson(raw);
    fprintf(stream, "\"%s\"", escaped.at);
    destroyString(raw);
    destroyString(escaped);
}

static char *traceStateOf(const char *event) {
    struct { char *event; char *state; } states[] = {
        { "begin", "starting" }, { "enter", "descending" }, { "match", "matched" }, { "fail", "failed" },
        { "literal", "consuming" }, { "backtrack", "backtracking" }, { "lookup", "looking_up" },
        { "store", "storing" }, { "frame_push", "attempting" }, { "frame_commit", "committing" },
        { "frame_rollback", "rolling_back" }, { "end", "finished" },
    };
    for(size_t i = 0; i < sizeof(states) / sizeof(states[0]); i++) {
        if(strcmp(states[i].event, event) == 0) {
            return states[i].state;
        }
    }
    return "unknown";
}

// writes one line. `extra` is the event's own fields as JSON text that
// starts with a comma (or is empty), e.g. `,"from":3,"to":1`
static void traceEvent(grammar_t *gram, iterstring_t *is, const char *event, const char *extra_fmt, ...) {
    if(gram->trace == NULL) {
        return;
    }
    FILE *out = gram->trace;
    fprintf(out, "{\"seq\":%zu,\"event\":\"%s\",\"state\":\"%s\"", gram->trace_sequence++, event, traceStateOf(event));

    fprintf(out, ",\"rule\":");
    traceJsonString(out, gram->trace_stack.count > 0 ? gram->trace_stack.at[gram->trace_stack.count - 1].at : "", SIZE_MAX);
    fprintf(out, ",\"depth\":%zu,\"stack\":[", gram->trace_stack.count);
    for(size_t i = 0; i < gram->trace_stack.count; i++) {
        if(i > 0) fputc(',', out);
        traceJsonString(out, gram->trace_stack.at[i].at, SIZE_MAX);
    }
    fprintf(out, "],\"cursor\":%zu,\"lookahead\":", is->index);
    traceJsonString(out, is->str.at + is->index, TRACE_LOOKAHEAD);

    fprintf(out, ",\"tables\":{\"frames\":[");
    for(size_t f = 0; f < gram->symbol_frames.count; f++) {
        symbol_frame_t *frame = &gram->symbol_frames.at[f];
        fprintf(out, f > 0 ? ",[" : "[");
        for(size_t i = 0; i < frame->count; i++) {
            fprintf(out, i > 0 ? ",{\"table\":" : "{\"table\":");
            traceJsonString(out, frame->at[i].table.at, SIZE_MAX);
            fprintf(out, ",\"key\":");
            traceJsonString(out, frame->at[i].key.at, SIZE_MAX);
            fputc('}', out);
        }
        fputc(']', out);
    }
    fprintf(out, "]}");

    va_list args;
    va_start(args, extra_fmt);
    vfprintf(out, extra_fmt, args);
    va_end(args);
    fprintf(out, "}\n");
}

// a string that goes into a traced field has to be escaped first. the result
// lives until the next call, which is long enough to print it.
static const char *traceEscapedIn(grammar_t *gram, const char *text) {
    if(gram->trace_scratch.at != NULL) {
        destroyString(gram->trace_scratch);
    }
    string raw = stringFromCharPtr(text);
    gram->trace_scratch = stringEscapeJson(raw);
    destroyString(raw);
    return gram->trace_scratch.at;
}

#define traceEscaped(text) traceEscapedIn(gram, text)
#define trace(gram, is, event, ...) traceEvent(gram, is, event, __VA_ARGS__)
#define traceEnterRule(gram, name) dynarray_append((gram)->trace_stack, name)
#define traceLeaveRule(gram) ((gram)->trace_stack.count--)

void parserTraceTo(grammar_t *gram, FILE *stream) {
    gram->trace = stream;
}

#else

#define trace(gram, is, event, ...) ((void) 0)
#define traceEnterRule(gram, name) ((void) 0)
#define traceLeaveRule(gram) ((void) 0)

void parserTraceTo(grammar_t *gram, FILE *stream) {
    (void) gram;
    (void) stream;
}

#endif

// the rules of the symbol tables, as the sentences the trace quotes
#define STORAGE_RULE_PUSH "an attempt begins (an alternative, a repetition or an optional): what it stores goes into a frame of its own"
#define STORAGE_RULE_COMMIT "the attempt succeeded: what it stored moves into the enclosing frame"
#define STORAGE_RULE_ROLLBACK "the attempt failed: what it stored is discarded, as if it had never been stored"
#define STORAGE_RULE_STORE "rule>table: the matched text is recorded in the innermost frame, it only stays if every enclosing attempt succeeds"
#define STORAGE_RULE_LOOKUP "{table}: an identifier only matches if it is visible in some frame, the innermost frame is asked first"

static void pushSymbolFrame(grammar_t *gram, iterstring_t *is) {
    symbol_frame_t frame = create_dynarray(symbol_entry_t);
    dynarray_append(gram->symbol_frames, frame);
    trace(gram, is, "frame_push", ",\"frame\":%zu,\"entries\":0,\"storage_rule\":\"%s\"",
        gram->symbol_frames.count - 1, STORAGE_RULE_PUSH);
}

static void vanishSymbolFrame(grammar_t *gram, iterstring_t *is) {
    symbol_frame_t frame = gram->symbol_frames.at[gram->symbol_frames.count - 1];
    trace(gram, is, "frame_rollback", ",\"frame\":%zu,\"entries\":%zu,\"storage_rule\":\"%s\"",
        gram->symbol_frames.count - 1, frame.count, STORAGE_RULE_ROLLBACK);
    gram->symbol_frames.count--;
    for(size_t i = 0; i < frame.count; i++) {
        destroyString(frame.at[i].table);
        destroyString(frame.at[i].key);
    }
    destroy_dynarray(frame);
}

static void materializeSymbolFrame(grammar_t *gram, iterstring_t *is) {
    symbol_frame_t frame = gram->symbol_frames.at[gram->symbol_frames.count - 1];
    trace(gram, is, "frame_commit", ",\"frame\":%zu,\"entries\":%zu,\"storage_rule\":\"%s\"",
        gram->symbol_frames.count - 1, frame.count, STORAGE_RULE_COMMIT);
    gram->symbol_frames.count--;
    symbol_frame_t *parent = &gram->symbol_frames.at[gram->symbol_frames.count - 1];
    for(size_t i = 0; i < frame.count; i++) {
        // ownership of the entry's strings transfers to the parent frame -
        // no copy/destroy needed here.
        dynarray_append((*parent), frame.at[i]);
    }
    destroy_dynarray(frame);
}

static void storeSymbol(grammar_t *gram, iterstring_t *is, string table, string matched_text) {
    symbol_frame_t *top = &gram->symbol_frames.at[gram->symbol_frames.count - 1];
    symbol_entry_t entry = {
        .table = stringFromString(table),
        .key = stringFromString(matched_text),
    };
    dynarray_append((*top), entry);
    trace(gram, is, "store", ",\"table\":\"%s\",\"key\":\"%s\",\"frame\":%zu,\"storage_rule\":\"%s\"",
        table.at, traceEscaped(matched_text.at), gram->symbol_frames.count - 1, STORAGE_RULE_STORE);
}

// Scans every visible frame, innermost (top of stack) first, down to the
// committed root - so a rule can see its own not-yet-fully-materialized
// stores from earlier in the same in-flight attempt.
static bool lookupSymbol(grammar_t *gram, iterstring_t *is, string table, const char *text) {
    for(size_t f = gram->symbol_frames.count; f-- > 0;) {
        symbol_frame_t *frame = &gram->symbol_frames.at[f];
        for(size_t i = 0; i < frame->count; i++) {
            if(stringeql(frame->at[i].table, table) && strcmp(frame->at[i].key.at, text) == 0) {
                trace(gram, is, "lookup", ",\"table\":\"%s\",\"key\":\"%s\",\"found\":true,\"frame\":%zu,\"storage_rule\":\"%s\"",
                    table.at, traceEscaped(text), f, STORAGE_RULE_LOOKUP);
                return true;
            }
        }
    }
    trace(gram, is, "lookup", ",\"table\":\"%s\",\"key\":\"%s\",\"found\":false,\"storage_rule\":\"%s\"",
        table.at, traceEscaped(text), STORAGE_RULE_LOOKUP);
    return false;
}

static option(obj_t_value_t) executeRuleBodyWithModifiers(iterstring_t *is, type_modifier_t mod,
                                                           rule_type_t rule_type, rule_node_seq_t *body,
                                                           grammar_t *gram) {
    if(mod & modifier_array) {
        array_t arr = createEmptyArray();

        size_t first_save_pos = is->index;
        pushSymbolFrame(gram, is);
        option(obj_t_value_t) first = executeBody(is, rule_type, body, gram);
        if(!first.valid) {
            // Restore position: a failed attempt may have partially
            // consumed input before failing (e.g. matched an operator
            // token but then failed on its required operand), and that
            // partial consumption must not leak out of a zero-or-more
            // ([]?) repetition that ultimately matched zero times.
            trace(gram, is, "backtrack", ",\"from\":%zu,\"to\":%zu", is->index, first_save_pos);
            is->index = first_save_pos;
            vanishSymbolFrame(gram, is);
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
        materializeSymbolFrame(gram, is);

        arr = insertIntoArray(arr, first.value);

        while(1) {
            size_t save_pos = is->index;
            pushSymbolFrame(gram, is);

            option(obj_t_value_t) next = executeBody(is, rule_type, body, gram);
            if(!next.valid) {
                trace(gram, is, "backtrack", ",\"from\":%zu,\"to\":%zu", is->index, save_pos);
                is->index = save_pos;
                vanishSymbolFrame(gram, is);
                break;
            }
            materializeSymbolFrame(gram, is);
            arr = insertIntoArray(arr, next.value);
        }

        obj_t_value_t ret = {
            .discriminant = obj_t_array,
            .arr = arr
        };
        return (option(obj_t_value_t)) some(ret);

    } else if(mod & modifier_optional) {
        size_t save_pos = is->index;
        pushSymbolFrame(gram, is);
        option(obj_t_value_t) opt = executeBody(is, rule_type, body, gram);
        if(!opt.valid) {
            trace(gram, is, "backtrack", ",\"from\":%zu,\"to\":%zu", is->index, save_pos);
            is->index = save_pos;
            vanishSymbolFrame(gram, is);
            obj_t_value_t ret = {
                .discriminant = obj_t_null
            };
            return (option(obj_t_value_t)) some(ret);
        }
        materializeSymbolFrame(gram, is);
        return opt;

    } else {
        return executeBody(is, rule_type, body, gram);
    }
}

static option(obj_t_value_t) executeRuleInner(iterstring_t *is, rule_t *rule, grammar_t *gram) {
    if(rule->literal_or_rule == is_literal) {
        if(!parseLiteralFromInput(is, rule->literal)) {
            return (option(obj_t_value_t)) none;
        }
        if(is->index > gram->last_parse.furthest) {
            gram->last_parse.furthest = is->index;
        }
        trace(gram, is, "literal", ",\"text\":\"%s\"", traceEscaped(rule->literal.at));
        
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
        [[maybe_unused]] size_t start = is->index;
        traceEnterRule(gram, rule->rule_name);
        trace(gram, is, "enter", "%s", "");
        option(obj_t_value_t) result = executeRuleBodyWithModifiers(is, rule->type_mod, rule->ge->rule_type, &rule->ge->element, gram);
        trace(gram, is, result.valid ? "match" : "fail", ",\"start\":%zu", start);
        traceLeaveRule(gram);
        return result;
    } else if(rule->literal_or_rule == is_position) {
        return (option(obj_t_value_t)) some(((obj_t_value_t) { .discriminant = obj_t_number, .num = makeNumber((uint64_t) is->index) }));
    } else if(rule->literal_or_rule == is_table_lookup) {
        // Phase 6 detour: '{table}' - matches an identifier-shaped token
        // from the input iff its text is currently visible in `table`.
        // parseIdentifier() is reused here even though it's also used at
        // grammar-COMPILE time - it operates generically on any
        // iterstring_t, so it's equally valid for matching a token out of
        // the INPUT text being parsed.
        size_t save_pos = is->index;
        option(string) matched = parseIdentifier(is);
        if(!matched.valid) {
            return (option(obj_t_value_t)) none;
        }
        if(!lookupSymbol(gram, is, rule->table_lookup_name, matched.value.at)) {
            destroyString(matched.value);
            trace(gram, is, "backtrack", ",\"from\":%zu,\"to\":%zu", is->index, save_pos);
            is->index = save_pos;
            return (option(obj_t_value_t)) none;
        }
        obj_t_value_t ret = {
            .discriminant = obj_t_string,
            .str = matched.value,
        };
        return (option(obj_t_value_t)) some(ret);
    } else {
        return executeRuleBodyWithModifiers(is, rule->type_mod, rule->inl.rule_type, &rule->inl.element, gram);
    }
}

static void destroyValue(obj_t_value_t val) {
    switch(val.discriminant) {
        case obj_t_string:
            destroyString(val.str);
        break;
        case obj_t_obj:
            destroyObject(val.obj);
        break;
        case obj_t_array:
            destroyArray(val.arr);
        break;
        default:
        // nothing to destroy
        break;
    }
}

// Wraps executeRuleInner() with the 'rule>table' store suffix: on a
// successful match, the matched result is flattened to text and recorded
// into `rule->store_table` (in the current, innermost transaction frame) -
// see pushSymbolFrame/vanishSymbolFrame/materializeSymbolFrame above for
// how that store is rolled back if the ENCLOSING attempt ultimately fails.
static option(obj_t_value_t) executeRule(iterstring_t *is, rule_t *rule, grammar_t *gram) {
    if(rule->negated) {
        // try the element only to see whether it would match: the position
        // is put back and whatever it stored is thrown away
        size_t save_pos = is->index;
        pushSymbolFrame(gram, is);
        option(obj_t_value_t) attempt = executeRuleInner(is, rule, gram);
        vanishSymbolFrame(gram, is);
        is->index = save_pos;
        if(attempt.valid) {
            destroyValue(attempt.value);
            return (option(obj_t_value_t)) none;
        }
        obj_t_value_t nothing = { .discriminant = obj_t_string, .str = string("") };
        return (option(obj_t_value_t)) some(nothing);
    }
    option(obj_t_value_t) result = executeRuleInner(is, rule, gram);
    if(result.valid && rule->store_table.valid) {
        string text = flattenToString(result.value);
        storeSymbol(gram, is, rule->store_table.value, text);
        destroyString(text);
    }
    return result;
}

// executes a single rule_t within a body and folds its result into the
// running accumulator: concatenated (flattened) for implicit_storage bodies,
// or inserted under its storage key for object_storage bodies
static bool executeElementInto(iterstring_t *is, rule_t *rule, grammar_t *gram,
                                rule_type_t rule_type, string *str_acc, object_t *obj_acc) {
    // a literal is by far the most common element, and most of them are not kept
    // (or are only appended to text): no value is made for them unless one is stored
    if(rule->literal_or_rule == is_literal && !rule->negated && !rule->store_table.valid) {
        if(!parseLiteralFromInput(is, rule->literal)) {
            return false;
        }
        if(is->index > gram->last_parse.furthest) {
            gram->last_parse.furthest = is->index;
        }
        trace(gram, is, "literal", ",\"text\":\"%s\"", traceEscaped(rule->literal.at));
        if(rule_type == implicit_storage) {
            *str_acc = stringAppendString(*str_acc, rule->literal);
        } else if(rule->storage_key.valid) {
            *obj_acc = insertObjectEntry(*obj_acc, stringFromString(rule->storage_key.value),
                (obj_t_value_t) { .discriminant = obj_t_string, .str = stringFromString(rule->literal) });
        }
        return true;
    }
    option(obj_t_value_t) val = executeRule(is, rule, gram);
    if(!val.valid) {
        return false;
    }

    if(rule_type == implicit_storage) {
        string part = flattenToString(val.value);
        *str_acc = stringAppendString(*str_acc, part);

        destroyValue(val.value);
        destroyString(part);
    } else if(rule->storage_key.valid) {
        string key_copy = stringFromString(rule->storage_key.value);
        *obj_acc = insertObjectEntry(*obj_acc, key_copy, val.value);
    } else {
        // an element without a storage key is executed, its output is discarded
        destroyValue(val.value);
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
        pushSymbolFrame(gram, is);
        rule_sequence_t *seq = &node->alternative.at[i];

        // only the accumulator this kind of rule fills is made: this runs for every
        // alternative that is tried, and most of them fail
        bool collects_text = rule_type == implicit_storage;
        // a text that gets nothing appended stays unallocated until it is needed
        string local_str = { 0 };
        object_t local_obj = { 0 };
        if(!collects_text) {
            local_obj = createEmptyObject();
        }

        if(executeSequenceInto(is, seq, gram, rule_type, &local_str, &local_obj)) {
            materializeSymbolFrame(gram, is);
            if(collects_text) {
                if(local_str.data) {
                    *str_acc = stringAppendString(*str_acc, local_str);
                    destroyString(local_str);
                }
            } else {
                // what the alternative collected moves over, it is not copied
                for(size_t k = 0; k < local_obj.count; k++) {
                    *obj_acc = insertObjectEntry(*obj_acc, local_obj.key[k], local_obj.value[k]);
                }
                free(local_obj.key);
                free(local_obj.value);
            }
            return true;
        }

        vanishSymbolFrame(gram, is);
        if(collects_text) {
            if(local_str.data) {
                destroyString(local_str);
            }
        } else {
            destroyObject(local_obj);
        }
        trace(gram, is, "backtrack", ",\"from\":%zu,\"to\":%zu", is->index, save_pos);
        is->index = save_pos;
    }

    return false;
}

// executes an entire rule body (a named grammar_entry_t or an inline rule),
// producing either a concatenated string (implicit_storage) or an object
// (object_storage) depending on rule_type
static option(obj_t_value_t) executeBody(iterstring_t *is, rule_type_t rule_type, rule_node_seq_t *body, grammar_t *gram) {
    // a rule collects either text or an object, never both
    bool collects_text = rule_type == implicit_storage;
    string str_acc = { 0 }; // stays unallocated until something is appended
    object_t obj_acc = { 0 };
    if(!collects_text) {
        obj_acc = createEmptyObject();
    }

    for(size_t i = 0; i < body->count; i++) {
        if(!executeNodeInto(is, &body->at[i], gram, rule_type, &str_acc, &obj_acc)) {
            if(collects_text) {
                if(str_acc.data) {
                    destroyString(str_acc);
                }
            } else {
                destroyObject(obj_acc);
            }
            return (option(obj_t_value_t)) none;
        }
    }

    if(collects_text) {
        obj_t_value_t ret = {
            .discriminant = obj_t_string,
            .str = str_acc.data ? str_acc : string("")
        };
        return (option(obj_t_value_t)) some(ret);
    } else {
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
    gram->last_parse = (parse_report_t) { .complete = false, .furthest = 0 };
    option(size_t) start_idx = findGrammarEntry(gram, &start_rule);

    if(!start_idx.valid) {
        fprintf(stderr, "Start rule '%s' not found in grammar\n", start_rule.at);
        return obj;
    }

    // Phase 6 detour: reset the symbol-table frame stack back to a single
    // empty root frame at the start of every parse - so '{table}'/'>table'
    // usage doesn't leak entries from a PRIOR parseIntoObject() call that
    // reused this same compiled grammar_t.
    for(size_t i = 0; i < gram->symbol_frames.count; i++) {
        symbol_frame_t *frame = &gram->symbol_frames.at[i];
        for(size_t j = 0; j < frame->count; j++) {
            destroyString(frame->at[j].table);
            destroyString(frame->at[j].key);
        }
        destroy_dynarray((*frame));
    }
    dynarray_clear(gram->symbol_frames);
    symbol_frame_t root_frame = create_dynarray(symbol_entry_t);
    dynarray_append(gram->symbol_frames, root_frame);
    
    grammar_entry_t *start_entry = &gram->entry.at[start_idx.value];

    gram->trace_sequence = 0;
    dynarray_clear(gram->trace_stack);
    traceEnterRule(gram, start_entry->name);
    trace(gram, &is, "begin", ",\"start_rule\":\"%s\",\"input\":\"%s\"", start_rule.at, traceEscaped(input.at));

    option(obj_t_value_t) result = executeGrammarEntry(&is, start_entry, gram);

    gram->last_parse.complete = result.valid && is.str.at[is.index] == '\0';
    trace(gram, &is, "end", ",\"complete\":%s,\"furthest\":%zu",
        gram->last_parse.complete ? "true" : "false", gram->last_parse.furthest);
    traceLeaveRule(gram);

    // a failed or incomplete parse is not announced here, the caller reads
    // gram->last_parse and says what it wants to say
    if(!result.valid) {
        return obj;
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

