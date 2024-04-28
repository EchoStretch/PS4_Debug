// golden
// 6/12/2018
//

#include "../include/../include/proc.h"
#include "../include/../include/compare.h"

int proc_list_handle(int fd, struct cmd_packet *packet) {
   void *data;
   uint64_t num;
   uint32_t length;

   sys_proc_list(NULL, &num);

   if (num > 0) {
      length = sizeof(struct proc_list_entry) * num;
      data = pfmalloc(length);
      if (!data) {
         net_send_status(fd, CMD_DATA_NULL);
         return 1;
      }

      sys_proc_list(data, &num);

      net_send_status(fd, CMD_SUCCESS);
      net_send_data(fd, &num, sizeof(uint32_t));
      net_send_data(fd, data, length);

      free(data);

      return 0;
   }

   net_send_status(fd, CMD_DATA_NULL);
   return 1;
}

int proc_read_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_read_packet *rp;
   void *data;
   uint64_t left;
   uint64_t address;

   rp = (struct cmd_proc_read_packet *)packet->data;

   if (rp) {
       // allocate a small buffer
      data = pfmalloc(NET_MAX_LENGTH);
      if (!data) {
         net_send_status(fd, CMD_DATA_NULL);
         return 1;
      }

      net_send_status(fd, CMD_SUCCESS);

      left = rp->length;
      address = rp->address;

      // send by chunks
      while (left > 0) {
         memset(data, NULL, NET_MAX_LENGTH);

         if (left > NET_MAX_LENGTH) {
            sys_proc_rw(rp->pid, address, data, NET_MAX_LENGTH, 0);
            net_send_data(fd, data, NET_MAX_LENGTH);

            address += NET_MAX_LENGTH;
            left -= NET_MAX_LENGTH;
         } else {
            sys_proc_rw(rp->pid, address, data, left, 0);
            net_send_data(fd, data, left);

            address += left;
            left -= left;
         }
      }

      free(data);

      return 0;
   }

   net_send_status(fd, CMD_DATA_NULL);

   return 1;
}

int proc_write_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_write_packet *wp;
   void *data;
   uint64_t left;
   uint64_t address;

   wp = (struct cmd_proc_write_packet *)packet->data;

   if (wp) {
       // only allocate a small buffer
      data = pfmalloc(NET_MAX_LENGTH);
      if (!data) {
         net_send_status(fd, CMD_DATA_NULL);
         return 1;
      }

      net_send_status(fd, CMD_SUCCESS);

      left = wp->length;
      address = wp->address;

      // write in chunks
      while (left > 0) {
         if (left > NET_MAX_LENGTH) {
            net_recv_data(fd, data, NET_MAX_LENGTH, 1);
            sys_proc_rw(wp->pid, address, data, NET_MAX_LENGTH, 1);

            address += NET_MAX_LENGTH;
            left -= NET_MAX_LENGTH;
         } else {
            net_recv_data(fd, data, left, 1);
            sys_proc_rw(wp->pid, address, data, left, 1);

            address += left;
            left -= left;
         }
      }

      net_send_status(fd, CMD_SUCCESS);

      free(data);

      return 0;
   }

   net_send_status(fd, CMD_DATA_NULL);

   return 1;
}

int proc_maps_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_maps_packet *mp;
   struct sys_proc_vm_map_args args;
   uint32_t size;
   uint32_t num;

   mp = (struct cmd_proc_maps_packet *)packet->data;

   if (mp) {
      memset(&args, NULL, sizeof(args));

      if (sys_proc_cmd(mp->pid, SYS_PROC_VM_MAP, &args)) {
         net_send_status(fd, CMD_ERROR);
         return 1;
      }

      size = args.num * sizeof(struct proc_vm_map_entry);

      args.maps = (struct proc_vm_map_entry *)pfmalloc(size);
      if (!args.maps) {
         net_send_status(fd, CMD_DATA_NULL);
         return 1;
      }

      if (sys_proc_cmd(mp->pid, SYS_PROC_VM_MAP, &args)) {
         free(args.maps);
         net_send_status(fd, CMD_ERROR);
         return 1;
      }

      net_send_status(fd, CMD_SUCCESS);
      num = (uint32_t)args.num;
      net_send_data(fd, &num, sizeof(uint32_t));
      net_send_data(fd, args.maps, size);

      free(args.maps);

      return 0;
   }

   net_send_status(fd, CMD_ERROR);

   return 1;
}

int proc_install_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_install_packet *ip;
   struct sys_proc_install_args args;
   struct cmd_proc_install_response resp;

   ip = (struct cmd_proc_install_packet *)packet->data;

   if (!ip) {
      net_send_status(fd, CMD_DATA_NULL);
      return 1;
   }

   args.stubentryaddr = NULL;
   sys_proc_cmd(ip->pid, SYS_PROC_INSTALL, &args);

   if (!args.stubentryaddr) {
      net_send_status(fd, CMD_DATA_NULL);
      return 1;
   }

   resp.rpcstub = args.stubentryaddr;

   net_send_status(fd, CMD_SUCCESS);
   net_send_data(fd, &resp, CMD_PROC_INSTALL_RESPONSE_SIZE);

   return 0;
}

int proc_call_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_call_packet *cp;
   struct sys_proc_call_args args;
   struct cmd_proc_call_response resp;

   cp = (struct cmd_proc_call_packet *)packet->data;

   if (!cp) {
      net_send_status(fd, CMD_DATA_NULL);
      return 1;
   }

   // copy over the arguments for the call
   args.pid = cp->pid;
   args.rpcstub = cp->rpcstub;
   args.rax = NULL;
   args.rip = cp->rpc_rip;
   args.rdi = cp->rpc_rdi;
   args.rsi = cp->rpc_rsi;
   args.rdx = cp->rpc_rdx;
   args.rcx = cp->rpc_rcx;
   args.r8 = cp->rpc_r8;
   args.r9 = cp->rpc_r9;

   sys_proc_cmd(cp->pid, SYS_PROC_CALL, &args);

   resp.pid = cp->pid;
   resp.rpc_rax = args.rax;

   net_send_status(fd, CMD_SUCCESS);
   net_send_data(fd, &resp, CMD_PROC_CALL_RESPONSE_SIZE);

   return 0;
}

int proc_elf_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_elf_packet *ep;
   struct sys_proc_elf_args args;
   void *elf;

   ep = (struct cmd_proc_elf_packet *)packet->data;

   if (ep) {
      elf = pfmalloc(ep->length);
      if (!elf) {
         net_send_status(fd, CMD_DATA_NULL);
         return 1;
      }

      net_send_status(fd, CMD_SUCCESS);

      net_recv_data(fd, elf, ep->length, 1);

      args.elf = elf;

      if (sys_proc_cmd(ep->pid, SYS_PROC_ELF, &args)) {
         free(elf);
         net_send_status(fd, CMD_ERROR);
         return 1;
      }

      free(elf);

      net_send_status(fd, CMD_SUCCESS);

      return 0;
   }

   net_send_status(fd, CMD_ERROR);

   return 1;
}

int proc_protect_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_protect_packet *pp;
   struct sys_proc_protect_args args;

   pp = (struct cmd_proc_protect_packet *)packet->data;

   if (pp) {
      args.address = pp->address;
      args.length = pp->length;
      args.prot = pp->newprot;
      sys_proc_cmd(pp->pid, SYS_PROC_PROTECT, &args);

      net_send_status(fd, CMD_SUCCESS);
   }

   net_send_status(fd, CMD_DATA_NULL);

   return 0;
}

size_t GetSizeOfProcScanValue(enum cmd_proc_scan_valuetype valType) {
   switch (valType) {
      case valTypeUInt8:
      case valTypeInt8:
         return 1;
      case valTypeUInt16:
      case valTypeInt16:
         return 2;
      case valTypeUInt32:
      case valTypeInt32:
      case valTypeFloat:
         return 4;
      case valTypeUInt64:
      case valTypeInt64:
      case valTypeDouble:
         return 8;
      case valTypeArrBytes:
      case valTypeString:
      default:
         return NULL;
   }
}

int CompareProcScanValues(enum cmd_proc_scan_comparetype cmpType, enum cmd_proc_scan_valuetype valType, size_t valTypeLength, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue) {
   switch (cmpType) {
      case cmpTypeExactValue:          return compare_value_exact(pScanValue, pMemoryValue, valTypeLength);
      case cmpTypeFuzzyValue:          return compare_value_fuzzy(pScanValue, pMemoryValue, valTypeLength);
      case cmpTypeBiggerThan:          return compare_value_bigger_than(valType, pScanValue, pMemoryValue);
      case cmpTypeSmallerThan:         return compare_value_smaller_than(valType, pScanValue, pMemoryValue);
      case cmpTypeValueBetween:        return compare_value_between(valType, pScanValue, pMemoryValue, pExtraValue);
      case cmpTypeIncreasedValue:      return compare_value_increased(valType, pScanValue, pMemoryValue, pExtraValue);
      case cmpTypeIncreasedValueBy:    return compare_value_increased_by(valType, pScanValue, pMemoryValue, pExtraValue);
      case cmpTypeDecreasedValue:      return compare_value_decreased(valType, pScanValue, pMemoryValue, pExtraValue);
      case cmpTypeDecreasedValueBy:    return compare_value_decreasedy_by(valType, pScanValue, pMemoryValue, pExtraValue);
      case cmpTypeChangedValue:        return compare_value_changed(valType, pScanValue, pMemoryValue, pExtraValue);
      case cmpTypeUnchangedValue:      return compare_value_unchanged(valType, pScanValue, pMemoryValue, pExtraValue);
      case cmpTypeUnknownInitialValue: return TRUE;
   };

   return FALSE;
}


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

int proc_scan_handle(int fd, struct cmd_packet *packet) {
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

   // Loop through each memory section of the process
   for (size_t i = 0; i < args.num; i++) {
      // Skip sections that cannot be read
      if ((args.maps[i].prot & PROT_READ) != PROT_READ) {
         continue;
      }

      // Calculate section start address and length
      uint64_t sectionStartAddr = args.maps[i].start;
      size_t sectionLen = args.maps[i].end - sectionStartAddr;

      // Iterate through the memory section
      for (uint64_t j = 0; j < sectionLen; j += valueLength) {
         // If the current offset is at a page boundary, read the next page
         if (j == 0 || !(j % PAGE_SIZE)) {
            sys_proc_rw(sp->pid, sectionStartAddr, scanBuffer, PAGE_SIZE, 0);
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
            // Send the address of a matching memory offset
            net_send_data(fd, &curAddress, sizeof(uint64_t));
         }
      }
   }

   // Notify the end of the scanning process
   uprintf("scan done");

   // Send an end flag to mark the end of data transmission
   uint64_t endflag = 0xFFFFFFFFFFFFFFFF;
   net_send_data(fd, &endflag, sizeof(uint64_t));

   // Free allocated memory
   free(scanBuffer);
   free(args.maps);
   free(data);

   return 0;
}


int proc_info_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_info_packet *ip;
   struct sys_proc_info_args args;
   struct cmd_proc_info_response resp;

   ip = (struct cmd_proc_info_packet *)packet->data;

   if (ip) {
      sys_proc_cmd(ip->pid, SYS_PROC_INFO, &args);

      resp.pid = args.pid;
      memcpy(resp.name, args.name, sizeof(resp.name));
      memcpy(resp.path, args.path, sizeof(resp.path));
      memcpy(resp.titleid, args.titleid, sizeof(resp.titleid));
      memcpy(resp.contentid, args.contentid, sizeof(resp.contentid));

      net_send_status(fd, CMD_SUCCESS);
      net_send_data(fd, &resp, CMD_PROC_INFO_RESPONSE_SIZE);
      return 0;
   }

   net_send_status(fd, CMD_DATA_NULL);

   return 0;
}

int proc_alloc_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_alloc_packet *ap;
   struct sys_proc_alloc_args args;
   struct cmd_proc_alloc_response resp;

   ap = (struct cmd_proc_alloc_packet *)packet->data;

   if (ap) {
      args.length = ap->length;
      sys_proc_cmd(ap->pid, SYS_PROC_ALLOC, &args);

      resp.address = args.address;

      net_send_status(fd, CMD_SUCCESS);
      net_send_data(fd, &resp, CMD_PROC_ALLOC_RESPONSE_SIZE);
      return 0;
   }

   net_send_status(fd, CMD_DATA_NULL);

   return 0;
}

int proc_free_handle(int fd, struct cmd_packet *packet) {
   struct cmd_proc_free_packet *fp;
   struct sys_proc_free_args args;

   fp = (struct cmd_proc_free_packet *)packet->data;

   if (fp) {
      args.address = fp->address;
      args.length = fp->length;
      sys_proc_cmd(fp->pid, SYS_PROC_FREE, &args);

      net_send_status(fd, CMD_SUCCESS);
      return 0;
   }

   net_send_status(fd, CMD_DATA_NULL);

   return 0;
}

int proc_handle(int fd, struct cmd_packet *packet) {
   switch (packet->cmd) {
      case CMD_PROC_LIST:
         return proc_list_handle(fd, packet);
      case CMD_PROC_READ:
         return proc_read_handle(fd, packet);
      case CMD_PROC_WRITE:
         return proc_write_handle(fd, packet);
      case CMD_PROC_MAPS:
         return proc_maps_handle(fd, packet);
      case CMD_PROC_INTALL:
         return proc_install_handle(fd, packet);
      case CMD_PROC_CALL:
         return proc_call_handle(fd, packet);
      case CMD_PROC_ELF:
         return proc_elf_handle(fd, packet);
      case CMD_PROC_PROTECT:
         return proc_protect_handle(fd, packet);
      case CMD_PROC_SCAN:
         return proc_scan_handle(fd, packet);
      case CMD_PROC_INFO:
         return proc_info_handle(fd, packet);
      case CMD_PROC_ALLOC:
         return proc_alloc_handle(fd, packet);
      case CMD_PROC_FREE:
         return proc_free_handle(fd, packet);
   }

   return 1;
}
