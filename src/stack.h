#ifndef STACK_H
#define STACK_H

#include "VLA.h"

typedef VLA Stack;

Stack* stack_initialize(size_t capacity, size_t item_size);
void stack_push_n(Stack* s, void* address, size_t amount);
void stack_pop_n(Stack* s, size_t amount);
void* stack_pop(Stack* s);

#endif