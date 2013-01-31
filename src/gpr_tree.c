#include "gpr_tree.h"

typedef struct
{
  U64  child;     // id of the first child
  U64  next;      // id of the next brother
  U64  prev;      // id of the previous brother
  U64  parent;    // id of the parent node
  U32  value_pos; // starting position of the item in the buffer
} node_t;

U64 _gpr_tree_init(gpr_tree_t *t, const U32 s, gpr_allocator_t *a, void *value)
{
  gpr_idlut_init(node_t, &t->nodes, a);
  gpr_array_init(U64, &t->item_ids, a);
  gpr_buffer_init(&t->item_vals, a);
  t->root = _gpr_tree_add_child(t, s, GPR_TREE_NO_NODE, value);
}

void _gpr_tree_destroy(gpr_tree_t *t)
{
  gpr_idlut_destroy(node_t, &t->nodes);
  gpr_array_destroy(&t->item_ids);
  gpr_buffer_destroy(&t->item_vals);
}
void _gpr_tree_reserve(gpr_tree_t *t, const U32 s, U32 capacity)
{
  gpr_idlut_reserve(node_t, &t->nodes, capacity);
  gpr_array_reserve(U64, &t->item_ids, capacity);
  gpr_buffer_reserve(&t->item_vals, capacity * s);
}

void *_gpr_tree_get(gpr_tree_t *t, U64 node)
{
  return t->item_vals.data + 
    gpr_idlut_lookup(node_t, &t->nodes, node)->value_pos;
}

I32 _gpr_tree_has(gpr_tree_t *t, U64 id)
{
  return gpr_idlut_has(node_t, &t->nodes, id);
}

I32 _gpr_tree_has_child(gpr_tree_t *t, U64 id)
{
  return gpr_idlut_lookup(node_t, &t->nodes, id)->child != GPR_TREE_NO_NODE;
}

U64 _gpr_tree_add_child(gpr_tree_t *t, const U32 s, U64 id, void *value)
{
  node_t *child;
  U64     child_id;

  { // create & insert the child node
    node_t  new_node;
    new_node.parent = id;
    new_node.next   = GPR_TREE_NO_NODE;
    new_node.child  = GPR_TREE_NO_NODE;
    new_node.prev   = GPR_TREE_NO_NODE;

    child_id = gpr_idlut_add(node_t, &t->nodes, &new_node);
    child    = gpr_idlut_lookup(node_t, &t->nodes, child_id);
  }

  // copy the item in the buffer
  child->value_pos = gpr_buffer_ncat(&t->item_vals, (char*)value, s);
  gpr_array_push_back(U64, &t->item_ids, child_id);

  // root case
  if(id == GPR_TREE_NO_NODE) return child_id; 

  { // maintain node references
    node_t *parent = gpr_idlut_lookup(node_t, &t->nodes, id);

    if(parent->child == GPR_TREE_NO_NODE) 
      return parent->child = child_id;
    {
      node_t *first_child = gpr_idlut_lookup(node_t, &t->nodes, parent->child);
      child->next = parent->child;
      first_child->prev = child_id;
      parent->child = child_id;
    }
  } return child_id;
}

void _gpr_tree_remove(gpr_tree_t *t, const U32 s, U64 id)
{
  gpr_tree_it it;
  void *e = _gpr_tree_step(t, &it, id);

  while(e)
  {
    if(it.child != GPR_TREE_NO_NODE)
      _gpr_tree_remove(t, s, it.child);

    { // remove the item data
      U32 value_pos = gpr_idlut_lookup(node_t, &t->nodes, it.id)->value_pos;
      node_t *last_item_node = gpr_idlut_lookup(node_t, &t->nodes, 
        gpr_array_item(&t->item_ids, t->nodes.num_items-1));

      memcpy(&t->item_vals.data + value_pos, 
             &t->item_vals.data + last_item_node->value_pos, s);
      last_item_node->value_pos = value_pos;

      //remove the item key
      gpr_array_item(&t->item_ids, value_pos/s) = 
        gpr_array_pop_back(&t->item_ids);
    }
    // remove the tree node
    gpr_idlut_remove(node_t, &t->nodes, it.id);
    e = _gpr_tree_step(t, &it, it.next);
  }
}

void *_gpr_tree_enter(gpr_tree_t *t, gpr_tree_it *it)
{
  return _gpr_tree_step(t, it, t->root);
}

void *_gpr_tree_step(gpr_tree_t *t, gpr_tree_it *it, U64 id)
{
  node_t *node;
  if(id == GPR_TREE_NO_NODE) return NULL;

  node = gpr_idlut_lookup(node_t, &t->nodes, id);
  it->id     = id;
  it->child  = node->child;
  it->next   = node->next;
  it->prev   = node->prev;
  it->parent = node->parent;
  return t->item_vals.data + node->value_pos;
}

void *_gpr_tree_begin(gpr_tree_t *t)
{
  return t->item_vals.data;
}

void *_gpr_tree_end(gpr_tree_t *t, const U32 s)
{
  return t->item_vals.data + t->nodes.num_items*s;
}