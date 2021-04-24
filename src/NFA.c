#include "NFA.h"
#include "debug.h"  // TODO: sollte vielleicht mit VLA.h mit importiert werden

NFA_Node *construct_NFA_Node(size_t *id) {
    NFA_Node *new = malloc(sizeof(NFA_Node));
    new->transitions = VLA_initialize(1, sizeof(Transition));
    new->id = *id;
    *id += 1;
    return new;
}

NFA *construct_NFA(size_t *state_start_id) {
    NFA *new = malloc(sizeof(NFA));
    new->start = construct_NFA_Node(state_start_id);
    new->stop = construct_NFA_Node(state_start_id);
    return new;
}

Transition *construct_NFA_Transition(char *matching, NFA_Node *to) {
    Transition *new = malloc(sizeof(Transition));
    new->matching = matching;
    new->advance_to = to;
    return new;
}

void NFA_add_connection_between(NFA_Node *from, NFA_Node *to, char *matching) {
    if (from == NULL || to == NULL) panic("At least one of the states doesn't exist, can't form connection between them.\n");
    debug("Now adding connection from z%lu to z%lu matching %s.\n", from->id, to->id, matching);
    Transition *connection = construct_NFA_Transition(matching, to);
    VLA_append(from->transitions, connection, 1);
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