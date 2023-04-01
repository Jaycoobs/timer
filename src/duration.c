#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "time_funcs.h"
#include "duration.h"

void duration_add(duration_t* d, duration_t* a, duration_t* b) {
    if (!a->present || !b->present) {
        d->present = false;
        return;
    }

    time_add(&d->value, &a->value, &b->value);
}

void duration_sub(duration_t* d, duration_t* a, duration_t* b) {
    if (!a->present || !b->present) {
        d->present = false;
    } else if (!time_sub(&d->value, &a->value, &b->value)) {
        time_sub(&d->value, &b->value, &a->value);
        d->present = true;
        d->minus = true;
    } else {
        d->present = true;
        d->minus = false;
    }
}

void duration_parse(duration_t* d, char* s) {
    if (!strcmp(s, "-")) {
        d->present = false;
    } else {
        d->present = true;
        if (*s == '-') {
            d->minus = true;
            s++;
        } else {
            d->minus = false;
        }
        time_parse(&d->value, s);
    }
}

void duration_print(char* buffer, size_t buffer_length, duration_t* d, bool sign, bool full) {
    if (!d->present) {
        strncpy(buffer, "-", buffer_length);
    } else {
        if (sign) {
            if (d->minus)
                *buffer = '-';
            else
                *buffer = '+';
            buffer++;
            buffer_length -= 1;
        }
        time_print(buffer, buffer_length, &d->value, full);
    }
}
