#ifndef GPR_CONTAINERS_H
#define GPR_CONTAINERS_H

//#include "memory.h"
#include <stdlib.h> 
#include "gpr_assert.h"
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

#define GPR_INDEX_MASK 0xffff
#define GPR_ID_ADD 0x10000

#define GPR_IDLUT_INIT(type)                                                \
typedef struct                                                              \
{                                                                           \
  U32  id;                                                                  \
  type value;                                                               \
} gpr_##type##_idlut_item;                                                  \
                                                                            \
typedef struct                                                              \
{                                                                           \
  U16 max_items;        /* maximum number of items available in the table */\
  U16 num_items;        /* number of items in the id lookup table */        \
  gpr_##type##_idlut_item                                                   \
    *items;             /* items of the table */                            \
  gpr_index_t *indices; /* table indices used to avoid item fragmentation*/ \
  U16 freelist_enqueue; /* index of the previous available table index */   \
  U16 freelist_dequeue; /* index of the next available table index */       \
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
  in->id    += GPR_ID_ADD ;                                                 \
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
  gpr_index_t *in = &table->indices[id & GPR_INDEX_MASK];                   \
  return in->id == id && in->index != USHRT_MAX;                            \
}                                                                           \
                                                                            \
type *gpr_##type##_idlut_lookup(gpr_##type##_idlut_t *table, U32 id)        \
{                                                                           \
  return &table->items[table->indices[id & GPR_INDEX_MASK].index].value;    \
}                                                                           \
                                                                            \
void gpr_##type##_idlut_remove(gpr_##type##_idlut_t *table, U32 id)         \
{                                                                           \
  gpr_##type##_idlut_item *item;                                            \
  gpr_index_t *in = &table->indices[id & GPR_INDEX_MASK];                   \
                                                                            \
  /* copy the last item at position of the item to delete to avoid holes */ \
  item = &table->items[in->index];                                          \
  *item = table->items[--table->num_items];                                 \
  table->indices[item->id & GPR_INDEX_MASK].index = in->index;              \
                                                                            \
  in->index = USHRT_MAX;                                                    \
	table->freelist_enqueue = id & GPR_INDEX_MASK;                            \
  if(table->freelist_dequeue == table->max_items)                           \
    table->freelist_dequeue = table->freelist_enqueue;                      \
}                                                                           \
                                                                            \
gpr_##type##_idlut_item *                                                   \
gpr_##type##_idlut_items(gpr_##type##_idlut_t *table)                       \
{                                                                           \
  return table->items;                                                      \
}                                                                           \

#define gpr_idlut_item(type) gpr_##type##_idlut_item
#define gpr_idlut_t(type) gpr_##type##_idlut_t

// create an id lookup table allocating memory for "max_items"
#define gpr_idlut_init(type, table, max_items) \
  gpr_##type##_idlut_init(table, max_items)

// free the id lookup table and all its memory
#define gpr_idlut_free(type, table) \
  gpr_##type##_idlut_free(table)

// add the specified item to the table and return its id
#define gpr_idlut_add(type, table, item_value) \
  gpr_##type##_idlut_add(table, item_value)

// return 1 or 0 if the table contains or not the item
#define gpr_idlut_has(type, table, id) \
  gpr_##type##_idlut_has(table, id)

// return a pointer to the item with the "id" identifier
#define gpr_idlut_lookup(type, table, id) \
  gpr_##type##_idlut_lookup(table, id)

// remove the item with the "id" identifier (does not free memory)
#define gpr_idlut_remove(type, table, id) \
  gpr_##type##_idlut_remove(table, id)

// return the table item list
#define gpr_idlut_items(type, table) \
  gpr_##type##_idlut_items(table)

#endif // GPR_CONTAINERS_H