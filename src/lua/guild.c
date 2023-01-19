#include "functions.h"

#include "wow_lua.h"
#include "log.h"

static int luaAPI_SetGuildRosterSelection(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetGuildRosterSelection(index)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_IsInGuild(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsInGuild()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_GetGuildRosterMOTD(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGuildRosterMOTD()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "Guild MOTD");
	return 1;
}

static int luaAPI_GuildControlGetNumRanks(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GuildControlGetNumRanks()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 3);
	return 1;
}

static int luaAPI_GetGuildInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetGuildInfo(\"player\")");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "My guild");
	lua_pushstring(L, "GuildRank");
	lua_pushinteger(L, 1); //Rank index
	return 3;
}

static int luaAPI_GetNumGuildMembers(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc > 1)
		return luaL_error(L, "Usage: GetNumGuildMembers([showOffline])");
	bool show_offline;
	if (argc == 1)
	{
		//TODO
		show_offline = true;
	}
	else
	{
		show_offline = true;
	}
	lua_pushinteger(L, 3);
	return 1;
}

static int luaAPI_GetGuildRosterSelection(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGuildRosterSelectection()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1); //Selected guild member
	return 1;
}

static int luaAPI_GetGuildRosterInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetGuildRosterInfo(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "MemberName");
	lua_pushstring(L, "Rank");
	lua_pushinteger(L, 1); //RankIndex
	lua_pushinteger(L, 70); //Level
	lua_pushinteger(L, 1); //Class
	lua_pushinteger(L, 1); //Zone
	lua_pushstring(L, "note");
	lua_pushstring(L, "officerNote");
	lua_pushboolean(L, true); //Online
	return 9;
}

static int luaAPI_CanEditPublicNote(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanEditPublicNote()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_CanViewOfficerNote(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanViewOfficerNote()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_CanEditOfficerNote(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanEditOfficerNote()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_CanGuildPromote(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanGuildPromote()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_CanGuildDemote(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanGuildDemote()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_CanGuildRemove(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanGuildRemove()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_CanEditMOTD(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanEditMOTD()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_IsGuildLeader(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: IsGuildLeader()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_CanGuildInvite(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanGuildInvite()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_GuildRoster(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GuildRoster()");
	LUA_UNIMPLEMENTED_FN();
	//TODO load roster from server
	return 0;
}

static int luaAPI_GuildControlGetRankFlags(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GuildControlGetRankFlags()");
	LUA_UNIMPLEMENTED_FN();
	for (size_t i = 0; i < 13; ++i)
		lua_pushboolean(L, true);
	return 13;
}

static int luaAPI_GuildControlGetRankName(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GuildControlGetRankName(index)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "MyRank");
	return 1;
}

static int luaAPI_GetNumGuildBankTabs(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetNumGuildBankTabs()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 3);
	return 1;
}

static int luaAPI_GetGuildInfoText(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGuildInfoText()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "info text");
	return 1;
}

static int luaAPI_CanEditGuildInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanEditGuildInfo()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, 1);
	return 1;
}

static int luaAPI_SortGuildRoster(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SortGuildRoster(\"sort\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GuildInvite(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GuildInvite(\"name\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_QueryGuildEventLog(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: QueryGuildEventLog()");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_SetGuildInfoText(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetGuildInfoText(\"text\"");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_SetGuildRosterShowOffline(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetGuildRosterShowOffline(offline)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetGuildBankMoney(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGuildBankMoney()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 13371337);
	return 1;
}

static int luaAPI_GetGuildBankWithdrawMoney(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGuildBankWithdrawMoney()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1337);
	return 1;
}

static int luaAPI_CanGuildBankRepair(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanGuildBankRepair()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_GetCurrentGuildBankTab(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetCurrentGuildBankTab()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushinteger(L, 1);
	return 1;
}

static int luaAPI_GetGuildBankTabInfo(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetGuildBankTabInfo(tab)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, "name");
	lua_pushstring(L, "Interface\\Icons\\Spell_Shadow_SoulGem.blp");
	lua_pushboolean(L, true); //viewable
	lua_pushboolean(L, true); //deposit
	lua_pushnumber(L, 10); //num withdraw
	lua_pushnumber(L, 5); //remaining withdraw
	return 6;
}

static int luaAPI_CanWithdrawGuildBankMoney(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: CanWithdrawGuildBankMoney()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true);
	return 1;
}

static int luaAPI_GetGuildTabardFileNames(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGuildTabardFileNames()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushstring(L, ""); //bg upper
	lua_pushstring(L, ""); //bg lower
	lua_pushstring(L, ""); //emblem upper
	lua_pushstring(L, ""); //emblem lower
	lua_pushstring(L, ""); //border upper
	lua_pushstring(L, ""); //border lower
	return 6;
}

static int luaAPI_GuildPromote(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GuildPromote(\"player\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GuildDemote(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GuildDemote(\"player\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GuildUninvite(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GuildUninvite(\"player\")");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetGuildBankTabPermissions(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: GetGuildBankTabPermissions(tab)");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, true); //view tab
	lua_pushboolean(L, true); //can deposit
	lua_pushboolean(L, true); //can update text
	lua_pushnumber(L, 1337); //num withdrawals
	return 4;
}

static int luaAPI_GetGuildBankWithdrawLimit(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGuildBankWithdrawLimit()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushnumber(L, 13371337);
	return 1;
}

static int luaAPI_GuildControlSetRankFlag(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "Usage: GuildControlSetRankFlag(rank, checked)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_SetGuildBankWithdrawLimit(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetGuildBankWithdrawLimit(limit)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

static int luaAPI_GetGuildRosterShowOffline(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 0)
		return luaL_error(L, "Usage: GetGuildRosterShowOffline()");
	LUA_UNIMPLEMENTED_FN();
	lua_pushboolean(L, false);
	return 1;
}

static int luaAPI_SetGuildRecruitmentMode(lua_State *L)
{
	LUA_VERBOSE_FN();
	int argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "Usage: SetGuildRecruitmentMode(mode)");
	LUA_UNIMPLEMENTED_FN();
	return 0;
}

void register_guild_functions(lua_State *L)
{
	LUA_REGISTER_FN(SetGuildRosterSelection);
	LUA_REGISTER_FN(IsInGuild);
	LUA_REGISTER_FN(GetGuildRosterMOTD);
	LUA_REGISTER_FN(GuildControlGetNumRanks);
	LUA_REGISTER_FN(GetGuildInfo);
	LUA_REGISTER_FN(GetNumGuildMembers);
	LUA_REGISTER_FN(GetGuildRosterSelection);
	LUA_REGISTER_FN(GetGuildRosterInfo);
	LUA_REGISTER_FN(CanEditPublicNote);
	LUA_REGISTER_FN(CanViewOfficerNote);
	LUA_REGISTER_FN(CanEditOfficerNote);
	LUA_REGISTER_FN(CanGuildPromote);
	LUA_REGISTER_FN(CanGuildDemote);
	LUA_REGISTER_FN(CanGuildRemove);
	LUA_REGISTER_FN(CanEditMOTD);
	LUA_REGISTER_FN(IsGuildLeader);
	LUA_REGISTER_FN(CanGuildInvite);
	LUA_REGISTER_FN(GuildRoster);
	LUA_REGISTER_FN(GuildControlGetRankFlags);
	LUA_REGISTER_FN(GuildControlGetRankName);
	LUA_REGISTER_FN(GetNumGuildBankTabs);
	LUA_REGISTER_FN(GetGuildInfoText);
	LUA_REGISTER_FN(CanEditGuildInfo);
	LUA_REGISTER_FN(SortGuildRoster);
	LUA_REGISTER_FN(GuildInvite);
	LUA_REGISTER_FN(QueryGuildEventLog);
	LUA_REGISTER_FN(SetGuildInfoText);
	LUA_REGISTER_FN(SetGuildRosterShowOffline);
	LUA_REGISTER_FN(GetGuildBankMoney);
	LUA_REGISTER_FN(GetGuildBankWithdrawMoney);
	LUA_REGISTER_FN(CanGuildBankRepair);
	LUA_REGISTER_FN(GetCurrentGuildBankTab);
	LUA_REGISTER_FN(GetGuildBankTabInfo);
	LUA_REGISTER_FN(CanWithdrawGuildBankMoney);
	LUA_REGISTER_FN(GetGuildTabardFileNames);
	LUA_REGISTER_FN(GuildPromote);
	LUA_REGISTER_FN(GuildDemote);
	LUA_REGISTER_FN(GuildUninvite);
	LUA_REGISTER_FN(GetGuildBankTabPermissions);
	LUA_REGISTER_FN(GetGuildBankWithdrawLimit);
	LUA_REGISTER_FN(GuildControlSetRankFlag);
	LUA_REGISTER_FN(SetGuildBankWithdrawLimit);
	LUA_REGISTER_FN(GetGuildRosterShowOffline);
	LUA_REGISTER_FN(SetGuildRecruitmentMode);
}
