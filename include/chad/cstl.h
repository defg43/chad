#ifndef _CSTL_H_
#define _CSTL_H_

#define CSTL_alloc malloc
#define CSTL_realloc realloc
#define CSTL_dealloc free

#define dynarray(T) struct dynarray_##T {					\
	T *at;													\
	size_t count;											\
	size_t capacity;										\
}

#define create_dynarray(T) ({								\
	(dynarray(T)) {											\
		.at = NULL,											\
		.count = 0,											\
		.capacity = 0, 										\
	};														\
})

#define dynarray_get_count(array) (array.count)

#define dynarray_append(array, element) ({					\
	bool return_value = false;								\
	size_t element_byte_size = sizeof(array.at[0]);			\
	if(array.capacity == 0) {								\
		void *alloc_result = 								\
			CSTL_alloc(4 * element_byte_size);				\
		if(alloc_result) {									\
			array.at = alloc_result;						\
			array.capacity = 4;								\
			return_value = true;							\
		} else {											\
			fprintf(stderr, 								\
			"failed to allocated memory for dynamic array");\
			exit(EXIT_FAILURE);								\
		}													\
	}														\
															\
	if(array.count >= array.capacity) {						\
		void *realloc_result =								\
			CSTL_realloc(array.at, 							\
			2 * array.capacity * element_byte_size);		\
		if (realloc_result) {								\
			array.at = realloc_result;						\
			array.capacity *= 2;							\
			array.at[array.count] = element;				\
			array.count++;									\
			return_value = true;							\
		} else {											\
			return_value = false;							\
		}													\
	} else if(array.count < array.capacity) {				\
		array.at[array.count] = element;					\
		array.count++;										\
		return_value = true;								\
	}														\
	return_value;											\
})

#define dynarray_insert(array, position, element)	({		\
	bool return_value = false;								\
	if (array.count < position) {							\
		return_value = false;								\
	} else if (array.count == position) {					\
		return_value = dynarray_append(array, element);		\
	} else if (dynarray_append(array, 						\
		array.at[array.count - 1])) { 						\
		memmove(array.at + position + 1, 					\
			array.at + position, 							\
			(array.count - position) 						\
			* sizeof(*array.at)); 							\
		array.at[position] = element;						\
		return_value = true;								\
	}														\
	return_value;											\
})

#define dynarray_remove(array, position)	({				\
	bool return_value = false;								\
	if(position >= array.count) {							\
		return_value = false;								\
	} else {												\
		memmove(array.at + position,						\
			array.at + position + 1,						\
			(array.count - position - 1) 					\
			* sizeof(*array.at));							\
															\
		array.count--;										\
		if(array.count * 2 < array.capacity) {				\
			void *resize_result = CSTL_realloc(array.at, 	\
				(sizeof(array.at[0]) * array.capacity) / 2);\
			if(resize_result == NULL) {						\
				return_value = false;						\
			} else {										\
				return_value = true;						\
				array.at = resize_result;					\
				array.capacity /= 2;						\
			}												\
		}													\
	}														\
	return_value;											\
})

#define destroy_dynarray(array) ({							\
	if (array.at) { 										\
    	CSTL_dealloc(array.at); 							\
	}														\
	array.at = NULL;										\
	array.count = 0;										\
	array.capacity = 0;										\
})

#define stack(T) struct stack_##T {							\
	T *at;													\
	size_t count;											\
	size_t capacity;										\
}
#define create_stack(T) ({               					\
    (stack(T)) {                         					\
        .at = NULL,                      					\
        .count = 0,                       					\
        .capacity = 0,                     					\
    };                                   					\
})

#define stack_get_count(stack) ((stack).count)

#define stack_push(stack, element) ({                       \
    bool return_value = false;                              \
    size_t element_byte_size = sizeof(stack.at[0]);         \
    if (stack.capacity == 0) {                              \
        void *alloc_result = 								\
        	CSTL_alloc(4 * element_byte_size);   			\
        if (alloc_result) {                                 \
            stack.at = alloc_result;                        \
            stack.capacity = 4;                             \
            return_value = true;                            \
        } else {                                            \
            fprintf(stderr, "Memory allocation failed\n");  \
            exit(EXIT_FAILURE);                             \
        }                                                   \
    }                                                       \
    if (stack.count >= stack.capacity) {                    \
        void *realloc_result = 								\
        	CSTL_realloc(stack.at, 							\
        		2 * stack.capacity * element_byte_size); 	\
        if (realloc_result) {                               \
            stack.at = realloc_result;                      \
            stack.capacity *= 2;                            \
        } else {                                            \
            return_value = false;                           \
        }                                                   \
    }                                                       \
    if (stack.count < stack.capacity) {                     \
        stack.at[stack.count] = element;                    \
        stack.count++;                                      \
        return_value = true;                                \
    }                                                       \
    return_value;                                           \
})

#define stack_pop(stack) ({                                 \
    struct {												\
    	typeof(stack.at[0]) value;							\
    	bool valid;											\
    } return_value;			                     			\
    if (stack.count == 0) {                                 \
        return_value.valid = false;                         \
    } else {                                                \
        stack.count--;                                      \
        if (stack.count * 4 < stack.capacity) {             \
            void *resize_result = CSTL_realloc(stack.at,    \
                (sizeof(stack.at[0]) * stack.capacity) / 2);\
            if (resize_result) {                            \
                stack.at = resize_result;                   \
                stack.capacity /= 2;                        \
            }                                               \
        }                                                   \
        return_value.value = stack_top(stack);              \
    }                                                       \
      return_value;                                         \
})

#define stack_top(stack) ({                                 \
    typeof(stack.at[0]) return_value;                       \
    if (stack.count == 0) {                                 \
        fprintf(stderr, "Error: Stack is empty\n");         \
        exit(EXIT_FAILURE);                                 \
    }                                                       \
    return_value = stack.at[stack.count - 1];               \
    return_value;                                           \
})

#define destroy_stack(stack) ({                             \
    if (stack.at) { CSTL_dealloc(stack.at); }               \
    stack.at = NULL;                                        \
    stack.count = 0;                                        \
    stack.capacity = 0;                                     \
})

#define hashtable(T1, T2) struct hashtable_##T1##_##T2 { 	\
	T1 *key;												\
	T2 *value;												\
	size_t count;											\
	size_t capacity;										\
}

#define create_hashtable(T1, T2) ({                      	\
    (hashtable(T1, T2)) {                                	\
        .key = NULL,                                     	\
        .value = NULL,                                   	\
        .count = 0,                                      	\
        .capacity = 0,                                   	\
    };                                                  	\
})

// this probably isnt the best hash function
#define hash_function(key, cap) ((key) % (cap))

#define hashtable_expand(ht) ({                            	\
    size_t new_capacity = ((ht).capacity == 0) ?			\
    	4 : (ht).capacity * 2; 								\
    size_t key_size = sizeof(*(ht).key);                   	\
    size_t value_size = sizeof(*(ht).value);               	\
                                                           	\
    void *new_keys = CSTL_alloc(new_capacity * key_size);  	\
    void *new_values = 										\
    	CSTL_alloc(new_capacity * value_size);  			\
    uint8_t *new_occupied = 								\
    	CSTL_alloc(new_capacity * sizeof(uint8_t)); 		\
                                                           	\
    if (!new_keys || !new_values || !new_occupied) {       	\
        fprintf(stderr, "Memory allocation failed\n");     	\
        exit(EXIT_FAILURE);                                	\
    }                                                      	\
                                                           	\
    for (size_t i = 0; i < new_capacity; i++)              	\
        new_occupied[i] = 0;                               	\
                                                           	\
    for (size_t i = 0; i < (ht).capacity; i++) {           	\
        if ((ht).key[i] != 0) {                            	\
            size_t new_index = 								\
            	hash_function((ht).key[i], new_capacity); 	\
            while (((typeof((ht).key))new_keys)[new_index]	\
            	!= 0) { 									\
                new_index = (new_index + 1) % new_capacity; \
            }                                              	\
            ((typeof((ht).key))new_keys)[new_index] =		\
            	(ht).key[i]; 								\
            ((typeof((ht).value))new_values)[new_index] = 	\
            	(ht).value[i]; 								\
        }                                                  	\
    }                                                      	\
                                                           	\
    CSTL_dealloc((ht).key);                              	\
    CSTL_dealloc((ht).value);                             	\
                                                           	\
    (ht).key = new_keys;                                   	\
    (ht).value = new_values;                               	\
    (ht).capacity = new_capacity;                         	\
})

#define hashtable_insert(ht, key_, value_) ({              	\
    if ((ht).count * 2 >= (ht).capacity) {                	\
        hashtable_expand(ht);                             	\
    }                                                     	\
                                                          	\
    size_t index = hash_function(key_, (ht).capacity);     	\
    while ((ht).key[index] != 0) {                        	\
        if ((ht).key[index] == key_) break;                	\
        index = (index + 1) % (ht).capacity;              	\
    }                                                     	\
                                                          	\
    if ((ht).key[index] == 0) (ht).count++;               	\
                                                          	\
    (ht).key[index] = key_;                                	\
    (ht).value[index] = value_;                            	\
})

#define hashtable_find(ht, key_) ({                    		\
    typeof((ht).value) result = NULL;                 		\
    if ((ht).capacity) {                              		\
        size_t index = hash_function(key_, (ht).capacity); 	\
        while ((ht).key[index] != 0) {                		\
            if ((ht).key[index] == key_) {             		\
                result = &(ht).value[index];          		\
                break;                                		\
            }                                        		\
            index = (index + 1) % (ht).capacity;      		\
        }                                            		\
    }                                                		\
    result;                                          		\
})

#define hashtable_remove(ht, key_) ({                 		\
    bool return_value = false;                       		\
    if ((ht).capacity) {                             		\
        size_t index = hash_function(key_, (ht).capacity); 	\
        while ((ht).key[index] != 0) {               		\
            if ((ht).key[index] == key_) {            		\
                (ht).key[index] = 0;                 		\
                (ht).count--;                        		\
                return_value = true;                 		\
                break;                               		\
            }                                       		\
            index = (index + 1) % (ht).capacity;     		\
        }                                           		\
    }                                               		\
    return_value;                                   		\
})

#define destroy_hashtable(ht) ({                    		\
    CSTL_dealloc((ht).key); free((ht).value);         		\
    (ht).key = NULL; (ht).value = NULL;             		\
    (ht).count = 0; (ht).capacity = 0;             			\
})

#define hashset(T) struct hashset_##T {   					\
    T *data;                          						\
    size_t count;                     						\
    size_t capacity;                  						\
}

#define create_hashset(T) ({								\
    (hashset(T)) {											\
        .data = NULL,                     					\
        .count = 0,                        					\
        .capacity = 0,                     					\
    };                                    					\
})

#define hashset_function(value, cap) ((value) % (cap))

#define hashset_expand(hs) ({ 	        					\
    size_t new_capacity = ((hs).capacity == 0) ? 			\
    	4 : (hs).capacity * 2; 								\
    typeof(hs.data) new_data =								\
    	malloc(new_capacity * sizeof(*(hs).data));			\
    uint8_t *new_occupied = 								\
    	malloc(new_capacity * sizeof(uint8_t)); 			\
    if (!new_data || !new_occupied) {    					\
        fprintf(stderr, "Memory allocation failed\n"); 		\
        exit(EXIT_FAILURE);              					\
    }                                    					\
    for (size_t i = 0; i < new_capacity; i++) 				\
    	new_occupied[i] = 0; 								\
    for (size_t i = 0; i < (hs).capacity; i++) { 			\
        if ((hs).data[i] != 0) {         					\
            size_t new_index =								\
            	hashset_function(							\
            		(hs).data[i], new_capacity);			\
            while (new_data[new_index] != 0) { 				\
                new_index = (new_index + 1) % new_capacity; \
            }                            					\
            new_data[new_index] = (hs).data[i]; 			\
        }                                					\
    }                                    					\
    CSTL_dealloc((hs).data);              					\
    (hs).data = new_data;                					\
    (hs).capacity = new_capacity;        					\
})

#define hashset_insert(hs, value) ({	  					\
    if ((hs).count * 2 >= (hs).capacity) { 					\
        hashset_expand(hs);		           					\
    }                                    					\
    size_t index = hashset_function(value, (hs).capacity); 	\
    while ((hs).data[index] != 0) {      					\
        if ((hs).data[index] == value) break; 				\
        index = (index + 1) % (hs).capacity; 				\
    }                                    					\
    if ((hs).data[index] == 0) (hs).count++; 				\
    (hs).data[index] = value;            					\
})

#define hashset_contains(hs, value) ({   					\
    bool found = false;                  					\
    if ((hs).capacity) {                 					\
        size_t index =										\
        	hashset_function(value, (hs).capacity); 		\
        while ((hs).data[index] != 0) {  					\
            if ((hs).data[index] == value) { 				\
                found = true;            					\
                break;                    					\
            }                            					\
            index = (index + 1) % (hs).capacity; 			\
        }                                					\
    }                                    					\
    found;                               					\
})

#define hashset_remove(hs, value) ({     					\
    bool return_value = false;           					\
    if ((hs).capacity) {                 					\
        size_t index =										\
        	hashset_function(value, (hs).capacity); 		\
        while ((hs).data[index] != 0) {  					\
            if ((hs).data[index] == value) {	 			\
                (hs).data[index] = 0;     					\
                (hs).count--;            					\
                return_value = true;     					\
                break;                   					\
            }                            					\
            index = (index + 1) % (hs).capacity; 			\
        }                                					\
    }                                    					\
    return_value;                        					\
})

#define destroy_hashset(hs) ({           					\
    free((hs).data);                     					\
    (hs).data = NULL;                     					\
    (hs).count = 0;                       					\
    (hs).capacity = 0;                    					\
})

#define linked_list(T) struct linked_list_##T {  			\
    T value;                                    			\
    struct linked_list_##T *next;               			\
}

#define create_linked_list(T) (NULL)

#define linked_list_prepend(head, element) ({         		\
    typeof(head) new_node = CSTL_alloc(sizeof(*head)); 		\
    if (!new_node) {                                  		\
        fprintf(stderr, "Memory allocation failed\n");		\
        exit(EXIT_FAILURE);                           		\
    }                                                 		\
    new_node->value = element;                        		\
    new_node->next = head;                            		\
    head = new_node;                                  		\
    head;                                             		\
})

#define linked_list_append(head, element) ({          		\
    typeof(head) new_node = CSTL_alloc(sizeof(*head)); 		\
    if (!new_node) {                                  		\
        fprintf(stderr, "Memory allocation failed\n");		\
        exit(EXIT_FAILURE);                           		\
    }                                                 		\
    new_node->value = element;                        		\
    new_node->next = NULL;                            		\
    if (!head) {                                      		\
        head = new_node;                              		\
    } else {                                          		\
        typeof(head) temp = head;                     		\
        while (temp->next) { temp = temp->next; }     		\
        temp->next = new_node;                        		\
    }                                                 		\
    head;                                             		\
})

#define linked_list_remove(head, target) ({           		\
    typeof(head) temp = head, prev = NULL;            		\
    bool return_value = false;                        		\
    while (temp && temp->value != target) {           		\
        prev = temp;                                  		\
        temp = temp->next;                            		\
    }                                                 		\
    if (temp) {                                       		\
        if (prev) {                                   		\
            prev->next = temp->next;                  		\
        } else {                                      		\
            head = temp->next;                        		\
        }                                             		\
        CSTL_dealloc(temp);                           		\
        return_value = true;                          		\
    }                                                 		\
    return_value;                                     		\
})

#define linked_list_find(head, target) ({             		\
    typeof(head) temp = head;                         		\
    while (temp && temp->value != target) {           		\
        temp = temp->next;                            		\
    }                                                 		\
    temp;                                             		\
})

#define destroy_linked_list(head) ({                  		\
    typeof(head) temp;                                		\
    while (head) {                                    		\
        temp = head;                                  		\
        head = head->next;                            		\
        CSTL_dealloc(temp);                           		\
    }                                                 		\
    head = NULL;                                      		\
})


#define queue(T) struct queue_##T {         				\
    T *data;                               					\
    size_t head;                           					\
    size_t tail;                           					\
    size_t capacity;                       					\
}

#define create_queue(T) ({                  				\
    (queue(T)) {                            				\
        .data = NULL,                        				\
        .head = 0,                           				\
        .tail = 0,                           				\
        .capacity = 0,                       				\
    };                                      				\
})

#define queue_is_empty(q) ((q).head == (q).tail)

#define queue_size(q) (((q).tail >= (q).head) ? 			\
                        ((q).tail - (q).head) :  			\
                        ((q).capacity -						\
                        (q).head + (q).tail))

#define queue_expand(q) ({                                  \
    size_t new_capacity = (q).capacity ? 					\
    	(q).capacity * 2 : 4; 								\
    void *new_data = 										\
    	CSTL_alloc(new_capacity * sizeof((q).data[0])); 	\
    if (!new_data) {                                        \
        fprintf(stderr, "Memory allocation failed\n");      \
        exit(EXIT_FAILURE);                                 \
    }                                                       \
    if ((q).capacity) {                                     \
        size_t size = queue_size(q);                        \
        for (size_t i = 0; i < size; i++) {                 \
            ((typeof((q).data))new_data)[i] = 				\
            	(q).data[(q).head]; 						\
            (q).head = ((q).head + 1) % (q).capacity;       \
        }                                                   \
        CSTL_dealloc((q).data);                             \
    }                                                       \
    (q).data = new_data;                                    \
    (q).head = 0;                                           \
    (q).tail = queue_size(q);                               \
    (q).capacity = new_capacity;                           	\
})

#define queue_enqueue(q, element) ({                      	\
	if((q).capacity == 0) {									\
		(q).data = CSTL_alloc(4 * sizeof(*((q).data)));		\
		(q).capacity = 4;									\
		if(!(q).data) {										\
			fprintf(stderr, "Memory allocation failed\n");	\
			exit(EXIT_FAILURE);								\
		}													\
	}														\
    bool return_value = false;                            	\
    if (((q).tail + 1) % (q).capacity == (q).head) {      	\
        queue_expand(q);                                  	\
    }                                                     	\
    (q).data[(q).tail] = element;                         	\
    (q).tail = ((q).tail + 1) % (q).capacity;            	\
    return_value = true;                                  	\
    return_value;                                         	\
})

#define queue_dequeue(q) ({                               	\
    bool return_value = false;                            	\
    if (!queue_is_empty(q)) {                             	\
        (q).head = ((q).head + 1) % (q).capacity;        	\
        return_value = true;                              	\
    }                                                     	\
    return_value;                                         	\
})

#define queue_front(q) ({                                 	\
    typeof((q).data[0]) return_value;                    	\
    if (queue_is_empty(q)) {                             	\
        fprintf(stderr, "Error: Queue is empty\n");      	\
        exit(EXIT_FAILURE);                              	\
    }                                                    	\
    return_value = (q).data[(q).head];                   	\
    return_value;                                        	\
})

#define destroy_queue(q) ({                              	\
    if ((q).data) { CSTL_dealloc((q).data); }           	\
    (q).data = NULL;                                    	\
    (q).head = 0;                                       	\
    (q).tail = 0;                                       	\
    (q).capacity = 0;                                   	\
})

#define deque(T) struct deque_##T { 						\
    T *data;                        						\
    size_t front;                    						\
    size_t back;                     						\
    size_t capacity;                  						\
}

#define create_deque(T) ({            						\
    (deque(T)) {                      						\
        .data = NULL,                  						\
        .front = 0,                     					\
        .back = 0,                      					\
        .capacity = 0,                  					\
    };                                 						\
})

#define deque_is_empty(dq) ((dq).front == (dq).back)

#define deque_expand(dq, T) ({                     			\
    size_t new_capacity = (dq).capacity ? 					\
    (dq).capacity * 2 : 4; 									\
    T *new_data = CSTL_alloc(new_capacity * sizeof(T)); 	\
    if (!new_data) {                               			\
        fprintf(stderr, "Memory allocation failed\n"); 		\
        exit(EXIT_FAILURE);                        			\
    }                                              			\
    size_t size = ((dq).back >= (dq).front) ?      			\
                  ((dq).back - (dq).front) :       			\
                  ((dq).capacity - (dq).front + (dq).back); \
    for (size_t i = 0; i < size; i++) {           			\
        new_data[i] = (dq).data[(dq).front];      			\
        (dq).front = ((dq).front + 1) % (dq).capacity; 		\
    }                                              			\
    CSTL_dealloc((dq).data);                       			\
    (dq).data = new_data;                         			\
    (dq).front = 0;                               			\
    (dq).back = size;                             			\
    (dq).capacity = new_capacity;                 			\
})

#define deque_push_front(dq, element, T) ({       			\
	if((dq).capacity == 0) {								\
		(dq).data = CSTL_alloc(sizeof(*((dq).data)) * 4);	\
		(dq).capacity = 4;									\
		if(!(dq).data) {									\
			fprintf(stderr, "Memory allocation failed\n");	\
			exit(EXIT_FAILURE);								\
		}													\
	}														\
    if (((dq).back + 1) % (dq).capacity == (dq).front) { 	\
        deque_expand(dq, T);                      			\
    }                                             			\
    (dq).front = ((dq).front - 1 + 							\
    	(dq).capacity) % (dq).capacity; 					\
    (dq).data[(dq).front] = element;              			\
})

#define deque_push_back(dq, element, T) ({        			\
	if((dq).capacity == 0) {								\
		(dq).data = CSTL_alloc(sizeof(*((dq).data)) * 4);	\
		(dq).capacity = 4;									\
		if(!(dq).data) {									\
			fprintf(stderr, "Memory allocation failed\n");	\
			exit(EXIT_FAILURE);								\
		}													\
	}														\
    if (((dq).back + 1) % (dq).capacity == (dq).front) { 	\
        deque_expand(dq, T);                      			\
    }                                             			\
    (dq).data[(dq).back] = element;               			\
    (dq).back = ((dq).back + 1) % (dq).capacity;  			\
})

#define deque_pop_front(dq) ({                    			\
    bool return_value = false;                     			\
    if (!deque_is_empty(dq)) {                     			\
        (dq).front = ((dq).front + 1) % (dq).capacity; 		\
        return_value = true;                        		\
    }                                              			\
    return_value;                                  			\
})

#define deque_pop_back(dq) ({                     			\
    bool return_value = false;                     			\
    if (!deque_is_empty(dq)) {                     			\
        (dq).back = ((dq).back - 1 + (dq).capacity) 		\
        % (dq).capacity; 									\
        return_value = true;                        		\
    }                                              			\
    return_value;                                  			\
})

#define deque_front(dq) ({                        			\
    typeof((dq).data[0]) return_value;            			\
    if (deque_is_empty(dq)) {                     			\
        fprintf(stderr, "Error: Deque is empty\n"); 		\
        exit(EXIT_FAILURE);                        			\
    }                                             			\
    return_value = (dq).data[(dq).front];         			\
    return_value;                                 			\
})

#define deque_back(dq) ({                         			\
    typeof((dq).data[0]) return_value;            			\
    if (deque_is_empty(dq)) {                     			\
        fprintf(stderr, "Error: Deque is empty\n"); 		\
        exit(EXIT_FAILURE);                        			\
    }                                             			\
    return_value = (dq).data[((dq).back - 1 + 				\
    	(dq).capacity) % (dq).capacity]; 					\
    return_value;                                 			\
})

#define destroy_deque(dq) ({                      			\
    if ((dq).data) { CSTL_dealloc((dq).data); }   			\
    (dq).data = NULL;                             			\
    (dq).front = 0;                               			\
    (dq).back = 0;                                			\
    (dq).capacity = 0;                            			\
})

#define priority_queue(T) struct priority_queue_##T { 		\
    T *data;                                          		\
    size_t count;                                     		\
    size_t capacity;                                  		\
}

#define create_priority_queue(T) ({                   		\
    (priority_queue(T)) {                             		\
        .data = NULL,                                 		\
        .count = 0,                                   		\
        .capacity = 0,                                		\
    };                                               		\
})

#define swap(a, b) ({ typeof(a) temp = a; a = b; b = temp; })

#define priority_queue_expand(pq, T) ({               		\
    size_t new_capacity = ((pq).capacity == 0) ? 			\
    	4 : (pq).capacity * 2; 								\
    T *new_data = CSTL_alloc(new_capacity * sizeof(T)); 	\
    if (!new_data) {                                  		\
        fprintf(stderr, "Memory allocation failed\n"); 		\
        exit(EXIT_FAILURE);                           		\
    }                                                 		\
    for (size_t i = 0; i < (pq).count; i++) new_data[i] = 	\
    	(pq).data[i]; 										\
    CSTL_dealloc((pq).data);                          		\
    (pq).data = new_data;                            		\
    (pq).capacity = new_capacity;                    		\
})

#define priority_queue_push(pq, element, T) ({       		\
    if ((pq).count == (pq).capacity) {              		\
        priority_queue_expand(pq, T);               		\
    }                                               		\
    size_t i = (pq).count++;                        		\
    (pq).data[i] = element;                         		\
    while (i > 0) {                                 		\
        size_t parent = (i - 1) / 2;                		\
        if ((pq).data[parent] <= (pq).data[i]) break; 		\
        swap((pq).data[i], (pq).data[parent]);      		\
        i = parent;                                 		\
    }                                               		\
})

#define priority_queue_pop(pq) ({                   		\
    bool return_value = false;                      		\
    if ((pq).count > 0) {                           		\
        (pq).data[0] = (pq).data[--(pq).count];     		\
        size_t i = 0;                               		\
        while (2 * i + 1 < (pq).count) {            		\
            size_t child = 2 * i + 1;               		\
            if (child + 1 < (pq).count && 					\
            	(pq).data[child + 1] < (pq).data[child]) 	\
                child++;                            		\
            if ((pq).data[i] <= (pq).data[child]) break; 	\
            swap((pq).data[i], (pq).data[child]);   		\
            i = child;                              		\
        }                                           		\
        return_value = true;                        		\
    }                                               		\
    return_value;                                   		\
})

#define priority_queue_top(pq) ({                   		\
    if ((pq).count == 0) {                          		\
        fprintf(stderr, "Error: Priority queue is empty\n");\
        exit(EXIT_FAILURE);                         		\
    }                                               		\
    (pq).data[0];                                   		\
})

#define destroy_priority_queue(pq) ({               		\
    if ((pq).data) CSTL_dealloc((pq).data);         		\
    (pq).data = NULL;                               		\
    (pq).count = 0;                                 		\
    (pq).capacity = 0;                              		\
})


#define pair(T1, T2) struct pair_##T1##_##T2 {  			\
    T1 first;                               				\
    T2 second;                              				\
}

#define span(T) struct span_##T { 							\
    T *data;                     							\
    size_t size;                 							\
}

#define option(T) struct option_##T { 						\
	T value;												\
	bool valid;												\
}

#define some(val) { .value = val, .valid = true, }
#define none { .valid = false, }

#define result(T1, T2) struct result_##T1##_##T2 {			\
	T1 value;												\
	T2 error;												\
}

#endif // _CSTL_H_
