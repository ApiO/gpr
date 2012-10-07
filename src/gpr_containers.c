#include <stdlib.h>
#include "gpr_assert.h"
#include "gpr_containers.h"

#define INDEX_TO_DATA_POINTER(index) (table->item_data + (index * table->sz_item)) 

void gpr_idlut_init(gpr_idlut_t *table, U16 max_items, SZ sz_item)
{
  table->item_data = (U8*)malloc(max_items * sz_item);
  GPR_ASSERT_ALLOC(table->item_data);

  table->item_ids  = (U32*)malloc(max_items * sizeof(U32));
  GPR_ASSERT_ALLOC(table->item_ids);

  table->indices = (gpr_index_t*)malloc(max_items * sizeof(gpr_index_t));
  GPR_ASSERT_ALLOC(table->indices);

  table->max_items        = max_items;
  table->sz_item          = sz_item;
  table->num_items        = 0;
  table->freelist_dequeue = 0;
  table->freelist_enqueue = max_items;

  {
    U16 i;
    for(i = 0; i < table->max_items; ++i)
    {
      table->indices[i].id   = i;
      table->indices[i].next = i+1;
    }
  }
}

void gpr_idlut_free(gpr_idlut_t *table)
{
  free(table->item_ids);
  free(table->item_data);
  free(table->indices);
}

U32 gpr_idlut_add(gpr_idlut_t *table, const void *item)
{
  gpr_index_t *in = &table->indices[table->freelist_dequeue];

  in->id    += NEW_ITEM_ID_ADD;
  in->index  = table->num_items;

  memcpy(INDEX_TO_DATA_POINTER(in->index), item, table->sz_item);
  table->item_ids[table->num_items] = in->id;

  ++table->num_items;
  table->freelist_dequeue = in->next;
  
  return in->id;
}

I32 gpr_idlut_has(gpr_idlut_t *table, U32 id)
{
  gpr_index_t *in = &table->indices[id & INDEX_MASK];
  return in->id == id && in->index != USHRT_MAX;
}

void *gpr_idlut_lookup(gpr_idlut_t *table, U32 id)
{
  return INDEX_TO_DATA_POINTER(table->indices[id & INDEX_MASK].index);
}

void gpr_idlut_remove(gpr_idlut_t *table, U32 id)
{
  gpr_index_t *in = &table->indices[id & INDEX_MASK];

  --table->num_items;

  // copy the last item at position of the item to delete to avoid holes
  memcpy(INDEX_TO_DATA_POINTER(in->index),  
         INDEX_TO_DATA_POINTER(table->num_items),
         table->sz_item);

  table->item_ids[in->index] = table->item_ids[table->num_items];

  // update the table index of the moved last item
  table->indices[table->item_ids[table->num_items] & INDEX_MASK].index = in->index;

  // free this table index
  in->index = USHRT_MAX;
  table->indices[table->freelist_enqueue].next = id & INDEX_MASK;
	table->freelist_enqueue = id & INDEX_MASK;

  if(table->freelist_dequeue == table->max_items)
    table->freelist_dequeue = table->freelist_enqueue;
}

void *gpr_idlut_items(gpr_idlut_t *table)
{
  return (void*) table->item_data;
}