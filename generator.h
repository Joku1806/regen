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
};

typedef struct {
    DFA_State* start;
    DFA_State* stop;
} DFA;

#endif