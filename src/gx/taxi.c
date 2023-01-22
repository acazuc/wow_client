#include "gx/frame.h"
#include "gx/taxi.h"

#include "graphics.h"
#include "shaders.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <gfx/device.h>

#include <libwow/mpq.h>
#include <libwow/dbc.h>

#include <jks/array.h>

MEMORY_DECL(GX);

struct gx_taxi *gx_taxi_new(uint32_t mapid)
{
	struct gx_taxi *taxi = mem_malloc(MEM_GX, sizeof(*taxi));
	if (!taxi)
	{
		LOG_ERROR("gx taxi allocation failed");
		return NULL;
	}
	taxi->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		taxi->uniform_buffers[i] = GFX_BUFFER_INIT();
	taxi->vertexes_buffer = GFX_BUFFER_INIT();
	taxi->indices_buffer = GFX_BUFFER_INIT();
	taxi->mapid = mapid;
	taxi->initialized = false;
	return taxi;
}

void gx_taxi_delete(struct gx_taxi *taxi)
{
	if (!taxi)
		return;
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &taxi->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &taxi->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &taxi->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &taxi->attributes_state);
	mem_free(MEM_GX, taxi);
}

static void initialize(struct gx_taxi *taxi)
{
	struct jks_array vertexes; /* struct vec3f */
	struct jks_array indices; /* uint16_t */
	jks_array_init(&vertexes, sizeof(struct vec3f), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&indices, sizeof(uint16_t), NULL, &jks_array_memory_fn_GX);
	struct wow_mpq_file *mpq_file = wow_mpq_get_file(g_wow->mpq_compound, "DBFILESCLIENT\\TAXIPATHNODE.DBC");
	if (!mpq_file)
	{
		LOG_ERROR("failed to get TaxiPathNode.dbc");
		goto end;
	}
	struct wow_dbc_file *file = wow_dbc_file_new(mpq_file);
	wow_mpq_file_delete(mpq_file);
	if (!file)
	{
		LOG_ERROR("failed to parse TaxiPathNode.dbc");
		goto end;
	}
	uint32_t prev = (uint32_t)-1;
	for (size_t i = 0; i < file->header.record_count; ++i)
	{
		struct wow_dbc_row row = wow_dbc_get_row(file, i);
		if (wow_dbc_get_u32(&row, 12) != taxi->mapid)
			continue;
		struct vec3f *vec = jks_array_grow(&vertexes, 1);
		if (!vec)
		{
			LOG_ERROR("failed to grow taxi vertexes buffer");
			wow_dbc_file_delete(file);
			goto end;
		}
		vec->x = wow_dbc_get_flt(&row, 16);
		vec->y = wow_dbc_get_flt(&row, 24);
		vec->z = -wow_dbc_get_flt(&row, 20);
		uint32_t path = wow_dbc_get_u32(&row, 4);
		if (path == prev)
		{
			uint16_t *indice = jks_array_grow(&indices, 2);
			if (!indice)
			{
				LOG_ERROR("failed to grow taxi indices buffer");
				wow_dbc_file_delete(file);
				goto end;
			}
			indice[0] = vertexes.size - 2;
			indice[1] = vertexes.size - 1;
		}
		prev = path;
	}
	wow_dbc_file_delete(file);
	if (!vertexes.size)
		goto end;
	gfx_create_buffer(g_wow->device, &taxi->vertexes_buffer, GFX_BUFFER_VERTEXES, vertexes.data, sizeof(struct vec3f) * vertexes.size, GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &taxi->indices_buffer, GFX_BUFFER_INDICES, indices.data, sizeof(uint16_t) * indices.size, GFX_BUFFER_IMMUTABLE);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_create_buffer(g_wow->device, &taxi->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_aabb_mesh_block), GFX_BUFFER_STREAM);
	gfx_attribute_bind_t binds[] =
	{
		{&taxi->vertexes_buffer, sizeof(struct vec3f), 0},
	};
	gfx_create_attributes_state(g_wow->device, &taxi->attributes_state, binds, sizeof(binds) / sizeof(*binds), &taxi->indices_buffer, GFX_INDEX_UINT16);
end:
	taxi->indices_nb = indices.size;
	jks_array_destroy(&vertexes);
	jks_array_destroy(&indices);
	taxi->initialized = true;
}

void gx_taxi_render(struct gx_taxi *taxi)
{
	if (!taxi->initialized)
		initialize(taxi);
	if (!taxi->indices_nb)
		return;
	gfx_bind_attributes_state(g_wow->device, &taxi->attributes_state, &g_wow->graphics->aabb_input_layout);
	struct shader_aabb_mesh_block mesh_block;
	mesh_block.mvp = g_wow->draw_frame->view_vp;
	VEC4_SET(mesh_block.color, 1, 1, 1, 1);
	gfx_set_buffer_data(&taxi->uniform_buffers[g_wow->draw_frame_id], &mesh_block, sizeof(mesh_block), 0);
	gfx_bind_constant(g_wow->device, 0, &taxi->uniform_buffers[g_wow->draw_frame_id], sizeof(mesh_block), 0);
	gfx_set_line_width(g_wow->device, 3);
	gfx_draw_indexed(g_wow->device, taxi->indices_nb, 0);
}
