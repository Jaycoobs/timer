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

#define VEC_IMPL
#include "vecs.h"
#undef VEC_IMPL

#include "tty.h"
#include "time_funcs.h"
#include "category.h"

#define TIMER_GREEN "\e[38;2;0;255;0m"
#define TIMER_RED "\e[38;2;255;0;0m"

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
} run_timer_t;

const size_t column_length = 16;

void timer_check_run(run_timer_t* timer) {
    size_t split_count = timer->category.names.length;

    struct timespec tdiff;

    if (timer->current_split >= split_count) {
        if (!timer->category.pb.present ||
            !time_sub(&tdiff, &timer->current_run.splits.data[split_count-1].value,
                      &timer->category.pb.splits.data[split_count-1].value))
        {
            timer->category.pb.present = true;
            for (size_t i = 0; i < split_count; i++) {
                timer->category.pb.splits.data[i] = timer->current_run.splits.data[i];
            }
        }
    }

    duration_t diff;
    for (size_t i = 0; i <= timer->current_split; i++) {
        timer->category.attempts.data[i] += 1;

        // don't overwrite gold on current split
        if (i == timer->current_split)
            continue;

        duration_t current_segment;
        duration_t best_segment = timer->category.golds.splits.data[i];

        run_get_split_duration(&current_segment, &timer->current_run, i);

        duration_sub(&diff, &current_segment, &best_segment);

        if (!best_segment.present || (diff.present && diff.minus)) {
            timer->category.golds.splits.data[i] = current_segment;
        }
    }
}

void timer_split(run_timer_t* timer) {
    if (timer->state == STOPPED && timer->current_split < timer->category.names.length) {
        timer->state = RUNNING;
        time_now(&timer->start_time);
    } else if (timer->state == PAUSED && timer->current_split < timer->category.names.length) {
        timer->state = RUNNING;

        struct timespec diff;
        struct timespec now;
        time_now(&now);

        time_sub(&diff, &now, &timer->pause_time);
        time_add(&timer->start_time, &timer->start_time, &diff);
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
        timer_check_run(timer);

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
                run_write(&timer.current_run, &timer.category.names, path_buffer, false);

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
