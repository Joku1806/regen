#include "validator.h"

int get_token_type(char token) {
    if (token == '(') return block_open;
    if (token == ')') return block_close;
    if (token == '*') return mod_multiple;
    if (token == '+') return mod_choice;
    if (token == '\\') return escape;
    return character;
}

size_t token_type_to_grammar_index(unsigned int token_type) {
    unsigned int bitmask = 0x1;
    for (size_t idx = 0;; idx++) {
        if (token_type & bitmask) return idx;
        bitmask = bitmask << 1;
    }
}

int is_valid_regex(char *regex) {
    // previous_token_type ist am Anfang auf 0 gesetzt, weil die zugehörigen Regeln für block_open
    // genau die gleichen sind wie für das erste Token im Regex (siehe Definition grammar_table).
    int previous_token_type = 0;
    int current_token_type = 0;
    int open_blocks_counter = 0;
    for (size_t idx = 0; idx < strlen(regex); idx++, previous_token_type = current_token_type) {
        int current_token_type = get_token_type(regex[idx]);
        if (!grammar_table[previous_token_type][current_token_type]) return 0;
        if (current_token_type == block_close && open_blocks_counter == 0) return 0;
        if (current_token_type == block_open) open_blocks_counter++;
        if (current_token_type == block_close) open_blocks_counter--;
    }

    // prüfen, ob am Ende noch Gruppen angefangen und dann nicht geschlossen wurden
    if (open_blocks_counter != 0) return 0;
    if (previous_token_type & (mod_choice | escape)) return 0;
    return 1;
}