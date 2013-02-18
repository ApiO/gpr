#include <stdio.h>
#include <time.h>
#include <string.h>

#include "gpr_assert.h"
#include "gpr_idlut.h"
#include "gpr_memory.h"
#include "gpr_array.h"
#include "gpr_tmp_allocator.h"
#include "gpr_pool_allocator.h"
#include "gpr_hash.h"
#include "gpr_murmur_hash.h"
#include "gpr_tree.h"
#include "gpr_string_pool.h"
#include "gpr_json_read.h"
#include "gpr_json_write.h"


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

  gpr_array_init(int, &v1, a);

  gpr_assert(gpr_array_size(&v1) == 0);
  gpr_array_push_back(int, &v1, 3);

  gpr_assert(gpr_array_size(&v1) == 1);
  gpr_assert(gpr_array_item(&v1, 0) == 3);

  gpr_array_init(int, &v2, a);
  gpr_array_copy(int, &v2, &v1);
  gpr_assert(gpr_array_item(&v2, 0) == 3);
  gpr_array_item(&v2, 0) = 5;
  gpr_assert(gpr_array_item(&v1,  0) == 3);
  gpr_assert(gpr_array_item(&v2, 0) == 5);
  gpr_array_copy(int, &v2, &v1);
  gpr_assert(gpr_array_item(&v2, 0) == 3);

  gpr_assert(gpr_array_end(&v1) - gpr_array_begin(&v1) == gpr_array_size(&v1));
  gpr_assert(*gpr_array_begin(&v1) == 3);
  gpr_array_pop_back(&v1);
  gpr_assert(gpr_array_empty(&v1));

  {
    int i;
    for (i=0; i<100; ++i) gpr_array_push_back(int, &v1, i);
    for (i=0; i<100; ++i) gpr_assert(gpr_array_item(&v1, i) == i);
  }
  gpr_assert(gpr_array_size(&v1) == 100);

  gpr_array_remove(&v1, 50);
  gpr_assert(gpr_array_size(&v1) == 99);

  gpr_array_destroy(&v1);
  gpr_array_destroy(&v2);
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
    gpr_array_init(int, &ar, a);
    {
      int i;
      for (i=0; i<100; ++i) gpr_array_push_back(int, &ar, i);
    }
    gpr_allocate(a, 2*1024);
    gpr_tmp_allocator_destroy(a);
  }
  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// Pool allocator test
// ---------------------------------------------------------------

void test_pool_allocator()
{
  gpr_memory_init(4*1024*1024);
  {
    gpr_pool_allocator_t  pa;
    gpr_allocator_t      *a = (gpr_allocator_t*)&pa;
    gpr_array_t(int*)     ar;

    gpr_pool_allocator_init(&pa, 4, 100, gpr_default_allocator);
    gpr_array_init(int*, &ar, gpr_default_allocator);
    {
      int i;
      for (i=0; i<150; ++i)
        gpr_array_push_back(int*, &ar, (int*)gpr_allocate(a, sizeof(int)));

      for (i=0; i<150; ++i) 
        gpr_deallocate(a, gpr_array_item(&ar, i));
    }
    gpr_array_destroy(&ar);
    {
      void *p = gpr_allocate(a, 2*1024);
      gpr_deallocate(a, p);
    }
    gpr_pool_allocator_destroy(a);
  }
  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// ID lookup table test
// ---------------------------------------------------------------

#define NUM_INSERT 100

void test_idlut()
{
  I32 item;
  U64   id1, id2, id3;
  gpr_idlut_t t;
  gpr_allocator_t *a;
  U64 keys[NUM_INSERT];

  gpr_memory_init(512);
  a = gpr_default_allocator;

  gpr_idlut_init(I32, &t, a);

  item = 1;
  id1 = gpr_idlut_add(I32, &t, &item);
  gpr_assert(gpr_idlut_has(I32, &t, id1));
  gpr_assert(*gpr_idlut_lookup(I32, &t, id1) == 1);

  item = 2;
  id2 = gpr_idlut_add(I32, &t, &item);
  gpr_assert(gpr_idlut_has(I32, &t, id2));
  gpr_assert(*gpr_idlut_lookup(I32, &t, id2)== 2);

  gpr_idlut_remove(I32, &t, id1);
  gpr_assert(!gpr_idlut_has(I32, &t, id1));

  item = 3;
  id3 = gpr_idlut_add(I32, &t, &item);
  gpr_assert(gpr_idlut_has(I32, &t, id3));
  gpr_assert(*gpr_idlut_lookup(I32, &t, id3) == 3);

  {
    int i; for (i=0; i<NUM_INSERT; ++i)
      keys[i] = gpr_idlut_add(I32, &t, &i);
  }

  gpr_assert(*gpr_idlut_lookup(I32, &t, id2) == 2);
  gpr_idlut_remove(I32, &t, id2);
  gpr_assert(*gpr_idlut_lookup(I32, &t, id3) == 3);
  gpr_idlut_remove(I32, &t, id3);

  gpr_assert(gpr_idlut_end(I32, &t) - gpr_idlut_begin(I32, &t) == NUM_INSERT);
  gpr_idlut_remove(I32, &t, keys[0]);
  gpr_idlut_remove(I32, &t, keys[NUM_INSERT-1]);
  {
    int i; for (i=1; i<NUM_INSERT-1; ++i)
      gpr_assert(*gpr_idlut_lookup(I32, &t, keys[i]) == i);
  }

  gpr_idlut_destroy(I32, &t);

  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// Hash table tests
// ---------------------------------------------------------------

void test_hash()
{
  gpr_allocator_t *a;
  gpr_hash_t       h;
  I32              v;

  gpr_memory_init(0);
  a = gpr_default_allocator;

  gpr_hash_init(I32, &h, a);

  gpr_assert(!gpr_hash_has(I32, &h, 0));
  gpr_hash_remove(I32, &h, 0);

  v = 123;
  gpr_hash_set(I32, &h, 1000, &v);
  gpr_assert(*gpr_hash_get(I32, &h, 1000) == 123);

  {
    int i;
    for (i=0; i<100; ++i) {
      v = i*i;
      gpr_hash_set(I32, &h, i, &v);
    }
    for (i=0; i<100; ++i)
      gpr_assert(*gpr_hash_get(I32, &h, i) == i*i);
  }

  gpr_hash_remove(I32, &h, 1000);
  gpr_assert(!gpr_hash_has(I32, &h, 1000));
  gpr_hash_remove(I32, &h, 2000);
  gpr_assert(gpr_hash_get(I32, &h, 1000) == NULL);
  { 
    int i;
    for (i=0; i<100; ++i)
      gpr_assert(*gpr_hash_get(I32, &h, i) == i*i);
  }
  gpr_hash_destroy(I32, &h);
  gpr_memory_shutdown();
}

void test_multi_hash()
{
  gpr_hash_t       h;
  I32              v;

  gpr_memory_init(4*1024);

  gpr_multi_hash_init(I32, &h, gpr_default_allocator);

  gpr_assert(gpr_multi_hash_count(I32, &h, 0) == 0);
  v = 1; gpr_multi_hash_insert(I32, &h, 0, &v);
  v = 2; gpr_multi_hash_insert(I32, &h, 0, &v);
  v = 3; gpr_multi_hash_insert(I32, &h, 0, &v);
  gpr_assert(gpr_multi_hash_count(I32, &h, 0) == 3);

  {
    gpr_tmp_allocator_128_t ta;
    gpr_array_t(I32) arr;
    gpr_tmp_allocator_init(&ta, 128);
    gpr_array_init(I32, &arr, (gpr_allocator_t*)&ta);

    gpr_multi_hash_get(I32, &h, 0, &arr);

    gpr_assert(gpr_array_size(&arr) == 3);

    gpr_multi_hash_remove(I32, &h, gpr_multi_hash_find_first(I32, &h, 0, 0));
    gpr_assert(gpr_multi_hash_count(I32, &h,0) == 2);
    gpr_multi_hash_remove_all(I32, &h, 0);
    gpr_assert(gpr_multi_hash_count(I32, &h, 0) == 0);

    gpr_tmp_allocator_destroy(&ta);
  }

  gpr_multi_hash_destroy(I32, &h);

  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// Murmur Hash
// ---------------------------------------------------------------

void test_murmur_hash()
{
  const char *s = "test_string";
  U64 h = gpr_murmur_hash_64(s, strlen(s), 0);
  gpr_assert(h == 0xe604acc23b568f83ull);
}

// ---------------------------------------------------------------
// Tree
// ---------------------------------------------------------------

void test_tree()
{
  gpr_tree_t t;
  I32 val = 0;
  U64 root, n1, n2, n3, n31, n32;

  gpr_memory_init(4*1024);
  root = gpr_tree_init(I32, &t, gpr_default_allocator, &val);

  val = 1;  n1  = gpr_tree_add_child(I32, &t, root, &val);
  val = 2;  n2  = gpr_tree_add_child(I32, &t, root, &val);
  val = 3;  n3  = gpr_tree_add_child(I32, &t, root, &val);
  val = 31; n31 = gpr_tree_add_child(I32, &t, n3, &val);
  val = 32; n32 = gpr_tree_add_child(I32, &t, n3, &val);

  gpr_assert(*gpr_tree_get(I32, &t, n3) == 3);
  gpr_assert(*gpr_tree_get(I32, &t, n31) == 31);
  gpr_assert(*gpr_tree_get(I32, &t, n32) == 32);
  gpr_tree_remove(I32, &t, n3);
  gpr_assert(!gpr_tree_has(I32, &t, n3));
  gpr_assert(!gpr_tree_has(I32, &t, n31));
  gpr_assert(!gpr_tree_has(I32, &t, n32));

  gpr_tree_destroy(I32, &t);

  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// String pool test
// --------------------------------------------------------------

void test_string_pool()
{
  gpr_string_pool_t sp;
  char *s1, *s2;

  gpr_memory_init(1024*4);
  gpr_string_pool_init(&sp, gpr_default_allocator, gpr_default_allocator);

  s1 = gpr_string_pool_get(&sp, "hello world!");
  s2 = gpr_string_pool_get(&sp, "hello world!");

  gpr_assert(s1 == s2);
  gpr_string_pool_release(&sp, "hello world!");
  gpr_assert(strcmp(s2,"hello world!") == 0);
  gpr_string_pool_release(&sp, "hello world!");
  gpr_assert(!gpr_string_pool_has(&sp, "hello world!"));


  gpr_string_pool_destroy(&sp);
}

// ---------------------------------------------------------------
// JSON Test
// ---------------------------------------------------------------

void test_json()
{
  gpr_buffer_t buf;
  gpr_json_t jsn;
  gpr_string_pool_t sp;
  gpr_pool_allocator_t pa;
  U64 entity;

  gpr_memory_init(1024*4);
  gpr_pool_allocator_init(&pa, 16, 256, gpr_default_allocator);
  gpr_string_pool_init(&sp, gpr_default_allocator, (gpr_allocator_t*)&pa);

  // ---------------------------------------------------------------
  entity = gpr_json_init(&jsn, &sp, gpr_default_allocator);

  gpr_json_set_integer(&jsn, entity, "x", 10);
  gpr_json_set_integer(&jsn, entity, "y", 20);
  gpr_json_set_string (&jsn, entity, "desc", "this is a description string");
  {
    U64 matrix = gpr_json_create_object(&jsn, entity, "matrix3");
    U64 mx = gpr_json_create_array(&jsn, matrix, "x");
    U64 my = gpr_json_create_array(&jsn, matrix, "y");
    U64 mz = gpr_json_create_array(&jsn, matrix, "z");

    I32 values[] = {1,2,3};
    gpr_json_array_add_integers(&jsn, mx, values, 3);
    gpr_json_array_add_integers(&jsn, my, values, 3);
    gpr_json_array_add_integers(&jsn, mz, values, 3);

    gpr_assert(matrix == gpr_json_get(&jsn, entity, "matrix3")->object);
    gpr_assert(mz     == gpr_json_get(&jsn, matrix, "z")->arr);

    gpr_json_array_add_integer(&jsn, mz, 666);
    gpr_assert(gpr_json_array_size(&jsn, mz) == 4);
    gpr_assert(gpr_json_array_get(&jsn, mz, 3)->integer == 666);

    gpr_json_array_remove(&jsn, mz, 3);
    gpr_assert(gpr_json_array_size(&jsn, mz) == 3);

    gpr_json_remove(&jsn, matrix, "z");
    gpr_assert(!gpr_json_has(&jsn, matrix, "z"));

    gpr_json_set_integer(&jsn, entity, "z", 666);
    gpr_json_set_number (&jsn, entity, "z", 0.666);
  }


  gpr_buffer_init(&buf, gpr_default_allocator);
  gpr_json_write(&jsn, entity, &buf, 1);
  printf(buf.data);

  // ---------------------------------------------------------------

  gpr_buffer_destroy(&buf);
  gpr_json_destroy(&jsn);
  gpr_string_pool_destroy(&sp);
  gpr_pool_allocator_destroy(&pa);
  gpr_memory_shutdown();
}

// ---------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------

int main()
{
  /*test_memory();
  test_scratch();
  test_tmp_allocator();
  test_pool_allocator();
  test_array();
  test_idlut();
  test_hash();
  test_multi_hash();
  test_murmur_hash();
  test_tree();
  test_string_pool();*/
  test_json();
  return 0;
}