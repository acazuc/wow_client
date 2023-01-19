#include "functions.h"

#include "itf/interface.h"

#include "wow_lua.h"
#include "log.h"
#include "wow.h"

static const char *g_bindings[] =
{
	"HEADER_MOVEMENT", /* movement */
	"MOVEANDSTEER",
	"MOVEFORWARD",
	"MOVEBACKWARD",
	"TURNLEFT",
	"TURNRIGHT",
	"STRAFELEFT",
	"STRAFERIGHT",
	"JUMP",
	"SITORSTAND",
	"TOGGLESHEATH",
	"TOGGLEAUTORUN",
	"PITCHUP"
	"PITCHDOWN",
	"TOGGLERUN",
	"FOLLOWTARGET",
	"HEADER_CHAT", /* chat */
	"OPENCHAT",
	"OPENCHATSLASH",
	"CHATPAGEUP",
	"CHATPAGEDOWN",
	"CHATBOTTOM",
	"REPLY",
	"REPLY2",
	"COMBATLOGPAGEUP",
	"COMBATLOGPAGEDOWN",
	"COMBATLOGBOTTOM",
	"TOGGLECOMBATLOG",
	"HEADER_ACTIONBAR", /* action bars */
	"ACTIONBUTTON1",
	"ACTIONBUTTON2",
	"ACTIONBUTTON3",
	"ACTIONBUTTON4",
	"ACTIONBUTTON5",
	"ACTIONBUTTON6",
	"ACTIONBUTTON7",
	"ACTIONBUTTON8",
	"ACTIONBUTTON9",
	"ACTIONBUTTON10",
	"ACTIONBUTTON11",
	"ACTIONBUTTON12",
	"SHAPESHIFTBUTTON1",
	"SHAPESHIFTBUTTON2",
	"SHAPESHIFTBUTTON3",
	"SHAPESHIFTBUTTON4",
	"SHAPESHIFTBUTTON5",
	"SHAPESHIFTBUTTON6",
	"SHAPESHIFTBUTTON7",
	"SHAPESHIFTBUTTON8",
	"SHAPESHIFTBUTTON9",
	"SHAPESHIFTBUTTON10",
	"BONUSACTIONBUTTON1",
	"BONUSACTIONBUTTON2",
	"BONUSACTIONBUTTON3",
	"BONUSACTIONBUTTON4",
	"BONUSACTIONBUTTON5",
	"BONUSACTIONBUTTON6",
	"BONUSACTIONBUTTON7",
	"BONUSACTIONBUTTON8",
	"BONUSACTIONBUTTON9",
	"ACTIONPAGE1",
	"ACTIONPAGE2",
	"ACTIONPAGE3",
	"ACTIONPAGE4",
	"ACTIONPAGE5",
	"ACTIONPAGE6",
	"PREVIOUSACTIONPAGE",
	"NEXTACTIONPAGE",
	"TOGGLEACTIONBARLOCK",
	"TOGGLEAUTOSELFCAST",
	"HEADER_TARGETING", /* targeting */
	"TARGETNEARESTENEMY",
	"TARGETPREVIOUSENEMY",
	"TARGETNEARESTFRIEND",
	"TARGETPREVIOUSFRIEND",
	"TARGETNEARESTENEMYPLAYER",
	"TARGETPREVIOUSENEMYPLAYER",
	"TARGETNEARESTFRIENDPLAYER",
	"TARGETPREVIOUSFRIENDPLAYER",
	"TARGETSELF",
	"TARGETPARTYMEMBER1",
	"TARGETPARTYMEMBER2",
	"TARGETPARTYMEMBER3",
	"TARGETPARTYMEMBER4",
	"TARGETPET",
	"TARGETPARTYPET1",
	"TARGETPARTYPET2",
	"TARGETPARTYPET3",
	"TARGETPARTYPET4",
	"TARGETLASTHOSTILE",
	"TARGETLASTTARGET",
	"ASSISTTARGET",
	"NAMEPLATES",
	"FRIENDNAMEPLATES",
	"ALLNAMEPLATES",
	"ATTACKTARGET",
	"PETATTACK",
	"FOCUSTARGET",
	"TARGETFOCUS",
	"TARGETTALKER",
	"HEADER_INTERFACE", /* interface */
	"TOGGLECHARACTER0",
	"TOGGLEBACKPACK",
	"TOGGLEBAG1",
	"TOGGLEBAG2",
	"TOGGLEBAG3",
	"TOGGLEBAG4",
	"OPENALLBAGS",
	"TOGGLEKEYRING",
	"TOGGLESPELLBOOK",
	"TOGGLEPETBOOK",
	"TOGGLETALENTS",
	"TOGGLECHARACTER4",
	"TOGGLECHARACTER3",
	"TOGGLECHARACTER2",
	"TOGGLECHARACTER1",
	"TOGGLEQUESTLOG",
	"TOGGLEGAMEMENU",
	"TOGGLEMINIMAP",
	"TOGGLEWORLDMAP",
	"TOGGLESOCIAL",
	"TOGGLEFRIENDSTAB",
	"TOGGLEWHOTAB",
	"TOGGLEGUILDTAB",
	"TOGGLECHATTAB",
	"TOGGLERAIDTAB",
	"TOGGLELFGPARENT",
	"TOGGLELFGTAB",
	"TOGGLELFMTAB",
	"TOGGLEWORLDSTATESCORES",
	"TOGGLEBATTLEFIELDMINIMAP",
	"TOGGLEMINIMAPROTATION",
	"TOGGLECHANNELPULLOUT",
	"HEADER_MISC", /* misc */
	"MINIMAPZOOMIN",
	"MINIMAPZOOMOUT",
	"TOGGLEMUSIC",
	"TOGGLESOUND",
	"MASTERVOLUMEUP",
	"MASTERVOLUMEDOWN",
	"TOGGLESELFMUTE",
	"TOGGLEUI",
	"SCREENSHOT",
	"HEADER_CAMERA", /* camera */
	"NEXTVIEW",
	"PREVVIEW",
	"CAMERAZOOMIN",
	"CAMERAZOOMOUT",
	"SETVIEW1",
	"SETVIEW2",
	"SETVIEW3",
	"SETVIEW4",
	"SETVIEW5",
	"SAVEVIEW2",
	"SAVEVIEW3",
	"SAVEVIEW4",
	"SAVEVIEW5",
	"RESETVIEW2",
	"RESETVIEW3",
	"RESETVIEW4",
	"RESETVIEW5",
	"FLIPCAMERAYAW",
	"HEADER_MULTIACTIONBAR", /* multi action bar */
	"MULTIACTIONBAR1BUTTON1",
	"MULTIACTIONBAR1BUTTON2",
	"MULTIACTIONBAR1BUTTON3",
	"MULTIACTIONBAR1BUTTON4",
	"MULTIACTIONBAR1BUTTON5",
	"MULTIACTIONBAR1BUTTON6",
	"MULTIACTIONBAR1BUTTON7",
	"MULTIACTIONBAR1BUTTON8",
	"MULTIACTIONBAR1BUTTON9",
	"MULTIACTIONBAR1BUTTON10",
	"MULTIACTIONBAR1BUTTON11",
	"MULTIACTIONBAR1BUTTON12",
	"MULTIACTIONBAR2BUTTON1",
	"MULTIACTIONBAR2BUTTON2",
	"MULTIACTIONBAR2BUTTON3",
	"MULTIACTIONBAR2BUTTON4",
	"MULTIACTIONBAR2BUTTON5",
	"MULTIACTIONBAR2BUTTON6",
	"MULTIACTIONBAR2BUTTON7",
	"MULTIACTIONBAR2BUTTON8",
	"MULTIACTIONBAR2BUTTON9",
	"MULTIACTIONBAR2BUTTON10",
	"MULTIACTIONBAR2BUTTON11",
	"MULTIACTIONBAR2BUTTON12",
	"MULTIACTIONBAR3BUTTON1",
	"MULTIACTIONBAR3BUTTON2",
	"MULTIACTIONBAR3BUTTON3",
	"MULTIACTIONBAR3BUTTON4",
	"MULTIACTIONBAR3BUTTON5",
	"MULTIACTIONBAR3BUTTON6",
	"MULTIACTIONBAR3BUTTON7",
	"MULTIACTIONBAR3BUTTON8",
	"MULTIACTIONBAR3BUTTON9",
	"MULTIACTIONBAR3BUTTON10",
	"MULTIACTIONBAR3BUTTON11",
	"MULTIACTIONBAR3BUTTON12",
	"MULTIACTIONBAR4BUTTON1",
	"MULTIACTIONBAR4BUTTON2",
	"MULTIACTIONBAR4BUTTON3",
	"MULTIACTIONBAR4BUTTON4",
	"MULTIACTIONBAR4BUTTON5",
	"MULTIACTIONBAR4BUTTON6",
	"MULTIACTIONBAR4BUTTON7",
	"MULTIACTIONBAR4BUTTON8",
	"MULTIACTIONBAR4BUTTON9",
	"MULTIACTIONBAR4BUTTON10",
	"MULTIACTIONBAR4BUTTON11",
	"MULTIACTIONBAR4BUTTON12",
	"HEADER_RAID_TARGET", /* raid target */
	"RAIDTARGET1",
	"RAIDTARGET2",
	"RAIDTARGET3",
	"RAIDTARGET4",
	"RAIDTARGET5",
	"RAIDTARGET6",
	"RAIDTARGET7",
	"RAIDTARGET8",
	"RAIDTARGETNONE",
};

static int luaAPI_GetNumBindings(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumBindings()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, sizeof(g_bindings) / sizeof(*g_bindings));
	return 1;
}

static int luaAPI_GetBinding(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetBinding(index)");
	int index = lua_tointeger(L, 1);
	if (index <= 0 || (size_t)index > sizeof(g_bindings) / sizeof(*g_bindings))
		return 0;//luaL_argerror(L, 1, "invalid binding index");
	index--;
	lua_pushstring(L, g_bindings[index]);
	lua_pushstring(L, "A");
	lua_pushstring(L, "B");
	return 3;
}

static int luaAPI_GetBindingText(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetBindingText(\"binding\", \"prefix\")");
	const char *binding = lua_tostring(L, 1);
	if (!binding)
		return luaL_argerror(L, 1, "string expected for binding");
	const char *prefix = lua_tostring(L, 2);
	if (!prefix)
		return luaL_argerror(L, 2, "string expected for prefix");
	char tmp[256];
	snprintf(tmp, sizeof(tmp), "%s%s", prefix, binding);
	lua_getglobal(L, tmp);
	return 1;
}

static int luaAPI_GetCurrentBindingSet(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCurrentBindingSet()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1);
	return 1;
}

static int luaAPI_GetBindingByKey(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetBindingByKey(\"key\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, lua_tostring(L, 1));
	return 1;
}

static int luaAPI_GetBindingAction(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetBindingAction(\"key\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "");
	return 1;
}

static int luaAPI_SetBinding(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	const char *binding;
	if (argc == 2)
		binding = lua_tostring(L, 2);
	else if (argc == 1)
		binding = NULL;
	else
		return luaL_error(L, "Usage: SetBinding(\"key\"[, \"binding\")");
	interface_execute_event(g_wow->interface, EVENT_UPDATE_BINDINGS, 0);
	return 0;
}

static int luaAPI_LoadBindings(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: LoadBindings(bindingSet)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetBindingKey(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetBindingKey(\"binding\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnil(L); //binding1
	lua_pushnil(L); //binding2
	return 2;
}

static int luaAPI_SaveBindings(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SaveBindings(bindingSet)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_bindings_functions(lua_State *L)
{
	LUA_REGISTER_FN(GetNumBindings);
	LUA_REGISTER_FN(GetBinding);
	LUA_REGISTER_FN(GetBindingText);
	LUA_REGISTER_FN(GetCurrentBindingSet);
	LUA_REGISTER_FN(GetBindingByKey);
	LUA_REGISTER_FN(GetBindingAction);
	LUA_REGISTER_FN(SetBinding);
	LUA_REGISTER_FN(LoadBindings);
	LUA_REGISTER_FN(GetBindingKey);
	LUA_REGISTER_FN(SaveBindings);
}
