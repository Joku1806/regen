#include <string.h>
#include "debug.h"
#include "parser.h"
#include "generator.h"

static DFA_State* construct_DFA_State(size_t id);
static DFA* construct_DFA();
static Transition* construct_DFA_Transition(char* matching, DFA_State *to);
static void DFA_add_connection_between(DFA_State *from, DFA_State *to, char *matching);
static void DFA_add_dummy_connection_between(DFA_State *from, DFA_State *to);
static size_t get_continuous_character_group_length(char *regex, Token *tokens, size_t starting_at);

DFA_State* construct_DFA_State(size_t id) {
    DFA_State *new = malloc(sizeof(DFA_State));
    new->transitions = VLA_initialize(1, sizeof(Transition));
    new->id = id;
    return new;
}

DFA* construct_DFA() {
    DFA* new = malloc(sizeof(DFA));
    new->start = construct_DFA_State(0);
    new->stop = construct_DFA_State(1);
    return new;
}

Transition* construct_DFA_Transition(char* matching, DFA_State *to) {
    Transition *new = malloc(sizeof(Transition));
    new->matching = matching;
    new->advance_to = to;
    return new;
}

void DFA_add_connection_between(DFA_State *from, DFA_State *to, char *matching) {
    if (from == NULL || to == NULL) panic("At least one of the states doesn't exist, can't from connection between them.\n");
    debug("Now adding connection from z%u to z%u matching %s.\n", from->id, to->id, matching);
    Transition *connection = construct_DFA_Transition(matching, to);
    VLA_append(from->transitions, connection, 1);
}

void DFA_add_dummy_connection_between(DFA_State *from, DFA_State *to) {
    DFA_add_connection_between(from, to, NULL);
}

size_t get_continuous_character_group_length(char *regex, Token *tokens, size_t starting_at) {
    if (starting_at >= strlen(regex)) panic("Character Index %ld is out of bounds for %s\n", starting_at, regex);
    size_t offset = 0;
    while (starting_at + offset < strlen(regex) && tokens[starting_at + offset] == character) offset++;
    // letzten Buchstaben vor mod_multiple nicht mit reinnehmen, der hat dann eine komplett neue Gruppe
    return (offset > 1 && tokens[starting_at + offset] == mod_multiple) ? offset - 1 : offset;
}

size_t VLA_binding_get_size_t(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(size_t));
    return *(size_t *)VLA_get(v, idx);
}

DFA_State *VLA_binding_get_DFA_State(VLA *v, signed long idx) {
    VLA_assert_item_size_matches(v, sizeof(DFA_State));
    return (DFA_State *)VLA_get(v, idx);
}

void size_t_formatter(VLA* formatter, void *item) {
    size_t casted = *(size_t *)item;
    const int n = snprintf(NULL, 0, "%zu", casted);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted);

    VLA_append(formatter, &buffer, n);
}

void DFA_State_formatter(VLA* formatter, void *item) {
    DFA_State* casted = (DFA_State*)item;
    VLA_append(formatter, "z", 1);

    const int n = snprintf(NULL, 0, "%zu", casted->id);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted->id);

    VLA_append(formatter, &buffer, n);
}

void increment_current_level_group_counter(VLA *levels) {
    VLA_replace_at_index(levels, &(size_t){VLA_binding_get_size_t(levels, -1) + 1}, -1);
}

DFA* generate_DFA_from_parsed_regex(ParserState *parsed) {
    size_t id_counter = 2;
    DFA *generated = construct_DFA();
    
    VLA *start_node_stack = VLA_initialize(2, sizeof(DFA_State));
    VLA_set_item_formatter(start_node_stack, DFA_State_formatter);
    VLA *end_node_stack = VLA_initialize(2, sizeof(DFA_State));
    VLA_set_item_formatter(end_node_stack, DFA_State_formatter);
    VLA_append(start_node_stack, generated->start, 1);
    VLA_append(end_node_stack, generated->stop, 1);

    VLA *level_group_counters = VLA_initialize(2, sizeof(size_t));
    VLA_set_item_formatter(level_group_counters, size_t_formatter);
    VLA_append(level_group_counters, &(size_t){0}, 1);

    for (size_t idx = 0; idx < strlen(parsed->cleaned_regex); idx++) {
        printf("Current group counter stack is:\n");
        VLA_print(level_group_counters);
        printf("Current start stack is:\n");
        VLA_print(start_node_stack);
        printf("Current end stack is:\n");
        VLA_print(end_node_stack);
        Token current = parsed->tokens[idx];
        DFA_State *current_start = VLA_binding_get_DFA_State(start_node_stack, -1);
        DFA_State *current_stop = VLA_binding_get_DFA_State(end_node_stack, -1);
        
        if (current == block_open) {
            DFA_State *start = construct_DFA_State(id_counter);
            id_counter++;
            DFA_State *stop = construct_DFA_State(id_counter);
            id_counter++;
            DFA_add_dummy_connection_between(current_start, start);
            VLA_append(start_node_stack, start, 1);
            VLA_append(end_node_stack, stop, 1);

            increment_current_level_group_counter(level_group_counters);
            VLA_append(level_group_counters, &(size_t){0}, 1);
        }

        if (current == block_close) {
            DFA_add_dummy_connection_between(current_start, current_stop);
            VLA_delete_at_index_order_safe(end_node_stack, -1);
            VLA_append(start_node_stack, current_stop, 1);
            increment_current_level_group_counter(level_group_counters);
        }
        
        if (current == character) {
            DFA_State *new = construct_DFA_State(id_counter);
            id_counter++;
            size_t character_group_length = get_continuous_character_group_length(parsed->cleaned_regex, parsed->tokens, idx);
            char *matching = calloc(character_group_length + 1, sizeof(char));
            strncpy(matching, parsed->cleaned_regex + idx, character_group_length);
            DFA_add_connection_between(current_start, new, matching);
            VLA_append(start_node_stack, new, 1);
            increment_current_level_group_counter(level_group_counters);
            idx += character_group_length - 1;
        }

        if (current == mod_choice) {
            size_t groups = VLA_binding_get_size_t(level_group_counters, -1);
            debug("Now stepping back %u starting nodes.\n", groups);
            VLA_delete_at_index_order_safe(level_group_counters, -1);
            for (size_t cnt = 0; cnt < groups; cnt++) {
                // TODO: Löschen mit einem Funktionsaufruf unterstützen
                VLA_delete_at_index_order_safe(start_node_stack, -1);
            }
            DFA_add_dummy_connection_between(current_start, current_stop);
        }

        if (current == mod_multiple) {
            size_t groups = parsed->tokens[idx - 1] == character ? 2 : VLA_binding_get_size_t(level_group_counters, -1);
            DFA_State *previous_start = VLA_binding_get_DFA_State(start_node_stack, -groups);
            DFA_add_dummy_connection_between(current_start, previous_start);
            DFA_add_dummy_connection_between(previous_start, current_start);
        }
    }

    DFA_State *last_start = VLA_binding_get_DFA_State(start_node_stack, -1);
    DFA_State *last_stop = VLA_binding_get_DFA_State(end_node_stack, -1);
    DFA_add_dummy_connection_between(last_start, last_stop);

    VLA_free(level_group_counters);
    VLA_free(start_node_stack);
    VLA_free(end_node_stack);

    return generated;
}