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


    title_art.push_back("                                   ");
    title_art.push_back(" '||''''| '||' '||'      '||''''|  ");
    title_art.push_back("  ||  .    ||   ||        ||  .    ");
    title_art.push_back("  ||''|    ||   ||        ||''|    ");
    title_art.push_back("  ||       ||   ||        ||       ");
    title_art.push_back(" .||.     .||. .||.....| .||.....| ");
    title_art.push_back("                                                              ");
    title_art.push_back("   ..|'''.|     |     '||''|.   '||'  '|' '||''''|  '||''|.   ");
    title_art.push_back(" .|'     '     |||     ||   ||   '|.  .'   ||  .     ||   ||  ");
    title_art.push_back(" ||           |  ||    ||''|'     ||  |    ||''|     ||''|'   ");
    title_art.push_back(" '|.      .  .''''|.   ||   |.     |||     ||        ||   |.  ");
    title_art.push_back("   '|....'  .|.  .||. .||.  '|'     |     .||.....| .||.  '|' ");
    title_art.push_back("                                                              ");



    win = initscr();
    // resizeterm(25, 50);

    init_pair(2, COLOR_BLACK, COLOR_BLACK);


    clear();
    start_color();
    curs_set(0);
    border(0,0,0,0,0,0,0,0);
    getmaxyx(win, row, col);
    screen_center_row = row / 3;
    screen_center_col = col / 2;

    init_pair(1, COLOR_WHITE, COLOR_BLUE);

    attron(COLOR_PAIR(1));
    for(int i = 0; i < title_art.size(); ++i){
      oss << title_art[i];
      print_centered_string((screen_center_row - 6) + i);
    }
    attroff(COLOR_PAIR(1));
    title_ending_row = getcury(win);
    refresh();
  }

  ~ScreenObj() {
    resetty();
    clear();
    endwin();
  }

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
  vector<string> title_art;
  ostringstream oss;
  int       title_ending_row;

  int half_len() {

    return oss.str().size()/2;
  }

  void print_centered_string(int row) {

    mvprintw(row, screen_center_col - half_len(), "%s",
              oss.str().c_str(), NULL);
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

    oss << "Files found: " << file_counter;
    print_centered_string(title_ending_row +4);

    oss << "MB scanned: " << blocks_scanned/mb_size;
    print_centered_string(title_ending_row +6);

    oss << "GB scanned: " << blocks_scanned/gb_size;
    print_centered_string(title_ending_row+7);

    refresh();
  }
};







/*=========------------=============
        FUNCTION DEFINITIONS
 -------------======================*/
Cmd_Options get_cmd_options (int, char**);
void open_io_files   (ifstream&, ofstream&, Cmd_Options&, Logger&);
void sig_handler     (int signal_number);
void search_disk     (int, disk_pos, bool);
void print_results   (Cmd_Options&, ofstream&);
string print_hexdump   (int, char*);
void print_progress  ();
void ncurses_testing ();
string print_all_options(Logger&);



extern mutex                print_lock;
extern mutex                main_finished_lock;
extern mutex                block_match_mtx;
extern mutex                progress_mtx;
extern mutex                main_wait_lock;
extern condition_variable   children_are_running;
extern condition_variable   progress_bar_cv;
extern condition_variable   main_cv;
extern atomic<bool>         readingIsDone;
extern int                  terminate_early;
extern int                  match_count;
extern disk_pos*            file_pointer_position;
extern map<disk_pos, char*> block_matches;
extern disk_pos             offset_start;
extern disk_pos             offset_end;
extern disk_pos             filesize;
extern const int            thread_array_size = 8;

extern array<mutex,thread_array_size> queue_wait_lck;
extern array<mutex,thread_array_size> queue_lck;
extern array<condition_variable,thread_array_size> queue_is_empty;
extern array<deque<pair<uint8_t*,disk_pos>>,thread_array_size> block_queue;


extern ScreenObj screenObj;
extern disk_pos main_loop_counter;

