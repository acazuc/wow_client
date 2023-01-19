#include "memory.h"
#include "cvars.h"
#include "log.h"

#include <jks/hmap.h>

#include <strings.h>
#include <ctype.h>

static struct jks_hmap cvars;

MEMORY_DECL(GENERIC);

static void cvar_dtr(jks_hmap_key_t key, void *ptr)
{
	mem_free(MEM_LUA, key.ptr);
	mem_free(MEM_LUA, *(char**)ptr);
}

static int cvar_cmp(jks_hmap_key_t k1, jks_hmap_key_t k2)
{
	return strcasecmp(k1.ptr, k2.ptr);
}

static uint32_t cvar_hash(jks_hmap_key_t k)
{
	const char *str = k.ptr;
	uint32_t hash = 5381;
	for (int c; (c = *str); str++)
		hash = ((hash << 5) + hash) + toupper(c);
	return hash;
}

bool cvars_init(void)
{
	jks_hmap_init(&cvars, sizeof(char*), cvar_dtr, cvar_hash, cvar_cmp, &jks_hmap_memory_fn_GENERIC);
#define ADD(key, val) \
	do \
	{ \
		char *k = mem_strdup(MEM_LUA, #key); \
		char *v = mem_strdup(MEM_LUA, #val); \
		if (!k || !v) \
		{ \
			mem_free(MEM_LUA, k); \
			mem_free(MEM_LUA, v); \
			LOG_ERROR("allocation failed"); \
			return false; \
		} \
		if (!jks_hmap_set(&cvars, JKS_HMAP_KEY_STR(k), &v)) \
		{ \
			mem_free(MEM_LUA, k); \
			mem_free(MEM_LUA, v); \
			LOG_ERROR("failed to add %s cvar", k); \
			return false; \
		} \
	} while (0)
	/* sub_5358C0 */
	ADD(mouseInvertYaw, 0);
	ADD(mouseInvertPitch, 0);
	ADD(cameraBobbing, 0);
	ADD(cameraDistanceMoveSpeed, 8.33);
	ADD(cameraPitchMoveSpeed, 90.0);
	ADD(cameraYawMoveSpeed, 180.0);
	ADD(cameraBobbingSmoothSpeed, 0.8);
	ADD(cameraFoVSmoothSpeed, 0.5);
	ADD(cameraDistanceSmoothSpeed, 8.33);
	ADD(cameraGroundSmoothSpeed, 7.5);
	ADD(cameraHeightSmoothSpeed, 1.2);
	ADD(cameraPitchSmoothSpeed, 45.0);
	ADD(cameraTargetSmoothSpeed, 90.0);
	ADD(cameraYawSmoothSpeed, 180.0);
	ADD(cameraFlyingMountHeightSmoothSpeed, 2.0);
	ADD(cameraViewBlendStyle, 1);
	ADD(cameraView, 1);
	ADD(cameraDistance, 0.0);
	ADD(cameraPitch, 0.0);
	ADD(camerasmooth, 1);
	ADD(cameraSmoothPitch, 0);
	ADD(cameraSmoothYaw, 1);
	ADD(cameraSmoothStyle, 1);
	ADD(cameraSmoothTrackingStyle, 1);
	ADD(cameraCustomViewSmoothing, 0);
	ADD(cameraTerrainTilt, 0);
	ADD(cameraTerrainTiltTimeMin, 3.0);
	ADD(cameraTerrainTiltTimeMax, 10.0);
	ADD(cameraWaterCollision, 1);
	ADD(cameraHeightIgnoreStandState, 0);
	ADD(cameraPivot, 1);
	ADD(cameraPivotDXMax, 0.05);
	ADD(cameraPivotDYMin, 0.00);
	ADD(cameraDive, 1);
	ADD(cameraSurfacePitch, 0.0);
	ADD(cameraSubmergePitch, 18.0);
	ADD(cameraSurfaceFinalPitch, 5.0);
	ADD(cameraSubmergeFinalPitch, 5.0);
	ADD(cameraDistanceMax, 15.0);
	ADD(cameraDistanceMaxFactor, 1.0);
	ADD(cameraPitchSmoothMin, 0.0);
	ADD(cameraPitchSmoothMax, 30.0);
	ADD(cameraYawSmoothMin, 0.0);
	ADD(cameraYawSmoothMax, 0.0);
	ADD(cameraSmoothTimeMin, 1.0);
	ADD(cameraSmoothTimeMax, 2.0);

	/* sub_4A9ED0 */
	ADD(taintLog, 0);
	ADD(scriptErrors, 0);
	ADD(scriptProfile, 0);
	ADD(xpBarText, 0);
	ADD(playerStatusText, 0);
	ADD(partyStatusText, 0);
	ADD(petStatusText, 0);
	ADD(targetStatusText, 0);
	ADD(statusTextPercentage, 0);
	ADD(displayFreeBagSlots, 0);
	ADD(minimapZoom, 3);
	ADD(minimapInsideZoom, 3);
	ADD(combatLogOn, 1);
	ADD(uiScale, 1.0);
	ADD(useUiScale, 0);
	ADD(deselectOnClick, 1);
	ADD(showLootSpam, 1);
	ADD(assistAttack, 0);
	ADD(stopAutoAttackOnTargetChange, 0);
	ADD(unitHighlights, 1);
	ADD(guildMemberNotify, 0);
	ADD(autoClearAFK, 1);
	ADD(autoRangedCombat, 1);
	ADD(autoSelfCast, 0);
	ADD(secureAbilityToggle, 1);
	ADD(lootUnderMouse, 0);
	ADD(autoLootCorpse, 0);
	ADD(showTargetOfTarget, 0);
	ADD(targetOfTargetMode, 5);
	ADD(buffDurations, 1);
	ADD(questFadingDisable, 0);
	ADD(autoQuestWatch, 1);
	ADD(removeChatDelay, 0);
	ADD(guildRecruitmentChannel, 1);
	ADD(chatLocked, 0);
	ADD(lockActionBars, 0);
	ADD(alwaysShowActionBars, 0);
	ADD(displayWorldPVPObjectives, 1);
	ADD(enableCombatText, 0);
	ADD(combatTextFloatMode, 1);
	ADD(fctCombatState, 0);
	ADD(fctDodgeParryMiss, 0);
	ADD(fctDamageReduction, 0);
	ADD(fctRepChanges, 0);
	ADD(fctReactives, 0);
	ADD(fctFriendlyHealers, 0);
	ADD(fctComboPoints, 0);
	ADD(fctLowManaHealth, 0);
	ADD(fctEnergyGains, 0);
	ADD(fctHonorGains, 0);
	ADD(fctAuras, 0);
	ADD(showChatIcons, 0);
	ADD(showNewbieTips, 1);
	ADD(showPartyBackground, 0);
	ADD(hidePartyInRaid, 0);
	ADD(showPartyBuffs, 0);
	ADD(showPartyDebuffs, 1);
	ADD(showPartyPets, 1);
	ADD(showRaidRange, 0);
	ADD(screenEdgeFlash, 1);
	ADD(useSimpleChat, 0);
	ADD(showClock, 1);
	ADD(timeMgrUseMilitaryTime, 0);
	ADD(timeMgrUseLocalTime, 0);
	ADD(timeMgrAlarmTime, 0);
	ADD(timeMgrAlarmMessage, 0);
	ADD(timeMgrAlarmEnabled, 0);
	ADD(autojoinPartyVoice, 1);
	ADD(autojoinBGVoice, 0);
	ADD(enablePVPNotifyAFK, 1);
	ADD(targetNearestDistanceRadius, 10.0);
	ADD(targetNearestDistance, 41.0);
	ADD(UnitNameRenderMode, 2);
	ADD(UberTooltips, 1);
	ADD(BlockTrades, 0);
	ADD(ShowTargetCastbar, 0);
	ADD(ShowVKeyCastbar, 0);

	/* sub_6D53E0 */
	ADD(UnitNameOwn, 0);
	ADD(UnitNameNPC, 0);
	ADD(UnitNamePlayerGuild, 1);
	ADD(UnitNamePlayerPVPTitle, 1);
	ADD(UnitNameEnemyPlayerName, 1);
	ADD(UnitNameEnemyPetName, 1);
	ADD(UnitNameEnemyCreationName, 1);
	ADD(UnitNameFriendlyPlayerName, 1);
	ADD(UnitNameFriendlyPetName, 1);
	ADD(UnitNameFriendlyCreationName, 1);
	ADD(UnitNameCompanionName, 1);

	/* sub_60D780 */
	ADD(PetMeleeDamage, 1);
	ADD(PetSpellDamage, 1);
	ADD(CombatDamage, 1);
	ADD(CombatHealing, 1);
	ADD(CombatLogPeriodicSpells, 1);
	ADD(ChatBubbles, 1);
	ADD(ChatBubblesParty, 0);
	ADD(PlayerAnim, 0);
	ADD(TargetAnim, 0);
	ADD(AutoInteract, 1);
	ADD(autoStand, 1);
	ADD(autoDismount, 1);
	ADD(autoDismountFlying, 0);
	ADD(autoUnshift, 1);
	ADD(pathDistTol, 1);

	/* sub_403590 */
	ADD(accountName, "");
	ADD(movie, 1);
	ADD(expansionMovie, 1);
	ADD(movieSubtitle, 0);
	ADD(checkAddonVersion, 1);
	ADD(mouseSpeed, 10.0);
	ADD(Errors, 0);
	ADD(ShowErrors, 1);
	ADD(ErrorLevelMin, 2);
	ADD(ErrorLevelMax, 3);
	ADD(ErrorFilter, 0);
	ADD(DesktopGamma, 0);
	ADD(Gamma, 1.0);
	ADD(lastCharacterIndex, 0);
	ADD(readTOS, 0);
	ADD(readEULA, 0);
	ADD(readTerminationWithoutNotice, 0);
	ADD(readScanning, 0);
	ADD(readContest, 0);
	ADD(profanityFilter, 1);
	ADD(spamFilter, 1);
	ADD(screenshotFormat, jpeg);
	ADD(screenshotQuality, 3);
	ADD(showToolsUI, -1);

	/* sub_4C8970 */
	ADD(rotateMinimap, 0);

	/* sub_63D110 */
	ADD(widescreen, 1);
	ADD(gxWindow, 1);
	ADD(gxMaximize, 1);
	ADD(gxColorBits, 24);
	ADD(gxDepthBits, 24);
	ADD(gxResolution, 640x480);
	ADD(gxRefresh, 75);
	ADD(gxTripleBuffer, 0);
	ADD(gxApi, direct3d);
	ADD(gxVSync, 1);
	ADD(gxAspect, 1);
	ADD(gxCursor, 1);
	ADD(gxMultisample, 1);
	ADD(gxMultisampleQuality, 0.0);
	ADD(gxFixLag, 1);
	ADD(gxOverride, 0);
	ADD(maxFPS, 0);
	ADD(maxFPSBk, 0);
	ADD(videoOptionsVersion, 0);
	ADD(windowResizeLock, 0);

	/* sub_467950 */
	ADD(Sound_OutputQuality, 1);
	ADD(Sound_NumChannels, 32);
	ADD(Sound_EnableReverb, 0);
	ADD(Sound_VoiceChatInputDriverIndex, 0);
	ADD(Sound_VoiceChatInputDriverName, );
	ADD(Sound_VoiceChatOutputDriverIndex, 0);
	ADD(Sound_VoiceChatOutputDriverName, );
	ADD(Sound_OutputDriverIndex, 0);
	ADD(Sound_OutputDriverName, );
	ADD(Sound_DSPBufferSize, 0);
	ADD(Sound_EnableHardware, 0);
	ADD(Sound_EnableMode2, 0);
	ADD(Sound_EnableMixMode2, 0);

	/* sub_467D20 */
	ADD(ChatMusicVolume, 0.3);
	ADD(ChatSoundVolume, 0.4);
	ADD(ChatAmbienceVolume, 0.3);
	ADD(Sound_EnableSFX, 1);
	ADD(Sound_EnableAmbience, 1);
	ADD(Sound_EnableErrorSpeech, 1);
	ADD(Sound_EnableMusic, 1);
	ADD(Sound_EnableAllSound, 1);
	ADD(Sound_MasterVolume, 1.0);
	ADD(Sound_SFXVolume, 1.0);
	ADD(Sound_MusicVolume, 0.4);
	ADD(Sound_AmbienceVolume, 0.6);
	ADD(Sound_ListenerAtCharacter, 1);
	ADD(Sound_EnableEmoteSounds, 1);
	ADD(Sound_ZoneMusicNoDelay, 0);
	ADD(Sound_EnableArmorFoleySoundForSelf, 1);
	ADD(Sound_EnableArmorFoleySoundForOthers, 1);
	ADD(Sound_EnableSoundWhenGameIsInBG, 0);

	/* sub_6DBDE0 */
	ADD(StartTalkingDelay, 0.0);
	ADD(StartTalkingTime, 1.0);
	ADD(StopTalkingDelay, 0.0);
	ADD(StopTalkingTime, 2.0);
	ADD(OutboundChatVolume, 1.0);
	ADD(InboundChatVolume, 1.0);
	ADD(VoiceChatMode, 0);
	ADD(VoiceActivationSensitivity, 0.4);
	ADD(EnableMicrophone, 1);
	ADD(EnableVoiceChat, 0);
	ADD(VoiceChatSelfMute, 0);
	ADD(PushToTalkButton, `);

	/* sub_46F460 */
	ADD(gameTip, 0);
	ADD(showGameTips, 1);

	/* sub_4063B0 */
	ADD(dbCompress, -1);
	ADD(locale, g_wow->lang);

	return true;
#undef ADD
}

const char *cvar_get(const char *name)
{
	char **cvar = jks_hmap_get(&cvars, JKS_HMAP_KEY_STR((char*)name));
	if (!cvar)
		return NULL;
	return *cvar;
}

bool cvar_set(const char *key, const char *val)
{
	char *k = mem_strdup(MEM_GENERIC, key);
	char *v = mem_strdup(MEM_GENERIC, val ? val : "");
	if (!k || !v)
	{
		mem_free(MEM_GENERIC, k);
		mem_free(MEM_GENERIC, v);
		LOG_ERROR("allocation failed");
		return false;
	}
	if (!jks_hmap_set(&cvars, JKS_HMAP_KEY_STR(k), &v))
	{
		mem_free(MEM_GENERIC, k);
		mem_free(MEM_GENERIC, v);
		LOG_ERROR("failed to set jks hmap");
		return false;
	}
	return true;
}

void cvars_cleanup(void)
{
	jks_hmap_destroy(&cvars);
}
