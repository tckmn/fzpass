#include <stdlib.h>
#include <unistd.h>
#include <ftw.h>
#include <ncurses.h>
#include <string.h>

char **passwords = NULL;
int nPasswords = 0;
int addPassword(const char *fpath, const struct stat *sb, int typeflag) {
    if (typeflag == FTW_F && !strstr(fpath, "/.git/") && strstr(fpath, ".gpg")) {
        passwords = realloc(passwords, (++nPasswords) * sizeof *passwords);
        int passwordLen = strlen(fpath) - 5;
        passwords[nPasswords - 1] = malloc(passwordLen * sizeof *fpath);
        strncpy(passwords[nPasswords - 1], fpath + 2, passwordLen);
        passwords[nPasswords - 1][passwordLen - 1] = '\0';
    }
    return 0;
}

int keypress(WINDOW *typeWin, WINDOW *outputWin, char ch, char *buf) {
    char *p = buf;
    for (; *p; ++p);

    switch (ch) {
    case 3: // ^C
    case 4: // ^D
        return 0;
    case 21: // ^U
        werase(typeWin);
        box(typeWin, 0, 0);
        wmove(typeWin, 1, 1);
        for (p = buf; *p; ++p) *p = '\0';
        break;
    case 23: // ^W
        --p;
        for (; *p != ' ' && p != buf; --p) {
            *p = '\0';
            waddstr(typeWin, "\b \b");
        }
        *p = '\0';
        waddstr(typeWin, "\b \b");
        break;
    case 127: // ^? (backspace)
    case '\b':
        if (*buf) {
            waddstr(typeWin, "\b \b");
            *(p-1) = '\0';
        }
        break;
    default:
        if (ch >= ' ' && ch <= '~') {
            waddch(typeWin, ch);
            *p = ch;
        }
    }

    int numPrinted = 0;
    werase(outputWin);
    box(outputWin, 0, 0);
    for (int i = 0; i < nPasswords; ++i) {
        char *password = passwords[i];
        int pos = 0, npat = 0, len = strlen(password);
        int *highlight = calloc(len, sizeof *highlight);
        for (char *pat = buf; *pat; ++pat) {
            if (*pat == ' ') {
                pos = 0;
                ++npat;
            } else {
                for (; pos < len && password[pos] != *pat; ++pos);
                if (pos == len) goto skipprint;
                highlight[pos++] = npat + 1;
            }
        }
        if (numPrinted > LINES - 6) break;
        wmove(outputWin, ++numPrinted, 1);
        for (int i = 0; i < len; ++i) {
            wattrset(outputWin, highlight[i] ?
                    A_BOLD | COLOR_PAIR(highlight[i]) :
                    A_NORMAL);
            waddch(outputWin, password[i]);
        }
skipprint:
        free(highlight);
    }
    wattrset(outputWin, A_NORMAL);
    wrefresh(outputWin);
    wrefresh(typeWin);

    return 1;
}

int main(int argc, char* argv[]) {
    initscr();
    raw();
    noecho();

    start_color();
    for (int i = 1; i <= 7; ++i) {
        init_pair(i, i, COLOR_BLACK);
    }

    WINDOW *outputWin = newwin(LINES - 3, COLS, 3, 0);

    WINDOW *typeWin = newwin(3, COLS, 0, 0);
    refresh();
    box(typeWin, 0, 0);
    wmove(typeWin, 1, 1);
    wrefresh(typeWin);

    char *passDir;
    char *passEnvDir = getenv("PASSWORD_STORE_DIR");
    if (passEnvDir) passDir = passEnvDir;
    else {
        char *homeDir = getenv("HOME");
        passDir = malloc((strlen(homeDir) + 18) * sizeof *passDir);
        sprintf(passDir, "%s/.password-store/", homeDir);
    }
    chdir(passDir);
    if (ftw(".", addPassword, 15) == -1) {
        perror("ftw");
        return 1;
    }

    char *buf = calloc(512, sizeof *buf);
    keypress(typeWin, outputWin, '\b', buf);
    while (keypress(typeWin, outputWin, getch(), buf));

    endwin();
    return 0;
}
