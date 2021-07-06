#include <string.h>
#include <stdbool.h>
#include "generator.h"
#include "debug.h"

GeneratorState *construct_GeneratorState() {
    GeneratorState *new = malloc(sizeof(GeneratorState));
    new->start_nodes = stack_initialize(2, sizeof(Node *));
    new->stop_nodes = stack_initialize(2, sizeof(Node *));
    new->group_counters = stack_initialize(2, sizeof(size_t));
    new->global_node_index = calloc(1, sizeof(size_t));
    new->generated = construct_NFA();

    VLA_set_item_formatter(new->start_nodes, NFA_Node_Pointer_formatter);
    VLA_set_item_formatter(new->stop_nodes, NFA_Node_Pointer_formatter);
    VLA_set_item_formatter(new->group_counters, size_t_formatter);

    new->generated->start = construct_NFA_Node(new->global_node_index);
    new->generated->stop = construct_NFA_Node(new->global_node_index);

    stack_push(new->start_nodes, &(new->generated->start));
    stack_push(new->stop_nodes, &(new->generated->stop));
    // Einen zusätzlich, damit es beim letzten Aufruf von close_current_block_level() keinen Segfault gibt
    stack_push(new->group_counters, &(size_t){0});
    stack_push(new->group_counters, &(size_t){0});

    return new;
}

void destruct_GeneratorState(GeneratorState *state) {
    VLA_free(state->group_counters);
    VLA_free(state->start_nodes);
    VLA_free(state->stop_nodes);
    free(state->global_node_index);
    free(state);
}

size_t VLA_binding_get_size_t(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(size_t));
    return *(size_t *)VLA_get(v, idx);
}

void size_t_formatter(VLA *formatter, void *item) {
    size_t casted = *(size_t *)item;
    const int n = snprintf(NULL, 0, "%zu", casted);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted);

    VLA_append(formatter, &buffer, n);
}

void increment_current_group_counter(VLA *levels) {
    VLA_replace_at_index(levels, &(size_t){VLA_binding_get_size_t(levels, -1) + 1}, -1);
}

void open_new_block_level(GeneratorState *state) {
    Node *last_start = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -1);
    Node *start = construct_NFA_Node(state->global_node_index);
    Node *stop = construct_NFA_Node(state->global_node_index);

    NFA_add_empty_connection_between(last_start, start);
    stack_push(state->start_nodes, &start);
    stack_push(state->stop_nodes, &stop);
    increment_current_group_counter(state->group_counters);
    stack_push(state->group_counters, &(size_t){0});
}

void close_current_block_level(GeneratorState *state) {
    Node *last_start = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -1);
    Node *block_stop = VLA_binding_get_NFA_Node_Pointer(state->stop_nodes, -1);

    size_t groups = VLA_binding_get_size_t(state->group_counters, -1);
    stack_pop_n(state->group_counters, 1);

    NFA_add_empty_connection_between(last_start, block_stop);
    stack_pop_n(state->start_nodes, groups);
    stack_pop_n(state->stop_nodes, 1);
    stack_push(state->start_nodes, &block_stop);
    increment_current_group_counter(state->group_counters);
}

void insert_proxy_start(GeneratorState *state) {
    char *empty = calloc(1, sizeof(char));
    advance_current_path(state, empty);
}

void advance_current_path(GeneratorState *state, char *match) {
    Node *last_start = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -1);
    Node *new = construct_NFA_Node(state->global_node_index);
    NFA_add_connection_between(last_start, new, match);
    stack_push(state->start_nodes, &new);
    increment_current_group_counter(state->group_counters);
}

void backtrack_to_path_start(GeneratorState *state) {
    size_t groups = VLA_binding_get_size_t(state->group_counters, -1);
    Node *last_start = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -1);
    Node *path_stop = VLA_binding_get_NFA_Node_Pointer(state->stop_nodes, -1);

    NFA_add_empty_connection_between(last_start, path_stop);
    stack_pop_n(state->start_nodes, groups);
    stack_pop_n(state->group_counters, 1);
    stack_push(state->group_counters, &(size_t){0});
}

void loop_current_path_bidirectional(GeneratorState *state, size_t anchor_offset) {
    Node *loop_start = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -anchor_offset);
    Node *loop_stop = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -1);

    NFA_add_empty_connection_between(loop_start, loop_stop);
    NFA_add_empty_connection_between(loop_stop, loop_start);
}

void loop_current_path_forward(GeneratorState *state, size_t anchor_offset) {
    Node *loop_start = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -anchor_offset);
    Node *loop_stop = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -1);

    NFA_add_empty_connection_between(loop_start, loop_stop);
}

void loop_current_path_backward(GeneratorState *state, size_t anchor_offset) {
    Node *loop_start = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -anchor_offset);
    Node *loop_stop = VLA_binding_get_NFA_Node_Pointer(state->start_nodes, -1);

    NFA_add_empty_connection_between(loop_stop, loop_start);
}

NFA *generate_NFA_from_parsed_regex(ParserState *parsed) {
    GeneratorState *state = construct_GeneratorState();

    for (size_t idx = 0; idx < strlen(parsed->regex); idx++) {
        Token current = parsed->tokens[idx];

        if (current == block_open) {
            open_new_block_level(state);
        } else if (current == block_close) {
            close_current_block_level(state);
        } else if (current == character) {
            if (idx + 1 < strlen(parsed->regex) && (parsed->tokens[idx + 1] == mod_any || parsed->tokens[idx + 1] == mod_multiple)) {
                insert_proxy_start(state);
            }

            char *match = calloc(2, sizeof(char));
            match[0] = parsed->regex[idx];
            advance_current_path(state, match);
        } else if (current == mod_choice) {
            backtrack_to_path_start(state);
        } else if (current == mod_any) {
            loop_current_path_bidirectional(state, 2);
        } else if (current == mod_multiple) {
            loop_current_path_backward(state, 2);
        } else if (current == mod_optional) {
            loop_current_path_forward(state, 2);
        } else {
            warn("Encountered unexpected token %s, should have been removed in the parsing stage.\n", get_token_description(current));
        }
    }

    close_current_block_level(state);
    NFA *generated = state->generated;
    generated->number_of_nodes = *state->global_node_index;
    destruct_GeneratorState(state);

    // Kein hängender Pointer, weil die data policy auf immutable gesetzt ist
    return generated;
}

Compact_NFA *compact_generated_NFA(NFA *NFA) {
    Node **lookup = calloc(NFA->number_of_nodes, sizeof(Node *));
    Compact_NFA *cNFA = construct_Compact_NFA(NFA->number_of_nodes);
    Stack *visitor_order = stack_initialize(NFA->number_of_nodes, sizeof(Node *));
    VLA_set_item_formatter(visitor_order, NFA_Node_Pointer_formatter);
    stack_push(visitor_order, &(NFA->start));

    while (VLA_get_length(visitor_order) > 0) {
        VLA_print(visitor_order);
        Node *visiting = *(Node **)stack_pop(visitor_order);
        debug("Node at %p with id=%lu, and %lu edges.\n", visiting, visiting->id, VLA_get_length(visiting->NFA_Edges));
        lookup[visiting->id] = visiting;
        cNFA->nodes[visiting->id] = generate_Compact_Node(visiting);

        for (size_t idx = 0; idx < cNFA->nodes[visiting->id].number_of_outgoing_edges; idx++) {
            Edge *connection = VLA_binding_get_NFA_Edge(visiting->NFA_Edges, idx);
            cNFA->nodes[visiting->id].outgoing_edges[idx] = generate_Compact_Edge(connection);
            if (lookup[connection->advance_to->id] == NULL) {
                stack_push(visitor_order, &(connection->advance_to));
            }
        }
    }

    VLA_free(visitor_order);
    NFA_free(NFA, lookup);
    free(lookup);
    return cNFA;
}