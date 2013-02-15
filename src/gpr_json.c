#include <string.h>
#include "gpr_assert.h"
#include "gpr_json.h"
#include "gpr_idlut.h"
#include "gpr_murmur_hash.h"

#define NO_NODE 0xffffffffu

typedef struct
{
  char        *name;
  gpr_json_val value;
  U64          prev;  // parent if first child
  U64          next;
  U64          child;
} node_t;

static U64 create_node(gpr_json_t *jsn, const char *name,  
                       gpr_json_type type, U64 value, U64 parent)
{
  U64    id;
  node_t n, *np;

  n.prev         = NO_NODE;
  n.next         = NO_NODE;
  n.child        = NO_NODE;
  n.value.type   = type;
  n.name         = name ? gpr_string_pool_get(jsn->sp, name) : 0;

  if (type == GPR_JSON_STRING) 
    n.value.string = gpr_strdup(jsn->sp->string_allocator, (char*)value);
  else n.value.raw = value;

  id = gpr_idlut_add(node_t, &jsn->nodes, &n);
  np = gpr_idlut_lookup(node_t, &jsn->nodes, id);
  np->value.object = id;

  // add the newly created node into the kv_hash
  if(name) 
  {
    gpr_assert(!gpr_hash_has(U64, &jsn->kv_access, 
      gpr_murmur_hash_64(name, strlen(name), parent)), "hash collision!");
    gpr_hash_set(U64, &jsn->kv_access, 
      gpr_murmur_hash_64(name, strlen(name), parent), &id);
  }

  if(parent == NO_NODE) return id;

  // link the new node to the last child of his parent
  {
    node_t *p = gpr_idlut_lookup(node_t, &jsn->nodes, parent); // parent
    if(p->child == NO_NODE) 
    {
      np->prev = p->value.object;
      return p->child = id;
    }

    p = gpr_idlut_lookup(node_t, &jsn->nodes, p->child);
    while(p->next != NO_NODE)
      p = gpr_idlut_lookup(node_t, &jsn->nodes, p->next);
    np->prev = p->value.object;
    return p->next = id;
  }
}

static void remove_node(gpr_json_t *jsn, U64 id, U64 parent)
{
  node_t *n = gpr_idlut_lookup(node_t, &jsn->nodes, id);
  switch (n->value.type)
  {
  case GPR_JSON_OBJECT:
  case GPR_JSON_ARRAY:
    {
      U64 child = n->child;
      while(child != NO_NODE)
      {
        U64 next = gpr_idlut_lookup(node_t, &jsn->nodes, child)->next;
        remove_node(jsn, child, id);
        child = next;
      }
    } 
    break;
  case GPR_JSON_STRING:
    gpr_deallocate(jsn->sp->string_allocator, n->value.string);
  default:
    break;
  }
  if(n->name) 
  {
    gpr_string_pool_release(jsn->sp, n->name);
    gpr_hash_remove(U64, &jsn->kv_access, 
      gpr_murmur_hash_64(n->name, strlen(n->name), parent));
  }

  if(n->prev != NO_NODE) gpr_idlut_lookup(node_t, &jsn->nodes, n->prev)->next = n->next;
  if(n->next != NO_NODE) gpr_idlut_lookup(node_t, &jsn->nodes, n->next)->prev = n->prev;
  gpr_idlut_remove(node_t, &jsn->nodes, id);
}

static void reset_node(gpr_json_t *jsn, U64 id, gpr_json_type type, U64 value)
{
  node_t *n = gpr_idlut_lookup(node_t, &jsn->nodes, id);
  switch (n->value.type)
  {
  case GPR_JSON_OBJECT:
  case GPR_JSON_ARRAY:
    {
      U64 child = n->child;
      while(child != NO_NODE)
      {
        U64 next = gpr_idlut_lookup(node_t, &jsn->nodes, child)->next;
        remove_node(jsn, child, id);
        child = next;
      }
    }
  case GPR_JSON_STRING:
    gpr_string_pool_release(jsn->sp, n->value.string);
  default:
    break;
  }
  n->child = NO_NODE;
  n->value.type = type;
  if (type == GPR_JSON_STRING) 
    n->value.string = gpr_strdup(jsn->sp->string_allocator, (char*)value);
  else n->value.raw = value;
}

U64 gpr_json_init(gpr_json_t *jsn, gpr_string_pool_t *sp, 
                  gpr_allocator_t *a)
{
  gpr_idlut_init(node_t, &jsn->nodes,   a);
  gpr_hash_init (U64,    &jsn->kv_access, a);
  jsn->sp = sp;
  return create_node(jsn, 0, GPR_JSON_OBJECT, NO_NODE, NO_NODE);
}

void gpr_json_destroy(gpr_json_t *jsn)
{
  // destroy strings
  node_t *n = gpr_idlut_begin(node_t, &jsn->nodes);
  node_t *m = gpr_idlut_end  (node_t, &jsn->nodes);

  while(n < m)
  {
    if(n->value.type == GPR_JSON_STRING) 
      gpr_deallocate(jsn->sp->string_allocator, n->value.string);
    if(n->name) gpr_string_pool_release(jsn->sp, n->name);
    ++n;
  }

  // destroy containers
  gpr_idlut_destroy (gpr_json_t, &jsn->nodes);
  gpr_hash_destroy  (U64,        &jsn->kv_access);
}

static void write(gpr_json_t *jsn, U64 obj, gpr_buffer_t *buf, I32 formated, I32 depth)
{
  U64 nid = obj;
  while (nid != NO_NODE)
  {
    node_t *n = gpr_idlut_lookup(node_t, &jsn->nodes, nid);

    if(nid != obj) gpr_buffer_cat (buf, ",");

    if(formated) 
    {
      int i = 0;
      if(n->name) gpr_buffer_cat(buf, "\n");
      while(i++ < depth) gpr_buffer_cat(buf, "  ");
    }

    if(n->name) 
    { // display the property name
      gpr_buffer_cat(buf, "\"");
      gpr_buffer_cat(buf, (char*)n->name);
      gpr_buffer_cat(buf, "\":");
      if(formated) gpr_buffer_cat(buf, " ");
    }

    switch(n->value.type)
    {
    case GPR_JSON_FALSE:   gpr_buffer_cat (buf, "false"); break;
    case GPR_JSON_TRUE:    gpr_buffer_cat (buf, "true");  break;
    case GPR_JSON_NULL:    gpr_buffer_cat (buf, "null");  break;
    case GPR_JSON_INTEGER: gpr_buffer_xcat(buf, "%d", n->value.integer); break;
    case GPR_JSON_NUMBER:  gpr_buffer_xcat(buf, "%f", n->value.number);  break;

    case GPR_JSON_STRING:
      gpr_buffer_cat(buf, "\"");
      gpr_buffer_cat (buf, n->value.string);
      gpr_buffer_cat(buf, "\"");
      break;

    case GPR_JSON_OBJECT:  
      gpr_buffer_cat (buf, "{");
      if(n->child != NO_NODE) 
        write(jsn, n->child, buf, formated, depth+1);
      if(formated) 
      {
        int i = 0;
        gpr_buffer_cat(buf, "\n");
        while(i++ < depth) gpr_buffer_cat(buf, "  ");
      }
      gpr_buffer_cat(buf, "}");
      break;

    case GPR_JSON_ARRAY:
      gpr_buffer_cat (buf, "[");
      if(n->child != NO_NODE) 
        write(jsn, n->child, buf, formated, depth+1);
      gpr_buffer_cat(buf, "]");
      break;

    default: break;
    }
    nid = n->next;
  }
}

void gpr_json_write(gpr_json_t *jsn, U64 obj, gpr_buffer_t *buf, I32 formated)
{
  write(jsn, obj, buf, formated, 0);
}

I32 gpr_json_has(gpr_json_t *jsn, U64 obj, const char *member)
{
  return gpr_hash_has(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(member, strlen(member), obj));
}

gpr_json_val *gpr_json_get(gpr_json_t *jsn, U64 obj, const char *member)
{
  U64 *node_id = gpr_hash_get(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(member, strlen(member), obj));

  if(node_id == NULL) return NULL;

  return &gpr_idlut_lookup(node_t, &jsn->nodes, *node_id)->value;
}

void gpr_json_remove(gpr_json_t *jsn, U64 obj, const char *member)
{
  remove_node(jsn, *gpr_hash_get(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(member, strlen(member), obj)), obj);
}

void gpr_json_rename(gpr_json_t *jsn, U64 obj, const char *member)
{
  U64 key = gpr_murmur_hash_64(member, strlen(member), obj);
  U64 *old = gpr_hash_get(U64, &jsn->kv_access, key);
  if (old != NULL) remove_node(jsn, *old, obj);

  gpr_hash_set(U64, &jsn->kv_access, key, &obj);
}

U64 gpr_json_set(gpr_json_t *jsn, U64 obj, const char *member, gpr_json_type type, U64 value)
{
  U64 *id = gpr_hash_get(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(member, strlen(member), obj));

  if(id == NULL) return create_node(jsn, member, type, value, obj);

  reset_node(jsn, *id, type, value);
  return *id;
}

U64 gpr_json_array_add(gpr_json_t *jsn, U64 arr, gpr_json_type type, U64 value)
{
  return create_node(jsn, 0, type, value, arr);
}

U32 gpr_json_array_size(gpr_json_t *jsn, U64 arr)
{
  U32 size = 0;
  U64 next = gpr_idlut_lookup(node_t, &jsn->nodes, arr)->child;
  while(next != NO_NODE)
  {
    next = gpr_idlut_lookup(node_t, &jsn->nodes, next)->next;
    ++size;
  }
  return size;
}

static node_t *get_array_node(gpr_json_t *jsn, U64 arr, U32 i)
{
  node_t *n = gpr_idlut_lookup(node_t, &jsn->nodes, 
    gpr_idlut_lookup(node_t, &jsn->nodes, arr)->child);
  U32 pos = 0;

  while(pos < i)
  {
    n = gpr_idlut_lookup(node_t, &jsn->nodes, n->next);
    ++pos;
  } return n;
}

gpr_json_val *gpr_json_array_get(gpr_json_t *jsn, U64 arr, U32 i)
{
  return &get_array_node(jsn, arr, i)->value;
}

void gpr_json_array_remove(gpr_json_t *jsn, U64 arr, U32 i)
{
  gpr_assert_msg(i < gpr_json_array_size(jsn, arr), 
    "\"i\" is out of the array range");
  remove_node(jsn, get_array_node(jsn, arr, i)->value.object, arr);
}

U64 gpr_json_array_set(gpr_json_t *jsn, U64 arr, U32 i, 
                       gpr_json_type type, U64 value)
{
  gpr_assert_msg(i < gpr_json_array_size(jsn, arr), 
    "\"i\" is out of the array range");
  reset_node(jsn, get_array_node(jsn, arr, i)->value.object, type, value);
}