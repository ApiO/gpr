#ifndef GPR_TYPES_H
#define GPR_TYPES_H

#include <stdint.h>

// short type aliases: please use that

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

static U16 gpr_swap_U16(U16 value)
{
  return ((value & 0x00FF) << 8) 
       | ((value & 0xFF00) >> 8);
}

static U32 gpr_swap_U32(U32 value)
{
  return ((value & 0x000000FF) << 24)
       | ((value & 0x0000FF00) << 8)
       | ((value & 0x00FF0000) >> 8)
       | ((value & 0xFF000000) >> 24);
}

static U64 gpr_swap_U64(U64 value)
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

static F32 gpr_swap_F32(F32 value)
{
  union U32F32
  {
    U32 asU32;
    F32 asF32;
  } u;

  u.asF32 = value;
  u.asU32 = gpr_swap_U32(u.asU32);

  return u.asF32;
}

static F64 gpr_swap_F64(F64 value)
{
  union U64F64
  {
    U64 asU64;
    F64 asF64;
  } u;

  u.asF64 = value;
  u.asU64 = gpr_swap_U64(u.asU64);

  return u.asF64;
}

// next power of 2 roundups

static U8 gpr_next_pow2_U8(U8 value)
{
  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  ++value;
  return value;
}

static U16 gpr_next_pow2_U16(U16 value)
{
  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  ++value;
  return value;
}

static U32 gpr_next_pow2_U32(U32 value)
{
  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  ++value;
  return value;
}

static U64 gpr_next_pow2_U64(U64 value)
{
  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value |= value >> 32;
  ++value;
  return value;
}

// compiler shit & stuff

#ifndef alignof
  #define alignof(x) __alignof(x)
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#endif // GPR_TYPES_H
