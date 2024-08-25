#pragma once
#ifndef _COMPARE_TYPES_H
#define _COMPARE_TYPES_H

#include "proc.h"

// Make sure the TRUE macro has not been defined yet
#ifndef TRUE
#define TRUE 1
#endif

// Make sure the FALSE macro has not been defined yet
#ifndef FALSE
#define FALSE 0
#endif

typedef unsigned char BYTE;
int compare_value_exact(BYTE *pScanValue, BYTE *pMemoryValue, size_t valTypeLength) ;
int compare_value_fuzzy(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue) ;
int compare_value_unchanged(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue);
int compare_value_changed(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue);
int compare_value_decreased_by(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue);
int compare_value_decreased(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue);
int compare_value_increased_by(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue);
int compare_value_increased(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue);
int compare_value_between(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue);
int compare_value_smaller_than(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue);
int compare_value_bigger_than(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue);

#endif