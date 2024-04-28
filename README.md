# PS4DebugV2



### Console Scan Feature
So the developer [ctn123](https://github.com/ctn123) said, that the console scan feature he made is basically: "Read memory, compare bytes, rinse, repeat" and i assume then, when all process memory sections have been scanned, send back data." By his description, I've come up with an idea on how to replicate this.

Here are the steps of my idea:
1. As he said, we compare bytes, so first we need to allocate a buffer using the pfmalloc (malloc with prefaulting) function, which could hold, let's say, 10000 maximum number of (uint64_t-based) memory addresses, which gets stored in the <curAddress> variable if the comparing function <CompareProcScanValues> succeeds.<br><br>
   In the case of the <CompareProcScanValues> function succeeding, we could then append the memory address of <curAddress> to the buffer that can hold 10000 separate (uint64_t) values, and then we just repeat this until the initial for-loop (that goes through each memory section of the process) at the line `for (size_t i = 0; i < args.num; i++)` has finished.

2. Once all memory sections have been scanned, we then execute a loop where we use the array containing the 10000 separate uint64_t-based address values to send back the addresses one by one using the following line: `net_send_data(fd, <current address to be sent here> sizeof(uint64_t));` until all valid addresses have been sent back.

3. Then we send back the end flag.

So here is what i've come up with so far code wise:
```c
// The Default maximum number of addresses 10000
#define MAX_ADDRESS_COUNT 10000

// Currently Experimental, POC for replicating ctn123's console scan feature
// NOTE: haven't tested this yet!
int proc_console_scan_handle(int fd, struct cmd_packet *packet) {
   // Extracting data from the RPC packet
   struct cmd_proc_scan_packet *sp = (struct cmd_proc_scan_packet *)packet->data;

   // Check if the data pointer is valid
   if (!sp) {
      // Send status indicating null data
      net_send_status(fd, CMD_DATA_NULL);
      return 1;
   }

   // Calculate the length of the value to be scanned
   size_t valueLength = GetSizeOfProcScanValue(sp->valueType);
   if (!valueLength) {
      valueLength = sp->lenData;
   }

   // Allocate memory to store received data
   unsigned char *data = (unsigned char *)pfmalloc(sp->lenData);
   if (!data) {
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
   if (!args.maps) {
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
   unsigned char *pExtraValue = valueLength == sp->lenData ? NULL : &data[valueLength];
   unsigned char *scanBuffer = (unsigned char *)pfmalloc(PAGE_SIZE);

   // Allocate memory to hold <MAX_ADDRESS_COUNT> number of offsets
   uint64_t *valid_addresses = (uint64_t *)pfmalloc(MAX_ADDRESS_COUNT * sizeof(uint64_t));
   if (!valid_addresses) {
      free(scanBuffer);
      free(args.maps);
      free(data);
      net_send_status(fd, CMD_DATA_NULL);
      return 1;
   }

   // Counter used to increase the <valid_addresses> current array index
   // everytime a valid address is found
   size_t addressCount = 0;

   // Loop through each memory section of the process
   for (size_t i = 0; i < args.num; i++) {
      // Skip sections that cannot be read
      if ((args.maps[i].prot & PROT_READ) != PROT_READ)
         continue;


      // Calculate section start address and length
      uint64_t sectionStartAddr = args.maps[i].start;
      size_t sectionLen = args.maps[i].end - sectionStartAddr;

      // Iterate through the memory section
      for (uint64_t j = 0; j < sectionLen; j += valueLength) {
         // If the current offset is at a page boundary, read the next page
         if (j == 0 || !(j % PAGE_SIZE)) {
            sys_proc_rw(
               sp->pid,
               sectionStartAddr,
               scanBuffer,
               PAGE_SIZE,
               0
            );
         }

         // Calculate the scan offset and current address
         uint64_t scanOffset = j % PAGE_SIZE;
         uint64_t curAddress = sectionStartAddr + j;

         // Compare the scanned value with the requested value
         if (CompareProcScanValues(
            sp->compareType,
            sp->valueType,
            valueLength,
            data,
            scanBuffer + scanOffset,
            pExtraValue)) {

            // Check if the addressCount exceeds MAX_ADDRESS_COUNT, meaning the memory needs
            // to be resized using pfrealloc
            if (addressCount + 1 >= MAX_ADDRESS_COUNT || addressCount == MAX_ADDRESS_COUNT) {
               size_t oldSize = MAX_ADDRESS_COUNT * sizeof(uint64_t);
               size_t newSize = 2 * oldSize;

               // Resize the valid_addresses array, and make sure to check if pfrealloc
               // failed or not
               uint64_t *new_valid_addresses = (uint64_t *)pfrealloc(valid_addresses, newSize, oldSize);
               if (new_valid_addresses == NULL) {
                  free(scanBuffer);
                  free(args.maps);
                  free(data);
                  free(valid_addresses);
                  net_send_status(fd, CMD_DATA_NULL);
                  return 1;
               }

               // Assign valid_addresses the resized memory
               valid_addresses = new_valid_addresses;
            }

           // Append the new valid address to the buffer
            valid_addresses[addressCount++] = curAddress;
         }
      }
   }

   // Now we enter a for-loop, so that we can send back each and every individual
   // memory address that we saved to the <valid_addresses> array during the 
   // process scanning process, back to the PC
   for (size_t i = 0; i < addressCount; i++) {
      // Send the Offset back to the PC
      net_send_data(
         fd,
         &valid_addresses[i],
         sizeof(uint64_t)
      );
   }

   // Notify the end of the scanning process
   uprintf("scan done");

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
```
