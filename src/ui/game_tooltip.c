#include "ui/game_tooltip.h"

#include "xml/game_tooltip.h"

#include "wow_lua.h"
#include "log.h"

#ifdef interface
# undef interface
#endif

#define LUA_METHOD_GAME_TOOLTIP() LUA_METHOD(GameTooltip, game_tooltip)

#define UI_OBJECT (&UI_REGION->object)
#define UI_REGION (&UI_FRAME->region)
#define UI_FRAME (&game_tooltip->frame)

static bool ctr(struct ui_object *object, struct interface *interface, const char *name, struct ui_region *parent)
{
	if (!ui_frame_vtable.ctr(object, interface, name, parent))
		return false;
	struct ui_game_tooltip *game_tooltip = (struct ui_game_tooltip*)object;
	UI_OBJECT->mask |= UI_OBJECT_game_tooltip;
	game_tooltip->owner = NULL;
	return true;
}

static void dtr(struct ui_object *object)
{
	ui_frame_vtable.dtr(object);
}

static void render(struct ui_object *object)
{
	return;
	ui_frame_vtable.render(object);
}

void ui_game_tooltip_set_owner(struct ui_game_tooltip *game_tooltip, struct ui_frame *owner)
{
	if (game_tooltip->owner == owner)
		return;
	game_tooltip->owner = owner;
	/* TODO event ? */
}

static int lua_SetText(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	if (argc < 2 || argc > 7)
		return luaL_error(L, "Usage: GameTooltip:SetText(\"text\", [r, g, b [, a [, textWrap]]])");
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_GetOwner(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	if (argc != 1)
		return luaL_error(L, "Usage: GameTooltip:GetOwner()");
	if (game_tooltip->owner)
		ui_push_lua_object(L, (struct ui_object*)game_tooltip->owner);
	else
		lua_pushnil(L);
	return 1;
}

static int lua_SetOwner(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	if (argc < 2 || argc > 5)
		return luaL_error(L, "Usage: GameTooltip:SetOwner(owner [, \"anchor\", [offsetX, offsetY]])");
	struct ui_frame *owner = ui_get_lua_frame(L, 2);
	if (!owner)
		return luaL_argerror(L, 2, "Frame expected");
	ui_game_tooltip_set_owner(game_tooltip, owner);
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_IsOwned(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	if (argc != 2)
		return luaL_error(L, "Usage: GameTooltip:IsOwned(frame)");
	struct ui_frame *test = ui_get_lua_frame(L, 2);
	if (!test)
		return luaL_argerror(L, 2, "Frame expected");
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushboolean(L, test == game_tooltip->owner);
	return 1;
}

static int lua_SetAction(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_AddLine(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetTracking(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_GetPossession(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetPossession(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_AppendText(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetInventoryItem(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetUnit(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetUnitAura(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetUnitBuff(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetUnitDebuff(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetSpell(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetPadding(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetBagItem(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetTalent(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetAuctionItem(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_NumLines(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	lua_pushnumber(L, 1);
	return 1;
}

static int lua_SetMinimumWidth(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_GetItem(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetQuestLogItem(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_AddTexture(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_ClearLines(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_AddDoubleLine(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static int lua_SetPetAction(lua_State *L)
{
	LUA_METHOD_GAME_TOOLTIP();
	LUA_UNIMPLEMENTED_METHOD();
	return 0;
}

static bool register_methods(struct jks_array *methods)
{
	/*
	   AddFontStrings
	   FadeOut
	   GetAnchorType
	   GetSpell
	   GetUnit
	   IsUnit
	   SetAuctionCompareItem
	   SetAuctionSellItem
	   SetBuybackItem
	   SetCraftItem
	   SetCraftSpell
	   SetCurrencyToken
	   SetGuildBankItem
	   SetHyperlink
	   SetInboxItem
	   SetLootItem
	   SetLootRollItem
	   SetMerchantCompareItem
	   SetMerchantItem
	   SetPlayerBuff
	   SetQuestItem
	   SetQuestLogRewardSpell
	   SetQuestRewardSpell
	   SetSendMailItem
	   SetShapeshift
	   SetTradePlayerItem
	   SetTradeSkillItem
	   SetTradeTargetItem
	   SetTrainerService
	 */
	UI_REGISTER_METHOD(SetText);
	UI_REGISTER_METHOD(GetOwner);
	UI_REGISTER_METHOD(SetOwner);
	UI_REGISTER_METHOD(IsOwned);
	UI_REGISTER_METHOD(SetAction);
	UI_REGISTER_METHOD(AddLine);
	UI_REGISTER_METHOD(SetTracking);
	UI_REGISTER_METHOD(GetPossession);
	UI_REGISTER_METHOD(SetPossession);
	UI_REGISTER_METHOD(AppendText);
	UI_REGISTER_METHOD(SetInventoryItem);
	UI_REGISTER_METHOD(SetUnit);
	UI_REGISTER_METHOD(SetUnitAura);
	UI_REGISTER_METHOD(SetUnitBuff);
	UI_REGISTER_METHOD(SetUnitDebuff);
	UI_REGISTER_METHOD(SetSpell);
	UI_REGISTER_METHOD(SetPadding);
	UI_REGISTER_METHOD(SetBagItem);
	UI_REGISTER_METHOD(SetTalent);
	UI_REGISTER_METHOD(SetAuctionItem);
	UI_REGISTER_METHOD(NumLines);
	UI_REGISTER_METHOD(SetMinimumWidth);
	UI_REGISTER_METHOD(GetItem);
	UI_REGISTER_METHOD(SetQuestLogItem);
	UI_REGISTER_METHOD(AddTexture);
	UI_REGISTER_METHOD(ClearLines);
	UI_REGISTER_METHOD(AddDoubleLine);
	UI_REGISTER_METHOD(SetPetAction);
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
UI_INH1(frame, void, on_mouse_move, struct gfx_pointer_event*, event);
UI_INH1(frame, void, on_mouse_down, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_up, struct gfx_mouse_event*, event);
UI_INH1(frame, void, on_mouse_scroll, struct gfx_scroll_event*, event);
UI_INH1(frame, bool, on_key_down, struct gfx_key_event*, event);
UI_INH1(frame, bool, on_key_up, struct gfx_key_event*, event);
UI_INH0(frame, struct ui_font_instance*, as_font_instance);
UI_INH0(frame, const char*, get_name);

const struct ui_object_vtable ui_game_tooltip_vtable =
{
	UI_OBJECT_VTABLE("GameTooltip")
};
