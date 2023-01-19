#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_HasPetSpells(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: HasPetSpells()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 5);
	lua_pushstring(L, "DEMON"); //type: DEMON or PET
	return 2;
}

static int luaAPI_GetPetHappiness(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetPetHappiness()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 10); //happiness
	lua_pushinteger(L, 10); //damagePercentage
	lua_pushnumber(L, .1); //loyaltyRate
	return 3;
}

static int luaAPI_GetPetActionsUsable(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetPetActionsUsable()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_HasPetUI(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: HasPetUI()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true); //has pet ui
	lua_pushboolean(L, true); //can gain xp
	return 2;
}

static int luaAPI_GetPetActionInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetPetActionInfo(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "test"); //name
	lua_pushstring(L, "test"); //subtext
	lua_pushstring(L, "Interface\\Icons\\Spell_Shadow_SoulGem.blp"); //texture
	lua_pushboolean(L, false); //isToken
	lua_pushboolean(L, false); //isActive
	lua_pushboolean(L, false); //isCastAllowed
	lua_pushboolean(L, false); //autoCastEnabled
	return 7;
}

static int luaAPI_GetPetActionCooldown(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetPetActionCooldown(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1); //start
	lua_pushinteger(L, 5); //duration
	lua_pushinteger(L, 1); //enable
	return 3;
}

static int luaAPI_GetPetFoodTypes(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetPetFoodTypes()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "Meat"); //Meat, Fish, Fruit, Fungus, Bread, Cheese
	return 1;
}

static int luaAPI_GetPetLoyalty(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetPetLoyalty()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "test");
	return 1;
}

static int luaAPI_GetPetExperience(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetPetExperience()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 20);
	lua_pushinteger(L, 50);
	return 2;
}

static int luaAPI_GetPetTrainingPoints(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetPetTrainingPoints()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 20); //total
	lua_pushinteger(L, 5); //spent
	return 2;
}

static int luaAPI_PetHasActionBar(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: PetHasActionBar()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_IsPetAttackActive(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: IsPetAttackActive(id)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, false);
	return 1;
}

void register_pet_functions(lua_State *L)
{
	LUA_REGISTER_FN(HasPetSpells);
	LUA_REGISTER_FN(GetPetHappiness);
	LUA_REGISTER_FN(GetPetActionsUsable);
	LUA_REGISTER_FN(HasPetUI);
	LUA_REGISTER_FN(GetPetActionInfo);
	LUA_REGISTER_FN(GetPetActionCooldown);
	LUA_REGISTER_FN(GetPetFoodTypes);
	LUA_REGISTER_FN(GetPetLoyalty);
	LUA_REGISTER_FN(GetPetExperience);
	LUA_REGISTER_FN(GetPetTrainingPoints);
	LUA_REGISTER_FN(PetHasActionBar);
	LUA_REGISTER_FN(IsPetAttackActive);
}
