#ifndef GPR_HASH_H
#define GPR_HASH_H

#include "gpr_types.h"

typedef struct index_s
{
  U64 key;
  U32 next;
} index_t;

typedef struct
{
  gpr_array_t(U32)      hashes;
  gpr_array_t(index_t)  indexes;
  char                 *data;
} gpr_hash_t;

// ---------------------------------------------------------------
// Hash
// ---------------------------------------------------------------

void  gpr_hash_init    (gpr_hash_t *h, SZ s, gpr_allocator_t *a);
void  gpr_hash_destroy (gpr_hash_t *h);

// returns 1 if the specified key exists in the hash
I32   gpr_hash_has     (gpr_hash_t *h, U64 key);

// returns the value pointer for the specified key
void *gpr_hash_get     (gpr_hash_t *h, SZ s, U64 key);

// sets the value for the key
void  gpr_hash_set     (gpr_hash_t *h, SZ s, U64 key, const void *value);

// removes the key from the hash if it exists
void  gpr_hash_remove  (gpr_hash_t *h, U64 key);

// resizes the hash lookup table to the specified size
void  gpr_hash_reserve (gpr_hash_t *h, SZ s, U32 size);

// Returns a pointer to the first entry in the hash table, can be used to
// efficiently iterate over the elements (in random order).
void *gpr_hash_begin   (gpr_hash_t *h);
void *gpr_hash_end     (gpr_hash_t *h, SZ s);

// ---------------------------------------------------------------
// Multi Hash
// ---------------------------------------------------------------

void  gpr_multi_hash_init       (gpr_hash_t *h, gpr_allocator_t *a);
void  gpr_multi_hash_destroy    (gpr_hash_t *h);

// Finds the first entry with the specified key.
void *gpr_multi_hash_find_first (gpr_hash_t *h, SZ s, U64 key);

// Finds the next entry with the same key as e.
void *gpr_multi_hash_find_next  (gpr_hash_t *h, SZ s, void *e);

// Returns the number of entries with the key.
U32   gpr_multi_hash_count      (gpr_hash_t *h, SZ s, U64 key);

// returns all the entries with the specified key
// use a TempAllocator for the array to avoid allocating memory
//void  gpr_multi_hash_get        (gpr_hash_t *h, U64 key, void **values);

// Inserts the value as an aditional value for the key.
void  gpr_multi_hash_insert     (gpr_hash_t *h, SZ s, U64 key, const void *value);

// Removes the specified entry.
void  gpr_multi_hash_remove     (gpr_hash_t *h, void *e);

// Removes all entries with the specified key.
void  gpr_multi_hash_remove_all (gpr_hash_t *h, U64 key);

#endif // GPR_HASH_H