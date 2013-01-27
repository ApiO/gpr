#ifndef GPR_MURMUR_HASH_H
#define GPR_MURMUR_HASH_H

#include "gpr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

U64 gpr_murmur_hash_64(const void *key, U32 len, U64 seed);

#ifdef __cplusplus
}
#endif

#endif