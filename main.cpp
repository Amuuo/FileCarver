#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#include <menu.h>
#include <dirent.h>
#include <cstdio>
#include <memory>



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


using namespace std;
typedef unsigned long long disk_pos;


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

  void print_all_options();
};



/*=========------------=============
        FUNCTION DEFINITIONS
 -------------======================*/
Cmd_Options get_cmd_options (int, char**);
void open_io_files   (ifstream&, ofstream&, Cmd_Options&);
void sig_handler     (int signal_number);
void search_disk     (int, disk_pos, int, bool);
void print_results   (Cmd_Options&, ofstream&);
void print_hexdump   (int, vector<uint8_t>&);
void print_progress  ();
//void launch_menu     ();
void ncurses_testing ();

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
int                  row;
int                  col;
deque<pair<shared_ptr<uint8_t*>,disk_pos>> block_queue{};
disk_pos       filesize{0};

//#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
//#define CTRLD 5
//char* menu_choices[]{"Continue", "New", "Settings", "More", "About", "Exit"};
struct dirent64** dir_list;




struct ScreenObj {


  ScreenObj(){}
  void init() {
    win = initscr();
    // resizeterm(25, 50);
    clear();
    curs_set(0);
    border(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK,
           COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
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
    ~ScreenObj() {

    }

    int       title_color;
    int       x, y, row, col;
    int       screen_center_row;
    int       screen_center_col;
    int       file_counter;
    disk_pos  blocks_scanned;
    string    title;
    WINDOW*   win;

    string oss;


    int half_len(string& line) {

      return line.size()/2;
    }

    void print_centered_string(string& line, int row) {
      mvwprintw(win, row, screen_center_col - half_len(line), "%s",
                line.c_str(), NULL);
      oss.erase();
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
      oss += "Files found: " + to_string(file_counter);
      print_centered_string(oss, screen_center_row - 2);
      oss += "Bytes scanned: " + to_string(blocks_scanned);
      print_centered_string(oss, screen_center_row);
      oss.erase();
      refresh();
    }
};



ScreenObj screenObj;


/*==============================
  ==============================
             M A I N
  ==============================
  ==============================
 */
int main(int argc, char** argv) {

  //int intput;
  //cin >> intput;


  Cmd_Options cmds{get_cmd_options(argc, argv)};

  array<std::thread, 8> thread_arr;
  ifstream input_file;
  ofstream output_file;
  ofstream header_file;


  //long double progress_multiplier = 100.0f /  (cmds.offset_end-cmds.offset_start);
  disk_pos thread_blk_sz = cmds.blocksize / 8;

  //open_io_files(input_file, output_file, cmds);
  //uint8_t* buffer = new uint8_t[cmds.blocksize];
  uint8_t buffer[cmds.blocksize];

  // launch search threads
  for (int j = 0; j < 8; ++j) {
    if (cmds.verbose) cout << "\nSpawing thread #" << j + 1 << endl;

    thread_arr[j] = thread{&search_disk, j + 1, thread_blk_sz,
                           cmds.searchsize, cmds.verbose};
  }


  system("clear");
  offset_start = cmds.offset_start;
  offset_end = cmds.offset_end;

  screenObj.init();

  // ===============================================
  //        N C U R S E S   T E S T   A R E A
  // ===============================================







  // ===============================================
  // read disk and push data chunks into block_queue
  // ===============================================

  for (disk_pos i = cmds.offset_start; i < cmds.offset_end; i += cmds.blocksize)
  {

    /*
    if (i == cmds.offset_start)
    {
      file_pointer_position = &i;
      thread progress_thread{&print_progress};
      //progress_thread.detach();
    }
    */

    input_file.seekg(i);
    input_file.read((char*)buffer, cmds.blocksize);


    if (cmds.verbose)
    {
      {
        int counter = 0;
        lock_guard<mutex> lck(print_lock);
        cout << endl << endl << "MAIN THREAD: " << endl;
        for (int k = 0; k < cmds.blocksize; ++k)
        {
          cout << setw(2) << setfill('0') << hex <<
                  static_cast<unsigned short>(buffer[k]);
          if(counter++ % cmds.searchsize == 0)
            cout << endl;
        }
        cout << endl << endl;
      }
    }

    for (disk_pos j = 0; j < cmds.blocksize; j += thread_blk_sz)
    {
      {
        lock_guard<mutex> lck(queue_lck);
        /*shared_ptr<uint8_t>*/auto transfer = make_shared<uint8_t*>(new uint8_t(thread_blk_sz));
        memcpy(transfer.get(), buffer + j, thread_blk_sz);
        //auto pos = i + j;
        block_queue.push_back({transfer, i+j});
      }
      queue_is_empty.notify_one();
    }
    if (/*cmds.verbose && */(i % 10000000) < 100 )
    {
      lock_guard<mutex> lck(queue_lck);
      screenObj.update_scan_counter(i - cmds.offset_start);
    }
  }
  // ===============================================




  // ========================================================
  // set finished state and wait for threads to finish search
  // ========================================================
  readingIsDone = true;
  unique_lock<mutex> main_lck(main_finished_lock);

  children_are_running.wait_for(main_lck, 3000ms, [] { return block_queue.empty(); });

  print_results(cmds, output_file);

  for(auto& match : block_matches){
    output_file << match.first << " || ";
    for (int i = 0; i < 16; ++i) {
     output_file << (*(match.second + i) > 33 && *(match.second + i) <
              127 ? *(match.second + i) : '.');
    }
    output_file << endl;
  }

  input_file.close();
  output_file.close();

  curs_set(1);
  clear();
  endwin();
  //print results to console
  printf("\n\nSUCCESS!\n\n");
}
  /*
  cout << "Match count: " << block_matches.size() << endl;
	for (auto& match : block_matches)
		cout << match.first << ": " << match.second << endl;

  print_results(cmds);

}
/*
  ==============================
  ==============================
  ==============================
  ==============================
*/





/*=========------------=============
            OPEN IO FILES
 -------------======================*/
void open_io_files(ifstream& input_file, ofstream& output_file, Cmd_Options& cmds) {

  cout << "\n"
       << "Trying to open input: " << cmds.input << endl;
  input_file.open(cmds.input.c_str(), ios::in | ios::binary);
  if (input_file.fail()) {
    fprintf(stderr, "\n>> Input file failed to open");
    exit(1);
  }
  cout << "\n>> Input file opened: " << cmds.input << endl;

  cmds.output_file += cmds.output_folder;
  cmds.output_file += "/output.txt";
  cout << "\n"
       << "Trying to open output_file: " << cmds.output_file.c_str() << endl;
  output_file.open(cmds.output_file.c_str());
  if (output_file.fail()) {
    fprintf(stderr, "\n>> Output file failed to open");
    exit(1);
  }
  cout << "\n>> Output file opened: " << cmds.output_file << endl;
}









/*=========------------=============
           GET CMD OPTIONS
 -------------======================*/
Cmd_Options get_cmd_options(int argc, char** argv) {
  setlocale(LC_NUMERIC, "");

  static struct option long_options[] =
  {
      {"blocksize",     required_argument, NULL, 'b'},
      {"offset_start",  required_argument, NULL, 'S'},
      {"offset_end",    required_argument, NULL, 'E'},
      {"searchsize",    required_argument, NULL, 's'},
      {"input",         required_argument, NULL, 'i'},
      {"output_folder", required_argument, NULL, 'o'},
      {"searchsize",    required_argument, NULL, 's'},
      {"verbose",       no_argument,       NULL, 'v'},
      {"help",          no_argument,       NULL, 'h'},
      {NULL, 0, NULL, 0}
  };

  int           cmd_option;
  extern char*  optarg;
  Cmd_Options   cmds;

  while ((cmd_option = getopt_long(argc, argv, "b:S:E:s:i:o:vh",
                                   long_options, NULL)) != -1) {
    switch (cmd_option) {

      case 'b': cmds.blocksize     = atoll(optarg);  break;
      case 'S': cmds.offset_start  = atoll(optarg);  break;
      case 'E': cmds.offset_end    = atoll(optarg);  break;
      case 's': cmds.searchsize    = atoi(optarg);   break;
      case 'i': cmds.input         = string{optarg}; break;
      case 'o': cmds.output_folder = string{optarg}; break;
      case 'v': cmds.verbose       = true;           break;

      case 'h':
        printf (
        "\n\t--blocksize      -b  <size of logical blocks in bytes>"
        "\n\t--offset_start   -S  <offset from start of disk in bytes>"
        "\n\t--offset_end     -E  <offset from start of disk in bytes>"
        "\n\t--searchsize     -s  <size of chunk to search for input disk in bytes>"
        "\n\t--input          -i  <file to use for header search>"
        "\n\t--output_folder  -o  <folder to use for output files>"
        "\n\t--verbose        -v  <display verbose output of ongoing process>"
        "\n\t--help           -h  <print help options>\n\n"
        );

        exit(0);

      default: break;
    }
  }

  if (cmds.output_folder == "") {
    char tmp_output_folder[50];

    if (!(getcwd(tmp_output_folder, 50)))
      printf("\nCould not retrieve directory with 'getcwd'");

    cmds.output_folder = string{tmp_output_folder};

    if (cmds.verbose) {
      cmds.output_folder += '/';
      printf("\n>> Setting current directory for output: %s",
             cmds.output_folder.c_str());
    }
  }

  if (cmds.offset_end == 0) {
    printf("\n>> No offset end detected, setting offset to end of file");
    FILE* file_size_stream = fopen(cmds.input.c_str(), "r");
    fseek(file_size_stream, 0, SEEK_END);
    cmds.offset_end = ftell(file_size_stream);
    fclose(file_size_stream);
  }


  long long filesize = cmds.offset_end - cmds.offset_start;
  //if (filesize < cmds->blocksize) cmds->blocksize = filesize;


  if (cmds.verbose) {
    printf("\n>> Offset Start: %'lld bytes", cmds.offset_start);
    printf("\n>> Offset End: %'lld bytes", cmds.offset_end);
    printf("\n>> Input file: %s", cmds.input.c_str());
    printf("\n>> Input file size: %'lld bytes", filesize);
    printf("\n>> Blocksize: %'lld bytes", cmds.blocksize);
    printf("\n>> Search Size: %'d bytes", cmds.searchsize);
    printf("\n>> Output folder: %s", cmds.output_folder.c_str());
  }

  if (cmds.input == "") {
    printf("\n>> Input file required");
    exit(1);
  }
  return cmds;
}








/*=========------------=============
           SIGNAL HANDLER
 -------------======================*/
void sig_handler(int signal_number) {
  // terminate_early = 1;
}






/*=========------------=============
           	 SEARCH DISK
 -------------======================*/
void search_disk(int thread_id, disk_pos blocksize,
                 int searchsize, bool verbose) {


  if (verbose) {

    lock_guard<mutex> print_lck(queue_lck);
    cout << endl;
    cout << "Thread ID: "  << thread_id  << endl;
    cout << "Blocksize: "  << blocksize  << endl;
    cout << "Searchsize: " << searchsize << endl;
    cout << "Verbose: "    << boolalpha  << verbose << endl;
    cout << endl;
  }

  int counter {0};

  vector<uint8_t> pattern {0x49, 0x49, 0x2a, 0x00, 0x10,
                          0x00, 0x00, 0x00, 0x43, 0x52};

  shared_ptr<uint8_t*> tmp;
  disk_pos block_location {};

  if (verbose)
    printf("\n>> Transfer alloc... %'d bytes", searchsize);



  for (;;) {

    if(verbose && block_queue.size() % 1000 == 0){
      cout << "Thread #" << thread_id << ": Queue.size(): " <<
            block_queue.size() << ", Block location: " <<block_location << endl;
    }

  	if (readingIsDone && block_queue.empty())
			return;

    else{
      {
        unique_lock<mutex> lck(queue_wait_lck);
        queue_is_empty.wait(lck, [] { return !block_queue.empty(); });
        lock_guard<mutex> q_lck(queue_lck);
        tmp = block_queue.front().first;
        block_location = block_queue.front().second;
        block_queue.pop_front();
      }
      if(verbose) {

        cout << endl << endl;
        cout << "Thread #" << thread_id << ": " << endl;

        for (int c = 0; c < blocksize; ++c) {
            cout << setw(2) << setfill('0') << hex
                 << static_cast<unsigned short>(c);

            if (counter++ % searchsize == 0)
              cout << endl;
          }
        cout << endl << endl;
      }
    }


    for (disk_pos pattern_counter {0}, i {0}; i < blocksize; ++i) {
      if(pattern[pattern_counter] == *(tmp.get())[i]) {
        if ((pattern_counter+1) == pattern.size()) {
          char preview[16];
          lock_guard<mutex> lock(print_lock);
          memcpy(preview, (tmp.get()) + (i-pattern_counter), 16);
          block_matches[block_location + (i - pattern_counter)] = preview;

          screenObj.update_found_counter();
          /*
          for (int j = 0; j < (block_matches.size() % 30) + 1; ++j)
            cout << endl;

          cout << block_location + (i - pattern_counter) << " || ";

          for (int d = 0; d < 16; ++d)
            cout << ((preview[d] > 33 && preview[d] < 127) ? preview[d] : '.');

          cout << endl;
          pattern_counter = 0;
          i += blocksize;*/
        }
        else
          pattern_counter++;
      }
      else
        pattern_counter = 0;
    }
    //delete [] tmp;
  }
}





/*=========------------=============
          PRINT ALL OPTIONS
 -------------======================*/
void Cmd_Options::print_all_options() {
  cout << endl;
  cout << "argc: " << argc << endl;
  cout << "argv: " << argv << endl;
  cout << "verbose: " << verbose << endl;
  cout << "blocksize: " << blocksize << endl;
  cout << "offset_start: " << offset_start << endl;
  cout << "offset_end: " << offset_end << endl;
  cout << "searchsize: " << searchsize << endl;
  cout << "output_folder: " << output_folder << endl;
  cout << "input: " << input << endl;
  cout << "output_file: " << output_file << endl;
}









/*=========------------=============
           	PRINT RESULTS
 -------------======================*/
void print_results(Cmd_Options& cmds, ofstream& output_file){


	time_t current_time;
	current_time = time(NULL);

	output_file << "offset start: " << cmds.offset_start << endl;
	output_file << "offset end: "   << cmds.offset_end   << endl;
	output_file << "searchsize: "   << cmds.searchsize   << endl;
	output_file << "blocksize: "    << cmds.blocksize    << endl;
  output_file << endl;

	for (auto& match : block_matches)
    output_file << match.first << ": " << match.second << endl;

}





/*=========------------=============
           	PRINT HEXDUMP
 -------------======================*/
void print_hexdump(int line_size, vector<uint8_t>& tmp) {

  int j = 0;

  for (int i = 0; i < line_size; ++i)
    cout << setw(2) << setfill('0') << hex <<
            static_cast<unsigned int>(tmp[j + i]) << " ";

  cout << "  |";

  for (int i = 0; i < line_size; ++i)
    cout << (tmp[j + i] > 33 && tmp[j + i] < 127 ?
                            static_cast<char>(tmp[j + i]) : '.');

  cout << "|";
  cout << endl;
};





/*=========------------=============
           PRINT PROGRESS
 -------------======================*/
void print_progress(){

  const disk_pos filesize = offset_end - offset_start;
  float          percentage{0.0f};
  int            progress_bar_length{40};
  const float    ratio      = static_cast<float>(progress_bar_length)/100;
  const float    simplify_1 = 100.0f/filesize;
  const float    simplify_2 = 100.0f*(offset_start/filesize);
  ostringstream  progress_stream {ios::binary|ios::out};



      progress_stream << "[" << B_BLACK << setw(progress_bar_length)
                      << " " << RESET << "]";

      cout << CURSOR_L << progress_stream.str();
      progress_stream.seekp(1);

      progress_stream << B_GREEN << setw(progress_bar_length) << " " << RESET;

      progress_stream.seekp(progress_bar_length + 4);
      progress_stream  << RESET  <<  right << setprecision(0) << " ";

      progress_stream.seekp((progress_bar_length/2)+2);
      progress_stream  << REVERSE
                       << (short)(percentage >= 100 ? 100 : percentage)
                       << "%" << RESET;

      cout << CURSOR_L << progress_stream.str();
}








