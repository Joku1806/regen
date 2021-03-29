#include <stdio.h>
#include <stdlib.h>
#include "linked_list.h"

LinkedList* construct_linked_list() {
    LinkedList *new = malloc(sizeof(LinkedList));
    new->first = NULL;
    new->last = NULL;
    new->length = 0;
    return new;
}

void linked_list_insert(LinkedList *list, int token) {
    Node *new = malloc(sizeof(Node));
    new->token = token;
    new->next = NULL;
    if (list->last != NULL) list->last->next = new;
    new->previous = list->last;
    list->last = new;
    if (list->first == NULL) list->first = new;
    list->length++;
}

void linked_list_remove_nth(LinkedList *list, int idx) {
    if (idx >= list->length) return;
    Node *current = list->first;
    for (size_t counter = 0; counter < idx; counter++) current = current->next;
    
    if (current != list->first) current->previous->next = current->next;
    if (current != list->last) current->next->previous = current->previous;
    if (current == list->first) list->first = current->next;
    if (current == list->last) list->last = current->previous;

    free(current);
    list->length--;
}

void linked_list_iterate(LinkedList *list) {
    Node *current = list->first;

    while (current != NULL) {
        printf("%x\n", current->token);
        current = current->next;
    }
}