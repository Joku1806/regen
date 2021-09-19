#include <string.h>
#include <stdbool.h>
#include "generator.h"
#include "stack.h"
#include "debug.h"

typedef struct Generator {
    Stack *block_start_nodes;
    Stack *block_stop_nodes;
    Stack *block_start_offsets;
    size_t *global_node_index;
    NFA *generated;
} Generator;

size_t VLA_binding_get_size_t(VLA *v, signed long index);
void size_t_formatter(VLA *output, void *item);
Generator *initialize_generator();
void free_generator(Generator *state);
void increment_current_block_offset(VLA *offsets);
void advance_current_path(Generator *state, char *match);
void backtrack_to_path_start(Generator *state);
void loop_current_path_bidirectional(Generator *state, size_t anchor_offset);
void loop_current_path_forward(Generator *state, size_t anchor_offset);
void loop_current_path_backward(Generator *state, size_t anchor_offset);
void open_new_block_level(Generator *state);
void close_current_block_level(Generator *state);

Generator *initialize_generator() {
    Generator *new = malloc(sizeof(Generator));
    new->block_start_nodes = stack_initialize(2, sizeof(Node *));
    new->block_stop_nodes = stack_initialize(2, sizeof(Node *));
    new->block_start_offsets = stack_initialize(2, sizeof(size_t));
    new->global_node_index = calloc(1, sizeof(size_t));
    new->generated = initialize_nfa();
    new->generated->start = create_node(new->global_node_index);
    new->generated->stop = create_node(new->global_node_index);

    stack_push(new->block_start_nodes, &(new->generated->start));
    stack_push(new->block_stop_nodes, &(new->generated->stop));
    // Einen zusätzlich, damit es beim letzten Aufruf von close_current_block_level() keinen Segfault gibt
    stack_push(new->block_start_offsets, &(size_t){0});
    stack_push(new->block_start_offsets, &(size_t){0});

    return new;
}

void free_generator(Generator *generator) {
    VLA_free(generator->block_start_offsets);
    VLA_free(generator->block_start_nodes);
    VLA_free(generator->block_stop_nodes);
    free(generator->global_node_index);
    free(generator);
}

size_t VLA_binding_get_size_t(VLA *v, signed long index) {
    VLA_assert_item_size_matches(v, sizeof(size_t));
    return *(size_t *)VLA_get(v, index);
}

void size_t_formatter(VLA *output, void *item) {
    size_t casted = *(size_t *)item;
    const int n = snprintf(NULL, 0, "%zu", casted);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted);

    VLA_batch_append(output, &buffer, n);
}

void increment_current_block_offset(VLA *offsets) {
    size_t *offset = (size_t *)VLA_get(offsets, -1);
    *offset += 1;
}

void open_new_block_level(Generator *generator) {
    Node *last_start = VLA_binding_get_node_pointer(generator->block_start_nodes, -1);
    Node *start = create_node(generator->global_node_index);
    Node *stop = create_node(generator->global_node_index);

    add_empty_edge_between(last_start, start);
    stack_push(generator->block_start_nodes, &start);
    stack_push(generator->block_stop_nodes, &stop);
    increment_current_block_offset(generator->block_start_offsets);
    stack_push(generator->block_start_offsets, &(size_t){0});
}

void close_current_block_level(Generator *generator) {
    Node *last_start = VLA_binding_get_node_pointer(generator->block_start_nodes, -1);
    Node *block_stop = VLA_binding_get_node_pointer(generator->block_stop_nodes, -1);

    size_t offset = VLA_binding_get_size_t(generator->block_start_offsets, -1);
    stack_pop_n(generator->block_start_offsets, 1);

    add_empty_edge_between(last_start, block_stop);
    stack_pop_n(generator->block_start_nodes, offset);
    stack_pop_n(generator->block_stop_nodes, 1);
    stack_push(generator->block_start_nodes, &block_stop);
    increment_current_block_offset(generator->block_start_offsets);
}

void insert_proxy_start(Generator *generator) {
    char *empty = calloc(1, sizeof(char));
    advance_current_path(generator, empty);
}

void advance_current_path(Generator *generator, char *match) {
    Node *last_start = VLA_binding_get_node_pointer(generator->block_start_nodes, -1);
    Node *new = create_node(generator->global_node_index);
    add_edge_between(last_start, new, match);
    stack_push(generator->block_start_nodes, &new);
    increment_current_block_offset(generator->block_start_offsets);
}

void backtrack_to_path_start(Generator *generator) {
    size_t offset = VLA_binding_get_size_t(generator->block_start_offsets, -1);
    Node *last_start = VLA_binding_get_node_pointer(generator->block_start_nodes, -1);
    Node *path_stop = VLA_binding_get_node_pointer(generator->block_stop_nodes, -1);

    add_empty_edge_between(last_start, path_stop);
    stack_pop_n(generator->block_start_nodes, offset);
    stack_pop_n(generator->block_start_offsets, 1);
    stack_push(generator->block_start_offsets, &(size_t){0});
}

void loop_current_path_bidirectional(Generator *generator, size_t anchor_offset) {
    Node *loop_start = VLA_binding_get_node_pointer(generator->block_start_nodes, -anchor_offset);
    Node *loop_stop = VLA_binding_get_node_pointer(generator->block_start_nodes, -1);

    add_empty_edge_between(loop_start, loop_stop);
    add_empty_edge_between(loop_stop, loop_start);
}

void loop_current_path_forward(Generator *generator, size_t anchor_offset) {
    Node *loop_start = VLA_binding_get_node_pointer(generator->block_start_nodes, -anchor_offset);
    Node *loop_stop = VLA_binding_get_node_pointer(generator->block_start_nodes, -1);

    add_empty_edge_between(loop_start, loop_stop);
}

void loop_current_path_backward(Generator *generator, size_t anchor_offset) {
    Node *loop_start = VLA_binding_get_node_pointer(generator->block_start_nodes, -anchor_offset);
    Node *loop_stop = VLA_binding_get_node_pointer(generator->block_start_nodes, -1);

    add_empty_edge_between(loop_stop, loop_start);
}

NFA *generate_nfa_from_parsed_regex(ParserState *parsed) {
    Generator *generator = initialize_generator();

    for (size_t index = 0; index < strlen(parsed->regex); index++) {
        Token current = parsed->tokens[index];

        if (current == block_open) {
            open_new_block_level(generator);
        } else if (current == block_close) {
            close_current_block_level(generator);
        } else if (current == utf8_codepoint) {
            if (index + 1 < strlen(parsed->regex) && (parsed->tokens[index + 1] == mod_any || parsed->tokens[index + 1] == mod_multiple)) {
                insert_proxy_start(generator);
            }

            char *match = calloc(2, sizeof(char));
            match[0] = parsed->regex[index];
            advance_current_path(generator, match);
        } else if (current == mod_choice) {
            backtrack_to_path_start(generator);
        } else if (current == mod_any) {
            loop_current_path_bidirectional(generator, 2);
        } else if (current == mod_multiple) {
            loop_current_path_backward(generator, 2);
        } else if (current == mod_optional) {
            loop_current_path_forward(generator, 2);
        } else {
            // TODO: Unterstützung von Repetition/ValueRange
            warn("NFA generation for tokens of type %s is not handled yet.\n", get_token_description(current));
        }
    }

    close_current_block_level(generator);
    NFA *generated = generator->generated;
    generated->node_count = *generator->global_node_index;
    free_parser_state(parsed);
    free_generator(generator);
    return generated;
}

Compact_NFA *compact_generated_NFA(NFA *nfa) {
    Compact_NFA *compact_nfa = initialize_compact_nfa(nfa->node_count);
    Node **visited_nodes = calloc(nfa->node_count, sizeof(Node *));
    visited_nodes[nfa->start->id] = nfa->start;
    Stack *visitor_order = stack_initialize(nfa->node_count, sizeof(Node *));
    stack_push(visitor_order, &(nfa->start));

    while (VLA_get_length(visitor_order) > 0) {
        VLA_print(visitor_order, node_pointer_formatter);
        Node *visiting = *(Node **)stack_pop(visitor_order);
        debug("NFA state at %p with id=%lu and %lu outgoing edges.\n", visiting, visiting->id, VLA_get_length(visiting->edges));
        compact_nfa->nodes[visiting->id] = create_compact_node(visiting);

        for (size_t index = 0; index < compact_nfa->nodes[visiting->id].edge_count; index++) {
            Edge *edge = VLA_binding_get_edge(visiting->edges, index);
            compact_nfa->nodes[visiting->id].edges[index] = create_compact_edge(edge);
            if (visited_nodes[edge->endpoint->id] == NULL) {
                visited_nodes[edge->endpoint->id] = edge->endpoint;
                stack_push(visitor_order, &(edge->endpoint));
            }
        }
    }

    VLA_free(visitor_order);
    free_nfa(nfa, visited_nodes);
    free(visited_nodes);
    return compact_nfa;
}