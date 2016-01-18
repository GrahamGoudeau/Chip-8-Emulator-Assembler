#include <ncurses.h>
#include <unistd.h>

// sudo apt-get install libncurses5-dev
// http://stackoverflow.com/questions/4025891/create-a-function-to-check-for-key-press-in-unix-using-ncurses
// https://viget.com/extend/game-programming-in-c-with-the-ncurses-library
int main() {
    initscr();
    noecho();
    curs_set(false);
    sleep(1);
    endwin();
}
