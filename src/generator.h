#ifndef GENERATOR_H
#define GENERATOR_H

#include "stack.h"
#include "NFA.h"
#include "parser.h"

typedef struct GeneratorState {
    Stack *start_nodes;
    Stack *stop_nodes;
    Stack *group_counters;
    size_t *global_node_index;
    NFA *generated;
} GeneratorState;

// Interne Methoden
size_t VLA_binding_get_size_t(VLA *v, signed long idx);
void size_t_formatter(VLA *formatter, void *item);
GeneratorState *construct_GeneratorState();
void destruct_GeneratorState(GeneratorState *state);
void increment_current_group_counter(VLA *levels);
void advance_current_path(GeneratorState *state, char *match);
void backtrack_to_path_start(GeneratorState *state);
void loop_current_path_bidirectional(GeneratorState *state, size_t anchor_offset);
void loop_current_path_forward(GeneratorState *state, size_t anchor_offset);
void loop_current_path_backward(GeneratorState *state, size_t anchor_offset);
void open_new_block_level(GeneratorState *state);
void close_current_block_level(GeneratorState *state);

// API-Methoden
NFA *generate_NFA_from_parsed_regex(ParserState *parsed);
Compact_NFA *compact_generated_NFA(NFA *NFA);

#endif