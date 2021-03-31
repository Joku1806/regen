#ifndef PARSER_DEFS_H
#define PARSER_DEFS_H

#include <stddef.h>
#include "VLA.h"

typedef enum {
    block_open = 0,
    block_close = 1,
    mod_multiple = 2,
    mod_choice = 3,
    mod_escape = 4,
    whitespace = 5,
    character = 6,
} Token;

typedef struct {
    VLA *token_history;
    Token previous;
    Token current;
    int escape_active;
    int open_blocks;
    int invalid;
} ParserState;

// beschreibt, welcher Token-Typ nach einem anderen Token-Typ kommen darf
// grammar_table[block_open][block_close] z.B. sagt aus, ob ein gerade geöffneter Block
// ohne Inhalt sofort wieder geschlossen werden darf.
extern unsigned char grammar_table[7][7];

ParserState* construct_parser_state();
// Definiert ein Mapping von Buchstaben auf die einzelnen Token-Typen.
// Sollte man also den von mir gewählten Regex-Dialekt (weil in FoSA so gemacht lol)
// umändern wollen, dann muss man einfach nur die Vergleiche in dieser Funktion umschreiben,
// da alle anderen Funktionen nur mit den Enums arbeiten.
// escape_active wird gebraucht, damit alles nach escape als character interpretiert wird.
Token get_token_type(char token, int escape_active);
char* get_token_description(Token token);
// Versucht den Regex zu parsen und prüft, ob er syntaktisch richtig ist (semantisch ist nochmal ne Ecke schwerer)
ParserState* parse_regex(char *regex);

// Gibt das Token an der idx'ten Stelle zurück.
// Unterstützt außerdem negative Indizes (also -1 für letztes, -2 für vorletztes Token etc).
Token VLA_get_token_at_index(VLA *v, signed long idx);

// Gibt alle Tokens im VLA auf stdin aus.
void VLA_print_tokens(VLA* v);

#endif