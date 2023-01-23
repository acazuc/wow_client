#ifndef UI_ENUM_H
#define UI_ENUM_H

#include "enum_def.h"

#include <jks/optional.h>

#include <stdbool.h>

enum frame_point
{
#define ENUM_DEFINE(val) FRAMEPOINT_##val,
	ENUM_FRAMEPOINT
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_frame_point, enum frame_point);

bool frame_point_from_string(const char *value, enum frame_point *frame_point);
const char *frame_point_to_string(enum frame_point frame_point);

enum frame_strata
{
#define ENUM_DEFINE(val) FRAMESTRATA_##val,
	ENUM_FRAMESTRATA
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_frame_strata, enum frame_strata);

bool frame_strata_from_string(const char *value, enum frame_strata *frame_strata);
const char *frame_strata_to_string(enum frame_strata frame_strata);

enum draw_layer
{
#define ENUM_DEFINE(val) DRAWLAYER_##val,
	ENUM_DRAWLAYER
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_draw_layer, enum draw_layer);

bool draw_layer_from_string(const char *value, enum draw_layer *draw_layer);
const char *draw_layer_to_string(enum draw_layer draw_layer);

enum alpha_mode
{
#define ENUM_DEFINE(val) ALPHA_##val,
	ENUM_ALPHA
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_alpha_mode, enum alpha_mode);

bool alpha_mode_from_string(const char *value, enum alpha_mode *alpha_mode);
const char *alpha_mode_to_string(enum alpha_mode alpha_mode);

enum outline_type
{
#define ENUM_DEFINE(val) OUTLINE_##val,
	ENUM_OUTLINE
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_outline_type, enum outline_type);

bool outline_from_string(const char *value, enum outline_type *outline_type);
const char *outline_to_string(enum outline_type outline_type);

enum justify_v_type
{
#define ENUM_DEFINE(val) JUSTIFYV_##val,
	ENUM_JUSTIFYV
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_justify_v_type, enum justify_v_type);

bool justify_v_from_string(const char *value, enum justify_v_type *type);
const char *justify_v_to_string(enum justify_v_type justify_v);

enum justify_h_type
{
#define ENUM_DEFINE(val) JUSTIFYH_##val,
	ENUM_JUSTIFYH
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_justify_h_type, enum justify_h_type);

bool justify_h_from_string(const char *value, enum justify_h_type *type);
const char *justify_h_to_string(enum justify_h_type justify_h);

enum insert_mode
{
#define ENUM_DEFINE(val) INSERT_##val,
	ENUM_INSERT
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_insert_mode, enum insert_mode);

bool insert_mode_from_string(const char *value, enum insert_mode *insert_mode);
const char *insert_mode_to_string(enum insert_mode insert_mode);

enum orientation
{
#define ENUM_DEFINE(val) ORIENTATION_##val,
	ENUM_ORIENTATION
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_orientation, enum orientation);

bool orientation_from_string(const char *value, enum orientation *orientation);
const char *orientation_to_string(enum orientation orientation);

enum attribute_type
{
#define ENUM_DEFINE(val) ATTRIBUTE_##val,
	ENUM_ATTRIBUTE
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_attribute_type, enum attribute_type);

bool  attribute_type_from_string(const char *value, enum attribute_type *attribute_type);
const char *attribute_type_to_string(enum attribute_type attribute_type);

enum blend_mode
{
#define ENUM_DEFINE(val) BLEND_##val,
	ENUM_BLEND
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_blend_mode, enum blend_mode);

bool blend_mode_from_string(const char *value, enum blend_mode *blend_mode);
const char *blend_mode_to_string(enum blend_mode blend_mode);

enum anchor_point
{
#define ENUM_DEFINE(val) ANCHOR_##val,
	ENUM_ANCHOR
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_anchor_point, enum anchor_point);

bool anchor_point_from_string(const char *value, enum anchor_point *anchor_point);
const char *anchor_point_to_string(enum anchor_point anchor_point);

enum dimension_type
{
	DIMENSION_RELATIVE,
	DIMENSION_ABSOLUTE
};

OPTIONAL_DEF(optional_dimension_type, enum dimension_type);

enum value_type
{
	VALUE_RELATIVE,
	VALUE_ABSOLUTE
};

OPTIONAL_DEF(optional_value_type, enum value_type);

enum inset_type
{
	INSET_RELATIVE,
	INSET_ABSOLUTE
};

OPTIONAL_DEF(optional_inset_type, enum inset_type);

enum event_type
{
#define ENUM_DEFINE(val) EVENT_##val,
	ENUM_EVENT
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_event_type, enum event_type);

bool event_type_from_string(const char *value, enum event_type *type);
const char *event_type_to_string(enum event_type type);

enum inventory_slot
{
	INVSLOT_AMMO = 0,
	INVSLOT_HEAD = 1,
	INVSLOT_NECK = 2,
	INVSLOT_SHOULDER = 3,
	INVSLOT_BODY = 4,
	INVSLOT_CHEST = 5,
	INVSLOT_WAIST = 6,
	INVSLOT_LEGS = 7,
	INVSLOT_FEET = 8,
	INVSLOT_WRIST = 9,
	INVSLOT_HANDS = 10,
	INVSLOT_FINGER0 = 11,
	INVSLOT_FINGER1 = 12,
	INVSLOT_TRINKET0 = 13,
	INVSLOT_TRINKET1 = 14,
	INVSLOT_BACK = 15,
	INVSLOT_MAINHAND = 16,
	INVSLOT_SECONDARYHAND = 17,
	INVSLOT_RANGED = 18,
	INVSLOT_TABARD = 19,
	INVSLOT_BAG0 = 20,
	INVSLOT_BAG1 = 21,
	INVSLOT_BAG2 = 22,
	INVSLOT_BAG3 = 23,
};

OPTIONAL_DEF(optional_inventory_slot, enum inventory_slot);

bool inventory_slot_from_string(const char *value, enum inventory_slot *slot);
const char *inventory_slot_to_string(enum inventory_slot slot);

enum cursor_type
{
#define ENUM_DEFINE(val) CURSOR_##val,
	ENUM_CURSOR
#undef ENUM_DEFINE
};

OPTIONAL_DEF(optional_cursor_type, enum cursor_type);

bool cursor_type_from_string(const char *value, enum cursor_type *type);
const char *cursor_type_to_string(enum cursor_type type);

enum chat_msg
{
#define ENUM_DEFINE(val) CHAT_MSG_##val,
	ENUM_CHAT_MSG
#undef ENUM_DEFINE
	CHAT_MSG_ADDON = 0xFFFFFFFF
};

OPTIONAL_DEF(optional_chat_msg, enum chat_msg);

/* strings with CHAT_MSG_ */
bool chat_msg_from_string(const char *value, enum chat_msg *type);
const char *chat_msg_to_string(enum chat_msg type);

/* strings without CHAT_MSG_ */
bool chat_channel_from_string(const char *value, enum chat_msg *type);
const char *chat_channel_to_string(enum chat_msg type);

enum chat_flag
{
	CHAT_FLAG_NONE = 0x0,
	CHAT_FLAG_AFK  = 0x1,
	CHAT_FLAG_DND  = 0x2,
	CHAT_FLAG_GM   = 0x4,
};

bool chat_flag_from_string(const char *value, enum chat_flag *flag);
const char *chat_flag_to_string(enum chat_flag flag);

#endif
