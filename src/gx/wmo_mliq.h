#ifndef GX_WMO_MLIQ_H
#define GX_WMO_MLIQ_H

#include <jks/array.h>
#include <jks/mat4.h>
#include <jks/vec3.h>
#include <jks/vec2.h>

#include <gfx/objects.h>

#include <stdbool.h>

#define WMO_MLIQ_LIQUIDS_COUNT 9

struct gx_wmo_mliq_init_data;
struct wow_wmo_group_file;
struct gx_wmo_instance;

struct gx_wmo_mliq_liquid
{
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	uint32_t indices_offset;
	uint32_t indices_nb;
};

struct gx_wmo_mliq_frame
{
	struct jks_array to_render[WMO_MLIQ_LIQUIDS_COUNT]; /* gx_wmo_instance_t* */
};

struct gx_wmo_mliq
{
	struct gx_wmo_mliq_init_data *init_data;
	struct gx_wmo_mliq_liquid liquids[WMO_MLIQ_LIQUIDS_COUNT];
	struct gx_wmo_mliq_frame render_frames[RENDER_FRAMES_COUNT];
	gfx_attributes_state_t attributes_state;
	gfx_buffer_t uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t vertexes_buffer;
	gfx_buffer_t indices_buffer;
	struct vec3f position;
	bool empty;
	bool in_render_list;
	bool in_render_lists[WMO_MLIQ_LIQUIDS_COUNT];
};

struct gx_wmo_mliq *gx_wmo_mliq_new(struct wow_wmo_group_file *file);
void gx_wmo_mliq_delete(struct gx_wmo_mliq *mliq);
void gx_wmo_mliq_initialize(struct gx_wmo_mliq *mliq);
void gx_wmo_mliq_clear_update(struct gx_wmo_mliq *mliq);
void gx_wmo_mliq_add_to_render(struct gx_wmo_mliq *mliq, struct gx_wmo_instance *instance);
void gx_wmo_mliq_render(struct gx_wmo_mliq *mliq, uint8_t type);

#endif
