#include "functions.h"

#include "snd/snd.h"

#include "wow_lua.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

#include <string.h>

static int luaAPI_PlaySound(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: PlaySound(\"sound\")");
	const char *sound = lua_tostring(L, 1);
	if (!sound)
		return luaL_argerror(L, 1, "failed to get string");
	char path[512] = "";
	wow_dbc_row_t row;
	if (dbc_get_row_indexed_str(g_wow->dbc.sound_entries, &row, sound))
	{
		const char *directory = wow_dbc_get_str(&row, 92);
		const char *file = wow_dbc_get_str(&row, 12);
		if (directory && file)
			snprintf(path, sizeof(path), "%s/%s", directory, file);
	}
	normalize_mpq_filename(path, sizeof(path));
	if (!path[0])
	{
		LOG_WARN("sound not found: %s", sound);
		return 0;
	}
	snd_play_sound(g_wow->snd, path);
	return 0;
}

static int luaAPI_PlayCreditsMusic(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: PlayCreditsMusic(\"music\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_PlayGlueMusic(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: PlayGlueMusic(\"music\")");
	const char *music = lua_tostring(L, 1);
	if (!music)
		return luaL_argerror(L, 1, "failed to get string");
	char path[512] = "";
	wow_dbc_row_t row;
	if (dbc_get_row_indexed_str(g_wow->dbc.sound_entries, &row, music))
	{
		const char *directory = wow_dbc_get_str(&row, 92);
		const char *file = wow_dbc_get_str(&row, 12);
		if (directory && file)
			snprintf(path, sizeof(path), "%s/%s", directory, file);
	}
	normalize_mpq_filename(path, sizeof(path));
	if (!path[0])
	{
		LOG_WARN("glue music not found: %s", music);
		return 0;
	}
	snd_set_glue_music(g_wow->snd, path);
	return 0;
}

static int luaAPI_StopGlueMusic(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: StopGlueMusic()");
	snd_set_glue_music(g_wow->snd, NULL);
	return 0;
}

static int luaAPI_PlayGlueAmbience(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc < 1 || argc > 2)
		return luaL_error(L, "Usage: PlayGlueAmbience(\"ambience\" [, fadeInTime])");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_StopGlueAmbience(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: StopGlueAmbience()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_Sound_GameSystem_GetNumOutputDrivers(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: Sound_GameSystem_GetNumOutputDrivers()");
	lua_pushinteger(L, snd_get_output_dev_count(g_wow->snd));
	return 1;
}

static int luaAPI_Sound_ChatSystem_GetNumOutputDrivers(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: Sound_ChatSystem_GetNumOutputDrivers()");
	lua_pushinteger(L, snd_get_output_dev_count(g_wow->snd));
	return 1;
}

static int luaAPI_Sound_GameSystem_GetNumInputDrivers(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: Sound_GameSystem_GetNumInputDrivers()");
	lua_pushinteger(L, snd_get_input_dev_count(g_wow->snd));
	return 1;
}

static int luaAPI_Sound_ChatSystem_GetNumInputDrivers(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: Sound_ChatSystem_GetNumInputDrivers()");
	lua_pushinteger(L, snd_get_input_dev_count(g_wow->snd));
	return 1;
}

static int luaAPI_Sound_GameSystem_GetOutputDriverNameByIndex(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: Sound_GameSystem_GetOutputDriverNameByIndex(index)");
	int idx = lua_tointeger(L, 1);
	const PaDeviceInfo *dev = snd_get_output_dev(g_wow->snd, idx);
	if (!dev)
		return 0;
	lua_pushstring(L, dev->name);
	return 1;
}

static int luaAPI_Sound_ChatSystem_GetOutputDriverNameByIndex(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: Sound_ChatSystem_GetOutputDriverNameByIndex(index)");
	int idx = lua_tointeger(L, 1);
	const PaDeviceInfo *dev = snd_get_output_dev(g_wow->snd, idx);
	if (!dev)
		return 0;
	lua_pushstring(L, dev->name);
	return 1;
}

static int luaAPI_Sound_GameSystem_GetInputDriverNameByIndex(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: Sound_GameSystem_GetInputDriverNameByIndex(index)");
	int idx = lua_tointeger(L, 1);
	const PaDeviceInfo *dev = snd_get_input_dev(g_wow->snd, idx);
	if (!dev)
		return 0;
	lua_pushstring(L, dev->name);
	return 1;
}

static int luaAPI_Sound_ChatSystem_GetInputDriverNameByIndex(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: Sound_ChatSystem_GetInputDriverNameByIndex(index)");
	int idx = lua_tointeger(L, 1);
	const PaDeviceInfo *dev = snd_get_input_dev(g_wow->snd, idx);
	if (!dev)
		return 0;
	lua_pushstring(L, dev->name);
	return 1;
}

static int luaAPI_Sound_GameSystem_RestartSoundSystem(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: Sound_GameSystem_RestartSoundSystem()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_Sound_RestartSoundEngine(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: Sound_RestartSoundEngine()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_audio_functions(lua_State *L)
{
	LUA_REGISTER_FN(PlaySound);
	LUA_REGISTER_FN(PlayCreditsMusic);
	LUA_REGISTER_FN(PlayGlueMusic);
	LUA_REGISTER_FN(StopGlueMusic);
	LUA_REGISTER_FN(PlayGlueAmbience);
	LUA_REGISTER_FN(StopGlueAmbience);
	LUA_REGISTER_FN(Sound_GameSystem_GetNumOutputDrivers);
	LUA_REGISTER_FN(Sound_ChatSystem_GetNumOutputDrivers);
	LUA_REGISTER_FN(Sound_GameSystem_GetNumInputDrivers);
	LUA_REGISTER_FN(Sound_ChatSystem_GetNumInputDrivers);
	LUA_REGISTER_FN(Sound_GameSystem_GetOutputDriverNameByIndex);
	LUA_REGISTER_FN(Sound_ChatSystem_GetOutputDriverNameByIndex);
	LUA_REGISTER_FN(Sound_GameSystem_GetInputDriverNameByIndex);
	LUA_REGISTER_FN(Sound_ChatSystem_GetInputDriverNameByIndex);
	LUA_REGISTER_FN(Sound_GameSystem_RestartSoundSystem);
	LUA_REGISTER_FN(Sound_RestartSoundEngine);
}
