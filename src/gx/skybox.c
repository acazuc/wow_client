#include "gx/skybox.h"
#include "gx/frame.h"
#include "gx/m2.h"

#include "graphics.h"
#include "shaders.h"
#include "camera.h"
#include "memory.h"
#include "loader.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

#include <gfx/window.h>
#include <gfx/device.h>

#include <libwow/dbc.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

MEMORY_DECL(GX);

#define XZPARTS 128
#define YPARTS 10

#define CLOUDS_WIDTH (CLOUDS_HEIGHT / 2)
#define CLOUDS_HEIGHT 1024

#define CLOUDS_OFFSET_X 456
#define CLOUDS_OFFSET_Y 947

#define CLOUDS_DRIFT_X 0.0005
#define CLOUDS_DRIFT_Y -0.05

#define CLOUDS_SCALE_X (CLOUDS_SCALE_Y * 2)
#define CLOUDS_SCALE_Y 70
#define CLOUDS_SCALE_Z 1

#define CLOUDS_INTERVAL 5000000000

struct skybox_float_track
{
	struct
	{
		uint32_t time;
		float value;
	} data[16];
	uint32_t size;
};

struct skybox_int_track
{
	struct
	{
		uint32_t time;
		struct vec3f value;
	} data[16];
	uint32_t size;
};

struct skybox_weather
{
	struct skybox_float_track float_values[SKYBOX_FLOAT_VALUES];
	struct skybox_int_track int_values[SKYBOX_INT_VALUES];
	float ocean_min_alpha;
	float ocean_max_alpha;
	float river_min_alpha;
	float river_max_alpha;
	float glow;
	uint32_t flags;
	bool has_data;
	char *skybox;
};

struct skybox_entry
{
	uint32_t id;
	struct vec3f pos;
	float inner_radius;
	float outer_radius;
	struct skybox_weather weathers[5];
};

static void gen_clouds_data(struct gx_skybox *skybox, uint64_t t, uint8_t *data);

static uint16_t get_indice(uint32_t x, uint32_t y)
{
	return y * XZPARTS + x;
}

static void load_skybox(struct skybox_entry *entry, struct dbc *dbc_light_int_band, struct dbc *dbc_light_float_band, struct dbc *dbc_light_params, struct dbc *dbc_light_skybox, struct vec3f pos, float inner_radius, float outer_radius, uint32_t ids[5])
{
	for (size_t i = 0; i < 5; ++i)
		entry->weathers[i].skybox = NULL;
	entry->inner_radius = inner_radius / 36;
	entry->outer_radius = outer_radius / 36;
	entry->pos.x = -(pos.z / 36 - 17066);
	entry->pos.y =   pos.y / 36;
	entry->pos.z =   pos.x / 36 - 17066;
	for (uint32_t i = 0; i < dbc_light_int_band->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(dbc_light_int_band, i);
		for (uint32_t j = 0; j < 5; ++j)
		{
			uint32_t id = wow_dbc_get_u32(&row, 0);
			if (id < ids[j] * 18 - 17 || id > ids[j] * 18)
				continue;
			struct skybox_int_track *track = &entry->weathers[j].int_values[id - (ids[j] * 18 - 17)];
			track->size = wow_dbc_get_u32(&row, 4);
			for (uint32_t k = 0; k < track->size && k < 16; ++k)
			{
				uint32_t val = wow_dbc_get_u32(&row, 72 + k * 4);
				track->data[k].value.x = ((val >> 16) & 0xff) / 255.;
				track->data[k].value.y = ((val >> 8) & 0xff) / 255.;
				track->data[k].value.z = ((val >> 0) & 0xff) / 255.;
				track->data[k].time = wow_dbc_get_u32(&row, 8 + k * 4) % 2880;
			}
		}
	}
	for (uint32_t i = 0; i < dbc_light_float_band->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(dbc_light_float_band, i);
		for (uint32_t j = 0; j < 5; ++j)
		{
			uint32_t id = wow_dbc_get_u32(&row, 0);
			if (id < ids[j] * 6 - 5 || id > ids[j] * 6)
				continue;
			struct skybox_float_track *track = &entry->weathers[j].float_values[id - (ids[j] * 6 - 5)];
			track->size = wow_dbc_get_u32(&row, 4);
			for (uint32_t k = 0; k < track->size && k < 16; ++k)
			{
				track->data[k].value = wow_dbc_get_flt(&row, 72 + k * 4);
				track->data[k].time = wow_dbc_get_u32(&row, 8 + k * 4) % 2880;
			}
		}
	}
	for (uint32_t i = 0; i < dbc_light_params->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(dbc_light_params, i);
		for (uint32_t j = 0; j < 5; ++j)
		{
			uint32_t id = wow_dbc_get_u32(&row, 0);
			if (id != ids[j])
				continue;
			entry->weathers[j].has_data = true;
			entry->weathers[j].glow = wow_dbc_get_flt(&row, 16);
			entry->weathers[j].river_min_alpha = wow_dbc_get_flt(&row, 20);
			entry->weathers[j].river_max_alpha = wow_dbc_get_flt(&row, 24);
			entry->weathers[j].ocean_min_alpha = wow_dbc_get_flt(&row, 28);
			entry->weathers[j].ocean_max_alpha = wow_dbc_get_flt(&row, 32);
			uint32_t skybox_id = wow_dbc_get_u32(&row, 8);
			for (uint32_t k = 0; k < dbc_light_skybox->file->header.record_count; ++k)
			{
				struct wow_dbc_row skybox_row = dbc_get_row(dbc_light_skybox, k);
				if (wow_dbc_get_u32(&skybox_row, 0) != skybox_id)
					continue;
				entry->weathers[j].skybox = mem_strdup(MEM_GX, wow_dbc_get_str(&skybox_row, 4));
				if (!entry->weathers[j].skybox)
				{
					LOG_ERROR("skybox file allocation failed");
					continue;
				}
				normalize_m2_filename(entry->weathers[j].skybox, strlen(entry->weathers[j].skybox) + 1);
				entry->weathers[j].flags = wow_dbc_get_u32(&skybox_row, 8);
				break;
			}
		}
	}
}

void entry_destroy(struct skybox_entry *entry)
{
	for (size_t i = 0; i < 5; ++i)
		mem_free(MEM_GX, entry->weathers[i].skybox);
}

struct gx_skybox *gx_skybox_new(uint32_t mapid)
{
	static const float angles[YPARTS] = {90, 55, 40, 25, 15, 4, 3.5, 0, -2.25, -90};
	static const float colorsidx[YPARTS][5] =
	{
		{0, 0, 0, 0, 0},
		{.3, .15, 0, 0, 0},
		{.6, .3, 0, 0, 0},
		{.8, .4, 0, 0, 0},
		{1, .5, 0, 0, 0},
		{1, .9, .9, 0, 0},
		{0, 1, 1, 0, 0},
		{1, 0, 0, 1, 0},
		{0, 0, 0, 0, 1},
		{0, 0, 0, 0, 1}
	};
	struct shader_skybox_input vertexes[XZPARTS * YPARTS];
	uint16_t indices[6 * XZPARTS * (YPARTS - 1)];
	uint32_t idx = 0;
	struct gx_skybox *skybox = NULL;
	struct dbc *dbc_light = NULL;
	struct dbc *dbc_light_params = NULL;
	struct dbc *dbc_light_int_band = NULL;
	struct dbc *dbc_light_float_band = NULL;
	struct dbc *dbc_light_skybox = NULL;
	if (!cache_ref_by_key_dbc(g_wow->cache, "DBFilesClient\\Light.dbc", &dbc_light))
	{
		LOG_ERROR("failed to get Light.dbc");
		goto err;
	}
	if (!cache_ref_by_key_dbc(g_wow->cache, "DBFilesClient\\LightParams.dbc", &dbc_light_params))
	{
		LOG_ERROR("failed to get LightParams.dbc");
		goto err;
	}
	if (!cache_ref_by_key_dbc(g_wow->cache, "DBFilesClient\\LightIntBand.dbc", &dbc_light_int_band))
	{
		LOG_ERROR("failed to get LightIntBand.dbc");
		goto err;
	}
	if (!cache_ref_by_key_dbc(g_wow->cache, "DBFilesClient\\LightFloatBand.dbc", &dbc_light_float_band))
	{
		LOG_ERROR("failed to get LightFloatBand.dbc");
		goto err;
	}
	if (!cache_ref_by_key_dbc(g_wow->cache, "DBFilesClient\\LightSkybox.dbc", &dbc_light_skybox))
	{
		LOG_ERROR("failed to get LightSkybox.dbc");
		goto err;
	}
	skybox = mem_malloc(MEM_GX, sizeof(*skybox));
	if (!skybox)
	{
		LOG_ERROR("failed to alloc skybox skybox");
		goto err;
	}
	skybox->clouds_data = NULL;
	skybox->skybox_m2 = NULL;
	skybox->current_skybox = NULL;
	skybox->has_default_skybox = false;
	skybox->last_clouds = g_wow->frametime;
	skybox->clouds_idx = 0;
	if (pthread_mutex_init(&skybox->clouds_mutex, NULL))
	{
		LOG_ERROR("failed to create clouds mutex");
		goto err;
	}
	simplex_noise_init(&skybox->clouds_noise, 6, 0.5, rand());
	jks_array_init(&skybox->entries, sizeof(struct skybox_entry), (jks_array_destructor_t)entry_destroy, &jks_array_memory_fn_GX);
	for (uint32_t i = 0; i < dbc_light->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(dbc_light, i);
		if (wow_dbc_get_u32(&row, 4) == mapid)
		{
			uint32_t id = wow_dbc_get_u32(&row, 0);
			struct vec3f pos = {wow_dbc_get_flt(&row, 8), wow_dbc_get_flt(&row, 12), wow_dbc_get_flt(&row, 16)};
			float inner_radius = wow_dbc_get_flt(&row, 20);
			float outer_radius = wow_dbc_get_flt(&row, 24);
			uint32_t ids[5];
			ids[0] = wow_dbc_get_u32(&row, 28);
			ids[1] = wow_dbc_get_u32(&row, 32);
			ids[2] = wow_dbc_get_u32(&row, 36);
			ids[3] = wow_dbc_get_u32(&row, 40);
			ids[4] = wow_dbc_get_u32(&row, 44);
			struct skybox_entry *entry = jks_array_grow(&skybox->entries, 1);
			if (!entry)
			{
				LOG_ERROR("failed to alloc space for new skybox");
				continue;
			}
			load_skybox(entry, dbc_light_int_band, dbc_light_float_band, dbc_light_params, dbc_light_skybox, pos, inner_radius, outer_radius, ids);
			entry->id = id;
			if (pos.x == 0 && pos.y == 0 && pos.z == 0 && inner_radius == 0 && outer_radius == 0)
			{
				skybox->has_default_skybox = true;
				skybox->default_skybox = skybox->entries.size - 1;
			}
		}
	}
	if (!skybox->has_default_skybox)
	{
		for (size_t i = 0; i < skybox->entries.size; ++i)
		{
			struct skybox_entry *entry = jks_array_get(&skybox->entries, i);
			if (entry->id == 1)
			{
				skybox->has_default_skybox = true;
				skybox->default_skybox = i;
				break;
			}
		}
		if (!skybox->has_default_skybox && skybox->entries.size)
		{
			skybox->has_default_skybox = true;
			skybox->default_skybox = 0;
		}
	}
	cache_unref_by_ref_dbc(g_wow->cache, dbc_light);
	cache_unref_by_ref_dbc(g_wow->cache, dbc_light_params);
	cache_unref_by_ref_dbc(g_wow->cache, dbc_light_int_band);
	cache_unref_by_ref_dbc(g_wow->cache, dbc_light_float_band);
	cache_unref_by_ref_dbc(g_wow->cache, dbc_light_skybox);
	for (uint32_t y = 0; y < YPARTS; ++y)
	{
		for (uint32_t x = 0; x < XZPARTS; ++x)
		{
			{
				uint16_t index = get_indice(x, y);
				struct shader_skybox_input *vertex = &vertexes[index];
				float fac = M_PI * 2 / (XZPARTS - 1);
				float y_fac = 1 - fabsf(sinf(angles[y] / 180. * M_PI));
				for (size_t i = 0; i < 5; ++i)
					vertex->colors[i] = colorsidx[y][i];
				vertex->position.x = cosf(x * fac) * y_fac * 100;
				vertex->position.y = sinf(angles[y] / 180. * M_PI) * 100;
				vertex->position.z = sinf(x * fac) * y_fac * 100;
				vertex->uv.x = x / (float)(XZPARTS - 1);
				if (y < YPARTS - 1)
					vertex->uv.y = sinf((angles[y] + 2.25) / 184.5 * M_PI);
				else
					vertex->uv.y = 0;
			}
			if (y == YPARTS - 1)
				continue;
			uint16_t p1 = get_indice( x               , y);
			uint16_t p2 = get_indice((x + 1) % XZPARTS, y);
			uint16_t p3 = get_indice((x + 1) % XZPARTS, y + 1);
			uint16_t p4 = get_indice( x               , y + 1);
			indices[idx++] = p2;
			indices[idx++] = p1;
			indices[idx++] = p4;
			indices[idx++] = p4;
			indices[idx++] = p3;
			indices[idx++] = p2;
		}
	}
	skybox->indices_nb = sizeof(indices) / sizeof(*indices);
	skybox->attributes_state = GFX_ATTRIBUTES_STATE_INIT();
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		skybox->m2_uniform_buffers[i] = GFX_BUFFER_INIT();
		skybox->uniform_buffers[i] = GFX_BUFFER_INIT();
	}
	skybox->vertexes_buffer = GFX_BUFFER_INIT();
	skybox->indices_buffer = GFX_BUFFER_INIT();
	for (size_t i = 0; i < sizeof(skybox->clouds) / sizeof(*skybox->clouds); ++i)
	{
		skybox->clouds[i] = GFX_TEXTURE_INIT();
		gfx_create_texture(g_wow->device, &skybox->clouds[i], GFX_TEXTURE_2D, GFX_R8, 1, CLOUDS_WIDTH, CLOUDS_HEIGHT, 0);
		gfx_set_texture_levels(&skybox->clouds[i], 0, 0);
		gfx_set_texture_anisotropy(&skybox->clouds[i], g_wow->anisotropy);
		gfx_set_texture_filtering(&skybox->clouds[i], GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR, GFX_FILTERING_LINEAR);
		gfx_set_texture_addressing(&skybox->clouds[i], GFX_TEXTURE_ADDRESSING_REPEAT, GFX_TEXTURE_ADDRESSING_REPEAT, GFX_TEXTURE_ADDRESSING_REPEAT);
		uint8_t data[CLOUDS_WIDTH * CLOUDS_HEIGHT];
		gen_clouds_data(skybox, g_wow->frametime + i * CLOUDS_INTERVAL, data);
		gfx_set_texture_data(&skybox->clouds[i], 0, 0, CLOUDS_WIDTH, CLOUDS_HEIGHT, 0, CLOUDS_WIDTH * CLOUDS_HEIGHT, data);
	}
	gfx_create_buffer(g_wow->device, &skybox->vertexes_buffer, GFX_BUFFER_VERTEXES, vertexes, sizeof(vertexes), GFX_BUFFER_IMMUTABLE);
	gfx_create_buffer(g_wow->device, &skybox->indices_buffer, GFX_BUFFER_INDICES, indices, sizeof(indices), GFX_BUFFER_IMMUTABLE);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_create_buffer(g_wow->device, &skybox->uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_skybox_model_block), GFX_BUFFER_STREAM);
		gfx_create_buffer(g_wow->device, &skybox->m2_uniform_buffers[i], GFX_BUFFER_UNIFORM, NULL, sizeof(struct shader_m2_scene_block), GFX_BUFFER_STREAM);
	}
	const struct gfx_attribute_bind binds[] =
	{
		{&skybox->vertexes_buffer},
		{&skybox->vertexes_buffer},
		{&skybox->vertexes_buffer},
		{&skybox->vertexes_buffer},
		{&skybox->vertexes_buffer},
		{&skybox->vertexes_buffer},
		{&skybox->vertexes_buffer},
	};
	gfx_create_attributes_state(g_wow->device, &skybox->attributes_state, binds, sizeof(binds) / sizeof(*binds), &skybox->indices_buffer, GFX_INDEX_UINT16);
	return skybox;

err:
	if (dbc_light)
		cache_unref_by_ref_dbc(g_wow->cache, dbc_light);
	if (dbc_light_params)
		cache_unref_by_ref_dbc(g_wow->cache, dbc_light_params);
	if (dbc_light_int_band)
		cache_unref_by_ref_dbc(g_wow->cache, dbc_light_int_band);
	if (dbc_light_float_band)
		cache_unref_by_ref_dbc(g_wow->cache, dbc_light_float_band);
	if (dbc_light_skybox)
		cache_unref_by_ref_dbc(g_wow->cache, dbc_light_skybox);
	mem_free(MEM_GX, skybox);
	return NULL;
}

void gx_skybox_delete(struct gx_skybox *skybox)
{
	if (!skybox)
		return;
	mem_free(MEM_GX, skybox->clouds_data);
	gx_m2_instance_gc(skybox->skybox_m2);
	jks_array_destroy(&skybox->entries);
	for (size_t i = 0; i < RENDER_FRAMES_COUNT; ++i)
	{
		gfx_delete_buffer(g_wow->device, &skybox->m2_uniform_buffers[i]);
		gfx_delete_buffer(g_wow->device, &skybox->uniform_buffers[i]);
	}
	gfx_delete_buffer(g_wow->device, &skybox->vertexes_buffer);
	gfx_delete_buffer(g_wow->device, &skybox->indices_buffer);
	gfx_delete_attributes_state(g_wow->device, &skybox->attributes_state);
	for (size_t i = 0; i < sizeof(skybox->clouds) / sizeof(*skybox->clouds); ++i)
		gfx_delete_texture(g_wow->device, &skybox->clouds[i]);
	simplex_noise_destroy(&skybox->clouds_noise);
	pthread_mutex_destroy(&skybox->clouds_mutex);
	mem_free(MEM_GX, skybox);
}

void gx_skybox_update(struct gx_skybox *skybox)
{
	struct vec3f int_total[SKYBOX_INT_VALUES];
	float int_count[SKYBOX_INT_VALUES];
	float float_total[SKYBOX_FLOAT_VALUES];
	float float_count[SKYBOX_FLOAT_VALUES];
	float ocean_min_alpha;
	float ocean_max_alpha;
	float river_min_alpha;
	float river_max_alpha;
	float glow;
	float liquid_alpha_count;
	char *skybox_file = NULL;
	float max_skybox_file_percentage;
	int weather = 0;
	for (uint32_t i = 0; i < SKYBOX_INT_VALUES; ++i)
	{
		if (skybox->has_default_skybox)
		{
			struct skybox_entry *entry = jks_array_get(&skybox->entries, skybox->default_skybox);
			if (entry->weathers[weather].int_values[i].size)
				skybox->int_values[i] = entry->weathers[weather].int_values[i].data[0].value;
			else
				VEC3_SET(skybox->int_values[i], 0, 0, 1);
		}
		else
		{
			VEC3_SET(skybox->int_values[i], 0, 0, 1);
		}
		VEC3_SETV(int_total[i], 0);
		int_count[i] = 0;
	}
	for (uint32_t i = 0; i < SKYBOX_FLOAT_VALUES; ++i)
	{
		if (skybox->has_default_skybox)
		{
			struct skybox_entry *entry = jks_array_get(&skybox->entries, skybox->default_skybox);
			if (entry->weathers[weather].float_values[i].size)
				skybox->float_values[i] = entry->weathers[weather].float_values[i].data[0].value;
			else
				skybox->float_values[i] = .01;
		}
		else
		{
			skybox->float_values[i] = .01;
		}
		float_total[i] = 0;
		float_count[i] = 0;
	}
	if (skybox->has_default_skybox)
	{
		struct skybox_entry *entry = jks_array_get(&skybox->entries, skybox->default_skybox);
		skybox->ocean_min_alpha = entry->weathers[weather].ocean_min_alpha;
		skybox->ocean_max_alpha = entry->weathers[weather].ocean_max_alpha;
		skybox->river_min_alpha = entry->weathers[weather].river_min_alpha;
		skybox->river_max_alpha = entry->weathers[weather].river_max_alpha;
		skybox->glow = entry->weathers[weather].glow;
	}
	else
	{
		skybox->ocean_min_alpha = 0;
		skybox->ocean_max_alpha = 1;
		skybox->river_min_alpha = 0;
		skybox->river_max_alpha = 1;
		skybox->glow = 0;
	}
	ocean_min_alpha = 0;
	ocean_max_alpha = 1;
	river_min_alpha = 0;
	river_max_alpha = 1;
	glow = 0;
	liquid_alpha_count = 0;
	max_skybox_file_percentage = 0;
	int64_t t = g_wow->frametime / 1000000;
	t %= 2880;
	t = 1800;
	for (size_t i = 0; i < skybox->entries.size; ++i)
	{
		struct skybox_entry *entry = jks_array_get(&skybox->entries, i);
		struct vec3f delta;
		VEC3_SUB(delta, g_wow->cull_frame->cull_pos, entry->pos);
		float len = VEC3_NORM(delta);
		if (len > entry->outer_radius)
			continue;
		float skybox_percentage = (entry->outer_radius - len) / (entry->outer_radius - entry->inner_radius);
		if (skybox_percentage < 0)
			skybox_percentage = 0;
		if (skybox_percentage > max_skybox_file_percentage)
			skybox_file = entry->weathers[weather].skybox;
		for (size_t c = 0; c < SKYBOX_INT_VALUES; ++c)
		{
			struct skybox_int_track *values = &entry->weathers[weather].int_values[c];
			if (!values->size)
				continue;
			uint32_t timelowest = 2881;
			uint32_t timelowestidx = 0;
			uint32_t timeless = 0;
			uint32_t timelessidx = 2881;
			uint32_t timecurplus = 2881;
			uint32_t timecurplusidx = 0;
			for (size_t j = 0; j < values->size; ++j)
			{
				uint32_t tmp = values->data[j].time;
				if (tmp > timeless && tmp < t)
				{
					timeless = tmp;
					timelessidx = j;
				}
				if (tmp < timecurplus && tmp >= t)
				{
					timecurplus = tmp;
					timecurplusidx = j;
				}
				if (tmp < timelowest)
				{
					timelowest = tmp;
					timelowestidx = j;
				}
			}
			if (timecurplus == 2881)
			{
				timecurplus = timelowest;
				timecurplusidx = timelowestidx;
			}
			if (timelessidx == 2881)
			{
				timeless = timelowest;
				timelessidx = timelowestidx;
			}
			struct vec3f value;
			if (timeless == timecurplus)
			{
				value = values->data[timelessidx].value;
			}
			else
			{
				uint32_t curdiff = t > timeless ? t - timeless : 2880 - timeless + t;
				uint32_t lessplusdiff = timecurplus > timeless ? timecurplus - timeless : 2880 - timeless + timecurplus;
				float perc = curdiff / (float)lessplusdiff;
				struct vec3f tmp;
				VEC3_MULV(tmp, values->data[timelessidx].value, 1 - perc);
				VEC3_MULV(value, values->data[timecurplusidx].value, perc);
				VEC3_ADD(value, value, tmp);
			}
			VEC3_MULV(value, value, skybox_percentage);
			VEC3_ADD(int_total[c], int_total[c], value);
			int_count[c] += skybox_percentage;
		}
		for (size_t c = 0; c < SKYBOX_FLOAT_VALUES; ++c)
		{
			struct skybox_float_track *values = &entry->weathers[weather].float_values[c];
			if (!values->size)
				continue;
			uint32_t timelowest = 2881;
			uint32_t timelowestidx = 0;
			uint32_t timeless = 0;
			uint32_t timelessidx = 2881;
			uint32_t timecurplus = 2881;
			uint32_t timecurplusidx = 0;
			for (size_t j = 0; j < values->size; ++j)
			{
				uint32_t tmp = values->data[j].time;
				if (tmp > timeless && tmp < t)
				{
					timeless = tmp;
					timelessidx = j;
				}
				if (tmp < timecurplus && tmp >= t)
				{
					timecurplus = tmp;
					timecurplusidx = j;
				}
				if (tmp < timelowest)
				{
					timelowest = tmp;
					timelowestidx = j;
				}
			}
			if (timecurplus == 2881)
			{
				timecurplus = timelowest;
				timecurplusidx = timelowestidx;
			}
			if (timelessidx == 2881)
			{
				timeless = timelowest;
				timelessidx = timelowestidx;
			}
			float value;
			if (timeless == timecurplus)
			{
				value = values->data[timelessidx].value;
			}
			else
			{
				uint32_t curdiff = t > timeless ? t - timeless : 2880 - timeless + t;
				uint32_t lessplusdiff = timecurplus > timeless ? timecurplus - timeless : 2880 - timeless + timecurplus;
				float perc = curdiff / (float)lessplusdiff;
				value = (1 - perc) * values->data[timelessidx].value + perc * values->data[timecurplusidx].value;
			}
			float_total[c] += value * skybox_percentage;
			float_count[c] += skybox_percentage;
		}
		if (entry->weathers[weather].has_data)
		{
			ocean_min_alpha += entry->weathers[weather].ocean_min_alpha * skybox_percentage;
			ocean_max_alpha += entry->weathers[weather].ocean_max_alpha * skybox_percentage;
			river_min_alpha += entry->weathers[weather].river_min_alpha * skybox_percentage;
			river_max_alpha += entry->weathers[weather].river_max_alpha * skybox_percentage;
			glow += entry->weathers[weather].glow * skybox_percentage;
			liquid_alpha_count += skybox_percentage;
		}
	}
	for (size_t i = 0; i < SKYBOX_INT_VALUES; ++i)
	{
		if (int_count[i] >= 1)
		{
			VEC3_DIVV(skybox->int_values[i], int_total[i], int_count[i]);
		}
		else
		{
			struct vec3f tmp;
			VEC3_MULV(tmp, skybox->int_values[i], 1 - int_count[i]);
			VEC3_MULV(skybox->int_values[i], int_total[i], int_count[i]);
			VEC3_ADD(skybox->int_values[i], skybox->int_values[i], tmp);
		}
	}
	for (size_t i = 0; i < SKYBOX_FLOAT_VALUES; ++i)
	{
		if (float_count[i] >= 1)
			skybox->float_values[i] = float_total[i] / float_count[i];
		else
			skybox->float_values[i] = float_count[i] * float_total[i] + (1 - float_count[i]) * skybox->float_values[i];
	}
	if (liquid_alpha_count >= 1)
	{
		skybox->ocean_min_alpha = ocean_min_alpha / liquid_alpha_count;
		skybox->ocean_max_alpha = ocean_max_alpha / liquid_alpha_count;
		skybox->river_min_alpha = river_min_alpha / liquid_alpha_count;
		skybox->river_max_alpha = river_max_alpha / liquid_alpha_count;
		skybox->glow = glow / liquid_alpha_count;
	}
	else
	{
		skybox->ocean_min_alpha = ocean_min_alpha * liquid_alpha_count + (1 - liquid_alpha_count) * skybox->ocean_min_alpha;
		skybox->ocean_max_alpha = ocean_max_alpha * liquid_alpha_count + (1 - liquid_alpha_count) * skybox->ocean_max_alpha;
		skybox->river_min_alpha = river_min_alpha * liquid_alpha_count + (1 - liquid_alpha_count) * skybox->river_min_alpha;
		skybox->river_max_alpha = river_max_alpha * liquid_alpha_count + (1 - liquid_alpha_count) * skybox->river_max_alpha;
		skybox->glow = glow * liquid_alpha_count + (1 - liquid_alpha_count) * skybox->glow;
	}
	/* hack: avoid fog behind distance view, maybe there is a better solution */
	if (skybox->float_values[0] > 1000 * 36)
		skybox->float_values[0] = 1000 * 36;
	if (!(g_wow->render_opt & RENDER_OPT_FOG))
	{
		skybox->float_values[0] = 1000000000;
		skybox->float_values[1] = 1;
	}
	if (!skybox_file)
	{
		skybox->current_skybox = skybox_file;
		gx_m2_instance_gc(skybox->skybox_m2);
		skybox->skybox_m2 = NULL;
	}
	else if (skybox->current_skybox != skybox_file) /* legit ptr cmp */
	{
		skybox->current_skybox = skybox_file;
		gx_m2_instance_gc(skybox->skybox_m2);
		if (skybox_file)
		{
			skybox->skybox_m2 = gx_m2_instance_new_filename(skybox_file);
			if (skybox->skybox_m2)
			{
				skybox->skybox_m2->parent->skybox = true;
				gx_m2_ask_load(skybox->skybox_m2->parent);
			}
			else
			{
				LOG_ERROR("failed to allocate skybox m2");
			}
		}
		else
		{
			skybox->skybox_m2 = NULL;
		}
	}
}

static void gen_clouds_data(struct gx_skybox *skybox, uint64_t t, uint8_t *data)
{
	float vz = (t / 1000000) / 2880. * CLOUDS_SCALE_Z;
	for (size_t y = 0; y < CLOUDS_HEIGHT; ++y)
	{
		float yfac;
		if (y < CLOUDS_HEIGHT / 10)
		{
			yfac = (float)y / (CLOUDS_HEIGHT / 10);
		}
		else if (y >= CLOUDS_HEIGHT - (CLOUDS_HEIGHT / 2))
		{
			yfac = 1 - ((y - (CLOUDS_HEIGHT - (CLOUDS_HEIGHT / 2))) / (float)((CLOUDS_HEIGHT / 4) - 1));
			if (yfac <= 0)
				memset(&data[y * CLOUDS_WIDTH], 0, CLOUDS_WIDTH);
		}
		else
		{
			yfac = 1;
		}
		for (size_t x = 0; x < CLOUDS_WIDTH; ++x)
		{
			float vx = (CLOUDS_OFFSET_X + (x / (float)CLOUDS_WIDTH)) * CLOUDS_SCALE_X;
			float vy = (CLOUDS_OFFSET_Y + (y / (float)CLOUDS_HEIGHT)) * CLOUDS_SCALE_Y + (t / 1000000000.f) * CLOUDS_DRIFT_Y;
			float v = simplex_noise_get3(&skybox->clouds_noise, vx, vy, vz);
			if (x < CLOUDS_WIDTH / 10)
			{
				float vvx = (CLOUDS_OFFSET_X + ((x + CLOUDS_WIDTH) / (float)CLOUDS_WIDTH)) * CLOUDS_SCALE_X;
				float vv = simplex_noise_get3(&skybox->clouds_noise, vvx, vy, vz);
				float f = x / (float)(CLOUDS_WIDTH / 10);
				v = v * f + vv * (1 - f);
			}
			v = (v + 1) * .5 * yfac;
			if (v > 1)
				v = 1;
			if (v < 0)
				v = 0;
			data[x + y * CLOUDS_WIDTH] = v * 255;
		}
	}
}

void generate_clouds_texture(struct gx_skybox *skybox)
{
	uint8_t *data = mem_malloc(MEM_GX, CLOUDS_WIDTH * CLOUDS_HEIGHT);
	if (!data)
	{
		LOG_ERROR("failed to allocate clouds data");
		return;
	}
	gen_clouds_data(skybox, skybox->clouds_time, data);
	pthread_mutex_lock(&skybox->clouds_mutex);
	mem_free(MEM_GX, skybox->clouds_data);
	skybox->clouds_data = data;
	pthread_mutex_unlock(&skybox->clouds_mutex);
}

void gx_skybox_render(struct gx_skybox *skybox)
{
	struct shader_skybox_model_block model_block;
	struct mat4f mv;
	MAT4_TRANSLATE(mv, g_wow->draw_frame->view_v, g_wow->draw_frame->cull_pos);
	struct mat4f p;
	MAT4_PERSPECTIVE(p, g_wow->draw_frame->fov, (float)g_wow->window->width / (float)g_wow->window->height, 1.f, 10000.0f);
	MAT4_MUL(model_block.mvp, p, mv);
	for (size_t i = 0; i < 6; ++i)
	{
		VEC3_CPY(model_block.sky_colors[i], skybox->int_values[SKYBOX_INT_SKY0 + i]);
		model_block.sky_colors[i].w = 1;
	}
	VEC3_CPY(model_block.clouds_colors[0], skybox->int_values[SKYBOX_INT_SUN]);
	model_block.clouds_colors[0].w = 1;
	VEC3_CPY(model_block.clouds_colors[1], skybox->int_values[SKYBOX_INT_CLOUD2]);
	model_block.clouds_colors[1].w = 1;
	model_block.clouds_factors.x = 1 - skybox->float_values[SKYBOX_FLOAT_CLOUD];
	model_block.clouds_factors.y = 1;
	if (g_wow->frametime - skybox->last_clouds > CLOUDS_INTERVAL)
	{
		skybox->clouds_time = g_wow->frametime + CLOUDS_INTERVAL * 2;
		loader_push(g_wow->loader, ASYNC_TASK_CLOUDS_TEXTURE, NULL);
		skybox->last_clouds = g_wow->frametime;
		skybox->clouds_idx = (skybox->clouds_idx + 1) % 3;
	}
	model_block.clouds_blend = ((g_wow->frametime - skybox->last_clouds) % CLOUDS_INTERVAL) / (float)CLOUDS_INTERVAL;
	model_block.clouds_drift = g_wow->frametime / 1000000000.f * CLOUDS_DRIFT_X;
	pthread_mutex_lock(&skybox->clouds_mutex);
	if (skybox->clouds_data)
	{
		gfx_set_texture_data(&skybox->clouds[(skybox->clouds_idx + 2) % 3], 0, 0, CLOUDS_WIDTH, CLOUDS_HEIGHT, 0, CLOUDS_WIDTH * CLOUDS_HEIGHT, skybox->clouds_data);
		mem_free(MEM_GX, skybox->clouds_data);
		skybox->clouds_data = NULL;
	}
	pthread_mutex_unlock(&skybox->clouds_mutex);
	const gfx_texture_t *clouds[] = {&skybox->clouds[skybox->clouds_idx], &skybox->clouds[(skybox->clouds_idx + 1) % 3]};
	gfx_bind_samplers(g_wow->device, 0, 2, clouds);
	gfx_set_buffer_data(&skybox->uniform_buffers[g_wow->draw_frame_id], &model_block, sizeof(model_block), 0);
	gfx_bind_constant(g_wow->device, 1, &skybox->uniform_buffers[g_wow->draw_frame_id], sizeof(model_block), 0);
	gfx_bind_attributes_state(g_wow->device, &skybox->attributes_state, &g_wow->graphics->skybox_input_layout);
	gfx_draw_indexed(g_wow->device, skybox->indices_nb, 0);
	if (skybox->skybox_m2 && skybox->skybox_m2->parent && skybox->skybox_m2->parent->loaded && (g_wow->render_opt & RENDER_OPT_M2))
	{
		struct shader_m2_scene_block scene_block;
		VEC4_SET(scene_block.light_direction, -1, -1, 1, 0);
		VEC3_CPY(scene_block.diffuse_color, skybox->int_values[SKYBOX_INT_DIFFUSE]);
		scene_block.diffuse_color.w = 1;
		VEC3_CPY(scene_block.ambient_color, skybox->int_values[SKYBOX_INT_AMBIENT]);
		scene_block.ambient_color.w = 1;
		VEC2_SETV(scene_block.fog_range, INFINITY);
		gfx_set_buffer_data(&skybox->m2_uniform_buffers[g_wow->draw_frame_id], &scene_block, sizeof(scene_block), 0);
		gfx_bind_constant(g_wow->device, 2, &skybox->m2_uniform_buffers[g_wow->draw_frame_id], sizeof(scene_block), 0);
		VEC3_CPY(skybox->skybox_m2->pos, g_wow->draw_frame->cull_pos);
		struct mat4f tmp1;
		struct mat4f tmp2;
		MAT4_IDENTITY(tmp1);
		MAT4_TRANSLATE(tmp2, tmp1, g_wow->draw_frame->cull_pos);
		skybox->skybox_m2->bones_calculated = false;
		gx_m2_instance_set_mat(skybox->skybox_m2, &tmp2);
		gx_m2_instance_force_update(skybox->skybox_m2, &g_wow->draw_frame->m2_params);
		gx_m2_instance_render(skybox->skybox_m2, false, &g_wow->draw_frame->m2_params);
		gx_m2_instance_render(skybox->skybox_m2, true, &g_wow->draw_frame->m2_params);
	}
}
