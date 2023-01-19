#include "phys/physics.h"

#include "map/map.h"

#include <stdlib.h>
#include <math.h>

static float triangle_collide(struct collision_triangle *triangle, struct vec3f src, struct vec3f dir, struct vec3f *normp)
{
	struct vec3f e2;
	struct vec3f e1;
	struct vec3f p;
	VEC3_SUB(e1, triangle->points[1], triangle->points[0]);
	VEC3_SUB(e2, triangle->points[2], triangle->points[0]);
	VEC3_CROSS(p, dir, e2);
	float det = VEC3_DOT(e1, p);
	if (det < EPSILON)
		return -1;
	det = 1. / det;
	struct vec3f tt;
	VEC3_SUB(tt, src, triangle->points[0]);
	float u = VEC3_DOT(tt, p) * det;
	if (u < -EPSILON || u > 1 + EPSILON)
		return -1;
	struct vec3f q;
	VEC3_CROSS(q, tt, e1);
	float v = VEC3_DOT(dir, q) * det;
	if (v < -EPSILON || u + v > 1 + EPSILON)
		return -1;
	struct vec3f norm;
	VEC3_CROSS(norm, e1, e2);
#if 1 /* norm test (should be set in collision_triangle instead) */
	if (VEC3_DOT(dir, norm) >= -EPSILON)
		return -1;
#endif
	*normp = norm;
	return VEC3_DOT(e2, q) * det;
}

bool find_nearest_triangle(struct jks_array *triangles, struct vec3f src, struct vec3f dir, float dir_len, struct collision_triangle **trianglep, struct vec3f *norm, float *t)
{
	float lowest = INFINITY;
	struct collision_triangle *best = NULL;
	struct vec3f best_norm;
	for (size_t i = 0; i < triangles->size; ++i)
	{
		struct collision_triangle *triangle = JKS_ARRAY_GET(triangles, i, struct collision_triangle);
		struct vec3f triangle_norm;
		float tmp = triangle_collide(triangle, src, dir, &triangle_norm);
		if (tmp < 0 || tmp > dir_len + EPSILON)
			continue;
		if (tmp >= lowest)
			continue;
		best_norm = triangle_norm;
		best = triangle;
		lowest = tmp;
	}
	if (!best)
		return false;
	if (norm)
		VEC3_NORMALIZE(float, *norm, best_norm);
	if (trianglep)
		*trianglep = best;
	if (t)
		*t = lowest;
	return true;
}
