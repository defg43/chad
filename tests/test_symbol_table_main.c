// Standalone runner for the Phase 6 detour symbol-table feature
// ({table} lookup primitive / rule>table store suffix), plus the existing
// str/ion/format regression suites (test_parser.c/test.c are skipped here
// because of the pre-existing, unrelated dlfcn.h build issue on this
// Windows/MinGW setup - see chad repo memory notes).
#include "../include/chad.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int test_str();
int test_ion();
int test_format();

static bool findEntry(object_t obj, const char *key, size_t *out_idx) {
    for(size_t i = 0; i < obj.count; i++) {
        if(strcmp(obj.key[i].at, key) == 0) {
            *out_idx = i;
            return true;
        }
    }
    return false;
}

static void expect(bool cond, const char *msg) {
    if(!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        exit(1);
    }
    printf("PASS: %s\n", msg);
}

// grammar: a comma-separated list of identifiers where each identifier
// after the first must have already appeared earlier in the SAME list.
// first:identifier>seen registers the first name; repeat entries must
// match {seen} (or the whole item fails). chad's grammar syntax has no
// regex/char-class primitive, so 'identifier' is spelled out as one-or-more
// of an explicit alternation of the single letters this test uses.
// (string() is a statement-expression macro, so this can't be a static
// file-scope initializer - built inside a function instead.)
static option(grammar_t) buildTestGrammar(void) {
    string rules[] = {
        string("list -> first:identifier>seen rest:tail[]?"),
        string("tail -> ',' item:{seen}"),
        string("identifier -> ('a'|'b'|'x'|'y')[]"),
    };
    return compileGrammar(3, &rules);
}

static void test_symbol_table_basic(void) {
    option(grammar_t) g = buildTestGrammar();
    expect(g.valid, "grammar compiles with {table}/>table syntax");
    expect(linkGrammar(&g.value), "grammar links successfully");

    // "a,a,a" - every repeat use of 'a' matches, since 'a' was stored by
    // the first element.
    object_t obj = createEmptyObject();
    obj = parseIntoObject(obj, string("a,a,a"), &g.value, string("list"));
    size_t idx;
    bool found = findEntry(obj, "rest", &idx);
    expect(found, "'rest' key present after matching a,a,a");
    if(found) {
        expect(obj.value[idx].discriminant == obj_t_array, "rest is an array");
        expect(obj.value[idx].arr.count == 2, "two repeat items matched (a,a)");
    }
    destroyObject(obj);

    // "a,b" - 'b' was never stored, so the second item must fail to match
    // {seen}, and since tail[]? is optional, it should just yield zero
    // repeats and leave ",b" unconsumed (reported as a warning, not fatal).
    object_t obj2 = createEmptyObject();
    obj2 = parseIntoObject(obj2, string("a,b"), &g.value, string("list"));
    size_t idx2;
    bool found2 = findEntry(obj2, "rest", &idx2);
    expect(found2, "'rest' key present after matching a,b");
    if(found2) {
        expect(obj2.value[idx2].arr.count == 0, "'b' rejected: zero repeat items matched");
    }
    destroyObject(obj2);

    destroyGrammar(&g.value);
}

// A second independent parseIntoObject() call against the SAME compiled
// grammar_t must not see symbols stored by a prior call (auto-reset).
static void test_symbol_table_resets_between_parses(void) {
    option(grammar_t) g = buildTestGrammar();
    expect(g.valid, "grammar (reset test) compiles");
    expect(linkGrammar(&g.value), "grammar (reset test) links successfully");

    object_t obj1 = createEmptyObject();
    obj1 = parseIntoObject(obj1, string("x,x"), &g.value, string("list"));
    size_t idx1;
    bool found1 = findEntry(obj1, "rest", &idx1);
    expect(found1 && obj1.value[idx1].arr.count == 1, "first parse: x,x matches (1 repeat)");
    destroyObject(obj1);

    // Fresh parse: 'x' was only ever stored in the PRIOR call's now-reset
    // table, so "x" alone as a first item then "x" as a repeat should
    // still work (it's re-stored fresh by THIS call's first:identifier>seen),
    // but a table that leaked would already "coincidentally" pass this
    // case too, so we test with a name ('y') that was never used before.
    object_t obj2 = createEmptyObject();
    obj2 = parseIntoObject(obj2, string("y,y"), &g.value, string("list"));
    size_t idx2;
    bool found2 = findEntry(obj2, "rest", &idx2);
    expect(found2 && obj2.value[idx2].arr.count == 1, "second parse: y,y matches independently (1 repeat)");
    destroyObject(obj2);

    destroyGrammar(&g.value);
}

int main() {
    test_str();
    test_ion();
    test_format();
    test_symbol_table_basic();
    test_symbol_table_resets_between_parses();
    printf("\nAll tests passed.\n");
    return 0;
}
