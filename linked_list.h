#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

typedef struct Node {
    // TODO: Struktur so ändern dass der richtige Token-Typ benutzt werden kann
    int token;
    struct Node *previous;
    struct Node *next;
} Node;

typedef struct {
    Node *first;
    Node *last;
    size_t length;
} LinkedList;

LinkedList* construct_linked_list();
void linked_list_insert(LinkedList *list, int token);
void linked_list_remove_nth(LinkedList *list, int idx);
void linked_list_iterate(LinkedList *list);

#endif