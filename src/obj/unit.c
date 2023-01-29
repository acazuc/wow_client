#include "unit.h"

#include "obj/update_fields.h"

#include "net/world_socket.h"
#include "net/network.h"
#include "net/packet.h"

#include "gx/frame.h"
#include "gx/text.h"
#include "gx/m2.h"

#include "map/map.h"

#include "phys/physics.h"

#include "performance.h"
#include "const.h"
#include "cache.h"
#include "log.h"
#include "blp.h"
#include "wow.h"
#include "dbc.h"

#include <jks/optional.h>
#include <jks/mat3.h>

#include <libwow/dbc.h>
#include <libwow/mpq.h>
#include <libwow/blp.h>

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define OBJECT ((struct object*)unit)
#define UNIT ((struct unit*)object)
#define WORLD_OBJECT ((struct worldobj*)object)

static const char *g_object_components_dirs[UNIT_ITEM_LAST] =
{
	"head",
	"shoulder",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"cape",
	"weapon",
	"shield",
};

static const uint16_t g_attachments_ids[UNIT_ITEM_LAST * 2] =
{
	11, 11,
	6 , 5 ,
	0 , 0 ,
	0 , 0 ,
	0 , 0 ,
	0 , 0 ,
	0 , 0 ,
	0 , 0 ,
	0 , 0 ,
	0 , 0 ,
	12, 12,
	1 , 1 ,
	2 , 2 ,
};

static void (*g_field_functions[UNIT_FIELD_MAX - UNIT_FIELD_MIN])(struct object *object) =
{
};

static bool ctr(struct object *object, uint64_t guid)
{
	if (!worldobj_object_vtable.ctr(object, guid))
		return false;
	WORLD_OBJECT->worldobj_vtable = &unit_worldobj_vtable;
	if (!object_fields_resize(&object->fields, UNIT_FIELD_MAX))
		return false;
	object_fields_set_u32(&object->fields, OBJECT_FIELD_TYPE, OBJECT_MASK_OBJECT | OBJECT_MASK_UNIT);
	UNIT->text = gx_text_new();
	if (!UNIT->text)
		return false;
	gx_text_set_color(UNIT->text, (struct vec4f){.5f, .5f, 1.f, 1.f});
	for (size_t i = 0; i < sizeof(UNIT->items) / sizeof(*UNIT->items); ++i)
	{
		UNIT->items[i].id = 0;
		UNIT->items[i].m2_left = NULL;
		UNIT->items[i].m2_right = NULL;
		UNIT->items[i].tex_left = NULL;
		UNIT->items[i].tex_right = NULL;
	}
	UNIT->scale = 1;
	return true;
}

static void clear_item(struct unit_item *item)
{
	if (item->m2_left)
	{
		render_gc_m2(item->m2_left);
		item->m2_left = NULL;
	}
	if (item->m2_right)
	{
		render_gc_m2(item->m2_right);
		item->m2_right = NULL;
	}
	if (item->tex_left)
	{
		cache_unref_by_ref_blp(g_wow->cache, item->tex_left);
		item->tex_left = NULL;
	}
	if (item->tex_right)
	{
		cache_unref_by_ref_blp(g_wow->cache, item->tex_right);
		item->tex_right = NULL;
	}
}

static void dtr(struct object *object)
{
	render_gc_text(UNIT->text);
	for (size_t i = 0; i < sizeof(UNIT->items) / sizeof(*UNIT->items); ++i)
		clear_item(&UNIT->items[i]);
	worldobj_object_vtable.dtr(object);
}

static void on_field_changed(struct object *object, uint32_t field)
{
	if (field < UNIT_FIELD_MIN)
		return worldobj_object_vtable.on_field_changed(object, field);
	if (field >= UNIT_FIELD_MAX)
		return;
	if (g_field_functions[field - UNIT_FIELD_MIN])
		g_field_functions[field - UNIT_FIELD_MIN](object);
}

static bool get_char_section(const struct unit *unit, uint8_t gender, uint8_t base_section, uint8_t variation, uint8_t color, struct wow_dbc_row *row)
{
	uint8_t race = unit_get_race(unit);
	if (!race)
	{
		LOG_WARN("no race found");
		return false;
	}
	struct dbc *dbc = g_wow->dbc.char_sections;
	for (uint32_t i = 0; i < dbc->file->header.record_count; ++i)
	{
		*row = dbc_get_row(dbc, i);
		if (wow_dbc_get_u32(row, 4) == race
		 && wow_dbc_get_u32(row, 8) == gender
		 && wow_dbc_get_u32(row, 12) == base_section
		 && wow_dbc_get_u32(row, 16) == variation
		 && wow_dbc_get_u32(row, 20) == color)
			return true;
	}
	LOG_WARN("can't find section for {race: %" PRIu8 ", gender: %" PRIu8 ", base_section: %" PRIu8 ", variation: %" PRIu8 ", color: %" PRIu8 "}", race, gender, base_section, variation, color);
	return false;
}

static bool get_textures_names(const struct unit *unit, uint8_t base_section, uint8_t variation, uint8_t color, char *tex1, char *tex2, char *tex3)
{
	struct wow_dbc_row row;
	uint8_t gender = unit_get_gender(unit);
	if (!get_char_section(unit, gender, base_section, variation, color, &row))
		return false;
	if (tex1)
	{
		snprintf(tex1, 256, "%s", wow_dbc_get_str(&row, 24));
		normalize_blp_filename(tex1, 256);
	}
	if (tex2)
	{
		snprintf(tex2, 256, "%s", wow_dbc_get_str(&row, 28));
		normalize_blp_filename(tex2, 256);
	}
	if (tex3)
	{
		snprintf(tex3, 256, "%s", wow_dbc_get_str(&row, 32));
		normalize_blp_filename(tex3, 256);
	}
	return true;
}

static void get_texture_file(char *name, struct wow_blp_file **texture)
{
	if (!name[0])
	{
		*texture = NULL;
		return;
	}
	struct wow_mpq_file *mpq_file = wow_mpq_get_file(g_wow->mpq_compound, name);
	if (!mpq_file)
	{
		LOG_ERROR("file %s not found", name);
		*texture = NULL;
		return;
	}
	*texture = wow_blp_file_new(mpq_file);
	wow_mpq_file_delete(mpq_file);
	if (!*texture)
		LOG_ERROR("invalid blp file %s", name);
}

static void get_texture(char *name, struct blp_texture **texture)
{
	if (!name[0])
	{
		*texture = NULL;
		return;
	}
	if (!cache_ref_by_key_blp(g_wow->cache, name, texture))
	{
		*texture = NULL;
		return;
	}
	blp_texture_ask_load(*texture);
}

static void get_textures_files(const struct unit *unit, uint8_t base_section, uint8_t variation, uint8_t color, struct wow_blp_file **tex1, struct wow_blp_file **tex2, struct wow_blp_file **tex3)
{
	char tex1_name[256];
	char tex2_name[256];
	char tex3_name[256];
	if (!get_textures_names(unit, base_section, variation, color, tex1 ? tex1_name : NULL, tex2 ? tex2_name : NULL, tex3 ? tex3_name : NULL))
	{
		if (tex1)
			*tex1 = NULL;
		if (tex2)
			*tex2 = NULL;
		if (tex3)
			*tex3 = NULL;
		return;
	}
	if (tex1)
		get_texture_file(tex1_name, tex1);
	if (tex2)
		get_texture_file(tex2_name, tex2);
	if (tex3)
		get_texture_file(tex3_name, tex3);
}

static void get_textures(const struct unit *unit, uint8_t base_section, uint8_t variation, uint8_t color, struct blp_texture **tex1, struct blp_texture **tex2, struct blp_texture **tex3)
{
	char tex1_name[256];
	char tex2_name[256];
	char tex3_name[256];
	if (!get_textures_names(unit, base_section, variation, color, tex1 ? tex1_name : NULL, tex2 ? tex2_name : NULL, tex3 ? tex3_name : NULL))
	{
		if (tex1)
			*tex1 = NULL;
		if (tex2)
			*tex2 = NULL;
		if (tex3)
			*tex3 = NULL;
		return;
	}
	if (tex1)
		get_texture(tex1_name, tex1);
	if (tex2)
		get_texture(tex2_name, tex2);
	if (tex3)
		get_texture(tex3_name, tex3);
}

void unit_get_skin_textures_files(const struct unit *unit, struct wow_blp_file **skin, struct wow_blp_file **skin_extra)
{
	if (!skin && !skin_extra)
		return;
	uint8_t color;
	unit_get_visual_settings(unit, NULL, NULL, NULL, &color, NULL);
	get_textures_files(unit, 0, 0, color, skin, skin_extra, NULL);
}

void unit_get_face_textures_files(const struct unit *unit, struct wow_blp_file **lower, struct wow_blp_file **upper)
{
	if (!lower && !upper)
		return;
	uint8_t color;
	uint8_t face;
	unit_get_visual_settings(unit, &face, NULL, NULL, &color, NULL);
	get_textures_files(unit, 1, face, color, lower, upper, NULL);
}

void unit_get_facial_hair_textures_files(const struct unit *unit, struct wow_blp_file **lower, struct wow_blp_file **upper)
{
	if (!lower && !upper)
		return;
	uint8_t color;
	uint8_t style;
	unit_get_visual_settings(unit, NULL, &color, NULL, NULL, &style);
	get_textures_files(unit, 2, style, color, lower, upper, NULL);
}

void unit_get_hair_textures_files(const struct unit *unit, struct wow_blp_file **hair, struct wow_blp_file **lower, struct wow_blp_file **upper)
{
	if (!hair && !lower && !upper)
		return;
	uint8_t color;
	uint8_t style;
	unit_get_visual_settings(unit, NULL, &color, &style, NULL, NULL);
	get_textures_files(unit, 3, style ? style : 1, color, hair, lower, upper); /* style 0 is for "no hair", but facial hair might need hair texture */
}

void unit_get_underwear_textures_files(const struct unit *unit, struct wow_blp_file **pelvis, struct wow_blp_file **torso)
{
	if (!pelvis && !torso)
		return;
	uint8_t color;
	unit_get_visual_settings(unit, NULL, NULL, NULL, &color, NULL);
	get_textures_files(unit, 4, 0, color, pelvis, torso, NULL);
}

void unit_get_skin_textures(const struct unit *unit, struct blp_texture **skin, struct blp_texture **skin_extra)
{
	if (!skin && !skin_extra)
		return;
	uint8_t color;
	unit_get_visual_settings(unit, NULL, NULL, NULL, &color, NULL);
	get_textures(unit, 0, 0, color, skin, skin_extra, NULL);
}

void unit_get_face_textures(const struct unit *unit, struct blp_texture **lower, struct blp_texture **upper)
{
	if (!lower && !upper)
		return;
	uint8_t color;
	uint8_t face;
	unit_get_visual_settings(unit, &face, NULL, NULL, &color, NULL);
	get_textures(unit, 1, face, color, lower, upper, NULL);
}

void unit_get_facial_hair_textures(const struct unit *unit, struct blp_texture **lower, struct blp_texture **upper)
{
	if (!lower && !upper)
		return;
	uint8_t color;
	uint8_t style;
	unit_get_visual_settings(unit, NULL, &color, NULL, NULL, &style);
	get_textures(unit, 2, style, color, lower, upper, NULL);
}

void unit_get_hair_textures(const struct unit *unit, struct blp_texture **hair, struct blp_texture **lower, struct blp_texture **upper)
{
	if (!hair && !lower && !upper)
		return;
	uint8_t color;
	uint8_t style;
	unit_get_visual_settings(unit, NULL, &color, &style, NULL, NULL);
	get_textures(unit, 3, style ? style : 1, color, hair, lower, upper); /* style 0 is for "no hair", but facial hair might need hair texture */
}

void unit_get_underwear_textures(const struct unit *unit, struct blp_texture **pelvis, struct blp_texture **torso)
{
	if (!pelvis && !torso)
		return;
	uint8_t color;
	unit_get_visual_settings(unit, NULL, NULL, NULL, &color, NULL);
	get_textures(unit, 4, 0, color, pelvis, torso, NULL);
}

static void add_m2_item(struct object *object, struct gx_m2_instance *m2, uint32_t attachment_id)
{
	if (WORLD_OBJECT->m2->parent->attachment_lookups_nb < attachment_id)
	{
		LOG_ERROR("no attachment lookup id %" PRIu32, attachment_id);
		return;
	}
	attachment_id = WORLD_OBJECT->m2->parent->attachment_lookups[attachment_id];
	if (WORLD_OBJECT->m2->parent->attachments_nb < attachment_id)
	{
		LOG_ERROR("no attachment id %" PRIu32, attachment_id);
		return;
	}
	struct wow_m2_attachment *attachment = &WORLD_OBJECT->m2->parent->attachments[attachment_id];
	uint16_t bone_id = attachment->bone;
	if (bone_id == UINT16_MAX)
		return;
	if (WORLD_OBJECT->m2->render_frames[g_wow->cull_frame_id].bone_mats.size <= bone_id)
	{
		LOG_ERROR("invalid key bone id: %" PRIu16, bone_id);
		return;
	}
	struct mat4f *bone_mat = JKS_ARRAY_GET(&WORLD_OBJECT->m2->render_frames[g_wow->cull_frame_id].bone_mats, bone_id, struct mat4f);
	struct mat4f tmp1;
	struct mat4f tmp2;
	MAT4_MUL(tmp1, WORLD_OBJECT->m2->m, *bone_mat);
	MAT4_TRANSLATE(tmp2, tmp1, attachment->position);
	gx_m2_instance_set_mat(m2, &tmp2);
	VEC3_CPY(m2->pos, WORLD_OBJECT->position);
	gx_m2_instance_add_to_render(m2, true);
}

static void add_text_to_render(struct object *object)
{
	if (WORLD_OBJECT->m2->parent->attachment_lookups_nb < 18)
	{
		LOG_ERROR("no attachment lookup id %" PRIu32, 18);
		return;
	}
	uint16_t attachment_id = WORLD_OBJECT->m2->parent->attachment_lookups[18];
	if (attachment_id == UINT16_MAX)
		return;
	if (WORLD_OBJECT->m2->parent->attachments_nb < attachment_id)
	{
		LOG_ERROR("no attachment id %" PRIu32, attachment_id);
		return;
	}
	struct wow_m2_attachment *attachment = &WORLD_OBJECT->m2->parent->attachments[attachment_id];
	uint16_t bone_id = attachment->bone;
	if (bone_id == UINT16_MAX)
		return;
	if (WORLD_OBJECT->m2->render_frames[g_wow->cull_frame_id].bone_mats.size <= bone_id)
	{
		LOG_ERROR("invalid key bone id: %" PRIu16, bone_id);
		return;
	}
	WORLD_OBJECT->m2->bones_calculated = false;
	gx_m2_instance_calc_bones(WORLD_OBJECT->m2);
	struct mat4f *bone_mat = JKS_ARRAY_GET(&WORLD_OBJECT->m2->render_frames[g_wow->cull_frame_id].bone_mats, bone_id, struct mat4f);
	struct mat4f tmp1;
	struct mat4f tmp2;
	MAT4_MUL(tmp1, WORLD_OBJECT->m2->m, *bone_mat);
	MAT4_TRANSLATE(tmp2, tmp1, attachment->position);
	struct vec3f pos;
	struct vec3f scale;
	VEC3_SET(pos, tmp2.w.x, tmp2.w.y, tmp2.w.z);
	VEC3_SETV(scale, UNIT->scale);
	gx_text_set_pos(UNIT->text, pos);
	gx_text_set_scale(UNIT->text, scale);
	gx_text_add_to_render(UNIT->text);
}

static void add_to_render(struct object *object)
{
	if (!WORLD_OBJECT->m2)
		return;
	VEC3_CPY(WORLD_OBJECT->m2->pos, WORLD_OBJECT->position);
	struct mat4f tmp;
	struct mat4f mat;
	MAT4_IDENTITY(tmp);
	MAT4_TRANSLATE(mat, tmp, WORLD_OBJECT->position);
	MAT4_ROTATEY(float, tmp, mat, WORLD_OBJECT->orientation);
	MAT4_ROTATEX(float, mat, tmp, WORLD_OBJECT->slope);
	MAT4_ROTATEY(float, tmp, mat, M_PI / 2);
	MAT4_SCALEV(mat, tmp, UNIT->scale * object_fields_get_flt(&object->fields, OBJECT_FIELD_SCALE));
	gx_m2_instance_set_mat(WORLD_OBJECT->m2, &mat);
	worldobj_vtable.add_to_render(object);
	if (WORLD_OBJECT->m2->render_frames[g_wow->cull_frame_id].culled)
		return;
	for (size_t i = 0; i < sizeof(UNIT->items) / sizeof(*UNIT->items); ++i)
	{
		if (UNIT->items[i].m2_left)
			add_m2_item(object, UNIT->items[i].m2_left , UNIT->items[i].attachment_left);
		if (UNIT->items[i].m2_right)
			add_m2_item(object, UNIT->items[i].m2_right, UNIT->items[i].attachment_right);
	}
	add_text_to_render(object);
}

uint8_t unit_get_class(const struct unit *unit)
{
	return (object_fields_get_u32(&OBJECT->fields, UNIT_FIELD_BYTES_0) >> 8) & 0xff;
}

uint8_t unit_get_race(const struct unit *unit)
{
	return (object_fields_get_u32(&OBJECT->fields, UNIT_FIELD_BYTES_0) >> 0) & 0xff;
}

uint8_t unit_get_gender(const struct unit *unit)
{
	return (object_fields_get_u32(&OBJECT->fields, UNIT_FIELD_BYTES_0) >> 16) & 0xff;
}

void unit_get_visual_settings(const struct unit *unit, uint8_t *face, uint8_t *hair_color, uint8_t *hair_style, uint8_t *skin_color, uint8_t *facial_hair)
{
	unit->unit_vtable->get_visual_settings(OBJECT, face, hair_color, hair_style, skin_color, facial_hair);
}

static bool get_hair_geoset(uint8_t race, uint8_t gender, const uint8_t *hair_style, uint32_t *hair_geoset, bool *bald)
{
	if (!race)
		return false;
	if (!hair_style)
		return false;
	struct dbc *dbc = g_wow->dbc.char_hair_geosets;
	for (size_t i = 0; i < dbc->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(dbc, i);
		if (wow_dbc_get_u32(&row, 4) == race
		 && wow_dbc_get_u32(&row, 8) == gender
		 && wow_dbc_get_u32(&row, 12) == *hair_style)
		{
			*hair_geoset = wow_dbc_get_u32(&row, 16);
			*bald = wow_dbc_get_u32(&row, 20);
			return true;
		}
	}
	LOG_WARN("no hair geoset found: {race: %" PRIu8 ", gender: %" PRIu8 ", hair_style: %" PRIu8 "}", race, gender, *hair_style);
	return false;
}

static bool get_facial_hair_geosets(uint8_t race, uint8_t gender, const uint8_t *facial_hair, uint32_t *facial_hair_geosets)
{
	if (!race)
		return false;
	if (!facial_hair)
		return false;
	struct dbc *dbc = g_wow->dbc.character_facial_hair_styles;
	for (size_t i = 0; i < dbc->file->header.record_count; ++i)
	{
		struct wow_dbc_row row = dbc_get_row(dbc, i);
		if (wow_dbc_get_u32(&row, 0) == race
		 && wow_dbc_get_u32(&row, 4) == gender
		 && wow_dbc_get_u32(&row, 8) == *facial_hair)
		{
			facial_hair_geosets[0] = wow_dbc_get_u32(&row, 24);
			facial_hair_geosets[1] = wow_dbc_get_u32(&row, 28);
			facial_hair_geosets[2] = wow_dbc_get_u32(&row, 32);
			facial_hair_geosets[3] = wow_dbc_get_u32(&row, 36);
			facial_hair_geosets[4] = wow_dbc_get_u32(&row, 40);
			return true;
		}
	}
	LOG_WARN("no facial hair geoset found: {race: %" PRIu8 ", gender: %" PRIu8 ", facial_hair: %" PRIu8 "}", race, gender, *facial_hair);
	return false;
}

void unit_update_items_batches(struct object *object, const uint8_t *hair_style, const uint8_t *facial_hair)
{
	uint8_t race = unit_get_race(UNIT);
	uint8_t gender = unit_get_gender(UNIT);
	OPTIONAL_DEF(, struct wow_dbc_row) items_rows[UNIT_ITEM_LAST] = {{0}};
	for (size_t i = 0; i < UNIT_ITEM_LAST; ++i)
	{
		if (!UNIT->items[i].id)
			continue;
		struct wow_dbc_row item_row;
		if (!dbc_get_row_indexed(g_wow->dbc.item_display_info, &item_row, UNIT->items[i].id))
		{
			LOG_WARN("can't find item display %" PRIu32, UNIT->items[i].id);
			continue;
		}
		OPTIONAL_CTR(items_rows[i], item_row);
	}
	uint8_t hidden_head = 0;
	if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_HELM]))
	{
		uint32_t geoset_vis_id = wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_HELM]), gender ? 56 : 52);
		if (geoset_vis_id)
		{
			struct wow_dbc_row row;
			if (dbc_get_row_indexed(g_wow->dbc.helmet_geoset_vis_data, &row, geoset_vis_id))
			{
				uint32_t mask = (1 << race);
				for (uint32_t i = 0; i < 7; ++i)
				{
					if (wow_dbc_get_u32(&row, 4 + i * 4) & mask)
						hidden_head |= (1 << i);
				}
			}
			else
			{
				LOG_WARN("can't find helmet geoset vis data %" PRIu32, geoset_vis_id);
			}
		}
	}
	uint32_t facial_hair_geosets[5];
	uint32_t hair_geoset;
	uint32_t ears_geoset;
	bool bald;
	if (hidden_head & (1 << 0))
	{
		hair_geoset = 1;
	}
	else
	{
		if (!get_hair_geoset(race, gender, hair_style, &hair_geoset, &bald) || bald || !hair_geoset)
			hair_geoset = 1;
	}
	if (get_facial_hair_geosets(race, gender, facial_hair, &facial_hair_geosets[0]))
	{
		if (!facial_hair_geosets[0])
			facial_hair_geosets[0] = 1;
		if (facial_hair_geosets[2])
			facial_hair_geosets[2] = 1;
		if (facial_hair_geosets[1])
			facial_hair_geosets[1] = 1;
		if (facial_hair_geosets[3])
			facial_hair_geosets[3] = 1;
		if (facial_hair_geosets[4])
			facial_hair_geosets[4] = 1;
	}
	else
	{
		facial_hair_geosets[0] = 1;
		facial_hair_geosets[1] = 1;
		facial_hair_geosets[2] = 1;
		facial_hair_geosets[3] = 1;
		facial_hair_geosets[4] = 1;
	}
	if (hidden_head & (1 << 1))
		facial_hair_geosets[0] = 1;
	if (hidden_head & (1 << 2))
		facial_hair_geosets[1] = 1;
	if (hidden_head & (1 << 3))
		facial_hair_geosets[2] = 1;
	if (hidden_head & (1 << 5))
		facial_hair_geosets[3] = 1;
	if (hidden_head & (1 << 6))
		facial_hair_geosets[4] = 1;
	if (hidden_head & (1 << 4))
		ears_geoset = 1;
	else
		ears_geoset = 2;
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_HAIR + hair_geoset);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_FACIAL1 + facial_hair_geosets[0]);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_FACIAL2 + facial_hair_geosets[2]);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_FACIAL3 + facial_hair_geosets[1]);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_MISC + facial_hair_geosets[3]);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_EYES + facial_hair_geosets[4]);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_EARS + ears_geoset);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_GLOVES + 1);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_BOOTS + 1);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_PANTS + 1);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_TROUSERS + 1);
	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_CLOAK + 1);

	gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_BONE + 1);

	if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_GLOVES]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_GLOVES]), 28))
	{
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_GLOVES, 100 * WOW_M2_SKIN_SECTION_GLOVES + 99);
		gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_GLOVES + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_GLOVES]), 28));
	}
	else if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_CHEST]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_CHEST]), 28))
	{
		gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_WRISTS + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_CHEST]), 28));
	}
	if (true) /* XXX: shirt related check */
	{
		if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_SHIRT]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_SHIRT]), 28))
		{
			gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_WRISTS + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_SHIRT]), 28));
		}
	}
	bool dress_chest = false;
	bool dress_pants = false;
	bool dress_tabard = false;
	if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_CHEST]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_CHEST]), 36))
	{
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_BOOTS, 100 * WOW_M2_SKIN_SECTION_BOOTS + 99);
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_KNEEPADS, 100 * WOW_M2_SKIN_SECTION_KNEEPADS + 99);
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_PANTS, 100 * WOW_M2_SKIN_SECTION_PANTS + 99);
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_TROUSERS, 100 * WOW_M2_SKIN_SECTION_TROUSERS + 99);
		gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_TROUSERS + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_CHEST]), 36));
		dress_chest = true;
	}
	else if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_LEGS]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_LEGS]), 36))
	{
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_BOOTS, 100 * WOW_M2_SKIN_SECTION_BOOTS + 99);
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_KNEEPADS, 100 * WOW_M2_SKIN_SECTION_KNEEPADS + 99);
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_PANTS, 100 * WOW_M2_SKIN_SECTION_PANTS + 99);
		gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_TROUSERS, 100 * WOW_M2_SKIN_SECTION_TROUSERS + 99);
		gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_TROUSERS + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_LEGS]), 36));
		dress_pants = true;
	}
	else
	{
		if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_BOOTS]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_BOOTS]), 28))
		{
			gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_BOOTS, 100 * WOW_M2_SKIN_SECTION_BOOTS + 99);
			gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_KNEEPADS + 1);
			gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_BOOTS + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_BOOTS]), 28));
		}
		else if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_LEGS]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_LEGS]), 32))
		{
			gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_KNEEPADS + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_LEGS]), 32));
		}
		else
		{
			gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_KNEEPADS + 1);
		}
		if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_TABARD]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_TABARD]), 28))
		{
			gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_TABARD + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_TABARD]), 28));
			dress_tabard = true;
		}
	}
	/* gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_TABARD + 1); */
	if (!dress_chest)
	{
		if (!dress_pants)
		{
			/* gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_TABARD + 2); */
		}
		if (!dress_tabard)
		{
			if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_SHIRT]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_SHIRT]), 32))
			{
				/* gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_CHEST, 100 * WOW_M2_SKIN_SECTION_CHEST + 99); */
				gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_CHEST + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_SHIRT]), 32));
			}
			if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_LEGS]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_LEGS]), 28))
			{
				/* gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_PANTS, 100 * WOW_M2_SKIN_SECTION_PANTS + 99); */
				gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_PANTS + 2 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_LEGS]), 28));
			}
		}
	}
	if (OPTIONAL_ISSET(items_rows[UNIT_ITEM_CAPE]) && wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_CAPE]), 28))
	{
		const char *fn = wow_dbc_get_str(&OPTIONAL_GET(items_rows[UNIT_ITEM_CAPE]), 12);
		if (fn)
		{
			char cape_filename[256];
			snprintf(cape_filename, sizeof(cape_filename), "Item\\ObjectComponents\\Cape\\%s", fn);
			normalize_blp_filename(cape_filename, sizeof(cape_filename));
			struct blp_texture *texture;
			if (cache_ref_by_key_blp(g_wow->cache, cape_filename, &texture))
				blp_texture_ask_load(texture);
			else
				texture = NULL;
			gx_m2_instance_set_object_texture(WORLD_OBJECT->m2, texture);
			gx_m2_instance_disable_batches(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_CLOAK, 100 * WOW_M2_SKIN_SECTION_CLOAK + 99);
			gx_m2_instance_enable_batch(WORLD_OBJECT->m2, 100 * WOW_M2_SKIN_SECTION_CLOAK + 1 + wow_dbc_get_u32(&OPTIONAL_GET(items_rows[UNIT_ITEM_CAPE]), 28));
		}
	}
}

static bool unit_update_item_side(struct object *object, uint32_t i, const char *model_name, const char *texture_name, struct gx_m2_instance **model, struct blp_texture **texture)
{
	char model_fn[256];
	char texture_fn[256];
	const char *dir;
	if (i == UNIT_ITEM_OFFHAND)
	{
		dir = "weapon";//g_object_components_dirs[i]; /* XXX */
	}
	else
	{
		dir = g_object_components_dirs[i];
	}
	/* gauntlet:
	 * slotid 13, 21, 22 is a weapon texture
	 * slotid 10, is a shoulder texture
	 */
	snprintf(model_fn, sizeof(model_fn), "Item/ObjectComponents/%s/%s", dir, model_name);
	snprintf(texture_fn, sizeof(texture_fn), "Item/ObjectComponents/%s/%s", dir, texture_name);
	normalize_m2_filename(model_fn, sizeof(model_fn));
	normalize_blp_filename(texture_fn, sizeof(texture_fn));
	if (i == UNIT_ITEM_HELM)
	{
		uint8_t race = unit_get_race(UNIT) - 1;
		uint8_t gender = unit_get_gender(UNIT);
		if (race * 2 + gender < 22)
		{
			size_t len = strlen(model_fn);
			char tmp[256];
			const char *extensions[11 * 2] =
			{
				"HUM", "HUF",
				"ORM", "ORF",
				"DWM", "DWF",
				"NIM", "NIF",
				"SCM", "SCF",
				"TAM", "TAF",
				"GNM", "GNF",
				"TRM", "TRF",
				"GOM", "GOF",
				"BEM", "BEF",
				"DRM", "DRF",
			};
			snprintf(tmp, sizeof(tmp), "%.*s_%s.M2", (int)len - 3, model_fn, extensions[race * 2 + gender]);
			strcpy(model_fn, tmp);
		}
	}
	struct gx_m2 *m2;
	if (!cache_ref_by_key_m2(g_wow->cache, model_fn, &m2))
	{
		LOG_ERROR("can't get m2 %s", model_fn);
		return false;
	}
	*model = gx_m2_instance_new(m2);
	cache_unref_by_ref_m2(g_wow->cache, m2);
	if (!*model)
	{
		LOG_ERROR("can't create instance for m2 %s", model_fn);
		return false;
	}
	if (!cache_ref_by_key_blp(g_wow->cache, texture_fn, texture))
	{
		LOG_ERROR("can't get blp");
		gx_m2_instance_delete(*model);
		return false;
	}
	blp_texture_ask_load(*texture);
	gx_m2_instance_set_object_texture(*model, *texture);
	gx_m2_ask_load(m2);
	return true;
}

static void unit_update_item(struct object *object, uint32_t i)
{
	if (!UNIT->items[i].id)
	{
#if 0
		LOG_INFO("no id for %u", i);
#endif
		return;
	}
	struct wow_dbc_row item_display_row;
	if (!dbc_get_row_indexed(g_wow->dbc.item_display_info, &item_display_row, UNIT->items[i].id))
	{
		LOG_WARN("can't find item display %" PRIu32, UNIT->items[i].id);
		return;
	}
	UNIT->items[i].attachment_left = g_attachments_ids[i * 2 + 0];
	UNIT->items[i].attachment_right = g_attachments_ids[i * 2 + 1];
	const char *model_left = wow_dbc_get_str(&item_display_row, 4);
	const char *model_right = wow_dbc_get_str(&item_display_row, 8);
	const char *texture_left = wow_dbc_get_str(&item_display_row, 12);
	const char *texture_right = wow_dbc_get_str(&item_display_row, 16);
	struct gx_m2_instance *m2_left = NULL;
	struct gx_m2_instance *m2_right = NULL;
	struct blp_texture *tex_left = NULL;
	struct blp_texture *tex_right = NULL;
	if (model_left[0])
	{
		if (!unit_update_item_side(object, i, model_left, texture_left, &m2_left, &tex_left))
		{
			LOG_ERROR("can't load left model");
			return;
		}
	}
	if (model_right[0])
	{
		if (!unit_update_item_side(object, i, model_right, texture_right, &m2_right, &tex_right))
		{
			LOG_ERROR("can't load right model");
			if (m2_left)
				gx_m2_instance_delete(m2_left);
			if (tex_left)
				blp_texture_ask_unload(tex_left);
			return;
		}
	}
	UNIT->items[i].m2_left = m2_left;
	UNIT->items[i].m2_right = m2_right;
	UNIT->items[i].tex_left = tex_left;
	UNIT->items[i].tex_right = tex_right;
}

void unit_update_items(struct object *object)
{
	for (size_t i = 0; i < UNIT_ITEM_LAST; ++i)
	{
		clear_item(&UNIT->items[i]);
		unit_update_item(object, i);
	}
}

static struct vec3f update_position_collision(struct unit *unit, struct jks_array *triangles, struct vec3f src, struct vec3f dst, size_t recursion, struct vec3f *norm, bool *ground_touched)
{
	if (recursion >= 10)
		return src;
	struct vec3f ray_dir;
	VEC3_SUB(ray_dir, dst, src);
	float dir_len = VEC3_NORM(ray_dir);
	VEC3_DIVV(ray_dir, ray_dir, dir_len);
	float lowest;
	struct collision_triangle *best;
	struct vec3f best_norm;
	if (!find_nearest_triangle(triangles, src, ray_dir, dir_len, &best, &best_norm, &lowest))
		return dst;
	*norm = best_norm;
	struct vec3f collision_point;
	VEC3_MULV(collision_point, ray_dir, lowest);
	VEC3_ADD(collision_point, collision_point, src);

	if (!*ground_touched && collision_point.y >= dst.y && best_norm.y > WALL_CLIMB)
	{
		*ground_touched = true;
		return src;
	}
	else
	{
		float len = dir_len - lowest;
		if (len > 0)
		{
			float d = VEC3_DOT(ray_dir, best_norm);
			struct vec3f repulsion; 
			VEC3_MULV(repulsion, best_norm, d);
			VEC3_MULV(repulsion, repulsion, len + 0.01f);
#if 0
			LOG_INFO("correct %f of {%f, %f, %f} because of %p; norm: %f, %f, %f; d: %f; y: %f", len, dst.x, dst.y, dst.z, best, best_norm.x, best_norm.y, best_norm.z, d, best->points[0].y);
#endif
			VEC3_SUB(dst, dst, repulsion);
		}
	}

	if (!(unit->worldobj.movement_data.flags & (MOVEFLAG_FLYING | MOVEFLAG_FALLING)) && dst.y > src.y && best_norm.y < WALL_CLIMB)
		dst.y = src.y;

	return update_position_collision(unit, triangles, src, dst, recursion + 1, norm, ground_touched);
}

static struct vec3f get_next_pos(struct unit *unit)
{
	struct vec3f dst = unit->worldobj.position;
	float dt = (g_wow->frametime - g_wow->lastframetime) / 1000000000.f;
	struct vec3f nxt = {0};
	if (unit->worldobj.movement_data.flags & MOVEFLAG_TURN_LEFT)
		worldobj_set_orientation(&unit->worldobj, unit->worldobj.orientation + unit->worldobj.turn_rate * dt);
	if (unit->worldobj.movement_data.flags & MOVEFLAG_TURN_RIGHT)
		worldobj_set_orientation(&unit->worldobj, unit->worldobj.orientation - unit->worldobj.turn_rate * dt);
	if (!(unit->worldobj.movement_data.flags & MOVEFLAG_FALLING))
	{
		if (unit->worldobj.movement_data.flags & MOVEFLAG_FORWARD)
			nxt.z -= 1;
		if (unit->worldobj.movement_data.flags & MOVEFLAG_BACKWARD)
			nxt.z += 1;
		if (unit->worldobj.movement_data.flags & MOVEFLAG_STRAFE_LEFT)
			nxt.x -= 1;
		if (unit->worldobj.movement_data.flags & MOVEFLAG_STRAFE_RIGHT)
			nxt.x += 1;
	}
	struct mat3f mat;
	MAT3_IDENTITY(mat);
	float norm = VEC3_NORM(nxt);
	if (norm != 0)
	{
		struct mat3f tmp;
		VEC3_DIVV(nxt, nxt, norm);
		MAT3_ROTATEY(float, tmp, mat, unit->worldobj.orientation);
		if ((unit->worldobj.movement_data.flags & MOVEFLAG_FLYING)
		 && !(unit->worldobj.movement_data.flags & (MOVEFLAG_ASCENDING | MOVEFLAG_DESCENDING)))
			MAT3_ROTATEX(float, mat, tmp, unit->worldobj.slope);
		else
			mat = tmp;
	}
	if (unit->worldobj.movement_data.flags & MOVEFLAG_FLYING)
	{
		if (unit->worldobj.movement_data.flags & MOVEFLAG_ASCENDING)
			nxt.y += 1;
		if (unit->worldobj.movement_data.flags & MOVEFLAG_DESCENDING)
			nxt.y -= 1;
	}
	else
	{
		if (!(unit->worldobj.movement_data.flags & MOVEFLAG_FLYING)
		 && (unit->worldobj.movement_data.flags & MOVEFLAG_ASCENDING))
		{
			if ((unit->worldobj.movement_data.flags & MOVEFLAG_CAN_FLY)
			 && unit->worldobj.movement_data.fall_time > 400)
			{
				unit->worldobj.movement_data.flags |= MOVEFLAG_FLYING;
				unit->worldobj.movement_data.flags &= ~MOVEFLAG_FALLING;
			}
		}
	}
	float nxt_norm = VEC3_NORM(nxt);
	if (nxt_norm >= EPSILON)
	{
		VEC3_DIVV(nxt, nxt, nxt_norm);
		struct vec3f tmp;
		MAT3_VEC3_MUL(tmp, mat, nxt);
		float step = worldobj_get_speed(&unit->worldobj, unit->worldobj.movement_data.flags & MOVEFLAG_BACKWARD) * dt;
		VEC3_MULV(nxt, tmp, step);
		VEC3_ADD(dst, unit->worldobj.position, nxt);
	}
	else
	{
		dst = unit->worldobj.position;
	}
	return dst;
}

static uint32_t get_flying_animation(struct unit *unit)
{
	if (unit->worldobj.movement_data.flags & MOVEFLAG_STRAFE_RIGHT)
		return ANIM_SWIM_RIGHT;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_STRAFE_LEFT)
		return ANIM_SWIM_LEFT;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_FORWARD)
		return ANIM_SWIM;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_BACKWARD)
		return ANIM_SWIM_BACKWARDS;
	return ANIM_SWIMIDLE;
}

static uint32_t get_standing_animation(struct unit *unit)
{
	if (unit->worldobj.movement_data.flags & MOVEFLAG_FORWARD)
		return ANIM_RUN;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_BACKWARD)
		return ANIM_WALK_BACKWARDS;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_STRAFE_LEFT)
		return ANIM_RUN_LEANLEFT;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_STRAFE_RIGHT)
		return ANIM_RUN_LEANRIGHT;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_TURN_LEFT)
		return ANIM_SHUFFLE_LEFT;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_TURN_RIGHT)
		return ANIM_SHUFFLE_RIGHT;
	return ANIM_STOP;
}

static uint32_t get_falling_animation(struct unit *unit)
{
	if (unit->worldobj.movement_data.fall_time < 1000)
	{
		if (unit->worldobj.movement_data.jump_velocity)
			return ANIM_JUMP; /* XXX JUMPSTART */
		return get_standing_animation(unit);
	}
	if (unit->worldobj.movement_data.fall_time >= 500)
		return ANIM_FALL;
	return ANIM_FALL;
}

static void update_animation(struct unit *unit)
{
	if (!unit->worldobj.m2)
		return;
	uint32_t sequence_id;
	if (unit->worldobj.movement_data.flags & MOVEFLAG_FLYING)
		sequence_id = get_flying_animation(unit);
	else if (unit->worldobj.movement_data.flags & MOVEFLAG_FALLING)
		sequence_id = get_falling_animation(unit);
	else if (unit->worldobj.movement_data.fall_time)
		sequence_id = ANIM_JUMPEND;
	else
		sequence_id = get_standing_animation(unit);
	gx_m2_instance_set_sequence(unit->worldobj.m2, sequence_id);
}

void unit_physics(struct unit *unit)
{
	struct vec3f src = unit->worldobj.position;
	struct vec3f dst = get_next_pos(unit);
	int64_t diff = g_wow->frametime - g_wow->lastframetime;
	float dt = diff / 1000000000.f;
	float gravity = 0;
	if ((g_wow->wow_opt & WOW_OPT_GRAVITY) && unit->worldobj.movement_data.flags & MOVEFLAG_FALLING)
		unit->worldobj.movement_data.fall_time += diff / 1000000;
	if ((g_wow->wow_opt & WOW_OPT_GRAVITY) && !(unit->worldobj.movement_data.flags & MOVEFLAG_FLYING))
	{
		gravity += dt * (GRAVITY * (unit->worldobj.movement_data.fall_time / 1000.) - dt * GRAVITY * .5f);
		if (unit->worldobj.movement_data.flags & MOVEFLAG_FALLING)
		{
			gravity += dt * unit->worldobj.movement_data.jump_velocity;
			dst.z -= dt * unit->worldobj.movement_data.jump_xy_speed * unit->worldobj.movement_data.jump_cos_angle;
			dst.x -= dt * unit->worldobj.movement_data.jump_xy_speed * unit->worldobj.movement_data.jump_sin_angle;
		}
	}
	dst.y -= gravity;
	PERFORMANCE_BEGIN(COLLISIONS);
	struct collision_params params;
	params.wmo_cam = false;
	VEC3_MIN(params.aabb.p0, dst, src);
	VEC3_MAX(params.aabb.p1, dst, src);
	params.aabb.p0.x -= SPHERE_RADIUS;
	params.aabb.p0.y -= SPHERE_RADIUS;
	params.aabb.p0.z -= SPHERE_RADIUS;
	params.aabb.p1.x += SPHERE_RADIUS;
	params.aabb.p1.y += SPHERE_RADIUS;
	params.aabb.p1.z += SPHERE_RADIUS;
	struct vec3f delta;
	VEC3_SUB(delta, params.aabb.p1, params.aabb.p0);
	params.radius = VEC3_NORM(delta);
	VEC3_MULV(params.center, delta, .5);
	VEC3_ADD(params.center, params.center, params.aabb.p0);
	struct jks_array triangles;
	jks_array_init(&triangles, sizeof(struct collision_triangle), NULL, NULL);
	map_collect_collision_triangles(g_wow->map, &params, &triangles);
#ifdef WITH_DEBUG_RENDERING
	if (unit == (struct unit*)g_wow->player)
		gx_collisions_update(&g_wow->cull_frame->gx_collisions, triangles.data, triangles.size);
#endif
	struct vec3f norm;
	VEC3_SET(norm, 0, -1, 0);
	bool ground_touched = false;
	for (size_t i = 0; ; ++i)
	{
#if 0
		LOG_INFO("src: {%f, %f, %f}, dst: {%f, %f, %f}, velocity: {%f, %f, %f}; ground: %d", src.x, src.y, src.z, dst.x, dst.y, dst.z, camera->velocity.x, camera->velocity.y, camera->velocity.z, camera->grounded);
#endif
		struct vec3f tmp = update_position_collision(unit, &triangles, src, dst, 0, &norm, &ground_touched);
		if (!ground_touched)
		{
			if (!(unit->worldobj.movement_data.flags & (MOVEFLAG_FLYING | MOVEFLAG_FALLING)))
			{
				unit->worldobj.movement_data.flags |= MOVEFLAG_FALLING;
				unit->worldobj.movement_data.fall_time = 0;
				unit->worldobj.movement_data.jump_velocity = 0;
				unit->worldobj.movement_data.jump_cos_angle = cos(unit->worldobj.orientation);
				unit->worldobj.movement_data.jump_sin_angle = sin(unit->worldobj.orientation);
				unit->worldobj.movement_data.jump_xy_speed = (unit->worldobj.movement_data.flags & (MOVEFLAG_FORWARD | MOVEFLAG_BACKWARD | MOVEFLAG_STRAFE_LEFT | MOVEFLAG_STRAFE_RIGHT)) ? worldobj_get_speed(&unit->worldobj, unit->worldobj.movement_data.flags & MOVEFLAG_BACKWARD) : 0;
			}
			dst = tmp;
			break;
		}
		if (i == 1)
		{
			dst = tmp;
			break;
		}
		dst.y += gravity;
		unit->worldobj.movement_data.fall_time = 0;
		unit->worldobj.movement_data.flags &= ~MOVEFLAG_FLYING;
		unit->worldobj.movement_data.flags &= ~MOVEFLAG_FALLING;
	}
	if (ground_touched)
	{
		unit->worldobj.slope = 0;
		unit->worldobj.movement_data.flags &= ~MOVEFLAG_FLYING;
	}
	jks_array_destroy(&triangles);
	PERFORMANCE_END(COLLISIONS);
	worldobj_set_position(&unit->worldobj, dst);
	update_animation(unit);
}

void unit_jump(struct unit *unit)
{
	unit->worldobj.movement_data.flags |= MOVEFLAG_FALLING;
	unit->worldobj.movement_data.fall_time = 0;
	unit->worldobj.movement_data.jump_velocity = JUMP_VELOCITY;
	unit->worldobj.movement_data.jump_cos_angle = cos(unit->worldobj.orientation);
	unit->worldobj.movement_data.jump_sin_angle = sin(unit->worldobj.orientation);
	unit->worldobj.movement_data.jump_xy_speed = (unit->worldobj.movement_data.flags & (MOVEFLAG_FORWARD | MOVEFLAG_BACKWARD | MOVEFLAG_STRAFE_LEFT | MOVEFLAG_STRAFE_RIGHT)) ? worldobj_get_speed(&unit->worldobj, unit->worldobj.movement_data.flags & MOVEFLAG_BACKWARD) : 0;
}

const struct worldobj_vtable unit_worldobj_vtable =
{
	.add_to_render = add_to_render,
};

const struct object_vtable unit_object_vtable =
{
	.ctr = ctr,
	.dtr = dtr,
	.on_field_changed = on_field_changed,
};
