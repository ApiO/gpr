#ifndef GPR_MATH_H
#define GPR_MATH_H

#include "gpr_types.h"

typedef struct
{
  F32 x, y;
} Vector2;

typedef struct
{
  Vector2 a, b;
} AABB;

typedef struct
{
  F32 x, y, z;
} Vector3;

typedef struct
{
  F32 x, y, z, w;
} Vector4;

typedef struct
{
  Vector3 x, y, z;
} Matrix3;

typedef struct
{
  Vector4 x, y, z, t;
} Matrix4;

void gpr_math_V2_add(Vector2 *v1, Vector2 *v2, Vector2 *vr);
void gpr_math_V3_add(Vector2 *v1, Vector2 *v2, Vector2 *vr);
void gpr_math_V4_add(Vector2 *v1, Vector2 *v2, Vector2 *vr);

#endif // GPR_MATH_H