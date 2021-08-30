#include <math.h>
#include <string.h>
#include "parser.h"
#include "debug.h"

// Initialisiert einen VLA mit capacity vielen Bytes reserviert.
VLA* VLA_initialize(size_t capacity, size_t item_size) {
    if (item_size == 0) {
        warn("Item size of 0 is not allowed, since those items can't exist.\n");
        return NULL;
    }

    if (capacity == 0) {
        warn("The resizing strategy of this VLA needs the initial capacity to be non-zero. Bumping to default value of 1.\n");
        capacity = 1;
    }

    VLA* v = calloc(1, sizeof(VLA));
    if (v == NULL) {
        panic("Could not initialize VLA using calloc(1, %lu), aborting.\n", sizeof(VLA));
    }

    v->capacity = capacity * item_size;
    v->length = 0;
    v->item_size = item_size;
    v->data = calloc(capacity, item_size);
    if (v->data == NULL) {
        panic("Could not allocate data region of the VLA using calloc(%lu, %lu), aborting.\n", capacity, item_size);
    }

    return v;
}

void VLA_assert_in_bounds(VLA* v, size_t index) {
    if (index * v->item_size >= v->capacity) {
        panic("Index %ld is out of bounds for this VLA with capacity=%ld.\n", index, v->capacity / v->item_size);
    }
}

void VLA_assert_item_size_matches(VLA* v, size_t item_size) {
    if (item_size != v->item_size) {
        panic("Provided item size %ld doesn't match this VLA (item size=%ld).\n", item_size, v->item_size);
    }
}

size_t VLA_normalize_index(VLA* v, signed long index) {
    if (index >= 0) return index;
    return v->length / v->item_size + index;
}

void VLA_resize(VLA* v, size_t size) {
    v->capacity = size;
    v->data = realloc(v->data, size);
    if (v->data == NULL) {
        panic("Couldn't reallocate data region, aborting.\n");
    }
}

// Vergrößert die Kapazität des VLA um factor.
void VLA_expand(VLA* v, double factor) {
    // Der Wert wird hochgerundet, damit man bei 1.x Faktoren über die 1er-capacity hinauskommt
    VLA_resize(v, (size_t)ceil(v->capacity * factor));
}

// FIXME: Diese Funktion macht implizit zu viele Sachen, die nicht eindeutig klar sind,
// z.B. was der zweite Parameter macht und dass der VLA vergrößert wird.
uint8_t* VLA_reserve_next_slots(VLA* v, size_t item_count) {
    if (v->length + item_count * v->item_size >= v->capacity) {
        // 1.5 statt 2, weil es vor allem für viele Items weniger Speicher verbraucht und trotzdem genauso gut funktioniert.
        // Der andere Teil der Formel sorgt dafür, dass bei großen Einfügungen der Faktor automatisch mitwächst.
        // TODO: Eine alternative Herangehensweise wäre es, immer den letzten Faktor abzuspeichern, und dann mit
        // dem aktuellen Faktor den Durchschnitt zu bilden. Diese Methode ist vielleicht noch präziser als die aktuelle.
        VLA_expand(v, (double)(v->length + item_count * v->item_size) / (double)v->capacity * 1.5);
    }

    uint8_t* slot_start = v->data + v->length;
    v->length += item_count * v->item_size;
    return slot_start;
}

// Fügt den an der Adresse gespeicherten Wert ans Ende des VLA hinzu und vergrößert ihn vorher, wenn nötig.
void VLA_append(VLA* v, void* address) {
    VLA_batch_append(v, address, 1);
}

// Fügt beliebig viele Items ans Ende des VLA hinzu und vergrößert ihn vorher, wenn nötig.
void VLA_batch_append(VLA* v, void* address, size_t amount) {
    memcpy(VLA_reserve_next_slots(v, amount), address, amount * v->item_size);
}

void VLA_replace_at_index(VLA* v, void* address, signed long index) {
    index = VLA_normalize_index(v, index);
    VLA_assert_in_bounds(v, index);

    memcpy(v->data + index * v->item_size, address, v->item_size);
}

// Löscht das index'te Item, indem das letzte Item dorthin kopiert und die Länge um v->item_size verringert wird.
// Diese Methode erhält nicht die Reihenfolge der Items.
void VLA_delete_at_index(VLA* v, signed long index) {
    index = VLA_normalize_index(v, index);
    VLA_assert_in_bounds(v, index);

    memcpy(v->data + index * v->item_size, v->data + v->length - v->item_size, v->item_size);
    v->length -= v->item_size;
}

void VLA_clear(VLA* v) {
    if (v == NULL) return;
    v->length = 0;
}

uint8_t* VLA_get(VLA* v, signed long index) {
    index = VLA_normalize_index(v, index);
    VLA_assert_in_bounds(v, index);

    return v->data + index * v->item_size;
}

size_t VLA_get_length(VLA* v) {
    if (v == NULL) return 0;
    return v->length / v->item_size;
}

void VLA_address_formatter(VLA* output, void* item) {
    unsigned long address = *(unsigned long*)item;
    const int n = snprintf(NULL, 0, "%p", (void*)address);
    char buffer[n + 1];
    snprintf(buffer, n + 1, "%p", (void*)address);
    VLA_batch_append(output, &buffer, n);
}

void VLA_print_setup_information_header(VLA* v, VLA* formatter) {
    int chars_written;
    const int n = snprintf(NULL, 0, "%zu", SIZE_MAX);
    char buffer[n + 1];

    VLA_batch_append(formatter, "VLA with item size: ", 20);
    chars_written = snprintf(buffer, n + 1, "%zu", v->item_size);
    VLA_batch_append(formatter, buffer, chars_written);
    VLA_batch_append(formatter, " bytes -- used space: ", 22);
    chars_written = snprintf(buffer, n + 1, "%zu", v->length);
    VLA_batch_append(formatter, buffer, chars_written);
    VLA_batch_append(formatter, "/", 1);
    chars_written = snprintf(buffer, n + 1, "%zu", v->capacity);
    VLA_batch_append(formatter, buffer, chars_written);
    VLA_batch_append(formatter, " bytes", 6);
}

void VLA_print_dump_data(VLA* v, VLA* output, void (*item_formatter)(VLA* formatter, void* item)) {
    if (v->length > 0) VLA_batch_append(output, " | ", 3);
    for (size_t offset = 0; offset < v->length; offset += v->item_size) {
        item_formatter(output, v->data + offset);
        VLA_batch_append(output, " | ", 3);
    }
}

void VLA_print(VLA* v, void (*item_formatter)(VLA* output, void* item)) {
#ifdef DEBUG
    if (item_formatter == NULL) {
        warn("To print items, please specify a formatter that can interpret them.\n");
        return;
    }

    VLA* output = VLA_initialize(v->length / v->item_size * 3 + INFO_HEADER_SIZE, sizeof(char));  // Rekursion!
    VLA_print_setup_information_header(v, output);
    VLA_print_dump_data(v, output, item_formatter);
    VLA_append(output, &(char){'\0'});

    debug("%s\n", (char*)output->data);
    VLA_free(output);
#endif
}

// Löscht den VLA und seine gespeicherten Daten
void VLA_free(VLA* v) {
    free(v->data);
    free(v);
}

// Löscht den VLA und gibt die gespeicherten Daten zurück
uint8_t* VLA_extract(VLA* v) {
    uint8_t* data = v->data;
    free(v);
    return data;
}