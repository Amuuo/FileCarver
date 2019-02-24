#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>


typedef struct Block_info {
    long long* headers;
    __uint8_t** info;        
    int header_count;
} Block_info;


void resize_block_info(Block_info* block) {

    int count = block->header_count + 1;
        
    
    block->headers = 
        (long long*)realloc(block->headers, sizeof(long long)*count);                            
    
    block->info = 
        (__uint8_t**)realloc(block->info, sizeof(__uint8_t*)*count);    
    
    block->info[count-1] = (__uint8_t*)malloc(sizeof(__uint8_t)*128);
    
    printf("\nblock_info.headers realloc: %ld", sizeof(long long)*count);
    printf("\nblock_info.info realloc: %ld", sizeof(__uint8_t*)*count);    
    printf("\nblock_info.info[%d] realloc: %ld", count-1, sizeof(__uint8_t)*128);
    //printf("\nblock_info.info[%d] content: %s", count-1, block->info[count-1]);
    
    fflush(stdout);                
}


void add_header_info(Block_info* block, __uint8_t* transfer_info, long long block_offset) {    
    memcpy(block->info[block->header_count], transfer_info, 128);      
    block->headers[block->header_count++] = block_offset;
}


int main(){

    Block_info block_info;

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
        
    block_info.header_count = 0;
    
    block_info.headers = (long long*)malloc(sizeof(long long));                            
    block_info.info = (__uint8_t**)malloc(sizeof(__uint8_t*));
    block_info.info[0] = (__uint8_t*)malloc(sizeof(__uint8_t)*128);
    /*
    printf("\nblock_info.headers realloc: %d", sizeof(block_info.headers[0]));    
    printf("\nblock_info.info realloc: %d", sizeof(*block_info.info));    
    printf("\nblock_info.info[%d] realloc: %d", 0, sizeof(*block_info.info[0]));
    printf("\nblock_info.info[%d] content: %s", 0, block_info.info[0]);
    */

    regex_i = regcomp(&regex, regex_str, REG_EXTENDED);
    printf("\n>> Buffer alloc...");
    __uint8_t* buffer = malloc(32768);
    memset(buffer, 0, 32768);
    printf("\n>> Opening input file");
    fflush(stdout);
    input_file = fopen("/dev/nvme0n1p2", "r");
    
    if(!input_file){
        fprintf(stderr, "\n>> Input file failed to open");
    }
    else{
        printf("\n>> Input file opened");
        fflush(stdout);
    }
    header_file = fopen("headers.txt", "w+");
    output_file = fopen("cr2_test2.cr2", "wb+");
    
    if(!output_file) {
            fprintf(stderr, "\n>> Output file failed to open");
            exit(1);
    }
    else {
        printf("\n>> Output file opened");
    }
    
    long long i;
    int j;
    for (i = 50000000000; i < /*34000000*/100000000000; i+=32768 ) {
        
        fseek(input_file, offset+i, 0);
        //printf("\n>> Needle moved to %ld", offset+i);
        //fflush(stdout);

        if ((fread(buffer, 1, 32768, input_file) < 0)) {
            fprintf(stderr, "fread failed..");
            exit(1);
        }    
        if (i % 1000000 == 0) {
            printf("\nBlock %lld", i);
        }
                
        /*for( j = 0; j < 32768-16; j+=16){
            memcpy(transfer, &buffer[j] , 16);
            regex_i = regexec(&regex, transfer, 0, NULL, REG_EXTENDED);
            if(!regex_i) {
                printf("\n>> MATCH");  
                fprintf(header_file, "0x%08x ", j);
                for (k = 0; k < 16; ++k) {
                    fprintf(header_file, "%02x ", transfer[k]);                                        
                    if (k == 7){
                        fprintf(header_file, " ");
                    }
                }
                fprintf(header_file, "\t|");                                                    
                
                for (k = 0; k < 16; ++k) {
                    fprintf(header_file, "%c", (transfer[k] > 127 || transfer[k] < 33) ? '.' : transfer[k]);                                                            
                }
                fprintf(header_file, "|\n"); 
            }                                         
        }*/

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