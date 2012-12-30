#include <stdio.h>
#include <time.h>
#include <string.h>

#include "gpr_assert.h"
#include "gpr_idlut.h"
#include "gpr_memory.h"

// ---------------------------------------------------------------
// Allocator tests
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
  gpr_allocator_t *a = gpr_default_allocator;

  gpr_memory_init(512);

  gpr_idlut_init(entry, &table, a, 3);

  // max items should be the next power of 2
  assert(table.max_items == 4);

  new_entry.val = 1;
  id1 = gpr_idlut_add(entry, &table, &new_entry);
  gpr_assert(gpr_idlut_has(entry, &table, id1));
  gpr_assert(gpr_idlut_lookup(entry, &table, id1)->val == 1);

  new_entry.val = 2;
  id2 = gpr_idlut_add(entry, &table, &new_entry);
  gpr_assert(gpr_idlut_has(entry, &table, id2));
  gpr_assert(gpr_idlut_lookup(entry, &table, id2)->val == 2);

  gpr_idlut_remove(entry, &table, id2);
  gpr_assert(!gpr_idlut_has(entry, &table, id2));

  new_entry.val = 3;
  id3 = gpr_idlut_add(entry, &table, &new_entry);
  gpr_assert(gpr_idlut_has(entry, &table, id3));
  gpr_assert(gpr_idlut_lookup(entry, &table, id3)->val == 3);

  gpr_idlut_remove(entry, &table, id1);
  gpr_assert(!gpr_idlut_has(entry, &table, id1));
  
  gpr_idlut_remove(entry, &table, id3);
  gpr_assert(!gpr_idlut_has(entry, &table, id3));

  gpr_idlut_free(entry, &table);

  gpr_memory_shutdown();
}

int main()
{
  test_memory();
  test_scratch();
  test_idlut();
  return 0;
}