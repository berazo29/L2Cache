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
size_t** FIFO(size_t** cache, size_t address, int blocks_offset, int sets, int blocks,size_t **cache_l2, int blocks_offset_l2, int sets_l2, int blocks_l2);
size_t** LRU(size_t** cache, size_t address, int block_offset, int sets, int blocks, size_t **cache_l2, int blocks_offset_l2, int sets_l2, int blocks_l2);
void updateCache(FILE * trace_file, size_t** cache, int blocks_offset, int sets, int blocks, int cache_policy,size_t** cache_l2, int blocks_offset_l2, int sets_l2, int blocks_l2, int cache_policy_l2);


void printGlobalVars();

// GLOBAL
int MEM_READS = 0;
int MEM_WRITES = 0;

// CACHE L1
int CACHE_HITS_L1 = 0;
int CACHE_MISS_L1 = 0;
int NUM_SETS_L1 = 0;
int NUM_BLOCKS_L1 = 0;
int SET_BITS_L1 = 0;
int OFFSET_BITS_L1 = 0;

// CACHE L2
int CACHE_HITS_L2 = 0;
int CACHE_MISS_L2 = 0;
int NUM_SETS_L2 = 0;
int NUM_BLOCKS_L2 = 0;
int SET_BITS_L2 = 0;
int OFFSET_BITS_L2 = 0;

// Print for graters
void printSubmitOutputFormat(int dev){

    if (dev == 1){
        printf("memread:%d\n", MEM_READS-CACHE_HITS_L2);
        printf("memwrite:%d\n", MEM_WRITES);
        printf("l1cachehit:%d\n", CACHE_HITS_L1);
        printf("l1cachemiss:%d\n", CACHE_MISS_L1);
        printf("l2cachehit:%d\n", CACHE_HITS_L2);
        printf("l2cachemiss:%d\n", MEM_READS-CACHE_HITS_L2);
    } else{
        printf("memread:%d\n", MEM_READS);
        printf("memwrite:%d\n", MEM_WRITES);
        printf("l1cachehit:%d\n", CACHE_HITS_L1);
        printf("l1cachemiss:%d\n", CACHE_MISS_L1);
        printf("l2cachehit:%d\n", CACHE_HITS_L2);
        printf("l2cachemiss:%d\n", CACHE_MISS_L2);
    }
}

int main( int argc, char *argv[argc+1]) {

    long cache_size_l1;
    long block_size_l1;
    long cache_size_l2;
    long block_size_l2;

    //File name from arguments
    if (argc != 9 ){
        printf("DEV Error 1: Give 5 arg as int: cache_size_l1, str: associativity_l1, str: cache_policy_l1, int: block_size_l1, int: cache_size_l2, str: associativity_l2, str: cache_policy_l2, str: trace_file\n");
        printf("error");
        return EXIT_SUCCESS;
    }
    // Check for power of 2 for cache_size_l1 and block_size_l1
    cache_size_l1 = getCacheSize(argv[1]);
    block_size_l1 = getBlockSize(argv[4]);
    cache_size_l2 = getCacheSize(argv[5]);
    block_size_l2 = block_size_l1;
    if (cache_size_l1 == 0 || block_size_l1 == 0){
        printf("DEV Error 2: cache_size_l1 and block_size_l1 must be power of 2 and > 0\n");
        printf("error");
        return EXIT_SUCCESS;
    }
    long associativity_l1;
    unsigned int associativityAction_l1 = checkAssociativityInput(argv[2]);
    long associativity_l2;
    unsigned int associativityAction_l2 = checkAssociativityInput(argv[6]);

    // action 1,2,3 -> direct, fully, n associativity_l1 and 0 is error
    if (associativityAction_l1 == 1){
        associativity_l1 = 1;
    } else if (associativityAction_l1 == 2){
        associativity_l1 = calculateNumberCacheAddresses(cache_size_l1, block_size_l1);
    } else if (associativityAction_l1 == 3){
        associativity_l1 = getAssociativity(argv[2]);
    } else{
        //printf("DEV ERROR: associativityAction_l1 input is incorrect\n");
        return 0;
    }
    // action 1,2,3 -> direct, fully, n associativity_l2 and 0 is error
    if (associativityAction_l2 == 1){
        associativity_l2 = 1;
    } else if (associativityAction_l2 == 2){
        associativity_l2 = calculateNumberCacheAddresses(cache_size_l2, block_size_l2);
    } else if (associativityAction_l2 == 3){
        associativity_l2 = getAssociativity(argv[6]);
    } else{
        //printf("DEV ERROR: associativityAction_l1 input is incorrect\n");
        return 0;
    }

    int cache_policy_l1 = getCachePolicy(argv[3]);
    int cache_policy_l2 = getCachePolicy(argv[7]);


    // Declare the read file and read it
    FILE *fp;
    fp = fopen( argv[8], "r");
    // Check if the file is empty
    if ( fp == NULL ){
        //printf("DEV Error 3:Unable to read the file\n");
        printf("error\n");
        return EXIT_SUCCESS;
    }


    // Calculate parameters for the new cache_l1
    NUM_SETS_L1 = calculateSets(cache_size_l1, block_size_l1, associativityAction_l1, associativity_l1);
    NUM_BLOCKS_L1 = cache_size_l1 / (block_size_l1 * NUM_SETS_L1);
    NUM_SETS_L2 = calculateSets(cache_size_l2, block_size_l2, associativityAction_l2, associativity_l2);
    NUM_BLOCKS_L2 = cache_size_l2 / (block_size_l2 * NUM_SETS_L2);



    // Calculate the sets and offset bits
    SET_BITS_L1 = log(NUM_SETS_L1) / log(2);
    OFFSET_BITS_L1 = log(block_size_l1) / log(2);
    SET_BITS_L2 = log(NUM_SETS_L2) / log(2);
    OFFSET_BITS_L2 = log(block_size_l2) / log(2);

    //printGlobalVars();

    // Create a new cache_l1
    size_t** cache_l1 = createNewCache(NUM_SETS_L1, NUM_BLOCKS_L1);
    size_t** cache_l2 = createNewCache(NUM_SETS_L2, NUM_BLOCKS_L2);

    // Receive the address and simulate the cache_l1
    updateCache(fp, cache_l1, OFFSET_BITS_L1, SET_BITS_L1, NUM_BLOCKS_L1, cache_policy_l1, cache_l2, OFFSET_BITS_L2, SET_BITS_L2, NUM_BLOCKS_L2, cache_policy_l2);

    // Print the results
    printSubmitOutputFormat(1);

    // Close the file and destroy memory allocations
    fclose(fp);
    deleteCache(cache_l1, NUM_SETS_L1, NUM_BLOCKS_L1);
    deleteCache(cache_l2, NUM_SETS_L2, NUM_BLOCKS_L2);

    return EXIT_SUCCESS;
}
// read from trace file and read/write addresses
void updateCache(FILE * trace_file, size_t** cache, int blocks_offset, int sets, int blocks, int cache_policy,size_t** cache_l2, int blocks_offset_l2, int sets_l2, int blocks_l2, int cache_policy_l2){

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
            CACHE_HITS_L1++;

            // Use th LRU eviction policy
            if(isLRU != 0){
                // update which block has been most recently used
                cache = LRU(cache, address, blocks_offset, sets, blocks, cache_l2, blocks_offset_l2,sets_l2,blocks);
            }
        }else {
            // Update miss and MEM_READS
            int isHit2 = searchAddressInCache(cache_l2, address, blocks_offset_l2, sets_l2, blocks_l2);
            if (isHit2 == 1){
                CACHE_HITS_L2++;
            }
            CACHE_MISS_L1++;
            MEM_READS++;

            cache = FIFO(cache, address, blocks_offset, sets, blocks,cache_l2, blocks_offset_l2,sets_l2,blocks_l2);
        }
    }
}
// Insert in the cache 2
size_t **FIFOCACHE2(size_t** cache, size_t address, int blocks_offset, int sets, int blocks){

    size_t index = (address >> blocks_offset) & ((1 << sets) - 1);

    int flag = 0;
    int i;

    // Add to the cache
    for(i = 0; i < blocks; i++){

        // Eviction is not required

        if(cache[index][i] == (size_t) NULL){
            // Store the address
            //cache[index][i] = address >> (blocks_offset + sets);
            cache[index][i] = address;
            flag = 1;
            break;
        }
    }
    if( flag == 0 ){

        for(i = 1; i < blocks; i++){
            cache[index][i - 1] = cache[index][i];
        }
        // Store only the address
        //cache[index][blocks - 1] = address >> (blocks_offset + sets);
        cache[index][blocks - 1] = address;
    }
    return cache;
}

// insert the cache
size_t** FIFO(size_t** cache, size_t address, int blocks_offset, int sets, int blocks,size_t **cache_l2, int blocks_offset_l2, int sets_l2, int blocks_l2){

    size_t index = (address >> blocks_offset) & ((1 << sets) - 1);

    int flag = 0;
    int i;

    // Add to the cache
    for(i = 0; i < blocks; i++){

        // Eviction is not required
        if(cache[index][i] == (size_t) NULL){
            // Store the address
            //cache[index][i] = address >> (blocks_offset + sets);
            cache[index][i] = address;
            flag = 1;
            break;
        }
    }
    if( flag == 0 ){
        cache_l2 = FIFOCACHE2(cache_l2, cache[index][0], blocks_offset_l2, sets_l2, blocks_l2);
        for(i = 1; i < blocks; i++){
            cache[index][i - 1] = cache[index][i];
        }
        // Store only the address
        //cache[index][blocks - 1] = address >> (blocks_offset + sets);
        cache[index][blocks - 1] = address;
    }
    return cache;
}
// Update the block by the most recent use
size_t** LRUCACHEL2(size_t** cache, size_t address, int block_offset, int sets, int blocks){
    // use bit manipulation to obtain the the corresponding set and tag
    size_t index = (address >> block_offset) & ((1 << sets) - 1);
    //size_t tag = address >> (block_offset + sets);
    int flag = 0;
    int i;
    for(i = 0; i < blocks - 1; i++){
        if(cache[index][i + 1] == (size_t) NULL){
            break;
        }
        // Find for a true flag
        //if(cache[index][i] == tag || flag){
        if(cache[index][i] == address || flag){
            flag = 1;
            cache[index][i] = cache[index][i + 1];
        }
    }
    cache[index][i] = address;

    return cache;
}
// Update the block by the most recent use
size_t** LRU(size_t** cache, size_t address, int block_offset, int sets, int blocks, size_t **cache_l2, int blocks_offset_l2, int sets_l2, int blocks_l2){
    // use bit manipulation to obtain the the corresponding set and tag
    size_t index = (address >> block_offset) & ((1 << sets) - 1);
    //size_t tag = address >> (block_offset + sets);
    int flag = 0;
    int isFull = 1;
    int i;
    for(i = 0; i < blocks - 1; i++){
        if(cache[index][i + 1] == (size_t) NULL){
            isFull = 0;
            break;
        }
        // Find for a true flag
        //if(cache[index][i] == tag || flag){
        if(cache[index][i] == address || flag){
            flag = 1;
            cache[index][i] = cache[index][i + 1];
        }
    }
    if (isFull == 1){
        cache_l2 = FIFOCACHE2(cache_l2,cache[index][0],blocks_offset_l2,sets_l2,blocks_l2);
    }
    cache[index][i] = address;

    return cache;
}

int searchAddressInCache(size_t** cache, size_t address, int num_block_offsets, int num_sets, int blocks){

    size_t setIndex = (address >> num_block_offsets) & ((1 << num_sets) - 1);
    // Find the set in the block 1 for true and 0 for false
    for(int i = 0; i < blocks; i++){

        //if ((address >> (num_sets + num_block_offsets)) == cache[setIndex][i]){
        if (address == cache[setIndex][i]){
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

void printGlobalVars(){
    printf("NUM_SETS_L1: %d\n",NUM_SETS_L1);
    printf("NUM_BLOCKS_L1: %d\n",NUM_BLOCKS_L1);
    printf("NUM_SETS_L2: %d\n",NUM_SETS_L2);
    printf("NUM_BLOCKS_L2: %d\n",NUM_BLOCKS_L2);
}