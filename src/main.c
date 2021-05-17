#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "generator.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Benutzung: %s Regex\n", argv[0]);
        return 0;
    }

    char* regex = argv[1];
    ParserState* state = parse_regex(regex);
    if (!state->invalid) {
        printf("%s ist ein syntaktisch richtiger Regex. Herzlichen Glückwunsch!\n", regex);
        // einfach nur sehen ob es kompiliert
        NFA* generated = generate_NFA_from_parsed_regex(state);
        printf("Got back generated NFA starting at %p.\n", generated->start);
        return 1;
    } else {
        printf("%s ist kein syntaktisch richtiger Regex.\n", regex);
        return 0;
    }
}