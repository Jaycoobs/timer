#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "run.h"
#include "util.h"

void run_empty(run_t* r, size_t split_count) {
    r->present = true;
    vec_duration_t_init(&r->splits);

    duration_t s = { .present = false };
    for (size_t i = 0; i < split_count; i++) {
        vec_duration_t_push(&r->splits, s);
    }
}

bool run_load(run_t* r, char* path, bool segments_only) {
    FILE* f = fopen(path, "r");

    if (!f) {
        if (errno == ENOENT) {
            r->present = false;
            return true;
        } else {
            fprintf(stderr, "Failed to read run file: %s\n", path);
            perror(NULL);
            return false;
        }
    }

    r->present = true;
    r->segments = segments_only;
    vec_duration_t_init(&r->splits);

    size_t field = 2;

    if (segments_only)
        field = 1;

    size_t line_num = 1;

    vec_char_t line;
    vec_char_init(&line);

    while (read_line(&line, f)) {
        vec_char_push(&line, '\0');

        char* saveptr;
        char* token;

        token = strtok_r(line.data, " \t", &saveptr);
        for (size_t i = 0; i < 1; i++) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (!token) {
                fprintf(stderr, "Failed to read run file %s at line %d\n", path, line_num);

                if (segments_only)
                    fprintf(stderr, "line must have split name and segment time\n");
                else
                    fprintf(stderr, "line must have split name, segment time, and split time\n");

                return false;
            }
        }

        duration_t s;
        duration_parse(&s, token);

        vec_duration_t_push(&r->splits, s);

        line.length = 0;
        line_num++;
    }

    vec_char_delete(&line);

    fclose(f);
    return true;
}

void run_get_split_duration(duration_t* d, run_t* r, size_t i) {
    if (i == 0) {
        *d = r->splits.data[i];
    } else {
        duration_sub(d, &r->splits.data[i], &r->splits.data[i-1]);
    }
}

bool run_write(run_t* r, vec_vec_char_t_t* names, char* path) {
    FILE* f = fopen(path, "w");

    if (!f) {
        fprintf(stderr, "Failed to open file for writing: %s\n", path);
        perror(NULL);
        return false;
    }

    duration_t duration;
    duration_t time;

    if (r->segments) {
        time.minus = false;
        time.present = true;
        time.value = (struct timespec) {
            .tv_sec = 0,
            .tv_nsec = 0
        };
    }

    size_t buffer_lengths = 15;
    char duration_buffer[buffer_lengths];
    char time_buffer[buffer_lengths];

    for (size_t i = 0; i < r->splits.length; i++) {
        if (r->segments) {
            duration = r->splits.data[i];
            duration_add(&time, &time, &duration);
        } else {
            time = r->splits.data[i];
            run_get_split_duration(&duration, r, i);
        }

        duration_print(duration_buffer, buffer_lengths, &duration, false, true);
        duration_print(time_buffer, buffer_lengths, &time, false, true);
        fprintf(f, "%-20s%15s%15s\n", names->data[i].data, duration_buffer, time_buffer);
    }

    fclose(f);
}

void run_delete(run_t* r) {
    if (r->present)
        vec_duration_t_delete(&r->splits);
}
