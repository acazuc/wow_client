#ifndef UNIT_H
#define UNIT_H

#include "obj/worldobj.h"

struct wow_blp_file;
struct blp_texture;
struct gx_text;
struct object;
struct unit;

struct unit_vtable
{
	void (*get_visual_settings)(const struct object *object, uint8_t *face, uint8_t *hair_color, uint8_t *hair_style, uint8_t *skin_color, uint8_t *facial_hair);
};

struct unit_item
{
	uint32_t id;
	uint16_t attachment_left;
	uint16_t attachment_right;
	struct gx_m2_instance *m2_left;
	struct gx_m2_instance *m2_right;
	struct blp_texture *tex_left;
	struct blp_texture *tex_right;
};

enum unit_item_type
{
	UNIT_ITEM_HELM,
	UNIT_ITEM_SHOULDER,
	UNIT_ITEM_SHIRT,
	UNIT_ITEM_CHEST,
	UNIT_ITEM_BELT,
	UNIT_ITEM_LEGS,
	UNIT_ITEM_BOOTS,
	UNIT_ITEM_WRISTS,
	UNIT_ITEM_GLOVES,
	UNIT_ITEM_TABARD,
	UNIT_ITEM_CAPE,
	UNIT_ITEM_MAINHAND,
	UNIT_ITEM_OFFHAND,
	UNIT_ITEM_LAST
};

struct unit
{
	struct worldobj worldobj;
	const struct unit_vtable *unit_vtable;
	struct unit_item items[UNIT_ITEM_LAST];
	struct gx_m2_instance *mount_m2;
	struct gx_text *text;
	float scale;
};

void unit_get_skin_textures_files(const struct unit *unit, struct wow_blp_file **skin, struct wow_blp_file **skin_extra);
void unit_get_face_textures_files(const struct unit *unit, struct wow_blp_file **lower, struct wow_blp_file **upper);
void unit_get_facial_hair_textures_files(const struct unit *unit, struct wow_blp_file **lower, struct wow_blp_file **upper);
void unit_get_hair_textures_files(const struct unit *unit, struct wow_blp_file **hair, struct wow_blp_file **lower, struct wow_blp_file **upper);
void unit_get_underwear_textures_files(const struct unit *unit, struct wow_blp_file **lower, struct wow_blp_file **upper);
void unit_get_skin_textures(const struct unit *unit, struct blp_texture **skin, struct blp_texture **skin_extra);
void unit_get_face_textures(const struct unit *unit, struct blp_texture **lower, struct blp_texture **upper);
void unit_get_facial_hair_textures(const struct unit *unit, struct blp_texture **lower, struct blp_texture **upper);
void unit_get_hair_textures(const struct unit *unit, struct blp_texture **hair, struct blp_texture **lower, struct blp_texture **upper);
void unit_get_underwear_textures(const struct unit *unit, struct blp_texture **lower, struct blp_texture **upper);
uint8_t unit_get_class(const struct unit *unit);
uint8_t unit_get_race(const struct unit *unit);
uint8_t unit_get_gender(const struct unit *unit);
void unit_get_visual_settings(const struct unit *unit, uint8_t *face, uint8_t *hair_color, uint8_t *hair_style, uint8_t *skin_color, uint8_t *facial_hair);
void unit_update_items_batches(struct object *object, const uint8_t *hair_style, const uint8_t *facial_hair);
void unit_update_items(struct object *object);
void unit_physics(struct unit *unit);
void unit_jump(struct unit *unit);

extern const struct worldobj_vtable unit_worldobj_vtable;
extern const struct object_vtable unit_object_vtable;

#endif
