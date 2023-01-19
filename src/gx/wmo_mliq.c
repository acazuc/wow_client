#include "gx/wmo_mliq.h"
#include "gx/frame.h"
#include "gx/wmo.h"

#include "graphics.h"
#include "shaders.h"
#include "camera.h"
#include "memory.h"
#include "const.h"
#include "blp.h"
#include "log.h"
#include "wow.h"

#include <libwow/wmo_group.h>

#include <gfx/device.h>

#include <sys/types.h>

MEMORY_DECL(GX);

struct gx_wmo_mliq_init_data
{
	struct jks_array indices[WMO_MLIQ_LIQUIDS_COUNT]; /* uint16_t */
	struct jks_array vertexes; /* struct shader_mliq_input */
};

static void init_data_init(struct gx_wmo_mliq_init_data *init_data)
{
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
		jks_array_init(&init_data->indices[i], sizeof(uint16_t), NULL, &jks_array_memory_fn_GX);
	jks_array_init(&init_data->vertexes, sizeof(struct shader_mliq_input), NULL, &jks_array_memory_fn_GX);
}

static void init_data_delete(struct gx_wmo_mliq_init_data *init_data)
{
	if (!init_data)
		return;
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
		jks_array_destroy(&init_data->indices[i]);
	jks_array_destroy(&init_data->vertexes);
	mem_free(MEM_GX, init_data);
}

struct gx_wmo_mliq *gx_wmo_mliq_new(struct wow_wmo_group_file *file)
{
	struct gx_wmo_mliq *mliq = mem_zalloc(MEM_GX, sizeof(*mliq));
	if (!mliq)
		return NULL;
	wow_mliq_t *wow_mliq = &file->mliq;
	VEC3_SET(mliq->position, wow_mliq->header.coords.x, 0, -wow_mliq->header.coords.y);
	for (size_t x = 0; x < wow_mliq->header.xtiles; ++x)
	{
		for (size_t z = 0; z < wow_mliq->header.ytiles; ++z)
		{
			size_t p1 = x + z * wow_mliq->header.xverts;
			size_t p2 = p1 + 1;
			size_t p3 = p1 + wow_mliq->header.xverts + 1;
			size_t p4 = p1 + wow_mliq->header.xverts;
			uint8_t liquid_id = wow_mliq->tiles[x + z * wow_mliq->header.xtiles] & WOW_MLIQ_TILE_LIQUID_TYPE;
			if (liquid_id == 0xf) /* no liquid flag */
				continue;
			if (liquid_id >= WMO_MLIQ_LIQUIDS_COUNT)
			{
				LOG_ERROR("unknown WMO MLIQ id: %u", (int)liquid_id);
				continue;
			}
			if (liquid_id == 5)
			{
				LOG_ERROR("invalid WMO MLIQ id: 5");
				continue;
			}
			if (!mliq->init_data)
			{
				mliq->init_data = mem_malloc(MEM_GX, sizeof(*mliq->init_data));
				if (!mliq->init_data)
					goto err1;
				init_data_init(mliq->init_data);
			}
			uint16_t *data = jks_array_grow(&mliq->init_data->indices[liquid_id], 6);
			if (!data)
				goto err2;
			data[0] = p1;
			data[1] = p2;
			data[2] = p3;
			data[3] = p1;
			data[4] = p3;
			data[5] = p4;
		}
	}
	mliq->empty = true;
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
	{
		jks_array_init(&mliq->to_render[i], sizeof(struct gx_wmo_instance*), NULL, &jks_array_memory_fn_GX);
		if ((mliq->liquids[i].indices_nb = mliq->init_data->indices[i].size))
			mliq->empty = false; /* shouln't break to continue indices_nb */
	}
	if (!mliq->empty)
	{
		if (!jks_array_resize(&mliq->init_data->vertexes, wow_mliq->header.xverts * wow_mliq->header.yverts))
			goto err2;
		struct shader_mliq_input *vertex = JKS_ARRAY_GET(&mliq->init_data->vertexes, 0, struct shader_mliq_input);
		for (uint32_t z = 0; z < wow_mliq->header.yverts; ++z)
		{
			for (uint32_t x = 0; x < wow_mliq->header.xverts; ++x)
			{
				size_t n = z * wow_mliq->header.xverts + x;
				vertex[n].position.x = x / 8. * CHUNK_WIDTH;
				vertex[n].position.y = wow_mliq->vertexes[n].magma.height;
				vertex[n].position.z = -(ssize_t)z / 8. * CHUNK_WIDTH;
				size_t tilen = z * wow_mliq->header.xtiles + x;
				if (z == wow_mliq->header.yverts - 1)
					tilen -= wow_mliq->header.xtiles;
				if (x == wow_mliq->header.xverts - 1)
					tilen--;
				switch (wow_mliq->tiles[tilen] & WOW_MLIQ_TILE_LIQUID_TYPE)
				{
					case 0x6: /* this lava flow (ironforge center) */
					case 0x7:
					case 0x2: /* ironforge circle magma TODO: should lower brightness */
					{
						float dividor = 510 / 2;
						vertex[n].uv.x = wow_mliq->vertexes[n].magma.s / dividor;
						vertex[n].uv.y = wow_mliq->vertexes[n].magma.t / dividor;
						break;
					}
					case 0x1:
					case 0x4:
					default:
						vertex[n].uv.x = x;
						vertex[n].uv.y = z;
						break;
				}
			}
		}
	}
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
	{
		struct gx_wmo_mliq_liquid *liquid = &mliq->liquids[i];
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
			liquid->uniform_buffers[j] = GFX_BUFFER_INIT();
	}
	mliq->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
		mliq->uniform_buffers[j] = GFX_BUFFER_INIT();
	mliq->vertexes_buffer = GFX_BUFFER_INIT();
	mliq->indices_buffer = GFX_BUFFER_INIT();
	return mliq;

err2:
	init_data_delete(mliq->init_data);
err1:
	mem_free(MEM_GX, mliq);
	return NULL;
}

void gx_wmo_mliq_delete(struct gx_wmo_mliq *mliq)
{
	if (!mliq)
		return;
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
	{
		jks_array_destroy(&mliq->to_render[i]);
		struct gx_wmo_mliq_liquid *liquid = &mliq->liquids[i];
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
			gfx_delete_buffer(g_wow->device, &liquid->uniform_buffers[j]);
	}
	for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
		gfx_delete_buffer(g_wow->device, &mliq->uniform_buffers[j]);
	gfx_delete_buffer(g_wow->device, &mliq->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &mliq->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &mliq->attributes_state);
	init_data_delete(mliq->init_data);
	mem_free(MEM_GX, mliq);
}

void gx_wmo_mliq_initialize(struct gx_wmo_mliq *mliq)
{
	size_t indices_total = 0;
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
	{
		struct gx_wmo_mliq_liquid *liquid = &mliq->liquids[i];
		liquid->indices_offset = indices_total;
		indices_total += liquid->indices_nb;
	}
	gfx_create_buffer(g_wow->device, &mliq->indices_buffer, GFX_BUFFER_INDICES, NULL, indices_total * sizeof(uint16_t), GFX_BUFFER_STATIC);
	size_t indices_pos = 0;
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
	{
		struct gx_wmo_mliq_liquid *liquid = &mliq->liquids[i];
		if (!liquid->indices_nb)
			continue;
		gfx_set_buffer_data(&mliq->indices_buffer, mliq->init_data->indices[i].data, mliq->init_data->indices[i].size * sizeof(uint16_t), indices_pos * sizeof(uint16_t));
		for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
			gfx_create_buffer(g_wow->device, &liquid->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mliq_mesh_block), GFX_BUFFER_STREAM);
		indices_pos += liquid->indices_nb;
	}
	gfx_create_buffer(g_wow->device, &mliq->vertexes_buffer, GFX_BUFFER_VERTEXES, mliq->init_data->vertexes.data, mliq->init_data->vertexes.size * sizeof(struct shader_mliq_input), GFX_BUFFER_IMMUTABLE);
	for (size_t j = 0; j < RENDER_FRAMES_COUNT; ++j)
		gfx_create_buffer(g_wow->device, &mliq->uniform_buffers[j], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_mliq_model_block), GFX_BUFFER_STREAM);
	init_data_delete(mliq->init_data);
	mliq->init_data = NULL;
	gfx_attribute_bind_t binds[] =
	{
		{&mliq->vertexes_buffer, sizeof(struct shader_mliq_input), offsetof(struct shader_mliq_input, position)},
		{&mliq->vertexes_buffer, sizeof(struct shader_mliq_input), offsetof(struct shader_mliq_input, uv)},
	};
	gfx_create_attributes_state(g_wow->device, &mliq->attributes_state, binds, sizeof(binds) / sizeof(*binds), &mliq->indices_buffer, GFX_INDEX_UINT16);
}

static void render_liquid(struct gx_wmo_mliq_liquid *liquid, uint8_t type)
{
	const float alphas[9] = {.5, 1, 1, 1, .5, 0, 1, 1, .5};
	struct shader_mliq_mesh_block mesh_block;
	mesh_block.alpha = alphas[type];
	mesh_block.type = type;
	gfx_set_buffer_data(&liquid->uniform_buffers[g_wow->draw_frame_id], &mesh_block, sizeof(mesh_block), 0);
	gfx_bind_constant(g_wow->device, 0, &liquid->uniform_buffers[g_wow->draw_frame_id], sizeof(mesh_block), 0);
	gfx_draw_indexed(g_wow->device, liquid->indices_nb, liquid->indices_offset);
}

void gx_wmo_mliq_clear_update(struct gx_wmo_mliq *mliq)
{
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
		jks_array_resize(&mliq->to_render[i], 0);
}

void gx_wmo_mliq_add_to_render(struct gx_wmo_mliq *mliq, struct gx_wmo_instance *instance)
{
	return;
	if (mliq->empty)
		return;
	for (size_t i = 0; i < WMO_MLIQ_LIQUIDS_COUNT; ++i)
	{
		if (!mliq->liquids[i].indices_nb)
			continue;
		if (render_add_wmo_mliq(mliq, i))
		{
			if (!jks_array_push_back(&mliq->to_render[i], &instance))
			{
				LOG_ERROR("failed to add instance to mliq render list");
				continue;
			}
		}
	}
}

void gx_wmo_mliq_render(struct gx_wmo_mliq *mliq, uint8_t type)
{
	gfx_bind_attributes_state(g_wow->device, &mliq->attributes_state, &g_wow->graphics->wmo_mliq_input_layout);
	for (size_t i = 0; i < mliq->to_render[type].size; ++i)
	{
		struct gx_wmo_instance *instance = *JKS_ARRAY_GET(&mliq->to_render[type], i, struct gx_wmo_instance*);
		struct mat4f tmp;
		MAT4_TRANSLATE(tmp, instance->m, mliq->position);
		struct shader_mliq_model_block model_block;
		model_block.v = g_wow->draw_frame->view_v;
		MAT4_MUL(model_block.mv, model_block.v, tmp);
		MAT4_MUL(model_block.mvp, g_wow->draw_frame->view_p, model_block.mv);
		gfx_set_buffer_data(&mliq->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
		gfx_bind_constant(g_wow->device, 1, &mliq->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
		render_liquid(&mliq->liquids[i], type);
	}
}
