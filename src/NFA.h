#ifndef NFA_H
#define NFA_H

#include "VLA.h"

typedef struct NFA NFA;
typedef struct NFA_Node NFA_Node;
typedef struct Transition Transition;

struct Transition {
    char *matching;
    NFA_Node *advance_to;
};

struct NFA_Node {
    VLA *transitions;
    size_t id;
};

struct NFA {
    NFA_Node *start;
    NFA_Node *stop;
};

void NFA_Node_formatter(VLA *formatter, void *item);
NFA_Node *VLA_binding_get_NFA_Node(VLA *v, signed long idx);
NFA_Node *construct_NFA_Node(size_t *id);
NFA *construct_NFA();
Transition *construct_NFA_Transition(char *matching, NFA_Node *to);
void NFA_add_connection_between(NFA_Node *from, NFA_Node *to, char *matching);
void NFA_add_empty_connection_between(NFA_Node *from, NFA_Node *to);

#endif