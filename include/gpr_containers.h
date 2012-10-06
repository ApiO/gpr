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
typedef struct gpr_index_t;

typedef struct
{
  U16  max_items; // maximum number of items available in the table
  SZ   sz_item;   // size of an item
  U16  num_items; // number of items in the id lookup table

  U8  *item_data; // byte array containing items data
  U32 *item_ids;  // item ids in the same order of item data

  struct
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

#endif // GPR_CONTAINERS_H