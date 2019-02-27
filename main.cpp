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

using namespace std;

struct Cmd_Options {
  Cmd_Options(int _argc, char** _argv, bool v, int b, long long os,
              long long oe, int s)
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
  long long blocksize;
  long long offset_start;
  long long offset_end;
  int searchsize;
  string output_folder{""};
  string input{""};
  string output_file{""};

  void print_all_options();
};

struct Block_info {
  map<long long, string> info;
};

struct Arg_struct {
  Cmd_Options cmd;
  Block_info block{};
  uint8_t* thread_buff{};
  long long offset{};
  short thread_id{};
  bool ready{};
};


void add_header_info(Block_info*, uint8_t*, long long, int);
void open_io_files(ifstream&, ofstream&, Cmd_Options&);
void get_cmd_options(Cmd_Options*);
void print_headers_to_file(Cmd_Options*, Block_info*, ofstream*);
void sig_handler(int signal_number);
void search_disk(int, int, int, bool);



/*=========------------=============
           GLOBAL VARIABLES
 -------------======================*/
mutex queue_lck;
mutex queue_wait_lck;
mutex print_lock;
mutex main_finished_lock;
condition_variable queue_is_empty;
condition_variable children_are_running;
atomic<bool> readingIsDone{false};
int terminate_early;
int threads_ready;
int threads_go;
int* input_blocks;
int match_count{0};
deque<uint8_t*> block_queue{};
vector<Block_info> matches_vec{};



/*======================
           MAIN
 ======================*/
int main(int argc, char** argv) {
  Cmd_Options cmds{argc, argv, false, 32768, 0, 0, 128};
	cout.setf(ios::unitbuf);
  threads_ready = 0;
  get_cmd_options(&cmds);

  setlocale(LC_NUMERIC, "");

  array<std::thread, 8> thread_arr;
  ifstream input_file;
  ofstream output_file;
  ofstream header_file;
  int thread_blk_sz = cmds.blocksize / 8;

  // uint8_t* buffer = new uint8_t[cmds.blocksize];
  uint8_t* buffer = new uint8_t[cmds.blocksize];
  if (cmds.verbose)
    cout << "\n>> Buffer alloc..." << cmds.blocksize << "bytes" << endl;

  open_io_files(input_file, output_file, cmds);

  for (int j = 0; j < 8; ++j) {
    if (cmds.verbose) cout << "\nSpawing thread #" << j + 1 << endl;

    thread_arr[j] = thread{&search_disk, j + 1, cmds.blocksize / 8,
                           cmds.searchsize, cmds.verbose};
  }

  for (long long i = cmds.offset_start; i < cmds.offset_end;
       i += cmds.blocksize) {
    input_file.seekg(i);
    input_file.read((char*)buffer, cmds.blocksize);
    for (long j = 0; j < cmds.blocksize; j += thread_blk_sz) {
      {
        
				uint8_t transfer[thread_blk_sz];
				memcpy(transfer, &buffer[j], thread_blk_sz);
				//unique_lock<mutex> queue_wait(queue_wait_lck);
        lock_guard<mutex> q_lock(queue_lck);
        
        block_queue.push_back(transfer);
      }
      queue_is_empty.notify_one();
    }
		if ((i % 1000000) < 3000 )
				cout << "Block " << i << endl;
  }
  unique_lock<mutex> lck(main_finished_lock);
  children_are_running.wait_for(lck, 1000ms, []{return block_queue.empty();});
  readingIsDone = true;
  input_file.close();
  output_file.close();
  //for (auto& thread : thread_arr) thread.join();
  printf("\n\nSUCCESS!\n\n");

  cout << "Match count: " << match_count << endl;
}










/*=========------------=============
           ADD HEADER INFO
 -------------======================*/
void add_header_info(Block_info* block, uint8_t* transfer_info,
                     long long block_offset, int searchsize) {
  block->info[block_offset] = string{(char*)transfer_info};
}








/*=========------------=============
            OPEN IO FILES
 -------------======================*/
void open_io_files(ifstream& input_file, ofstream& output_file,
                   Cmd_Options& cmds) {
  cout << "\n"
       << "Trying to open input: " << cmds.input << endl;
  input_file.open(cmds.input.c_str(), ios_base::in | ios_base::binary);
  if (input_file.fail()) {
    fprintf(stderr, "\n>> Input file failed to open");
    exit(1);
  }
  cout << "\n>> Input file opened: " << cmds.input << endl;

  cmds.output_file += cmds.output_folder;
  cmds.output_file += "output.txt";
  cout << "\n"
       << "Trying to open output_file: " << cmds.output_file.c_str() << endl;
  output_file.open(cmds.output_file.c_str(), ios_base::in | ios_base::binary);
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

  regex header_regex("II\\*\\.\\.\\.\\.\\.CR", regex::extended);
  // vector<uint8_t> tmp{};
  uint8_t tmp[blocksize];
  // vector<uint8_t> transfer{};
  // uint8_t transfer[searchsize];
  if (verbose) {
    printf("\n>> Transfer alloc... %'d bytes", searchsize);
  }
	cout.setf(ios::unitbuf);
  for (;;) {
		if (readingIsDone && block_queue.empty()) {
				return;
			}  
			{
      unique_lock<mutex> lck(queue_wait_lck);
      queue_is_empty.wait(lck, [] { return !block_queue.empty() && !readingIsDone; });
      lock_guard<mutex> q_lck(queue_lck);
      memcpy(tmp, block_queue.front(), blocksize);
      block_queue.pop_front();
    }

    for (int j = 0; j < blocksize; j += searchsize) {
      // memcpy(transfer, &tmp[0] + j, searchsize);

      if (verbose) {        
        cout << "\n>> Thread #" << thread_id << " searching for match" << endl;
      }
      ostringstream oss{};
      
	  	{
        //unique_lock<mutex> queue_lock(queue_lck);        
        for (int i = 0; i < searchsize; ++i) {
          oss << (tmp[j + i] > 33 && tmp[j + i] < 127 ? (char)tmp[j + i] : '.');
				if(verbose)
		      cout << setw(2) << setfill('0') << hex << (int)tmp[j + i] << " ";
          // printf("%02x ", (int)tmp[j+i]);
        }
		  }
		  if(verbose){
        cout << "  |";
        for (int i = 0; i < searchsize; ++i) {
          cout << (tmp[j + i] > 33 && tmp[j + i] < 127 ? (char)tmp[j + i] : '.');
        }
        cout << "|";
        cout << endl;
      }
	

      if (regex_search(oss.str(), header_regex)) {
        lock_guard<mutex> guard(queue_lck);

        cout << "Thread #" << thread_id << " MATCH!" << endl;
				cout << oss.str() << endl;
        match_count++;
      }
    }
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