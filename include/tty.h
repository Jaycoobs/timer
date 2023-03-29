#ifndef TTY_H
#define TTY_H

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#define TTY_RED "\033[31m"
#define TTY_GREEN "\033[32m"
#define TTY_WHITE "\033[37m"

#define TTY_BOLD "\033[1m"
#define TTY_NORMAL "\033[0m"

void tty_get_cursor_pos(size_t* r, size_t* c) {
    // Save terminal attrs...
    struct termios attrs;
    tcgetattr(STDIN_FILENO, &attrs);

    // Set terminal to raw mode
    struct termios raw;
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    // Request cursor position
    printf("\033[6n");

    // Read response...
    const size_t RESPONSE_LENGTH = 100;
    char response[RESPONSE_LENGTH];
    int in;
    size_t i = 0;
    while ((in = fgetc(stdin)) != 'R' && i < RESPONSE_LENGTH-1) {
	    response[i++] = (char)in;
    }
    response[i] = '\0';
    printf("%s\n", response);

    // row begins after '['
    char* row = strchr(response, '[') + 1;

    // column begins after ';'
    char* column = strchr(row, ';') + 1;

    // Mark the end of the row text.
    *(column-1) = '\0';

    *r = atol(row);
    *c = atol(column);

    // Restore terminal attrs
    tcsetattr(STDIN_FILENO, TCSANOW, &attrs);
}

void tty_get_terminal_size(size_t* r, size_t* c) {
    // Save cursor pos
    printf("\033[s");

    // Move cursor down and right
    printf("\033[999;999H");

    // Get the cursor position
    tty_get_cursor_pos(r, c);

    // Restore cursor pos
    printf("\033[u");
}

void tty_home() {
    printf("\033[H");
}

void tty_clear() {
    printf("\033[2J");
}

void tty_hide_cursor() {
    printf("\033[?25l");
}

void tty_show_cursor() {
    printf("\033[?25h");
}

void tty_set_cursor_pos(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

#endif // TTY_H
