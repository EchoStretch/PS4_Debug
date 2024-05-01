//
// This header file, contains macro-constants, and values, etc.
//

#pragma once
#ifndef _DBG_TYPES_HH
#define _DBG_TYPES_HH

//
// int-based boolean type values
//

// Check if TRUE has not been defined before,
// if not, define it.
#ifndef TRUE
#define TRUE 1
#endif

// Check if FALSE has not been defined before,
// if not, define it.
#ifndef FALSE
#define FALSE 0
#endif


//
// Server related
//

#ifndef START_SERVER_ERR
#define START_SERVER_ERR -3
#endif

#endif
