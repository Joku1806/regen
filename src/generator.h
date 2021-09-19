#ifndef GENERATOR_H
#define GENERATOR_H

#include "NFA.h"
#include "parser.h"

NFA *generate_nfa_from_parsed_regex(ParserState *parsed);
Compact_NFA *compact_generated_NFA(NFA *NFA);

#endif