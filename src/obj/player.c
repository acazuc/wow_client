#include "player.h"

#include "obj/update_fields.h"

#include "itf/interface.h"

#include "gx/text.h"
#include "gx/m2.h"

#include "wow_lua.h"
#include "memory.h"
#include "const.h"
#include "cache.h"
#include "log.h"
#include "wow.h"
#include "blp.h"
#include "dbc.h"
#include "db.h"

#include <libwow/dbc.h>
#include <libwow/blp.h>
#include <libwow/mpq.h>

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#define OBJECT ((struct object*)player)
#define UNIT ((struct unit*)object)
#define WORLD_OBJECT ((struct worldobj*)object)
#define PLAYER ((struct player*)object)

struct texture_section
{
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
};

enum tex_idx
{
	TEX_ARM_UPPER,
	TEX_ARM_LOWER,
	TEX_HAND,
	TEX_TORSO_UPPER,
	TEX_TORSO_LOWER,
	TEX_LEG_UPPER,
	TEX_LEG_LOWER,
	TEX_FOOT,
	TEX_SCALP_UPPER,
	TEX_SCALP_LOWER,
	TEX_LAST,
};

static const struct texture_section g_texture_sections[TEX_LAST] =
{
	[TEX_ARM_UPPER]   = {  0,   0, 128,  64},
	[TEX_ARM_LOWER]   = {  0,  64, 128,  64},
	[TEX_HAND]        = {  0, 128, 128,  32},
	[TEX_TORSO_UPPER] = {128,   0, 128,  64},
	[TEX_TORSO_LOWER] = {128,  64, 128,  32},
	[TEX_LEG_UPPER]   = {128,  96, 128,  64},
	[TEX_LEG_LOWER]   = {128, 160, 128,  64},
	[TEX_FOOT]        = {128, 224, 128,  32},
	[TEX_SCALP_UPPER] = {  0, 160, 128,  32},
	[TEX_SCALP_LOWER] = {  0, 192, 128,  64},
};

static const uint32_t g_texture_indexes[] =
{
	EQUIPMENT_SLOT_SHIRT,
	EQUIPMENT_SLOT_LEGS,
	EQUIPMENT_SLOT_CHEST,
	EQUIPMENT_SLOT_WAIST,
	EQUIPMENT_SLOT_FEET,
	EQUIPMENT_SLOT_WRISTS,
	EQUIPMENT_SLOT_HANDS,
	EQUIPMENT_SLOT_TABARD,
};

static const uint32_t g_texture_masks[] =
{
	(1 << TEX_TORSO_UPPER) | (1 << TEX_TORSO_LOWER) | (1 << TEX_ARM_UPPER) | (1 << TEX_ARM_LOWER), /* shirt */
	(1 << TEX_LEG_UPPER) | (1 << TEX_LEG_LOWER), /* legs */
	(1 << TEX_TORSO_UPPER) | (1 << TEX_TORSO_LOWER) | (1 << TEX_ARM_UPPER) | (1 << TEX_ARM_LOWER), /* chest */
	(1 << TEX_TORSO_LOWER), /* waist */
	(1 << TEX_FOOT), /* feet */
	(1 << TEX_ARM_LOWER), /* wrists */
	(1 << TEX_HAND) | (1 << TEX_ARM_LOWER), /* hands */
	(1 << TEX_TORSO_UPPER) | (1 << TEX_TORSO_LOWER), /* tabard */
};

static const uint32_t g_items_display_offsets[UNIT_ITEM_LAST] =
{
	EQUIPMENT_SLOT_HEAD,
	EQUIPMENT_SLOT_SHOULDERS,
	EQUIPMENT_SLOT_SHIRT,
	EQUIPMENT_SLOT_CHEST,
	EQUIPMENT_SLOT_WAIST,
	EQUIPMENT_SLOT_LEGS,
	EQUIPMENT_SLOT_FEET,
	EQUIPMENT_SLOT_WRISTS,
	EQUIPMENT_SLOT_HANDS,
	EQUIPMENT_SLOT_TABARD,
	EQUIPMENT_SLOT_BACK,
	EQUIPMENT_SLOT_MAINHAND,
	EQUIPMENT_SLOT_OFFHAND,
};

static const char *g_texture_components_dirs[8] =
{
	"armuppertexture",
	"armlowertexture",
	"handtexture",
	"torsouppertexture",
	"torsolowertexture",
	"leguppertexture",
	"leglowertexture",
	"foottexture",
};

static void (*g_field_functions[PLAYER_FIELD_MAX - PLAYER_FIELD_MIN])(struct object *object) =
{
};

static bool ctr(struct object *object, uint64_t guid)
{
	if (!unit_object_vtable.ctr(object, guid))
		return false;
	WORLD_OBJECT->worldobj_vtable = &player_worldobj_vtable;
	UNIT->unit_vtable = &player_unit_vtable;
	if (!object_fields_resize(&object->fields, PLAYER_FIELD_MAX))
		return false;
	PLAYER->skin_extra_texture = NULL;
	PLAYER->skin_texture = NULL;
	PLAYER->hair_texture = NULL;
	PLAYER->clean_batches = true;
	PLAYER->db_get = false;
	object_fields_set_u32(&object->fields, OBJECT_FIELD_TYPE, OBJECT_MASK_OBJECT | OBJECT_MASK_UNIT | OBJECT_MASK_PLAYER);
#if 0 /* cot hourglass */
	worldobj_set_position(WORLD_OBJECT, (struct vec3f){-8390.084961, -127.404030, 4337.700684});
#endif
#if 0 /* flight over shattrath */
	VEC3_SET(WORLD_OBJECT->position, -1865, 142, -5427);
#endif
#if 0 /* shattrath entry */
	VEC3_SET(WORLD_OBJECT->position, -1984.962280, 100.217883, -5087.445312);
#endif
#if 0
	VEC3_SET(WORLD_OBJECT->position, -10775.298828, 40.753269, -1721.312866);
#endif
#if 0 /* netherstorm collectors */
	VEC3_SET(WORLD_OBJECT->position, 2577.359863, 297.467316, -3346.378662);
#endif
#if 1 /* barrens */
	VEC3_SET(WORLD_OBJECT->position, -818.711609, 198.758865, 3291.442871);
#endif
	return true;
}

static void dtr(struct object *object)
{
	blp_texture_delete(PLAYER->skin_texture);
	if (PLAYER->skin_extra_texture)
		cache_unref_by_ref_blp(g_wow->cache, PLAYER->skin_extra_texture);
	if (PLAYER->hair_texture)
		cache_unref_by_ref_blp(g_wow->cache, PLAYER->hair_texture);
	unit_object_vtable.dtr(object);
}

static void on_field_changed(struct object *object, uint32_t field)
{
	if (field < PLAYER_FIELD_MIN)
		return unit_object_vtable.on_field_changed(object, field);
	if (field >= PLAYER_FIELD_MAX)
		return;
	if (g_field_functions[field - PLAYER_FIELD_MIN])
		g_field_functions[field - PLAYER_FIELD_MIN](object);
}

static void patch_texture(uint8_t *image_data, uint32_t image_width, struct wow_blp_file *blp_file, const struct texture_section *dst)
{
	if (blp_file->header.width != dst->width)
	{
		LOG_ERROR("invalid width: %" PRIu32 " instead of %" PRIu32, blp_file->header.width, dst->width);
		return;
	}
	if (blp_file->header.height != dst->height)
	{
		LOG_ERROR("invalid height: %" PRIu32 " instead of %" PRIu32, blp_file->header.height, dst->height);
		return;
	}
	uint8_t *comp_data;
	uint32_t comp_width;
	uint32_t comp_height;
	if (!blp_decode_rgba(blp_file, 0, &comp_width, &comp_height, &comp_data))
	{
		LOG_ERROR("can't decode component");
		return;
	}
	for (size_t y = 0; y < dst->height; ++y)
	{
		for (size_t x = 0; x < dst->width; ++x)
		{
			uint8_t *d = &image_data[((dst->y + y) * image_width + dst->x + x) * 4];
			uint8_t *s = &comp_data[(comp_width * y + x) * 4];
			uint8_t b = s[3];
			uint8_t a = 0xFF - b;
			d[0] = (d[0] * a + s[0] * b) / 0xFF;
			d[1] = (d[1] * a + s[1] * b) / 0xFF;
			d[2] = (d[2] * a + s[2] * b) / 0xFF;
		}
	}
	mem_free(MEM_GENERIC, comp_data);
}

static void generate_skin_texture(struct object *object)
{
	struct wow_blp_file *skin_texture = NULL;
	unit_get_skin_textures_files(UNIT, &skin_texture, NULL);
	if (!skin_texture)
	{
		LOG_ERROR("can't get base skin texture");
		return;
	}
	uint8_t *image_data;
	uint32_t image_width;
	uint32_t image_height;
	if (!blp_decode_rgba(skin_texture, 0, &image_width, &image_height, &image_data))
	{
		wow_blp_file_delete(skin_texture);
		LOG_ERROR("can't decode skin texture");
		return;
	}
	wow_blp_file_delete(skin_texture);
	if (image_width != 256 || image_height != 256)
	{
		LOG_ERROR("invalid image width / image height: %" PRIu32 "x%" PRIu32, image_width, image_height);
		return;
	}
	{
		struct wow_blp_file *face_lower;
		struct wow_blp_file *face_upper;
		struct wow_blp_file *scalp_lower;
		struct wow_blp_file *scalp_upper;
		struct wow_blp_file *facial_lower;
		struct wow_blp_file *facial_upper;
		struct wow_blp_file *pelvis;
		struct wow_blp_file *torso;
		unit_get_face_textures_files(UNIT, &face_lower, &face_upper);
		unit_get_hair_textures_files(UNIT, NULL, &scalp_lower, &scalp_upper);
		unit_get_facial_hair_textures_files(UNIT, &facial_lower, &facial_upper);
		unit_get_underwear_textures_files(UNIT, &pelvis, &torso);
		if (face_lower)
			patch_texture(image_data, image_width, face_lower, &g_texture_sections[TEX_SCALP_LOWER]);
		else
			LOG_WARN("no face lower texture");
		if (face_upper)
			patch_texture(image_data, image_width, face_upper, &g_texture_sections[TEX_SCALP_UPPER]);
		else
			LOG_WARN("no face upper texture");
		if (scalp_lower)
			patch_texture(image_data, image_width, scalp_lower, &g_texture_sections[TEX_SCALP_LOWER]);
		else
			LOG_WARN("no scalp lower texture");
		if (scalp_upper)
			patch_texture(image_data, image_width, scalp_upper, &g_texture_sections[TEX_SCALP_UPPER]);
		else
			LOG_WARN("no scalp upper texture");
		if (facial_lower)
			patch_texture(image_data, image_width, facial_lower, &g_texture_sections[TEX_SCALP_LOWER]);
		else
			LOG_WARN("no facial lower texture");
		if (facial_upper)
			patch_texture(image_data, image_width, facial_upper, &g_texture_sections[TEX_SCALP_UPPER]);
		else
			LOG_WARN("no facial upper texture");
		if (pelvis)
			patch_texture(image_data, image_width, pelvis, &g_texture_sections[TEX_LEG_UPPER]);
		else
			LOG_WARN("no pelvis texture");
		if (torso)
			patch_texture(image_data, image_width, torso, &g_texture_sections[TEX_TORSO_UPPER]);
		else
			LOG_WARN("no torso texture");
		wow_blp_file_delete(face_lower);
		wow_blp_file_delete(face_upper);
		wow_blp_file_delete(scalp_lower);
		wow_blp_file_delete(scalp_upper);
		wow_blp_file_delete(facial_lower);
		wow_blp_file_delete(facial_upper);
		wow_blp_file_delete(pelvis);
		wow_blp_file_delete(torso);
	}
	for (size_t i = 0; i < sizeof(g_texture_indexes) / sizeof(*g_texture_indexes); ++i)
	{
		uint32_t item = object_fields_get_u64(&object->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + 16 * g_texture_indexes[i]);
		if (!item)
			continue;
		LOG_INFO("items[%d] = %" PRIu32, (int)i, item);
		struct wow_dbc_row item_row;
		if (!dbc_get_row_indexed(g_wow->dbc.item, &item_row, item))
		{
			LOG_WARN("can't find player item %" PRIu32, item);
			continue;
		}
		uint32_t item_display = wow_dbc_get_u32(&item_row, 4);
		if (!item_display)
			continue;
		struct wow_dbc_row item_display_row;
		if (!dbc_get_row_indexed(g_wow->dbc.item_display_info, &item_display_row, item_display))
		{
			LOG_WARN("can't find item display %" PRIu32, item_display);
			continue;
		}
		for (size_t j = 0; j < 8; ++j)
		{
			if (!(g_texture_masks[i] & (1 << j)))
				continue;
			const char *texture = wow_dbc_get_str(&item_display_row, 60 + j * 4);
			if (!texture[0])
				continue;
			char texture_fn[256];
			snprintf(texture_fn, sizeof(texture_fn), "item/texturecomponents/%s/%s", g_texture_components_dirs[j], texture);
			normalize_blp_filename(texture_fn, sizeof(texture_fn));
			struct wow_mpq_file *mpq_file = wow_mpq_get_file(g_wow->mpq_compound, texture_fn);
			if (!mpq_file)
			{
				int len = strlen(texture_fn) - 4;
				char gendered_fn[256];
				snprintf(gendered_fn, sizeof(gendered_fn), "%.*s%s.BLP", len, texture_fn, unit_get_gender(UNIT) ? "_F" : "_M");
				mpq_file = wow_mpq_get_file(g_wow->mpq_compound, gendered_fn);
				if (!mpq_file)
				{
					snprintf(gendered_fn, sizeof(gendered_fn), "%.*s_U.BLP", len, texture_fn);
					mpq_file = wow_mpq_get_file(g_wow->mpq_compound, gendered_fn);
					if (!mpq_file)
					{
						LOG_ERROR("file %s not found", texture_fn);
						continue;
					}
				}
			}
			struct wow_blp_file *blp_file = wow_blp_file_new(mpq_file);
			wow_mpq_file_delete(mpq_file);
			if (!blp_file)
			{
				LOG_ERROR("invalid blp file %s", texture_fn);
				continue;
			}
			patch_texture(image_data, image_width, blp_file, &g_texture_sections[j]);
			wow_blp_file_delete(blp_file);
		}
	}
	blp_texture_delete(PLAYER->skin_texture);
	for (size_t i = 0; i < image_width * image_height; i++)
	{
		uint8_t *n = &image_data[i * 4];
		uint8_t tmp = n[0];
		n[0] = n[2];
		n[2] = tmp;
	}
	PLAYER->skin_texture = blp_texture_from_data(image_data, image_width, image_height);
	if (WORLD_OBJECT->m2)
		gx_m2_instance_set_skin_texture(WORLD_OBJECT->m2, PLAYER->skin_texture);
	mem_free(MEM_GENERIC, image_data);
}

static void update_render_batches(struct object *object)
{
	uint8_t facial_hair;
	uint8_t hair_style;
	unit_get_visual_settings(UNIT, NULL, NULL, &hair_style, NULL, &facial_hair);
	uint32_t items[UNIT_ITEM_LAST] = {0};
	for (size_t i = 0; i < UNIT_ITEM_LAST; ++i)
	{
		items[i] = object_fields_get_u64(&object->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + 16 * g_items_display_offsets[i]);
		if (!items[i])
		{
			UNIT->items[i].id = 0;
			continue;
		}
		LOG_DEBUG("items[%d] = %" PRIu32, (int)i, items[i]);
		struct wow_dbc_row item_row;
		if (!dbc_get_row_indexed(g_wow->dbc.item, &item_row, items[i]))
		{
			LOG_WARN("can't find player item %" PRIu32, items[i]);
			UNIT->items[i].id = 0;
			continue;
		}
		UNIT->items[i].id = wow_dbc_get_u32(&item_row, 4);
		LOG_DEBUG("display: %" PRIu32, UNIT->items[i].id);
	}
	unit_update_items_batches(object, &hair_style, &facial_hair);
}

static void update_displayid(struct object *object)
{
	struct wow_dbc_row row;
	if (!dbc_get_row_indexed(g_wow->dbc.creature_display_info, &row, object_fields_get_u32(&object->fields, UNIT_FIELD_DISPLAYID)))
	{
		LOG_WARN("unknown displayid %" PRIu32, object_fields_get_u32(&object->fields, UNIT_FIELD_DISPLAYID));
		return;
	}
	generate_skin_texture(object);
	uint32_t model = wow_dbc_get_u32(&row, 4);
	UNIT->scale = wow_dbc_get_flt(&row, 16);
	if (!dbc_get_row_indexed(g_wow->dbc.creature_model_data, &row, model))
	{
		LOG_WARN("unknown model data for display id %" PRIu32, object_fields_get_u32(&object->fields, UNIT_FIELD_DISPLAYID));
		return;
	}
	char filename[512];
	snprintf(filename, sizeof(filename), "%s", wow_dbc_get_str(&row, 8));
	LOG_INFO("filename: %s", filename);
	normalize_m2_filename(filename, sizeof(filename));
	worldobj_set_m2(WORLD_OBJECT, filename);
	PLAYER->clean_batches = false;
	struct blp_texture *skin_texture;
	struct blp_texture *skin_extra_texture;
	struct blp_texture *hair_texture;
	unit_get_skin_textures(UNIT, &skin_texture, &skin_extra_texture);
	unit_get_hair_textures(UNIT, &hair_texture, NULL, NULL);
	gx_m2_instance_set_skin_texture(WORLD_OBJECT->m2, PLAYER->skin_texture);
	gx_m2_instance_set_skin_extra_texture(WORLD_OBJECT->m2, skin_extra_texture);
	gx_m2_instance_set_hair_texture(WORLD_OBJECT->m2, hair_texture);
	if (PLAYER->skin_extra_texture)
		cache_unref_by_ref_blp(g_wow->cache, PLAYER->skin_extra_texture);
	PLAYER->skin_extra_texture = skin_extra_texture;
	if (PLAYER->hair_texture)
		cache_unref_by_ref_blp(g_wow->cache, PLAYER->hair_texture);
	PLAYER->hair_texture = hair_texture;
}

static void add_to_render(struct object *object)
{
	for (size_t i = 0; i < 4; ++i)
	{
		if (object_fields_get_bit(&object->fields, PLAYER_FIELD_INV_SLOT_BAG0 + i))
		{
			/* XXX all bag slots */
			lua_pushstring(g_wow->interface->L, "bag");
			lua_pushinteger(g_wow->interface->L, i + 1);
			interface_execute_event(g_wow->interface, EVENT_BAG_UPDATE, 1);
			object_fields_disable_bit(&object->fields, PLAYER_FIELD_INV_SLOT_BAG0 + i);
		}
	}
	if (object_fields_get_bit(&object->fields, UNIT_FIELD_DISPLAYID))
	{
		update_displayid(object);
		object_fields_disable_bit(&object->fields, UNIT_FIELD_DISPLAYID);
	}
	if (!PLAYER->db_get)
	{
		const struct db_name *name = db_get_name(g_wow->db, object_fields_get_u64(&object->fields, OBJECT_FIELD_GUID));
		/* XXX: as callback */
		if (name && name->id)
		{
			PLAYER->db_get = true;
			gx_text_set_text(UNIT->text, name->name);
		}
	}
	if (!PLAYER->clean_batches && WORLD_OBJECT->m2 && WORLD_OBJECT->m2->parent->loaded)
	{
		update_render_batches(object);
		PLAYER->clean_batches = true;
	}
	{
		bool update = false;
		for (size_t i = 0; i < 19; ++i)
		{
			if (object_fields_get_bit(&object->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + i * 16))
			{
				object_fields_disable_bit(&object->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + i * 16);
				update = true;
			}
			if (object_fields_get_bit(&object->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + i * 16 + 1))
			{
				object_fields_disable_bit(&object->fields, PLAYER_FIELD_VISIBLE_ITEM_1_0 + i * 16 + 1);
				update = true;
			}
		}
		if (update)
		{
			update_render_batches(object);
			generate_skin_texture(object);
			unit_update_items(object);
		}
	}
	unit_worldobj_vtable.add_to_render(object);
}

static void get_visual_settings(const struct object *object, uint8_t *face, uint8_t *hair_color, uint8_t *hair_style, uint8_t *skin_color, uint8_t *facial_hair)
{
	uint32_t bytes = object_fields_get_u32(&object->fields, PLAYER_FIELD_BYTES);
	uint32_t bytes2 = object_fields_get_u32(&object->fields, PLAYER_FIELD_BYTES_2);
	if (face)
		*face = bytes >> 8;
	if (hair_color)
		*hair_color = bytes >> 24;
	if (hair_style)
		*hair_style = bytes >> 16;
	if (skin_color)
		*skin_color = bytes >> 0;
	if (facial_hair)
		*facial_hair = bytes2 >> 0;
}

const struct object_vtable player_object_vtable =
{
	.ctr = ctr,
	.dtr = dtr,
	.on_field_changed = on_field_changed,
};

const struct unit_vtable player_unit_vtable =
{
	.get_visual_settings = get_visual_settings,
};

const struct worldobj_vtable player_worldobj_vtable =
{
	.add_to_render = add_to_render,
};

struct player *player_new(uint64_t guid)
{
	struct object *object = mem_malloc(MEM_OBJ, sizeof(struct player));
	if (!object)
		return NULL;
	object->vtable = &player_object_vtable;
	if (!object->vtable->ctr(object, guid))
	{
		object->vtable->dtr(object);
		mem_free(MEM_OBJ, object);
		return NULL;
	}
	return (struct player*)object;
}
