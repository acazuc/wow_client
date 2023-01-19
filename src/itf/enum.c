#include "enum.h"

#include "log.h"

#include <string.h>

#define ENUM_DEFINE(val) #val,

static inline bool enum_from_string(const char *value, const char **values, size_t values_size, int *ret)
{
	for (size_t i = 0; i < values_size; ++i)
	{
		if (!strcmp(values[i], value))
		{
			*ret = i;
			return true;
		}
	}
	return false;
}

static inline const char *enum_to_string(size_t value, const char **values, size_t values_size)
{
	if (value >= values_size)
		return NULL;
	return values[value];
}

static const char *frame_points[] =
{
	ENUM_FRAMEPOINT
};

bool frame_point_from_string(const char *value, enum frame_point *frame_point)
{
	return enum_from_string(value, frame_points, sizeof(frame_points) / sizeof(*frame_points), (int*)frame_point);
}

const char *frame_point_to_string(enum frame_point frame_point)
{
	return enum_to_string(frame_point, frame_points, sizeof(frame_points) / sizeof(*frame_points));
}

static const char *frame_stratas[] =
{
	ENUM_FRAMESTRATA
};

bool frame_strata_from_string(const char *value, enum frame_strata *frame_strata)
{
	return enum_from_string(value, frame_stratas, sizeof(frame_stratas) / sizeof(*frame_stratas), (int*)frame_strata);
}

const char *frame_strata_to_string(enum frame_strata frame_strata)
{
	return enum_to_string(frame_strata, frame_stratas, sizeof(frame_stratas) / sizeof(*frame_stratas));
}

static const char *draw_layers[] =
{
	ENUM_DRAWLAYER
};

bool draw_layer_from_string(const char *value, enum draw_layer *draw_layer)
{
	return enum_from_string(value, draw_layers, sizeof(draw_layers) / sizeof(*draw_layers), (int*)draw_layer);
}

const char *draw_layer_to_string(enum draw_layer draw_layer)
{
	return enum_to_string(draw_layer, draw_layers, sizeof(draw_layers) / sizeof(*draw_layers));
}

static const char *alpha_modes[] =
{
	ENUM_ALPHA
};

bool alpha_mode_from_string(const char *value, enum alpha_mode *alpha_mode)
{
	return enum_from_string(value, alpha_modes, sizeof(alpha_modes) / sizeof(*alpha_modes), (int*)alpha_mode);
}

const char *alpha_mode_to_string(enum alpha_mode alpha_mode)
{
	return enum_to_string(alpha_mode, alpha_modes, sizeof(alpha_modes) / sizeof(*alpha_modes));
}

static const char *outline_types[] =
{
	ENUM_OUTLINE
};

bool outline_from_string(const char *value, enum outline_type *outline_type)
{
	return enum_from_string(value, outline_types, sizeof(outline_types) / sizeof(*outline_types), (int*)outline_type);
}

const char *outline_to_string(enum outline_type outline_type)
{
	return enum_to_string(outline_type, outline_types, sizeof(outline_types) / sizeof(*outline_types));
}

static const char *justify_v_types[] =
{
	ENUM_JUSTIFYV
};

bool justify_v_from_string(const char *value, enum justify_v_type *type)
{
	return enum_from_string(value, justify_v_types, sizeof(justify_v_types) / sizeof(*justify_v_types), (int*)type);
}

const char *justify_v_to_string(enum justify_v_type justify_v)
{
	return enum_to_string(justify_v, justify_v_types, sizeof(justify_v_types) / sizeof(*justify_v_types));
}

static const char *justify_h_types[] =
{
	ENUM_JUSTIFYH
};

bool justify_h_from_string(const char *value, enum justify_h_type *type)
{
	return enum_from_string(value, justify_h_types, sizeof(justify_h_types) / sizeof(*justify_h_types), (int*)type);
}

const char *justify_h_to_string(enum justify_h_type justify_h)
{
	return enum_to_string(justify_h, justify_h_types, sizeof(justify_h_types) / sizeof(*justify_h_types));
}

static const char *insert_modes[] =
{
	ENUM_INSERT
};

bool insert_mode_from_string(const char *value, enum insert_mode *insert_mode)
{
	return enum_from_string(value, insert_modes, sizeof(insert_modes) / sizeof(*insert_modes), (int*)insert_mode);
}

const char *insert_mode_to_string(enum insert_mode insert_mode)
{
	return enum_to_string(insert_mode, insert_modes, sizeof(insert_modes) / sizeof(*insert_modes));
}

static const char *orientations[] =
{
	ENUM_ORIENTATION
};

bool orientation_from_string(const char *value, enum orientation *orientation)
{
	return enum_from_string(value, orientations, sizeof(orientations) / sizeof(*orientations), (int*)orientation);
}

const char *orientation_to_string(enum orientation orientation)
{
	return enum_to_string(orientation, orientations, sizeof(orientations) / sizeof(*orientations));
}

static const char *attribute_types[] =
{
	ENUM_ATTRIBUTE
};

bool attribute_type_from_string(const char *value, enum attribute_type *attribute_type)
{
	return enum_from_string(value, attribute_types, sizeof(attribute_types) / sizeof(*attribute_types), (int*)attribute_type);
}

const char *attribute_type_to_string(enum attribute_type attribute_type)
{
	return enum_to_string(attribute_type, attribute_types, sizeof(attribute_types) / sizeof(*attribute_types));
}

static const char *blend_modes[] =
{
	ENUM_BLEND
};

bool blend_mode_from_string(const char *value, enum blend_mode *blend_mode)
{
	return enum_from_string(value, blend_modes, sizeof(blend_modes) / sizeof(*blend_modes), (int*)blend_mode);
}

const char *blend_mode_to_string(enum blend_mode blend_mode)
{
	return enum_to_string(blend_mode, blend_modes, sizeof(blend_modes) / sizeof(*blend_modes));
}

static const char *anchor_points[] =
{
	ENUM_ANCHOR
};

bool anchor_point_from_string(const char *value, enum anchor_point *anchor_point)
{
	return enum_from_string(value, anchor_points, sizeof(anchor_points) / sizeof(*anchor_points), (int*)anchor_point);
}

const char *anchor_point_to_string(enum anchor_point anchor_point)
{
	return enum_to_string(anchor_point, anchor_points, sizeof(anchor_points) / sizeof(*anchor_points));
}

static const char *event_types[] =
{
	ENUM_EVENT
};

bool event_type_from_string(const char *value, enum event_type *type)
{
	return enum_from_string(value, event_types, sizeof(event_types) / sizeof(*event_types), (int*)type);
}

const char *event_type_to_string(enum event_type type)
{
	return enum_to_string(type, event_types, sizeof(event_types) / sizeof(*event_types));
}

#undef ENUM_DEFINE

bool inventory_slot_from_string(const char *value, enum inventory_slot *slot)
{
#define DO_TEST(str, en) \
	if (!strcmp(value, str)) \
	{ \
		*slot = en; \
		return true; \
	}
	DO_TEST("AMMOSLOT", INVSLOT_AMMO);
	DO_TEST("HEADSLOT", INVSLOT_HEAD);
	DO_TEST("NECKSLOT", INVSLOT_NECK);
	DO_TEST("SHOULDERSLOT", INVSLOT_SHOULDER);
	DO_TEST("SHIRTSLOT", INVSLOT_BODY);
	DO_TEST("CHESTSLOT", INVSLOT_CHEST);
	DO_TEST("WAISTSLOT", INVSLOT_WAIST);
	DO_TEST("LEGSSLOT", INVSLOT_LEGS);
	DO_TEST("FEETSLOT", INVSLOT_FEET);
	DO_TEST("WRISTSLOT", INVSLOT_WRIST);
	DO_TEST("HANDSSLOT", INVSLOT_HANDS);
	DO_TEST("FINGER0SLOT", INVSLOT_FINGER0);
	DO_TEST("FINGER1SLOT", INVSLOT_FINGER1);
	DO_TEST("TRINKET0SLOT", INVSLOT_TRINKET0);
	DO_TEST("TRINKET1SLOT", INVSLOT_TRINKET1);
	DO_TEST("BACKSLOT", INVSLOT_BACK);
	DO_TEST("MAINHANDSLOT", INVSLOT_MAINHAND);
	DO_TEST("SECONDARYHANDSLOT", INVSLOT_SECONDARYHAND);
	DO_TEST("RANGEDSLOT", INVSLOT_RANGED);
	DO_TEST("TABARDSLOT", INVSLOT_TABARD);
	DO_TEST("BAG0SLOT", INVSLOT_BAG0);
	DO_TEST("BAG1SLOT", INVSLOT_BAG1);
	DO_TEST("BAG2SLOT", INVSLOT_BAG2);
	DO_TEST("BAG3SLOT", INVSLOT_BAG3);
	return false;
#undef DO_TEST
}

const char *inventory_slot_to_string(enum inventory_slot slot)
{
#define DO_TEST(str, en) \
	case en: \
		return str;
	switch (slot)
	{
	DO_TEST("AMMOSLOT", INVSLOT_AMMO);
	DO_TEST("HEADSLOT", INVSLOT_HEAD);
	DO_TEST("NECKSLOT", INVSLOT_NECK);
	DO_TEST("SHOULDERSLOT", INVSLOT_SHOULDER);
	DO_TEST("SHIRTSLOT", INVSLOT_BODY);
	DO_TEST("CHESTSLOT", INVSLOT_CHEST);
	DO_TEST("WAISTSLOT", INVSLOT_WAIST);
	DO_TEST("LEGSSLOT", INVSLOT_LEGS);
	DO_TEST("FEETSLOT", INVSLOT_FEET);
	DO_TEST("WRISTSLOT", INVSLOT_WRIST);
	DO_TEST("HANDSSLOT", INVSLOT_HANDS);
	DO_TEST("FINGER1SLOT", INVSLOT_FINGER0);
	DO_TEST("FINGER2SLOT", INVSLOT_FINGER1);
	DO_TEST("TRINKET0SLOT", INVSLOT_TRINKET0);
	DO_TEST("TRINKET1SLOT", INVSLOT_TRINKET1);
	DO_TEST("BACKSLOT", INVSLOT_BACK);
	DO_TEST("MAINHANDSLOT", INVSLOT_MAINHAND);
	DO_TEST("SECONDARYHANDSLOT", INVSLOT_SECONDARYHAND);
	DO_TEST("RANGEDSLOT", INVSLOT_RANGED);
	DO_TEST("TABARDSLOT", INVSLOT_TABARD);
	DO_TEST("BAG0SLOT", INVSLOT_BAG0);
	DO_TEST("BAG1SLOT", INVSLOT_BAG1);
	DO_TEST("BAG2SLOT", INVSLOT_BAG2);
	DO_TEST("BAG3SLOT", INVSLOT_BAG3);
		default:
	return "";
	}
	return "";
#undef DO_TEST
}

static const char *cursor_types[] =
{
#define ENUM_DEFINE(val) #val "_CURSOR",
	ENUM_CURSOR
#undef ENUM_DEFINE
};

bool cursor_type_from_string(const char *value, enum cursor_type *type)
{
	return enum_from_string(value, cursor_types, sizeof(cursor_types) / sizeof(*cursor_types), (int*)type);
}

const char *cursor_type_to_string(enum cursor_type type)
{
	return enum_to_string(type, cursor_types, sizeof(cursor_types) / sizeof(*cursor_types));
}

