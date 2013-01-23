#ifndef GPR_BUFFER_H
#define GPR_BUFFER_H

#include "gpr_types.h"
#include "gpr_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  char *data;
  U32   size, capacity; 
  gpr_allocator_t *allocator; 
} gpr_buffer_t;

void  gpr_buffer_init    (gpr_buffer_t *buf, gpr_allocator_t *a);
void  gpr_buffer_destroy (gpr_buffer_t *buf);
void  gpr_buffer_clear   (gpr_buffer_t *buf);
void  gpr_buffer_reserve (gpr_buffer_t *buf, U32 capacity);
void  gpr_buffer_resize  (gpr_buffer_t *buf, U32 size);
char *gpr_buffer_cat     (gpr_buffer_t *buf, char *str);
char *gpr_buffer_ncat    (gpr_buffer_t *buf, char *src, U32 size);
char *gpr_buffer_xcat    (gpr_buffer_t *buf, char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // GPR_BUFFER_H
