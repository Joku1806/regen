#include <stdbool.h>
#include <string.h>
#include "matcher.h"
#include "stack.h"
#include "debug.h"

PartialMatch* advance_current_PartialMatch(PartialMatch* current, Compact_Edge* connection) {
    PartialMatch* next = calloc(1, sizeof(PartialMatch));

    if (connection->match_length == 0) {
        current->references++;
        next->infinite_loop_detector = current;
    } else {
        next->infinite_loop_detector = NULL;
    }

    next->match_length = current->match_length + connection->match_length;
    next->node_idx = connection->target_index;

    return next;
}

Match_Container* match(char* to_match, Compact_NFA* match_with) {
    Stack* paths = stack_initialize(5, sizeof(PartialMatch*));
    VLA* matches = VLA_initialize(5, sizeof(Match));
    VLA_set_data_freeing_policy(matches, immutable);

    for (size_t match_offset = 0; match_offset < strlen(to_match); match_offset++) {
        PartialMatch* start = calloc(1, sizeof(PartialMatch));
        start->infinite_loop_detector = NULL;
        start->references = 0;
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
                VLA_append(matches, &match, 1);
            }

            if (match_offset + current->match_length >= strlen(to_match)) continue;
            char* position = to_match + match_offset + current->match_length;
            Compact_Edge* possible_connections = match_with->nodes[current->node_idx].outgoing_edges;
            for (size_t edge_idx = 0; edge_idx < match_with->nodes[current->node_idx].number_of_outgoing_edges; edge_idx++) {
                if (edge_matches_current_position(&possible_connections[edge_idx], position)) {
                    if (would_enter_inifinite_loop(current, &possible_connections[edge_idx])) continue;
                    PartialMatch* next = advance_current_PartialMatch(current, &possible_connections[edge_idx]);
                    stack_push(paths, &next);
                }
            }
            free_PartialMatch_branch(current);
        }
    }

    Match_Container* container = into_match_container(matches);
    VLA_free(paths);
    VLA_free(matches);
    return container;
}

bool edge_matches_current_position(Compact_Edge* edge, char* position) {
    if (strlen(position) < edge->match_length) return false;
    return !memcmp(edge->matching, position, edge->match_length);
}

bool would_enter_inifinite_loop(PartialMatch* state, Compact_Edge* connection) {
    if (connection->match_length > 0) return false;

    PartialMatch* backtrack = state;
    while (backtrack->infinite_loop_detector != NULL) {
        debug("(From z%lu) Currently checking z%lu for infinite loop occurance.\n", state->node_idx, backtrack->node_idx);
        if (backtrack->infinite_loop_detector->node_idx == connection->target_index) {
            debug("z%lu -> z%lu would enter infinite loop, removing this path!\n", state->node_idx, connection->target_index);
            return true;
        }
        backtrack = backtrack->infinite_loop_detector;
    }

    return false;
}

void free_PartialMatch_branch(PartialMatch* starting_at) {
    if (starting_at->references > 0) return;
    PartialMatch* backtrack = starting_at->infinite_loop_detector;

    while (backtrack != NULL && backtrack->references == 1) {
        PartialMatch* tmp = backtrack->infinite_loop_detector;
        debug("Freeing PartialMatch in state z%lu\n", backtrack->node_idx);
        free(backtrack);
        backtrack = tmp;
    }

    if (backtrack != NULL) backtrack->references--;
    free(starting_at);
}

Match_Container* into_match_container(VLA* matches) {
    Match_Container* container = calloc(1, sizeof(Match_Container));
    container->matches = (Match*)matches->data;
    container->number_of_matches = VLA_get_length(matches);
    return container;
}