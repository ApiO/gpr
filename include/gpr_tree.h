#ifndef GPR_TREE_H
#define GPR_TREE_H

#include "gpr_memory.h"
#include "gpr_array.h"
#include "gpr_buffer.h"
#include "gpr_idlut.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPR_TREE_NO_NODE 0xffffffffu

typedef struct
{
  gpr_idlut_t  nodes;     // tree nodes
  gpr_array_t(U64) 
               item_ids;  // item external ids
  gpr_buffer_t item_vals; // items contained by the tree
  U64          root;
} gpr_tree_t;

typedef struct
{
  U64 id, child, next, prev, parent;
} gpr_tree_it;

U64   _gpr_tree_init        (gpr_tree_t *t, const U32 s, gpr_allocator_t *a, void *value);
void  _gpr_tree_destroy     (gpr_tree_t *t);
void  _gpr_tree_reserve     (gpr_tree_t *t, const U32 s, U32 capacity);

void *_gpr_tree_get         (gpr_tree_t *t, U64 id);
I32   _gpr_tree_has         (gpr_tree_t *t, U64 id);
I32   _gpr_tree_has_child   (gpr_tree_t *t, U64 id);
U64   _gpr_tree_add_child   (gpr_tree_t *t, const U32 s, U64 id, void *value);
void  _gpr_tree_remove      (gpr_tree_t *t, const U32 s, U64 id);

void *_gpr_tree_enter       (gpr_tree_t *t, gpr_tree_it *it);
void *_gpr_tree_step        (gpr_tree_t *t, gpr_tree_it *it, U64 id);

void *_gpr_tree_begin       (gpr_tree_t *t);
void *_gpr_tree_end         (gpr_tree_t *t, const U32 s);

#define gpr_tree_init(type, t, alct)        _gpr_tree_init(t, sizeof(type), alct)
#define gpr_tree_destroy(type, t)           _gpr_tree_destroy(t)
#define gpr_tree_reserve(type, t, capacity) _gpr_tree_reserve(t, sizeof(type), capacity)

#define gpr_tree_get(type, t, id)           ((type*)_gpr_tree_get(t, id))
#define gpr_tree_has(type, t, id)           _gpr_tree_has(t, id)
#define gpr_tree_has_child(type, t, id)     _gpr_tree_has_child(t, id)
#define gpr_tree_add_child(type, t, id, v)  _gpr_tree_add_child(t, sizeof(type), id, v) 
#define gpr_tree_remove(type, t, id)        _gpr_tree_remove(t, sizeof(type), id)

#define gpr_tree_enter(type, t, it)         ((type*)_gpr_tree_enter(t, it))
#define gpr_tree_step(type, t, it, id)      ((type*)_gpr_tree_step(t, it, id))

#define gpr_tree_begin(type, t)             ((type*)_gpr_tree_begin(t))
#define gpr_tree_end(type, t)               ((type*)_gpr_tree_end(t, sizeof(type)))

#ifdef __cplusplus
}
#endif

#endif // GPR_TREE_H