#include "VLA.h"

typedef VLA Stack;

#define stack_initialize(capacity, item_size) VLA_initialize(capacity, item_size);
#define stack_push_n(s, address, amount) VLA_append(s, address, amount);
#define stack_pop_n(s, amount)                                               \
    {                                                                        \
        size_t real_amount = amount;                                         \
        if (s->length / s->item_size < amount) {                             \
            warn("Can't delete more items than are stored in the stack!\n"); \
            real_amount = s->length / s->item_size;                          \
        }                                                                    \
                                                                             \
        for (size_t counter = 0; counter < real_amount; counter++) {         \
            VLA_delete_at_index_order_safe(s, -1);                           \
        }                                                                    \
    }
