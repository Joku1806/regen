#include <stdbool.h>
#include <string.h>
#include "matcher.h"
#include "stack.h"
#include "debug.h"

PartialMatch* advance_current_PartialMatch(PartialMatch* current, Compact_Edge* connection) {
    PartialMatch* next = calloc(1, sizeof(PartialMatch));
    next->match_length = current->match_length + connection->match_length;
    next->node_idx = connection->target_index;

    return next;
}

VLA** setup_cycle_guards(Compact_NFA* nfa) {
    VLA** cycle_guards = calloc(nfa->number_of_nodes, sizeof(VLA*));
    bool* visited_nodes = calloc(nfa->number_of_nodes, sizeof(bool));
    Stack* node_indices = stack_initialize(nfa->number_of_nodes, sizeof(size_t));
    stack_push(node_indices, &nfa->start_node_index);

    while (VLA_get_length(node_indices) > 0) {
        size_t current_index = *(size_t*)stack_pop(node_indices);
        Compact_Node current = nfa->nodes[current_index];
        visited_nodes[current_index] = true;

        for (size_t edge_index = 0; edge_index < current.number_of_outgoing_edges; edge_index++) {
            Compact_Edge c_edge = current.outgoing_edges[edge_index];
            if (c_edge.match_length > 0) continue;
            if (visited_nodes[c_edge.target_index]) {
                cycle_guards[c_edge.target_index] = VLA_initialize(1, sizeof(size_t));
            } else {
                stack_push(node_indices, &c_edge.target_index);
            }
        }
    }

    free(visited_nodes);
    VLA_free(node_indices);
    return cycle_guards;
}

void clear_cycle_guards(VLA** guards, size_t number_of_guards) {
    for (size_t idx = 0; idx < number_of_guards; idx++) {
        if (guards[idx] == NULL) continue;
        VLA_clear(guards[idx]);
    }
}

void remove_unused_cycle_guard(VLA* guard, size_t entry) {
    if (guard == NULL) {
        warn("Cycle guard is NULL, check the caller for logic errors!\n");
        return;
    }

    for (size_t search_idx = 0; search_idx < VLA_get_length(guard); search_idx++) {
        if (*(size_t*)VLA_get(guard, search_idx) == entry) {
            VLA_delete_at_index(guard, search_idx);
            return;
        }
    }

    warn("Couldn't find entry %lu in guard with size=%lu", entry, VLA_get_length(guard));
}

Match_Container* match(char* to_match, Compact_NFA* match_with) {
    Stack* paths = stack_initialize(5, sizeof(PartialMatch*));
    VLA* matches = VLA_initialize(5, sizeof(Match));
    VLA** cycle_guards = setup_cycle_guards(match_with);

    for (size_t match_offset = 0; match_offset < strlen(to_match); match_offset++) {
        PartialMatch* start = calloc(1, sizeof(PartialMatch));
        start->match_length = 0;
        start->node_idx = match_with->start_node_index;
        stack_push(paths, &start);

        while (VLA_get_length(paths) > 0) {
            PartialMatch* current = *(PartialMatch**)stack_pop(paths);

            if (current->node_idx == match_with->stop_node_index) {
                Match match = {
                    .offset = match_offset,
                    .length = current->match_length,
                };
                VLA_append(matches, &match);
            }

            if (match_offset + current->match_length > strlen(to_match)) continue;
            char* position = to_match + match_offset + current->match_length;
            Compact_Edge* possible_connections = match_with->nodes[current->node_idx].outgoing_edges;
            for (size_t edge_idx = 0; edge_idx < match_with->nodes[current->node_idx].number_of_outgoing_edges; edge_idx++) {
                if (edge_matches_current_position(&possible_connections[edge_idx], position)) {
                    if (would_enter_infinite_loop(cycle_guards, current, &possible_connections[edge_idx])) continue;
                    PartialMatch* next = advance_current_PartialMatch(current, &possible_connections[edge_idx]);
                    if (cycle_guards[next->node_idx] != NULL) VLA_append(cycle_guards[next->node_idx], &next->match_length);
                    stack_push(paths, &next);
                }
            }
            free(current);
        }

        clear_cycle_guards(cycle_guards, match_with->number_of_nodes);
    }

    VLA_free(paths);
    for (size_t delete_index = 0; delete_index < match_with->number_of_nodes; delete_index++) {
        if (cycle_guards[delete_index] == NULL) continue;
        VLA_free(cycle_guards[delete_index]);
    }

    return into_match_container(matches);
}

bool edge_matches_current_position(Compact_Edge* edge, char* position) {
    if (strlen(position) < edge->match_length) return false;
    return !memcmp(edge->matching, position, edge->match_length);
}

bool would_enter_infinite_loop(VLA** cycle_guards, PartialMatch* state, Compact_Edge* connection) {
    if (connection->match_length > 0) return false;
    if (cycle_guards[connection->target_index] == NULL) return false;

    for (size_t cycle_entry_index = 0; cycle_entry_index < VLA_get_length(cycle_guards[connection->target_index]); cycle_entry_index++) {
        size_t cycle_entry = *(size_t*)VLA_get(cycle_guards[connection->target_index], cycle_entry_index);
        if (state->match_length == cycle_entry) return true;
    }
    return false;
}

Match_Container* into_match_container(VLA* matches) {
    Match_Container* container = calloc(1, sizeof(Match_Container));
    container->number_of_matches = VLA_get_length(matches);
    container->matches = (Match*)VLA_extract(matches);
    return container;
}