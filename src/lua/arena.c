#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_GetArenaTeam(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetArenaTeam()");
	LUA_UNIMPLEMENTED_FN();
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	int id = lua_tointeger(L, 1);
	switch (id)
	{
		case 1: //2v2
			lua_pushstring(L, "On en aggro"); //Name
			lua_pushinteger(L, 2); //Size
			lua_pushinteger(L, 1500); //Rating
			lua_pushinteger(L, 500); //Played
			lua_pushinteger(L, 354); //Wins
			lua_pushinteger(L, 800); //Season played
			lua_pushinteger(L, 752); //Season wins
			lua_pushinteger(L, 500); //Player played
			lua_pushinteger(L, 800); //Player season played
			lua_pushinteger(L, 5); //Rank
			lua_pushinteger(L, 1800); //Player rating
			lua_pushnumber(L, 1); //Background r
			lua_pushnumber(L, 0); //Background g
			lua_pushnumber(L, 0); //Background b
			lua_pushinteger(L, 1); //Emblem
			lua_pushnumber(L, 0); //Emblem r
			lua_pushnumber(L, 1); //Emblem g
			lua_pushnumber(L, 0); //Emblem b
			lua_pushinteger(L, 1); //Border
			lua_pushnumber(L, 0); //Border r
			lua_pushnumber(L, 0); //Border g
			lua_pushnumber(L, 1); //Border b
			return 22;
		case 2: //3v3
			lua_pushstring(L, "On en aggro"); //Name
			lua_pushinteger(L, 3); //Size
			lua_pushinteger(L, 1500); //Rating
			lua_pushinteger(L, 500); //Played
			lua_pushinteger(L, 354); //Wins
			lua_pushinteger(L, 800); //Season played
			lua_pushinteger(L, 752); //Season wins
			lua_pushinteger(L, 500); //Player played
			lua_pushinteger(L, 800); //Player season played
			lua_pushinteger(L, 5); //Rank
			lua_pushinteger(L, 1800); //Player rating
			lua_pushnumber(L, 0); //Background r
			lua_pushnumber(L, 1); //Background g
			lua_pushnumber(L, 0); //Background b
			lua_pushinteger(L, 2); //Emblem
			lua_pushnumber(L, 0); //Emblem r
			lua_pushnumber(L, 0); //Emblem g
			lua_pushnumber(L, 1); //Emblem b
			lua_pushinteger(L, 2); //Border
			lua_pushnumber(L, 1); //Border r
			lua_pushnumber(L, 0); //Border g
			lua_pushnumber(L, 0); //Border b
			return 22;
		case 3: //5v5
			lua_pushstring(L, "On en aggro"); //Name
			lua_pushinteger(L, 5); //Size
			lua_pushinteger(L, 1500); //Rating
			lua_pushinteger(L, 500); //Played
			lua_pushinteger(L, 354); //Wins
			lua_pushinteger(L, 800); //Season played
			lua_pushinteger(L, 752); //Season wins
			lua_pushinteger(L, 500); //Player played
			lua_pushinteger(L, 800); //Player season played
			lua_pushinteger(L, 5); //Rank
			lua_pushinteger(L, 1800); //Player rating
			lua_pushnumber(L, 0); //Background r
			lua_pushnumber(L, 0); //Background g
			lua_pushnumber(L, 1); //Background b
			lua_pushinteger(L, 3); //Emblem
			lua_pushnumber(L, 1); //Emblem r
			lua_pushnumber(L, 0); //Emblem g
			lua_pushnumber(L, 0); //Emblem b
			lua_pushinteger(L, 3); //Border
			lua_pushnumber(L, 0); //Border r
			lua_pushnumber(L, 1); //Border g
			lua_pushnumber(L, 0); //Border b
			return 22;
	}
	LOG_ERROR("unknown arena team id: %d", id);
	return 0;
}

static int luaAPI_GetArenaCurrency(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetArenaCurrency()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1337);
	return 1;
}

static int luaAPI_ArenaTeamRoster(lua_State *L)
{
	LUA_VERBOSE_FN();
	//Query informations from server
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: ArenaTeamRoster(teamID)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetNumArenaTeamMembers(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc < 1 || argc > 2)
		return luaL_error(L, "Usage: GetNumArenaTeamMembers(team, [showOffline])");
	LUA_UNIMPLEMENTED_FN();
	if (!lua_isinteger(L, 1))
		return 0;
	bool show_offline;
	if (argc == 2)
		show_offline = lua_toboolean(L, 2);
	else
		show_offline = 1;
	int id = lua_tointeger(L, 1);
	switch (id)
	{
		case 1: //2v2
			lua_pushinteger(L, 2);
			return 1;
		case 2: //3v3
			lua_pushinteger(L, 3);
			return 1;
		case 3: //5v5
			lua_pushinteger(L, 5);
			return 1;
		default:
			LOG_ERROR("unknown teamid: %d", id);
	}
	return 0;
}

static int luaAPI_GetArenaTeamRosterInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetArenaTeamRosterInfo(team, rosterIdx)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "bsrbsr"); //name
	lua_pushinteger(L, 1); //rank: 0 or 1 ?
	lua_pushinteger(L, 70); //level
	lua_pushstring(L, "test"); //class
	lua_pushboolean(L, true); //online
	lua_pushinteger(L, 77); //played
	lua_pushinteger(L, 50); //win
	lua_pushinteger(L, 67); //seasonPlayed
	lua_pushinteger(L, 48); //seasonWin
	lua_pushinteger(L, 1970); //rating
	return 10;
}

static int selections[3] = {1, 1, 1};

static int luaAPI_GetArenaTeamRosterSelection(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetArenaTeamRosterSelection(teamID)");
	LUA_UNIMPLEMENTED_FN();
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	int team = lua_tointeger(L, 1);
	if (team < 1 || team > 3)
		return luaL_error(L, "Usage: GetArenaTeamRosterSelection(teamID)");
	lua_pushinteger(L, selections[team - 1]);
	return 1;
}

static int luaAPI_SetArenaTeamRosterSelection(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: SetArenaTeamRosterSelection(teamID, selection)");
	LUA_UNIMPLEMENTED_FN();
	if (!lua_isinteger(L, 1))
		return luaL_argerror(L, 1, "integer expected");
	int team = lua_tointeger(L, 1);
	if (team < 1 || team > 3)
		return luaL_error(L, "Usage: SetArenaTeamRosterSelection(teamID, selection)");
	if (!lua_isinteger(L, 2))
		return luaL_argerror(L, 2, "integer expected");
	int selection = lua_tointeger(L, 2);
	selections[team] = selection;
	return 0;
}

static int luaAPI_CloseArenaTeamRoster(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CloseArenaTeamRoster()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_arena_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetArenaTeam);
	LUA_REGISTER_FN(GetArenaCurrency);
	LUA_REGISTER_FN(ArenaTeamRoster);
	LUA_REGISTER_FN(GetNumArenaTeamMembers);
	LUA_REGISTER_FN(GetArenaTeamRosterInfo);
	LUA_REGISTER_FN(GetArenaTeamRosterSelection);
	LUA_REGISTER_FN(SetArenaTeamRosterSelection);
	LUA_REGISTER_FN(CloseArenaTeamRoster);
}
