#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "terminal.h"

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

static bool prepare_output(FILE *output) {
#ifdef _WIN32
    intptr_t raw = _get_osfhandle(_fileno(output));
    DWORD mode;
    if (raw != -1 && GetConsoleMode((HANDLE)raw, &mode)) {
        if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0U &&
            !SetConsoleMode((HANDLE)raw, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) return false;
    }
#else
    (void)output;
#endif
    return true;
}

static bool write_escape(FILE *output, const char *sequence) {
    if (!prepare_output(output)) return false;
    return fputs(sequence, output) >= 0 && fflush(output) == 0;
}

bool terminal_clear(FILE *output) { return write_escape(output, "\x1b[2J\x1b[H"); }

bool terminal_position(FILE *output, size_t column, size_t row) {
    return prepare_output(output) && fprintf(output, "\x1b[%zu;%zuH", row, column) >= 0 && fflush(output) == 0;
}

bool terminal_cursor(FILE *output, bool visible) {
    return write_escape(output, visible ? "\x1b[?25h" : "\x1b[?25l");
}

void terminal_size(FILE *output, size_t *columns, size_t *rows) {
    *columns = 80U; *rows = 24U;
#ifdef _WIN32
    {
        intptr_t raw = _get_osfhandle(_fileno(output));
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (raw != -1 && GetConsoleScreenBufferInfo((HANDLE)raw, &info)) {
            *columns = (size_t)(info.srWindow.Right - info.srWindow.Left + 1);
            *rows = (size_t)(info.srWindow.Bottom - info.srWindow.Top + 1);
        }
    }
#else
    {
        struct winsize size;
        if (ioctl(fileno(output), TIOCGWINSZ, &size) == 0 && size.ws_col > 0U && size.ws_row > 0U) {
            *columns = (size_t)size.ws_col; *rows = (size_t)size.ws_row;
        }
    }
#endif
}

bool terminal_read_key(FILE *input, bool blocking, int *character, bool *available) {
    *available = false; *character = EOF;
#ifdef _WIN32
    if (input == stdin) {
        if (!blocking && !_kbhit()) return true;
        *character = _getch(); *available = true; return true;
    }
    if (!blocking) {
        long current = ftell(input), end;
        if (current < 0L || fseek(input, 0L, SEEK_END) != 0) return true;
        end = ftell(input); (void)fseek(input, current, SEEK_SET);
        if (end <= current) return true;
    }
    *character = fgetc(input); *available = *character != EOF; return true;
#else
    {
        int descriptor = fileno(input);
        struct termios previous, raw;
        bool changed = false;
        if (descriptor < 0) return false;
        if (isatty(descriptor) && tcgetattr(descriptor, &previous) == 0) {
            raw = previous;
            raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
            raw.c_cc[VMIN] = blocking ? 1 : 0;
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(descriptor, TCSANOW, &raw) == 0) changed = true;
        }
        if (!blocking) {
            fd_set set; struct timeval timeout = {0, 0}; int ready;
            FD_ZERO(&set); FD_SET(descriptor, &set);
            ready = select(descriptor + 1, &set, NULL, NULL, &timeout);
            if (ready <= 0) { if (changed) (void)tcsetattr(descriptor, TCSANOW, &previous); return ready == 0; }
        }
        *character = fgetc(input); *available = *character != EOF;
        if (changed) (void)tcsetattr(descriptor, TCSANOW, &previous);
        return true;
    }
#endif
}
