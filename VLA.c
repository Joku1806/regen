#include <math.h>
#include <string.h>
#include <errno.h>
#include "parser_defs.h"
#include "debug.h"

// Initialisiert einen VLA mit capacity vielen Bytes reserviert.
VLA* VLA_initialize(size_t capacity, size_t item_size) {
    VLA* v = calloc(1, sizeof(VLA));
    if (v == NULL) {
        panic("%s\n", strerror(errno));
    }

    v->capacity = capacity * item_size;
    v->length = 0;
    v->item_size = item_size;
    v->data = malloc(capacity * item_size);

    return v;
}

// Vergrößert die capacity des VLA um factor.
void VLA_expand(VLA* v, double factor) {
    // ceil() wird benutzt, damit man bei 1.x Faktoren über die 1er-capacity hinauskommt
    v->capacity = (size_t)ceil(v->capacity * factor);
    v->data = realloc(v->data, v->capacity);
    if (v->data == NULL) {
        panic("%s\n", strerror(errno));
    }
}

// Fügt beliebig viele Items ans Ende des VLA hinzu und vergrößert ihn vorher, wenn nötig.
void VLA_insert(VLA* v, void* address, size_t amount) {
    if (v->length + amount * v->item_size >= v->capacity) {
        // 1.5 statt 2, weil es vor allem für viele Items weniger Memory verbraucht und trotzdem genauso gut funktioniert
        VLA_expand(v, (double)(v->length + amount * v->item_size) / (double)v->capacity * 1.5);
    }

    memcpy(v->data + v->length, address, amount * v->item_size);
    v->length += amount * v->item_size;
}

// Löscht das idx'te Item, indem das letzte Item dorthin kopiert und die Länge um v->item_size verringert wird.
// Diese Methode erhält nicht die Reihenfolge der Items, pass also auf, dass das im aufrufenden Code nicht wichtig ist.
void VLA_delete_at_index(VLA* v, size_t idx) {
    if (idx * v->item_size >= v->length) {
        warn("Index %ld is out of bounds for VLA with length=%ld. Skipping deletion, check your indices.\n", idx, v->length / v->item_size);
        return;
    }

    memcpy(v->data + idx * v->item_size, v->data + v->length - v->item_size, v->item_size);
    v->length -= v->item_size;
}

// Löscht das idx'te Item, indem alle Items nach idx eine Stelle nach links verschoben werden.
// Diese Methode erhält die Reihenfolge der Elemente, sollte aber nicht für große VLA's benutzt werden.
void VLA_delete_at_index_order_safe(VLA* v, size_t idx) {
    if (idx * v->item_size >= v->length) {
        warn("Index %ld is out of bounds for VLA with length=%ld. Skipping deletion, check your indices.\n", idx, v->length / v->item_size);
        return;
    }

    for (size_t i = idx; i < v->length / v->item_size - 1; i++) {
        v->data[i] = v->data[i + 1];
    }
    v->length -= v->item_size;
}

// Führt eine selbst definierte Funktion handler() auf allen Items aus,
// solange handler() != NULL ist und löscht danach den VLA selbst.
void VLA_cleanup(VLA* v, void (*handler)(void*)) {
    if (handler != NULL) {
        for (size_t i = 0; i < v->length; i += v->item_size) {
            handler(v->data + i);
        }
    }

    free(v->data);
    free(v);
}

// Bindings für verschiedene Typen

Token VLA_get_token_at_index(VLA *v, signed long idx) {
    if (idx >= 0 && idx * v->item_size >= v->length || idx < 0 && (-idx - 1) * v->item_size >= v->length) {
        panic("Can't get token at index %ld because thats out of bounds for this VLA with length=%ld.\n", idx, v->length / v->item_size);
    }
    if (idx < 0) idx = v->length / v->item_size + idx;

    return *(Token *)(v->data + idx * v->item_size);
}

void VLA_print_tokens(VLA* v) {
    for (size_t idx = 0; idx < v->length / v->item_size; idx++) {
        printf("%ld. %s\n", idx + 1, get_token_description(VLA_get_token_at_index(v, idx)));
    }
}