#ifndef HPP_TYPES
#define HPP_TYPES

#include <string>
#include <sstream>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

// Multi-precision number (256-bit)
typedef struct {
	cl_uint d[8];
} mp_number;

// Elliptic curve point
typedef struct {
	mp_number x;
	mp_number y;
} point;

// Result structure for found addresses
typedef struct {
	cl_uint found;
	cl_uint foundId;
	cl_uchar foundHash[20];
} result;

// Ethereum hash structure
typedef struct {
	cl_uint d[17];
} ethhash;

// Note: toString is defined in lexical_cast.hpp
// Removed duplicate definition to avoid ambiguous calls

#endif /* HPP_TYPES */
