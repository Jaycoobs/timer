#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BILLION 1000000000

void time_now(struct timespec* t) {
    clock_gettime(CLOCK_MONOTONIC, t);
}

void time_add(struct timespec* d, struct timespec* a, struct timespec* b) {
    d->tv_sec = a->tv_sec + b->tv_sec;
    d->tv_nsec = a->tv_nsec + b->tv_nsec;

    if (d->tv_nsec > BILLION) {
        d->tv_sec += (time_t)(d->tv_nsec / BILLION);
        d->tv_nsec = d->tv_nsec % BILLION;
    } 
}

bool time_sub(struct timespec* d, struct timespec* a, struct timespec* b) {
    if (b->tv_sec > a->tv_sec || (b->tv_sec == a->tv_sec && b->tv_nsec > a->tv_nsec)) {
        return false;
    }

    d->tv_sec = a->tv_sec - b->tv_sec;
    d->tv_nsec = a->tv_nsec - b->tv_nsec;

    if (d->tv_nsec < 0) {
        d->tv_sec -= 1;
        d->tv_nsec += BILLION;
    }

    return true;
}

void time_parse(struct timespec* d, char* s) {
    char* saveptr1;
    char* saveptr2;

    char* whole = strtok_r(s, ".", &saveptr1);
    char* fraction = strtok_r(NULL, ".", &saveptr1);
    char* division = strtok_r(whole, ":", &saveptr2);

    long nsec = 0;

    if (fraction != NULL) {
        size_t fraction_len = strlen(fraction);

        if (fraction_len > 9) {
            fraction[9] = '\0';
            fraction_len = 9;
        }

        nsec = atol(fraction);
        
        for (; fraction_len < 9; fraction_len++)
            nsec *= 10;
    }

    time_t seconds = 0;

    while (division != NULL) {
        seconds *= 60;
        seconds += atol(division);
        division = strtok_r(NULL, ":", &saveptr2);
    }

    d->tv_sec = seconds;
    d->tv_nsec = nsec;
}

void time_print(char* buffer, size_t buffer_length, struct timespec* t, bool full) {
    int hours = t->tv_sec / 60 / 60;
    int minutes = (t->tv_sec / 60) % 60;
    int seconds = t->tv_sec % 60;
    int millis = (int)(t->tv_nsec / 1E6) % 1000;

    size_t length;

    if (hours > 0) {
        length = snprintf(buffer, buffer_length, "%d:%02d:%02d", hours, minutes, seconds);
    } else if (minutes > 0) {
        length = snprintf(buffer, buffer_length, "%d:%02d", minutes, seconds);
    } else {
        length = snprintf(buffer, buffer_length, "%d.%03d", seconds, millis);
    }

    if (full && (hours > 0 || minutes > 0)) {
        snprintf(buffer + length, buffer_length - length, ".%03d", millis);
    }
}
