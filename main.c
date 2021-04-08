#include <stdio.h>
#include "parser.h"
#include "generator.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Benutzung: %s Regex\n", argv[0]);
        return 0;
    }

    char* regex = argv[1];
    ParserState *state = parse_regex(regex);
    if (!state->invalid) {
        printf("%s ist ein syntaktisch richtiger Regex. Herzlichen Glückwunsch!\n", regex);
        // einfach nur sehen ob es kompiliert
        DFA* generated = generate_first_pass_from_parsed_regex(state);
        return 1;
    } else {
        printf("%s ist kein syntaktisch richtiger Regex.\n", regex);
        return 0;
    }
}