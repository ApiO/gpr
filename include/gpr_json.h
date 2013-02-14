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

U64  gpr_json_init          (gpr_json_t *jsn, gpr_string_pool_t *sp, gpr_allocator_t *a);
void gpr_json_destroy       (gpr_json_t *jsn);

U64  gpr_json_parse         (gpr_json_t *jsn, U64 obj, const char *text);
void gpr_json_write         (gpr_json_t *jsn, U64 obj, gpr_buffer_t *buf, I32 formated);

U64  gpr_json_create_object (gpr_json_t *jsn, U64 obj, const char *member);
U64  gpr_json_create_array  (gpr_json_t *jsn, U64 obj, const char *member);

gpr_json_val *gpr_json_get  (gpr_json_t *jsn, U64 obj, const char *member);
I32  gpr_json_has           (gpr_json_t *jsn, U64 obj, const char *member);
void gpr_json_remove        (gpr_json_t *jsn, U64 obj, const char *member);
void gpr_json_rename        (gpr_json_t *jsn, U64 obj, const char *member);
void gpr_json_set           (gpr_json_t *jsn, U64 obj, const char *member, 
                             gpr_json_type type, U64 value);

#define gpr_json_set_null(jsn,obj,member)      gpr_json_set(jsn,obj,member,GPR_JSON_NULL,    0)
#define gpr_json_set_true(jsn,obj,member)      gpr_json_set(jsn,obj,member,GPR_JSON_TRUE,    0)
#define gpr_json_set_false(jsn,obj,member)     gpr_json_set(jsn,obj,member,GPR_JSON_FALSE,   0)
#define gpr_json_set_boolean(jsn,obj,member,b) gpr_json_set(jsn,obj,member,(b!=0),           0)
#define gpr_json_set_integer(jsn,obj,member,i) gpr_json_set(jsn,obj,member,GPR_JSON_INTEGER, i)
#define gpr_json_set_string(jsn,obj,member,s)  gpr_json_set(jsn,obj,member,GPR_JSON_STRING, (U64)s)
#define gpr_json_set_number(jsn,obj,member,n)  do{union{U64 asU64; F64 asF64;}_u;_u.asF64=n;\
                                               gpr_json_set(jsn,obj,member,GPR_JSON_NUMBER,_u.asU64);\
                                               }while(0)

U32  gpr_json_array_size         (gpr_json_t *jsn, U64 arr);
gpr_json_val *gpr_json_array_get (gpr_json_t *jsn, U64 arr, U32 i);
U64  gpr_json_array_add          (gpr_json_t *jsn, U64 arr, 
                                  gpr_json_type type, U64 value);
void gpr_json_array_set          (gpr_json_t *jsn, U64 arr, U32 i, 
                                  gpr_json_type type, U64 value);
void gpr_json_array_remove       (gpr_json_t *jsn, U64 arr, U32 i);

#define gpr_json_array_add_object(jsn,arr)    gpr_json_array_add(jsn,arr,GPR_JSON_OBJECT,  0)
#define gpr_json_array_add_array(jsn,arr)     gpr_json_array_add(jsn,arr,GPR_JSON_ARRAY,   0)
#define gpr_json_array_add_null(jsn,arr)      gpr_json_array_add(jsn,arr,GPR_JSON_NULL,    0)
#define gpr_json_array_add_true(jsn,arr)      gpr_json_array_add(jsn,arr,GPR_JSON_TRUE,    0)
#define gpr_json_array_add_false(jsn,arr)     gpr_json_array_add(jsn,arr,GPR_JSON_FALSE,   0)
#define gpr_json_array_add_boolean(jsn,arr,b) gpr_json_array_add(jsn,arr,(b!=0),           0)
#define gpr_json_array_add_integer(jsn,arr,i) gpr_json_array_add(jsn,arr,GPR_JSON_INTEGER, i)
#define gpr_json_array_add_string(jsn,arr,s)  gpr_json_array_add(jsn,arr,GPR_JSON_STRING, (U64)s)
#define gpr_json_array_add_number(jsn,arr,n)  do{union{U64 asU64; F64 asF64;}_u;_u.asF64=n;\
                                              gpr_json_array_add(jsn,arr,GPR_JSON_NUMBER,_u.asU64);\
                                              }while(0)

#define _gpr_json_array_madd(jsn,arr,src,size,proc)\
do{U32 _i=0;while(_i<size){proc(jsn,arr,src[_i]);++_i;}}while(0)

#define gpr_json_array_add_integers(jsn,arr,src,size) \
  _gpr_json_array_madd(jsn,arr,src,size,gpr_json_array_add_integer)
#define gpr_json_array_add_booleans(jsn,arr,src,size) \
  _gpr_json_array_madd(jsn,arr,src,size,gpr_json_array_add_boolean)
#define gpr_json_array_add_strings(jsn,arr,src,size) \
  _gpr_json_array_madd(jsn,arr,src,size,gpr_json_array_add_string)
#define gpr_json_array_add_numbers(jsn,arr,src,size) \
  _gpr_json_array_madd(jsn,arr,src,size,gpr_json_array_add_number)

#define gpr_json_array_set_null(jsn,pos,arr)      gpr_json_array_set(jsn,arr,pos,GPR_JSON_NULL,    0)
#define gpr_json_array_set_true(jsn,pos,arr)      gpr_json_array_set(jsn,arr,pos,GPR_JSON_TRUE,    0)
#define gpr_json_array_set_false(jsn,pos,arr)     gpr_json_array_set(jsn,arr,pos,GPR_JSON_FALSE,   0)
#define gpr_json_array_set_boolean(jsn,pos,arr,b) gpr_json_array_set(jsn,arr,pos,(b!=0),           0)
#define gpr_json_array_set_integer(jsn,pos,arr,i) gpr_json_array_set(jsn,arr,pos,GPR_JSON_INTEGER, i)
#define gpr_json_array_set_string(jsn,pos,arr,s)  gpr_json_array_set(jsn,arr,pos,GPR_JSON_STRING, (U64)s)
#define gpr_json_array_set_number(jsn,pos,arr,n)  do{union{U64 asU64; F64 asF64;}_u;_u.asF64=n;\
                                                  gpr_json_array_set(jsn,arr,pos,GPR_JSON_NUMBER,_u.asU64);\
                                                  }while(0)
#ifdef __cplusplus
}
#endif

#endif // GPR_JSON_H