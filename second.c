/*
 * Cache Cache simulator with L1 and L2 exclusive Cache
 * Author: Bryan Erazo
 * Interface: ./second <L1 cache size><L1 associativity><L1 cache policy><L1 block size><L2 cache size><L2 associativity><L2 cache policy><trace file>
 *      Example: ./first 64 assoc:2 lru 4 trace1.txt
 *      TraceFile:
 *      R 0x01
 *      W 0x02
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "second.h"

#define ARR_MAX 100

int calculateSets(size_t cache_size,size_t block_size,unsigned int assocAction, size_t assoc);
size_t ** createNewCache(int sets, int blocks);
void deleteCache(size_t ** cache, int sets, int blocks);
int searchAddressInCache(size_t** cache, size_t address, int num_block_offsets, int num_sets, int blocks);
size_t** LRU(size_t** cache, size_t address, int block_offset, int sets, int blocks);
size_t** insertNewTagToCache(size_t** cache, size_t address, int blocks_offset, int sets, int blocks);
void updateCache(FILE * trace_file, size_t** cache, int blocks_offset, int sets, int blocks, int cache_policy);

// GLOBAL
int MEM_READS = 0;
int MEM_WRITES = 0;
int CACHE_HITS = 0;
int CACHE_MISS = 0;
int NUM_SETS = 0;
int NUM_BLOCKS = 0;
int SET_BITS = 0;
int OFFSET_BITS = 0;

// Print for graters
void printSubmitOutputFormat(){

    printf("memread:%d\n", MEM_READS);
    printf("memwrite:%d\n", MEM_WRITES);
    printf("cachehit:%d\n", CACHE_HITS);
    printf("cachemiss:%d\n", CACHE_MISS);
}

int main( int argc, char *argv[argc+1]) {

    long cache_size;
    long block_size;

    //File name from arguments
    if (argc != 6 ){
        printf("DEV Error 1: Give 5 arg as int: cache_size, str: associativity, str: cache_policy, int: block_size, str: trace_file\n");
        printf("error");
        return EXIT_SUCCESS;
    }
    // Check for power of 2 for cache_size and block_size
    cache_size = getCacheSize(argv[1]);
    block_size = getBlockSize(argv[4]);
    if (cache_size == 0 || block_size == 0){
        printf("DEV Error 2: cache_size and block_size must be power of 2 and > 0\n");
        printf("error");
        return EXIT_SUCCESS;
    }
    long associativity;
    unsigned int associativityAction = checkAssociativityInput(argv[2]);

    // action 1,2,3 -> direct, fully, n associativity and 0 is error
    if (associativityAction == 1){
        associativity = 1;
    } else if (associativityAction == 2){
        associativity = calculateNumberCacheAddresses(cache_size, block_size);
    } else if (associativityAction == 3){
        associativity = getAssociativity(argv[2]);
    } else{
        //printf("DEV ERROR: associativityAction input is incorrect\n");
        return 0;
    }
    int cache_policy = getCachePolicy(argv[3]);

//    printf("cache_size: %lu\n",cache_size);
//    printf("block_size: %lu\n",block_size);
//    printf("associativityAction: %d\n",associativityAction);
//    printf("associativity: %lu\n",associativity);
//    printf("cache_policy: %d\n",cache_policy);

    // Declare the read file and read it
    FILE *fp;
    fp = fopen( argv[5], "r");
    // Check if the file is empty
    if ( fp == NULL ){
        //printf("DEV Error 3:Unable to read the file\n");
        printf("error\n");
        return EXIT_SUCCESS;
    }


    // Calculate parameters for the new cache
    NUM_SETS = calculateSets(cache_size, block_size, associativityAction, associativity);
    NUM_BLOCKS = cache_size / (block_size * NUM_SETS);

    // Calculate the sets and offset bits
    SET_BITS = log(NUM_SETS) / log(2);
    OFFSET_BITS = log(block_size) / log(2);

    // Create a new cache
    size_t** cache = createNewCache(NUM_SETS, NUM_BLOCKS);

    // Receive the address and simulate the cache
    updateCache(fp, cache, OFFSET_BITS, SET_BITS, NUM_BLOCKS, cache_policy);

    // Print the results
    printSubmitOutputFormat();

    // Close the file and destroy memory allocations
    fclose(fp);
    deleteCache(cache, NUM_SETS, NUM_BLOCKS);

    return EXIT_SUCCESS;
}

// insert the cache
size_t** insertNewTagToCache(size_t** cache, size_t address, int blocks_offset, int sets, int blocks){

    size_t index = (address >> blocks_offset) & ((1 << sets) - 1);

    int flag = 0;
    int i;

    // Add to the cache
    for(i = 0; i < blocks; i++){

        // Eviction is not required
        if(cache[index][i] == (size_t) NULL){
            cache[index][i] = address >> (blocks_offset + sets);
            flag = 1;
            break;
        }
    }
    if( flag == 0 ){

        for(i = 1; i < blocks; i++){
            cache[index][i - 1] = cache[index][i];
        }
        cache[index][blocks - 1] = address >> (blocks_offset + sets);
    }
    return cache;
}

// Update the block by the most recent use
size_t** LRU(size_t** cache, size_t address, int block_offset, int sets, int blocks){
    // use bit manipulation to obtain the the corresponding set and tag
    size_t index = (address >> block_offset) & ((1 << sets) - 1);
    size_t tag = address >> (block_offset + sets);
    int flag = 0;
    int i;
    for(i = 0; i < blocks - 1; i++){
        if(cache[index][i + 1] == (size_t) NULL){
            break;
        }
        // Find for a true flag
        if(cache[index][i] == tag || flag){
            flag = 1;
            cache[index][i] = cache[index][i + 1];
        }
    }
    cache[index][i] = tag;

    return cache;
}

int searchAddressInCache(size_t** cache, size_t address, int num_block_offsets, int num_sets, int blocks){

    size_t setIndex = (address >> num_block_offsets) & ((1 << num_sets) - 1);
    // Find the set in the block 1 for true and 0 for false
    for(int i = 0; i < blocks; i++){

        if ((address >> (num_sets + num_block_offsets)) == cache[setIndex][i]){
            return 1;
        }
    }
    return 0;
}

size_t ** createNewCache(int sets, int blocks){

    // Allocate an 2D array as a new cache with with sets and blocks
    size_t** cache =(size_t**) malloc(sizeof(size_t*) * sets);
    for(int i = 0; i < sets; i++){
        cache[i] = (size_t*) malloc(sizeof(size_t) * blocks);
        for(int j = 0; j < blocks; j++){
            cache[i][j] = (size_t) NULL;
        }
    }
    return cache;
}

// Free the 2d array allocated
void deleteCache(size_t ** cache, int sets, int blocks){
    for(int i = 0; i < sets; i++){
        for (int j = 0; j < blocks; j++){
            cache[i][j] = (size_t) NULL;
        }
        free(cache[i]);
    }
    free(cache);
    cache = NULL;
}

// read from trace file and read/write addresses
void updateCache(FILE * trace_file, size_t** cache, int blocks_offset, int sets, int blocks, int cache_policy){

    // Set the policy to FIFO or LRU
    int isLRU;
    if (cache_policy == 1){
        isLRU=0;
    } else if ( cache_policy == 2){
        isLRU=1;
    }

    char action;
    size_t address;
    // reads until end of file
    while((fscanf(trace_file, "%c %zx\n", &action, &address) != EOF) && (action != '#')){

        // Increment for each write
        if(action != 'R') {
            MEM_WRITES++;
        }

        int isHit = searchAddressInCache(cache, address, blocks_offset, sets, blocks);
        if(isHit == 1){
            CACHE_HITS++;

            // Use th LRU eviction policy
            if(isLRU != 0){
                // update which block has been most recently used
                cache = LRU(cache, address, blocks_offset, sets, blocks);
            }
        }else {
            // Update miss and MEM_READS
            CACHE_MISS++;
            MEM_READS++;

            cache = insertNewTagToCache(cache, address, blocks_offset, sets, blocks);
        }
    }
}
// Create an empty cache with given capacity or lines
int calculateSets(size_t cache_size,size_t block_size,unsigned int assocAction, size_t assoc){
    int set=0;
    if (assocAction == 1){
        set = cache_size/block_size;
        return set;
    } else if (assocAction == 2){
        return 1;
    } else if (assocAction == 3){
        set = cache_size/(block_size*assoc);
        return set;
    } else{
        perror("Error assocAction");
        return 0;
    }
}
struct Cache *createCache(struct Cache *cache, size_t cache_size, size_t block_size, unsigned int assocAction, size_t assoc) {

    assert(cache == NULL);
    struct Cache *new_cache = NULL;
    if ( assocAction == 1){
        size_t number_addresses = calculateNumberCacheAddresses(cache_size, block_size);
        if (number_addresses == 0 ){
            return NULL;
        }
        // Initialize cache in an array
        new_cache = malloc(number_addresses*sizeof(struct Cache));
        for (size_t i = 0; i < number_addresses; ++i) {
            new_cache[i].len = number_addresses;
            new_cache[i].max_nodes_allow = assoc;
            new_cache[i].number_nodes_in_linked_list = 0;
            new_cache[i].linked_list = NULL;
        }
    } else if ( assocAction == 2 ){
        size_t number_addresses = calculateNumberCacheAddresses(cache_size, block_size);
        if (number_addresses == 0 ){
            return NULL;
        }
        new_cache = malloc(sizeof(struct Cache));
        new_cache[0].len = 1;
        new_cache[0].max_nodes_allow = number_addresses;
        new_cache[0].number_nodes_in_linked_list = 0;
        new_cache[0].linked_list = NULL;
    } else if (assocAction == 3){
        size_t number_addresses = calculateNumberCacheAddresses(cache_size, block_size)/assoc;
        if ( number_addresses == 0 ){
            number_addresses = calculateNumberCacheAddresses(cache_size, block_size);
            if ( number_addresses == 0 ){
                return NULL;
            }
            new_cache = malloc(sizeof(struct Cache));
            new_cache[0].len = 1;
            new_cache[0].max_nodes_allow = number_addresses;
            new_cache[0].number_nodes_in_linked_list = 0;
            new_cache[0].linked_list = NULL;
        } else{
            // Initialize cache in an array
            new_cache = malloc(number_addresses*sizeof(struct Cache));
            for (size_t i = 0; i < number_addresses; ++i) {
                new_cache[i].len = number_addresses;
                new_cache[i].max_nodes_allow = assoc;
                new_cache[i].number_nodes_in_linked_list = 0;
                new_cache[i].linked_list = NULL;
            }
        }

    } else{
        return NULL;
    }
    return new_cache;
}
bool IsPowerOfTwo(unsigned long x){
    return (x != 0) && ((x & (x - 1)) == 0);
}

bool isEven(long int n){
    // n^1 is n+1, then even, else odd
    assert(n >= 0);
    if ( (n ^ 1) == n + 1 ){
        return true;
    } else{
        return false;
    }
}

// Functions
long getCacheSize(char *arg){

    assert(arg != NULL);

    char *ptr;
    long cache_size;
    cache_size = strtol(arg, &ptr,10);
    if ( IsPowerOfTwo(cache_size) ){
        return cache_size;
    }
    return 0;
}

long getBlockSize(char *arg){

    assert(arg != NULL);

    char *ptr;
    long block_size;
    block_size = strtol(arg, &ptr,10);
    if ( IsPowerOfTwo(block_size) ){
        return block_size;
    }
    return 0;
}
unsigned int checkAssociativityInput(char *arg){
    /* Doc Function use
     * 0 is an error
     * 1 is a direct
     * 2 is full associativity
     * 3 is n associativity: the argument is in the correct input and contains the n value;
    */
    char *direct = "direct";
    char *assoc = "assoc";
    char *assoc_n = "assoc:";
    if (arg == NULL){
        return 0;
    }
    char str[ARR_MAX];
    str[0]='\0';
    strcpy(str,arg);

    if ( strcmp(str,direct) == 0 ){
        return 1;
    } else if ( strcmp(str, assoc) == 0 ){
        return 2;
    } else{
        if (strcmp(str, assoc_n) <= 0 ){
            return 0;
        }
        unsigned int len = strlen(assoc_n);
        char tmp[ARR_MAX];
        tmp[0]='\0';
        for (int i = 0; i < len; ++i) {
            if (str[i] != assoc_n[i]){
                return 0;
            }
        }
        unsigned int len_arg = strlen(arg);
        int k = 0;
        for (unsigned int i = len; i < len_arg; ++i) {
            tmp[k]=str[i];
            k++;
        }
        tmp[len_arg-len]='\0';
        char *ptr;
        long num;
        num = strtol(tmp, &ptr,10);
        if (num == 1){
            return 1;
        }
        else if (num > 0){
            return 3;
        } else{
            return 0;
        }
    }
}

// Get the direct, assoc, and assoc:n
unsigned int getAssociativity(char *arg){
    assert(arg != NULL);
    char str[ARR_MAX];
    str[0]='\0';
    strcpy(str,arg);

    unsigned int assoc_action = checkAssociativityInput(str);
    assert( assoc_action >= 0 && assoc_action <= 3);

    if ( assoc_action == 0 ){
        printf("DEV ERROR 5: Action 0 is invalid and must print error in production\n");
        return 0;
    }
    // This is fully direct cache
    if ( assoc_action == 1){
        return 1;
    }
    // This is fully associate cache
    if ( assoc_action == 2){
        return 2;
    } else{
        // This is n associate cache
        long num = getNumberFromAssoc(arg);
        //printf("DEV num:%lu\n",num);
        return num;
    }
}

long getNumberFromAssoc(char *arg){
    char *assoc_n = "assoc:";
    unsigned int len = strlen(assoc_n);
    char tmp[ARR_MAX];
    tmp[0]='\0';
    unsigned int len_arg = strlen(arg);
    int k = 0;
    for (unsigned int i = len; i < len_arg; ++i) {
        tmp[k]=arg[i];
        k++;
    }
    tmp[len_arg-len]='\0';
    char *ptr;
    long num;
    num = strtol(tmp, &ptr,10);
    return num;
}

int getCachePolicy(char *arg){
    /* Function eviction policy
     * 0 is an error
     * 1 is FIFO -> First In First Out
     * 2 is LRU -> least Recently Used
     */
    if (arg == NULL){
        return 0;
    }
    char str[ARR_MAX];
    str[0]='\0';
    char *fifo = "fifo";
    char *lru = "lru";

    // Accept the input in any form if the same word
    strcpy(str,arg);
    for(int i = 0; str[i]; i++){
        str[i] = (char )tolower(str[i]);
    }

    if ( strcmp(str, fifo) == 0 ){
        return 1;
    } else if (strcmp(str, lru) == 0 ){
        return 2;
    } else {
        return 0;
    }
}
size_t calculateNumberCacheAddresses(size_t cache_size, size_t cache_block ){
    return cache_size/cache_block;
}