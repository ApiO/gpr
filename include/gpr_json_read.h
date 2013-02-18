#ifndef GPR_JSON_READ_H
#define GPR_JSON_READ_H

#include "gpr_json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  I32 type;
  union {
    U64 arr;
    U64 object;
  };
  union {
    I32   integer;
    F64   number;
    char *string;
    U64   raw;
  };
} gpr_json_val;

I32  gpr_json_parse (gpr_json_t *jsn, U64 obj, const char *text);

gpr_json_val 
    *gpr_json_get   (gpr_json_t *jsn, U64 obj, const char *member);
I32  gpr_json_has   (gpr_json_t *jsn, U64 obj, const char *member);

gpr_json_val 
    *gpr_json_array_get  (gpr_json_t *jsn, U64 arr, U32 i);
U32  gpr_json_array_size (gpr_json_t *jsn, U64 arr);

#ifdef __cplusplus
}
#endif

#endif //GPR_JSON_READ_H