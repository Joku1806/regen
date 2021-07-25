#include <math.h>
#include <string.h>
#include <errno.h>
#include "parser.h"
#include "debug.h"

// Initialisiert einen VLA mit capacity vielen Bytes reserviert.
VLA* VLA_initialize(size_t capacity, size_t item_size) {
    if (capacity == 0) {
        warn("Because of the expansion strategy of this implementation, an initial capacity > 0 is needed.\n\tProceeding with default value of 1.\n");
        capacity = 1;
    }

    VLA* v = calloc(1, sizeof(VLA));
    if (v == NULL) {
        panic("%s\n", strerror(errno));
    }

    v->capacity = capacity * item_size;
    v->length = 0;
    v->item_size = item_size;
    v->data = calloc(capacity, item_size);
    v->data_freeing_policy = freeable;
    v->item_formatter = NULL;

    return v;
}

void VLA_set_item_formatter(VLA* v, void (*item_formatter)(VLA* formatter, void* item)) {
    v->item_formatter = item_formatter;
}

void VLA_set_data_freeing_policy(VLA* v, data_policy policy) {
    v->data_freeing_policy = policy;
}

void VLA_assert_in_bounds(VLA* v, size_t idx) {
    if (idx * v->item_size >= v->length) {
        panic("Index %ld is out of bounds for this VLA with length=%ld.\n", idx, v->length / v->item_size);
    }
}

void VLA_assert_item_size_matches(VLA* v, size_t item_size) {
    if (item_size != v->item_size) {
        panic("Provided item size %ld doesn't match VLA with item_size=%ld.\n", item_size, v->item_size);
    }
}

size_t VLA_normalize_index(VLA* v, signed long idx) {
    if (idx >= 0) return idx;
    return v->length / v->item_size + idx;
}

void VLA_resize(VLA* v, size_t size) {
    v->capacity = size;
    v->data = realloc(v->data, size);
    if (v->data == NULL) {
        panic("%s\n", strerror(errno));
    }
}

// Vergrößert die Kapazität des VLA um factor.
void VLA_expand(VLA* v, double factor) {
    // Der Wert wird hochgerundet, damit man bei 1.x Faktoren über die 1er-capacity hinauskommt
    VLA_resize(v, (size_t)ceil(v->capacity * factor));
}

// Fügt den an der Adresse gespeicherten Wert ans Ende des VLA hinzu und vergrößert ihn vorher, wenn nötig.
void VLA_append(VLA* v, void* address) {
    VLA_batch_append(v, address, 1);
}

// Fügt beliebig viele Items ans Ende des VLA hinzu und vergrößert ihn vorher, wenn nötig.
void VLA_batch_append(VLA* v, void* address, size_t amount) {
    if (v->length + amount * v->item_size >= v->capacity) {
        // 1.5 statt 2, weil es vor allem für viele Items weniger Speicher verbraucht und trotzdem genauso gut funktioniert.
        // Der andere Teil der Formel sorgt dafür, dass bei großen Einfügungen der Faktor automatisch mitwächst.
        // TODO: Eine alternative Herangehensweise wäre es, immer den letzten Faktor abzuspeichern, und dann mit
        // dem aktuellen Faktor den Durchschnitt zu bilden. Diese Methode ist vielleicht noch präziser als die aktuelle.
        VLA_expand(v, (double)(v->length + amount * v->item_size) / (double)v->capacity * 1.5);
    }

    memcpy(v->data + v->length, address, amount * v->item_size);
    v->length += amount * v->item_size;
}

void VLA_replace_at_index(VLA* v, void* address, signed long idx) {
    idx = VLA_normalize_index(v, idx);
    VLA_assert_in_bounds(v, idx);

    memcpy(v->data + idx * v->item_size, address, v->item_size);
}

// Löscht das idx'te Item, indem das letzte Item dorthin kopiert und die Länge um v->item_size verringert wird.
// Diese Methode erhält nicht die Reihenfolge der Items, in diesem Fall sollte VLA_delete_at_index_order_safe() benutzt werden.
void VLA_delete_at_index(VLA* v, signed long idx) {
    idx = VLA_normalize_index(v, idx);
    VLA_assert_in_bounds(v, idx);

    memcpy(v->data + idx * v->item_size, v->data + v->length - v->item_size, v->item_size);
    v->length -= v->item_size;
}

// FIXME: Wird diese Methode wirklich irgendwo gebraucht?
// Löscht das idx'te Item, indem alle Items nach idx eine Stelle nach links verschoben werden.
// Diese Methode erhält die Reihenfolge der Elemente, sollte aber nicht für große VLA's benutzt werden,
// da sonst sehr viele Einträge kopiert werden müssten.
void VLA_delete_at_index_order_safe(VLA* v, signed long idx) {
    idx = VLA_normalize_index(v, idx);
    VLA_assert_in_bounds(v, idx);

    for (size_t i = idx; i < v->length / v->item_size - 1; i++) {
        v->data[i] = v->data[i + 1];
    }
    v->length -= v->item_size;
}

void VLA_clear(VLA* v) {
    if (v == NULL) return;
    v->length = 0;
}

uint8_t* VLA_get(VLA* v, signed long idx) {
    idx = VLA_normalize_index(v, idx);
    VLA_assert_in_bounds(v, idx);

    return v->data + idx * v->item_size;
}

size_t VLA_get_length(VLA* v) {
    if (v == NULL) return 0;
    return v->length / v->item_size;
}

void VLA_address_formatter(VLA* formatter, void* item) {
    unsigned long address = *(unsigned long*)item;
    const int n = snprintf(NULL, 0, "%p", (void*)address);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%p", (void*)address);
    VLA_batch_append(formatter, &buffer, n);
}

void VLA_print_setup_information_header(VLA* v, VLA* formatter) {
    int chars_written;
    const int n = snprintf(NULL, 0, "%zu", SIZE_MAX);
    char buffer[n + 1];

    VLA_batch_append(formatter, "VLA with Item size: ", 20);
    chars_written = snprintf(buffer, n + 1, "%zu", v->item_size);
    VLA_batch_append(formatter, buffer, chars_written);
    VLA_batch_append(formatter, " Bytes -- Space used: ", 22);
    chars_written = snprintf(buffer, n + 1, "%zu", v->length);
    VLA_batch_append(formatter, buffer, chars_written);
    VLA_batch_append(formatter, "/", 1);
    chars_written = snprintf(buffer, n + 1, "%zu", v->capacity);
    VLA_batch_append(formatter, buffer, chars_written);
    VLA_batch_append(formatter, " Bytes (", 8);

    if (v->data_freeing_policy == freeable) {
        VLA_batch_append(formatter, "freeable", 8);
    } else {
        VLA_batch_append(formatter, "immutable", 9);
    }

    VLA_batch_append(formatter, ") ", 2);
}

void VLA_print_dump_data(VLA* v, VLA* formatter) {
    if (v->length > 0) VLA_batch_append(formatter, "| ", 2);
    for (size_t offset = 0; offset < v->length; offset += v->item_size) {
        v->item_formatter(formatter, v->data + offset);
        VLA_batch_append(formatter, " | ", 3);
    }
}

void VLA_print(VLA* v) {
#ifdef DEBUG
    if (v->item_formatter == NULL) {
        warn("Item formatter is not set for this VLA, returning.\n");
        return;
    }

    VLA* output = VLA_initialize(v->length / v->item_size * 3 + INFO_HEADER_SIZE, sizeof(char));  // Rekursion!
    VLA_print_setup_information_header(v, output);
    VLA_print_dump_data(v, output);
    VLA_append(output, &(char){'\0'});

    debug("%s\n", (char*)output->data);
    VLA_free(output);
#endif
}

// Löscht den VLA selbst und optional auch den data-Array, falls.
// data_freeing_policy auf freeable gesetzt ist.
void VLA_free(VLA* v) {
    if (v->data_freeing_policy == freeable) free(v->data);
    free(v);
}