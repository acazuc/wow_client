#ifndef PHYSICS_H
#define PHYSICS_H

#include <jks/vec3.h>

#include <stdbool.h>

#define GRAVITY 19.29110336303711f
#define WALL_CLIMB 0.64278764f
#define EPSILON 0.0000001f
#define SPHERE_RADIUS 0.5f
#define JUMP_VELOCITY -7.9555473f

struct collision_triangle;
struct jks_array;

bool find_nearest_triangle(struct jks_array *triangles, struct vec3f src, struct vec3f dir, float dir_len, struct collision_triangle **trianglep, struct vec3f *norm, float *t);

#endif
