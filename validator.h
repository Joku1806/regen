#ifndef VALIDATOR_H
#define VALIDATOR_H

// gebraucht für size_t und andere Typen (aber hauptsächlich für size_t :^))
#include <stddef.h>
#include "linked_list.h"

typedef enum {
    block_open = 0,
    block_close = 1,
    mod_multiple = 2,
    mod_choice = 3,
    escape = 4,
    whitespace = 5,
    character = 6,
} Token;

// beschreibt, welcher Token-Typ nach einem anderen Token-Typ kommen darf
// grammar_table[block_open][block_close] z.B. sagt aus, ob ein gerade geöffneter Block
// ohne Inhalt sofort wieder geschlossen werden darf.
extern unsigned char grammar_table[7][7];

typedef struct {
    LinkedList *token_history;
    Token previous;
    Token current;
    int escape_active;
    int open_blocks;
    int invalid;
} ParserState;

// Definiert ein Mapping von Buchstaben auf die einzelnen Token-Typen.
// Sollte man also den von mir gewählten Regex-Dialekt (weil in FoSA so gemacht lol)
// umändern wollen, dann muss man einfach nur die Vergleiche in dieser Funktion umschreiben,
// da alle anderen Funktionen nur mit den Enums arbeiten.
// escape_active wird gebraucht, damit alles nach escape als character interpretiert wird.
Token get_token_type(char token, int escape_active);
// Versucht den Regex zu parsen und prüft, ob er syntaktisch richtig ist (semantisch ist nochmal ne Ecke schwerer)
ParserState* parse_regex(char *regex);

#endif