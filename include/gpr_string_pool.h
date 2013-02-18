#ifndef GPR_STRING_POOL_H
#define GPR_STRING_POOL_H

#include <string.h>
#include "gpr_hash.h"
#include "gpr_murmur_hash.h"

typedef struct
{
  char *string;
  I32   refs;
} gpr_string_entry_t;

typedef struct
{
  gpr_hash_t       string_map;
  gpr_allocator_t *string_allocator;
} gpr_string_pool_t;

static void  gpr_string_pool_init(gpr_string_pool_t *pool, gpr_allocator_t *map_allocator, 
                                  gpr_allocator_t *string_allocator)
{
  gpr_hash_init(gpr_string_entry_t, &pool->string_map, map_allocator);
  pool->string_allocator = string_allocator;
}

static void  gpr_string_pool_destroy(gpr_string_pool_t *pool)
{
  gpr_string_entry_t *e   = gpr_hash_begin(gpr_string_entry_t, &pool->string_map);
  gpr_string_entry_t *end = gpr_hash_end  (gpr_string_entry_t, &pool->string_map);

  while(e < end) 
  {
    gpr_deallocate(pool->string_allocator, e->string);
    ++e;
  }
  gpr_hash_destroy(gpr_string_entry_t, &pool->string_map);
}

static char *gpr_string_pool_nget(gpr_string_pool_t *pool, const char *string, U32 size)
{
  U64 key = gpr_murmur_hash_64(string, size, 0);
  gpr_string_entry_t *e = gpr_hash_get(gpr_string_entry_t, &pool->string_map, key);

  if(e) 
  {
    ++e->refs;
    return e->string;
  }{
    gpr_string_entry_t ne;
    ne.refs = 1;
    ne.string = gpr_strdup(pool->string_allocator, string);
    gpr_hash_set(gpr_string_entry_t, &pool->string_map, key, &ne);
    return ne.string;
  }
}

static char *gpr_string_pool_get(gpr_string_pool_t *pool, const char *string)
{
  return gpr_string_pool_nget(pool, string, strlen(string));
}

static I32 gpr_string_pool_has(gpr_string_pool_t *pool, const char *string)
{
  return gpr_hash_has(gpr_string_entry_t, &pool->string_map, 
    gpr_murmur_hash_64(string, strlen(string), 0));
}

static void gpr_string_pool_release(gpr_string_pool_t *pool, char *string)
{
  U64 key = gpr_murmur_hash_64(string, strlen(string), 0);
  gpr_string_entry_t *e = gpr_hash_get(gpr_string_entry_t, &pool->string_map, key);

  if(!e) return;
  if(e->refs == 1) 
  {
    gpr_deallocate(pool->string_allocator, e->string);
    gpr_hash_remove(gpr_string_entry_t, &pool->string_map, key);
  }
  --e->refs;
}

#endif // GPR_STRING_POOL_H