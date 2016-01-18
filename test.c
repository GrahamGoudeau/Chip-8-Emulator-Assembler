#include <ncurses.h>
#include <unistd.h>

// sudo apt-get install libncurses5-dev
// http://stackoverflow.com/questions/4025891/create-a-function-to-check-for-key-press-in-unix-using-ncurses
int main() {
    initscr();
    noecho();
    curs_set(false);
    sleep(1);
    endwin();
}
