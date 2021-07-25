#include <stdbool.h>
#include <string.h>
#include "NFA.h"
#include "debug.h"

Node *construct_NFA_Node(size_t *id) {
    Node *new = malloc(sizeof(Node));
    new->NFA_Edges = VLA_initialize(1, sizeof(Edge));
    new->id = *id;
    // debug("Created NFA Node z%u\n", *id);

    *id += 1;
    return new;
}

void free_NFA_Node(Node *to_free) {
    // TODO: Überlegen, was eigentlich mit den erstellten Objekten aus construct_NFA_Edge() passieren soll,
    // nachdem sie in den VLA kopiert wurden. Einfach löschen?
    // Alternativ könnte man auch einen Weg finden, die Objekte direkt im VLA zu instanziieren, dann müsste man
    // nicht extra kopieren und dann löschen. Bin mir allerdings nicht sicher, wie praktisch/möglich das in C ist.
    // Ist möglich! Siehe https://stackoverflow.com/questions/28465151/initialize-array-starting-from-specific-address-in-memory-c-programming
    VLA_free(to_free->NFA_Edges);
    free(to_free);
}

Edge *construct_NFA_Edge(char *matching, Node *to) {
    Edge *new = malloc(sizeof(Edge));
    new->matching = matching;
    new->advance_to = to;
    return new;
}

NFA *construct_NFA() {
    NFA *new = malloc(sizeof(NFA));
    new->start = NULL;
    new->stop = NULL;
    return new;
}

Compact_NFA *construct_Compact_NFA(size_t number_of_nodes) {
    Compact_NFA *new = malloc(sizeof(Compact_NFA));
    new->nodes = calloc(number_of_nodes, sizeof(Compact_Node));
    new->number_of_nodes = number_of_nodes;
    new->start_node_index = 0;
    new->stop_node_index = 1;

    return new;
}

Compact_Node generate_Compact_Node(Node *from) {
    Compact_Node new = {
        .outgoing_edges = calloc(VLA_get_length(from->NFA_Edges), sizeof(Compact_Edge)),
        .number_of_outgoing_edges = VLA_get_length(from->NFA_Edges),
    };

    return new;
}

Compact_Edge generate_Compact_Edge(Edge *from) {
    Compact_Edge new = {
        .matching = (uint8_t *)from->matching,
        .match_length = strlen(from->matching),
        .target_index = from->advance_to->id,
    };

    return new;
}

void NFA_add_connection_between(Node *from, Node *to, char *matching) {
    if (from == NULL || to == NULL) {
        warn("At least one of the nodes (at %p and %p) is NULL. Can't connect them with a new edge.\n", from, to);
        return;
    }

    debug("Now adding connection from z%u to z%u matching %s.\n", from->id, to->id, matching);
    Edge *connection = construct_NFA_Edge(matching, to);
    VLA_append(from->NFA_Edges, connection);
}

void NFA_add_empty_connection_between(Node *from, Node *to) {
    char *empty = calloc(1, sizeof(char));
    NFA_add_connection_between(from, to, empty);
}

Node *VLA_binding_get_NFA_Node(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(Node));
    return (Node *)VLA_get(v, idx);
}

Node *VLA_binding_get_NFA_Node_Pointer(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(Node *));
    return *(Node **)VLA_get(v, idx);
}

void NFA_Node_formatter(VLA *formatter, void *item) {
    Node *casted = (Node *)item;
    VLA_append(formatter, "z");

    const int n = snprintf(NULL, 0, "%zu", casted->id);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted->id);

    VLA_batch_append(formatter, &buffer, n);
}

void NFA_Node_Pointer_formatter(VLA *formatter, void *item) {
    Node *casted = *(Node **)item;
    VLA_append(formatter, "z");

    const int n = snprintf(NULL, 0, "%zu", casted->id);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted->id);

    VLA_batch_append(formatter, &buffer, n);
}

Edge *VLA_binding_get_NFA_Edge(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(Edge));
    return (Edge *)VLA_get(v, idx);
}

void NFA_free(NFA *NFA, Node **nodes) {
    for (size_t idx = 0; idx < NFA->number_of_nodes; idx++) {
        debug("Now trying to free node at %p (id=%ld (supposed) vs %ld (real)).\n", nodes[idx], idx, nodes[idx]->id);
        free_NFA_Node(nodes[idx]);
    }
    free(NFA);
}