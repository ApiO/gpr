#include <string.h>
#include "gpr_json.h"
#include "gpr_idlut.h"
#include "gpr_murmur_hash.h"

#define NO_NODE 0

typedef struct
{
  U64          next;
  char        *name;
  gpr_json_val value;
} node_t;

static U64 create_node(gpr_json_t *jsn, const char *name,  
                       gpr_json_type type, U64 value, U64 parent)
{
  U64    nid;
  node_t n;

  n.next       = NO_NODE;
  n.value.type = type;
  n.name       = name ? gpr_strdup(name, jsn->nodes.items.allocator) : 0;
  if (type == GPR_JSON_STRING) 
    n.value.string = gpr_strdup((char*)value, jsn->nodes.items.allocator);
  else 
    n.value.object = value;

  nid = gpr_idlut_add(node_t, &jsn->nodes, &n);

  if(parent == NO_NODE) return nid;

  if (name)
  {
    // add the newly created node into the kv_hash
    gpr_hash_set(U64, &jsn->kv_access, 
      gpr_murmur_hash_64(name, strlen(name), parent), &nid);
  }

  // link the new node to the last child of his parent
  {
    node_t *p = gpr_idlut_lookup(node_t, &jsn->nodes, parent); 
    if(p->value.object == NO_NODE) return p->value.object = nid;

    p = gpr_idlut_lookup(node_t, &jsn->nodes, p->value.object);
    while(p->next != NO_NODE)
      p = gpr_idlut_lookup(node_t, &jsn->nodes, p->next);
    p->next = nid;
  }

  return nid;
}

static void remove_node(gpr_json_t *jsn, node_t *node)
{
}

static void reset_node(gpr_json_t *jsn, node_t *node, gpr_json_type type, U64 value)
{
  switch (node->value.type)
  {
  case GPR_JSON_OBJECT:
  case GPR_JSON_ARRAY:
    remove_node(jsn, gpr_idlut_lookup(node_t, &jsn->nodes, node->value.object));
    value = (U64)gpr_strdup((char *)value, jsn->nodes.items.allocator);
    break;
  case GPR_JSON_STRING:
    gpr_deallocate(jsn->nodes.items.allocator, node->value.string);
    break;
  default:
    break;
  }
  node->value.type   = type;
  node->value.object = value;
}

U64 gpr_json_init(gpr_json_t *jsn, gpr_allocator_t *a)
{
  gpr_idlut_init(node_t, &jsn->nodes,   a);
  gpr_hash_init (U64,    &jsn->kv_access, a);
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
      gpr_deallocate(jsn->nodes.items.allocator, n->value.string);
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
      gpr_buffer_cat(buf, n->name);
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
      if(n->value.object != NO_NODE) 
        write(jsn, n->value.object, buf, formated, depth+1);
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
      if(n->value.object != NO_NODE) 
        write(jsn, n->value.arr, buf, formated, depth+1);
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

}

void gpr_json_set(gpr_json_t *jsn, U64 obj, const char *member, gpr_json_type type, U64 value)
{
  U64 *node_id = gpr_hash_get(U64, &jsn->kv_access, 
    gpr_murmur_hash_64(member, strlen(member), obj));

  if(node_id != NULL)
    reset_node(jsn, gpr_idlut_lookup(node_t, &jsn->nodes, *node_id), type, value);
  else 
    create_node(jsn, member, type, value, obj);
}