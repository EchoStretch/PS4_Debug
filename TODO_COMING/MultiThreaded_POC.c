#include "../debugger/include/compare.h"
#include "../debugger/include/proc.h"

#define INIT_MAX_ADDR_COUNT 10000

typedef
struct PROC_SCAN_THREAD_INFO {
    // Memory Related
    struct sys_proc_vm_map_args *processMaps;
    struct proc_vm_map_entry *sectionToScan;
    struct cmd_proc_scan_packet *scanPkt;
    int sectionIndex;
    // Process Related
    int fd;
    int pid;
    // Scan Related
    size_t scanValSize;
    unsigned char *pExtraValue;
    unsigned char *scanBuffer;
    unsigned char *data;
    // Address
    uint64_t *tempValidAddr;
} PROC_SCAN_ARGS;


// Thread function for scanning one memory section of process
void *ThreadFunctionScanProcSection(void *arg) {
    // Define variables useed inside of the function
    struct proc_vm_map_entry *sectionToScan;
    PROC_SCAN_ARGS *scanArguments;

    // Extract information about the scan, using <arg>
    scanArguments = (PROC_SCAN_ARGS *)arg;
    sectionToScan = &scanArguments->processMaps[scanArguments->sectionIndex];

    // Initialize variables for scanning this section
    size_t addressCount = 0;
    size_t init_memSize = INIT_MAX_ADDR_COUNT * sizeof(uint64_t);
    uint64_t *valid_addresses = (uint64_t *)pfmalloc(init_memSize);

    // Check if memory allocation was successful
    if (valid_addresses == NULL) {
        free(scanArguments->processMaps);
        free(scanArguments->data);
        net_send_status(scanArguments->fd, CMD_DATA_NULL);
        return NULL;
    }

    // Calculate section start address and length
    uint64_t sectionStartAddr = sectionToScan->start;
    size_t sectionLen = sectionToScan->end - sectionStartAddr;

    // BEGIN THE PROCESS MEMORY SECTION SCANNING
    // Iterate through the memory section
    for (uint64_t j = 0; j < sectionLen; j += scanArguments->scanValSize) {
        // If the current offset is at a page boundary, read the next page
        if (j == 0 || !(j % PAGE_SIZE)) {
            sys_proc_rw(
                scanArguments->pid,
                j,
                scanArguments->scanBuffer,
                PAGE_SIZE,
                FALSE
            );
        }

         // Calculate the scan offset and current address
        uint64_t scanOffset = j % PAGE_SIZE;
        uint64_t curAddress = sectionStartAddr + j;

        // Run comparison on the found value
        if (CompareProcScanValues(
            scanArguments->scanPkt->compareType,
            scanArguments->scanPkt->valueType,
            scanArguments->scanValSize,
            scanArguments->data,
            scanArguments->scanBuffer + scanOffset,
            scanArguments->pExtraValue)) {

           // Resize the address list if needed
            if (addressCount + 1 >= INIT_MAX_ADDR_COUNT || addressCount == INIT_MAX_ADDR_COUNT) {
                if (ResizeTheAddrListBuffer(&valid_addresses, &init_memSize) == -1) {
                    free(scanArguments->processMaps);
                    free(scanArguments->data);
                    free(valid_addresses);
                    net_send_status(scanArguments->fd, CMD_DATA_NULL);
                    return NULL;
                }
            }

            // Add the new found address to the list
            valid_addresses[addressCount++] = curAddress;
        }
    }

    //
    // TODO: 
    // IMPLEMENT FUNCTION WHICH ADDS THE TEMPORARY BUFFER CONTAINING ADDRESSES FOUND DURING SCAN
    // TO THE GLOBALLY USED ADDRESS LIST VARIABLE HERE
    //


    // Free allocated memory
    free(valid_addresses);

    return NULL;
}


// This replaces the previous version! It's console scan recreation
int proc_scan_handle(int fd, struct cmd_packet *packet) {
    struct cmd_proc_scan_packet *sp;
    size_t scanValSize;
    unsigned char *data;

    // Extract the data from the RPC packet, and check if the data 
    // pointer isn't valid, and return early if so
    if (!(sp = (struct cmd_proc_scan_packet *)packet->data)) {
       // Send status indicating null data
        net_send_status(fd, CMD_DATA_NULL);
        return 1;
    }

    // Calculate the length of the value to be scanned
    if (!(scanValSize = GetSizeOfProcScanValue(sp->valueType)))
        scanValSize = sp->lenData;


     // Allocate memory to store received data
    if ((data = (unsigned char *)pfmalloc(sp->lenData)) == NULL) {
        net_send_status(fd, CMD_DATA_NULL);
        return 1;
    }

    // Notify successful data reception
    net_send_status(fd, CMD_SUCCESS);

    // Receive data from the network
    net_recv_data(fd, data, sp->lenData, 1);

    // Query for the process ID's memory map
    struct sys_proc_vm_map_args args;
    memset(&args, NULL, sizeof(struct sys_proc_vm_map_args));
    if (sys_proc_cmd(sp->pid, SYS_PROC_VM_MAP, &args)) {
        free(data);
        net_send_status(fd, CMD_ERROR);
        return 1;
    }

    // Calculate the size of the memory map
    size_t size = args.num * sizeof(struct proc_vm_map_entry);
    args.maps = (struct proc_vm_map_entry *)pfmalloc(size);
    if (args.maps == NULL) {
        free(data);
        net_send_status(fd, CMD_DATA_NULL);
        return 1;
    }

    // Retrieve the process memory map
    if (sys_proc_cmd(sp->pid, SYS_PROC_VM_MAP, &args)) {
        free(args.maps);
        free(data);
        net_send_status(fd, CMD_ERROR);
        return 1;
    }

    // Notify successful memory map retrieval
    net_send_status(fd, CMD_SUCCESS);

    // Start the scanning process
    uprintf("scan start");

    // Initialize variables for scanning
    unsigned char *pExtraValue = scanValSize == sp->lenData ? NULL : &data[scanValSize];
    unsigned char *scanBuffer = (unsigned char *)pfmalloc(PAGE_SIZE);
    if (scanBuffer == NULL) {
        free(args.maps);
        free(data);
        net_send_status(fd, CMD_DATA_NULL);
        return 1;
    }

    // Counter used to increase the <valid_addresses> current array index
    // everytime a valid address is found
    size_t addressCount = 0;

    size_t init_memSize = INIT_MAX_ADDR_COUNT * sizeof(uint64_t);

    // Allocate memory to hold <INIT_MAX_ADDR_COUNT> number of offsets
    uint64_t *valid_addresses = (uint64_t *)pfmalloc(init_memSize);
    if (valid_addresses == NULL) {
        free(scanBuffer);
        free(args.maps);
        free(data);
        net_send_status(fd, CMD_DATA_NULL);
        return 1;
    }
    
    // Array to hold thread descriptors
    ScePthread threads[10];

    // Loop through each memory section of the process
    for (size_t i = 0; i < args.num; i++) {
       // Skip sections that cannot be read
        if ((args.maps[i].prot & PROT_READ) != PROT_READ)
            continue;

        // Check if we've created 10 threads, if so, break the loop
        if (addressCount >= 10) break;

        // Now we build the structure which will hold necessary info
        // about the current process memory section to be scanned
        PROC_SCAN_ARGS *thread_info = (PROC_SCAN_ARGS *)pfmalloc(sizeof(PROC_SCAN_ARGS));
        if (thread_info == NULL) {
            net_send_status(fd, CMD_ERROR);
            free(args.maps);
            free(valid_addresses);
            free(scanBuffer);
            free(data);
            return -1;
        }
        
        // Otherwise if allocation succeeded we initialize the members
        thread_info->processMaps = &args.maps;
        thread_info->sectionIndex = i;
        thread_info->scanPkt = sp;
        thread_info->scanValSize = scanValSize;
        thread_info->scanBuffer = scanBuffer;
        thread_info->pExtraValue = pExtraValue;
        thread_info->data = data;
        thread_info->fd = fd;


        // Try to create a new thread for the current process memory section
        ScePthread fdThread = NULL;
        int threadResult = scePthreadCreate(
            &fdThread,
            NULL,
            (void *(*)(void *))ThreadFunctionScanProcSection,
            (void *)thread_info,
            "sectionScanner"
        );

        // Check if the thread creation was successfull, and if so we add the
        // thread descriptor to the <threads> array, and increase the thread
        // count by one
        if (threadResult == 0 && fdThread != NULL)
            threads[addressCount++] = fdThread;

        // Otherwise if it failed, we set the current thread descriptor entry
        // to the value of NULL, to indicate it's an invalid thread
        else threads[addressCount++] = NULL;
    } 
    
    
    // Wait for all threads to finish
    for (size_t i = 0; i < addressCount; i++) {
        if (threads[i] != NULL)
            scePthreadJoin(threads[i], NULL);
    }

    // Notify the end of the scanning process
    uprintf("scan done");

    // Now we enter a for-loop, so that we can send back each and every individual
    // memory address that we saved to the <valid_addresses> array during the 
    // process scanning process, back to the PC
    for (size_t i = 0; i < addressCount; i++) {
       // Send the Offset back to the PC
        net_send_data(fd, &valid_addresses[i], sizeof(uint64_t));
    }

    // Send an end flag to mark the end of data transmission
    uint64_t endflag = 0xFFFFFFFFFFFFFFFF;
    net_send_data(fd, &endflag, sizeof(uint64_t));

    // Free allocated memory
    free(scanBuffer);
    free(valid_addresses);
    free(args.maps);
    free(data);

    return 0;
}
