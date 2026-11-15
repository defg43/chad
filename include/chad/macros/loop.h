// SPDX-License-Identifier: MIT
// Copyright (C) 2023-2025 defg43
// https://github.com/defg43/

#ifndef LOOP_H
#define LOOP_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define loop for(;;)
#define repeat(num) for(size_t _hidden_repeat_counter_dont_touch = 0; _hidden_repeat_counter_dont_touch < (num); _hidden_repeat_counter_dont_touch++)

#define maybe(num) if(({srand(time(NULL)); rand() > (num) * RAND_MAX;}))

// the outer loop only holds the toggler, so that the same defer can run any number of times
// (a break or return inside the body still skips post)
#define defer(pre, post) 																					\
for(int _hidden_iterator_d_dont_touch = 1; _hidden_iterator_d_dont_touch; _hidden_iterator_d_dont_touch = 0) 	\
for(pre; _hidden_iterator_d_dont_touch; _hidden_iterator_d_dont_touch = 0, post)

#define unless(condition) if(!(condition))
#endif /* LOOP_H */
