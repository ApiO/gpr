#include <stdio.h>
#include <time.h>
#include <string.h>

#include "gpr_assert.h"
#include "gpr_idlut.h"
#include "gpr_memory.h"
#include "gpr_array.h"
#include "gpr_tmp_allocator.h"
#include "gpr_hash.h"

// ---------------------------------------------------------------
// Default allocator tests
// ---------------------------------------------------------------

void test_memory()
{
  gpr_allocator_t *a;
  void *p, *q;

  gpr_memory_init(4*1024*1024);
  a = gpr_default_allocator;

  p = gpr_allocate(a, 100);
  gpr_assert(gpr_allocated_for(a,p) >= 100);
  gpr_assert(gpr_allocated_tot(a)   >= 100);
  q = gpr_allocate(a, 100);
  gpr_assert(gpr_allocated_for(a,q) >= 100);
  gpr_assert(gpr_allocated_tot(a)   >= 200);

  gpr_deallocate(a,p);
  gpr_deallocate(a,q);

  gpr_memory_shutdown();
}

void test_scratch()
{
  gpr_allocator_t *a;
  char *p, *pointers[100];
  int   i;

  gpr_memory_init(256*1024);
  a = gpr_scratch_allocator;

  p = (char*)gpr_allocate(a, 10*1024);

  for (i=0; i<100; ++i) pointers[i] = (char*)gpr_allocate(a, 1024);
  for (i=0; i<100; ++i) gpr_deallocate(a, pointers[i]);

  gpr_deallocate(a, p);

  for (i=0; i<100; ++i) pointers[i] = (char *)gpr_allocate(a, 4*1024);
  for (i=0; i<100; ++i) gpr_deallocate(a, pointers[i]);

  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// Array test
// ---------------------------------------------------------------

void test_array()
{
  gpr_allocator_t *a;
  gpr_array_t(int) v1, v2;

  gpr_memory_init(4*1024*1024);
  a = gpr_default_allocator;

  gpr_array_init(int, v1, a);

  gpr_assert(gpr_array_size(v1) == 0);
  gpr_array_push_back(int, v1, 3);

  gpr_assert(gpr_array_size(v1) == 1);
  gpr_assert(gpr_array_item(v1, 0) == 3);

  gpr_array_init(int, v2, a);
  gpr_array_copy(int, v2, v1);
  gpr_assert(gpr_array_item(v2, 0) == 3);
  gpr_array_item(v2, 0) = 5;
  gpr_assert(gpr_array_item(v1,  0) == 3);
  gpr_assert(gpr_array_item(v2, 0) == 5);
  gpr_array_copy(int, v2, v1);
  gpr_assert(gpr_array_item(v2, 0) == 3);

  gpr_assert(gpr_array_end(v1) - gpr_array_begin(v1) == gpr_array_size(v1));
  gpr_assert(*gpr_array_begin(v1) == 3);
  gpr_array_pop_back(v1);
  gpr_assert(gpr_array_empty(v1));

  {
    int i;
    for (i=0; i<100; ++i) gpr_array_push_back(int, v1, i);
    for (i=0; i<100; ++i) gpr_assert(gpr_array_item(v1, i) == i);
  }
  gpr_assert(gpr_array_size(v1) == 100);

  gpr_array_remove(v1, 50);
  gpr_assert(gpr_array_size(v1) == 99);

  gpr_array_destroy(v1);
  gpr_array_destroy(v2);
  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// Temporary allocator test
// ---------------------------------------------------------------

void test_tmp_allocator()
{
  gpr_memory_init(4*1024*1024);
  {
    gpr_tmp_allocator_128_t  ta;
    gpr_allocator_t         *a = (gpr_allocator_t*)&ta;
    gpr_array_t(int)         ar;

    gpr_tmp_allocator_init(a, 128);
    gpr_array_init(int, ar, a);
    {
      int i;
      for (i=0; i<100; ++i) gpr_array_push_back(int, ar, i);
    }
    gpr_allocate(a, 2*1024);
    gpr_tmp_allocator_destroy(a);
  }
  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// ID lookup table test
// ---------------------------------------------------------------

typedef struct {
  I32 val;
} entry;

GPR_IDLUT_INIT(entry)

void test_idlut()
{
  entry new_entry;
  U32   id1, id2, id3;
  gpr_idlut_t(entry) table;
  gpr_allocator_t *a;

  gpr_memory_init(512);
  a = gpr_default_allocator;

  gpr_idlut_init(entry, &table, a, 3);

  // max items should be the next power of 2
  gpr_assert(table.max_items == 4);

  new_entry.val = 1;
  id1 = gpr_idlut_add(entry, &table, &new_entry);
  gpr_assert(gpr_idlut_has(entry, &table, id1));
  gpr_assert(gpr_idlut_lookup(entry, &table, id1)->val == 1);

  new_entry.val = 2;
  id2 = gpr_idlut_add(entry, &table, &new_entry);
  gpr_assert(gpr_idlut_has(entry, &table, id2));
  gpr_assert(gpr_idlut_lookup(entry, &table, id2)->val == 2);

  new_entry.val = 3;
  id3 = gpr_idlut_add(entry, &table, &new_entry);
  gpr_assert(gpr_idlut_has(entry, &table, id3));
  gpr_assert(gpr_idlut_lookup(entry, &table, id3)->val == 3);

  gpr_idlut_swap(entry, &table, id3, id1);
  gpr_assert(table.items[0].value.val == 3);
  gpr_assert(gpr_idlut_lookup(entry, &table, id3)->val == 3);

  gpr_idlut_swap_to(entry, &table, id2, 0);
  gpr_assert(table.items[0].value.val == 2);
  gpr_assert(gpr_idlut_lookup(entry, &table, id2)->val == 2);

  gpr_idlut_remove(entry, &table, id1);
  gpr_assert(!gpr_idlut_has(entry, &table, id1));

  gpr_idlut_remove(entry, &table, id2);
  gpr_assert(!gpr_idlut_has(entry, &table, id2));

  gpr_idlut_remove(entry, &table, id3);
  gpr_assert(!gpr_idlut_has(entry, &table, id3));

  gpr_idlut_destroy(entry, &table);

  gpr_memory_shutdown();
}

void test_hash()
{
  gpr_allocator_t *a;
  gpr_hash_t       h;
  int              v;

  gpr_memory_init(0);
  a = gpr_default_allocator;

  gpr_hash_init(&h, sizeof(int), a);

  gpr_assert(!gpr_hash_has(&h, 0));
  gpr_hash_remove(&h, 0);

  v = 123;
  gpr_hash_set(&h, sizeof(int), 1000, &v);
  //gpr_assert(*(int*)gpr_hash_get(&h,sizeof(int),1000) == 123);

  //{
  //  int i;
  //  for (i=0; i<100; ++i) {
  //    v = i*i;
  //    gpr_hash_set(&h, sizeof(int), i, &v);
  //  }
  //  for (i=0; i<100; ++i)
  //    gpr_assert(*(int*)gpr_hash_get(&h,sizeof(int),i) == i*i);
  //}

  //gpr_hash_remove(&h, 1000);
  //gpr_assert(!gpr_hash_has(&h, 1000));
  //gpr_hash_remove(&h, 2000);
  //gpr_assert(gpr_hash_get(&h,sizeof(int),1000) == NULL);
  //{ 
  //  int i;
  //  for (i=0; i<100; ++i)
  //    gpr_assert(*(int*)gpr_hash_get(&h,sizeof(int),i) == i*i);
  //}
  gpr_hash_destroy(&h);
  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------

int main()
{
  //test_memory();
  //test_scratch();
  //test_tmp_allocator();
  //test_array();
  //test_idlut();
  test_hash();
  return 0;
}