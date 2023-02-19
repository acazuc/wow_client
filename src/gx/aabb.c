#include "gx/aabb.h"

#include "graphics.h"
#include "wow.h"

#include <gfx/device.h>

void gx_aabb_init(struct gx_aabb *aabb, struct vec4f color, float line_width)
{
	aabb->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		aabb->uniform_buffers[i] = GFX_BUFFER_INIT();
	aabb->positions_buffer = GFX_BUFFER_INIT();
	aabb->indices_buffer = GFX_BUFFER_INIT();
	aabb->color = color;
	aabb->line_width = line_width;
	aabb->flags = 0;
}

void gx_aabb_destroy(struct gx_aabb *aabb)
{
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_delete_buffer(g_wow->device, &aabb->uniform_buffers[i]);
	gfx_delete_buffer(g_wow->device, &aabb->positions_buffer);
	gfx_delete_buffer(g_wow->device, &aabb->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &aabb->attributes_state);
}

static void update_vertexes(struct gx_aabb *aabb)
{
	struct vec3f vertexes[8] =
	{
		{aabb->aabb.p0.x, aabb->aabb.p0.y, aabb->aabb.p0.z},
		{aabb->aabb.p1.x, aabb->aabb.p0.y, aabb->aabb.p0.z},
		{aabb->aabb.p0.x, aabb->aabb.p1.y, aabb->aabb.p0.z},
		{aabb->aabb.p1.x, aabb->aabb.p1.y, aabb->aabb.p0.z},
		{aabb->aabb.p0.x, aabb->aabb.p0.y, aabb->aabb.p1.z},
		{aabb->aabb.p1.x, aabb->aabb.p0.y, aabb->aabb.p1.z},
		{aabb->aabb.p0.x, aabb->aabb.p1.y, aabb->aabb.p1.z},
		{aabb->aabb.p1.x, aabb->aabb.p1.y, aabb->aabb.p1.z},
	};
	gfx_set_buffer_data(&aabb->positions_buffer, vertexes, sizeof(vertexes), 0);
}

static void initialize(struct gx_aabb *aabb)
{
	gfx_create_buffer(g_wow->device, &aabb->positions_buffer, GFX_BUFFER_VERTEXES, NULL, sizeof(struct vec3f) * 8, GFX_BUFFER_STATIC);

	static const uint16_t indices[24] =
	{
		0, 1, 1, 3,
		3, 2, 2, 0,
		0, 4, 1, 5,
		2, 6, 3, 7,
		4, 5, 5, 7,
		7, 6, 6, 4,
	};
	aabb->indices_nb = sizeof(indices) / sizeof(*indices);
	gfx_create_buffer(g_wow->device, &aabb->indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
	const struct gfx_attribute_bind binds[] =
	{
		{&aabb->positions_buffer},
	};
	gfx_create_attributes_state(g_wow->device, &aabb->attributes_state, binds, sizeof(binds) / sizeof(*binds), &aabb->indices_buffer, GFX_INDEX_UINT16);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
		gfx_create_buffer(g_wow->device, &aabb->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_aabb_mesh_block), GFX_BUFFER_STREAM);
}

void gx_aabb_update(struct gx_aabb *aabb, const struct mat4f *mvp)
{
	aabb->mesh_block.mvp = *mvp;
	aabb->mesh_block.color = aabb->color;
}

void gx_aabb_render(struct gx_aabb *aabb)
{
	if (!(aabb->flags & GX_AABB_INIT))
	{
		initialize(aabb);
		aabb->flags |= GX_AABB_INIT;
	}
	if (aabb->flags & GX_AABB_DIRTY)
	{
		update_vertexes(aabb);
		aabb->flags &= ~GX_AABB_DIRTY;
	}
	gfx_bind_attributes_state(g_wow->device, &aabb->attributes_state, &g_wow->graphics->aabb_input_layout);
	gfx_set_line_width(g_wow->device, aabb->line_width);
	gfx_set_buffer_data(&aabb->uniform_buffers[g_wow->draw_frame_id], &aabb->mesh_block, sizeof(aabb->mesh_block), 0);
	gfx_bind_constant(g_wow->device, 0, &aabb->uniform_buffers[g_wow->draw_frame_id], sizeof(aabb->mesh_block), 0);
	gfx_draw_indexed(g_wow->device, aabb->indices_nb, 0);
}
