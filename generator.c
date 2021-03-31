#include "parser_defs.h"
#include "generator.h"

DFA* construct_DFA() {
    DFA* new = malloc(sizeof(DFA));
    new->start = construct_DFA_State(NULL);
    new->stop = construct_DFA_State(NULL);
    return new;
}

Transition* construct_DFA_Transition(char* matching) {
    Transition *new = malloc(sizeof(Transition));
    new->matching = matching;
    new->advance_to = NULL;
    return new;
}

DFA_State* construct_DFA_State(char matching) {
    DFA_State *new = malloc(sizeof(DFA_State));
    char* casted = malloc(2);
    casted[0] = matching;
    casted[1] = '\0';
    new->transitions = VLA_initialize(1, sizeof(Transition));
    Transition *t = construct_DFA_Transition(casted);
    VLA_insert(new->transitions, t, 1);
    return new;
}

DFA* generate_first_pass_from_parsed_regex(char *original_regex, Token *parsed_tokens) {
    DFA *generated = construct_DFA();
    for (size_t idx = 0; idx < strlen(original_regex); idx++) {
        Token current = parsed_tokens[idx];
        if (current == whitespace || current == mod_escape) continue;
        if (current == character) {
            DFA_State *new = construct_DFA_State(original_regex[idx]);
        
        }
    }
}