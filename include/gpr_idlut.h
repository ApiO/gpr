#ifndef GPR_IDLUT_H
#define GPR_IDLUT_H

#include "gpr_memory.h"
#include "gpr_array.h"
#include "gpr_buffer.h"

// -------------------------------------------------------------------------
// Id Lookup Table Container 
// -------------------------------------------------------------------------
// Design & Implementation inspired from
// http://www.altdevblogaday.com/2011/09/23/managing-decoupling-part-4-the-id-lookup-table/
// -------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  U64  id;        // external id
  U32  item_pos; // starting position of the item in the buffer
  U32  next;      // index of the next gpr_index_t
} gpr_idlut_index_t;

typedef struct
{
  gpr_array_t(gpr_idlut_index_t) 
               indices;          // table indices
  gpr_array_t(U64) 
               ids;              // item external ids
  gpr_buffer_t items;            // items contained by the table
  U32          num_items;        // number of items in the table
  U32          freelist_enqueue; // index of the previous available table index
  U32          freelist_dequeue; // index of the next available table index
} gpr_idlut_t;

void  _gpr_idlut_init    (gpr_idlut_t *t, const U32 s, gpr_allocator_t *a);
void  _gpr_idlut_destroy (gpr_idlut_t *t);
void  _gpr_idlut_reserve (gpr_idlut_t *t, const U32 s, U32 capacity);
U64   _gpr_idlut_add     (gpr_idlut_t *t, const U32 s, void *item);
I32   _gpr_idlut_has     (gpr_idlut_t *t, U64 id);
void *_gpr_idlut_lookup  (gpr_idlut_t *t, U64 id);
void  _gpr_idlut_remove  (gpr_idlut_t *t, const U32 s, U64 id);
void *_gpr_idlut_begin   (gpr_idlut_t *t);
void *_gpr_idlut_end     (gpr_idlut_t *t, const U32 s);

#define gpr_idlut_init(type, t, alct)        _gpr_idlut_init(t, sizeof(type), alct)
#define gpr_idlut_destroy(type, t)           _gpr_idlut_destroy(t)
#define gpr_idlut_reserve(type, t, capacity) _gpr_idlut_reserve(t, sizeof(type), capacity)
#define gpr_idlut_add(type, t, item)         _gpr_idlut_add(t, sizeof(type), item) 
#define gpr_idlut_has(type, t, id)           _gpr_idlut_has(t, id)
#define gpr_idlut_lookup(type, t, id)        (type*)_gpr_idlut_lookup(t, id)
#define gpr_idlut_remove(type, t, id)        _gpr_idlut_remove(t, sizeof(type), id)
#define gpr_idlut_begin(type, t)             (type*)_gpr_idlut_begin(t)
#define gpr_idlut_end(type, t)               (type*)_gpr_idlut_end(t, sizeof(type))

#ifdef __cplusplus
}
#endif


#endif // GPR_IDLUT_H