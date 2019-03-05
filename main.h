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


using namespace std;
typedef unsigned long long disk_pos;



const char* BLUE      = "[34m";
const char* GREEN     = "[32m";
const char* RED       = "[31m";
const char* YELLOW    = "[33m";
const char* WHITE     = "[37m";
const char* CYAN      = "[36m";
const char* MAGENTA   = "[35m";
const char* BLACK     = "[30m";
const char* B_CYAN    = "[46m";
const char* B_BLACK   = "[40m";
const char* B_BLUE    = "[44m";
const char* B_RED     = "[41m";
const char* B_GREEN   = "[42m";
const char* B_WHITE   = "[47m";
const char* B_YELLOW  = "[43m";
const char* B_MAGENTA = "[45m";
const char* BOLD      = "[1m";
const char* RESET     = "[0m";
const char* REVERSE   = "[7m";
const char* TOP_LEFT  = "[H";
const char* CURSOR_S  = "[s";
const char* CURSOR_L  = "[u";





/*=========------------=============
         STRUCT: LOGGER
 -------------======================*/
struct Logger {


  Logger(){}

  ofstream outputFile{};
  int fd;

  void LaunchLogWindow()
  {

      char *name = tempnam(NULL, NULL);
      char cmd[256];

      mkfifo(name, 0600);
      if(fork() == 0)
      {
      sprintf(cmd, "xterm -e cat %s", name);
      system(cmd);
      exit(0);
      }
      outputFile.open(name);
  }

  Logger& operator<<(const char* in){
      outputFile << in;
      outputFile.flush();
  }
};







/*=========------------=============
         STRUCT: CMD_OPTIONS
 -------------======================*/
struct Cmd_Options {

  Cmd_Options(){}
  Cmd_Options(int _argc, char** _argv, bool v, disk_pos b,
              disk_pos os, disk_pos oe, int s) :
              argc{_argc}, argv{_argv}, verbose{v}, blocksize{b},
              offset_start{os}, offset_end{oe}, searchsize{s} {}
  Cmd_Options(const Cmd_Options& cpy){
    blocksize      = cpy.blocksize;
    offset_start   = cpy.offset_start;
    offset_end     = cpy.offset_end;
    output_folder  = cpy.output_folder;
    input          = cpy.input;
    output_file    = cpy.output_file;
    argc           = cpy.argc;
    searchsize     = cpy.searchsize;
    argv           = cpy.argv;
    verbose        = cpy.verbose;
  }
  Cmd_Options(Cmd_Options&& cpy){
    blocksize      = move(cpy.blocksize);
    offset_start   = move(cpy.offset_start);
    offset_end     = move(cpy.offset_end);
    output_folder  = move(cpy.output_folder);
    input          = move(cpy.input);
    output_file    = move(cpy.output_file);
    argc           = move(cpy.argc);
    searchsize     = move(cpy.searchsize);
    argv           = move(cpy.argv);
    verbose        = move(cpy.verbose);
  }

  disk_pos  blocksize     {0};
  disk_pos  offset_start  {0};
  disk_pos  offset_end    {0};
  string    output_folder {};
  string    input         {};
  string    output_file   {};
  int       argc          {0};
  int       searchsize    {0};
  char**    argv          {nullptr};
  bool      verbose       {false};

  void print_all_options(Logger&);
};





/*=========------------=============
       STRUCT: SCREEN_OBJECT
 -------------======================*/
struct ScreenObj {


  ScreenObj(){}


  void init() {

    win = initscr();
    // resizeterm(25, 50);
    clear();
    curs_set(0);
    border(0,0,0,0,0,0,0,0);
    getmaxyx(win, row, col);
    screen_center_row = row / 3;
    screen_center_col = col / 2;

    title_color = init_pair(1, COLOR_BLACK, COLOR_GREEN);

    title = "F I L E  C A R V E R";
    attron(COLOR_PAIR(1));
    print_centered_string(title, screen_center_row - 4);
    attroff(COLOR_PAIR(1));
    refresh_screen();
  }


  ~ScreenObj() { endwin(); }

  int       title_color;
  int       x, y, row, col;
  int       screen_center_row;
  int       screen_center_col;
  int       file_counter;
  disk_pos  blocks_scanned;
  string    title;
  WINDOW*   win;
  int       mb_size = 1024*1024;
  int       gb_size = 1024*1024*1024;
  ostringstream oss;


  int half_len(string& line) {

    return line.size()/2;
  }

  void print_centered_string(string line, int row) {
    mvprintw(row, screen_center_col - half_len(line), "%s",
              line.c_str(), NULL);
    oss.str("");
  }

  void update_found_counter() {
    flash();
    ++file_counter;
    refresh_screen();
  }

  void update_scan_counter(disk_pos&& i) {
    blocks_scanned = i;
    refresh_screen();
  }

  void refresh_screen() {
    oss << "Files found: " << to_string(file_counter);
    print_centered_string(oss.str(), screen_center_row - 2);
    oss.str("");
    oss << "MB scanned: " << to_string(blocks_scanned/mb_size);
    print_centered_string(oss.str(), screen_center_row);
    oss.str("");
    oss << "GB scanned: " << to_string(blocks_scanned/gb_size);
    print_centered_string(oss.str(), screen_center_row+1);
    oss.str("");
    refresh();
  }
};













/*=========------------=============
           GLOBAL VARIABLES
 -------------======================*/
mutex                queue_lck{};
mutex                queue_wait_lck{};
mutex                print_lock{};
mutex                main_finished_lock{};
mutex                block_match_mtx{};
mutex                progress_mtx{};
condition_variable   queue_is_empty{};
condition_variable   children_are_running{};
condition_variable   progress_bar_cv{};
atomic<bool>         readingIsDone{false};
int                  terminate_early;
int                  match_count{0};
disk_pos*            file_pointer_position{nullptr};
map<disk_pos, char*> block_matches{};
disk_pos             offset_start{};
disk_pos             offset_end{};
deque<pair<uint8_t*,disk_pos>> block_queue{};
disk_pos       filesize{0};

int log_file_descriptor;
int fd[2];
//#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
//#define CTRLD 5
// char* menu_choices[]{"Continue", "New", "Settings", "More", "About", "Exit"};
struct dirent64** dir_list;
ScreenObj screenObj;




/*=========------------=============
        FUNCTION DEFINITIONS
 -------------======================*/
Cmd_Options get_cmd_options (int, char**);
void open_io_files   (ifstream&, ofstream&, Cmd_Options&, Logger&);
void sig_handler     (int signal_number);
void search_disk     (int, disk_pos, int, bool);
void print_results   (Cmd_Options&, ofstream&);
string print_hexdump   (int, char*);
void print_progress  ();
void ncurses_testing ();







