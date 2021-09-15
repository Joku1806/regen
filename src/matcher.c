#include <string.h>
#include "parser.h"
#include "generator.h"
#include "matcher.h"
#include "stack.h"
#include "debug.h"

VLA** setup_cycle_guards(Compact_NFA* nfa) {
    VLA** cycle_guards = calloc(nfa->node_count, sizeof(VLA*));
    bool* visited_nodes = calloc(nfa->node_count, sizeof(bool));
    Stack* node_indices = stack_initialize(nfa->node_count, sizeof(size_t));
    stack_push(node_indices, &nfa->start_node_index);

    while (VLA_get_length(node_indices) > 0) {
        size_t current_index = *(size_t*)stack_pop(node_indices);
        Compact_Node current_node = nfa->nodes[current_index];
        visited_nodes[current_index] = true;

        for (size_t edge_index = 0; edge_index < current_node.edge_count; edge_index++) {
            Compact_Edge current_edge = current_node.edges[edge_index];
            if (current_edge.match_length > 0) continue;
            if (visited_nodes[current_edge.endpoint]) {
                cycle_guards[current_edge.endpoint] = VLA_initialize(1, sizeof(size_t));
            } else {
                stack_push(node_indices, &current_edge.endpoint);
            }
        }
    }

    free(visited_nodes);
    VLA_free(node_indices);
    return cycle_guards;
}

void clear_cycle_guards(VLA** guards, size_t guard_count) {
    for (size_t index = 0; index < guard_count; index++) {
        if (guards[index] == NULL) continue;
        VLA_clear(guards[index]);
    }
}

bool would_enter_infinite_loop(VLA* cycle_guard, PartialMatch* match, Compact_Edge* edge) {
    if (cycle_guard == NULL) return false;
    if (edge->match_length > 0) return false;

    for (size_t cycle_entry_index = 0; cycle_entry_index < VLA_get_length(cycle_guard); cycle_entry_index++) {
        size_t cycle_entry = *(size_t*)VLA_get(cycle_guard, cycle_entry_index);
        if (match->length == cycle_entry) return true;
    }
    return false;
}

bool matches_edge(char* position, Compact_Edge* edge) {
    if (strlen(position) < edge->match_length) return false;
    return !memcmp(edge->matches, position, edge->match_length);
}

PartialMatch* take_matching_edge(PartialMatch* current_match, Compact_Edge* edge) {
    PartialMatch* advanced = calloc(1, sizeof(PartialMatch));
    advanced->length = current_match->length + edge->match_length;
    advanced->node_index = edge->endpoint;

    return advanced;
}

Match* match(char* to_match, char* regex, size_t* matches_count) {
    ParserState* state = parse_regex(regex);
    if (state->invalid) {
        printf("%s is not a syntactically correct regex.\n", regex);
        return 0;
    }

    NFA* nfa = generate_nfa_from_parsed_regex(state);
    // FIXME: Bin mir nicht sicher, ob der kompakte VLA wirklich einen großen Unterschied in der Geschwindigkeit ausmacht.
    // Und selbst falls es schneller ist, ob es den Aufwand ausgleicht, alles doppelt implementieren zu müssen.
    Compact_NFA* compacted = compact_generated_NFA(nfa);

    Stack* partial_matches = stack_initialize(5, sizeof(PartialMatch*));
    VLA* matches = VLA_initialize(5, sizeof(Match));
    VLA** cycle_guards = setup_cycle_guards(compacted);

    for (size_t offset = 0; offset < strlen(to_match); offset++) {
        PartialMatch* start = calloc(1, sizeof(PartialMatch));
        start->length = 0;
        start->node_index = compacted->start_node_index;
        stack_push(partial_matches, &start);

        while (VLA_get_length(partial_matches) > 0) {
            PartialMatch* current_match = *(PartialMatch**)stack_pop(partial_matches);

            if (current_match->node_index == compacted->stop_node_index) {
                Match* match = (Match*)VLA_reserve_next_slots(matches, 1);
                match->offset = offset;
                match->length = current_match->length;
            }

            if (offset + current_match->length > strlen(to_match)) continue;
            char* matching_position = to_match + offset + current_match->length;

            for (size_t edge_index = 0; edge_index < compacted->nodes[current_match->node_index].edge_count; edge_index++) {
                Compact_Edge* current_edge = &compacted->nodes[current_match->node_index].edges[edge_index];
                if (matches_edge(matching_position, current_edge)) {
                    VLA* responsible_guard = cycle_guards[current_edge->endpoint];
                    if (would_enter_infinite_loop(responsible_guard, current_match, current_edge)) continue;
                    PartialMatch* advanced_match = take_matching_edge(current_match, current_edge);
                    if (cycle_guards[advanced_match->node_index] != NULL) VLA_append(cycle_guards[advanced_match->node_index], &advanced_match->length);
                    stack_push(partial_matches, &advanced_match);
                }
            }
            free(current_match);
        }

        clear_cycle_guards(cycle_guards, compacted->node_count);
    }

    VLA_free(partial_matches);
    for (size_t delete_index = 0; delete_index < compacted->node_count; delete_index++) {
        if (cycle_guards[delete_index] == NULL) continue;
        VLA_free(cycle_guards[delete_index]);
    }
    free(cycle_guards);
    free_compact_nfa(compacted);

    *matches_count = VLA_get_length(matches);
    return (Match*)VLA_extract(matches);
}