#include "gpr_murmur_hash.h"

U64 gpr_murmur_hash_64(const void *key, U32 len, U64 seed)
{
  const U64 m = 0xc6a4a7935bd1e995ULL;
  const U32 r = 47;

  U64 h = seed ^ (len * m);

  const U64 *data = (const U64*)key;
  const U64 *end = data + (len/8);

  while(data != end)
  {
    U64 k = *data++;
#ifdef PLATFORM_BIG_ENDIAN
    k = gpr_swap_U64(k);
#endif

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }
  {
    const unsigned char *data2 = (const unsigned char*)data;
    switch(len & 7)
    {
    case 7: h ^= ((U64)data2[6]) << 48;
    case 6: h ^= ((U64)data2[5]) << 40;
    case 5: h ^= ((U64)data2[4]) << 32;
    case 4: h ^= ((U64)data2[3]) << 24;
    case 3: h ^= ((U64)data2[2]) << 16;
    case 2: h ^= ((U64)data2[1]) << 8;
    case 1: h ^= ((U64)data2[0]);
      h *= m;
    };
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}