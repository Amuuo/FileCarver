#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <getopt.h>


typedef struct Block_info {
    long long* headers;
    __uint8_t** info;        
    int header_count;
} Block_info;


typedef struct Cmd_Options {
    int argc;
    char** argv;
    int verbose;
    int blocksize;
    int offset_start;
    int offset_end;
    int searchsize;
    char output_folder[30];
    char input[30];
} Cmd_Options;


void resize_block_info(Block_info* block);
void init_block_memory(Block_info* block);
void add_header_info(Block_info*, __uint8_t*, long long);    
void open_io_files(FILE*, FILE*, FILE*);
void get_cmd_options();



/*======================
           MAIN
 ======================*/
int main(int argc, char** argv)
{

    Block_info block_info;
    Cmd_Options cmd_options = {argc, argv, 0, 32768, 0, 0, 128, "", ""};

    regex_t regex;
    FILE* input_file;
    FILE* output_file;
    FILE* header_file;
    long int offset = 116267458560;
    int filesize;        
    //const char* regex_str = "\\x49\\x49\\x2a\\x00\\x10\\x00\\x00\\x00\\x43\\x52";
    const char* regex_str = "II......CR";
    int regex_i;
    __uint8_t transfer[128];
        

    get_cmd_options(&cmd_options);


    regex_i = regcomp(&regex, regex_str, REG_EXTENDED);
    printf("\n>> Buffer alloc...");
    __uint8_t* buffer = malloc(32768);
    memset(buffer, 0, 32768);
    printf("\n>> Opening input file");
    fflush(stdout);
        
    open_io_files(input_file, output_file, header_file);
    long long i;
    int j;
    for (i = cmd_options.offset_start; i < cmd_options.offset_end; i+=32768 ) {
        
        fseek(input_file, offset+i, 0);
        //printf("\n>> Needle moved to %ld", offset+i);
        //fflush(stdout);

        if ((fread(buffer, 1, 32768, input_file) < 0)) {
            fprintf(stderr, "fread failed..");
            exit(1);
        }    
        
        if (i % 1000000 == 0)
            printf("\nBlock %lld", i);
          
        for (j = 0; j < 32768; j+=128){
            memcpy(transfer, &buffer[j] , 128);
            regex_i = regexec(&regex, transfer, 0, NULL, REG_EXTENDED);
            if(!regex_i) {
                printf("\nMATCH!");
                fflush(stdout);
                resize_block_info(&block_info);
                add_header_info(&block_info, transfer, i);
            }
        }
        //fwrite(buffer, 1, 4096, output_file);
        //printf("\n>> Buffer written to output_file");
        memset(buffer, 0, 32768);
    }
    printf("\nHEADER COUNT: %d", block_info.header_count);
    for (i = 0; i < block_info.header_count; ++i ){
        printf("\n%d: %lld", i, block_info.headers[i]);
        printf("\n%s", block_info.info[i]);
    }
    
    fclose(input_file);
    printf("\n>> Input file closed");
    fflush(stdout);
    
    fclose(output_file);
    fclose(header_file);
    free(buffer);

}



void resize_block_info(Block_info* block) 
{
    int count = block->header_count + 1;            
    block->headers = 
        (long long*)realloc(block->headers, sizeof(long long)*count);                            
    
    block->info = 
        (__uint8_t**)realloc(block->info, sizeof(__uint8_t*)*count);    
    
    block->info[count-1] = (__uint8_t*)malloc(sizeof(__uint8_t)*128);
    
    printf("\nblock_info.headers realloc: %ld", sizeof(long long)*count);
    printf("\nblock_info.info realloc: %ld", sizeof(__uint8_t*)*count);    
    printf("\nblock_info.info[%d] malloc: %ld", count-1, sizeof(__uint8_t)*128);    
    
    fflush(stdout);                
}



void init_block_memory(Block_info* block) 
{
    block->header_count = 0;    
    block->headers = (long long*)malloc(sizeof(long long));                            
    block->info = (__uint8_t**)malloc(sizeof(__uint8_t*));
    block->info[0] = (__uint8_t*)malloc(sizeof(__uint8_t)*128);
}




void add_header_info(Block_info* block, __uint8_t* transfer_info, long long block_offset) 
{    
    memcpy(block->info[block->header_count], transfer_info, 128);      
    block->headers[block->header_count++] = block_offset;
}



void open_io_files(FILE* input_file, FILE* output_file, FILE* header_file)
{    
    if (!(input_file = fopen("/dev/nvme0n1p2", "r"))) {
        fprintf(stderr, "\n>> Input file failed to open");
    }
    else {
        printf("\n>> Input file opened");
        fflush(stdout);
    }
    
    if (!(header_file = fopen("headers.txt", "w+"))) {
            fprintf(stderr, "\n>> Output file failed to open");
            exit(1);
    }
    else {
        printf("\n>> Header file opened");
    }
    
    if (!(output_file = fopen("cr2_test2.cr2", "wb+"))) {
            fprintf(stderr, "\n>> Output file failed to open");
            exit(1);
    }
    else {
        printf("\n>> Output file opened");
    }
    
}

void get_cmd_options(Cmd_Options* cmd_options)
{

    static struct option long_options[] =
    {
        {"blocksize",     required_argument, NULL, 'b'},
        {"offset_start",  required_argument, NULL, 'os'},
        {"offset_start",  required_argument, NULL, 'oe'},
        {"searchsize",    required_argument, NULL, 's'},
        {"input",         required_argument, NULL, 'i'},
        {"output_folder", required_argument, NULL, 'o'},
        {"searchsize",    required_argument, NULL, 's'},
        {"verbose",       no_argument,       NULL, 'v'},                
        {NULL, 0, NULL, 0}
    };    

    extern char *optarg;
    extern int optind;  
    int cmd_option;

    while ((cmd_option = getopt_long(cmd_options->argc, cmd_options->argv,
                                     "b:os:oe:s:i:o:vh", long_options, NULL)) != -1) {
    
        switch (cmd_option) {
            
            case 'b':
                cmd_options->blocksize = optarg;
                break;
            
            case 'os':
                cmd_options->offset_start = optarg;
                break;
            
            case 'oe':
                cmd_options->offset_end = optarg;
                break;

            case 's':
                cmd_options->searchsize = optarg;
                break;
            
            case 'i':
                *cmd_options->input = optarg;                
                break;

            case 'o':
                *cmd_options->output_folder = optarg;
                break;
            
            case 'v':
                cmd_options->verbose = 1;
                break;

            case 'h':
                printf("\n\t--blocksize -b <size of logical blocks in bytes>");
                printf("\n\t--offset_start -os <offset from start of disk in bytes>");
                printf("\n\t--offset_end -oe <offset from start of disk in bytes>");
                printf("\n\t--searchsize -s <size of chunk to search for input disk in bytes>");
                printf("\n\t--input -i <file to use for header search>");
                printf("\n\t--output_folder -o <folder to use for output files>");
                printf("\n\t--verbose -v <display verbose output of ongoing process>");
                printf("\n\t--help -h <print help options>");
                break;
            
            default: break;
        }
    }
}