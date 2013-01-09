#include "gpr_types.h"
#include "gpr_array.h"
#include "gpr_memory.h"
#include "gpr_hash.h"

// ---------------------------------------------------------------
// Hash internals
// ---------------------------------------------------------------

#define END_OF_LIST     0xffffffffu
#define MAX_LOAD_FACTOR 0.7f

typedef struct 
{
  U32 hash_i;
  U32 data_prev;
  U32 data_i;
} find_result_t;

U32 add_entry(gpr_hash_t *h, U64 key)
{
  index_t e;
  U32 ei = gpr_array_size(h->indexes);
  e.key = key;
  e.next = END_OF_LIST;
  gpr_array_push_back(index_t, h->indexes, e);
  return ei;
}

void find(gpr_hash_t *h, U64 key, find_result_t *fr)
{
  fr->hash_i    = END_OF_LIST;
  fr->data_prev = END_OF_LIST;
  fr->data_i    = END_OF_LIST;

  if (gpr_array_size(h->hashes) == 0) return;

  fr->hash_i = key % gpr_array_size(h->hashes);
  fr->data_i = gpr_array_item(h->hashes, fr->hash_i);
  while (fr->data_i != END_OF_LIST) {
    if (gpr_array_item(h->indexes, fr->data_i).key == key) return;
    fr->data_prev = fr->data_i;
    fr->data_i = gpr_array_item(h->indexes, fr->data_i).next;
  }
}


void find_entry(const gpr_hash_t *h, index_t *e, find_result_t *fr)
{
  fr->hash_i    = END_OF_LIST;
  fr->data_prev = END_OF_LIST;
  fr->data_i    = END_OF_LIST;

  if (gpr_array_size(h->hashes) == 0) return;

  fr->hash_i = e->key % gpr_array_size(h->hashes);
  fr->data_i = gpr_array_item(h->hashes, fr->hash_i);
  while (fr->data_i != END_OF_LIST) 
  {
    if (&gpr_array_item(h->indexes, fr->data_i) == e) return;
    fr->data_prev = fr->data_i;
    fr->data_i = gpr_array_item(h->indexes, fr->data_i).next;
  }
}

void erase(gpr_hash_t *h, find_result_t *fr)
{
  if (fr->data_prev == END_OF_LIST)
  {
    gpr_array_item(h->hashes, fr->hash_i) = 
      gpr_array_item(h->indexes, fr->data_i).next;
  }
  else
  {
    gpr_array_item(h->indexes, fr->data_prev).next = 
      gpr_array_item(h->indexes, fr->data_i).next;
  }

  if (fr->data_i == gpr_array_size(h->indexes) - 1) 
  {
    gpr_array_pop_back(h->indexes);
    return;
  }

  {
    find_result_t last;
    gpr_array_item(h->indexes, fr->data_i) = 
      gpr_array_item(h->indexes, gpr_array_size(h->indexes) - 1);
    find(h, gpr_array_item(h->indexes, fr->data_i).key, &last);

    if (last.data_prev != END_OF_LIST)
      gpr_array_item(h->indexes, last.data_prev).next = fr->data_i;
    else
      gpr_array_item(h->hashes, last.hash_i) = fr->data_i;
  }
}

U32 find_or_fail(gpr_hash_t *h, U64 key)
{
  find_result_t fr;
  find(h, key, &fr);
  return fr.data_i;
}

U32 find_or_make(gpr_hash_t *h, U64 key)
{
  U32 i;
  find_result_t fr;
  find(h, key, &fr);
  if (fr.data_i != END_OF_LIST) return fr.data_i;

  i = add_entry(h, key);
  if (fr.data_prev == END_OF_LIST)
    gpr_array_item(h->hashes, fr.hash_i) = i;
  else
    gpr_array_item(h->indexes, fr.data_prev).next = i;
  return i;
}

U32 make(gpr_hash_t *h, U64 key)
{
  const U32 i = add_entry(h, key);
  find_result_t fr;
  find(h, key, &fr);


  if (fr.data_prev == END_OF_LIST)
    gpr_array_item(h->hashes, fr.hash_i) = i;
  else
    gpr_array_item(h->indexes, fr.data_prev).next = i;

  gpr_array_item(h->indexes, i).next = fr.data_i;
  return i;
}       

void find_and_erase(gpr_hash_t *h, U64 key)
{
  find_result_t fr;
  find(h, key, &fr);
  if (fr.data_i != END_OF_LIST) erase(h, &fr);
}

I32 full(const gpr_hash_t *h)
{
  return gpr_array_size(h->indexes) >= 
    gpr_array_size(h->hashes) * MAX_LOAD_FACTOR;
}

void rehash(gpr_hash_t *h, SZ s, U32 new_size)
{
  gpr_hash_t nh;
  gpr_hash_init(&nh, s, h->hashes.allocator);

  nh.data = (char*)gpr_allocate(h->hashes.allocator, s*new_size);
  memcpy(nh.data, h->data, gpr_array_size(h->indexes));

  gpr_array_resize (U32, nh.hashes, new_size);
  gpr_array_reserve(index_t, nh.indexes, gpr_array_size(h->indexes));
  {
    U32 i;
    for (i=0; i<new_size; ++i)
      gpr_array_item(nh.hashes, i) = END_OF_LIST;
    for (i=0; i<gpr_array_size(h->indexes); ++i) 
    {
      index_t *e = &gpr_array_item(h->indexes, i);
      gpr_multi_hash_insert(&nh, s, e->key, &h->data[i*s]);
    }
  }
  gpr_hash_destroy(h);
  memcpy(h, &nh, sizeof(gpr_hash_t));
}

void grow(gpr_hash_t *h, SZ s)
{
  U32 new_size = gpr_array_size(h->indexes) * 2 + 10;//gpr_next_pow2_U32(gpr_array_size(h->indexes)+1);
  rehash(h, s, new_size);
}

// ---------------------------------------------------------------
// Hash implementation
// ---------------------------------------------------------------

void gpr_hash_init(gpr_hash_t *h, SZ s, gpr_allocator_t *a)
{
  h->data = (char*)gpr_allocate(a, s*2);
  gpr_array_init(U32,     h->hashes,  a);
  gpr_array_init(index_t, h->indexes, a);
}

void gpr_hash_destroy(gpr_hash_t *h)
{
  gpr_deallocate(h->hashes.allocator, h->data);
  gpr_array_destroy(h->hashes );
  gpr_array_destroy(h->indexes);
}

I32 gpr_hash_has(gpr_hash_t *h, U64 key)
{
  return find_or_fail(h, key) != END_OF_LIST;
}

void *gpr_hash_get(gpr_hash_t *h, SZ s, U64 key)
{
  U32 i = find_or_fail(h, key);
  return (i == END_OF_LIST) ? NULL : 
    &h->data[i*s];
}

void gpr_hash_set(gpr_hash_t *h, SZ s, U64 key, const void *value)
{
  U32 i;
  if(gpr_array_size(h->hashes) == 0)
    grow(h, s);

  i = find_or_make(h, key);
  memcpy(&h->data[i*s], value, s);
  if (full(h)) grow(h, s);
}

void gpr_hash_remove(gpr_hash_t *h, U64 key)
{
  find_and_erase(h, key);
}

void gpr_hash_reserve(gpr_hash_t *h, SZ s, U32 size)
{
  rehash(h, s, gpr_next_pow2_U32(size));
}

void *gpr_hash_begin(gpr_hash_t *h)
{
  return h->data;
}

void *gpr_hash_end(gpr_hash_t *h, SZ s)
{
  return h->data + (gpr_array_size(h->indexes) * s);
}

// ---------------------------------------------------------------
// Multi Hash implementation
// ---------------------------------------------------------------

void *gpr_multi_hash_find_first(gpr_hash_t *h, SZ s, U64 key)
{
  U32 i = find_or_fail(h, key);
  return i == END_OF_LIST ? NULL : &h->data[i*s];
}

void *gpr_multi_hash_find_next(gpr_hash_t *h, SZ s, index_t *e)
{
  U32 i = e->next;
  while (i != END_OF_LIST) {
    if (gpr_array_item(h->indexes, i).key == e->key)
      return &h->data[i*s];
    i = gpr_array_item(h->indexes, i).next;
  }
  return 0;
}

U32 gpr_multi_hash_count(gpr_hash_t *h, SZ s, U64 key)
{
  U32 i = 0;
  void *e = gpr_multi_hash_find_first(h, s, key);
  while (e) {
    ++i;
    e = gpr_multi_hash_find_next(h, s, e);
  }
  return i;
}

//void gpr_multi_hash_get(gpr_hash_t *h, uint64_t key, gpr_hash_array_t *items)
//{
//  index_t *e = gpr_multi_hash_find_first(h, key);
//  while (e) {
//    gpr_array_push_back(T, (*items), e->value);
//    e = gpr_multi_hash_find_next(h, e);
//  }
//}

void gpr_multi_hash_insert(gpr_hash_t *h, SZ s, U64 key, const void *value)
{
  U32 i ;
  if (gpr_array_size(h->hashes) == 0)
    grow(h, s);

  i = make(h, key);
  memcpy(&h->data[i*s], value, s);
  if (full(h))
    grow(h, s);
}

// Removes the specified entry.
void gpr_multi_hash_remove(gpr_hash_t *h, index_t *e)
{
  find_result_t fr;
  find_entry(h, e, &fr);
  if (fr.data_i != END_OF_LIST)
    erase(h, &fr);
}

// Removes all entries with the specified key.
void gpr_multi_hash_remove_all(gpr_hash_t *h, uint64_t key)
{
  while (gpr_hash_has(h, key))
    gpr_hash_remove(h, key);
}