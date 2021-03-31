#ifndef VLA_H
#define VLA_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
    size_t capacity;
    size_t length;
    size_t item_size;
    uint8_t* data;
} VLA;

VLA* VLA_initialize(size_t capacity, size_t item_size);
void VLA_insert(VLA* v, void* address, size_t amount);
void VLA_delete_at_index(VLA* v, size_t idx);
void VLA_delete_at_index_order_safe(VLA* v, size_t idx);
void VLA_cleanup(VLA* v, void (*handler)(void*));

#endif