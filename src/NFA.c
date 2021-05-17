#include "NFA.h"
#include "debug.h"

NFA_Node *construct_NFA_Node(size_t *id) {
    NFA_Node *new = malloc(sizeof(NFA_Node));
    new->NFA_Edges = VLA_initialize(1, sizeof(NFA_Edge));
    new->id = *id;
    // debug("Created NFA Node z%u\n", *id);

    *id += 1;
    return new;
}

void free_NFA_Node(NFA_Node *to_free) {
    // TODO: Überlegen, was eigentlich mit den erstellten Objekten aus construct_NFA_Edge() passieren soll,
    // nachdem sie in den VLA kopiert wurden. Einfach löschen?
    // Alternativ könnte man auch einen Weg finden, die Objekte direkt im VLA zu instanziieren, dann müsste man
    // nicht extra kopieren und dann löschen. Bin mir allerdings nicht sicher, wie praktisch/möglich das in C ist.
    // Ist möglich! Siehe https://stackoverflow.com/questions/28465151/initialize-array-starting-from-specific-address-in-memory-c-programming
    VLA_free(to_free->NFA_Edges);
    free(to_free);
}

NFA_Edge *construct_NFA_Edge(char *matching, NFA_Node *to) {
    NFA_Edge *new = malloc(sizeof(NFA_Edge));
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

void NFA_add_connection_between(NFA_Node *from, NFA_Node *to, char *matching) {
    if (from == NULL || to == NULL) {
        warn("At least one of the nodes (at %p and %p) is NULL. Can't connect them with a new edge.\n", from, to);
        return;
    }

    debug("Now adding connection from z%u to z%u matching %s.\n", from->id, to->id, matching);
    NFA_Edge *connection = construct_NFA_Edge(matching, to);
    VLA_append(from->NFA_Edges, connection, 1);
}

void NFA_add_empty_connection_between(NFA_Node *from, NFA_Node *to) {
    NFA_add_connection_between(from, to, NULL);
}

NFA_Node *VLA_binding_get_NFA_Node(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(NFA_Node));
    return (NFA_Node *)VLA_get(v, idx);
}

void NFA_Node_formatter(VLA *formatter, void *item) {
    NFA_Node *casted = (NFA_Node *)item;
    VLA_append(formatter, "z", 1);

    const int n = snprintf(NULL, 0, "%zu", casted->id);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted->id);

    VLA_append(formatter, &buffer, n);
}

NFA_Edge *VLA_binding_get_NFA_Edge(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(NFA_Edge));
    return (NFA_Edge *)VLA_get(v, idx);
}