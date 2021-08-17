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

ParserState *initialize_parser_state(char *regex) {
    ParserState *state = malloc(sizeof(ParserState));
    state->tokens = NULL;
    state->regex = calloc(strlen(regex) + 1, sizeof(char));
    state->open_blocks = 0;
    state->escape_active = false;
    state->invalid = false;
    return state;
}

void free_parser_state(ParserState *state) {
    free(state->tokens);
    free(state->regex);
    free(state);
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
    if (token == ' ' || token == '\a' || token == '\b' ||
        token == '\t' || token == '\n' || token == '\v' ||
        token == '\f' || token == '\r') return whitespace;
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

bool encodes_special_character(char c1, char c2) {
    return c1 == '\\' && (c2 == '0' || c2 == 'a' || c2 == 'b' || c2 == 't' ||
                          c2 == 'n' || c2 == 'v' || c2 == 'f' || c2 == 'r');
}

// FIXME: Compiler irgendwie erklären, dass das Ende der Funktion nie erreicht werden wird.
// Im Moment muss diese hässliche Lösung herhalten
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
char get_replacement_for_special_character(char special) {
    switch (special) {
        case '0':
            return '\0';
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 't':
            return '\t';
        case 'n':
            return '\n';
        case 'v':
            return '\v';
        case 'f':
            return '\f';
        case 'r':
            return '\r';
        default:
            panic("%c is not a supported special character!\n");
    }
}
#pragma GCC diagnostic pop

Token VLA_binding_get_token(VLA *v, signed long index) {
    VLA_assert_item_size_matches(v, sizeof(Token));
    return *(Token *)VLA_get(v, index);
}

void token_formatter(VLA *output, void *item) {
    Token casted = *(Token *)item;
    char *description = get_token_description(casted);
    VLA_batch_append(output, description, strlen(description));
}

ParserState *parse_regex(char *regex) {
    ParserState *state = initialize_parser_state(regex);
    VLA *token_history = VLA_initialize(strlen(regex), sizeof(Token));

    // Dummy-Element, damit man direkt am Anfang auf grammar_table zugreifen kann.
    // block_open, weil es Anfang genau einen globalen Block gibt und die Regeln dafür komplett gleich sind.
    Token previous = block_open;
    for (size_t index = 0; index < strlen(regex); index++) {
        if (token_history->length != 0) previous = VLA_binding_get_token(token_history, -1);
        Token current = get_token_type(regex[index], state->escape_active);

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

        if (current != mod_escape && current != whitespace) {
            if (previous == character && current == character && encodes_special_character(regex[index - 1], regex[index])) {
                state->regex[VLA_get_length(token_history) - 1] = get_replacement_for_special_character(regex[index]);
            } else {
                state->regex[VLA_get_length(token_history)] = regex[index];
                VLA_append(token_history, &current);
            }
        }
    }

    // prüfen, ob am Ende Gruppen neu angefangen oder nicht geschlossen wurden
    Token last = VLA_binding_get_token(token_history, -1);
    if (state->open_blocks > 0 || state->escape_active || last == mod_choice) {
        state->invalid = true;
        return state;
    }

    VLA_print(token_history, token_formatter);
    debug("Gereinigter Regex: %s\n", state->regex);

    state->tokens = (Token *)VLA_extract(token_history);
    return state;
}