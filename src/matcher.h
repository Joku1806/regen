#include <stddef.h>
#include "NFA.h"

typedef struct Match Match;
typedef struct PartialMatch PartialMatch;
typedef struct Match_Container Match_Container;

struct Match {
    size_t offset;
    size_t length;
};

struct PartialMatch {
    PartialMatch* infinite_loop_detector;
    size_t references;
    size_t match_length;
    size_t node_idx;
};

struct Match_Container {
    Match* matches;
    size_t number_of_matches;
};

PartialMatch* advance_current_PartialMatch(PartialMatch* current, Compact_Edge* connection);
bool edge_matches_current_position(Compact_Edge* edge, char* position);
bool would_enter_inifinite_loop(PartialMatch* state, Compact_Edge* connection);
void free_PartialMatch_branch(PartialMatch* starting_at);
Match_Container* match(char* to_match, Compact_NFA* match_with);
Match_Container* into_match_container(VLA* matches);