#include <stddef.h>

typedef struct {
    size_t offset;
    size_t length;
} Match;

Match* match(char* to_match, char* regex, size_t* matches_count);