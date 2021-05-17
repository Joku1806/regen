#include "stack.h"
#include "debug.h"

Stack* stack_initialize(size_t capacity, size_t item_size) { return VLA_initialize(capacity, item_size); }
void stack_push_n(Stack* s, void* address, size_t amount) { VLA_append(s, address, amount); }

void stack_pop_n(Stack* s, size_t amount) {
    if (VLA_get_length(s) < amount) {
        warn("Can't delete more items than are stored in the stack!\n");
        amount = VLA_get_length(s);
    }

    for (size_t counter = 0; counter < amount; counter++) {
        VLA_delete_at_index_order_safe(s, -1);
    }
}

void* stack_pop(Stack* s) {
    void* item = VLA_get(s, -1);
    VLA_delete_at_index_order_safe(s, -1);
    return item;
}