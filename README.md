# PS4DebugV2



### Console Scan Feature
So the developer [ctn123](https://github.com/ctn123) said, that the console scan feature he made is basically: "Read memory, compare bytes, rinse, repeat" and i assume then, when all process memory sections have been scanned, send back data." By his description, I've come up with an idea on how to replicate this.

Here are the steps of my idea:
1. As he said, we compare bytes, so first we need to allocate a buffer using the pfmalloc (malloc with prefaulting) function, which could hold, let's say, 10000 maximum number of (uint64_t-based) memory addresses, which gets stored in the <curAddress> variable if the comparing function <CompareProcScanValues> succeeds.<br><br>
   In the case of the <CompareProcScanValues> function succeeding, we could then append the memory address of <curAddress> to the buffer that can hold 10000 separate (uint64_t) values, and then we just repeat this until the initial for-loop (that goes through each memory section of the process) at the line `for (size_t i = 0; i < args.num; i++)` has finished.

2. Once all memory sections have been scanned, we then execute a loop where we use the array containing the 10000 separate uint64_t-based address values to send back the addresses one by one using the following line: `net_send_data(fd, <current address to be sent here> sizeof(uint64_t));` until all valid addresses have been sent back.

3. Then we send back the end flag.
