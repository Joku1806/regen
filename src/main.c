#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "parser.h"
#include "generator.h"
#include "matcher.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Benutzung: %s Regex Text\n", argv[0]);
        return 0;
    }

    char* regex = argv[1];
    char* text = argv[2];
    ParserState* state = parse_regex(regex);
    if (!state->invalid) {
        printf("%s ist ein syntaktisch richtiger Regex. Herzlichen Glückwunsch!\n", regex);
        NFA* generated = generate_NFA_from_parsed_regex(state);
        Compact_NFA* compacted = compact_generated_NFA(generated);
        debug("Kompakter NFA hat %ld Knoten.\n", compacted->number_of_nodes);

        debug("z%lu ist Startzustand, z%lu ist Endzustand\n", compacted->start_node_index, compacted->stop_node_index);

        for (size_t node_idx = 0; node_idx < compacted->number_of_nodes; node_idx++) {
            Compact_Node current_node = compacted->nodes[node_idx];
            for (size_t edge_idx = 0; edge_idx < current_node.number_of_outgoing_edges; edge_idx++) {
                Compact_Edge current_edge = current_node.outgoing_edges[edge_idx];
                debug("z%lu -> z%lu mit %s (%lu Bytes)\n", node_idx, current_edge.target_index, current_edge.matching, current_edge.match_length);
            }
        }

        printf("Input: %s\n", text);
        Match_Container* container = match(text, compacted);
        for (size_t match_idx = 0; match_idx < container->number_of_matches; match_idx++) {
            Match current = container->matches[match_idx];
            // FIXME: keinen hässlichen Cast benutzen, vielleicht mit fwrite()?
            printf("Matched %.*s (Offset=%lu, Length=%lu)\n", (int)current.length, text + current.offset, current.offset, current.length);
        }

        return 1;
    } else {
        printf("%s ist kein syntaktisch richtiger Regex.\n", regex);
        return 0;
    }
}