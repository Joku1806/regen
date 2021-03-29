#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "validator.h"

unsigned char grammar_table[7][7] = {
    {1, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1},
    {1, 1, 0, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1}
};

ParserState* construct_parser_state() {
    ParserState* state = malloc(sizeof(ParserState));
    state->token_history = construct_linked_list();
    // Dummy-Element, damit man direkt am Anfang auf grammar_table zugreifen kann.
    // block_open, weil es Anfang genau einen globalen Block gibt und die Regeln dafür komplett gleich sind.
    linked_list_insert(state->token_history, block_open);
    state->escape_active = 0;
    state->open_blocks = 0;
    state->invalid = 0;
    return state;
}

Token get_token_type(char token, int escape_active) {
    if (escape_active) return character;
    if (token == '(') return block_open;
    if (token == ')') return block_close;
    if (token == '*') return mod_multiple;
    if (token == '+') return mod_choice;
    if (token == '\\') return escape;
    if (token == ' ' || token == '\t' || token == '\n') return whitespace;
    return character;
}

ParserState* parse_regex(char *regex) {
    ParserState *state = construct_parser_state();
    for (size_t idx = 0; idx < strlen(regex); idx++) {
        linked_list_insert(state->token_history, get_token_type(regex[idx], state->escape_active));
        Token previous = state->token_history->last->previous->token;
        Token current = state->token_history->last->token;
        if (!grammar_table[previous][current] || current == block_close && state->open_blocks == 0) {
            state->invalid = 1;
            return state;
        }

        if (current == block_open) state->open_blocks++;
        if (current == block_close) state->open_blocks--;
        
        if (current == escape) state->escape_active = 1;
        else state->escape_active = 0;
    }

    // prüfen, ob am Ende noch Gruppen angefangen und dann nicht geschlossen wurden
    Token last = state->token_history->last->token;
    if (state->open_blocks != 0 || last == mod_choice || last == escape) {
        state->invalid = 1;
        return state;
    }
    
    // Dummy-Element vom Anfang entfernen
    linked_list_remove_nth(state->token_history, 0);
    linked_list_iterate(state->token_history);
    return state;
}