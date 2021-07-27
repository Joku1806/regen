#ifndef NFA_H
#define NFA_H

#include "VLA.h"

typedef struct Node Node;
typedef struct Edge Edge;
typedef struct NFA NFA;
typedef struct Compact_Node Compact_Node;
typedef struct Compact_Edge Compact_Edge;
typedef struct Compact_NFA Compact_NFA;

struct Edge {
    Node *advance_to;
    char *matching;
};

struct Node {
    VLA *NFA_Edges;
    size_t id;
};

struct NFA {
    Node *start;
    Node *stop;
    size_t number_of_nodes;
};

struct Compact_Edge {
    uint8_t *matching;
    size_t match_length;
    size_t target_index;
};

struct Compact_Node {
    Compact_Edge *outgoing_edges;
    size_t number_of_outgoing_edges;
};

struct Compact_NFA {
    Compact_Node *nodes;
    size_t number_of_nodes;
    size_t start_node_index;
    size_t stop_node_index;
};

void NFA_Node_formatter(VLA *output, void *item);
void NFA_Node_Pointer_formatter(VLA *output, void *item);
Node *VLA_binding_get_NFA_Node(VLA *v, signed long idx);
Node *VLA_binding_get_NFA_Node_Pointer(VLA *v, signed long idx);
Edge *VLA_binding_get_NFA_Edge(VLA *v, signed long idx);
Node *construct_NFA_Node(size_t *id);
Compact_Node generate_Compact_Node(Node *from);
Compact_Edge generate_Compact_Edge(Edge *from);
NFA *construct_NFA();
Compact_NFA *construct_Compact_NFA(size_t number_of_nodes);
Edge *construct_NFA_Edge(char *matching, Node *to);
void NFA_add_connection_between(Node *from, Node *to, char *matching);
void NFA_add_empty_connection_between(Node *from, Node *to);
void free_NFA_Node(Node *to_free);
void NFA_free(NFA *NFA, Node **nodes);

#endif