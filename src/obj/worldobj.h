#ifndef WORLDOBJ_H
#define WORLDOBJ_H

#include "obj/object.h"

#include <jks/vec4.h>
#include <jks/vec3.h>

struct net_packet_writer;
struct net_packet_reader;
struct gx_m2_instance;

struct world_movement_data
{
	uint64_t transport_guid;
	float transport_x;
	float transport_y;
	float transport_z;
	float transport_w;
	uint32_t transport_time;
	uint32_t flags;
	uint8_t flags2;
	uint32_t time; /* milliseconds */
	uint32_t fall_time; /* milliseconds */
	float x;
	float y;
	float z;
	float w;
	float pitch;
	float jump_velocity;
	float jump_sin_angle;
	float jump_cos_angle;
	float jump_xy_speed;
	float unk1;
};

struct worldobj_vtable
{
	void (*add_to_render)(struct object *object);
};

struct worldobj
{
	struct object object;
	const struct worldobj_vtable *worldobj_vtable;
	struct gx_m2_instance *m2;
	struct world_movement_data movement_data;
	struct vec3f position;
	float orientation;
	float slope;
	uint64_t transport_guid;
	struct vec3f transport_position;
	float transport_orientation;
	float walk_speed;
	float run_speed;
	float walk_back_speed;
	float swim_speed;
	float swim_back_speed;
	float flight_speed;
	float flight_back_speed;
	float turn_rate;
};

void worldobj_send_move_packet(struct worldobj *worldobj, uint32_t opcode);
bool worldobj_write_movement_data(struct worldobj *worldobj, struct net_packet_writer *packet);
bool worldobj_read_movement_data(struct worldobj *worldobj, struct net_packet_reader *packet);
void worldobj_set_position(struct worldobj *worldobj, struct vec3f position);
void worldobj_set_orientation(struct worldobj *worldobj, float orientation);
void worldobj_set_slope(struct worldobj *worldobj, float slope);
float worldobj_get_speed(struct worldobj *worldobj, bool backward);
void worldobj_set_m2(struct worldobj *worldobj, const char *file);
void worldobj_add_to_render(struct worldobj *worldobj);

extern const struct worldobj_vtable worldobj_vtable;
extern const struct object_vtable worldobj_object_vtable;

#endif
