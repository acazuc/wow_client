#include "functions.h"

#include "wow_lua.h"
#include "log.h"
#include "wow.h"
#include "dbc.h"

#include <libwow/dbc.h>

static int luaAPI_CanSendAuctionQuery(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: CanSendAuctionQuery(\"query\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, 1);
	return 1;
}

static int luaAPI_GetAuctionSort(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetAuctionSort(\"type\", reversed)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "name"); //column
	lua_pushboolean(L, 1); //reversed
	return 2;
}

static int luaAPI_GetNumAuctionItems(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetNumAuctionItems(\"category\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 5); //num batch
	lua_pushnumber(L, 10); //total
	return 2;
}

static int selected_item = 1;

static int luaAPI_SetSelectedAuctionItem(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: SetSelectedAuctionItem(\"category\", index)");
	LUA_UNIMPLEMENTED_FN();
	selected_item = lua_tointeger(L, 1);
	return 0;
}

static int luaAPI_GetSelectedAuctionItem(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetSelectedAuctionItem(\"category\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, selected_item);
	return 1;
}

static int luaAPI_CalculateAuctionDeposit(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: CalculateAuctionDeposit(duration)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 13371337);
	return 1;
}

static int luaAPI_GetAuctionItemInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetAuctionItemInfo(\"category\", index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "item"); //name
	lua_pushstring(L, "Interface\\Icons\\Spell_Shadow_SoulGem.blp"); //texture
	lua_pushinteger(L, 5); //number
	lua_pushinteger(L, rand() % 5); //quality
	lua_pushboolean(L, rand() & 1); //can use
	lua_pushinteger(L, 70); //level
	lua_pushinteger(L, 10); //min bid
	lua_pushinteger(L, 20); //min increment
	lua_pushinteger(L, 45896); //buyout price
	lua_pushinteger(L, 50); //bid amount
	lua_pushstring(L, "test"); //highest bidder
	lua_pushstring(L, "ccbsr"); //owner
	return 12;
}

static int luaAPI_GetAuctionItemTimeLeft(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetAuctionItemTimeLeft(\"category\", index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 6564);
	return 1;
}

static int luaAPI_GetBidderAuctionItems(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc > 1)
		return luaL_error(L, "Usage: GetAuctionItemTimeLeft([page])");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 2);
	return 1;
}

static int luaAPI_GetAuctionSellItemInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc > 1)
		return luaL_error(L, "Usage: GetAuctionSellItemInfo()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "Object"); //name
	lua_pushstring(L, "Interface\\Icons\\Spell_Shadow_SoulGem.blp"); //texture
	lua_pushinteger(L, 2); //count
	lua_pushinteger(L, 4); //quality
	lua_pushboolean(L, true); //can use
	lua_pushinteger(L, 13381338); //price
	return 6;
}

static int luaAPI_SortAuctionClearSort(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SortAuctionClearSort(\"category\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetOwnerAuctionItems(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc > 1)
		return luaL_error(L, "Usage: GetOwnerAuctionItems([page])");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_SortAuctionSetSort(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 3)
		return luaL_error(L, "Usage: SortAuctionSetSort(\"table\", column, reverse)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetAuctionItemClasses(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetAuctionItemClasses()");
	for (size_t i = 0; i < g_wow->dbc.item_class->file->header.record_count; ++i)
	{
		wow_dbc_row_t row = dbc_get_row(g_wow->dbc.item_class, i);
		lua_pushstring(L, wow_dbc_get_str(&row, 20));
	}
	return g_wow->dbc.item_class->file->header.record_count;
}

static int luaAPI_GetAuctionItemSubClasses(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetAuctionItemSubClasses(index)");
	int index = lua_tointeger(L, 1) - 1;
	if (index < 0 || (unsigned)index >= g_wow->dbc.item_class->file->header.record_count)
		return luaL_argerror(L, 1, "invalid class");
	wow_dbc_row_t row = dbc_get_row(g_wow->dbc.item_class, index);
	int class_id = wow_dbc_get_i32(&row, 0);
	size_t nb = 0;
	for (size_t i = 0; i < g_wow->dbc.item_sub_class->file->header.record_count; ++i)
	{
		row = dbc_get_row(g_wow->dbc.item_sub_class, i);
		if (wow_dbc_get_i32(&row, 0) != class_id)
			continue;
		lua_pushstring(L, wow_dbc_get_str(&row, 48));
		nb++;
	}
	return nb;
}

static int luaAPI_CloseAuctionHouse(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CloseAuctionHouse()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_PlaceAuctionBid(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 3)
		return luaL_error(L, "Usage: PlaceAuctionBid(\"list\", \"item\", price)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_QueryAuctionItems(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 9)
		return luaL_error(L, "Usage: QueryAuctionItems(\"name\", minLevel, maxLevel, invType, itemClass, itemSubClass, page, usable, rarity)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetAuctionInvTypes(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GetAuctionInvTypes(class, subClass)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_auction_functions(lua_State *L)
{
	LUA_REGISTER_FN(CanSendAuctionQuery);
	LUA_REGISTER_FN(GetAuctionSort);
	LUA_REGISTER_FN(GetNumAuctionItems);
	LUA_REGISTER_FN(GetSelectedAuctionItem);
	LUA_REGISTER_FN(SetSelectedAuctionItem);
	LUA_REGISTER_FN(CalculateAuctionDeposit);
	LUA_REGISTER_FN(GetAuctionItemInfo);
	LUA_REGISTER_FN(GetAuctionItemTimeLeft);
	LUA_REGISTER_FN(GetBidderAuctionItems);
	LUA_REGISTER_FN(GetAuctionSellItemInfo);
	LUA_REGISTER_FN(SortAuctionClearSort);
	LUA_REGISTER_FN(GetOwnerAuctionItems);
	LUA_REGISTER_FN(SortAuctionSetSort);
	LUA_REGISTER_FN(GetAuctionItemClasses);
	LUA_REGISTER_FN(GetAuctionItemSubClasses);
	LUA_REGISTER_FN(CloseAuctionHouse);
	LUA_REGISTER_FN(PlaceAuctionBid);
	LUA_REGISTER_FN(QueryAuctionItems);
	LUA_REGISTER_FN(GetAuctionInvTypes);
}
