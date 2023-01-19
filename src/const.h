#ifndef CONST_H
#define CONST_H

#define CHUNK_WIDTH (33.3334f)

enum class_type
{
	CLASS_WARRIOR = 0x1,
	CLASS_PALADIN = 0x2,
	CLASS_HUNTER  = 0x3,
	CLASS_ROGUE   = 0x4,
	CLASS_PRIEST  = 0x5,
	CLASS_SHAMAN  = 0x7,
	CLASS_MAGE    = 0x8,
	CLASS_WARLOCK = 0x9,
	CLASS_DRUID   = 0xB,
	CLASS_MAX     = 0xC,
};

enum class_mask
{
	CLASS_MASK_WARRIOR = 1 << (CLASS_WARRIOR - 1), /* 0x001 */
	CLASS_MASK_PALADIN = 1 << (CLASS_PALADIN - 1), /* 0x002 */
	CLASS_MASK_HUNTER  = 1 << (CLASS_HUNTER - 1),  /* 0x004 */
	CLASS_MASK_ROGUE   = 1 << (CLASS_ROGUE - 1),   /* 0x008 */
	CLASS_MASK_PRIEST  = 1 << (CLASS_PRIEST - 1),  /* 0x010 */
	CLASS_MASK_SHAMAN  = 1 << (CLASS_SHAMAN - 1),  /* 0x040 */
	CLASS_MASK_MAGE    = 1 << (CLASS_MAGE - 1),    /* 0x080 */
	CLASS_MASK_WARLOCK = 1 << (CLASS_WARLOCK - 1), /* 0x100 */
	CLASS_MASK_DRUID   = 1 << (CLASS_DRUID - 1),   /* 0x800 */
};

enum race_type
{
	RACE_HUMAN        = 1,
	RACE_ORC          = 2,
	RACE_DWARF        = 3,
	RACE_NIGHT_ELF    = 4,
	RACE_UNDEAD       = 5,
	RACE_TAUREN       = 6,
	RACE_GNOME        = 7,
	RACE_TROLL        = 8,
	RACE_GOBLIN       = 9,
	RACE_BLOODELF     = 10,
	RACE_DRAENEI      = 11,
	RACE_FEL_ORC      = 12,
	RACE_NAGA         = 13,
	RACE_BROKEN       = 14,
	RACE_SKELETON     = 15,
	RACE_VRYKUL       = 16,
	RACE_TUSKARR      = 17,
	RACE_FOREST_TROLL = 18,
	RACE_MAX          = 19,
};

enum race_mask
{
	RACE_MASK_HUMAN     = 1 << (RACE_HUMAN - 1),     /* 0x001 */
	RACE_MASK_ORC       = 1 << (RACE_ORC - 1),       /* 0x002 */
	RACE_MASK_DWARF     = 1 << (RACE_DWARF - 1),     /* 0x004 */
	RACE_MASK_NIGHT_ELF = 1 << (RACE_NIGHT_ELF - 1), /* 0x008 */
	RACE_MASK_UNDEAD    = 1 << (RACE_UNDEAD - 1),    /* 0x010 */
	RACE_MASK_TAUREN    = 1 << (RACE_TAUREN - 1),    /* 0x020 */
	RACE_MASK_GNOME     = 1 << (RACE_GNOME - 1),     /* 0x040 */
	RACE_MASK_TROLL     = 1 << (RACE_TROLL - 1),     /* 0x080 */
	RACE_MASK_BLOODELF  = 1 << (RACE_BLOODELF - 1),  /* 0x200 */
	RACE_MASK_DRAENEI   = 1 << (RACE_DRAENEI - 1),   /* 0x400 */
};

enum gender_type
{
	GENDER_MALE   = 0x0,
	GENDER_FEMALE = 0x1,
};

enum object_type
{
	OBJECT_OBJECT        = 0,
	OBJECT_ITEM          = 1,
	OBJECT_CONTAINER     = 2,
	OBJECT_UNIT          = 3,
	OBJECT_PLAYER        = 4,
	OBJECT_GAMEOBJECT    = 5,
	OBJECT_DYNAMICOBJECT = 6,
	OBJECT_CORPSE        = 7,
};

enum object_mask
{
	OBJECT_MASK_OBJECT        = (1 << OBJECT_OBJECT),
	OBJECT_MASK_ITEM          = (1 << OBJECT_ITEM),
	OBJECT_MASK_CONTAINER     = (1 << OBJECT_CONTAINER),
	OBJECT_MASK_UNIT          = (1 << OBJECT_UNIT),
	OBJECT_MASK_PLAYER        = (1 << OBJECT_PLAYER),
	OBJECT_MASK_GAMEOBJECT    = (1 << OBJECT_GAMEOBJECT),
	OBJECT_MASK_DYNAMICOBJECT = (1 << OBJECT_DYNAMICOBJECT),
	OBJECT_MASK_CORPSE        = (1 << OBJECT_CORPSE),
};

enum object_update_type
{
	OBJECT_UPDATE              = 0,
	OBJECT_UPDATE_MOVEMENT     = 1,
	OBJECT_CREATE              = 2,
	OBJECT_CREATE2             = 3,
	OBJECT_UPDATE_OUT_OF_RANGE = 4,
	OBJECT_UPDATE_IN_RANGE     = 5,
};

enum movement_flag
{
	MOVEFLAG_NONE               = 0x00000000,
	MOVEFLAG_FORWARD            = 0x00000001,
	MOVEFLAG_BACKWARD           = 0x00000002,
	MOVEFLAG_STRAFE_LEFT        = 0x00000004,
	MOVEFLAG_STRAFE_RIGHT       = 0x00000008,
	MOVEFLAG_TURN_LEFT          = 0x00000010,
	MOVEFLAG_TURN_RIGHT         = 0x00000020,
	MOVEFLAG_PITCH_UP           = 0x00000040,
	MOVEFLAG_PITCH_DOWN         = 0x00000080,
	MOVEFLAG_WALK_MODE          = 0x00000100,
	MOVEFLAG_ONTRANSPORT        = 0x00000200,
	MOVEFLAG_LEVITATING         = 0x00000400,
	MOVEFLAG_ROOT               = 0x00000800,
	MOVEFLAG_FALLING            = 0x00001000,
	MOVEFLAG_FALLINGFAR         = 0x00002000,
	MOVEFLAG_PENDINGSTOP        = 0x00004000,
	MOVEFLAG_PENDINGSTRAFESTOP  = 0x00008000,
	MOVEFLAG_PENDINGFORWARD     = 0x00010000,
	MOVEFLAG_PENDINGBACKWARD    = 0x00020000,
	MOVEFLAG_PENDINGSTRAFELEFT  = 0x00040000,
	MOVEFLAG_PENDINGSTRAFERIGHT = 0x00080000,
	MOVEFLAG_PENDINGROOT        = 0x00100000,
	MOVEFLAG_SWIMMING           = 0x00200000,
	MOVEFLAG_ASCENDING          = 0x00400000,
	MOVEFLAG_DESCENDING         = 0x00800000,
	MOVEFLAG_CAN_FLY            = 0x01000000,
	MOVEFLAG_FLYING             = 0x02000000,
	MOVEFLAG_SPLINE_ELEVATION   = 0x04000000,
	MOVEFLAG_SPLINE_ENABLED     = 0x08000000,
	MOVEFLAG_WATERWALKING       = 0x10000000,
	MOVEFLAG_SAFE_FALL          = 0x20000000,
	MOVEFLAG_HOVER              = 0x40000000,
};

enum friend_status
{
	FRIEND_STATUS_OFFLINE = 0,
	FRIEND_STATUS_ONLINE  = 1,
	FRIEND_STATUS_AFK     = 2,
	FRIEND_STATUS_AFK2    = 3,
	FRIEND_STATUS_DND     = 4,
};

enum friend_result
{
	FRIEND_RESULT_DB_ERROR         = 0x00,
	FRIEND_RESULT_LIST_FULL        = 0x01,
	FRIEND_RESULT_ONLINE           = 0x02,
	FRIEND_RESULT_OFFLINE          = 0x03,
	FRIEND_RESULT_NOT_FOUND        = 0x04,
	FRIEND_RESULT_REMOVED          = 0x05,
	FRIEND_RESULT_ADDED_ONLINE     = 0x06,
	FRIEND_RESULT_ADDED_OFFLINE    = 0x07,
	FRIEND_RESULT_ALREADY          = 0x08,
	FRIEND_RESULT_SELF             = 0x09,
	FRIEND_RESULT_ENEMY            = 0x0A,
	FRIEND_RESULT_IGNORE_FULL      = 0x0B,
	FRIEND_RESULT_IGNORE_SELF      = 0x0C,
	FRIEND_RESULT_IGNORE_NOT_FOUND = 0x0D,
	FRIEND_RESULT_IGNORE_ALREADY   = 0x0E,
	FRIEND_RESULT_IGNORE_ADDED     = 0x0F,
	FRIEND_RESULT_IGNORE_REMOVED   = 0x10,
	FRIEND_RESULT_IGNORE_AMBIGUOUS = 0x11,
	FRIEND_RESULT_MUTE_FULL        = 0x12,
	FRIEND_RESULT_MUTE_SELF        = 0x13,
	FRIEND_RESULT_MUTE_NOT_FOUND   = 0x14,
	FRIEND_RESULT_MUTE_ALREADY     = 0x15,
	FRIEND_RESULT_MUTE_ADDED       = 0x16,
	FRIEND_RESULT_MUTE_REMOVED     = 0x17,
	FRIEND_RESULT_MUTE_AMBIGUOUS   = 0x18, /* ERR_VOICE_IGNORE_AMBIGUOUS */
	FRIEND_RESULT_UNK7             = 0x19, /* ERR_MAX_VALUE (nothing is showed) */
	FRIEND_RESULT_UNKNOWN          = 0x1A, /* Unknown friend response from server */
};

enum quest_giver_status
{
	QUESTGIVER_STATUS_NONE          = 0,
	QUESTGIVER_STATUS_FUTURE        = 1,  /* quest not unlocked yet */
	QUESTGIVER_STATUS_CHAT          = 2,  /* quest cursor but no 3D yellow '!' on NPC */
	QUESTGIVER_STATUS_INCOMPLETE    = 3,  /* objective not yet completed */
	QUESTGIVER_STATUS_REWARD_REP    = 4,  /* blue 3D '?' over NPC, daily quest */
	QUESTGIVER_STATUS_AVAILABLE_REP = 5,  /* blue 3D '!' over NPC, daily quest */
	QUESTGIVER_STATUS_AVAILABLE     = 6,  /* default quest available */
	QUESTGIVER_STATUS_REWARD2       = 7,  /* quest can be turned in, no yellow dot on minimap */
	QUESTGIVER_STATUS_REWARD        = 8,  /* default quest can be turned in, yellow dot on minimap */
	QUESTGIVER_STATUS_UNDEFINED     = 100 /* used as result for unassigned ScriptCall */
};

enum party_loot_method
{
	PARTY_LOOT_METHOD_FREEFORALL   = 0,
	PARTY_LOOT_METHOD_ROUNDROBIN   = 1,
	PARTY_LOOT_METHOD_MASTERLOOTER = 2,
};

enum item_dyn_flags
{
	ITEM_DYNFLAG_BINDED   = 0x00000001, /* set in game at binding */
	ITEM_DYNFLAG_UNK1     = 0x00000002,
	ITEM_DYNFLAG_UNLOCKED = 0x00000004, /* have meaning only for item with proto->LockId, if not set show as "Locked, req. lockpicking N" */
	ITEM_DYNFLAG_WRAPPED  = 0x00000008, /* mark item as wrapped into wrapper container */
	ITEM_DYNFLAG_UNK4     = 0x00000010, /* can't repeat old note: appears red icon (like when item durability==0) */
	ITEM_DYNFLAG_UNK5     = 0x00000020,
	ITEM_DYNFLAG_UNK6     = 0x00000040, /* ? old note: usable */
	ITEM_DYNFLAG_UNK7     = 0x00000080,
	ITEM_DYNFLAG_UNK8     = 0x00000100,
	ITEM_DYNFLAG_READABLE = 0x00000200, /* can be open for read, it or item proto pagetText make show "Right click to read" */
	ITEM_DYNFLAG_UNK10    = 0x00000400,
	ITEM_DYNFLAG_UNK11    = 0x00000800,
	ITEM_DYNFLAG_UNK12    = 0x00001000,
	ITEM_DYNFLAG_UNK13    = 0x00002000,
	ITEM_DYNFLAG_UNK14    = 0x00004000,
	ITEM_DYNFLAG_UNK15    = 0x00008000,
	ITEM_DYNFLAG_UNK16    = 0x00010000,
	ITEM_DYNFLAG_UNK17    = 0x00020000,
	ITEM_DYNFLAG_UNK18    = 0x00040000,
	ITEM_DYNFLAG_UNK19    = 0x00080000,
	ITEM_DYNFLAG_UNK20    = 0x00100000,
	ITEM_DYNFLAG_UNK21    = 0x00200000,
	ITEM_DYNFLAG_UNK22    = 0x00400000,
	ITEM_DYNFLAG_UNK23    = 0x00800000,
	ITEM_DYNFLAG_UNK24    = 0x01000000,
	ITEM_DYNFLAG_UNK25    = 0x02000000,
	ITEM_DYNFLAG_UNK26    = 0x04000000,
	ITEM_DYNFLAG_UNK27    = 0x08000000,
};

enum equipment_slot
{
	EQUIPMENT_SLOT_HEAD      = 0,
	EQUIPMENT_SLOT_NECK      = 1,
	EQUIPMENT_SLOT_SHOULDERS = 2,
	EQUIPMENT_SLOT_SHIRT     = 3,
	EQUIPMENT_SLOT_CHEST     = 4,
	EQUIPMENT_SLOT_WAIST     = 5,
	EQUIPMENT_SLOT_LEGS      = 6,
	EQUIPMENT_SLOT_FEET      = 7,
	EQUIPMENT_SLOT_WRISTS    = 8,
	EQUIPMENT_SLOT_HANDS     = 9,
	EQUIPMENT_SLOT_FINGER1   = 10,
	EQUIPMENT_SLOT_FINGER2   = 11,
	EQUIPMENT_SLOT_TRINKET1  = 12,
	EQUIPMENT_SLOT_TRINKET2  = 13,
	EQUIPMENT_SLOT_BACK      = 14,
	EQUIPMENT_SLOT_MAINHAND  = 15,
	EQUIPMENT_SLOT_OFFHAND   = 16,
	EQUIPMENT_SLOT_RANGED    = 17,
	EQUIPMENT_SLOT_TABARD    = 18,
};

enum item_class
{
	ITEM_CLASS_CONSUMABLE  = 0,
	ITEM_CLASS_CONTAINER   = 1,
	ITEM_CLASS_WEAPON      = 2,
	ITEM_CLASS_GEM         = 3,
	ITEM_CLASS_ARMOR       = 4,
	ITEM_CLASS_REAGENT     = 5,
	ITEM_CLASS_PROJECTILE  = 6,
	ITEM_CLASS_TRADE_GOODS = 7,
	ITEM_CLASS_GENERIC     = 8,
	ITEM_CLASS_RECIPE      = 9,
	ITEM_CLASS_MONEY       = 10,
	ITEM_CLASS_QUIVER      = 11,
	ITEM_CLASS_QUEST       = 12,
	ITEM_CLASS_KEY         = 13,
	ITEM_CLASS_PERMANENT   = 14,
	ITEM_CLASS_MISC        = 15,
};

enum npc_flag
{
	NPC_FLAG_NONE               = 0x00000000,
	NPC_FLAG_GOSSIP             = 0x00000001,
	NPC_FLAG_QUESTGIVER         = 0x00000002,
	NPC_FLAG_UNK1               = 0x00000004,
	NPC_FLAG_UNK2               = 0x00000008,
	NPC_FLAG_TRAINER            = 0x00000010,
	NPC_FLAG_TRAINER_CLASS      = 0x00000020,
	NPC_FLAG_TRAINER_PROFESSION = 0x00000040,
	NPC_FLAG_VENDOR             = 0x00000080,
	NPC_FLAG_VENDOR_AMMO        = 0x00000100,
	NPC_FLAG_VENDOR_FOOD        = 0x00000200,
	NPC_FLAG_VENDOR_POISON      = 0x00000400,
	NPC_FLAG_VENDOR_REAGENT     = 0x00000800,
	NPC_FLAG_REPAIR             = 0x00001000,
	NPC_FLAG_FLIGHTMASTER       = 0x00002000,
	NPC_FLAG_SPIRITHEALER       = 0x00004000,
	NPC_FLAG_SPIRITGUIDE        = 0x00008000,
	NPC_FLAG_INNKEEPER          = 0x00010000,
	NPC_FLAG_BANKER             = 0x00020000,
	NPC_FLAG_PETITIONER         = 0x00040000,
	NPC_FLAG_TABARDDESIGNER     = 0x00080000,
	NPC_FLAG_BATTLEMASTER       = 0x00100000,
	NPC_FLAG_AUCTIONEER         = 0x00200000,
	NPC_FLAG_STABLEMASTER       = 0x00400000,
	NPC_FLAG_GUILD_BANKER       = 0x00800000,
	NPC_FLAG_SPELLCLICK         = 0x01000000,
};

enum Anim
{
	ANIM_STAND                         = 0,
	ANIM_DEATH                         = 1,
	ANIM_SPELL                         = 2,
	ANIM_STOP                          = 3,
	ANIM_WALK                          = 4,
	ANIM_RUN                           = 5,
	ANIM_DEAD                          = 6,
	ANIM_RISE                          = 7,
	ANIM_STANDWOUND                    = 8,
	ANIM_COMBATWOUND                   = 9,
	ANIM_COMBATCRITICAL                = 10,
	ANIM_SHUFFLE_LEFT                  = 11,
	ANIM_SHUFFLE_RIGHT                 = 12,
	ANIM_WALK_BACKWARDS                = 13,
	ANIM_STUN                          = 14,
	ANIM_HANDS_CLOSED                  = 15,
	ANIM_ATTACKUNARMED                 = 16,
	ANIM_ATTACK1H                      = 17,
	ANIM_ATTACK2HTIGHT                 = 18,
	ANIM_ATTACK2HLOOSE                 = 19,
	ANIM_PARRYUNARMED                  = 20,
	ANIM_PARRY1H                       = 21,
	ANIM_PARRY2HTIGHT                  = 22,
	ANIM_PARRY2HLOOSE                  = 23,
	ANIM_PARRYSHIELD                   = 24,
	ANIM_READYUNARMED                  = 25,
	ANIM_READY1H                       = 26,
	ANIM_READY2HTIGHT                  = 27,
	ANIM_READY2HLOOSE                  = 28,
	ANIM_READYBOW                      = 29,
	ANIM_DODGE                         = 30,
	ANIM_SPELLPRECAST                  = 31,
	ANIM_SPELLCAST                     = 32,
	ANIM_SPELLCASTAREA                 = 33,
	ANIM_NPCWELCOME                    = 34,
	ANIM_NPCGOODBYE                    = 35,
	ANIM_BLOCK                         = 36,
	ANIM_JUMPSTART                     = 37,
	ANIM_JUMP                          = 38,
	ANIM_JUMPEND                       = 39,
	ANIM_FALL                          = 40,
	ANIM_SWIMIDLE                      = 41,
	ANIM_SWIM                          = 42,
	ANIM_SWIM_LEFT                     = 43,
	ANIM_SWIM_RIGHT                    = 44,
	ANIM_SWIM_BACKWARDS                = 45,
	ANIM_ATTACKBOW                     = 46,
	ANIM_FIREBOW                       = 47,
	ANIM_READYRIFLE                    = 48,
	ANIM_ATTACKRIFLE                   = 49,
	ANIM_LOOT                          = 50,
	ANIM_SPELL_PRECAST_DIRECTED        = 51,
	ANIM_SPELL_PRECAST_OMNI            = 52,
	ANIM_SPELL_CAST_DIRECTED           = 53,
	ANIM_SPELL_CAST_OMNI               = 54,
	ANIM_SPELL_BATTLEROAR              = 55,
	ANIM_SPELL_READYABILITY            = 56,
	ANIM_SPELL_SPECIAL1H               = 57,
	ANIM_SPELL_SPECIAL2H               = 58,
	ANIM_SPELL_SHIELDBASH              = 59,
	ANIM_EMOTE_TALK                    = 60,
	ANIM_EMOTE_EAT                     = 61,
	ANIM_EMOTE_WORK                    = 62,
	ANIM_EMOTE_USE_STANDING            = 63,
	ANIM_EMOTE_EXCLAMATION             = 64,
	ANIM_EMOTE_QUESTION                = 65,
	ANIM_EMOTE_BOW                     = 66,
	ANIM_EMOTE_WAVE                    = 67,
	ANIM_EMOTE_CHEER                   = 68,
	ANIM_EMOTE_DANCE                   = 69,
	ANIM_EMOTE_LAUGH                   = 70,
	ANIM_EMOTE_SLEEP                   = 71,
	ANIM_EMOTE_SIT_GROUND              = 72,
	ANIM_EMOTE_RUDE                    = 73,
	ANIM_EMOTE_ROAR                    = 74,
	ANIM_EMOTE_KNEEL                   = 75,
	ANIM_EMOTE_KISS                    = 76,
	ANIM_EMOTE_CRY                     = 77,
	ANIM_EMOTE_CHICKEN                 = 78,
	ANIM_EMOTE_BEG                     = 79,
	ANIM_EMOTE_APPLAUD                 = 80,
	ANIM_EMOTE_SHOUT                   = 81,
	ANIM_EMOTE_FLEX                    = 82,
	ANIM_EMOTE_SHY                     = 83,
	ANIM_EMOTE_POINT                   = 84,
	ANIM_ATTACK1HPIERCE                = 85,
	ANIM_ATTACK2HLOOSEPIERCE           = 86,
	ANIM_ATTACKOFF                     = 87,
	ANIM_ATTACKOFFPIERCE               = 88,
	ANIM_SHEATHE                       = 89,
	ANIM_HIPSHEATHE                    = 90,
	ANIM_MOUNT                         = 91,
	ANIM_RUN_LEANRIGHT                 = 92,
	ANIM_RUN_LEANLEFT                  = 93,
	ANIM_MOUNT_SPECIAL                 = 94,
	ANIM_KICK                          = 95,
	ANIM_SITDOWN                       = 96,
	ANIM_SITTING                       = 97,
	ANIM_SITUP                         = 98,
	ANIM_SLEEPDOWN                     = 99,
	ANIM_SLEEPING                      = 100,
	ANIM_SLEEPUP                       = 101,
	ANIM_SITCHAIRLOW                   = 102,
	ANIM_SITCHAIRMEDIUM                = 103,
	ANIM_SITCHAIRHIGH                  = 104,
	ANIM_LOADBOW                       = 105,
	ANIM_LOADRIFLE                     = 106,
	ANIM_ATTACKTHROWN                  = 107,
	ANIM_READYTHROWN                   = 108,
	ANIM_HOLDBOW                       = 109,
	ANIM_HOLDRIFLE                     = 110,
	ANIM_HOLDTHROWN                    = 111,
	ANIM_LOADTHROWN                    = 112,
	ANIM_EMOTE_SALUTE                  = 113,
	ANIM_KNEELDOWN                     = 114,
	ANIM_KNEELING                      = 115,
	ANIM_KNEELUP                       = 116,
	ANIM_ATTACKUNARMEDOFF              = 117,
	ANIM_SPECIALUNARMED                = 118,
	ANIM_STEALTHWALK                   = 119,
	ANIM_STEALTHSTAND                  = 120,
	ANIM_KNOCKDOWN                     = 121,
	ANIM_EATING                        = 122,
	ANIM_USESTANDINGLOOP               = 123,
	ANIM_CHANNELCASTDIRECTED           = 124,
	ANIM_CHANNELCASTOMNI               = 125,
	ANIM_WHIRLWIND                     = 126,
	ANIM_BIRTH                         = 127,
	ANIM_USESTANDINGSTART              = 128,
	ANIM_USESTANDINGEND                = 129,
	ANIM_HOWL                          = 130,
	ANIM_DROWN                         = 131,
	ANIM_DROWNED                       = 132,
	ANIM_FISHINGCAST                   = 133,
	ANIM_FISHINGLOOP                   = 134,
	ANIM_FLY                           = 135,
	ANIM_EMOTE_WORK_NO_SHEATHE         = 136,
	ANIM_EMOTE_STUN_NO_SHEATHE         = 137,
	ANIM_EMOTE_USE_STANDING_NO_SHEATHE = 138,
	ANIM_SPELL_SLEEP_DOWN              = 139,
	ANIM_SPELL_KNEEL_START             = 140,
	ANIM_SPELL_KNEEL_LOOP              = 141,
	ANIM_SPELL_KNEEL_END               = 142,
	ANIM_SPRINT                        = 143,
	ANIM_IN_FIGHT                      = 144,
	ANIM_GAMEOBJ_SPAWN                 = 145,
	ANIM_GAMEOBJ_CLOSE                 = 146,
	ANIM_GAMEOBJ_CLOSED                = 147,
	ANIM_GAMEOBJ_OPEN                  = 148,
	ANIM_GAMEOBJ_OPENED                = 149,
	ANIM_GAMEOBJ_DESTROY               = 150,
	ANIM_GAMEOBJ_DESTROYED             = 151,
	ANIM_GAMEOBJ_REBUILD               = 152,
	ANIM_GAMEOBJ_CUSTOM0               = 153,
	ANIM_GAMEOBJ_CUSTOM1               = 154,
	ANIM_GAMEOBJ_CUSTOM2               = 155,
	ANIM_GAMEOBJ_CUSTOM3               = 156,
	ANIM_GAMEOBJ_DESPAWN               = 157,
	ANIM_HOLD                          = 158,
	ANIM_DECAY                         = 159,
	ANIM_BOWPULL                       = 160,
	ANIM_BOWRELEASE                    = 161,
	ANIM_SHIPSTART                     = 162,
	ANIM_SHIPMOVEING                   = 163,
	ANIM_SHIPSTOP                      = 164,
	ANIM_GROUPARROW                    = 165,
	ANIM_ARROW                         = 166,
	ANIM_CORPSEARROW                   = 167,
	ANIM_GUIDEARROW                    = 168,
	ANIM_SWAY                          = 169,
	ANIM_DRUIDCATPOUNCE                = 170,
	ANIM_DRUIDCATRIP                   = 171,
	ANIM_DRUIDCATRAKE                  = 172,
	ANIM_DRUIDCATRAVAGE                = 173,
	ANIM_DRUIDCATCLAW                  = 174,
	ANIM_DRUIDCATCOWER                 = 175,
	ANIM_DRUIDBEARSWIPE                = 176,
	ANIM_DRUIDBEARBITE                 = 177,
	ANIM_DRUIDBEARMAUL                 = 178,
	ANIM_DRUIDBEARBASH                 = 179,
	ANIM_DRAGONTAIL                    = 180,
	ANIM_DRAGONSTOMP                   = 181,
	ANIM_DRAGONSPIT                    = 182,
	ANIM_DRAGONSPITHOVER               = 183,
	ANIM_DRAGONSPITFLY                 = 184,
	ANIM_EMOTEYES                      = 185,
	ANIM_EMOTENO                       = 186,
	ANIM_JUMPLANDRUN                   = 187,
	ANIM_LOOTHOLD                      = 188,
	ANIM_LOOTUP                        = 189,
	ANIM_STANDHIGH                     = 190,
	ANIM_IMPACT                        = 191,
	ANIM_LIFTOFF                       = 192,
	ANIM_HOVER                         = 193,
	ANIM_SUCCUBUSENTICE                = 194,
	ANIM_EMOTETRAIN                    = 195,
	ANIM_EMOTEDEAD                     = 196,
	ANIM_EMOTEDANCEONCE                = 197,
	ANIM_DEFLECT                       = 198,
	ANIM_EMOTEEATNOSHEATHE             = 199,
	ANIM_LAND                          = 200,
	ANIM_SUBMERGE                      = 201,
	ANIM_SUBMERGED                     = 202,
	ANIM_CANNIBALIZE                   = 203,
	ANIM_ARROWBIRTH                    = 204,
	ANIM_GROURARROWBIRTH               = 205,
	ANIM_CORPSEARROWBIRTH              = 206,
	ANIM_GUIDEARROWBIRTH               = 207,
	ANIM_EMOTETALKNOSHEATHE            = 208,
	ANIM_EMOTEPOINTNOSHEATHE           = 209,
	ANIM_EMOTESALUTENOSHEATHE          = 210,
	ANIM_EMOTEDANCESPECIAL             = 211,
	ANIM_MUTILATE                      = 212,
	ANIM_CUSTOMSPELL01                 = 213,
	ANIM_CUSTOMSPELL02                 = 214,
	ANIM_CUSTOMSPELL03                 = 215,
	ANIM_CUSTOMSPELL04                 = 216,
	ANIM_CUSTOMSPELL05                 = 217,
	ANIM_CUSTOMSPELL06                 = 218,
	ANIM_CUSTOMSPELL07                 = 219,
	ANIM_CUSTOMSPELL08                 = 220,
	ANIM_CUSTOMSPELL09                 = 221,
	ANIM_CUSTOMSPELL10                 = 222,
	ANIM_STEALTHRUN                    = 223,
	ANIM_EMERGE                        = 224,
	ENIM_COWER                         = 225,
};

#endif
