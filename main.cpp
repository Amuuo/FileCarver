
#include "main.h"


mutex                print_lock{};
mutex                main_finished_lock{};
mutex                block_match_mtx{};
mutex                progress_mtx{};
mutex                main_wait_lock{};
condition_variable   children_are_running{};
condition_variable   progress_bar_cv{};
condition_variable   main_cv{};
atomic<bool>         readingIsDone{false};
int                  terminate_early;
int                  match_count{0};
disk_pos*            file_pointer_position{nullptr};
map<disk_pos, char*> block_matches{};
disk_pos             offset_start{};
disk_pos             offset_end{};
disk_pos             filesize{0};
//const int            thread_array_size = 7;

array<mutex,thread_array_size> queue_wait_lck{};
array<mutex,thread_array_size> queue_lck{};
array<condition_variable,thread_array_size> queue_is_empty{};
array<deque<pair<uint8_t*,disk_pos>>,thread_array_size> block_queue{};


ScreenObj screenObj;
disk_pos main_loop_counter;










/*==============================
  ==============================
             M A I N
  ==============================
  ==============================
 */
int main(int argc, char** argv) {

  setlocale(LC_NUMERIC, "");


  signal(SIGINT, sig_handler);
  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGQUIT, sig_handler);
  signal(SIGHUP, sig_handler);
  signal(SIGABRT, sig_handler);



  Logger log;
  log.LaunchLogWindow();


  Cmd_Options cmds{get_cmd_options(argc, argv)};

  array<std::thread, thread_array_size> thread_arr;
  ifstream input_file;
  ofstream output_file;
  ofstream header_file;


  disk_pos thread_blk_sz = (cmds.blocksize / thread_array_size);

  open_io_files(input_file, output_file, cmds, log);
  uint8_t buffer[cmds.blocksize];

  cmds.print_all_options(log);

  // launch search threads
  for (int j = 0; j < thread_array_size; ++j)
  {
    thread_arr[j] = thread{&search_disk, j, thread_blk_sz, cmds.verbose};
  }

  offset_start = cmds.offset_start;
  offset_end   = cmds.offset_end;
  screenObj.init();


  short thrd_pos;
  // read disk and push data chunks into block_queue
  for (main_loop_counter =  cmds.offset_start;
       main_loop_counter <  cmds.offset_end;
       main_loop_counter += cmds.blocksize)
  {
    input_file.seekg(main_loop_counter);
    input_file.read((char*)buffer, cmds.blocksize);
    thrd_pos = main_loop_counter%thread_array_size;
    {
      unique_lock<mutex> lck(main_wait_lock);
      main_cv.wait_for(lck, 5us, [thrd_pos]{return block_queue[thrd_pos].size() < 500;});
      if(main_loop_counter%1000 == 0)
      {
        log << "\nQueue sizes: ";
        for(int i = 0; i < thread_array_size; ++i)
        {
          log << to_string(block_queue[i].size()).c_str() << ", ";
        }
      }
    }
    int i = 0;
    for (disk_pos j = 0; j < cmds.blocksize; j += thread_blk_sz, ++i)
    {
      uint8_t* transfer = new uint8_t[thread_blk_sz];
      memcpy(transfer, buffer + j, thread_blk_sz);
      {
        lock_guard<mutex> lck(queue_lck[i]);
        block_queue[i].push_back({transfer, main_loop_counter+j});
      }
      if(block_queue[i].size() > 500)
        queue_is_empty[i].notify_one();
    }
    if ((main_loop_counter % 10000) == 0 )
    {
      lock_guard<mutex> lck(print_lock);
      screenObj.update_scan_counter(main_loop_counter - cmds.offset_start);
    }
  }



  // set finished state and wait for threads to finish search
  readingIsDone = true;
  unique_lock<mutex> main_lck(main_finished_lock);

  children_are_running.wait_for(main_lck, 3000ms,
                                [] { return block_queue.empty(); });

  print_results(cmds, output_file);

  for(auto& match : block_matches)
  {
    output_file << match.first << " || ";
    for (int i = 0; i < 16; ++i)
    {
     output_file << (*(match.second + i) > 33 && *(match.second + i) <
              127 ? *(match.second + i) : '.');
    }
    output_file << endl;
  }

  input_file.close();
  output_file.close();

  //print results to console
  printf("\n\nSUCCESS!\n\n");
}

























/*=========------------=============
            OPEN IO FILES
 -------------======================*/
void open_io_files(ifstream& input_file, ofstream& output_file,
                   Cmd_Options& cmds, Logger& log) {


  log << "\nTrying to open input: " << cmds.input.c_str();

  input_file.open(cmds.input.c_str(), ios::in | ios::binary);
  if (input_file.fail())
  {
    log << "\n>> Input file failed to open";
    exit(1);
  }
  log << "\n>> Input file opened: " << cmds.input.c_str();


  cmds.output_file += cmds.output_folder;
  cmds.output_file += "/output.txt";
  log << "\nTrying to open output_file: " << cmds.output_file.c_str();

  output_file.open(cmds.output_file.c_str());
  if (output_file.fail())
  {
    log << "\n>> Output file failed to open";
    exit(1);
  }
  log << "\n>> Output file opened: " << cmds.output_file.c_str();
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
    {"input",         required_argument, NULL, 'i'},
    {"output_folder", required_argument, NULL, 'o'},
    {"verbose",       no_argument,       NULL, 'v'},
    {"help",          no_argument,       NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  int           cmd_option;
  extern char*  optarg;
  Cmd_Options   cmds;

  while ((cmd_option = getopt_long(argc, argv, "b:S:E:s:i:o:vh",
                                   long_options, NULL)) != -1)
  {
    switch (cmd_option)
    {
      case 'b': cmds.blocksize     = atoll(optarg);  break;
      case 'S': cmds.offset_start  = atoll(optarg);  break;
      case 'E': cmds.offset_end    = atoll(optarg);  break;
      case 'i': cmds.input         = string{optarg}; break;
      case 'o': cmds.output_folder = string{optarg}; break;
      case 'v': cmds.verbose       = true;           break;

      case 'h':
        printf (
        "\n\t--blocksize      -b  <size of logical blocks in bytes>"
        "\n\t--offset_start   -S  <offset from start of disk in bytes>"
        "\n\t--offset_end     -E  <offset from start of disk in bytes>"
        "\n\t--input          -i  <file to use for header search>"
        "\n\t--output_folder  -o  <folder to use for output files>"
        "\n\t--verbose        -v  <display verbose output of ongoing process>"
        "\n\t--help           -h  <print help options>\n\n"
        );

        exit(0);

      default: break;
    }
  }

  if (cmds.output_folder == "")
  {
    char tmp_output_folder[50];
    if (!(getcwd(tmp_output_folder, 50)))
      printf("\nCould not retrieve directory with 'getcwd'");
    cmds.output_folder = string{tmp_output_folder};
  }

  if (cmds.offset_end == 0)
  {
    printf("\n>> No offset end detected, setting offset to end of file");
    FILE* file_size_stream = fopen(cmds.input.c_str(), "r");
    fseek(file_size_stream, 0, SEEK_END);
    cmds.offset_end = ftell(file_size_stream);
    fclose(file_size_stream);
  }

  long long filesize = cmds.offset_end - cmds.offset_start;

  if (cmds.input == "")
  {
    printf("\n>> Input file required");
    exit(1);
  }
  return cmds;
}








/*=========------------=============
           SIGNAL HANDLER
 -------------======================*/
void sig_handler(int signal_number) {

  clear();
  curs_set(1);
  delwin(screenObj.win);
  endwin();
  refresh();
  system("reset");
  exit(0);
}

/*=========------------=============
           	 SEARCH DISK
 -------------======================*/
void search_disk(int thread_id, disk_pos blocksize, bool verbose) {

  Logger log;
  {
    lock_guard<mutex> lck{print_lock};
    log.LaunchLogWindow();
    log << "\nThread #" << to_string(thread_id).c_str();
  }

  vector<uint8_t> pattern {0x49, 0x49, 0x2a, 0x00, 0x10,
                           0x00, 0x00, 0x00, 0x43, 0x52};
  uint8_t* tmp = nullptr;
  disk_pos block_location {};
  this_thread::sleep_for(2s);

  for (;;)
  {
    {
      if(block_queue[thread_id].empty())
      {
        unique_lock<mutex> lck(queue_wait_lck[thread_id]);
        queue_is_empty[thread_id].wait(lck, [&, thread_id]
                                { return !block_queue[thread_id].empty(); });
      }
      lock_guard<mutex> q_lck(queue_lck[thread_id]);
      tmp = block_queue[thread_id].front().first;
      block_location = block_queue[thread_id].front().second;
      block_queue[thread_id].pop_front();
    }

    // header search algorithm
    for (disk_pos pattern_counter {0}, i {0}; i < blocksize; ++i)
    {
      if(pattern[pattern_counter] == tmp[i])
      {
        if ((pattern_counter+1) == pattern.size())
        {
          char preview[16];
          lock_guard<mutex> lock(print_lock);
          memcpy(preview, tmp + (i-pattern_counter), 16);
          block_matches[block_location + (i - pattern_counter)] = preview;

          screenObj.update_found_counter();

          log << "\nFOUND: " << print_hexdump(16, preview).c_str();
          log << "\n\tQueue.size(): " << to_string(block_queue[thread_id].size()).c_str();

        } else pattern_counter++;
      } else pattern_counter = 0;
    } delete [] tmp;
  }
}





/*=========------------=============
          PRINT ALL OPTIONS
 -------------======================*/
string Cmd_Options::print_all_options(Logger& log) {

  ostringstream oss;

  oss << "\nargc: "          << argc
      << "\nargv: "          << argv
      << "\nverbose: "       << verbose
      << "\nblocksize: "     << blocksize
      << "\noffset_start: "  << offset_start
      << "\noffset_end: "    << offset_end
      << "\noutput_folder: " << output_folder
      << "\ninput: "         << input
      << "\noutput_file: "   << output_file;

  log << oss.str();
}






/*=========------------=============
           	PRINT RESULTS
 -------------======================*/
void print_results(Cmd_Options& cmds, ofstream& output_file){


  time_t current_time;
  current_time = time(NULL);

  output_file << "offset start: " << cmds.offset_start << endl;
  output_file << "offset end: "   << cmds.offset_end   << endl;
  output_file << "blocksize: "    << cmds.blocksize    << endl;
  output_file << endl;

  for (auto& match : block_matches)
    output_file << match.first << ": " << match.second << endl;

}





/*=========------------=============
           	PRINT HEXDUMP
 -------------======================*/
string print_hexdump(int line_size, char* tmp) {

  ostringstream oss;
  using namespace boost;

  int j = 0;

  for (int i = 0; i < line_size; ++i) {
    oss << format("%02x ") % (unsigned short)(tmp[j+i]);
    //oss << setw(2) << setfill('0') << hex <<
    //        static_cast<unsigned short>(tmp[j + i]) << " ";
  }

  oss << "  |";

  for (int i = 0; i < line_size; ++i)

    oss << (tmp[j + i] > 33 && tmp[j + i] < 127 ? (char)(tmp[j+i]) : '.');

  oss << "|";
  return oss.str();
};