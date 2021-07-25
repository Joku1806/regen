#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "parser.h"

bool grammar_blocklist[TOKEN_COUNT][TOKEN_COUNT] = {
    [block_open][block_close] = true,
    [block_open][mod_optional] = true,
    [block_open][mod_any] = true,
    [block_open][mod_multiple] = true,
    [block_open][mod_choice] = true,
    [mod_optional][mod_optional] = true,
    [mod_optional][mod_any] = true,
    [mod_optional][mod_multiple] = true,
    [mod_any][mod_optional] = true,
    [mod_any][mod_any] = true,
    [mod_any][mod_multiple] = true,
    [mod_multiple][mod_optional] = true,
    [mod_multiple][mod_any] = true,
    [mod_multiple][mod_multiple] = true,
    [mod_choice][block_close] = true,
    [mod_choice][mod_optional] = true,
    [mod_choice][mod_any] = true,
    [mod_choice][mod_multiple] = true,
    [mod_choice][mod_choice] = true,
};

ParserState *construct_parser_state(char *regex) {
    ParserState *state = malloc(sizeof(ParserState));
    state->tokens = NULL;
    state->regex = calloc(strlen(regex) + 1, sizeof(char));
    state->escape_active = 0;
    state->open_blocks = 0;
    state->invalid = 0;
    return state;
}

Token get_token_type(char token, bool escape_active) {
    if (escape_active) return character;
    if (token == '(') return block_open;
    if (token == ')') return block_close;
    if (token == '?') return mod_optional;
    if (token == '*') return mod_any;
    if (token == '+') return mod_multiple;
    if (token == '|') return mod_choice;
    if (token == '\\') return mod_escape;
    if (token == ' ' || token == '\t' || token == '\n') return whitespace;
    return character;
}

char *get_token_description(Token token) {
    if (token == block_open) return "Block::open";
    if (token == block_close) return "Block::close";
    if (token == mod_optional) return "Mod::optional";
    if (token == mod_any) return "Mod::any";
    if (token == mod_multiple) return "Mod::multiple";
    if (token == mod_choice) return "Mod::choice";
    if (token == mod_escape) return "Mod::escape";
    if (token == whitespace) return "Whitespace";
    return "Character";
}

Token VLA_binding_get_Token(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(Token));
    return *(Token *)VLA_get(v, idx);
}

void Token_formatter(VLA *formatter, void *item) {
    Token casted = *(Token *)item;
    char *description = get_token_description(casted);
    VLA_batch_append(formatter, description, strlen(description));
}

ParserState *parse_regex(char *regex) {
    ParserState *state = construct_parser_state(regex);
    VLA *token_history = VLA_initialize(strlen(regex), sizeof(Token));
    VLA_set_data_freeing_policy(token_history, immutable);
    VLA_set_item_formatter(token_history, Token_formatter);

    // Dummy-Element, damit man direkt am Anfang auf grammar_table zugreifen kann.
    // block_open, weil es Anfang genau einen globalen Block gibt und die Regeln dafür komplett gleich sind.
    Token previous = block_open;
    for (size_t idx = 0; idx < strlen(regex); idx++) {
        if (token_history->length != 0) previous = VLA_binding_get_Token(token_history, -1);
        Token current = get_token_type(regex[idx], state->escape_active);

        if (grammar_blocklist[previous][current] || (current == block_close && state->open_blocks == 0)) {
            state->invalid = true;
            return state;
        }

        if (current == block_open) state->open_blocks++;
        if (current == block_close) state->open_blocks--;

        if (current == mod_escape)
            state->escape_active = true;
        else
            state->escape_active = false;

        // Nur zu den Token hinzufügen wenn es syntaktisch wichtig ist
        if (current != mod_escape && current != whitespace) {
            size_t char_index = token_history->length / token_history->item_size;
            state->regex[char_index] = regex[idx];
            VLA_append(token_history, &current);
        }
    }

    // prüfen, ob am Ende noch Gruppen angefangen und dann nicht geschlossen wurden
    Token last = VLA_binding_get_Token(token_history, -1);
    if (state->open_blocks != 0 || state->escape_active || last == mod_choice) {
        state->invalid = true;
        return state;
    }

    VLA_print(token_history);
    debug("Cleaned regex: %s\n", state->regex);

    state->tokens = (Token *)token_history->data;
    VLA_free(token_history);
    return state;
}