#include "../include/compare.h"

int compare_value_exact(BYTE *pScanValue, BYTE *pMemoryValue, size_t valTypeLength) {
    int isFound = FALSE;
    for (size_t j = 0; j < valTypeLength - 1; j++) {
        isFound = (pScanValue[j] == pMemoryValue[j]);
        if (!isFound)
            break;
    }

    return isFound;
}

int compare_value_fuzzy(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue) {
    if (valType == valTypeFloat) {
        float diff = *(float *)pScanValue - *(float *)pMemoryValue;
        return diff < 1.0f && diff > -1.0f;
    } else if (valType == valTypeDouble) {
        double diff = *(double *)pScanValue - *(double *)pMemoryValue;
        return diff < 1.0 && diff > -1.0;
    }

    return FALSE;
}

int compare_value_bigger_than(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue) {
    switch (valType) {
        case valTypeUInt8:      return *pMemoryValue > *pScanValue;
        case valTypeInt8:       return *(int8_t *)pMemoryValue > *(int8_t *)pScanValue;
        case valTypeUInt16:     return *(uint16_t *)pMemoryValue > *(uint16_t *)pScanValue;
        case valTypeInt16:      return *(int16_t *)pMemoryValue > *(int16_t *)pScanValue;
        case valTypeUInt32:     return *(uint32_t *)pMemoryValue > *(uint32_t *)pScanValue;
        case valTypeInt32:      return *(int32_t *)pMemoryValue > *(int32_t *)pScanValue;
        case valTypeUInt64:     return *(uint64_t *)pMemoryValue > *(uint64_t *)pScanValue;
        case valTypeInt64:      return *(int64_t *)pMemoryValue > *(int64_t *)pScanValue;
        case valTypeFloat:      return *(float *)pMemoryValue > *(float *)pScanValue;
        case valTypeDouble:     return *(double *)pMemoryValue > *(double *)pScanValue;
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}

int compare_value_smaller_than(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue) {
    switch (valType) {
        case valTypeUInt8:      return *pMemoryValue < *pScanValue;
        case valTypeInt8:       return *(int8_t *)pMemoryValue < *(int8_t *)pScanValue;
        case valTypeUInt16:     return *(uint16_t *)pMemoryValue < *(uint16_t *)pScanValue;
        case valTypeInt16:      return *(int16_t *)pMemoryValue < *(int16_t *)pScanValue;
        case valTypeUInt32:     return *(uint32_t *)pMemoryValue < *(uint32_t *)pScanValue;
        case valTypeInt32:      return *(int32_t *)pMemoryValue < *(int32_t *)pScanValue;
        case valTypeUInt64:     return *(uint64_t *)pMemoryValue < *(uint64_t *)pScanValue;
        case valTypeInt64:      return *(int64_t *)pMemoryValue < *(int64_t *)pScanValue;
        case valTypeFloat:      return *(float *)pMemoryValue < *(float *)pScanValue;
        case valTypeDouble:     return *(double *)pMemoryValue < *(double *)pScanValue;
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}


// Doesn't include casting
#define COMPARE_BETWEEN_HELPER_REGULAR(ScanValue, MemValue, ExtraValue) \
    if (*ExtraValue > *ScanValue)                                       \
        return *MemValue > *ScanValue && *MemValue < *ExtraValue;       \
    else                                                                \
        return *MemValue < *ScanValue && *MemValue > *ExtraValue;  

// Supports a custom provided variable type to use for casting
#define COMPARE_BETWEEN_HELPER_CAST(type, ScanValue, MemValue, ExtraValue)                            \
    if (*((type*)ExtraValue) > *((type*)ScanValue))                                                   \
        return *((type*)MemValue) > *((type*)ScanValue) && *((type*)MemValue) < *((type*)ExtraValue); \
    else                                                                                              \
        return *((type*)MemValue) < *((type*)ScanValue) && *((type*)MemValue) > *((type*)ExtraValue);


int compare_value_between(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue) {
    switch (valType) {
        case valTypeUInt8:  COMPARE_BETWEEN_HELPER_REGULAR(pScanValue, pMemoryValue, pExtraValue);
        case valTypeInt8:   COMPARE_BETWEEN_HELPER_CAST(int8_t, pScanValue, pMemoryValue, pExtraValue);
        case valTypeUInt16: COMPARE_BETWEEN_HELPER_CAST(uint16_t, pScanValue, pMemoryValue, pExtraValue);
        case valTypeInt16:  COMPARE_BETWEEN_HELPER_CAST(int16_t, pScanValue, pMemoryValue, pExtraValue);
        case valTypeUInt32: COMPARE_BETWEEN_HELPER_CAST(uint32_t, pScanValue, pMemoryValue, pExtraValue);
        case valTypeInt32:  COMPARE_BETWEEN_HELPER_CAST(int32_t, pScanValue, pMemoryValue, pExtraValue);
        case valTypeUInt64: COMPARE_BETWEEN_HELPER_CAST(uint64_t, pScanValue, pMemoryValue, pExtraValue);
        case valTypeInt64:  COMPARE_BETWEEN_HELPER_CAST(int64_t, pScanValue, pMemoryValue, pExtraValue);
        case valTypeFloat:  COMPARE_BETWEEN_HELPER_CAST(float, pScanValue, pMemoryValue, pExtraValue);
        case valTypeDouble: COMPARE_BETWEEN_HELPER_CAST(double, pScanValue, pMemoryValue, pExtraValue);
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}

int compare_value_increased(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue) {
    switch (valType) {
        case valTypeUInt8:      return *pMemoryValue > *pExtraValue;
        case valTypeInt8:       return *(int8_t *)pMemoryValue > *(int8_t *)pExtraValue;
        case valTypeUInt16:     return *(uint16_t *)pMemoryValue > *(uint16_t *)pExtraValue;
        case valTypeInt16:      return *(int16_t *)pMemoryValue > *(int16_t *)pExtraValue;
        case valTypeUInt32:     return *(uint32_t *)pMemoryValue > *(uint32_t *)pExtraValue;
        case valTypeInt32:      return *(int32_t *)pMemoryValue > *(int32_t *)pExtraValue;
        case valTypeUInt64:     return *(uint64_t *)pMemoryValue > *(uint64_t *)pExtraValue;
        case valTypeInt64:      return *(int64_t *)pMemoryValue > *(int64_t *)pExtraValue;
        case valTypeFloat:      return *(float *)pMemoryValue > *(float *)pExtraValue;
        case valTypeDouble:     return *(double *)pMemoryValue > *(double *)pExtraValue;
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}

int compare_value_increased_by(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue) {
    switch (valType) {
        case valTypeUInt8:      return *pMemoryValue == (*pExtraValue + *pScanValue);
        case valTypeInt8:       return *(int8_t *)pMemoryValue == (*(int8_t *)pExtraValue + *(int8_t *)pScanValue);
        case valTypeUInt16:     return *(uint16_t *)pMemoryValue == (*(uint16_t *)pExtraValue + *(uint16_t *)pScanValue);
        case valTypeInt16:      return *(int16_t *)pMemoryValue == (*(int16_t *)pExtraValue + *(int16_t *)pScanValue);
        case valTypeUInt32:     return *(uint32_t *)pMemoryValue == (*(uint32_t *)pExtraValue + *(uint32_t *)pScanValue);
        case valTypeInt32:      return *(int32_t *)pMemoryValue == (*(int32_t *)pExtraValue + *(int32_t *)pScanValue);
        case valTypeUInt64:     return *(uint64_t *)pMemoryValue == (*(uint64_t *)pExtraValue + *(uint64_t *)pScanValue);
        case valTypeInt64:      return *(int64_t *)pMemoryValue == (*(int64_t *)pExtraValue + *(int64_t *)pScanValue);
        case valTypeFloat:      return *(float *)pMemoryValue == (*(float *)pExtraValue + *(float *)pScanValue);
        case valTypeDouble:     return *(double *)pMemoryValue == (*(double *)pExtraValue + *(float *)pScanValue);
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}

int compare_value_decreased(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue) {
    switch (valType) {
        case valTypeUInt8:      return *pMemoryValue < *pExtraValue;
        case valTypeInt8:       return *(int8_t *)pMemoryValue < *(int8_t *)pExtraValue;
        case valTypeUInt16:     return *(uint16_t *)pMemoryValue < *(uint16_t *)pExtraValue;
        case valTypeInt16:      return *(int16_t *)pMemoryValue < *(int16_t *)pExtraValue;
        case valTypeUInt32:     return *(uint32_t *)pMemoryValue < *(uint32_t *)pExtraValue;
        case valTypeInt32:      return *(int32_t *)pMemoryValue < *(int32_t *)pExtraValue;
        case valTypeUInt64:     return *(uint64_t *)pMemoryValue < *(uint64_t *)pExtraValue;
        case valTypeInt64:      return *(int64_t *)pMemoryValue < *(int64_t *)pExtraValue;
        case valTypeFloat:      return *(float *)pMemoryValue < *(float *)pExtraValue;
        case valTypeDouble:     return *(double *)pMemoryValue < *(double *)pExtraValue;
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}

int compare_value_decreased_by(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue) {
    switch (valType) {
        case valTypeUInt8:      return *pMemoryValue == (*pExtraValue - *pScanValue);
        case valTypeInt8:       return *(int8_t *)pMemoryValue == (*(int8_t *)pExtraValue - *(int8_t *)pScanValue);
        case valTypeUInt16:     return *(uint16_t *)pMemoryValue == (*(uint16_t *)pExtraValue - *(uint16_t *)pScanValue);
        case valTypeInt16:      return *(int16_t *)pMemoryValue == (*(int16_t *)pExtraValue - *(int16_t *)pScanValue);
        case valTypeUInt32:     return *(uint32_t *)pMemoryValue == (*(uint32_t *)pExtraValue - *(uint32_t *)pScanValue);
        case valTypeInt32:      return *(int32_t *)pMemoryValue == (*(int32_t *)pExtraValue - *(int32_t *)pScanValue);
        case valTypeUInt64:     return *(uint64_t *)pMemoryValue == (*(uint64_t *)pExtraValue - *(uint64_t *)pScanValue);
        case valTypeInt64:      return *(int64_t *)pMemoryValue == (*(int64_t *)pExtraValue - *(int64_t *)pScanValue);
        case valTypeFloat:      return *(float *)pMemoryValue == (*(float *)pExtraValue - *(float *)pScanValue);
        case valTypeDouble:     return *(double *)pMemoryValue == (*(double *)pExtraValue - *(float *)pScanValue);
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}

int compare_value_changed(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue) {
    switch (valType) {
        case valTypeUInt8:      return *pMemoryValue != *pExtraValue;
        case valTypeInt8:       return *(int8_t *)pMemoryValue != *(int8_t *)pExtraValue;
        case valTypeUInt16:     return *(uint16_t *)pMemoryValue != *(uint16_t *)pExtraValue;
        case valTypeInt16:      return *(int16_t *)pMemoryValue != *(int16_t *)pExtraValue;
        case valTypeUInt32:     return *(uint32_t *)pMemoryValue != *(uint32_t *)pExtraValue;
        case valTypeInt32:      return *(int32_t *)pMemoryValue != *(int32_t *)pExtraValue;
        case valTypeUInt64:     return *(uint64_t *)pMemoryValue != *(uint64_t *)pExtraValue;
        case valTypeInt64:      return *(int64_t *)pMemoryValue != *(int64_t *)pExtraValue;
        case valTypeFloat:      return *(float *)pMemoryValue != *(float *)pExtraValue;
        case valTypeDouble:     return *(double *)pMemoryValue != *(double *)pExtraValue;
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}

int compare_value_unchanged(enum cmd_proc_scan_valuetype valType, BYTE *pScanValue, BYTE *pMemoryValue, BYTE *pExtraValue) {
    switch (valType) {
        case valTypeUInt8:     return *pMemoryValue == *pExtraValue;
        case valTypeInt8:      return *(int8_t *)pMemoryValue == *(int8_t *)pExtraValue;
        case valTypeUInt16:    return *(uint16_t *)pMemoryValue == *(uint16_t *)pExtraValue;
        case valTypeInt16:     return *(int16_t *)pMemoryValue == *(int16_t *)pExtraValue;
        case valTypeUInt32:    return *(uint32_t *)pMemoryValue == *(uint32_t *)pExtraValue;
        case valTypeInt32:     return *(int32_t *)pMemoryValue == *(int32_t *)pExtraValue;
        case valTypeUInt64:    return *(uint64_t *)pMemoryValue == *(uint64_t *)pExtraValue;
        case valTypeInt64:     return *(int64_t *)pMemoryValue == *(int64_t *)pExtraValue;
        case valTypeFloat:     return *(float *)pMemoryValue == *(float *)pExtraValue;
        case valTypeDouble:    return *(double *)pMemoryValue == *(double *)pExtraValue;
        case valTypeArrBytes:
        case valTypeString:
        default:
            return FALSE;
    };
}