#ifndef GENERATOR_H
#define GENERATOR_H

#include "stack.h"
#include "NFA.h"
#include "parser.h"

typedef struct Generator {
    Stack *block_start_nodes;
    Stack *block_stop_nodes;
    Stack *block_start_offsets;
    size_t *global_node_index;
    NFA *generated;
} Generator;

size_t VLA_binding_get_size_t(VLA *v, signed long index);
void size_t_formatter(VLA *output, void *item);
Generator *initialize_generator();
void free_generator(Generator *state);
void increment_current_block_offset(VLA *levels);
void advance_current_path(Generator *state, char *match);
void backtrack_to_path_start(Generator *state);
void loop_current_path_bidirectional(Generator *state, size_t anchor_offset);
void loop_current_path_forward(Generator *state, size_t anchor_offset);
void loop_current_path_backward(Generator *state, size_t anchor_offset);
void open_new_block_level(Generator *state);
void close_current_block_level(Generator *state);
NFA *generate_nfa_from_parsed_regex(ParserState *parsed);
Compact_NFA *compact_generated_NFA(NFA *NFA);

#endif