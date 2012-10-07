#ifndef GPR_CONTAINERS_H
#define GPR_CONTAINERS_H

//#include "memory.h"
#include "short_types.h"

// -------------------------------------------------------------------------
// Id Lookup Table Container 
// -------------------------------------------------------------------------
// More information about the implementation at
// http://www.altdevblogaday.com/2011/09/23/managing-decoupling-part-4-the-id-lookup-table/
// -------------------------------------------------------------------------

// opaque table index structure 
// used to avoid item fragmentation adding a level of indirection
typedef struct
{
  U32  id;    // external id
  U16  index; // index of the item
  U16  next;  // index of the next gpr_index_t
} gpr_index_t;

typedef struct
{
  U16  max_items; // maximum number of items available in the table
  SZ   sz_item;   // size of an item
  U16  num_items; // number of items in the id lookup table

  U8  *item_data; // byte array containing items data
  U32 *item_ids;  // item ids in the same order of item data

  gpr_index_t *indices; // table indices used to avoid item fragmentation
  U16 freelist_enqueue; // index of gpr_index_t pointing to the previous hole
  U16 freelist_dequeue; // index of gpr_index_t pointing to the next hole 
} gpr_idlut_t;

// create an id lookup table allocating memory for "max_items"
void  gpr_idlut_init   (gpr_idlut_t *table, U16 max_items, SZ sz_item);

// free the id lookup table and all its memory
void  gpr_idlut_free   (gpr_idlut_t *table);

// add the specified item to the table and return its id
U32   gpr_idlut_add    (gpr_idlut_t *table, const void *item);

// return 1 or 0 if the table contains or not the item
I32   gpr_idlut_has    (gpr_idlut_t *table, U32 id);

// return a pointer to the item with the "id" identifier
void *gpr_idlut_lookup (gpr_idlut_t *table, U32 id);

// remove the item with the "id" identifier (does not free memory)
void  gpr_idlut_remove (gpr_idlut_t *table, U32 id);

// return the table item list
void *gpr_idlut_items  (gpr_idlut_t *table);

// return the table item ids
U32  *gpr_idlut_ids    (gpr_idlut_t *table);

#define INDEX_MASK 0xffff
#define NEW_ITEM_ID_ADD 0x10000

// -------------------------------------------------------------------------
// Strongly typed version
// -------------------------------------------------------------------------

#include <stdlib.h> 
#include "gpr_assert.h"

#define GPR_IDLUT_DEF(type)                                                 \
typedef struct                                                              \
{                                                                           \
  U32  id;                                                                  \
  type value;                                                               \
} gpr_##type##_idlut_item;                                                  \
                                                                            \
typedef struct                                                              \
{                                                                           \
  U16   max_items;                                                          \
  U16   num_items;                                                          \
  gpr_##type##_idlut_item *items;                                           \
  gpr_index_t *indices;                                                     \
  U16 freelist_enqueue;                                                     \
  U16 freelist_dequeue;                                                     \
} gpr_##type##_idlut_t;                                                     \
                                                                            \
void gpr_##type##_idlut_init(gpr_##type##_idlut_t *table, U16 max_items)    \
{                                                                           \
  table->items = (gpr_##type##_idlut_item*)                                 \
                 malloc(max_items * sizeof(gpr_##type##_idlut_item));       \
  GPR_ASSERT_ALLOC(table->items);                                           \
                                                                            \
  table->indices = (gpr_index_t*)malloc(max_items * sizeof(gpr_index_t));   \
  GPR_ASSERT_ALLOC(table->indices);                                         \
                                                                            \
  table->max_items        = max_items;                                      \
  table->num_items        = 0;                                              \
  table->freelist_dequeue = 0;                                              \
  table->freelist_enqueue = max_items;                                      \
                                                                            \
  {                                                                         \
    U16 i;                                                                  \
    for(i = 0; i < table->max_items; ++i)                                   \
    {                                                                       \
      table->indices[i].id = i;                                             \
      table->indices[i].next = i+1;                                         \
    }                                                                       \
  }                                                                         \
}                                                                           \
                                                                            \
void gpr_##type##_idlut_free(gpr_##type##_idlut_t *table)                   \
{                                                                           \
  free(table->items);                                                       \
  free(table->indices);                                                     \
}                                                                           \
                                                                            \
U32 gpr_##type##_idlut_add(gpr_##type##_idlut_t *table,                     \
                           const type *item_value)                          \
{                                                                           \
  gpr_##type##_idlut_item *item;                                            \
  gpr_index_t *in = &table->indices[table->freelist_dequeue];               \
                                                                            \
  in->id    += NEW_ITEM_ID_ADD;                                             \
  in->index  = table->num_items;                                            \
                                                                            \
  item = &table->items[in->index];                                          \
  item->value = *item_value;                                                \
  item->id    =  in->id;                                                    \
                                                                            \
  ++table->num_items;                                                       \
  table->freelist_dequeue = in->next;                                       \
                                                                            \
  return in->id;                                                            \
}                                                                           \
                                                                            \
I32 gpr_##type##_idlut_has(gpr_##type##_idlut_t *table, U32 id)             \
{                                                                           \
  gpr_index_t *in = &table->indices[id & INDEX_MASK];                       \
  return in->id == id && in->index != USHRT_MAX;                            \
}                                                                           \
                                                                            \
type *gpr_##type##_idlut_lookup(gpr_##type##_idlut_t *table, U32 id)        \
{                                                                           \
  return &table->items[table->indices[id & INDEX_MASK].index].value;        \
}                                                                           \
                                                                            \
void gpr_##type##_idlut_remove(gpr_##type##_idlut_t *table, U32 id)         \
{                                                                           \
  gpr_##type##_idlut_item *item;                                            \
  gpr_index_t *in = &table->indices[id & INDEX_MASK];                       \
                                                                            \
  item = &table->items[in->index];                                          \
  *item = table->items[--table->num_items];                                 \
  table->indices[item->id & INDEX_MASK].index = in->index;                  \
                                                                            \
  in->index = USHRT_MAX;                                                    \
	table->freelist_enqueue = id & INDEX_MASK;                                \
  if(table->freelist_dequeue == table->max_items)                           \
    table->freelist_dequeue = table->freelist_enqueue;                      \
}                                                                           \
                                                                            \
gpr_##type##_idlut_item *                                                   \
gpr_##type##_idlut_items(gpr_##type##_idlut_t *table)                       \
{                                                                           \
  return table->items;                                                      \
}                                                                           \

#endif // GPR_CONTAINERS_H