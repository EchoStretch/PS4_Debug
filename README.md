# PS4Debug (A PS4 Debugging Payload)
This repository contains my custom version or fork of GiantPluto's FW 6.72 port of the ps4debug payload. I am the sole maintainer of this version and will be responsible for adding new features, pushing code improvements, fixes, etc. 

A huge objective/goal is to make this PS4Debug Source Code firmware-agnostic, meaning that the ps4debug.bin payload compiled from this Source Code will be compatible with the following system firmwares: 5.05, 6.72, 7.02, 7.55, and 9.00. Eliminating the need of having 5 seperate source codes, one for each firmware version.

## TODOs and News

### Recreating ctn123's Console Scan Feature

Developer [ctn123](https://github.com/ctn123) described the console scan feature as essentially "Read memory, compare bytes, rinse, repeat," and I assume that once all process memory sections have been scanned, data is sent back. Based on this description, I've outlined a plan to replicate this functionality:

1. **Memory Comparison:** Allocate a buffer using the `pfmalloc` (malloc with prefaulting) function to hold a maximum of 10,000 (uint64_t-based) memory addresses. If the comparing function `CompareProcScanValues` succeeds, append the memory address to the buffer. Repeat this process until all memory sections of the process have been scanned.

2. **Data Transmission:** Once all memory sections have been scanned, loop through the array containing the uint64_t-based address values. Send back the addresses one by one using the `net_send_data` function until all valid addresses have been transmitted.

3. **End Flag:** Send back an end flag to mark the completion of data transmission.

Check out [debugger/source/proc.c](https://github.com/a0zhar/PS4DebugV2/blob/9022062adf644a9f63bd490e5db00e96f3dedc3a/debugger/source/proc.c#L352) to view the current implementation of this feature.

### Instructions for using Console Scan
You need to send the value of 0xBDAA000D to the PS4Debug server if you want it to use the `proc_console_scan_handle` function instead of `proc_scan_handle`
