#pragma once
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "../../ps4-payload-sdk/include/types.h"
#include "../../ps4-payload-sdk/include/network.h"
#include "./pkt_sizes.h"
#include "./errno.h"
#include "./kdbg.h"


#define PACKET_VERSION          "1.2"
#define PACKET_MAGIC            0xFFAABBCC

#define CMD_VERSION             0xBD000001

#define CMD_PROC_LIST           0xBDAA0001
#define CMD_PROC_READ           0xBDAA0002
#define CMD_PROC_WRITE          0xBDAA0003
#define CMD_PROC_MAPS           0xBDAA0004
#define CMD_PROC_INTALL         0xBDAA0005
#define CMD_PROC_CALL           0xBDAA0006
#define CMD_PROC_ELF            0xBDAA0007
#define CMD_PROC_PROTECT        0xBDAA0008
#define CMD_PROC_SCAN           0xBDAA0009
#define CMD_PROC_CONSOLE_BASED_SCAN 0x38192
#define CMD_PROC_INFO           0xBDAA000A
#define CMD_PROC_ALLOC          0xBDAA000B
#define CMD_PROC_FREE           0xBDAA000C

#define CMD_DEBUG_ATTACH        0xBDBB0001
#define CMD_DEBUG_DETACH        0xBDBB0002
#define CMD_DEBUG_BREAKPT       0xBDBB0003
#define CMD_DEBUG_WATCHPT       0xBDBB0004
#define CMD_DEBUG_THREADS       0xBDBB0005
#define CMD_DEBUG_STOPTHR       0xBDBB0006
#define CMD_DEBUG_RESUMETHR     0xBDBB0007
#define CMD_DEBUG_GETREGS       0xBDBB0008
#define CMD_DEBUG_SETREGS       0xBDBB0009
#define CMD_DEBUG_GETFPREGS     0xBDBB000A
#define CMD_DEBUG_SETFPREGS     0xBDBB000B
#define CMD_DEBUG_GETDBGREGS    0xBDBB000C
#define CMD_DEBUG_SETDBGREGS    0xBDBB000D
#define CMD_DEBUG_STOPGO        0xBDBB0010
#define CMD_DEBUG_THRINFO       0xBDBB0011
#define CMD_DEBUG_SINGLESTEP    0xBDBB0012

#define CMD_KERN_BASE           0xBDCC0001
#define CMD_KERN_READ           0xBDCC0002
#define CMD_KERN_WRITE          0xBDCC0003

#define CMD_CONSOLE_REBOOT      0xBDDD0001
#define CMD_CONSOLE_END         0xBDDD0002
#define CMD_CONSOLE_PRINT       0xBDDD0003
#define CMD_CONSOLE_NOTIFY      0xBDDD0004
#define CMD_CONSOLE_INFO        0xBDDD0005

#define VALID_CMD(cmd)          (((cmd & 0xFF000000) >> 24) == 0xBD)
#define VALID_PROC_CMD(cmd)     (((cmd & 0x00FF0000) >> 16) == 0xAA)
#define VALID_DEBUG_CMD(cmd)    (((cmd & 0x00FF0000) >> 16) == 0xBB)
#define VALID_KERN_CMD(cmd)     (((cmd & 0x00FF0000) >> 16) == 0xCC)
#define VALID_CONSOLE_CMD(cmd)  (((cmd & 0x00FF0000) >> 16) == 0xDD)

#define CMD_SUCCESS              0x80000000
#define CMD_ERROR                0xF0000001
#define CMD_TOO_MUCH_DATA        0xF0000002
#define CMD_DATA_NULL            0xF0000003
#define CMD_ALREADY_DEBUG        0xF0000004
#define CMD_INVALID_INDEX        0xF0000005

#define CMD_FATAL_STATUS(s) ((s >> 28) == 15)

#define CMD_PACKET_SIZE 12


// Packet structure for communication between debugger and target
struct cmd_packet {
    uint32_t magic;     // Magic number identifying the packet
    uint32_t cmd;       // Command identifier
    uint32_t datalen;   // Length of data payload
    void *data;         // Pointer to the data payload (not part of the packet, added after)
} __attribute__((packed));

// Packet for reading data from a process
struct cmd_proc_read_packet {
    uint32_t pid;       // Process ID
    uint64_t address;   // Address to read from
    uint32_t length;    // Length of data to read
} __attribute__((packed));

// Packet for writing data to a process
struct cmd_proc_write_packet {
    uint32_t pid;       // Process ID
    uint64_t address;   // Address to write to
    uint32_t length;    // Length of data to write
} __attribute__((packed));

// Packet for retrieving memory maps of a process
struct cmd_proc_maps_packet {
    uint32_t pid;       // Process ID
} __attribute__((packed));

// Packet for installing a (hook?) in a process
struct cmd_proc_install_packet {
    uint32_t pid;       // Process ID
} __attribute__((packed));

// Response structure for usage of the install packet
struct cmd_proc_install_response {
    uint64_t rpcstub;   // RPC (Remote procedure call) stub
} __attribute__((packed));

// Packet for making a remote procedure call (RPC) to a process
struct cmd_proc_call_packet {
    uint32_t pid;       // Process ID
    uint64_t rpcstub;   // RPC (Remote procedure call) stub
    uint64_t rpc_rip;   // RPC (Remote procedure call) rip
    uint64_t rpc_rdi;   // RPC (Remote procedure call) rdi
    uint64_t rpc_rsi;   // RPC (Remote procedure call) rsi
    uint64_t rpc_rdx;   // RPC (Remote procedure call) rdx
    uint64_t rpc_rcx;   // RPC (Remote procedure call) rcx
    uint64_t rpc_r8;    // RPC (Remote procedure call) r8
    uint64_t rpc_r9;    // RPC (Remote procedure call) r9
} __attribute__((packed));

// Response structure for after having made an RPC
struct cmd_proc_call_response {
    uint32_t pid;       // Process ID
    uint64_t rpc_rax;   // RPC (Remote procedure call) rax
} __attribute__((packed));

// Packet for loading an ELF file into a process
struct cmd_proc_elf_packet {
    uint32_t pid;       // Process ID
    uint32_t length;    // Length
} __attribute__((packed));


// Packet for changing memory protection of a region in a process
struct cmd_proc_protect_packet {
    uint32_t pid;       // Process ID
    uint64_t address;   // Address
    uint32_t length;    // Length
    uint32_t newprot;   // New protection
} __attribute__((packed));

// Enumeration for value types used in process scanning
 enum cmd_proc_scan_valuetype {
    valTypeUInt8 = 0,
    valTypeInt8,
    valTypeUInt16,
    valTypeInt16,
    valTypeUInt32,
    valTypeInt32,
    valTypeUInt64,
    valTypeInt64,
    valTypeFloat,
    valTypeDouble,
    valTypeArrBytes,
    valTypeString
} __attribute__((__packed__));

// Enumeration for comparison types used in process scanning
enum cmd_proc_scan_comparetype {
    cmpTypeExactValue = 0,
    cmpTypeFuzzyValue,
    cmpTypeBiggerThan,
    cmpTypeSmallerThan,
    cmpTypeValueBetween,
    cmpTypeIncreasedValue,
    cmpTypeIncreasedValueBy,
    cmpTypeDecreasedValue,
    cmpTypeDecreasedValueBy,
    cmpTypeChangedValue,
    cmpTypeUnchangedValue,
    cmpTypeUnknownInitialValue
} __attribute__((__packed__));

// Structure for packet used in process scanning
struct cmd_proc_scan_packet {
    uint32_t pid;           // Process ID
    uint8_t valueType;      // Value type
    uint8_t compareType;    // Comparison type
    uint32_t lenData;       // Length of data
} __attribute__((packed));

// Structure for requesting information about a process
struct cmd_proc_info_packet {
    uint32_t pid;   // Process ID
} __attribute__((packed));

// Response structure for process information request
struct cmd_proc_info_response {
    uint32_t pid;           // Process ID
    char name[40];          // Process name
    char path[64];          // Process path
    char titleid[16];       // Process title ID
    char contentid[64];     // Process content ID
} __attribute__((packed));

// Structure for requesting memory allocation in a process
struct cmd_proc_alloc_packet {
    uint32_t pid;       // Process ID
    uint32_t length;    // Length
} __attribute__((packed));

// Response structure for memory allocation request
struct cmd_proc_alloc_response {
    uint64_t address;   // Address
} __attribute__((packed));


// Structure for requesting memory deallocation in a process
struct cmd_proc_free_packet {
    uint32_t pid;       // Process ID
    uint64_t address;   // Address
    uint32_t length;    // Length
} __attribute__((packed));


// Structure for attaching debugger to a process
struct cmd_debug_attach_packet {
    uint32_t pid;   // Process ID
} __attribute__((packed));


// Structure for setting breakpoints
struct cmd_debug_breakpt_packet {
    uint32_t index;     // Index
    uint32_t enabled;   // Enabled
    uint64_t address;   // Address
} __attribute__((packed));


// Structure for setting watchpoints
struct cmd_debug_watchpt_packet {
    uint32_t index;     // Index
    uint32_t enabled;   // Enabled
    uint32_t length;    // Length
    uint32_t breaktype; // Break type
    uint64_t address;   // Address
} __attribute__((packed));

// Structure for stopping a thread in debugging mode
struct cmd_debug_stopthr_packet {
    uint32_t lwpid;     // LWP ID
} __attribute__((packed));


// Structure for resuming a thread in debugging mode
struct cmd_debug_resumethr_packet {
    uint32_t lwpid;     // LWP ID
} __attribute__((packed));

// Structure for requesting register values of a thread in debugging mode
struct cmd_debug_getregs_packet {
    uint32_t lwpid;     // LWP ID
} __attribute__((packed));

// Structure for setting register values of a thread in debugging mode
struct cmd_debug_setregs_packet {
    uint32_t lwpid;     // LWP ID
    uint32_t length;    // Length
} __attribute__((packed));

// Structure for stopping or resuming debugging operations
struct cmd_debug_stopgo_packet {
    uint32_t stop;      // Stop
} __attribute__((packed));


// Structure for requesting thread information in debugging mode
struct cmd_debug_thrinfo_packet {
    uint32_t lwpid;     // LWP ID
} __attribute__((packed));

// Response structure for thread information request
struct cmd_debug_thrinfo_response {
    uint32_t lwpid;     // LWP ID
    uint32_t priority;  // Priority
    char name[32];      // Name
} __attribute__((packed));

// Structure for reading kernel memory
struct cmd_kern_read_packet {
    uint64_t address;   // Address
    uint32_t length;    // Length
} __attribute__((packed));

// Structure for writing to kernel memory
struct cmd_kern_write_packet {
    uint64_t address;   // Address
    uint32_t length;    // Length
} __attribute__((packed));

// Structure for printing messages to console
struct cmd_console_print_packet {
    uint32_t length;    // Length
} __attribute__((packed));

// Structure for sending notifications to console
struct cmd_console_notify_packet {
    uint32_t messageType;   // Message type
    uint32_t length;        // Length
} __attribute__((packed));

// Response structure for console information request
struct cmd_console_info_response {
    // todo
} __attribute__((packed));


#define MAX_BREAKPOINTS 30
#define MAX_WATCHPOINTS 4
struct debug_breakpoint {
    uint32_t enabled;   // Enabled
    uint64_t address;   // Address
    uint8_t original;   // Original
};

struct debug_watchpoint {
    uint32_t enabled;   // Enabled
    uint64_t address;   // Address
    uint8_t breaktype;  // Break type
    uint8_t length;     // Length
};

struct debug_context {
    int pid;    // Process ID
    int dbgfd;  // Debug file descriptor
    struct debug_breakpoint breakpoints[MAX_BREAKPOINTS]; // Breakpoints
    // XXX: use actual __dbreg64 structure please
    struct {
        uint64_t dr[16];
    } watchdata; // Watch data
};

struct server_client {
    int id; // ID
    int fd; // File descriptor
    int debugging;  // Debugging
    struct sockaddr_in client;  // Client address
    struct debug_context dbgctx;    // Debug context
};

#endif