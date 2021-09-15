#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "matcher.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Benutzung: %s Regex Text\n", argv[0]);
        return 0;
    }

    char* regex = argv[1];
    char* text = argv[2];

    size_t matches_count = 0;
    Match* matches = match(text, regex, &matches_count);

    printf("Input: %s\n", text);
    for (size_t match_index = 0; match_index < matches_count; match_index++) {
        Match current = matches[match_index];
        printf("Habe \"%.*s\" gefunden (Offset=%lu, Länge=%lu)\n", (int)current.length, text + current.offset, current.offset, current.length);
    }
    free(matches);

    return 0;
}