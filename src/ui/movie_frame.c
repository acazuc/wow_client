#include "movie_frame.h"

#include "xml/movie_frame.h"

#include "wow_lua.h"
#include "log.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_MOVIE_FRAME() LUA_METHOD(MovieFrame, movie_frame)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&movie_frame->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_movie_frame *movie_frame = (struct ui_movie_frame*)object;
	UI_OBJECT->mask |= UI_OBJECT_movie_frame;
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_frame_vtable.dtr(object);
}

static void render(struct ui_object *object)
{
	ui_frame_vtable.render(object);
}

static int lua_EnableSubtitles(lua_State *L)
{
	LUA_METHOD_MOVIE_FRAME();
	if (argc > 2)
		return luaL_error(L, "Usage: MovieFrame:EnableSubtitles(subtitles)");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_StartMovie(lua_State *L)
{
	LUA_METHOD_MOVIE_FRAME();
	if (argc < 2 || argc > 3)
		return luaL_error(L, "Usage: MovieFrame:StartMovie(id [, loop])");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_StopMovie(lua_State *L)
{
	LUA_METHOD_MOVIE_FRAME();
	if (argc != 1)
		return luaL_error(L, "Usage: MovieFrame:StopMovie()");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	UI_REGISTER_METHOD(EnableSubtitles);
	UI_REGISTER_METHOD(StartMovie);
	UI_REGISTER_METHOD(StopMovie);
	return ui_frame_vtable.register_methods(methods);
}

UI_INH1(frame, void, load_xml, const struct xml_layout_frame*, layout_frame);
UI_INH0(frame, void, post_load);
UI_INH0(frame, void, register_in_interface);
UI_INH0(frame, void, unregister_in_interface);
UI_INH0(frame, void, eval_name);
UI_INH1(frame, void, on_click, enum gfx_mouse_button, button);
UI_INH0(frame, float, get_alpha);
UI_INH1(frame, void, set_alpha, float, alpha);
UI_INH1(frame, void, set_hidden, bool, hidden);
UI_INH2(frame, void, get_size, int32_t*, x, int32_t*, y);
UI_INH0(frame, void, set_dirty_coords);
UI_INH1(frame, void, on_mouse_move, gfx_pointer_event_t*, event);
UI_INH1(frame, void, on_mouse_down, gfx_mouse_event_t*, event);
UI_INH1(frame, void, on_mouse_up, gfx_mouse_event_t*, event);
UI_INH1(frame, void, on_mouse_scroll, gfx_scroll_event_t*, event);
UI_INH1(frame, bool, on_key_down, gfx_key_event_t*, event);
UI_INH1(frame, bool, on_key_up, gfx_key_event_t*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_movie_frame_vtable =
{
	UI_OBJECT_VTABLE("MovieFrame")
};
