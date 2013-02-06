#include "gpr_tmp_allocator.h"

#define GPR_CHUNK_SIZE 4*1024

static void *allocate(void *a, U32 size, U32 align)
{
  void *result = 0;
  gpr_tmp_allocator_64_t *al = (gpr_tmp_allocator_64_t*) a;
  al->p = (char*)gpr_align_forward(al->p, align);
  if ((int)size > al->end - al->p) 
  {
    U32 to_allocate = sizeof(void *) + size + align;
    if (to_allocate < al->chunk_size)
      to_allocate = al->chunk_size;
    al->chunk_size *= 2;
    {
      void *p = gpr_allocate(al->backing, to_allocate);
      *(void **)al->start = p;
      al->p = al->start = (char*)p;
      al->end = al->start + to_allocate;
      *(void **)al->start = 0;
      al->p += sizeof(void*);
      gpr_align_forward(p, align);
    }
  }
  result = al->p;
  al->p += size;
  return result;
}

static void deallocate    (void *a, void *p) {}
static U32  allocated_for (void *a, void *p) { return GPR_SIZE_NOT_TRACKED; }
static U32  allocated_tot (void *a)          { return GPR_SIZE_NOT_TRACKED; }

void gpr_tmp_allocator_init(void *a, U32 size)
{
  gpr_tmp_allocator_64_t *al = (gpr_tmp_allocator_64_t*) a;
  al->p   = al->start = al->buffer;
  al->end = al->start + size;
  *(void **)al->start = 0;
  al->p += sizeof(void *);
  al->chunk_size = GPR_CHUNK_SIZE;
  al->backing    = gpr_scratch_allocator;

  gpr_set_allocator_functions(al,
    allocate,
    deallocate,
    allocated_for,
    allocated_tot);
}

void gpr_tmp_allocator_destroy (void *a)
{
  gpr_tmp_allocator_64_t *al = (gpr_tmp_allocator_64_t*) a;
  void *p = *(void **)al->buffer;
  while (p) {
    void *next = *(void **)p;
    gpr_deallocate(al->backing, p);
    p = next;
  }
}