#include <stddef.h>
#include "NFA.h"

typedef struct {
    size_t node_index;
    size_t length;
} PartialMatch;

typedef struct {
    size_t offset;
    size_t length;
} Match;

VLA** setup_cycle_guards(Compact_NFA* nfa);
void clear_cycle_guards(VLA** guards, size_t guard_count);
bool would_enter_infinite_loop(VLA* cycle_guard, PartialMatch* match, Compact_Edge* edge);
bool matches_edge(char* position, Compact_Edge* edge);
PartialMatch* take_matching_edge(PartialMatch* current_match, Compact_Edge* edge);
Match* match(char* to_match, Compact_NFA* match_with, size_t* matches_count);