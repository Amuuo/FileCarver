
#include "main.h"

/*==============================
  ==============================
             M A I N
  ==============================
  ==============================
 */
int main(int argc, char** argv) {


  signal(SIGINT, sig_handler);
  signal(SIGKILL, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGQUIT, sig_handler);
  signal(SIGHUP, sig_handler);
  signal(SIGABRT, sig_handler);

  Logger log;
  log.LaunchLogWindow();


  Cmd_Options cmds{get_cmd_options(argc, argv)};

  array<std::thread, 8> thread_arr;
  ifstream input_file;
  ofstream output_file;
  ofstream header_file;


  //long double progress_multiplier = 100.0f /  (cmds.offset_end-cmds.offset_start);
  disk_pos thread_blk_sz = cmds.blocksize / 8;

  open_io_files(input_file, output_file, cmds, log);
  uint8_t buffer[cmds.blocksize];

  cmds.print_all_options(log);

  // launch search threads
  for (int j = 0; j < thread_arr.size(); ++j) {
    if (cmds.verbose) cout << "\nSpawing thread #" << j + 1 << endl;

    thread_arr[j] = thread{&search_disk, j + 1, thread_blk_sz,
                           cmds.searchsize, cmds.verbose};
  }


  system("clear");
  offset_start = cmds.offset_start;
  offset_end = cmds.offset_end;

  screenObj.init();


  // read disk and push data chunks into block_queue
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
        log << "\n\nMAIN THREAD: \n";
        for (int k = 0; k < cmds.blocksize; ++k)
        {
          ostringstream oss{};
          oss << setw(2) << setfill('0') << hex <<
                  static_cast<unsigned short>(buffer[k]);
          log << oss.str().c_str();
          if(counter++ % cmds.searchsize == 0)
            log << "\n";
        }
        log << "\n\n";
      }
    }

    for (disk_pos j = 0; j < cmds.blocksize; j += thread_blk_sz)
    {
      {
        lock_guard<mutex> lck(queue_lck);
        uint8_t* transfer = new uint8_t[thread_blk_sz];
        memcpy(transfer, buffer + j, thread_blk_sz);
        //auto pos = i + j;
        block_queue.push_back({transfer, i+j});
      }
      queue_is_empty.notify_one();
    }
    if (/*cmds.verbose && */(i % 1000000) == 0 )
    {
      lock_guard<mutex> lck(queue_lck);
      screenObj.update_scan_counter(i - cmds.offset_start);
    }
  }


  // set finished state and wait for threads to finish search
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
void open_io_files(ifstream& input_file, ofstream& output_file, Cmd_Options& cmds, Logger& log) {


  log << "\nTrying to open input: " << cmds.input.c_str();

  input_file.open(cmds.input.c_str(), ios::in | ios::binary);
  if (input_file.fail()) {
    log << "\n>> Input file failed to open";
    exit(1);
  }
  log << "\n>> Input file opened: " << cmds.input.c_str();


  cmds.output_file += cmds.output_folder;
  cmds.output_file += "/output.txt";
  log << "\nTrying to open output_file: " << cmds.output_file.c_str();

  output_file.open(cmds.output_file.c_str());
  if (output_file.fail()) {
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
  clear();
  curs_set(1);
  delwin(screenObj.win);
  endwin();
  refresh();
  system("clear");
  exit(1);
}

/*=========------------=============
           	 SEARCH DISK
 -------------======================*/
void search_disk(int thread_id, disk_pos blocksize,
                 int searchsize, bool verbose) {

  Logger log;
  {
    lock_guard<mutex> lck{queue_lck};
    log.LaunchLogWindow();
  }


  ostringstream oss{};
  if (verbose) {
    lock_guard<mutex> print_lck(queue_lck);


    oss << "Thread ID: "  << thread_id
        << "Blocksize: "  << blocksize
        << "Searchsize: " << searchsize
        << "Verbose: "    << boolalpha
        << verbose        << endl;

    log << oss.str().c_str();
  }

  int counter {0};

  vector<uint8_t> pattern {0x49, 0x49, 0x2a, 0x00, 0x10,
                          0x00, 0x00, 0x00, 0x43, 0x52};

  uint8_t* tmp = nullptr;
  disk_pos block_location {};

  if (verbose)
    printf("\n>> Transfer alloc... %'d bytes", searchsize);



  for (;;) {

    if(verbose && block_queue.size() % 1000 == 0){
      lock_guard<mutex> lck(queue_lck);
      oss.str("");
      oss << "Thread #" << thread_id << ": Queue.size(): " <<
            block_queue.size() << ", Block location: " <<block_location << endl;
      log << oss.str().c_str();

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
        //fprintf(log, "\nThread #%d", thread_id);

        for (int c = 0; c < blocksize; ++c) {
            //fprintf(log, "%02x", c);


            if (counter++ % searchsize == 0)
              cout << endl;
          }
        cout << endl << endl;
      }
    }


    for (disk_pos pattern_counter {0}, i {0}; i < blocksize; ++i) {
      if(pattern[pattern_counter] == tmp[i]) {
        if ((pattern_counter+1) == pattern.size()) {
          char preview[16];
          lock_guard<mutex> lock(print_lock);
          memcpy(preview, tmp + (i-pattern_counter), 16);
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
    delete [] tmp;
  }
}





/*=========------------=============
          PRINT ALL OPTIONS
 -------------======================*/
void Cmd_Options::print_all_options(Logger& log) {
  ostringstream oss;

  oss << "\nargc: " << argc
      << "\nargv: " << argv
      << "\nverbose: " << verbose
      << "\nblocksize: " << blocksize
      << "\noffset_start: " << offset_start
      << "\noffset_end: " << offset_end
      << "\nsearchsize: " << searchsize
      << "\noutput_folder: " << output_folder
      << "\ninput: " << input
      << "\noutput_file: " << output_file;
  log << oss.str().c_str();
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



