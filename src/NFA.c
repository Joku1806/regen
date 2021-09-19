#include <stdbool.h>
#include <string.h>
#include "NFA.h"
#include "debug.h"

void free_node(Node *to_free);

Node *create_node(size_t *id) {
    Node *new = malloc(sizeof(Node));
    new->edges = VLA_initialize(1, sizeof(Edge));
    new->id = *id;

    *id += 1;
    return new;
}

void free_node(Node *to_free) {
    VLA_free(to_free->edges);
    free(to_free);
}

NFA *initialize_nfa() {
    NFA *new = malloc(sizeof(NFA));
    new->start = NULL;
    new->stop = NULL;
    return new;
}

void free_nfa(NFA *nfa, Node **nodes) {
    for (size_t index = 0; index < nfa->node_count; index++) {
        free_node(nodes[index]);
    }
    free(nfa);
}

Compact_NFA *initialize_compact_nfa(size_t node_count) {
    Compact_NFA *new = malloc(sizeof(Compact_NFA));
    new->nodes = calloc(node_count, sizeof(Compact_Node));
    new->node_count = node_count;
    new->start_node_index = 0;
    new->stop_node_index = 1;

    return new;
}

void free_compact_nfa(Compact_NFA *compact_nfa) {
    for (size_t node_index = 0; node_index < compact_nfa->node_count; node_index++) {
        for (size_t edge_index = 0; edge_index < compact_nfa->nodes[node_index].edge_count; edge_index++) {
            free(compact_nfa->nodes[node_index].edges[edge_index].matches);
        }
        free(compact_nfa->nodes[node_index].edges);
    }

    free(compact_nfa->nodes);
    free(compact_nfa);
}

Compact_Node create_compact_node(Node *from) {
    Compact_Node new = {
        .edges = calloc(VLA_get_length(from->edges), sizeof(Compact_Edge)),
        .edge_count = VLA_get_length(from->edges),
    };

    return new;
}

Compact_Edge create_compact_edge(Edge *from) {
    Compact_Edge new = {
        .matches = (uint8_t *)from->matching,
        .match_length = strlen(from->matching),
        .endpoint = from->endpoint->id,
    };

    return new;
}

void add_edge_between(Node *from, Node *to, char *matching) {
    if (from == NULL || to == NULL) {
        warn("Can't add edge between %p and %p because at least one of them doesn't exist.\n", from, to);
        return;
    }

    debug("Adding edge between states z%u and z%u matching %s.\n", from->id, to->id, matching);
    Edge *edge = (Edge *)VLA_reserve_next_slots(from->edges, 1);
    edge->matching = matching;
    edge->endpoint = to;
}

void add_empty_edge_between(Node *from, Node *to) {
    char *empty = calloc(1, sizeof(char));
    add_edge_between(from, to, empty);
}

Node *VLA_binding_get_node_pointer(VLA *v, signed long index) {
    VLA_assert_item_size_matches(v, sizeof(Node *));
    return *(Node **)VLA_get(v, index);
}

Edge *VLA_binding_get_edge(VLA *v, signed long index) {
    VLA_assert_item_size_matches(v, sizeof(Edge));
    return (Edge *)VLA_get(v, index);
}

void node_pointer_formatter(VLA *output, void *item) {
    Node *casted = *(Node **)item;
    VLA_append(output, "z");

    const int n = snprintf(NULL, 0, "%zu", casted->id);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted->id);

    VLA_batch_append(output, &buffer, n);
}