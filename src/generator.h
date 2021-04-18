#ifndef GENERATOR_H
#define GENERATOR_H

#include "VLA.h"
#include "stack.h"

typedef struct NFA NFA;
typedef struct NFA_Node NFA_Node;
typedef struct Transition Transition;

typedef struct GeneratorState {
    Stack* start_nodes;
    Stack* stop_nodes;
    Stack* group_counters;
    size_t* global_node_index;
    size_t token_index;
    NFA* generated;
} GeneratorState;

struct Transition {
    char* matching;
    NFA_Node* advance_to;
};

struct NFA_Node {
    VLA* transitions;
    size_t id;
};

struct NFA {
    NFA_Node* start;
    NFA_Node* stop;
};

NFA* generate_NFA_from_parsed_regex(ParserState* parsed);
NFA* optimize_NFA_layout(NFA* NFA);

#endif