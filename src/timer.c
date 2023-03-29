#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "tty.h"

// 1E9 is not an integer ???
#define BILLION 1000000000

#define TIMER_GREEN "\e[38;2;0;255;0m"
#define TIMER_RED "\e[38;2;255;0;0m"

const size_t column_length = 16;

typedef struct {
    bool minus;
    bool present;
    struct timespec value;
} duration_t;

#define VEC_IMPL

#define VEC_TYPE duration_t
#include "vec.h"
#undef VEC_TYPE

#define VEC_TYPE char
#include "vec.h"
#undef VEC_TYPE

#define VEC_TYPE vec_char_t
#include "vec.h"
#undef VEC_TYPE

#undef VEC_IMPL

typedef struct {
    bool present;
    vec_duration_t_t splits;
} run_t;

typedef struct {
    char* name;
    size_t attempts;
    size_t completed;
    vec_vec_char_t_t names;
    run_t pb;
} category_t;

typedef enum {
    RUNNING,
    PAUSED,
    STOPPED
} run_timer_state_t;

typedef struct {
    struct timespec start_time;
    struct timespec pause_time;
    run_timer_state_t state;
    category_t category;
    run_t current_run;
    size_t current_split;
    bool new_pb;
} run_timer_t;

char* path_join(char* base, char* path) {
    size_t base_len = strlen(base);
    size_t path_len = strlen(path);
    char* buffer = malloc(base_len + path_len + 1);
    strcpy(buffer, base);
    if (buffer[base_len-1] != '/') {
        buffer[base_len] = '/';
        buffer[base_len+1] = '\0';
        base_len += 1;
    }
    strcpy(buffer+base_len, path);
    return buffer;
}

bool read_line(vec_char_t* line, FILE* f) {
    line->length = 0;
    while (!feof(f) && !ferror(f)) {
        int c = fgetc(f);

        if (c == EOF || c == '\n')
            vec_char_push(line, '\0');

        if (c == EOF)
            return false;

        if (c == '\n')
            return true;

        vec_char_push(line, (char)c);
    }
}

void buffer_right_justify(char* buffer, size_t buffer_length) {
    size_t length = strlen(buffer);
    memmove(buffer + buffer_length - length - 1, buffer, length+1);
    memset(buffer, ' ', buffer_length - length - 1);
}

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

void run_empty(run_t* r, size_t split_count) {
    r->present = true;
    vec_duration_t_init(&r->splits);

    duration_t s = { .present = false };
    for (size_t i = 0; i < split_count; i++) {
        vec_duration_t_push(&r->splits, s);
    }
}

bool run_load(run_t* r, char* path) {
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
    vec_duration_t_init(&r->splits);

    size_t line_num = 1;

    vec_char_t line;
    vec_char_init(&line);

    while (read_line(&line, f)) {
        vec_char_push(&line, '\0');

        char* saveptr;
        char* token;

        token = strtok_r(line.data, " \t", &saveptr);
        for (size_t i = 0; i < 2; i++) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (!token) {
                fprintf(stderr, "Failed to read run file %s at line %d\n", path, line_num);
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

bool run_write(run_t* r, category_t* c, char* path) {
    FILE* f = fopen(path, "w");

    if (!f) {
        fprintf(stderr, "Failed to open file for writing: %s\n", path);
        perror(NULL);
        return false;
    }

    duration_t duration;
    duration_t time;

    size_t buffer_lengths = 15;
    char duration_buffer[buffer_lengths];
    char time_buffer[buffer_lengths];

    for (size_t i = 0; i < r->splits.length; i++) {
        run_get_split_duration(&duration, r, i);
        time = r->splits.data[i];

        duration_print(duration_buffer, buffer_lengths, &duration, false, true);
        duration_print(time_buffer, buffer_lengths, &time, false, true);

        fprintf(f, "%-20s%15s%15s\n", c->names.data[i].data, duration_buffer, time_buffer);
    }

    fclose(f);
}

void run_delete(run_t* r) {
    if (r->present)
        vec_duration_t_delete(&r->splits);
}

bool names_load(vec_vec_char_t_t* names, char* path) {
    FILE* f = fopen(path, "r");

    if (!f) {
        fprintf(stderr, "Failed to open names file: %s\n", path);
        perror(NULL);
        return false;
    }

    vec_vec_char_t_init(names);

    vec_char_t current_name;
    vec_char_init(&current_name);

    while (read_line(&current_name, f)) {
        if (current_name.length < 2)
            continue;

        vec_vec_char_t_push(names, current_name);
        vec_char_init(&current_name);
    }

    fclose(f);
    return true;
}

void names_delete(vec_vec_char_t_t* names) {
    for (size_t i = 0; i < names->length; i++)
        vec_char_delete(&names->data[i]);
    vec_vec_char_t_delete(names);
}

bool read_ulong(size_t* l, char* path) {
    FILE* f = fopen(path, "r");

    if (!f)
        return false;

    vec_char_t line;
    vec_char_init(&line);

    read_line(&line, f);
    *l = atol(line.data);

    vec_char_delete(&line);
    fclose(f);
    return true;
}

void write_ulong(size_t l, char* path) {
    FILE* f = fopen(path, "w");

    if (!f)
        return;

    fprintf(f, "%lu\n", l);
    fclose(f);
}

void category_delete(category_t* category) {
    run_delete(&category->pb);
}

bool category_load(category_t* category, char* path) {
    // DANGER
    category->name = path;

    size_t path_len = strlen(path);
    char* path_buffer = malloc(path_len + 10);

    char* names_path = path_join(path, "splits");
    if (!names_load(&category->names, names_path)) {
        free(names_path);
        return false;
    }
    free(names_path);

    char* pb_path = path_join(path, "pb");
    if (!run_load(&category->pb, path_buffer)) {
        free(pb_path);
        names_delete(&category->names);
        return false;
    }
    free(pb_path);

    char* attempt_path = path_join(path, "attempts");
    if (!read_ulong(&category->attempts, attempt_path)) {
        category->attempts = 0;
    }
    free(attempt_path);

    char* completed_path = path_join(path, "completed");
    if (!read_ulong(&category->completed, completed_path)) {
        category->completed = 0;
    }
    free(completed_path);

    if (!category->pb.present) {
        run_empty(&category->pb, category->names.length);
        category->pb.present = false;
    }

    if (category->pb.splits.length != category->names.length) {
        fprintf(stderr, "The number of splits in your pb doesn't match the number of splits in this category.");
        category_delete(category);
        return false;
    }

    free(path_buffer);
    return true;
}

void category_write(category_t* category, char* path) {
    if (category->pb.present) {
        char* pb_path = path_join(path, "pb");
        run_write(&category->pb, category, pb_path);
        free(pb_path);
    }

    char* attempt_path = path_join(path, "attempts");
    write_ulong(category->attempts, attempt_path);
    free(attempt_path);

    char* completed_path = path_join(path, "completed");
    write_ulong(category->completed, completed_path);
    free(completed_path);
}

void timer_check_run(run_timer_t* timer) {
    size_t split_count = timer->category.names.length;

    struct timespec diff;

    if (!timer->category.pb.present ||
        !time_sub(&diff, &timer->current_run.splits.data[split_count-1].value,
                  &timer->category.pb.splits.data[split_count-1].value))
    {
        timer->new_pb = true;
        timer->category.pb.present = true;
        for (size_t i = 0; i < split_count; i++) {
            timer->category.pb.splits.data[i] = timer->current_run.splits.data[i];
        }
    }
}

void timer_split(run_timer_t* timer) {
    if (timer->state == STOPPED) {
        if (timer->current_split < timer->category.names.length) {
            timer->state = RUNNING;
            time_now(&timer->start_time);
        }
    } else if (timer->state == PAUSED) {
        if (timer->current_split < timer->category.names.length) {
            timer->state = RUNNING;

            struct timespec diff;
            struct timespec now;
            time_now(&now);

            time_sub(&diff, &now, &timer->pause_time);
            time_add(&timer->start_time, &timer->start_time, &diff);
        }
    } else if (timer->state == RUNNING) {
        struct timespec now;
        time_now(&now);

        duration_t* d = &timer->current_run.splits.data[timer->current_split];
        d->minus = false;
        d->present = true;
        time_sub(&d->value, &now, &timer->start_time);

        timer->current_split += 1;

        if (timer->current_split >= timer->category.names.length) {
            timer->state = PAUSED;
            timer->current_split = timer->category.names.length;
        }
    } else {
        // HUH
    }
}

void timer_stop(run_timer_t* timer) {
    if (timer->state == RUNNING) {
        timer->state = PAUSED;
        time_now(&timer->pause_time);
    } else if (timer->state == PAUSED) {
        timer->category.attempts += 1;

        if (timer->current_split >= timer->category.names.length) {
            timer->category.completed += 1;
            timer_check_run(timer);
        }

        timer->state = STOPPED;
        timer->current_split = 0;
        run_delete(&timer->current_run);
        run_empty(&timer->current_run, timer->category.names.length);
    }
}

void timer_skip(run_timer_t* timer) {
    if (timer->current_split >= timer->category.names.length-1)
        return;

    timer->current_run.splits.data[timer->current_split].present = false;
    timer->current_split += 1;
}

void timer_undo(run_timer_t* timer) {
    if (timer->current_split == 0)
        return;

    timer->current_run.splits.data[timer->current_split].present = false;
    timer->current_split -= 1;
}

void timer_draw_row(char* name, char* delta_color, char* delta, char* duration, char* time, size_t term_width, size_t term_height) {
    int name_width = (int) term_width;

    if (name_width > 3 * (int) column_length) {
        name_width -= 3 * (int) column_length;
    }

    fprintf(stdout, "%-*s%s%*s%s%*s%*s",
            name_width,
            name,
            delta_color,
            column_length,
            delta,
            TTY_WHITE,
            column_length,
            duration,
            column_length,
            time);
}

void timer_draw_split(run_timer_t* timer, size_t i, size_t term_width, size_t term_height) {
    tty_set_cursor_pos(1, i+3);

    duration_t delta;
    duration_t duration;

    run_get_split_duration(&duration, &timer->current_run, i);
    duration_sub(&delta, &timer->current_run.splits.data[i], &timer->category.pb.splits.data[i]);

    char* delta_color = TIMER_RED;
    if (!delta.present)
        delta_color = TTY_WHITE;
    else if (delta.minus)
        delta_color = TIMER_GREEN;

    char delta_buffer[column_length+1];
    char duration_buffer[column_length+1];
    char time_buffer[column_length+1];

    duration_print(delta_buffer, column_length, &delta, true, false);
    duration_print(duration_buffer, column_length, &duration, false, false);
    duration_print(time_buffer, column_length, &timer->current_run.splits.data[i], false, false);

    if (i == timer->current_split)
        fprintf(stdout, "%s", TTY_BOLD);

    timer_draw_row(
            timer->category.names.data[i].data,
            delta_color,
            delta_buffer,
            duration_buffer,
            time_buffer,
            term_width,
            term_height
        );

    if (i == timer->current_split)
        fprintf(stdout, "%s", TTY_NORMAL);
}

void timer_draw_splits(run_timer_t* timer, size_t term_width, size_t term_height) {
    tty_set_cursor_pos(1, 1);
    timer_draw_row(
            timer->category.name,
            TTY_WHITE,
            "+/-",
            "sgmt",
            "time",
            term_width,
            term_height
        );

    for (size_t i = 0; i < term_width; i++)
        fputc('-', stdout);

    for (size_t i = 0; i < timer->category.names.length; i++) {
        timer_draw_split(timer, i, term_width, term_height);
    }
    fflush(stdout);
}

void timer_update(run_timer_t* timer) {
    if (timer->state == RUNNING) {
        duration_t* d = &timer->current_run.splits.data[timer->current_split];

        struct timespec now;
        time_now(&now);

        d->present = true;
        time_sub(&d->value, &now, &timer->start_time);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Use %s <split dir>\n", argv[0]);
        return 1;
    }

    run_timer_t timer = {
        .state = STOPPED,
        .current_split = 0,
        .new_pb = false
    };

    if (!category_load(&timer.category, argv[1])) {
        return 1;
    }

    run_empty(&timer.current_run, timer.category.names.length);

    struct termios prev_tty_attrs;
    tcgetattr(STDIN_FILENO, &prev_tty_attrs);

    struct termios raw_tty_attrs;
    cfmakeraw(&raw_tty_attrs);
    raw_tty_attrs.c_cc[VMIN] = 0;
    raw_tty_attrs.c_cc[VTIME] = 0;

    tty_hide_cursor();
    tcsetattr(STDIN_FILENO, TCSANOW, &raw_tty_attrs);

    size_t term_width, term_height;
    tty_clear();
    tty_get_terminal_size(&term_height, &term_width);

    timer_draw_splits(&timer, term_width, term_height);

    int c;
    int redraw_counter = 0;
    while (true) {
        c = 0;
        read(STDIN_FILENO, &c, 1);

        if (c == 'q' || c == '\x03') {
            timer_stop(&timer);
            break;
        } else if (c == ' ') {
            timer_split(&timer);
            timer_draw_splits(&timer, term_width, term_height);
        } else if (c == 'j') {
            timer_skip(&timer);
            timer_draw_splits(&timer, term_width, term_height);
        } else if (c == 'k') {
            timer_undo(&timer);
            timer_draw_splits(&timer, term_width, term_height);
        } else if (c == '\x1b') {
            timer_stop(&timer);
            timer_draw_splits(&timer, term_width, term_height);
        } else if (c == 'w' && timer.state == PAUSED) {
            tcsetattr(STDIN_FILENO, TCSANOW, &prev_tty_attrs);
            tty_clear();
            tty_set_cursor_pos(1,1);
            tty_show_cursor();

            char* file_name = readline("Enter file name: ");

            if (file_name) {
                char* path_buffer = malloc(strlen(argv[1]) + strlen(file_name) + 1);
                strcpy(path_buffer, argv[1]);
                if (path_buffer[strlen(argv[1])-1] != '/')
                    strcat(path_buffer, "/");
                strcat(path_buffer, file_name);
                run_write(&timer.current_run, &timer.category, path_buffer);

                free(path_buffer);
                free(file_name);
            }

            tty_clear();
            tty_hide_cursor();
            tcsetattr(STDIN_FILENO, TCSANOW, &raw_tty_attrs);
            timer_draw_splits(&timer, term_width, term_height);
        } else if (c == 'r') {
            tty_get_terminal_size(&term_height, &term_width);
            tty_clear();
            timer_draw_splits(&timer, term_width, term_height);
        }

        if (timer.state == RUNNING && redraw_counter++ > 100) {
            redraw_counter = 0;
            timer_update(&timer);
            timer_draw_split(&timer, timer.current_split, term_width, term_height);
        }

        usleep(100);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &prev_tty_attrs);
    tty_show_cursor();

    category_write(&timer.category, argv[1]);
    category_delete(&timer.category);
    return 0;
}
