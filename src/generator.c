#include <string.h>
#include "debug.h"
#include "parser.h"
#include "generator.h"

static size_t VLA_binding_get_size_t(VLA *v, signed long idx);
static void size_t_formatter(VLA *formatter, void *item);

static GeneratorState *construct_GeneratorState();
static void destruct_GeneratorState(GeneratorState *state);

static size_t get_continuous_character_group_length(char *regex, Token *tokens, size_t starting_at);
static void increment_current_group_counter(VLA *levels);

static void advance_current_path(GeneratorState *state, ParserState *parsed);
static void finish_current_path(GeneratorState *state);
static void loop_current_path(GeneratorState *state, ParserState *parsed);
static void open_new_block_level(GeneratorState *state);
static void close_current_block_level(GeneratorState *state);

GeneratorState *construct_GeneratorState() {
    GeneratorState *new = malloc(sizeof(GeneratorState));
    new->start_nodes = stack_initialize(2, sizeof(NFA_Node));
    new->stop_nodes = stack_initialize(2, sizeof(NFA_Node));
    new->group_counters = stack_initialize(2, sizeof(size_t));
    new->global_node_index = calloc(1, sizeof(size_t));
    new->token_index = 0;
    new->generated = construct_NFA(new->global_node_index);

    stack_push_n(new->start_nodes, new->generated->start, 1);
    stack_push_n(new->stop_nodes, new->generated->stop, 1);
    stack_push_n(new->group_counters, &(size_t){0}, 1);

    VLA_set_item_formatter(new->start_nodes, NFA_Node_formatter);
    VLA_set_item_formatter(new->stop_nodes, NFA_Node_formatter);
    VLA_set_item_formatter(new->group_counters, size_t_formatter);

    return new;
}

void destruct_GeneratorState(GeneratorState *state) {
    VLA_free(state->group_counters);
    VLA_free(state->start_nodes);
    VLA_free(state->stop_nodes);
    free(state->global_node_index);
    free(state);
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

void size_t_formatter(VLA *formatter, void *item) {
    size_t casted = *(size_t *)item;
    const int n = snprintf(NULL, 0, "%zu", casted);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%zu", casted);

    VLA_append(formatter, &buffer, n);
}

void increment_current_group_counter(VLA *levels) {
    VLA_replace_at_index(levels, &(size_t){VLA_binding_get_size_t(levels, -1) + 1}, -1);
}

void advance_current_path(GeneratorState *state, ParserState *parsed) {
    NFA_Node *last_start = VLA_binding_get_NFA_Node(state->start_nodes, -1);
    NFA_Node *new = construct_NFA_Node(state->global_node_index);
    size_t character_group_length = get_continuous_character_group_length(parsed->cleaned_regex, parsed->tokens, state->token_index);
    char *matching = calloc(character_group_length + 1, sizeof(char));

    strncpy(matching, parsed->cleaned_regex + state->token_index, character_group_length);
    NFA_add_connection_between(last_start, new, matching);
    stack_push_n(state->start_nodes, new, 1);
    increment_current_group_counter(state->group_counters);
    state->token_index += character_group_length - 1;
}

void finish_current_path(GeneratorState *state) {
    size_t groups = VLA_binding_get_size_t(state->group_counters, -1);
    NFA_Node *last_start = VLA_binding_get_NFA_Node(state->start_nodes, -1);
    NFA_Node *path_stop = VLA_binding_get_NFA_Node(state->stop_nodes, -1);

    debug("Now stepping back %u starting nodes.\n", groups);
    stack_pop_n(state->group_counters, 1);
    stack_pop_n(state->start_nodes, groups);
    NFA_add_empty_connection_between(last_start, path_stop);
}

void loop_current_path(GeneratorState *state, ParserState *parsed) {
    size_t groups = parsed->tokens[state->token_index - 1] == character ? 2 : VLA_binding_get_size_t(state->group_counters, -1);
    NFA_Node *loop_start = VLA_binding_get_NFA_Node(state->start_nodes, -groups);
    NFA_Node *loop_stop = VLA_binding_get_NFA_Node(state->start_nodes, -1);

    NFA_add_empty_connection_between(loop_start, loop_stop);
    NFA_add_empty_connection_between(loop_stop, loop_start);
}

void open_new_block_level(GeneratorState *state) {
    NFA_Node *last_start = VLA_binding_get_NFA_Node(state->start_nodes, -1);
    NFA_Node *start = construct_NFA_Node(state->global_node_index);
    NFA_Node *stop = construct_NFA_Node(state->global_node_index);

    NFA_add_empty_connection_between(last_start, start);
    stack_push_n(state->start_nodes, start, 1);
    stack_push_n(state->stop_nodes, stop, 1);

    increment_current_group_counter(state->group_counters);
    stack_push_n(state->group_counters, &(size_t){0}, 1);
}

void close_current_block_level(GeneratorState *state) {
    NFA_Node *last_start = VLA_binding_get_NFA_Node(state->start_nodes, -1);
    NFA_Node *block_stop = VLA_binding_get_NFA_Node(state->stop_nodes, -1);

    NFA_add_empty_connection_between(last_start, block_stop);
    stack_pop_n(state->stop_nodes, 1);
    stack_push_n(state->start_nodes, block_stop, 1);
    increment_current_group_counter(state->group_counters);
}

NFA *generate_NFA_from_parsed_regex(ParserState *parsed) {
    GeneratorState *state = construct_GeneratorState();

    while (state->token_index < strlen(parsed->cleaned_regex)) {
        VLA_print(state->group_counters);
        VLA_print(state->start_nodes);
        VLA_print(state->stop_nodes);

        Token current = parsed->tokens[state->token_index];

        if (current == block_open)
            open_new_block_level(state);
        else if (current == block_close)
            close_current_block_level(state);
        else if (current == character)
            advance_current_path(state, parsed);
        else if (current == mod_choice)
            finish_current_path(state);
        else if (current == mod_multiple)
            loop_current_path(state, parsed);
        else
            warn("Encountered unexpected token %s, should have been removed in the parsing stage.\n", get_token_description(current));

        state->token_index++;
    }

    finish_current_path(state);
    NFA *generated = state->generated;
    destruct_GeneratorState(state);

    return generated;
}