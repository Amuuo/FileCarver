
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
         STRUCT: CMD_OPTIONS
 -------------======================*/
struct Cmd_Options {
  Cmd_Options();
  Cmd_Options(int, char**);
  Cmd_Options(int, char**, bool, disk_pos, disk_pos, disk_pos, bool);
  Cmd_Options(const Cmd_Options& cpy);
  Cmd_Options(Cmd_Options&& cpy);

  disk_pos  blocksize     {0};
  disk_pos  offset_start  {0};
  disk_pos  offset_end    {0};
  string    output_folder {};
  string    input         {};
  string    output_file   {""};
  bool      carve_files   {false};
  int       argc          {0};
  int       searchsize    {0};
  char**    argv          {nullptr};
  bool      verbose       {false};

  vector<string> print_all_options();
};

