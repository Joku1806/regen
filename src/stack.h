#ifndef STACK_H
#define STACK_H

#include "VLA.h"

typedef VLA Stack;

Stack* stack_initialize(size_t capacity, size_t item_size);
void stack_push(Stack* s, void* address);
void stack_pop_n(Stack* s, size_t amount);
uint8_t* stack_pop(Stack* s);

#endif