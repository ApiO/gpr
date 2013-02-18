#ifndef GPR_JSON_H
#define GPR_JSON_H

// -------------------------------------------------------------------------
// JSON parser / writer
// -------------------------------------------------------------------------

#include "gpr_memory.h"
#include "gpr_buffer.h"
#include "gpr_idlut.h"
#include "gpr_hash.h"
#include "gpr_string_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
  gpr_idlut_t  nodes;
  gpr_hash_t   kv_access;
  gpr_string_pool_t *sp;
} gpr_json_t;

typedef enum
{
  GPR_JSON_FALSE = 0,
  GPR_JSON_TRUE,
  GPR_JSON_NULL,
  GPR_JSON_INTEGER,
  GPR_JSON_NUMBER,
  GPR_JSON_STRING,
  GPR_JSON_OBJECT,
  GPR_JSON_ARRAY
} gpr_json_type;

U64  gpr_json_init    (gpr_json_t *jsn, gpr_string_pool_t *sp, gpr_allocator_t *a);
void gpr_json_reserve (gpr_json_t *jsn, U32 capacity);
void gpr_json_destroy (gpr_json_t *jsn);

#ifdef __cplusplus
}
#endif

#endif // GPR_JSON_H