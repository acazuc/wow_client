#include "functions.h"

#include "itf/interface.h"

#include "ui/region.h"

#include "wow_lua.h"
#include "memory.h"
#include "log.h"
#include "wow.h"

#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

static int luaAPI_strlower(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 1)
		return 0;
	const char *str = lua_tostring(L, 1);
	if (!str)
		return 0;
	size_t len = strlen(str);
	if (!len)
	{
		lua_pushstring(L, "");
		return 1;
	}
	char *ret = mem_malloc(MEM_LUA, len + 1);
	if (!ret)
	{
		LOG_ERROR("failed to allocate new string");
		return 0;
	}
	for (size_t i = 0; i < len; ++i)
		ret[i] = tolower(str[i]);
	ret[len] = '\0';
	lua_pushstring(L, ret);
	mem_free(MEM_LUA, ret);
	return 1;
}

static int luaAPI_strupper(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 1)
		return 0;
	const char *str = lua_tostring(L, 1);
	if (!str)
		return 0;
	size_t len = strlen(str);
	if (!len)
	{
		lua_pushstring(L, "");
		return 1;
	}
	char *ret = mem_malloc(MEM_LUA, len + 1);
	if (!ret)
	{
		LOG_ERROR("failed to allocate new string");
		return 0;
	}
	for (size_t i = 0; i < len; ++i)
		ret[i] = toupper(str[i]);
	ret[len] = '\0';
	lua_pushstring(L, ret);
	mem_free(MEM_LUA, ret);
	return 1;
}

static int luaAPI_strsub(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 2)
		return 0;
	if (!lua_isinteger(L, 2))
		return luaL_argerror(L, 2, "expected integer");
	const char *s = lua_tostring(L, 1);
	if (!s)
		return 0;
	size_t slen = strlen(s);
	int i = lua_tointeger(L, 2);
	if (i < 1)
		i = slen + i;
	else
		i--;
	if (i < 0)
		return luaL_argerror(L, 2, "invalid position < 0");
	if ((size_t)i > slen)
		i = slen;
	if (argc == 3)
	{
		int j = lua_tointeger(L, 3);
		if (j < 1)
			j = slen + j;
		if (j < 0)
			return luaL_argerror(L, 3, "invalid position < 0");
		if ((size_t)j > slen)
			j = slen;
		if (i > j)
		{
			size_t tmp = i;
			i = j;
			j = tmp;
		}
		lua_pushlstring(L, s + i, j - i);
		return 1;
	}
	else
	{
		lua_pushstring(L, s + i);
		return 1;
	}
}

static int luaAPI_strfind(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 2)
		return 0;
#if 1
	lua_getglobal(L, "string");
	lua_pushstring(L, "find");
	lua_gettable(L, -2);
	lua_remove(L, -2);
	for (int i = 0; i < argc; ++i)
		lua_pushvalue(L, -1 - argc);
	lua_call(L, argc, 1);
	return 1;
#else
	const char *s = lua_tostring(L, 1);
	if (!s)
		return 0;
	const char *n = lua_tostring(L, 2);
	if (!n)
		return 0;
	const char *f = strstr(s, n);
	if (!f)
		return 0;
	lua_pushnumber(L, f - s + 1);
	return 1;
#endif
}

static int luaAPI_strrev(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 1)
		return 0;
	const char *s = lua_tostring(L, 1);
	if (!s)
		return 0;
	size_t len = strlen(s);
	char *ret = mem_malloc(MEM_LUA, len + 1);
	if (!ret)
	{
		LOG_ERROR("failed to malloc return");
		return 0;
	}
	for (size_t i = 0; i < len; ++i)
		ret[i] = s[len - 1 - i];
	ret[len] = '\0';
	lua_pushstring(L, ret);
	mem_free(MEM_LUA, ret);
	return 1;
}

static int luaAPI_strlen(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 1)
		return 0;
	const char *s = lua_tostring(L, 1);
	if (!s)
		return 0;
	lua_pushinteger(L, strlen(s));
	return 1;
}

static int luaAPI_gsub(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 3)
	{
		lua_pushstring(L, "");
		return 1;
	}
	const char *a1 = lua_tostring(L, 1);
	const char *a2 = lua_tostring(L, 2);
	const char *a3 = lua_tostring(L, 3);
	if (!a1 || !a2 || !a3)
	{
		lua_pushstring(L, "");
		return 1;
	}
	lua_getglobal(L, "string");
	lua_pushstring(L, "gsub");
	lua_gettable(L, -2);
	lua_remove(L, -2);
	for (int i = 0; i < argc; ++i)
		lua_pushvalue(L, -1 - argc);
	lua_call(L, argc, 1);
	return 1;
}

static int luaAPI_strreplace(lua_State *L)
{
	return luaAPI_gsub(L);
}

static int luaAPI_date(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 1)
	{
		lua_pushstring(L, "");
		return 1;
	}
	const char *fmt = lua_tostring(L, 1);
	if (!fmt)
	{
		lua_pushstring(L, "");
		return 1;
	}
	lua_getglobal(L, "os");
	lua_pushstring(L, "date");
	lua_gettable(L, -2);
	lua_remove(L, -2);
	for (int i = 0; i < argc; ++i)
		lua_pushvalue(L, -1 - argc);
	lua_call(L, argc, 1);
	return 1;
}

static int luaAPI_getglobal(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	const char *key = lua_tostring(L, 1);
	if (!key)
	{
		lua_pushnil(L);
		return 1;
	}
	//LOG_INFO("getting global %s", key);
	lua_getglobal(L, key);
	int type = lua_type(L, -1);
	if (type == LUA_TNIL)
	{
		size_t klen = strlen(key);
		if (klen > 2 && key[klen - 1] == '0' && key[klen - 2] == '.')
		{
			char tmp[256];
			snprintf(tmp, sizeof(tmp), "%.*s", (int)klen - 2, key);
			lua_getglobal(L, tmp);
			type = lua_type(L, -1);
			if (type == LUA_TNIL)
			{
				//LOG_INFO("getglobal(%s) = nil", key);
			}
			else if (type == LUA_TTABLE)
			{
				struct ui_region *region = interface_get_region(g_wow->interface, tmp);
				if (region)
				{
					luaL_getmetatable(L, ((struct ui_object*)region)->vtable->name);
					lua_setmetatable(L, -2);
				}
			}
		}
		else
		{
			//LOG_INFO("getglobal(%s) = nil", key);
		}
	}
	else if (type == LUA_TTABLE)
	{
		struct ui_region *region = interface_get_region(g_wow->interface, key);
		if (region)
		{
			luaL_getmetatable(L, ((struct ui_object*)region)->vtable->name);
			lua_setmetatable(L, -2);
		}
	}
	return 1;
}

static int luaAPI_setglobal(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 2)
		return 0;
	if (!lua_isstring(L, 1))
		return luaL_argerror(L, 1, "expected string");
	const char *key = lua_tostring(L, 1);
	if (!key)
		return luaL_argerror(L, 1, "lua_tostring failed");
	lua_setglobal(L, key);
	return 0;
}

static int luaAPI_tinsert(lua_State *L)
{
	int argc = lua_gettop(L);
	lua_getglobal(L, "table");
	lua_pushstring(L, "insert");
	lua_gettable(L, -2);
	lua_remove(L, -2);
	for (int i = 0; i < argc; ++i)
		lua_pushvalue(L, -1 - argc);
	if (lua_pcall(L, argc, 0, 0))
	{
		const char *err = lua_tostring(L, -1);
		if (err)
			LOG_ERROR("lua error: %s", err);
	}
	return 0;
}

static int luaAPI_getn(lua_State *L)
{
	lua_getglobal(L, "table");
	lua_pushstring(L, "getn");
	lua_gettable(L, -2);
	lua_remove(L, -2);
	lua_pushvalue(L, -2);
	if (lua_pcall(L, 1, 1, 0))
	{
		const char *err = lua_tostring(L, -1);
		if (err)
			LOG_ERROR("lua error: %s", err);
	}
	return 1;
}

#define TO_RAD(n) ((n) * (M_PI / 180.))
#define TO_DEG(n) ((n) * (180. / M_PI))

static int luaAPI_ceil(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushinteger(L, ceil(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_floor(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushinteger(L, floor(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_round(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushinteger(L, round(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_sin(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, sin(TO_RAD(lua_tonumber(L, 1))));
	return 1;
}

static int luaAPI_cos(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, cos(TO_RAD(lua_tonumber(L, 1))));
	return 1;
}

static int luaAPI_tan(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, tan(TO_RAD(lua_tonumber(L, 1))));
	return 1;
}

static int luaAPI_asin(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, TO_DEG(asin(lua_tonumber(L, 1))));
	return 1;
}

static int luaAPI_acos(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, TO_DEG(acos(lua_tonumber(L, 1))));
	return 1;
}

static int luaAPI_atan(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, TO_DEG(atan(lua_tonumber(L, 1))));
	return 1;
}

static int luaAPI_abs(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, fabs(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_deg(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, TO_DEG(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_rad(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, TO_RAD(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_exp(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, exp(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_log(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, log(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_log10(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, log10(lua_tonumber(L, 1)));
	return 1;
}

static int luaAPI_sqrt(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	if (!lua_isnumber(L, 1))
		return luaL_argerror(L, 1, "expected number");
	lua_pushnumber(L, sqrt(lua_tonumber(L, 1)));
	return 1;
}

static void register_alias(lua_State *L, const char *table, const char *key, const char *alias)
{
	lua_getglobal(L, table);
	lua_pushstring(L, key);
	lua_gettable(L, -2);
	lua_setglobal(L, alias);
	lua_pop(L, 1);
}

static int luaAPI_bit_bnot(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1)
		return 0;
	uint32_t n = lua_tointeger(L, 1);
	lua_pushinteger(L, ~n);
	return 1;
}

static int luaAPI_bit_band(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 1)
		return 0;
	uint32_t n = 0;
	for (int i = 1; i <= argc; ++i)
		n &= lua_tointeger(L, i);
	lua_pushinteger(L, ~n);
	return 1;
}

static int luaAPI_bit_bor(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 1)
		return 0;
	uint32_t n = 0;
	for (int i = 1; i <= argc; ++i)
		n |= lua_tointeger(L, i);
	lua_pushinteger(L, n);
	return 1;
}

static int luaAPI_bit_bxor(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc < 1)
		return 0;
	uint32_t n = 0;
	for (int i = 1; i <= argc; ++i)
		n ^= lua_tointeger(L, i);
	lua_pushinteger(L, n);
	return 1;
}

static int luaAPI_bit_lshift(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 2)
		return 0;
	uint32_t n = lua_tointeger(L, 1);
	uint32_t shift = lua_tointeger(L, 2);
	lua_pushinteger(L, n << shift);
	return 1;
}

static int luaAPI_bit_rshift(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 2)
		return 0;
	uint32_t n = lua_tointeger(L, 1);
	uint32_t shift = lua_tointeger(L, 2);
	lua_pushinteger(L, n > shift);
	return 1;
}

static int luaAPI_debuginfo(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: debuginfo()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void lua_format(lua_State *L, int arg_base, char *buffer, size_t buffer_size)
{
	int argc = lua_gettop(L);
	if (argc <= arg_base)
	{
		strcpy(buffer, "");
		return;
	}
	int arg_idx = arg_base + 1;
	const char *fmt = lua_tostring(L, arg_idx++);
	if (!fmt)
	{
		strcpy(buffer, "");
		return;
	}
	buffer[0] = '\0';
	const char *prv = fmt;
	const char *ite;
	while ((ite = strchr(prv, '%')))
	{
		strncat(buffer, prv, ite - prv);
		ite++;
		if (*ite == '%')
		{
			prv = ite + 1;
			strcat(buffer, "%");
			continue;
		}
		int arg;
		if (isdigit(*ite) && ite[1] == '$')
		{
			arg = arg_base + 1 + *ite - '0';
			ite += 2;
		}
		else
		{
			arg = arg_idx++;
		}
		if (*ite == '0')
		{
			//XXX 0 padding
			ite++;
		}
		if (*ite == ' ')
		{
			//XXX space padding
			ite++;
		}
		while (isdigit(*ite))
		{
			//XXX min length
			ite++;
		}
		if (*ite == '.')
		{
			//XXX
			ite++;
			while (isdigit(*ite))
			{
				//XXX max length
				ite++;
			}
		}
		switch (*ite)
		{
			case 'c':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%c", (char)lua_tonumber(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'e':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%e", lua_tonumber(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'E':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%E", lua_tonumber(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'g':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%g", lua_tonumber(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'G':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%G", lua_tonumber(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'f':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%f", lua_tonumber(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'o':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%lo", lua_tointeger(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'd':
			case 'i':
			case 'u':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%ld", lua_tointeger(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'x':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%lx", lua_tointeger(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 'X':
			{
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%lX", lua_tointeger(L, arg));
				strcat(buffer, tmp);
				break;
			}
			case 's':
			{
				const char *str = lua_tostring(L, arg);
				if (!str)
					LOG_WARN("lua_tostring failed");
				else
					strcat(buffer, str);
				break;
			}
			default:
				LOG_WARN("unsupported format: %c", *ite);
				arg_idx++;
				break;
		}
		prv = ite + 1;
	}
	strcat(buffer, prv);
}

static int luaAPI_format(lua_State *L)
{
	char buffer[1024 * 16] = ""; //XXX: alloc ?
	lua_format(L, 0, buffer, sizeof(buffer));
	lua_pushstring(L, buffer);
	return 1;
}

static void registerBitFunctions(lua_State *L)
{
	lua_createtable(L, 0, 0);
	//bnot
	lua_pushstring(L, "bnot");
	lua_pushcfunction(L, luaAPI_bit_bnot);
	lua_settable(L, -3);
	//band
	lua_pushstring(L, "band");
	lua_pushcfunction(L, luaAPI_bit_band);
	lua_settable(L, -3);
	//bor
	lua_pushstring(L, "bor");
	lua_pushcfunction(L, luaAPI_bit_bor);
	lua_settable(L, -3);
	//bxor
	lua_pushstring(L, "bxor");
	lua_pushcfunction(L, luaAPI_bit_bxor);
	lua_settable(L, -3);
	//lshift
	lua_pushstring(L, "lshift");
	lua_pushcfunction(L, luaAPI_bit_lshift);
	lua_settable(L, -3);
	//rshift
	lua_pushstring(L, "rshift");
	lua_pushcfunction(L, luaAPI_bit_rshift);
	lua_settable(L, -3);
	lua_setglobal(L, "bit");
}

void register_std_functions(lua_State *L)
{
	LUA_REGISTER_FN(strlower);
	LUA_REGISTER_FN(strupper);
	LUA_REGISTER_FN(strsub);
	LUA_REGISTER_FN(strfind);
	LUA_REGISTER_FN(strrev);
	LUA_REGISTER_FN(strlen);
	LUA_REGISTER_FN(gsub);
	LUA_REGISTER_FN(strreplace);
	register_alias(L, "string", "match", "strmatch");

	LUA_REGISTER_FN(getglobal);
	LUA_REGISTER_FN(setglobal);
	LUA_REGISTER_FN(tinsert);
	LUA_REGISTER_FN(getn);
	register_alias(L, "table", "remove", "tremove");
	register_alias(L, "table", "sort", "sort");

	register_alias(L, "math", "fmod", "mod");
	register_alias(L, "math", "max", "max");
	register_alias(L, "math", "min", "min");

	register_alias(L, "debug", "traceback", "debugstack");

	LUA_REGISTER_FN(ceil);
	LUA_REGISTER_FN(floor);
	LUA_REGISTER_FN(round);
	LUA_REGISTER_FN(sin);
	LUA_REGISTER_FN(cos);
	LUA_REGISTER_FN(tan);
	LUA_REGISTER_FN(asin);
	LUA_REGISTER_FN(acos);
	LUA_REGISTER_FN(atan);
	LUA_REGISTER_FN(abs);
	LUA_REGISTER_FN(deg);
	LUA_REGISTER_FN(rad);
	LUA_REGISTER_FN(exp);
	LUA_REGISTER_FN(log);
	LUA_REGISTER_FN(log10);
	LUA_REGISTER_FN(sqrt);

	registerBitFunctions(L);

	LUA_REGISTER_FN(debuginfo);
	LUA_REGISTER_FN(format);
	LUA_REGISTER_FN(date);
}
