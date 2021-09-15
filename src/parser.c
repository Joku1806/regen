#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
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

Token VLA_binding_get_token(VLA *v, signed long index) {
    VLA_assert_item_size_matches(v, sizeof(Token));
    return *(Token *)VLA_get(v, index);
}

void token_formatter(VLA *output, void *item) {
    Token casted = *(Token *)item;
    char *description = get_token_description(casted);
    VLA_batch_append(output, description, strlen(description));
}

ParserState *initialize_parser_state(char *regex) {
    ParserState *state = malloc(sizeof(ParserState));
    state->tokens = NULL;
    state->regex = NULL;
    state->open_blocks = 0;
    state->parse_mode = Default;
    state->escape_active = false;
    state->invalid = false;
    return state;
}

void free_parser_state(ParserState *state) {
    free(state->tokens);
    free(state->regex);
    free(state);
}

Token get_token_type(char value, ParseMode mode) {
    if ((mode == InValueRange || mode == InRepetitionRange) && value == RANGE_SEPARATOR) return range_separator;
    if (value == '(') return block_open;
    if (value == ')') return block_close;
    if (value == '?') return mod_optional;
    if (value == '*') return mod_any;
    if (value == '+') return mod_multiple;
    if (value == '|') return mod_choice;
    if (value == '\\') return mod_escape;
    if (value == '[') return value_range_start;
    if (value == ']') return value_range_stop;
    if (value == '{') return repetition_range_start;
    if (value == '}') return repetition_range_stop;
    return utf8_codepoint;
}

char *get_token_description(Token token) {
    if (token == block_open) return "Block::open";
    if (token == block_close) return "Block::close";
    if (token == mod_optional) return "Mod::optional";
    if (token == mod_any) return "Mod::any";
    if (token == mod_multiple) return "Mod::multiple";
    if (token == mod_choice) return "Mod::choice";
    if (token == mod_escape) return "Mod::escape";
    if (token == unsigned_long) return "Long::unsigned";
    if (token == utf8_codepoint) return "Utf8::codepoint";
    if (token == value_range_start) return "ValueRange::start";
    if (token == value_range_stop) return "ValueRange::stop";
    if (token == repetition_range_start) return "RepetitionRange::start";
    if (token == repetition_range_stop) return "RepetitionRange::stop";
    if (token == range_separator) return "Range::separator";
}

bool is_whitespace(char character) {
    return character == ' ' || character == '\t' || character == '\n' || character == '\v' || character == '\f' || character == '\r';
}

bool encodes_special_character(char c) {
    return (c == '0' || c == 'a' || c == 'b' || c == 't' ||
            c == 'n' || c == 'v' || c == 'f' || c == 'r');
}

// FIXME: Compiler irgendwie erklären, dass das Ende der Funktion nach panic() nie erreicht wird.
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
            panic("%c ist kein unterstützter besonderer Buchstabe!\n", special);
    }
}
#pragma GCC diagnostic pop

// FIXME: Ekelhaft zu verstehende Konditionen vereinfachen
char *remove_whitespace_and_encodings_from_regex(char *regex) {
    char *cleaned = calloc(strlen(regex) + 1, sizeof(char));
    size_t dest_index = 0;
    bool detected_special_character = false;

    for (size_t src_index = 0; src_index < strlen(regex); src_index++) {
        if ((src_index == 0 || regex[src_index - 1] != '\\') && is_whitespace(regex[src_index])) continue;
        if (!detected_special_character && regex[src_index] == '\\' &&
            src_index < strlen(regex) - 1 && encodes_special_character(regex[src_index + 1])) {
            detected_special_character = true;
            continue;
        }

        if (detected_special_character) {
            detected_special_character = false;
            cleaned[dest_index] = get_replacement_for_special_character(regex[src_index]);
        } else {
            cleaned[dest_index] = regex[src_index];
        }

        dest_index++;
    }

    return cleaned;
}

uint8_t get_valid_utf8_codepoint_size(uint8_t *at, size_t remaining_length) {
    if (remaining_length >= 1 && (at[0] & 0x80) == 0x00) return 1;
    if (remaining_length >= 2 && (at[0] & 0xE0) == 0xC0 && (at[1] & 0xC0) == 0x80) return 2;
    if (remaining_length >= 3 && (at[0] & 0xF0) == 0xE0 && (at[1] & 0xC0) == 0x80 && (at[2] & 0xC0) == 0x80) return 3;
    if (remaining_length >= 4 && (at[0] & 0xF8) == 0xF0 && (at[1] & 0xC0) == 0x80 && (at[2] & 0xC0) == 0x80 && (at[3] & 0xC0) == 0x80) return 4;
    // entweder war der Input schon komplett eingelesen oder es konnte kein valider utf8-codepoint gefunden werden - in beiden Fällen steht 0 für den Fehlercode (da es keinen ut8-codepoint gibt, der 0 Bytes groß ist)
    return 0;
}

void advance_to_next_token(char *regex, size_t *index) {
    if (*index >= strlen(regex)) {
        panic("Index %lu is out of bounds for string at %p (length=%lu)\n", index, regex, strlen(regex));
    }

    uint8_t current_token_size = get_valid_utf8_codepoint_size(regex + *index, strlen(regex) - *index);
    if (current_token_size == 0) {
        panic("Encountered invalid unicode codepoint %lu bytes in at %p, aborting.\n", *index, regex + *index);
    }

    *index += current_token_size;
}

bool parsed_correct_value_range(VLA *tokens) {
    return VLA_binding_get_token(tokens, -4) == value_range_start &&
           VLA_binding_get_token(tokens, -3) == utf8_codepoint &&
           VLA_binding_get_token(tokens, -2) == range_separator &&
           VLA_binding_get_token(tokens, -1) == utf8_codepoint;
}

bool parsed_correct_repetition_range(VLA *tokens) {
    return VLA_binding_get_token(tokens, -4) == repetition_range_start &&
           VLA_binding_get_token(tokens, -3) == unsigned_long &&
           VLA_binding_get_token(tokens, -2) == range_separator &&
           VLA_binding_get_token(tokens, -1) == unsigned_long;
}

ParserState *parse_regex(char *input) {
    char *cleaned_input = remove_whitespace_and_encodings_from_regex(input);
    ParserState *state = initialize_parser_state(cleaned_input);
    VLA *regex = VLA_initialize(strlen(cleaned_input), sizeof(char));
    VLA *tokens = VLA_initialize(strlen(cleaned_input), sizeof(Token));

    // Dummy-Element, damit man auch am Anfang auf grammar_table zugreifen kann.
    // Es ist block_open, weil es am Anfang genau einen globalen Block gibt.
    Token previous = block_open;
    size_t byte_offset = 0;
    while (byte_offset < strlen(cleaned_input)) {
        if (VLA_get_length(tokens) > 0) previous = VLA_binding_get_token(tokens, -1);
        Token current = state->escape_active ? utf8_codepoint : get_token_type(cleaned_input[byte_offset], state->parse_mode);

        if (grammar_blocklist[previous][current]) {
            warn("A %s followed by a %s is not supported by the regen syntax.\n", get_token_description(previous), get_token_description(current));
            state->invalid = true;
            return state;
        }

        if (current == block_close && state->open_blocks == 0) {
            warn("Trying to close a block that doesn't exist is not allowed.\n");
            state->invalid = true;
            return state;
        }

        if (current == block_open) state->open_blocks++;
        if (current == block_close) state->open_blocks--;

        if (current == mod_escape) {
            state->escape_active = true;
            advance_to_next_token(cleaned_input, &byte_offset);
            continue;
        }

        if (current == value_range_start) {
            if (state->parse_mode != Default) {
                warn("Trying to start a range while already being inside another range is not allowed.\n");
                state->invalid = true;
                return state;
            }
            state->parse_mode = InValueRange;
        }

        if (current == value_range_stop) {
            if (!parsed_correct_value_range(tokens)) {
                warn("Value range ending at offset %lu is formatted incorrectly.\n", VLA_get_length(tokens));
                VLA_print(tokens, token_formatter);
                state->invalid = true;
                return state;
            }

            state->parse_mode = Default;
        }

        if (current == repetition_range_start) {
            if (state->parse_mode != Default) {
                warn("Trying to start a range while already being inside another range is not allowed.\n");
                state->invalid = true;
                return state;
            }
            state->parse_mode = InRepetitionRange;
        }

        if (current == repetition_range_stop) {
            if (!parsed_correct_repetition_range(tokens)) {
                warn("Repetition range ending at offset %lu is formatted incorrectly.\n", VLA_get_length(tokens));
                VLA_print(tokens, token_formatter);
                state->invalid = true;
                return state;
            }

            state->parse_mode = Default;
        }

        if (state->parse_mode == InRepetitionRange && current == utf8_codepoint) {
            char *parse_end;
            errno = 0;
            unsigned long converted = strtoul(cleaned_input + byte_offset, &parse_end, 0);
            if (errno != 0) {
                warn("%s\n", strerror(errno));
                state->invalid = true;
                return state;
            }

            if (parse_end == cleaned_input + byte_offset) {
                warn("Could not convert number inside repetition range.\n");
                state->invalid = true;
                return state;
            }

            VLA_batch_append(regex, &converted, sizeof(unsigned long));
            VLA_append(tokens, &(Token){unsigned_long});
            byte_offset += parse_end - (cleaned_input + byte_offset);
        } else {
            VLA_batch_append(regex, cleaned_input + byte_offset, get_valid_utf8_codepoint_size(cleaned_input + byte_offset, strlen(cleaned_input) - byte_offset));
            VLA_append(tokens, &current);
            advance_to_next_token(cleaned_input, &byte_offset);
        }

        state->escape_active = false;
    }

    // prüft, ob am Ende Gruppen neu angefangen oder nicht geschlossen wurden
    Token last = VLA_binding_get_token(tokens, -1);
    if (state->parse_mode != Default || state->open_blocks > 0 || state->escape_active || last == mod_choice) {
        warn("Leaving a started group open is not allowed. Please close it explicitly.\n");
        state->invalid = true;
        return state;
    }

    VLA_append(regex, &(char){'\0'});
    state->regex = (char *)VLA_extract(regex);
    state->number_of_tokens = VLA_get_length(tokens);
    state->tokens = (Token *)VLA_extract(tokens);
    free(cleaned_input);
    return state;
}