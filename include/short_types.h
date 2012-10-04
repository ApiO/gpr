#ifndef SHORT_TYPES_H
#define SHORT_TYPES_H

#include "pstdint.h"

typedef size_t   SZ;

typedef uint8_t  U8;
typedef  int8_t  I8;
typedef uint16_t U16;
typedef  int16_t I16;
typedef uint32_t U32;
typedef  int32_t I32;
typedef uint64_t U64;
typedef  int64_t I64;

typedef float    F32;
typedef double   F64;

// endian swapping functions

static U16 swapU16(U16 value)
{
  return ((value & 0x00FF) << 8) 
       | ((value & 0xFF00) >> 8);
}

static U32 swapU32(U32 value)
{
  return ((value & 0x000000FF) << 24)
       | ((value & 0x0000FF00) << 8)
       | ((value & 0x00FF0000) >> 8)
       | ((value & 0xFF000000) >> 24);
}

static U64 swapU64(U64 value)
{
  return ((value & 0x00000000000000FF) << 56)
       | ((value & 0x000000000000FF00) << 40)
       | ((value & 0x0000000000FF0000) << 24)
       | ((value & 0x00000000FF000000) << 8)
       | ((value & 0x000000FF00000000) >> 8)
       | ((value & 0x0000FF0000000000) >> 24)
       | ((value & 0x00FF000000000000) >> 40)
       | ((value & 0xFF00000000000000) >> 56);
}

static F32 swapF32(F32 value)
{
  union U32F32
  {
    U32 asU32;
    F32 asF32;
  } u;

  u.asF32 = value;
  u.asU32 = swapU32(u.asU32);

  return u.asF32;
}

static F64 swapF64(F64 value)
{
  union U64F64
  {
    U64 asU64;
    F64 asF64;
  } u;

  u.asF64 = value;
  u.asU64 = swapU64(u.asU64);

  return u.asF64;
}

#endif // SHORT_TYPES_H
