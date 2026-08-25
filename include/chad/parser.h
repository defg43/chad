// SPDX-License-Identifier: MIT
// Copyright (C) 2023-2025 defg43
// https://github.com/defg43/

#ifndef PARSER_H
#define PARSER_H

/*
// there are two types of rules

// in the first rule elements do not specify their storage type the storage type is therefore implicitly a string
// literals are appended to the a rule-local string that is the result of the rule
// other entities such as arrays, objects and numbers are converted to strings and appended
// for arrays each entry is converted to a string if needed and all strings are concatenated
// objects are flattened and all values are concatenated, the keys are ignored
char -> 'a' | 'b' | 'c' | 'd' ...

// these rules can be nested and combined together; the outputs of subrules being concatenated together
token -> char[] charOrDigit[]

// the second rule specifies storage types for at least one element
// these are entries in a Javascript object
// the storage is specified with a key and an asociated rule in the form of key:rule
// the result of the rule will be stored under the key
// these key value pairs are collected in a rule-local Javascript object and is the result of the rule
// if some elements dont provide a storage specifier they are executed but their output is discarded
rule -> t1:token '(' t2:token ')'

the modifiers are [] which searches for at least one or more occurences, ? 
which looks for one or none occurences

it is also possible to write rules inline by writing key:(definition), the key is also optional like with other rules

in addition there are alternatives that can be used between elements where only one alternative 
needs to succeed. The first matched alternative has priority; matching will stop after the first
successfull match

alternatives (|) has low binding strength, so that a b c | d e f is equivalent to (a b c) | (d e f)
*//* examples:
r1 -> 'a'
r2 -> a:'a'
r3 -> a:r2
r4 -> a:r2 b:r3
r5 -> r4
r6 -> r4 r5
r7 -> r4 | r5
r8 -> r4 r5 | r6 r7
r9 -> a:r4 b:r5 c:r6 d:r7
r10 -> r4 (r5 | r6) r7
r11 -> a:r4 b:(r5 | r6 r7 'a')
r12 -> a:r4 b:(a:r5 | b:'abc' c:'def') '1234' c:'1234'
r13 -> a:r1[]
r14 -> a:r1?
r15 -> a:r1[]?
r16 -> 'a'[]
r17 -> 'a'?
r18 -> 'a'[]?
*/

#include "cstl.h"
#include "str.h"
#include "ion.h"
#include <stdio.h>

typedef enum {
	storage_type_not_set = 0, // missed during compilation
	implicit_storage = 1,
	object_storage = 2,	
} rule_type_t;

typedef	enum {
	modifier_none     = 0b00,
	modifier_array 	  = 0b01,
	modifier_optional = 0b10,
	modifier_both	  = 0b11,
} type_modifier_t;

typedef struct grammar_entry grammar_entry_t;
typedef struct rule_node rule_node_t;
typedef dynarray(rule_node_t) rule_node_seq_t;

// the body of an inline anonymous rule key:(definition); shares the same
// shape as grammar_entry_t (a rule_type_t plus a sequence of rule_node_t)
// since an inline rule is executed exactly like a named one, just without
// a name.
typedef struct {
	rule_type_t rule_type;
	rule_node_seq_t element;
} inline_rule_t;

// Phase 6 detour: transactional scoped symbol tables, used by the new
// '{table}' lookup primitive and 'rule>table' store suffix (see grammar_t
// below for the frame stack these read/write). One entry is one stored
// name in one named table, e.g. {table: "generics", key: "T"}.
typedef struct {
	string table;
	string key;
} symbol_entry_t;
// One frame = everything stored during one in-flight PEG attempt (one
// alternative try, or one []/[]? repetition attempt). Frames are pushed at
// every point the engine already saves/restores iterstring_t.index for
// backtracking; on failure the frame is discarded outright ("vanish"), on
// success its entries are appended onto the new top frame ("materialize") -
// see pushSymbolFrame/vanishSymbolFrame/materializeSymbolFrame in parser.c.
typedef dynarray(symbol_entry_t) symbol_frame_t;

typedef struct {
	type_modifier_t type_mod;
	option(string) storage_key; // if none, then discard output
	// Phase 6 detour: 'rule>table' suffix - if set, the matched TEXT is
	// stored into this table (in the current transaction frame) when this
	// rule_t succeeds. Independent of storage_key - a rule can both
	// contribute to the enclosing object under a key AND be recorded into
	// a symbol table.
	option(string) store_table;
	enum {
		is_literal,
		is_rule,
		is_inline,
		is_table_lookup, // '{table}' - matches an identifier iff it is
		                  // currently visible in `table`
	} literal_or_rule;
	union {
		struct {
			string rule_name; // produces
			grammar_entry_t *ge;
		};
		string literal;
		inline_rule_t inl;
		string table_lookup_name; // valid when literal_or_rule == is_table_lookup
	};	
} rule_t;

// one "a b c" run of rule_t, used as a single alternative branch when `|`
// (low binding) groups multiple elements together
typedef dynarray(rule_t) rule_sequence_t;

struct rule_node {
	enum {
		is_regular,
		has_alternative,
	} alternative_or_regular;
	union {
		dynarray(rule_sequence_t) alternative; // list of alternative sequences
		rule_t rule;
	};
};

struct grammar_entry {
	string name;
	rule_type_t rule_type;
	rule_node_seq_t element;
};

typedef struct {
	dynarray(grammar_entry_t) entry;
	// Phase 6 detour: stack of transaction frames for '{table}'/'>table'
	// scoped symbol tables. Pool-allocated here so it lives exactly as long
	// as the compiled grammar_t itself (per design). Always has at least
	// one frame (the committed root) while a grammar_t is valid - pushed by
	// compileGrammar(), reset to one empty frame at the start of every
	// parseIntoObject() call so tables don't leak between separate parses
	// reusing the same compiled grammar.
	dynarray(symbol_frame_t) symbol_frames;
} grammar_t;

/*
					    grammar_t
				_________________________
				|						|
	      ----- |grammar_entry_t entry[]|
		  |		|_______________________|
		  |
		  |
		  |
		  ---------> grammar_entry_t
				_________________________
				|						|
	 		  	|	  string name;		|
				|_______________________|
				|						|
				| rule_type_t rule_type |  => storage_not_set  |
				|_______________________|     implicit_storage |
				|  						|	  object_storage
		  ----  | rule_node_t element[] |
		  |		|_______________________|
	 	  |
		  |
		  |
		  --------->   rule_node_t
				_________________________
				|						|
				|						|
		  ----	| rule_sequence_t alternative[] |
		  |		|			/			|
		  |---	|	   rule_t rule 		|
		  |		|						|
		  |		|_______________________|
		  |		
		  |
		  |
		  |
		  --------->	 rule_t
				_________________________
				|						|
				|  type_modifier_t mod  |
				|_______________________|
				|						|
				|  string storage_key?	|
				|_______________________|
				| ________rule_________	|
				| |					  | |
				| | string rule_name  | |
				| |___________________| |
   				| |					  | |
				| |grammar_entry_t *ge| |
				| |___________________| |
				| 			/			|
				| _______literal_______ |
				| |					  | |
				| |   string literal  | |
				| |___________________| |
				| 			/			|
				| ______inline_rule____ |
				| |					  | |
				| |  inline_rule_t inl| |
				| |___________________| |
				|_______________________|

*/
/*

example -> key1:rule1[] rule2 'literal' key2:'literal2' | key3:'iteral3'
|----------------------------------------------------------------------| grammar_entry_t
|-----| grammar_entry_t.name
		   |-----------------------------------------------------------| grammar_entry_t.element[]
		   |----------| rule_node_t.rule
		   |--| 	    rule_t.storage_key
		   	   |----|	rule_t.rule_name
		   	   		 || rule_t.mod
		   	   		 					|------------------------------| rule_t.alternative[]
*/


option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]);
bool linkGrammar(grammar_t *gram);
option(size_t) findGrammarEntry(grammar_t *gram, string *name);

type_modifier_t parseTypeModifier(iterstring_t *rule);
option(string) parseLiteral(iterstring_t *rule);
option(string) parseIdentifier(iterstring_t *rule);
bool parseWhitespace(iterstring_t *rule);
bool parseSeperator(iterstring_t *rule);
bool isFollowedByAlternative(iterstring_t *rule);

option(rule_t) compileRule(iterstring_t *rule);
option(rule_node_seq_t) compileRuleBody(iterstring_t *rule);
option(grammar_entry_t) compileGrammarEntry(string rule_definition);

object_t parseIntoObject(object_t obj, string input, grammar_t *gram, string start_rule);

void printParsingMessage(FILE *stream, char *msg, string source, const char *const color, size_t color_start, size_t color_stop);
void printGrammar(grammar_t gram);
void destroyGrammar(grammar_t *gram);

#endif // PARSER_H
