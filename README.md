# PS4Debug (A PS4 Debugging Payload)
This repository contains my custom version or fork of EchoStretch FW 5.05, 6.72, and 7.00 to 11.00 port of the ps4debug payload. I am the sole maintainer of this version and will be responsible for adding new features, pushing code improvements, fixes, etc. 

## TODOs and News

### Recreating ctn123's Console Scan Feature
<details>
<summary>See Explaination + Code</summary>

Developer [ctn123](https://github.com/ctn123) described the console scan feature as essentially "Read memory, compare bytes, rinse, repeat," and I assume that once all process memory sections have been scanned, data is sent back. Based on this description, I've outlined a plan to replicate this functionality:

1. **Memory Comparison:** Allocate a buffer using the `pfmalloc` (malloc with prefaulting) function to hold a maximum of 10,000 (uint64_t-based) memory addresses. If the comparing function `CompareProcScanValues` succeeds, append the memory address to the buffer. Repeat this process until all memory sections of the process have been scanned.

2. **Data Transmission:** Once all memory sections have been scanned, loop through the array containing the uint64_t-based address values. Send back the addresses one by one using the `net_send_data` function until all valid addresses have been transmitted.

3. **End Flag:** Send back an end flag to mark the completion of data transmission.

Check out [debugger/source/proc.c](https://github.com/a0zhar/PS4DebugV2/blob/9022062adf644a9f63bd490e5db00e96f3dedc3a/debugger/source/proc.c#L352) to view the current implementation of this feature.

</details>

<details>
<summary>See Usage Instructions</summary>

You need to send the value of **0xBDAA000D** to the PS4Debug server if you want it to use the `proc_console_scan_handle` function instead of `proc_scan_handle`
</details>



## Credits
- [jogolden](https://github.com/jogolden/ps4debug) - for originally creating this
- [DeathRGH](https://github.com/DeathRGH/frame4) - for multi fw example
- [BestPig](https://github.com/BestPig) - Help with offsets
- [EchoStretch](https://github.com/EchoStretch/ps4debug) - Putting it all together
