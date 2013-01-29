#ifndef GPR_HASH_H
#define GPR_HASH_H

#include "gpr_types.h"
#include "gpr_array.h"
#include "gpr_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  U64 key;
  U32 next;
  U32 value_pos;
} gpr_hash_index_t;

typedef struct
{
  gpr_array_t(U32) buckets;
  gpr_array_t(gpr_hash_index_t)  
                   indices;
  gpr_array_t(U64) keys;
  gpr_buffer_t     values;
  U32              num_values;
} gpr_hash_t;

// ---------------------------------------------------------------
// Hash
// ---------------------------------------------------------------

void  _gpr_hash_init    (gpr_hash_t *h, const U32 s, gpr_allocator_t *a);
void  _gpr_hash_destroy (gpr_hash_t *h);
I32   _gpr_hash_has     (gpr_hash_t *h, U64 key);
void *_gpr_hash_get     (gpr_hash_t *h, U64 key);
void  _gpr_hash_set     (gpr_hash_t *h, const U32 s, U64 key, const void *value);
void  _gpr_hash_remove  (gpr_hash_t *h, const U32 s, U64 key);
void  _gpr_hash_reserve (gpr_hash_t *h, const U32 s, U32 capacity);
void *_gpr_hash_begin   (gpr_hash_t *h);
void *_gpr_hash_end     (gpr_hash_t *h, const U32 s);

#define gpr_hash_init(type, h, alct)        _gpr_hash_init(h, sizeof(type), alct)
#define gpr_hash_destroy(type, h)           _gpr_hash_destroy(h)
#define gpr_hash_has(type, h, key)          _gpr_hash_has(h, key)
#define gpr_hash_get(type, h, key)          ((type*)_gpr_hash_get(h, key))
#define gpr_hash_set(type, h, key, value)   _gpr_hash_set(h, sizeof(type), key, value)
#define gpr_hash_remove(type, h, key)       _gpr_hash_remove(h, sizeof(type), key)
#define gpr_hash_reserve(type, h, capacity) _gpr_hash_reserve(h, sizeof(type), capacity)
#define gpr_hash_begin(type, h)             ((type*)_gpr_hash_begin(h))
#define gpr_hash_end(type, h)               ((type*)_gpr_hash_enf(h, sizeof(type)))

// ---------------------------------------------------------------
// Multi Hash
// ---------------------------------------------------------------

typedef struct
{
  U64 key;
  U32 next;
} gpr_hash_it;

void *_gpr_multi_hash_find_first (gpr_hash_t *h, gpr_hash_it *it, U64 key);
void *_gpr_multi_hash_find_next  (gpr_hash_t *h, gpr_hash_it *it);
U32   _gpr_multi_hash_count      (gpr_hash_t *h, U64 key);
void *_gpr_multi_hash_get        (gpr_hash_t *h, const U32 s, U64 key, gpr_buffer_t *values);
void  _gpr_multi_hash_insert     (gpr_hash_t *h, const U32 s, U64 key, const void *value);
void  _gpr_multi_hash_remove     (gpr_hash_t *h, const U32 s, void *e);
void  _gpr_multi_hash_remove_all (gpr_hash_t *h, const U32 s, U64 key);

#define gpr_multi_hash_init(type, h, alct)          _gpr_hash_init(h, sizeof(type), alct)
#define gpr_multi_hash_destroy(type, h)             _gpr_hash_destroy(h)
#define gpr_multi_hash_find_first(type, h, it, key) ((type*)_gpr_multi_hash_find_first(h, it, key))
#define gpr_multi_hash_find_next(type, h, it)       ((type*)_gpr_multi_hash_find_next(h, it))
#define gpr_multi_hash_count(type, h, key)          _gpr_multi_hash_count(h, key)
#define gpr_multi_hash_insert(type, h, key, value)  _gpr_multi_hash_insert(h, sizeof(type), key, value)
#define gpr_multi_hash_remove(type, h, e)           _gpr_multi_hash_remove(h, sizeof(type), e)
#define gpr_multi_hash_remove_all(type, h, key)     _gpr_multi_hash_remove_all(h, sizeof(type), key)

#define gpr_multi_hash_get(type, h, key, arr)             \
do {                                                      \
  gpr_hash_it it;                                         \
  type *e = gpr_multi_hash_find_first(type, h, &it, key); \
  while (e) {                                             \
    gpr_array_push_back(type, arr, *e);                   \
    e = gpr_multi_hash_find_next(type, h, &it);           \
  }                                                       \
} while(0)

#endif // GPR_HASH_H

#ifdef __cplusplus
}
#endif
