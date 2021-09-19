#ifndef VLA_H
#define VLA_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define INFO_HEADER_SIZE 128

typedef enum {
    immutable,
    freeable
} data_policy;

typedef struct VLA VLA;
struct VLA {
    size_t capacity;
    size_t length;
    size_t item_size;
    uint8_t* data;
};

VLA* VLA_initialize(size_t capacity, size_t item_size);
void VLA_assert_item_size_matches(VLA* v, size_t item_size);
void VLA_append(VLA* v, void* address);
void VLA_batch_append(VLA* v, void* address, size_t amount);
void VLA_delete_at_index(VLA* v, signed long index);
void VLA_clear(VLA* v);
uint8_t* VLA_extract(VLA* v);
uint8_t* VLA_get(VLA* v, signed long index);
size_t VLA_get_length(VLA* v);
uint8_t* VLA_reserve_next_slots(VLA* v, size_t item_count);
void VLA_print(VLA* v, void (*item_formatter)(VLA* output, void* item));
void VLA_free(VLA* v);

#endif