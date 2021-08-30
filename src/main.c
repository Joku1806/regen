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
    if (state->invalid) {
        printf("%s ist kein syntaktisch richtiger Regex.\n", regex);
        return 0;
    }

    debug("%s is a syntactically correct regex. Congratulations!\n", regex);
    NFA* generated = generate_nfa_from_parsed_regex(state);
    // FIXME: Bin mir nicht sicher, ob der kompakte VLA wirklich einen großen Vorteil in der Geschwindigkeit hat.
    // Und selbst falls es schneller ist, ob es den Aufwand ausgleicht, alles doppelt implementieren zu müssen
    Compact_NFA* compacted = compact_generated_NFA(generated);
    debug("Compact NFA has %ld states.\n", compacted->node_count);
    debug("z%lu is start state, z%lu is end state\n", compacted->start_node_index, compacted->stop_node_index);

    for (size_t node_index = 0; node_index < compacted->node_count; node_index++) {
        Compact_Node current_node = compacted->nodes[node_index];
        for (size_t edge_index = 0; edge_index < current_node.edge_count; edge_index++) {
            Compact_Edge current_edge = current_node.edges[edge_index];
            debug("z%lu -> z%lu matching %s (%lu Bytes)\n", node_index, current_edge.endpoint, current_edge.matches, current_edge.match_length);
        }
    }

    size_t matches_count = 0;
    Match* matches = match(text, compacted, &matches_count);
    printf("Input: %s\n", text);
    for (size_t match_index = 0; match_index < matches_count; match_index++) {
        Match current = matches[match_index];
        printf("Habe \"%.*s\" gefunden (Offset=%lu, Länge=%lu)\n", (int)current.length, text + current.offset, current.offset, current.length);
    }
    free(matches);

    return 1;
}