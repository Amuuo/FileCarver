#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
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

using namespace std;

struct Cmd_Options {
  Cmd_Options(int _argc, char** _argv, bool v, int b, unsigned long long os,
              unsigned long long oe, int s)
      : argc{_argc},
        argv{_argv},
        verbose{v},
        blocksize{b},
        offset_start{os},
        offset_end{oe},
        searchsize{s} {}
  int argc;
  char** argv;
  bool verbose;
  unsigned long long blocksize;
  unsigned long long offset_start;
  unsigned long long offset_end;
  int searchsize;
  string output_folder{""};
  string input{""};
  string output_file{""};

  void print_all_options();
};



void open_io_files(ifstream&, ofstream&, Cmd_Options&);
void get_cmd_options(Cmd_Options*);
void sig_handler(int signal_number);
void search_disk(int, int, int, bool);
void print_results(Cmd_Options&);
void print_hexdump(int, vector<uint8_t>&);

/*=========------------=============
           GLOBAL VARIABLES
 -------------======================*/
mutex queue_lck{};
mutex queue_wait_lck{};
mutex print_lock{};
mutex main_finished_lock{};
mutex block_match_mtx{};
condition_variable queue_is_empty{};
condition_variable children_are_running{};
atomic<bool> readingIsDone{false};
int terminate_early;
int threads_ready;
int threads_go;
int* input_blocks;
int match_count{0};
deque<pair<vector<uint8_t>, unsigned long long>> block_queue{};
map<unsigned long long, uint8_t*> block_matches{};



/*======================
           MAIN
 ======================*/
int main(int argc, char** argv) {

  Cmd_Options cmds{argc, argv, false, 32768, 0, 0, 128};
  threads_ready = 0;
  get_cmd_options(&cmds);

  setlocale(LC_NUMERIC, "");
  array<std::thread, 8> thread_arr;
  ifstream input_file;
  ofstream output_file;
  ofstream header_file;
  int thread_blk_sz = cmds.blocksize / 8;


  open_io_files(input_file, output_file, cmds);
  vector<uint8_t> buffer(cmds.blocksize);

  // launch search threads
  for (int j = 0; j < 8; ++j) {
    if (cmds.verbose) cout << "\nSpawing thread #" << j + 1 << endl;

    thread_arr[j] = thread{&search_disk, j + 1, thread_blk_sz,
                           cmds.searchsize, cmds.verbose};
  }

  int intput;
  cin >> intput;
  system("clear");
  // read disk and push data chunks into block_queue
  for (unsigned long long i = cmds.offset_start; i < cmds.offset_end; i += cmds.blocksize) {

    if(!cmds.verbose && i%100000 < 100)
      cout << "[H" << "MAIN:\nBlock: " << i;

    input_file.seekg(i);
    //ifstream test_file{"program_results1551322980"};
    for(int file_pos = 0; file_pos < cmds.blocksize; ++file_pos){
      buffer[file_pos] = input_file.get();
    }

    //buffer.assign(istream_iterator<uint8_t>(input_file), istream_iterator<uint8_t>());
    int counter = 0;
    {
      lock_guard<mutex> lock(queue_lck);

      if(cmds.verbose){
        cout << endl << endl << "MAIN THREAD: " << endl;
        for(auto& c : buffer){
          cout << setw(2) << setfill('0') << hex << static_cast<unsigned short>(c);
          if(counter++ % cmds.searchsize == 0){
            cout << endl;
          }
        }
        cout << endl << endl;
      }

    }
    for (unsigned long long j = 0; j < cmds.blocksize; j += thread_blk_sz) {
      {
        vector<uint8_t> transfer(buffer.begin() + j, buffer.begin() + (j+thread_blk_sz));
        lock_guard<mutex> q_lock(queue_lck);

        block_queue.push_back(pair<vector<uint8_t>, unsigned long long>{transfer, i+j});
      }
			queue_is_empty.notify_one();
    }
		if (cmds.verbose && (i % 1000000) < 100 ){
			lock_guard<mutex> q_lock(queue_lck);
			cout << "Block " << i << ", Queue.size(): " << block_queue.size() << endl;
		}
  }

  // set finished state and wait for threads to finish search
  readingIsDone = true;
  unique_lock<mutex> lck(main_finished_lock);
  while(!block_queue.empty())
    children_are_running.wait_for(lck, 3000ms, []{return block_queue.empty();});

  input_file.close();
  output_file.close();

  cout << "Queue.size(): " << block_queue.size() << endl;


  //print results to console
  printf("\n\nSUCCESS!\n\n");
  cout << "Match count: " << block_matches.size() << endl;
	for (auto& match : block_matches){
		cout << match.first << ": " << match.second << endl;
	}

  print_results(cmds);
}








/*=========------------=============
            OPEN IO FILES
 -------------======================*/
void open_io_files(ifstream& input_file, ofstream& output_file,
                   Cmd_Options& cmds) {
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
void get_cmd_options(Cmd_Options* cmds) {
  setlocale(LC_NUMERIC, "");

  static struct option long_options[] = {
      {"blocksize", required_argument, NULL, 'b'},
      {"offset_start", required_argument, NULL, 'S'},
      {"offset_end", required_argument, NULL, 'E'},
      {"searchsize", required_argument, NULL, 's'},
      {"input", required_argument, NULL, 'i'},
      {"output_folder", required_argument, NULL, 'o'},
      {"searchsize", required_argument, NULL, 's'},
      {"verbose", no_argument, NULL, 'v'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};

  extern char* optarg;
  // extern int optind;
  int cmd_option;

  while ((cmd_option = getopt_long(cmds->argc, cmds->argv, "b:S:E:s:i:o:vh",
                                   long_options, NULL)) != -1) {
    switch (cmd_option) {
      case 'b':
        cmds->blocksize = atoll(optarg);
        break;

      case 'S':
        cmds->offset_start = atoll(optarg);
        break;

      case 'E':
        cmds->offset_end = atoll(optarg);
        break;

      case 's':
        cmds->searchsize = atoi(optarg);
        break;

      case 'i':
        cmds->input = string{optarg};
        break;

      case 'o':
        cmds->output_folder = string{optarg};
        break;

      case 'v':
        cmds->verbose = true;
        break;

      case 'h':
        printf("\n\t--blocksize    -b   <size of logical blocks in bytes>");
        printf("\n\t--offset_start -S  <offset from start of disk in bytes>");
        printf("\n\t--offset_end   -E  <offset from start of disk in bytes>");
        printf("\n\t--searchsize  -s  <size of chunk to search for input disk in bytes>");
        printf("\n\t--input    -i   <file to use for header search>");
        printf("\n\t--output_folder -o   <folder to use for output files>");
        printf("\n\t--verbose  -v   <display verbose output of ongoing process>");
        printf("\n\t--help     -h   <print help options>\n\n");

        exit(0);

        break;

      default:
        break;
    }
  }

  if (cmds->output_folder == "") {
    char tmp_output_folder[50];

    if (!(getcwd(tmp_output_folder, 50)))
      printf("\nCould not retrieve directory with 'getcwd'");

    cmds->output_folder = string{tmp_output_folder};

    if (cmds->verbose) {
      cmds->output_folder += '/';
      printf("\n>> Setting current directory for output: %s",
             cmds->output_folder.c_str());
    }
  }

  if (cmds->offset_end == 0) {
    printf("\n>> No offset end detected, setting offset to end of file");
    FILE* file_size_stream = fopen(cmds->input.c_str(), "r");
    fseek(file_size_stream, 0, SEEK_END);
    cmds->offset_end = ftell(file_size_stream);
    fclose(file_size_stream);
  }

  long long filesize = cmds->offset_end - cmds->offset_start;
  if (filesize < cmds->blocksize) cmds->blocksize = filesize;

  if (cmds->verbose) {
    printf("\n>> Offset Start: %'lld bytes", cmds->offset_start);
    printf("\n>> Offset End: %'lld bytes", cmds->offset_end);
    printf("\n>> Input file: %s", cmds->input.c_str());
    printf("\n>> Input file size: %'lld bytes", filesize);
    printf("\n>> Blocksize: %'lld bytes", cmds->blocksize);
    printf("\n>> Search Size: %'d bytes", cmds->searchsize);
    printf("\n>> Output folder: %s", cmds->output_folder.c_str());
  }

  if (cmds->input == "") {
    printf("\n>> Input file required");
    exit(1);
  }
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
void search_disk(int thread_id, int blocksize, int searchsize, bool verbose) {


  if (verbose) {

    lock_guard<mutex> print_lck(queue_lck);
    cout << endl;
    cout << "Thread ID: " << thread_id << endl;
    cout << "Blocksize: " << blocksize << endl;
    cout << "Searchsize: " << searchsize << endl;
    cout << "Verbose: " << boolalpha << verbose << endl;
    cout << endl;
  }

  int counter = 0;
  regex header_regex("49492a00100000004352", regex_constants::ECMAScript);
  vector<uint8_t> pattern{0x49, 0x49, 0x2a, 0x00, 0x10,
                          0x00, 0x00, 0x00, 0x43, 0x52};
  // uint8_t tmp[blocksize];
  vector<uint8_t> tmp(blocksize);
  long long block_location;

  if (verbose)
    printf("\n>> Transfer alloc... %'d bytes", searchsize);



  for (;;) {

    if(verbose && block_queue.size() % 1000 == 0){
      cout << "Thread #" << thread_id << ": Queue.size(): " <<
            block_queue.size() << ", Block location: " <<block_location << endl;
    }

  	if (readingIsDone && block_queue.empty()) {
			return;
		}
    else{
      {
        unique_lock<mutex> lck(queue_wait_lck);
        queue_is_empty.wait(lck, [] { return !block_queue.empty(); });
        lock_guard<mutex> q_lck(queue_lck);
        tmp = block_queue.front().first;
        //memcpy(tmp, block_queue.front().first, blocksize);
        block_location = block_queue.front().second;
        block_queue.pop_front();
      }
      if(verbose) {
        cout << endl << endl;
        cout << "Thread #" << thread_id << ": " << endl;
        for(auto& c : tmp){
          cout << setw(2) << setfill('0') << hex << static_cast<unsigned short>(c);
          if(counter++ % searchsize == 0){
            cout << endl;
          }
        }
        cout << endl << endl;
      }
    }


    for (int pattern_counter = 0, i = 0; i < blocksize; ++i) {

      if(pattern[pattern_counter] == tmp[i]) {

        if ((pattern_counter+1) == pattern.size()) {

          block_matches[block_location + (i - pattern_counter)] =
              (uint8_t*)malloc(searchsize);
          memcpy(&block_matches[block_location+(i-pattern_counter)],
            &tmp[i], searchsize);

          {
            lock_guard<mutex> lock(block_match_mtx);
            cout << "[H" << endl;

            for (int j = 0; j < block_matches.size(); ++j)
              cout << endl;

            cout << block_location + (i - pattern_counter);
          }
          pattern_counter = 0;
        }
        else
          pattern_counter++;
      }
      else
        pattern_counter = 0;
    }
    /*
    for (long long j = 0; j < blocksize; j += searchsize) {
      // memcpy(transfer, &tmp[0] + j, searchsize);
      ostringstream oss{};
      if (verbose){
        lock_guard<mutex> lock(queue_lck);
        cout << "\n>> Thread #" << thread_id << " searching for match" << endl;
      }

      //unique_lock<mutex> queue_lock(queue_lck);
      for (int i = 0; i < searchsize; ++i)
        oss << setw(2) << setfill('0') << hex << unsigned(tmp[j + i]);

      if (regex_search(oss.str(), header_regex)) {
        lock_guard<mutex> guard(queue_lck);

        block_matches[block_location + j] = oss.str();
        for (int count = 0; count < match_count+1; ++count)
          cout << endl;

        cout << "Thread #" << thread_id << " MATCH!" << endl;
        // cout << oss.str() << endl;
        match_count++;
      }
    }
    */
    // delete [] tmp;
  }
}






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




void print_results(Cmd_Options& cmds){

  // print results to output file
	time_t current_time;
	current_time = time(NULL);
	ofstream program_results_file{string{"program_results" + to_string(current_time)}};
	program_results_file << "offset start: " << cmds.offset_start << endl;
	program_results_file << "offset end: " << cmds.offset_end << endl;
	program_results_file << "searchsize: " << cmds.searchsize << endl;
	program_results_file << "blocksize: " << cmds.blocksize << endl << endl;

	for(auto& match : block_matches){
		program_results_file << match.first << ": " << match.second << endl;
	}

}






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

}