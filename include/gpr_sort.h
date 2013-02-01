#ifndef GPR_SORT_H
#define GPR_SORT_H

#include "gpr_types.h"
#include "gpr_allocator.h"

#define gpr_mergesort(arr, n, type, _compar, alc)                   \
do {                                                                \
	type *a2[2], *_a, *b;                                             \
	I32 curr, shift;                                                  \
                                                                    \
	a2[0] = (arr);                                                    \
	a2[1] = (type*)gpr_allocate((alc), (n)*sizeof(type));             \
	for (curr = 0, shift = 0; (1ul<<shift) < n; ++shift){             \
		_a = a2[curr]; b = a2[1-curr];                                  \
		if (shift == 0) {                                               \
			type *p = b, *i, *eb = _a + (n);                              \
			for (i = _a; i < eb; i += 2){                                 \
				if (i == eb - 1) *p++ = *i;                                 \
				else {                                                      \
					if (_compar((*(i+1)), (*i)))                              \
						{*p++ = *(i+1); *p++ = *i;}                             \
					else                                                      \
						{*p++ = *i; *p++ = *(i+1);}                             \
				}                                                           \
			}                                                             \
		} else {                                                        \
			U32 i, step = 1ul<<shift;                                     \
			for (i = 0; i < (n); i += step<<1) {                          \
				type *p, *j, *k, *ea, *eb;                                  \
				if ((n) < i + step)                                         \
					{ ea = _a + (n); eb = _a; }                               \
				else {                                                      \
					ea = _a + i + step;                                       \
					eb = _a + ((n) < i + (step<<1)? (n) : i + (step<<1));     \
				}                                                           \
				j = _a + i; k = _a + i + step; p = b + i;                   \
				while (j < ea && k < eb) {                                  \
					if (_compar((*k), (*j))) *p++ = *k++;                     \
					else *p++ = *j++;                                         \
				}                                                           \
				while (j < ea) *p++ = *j++;                                 \
				while (k < eb) *p++ = *k++;                                 \
			}                                                             \
		}                                                               \
		curr = 1 - curr;                                                \
	}                                                                 \
	if (curr == 1) {                                                  \
		type *p = a2[0], *i = a2[1], *eb = (arr) + (n);                 \
		for (; p < eb; ++i) *p++ = *i;                                  \
	}                                                                 \
  gpr_deallocate((alc), a2[1]);                                     \
} while(0)

#endif //GPR_SORT_H