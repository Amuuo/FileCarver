
#pragma once
#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <array>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <queue>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <utility>
#include <functional>
#include <ctime>
#include <cmath>
#include <ncurses.h>
#include <sys/wait.h>
#include <menu.h>
#include <dirent.h>
#include <cstdio>
#include <memory>
#include <boost/format.hpp>


using namespace std;
typedef unsigned long long disk_pos;



/*=========------------=============
       STRUCT: SCREEN_OBJECT
 -------------======================*/
struct ScreenObj {

  ScreenObj();
  ~ScreenObj();

  void init();
  void print_str_vec();
  int half_len();
  int half_len(string& line);
  void print_centered_string(int row);
  void print_centered_string(int row, string& line);
  void update_found_counter();
  void update_scan_counter(disk_pos&& i);
  void refresh_screen();
  void refresh_progress_bar();
  void print_latest_find(string);

  int            title_color;
  int            x, y, row, col;
  int            screen_center_row;
  int            screen_center_col;
  int            file_counter;
  disk_pos       blocks_scanned;
  string         title;
  WINDOW*        win;
  const long     mb_size = 1024*1024;
  const long     gb_size = 1024*1024*1024;
  vector<string> title_art;
  ostringstream  oss;
  int            title_ending_row;
  int            last_pos;
  vector<string> opts;
  long double         progress_counter{0.0f};
  long double         title_width;

};
