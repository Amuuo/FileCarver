#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <signal.h>
#include <pthread.h>
#include <queue>
#include <iostream>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <map>
#include <mutex>
#include <string>
#include <queue>
#include <fstream>
#include <sstream>
#include <stdint.h>


using namespace std;

struct Cmd_Options {
    Cmd_Options(int _argc, char** _argv, bool v, int b, long long os, long long oe, int s) :
        argc{_argc}, argv{_argv}, verbose{v}, blocksize{b}, offset_start{os},
        offset_end{oe}, searchsize{s} {}
    int        argc;
    char**     argv;
    bool       verbose;
    int        blocksize;
    long long  offset_start;
    long long  offset_end;
    int        searchsize;
    string     output_folder{""};
    string     input{""};
    string     output_file{""};

    void print_all_options();
};



struct Block_info {
    map<long long, string> info;        
};


struct Arg_struct {
    Cmd_Options cmd;
    Block_info block{};
    char* thread_buff{};
    long long offset{};
    short thread_id{};
    bool ready{};
};

/*
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
*/

mutex queue_lck;
mutex queue_wait_lck;
mutex print_lock;
condition_variable queue_is_empty;
atomic<bool> readingIsDone{false};


void add_header_info(Block_info*, char*, long long, int);    
void open_io_files(ifstream*, ofstream*, Cmd_Options*);
void get_cmd_options(Cmd_Options*);
void print_headers_to_file(Cmd_Options*, Block_info*, ofstream*);
void sig_handler(int signal_number);
void search_disk(int, int, int, bool);


int terminate_early;
int threads_ready;
int threads_go;
int* input_blocks;
int match_count{0};
queue<char*> block_queue{};















/*======================
           MAIN
 ======================*/
int main(int argc, char** argv)
{

    Cmd_Options cmds{argc, argv, false, 32768, 0, 0, 128};
    
    threads_ready = 0;
    get_cmd_options(&cmds);    
    
    setlocale(LC_NUMERIC, "");

    std::thread  thread_arr[8];
    ifstream     input_file;
    ofstream     output_file;
    ofstream     header_file;                                             
    int          thread_blk_sz = cmds.blocksize/8; 
    
    
    //char* buffer = new char[cmds.blocksize];
    char buffer[cmds.blocksize];

    if(cmds.verbose)
        printf("\n>> Buffer alloc... %'lld bytes", cmds.blocksize);        
    
    open_io_files(&input_file, &output_file, &cmds);            
    
    Arg_struct** args = (Arg_struct**)malloc(sizeof(Arg_struct*)*8);        
    
    for(int j = 0; j < 8; ++j)
        args[j] = (Arg_struct*)malloc(sizeof(Arg_struct));    
    
    for (int j = 0; j < 8; ++j) {                  
        if (cmds.verbose)
            printf("\nSpawing thread #%d", j+1);

        thread_arr[j] = thread{&search_disk, j+1, cmds.blocksize/8, cmds.searchsize, cmds.verbose};                            
    }                   

    for (long long i = cmds.offset_start; i < cmds.offset_end; i+=cmds.blocksize ) {
        
        input_file.seekg(i);        
        input_file.read(buffer, cmds.blocksize);
        for(int j = 0; j < cmds.blocksize ; j += cmds.blocksize/8){            
            char* tmp = new char[cmds.blocksize/8];                    
            memcpy(tmp, buffer + j, thread_blk_sz);                                                
            {
                unique_lock<mutex> queue_lock(queue_lck);
                block_queue.push(tmp);                
            }
            queue_is_empty.notify_one();
        }                                
        {
            unique_lock<mutex> queue_lock(queue_lck);
            cout << ">> BLOCKSIZE: " << dec << i << ", QUEUE_SIZE: " << block_queue.size() << endl;                      
        }
    }
        /*
        printf("\nMain thread waiting for children to be ready");
        pthread_mutex_lock(&lock2);
        while((threads_ready % 8 == 0) && (threads_ready != 0)) {
            printf("\nWaiting for children threads");
            pthread_cond_broadcast(&cond1);  
            pthread_cond_wait(&cond2, &lock2);
        }            
        pthread_mutex_unlock(&lock2);          
        printf("\nMAIN THREAD AWAKE!");
        for (j = 0; j < 8; ++j) {            
            memcpy(&args[j]->thread_buff, buffer+(j*thread_blk_sz), 
                   thread_blk_sz);
            args[j]->offset = i;
            args[j]->ready = 1;
        }
        printf("\nFrom Main: Waking children threads");
                      
        if ((i % 10000000) == 0)
                printf("\nBlock %'lld", i);
    
        for(j = 0; j < 8; ++j)
            free(args[j]);
        
        free(args);        
        
        if(cmds.verbose){
            if ((i % 10000000) == 0) printf("\nBlock %'lld", i);
        }
    
    */
    readingIsDone = true;

    //print_headers_to_file(&cmds, &block_info, &header_file);
            
    input_file.close();
    output_file.close();

    printf("\n\nSUCCESS!\n\n");
    
    
    cout << "Match count: " << match_count << endl;
}
















void add_header_info(Block_info* block, char* transfer_info, 
                     long long block_offset, int searchsize) {    
    block->info[block_offset] = string{(char*)transfer_info};
}




void open_io_files(ifstream* input_file, ofstream* output_file, Cmd_Options* cmds) {        

    cout << "\n" << "Trying to open input: " << cmds->input << endl;          
    input_file->open(cmds->input.c_str(), ifstream::binary);
    if (input_file->fail()) {
        fprintf(stderr, "\n>> Input file failed to open");
        exit(1);
    }    
    cout << "\n>> Input file opened: " << cmds->input << endl;        
               
    cmds->output_file += cmds->output_folder;
    cmds->output_file += "output.txt";    
    cout << "\n" << "Trying to open output_file: " << cmds->output_file.c_str() << endl;
    output_file->open(cmds->output_file.c_str(), ofstream::binary);
    if (output_file->fail()) {
            fprintf(stderr, "\n>> Output file failed to open");
            exit(1);
    }    
    cout << "\n>> Output file opened: " << cmds->output_file << endl;    
    
}

void get_cmd_options(Cmd_Options* cmds)
{
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

    extern char *optarg;
    //extern int optind;  
    int cmd_option;


    while ((cmd_option = getopt_long(cmds->argc, cmds->argv,
                                     "b:S:E:s:i:o:vh", long_options, NULL)) != -1) {
    
        switch (cmd_option) 
        {
            
        case 'b':
            cmds->blocksize = atoi(optarg);                
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
            cmds->verbose = 1;
            break;

        case 'h':
            printf("\n\t--blocksize     -b   <size of logical blocks in bytes>");
            printf("\n\t--offset_start  -S  <offset from start of disk in bytes>");
            printf("\n\t--offset_end    -E  <offset from start of disk in bytes>");
            printf("\n\t--searchsize    -s   <size of chunk to search for input disk in bytes>");
            printf("\n\t--input         -i   <file to use for header search>");
            printf("\n\t--output_folder -o   <folder to use for output files>");
            printf("\n\t--verbose       -v   <display verbose output of ongoing process>");
            printf("\n\t--help          -h   <print help options>\n\n");
        
            exit(0);
            
            break;
        
        
        default: 
            break;
        
        }
    }

    if(cmds->output_folder == "") {
        
        char tmp_output_folder[50];
        

        if(!(getcwd(tmp_output_folder, 50)))
            printf("\nCould not retrieve directory with 'getcwd'");
        
        cmds->output_folder = string{tmp_output_folder};
        
        if(cmds->verbose) {
            cmds->output_folder += '/';
            printf("\n>> Setting current directory for output: %s", 
                   cmds->output_folder.c_str());
        }
    }

    if(cmds->offset_end == 0) {
        printf("\n>> No offset end detected, setting offset to end of file");
        FILE* file_size_stream = fopen(cmds->input.c_str(), "r");
        fseek(file_size_stream, 0, SEEK_END);
        cmds->offset_end = ftell(file_size_stream);
        fclose(file_size_stream);        
    }

    long long filesize = cmds->offset_end - cmds->offset_start;
    if (filesize < cmds->blocksize) 
        cmds->blocksize = filesize;
    
    if(cmds->verbose) {
                
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


void sig_handler(int signal_number) {

    //terminate_early = 1;

}








void search_disk(int thread_id, int blocksize, int searchsize, bool verbose) {
    
    if (verbose) {
        lock_guard<mutex> print_lck(print_lock);
        cout << endl;
        cout << "Thread ID: " << thread_id << endl;
        cout << "Blocksize: " << blocksize << endl;
        cout << "Searchsize: " << searchsize << endl;
        cout << "Verbose: " << boolalpha << verbose << endl;
        cout << endl;        
    }
    
    regex_t regex;             
    char* tmp;
    char transfer[searchsize];
    if (verbose) {
        printf("\n>> Transfer alloc... %'d bytes", searchsize);
    }    
    int regex_i = regcomp(&regex, "^II.{6}CR", REG_EXTENDED); 
    
    
    for(;;){        
        if(!readingIsDone) {
            unique_lock<mutex> lck(queue_wait_lck);        
            queue_is_empty.wait(lck, []{ return !block_queue.empty(); });        
        }
        unique_lock<mutex> queue_lock(queue_lck);
        tmp = block_queue.front();            
        //delete [] block_queue.front();
        block_queue.pop();             
        queue_lock.unlock();                    
        
        for (int j = 0; j < blocksize; j+=searchsize) {              
            
            memcpy(transfer, tmp + j, searchsize);
            
            if(verbose){
                cout << "Thread #" << thread_id << ", j = " << j << endl;
                cout << "\n>> Thread #" << thread_id 
                    << " searching for match" << endl;
            }
            ostringstream oss{};
            
            {
                unique_lock<mutex> queue_lock(queue_lck);
                cout << "Thread #" << thread_id << ": ";
                for(int i = 0; i < searchsize; ++i){
                    oss << hex << (int)transfer[i];      
                    cout << dec << oss.str();
                }                             
                cout << endl;     
            }
            
            if(verbose) {                                                               
                cout << "\nSize of transfer: " << dec << sizeof(transfer) << endl;
            }
            
            regex_i = regexec(&regex, oss.str().c_str(), 0, NULL, 0);
            if(!regex_i) {                
                cout << "\nMATCH!" << endl;                                                
                match_count++;                    
                //add_header_info(&args->block, transfer, args->offset, args->cmd.searchsize);
            }              
        }   
    if (readingIsDone && block_queue.empty()) exit(1);     
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