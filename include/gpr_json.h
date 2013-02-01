#ifndef GPR_JSON_H
#define GPR_JSON_H

// -------------------------------------------------------------------------
// JSON parser / writer
// -------------------------------------------------------------------------
// The parser is in-place : the source string must be kept in memory
// during the use of the corresponding json_t context.
// -------------------------------------------------------------------------

#include "gpr_buffer.h"

typedef enum {
  GPR_JSON_ERR_INVAL = -1, // invalid character inside JSON string
  GPR_JSON_ERR_PART  = -2, // unterminated JSON string
  GPR_JSON_OK        = 0   // everything was fine
} gpr_json_err_t;

typedef enum
{
  GPR_JSON_NULL = 0,
  GPR_JSON_BOOLEAN,
  GPR_JSON_INTEGER,
  GPR_JSON_NUMBER,
  GPR_JSON_STRING,
  GPR_JSON_OBJECT
} gpr_json_type_t;

typedef struct gpr_json_s    gpr_json_t;
typedef struct gpr_json_it_s gpr_json_it;

typedef struct
{
  gpr_json_type_t type;
  union {
    I32         integer;
    F64         number;
    char       *string;
    gpr_json_t *object;
  };
} gpr_json_v;

gpr_json_v gpr_json_get(gpr_json_t *object, const char *member);
void       gpr_json_set(gpr_json_t *object, const char *member, gpr_json_v value);

#endif // GPR_JSON_H