#include "../include/chad/parser.h"
#include "../include/chad/cstl.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

// Thread-local storage for anonymous rules generated during parsing
// Used by compileRule when it encounters grouped expressions: (...)
typedef struct {
    dynarray(grammar_entry_t) rules;
    size_t anonymous_rule_counter;
} anonymous_rules_context_t;

static anonymous_rules_context_t anon_context = {
    .rules = {0},
    .anonymous_rule_counter = 0
};

// Generate a unique anonymous rule name
static string generateAnonymousRuleName(void) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "_group_%zu", anon_context.anonymous_rule_counter++);
    return string(buffer);
}

// Forward declaration
option(rule_node_t) compileRuleNode(iterstring_t *rule);

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

option(rule_t) compileRule(iterstring_t *rule) {
    parseWhitespace(rule);
    
    // Check for grouped expression: (...)
    if(rule->str.at[rule->index] == '(') {
        rule->index++;  // consume '('
        iterstringAdvance(rule);
        parseWhitespace(rule);
        
        // Parse the full alternation inside parentheses
        option(rule_node_t) grouped_node = compileRuleNode(rule);
        
        if(!grouped_node.valid) {
            iterstringReset(rule);
            return (option(rule_t)) none;
        }
        
        parseWhitespace(rule);
        if(rule->str.at[rule->index] != ')') {
            iterstringReset(rule);
            return (option(rule_t)) none;
        }
        
        rule->index++;  // consume ')'
        iterstringAdvance(rule);
        
        // Create an anonymous rule entry for this grouped expression
        string anon_name = generateAnonymousRuleName();
        grammar_entry_t entry = {
            .name = anon_name,
            .element = grouped_node.value,
        };
        
        // Store the anonymous rule
        dynarray_append(anon_context.rules, entry);
        
        // Return a rule that references the anonymous rule
        rule_t ret = {
            .storage_key = none,
            .literal_or_rule = is_rule,
            .rule_name = anon_name,
            .ge = NULL,
            .type_mod = parseTypeModifier(rule),
        };
        return (option(rule_t)) some(ret);
    }
    
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
    
    if((res = parseIdentifier(rule)).valid) {
        parseWhitespace(rule);
        
        if(rule->str.at[rule->index] == ':') {
            rule->index++;
            iterstringAdvance(rule);
            parseWhitespace(rule);

            // Check for grouped expression after storage key: key:(...)
            if(rule->str.at[rule->index] == '(') {
                rule->index++;  // consume '('
                iterstringAdvance(rule);
                parseWhitespace(rule);
                
                // Parse the full alternation inside parentheses
                option(rule_node_t) grouped_node = compileRuleNode(rule);
                
                if(!grouped_node.valid) {
                    iterstringReset(rule);
                    return (option(rule_t)) none;
                }
                
                parseWhitespace(rule);
                if(rule->str.at[rule->index] != ')') {
                    iterstringReset(rule);
                    return (option(rule_t)) none;
                }
                
                rule->index++;  // consume ')'
                iterstringAdvance(rule);
                
                // Create an anonymous rule entry for this grouped expression
                string anon_name = generateAnonymousRuleName();
                grammar_entry_t entry = {
                    .name = anon_name,
                    .element = grouped_node.value,
                };
                
                // Store the anonymous rule
                dynarray_append(anon_context.rules, entry);
                
                // Return a rule with storage key that references the anonymous rule
                rule_t ret = {
                    .storage_key = res,
                    .literal_or_rule = is_rule,
                    .rule_name = anon_name,
                    .ge = NULL,
                    .type_mod = parseTypeModifier(rule),
                };
                return (option(rule_t)) some(ret);
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

// compileSequence: Parse a sequence of rules until we hit |, ), or end
// Returns array of rule_t forming one alternative sequence
option(rule_sequence_t) compileSequence(iterstring_t *rule) {
    rule_sequence_t sequence = create_dynarray(rule_t);
    
    parseWhitespace(rule);
    
    option(rule_t) elem;
    while((elem = compileRule(rule)).valid) {
        dynarray_append(sequence, elem.value);
        parseWhitespace(rule);
        
        // Stop if we hit alternation, close paren, or end
        if(rule->str.at[rule->index] == '|' || 
           rule->str.at[rule->index] == ')' ||
           rule->str.at[rule->index] == '\0') {
            break;
        }
    }
    
    if(sequence.count == 0) {
        destroy_dynarray(sequence);
        return (option(rule_sequence_t)) none;
    }
    
    return (option(rule_sequence_t)) some(sequence);
}

// compileRuleNode: Parse alternatives (lowest precedence)
// Collects sequences separated by |
// Returns rule_node_t with either single sequence or multiple alternatives
option(rule_node_t) compileRuleNode(iterstring_t *rule) {
    parseWhitespace(rule);
    
    // Parse first sequence
    option(rule_sequence_t) first_seq = compileSequence(rule);
    if(!first_seq.valid) {
        return (option(rule_node_t)) none;
    }
    
    parseWhitespace(rule);
    
    // Check if there are alternatives (multiple sequences separated by |)
    if(rule->str.at[rule->index] == '|') {
        // We have alternatives - collect all sequences
        rule_alternatives_t alternatives = create_dynarray(rule_sequence_t);
        dynarray_append(alternatives, first_seq.value);
        
        // Collect remaining alternatives
        while(isFollowedByAlternative(rule)) {
            option(rule_sequence_t) alt_seq = compileSequence(rule);
            if(!alt_seq.valid) {
                destroy_dynarray(alternatives);
                return (option(rule_node_t)) none;
            }
            dynarray_append(alternatives, alt_seq.value);
            parseWhitespace(rule);
        }
        
        rule_node_t node = {
            .sequence_or_alternative = is_alternative,
            .alternatives = alternatives
        };
        return (option(rule_node_t)) some(node);
    } else {
        // Just a single sequence
        rule_node_t node = {
            .sequence_or_alternative = is_sequence,
            .sequence = first_seq.value
        };
        return (option(rule_node_t)) some(node);
    }
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
    
    // Parse the rule body as ONE node (which handles all alternatives/sequences)
    option(rule_node_t) node = compileRuleNode(&it);
    if(!node.valid) {
        fprintf(stderr, "Rule '%s' has no valid content\n", ret.name.at);
        destroyString(ret.name);
        return (option(grammar_entry_t)) none;
    }
    
    // Check that we consumed the entire rule
    parseWhitespace(&it);
    if(it.str.at[it.index] != '\0') {
        fprintf(stderr, "Rule '%s' has unexpected trailing content at position %zu\n", ret.name.at, it.index);
        destroyString(ret.name);
        // TODO: properly destroy node
        return (option(grammar_entry_t)) none;
    }
    
    // Determine rule type and storage keys by analyzing the node
    bool has_storage_key = false;
    if(node.value.sequence_or_alternative == is_sequence) {
        // Single sequence - check for storage keys
        for(size_t i = 0; i < node.value.sequence.count; i++) {
            if(node.value.sequence.at[i].storage_key.valid) {
                has_storage_key = true;
                break;
            }
        }
    } else {
        // Alternatives - check all sequences for storage keys
        for(size_t alt_idx = 0; alt_idx < node.value.alternatives.count; alt_idx++) {
            for(size_t seq_idx = 0; seq_idx < node.value.alternatives.at[alt_idx].count; seq_idx++) {
                if(node.value.alternatives.at[alt_idx].at[seq_idx].storage_key.valid) {
                    has_storage_key = true;
                    break;
                }
            }
            if(has_storage_key) break;
        }
    }
    
    ret.element = node.value;
    ret.rule_type = has_storage_key ? object_storage : implicit_storage;
    
    return (option(grammar_entry_t)) some(ret);
}

option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]) {
    // Reset anonymous rule context for this compilation
    anon_context.anonymous_rule_counter = 0;
    if(anon_context.rules.at != NULL) {
        free(anon_context.rules.at);
    }
    anon_context.rules = create_dynarray(grammar_entry_t);
    
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
    
    // Append all anonymous rules that were created during parsing
    for(size_t i = 0; i < anon_context.rules.count; i++) {
        dynarray_append(gram.entry, anon_context.rules.at[i]);
    }
    
    // Clear the anonymous rules context (but don't free, as entries are now owned by grammar)
    anon_context.rules.count = 0;
    
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

bool linkRule(rule_t *rule, grammar_t *gram) {
    if(rule->literal_or_rule == is_rule) {
        option(size_t) idx = findGrammarEntry(gram, &rule->rule_name);
        if(!idx.valid) {
            fprintf(stderr, "Linking failed: unknown rule '%s'\n", 
                    rule->rule_name.at);
            return false;
        }
        rule->ge = &gram->entry.at[idx.value];
    }
    return true;
}

// Helper: Link all rules in a sequence
bool linkSequence(rule_sequence_t *sequence, grammar_t *gram) {
    for(size_t i = 0; i < sequence->count; i++) {
        if(!linkRule(&sequence->at[i], gram)) {
            return false;
        }
    }
    return true;
}

// Helper: Link all rules in a rule_node_t (which can be sequence or alternatives)
bool linkRuleNode(rule_node_t *node, grammar_t *gram) {
    if(node->sequence_or_alternative == is_sequence) {
        return linkSequence(&node->sequence, gram);
    } else {
        // Alternatives - link all sequences
        for(size_t i = 0; i < node->alternatives.count; i++) {
            if(!linkSequence(&node->alternatives.at[i], gram)) {
                return false;
            }
        }
        return true;
    }
}

bool linkGrammar(grammar_t *gram) {
    if(!gram) return false;
    
    for(size_t i = 0; i < gram->entry.count; i++) {
        grammar_entry_t *entry = &gram->entry.at[i];
        if(!linkRuleNode(&entry->element, gram)) {
            return false;
        }
    }
    
    return true;
}

void destroyRule(rule_t *rule) {
    if(rule->storage_key.valid) {
        destroyString(rule->storage_key.value);
    }
    if(rule->literal_or_rule == is_literal) {
        destroyString(rule->literal);
    } else {
        destroyString(rule->rule_name);
    }
}

void destroySequence(rule_sequence_t *seq) {
    for(size_t i = 0; i < seq->count; i++) {
        destroyRule(&seq->at[i]);
    }
    if(seq->at) {
        free(seq->at);
    }
}

void destroyRuleNode(rule_node_t *node) {
    if(node->sequence_or_alternative == is_sequence) {
        destroySequence(&node->sequence);
    } else {
        // Alternatives - destroy all sequences
        for(size_t i = 0; i < node->alternatives.count; i++) {
            destroySequence(&node->alternatives.at[i]);
        }
        destroy_dynarray(node->alternatives);
    }
}

void destroyGrammar(grammar_t *gram) {
    if(!gram) return;
    
    for(size_t i = 0; i < gram->entry.count; i++) {
        grammar_entry_t *entry = &gram->entry.at[i];
        destroyString(entry->name);
        destroyRuleNode(&entry->element);
    }
    destroy_dynarray(gram->entry);
}

static option(obj_t_value_t) executeRule(iterstring_t *is, rule_t *rule, grammar_t *gram);
static option(obj_t_value_t) executeSequence(iterstring_t *is, rule_sequence_t *sequence, grammar_t *gram, rule_type_t rule_type);
static option(obj_t_value_t) executeRuleNode(iterstring_t *is, rule_node_t *node, grammar_t *gram, rule_type_t rule_type);
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
    } else {
        if(!rule->ge) {
            fprintf(stderr, "Unlinked rule reference\n");
            return (option(obj_t_value_t)) none;
        }
        
        if(rule->type_mod & modifier_array) {
            array_t arr = createEmptyArray();
            
            option(obj_t_value_t) first = executeGrammarEntry(is, rule->ge, gram);
            if(!first.valid) {
                destroyArray(arr);
                return (option(obj_t_value_t)) none;
            }
            
            arr = insertIntoArray(arr, first.value);
            
            while(1) {
                size_t save_pos = is->index;
              
                option(obj_t_value_t) next = executeGrammarEntry(is, rule->ge, gram);
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
            
        } else if(rule->type_mod & modifier_optional) {
            size_t save_pos = is->index;
            option(obj_t_value_t) opt = executeGrammarEntry(is, rule->ge, gram);
            if(!opt.valid) {
                is->index = save_pos;
                obj_t_value_t ret = {
                    .discriminant = obj_t_null
                };
                return (option(obj_t_value_t)) some(ret);
            }
            return opt;
            
        } else {
            return executeGrammarEntry(is, rule->ge, gram);
        }
    }
}

// Execute a sequence of rules
static option(obj_t_value_t) executeSequence(iterstring_t *is, rule_sequence_t *sequence, grammar_t *gram, rule_type_t rule_type) {
    if(rule_type == implicit_storage) {
        // Implicit storage: concatenate all outputs as strings
        string result = string("");
        
        for(size_t i = 0; i < sequence->count; i++) {
            option(obj_t_value_t) val = executeRule(is, &sequence->at[i], gram);
            if(!val.valid) {
                destroyString(result);
                return (option(obj_t_value_t)) none;
            }
            
            string part = flattenToString(val.value);
            result = stringAppendString(result, part);
            
            // Destroy intermediate values
            if(val.valid) {
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
                        break;
                }
            }
            destroyString(part);
        }
        
        obj_t_value_t ret = {
            .discriminant = obj_t_string,
            .str = result
        };
        return (option(obj_t_value_t)) some(ret);
        
    } else {
        // Object storage: collect keyed values
        object_t result = createEmptyObject();
        
        for(size_t i = 0; i < sequence->count; i++) {
            option(obj_t_value_t) val = executeRule(is, &sequence->at[i], gram);
            if(!val.valid) {
                destroyObject(result);
                return (option(obj_t_value_t)) none;
            }
            
            if(sequence->at[i].storage_key.valid) {
                string key_copy = stringFromString(sequence->at[i].storage_key.value);
                result = insertObjectEntry(result, key_copy, val.value);
            } else {
                // No storage key, discard value
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
                        break;
                }
            }
        }
        
        obj_t_value_t ret = {
            .discriminant = obj_t_obj,
            .obj = result
        };
        return (option(obj_t_value_t)) some(ret);
    }
}

static option(obj_t_value_t) executeRuleNode(iterstring_t *is, rule_node_t *node, grammar_t *gram, rule_type_t rule_type) {
    if(node->sequence_or_alternative == is_sequence) {
        // Single sequence
        return executeSequence(is, &node->sequence, gram, rule_type);
    } else {
        // Multiple alternatives - try each until one succeeds
        for(size_t alt_idx = 0; alt_idx < node->alternatives.count; alt_idx++) {
            size_t save_pos = is->index;
            option(obj_t_value_t) result = executeSequence(is, &node->alternatives.at[alt_idx], gram, rule_type);
            if(result.valid) {
                return result;
            }
            is->index = save_pos;
            iterstringReset(is);
        }
        return (option(obj_t_value_t)) none;
    }
}

static option(obj_t_value_t) executeGrammarEntry(iterstring_t *is, grammar_entry_t *entry, grammar_t *gram) {
    return executeRuleNode(is, &entry->element, gram, entry->rule_type);
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
        obj = insertObjectEntry(obj, start_rule, result.value);
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

