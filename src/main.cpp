
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
map<disk_pos, uint8_t*> block_matches{};
disk_pos             offset_start{};
disk_pos             offset_end{};
disk_pos             filesize{0};
//const int            thread_array_size = 6;

array<mutex,thread_array_size> queue_wait_lck{};
array<mutex,thread_array_size> queue_lck{};
array<condition_variable,thread_array_size> queue_is_empty{};
array<deque<pair<uint8_t*,disk_pos>>,thread_array_size> block_queue{};

int input_fd;
ScreenObj screenObj;
disk_pos main_counter;









/*==========================================================
  ==========================================================
                          M A I N
  ==========================================================
  ==========================================================
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

  Cmd_Options cmds(argc, argv);



  Logger log;

  #ifdef DEBUG
    log.LaunchLogWindow();
  #endif



  array<std::thread, thread_array_size> thread_arr;
  ifstream input_file;
  ofstream output_file;
  ofstream header_file;
  disk_pos thread_blk_sz = (cmds.blocksize / thread_array_size);

  open_io_files(input_file, output_file, cmds, log);



  // launch search threads
  for (int j = 0; j < thread_array_size; ++j)
    thread_arr[j] = thread{&search_disk, j, thread_blk_sz, cmds.verbose};


  offset_start = cmds.offset_start;
  offset_end   = cmds.offset_end;
  screenObj.init();

  screenObj.opts = cmds.print_all_options();

  int starting_row = screenObj.title_ending_row + 9;
  screenObj.print_str_vec();
  box(screenObj.win, 0, 0);
  refresh();

  uint8_t* buffers[thread_array_size];
  struct iovec _iovec[thread_array_size];

  for (int i = 0; i < thread_array_size; ++i)
    _iovec[i].iov_len = thread_blk_sz;


  thread progress_bar{&progress_bar_thread, cmds.offset_start, cmds.filesize};
  progress_bar.detach();

  main_counter = cmds.offset_start;



  /**************  M A I N  L O O P  *****************/
  /* read disk and push data chunks into block_queue */
  for (; main_counter <  cmds.offset_end; main_counter += cmds.blocksize)
  {
    for (int i = 0; i < thread_array_size; ++i)
    {
      buffers[i] = new uint8_t[thread_blk_sz];
      _iovec[i].iov_base = buffers[i];
    }
    preadv(input_fd, _iovec, thread_array_size, main_counter);

    int i = 0;
    for (disk_pos j = 0; j < cmds.blocksize; j += thread_blk_sz, ++i)
    {
      {
        unique_lock<mutex> main_lck(main_wait_lock);
        main_cv.wait_for(main_lck, 1s,
                         [] { return block_queue[0].size() < 10000; });

        lock_guard<mutex> lck(queue_lck[i]);
        block_queue[i].push_back({buffers[i], main_counter+j});
      }
      queue_is_empty[i].notify_one();
    }
  }
  /**************  M A I N  L O O P  *****************/




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
  if (!block_matches.empty())
    write_carved_files(cmds.output_file, input_file);
}




/*===================================
            OPEN IO FILES
 ===================================*/
inline void open_io_files(ifstream & input_file, ofstream & output_file,
                   Cmd_Options & cmds, Logger & log) {
#ifdef DEBUG
  log << "\nTrying to open input: " << cmds.input.c_str();
#endif
  input_fd = open(cmds.input.c_str(), O_RDONLY);
/*
  input_file.open(cmds.input.c_str(), ios::in | ios::binary);
  if (input_file.fail())
  {
#ifdef DEBUG
    log << "\n>> Input file failed to open";
#endif
    exit(1);
  }
*/
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



/*===================================
           	 SEARCH DISK
===================================*/
void search_disk(int thread_id, disk_pos blocksize, bool verbose) {

#ifdef DEBUG
  Logger log;
  {
    lock_guard<mutex> lck{print_lock};
    log.LaunchLogWindow();
    log << "\nThread #" << to_string(thread_id).c_str();
  }
#endif
  //vector<uint8_t> pattern{0xff, 0xd8, 0xff};
  vector<uint8_t> pattern{0x49, 0x49, 0x2a, 0x00, 0x10,
                          0x00, 0x00, 0x00, 0x43, 0x52};
  uint8_t* tmp = nullptr;
  disk_pos block_location {};
  disk_pos minimum_file_size = 1000000;
  this_thread::sleep_for(2s);

  for (;;)
  {
    if(block_queue[thread_id].empty())
    {
      unique_lock<mutex> lck(queue_wait_lck[thread_id]);
      queue_is_empty[thread_id].wait(lck, [&, thread_id]
                              { return !block_queue[thread_id].empty(); });
    }
    {
      lock_guard<mutex> q_lck(queue_lck[thread_id]);
      tmp = block_queue[thread_id].front().first;
      block_location = block_queue[thread_id].front().second;
      block_queue[thread_id].pop_front();
    }


    // header search algorithm
    for (disk_pos pattern_counter = 0, i = 0; i < blocksize; ++i)
    {
      if(pattern[pattern_counter] == tmp[i])
      {
        if ((pattern_counter+1) == pattern.size())
        {
          uint8_t preview[16];
          lock_guard<mutex> lock(print_lock);
          memcpy(preview, tmp + (i-pattern_counter), 16);
          block_matches[block_location + (i - pattern_counter)] = preview;

          screenObj.update_found_counter();
          screenObj.print_latest_find(
              (boost::format("%s -- %s") % to_string(main_counter).c_str() %
               print_hexdump(16, preview).c_str()).str());

          i += minimum_file_size;

#ifdef DEBUG
          log << "\nFOUND: " << print_hexdump(16, preview).c_str();
          log << "\n\tQueue.size(): " << to_string(block_queue[thread_id].size()).c_str()
              << ", Disk Marker: " << to_string(main_counter).c_str();
#endif

        } else pattern_counter++;
      } else pattern_counter = 0;
    } delete [] tmp;
  }
}



/*===================================
         PROGRESS BAR THREAD
===================================*/
void progress_bar_thread(disk_pos offset_start, disk_pos filesize) {

  while (!readingIsDone)
  {
    this_thread::sleep_for(500ms);
    lock_guard<mutex> lck(print_lock);
    //if (block_queue[0].size() < 100000) main_cv.notify_one();
    screenObj.progress_counter =
        (static_cast<long double>(main_counter - offset_start) /
          filesize);
    screenObj.refresh_progress_bar();
  }
}



/*===================================
           	PRINT HEXDUMP
 ===================================*/
inline string print_hexdump(int line_size, uint8_t* tmp) {

  ostringstream oss;
  using namespace boost;

  int j = 0;

  for (int i = 0; i < line_size; ++i) {

    oss << format("%02x ") % (short)(tmp[j+i]);
  }

  oss << "  |";

  for (int i = 0; i < line_size; ++i)
    oss << (isascii(static_cast<int>(tmp[j+i])) ? static_cast<char>(tmp[j+i]) : '.');

  oss << "|";
  return oss.str();
}



/*===================================
           SIGNAL HANDLER
 ===================================*/
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




/*===================================
          WRITE CARVED FILES
 ===================================*/
inline void write_carved_files(string output_file, ifstream& input_file){

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


/*===================================
           	PRINT RESULTS
 ===================================*/
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