#include "../include/chad/cstl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

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

static bool is_even(int x) {
    return x % 2 == 0;
}

static bool is_negative(int x) {
    return x < 0;
}

static bool is_positive(int x) {
    return x > 0;
}

static int compare_ints(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

static bool ints_equal(int a, int b) {
    return a == b;
}

static void test_dynarray_find(void) {
    printf("\n-- dynarray_find / dynarray_find_last --\n");

    dynarray(int) arr = create_dynarray(int);
    dynarray_append(arr, 1);
    dynarray_append(arr, 3);
    dynarray_append(arr, 4);
    dynarray_append(arr, 6);
    dynarray_append(arr, 7);

    ASSERT_TRUE("dynarray_find finds first even element", dynarray_find(arr, is_even) == 2);
    ASSERT_TRUE("dynarray_find_last finds last even element", dynarray_find_last(arr, is_even) == 3);
    ASSERT_TRUE("dynarray_find returns -1 when nothing matches", dynarray_find(arr, is_negative) == -1);
    ASSERT_TRUE("dynarray_find_last returns -1 when nothing matches", dynarray_find_last(arr, is_negative) == -1);

    destroy_dynarray(arr);

    dynarray(int) empty = create_dynarray(int);
    ASSERT_TRUE("dynarray_find on empty array returns -1", dynarray_find(empty, is_even) == -1);
    destroy_dynarray(empty);
}

static void test_dynarray_any_all(void) {
    printf("\n-- dynarray_any / dynarray_all --\n");

    dynarray(int) arr = create_dynarray(int);
    dynarray_append(arr, 2);
    dynarray_append(arr, 4);
    dynarray_append(arr, 6);

    ASSERT_TRUE("dynarray_all true when every element matches", dynarray_all(arr, is_even));
    ASSERT_TRUE("dynarray_any true when at least one element matches", dynarray_any(arr, is_even));
    ASSERT_TRUE("dynarray_any false when no element matches", !dynarray_any(arr, is_negative));
    ASSERT_TRUE("dynarray_all false when not every element matches", !dynarray_all(arr, is_negative));

    destroy_dynarray(arr);

    dynarray(int) empty = create_dynarray(int);
    ASSERT_TRUE("dynarray_any on empty array is false", !dynarray_any(empty, is_positive));
    ASSERT_TRUE("dynarray_all on empty array is vacuously true", dynarray_all(empty, is_positive));
    destroy_dynarray(empty);
}

static void test_dynarray_reverse(void) {
    printf("\n-- dynarray_reverse --\n");

    dynarray(int) arr = create_dynarray(int);
    dynarray_append(arr, 1);
    dynarray_append(arr, 2);
    dynarray_append(arr, 3);
    dynarray_append(arr, 4);

    dynarray_reverse(arr);
    ASSERT_TRUE("reverse of 4 elements is correct",
        arr.at[0] == 4 && arr.at[1] == 3 && arr.at[2] == 2 && arr.at[3] == 1);
    ASSERT_TRUE("reverse does not change count", arr.count == 4);

    destroy_dynarray(arr);

    dynarray(int) single = create_dynarray(int);
    dynarray_append(single, 42);
    dynarray_reverse(single);
    ASSERT_TRUE("reverse of single-element array is a no-op", single.at[0] == 42);
    destroy_dynarray(single);

    dynarray(int) empty = create_dynarray(int);
    dynarray_reverse(empty); // must not crash
    ASSERT_TRUE("reverse of empty array does not crash and count stays 0", empty.count == 0);
    destroy_dynarray(empty);
}

static void test_dynarray_clear(void) {
    printf("\n-- dynarray_clear --\n");

    dynarray(int) arr = create_dynarray(int);
    dynarray_append(arr, 1);
    dynarray_append(arr, 2);
    dynarray_append(arr, 3);

    size_t capacity_before = arr.capacity;
    dynarray_clear(arr);
    ASSERT_TRUE("clear resets count to 0", arr.count == 0);
    ASSERT_TRUE("clear keeps the buffer allocated for reuse", arr.at != NULL);
    ASSERT_TRUE("clear keeps capacity unchanged", arr.capacity == capacity_before);

    dynarray_append(arr, 99);
    ASSERT_TRUE("array is usable again after clear", arr.count == 1 && arr.at[0] == 99);

    destroy_dynarray(arr);
}

static void test_dynarray_sort(void) {
    printf("\n-- dynarray_sort --\n");

    dynarray(int) arr = create_dynarray(int);
    dynarray_append(arr, 5);
    dynarray_append(arr, 1);
    dynarray_append(arr, 4);
    dynarray_append(arr, 2);
    dynarray_append(arr, 3);

    dynarray_sort(arr, compare_ints);
    bool sorted = true;
    for(size_t i = 0; i + 1 < arr.count; i++) {
        if(arr.at[i] > arr.at[i + 1]) sorted = false;
    }
    ASSERT_TRUE("sort produces ascending order", sorted);
    ASSERT_TRUE("sort keeps count unchanged", arr.count == 5);
    ASSERT_TRUE("first element is smallest", arr.at[0] == 1);
    ASSERT_TRUE("last element is largest", arr.at[4] == 5);

    destroy_dynarray(arr);

    dynarray(int) empty = create_dynarray(int);
    dynarray_sort(empty, compare_ints); // must not crash
    ASSERT_TRUE("sort of empty array does not crash", empty.count == 0);
    destroy_dynarray(empty);
}

static void test_dynarray_dedup(void) {
    printf("\n-- dynarray_dedup --\n");

    dynarray(int) arr = create_dynarray(int);
    dynarray_append(arr, 1);
    dynarray_append(arr, 1);
    dynarray_append(arr, 2);
    dynarray_append(arr, 2);
    dynarray_append(arr, 2);
    dynarray_append(arr, 3);
    dynarray_append(arr, 1);

    dynarray_dedup(arr, ints_equal);
    ASSERT_TRUE("dedup removes adjacent duplicates only", arr.count == 4);
    ASSERT_TRUE("dedup result is correct",
        arr.at[0] == 1 && arr.at[1] == 2 && arr.at[2] == 3 && arr.at[3] == 1);

    destroy_dynarray(arr);

    dynarray(int) no_dups = create_dynarray(int);
    dynarray_append(no_dups, 1);
    dynarray_append(no_dups, 2);
    dynarray_dedup(no_dups, ints_equal);
    ASSERT_TRUE("dedup on array with no duplicates is a no-op", no_dups.count == 2);
    destroy_dynarray(no_dups);
}

static void test_dynarray_slice(void) {
    printf("\n-- dynarray_slice --\n");

    dynarray(int) arr = create_dynarray(int);
    dynarray_append(arr, 10);
    dynarray_append(arr, 20);
    dynarray_append(arr, 30);
    dynarray_append(arr, 40);
    dynarray_append(arr, 50);

    dynarray(int) middle = dynarray_slice(arr, 1, 4);
    ASSERT_TRUE("slice has correct count", middle.count == 3);
    ASSERT_TRUE("slice has correct elements",
        middle.at[0] == 20 && middle.at[1] == 30 && middle.at[2] == 40);

    dynarray(int) out_of_bounds = dynarray_slice(arr, 3, 100);
    ASSERT_TRUE("slice clamps end past array bounds", out_of_bounds.count == 2);

    dynarray(int) empty_slice = dynarray_slice(arr, 2, 2);
    ASSERT_TRUE("slice with start == end is empty", empty_slice.count == 0);

    destroy_dynarray(middle);
    destroy_dynarray(out_of_bounds);
    destroy_dynarray(empty_slice);
    destroy_dynarray(arr);
}

int test_cstl(void) {
    test_dynarray_find();
    test_dynarray_any_all();
    test_dynarray_reverse();
    test_dynarray_clear();
    test_dynarray_sort();
    test_dynarray_dedup();
    test_dynarray_slice();

    printf("\n");
    if (failed == 0) {
        printf("\033[32mAll %d cstl tests passed.\033[0m\n", passed);
    } else {
        printf("\033[31m%d/%d cstl tests FAILED.\033[0m\n", failed, passed + failed);
    }
    return failed == 0 ? 0 : 1;
}
