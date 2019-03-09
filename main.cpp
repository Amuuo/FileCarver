
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


  signal(SIGINT , sig_handler);
  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGQUIT, sig_handler);
  signal(SIGHUP , sig_handler);
  signal(SIGABRT, sig_handler);

  int tester;
  cin >> tester;

  Logger log;
#ifdef DEBUG
  log.LaunchLogWindow();
#endif


  Cmd_Options cmds(argc, argv);

  array<std::thread, thread_array_size> thread_arr;
  ifstream input_file;
  ofstream output_file;
  ofstream header_file;


  disk_pos thread_blk_sz = (cmds.blocksize / thread_array_size);

  open_io_files(input_file, output_file, cmds, log);
  uint8_t buffer[cmds.blocksize];

  // launch search threads
  for (int j = 0; j < thread_array_size; ++j)
  {
    thread_arr[j] = thread{&search_disk, j, thread_blk_sz, cmds.verbose};
  }

  offset_start = cmds.offset_start;
  offset_end   = cmds.offset_end;
  screenObj.init();

  screenObj.opts = cmds.print_all_options();

  int starting_row = screenObj.title_ending_row + 9;
  screenObj.print_str_vec();
  box(screenObj.win, 0, 0);
  refresh();
  // screenObj.print_centered_string(screenObj.title_ending_row + 7,
  //                                cmds.print_all_options());

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
#ifdef DEBUG
    if(main_loop_counter%1000 == 0)
    {
      log << "\nQueue sizes: ";
      for(int i = 0; i < thread_array_size; ++i)
      {
        log << to_string(block_queue[i].size()).c_str() << ", ";
      }
    }
#endif
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

  output_file.close();
  write_carved_files(cmds.output_file, input_file);
}














void write_carved_files(string output_file, ifstream& input_file){

  // write found file in output directory
  ifstream file_markers{output_file};
  string carved_files_folder_name = "/media/adam/The Vault/carved_files/";
  mkdir(carved_files_folder_name.c_str(), O_RDWR);

  int i = 0;

  disk_pos file_mark;
  disk_pos filesize = 1024 * 1024 * 35;
  uint8_t file_buff[1024 * 1024 * 35];

  string line;

  while (!file_markers.eof()) {

    getline(file_markers, line);
    istringstream iss{line};
    iss >> file_mark;

    input_file.seekg(file_mark);

    input_file.read((char*)file_buff, filesize);

    ofstream carved_file{
      string{carved_files_folder_name + to_string(i++) + ".cr2"}};

    carved_file.write((char*)file_buff, filesize);
  }

  input_file.close();
  // print results to console
  printf("\n\nSUCCESS!\n\n");

}

























/*=========------------=============
            OPEN IO FILES
 -------------======================*/
void open_io_files(ifstream & input_file, ofstream & output_file,
                   Cmd_Options & cmds, Logger & log) {
#ifdef DEBUG
  log << "\nTrying to open input: " << cmds.input.c_str();
#endif
  input_file.open(cmds.input.c_str(), ios::in | ios::binary);
  if (input_file.fail())
  {
#ifdef DEBUG
    log << "\n>> Input file failed to open";
#endif
    exit(1);
  }
#ifdef DEBUG
  log << "\n>> Input file opened: " << cmds.input.c_str();
#endif

  cmds.output_file += cmds.output_folder;
  cmds.output_file += "/output.txt";
#ifdef DEBUG
  log << "\nTrying to open output_file: " << cmds.output_file.c_str();
#endif

  output_file.open(cmds.output_file.c_str());
  if (output_file.fail())
  {
#ifdef DEBUG
    log << "\n>> Output file failed to open";
#endif
    exit(1);
  }
#ifdef DEBUG
  log << "\n>> Output file opened: " << cmds.output_file.c_str();
#endif
}







/*=========------------=============
           SIGNAL HANDLER
 -------------======================*/
void sig_handler(int signal_number) {

  ofstream quit_stream{"abort_data.txt"};

  for(auto& match : block_matches)
  {
    quit_stream << match.first << " || ";
    for (int i = 0; i < 16; ++i)
    {
     quit_stream << (*(match.second + i) > 33 && *(match.second + i) <
              127 ? *(match.second + i) : '.');
    }
    quit_stream << endl;
  }

  quit_stream.close();
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

#ifdef DEBUG
  Logger log;
  {
    lock_guard<mutex> lck{print_lock};
    log.LaunchLogWindow();
    log << "\nThread #" << to_string(thread_id).c_str();
  }
#endif

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

#ifdef DEBUG
          log << "\nFOUND: " << print_hexdump(16, preview).c_str();
          log << "\n\tQueue.size(): " << to_string(block_queue[thread_id].size()).c_str()
              << ", Disk Marker: " << to_string(main_loop_counter).c_str();
#endif

        } else pattern_counter++;
      } else pattern_counter = 0;
    } delete [] tmp;
  }
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