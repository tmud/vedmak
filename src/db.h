/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD	0
#define DB_BOOT_MOB	1
#define DB_BOOT_OBJ	2
#define DB_BOOT_ZON	3
#define DB_BOOT_SHP	4
#define DB_BOOT_HLP	5
#define DB_BOOT_TRG	6
#define DB_BOOT_QST 	7
#define DB_BOOT_SOCIAL  8
#define DB_BOOT_SOCIAL2  9

#define MAX_SAVED_ITEMS      500
#define OBJECT_SAVE_ACTIVITY 300
#define PLAYER_SAVE_ACTIVITY 300
#if defined(CIRCLE_MACINTOSH)
#define LIB_WORLD	":world:"
#define LIB_TEXT	":text:"
#define LIB_TEXT_HELP	":text:help:"
#define LIB_MISC	":misc:"
#define LIB_ETC		":etc:"
#define LIB_PLRTEXT	":plrtext:"
#define LIB_PLROBJS	":plrobjs:"
#define LIB_PLRALIAS	":plralias:"
#define LIB_HOUSE	":house:"
#define SLASH		":"
#elif defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN) || defined(CIRCLE_VMS)
#define LIB_WORLD	"world/"
#define LIB_TEXT	"text/"
#define LIB_TEXT_HELP	"text/help/"
#define LIB_MISC	"misc/"
#define LIB_GODS	"gods/"
#define LIB_ETC		"etc/"
#define LIB_PLRTEXT	"plrtext/"
#define LIB_PLROBJS	"plrobjs/"
#define LIB_PLRALIAS	"plralias/"
#define LIB_HOUSE	"house/"
#define LIB_PLRS        "plrs/"
#define LIB_PLRVARS	"plrvars/"
#define SLASH		"/"
#define LIB             "lib/"
#define CLAN_FILE       "plrs/clans" 
#else
#error "Unknown path components."
#endif


#define SUF_OBJS	"objs"
#define TEXT_SUF_OBJS	"textobjs"
#define TIME_SUF_OBJS	"timeobjs"
#define SUF_TEXT	"text"
#define SUF_ALIAS	"alias"
#define SUF_MEM		"mem"
#define SUF_PLAYER  	"player"
#define SUF_PKILLER 	"pkiller"
#define SUF_QUESTS  	"quests"
#define SUF_PMKILL	"mobkill"
#define SUF_PFILE	"pfile"
#define SUF_PLOG	"plog"
#define SUF_CHEST	"chest" // склад имя файла где сохраняем
#define SUF_EXCHANGE "exchange"
#define SUF_MAIL "mail"

#if defined(CIRCLE_AMIGA)
#define FASTBOOT_FILE   "/.fastboot"    /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "/.killscript"  /* autorun: shut mud down       */
#define PAUSE_FILE      "/pause"        /* autorun: don't restart mud   */
#elif defined(CIRCLE_MACINTOSH)
#define FASTBOOT_FILE	"::.fastboot"	/* autorun: boot without sleep	*/
#define KILLSCRIPT_FILE	"::.killscript"	/* autorun: shut mud down	*/
#define PAUSE_FILE	"::pause"	/* autorun: don't restart mud	*/
#else
#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */
#endif

/* names of various files and directories */
#define INDEX_FILE		"index"		/* index of world files		*/
#define MINDEX_FILE		"index.mini"	/* ... and for mini-mud-mode	*/
#define WLD_PREFIX		LIB_WORLD"wld"SLASH	/* room definitions	*/
#define MOB_PREFIX		LIB_WORLD"mob"SLASH	/* monster prototypes	*/
#define OBJ_PREFIX		LIB_WORLD"obj"SLASH	/* object prototypes	*/
#define ZON_PREFIX		LIB_WORLD"zon"SLASH	/* zon defs & command tables */
#define SHP_PREFIX		LIB_WORLD"shp"SLASH	/* shop definitions	*/
#define HLP_PREFIX		LIB_TEXT"help"SLASH	/* for HELP <keyword>	*/
#define TRG_PREFIX		LIB_WORLD"trg"SLASH /* файл для тригеров*/

#define SOC_PREFIX	LIB_MISC

#define PLAYER_F_PREFIX LIB_PLRS""LIB_F
#define PLAYER_K_PREFIX LIB_PLRS""LIB_K
#define PLAYER_P_PREFIX LIB_PLRS""LIB_P
#define PLAYER_U_PREFIX LIB_PLRS""LIB_U
#define PLAYER_Z_PREFIX LIB_PLRS""LIB_Z

#define CREDITS_FILE		LIB_TEXT"credits"/* for the 'credits' command	*/
#define NEWS_FILE		LIB_TEXT"news"	/* for the 'news' command	*/
#define MOTD_FILE		LIB_TEXT"motd"	/* messages of the day / mortal	*/
#define IMOTD_FILE		LIB_TEXT"imotd"	/* messages of the day / immort	*/
#define GREETINGS_FILE		LIB_TEXT"greetings"	/* The opening screen.	*/
#define HELP_PAGE_FILE		LIB_TEXT_HELP"screen" /* for HELP <CR>		*/
#define INFO_FILE		LIB_TEXT"info"		/* for INFO		*/
#define WIZLIST_FILE		LIB_TEXT"wizlist"	/* for WIZLIST		*/
#define IMMLIST_FILE		LIB_TEXT"immlist"	/* for IMMLIST		*/
#define BACKGROUND_FILE		LIB_TEXT"background"/* for the background story	*/
#define POLICIES_FILE		LIB_TEXT"policies" /* player policies/rules	*/
#define HANDBOOK_FILE		LIB_TEXT"handbook" /* handbook for new immorts	*/
#define ZONY_FILE		LIB_TEXT"zony" /* all zones                     */

#define IDEA_FILE		LIB_MISC"ideas"	/* for the 'idea'-command	*/
#define TYPO_FILE		LIB_MISC"typos"	/*         'typo'		*/
#define BUG_FILE		LIB_MISC"bugs"	/*         'bug'		*/
#define MESS_FILE		LIB_MISC"messages" /* damage messages		*/
#define SOCMESS_FILE		LIB_MISC"socials" /* messgs for social acts	*/
#define XNAME_FILE		LIB_MISC"xnames" /* invalid name substrings	*/

#define PLAYER_FILE		LIB_ETC"players" /* the player database		*/
#define MAIL_FILE		LIB_ETC"plrmail" /* for the mudmail system	*/
#define BAN_FILE		LIB_ETC"badsites" /* for the siteban system	*/
#define HCONTROL_FILE		LIB_ETC"hcontrol"  /* for the house system	*/
#define PKILLER_FILE		LIB_ETC"pkillers" /* the player database		*/
#define PROXI_FILE		LIB_ETC"proxisites" //прокси файл

#define USE_SINGLE_PLAYER 1 //новая система запомнинания в фале игроков..
/* public procedures in db.c */
void	boot_db(void);
void	unload_db(void);
int	create_entry(char *name);
void	zone_update(void);
room_rnum real_room(room_vnum vnum);
char	*fread_string(FILE *fl, char *error);
long	get_id_by_name(char *name);
CHAR_DATA *get_char_by_id(int id);
char	*get_name_by_id(long id);
struct char_data *get_mob_by_id(long id);
struct char_data *get_char_cmp_id(long id);
int     load_char(char *name, struct char_data * char_element);//новое сохранение
void	save_char(struct char_data *ch, room_rnum load_room);
void	init_char(struct char_data *ch);
struct char_data* create_char(void);
struct char_data *read_mobile(mob_vnum nr, int type);
mob_rnum real_mobile(mob_vnum vnum);
int	vnum_mobile(char *searchname, struct char_data *ch);
void	clear_char(struct char_data *ch);
void	reset_char(struct char_data *ch);
void	free_char(struct char_data *ch);
int		correct_unique(int unique);

struct obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
obj_rnum real_object(obj_vnum vnum);
struct obj_data *read_object(obj_vnum nr, int type);
int	vnum_object(char *searchname, struct char_data *ch);
int	vnum_search(char *searchname, struct char_data *ch);
int	vnum_affect(char *searchname, struct char_data *ch);
long	cmp_ptable_by_name(char *name, int len);
char	*get_name_by_unique(long id);

#define REAL 0
#define VIRTUAL 1


extern CHAR_DATA *combat_list;
extern const int sunrise[][2];
extern const int Reverse[];
extern const FLAG_DATA clear_flags;

/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   int	arg1;		/*                                      */
   int	arg2;		/* Arguments to the command             */
   int	arg3;		/*                                      */
   int  arg4;		/* Процент лоада вещей					*/
   int line;		/* line number this command appears on  */
   char *sarg1;		/* string argument                      */
   char *sarg2;		/* string argument                      */
   /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
	*  'T': Trigger command   *	
   */
};



/* zone definition structure. for the 'zone-table'   */
struct zone_data {
   char	*name;		    /* name of this zone                  */
   int	lifespan;           /* how long between resets (minutes)  */
   ubyte level;		   //  Уровень зоны.	
   int	age;                /* current age of this zone (minutes) */
   room_vnum top;           /* upper limit for rooms in this zone */
   char *aftor;				/* Автор зоны */

   int	reset_mode;         /* conditions for reset (see below)   */
   zone_vnum number;	    /* virtual number of this zone	  */
   struct reset_com *cmd;   /* command table for reset	          */

   /*
    * Reset mode:
    *   0: Don't reset, and don't update age.
    *   1: Reset if no PC's are located in zone.
    *   2: Just reset.
    */
};



/* for queueing zones for update   */
struct reset_q_element {
   zone_rnum zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};



/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};



struct player_index_element {
   char	   *name;
   long	   id;
   long    unique;
   int     level;
   int     last_logon;
   int     activity;         // When player be saved and checked
   struct  save_info *timer;
};


struct help_index_element {
   char	*keyword;
   char *entry;
   int duplicate;
};


/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define SEASON_WINTER		0
#define SEASON_SPRING		1
#define SEASON_SUMMER		2
#define SEASON_AUTUMN		3

#define MONTH_JANUARY   	0
#define MONTH_FEBRUARY  	1
#define MONTH_MART			2
#define MONTH_APRIL			3
#define MONTH_MAY			4
#define MONTH_JUNE			5
#define MONTH_JULY			6
#define MONTH_AUGUST		7
#define MONTH_SEPTEMBER		8
#define MONTH_OCTOBER		9
#define MONTH_NOVEMBER		10
#define MONTH_DECEMBER		11
#define DAYS_PER_WEEK		7

struct month_temperature_type { 
  int min;
  int max;
  int med;
};

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   struct ban_list_element *next;
};

struct proxi_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   struct proxi_list_element *next;
};




//таблица, которая позволяет выбирать из рандома от 0 до 42,
//значения, которые используются в дальнейшем в наборе очков
//для использования в расчетах какой дамаг и какие аффекты
//будут вешаться на предмет.
static struct TablObjRand ObjRand[] = 
{    // 1d  2d Base     WW        m_aff		       cost    add    coef	
	{3, 4,	9,	10,	AFF_INVISIBLE,		39,	1,	1},//0
	{2, 7,	10,	10,	AFF_SENSE_LIFE,		51,	1,	1},
	{2, 8,	11,	11,	AFF_NOTRACK,		42,	1,	1},
	{3, 5,	12,	11,	AFF_SNEAK,		46,	1,	1},
	{6, 2,	13,	12,	AFF_DETECT_ALIGN,	32,	1,	1},
	{2, 9,	14,	12,	AFF_DETECT_INVIS,	35,	1,	1},//5
	{4, 4,	15,	13,	AFF_DETECT_MAGIC,	6,	1,	1},
	{5, 3,	16,	13,	AFF_WATERWALK,		7,	1,	1},
	{3, 6,	17,	14,	AFF_SANCTUARY,		5,	1,	1},
	{7, 2,	18,	14,	AFF_CURSE,		8,	1,	1},//
	{2,10,	19,	15,	AFF_INFRAVISION,	15,	1,	1},//10
	{2,11,	20,	15,	AFF_HIDE,		7,	1,	1},
	{3, 7,	21,	16,	AFF_WATERBREATH,	11,	1,	1},
	{4, 5,	22,	17,	AFF_BLESS,		10,	1,	1},
	{5, 4,	23,	18,	AFF_FLY,		15,	1,	1},
	{6, 3,	24,	19,	AFF_AWARNESS,		10,	1,	1},//15
	{2,12,	25,	20,	AFF_BLINK,		15,	1,	1},
	{3, 8,	26,	21,	AFF_NOFLEE,		5,	1,	1},
	{4, 6,	27,	22,	AFF_HOLYLIGHT,		14,	1,	1},
	{3, 9,	28,	23,	AFF_SINGLELIGHT,	9,	1,	1},
	{5, 5,	29,	24,	AFF_HOLYDARK,		13,	1,	1},//20
	{6, 4,	30,	25,	AFF_DETECT_POISON,	5,	1,	1},
	{4, 7,	31,	26,	AFF_HASTE,		10,	1,	1},

	{4, 8,	33,	28,	APPLY_STR,		15,	1,	3},
	{4, 9,	34,	29,	APPLY_DEX,		15,	1,	3},// * 
	{5, 7,	35,	30,	APPLY_INT,		15,	1,	3},// * 25
	{6, 6,	36,	31,	APPLY_WIS,		15,	1,	3},// *
	{7, 5,	37,	32,	APPLY_CON,		15,	1,	3},// *
	{5, 8,	38,	33,	APPLY_CHA,		12,	1,	2},// * за стат
	{6, 7,	39,	34,	APPLY_MANAREG,		5,	50,	1},//
	{7, 6,	40,	35,	APPLY_HIT,		10,	60,	1},//+ за стат 30
	{5, 9,	41,	36,	APPLY_AC,		10,	-40, 	2},//+ за стат - это общий АС на чаре, а не полокационный что используется дополнительно
	{6, 8,	42,	37,	APPLY_HITROLL,		10,	3,	2},//*
	{7, 7,	43,	38,	APPLY_DAMROLL,		8,	3,	2},//*
	{6, 9,	45,	39,	APPLY_SAVING_WILL,	5,	-5,	4},//+ за стат 
	{7, 8,	49,	40,	APPLY_RESIST_FIRE,	5,	5,	4},//+ за стат 35
	{9, 6,	51,	41,	APPLY_SAVING_REFLEX,	5,	-5,	4},//+ за стат
	{8, 7,	53,	42, 	APPLY_SAVING_CRITICAL,	5,	-5,	4},//+ за стат
	{5,12,	55,	43,	APPLY_SAVING_STABILITY,	5,	-5,	4},//+ за стат
	{11,5,	57,	44,	APPLY_HITREG,		5,	5,	4},//+ за стат 
	{7, 9,	58,	45,	APPLY_ARESIST,		7,	4,	7},//+ за стат 40 надо будет MAGIC_RESIST доделать по человечески.
	{8, 8,	59,	46,	APPLY_CAST_SUCCESS,	12,	3,	2},//* за стат
	{7,10,	60,	47,	APPLY_MORALE,		10,	6,	7},//+ за стат
	{0, 0,	0,	0,	APPLY_RESIST_WATER,	5,	5,	4},//+ за стат 
	{0, 0,	0,	0,	APPLY_RESIST_EARTH,	5,	5,	4},//+ за стат
	{0, 0,	0,	0,	APPLY_RESIST_VITALITY,	5,	5,	4},//+ за стат 45
	{0, 0,	0,	0,	APPLY_RESIST_MIND,	5,	5,	4},//+ за стат
	{0, 0,	0,	0,	APPLY_RESIST_IMMUNITY,	5,	5,	4},//+ за стат
	{0, 0,	0,	0,	APPLY_C1,		16,	1,	16},//+за стат
	{0, 0,	0,	0,	APPLY_C2,		17,	1,	17},//+за стат
	{0, 0,	0,	0,	APPLY_C3,		18,	1,	18},//+за стат 50 НУЛИ ЗАПОЛНИТЬ ПОЭТОМУ И ДАМАГ НУЛЕВОЙ ПРИСВАИВаЕТСЯ
	{0, 0,	0,	0,	APPLY_C4,		19,	1,	19},//+за стат
	{0, 0,	0,	0,	APPLY_C5,		22,	2,	22},//+за стат
	{0, 0,	0,	0,	APPLY_C6,		26,	2,	26},//+за стат
	{0, 0,	0,	0,	APPLY_C7,		30,	2,	30},//+за стат
	{0, 0,	0,	0,	APPLY_C8,		40,	1,	40},//+за стат 55
	{0, 0,	0,	0,	APPLY_C9,		50,	1,	50} //+за стат
};
bool ToIncreaseOrPlus (int rnd);
void set_obj_aff(struct obj_data *itemobj, int bitv);
void set_obj_eff(struct obj_data *itemobj, int type, int mod);

/* global buffering system */

#ifdef __DB_C__
char	buf[MAX_STRING_LENGTH];
char	buf1[MAX_STRING_LENGTH];
char	buf2[MAX_STRING_LENGTH];
char	arg[MAX_STRING_LENGTH];
#else
extern room_rnum top_of_world;
extern struct player_special_data dummy_mob;
extern char	buf[MAX_STRING_LENGTH];
extern char	buf1[MAX_STRING_LENGTH];
extern char	buf2[MAX_STRING_LENGTH];
extern char	arg[MAX_STRING_LENGTH];
#endif


#ifndef __CONFIG_C__
extern const char	*OK;
extern const char	*NOPERSON;
extern const char	*NOEFFECT;
#endif

