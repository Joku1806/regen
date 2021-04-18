#ifndef GENERATOR_H
#define GENERATOR_H

#include "VLA.h"
#include "stack.h"

typedef struct NFA_State NFA_State;
typedef struct Transition Transition;

struct Transition {
    char *matching;
    NFA_State *advance_to;
};

struct NFA_State {
    VLA *transitions;
    size_t id;
};

typedef struct {
    NFA_State* start;
    NFA_State* stop;
} NFA;

NFA* generate_NFA_from_parsed_regex(ParserState *parsed);
NFA* optimize_NFA_layout(NFA* NFA);

#endif