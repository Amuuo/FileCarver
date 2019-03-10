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
#include "cmd_options.h"
#include "logger.h"
#include "screenObj.h"


using namespace std;
typedef unsigned long long disk_pos;




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
void write_carved_files(string, ifstream&);
void progress_bar_thread(disk_pos, disk_pos);

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

