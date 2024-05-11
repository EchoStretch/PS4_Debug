#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#define EasySending(...)             \
    printf("%s(): ", __FUNCTION__);  \
    printf(__VA_ARGS__);             \
    printf("\n");

typedef void *ScePthread, *ScePthreadAttr, *ScePthreadMutex, *ScePthreadMutexattr;

// Define SCE function prototypes
int (*scePthreadCreate)(ScePthread *thread, const ScePthreadAttr *attr, void *(*entry)(void *), void *arg, const char *name);
void (*scePthreadExit)(void *value);
int (*scePthreadDetach)(ScePthread thread);
int (*scePthreadJoin)(ScePthread thread, void **value_ptr);
ScePthread(*scePthreadSelf)(void);
int (*scePthreadCancel)(ScePthread thread);
int (*scePthreadMutexUnlock)(ScePthreadMutex *mutex);

// Define the pfmalloc, pfrealloc
#define pfmalloc malloc
#define pfrealloc realloc


// Just a simple function, which "hooks" the custom sce* version's
// to the FreeBSD SDK (pthread.h) library versions. Why? well this
// makes it easier to then port the code over to a PL SRC 
int hookSCEtoFreeBSDVersions() {
    // "Hook" the PS4 SCE functions to corresponding FreeBSD functions
    scePthreadCreate = (int (*)(ScePthread *, const ScePthreadAttr *, void *(*)(void *), void *, const char *)) & pthread_create;
    scePthreadExit = (void (*)(void *)) & pthread_exit;
    scePthreadDetach = (int (*)(ScePthread)) & pthread_detach;
    scePthreadJoin = (int (*)(ScePthread, void **)) & pthread_join;
    scePthreadSelf = (ScePthread(*)(void)) & pthread_self;
    scePthreadCancel = (int (*)(ScePthread)) & pthread_cancel;
    scePthreadMutexUnlock = (int (*)(ScePthreadMutex *)) & pthread_mutex_unlock;
    return 0;
}

#define INIT_ADDR_NUM 1000

// Struct for keeping track on scan results
typedef
struct PROC_SCAN_RESULT_T {
    // Buffer for containing <x> number of 64-bit unsigned 
    // integer memory addresses. All found while scanning
    // one of the memory sections of a process.
    uint64_t **validMemAddresses;

    // Total number of individual 64-bit unsigned integer 
    // addresses stored in <validMemAddresses>
    size_t addressCount;

    // Total size of the <validMemAddresses> memory
    size_t bufferMemSize;
} PROCESS_SCAN_RESULT;



typedef
struct SCAN_PROCSECTION_INFO_T {
    int idx;

    // Once the function has finished this will be filled with all
    // of the 64bit unsigned integer memory addresses found during
    // the scanning of a process section.
    // Then the contents of this will be appended to the main buff
    // containing the total found addresses from all the individual
    // threads...
    uint64_t *tempFoundAddresses;

    // This will contain the number of addresses found during the
    // execution of a certain thread. And just like the temporary
    // address buffer, the value of this will be appended to the
    // "main" variable containing the total number of address found
    // by all the individual threads
    size_t addressCount;
}SECTION_SCAN_INFO;

void *PrintValue(void *arg) {
    SECTION_SCAN_INFO *info = (SECTION_SCAN_INFO *)arg;
    EasySending("Current Thread Index %d", info->idx);


    // Init/create the buffer
    size_t memSize = INIT_ADDR_NUM * sizeof(uint64_t);
    info->tempFoundAddresses = pfmalloc(memSize);
    if (info->tempFoundAddresses == NULL) {
        EasySending("Unable to allocate memory for buffer in thread idx %d", info->idx);
        free(info);
        return NULL;
    }

    // Fill the buffer with fake offsets
    for (int i = 0; i < INIT_ADDR_NUM; i++)
        info->tempFoundAddresses[i] = 2000 + i;

    info->addressCount = INIT_ADDR_NUM;
    EasySending("Thread Index %d, is done", info->idx);
    return NULL;
}
#define MAX_THREAD_COUNT 10

int copy_addresses_to_dest(uint64_t *dest, SECTION_SCAN_INFO *src, int struct_count) {
    size_t temp_addressCount = 0;
    size_t temp_current_idx = 0;

    // Loop over each scanning thread-related structure, accessing
    // the buffer holding the addresses found by that thread 
    for (int i = 0; i < struct_count; i++) {
        uint64_t *bufferToCopyFrom = src[i].tempFoundAddresses;
        temp_addressCount = src[i].addressCount;

        // Calculate the size of data to copy
        size_t copySize = temp_addressCount * sizeof(uint64_t);

        // Copy the data
        memcpy(dest + temp_current_idx, bufferToCopyFrom, copySize);

        // Update the current index for the next iteration
        temp_current_idx += temp_addressCount;
    }
    return 0;
}

int main() {
    hookSCEtoFreeBSDVersions();
    // Array to hold thread descriptors
    ScePthread threads[MAX_THREAD_COUNT];
    //memset(threads, NULL, MAX_THREAD_COUNT);

    // Array to hold the structs created for each threads
    SECTION_SCAN_INFO *threadargs[MAX_THREAD_COUNT];
    //memset(threadargs, NULL, MAX_THREAD_COUNT);

    int successIdx = 0; // Array to hold arguments for threads
    int structIdx = 0;

    // Create 10 threads
    for (int i = 0; i < 10; i++) {
        // Initialize the struct containing process section information
        SECTION_SCAN_INFO *newInfo = pfmalloc(sizeof(SECTION_SCAN_INFO));
        if (newInfo == NULL) {
            EasySending("ERROR");
            return -1;
        }

        // Memory Section Entry Index
        newInfo->idx = i;

        // Try to create a new thread for the current process memory section
        ScePthread fdThread = NULL;
        int threadResult = scePthreadCreate(
            &fdThread,
            NULL,
            PrintValue,
            (void *)newInfo,
            "sectionScanner"
        );
        threads[successIdx++] = (threadResult == 0) ? fdThread : NULL;
        threadargs[structIdx++] = newInfo;
    }

    // Wait for all threads to finish
    EasySending("Waiting for threads to finish");
    for (int i = 0; i < successIdx; i++)
        if (threads[i] != NULL)
            scePthreadJoin(threads[i], NULL);

     // Now we calculate total number of found addresses
    size_t totalFound = 0;
    for (int i = 0; i < structIdx; i++) {
        if (threadargs[i] != NULL) {
            size_t tempAddrCount = threadargs[i]->addressCount;
            EasySending("Thread[%d] Found: %zu number of addresses", i, tempAddrCount);
            totalFound += tempAddrCount;
        }
    }
    
    EasySending("Total Found %zu", totalFound);

    // Now we make a buffer to collect all addresses found by all threads
    int currentAddrIdx = 0;
    uint64_t *collectionOfFoundAddr = pfmalloc(totalFound * sizeof(uint64_t));
    for (int i = 0; i < structIdx; i++) {
        copy_addresses_to_dest(
            collectionOfFoundAddr,
            threadargs[i],
            structIdx
        );
    }
    
    // Cleanup the thread related structs
    for (int i = 0; i < structIdx; i++) {
        if (threadargs[i] != NULL) {
            EasySending("Cleaning Up thread %d", i);
            if (threadargs[i]->tempFoundAddresses != NULL)
                free(threadargs[i]->tempFoundAddresses);

            free(threadargs[i]);
        }
    }

    EasySending("Done");
    return 0;
}
