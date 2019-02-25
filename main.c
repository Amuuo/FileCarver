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


typedef __uint8_t byte;


typedef struct Cmd_Options {
    int        argc;
    char**     argv;
    int        verbose;
    int        blocksize;
    long long  offset_start;
    long long  offset_end;
    int        searchsize;
    char*      output_folder;
    char*      input;
    char*      output_file;
} Cmd_Options;



typedef struct Block_info {
    long long* headers;
    byte** info;        
    int header_count;
} Block_info;


typedef struct Arg_struct {
    Cmd_Options cmd;
    Block_info block;
    byte* thread_buff;
    long long offset;
    short thread_id;
    short ready;
} Arg_struct;


pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;


void resize_block_info(Block_info*, int, Cmd_Options*);
void init_block_memory(Block_info*, int);
void add_header_info(Block_info*, byte*, long long, int);    
void open_io_files(FILE**, FILE**, Cmd_Options*);
void get_cmd_options(Cmd_Options*);
void print_headers_to_file(Cmd_Options*, Block_info*, FILE**);
void sig_handler(int signal_number);
void *search_disk(void*);


int terminate_early;
int threads_ready;
int threads_go;
int* input_blocks;




/*======================
           MAIN
 ======================*/
int main(int argc, char** argv)
{
    
    //terminate_early = 0;
    //signal(SIGINT, sig_handler);
    
    Cmd_Options cmds = {argc, argv, 0, 32768, 0, 0, 128, NULL, NULL, NULL};
    
    threads_ready = 0;
    get_cmd_options(&cmds);    
    setlocale(LC_NUMERIC, "");

    pthread_t    thread_arr[8];
    FILE*        input_file;
    FILE*        output_file;
    FILE*        header_file;                      
    int          regex_i;            
    //const char*  regex_str = "^II.{6}CR";        
    long long    i;
    int          j;
    int          thread_blk_sz = cmds.blocksize/8; 
    threads_go = 0;

    byte* buffer = (byte*)malloc(cmds.blocksize);
    byte** thread_buff = (byte**)malloc((thread_blk_sz)*sizeof(byte*));
    for (i = 0; i < 8; ++i) {
        thread_buff[i] = (byte*)malloc(thread_blk_sz);
    }
    if(cmds.verbose){
        printf("\n>> Buffer alloc... %'d bytes", cmds.blocksize);
    }    
    cmds.output_file = (char*)malloc(50);
    open_io_files(&input_file, &output_file, &cmds);    
        
    Arg_struct** args = (Arg_struct**)malloc(sizeof(Arg_struct*)*8);        
    
    for(j = 0; j < 8; ++j){
        args[j] = (Arg_struct*)malloc(sizeof(Arg_struct));
    }
    
    for (j = 0; j < 8; ++j) {  
        args[j]->block;
        memcpy(&args[j]->cmd, &cmds, sizeof(cmds));
        args[j]->cmd.blocksize = args[j]->cmd.blocksize/8;                       
        args[j]->thread_buff = (byte*)malloc(cmds.blocksize);     
        args[j]->thread_id = j+1;
        args[j]->offset = 1;
        args[j]->ready = 0;
        if (cmds.verbose)
            printf("\nSpawing thread #%d", j+1);
                    
        pthread_create(&thread_arr[j], NULL, &search_disk, (void*)&args[j]);
        pthread_detach(thread_arr[j]);
    }           

    for (i = cmds.offset_start; i < cmds.offset_end; i+=cmds.blocksize ) {
        
        threads_go = 0;
        
        fseek(input_file, i, 0);        
        
        if ((fread(buffer, 1, cmds.blocksize, input_file)) < 0) {            
            fprintf(stderr, "fread failed..");
            exit(1);
        }      
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
            memcpy(&args[j]->thread_buff, buffer+(j*thread_blk_sz), thread_blk_sz);
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
    }

    //print_headers_to_file(&cmds, &block_info, &header_file);
            
    fclose(input_file);
    fclose(output_file);
    free(buffer);
    printf("\n\nSUCCESS!\n\n");
}





void resize_block_info(Block_info* block, int searchsize, Cmd_Options* cmds) {    
    int count = block->header_count + 1;        
    
    if (cmds->verbose) {
        printf("\nblock_info.headers realloc: %'d", 8*count);
        printf("\nblock_info.info realloc: %'d", count*8);    
        printf("\nblock_info.info[%d] malloc: %'d", count-1, searchsize);    
        fflush(stdout);
    }
    
    block->headers = (long long*)realloc(block->headers, 8*count);                            
    
    block->info = (byte**)realloc(block->info, 8*count);    
    
    block->info[count-1] = (byte*)malloc(searchsize);
    
        
}






void add_header_info(Block_info* block, byte* transfer_info, 
                     long long block_offset, int searchsize) {        
    memcpy(block->info[block->header_count], transfer_info, searchsize);          
    block->headers[block->header_count++] = block_offset;
}






void init_block_memory(Block_info* block, int searchsize) {
    block->header_count = 0;        
    block->headers = (long long*)malloc(8);                            
    block->info = (byte**)malloc(8);
    block->info[0] = (byte*)malloc(searchsize);           

}




void open_io_files(FILE** input_file, FILE** output_file, Cmd_Options* cmds) {        
                
    if (!(*input_file = fopen(cmds->input, "rb"))) {
        fprintf(stderr, "\n>> Input file failed to open");
        exit(1);
    }    
    printf("\n>> Input file opened: %s", cmds->input);        
               
    
    strcat(cmds->output_file, cmds->output_folder);
    strcat(cmds->output_file, "output.txt");    
    if (!(*output_file = fopen(cmds->output_file, "wb+"))) {
            fprintf(stderr, "\n>> Output file failed to open");
            exit(1);
    }    
    printf("\n>> Output file opened: %s", cmds->output_file);
    fflush(stdout);
    
}





void print_headers_to_file(Cmd_Options* cmds, Block_info* block, FILE** header_file) {
    
    char file_str[50];
    byte line[16];
    int i, j, k;    
    
    strcat(file_str, cmds->output_folder);
    strcat(file_str, "headers.txt");
    printf("\nHeader File: %s", file_str);    

    
    if (!(*header_file = fopen(file_str, "w+"))) {
        fprintf(stderr, "\n>> Output file failed to open");
        exit(1);
    }    
    
    if(cmds->verbose)
        printf("\n>> Header file opened: %s", file_str);                 
               
    for (i = 0; i < block->header_count-1; ++i) {
        
        for (j = 0; j < cmds->searchsize; j+=16) {                        
            memcpy(line, (block->info[i])+j, 16);            
            fprintf(*header_file, "\n%lld  ", block->headers[i]);
            
            for( k = 0; k < 16; ++k) {
                fprintf(*header_file, "%02x ", line[k]);
                if (k == 7)
                    fprintf(*header_file, " ");                
            }            
            fprintf(*header_file, " |");
            for( k = 0; k < 16; ++k) {                
                fprintf(*header_file, "%c", 
                         (line[k] > 33 && line[k] < 127) ? line[k] : '.');                
            }
            fprintf(*header_file, "|");
        }
        fprintf(*header_file, "\n\n");
    }    
    fclose(*header_file);
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
    extern int optind;  
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
            cmds->input = (char*)malloc(50);                
            strcpy(cmds->input, optarg);
            break;

        case 'o':
            cmds->output_folder = (char*)malloc(50);                       
            strcpy(cmds->output_folder, optarg);
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

    if(!cmds->output_folder) {
        
        cmds->output_folder = (char*)malloc(sizeof(char)*50);
        memset(cmds->output_folder, 0, 50);  
        
        if(!(getcwd(cmds->output_folder, 50))){
            printf("\nCould not retrieve directory with 'getcwd'");
        }
        if(cmds->verbose) {
            strcat(cmds->output_folder, "/");
            printf("\n>> Setting current directory for output: %s", 
                   cmds->output_folder);
        }
    }

    if(cmds->offset_end == 0) {
        FILE* file_size_stream = fopen(cmds->input, "r");
        fseek(file_size_stream, 0, SEEK_END);
        cmds->offset_end = ftell(file_size_stream);
        fclose(file_size_stream);        
    }

    long long filesize = cmds->offset_end - cmds->offset_start;
    if (filesize < cmds->blocksize) 
        cmds->blocksize = filesize;
    
    if(cmds->verbose) {
                
        printf("\n>> Offset Start: %'lld bytes", cmds->offset_start);
        printf("\n>> No offset end detected, setting offset to end of file");
        printf("\n>> Offset End: %'lld bytes", cmds->offset_end);
        printf("\n>> Input file: %s", cmds->input);                 
        printf("\n>> Input file size: %'lld bytes", filesize);
        printf("\n>> Blocksize: %'d bytes", cmds->blocksize);        
        printf("\n>> Search Size: %'d bytes", cmds->searchsize);
        printf("\n>> Output folder: %s", cmds->output_folder);
    }

    
    if (!cmds->input) {
        printf("\n>> Input file required");
        exit(1);
    }
}



void sig_handler(int signal_number) {

    //terminate_early = 1;

}

void *search_disk(void* arguments) {

    int j;
    regex_t  regex;
    //pthread_detach(pthread_self());
    Arg_struct* args = *((Arg_struct**)arguments);
    //printf("\nthread buffer size: %d", args.cmd.blocksize);
    init_block_memory(&args->block, args->cmd.searchsize);
    byte transfer[args->cmd.searchsize];
    
    int regex_i = regcomp(&regex, "II.{6}CR", REG_EXTENDED); 

    if (args->cmd.verbose) {
        printf("\n>> Transfer alloc... %'d bytes", args->cmd.searchsize);
    }
    fprintf(stdout, "\nFrom thread %d, Threads ready: %d", args->thread_id, threads_ready); 
    fprintf(stdout, "\nThread %d signaling Main thread", args->thread_id);
    fprintf(stdout, "\nThread %d offset: %d", args->thread_id, args->offset);
    fflush(stdout);
    /*sleep(1);
    threads_ready++;
    if(threads_ready % 8 == 0) {
        pthread_cond_signal(&cond2);                                
    }                         
    pthread_mutex_unlock(&lock);  
    pthread_mutex_lock(&lock);  
    */
    pthread_mutex_lock(&lock);
    while(!input_blocks) {
        printf("\nThread #%d waiting, offset: %d", args->thread_id, args->offset);
        pthread_cond_wait(&cond1, &lock);
    }
    pthread_mutex_unlock(&lock);          
    printf("\nThread #%d AWAKE", args->thread_id);
    args->ready = 0;
    
     
    for(;;){

        for (j = 0; j < args->cmd.blocksize; j+=args->cmd.searchsize) {  
            memcpy(transfer, args->thread_buff+j, args->cmd.searchsize);
            regex_i = regexec(&regex, (const char*)transfer, 0, NULL, 0);
            printf("\n>> Thread #%d searching for match", args->thread_id);
            if(!regex_i) {
                printf("\nMATCH!");            
                printf("\n>> Thread #%d adding headers to table", args->thread_id);
                resize_block_info(&args->block, args->cmd.searchsize, &args->cmd);
                add_header_info(&args->block, transfer, args->offset, args->cmd.searchsize);
            }        
        }  
    }
}
