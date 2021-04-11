#ifndef GENERATOR_H
#define GENERATOR_H

#include "VLA.h"

typedef struct DFA_State DFA_State;
typedef struct Transition Transition;

struct Transition {
    char *matching;
    DFA_State *advance_to;
};

struct DFA_State {
    VLA *transitions;
    size_t id;
};

typedef struct {
    DFA_State* start;
    DFA_State* stop;
} DFA;

DFA* generate_DFA_from_parsed_regex(ParserState *parsed);
DFA* optimize_DFA_layout(DFA* dfa);

#endif