#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "VLA.h"

typedef enum {
    block_open = 0,
    block_close = 1,
    mod_optional = 2,
    mod_any = 3,
    mod_multiple = 4,
    mod_choice = 5,
    mod_escape = 6,
    utf8_codepoint = 7,
    unsigned_long = 8,
    value_range_start = 9,
    value_range_stop = 10,
    repetition_range_start = 11,
    repetition_range_stop = 12,
    range_separator = 13,
} Token;

typedef enum {
    Default = 0,
    InValueRange = 1,
    InRepetitionRange = 2,
} ParseMode;

#define TOKEN_COUNT 14
#define RANGE_SEPARATOR ','

typedef struct {
    Token* tokens;
    size_t number_of_tokens;
    char* regex;
    size_t open_blocks;
    ParseMode parse_mode;
    bool escape_active;
    bool invalid;
} ParserState;

// beschreibt, welcher Token-Typ nach einem anderen Token-Typ kommen darf
// grammar_blocklist[block_open][block_close] z.B. sagt aus, ob ein gerade geöffneter Block
// ohne Inhalt sofort wieder geschlossen werden darf. Die Kombinationen von Tokens, die nicht
// erlaubt sein sollen, müssen auf true gesetzt werden.
extern bool grammar_blocklist[TOKEN_COUNT][TOKEN_COUNT];

ParserState* initialize_parser_state();
void free_parser_state(ParserState* state);
// Definiert ein Mapping von Buchstaben auf die einzelnen Token-Typen.
// Sollte man also den von mir gewählten Regex-Dialekt umändern wollen,
// dann muss man einfach nur die Vergleiche in dieser Funktion umschreiben,
// da alle anderen Funktionen nur mit den Enums arbeiten.
Token get_token_type(char character, ParseMode mode);
char* get_token_description(Token token);
// Versucht den Regex zu parsen und prüft, ob er syntaktisch richtig ist.
ParserState* parse_regex(char* regex);

#endif