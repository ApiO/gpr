#include "gpr_types.h"
#include "gpr_array.h"
#include "gpr_memory.h"
#include "gpr_hash.h"
#include "gpr_buffer.h"

// ---------------------------------------------------------------
// Hash internals
// ---------------------------------------------------------------

#define END_OF_LIST     0xffffffffu
#define MAX_LOAD_FACTOR 0.7f

typedef gpr_hash_index_t index_t;

typedef struct
{
  U32 hash_i;
  U32 index_prev;
  U32 index_i;
} find_result_t;

static I32 full(gpr_hash_t *h)
{
  return h->num_values >= (U32)(gpr_array_size(&h->buckets)*MAX_LOAD_FACTOR);
}

static void find_index(gpr_hash_t *h, U64 key, find_result_t *fr)
{
  fr->hash_i     = END_OF_LIST;
  fr->index_prev = END_OF_LIST;
  fr->index_i    = END_OF_LIST;

  if (gpr_array_size(&h->buckets) == 0) return;

  fr->hash_i  = key % gpr_array_size(&h->buckets);
  fr->index_i = gpr_array_item(&h->buckets, fr->hash_i);

  while (fr->index_i != END_OF_LIST)
  {
    if (gpr_array_item(&h->indices, fr->index_i).key == key) return;
    fr->index_prev = fr->index_i;
    fr->index_i = gpr_array_item(&h->indices, fr->index_i).next;
  }
}

static U32 make_index(gpr_hash_t *h, U64 key, U32 value_pos)
{
  find_result_t fr;
  index_t       index;
  const U32 i = gpr_array_size(&h->indices);

  index.key       = key;
  index.next      = END_OF_LIST;
  index.value_pos = value_pos;

  find_index(h, key, &fr);

  gpr_array_push_back(index_t, &h->indices, index);

  if (fr.index_prev == END_OF_LIST)
    gpr_array_item(&h->buckets, fr.hash_i) = i;
  else
    gpr_array_item(&h->indices, fr.index_prev).next = i;

  gpr_array_item(&h->indices, i).next = fr.index_i;
  return i;
}

static U32 find_or_make_index(gpr_hash_t *h, U64 key)
{
  find_result_t fr;
  find_index(h, key, &fr);
  if (fr.index_i != END_OF_LIST) 
    return fr.index_i;
  {
    index_t       index;
    const U32 i = gpr_array_size(&h->indices);
    index.key     = key;
    index.next    = END_OF_LIST;
    index.value_pos = END_OF_LIST;

    gpr_array_push_back(index_t, &h->indices, index);

    if (fr.index_prev == END_OF_LIST)
      gpr_array_item(&h->buckets, fr.hash_i) = i;
    else
      gpr_array_item(&h->indices, fr.index_prev).next = i;
    return i;
  }
}

static void erase(gpr_hash_t *h, const U32 s, find_result_t *fr)
{
  index_t      *index     = &gpr_array_item(&h->indices, fr->index_i);
  const U32     value_pos = index->value_pos;
  const U32     value_i   = value_pos/s;
  find_result_t last;

  if (fr->index_prev == END_OF_LIST)
    gpr_array_item(&h->buckets, fr->hash_i) = index->next;
  else
    gpr_array_item(&h->indices, fr->index_prev).next = index->next;

  // remove from indices
  if (fr->index_i == --h->num_values)
  {
    gpr_array_pop_back(&h->indices);
  }
  else
  {
    *index = gpr_array_pop_back(&h->indices);
    find_index(h, index->key, &last);

    if (fr->index_prev == END_OF_LIST)
      gpr_array_item(&h->buckets, fr->hash_i) = fr->index_i;
    else
      gpr_array_item(&h->indices, fr->index_prev).next = fr->index_i;
  }

  // remove values & keys
  if(value_pos/s == h->num_values)
  {
    gpr_array_pop_back(&h->keys);
  }
  else
  {
    gpr_array_item(&h->keys, value_i) = gpr_array_pop_back(&h->keys);
    memcpy(h->values.data + value_pos, 
      h->values.data + h->num_values*s, s);

    find_index(h, gpr_array_item(&h->keys, value_i), &last);
    gpr_array_item(&h->indices, last.index_i).value_pos = value_pos;
  }
  gpr_buffer_resize(&h->values, h->num_values*s);
}

static void rehash(gpr_hash_t *h, const U32 s, U32 size)
{
  U32 i;
  gpr_array_t(index_t) tmp_indices;
  gpr_array_init(index_t, &tmp_indices, h->indices.allocator);
  gpr_array_copy(index_t, &tmp_indices, &h->indices);

  gpr_array_resize(U32, &h->buckets, size);
  for (i = 0; i < gpr_array_size(&h->buckets); ++i)
    gpr_array_item(&h->buckets, i) = END_OF_LIST;

  gpr_array_resize (index_t, &h->indices, 0);
  gpr_array_reserve(index_t, &h->indices, (U32)(size*MAX_LOAD_FACTOR)+1);
  for (i=0; i < gpr_array_size(&tmp_indices); ++i)
  {
    index_t *index = &gpr_array_item(&tmp_indices, i);
    make_index(h, index->key, index->value_pos);
  }
  gpr_array_destroy(&tmp_indices);
  gpr_array_reserve(U64, &h->keys, gpr_array_capacity(&h->indices));
  gpr_buffer_reserve(&h->values, gpr_array_capacity(&h->indices)*s);
}

static void grow(gpr_hash_t *h, const U32 s)
{
  rehash(h, s, gpr_array_size(&h->buckets) << 1);
}

// ---------------------------------------------------------------
// Hash implementation
// ---------------------------------------------------------------

void _gpr_hash_init(gpr_hash_t *h, const U32 s, gpr_allocator_t *a)
{
  gpr_array_init(U32,     &h->buckets, a);
  gpr_array_init(index_t, &h->indices, a);
  gpr_array_init(U64,     &h->keys,    a);
  gpr_buffer_init(&h->values, a);
  h->num_values = 0;
}

void _gpr_hash_destroy(gpr_hash_t *h)
{
  gpr_array_destroy(&h->buckets);
  gpr_array_destroy(&h->indices);
  gpr_array_destroy(&h->keys);
  gpr_buffer_destroy(&h->values);
}

I32 _gpr_hash_has(gpr_hash_t *h, U64 key)
{
  find_result_t fr;
  find_index(h, key, &fr);
  return fr.index_i != END_OF_LIST;
}

void *_gpr_hash_get(gpr_hash_t *h, U64 key)
{
  find_result_t fr;
  find_index(h, key, &fr);
  return (fr.index_i == END_OF_LIST) ? NULL : 
    h->values.data + (gpr_array_item(&h->indices, fr.index_i).value_pos);
}

void _gpr_hash_set(gpr_hash_t *h, const U32 s, U64 key, const void *value)
{
  index_t *index;

  if(gpr_array_size(&h->buckets) == 0)
    rehash(h, s, 4);

  index = &gpr_array_item(&h->indices, find_or_make_index(h, key));

  if(index->value_pos == END_OF_LIST)
  {
    // create a new value
    index->value_pos = gpr_buffer_ncat(&h->values, (char*)value, s);
    gpr_array_push_back(U64, &h->keys, key);
    ++h->num_values;
  } else {
    // change an existing value 
    memcpy(h->values.data + index->value_pos, value, s);
  }

  if (full(h)) grow(h, s);
}

void _gpr_hash_remove(gpr_hash_t *h, const U32 s, U64 key)
{
  find_result_t fr;
  find_index(h, key, &fr);
  if (fr.index_i != END_OF_LIST)
    erase(h, s, &fr);
}

void _gpr_hash_reserve(gpr_hash_t *h, const U32 s, U32 capacity)
{
  rehash(h, s, gpr_next_pow2_U32(capacity));
}

void *_gpr_hash_begin(gpr_hash_t *h)
{
  return h->values.data;
}

void *_gpr_hash_end(gpr_hash_t *h, const U32 s)
{
  return h->values.data + (h->num_values*s);
}

// ---------------------------------------------------------------
// Multi Hash implementation
// ---------------------------------------------------------------

void *_gpr_multi_hash_find_first(gpr_hash_t *h, gpr_hash_it *it, U64 key)
{
  find_result_t fr;
  find_index(h, key, &fr);

  if (it != NULL) 
  {
      it->key  = key;
      it->next = (fr.index_i == END_OF_LIST) ? END_OF_LIST : 
        gpr_array_item(&h->indices, fr.index_i).next;
  }
  if (fr.index_i == END_OF_LIST) return NULL;

  return h->values.data + gpr_array_item(&h->indices, fr.index_i).value_pos;
}

void *_gpr_multi_hash_find_next(gpr_hash_t *h, gpr_hash_it *it)
{
  U32 i = it->next;
  while (i != END_OF_LIST) 
  {
    if (gpr_array_item(&h->indices, i).key == it->key)
    {
      it->next = gpr_array_item(&h->indices, i).next;
      return h->values.data + gpr_array_item(&h->indices, i).value_pos;
    }
    i = gpr_array_item(&h->indices, i).next;
  }
  it->next = END_OF_LIST;
  return NULL;
}

U32 _gpr_multi_hash_count(gpr_hash_t *h, U64 key)
{
  U32 i = 0;
  gpr_hash_it it;
  void *e = _gpr_multi_hash_find_first(h, &it, key);
  while (e) {
    ++i;
    e = _gpr_multi_hash_find_next(h, &it);
  }
  return i;
}

void _gpr_multi_hash_insert(gpr_hash_t *h, const U32 s, U64 key, const void *value)
{
  if (gpr_array_size(&h->buckets) == 0)
    rehash(h, s, 4);

  gpr_array_push_back(U64, &h->keys, key);
  make_index(h, key,  gpr_buffer_ncat(&h->values, (char*)value, s));

  ++h->num_values;

  if (full(h)) grow(h, s);
}

void _gpr_multi_hash_remove(gpr_hash_t *h, const U32 s, void *e)
{
  find_result_t fr;
  find_index(h, gpr_array_item(&h->keys, ((char*)e - h->values.data)/s), &fr);
  if (fr.index_i != END_OF_LIST) erase(h, s, &fr);
}

void  _gpr_multi_hash_remove_all (gpr_hash_t *h, const U32 s, U64 key)
{
  while (_gpr_hash_has(h, key))
    _gpr_hash_remove(h, s, key);
}