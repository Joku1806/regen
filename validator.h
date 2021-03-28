// gebraucht für size_t und andere Typen (aber hauptsächlich für size_t :^))
#include <stddef.h>

// Jeder Typ wird durch ein spezifisches Bit in einer 8-bit Zahl dargestellt,
// sodass man mit &, |, ^ etc. übergreifende Regeln für mehrere Typgruppen formulieren kann,
// z.B. block_open, mod_choice und escape dürfen nicht das letzte Zeichen im Regex sein
enum Token {
    block_open = 0x01,
    block_close = 0x02,
    mod_multiple = 0x04,
    mod_choice = 0x08,
    escape = 0x10,
    character = 0x20,
};

// beschreibt, welcher Token-Typ nach einem anderen Token-Typ kommen darf
// grammar_table[block_open][block_close] z.B. sagt aus, ob ein gerade geöffneter Block
// ohne Inhalt sofort wieder geschlossen werden darf.
// WICHTIG: Der Index muss zuerst mit der Funktion token_type_to_grammar_index() berechnet werden,
// sonst gibt es möglicherweise einen Segfault.
unsigned char grammar_table[6][6] = {
    {1, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 1, 1},
    {1, 0, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1}
};

// Definiert ein Mapping von Buchstaben auf die einzelnen Token-Typen.
// Sollte man also den von mir gewählten Regex-Dialekt (weil in FoSA so gemacht lol)
// umändern wollen, dann muss man einfach nur die Vergleiche in dieser Funktion umschreiben,
// da alle anderen Funktionen nur mit den Enums arbeiten.
int get_token_type(char token);
// token_type muss mindestens ein bit gesetzt haben, sonst gibt es eine Endlosschleife.
// TODO: Überlegen, ob jedes Token wirklich ein einzelnes Bit braucht oder ob man einfach
// von 0 bis 5 durchnummerieren sollte, dann bräuchte man nämlich diese Funktion nicht mehr.
size_t token_type_to_grammar_index(unsigned int token_type);
// Prüft, ob ein eingegebener Regex syntaktisch richtig ist (semantisch ist nochmal ne Ecke schwerer)
// TODO: Testen ob das so funktioniert, bei dem Zeitaufwand bin ich schon ein wenig skeptisch.
int is_valid_regex(char *regex);