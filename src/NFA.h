#ifndef NFA_H
#define NFA_H

#include "VLA.h"

typedef struct Node Node;
typedef struct Edge Edge;
typedef struct NFA NFA;
typedef struct Compact_Node Compact_Node;
typedef struct Compact_Edge Compact_Edge;
typedef struct Compact_NFA Compact_NFA;

struct NFA {
    Node *start;
    Node *stop;
    size_t node_count;
};

struct Node {
    VLA *edges;
    size_t id;
};

struct Edge {
    Node *endpoint;
    char *matching;
};

struct Compact_NFA {
    Compact_Node *nodes;
    size_t node_count;
    size_t start_node_index;
    size_t stop_node_index;
};

struct Compact_Node {
    Compact_Edge *edges;
    size_t edge_count;
};

struct Compact_Edge {
    uint8_t *matches;
    size_t match_length;
    size_t endpoint;
};

Node *create_node(size_t *id);
void free_node(Node *to_free);
void add_edge_between(Node *from, Node *to, char *matching);
void add_empty_edge_between(Node *from, Node *to);
Compact_Node create_compact_node(Node *from);
Compact_Edge create_compact_edge(Edge *from);
NFA *initialize_nfa();
void free_nfa(NFA *NFA, Node **nodes);
Compact_NFA *initialize_compact_nfa(size_t node_count);
void free_compact_nfa(Compact_NFA *compact_nfa);

Node *VLA_binding_get_node(VLA *v, signed long index);
Node *VLA_binding_get_node_pointer(VLA *v, signed long index);
Edge *VLA_binding_get_edge(VLA *v, signed long index);
void node_formatter(VLA *output, void *item);
void node_pointer_formatter(VLA *output, void *item);

#endif