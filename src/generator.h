#ifndef GENERATOR_H
#define GENERATOR_H

#include "stack.h"
#include "NFA.h"

typedef struct GeneratorState {
    Stack* start_nodes;
    Stack* stop_nodes;
    Stack* group_counters;
    size_t* global_node_index;
    size_t token_index;
    NFA* generated;
} GeneratorState;

NFA* generate_NFA_from_parsed_regex(ParserState* parsed);
NFA* optimize_NFA_layout(NFA* NFA);

#endif