/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// bg_public.h -- definitions shared by both the server game and client game modules

#ifndef HAT_SERVER_BG_PUBLIC_H
#define HAT_SERVER_BG_PUBLIC_H

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define	GAME_VERSION		"XreaL-rev4"	// Tr3B: always increase if you change the ET_, EV_, GT_ or WP_ types

// Tr3B: define this to use the new Quake4 like player model system
#define XPPM 1

#if defined(XPPM)
#define	DEFAULT_MODEL			"shina"
#define	DEFAULT_HEADMODEL		"shina"
#else
#define	DEFAULT_MODEL			"harley"
#define	DEFAULT_HEADMODEL		"harley"
#endif

#define	DEFAULT_GRAVITY				800	// FIXME: should be 313.92 = 9.81 * 32 SI gravity in Quake units
#define DEFAULT_GRAVITY_STRING		"800"

#if 0							//def XPPM
#define	GIB_HEALTH			0
#else
#define	GIB_HEALTH			-40
#endif

#define	ARMOR_PROTECTION	0.66

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define	ITEM_RADIUS			15	// item sizes are needed for client side pickup detection

#define ACTIVATE_LENGTH		64	// player reach for +activate

#define	LIGHTNING_RANGE		768
#define LIGHTNING_ALPHA_LIMIT 20	// JUHOX

#define	SCORE_NOT_PRESENT	-9999	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

// Tr3B: changed to HL 2 / Quake 4 properties
#define	STEPSIZE			18
#define	DEFAULT_VIEWHEIGHT	44 // 68	// Tr3B: was 26
#define CROUCH_VIEWHEIGHT	16 // 32	// Tr3B: was 12
#define CROUCH_HEIGHT		20 // 38	// Tr3B: was 16
#define	DEAD_VIEWHEIGHT	   -16			// Tr3B: was -16

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC				2
#define	CS_MESSAGE				3	// from the map worldspawn's message field
#define	CS_MOTD					4	// g_motd string for server message of the day
#define	CS_WARMUP				5	// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11

#define CS_TEAMVOTE_TIME		12
#define CS_TEAMVOTE_STRING		14
#define	CS_TEAMVOTE_YES			16
#define	CS_TEAMVOTE_NO			18

#define	CS_GAME_VERSION			20
#define	CS_LEVEL_START_TIME		21	// so the timer only shows the current level
#define	CS_INTERMISSION			22	// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS			23	// string indicating flag status in CTF
#define CS_SHADERSTATE			24
#define CS_BOTINFO				25

#define	CS_ITEMS				27	// string of 0's and 1's that tell which items are present

#define	CS_MODELS				32
#define	CS_SOUNDS				(CS_MODELS+MAX_MODELS)
#define	CS_PLAYERS				(CS_SOUNDS+MAX_SOUNDS)
#define CS_LOCATIONS			(CS_PLAYERS+MAX_CLIENTS)
#define CS_EFFECTS				(CS_LOCATIONS+MAX_LOCATIONS)

#define CS_MAX					(CS_EFFECTS+MAX_EFFECTS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum
{
    GT_FFA,						// free for all
    GT_TOURNAMENT,				// one on one tournament
    GT_SINGLE_PLAYER,			// single player ffa

//	GT_KING_OF_THE_HILL			// TODO kill the king Unreal 1 style

    //-- team games go after this --

    GT_TEAM,					// team deathmatch
    GT_CTF,						// capture the flag
    GT_1FCTF,
    GT_OBELISK,
    GT_HARVESTER,

    GT_MAX_GAME_TYPE
} gametype_t;

typedef enum
{ GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum
{
    PM_NORMAL,					// can accelerate and turn
    PM_NOCLIP,					// noclip movement
    PM_SPECTATOR,				// still run into walls
    PM_DEAD,					// no acceleration or turning, but free falling
    PM_FREEZE,					// stuck in place with no control
    PM_INTERMISSION,			// no movement or status bar
    PM_SPINTERMISSION			// no movement or status bar
} pmtype_t;

typedef enum
{
    WEAPON_READY,
    WEAPON_RAISING,
    WEAPON_DROPPING,
    WEAPON_FIRING,

    MAX_WEAPON_STATES
} weaponstate_t;

// pmove->pm_flags	16 bits
enum
{
    PMF_DUCKED						= BIT(0),
    PMF_JUMP_HELD					= BIT(1),
    PMF_BACKWARDS_JUMP				= BIT(2),	// go into backwards land
    PMF_BACKWARDS_RUN				= BIT(3),	// coast down to backwards run
    PMF_TIME_LAND					= BIT(4),	// pm_time is time before rejump
    PMF_TIME_KNOCKBACK				= BIT(5),	// pm_time is an air-accelerate only time
    PMF_TIME_WATERJUMP				= BIT(6),	// pm_time is waterjump
    PMF_RESPAWNED					= BIT(7),	// clear after attack and jump buttons come up
    PMF_USE_ITEM_HELD				= BIT(8),
    PMF_GRAPPLE_PULL				= BIT(9),	// pull towards grapple location
    PMF_FOLLOW						= BIT(10),	// spectate following another player
    PMF_SCOREBOARD					= BIT(11),	// spectate as a scoreboard
    PMF_INVULEXPAND					= BIT(12),	// invulnerability sphere set to full size
    PMF_WALLCLIMBING				= BIT(13),
    PMF_WALLCLIMBINGCEILING			= BIT(14),

    PMF_ALL_TIMES					= (PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_KNOCKBACK)
};

extern const vec3_t playerMins;
extern const vec3_t playerMaxs;

#define	MAXTOUCH	32
typedef struct
{
    // state (in / out)
    playerState_t  *ps;

    // command (in)
    usercmd_t       cmd;
    int             tracemask;	// collide against these types of surfaces
    int             debugLevel;	// if set, diagnostic output will be printed
    int             airControl;	// if set, air control will be allowed
    int             fastWeaponSwitches;	// if set, weapon lower and raise animations will be skipped
    qboolean        noFootsteps;	// if the game is setup for no footsteps by the server
    qboolean        gauntletHit;	// true if a gauntlet attack would actually hit something

    int             framecount;

    // results (out)
    int             numtouch;
    int             touchents[MAXTOUCH];

    vec3_t          mins, maxs;	// bounding box size

    int             watertype;
    int             waterlevel;

    float           xyspeed;

    // fixed pmove
    int             fixedPmove;
    int             fixedPmoveFPS;

    // leaning
    int				allowLeaning;
    int				allowLeaningWithMovement;

    // callbacks to test the world
    // these will be different functions during game and cgame
    void            (*trace) (trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
                              int passEntityNum, int contentMask);
    int             (*pointcontents) (const vec3_t point, int passEntityNum);
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void            PM_UpdateViewAngles(playerState_t * ps, usercmd_t * cmd);
void            Pmove(pmove_t * pmove);

//===================================================================================


// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum
{
    STAT_HEALTH,
    STAT_HOLDABLE_ITEM,
#ifdef MISSIONPACK
    STAT_PERSISTANT_POWERUP,
#endif
    STAT_WEAPONS,				// 16 bit fields
    STAT_ARMOR,
    STAT_DEAD_YAW,				// look this direction when dead (FIXME: get rid of?)
    STAT_CLIENTS_READY,			// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
    STAT_MAX_HEALTH				// health / armor limit, changable by handicap
} statIndex_t;


// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum
{
    PERS_SCORE,					// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
    PERS_HITS,					// total points damage inflicted so damage beeps can sound on change
    PERS_RANK,					// player rank or team rank
    PERS_TEAM,					// player team
    PERS_SPAWN_COUNT,			// incremented every respawn
    PERS_PLAYEREVENTS,			// 16 bits that can be flipped for events
    PERS_ATTACKER,				// clientnum of last damage inflicter
    PERS_ATTACKEE_ARMOR,		// health/armor of last person we attacked
    PERS_KILLED,				// count of the number of times you died
    // player awards tracking
    PERS_IMPRESSIVE_COUNT,		// two railgun hits in a row
    PERS_EXCELLENT_COUNT,		// two successive kills in a short amount of time
    PERS_DEFEND_COUNT,			// defend awards
    PERS_ASSIST_COUNT,			// assist awards
    PERS_GAUNTLET_FRAG_COUNT,	// kills with the guantlet
    PERS_CAPTURES,				// captures
    PERS_TELEFRAG_FRAG_COUNT	// kills by telefragging
} persEnum_t;


// entityState_t->eFlags
// *INDENT-OFF*
enum
{
    EF_DEAD					= BIT(0),	// don't draw a foe marker over players with EF_DEAD
    EF_TELEPORT_BIT			= BIT(1),	// toggled every time the origin abruptly changes
    EF_AWARD_EXCELLENT		= BIT(2),	// draw an excellent sprite
    EF_PLAYER_EVENT			= BIT(3),
    EF_BOUNCE				= BIT(4),	// for missiles
    EF_BOUNCE_HALF			= BIT(5),	// for missiles
    EF_AWARD_GAUNTLET		= BIT(6),	// draw a gauntlet sprite
    EF_NODRAW				= BIT(7),	// may have an event, but no model (unspawned items)
    EF_FIRING				= BIT(8),	// for lightning gun
    EF_MOVER_STOP			= BIT(9),	// will push otherwise
    EF_AWARD_CAP			= BIT(10),	// draw the capture sprite
    EF_TALK					= BIT(11),	// draw a talk balloon
    EF_CONNECTION			= BIT(12),	// draw a connection trouble sprite
    EF_VOTED				= BIT(13),	// already cast a vote
    EF_AWARD_IMPRESSIVE		= BIT(14),	// draw an impressive sprite
    EF_AWARD_DEFEND			= BIT(15),	// draw a defend sprite
    EF_AWARD_ASSIST			= BIT(16),	// draw a assist sprite
    EF_AWARD_DENIED			= BIT(17),	// denied
    EF_AWARD_TELEFRAG		= BIT(18),	// draw a telefrag sprite
    EF_TEAMVOTED			= BIT(19),	// already cast a team vote
    EF_KAMIKAZE				= BIT(20),
    EF_TICKING				= BIT(21),	// used to make players play the prox mine ticking sound
    EF_FIRING2				= BIT(22),	// for lightning gun
    EF_WALLCLIMB			= BIT(23),	// TA: wall walking
    EF_WALLCLIMBCEILING		= BIT(24),	// TA: wall walking ceiling hack
};
// *INDENT-ON*

// NOTE: may not have more than 16
typedef enum
{
    PW_NONE,

    PW_QUAD,
    PW_BATTLESUIT,
    PW_HASTE,
    PW_INVIS,
    PW_REGEN,
    PW_FLIGHT,

    PW_REDFLAG,
    PW_BLUEFLAG,
    PW_NEUTRALFLAG,

    PW_SCOUT,
    PW_GUARD,
    PW_DOUBLER,
    PW_AMMOREGEN,
    PW_INVULNERABILITY,

    PW_NUM_POWERUPS
} powerup_t;

typedef enum
{
    HI_NONE,

    HI_TELEPORTER,
    HI_MEDKIT,
    HI_KAMIKAZE,
    HI_PORTAL,
    HI_INVULNERABILITY,

    HI_NUM_HOLDABLE
} holdable_t;


typedef enum
{
    WP_NONE,

    WP_GAUNTLET,
    WP_MACHINEGUN,
    WP_SHOTGUN,
    WP_FLAK_CANNON,
    WP_ROCKET_LAUNCHER,
    WP_LIGHTNING,
    WP_RAILGUN,
    WP_PLASMAGUN,
    WP_BFG,
#ifdef MISSIONPACK
    WP_PROX_LAUNCHER,
    WP_CHAINGUN,
#endif

    WP_NUM_WEAPONS
} weapon_t;


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define	PLAYEREVENT_DENIEDREWARD		0x0001
#define	PLAYEREVENT_GAUNTLETREWARD		0x0002
#define PLAYEREVENT_HOLYSHIT			0x0004
#define PLAYEREVENT_TELEFRAGREWARD		0x0008

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum
{
    EV_NONE,

    EV_FOOTSTEP,
    EV_FOOTSTEP_METAL,
    EV_FOOTSTEP_WALLWALK,
    EV_FOOTSPLASH,
    EV_FOOTWADE,
    EV_SWIM,

    EV_STEP_4,
    EV_STEP_8,
    EV_STEP_12,
    EV_STEP_16,

    EV_STEPDN_4,
    EV_STEPDN_8,
    EV_STEPDN_12,
    EV_STEPDN_16,

    EV_FALL_SHORT,
    EV_FALL_MEDIUM,
    EV_FALL_FAR,

    EV_JUMP_PAD,				// boing sound at origin, jump sound on player

    EV_JUMP,
    EV_WATER_TOUCH,				// foot touches
    EV_WATER_LEAVE,				// foot leaves
    EV_WATER_UNDER,				// head touches
    EV_WATER_CLEAR,				// head leaves

    EV_ITEM_PICKUP,				// normal item pickups are predictable
    EV_GLOBAL_ITEM_PICKUP,		// powerup / team sounds are broadcast to everyone

    EV_NOAMMO,
    EV_CHANGE_WEAPON,
    EV_FIRE_WEAPON,
    EV_FIRE_WEAPON2,			// Tr3B: new

    EV_USE_ITEM0,
    EV_USE_ITEM1,
    EV_USE_ITEM2,
    EV_USE_ITEM3,
    EV_USE_ITEM4,
    EV_USE_ITEM5,
    EV_USE_ITEM6,
    EV_USE_ITEM7,
    EV_USE_ITEM8,
    EV_USE_ITEM9,
    EV_USE_ITEM10,
    EV_USE_ITEM11,
    EV_USE_ITEM12,
    EV_USE_ITEM13,
    EV_USE_ITEM14,
    EV_USE_ITEM15,

    EV_ITEM_RESPAWN,
    EV_ITEM_POP,
    EV_PLAYER_TELEPORT_IN,
    EV_PLAYER_TELEPORT_OUT,

    EV_GRENADE_BOUNCE,			// eventParm will be the soundindex

    EV_GENERAL_SOUND,
    EV_GLOBAL_SOUND,			// no attenuation
    EV_GLOBAL_TEAM_SOUND,

    EV_BULLET_HIT_FLESH,
    EV_BULLET_HIT_WALL,

    EV_PROJECTILE_HIT,
    EV_PROJECTILE_MISS,
    EV_PROJECTILE_MISS_METAL,

    EV_RAILTRAIL,
    EV_SHOTGUN,
    EV_BULLET,					// otherEntity is the shooter

    EV_PAIN,
    EV_DEATH1,
    EV_DEATH2,
    EV_DEATH3,
    EV_OBITUARY,

    EV_POWERUP_QUAD,
    EV_POWERUP_BATTLESUIT,
    EV_POWERUP_REGEN,

    EV_GIB_PLAYER,				// gib a previously living player
    EV_SCOREPLUM,				// score plum

//#ifdef MISSIONPACK
    EV_PROXIMITY_MINE_STICK,
    EV_PROXIMITY_MINE_TRIGGER,
    EV_KAMIKAZE,				// kamikaze explodes
    EV_RAILEXLOSION,
    EV_OBELISKEXPLODE,			// obelisk explodes
    EV_OBELISKPAIN,				// obelisk is in pain
    EV_INVUL_IMPACT,			// invulnerability sphere impact
    EV_JUICED,					// invulnerability juiced effect
    EV_LIGHTNINGBOLT,			// lightning bolt bounced of invulnerability sphere
//#endif

    EV_EFFECT,					// Lua scripted special effect

    EV_EXPLODE,

    EV_DEBUG_LINE,
    EV_STOPLOOPINGSOUND,
    EV_TAUNT,
    EV_TAUNT_YES,
    EV_TAUNT_NO,
    EV_TAUNT_FOLLOWME,
    EV_TAUNT_GETFLAG,
    EV_TAUNT_GUARDBASE,
    EV_TAUNT_PATROL
} entity_event_t;


typedef enum
{
    GTS_RED_CAPTURE,
    GTS_BLUE_CAPTURE,
    GTS_RED_RETURN,
    GTS_BLUE_RETURN,
    GTS_RED_TAKEN,
    GTS_BLUE_TAKEN,
    GTS_REDOBELISK_ATTACKED,
    GTS_BLUEOBELISK_ATTACKED,
    GTS_REDTEAM_SCORED,
    GTS_BLUETEAM_SCORED,
    GTS_REDTEAM_TOOK_LEAD,
    GTS_BLUETEAM_TOOK_LEAD,
    GTS_TEAMS_ARE_TIED,
    GTS_KAMIKAZE
} global_team_sound_t;


typedef enum
{
    BOTH_DEATH1,
    BOTH_DEAD1,
    BOTH_DEATH2,
    BOTH_DEAD2,
    BOTH_DEATH3,
    BOTH_DEAD3,

    TORSO_GESTURE,

    TORSO_ATTACK,
    TORSO_ATTACK2,

    TORSO_DROP,
    TORSO_RAISE,

    TORSO_STAND,
    TORSO_STAND2,

    LEGS_WALKCR,
    LEGS_WALK,
    LEGS_RUN,
    LEGS_BACK,
    LEGS_SWIM,

    LEGS_JUMP,
    LEGS_LAND,

    LEGS_JUMPB,
    LEGS_LANDB,

    LEGS_IDLE,
    LEGS_IDLECR,

    LEGS_TURN,

    TORSO_GETFLAG,
    TORSO_GUARDBASE,
    TORSO_PATROL,
    TORSO_FOLLOWME,
    TORSO_AFFIRMATIVE,
    TORSO_NEGATIVE,

    LEGS_BACKCR,
    LEGS_BACKWALK,

    MAX_PLAYER_ANIMATIONS
} playerAnimNumber_t;

typedef enum
{
    FLAG_IDLE,
    FLAG_RUN,

    MAX_FLAG_ANIMATIONS
} flagAnimNumber_t;

typedef struct animation_s
{

    qhandle_t       handle;		// registered md5Animation or whatever
    qboolean        clearOrigin;	// reset the origin bone

    int             firstFrame;
    int             numFrames;
    int             loopFrames;	// 0 to numFrames
    int             frameTime;	// msec between frames
    int             initialLerp;	// msec to get to first frame
    int             reversed;	// true if animation is reversed
    int             flipflop;	// true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define	ANIM_TOGGLEBIT		128


typedef enum
{
    TEAM_FREE,
    TEAM_RED,
    TEAM_BLUE,
    TEAM_SPECTATOR,

    TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		32

//team task
typedef enum
{
    TEAMTASK_NONE,
    TEAMTASK_OFFENSE,
    TEAMTASK_DEFENSE,
    TEAMTASK_PATROL,
    TEAMTASK_FOLLOW,
    TEAMTASK_RETRIEVE,
    TEAMTASK_ESCORT,
    TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum
{
    MOD_UNKNOWN,
    MOD_SHOTGUN,
    MOD_GAUNTLET,
    MOD_MACHINEGUN,
    MOD_GRENADE,
    MOD_GRENADE_SPLASH,
    MOD_ROCKET,
    MOD_ROCKET_SPLASH,
    MOD_PLASMA,
    MOD_PLASMA_SPLASH,
    MOD_RAILGUN,
    MOD_RAILGUN_SPLASH,
    MOD_LIGHTNING,
    MOD_BFG,
    MOD_BFG_SPLASH,
    MOD_WATER,
    MOD_SLIME,
    MOD_LAVA,
    MOD_CRUSH,
    MOD_TELEFRAG,
    MOD_FALLING,
    MOD_SUICIDE,
    MOD_TARGET_LASER,
    MOD_TRIGGER_HURT,
    MOD_NAIL,
#ifdef MISSIONPACK
    MOD_CHAINGUN,
    MOD_PROXIMITY_MINE,
#endif
    MOD_KAMIKAZE,
#ifdef MISSIONPACK
    MOD_JUICED,
#endif
    MOD_GRAPPLE,
    MOD_BURN
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum
{
    IT_BAD,
    IT_WEAPON,					// EFX: rotate + upscale + minlight
    IT_AMMO,					// EFX: rotate
    IT_ARMOR,					// EFX: rotate + minlight
    IT_HEALTH,					// EFX: static external sphere + rotating internal
    IT_POWERUP,					// instant on, timer based
    // EFX: rotate + external ring that rotates
    IT_HOLDABLE,				// single use, holdable item
    // EFX: rotate + bob
    IT_PERSISTANT_POWERUP,
    IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct gitem_s
{
    char           *classname;	// spawning name
    char           *pickup_sound;
    char           *models[MAX_ITEM_MODELS];
    char           *skins[MAX_ITEM_MODELS];
    char           *icon;
    char           *pickup_name;	// for printing on pickup

    int             quantity;	// for ammo how much, or duration of powerup
    itemType_t      giType;		// IT_* flags

    int             giTag;

    char           *precaches;	// string of all models and images this item will use
    char           *sounds;		// string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern gitem_t  bg_itemlist[];
extern int      bg_numItems;

gitem_t        *BG_FindItem(const char *pickupName);
gitem_t        *BG_FindItemForClassname(const char *className);
gitem_t        *BG_FindItemForWeapon(weapon_t weapon);
gitem_t        *BG_FindItemForPowerup(powerup_t pw);
gitem_t        *BG_FindItemForHoldable(holdable_t pw);

#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean        BG_CanItemBeGrabbed(int gametype, const entityState_t * ent, const playerState_t * ps);


// g_dmflags->integer flags
#define	DF_NO_FALLING			8
#define DF_FIXED_FOV			16
#define	DF_NO_FOOTSTEPS			32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_BOTSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY|CONTENTS_BOTCLIP)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE|CONTENTS_SHOOTABLE)


//
// entityState_t->eType
//
typedef enum
{
    ET_GENERAL,
    ET_PLAYER,
    ET_ITEM,
    ET_PROJECTILE,
    ET_PROJECTILE2,
    ET_MOVER,
    ET_BEAM,
    ET_PORTAL,
    ET_SPEAKER,
    ET_PUSH_TRIGGER,
    ET_TELEPORT_TRIGGER,
    ET_INVISIBLE,
    ET_GRAPPLE,					// grapple hooked on wall
    ET_TEAM,
    ET_AI_NODE,					// AI visualization tool
    ET_AI_LINK,
    ET_EXPLOSIVE,
    ET_FIRE,
    ET_PHYSICS_BOX,				// JBullet visualization tool
    ET_GUI,

    ET_EVENTS					// any of the EV_* events can be added freestanding
        // by setting eType to ET_EVENTS + eventNum
        // this avoids having to set eFlags and eventNum
} entityType_t;



void            BG_EvaluateTrajectory(const trajectory_t * tr, int atTime, vec3_t result);
void            BG_EvaluateTrajectoryDelta(const trajectory_t * tr, int atTime, vec3_t result);

void            BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t * ps);

void            BG_TouchJumpPad(playerState_t * ps, entityState_t * jumppad);

void            BG_PlayerStateToEntityState(playerState_t * ps, entityState_t * s, qboolean snap);
void            BG_PlayerStateToEntityStateExtraPolate(playerState_t * ps, entityState_t * s, int time, qboolean snap);

qboolean        BG_PlayerTouchesItem(playerState_t * ps, entityState_t * item, int atTime);

qboolean        BG_RotateAxis(vec3_t surfNormal, vec3_t inAxis[3], vec3_t outAxis[3], qboolean inverse, qboolean ceiling);


#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		8192

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		8192


// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME		0
#define KAMI_SHOCKWAVEFADE_STARTTIME	1500
#define KAMI_SHOCKWAVE_ENDTIME			2000

// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME			250
#define KAMI_IMPLODE_STARTTIME			2000
#define KAMI_IMPLODE_ENDTIME			2250

// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME		2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME	2500
#define KAMI_SHOCKWAVE2_ENDTIME			3000

// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS		88
#define KAMI_BOOMSPHEREMODEL_RADIUS		72

// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS		1320
#define KAMI_BOOMSPHERE_MAXRADIUS		720
#define KAMI_SHOCKWAVE2_MAXRADIUS		704



// Railgun

// 1st shockwave times
#define RAILGUN_SHOCKWAVE_STARTTIME		0
#define RAILGUN_SHOCKWAVEFADE_STARTTIME	150
#define RAILGUN_SHOCKWAVE_ENDTIME		200

// explosion/implosion times
#define RAILGUN_EXPLODE_STARTTIME		25
#define RAILGUN_IMPLODE_STARTTIME		200
#define RAILGUN_IMPLODE_ENDTIME			225

// 2nd shockwave times
#define RAILGUN_SHOCKWAVE2_STARTTIME		200
#define RAILGUN_SHOCKWAVE2FADE_STARTTIME	250
#define RAILGUN_SHOCKWAVE2_ENDTIME			300

// radius of the models without scaling
#define RAILGUN_SHOCKWAVEMODEL_RADIUS	40
#define RAILGUN_BOOMSPHEREMODEL_RADIUS	32

// maximum radius of the models during the effect
#define RAILGUN_SHOCKWAVE_MAXRADIUS		132
#define RAILGUN_BOOMSPHERE_MAXRADIUS	72
#define RAILGUN_SHOCKWAVE2_MAXRADIUS	70


// entity->materialType
// DerSaidin 2009-01-15
typedef enum
{
    ENTMAT_NONE,
    ENTMAT_WOOD,				//chunks, thin shards
    ENTMAT_GLASS,				//large flat shards
    ENTMAT_METAL,
    ENTMAT_GIBS,				//blood, small chunks
    ENTMAT_BODY,				//head, arms, legs
    ENTMAT_BRICK,				//rectangular chunks
    ENTMAT_STONE,				//rough chunks
    ENTMAT_TILES,				//small square chunks
    ENTMAT_PLASTER,				//thin like riped paper chunks
    ENTMAT_FIBERS,				//thin wires
    ENTMAT_SPRITE,				//sprites, down
    ENTMAT_SMOKE,				//sprites, up
    ENTMAT_GAS,					//sprites, every direction
    ENTMAT_FIRE,				//many polys on random angles
    ENTMAT_NUMBER
} entityMaterial_t;

#endif // HAT_SERVER_BG_PUBLIC_H
