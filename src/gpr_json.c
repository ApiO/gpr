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
  U64          next;
  U64          child;
  U32          num_children;
} node_t;

static U64 create_node(gpr_json_t *jsn, const char *name,  
                       gpr_json_type type, U64 value, U64 parent)
{
  U64    nid;
  node_t n;

  n.next         = NO_NODE;
  n.child        = NO_NODE;
  n.num_children = 0;
  n.value.type   = type;
  n.name         = name ? gpr_string_pool_get(jsn->sp, name) : 0;

  if (type == GPR_JSON_STRING) 
    n.value.string = gpr_strdup(jsn->sp->string_allocator, (char*)value);
  else n.value.raw = value;

  nid = gpr_idlut_add(node_t, &jsn->nodes, &n);
  gpr_idlut_lookup(node_t, &jsn->nodes, nid)->value.object = nid;

  if(parent == NO_NODE) return nid;

  // add the newly created node into the kv_hash
  if(name) gpr_hash_set(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(name, strlen(name), parent), &nid);

  // link the new node to the last child of his parent
  {
    node_t *p = gpr_idlut_lookup(node_t, &jsn->nodes, parent);
    ++p->num_children;

    if(p->child != NO_NODE)
      gpr_idlut_lookup(node_t, &jsn->nodes, nid)->next = p->child;

    return p->child = nid;
  }
}

static void reset_node(gpr_json_t *jsn, U64 id, gpr_json_type type, U64 value)
{
  node_t *n = gpr_idlut_lookup(node_t, &jsn->nodes, id);
  switch (n->value.type)
  {
  case GPR_JSON_OBJECT:
    {
      U64 tmp = n->child;
      while(tmp != NO_NODE)
      {
        U64 next = gpr_idlut_lookup(node_t, &jsn->nodes, tmp)->next;
        if(n->name) gpr_hash_remove(U64, &jsn->kv_access, 
          gpr_murmur_hash_64(n->name, strlen(n->name), id));
        gpr_idlut_remove(node_t, &jsn->nodes, tmp);
        tmp = next;
      }
    }
  case GPR_JSON_ARRAY:
    {
      U32 i = 0;
      while(i < n->num_children)
      {
        const U64 hash_key = gpr_murmur_hash_64(&i, 4, id);
        gpr_idlut_remove(node_t, &jsn->nodes, gpr_idlut_lookup(node_t, &jsn->nodes, 
          *gpr_hash_get(U64, &jsn->kv_access, hash_key))->value.object);
        gpr_hash_remove(U64, &jsn->kv_access, hash_key);
        ++i;
      }
    }
    break;
  case GPR_JSON_STRING:
    gpr_deallocate(jsn->sp->string_allocator, n->value.string);
    break;
  default:
    break;
  }
  n->num_children = 0;
  n->child = NO_NODE;
  n->value.type = type;
  n->value.raw  = value;
}

static void remove_node(gpr_json_t *jsn, U64 id, U64 parent)
{
  node_t *n = gpr_idlut_lookup(node_t, &jsn->nodes, id);

  reset_node(jsn, id, GPR_JSON_NULL, 0);

  if(n->name) gpr_hash_remove(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(n->name, strlen(n->name), parent));

  gpr_idlut_remove(node_t, &jsn->nodes, id);

  --gpr_idlut_lookup(node_t, &jsn->nodes, parent)->num_children;

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
      /*gpr_buffer_cat (buf, "[");
      if(n->child != NO_NODE) 
        write(jsn, n->child, buf, formated, depth+1);
      gpr_buffer_cat(buf, "]");
      break;*/

    default: break;
    }
    nid = n->next;
  }
}

void gpr_json_write(gpr_json_t *jsn, U64 obj, gpr_buffer_t *buf, I32 formated)
{
  write(jsn, obj, buf, formated, 0);
}

U64 gpr_json_create_object(gpr_json_t *jsn, U64 obj, const char *member)
{
  return create_node(jsn, member, GPR_JSON_OBJECT, 0, obj);
}

U64 gpr_json_create_array(gpr_json_t *jsn, U64 obj, const char *member)
{
  return create_node(jsn, member, GPR_JSON_ARRAY, 0, obj);
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

void gpr_json_set(gpr_json_t *jsn, U64 obj, const char *member, gpr_json_type type, U64 value)
{
  U64 *node_id = gpr_hash_get(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(member, strlen(member), obj));

  if(node_id != NULL)
    reset_node(jsn, *node_id, type, value);
  else 
    create_node(jsn, member, type, value, obj);
}

U32 gpr_json_array_size(gpr_json_t *jsn, U64 arr)
{
  return gpr_idlut_lookup(node_t, &jsn->nodes, arr)->num_children;
}

U64 gpr_json_array_add(gpr_json_t *jsn, U64 arr, gpr_json_type type, U64 value)
{
  const U64 nid   = create_node(jsn, 0, type, value, arr);
  const U32 index = gpr_idlut_lookup(node_t, &jsn->nodes, arr)->num_children-1;
  gpr_hash_set(U64, &jsn->kv_access, gpr_murmur_hash_64(&index, 4, arr), &nid);
  return nid;
}

gpr_json_val *gpr_json_array_get(gpr_json_t *jsn, U64 arr, U32 i)
{
  return &gpr_idlut_lookup(node_t, &jsn->nodes, 
    *gpr_hash_get(U64, &jsn->kv_access, gpr_murmur_hash_64(&i, 4, arr)))->value;
}

void gpr_json_array_remove(gpr_json_t *jsn, U64 arr, U32 i)
{
  const U64 hash_key = gpr_murmur_hash_64(&i, 4, arr);
  remove_node(jsn, *gpr_hash_get(U64, &jsn->kv_access, hash_key), arr);
  gpr_hash_remove(U64, &jsn->kv_access, hash_key);
}

void gpr_json_array_set(gpr_json_t *jsn, U64 arr, U32 i, 
                        gpr_json_type type, U64 value)
{
  reset_node(jsn, *gpr_hash_get(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(&i, 4, arr)), type, value);
}


