#ifndef VLA_H
#define VLA_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// TODO: besseren Namen dafür finden
typedef enum {
    persistent,
    freeable
} data_policy;

typedef struct VLA VLA;
struct VLA {
    size_t capacity;
    size_t length;
    size_t item_size;
    uint8_t* data;
    data_policy data_freeing_policy;
    void (*item_formatter)(VLA* formatter, void *item);
};

VLA* VLA_initialize(size_t capacity, size_t item_size);
void VLA_set_item_formatter(VLA* v, void (*item_formatter)(VLA* formatter, void *item));
void VLA_set_data_freeing_policy(VLA* v, data_policy policy);
void VLA_assert_item_size_matches(VLA* v, size_t item_size);
void VLA_append(VLA* v, void* address, size_t amount);
void VLA_replace_at_index(VLA* v, void* address, signed long idx);
void VLA_delete_at_index(VLA* v, signed long idx);
void VLA_delete_at_index_order_safe(VLA* v, signed long idx);
void* VLA_get(VLA* v, signed long idx);
void VLA_print(VLA* v);
void VLA_free(VLA* v);

#endif