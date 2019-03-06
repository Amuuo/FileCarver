
#include <locale.h>
#include <ncurses.h>
#include <panel.h>
#include <unistd.h>
#include <cstring>
#include <mutex>
#include <thread>

void print_to_window(WINDOW*, long, int);

using namespace std;

mutex print_mtx{};

int main() {
  setlocale(LC_ALL, "");
  curs_set(0);

  int rows, cols;
  WINDOW* main_win = initscr();
  getmaxyx(main_win, rows, cols);
  WINDOW* my_wins[4];

  /* Create windows for the panels */
  my_wins[0] = derwin(main_win, rows / 2, cols / 2, 0, 0);
  my_wins[1] = derwin(main_win, rows / 2, cols / 2, 0, cols / 2);
  my_wins[2] = derwin(main_win, rows / 2, cols / 2, rows / 2, 0);
  my_wins[3] = derwin(main_win, rows / 2, cols / 2, rows / 2, cols / 2);

  for (int i = 0; i < 4; ++i) box(my_wins[i], 0, 0);

  refresh();

  thread win[4];

  win[0] = thread{&print_to_window, my_wins[0], 20000, 1};
  win[1] = thread{&print_to_window, my_wins[1], 50000, 2};
  win[2] = thread{&print_to_window, my_wins[2], 80000, 3};
  win[3] = thread{&print_to_window, my_wins[3], 60000, 4};

  for (int i = 0; i < 4; ++i) win[i].join();

  getch();
  endwin();
}

void print_to_window(WINDOW* cur_win, long time_to_sleep, int id) {
  int subwin_cur_row = 0;
  int subwin_cur_col = 1;
  int subwin_rows, subwin_cols;
  getmaxyx(cur_win, subwin_rows, subwin_cols);

  while (subwin_cur_col < subwin_cols) {
    while (subwin_cur_row++ < subwin_rows) {
      {
        lock_guard<mutex> lck(print_mtx);
        mvwprintw(cur_win, subwin_cur_row, subwin_cur_col, " Screen %d\n", id);
        // wprintw(my_wins[i], "  Screen %d\n", i+1);
        box(cur_win, 0, 0);
        wrefresh(cur_win);
      }
      usleep(time_to_sleep);
    }
    subwin_cur_row = 0;
    subwin_cur_col += strlen(" Screen #");
  }
}
