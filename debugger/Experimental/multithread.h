#include "proc.h"

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


// Struct for 

// Struct that can be passed to the function assigned to 
// all created threads, that both contain the information
// about the process memory section to scan, and that
// has the struct used to store scan results in
typedef 
struct _PROC_SCAN_THREAD_ARGS_T {
    // Contains found addresses, total number of found 
    // addresses, and the total size of memory used by
    // the address buffer
    PROCESS_SCAN_RESULT *memScanResult;

    // Contains information for the thread, about the
    // process mem section to be scanned.
} SCAN_THREAD_ARGS;