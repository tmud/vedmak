/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _STRUCTS_H_
#define _STRUCTS_H_


#include <string>
/*#include <iterator>
#include <bitset>
#include <list>
#include <memory>*/
#include <map>
#include <vector>
#include <deque>

using std::string;
//using std::iterator;
using std::vector;

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x030011 /* Major/Minor/Patchlevel - MMmmPP */

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD 3.0 to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ	1	/* TRUE/FALSE aren't defined yet. */

/* preamble *************************************************************/

/*
 * Eventually we want to be able to redefine the below to any arbitrary
 * value.  This will allow us to use unsigned data types to get more
 * room and also remove the '< 0' checks all over that implicitly
 * assume these values. -gg 12/17/99
 */
#define NOWHERE    -1    /* nil reference for room-database	*/
#define NOTHING	   -1    /* nil reference for objects		*/
#define NOBODY	   -1    /* nil reference for mobiles		*/

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)


/* room-related defines *************************************************/

extern char	str_empty[1];
extern const int ClasStat[][6];

/*
 * Structure types.
 */

typedef struct ExtendFlag
  FLAG_DATA;
typedef struct shop_data
  SHOP_DATA;
typedef struct room_data
  ROOM_DATA;
typedef struct index_data
  INDEX_DATA;
typedef struct script_data
  SCRIPT_DATA;
typedef struct room_direction_data
  EXIT_DATA;
typedef struct time_info_data
  TIME_INFO_DATA;
typedef struct extra_descr_data
  EXTRA_DESCR_DATA;
typedef struct descriptor_data
  DESCRIPTOR_DATA;
typedef struct affected_type
  AFFECT_DATA;
typedef struct char_data
  CHAR_DATA;
typedef struct obj_data
  OBJ_DATA;
typedef struct trig_data
  TRIG_DATA;
 

/* The cardinal directions: used as index to room_data.dir_option[] */
#define FORMAT_INDENT		(1 << 0)

#define KT_ALT        1
#define KT_WIN        2
#define KT_WINZ       3
#define KT_WINZ6      4
#define KT_LAST       5
#define KT_SELECTMENU 255

#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5


/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK			(1 << 0)   /* Dark			*/
#define ROOM_DEATH			(1 << 1)   /* Death trap		*/
#define ROOM_NOMOB			(1 << 2)   /* MOBs not allowed		*/
#define ROOM_INDOORS		(1 << 3)   /* Indoors			*/
#define ROOM_PEACEFUL		(1 << 4)   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF		(1 << 5)   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK		(1 << 6)   /* Track won't go through	*/
#define ROOM_NOMAGIC		(1 << 7)   /* Magic not allowed		*/
#define ROOM_TUNNEL		(1 << 8)   /* room for only 1 pers	*/
#define ROOM_PRIVATE		(1 << 9)   /* Can't teleport in		*/
#define ROOM_GODROOM		(1 << 10)  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		(1 << 11)  /* (R) Room is a house флаг, показывающий, что в этой руме неработает таймер дикея для клан_замков	*/
#define ROOM_HOUSE_CRASH	(1 << 12)  /* (R) House needs saving	этот флаг нафиг не нужен, при переписании убрать или в резерв сделать*/
#define ROOM_ATRIUM		(1 << 13)  /* (R) The door to a house	*/
#define ROOM_OLC		(1 << 14)  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK		(1 << 15)  /* (R) breath-first srch mrk	*/
#define ROOM_MAGE           	(1 << 16)  /*    q mages privelege   */
#define ROOM_CLERIC         	(1 << 17)  /*    r cleric priv	*/
#define ROOM_THIEF          	(1 << 18)  /*    s thief priv	*/
#define ROOM_WARRIOR        	(1 << 19)  /*    t warrior priv	*/
#define ROOM_ASSASINE       	(1 << 20)  /*    u ассасин		*/
#define ROOM_MONAH          	(1 << 21)  /*    v монах		*/
#define ROOM_TAMPLIER       	(1 << 22)  /*    w тамплиер		*/
#define ROOM_SLEDOPYT       	(1 << 23)  /*    x следопыт		*/
#define ROOM_POLY           	(1 << 24)  /*	 y жертвенник	*/
#define ROOM_MONO           	(1 << 25)  /*    z молелнья		*/
#define ROOM_VARVAR         	(1 << 26)  /*    А варвары     	*/
#define ROOM_AUCTION		(1 << 27)  /*    B Комната для аукциона */
#define ROOM_DRUID          	(1 << 28)  /*    С друиды		*/
#define ROOM_ARENA          	(1 << 29)  /*    D Арена		*/


#define ROOM_NOSUMMON       (INT_ONE | (1 << 0))
#define ROOM_NOTELEPORT     (INT_ONE | (1 << 1))
#define ROOM_NOHORSE        (INT_ONE | (1 << 2))
#define ROOM_NOWEATHER      (INT_ONE | (1 << 3))
#define ROOM_SLOWDEATH      (INT_ONE | (1 << 4))
#define ROOM_ICEDEATH       (INT_ONE | (1 << 5))
#define ROOM_SMITH          (INT_ONE | (1 << 6))	//комната для кузни
#define ROOM_WEDMAK         (INT_ONE | (1 << 7))    //комната для ведьмака

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_CLOSED		(1 << 1)   /* The door is closed	*/
#define EX_LOCKED		(1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF		(1 << 3)   /* Lock can't be picked	*/
#define EX_HIDDEN		(1 << 4)	/* скрытый выход        */

#define AF_BATTLEDEC		(1 << 0)
#define AF_DEADKEEP		(1 << 1)
#define AF_PULSEDEC		(1 << 2)

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_FLYING	     8		   /* Wheee!			*/
#define SECT_UNDERWATER	     9		   /* Underwater		*/
#define SECT_SECRET          10        /* Секретный сектор */

#define SECT_FIELD_SNOW      20
#define SECT_FIELD_RAIN      21
#define SECT_FOREST_SNOW     22
#define SECT_FOREST_RAIN     23
#define SECT_HILLS_SNOW      24
#define SECT_HILLS_RAIN      25
#define SECT_MOUNTAIN_SNOW   26
#define SECT_THIN_ICE        27
#define SECT_NORMAL_ICE      28
#define SECT_THICK_ICE       29

#define WEATHER_QUICKCOOL		(1 << 0)
#define WEATHER_QUICKHOT		(1 << 1)
#define WEATHER_LIGHTRAIN		(1 << 2)
#define WEATHER_MEDIUMRAIN		(1 << 3)
#define WEATHER_BIGRAIN			(1 << 4)
#define WEATHER_GRAD			(1 << 5)
#define WEATHER_LIGHTSNOW		(1 << 6)
#define WEATHER_MEDIUMSNOW		(1 << 7)
#define WEATHER_BIGSNOW			(1 << 8)
#define WEATHER_LIGHTWIND		(1 << 9)
#define WEATHER_MEDIUMWIND		(1 << 10)
#define WEATHER_BIGWIND			(1 << 11)


/* Auctioning states */

#define AUC_NULL_STATE		0   /* not doing anything */
#define AUC_OFFERING		1   /* object has been offfered */
#define AUC_GOING_ONCE		2	/* object is going once! */
#define AUC_GOING_TWICE		3	/* object is going twice! */
#define AUC_LAST_CALL		4	/* last call for the object! */
#define AUC_SOLD		5
/* Auction cancle states */
#define AUC_NORMAL_CANCEL	6	/* normal cancellation of auction */
#define AUC_QUIT_CANCEL		7	/* auction canclled because player quit */
#define AUC_WIZ_CANCEL		8	/* auction cancelled by a god */


/* char and mob-related defines *****************************************/


/* PC classes */
#define CLASS_UNDEFINED	  -1
#define CLASS_CLERIC      0
#define CLASS_MAGIC_USER  1
#define CLASS_BATTLEMAGE  1
#define CLASS_THIEF       2
#define CLASS_WARRIOR     3
#define CLASS_ASSASINE    4
#define CLASS_TAMPLIER    5
#define CLASS_SLEDOPYT    6
#define CLASS_DRUID       7
#define CLASS_MONAH       8
#define CLASS_VARVAR	  9
#define CLASS_WEDMAK	  10 //ведьмак
#define NUM_CLASSES	  11  /* This must be the number of classes!! */


#define MIN_NAME_LENGTH         4

/* NPC classes (currently unused - feel free to implement!) */

#define CLASS_BASIC_NPC     	3 
#define CLASS_DRAGON		14

#define CLASS_HUMAN		0 //гуманоид
#define CLASS_ANIMAL		1 //животное
#define CLASS_COPYTNOE		2 //животное-копытное
#define CLASS_NASECOM		3 //летающее (основное насекомые)
#define CLASS_PLAVCY		4 //плавающие
#define CLASS_SNEAK		5 //ползающие (змееподобные)
#define CLASS_EFIMER		6 //эфимерные (без тела)
#define CLASS_UNDEAD		7 //нежить люди
#define CLASS_UNDEAD_ANIM	8 //нежить животные
#define CLASS_UNDEAD_COPY	9 //нежить копытные
#define CLASS_UNDEAD_NASE	10//нежить насекомые
#define CLASS_UNDEAD_PLAV	11//нежить плавающие
#define CLASS_UNDEAD_SNEA	12//нежить ползающее (обычно змееподобные)
#define CLASS_UNDEAD_EFIM	13//нежить эфимерные (без тела)
#define CLASS_MONSTR		14//Монстр-человек
#define CLASS_MONSTR_ANIM	15//Монстр-животное
#define CLASS_MONSTR_COPY	16//Монстр-копытное
#define CLASS_MONSTR_NASE	17//Монстр-насеокмое
#define CLASS_MONSTR_PLAV	18//Монстр-плавающее
#define CLASS_MONSTR_SNEA	19//Монстр-ползающее(обычно змееподобное)
#define CLASS_MONSTR_EFIM	20//Монстр-эфимерное (без тела)
#define CLASS_LAST_NPC      	21


//дописать маски по животным, нужно для итендификации чарма.
#define IS_ANIMALS(ch)		(IS_NPC(ch) && 	(GET_CLASS(ch) == CLASS_ANIMAL || GET_CLASS(ch) == CLASS_COPYTNOE ||\
				GET_CLASS(ch) == CLASS_NASECOM || GET_CLASS(ch) == CLASS_PLAVCY || GET_CLASS(ch) == CLASS_SNEAK))	

#define IS_UNDEADS(ch)		(IS_NPC(ch) && 	(GET_CLASS(ch) == CLASS_UNDEAD || GET_CLASS(ch) == CLASS_UNDEAD_ANIM ||\
				GET_CLASS(ch) == CLASS_UNDEAD_COPY || GET_CLASS(ch) == CLASS_UNDEAD_NASE || \
				GET_CLASS(ch) == CLASS_UNDEAD_PLAV || GET_CLASS(ch) == CLASS_UNDEAD_SNEA ||  GET_CLASS(ch) == CLASS_UNDEAD_EFIM||\
				GET_CLASS(ch) == CLASS_EFIMER))
#define IS_MONSTRS(ch)		(IS_NPC(ch) && 	(GET_CLASS(ch) == CLASS_MONSTR || GET_CLASS(ch) == CLASS_MONSTR_ANIM ||\
				GET_CLASS(ch) == CLASS_MONSTR_COPY || GET_CLASS(ch) == CLASS_MONSTR_NASE || \
				GET_CLASS(ch) == CLASS_MONSTR_PLAV || GET_CLASS(ch) == CLASS_MONSTR_SNEA ||  GET_CLASS(ch) == CLASS_MONSTR_EFIM))



/* PC RACES */
#define RACE_UNDEFINED   -1
#define RACE_HUMAN        0
#define RACE_ELF          1
#define RACE_GNOME        2
#define RACE_DWARF        3
#define RACE_HOBBIT       4
#define RACE_POLUELF      5
#define RACE_OGR          6
#define RACE_LESELF       7 //лесной эльф
#define RACE_VAMPIRE      8
#define NUM_RACES         9 //пернести в линух

#define MOB_KILL_VICTIM  0
#define PC_KILL_MOB      1
#define PC_KILL_PC       2
#define PC_REVENGE_PC    3

//ПК флаги
#define MAX_PKILLER_MEM  100
#define MAX_PK_ONE_ALLOW 10
#define MAX_PK_ALL_ALLOW 100
#define PK_OK            0
#define MAX_PKILL_LAG    2*60
#define MAX_REVENGE_LAG  2*60
#define MAX_PTHIEF_LAG   10*60
#define MAX_DEST         10

/* Sex */
#define SEX_NEUTRAL       0
#define SEX_MALE          1
#define SEX_FEMALE        2
#define SEX_POLY          3

#define GF_GODSLIKE   (1 << 0)
#define GF_GODSCURSE  (1 << 1)
#define GF_HIGHGOD    (1 << 2)
#define GF_REMORT     (1 << 3)
#define GF_BADNAME    (1 << 4)
#define GF_PERSLOG    (1 << 5)	// записть персонального лога

/*Слоты и круги*/
#define MAX_SLOT	     12
#define MAX_KRUG	      9
	

/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_RESTING    5	/* resting		*/
#define POS_SITTING    6	/* sitting		*/
#define POS_FIGHTING   7	/* fighting		*/
#define POS_STANDING   8	/* standing		*/

/* PC religions */
#define RELIGION_POLY    0
#define RELIGION_MONO    1
#define MASK_RELIGION_POLY        (1 << RELIGION_POLY)
#define MASK_RELIGION_MONO        (1 << RELIGION_MONO)

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER		(1 << 0)   /* Player is a player-killer			*/
#define PLR_THIEF		(1 << 1)   /* Player is a player-thief			*/
#define PLR_FROZEN		(1 << 2)   /* Player is frozen					*/
#define PLR_DONTSET     	(1 << 3)   /* Don't EVER set (ISNPC bit)		*/
#define PLR_WRITING		(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING		(1 << 5)   /* Player is writing mail			*/
#define PLR_CRASH		(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK		(1 << 7)   /* Player has been site-cleared		*/
#define PLR_MUTE		(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE		(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED		(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM		(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOFOLLOW		(1 << 12)
#define PLR_NODELETE		(1 << 13)  /* Player shouldn't be deleted		*/
#define PLR_INVSTART		(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO		(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_HELLED	    	(1 << 16)  /* Посажен в АД	*/
#define PLR_NAMED	    	(1 << 17)  /* Игрок в комнате имени */
#define PLR_REGISTERED  	(1 << 18)  /* Игрок в комнате регистрации*/
#define PLR_DUMB	    	(1 << 19)  /* Игрок немой (не может произносить любой звук */
#define PLR_QUESTOR     	(1 << 20)  // Участник квеста
#define PLR_GOAHEAD     	(1 << 21)  /* Добавление IAC GA после промпта */
#define PLR_ANTISPAMM   	(1 << 22)  //против спамеров, войдет лишь через 5 минут
#define PLR_IMMCAST     	(1 << 23)  // разрешение или запрещение заклинаний реализация иммкоманд
#define PLR_IMMKILL     	(1 << 24)  // разрешение или запрещение боевых действий реализ. иммкоманд
#define PLR_DELETE      	(1 << 28)  /* RESERVED - ONLY INTERNALLY */
#define PLR_FREE        	(1 << 29)  /* RESERVED - ONLY INTERBALLY */

#define PLR_NOEXP     	    (INT_ONE | (1 << 0)) // no exp

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         	(1 << 0)  // Моб имеет спец процедуру (программу)
#define MOB_SENTINEL     	(1 << 1)  // Mob стоит на месте			
#define MOB_SCAVENGER    	(1 << 2)  // Моб подбирает вещи с земли		
#define MOB_ISNPC        	(1 << 3)  // Это моб (устанавливается программой)  
#define MOB_AWARE	     	(1 << 4)  // Моб не может быть застаблен			
#define MOB_AGGRESSIVE   	(1 << 5)  // Агро-моб (нападает на всех подряд)	
#define MOB_STAY_ZONE    	(1 << 6)  // Моб не выйдет из своей зоны		
#define MOB_WIMPY        	(1 << 7)  // Моб сбежит из боя при хитов менее 1/4	
#define MOB_AGGR_DAY	 	(1 << 8)  // Моб нападает на игроков только днем	
#define MOB_AGGR_NIGHT	 	(1 << 9)  // Моб нападает на игроков только ночью	
#define MOB_AGGR_FULLMOON 	(1 << 10) // Моб нападает при полнолунии
#define MOB_MEMORY		(1 << 11) // Моб запоминает нападавших на него РС
#define MOB_HELPER		(1 << 12) // Моб атакует игроков, дерущихся с NPC
#define MOB_CHARM		(1 << 13) // Моба нельзя зачармить					
#define MOB_NOSUMMON	 	(1 << 14) // Моба нельзя призвать					
#define MOB_NOSLEEP		(1 << 15) // Моба нельзя усыпить				
#define MOB_NOBASH	     	(1 << 16) // Моба нельзя сбить (e.g. trees)		
#define MOB_NOBLIND	     	(1 << 17) // Моба нельзя ослепить				
#define MOB_MOUNTING     	(1 << 18) // Моба можно оседлать		
#define MOB_NOHOLD       	(1 << 19) // Моба нельзя захолдить		 
#define MOB_NOSIELENCE   	(1 << 20) // Моба нельзя замолчать		
#define MOB_AGGRMONO     	(1 << 21) // Нападает на добрых		
#define MOB_AGGRPOLY     	(1 << 22) // Нападает на злых			
#define MOB_NOFEAR       	(1 << 23) // Не убежит при заклинашки страх
#define MOB_NOGROUP      	(1 << 24) // Моб не будет входить в группу другим мобам 
#define MOB_CORPSE       	(1 << 25) // Моб поднят из трупа (устанавливается программой)
#define MOB_LOOTER       	(1 << 26) // Моб лутит трупы
#define MOB_PROTECT      	(1 << 27) // Моб под защитой - дамаджи не проходят
#define MOB_DELETE       	(1 << 28) // RESERVED - ONLY INTERNALLY 
#define MOB_FREE         	(1 << 29) // RESERVED - ONLY INTERBALLY

#define MOB_SWIMMING      	(INT_ONE | (1 << 0))
#define MOB_FLYING        	(INT_ONE | (1 << 1))
#define MOB_ONLYSWIMMING  	(INT_ONE | (1 << 2))
#define MOB_AGGR_WINTER   	(INT_ONE | (1 << 3))
#define MOB_AGGR_SPRING   	(INT_ONE | (1 << 4))
#define MOB_AGGR_SUMMER   	(INT_ONE | (1 << 5))
#define MOB_AGGR_AUTUMN   	(INT_ONE | (1 << 6))
#define MOB_LIKE_DAY      	(INT_ONE | (1 << 7))
#define MOB_LIKE_NIGHT    	(INT_ONE | (1 << 8))
#define MOB_LIKE_FULLMOON 	(INT_ONE | (1 << 9))
#define MOB_LIKE_WINTER   	(INT_ONE | (1 << 10))
#define MOB_LIKE_SPRING   	(INT_ONE | (1 << 11))
#define MOB_LIKE_SUMMER   	(INT_ONE | (1 << 12))
#define MOB_LIKE_AUTUMN   	(INT_ONE | (1 << 13))
#define MOB_NOFIGHT       	(INT_ONE | (1 << 14))
#define MOB_EADECREASE    	(INT_ONE | (1 << 15))
#define MOB_HORDE         	(INT_ONE | (1 << 16))
#define MOB_CLONE         	(INT_ONE | (1 << 17))
#define MOB_FIREBREATH    	(INT_ONE | (1 << 18))
#define MOB_GASBREATH     	(INT_ONE | (1 << 19))
#define MOB_FROSTBREATH   	(INT_ONE | (1 << 20))
#define MOB_ACIDBREATH    	(INT_ONE | (1 << 21))
#define MOB_LIGHTBREATH   	(INT_ONE | (1 << 22))
#define MOB_OPENDOOR      	(INT_ONE | (1 << 23))
#define MOB_NOTRIP        	(INT_ONE | (1 << 24))
#define MOB_NOTRAIN       	(INT_ONE | (1 << 25))

#define TRACK_NPC              (1 << 0)
#define TRACK_HIDE             (1 << 1)

#define PRF_BRIEF       	(1 << 0)  // Краткий режим, не видно описание комнаты
#define PRF_COMPACT     	(1 << 1)  // No extra CRLF pair before prompts	
#define PRF_DEAF		(1 << 2)  // Не может слышать крики			
#define PRF_NOTELL		(1 << 3)  // не может получать сказанный текст кем либо		
#define PRF_DISPHP		(1 << 4)  // боевой дисплей
#define PRF_AGRO		(1 << 5)  // Режим агро (позволяет вводить сокращенные имена)
#define PRF_DIRECTION		(1 << 6)  // Направление (показывает куда движется персонаж) ПЕРЕНЕСЕНО 27.08.2007г.
#define PRF_AUTOEXIT		(1 << 7)  // автовыходы		
#define PRF_NOHASSLE		(1 << 8)  // Агрессивный моб не может атаковать		
#define PRF_QUEST		(1 << 9)  // On quest				
#define PRF_SUMMONABLE		(1 << 10) // Can be summoned			
#define PRF_NOREPEAT		(1 << 11) // No repetition of comm commands	
#define PRF_HOLYLIGHT		(1 << 12) // Способность видеть в темноте. Для Иммов
#define PRF_COLOR_1		(1 << 13) // Color (low bit)		
#define PRF_COLOR_2		(1 << 14) // Color (high bit)		
#define PRF_NOWIZ		(1 << 15) // Can't hear wizline		
#define PRF_LOG1		(1 << 16) // On-line System Log (low bit)
#define PRF_LOG2		(1 << 17) // On-line System Log (high bit)
#define PRF_NOAUCT		(1 << 18) // Не будет слышать канал аукциона	
#define PRF_NOGOSS		(1 << 19) // Не может болтать на канале	
#define PRF_NOGRATZ		(1 << 20) // Оффтопик	
#define PRF_ROOMFLAGS		(1 << 21) // Can see room flags (ROOM_x)
#define PRF_AFK         	(1 << 22) // Игрок в АФК 
#define PRF_AUTOMEM     	(1 << 23) // режим автозаучивания заклинаний
#define PRF_NOWBOARD		(1 << 24) // не может писать на досках
#define PRF_JOURNAL     	(1 << 25) // порядок вывода журналов
#define PRF_AGRO_AUTO       (1 << 25) // Autoagro flag / off after battle
#define PRF_DISPEXITS   	(1 << 26) // резерв
#define PRF_AUTOZLIB		(1 << 27) // Automatically do compression.
#define PRF_AWAKE       	(1 << 28)
#define PRF_NOSHOUT     	(1 << 29)


/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND             (1 << 0)	   /* (R) Char is blind		*/
#define AFF_INVISIBLE         (1 << 1)	   /* Char is invisible		*/
#define AFF_DETECT_ALIGN      (1 << 2)	   /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      (1 << 3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1 << 4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1 << 5)	   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (1 << 6)	   /* Char can walk on water	*/
#define AFF_SANCTUARY         (1 << 7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (1 << 8)	   /* (R) Char is grouped		*/
#define AFF_CURSE             (1 << 9)	   /* Char is cursed			*/
#define AFF_INFRAVISION       (1 << 10)	   /* Char can see in dark		*/
#define AFF_POISON            (1 << 11)	   /* (R) Char is poisoned		*/
#define AFF_PROTECT_EVIL      (1 << 12)	   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (1 << 13)	   /* Char protected from good  */
#define AFF_SLEEP             (1 << 14)	   /* (R) Char magically asleep	*/
#define AFF_NOTRACK 	      (1 << 15)	   /* Char can't be tracked		*/
#define AFF_TETHERED	      (1 << 16)	   /* Пока не работает			*/
#define AFF_BLESS	          (1 << 17)	   /* Благословение				*/
#define AFF_SNEAK             (1 << 18)	   /* Char can move quietly		*/
#define AFF_HIDE              (1 << 19)	   /* Char is hidden			*/
#define AFF_COURAGE	          (1 << 20)	   /* Оставить под берсерк		*/
#define AFF_CHARM             (1 << 21)	   /* Зачармлен (! не вставлять)*/
#define AFF_HOLD              (1 << 22)    /* похолжен (не вставлять)   */
#define AFF_FLY               (1 << 23)    /* в полете */
#define AFF_SIELENCE          (1 << 24)    /* помолчан*/
#define AFF_AWARNESS          (1 << 25)    /* насторожен*/
#define AFF_BLINK             (1 << 26)    /* мигает (уходит из-под удара)*/
#define AFF_HORSE             (1 << 27)    /* лошадь (после оседлания - не вставлять !)*/
#define AFF_NOFLEE            (1 << 28)    /* не сбежит*/
#define AFF_SINGLELIGHT       (1 << 29)    /* источник света (браслеты и т.п.)*/


#define AFF_HOLYLIGHT         	(INT_ONE | (1 << 0))
#define AFF_HOLYDARK          	(INT_ONE | (1 << 1))
#define AFF_DETECT_POISON     	(INT_ONE | (1 << 2))
#define AFF_DRUNKED           	(INT_ONE | (1 << 3))
#define AFF_ABSTINENT         	(INT_ONE | (1 << 4))
#define AFF_STOPRIGHT         	(INT_ONE | (1 << 5))
#define AFF_STOPLEFT          	(INT_ONE | (1 << 6))
#define AFF_STOPFIGHT         	(INT_ONE | (1 << 7))
#define AFF_HAEMORRAGIA       	(INT_ONE | (1 << 8))
#define AFF_CAMOUFLAGE        	(INT_ONE | (1 << 9))
#define AFF_WATERBREATH       	(INT_ONE | (1 << 10))
#define AFF_SLOW              	(INT_ONE | (1 << 11))
#define AFF_HASTE             	(INT_ONE | (1 << 12))
#define AFF_SHIELD		(INT_ONE | (1 << 13))
#define AFF_AIRSHIELD       	(INT_ONE | (1 << 14))
#define AFF_FIRESHIELD        	(INT_ONE | (1 << 15))
#define AFF_ICESHIELD		(INT_ONE | (1 << 16))
#define AFF_MAGICGLASS		(INT_ONE | (1 << 17))
#define AFF_STAIRS		(INT_ONE | (1 << 18))
#define AFF_STONEHAND		(INT_ONE | (1 << 19))
#define AFF_PRISMATICAURA   	(INT_ONE | (1 << 20))
#define AFF_HELPER		(INT_ONE | (1 << 21))
#define AFF_EVILESS             (INT_ONE | (1 << 22))
#define AFF_AIRAURA     	(INT_ONE | (1 << 23))
#define AFF_FIREAURA		(INT_ONE | (1 << 24))
#define AFF_ICEAURA             (INT_ONE | (1 << 25))
#define AFF_HOLDALL             (INT_ONE | (1 << 26))
#define AFF_MENTALLS		(INT_ONE | (1 << 27))// новое заклинание
#define AFF_DETECT_MENTALLS	(INT_ONE | (1 << 28))// новое заклинание

             /*Экстрафлаги мобов (Special_Bitvector)*/
#define NPC_NORTH         (1 << 0)      // Моб не пускает на север 
#define NPC_EAST          (1 << 1)      // Моб не пускает на восток
#define NPC_SOUTH         (1 << 2)      // Моб не пускает на юг 
#define NPC_WEST          (1 << 3)      // Моб не пускает на запад
#define NPC_UP            (1 << 4)      // Моб не пускает вверх  
#define NPC_DOWN          (1 << 5)      // Моб не пускает вниз  
#define NPC_POISON        (1 << 6)      // Моб травит при битве 
#define NPC_INVIS         (1 << 7)      // Моб стоит в невидимости
#define NPC_SNEAK         (1 << 8)      // Моб подкрадывается 
#define NPC_CAMOUFLAGE    (1 << 9)      // Моб маскируется  
#define NPC_STAND         (1 << 10)     // Моб встает при вхождении игрока не используется
#define NPC_MOVEFLY       (1 << 11)     // Моб летает
#define NPC_MOVECREEP     (1 << 12)	// Моб приползает
#define NPC_MOVEJUMP      (1 << 13)	// Моб припрыгивает
#define NPC_MOVESWIM      (1 << 14)	// Моб приплывает
#define NPC_MOVERUN       (1 << 15)	// Моб прибегает
#define NPC_NOEXP	  (1 << 16)   	// Моб не дает экспы
#define NPC_RANDHELP	  (1 << 17)	// Моб рандомно хелпит
#define NPC_AIRCREATURE   (1 << 20)	// Моб порождение возздуха
#define NPC_WATERCREATURE (1 << 21)	// Моб порождения воды
#define NPC_EARTHCREATURE (1 << 22)	// Моб порождения земли
#define NPC_FIRECREATURE  (1 << 23)	// Моб порождения огня
#define NPC_HELPED        (1 << 24)     // Моба можно нанять
#define NPC_QUEST_GOOD    (1 << 25)     //Участвует в квестах для добрых
#define NPC_QUEST_EVIL    (1 << 26)     //Участвует в квестах для злых
#define NPC_CITYGUARD     (1 << 27)     //Нападает на игроков, с флагом убийцы или вора

#define NPC_STEALING      (INT_ONE | (1 << 0))  // Моб ворует
#define NPC_WIELDING      (INT_ONE | (1 << 1))	// Моб вооружается из инвентаря
#define NPC_ARMORING      (INT_ONE | (1 << 2))	// Моб одевает доспехи
#define NPC_USELIGHT      (INT_ONE | (1 << 3))	// Моб может брать лампу для освещения
			 
/* Descriptor flags */
#define DESC_CANZLIB	(1 << 0)  /* Client says compression capable.   */
//количество запомненных теллов.
#define MAX_REMEMBER_TELLS          30

#define IS_BITS(mask, bitno) (IS_SET(mask,(1 << bitno)))
#define IS_CASTER(ch)        (IS_BITS(MASK_CASTER,GET_CLASS(ch)))
#define IS_MAGE(ch)          (IS_BITS(MASK_MAGES, GET_CLASS(ch)))

#define MASK_BATTLEMAGE   (1 << CLASS_MAGIC_USER)
#define MASK_CLERIC       (1 << CLASS_CLERIC)
#define MASK_THIEF        (1 << CLASS_THIEF)
#define MASK_WARRIOR      (1 << CLASS_WARRIOR)
#define MASK_ASSASINE     (1 << CLASS_ASSASINE)
#define MASK_VARVAR       (1 << CLASS_VARVAR)
#define MASK_SLEDOPYT     (1 << CLASS_SLEDOPYT)
#define MASK_TAMPLIER     (1 << CLASS_TAMPLIER)
#define MASK_MONAH        (1 << CLASS_MONAH)
#define MASK_DRUID		  (1 << CLASS_DRUID)
#define MASK_WEDMAK       (1 << CLASS_WEDMAK)//ведьмак

//#define MASK_RANGER       (1 << CLASS_RANGER)
//#define MASK_SMITH        (1 << CLASS_SMITH)
//#define MASK_MERCHANT     (1 << CLASS_MERCHANT)

#define MASK_MAGES        (MASK_BATTLEMAGE | MASK_DRUID)// | MASK_TAMPLIER | MASK_CHARMMAGE | MASK_NECROMANCER)
#define MASK_CASTER       (MASK_BATTLEMAGE | MASK_TAMPLIER | MASK_SLEDOPYT | MASK_CLERIC | MASK_DRUID | MASK_ASSASINE | MASK_MONAH)


//Клановые установки
#define AFF_CLAN_DAMROLL            		(INT_TWO | (1 << 0))	//дамролл	
#define AFF_CLAN_HITROLL	        	(INT_TWO | (1 << 1))	//хитролл
#define AFF_CLAN_INITIATIVE			(INT_TWO | (1 << 2))	//инициатива
#define AFF_CLAN_MOVE				(INT_TWO | (1 << 3))	//очки движения
#define AFF_CLAN_HITREG				(INT_TWO | (1 << 4))	//скорость восстановления жизни
#define AFF_CLAN_MOVEREG			(INT_TWO | (1 << 5))	//скорость восстановления мувов
#define AFF_CLAN_CAST_SUCCES	    		(INT_TWO | (1 << 6))	//шанс успешного колдовства
#define AFF_CLAN_MANAREG			(INT_TWO | (1 << 7))	//скорость мема
#define AFF_CLAN_AC				(INT_TWO | (1 << 8))	//класс брони
#define AFF_CLAN_ABSORBE            		(INT_TWO | (1 << 9))	//поглощение
#define AFF_CLAN_SAVE_SPELL			(INT_TWO | (1 << 10))	//сейвы
#define AFF_CLAN_SAVE_PARA          		(INT_TWO | (1 << 11))	//сейвы
#define AFF_CLAN_SAVE_ROD			(INT_TWO | (1 << 12))	//сейвы
#define AFF_CLAN_SAVE_BASIC			(INT_TWO | (1 << 13))	//сейвы
#define AFF_CLAN_WIS				(INT_TWO | (1 << 14))	//мудрость
#define AFF_CLAN_INT 				(INT_TWO | (1 << 15))	//интеллект
#define AFF_CLAN_DEX				(INT_TWO | (1 << 16))	//ловкость
#define AFF_CLAN_STR				(INT_TWO | (1 << 17))	//сила
#define AFF_CLAN_CON				(INT_TWO | (1 << 18))	//тело
#define AFF_CLAN_LCK				(INT_TWO | (1 << 19))	//удача
#define AFF_CLAN_HITPOINT	        	(INT_TWO | (1 << 20))	//количество жизни
#define AFF_CLAN_MORALE             		(INT_TWO | (1 << 21))	//мораль





/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	      0				/* Playing - Nominal state	*/
#define CON_CLOSE	      1				/* Disconnecting		*/
#define CON_GET_NAME	  2				/* By what name ..?		*/
#define CON_NAME_CNFRM	  3				/* Did I get that right, x?	*/
#define CON_PASSWORD	  4				/* Password:			*/
#define CON_NEWPASSWD	  5				/* Give me a password for x	*/
#define CON_CNFPASSWD	  6				/* Please retype password:	*/
#define CON_QSEX	      7				/* Sex?				*/
#define CON_QCLASS	      8				/* Class?			*/
#define CON_RMOTD		  9				/* PRESS RETURN after MOTD	*/
#define CON_MENU		 10				/* Your choice: (main menu)	*/
#define CON_EXDESC		 11				/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 12				/* Changing passwd: get old	*/
#define CON_CHPWD_GETNEW 13				/* Changing passwd: get new	*/
#define CON_CHPWD_VRFY   14				/* Verify new password		*/
#define CON_DELCNF1	     15				/* Delete confirmation 1	*/
#define CON_DELCNF2	     16				/* Delete confirmation 2	*/
#define CON_DISCONNECT	 17				/* In-game disconnection	*/
#define	CON_CODEPAGE	 18				/* Russian codepage		*/
#define CON_QRACE        19             /* Race? */   
#define CON_RNAME        20				/* Склонение имен*/
#define CON_DNAME        21				/* Склонение имен*/
#define CON_VNAME        22				/* Склонение имен*/
#define CON_TNAME        23				/* Склонение имен*/
#define CON_PNAME        24				/* Склонение имен*/
#define CON_GET_EMAIL    25				/* Адрес емейл игрока*/
#define CON_CREAT_NAME	 26				/* Создать персонаж..!		*/
#define CON_TEDIT        27	    		/* OLC mode - text editor	*/
#define CON_GET_ALIGNMENT 28			//алигмент 
#define CON_ACCEPT_STATS 29				//ролл_стат 
#define CON_ACCEPT_ROLL	30
#define CON_OEDIT        31	/*. OLC mode - object edit    перенести в линух для ОЛЦ 12.04.2005г.. */
#define CON_REDIT        32	/*. OLC mode - room edit       . */
#define CON_ZEDIT        33	/*. OLC mode - zone info edit  . */
#define CON_MEDIT        34	/*. OLC mode - mobile edit     . */
#define CON_SEDIT        35	/*. OLC mode - shop edit       . */
#define CON_TRIGEDIT     36	/*. OLC mode - trigger edit    . */
#define CON_MREDIT       37	/* OLC mode - make recept edit */
#define CON_ROLL_STATS	 38 //Выбор рола - ручной или автоматический. перенести в линух С и С++ 22.04.2006г.
#define CON_ROLL_STR	 39
#define CON_ROLL_DEX	 40
#define CON_ROLL_INT	 41
#define CON_ROLL_WIS	 42
#define CON_ROLL_CON	 43
#define CON_ROLL_CHA	 44
#define CON_ROLL_SIZE    45


#define EXPERT_WEAPON    80


/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */

  //РАСШИРЕНИЕ И УПОРЯДОЧЕНИЕ ОДЕВАЕМЫХ ПРЕДМЕТОВ НА ПЕРСОНАЖА

#define WEAR_LIGHT      0				// свет
#define WEAR_HEAD       1				// на голове
#define WEAR_EYES       2				// на глазах
#define WEAR_EAR_R		3				// на правом ухе
#define WEAR_EAR_L      4				// на левом ухе
#define WEAR_NECK_1     5				// на шее
#define WEAR_NECK_2     6				// на шее
#define WEAR_BACKPACK   7				// за спиной (рюкзак)
#define WEAR_BODY       8				// на теле
#define WEAR_ABOUT      9				// вокруг тела
#define WEAR_WAIST     10				// вокруг талии
#define WEAR_HANDS     11				// на руках
#define WEAR_WRIST_R   12				// на правом запястье
#define WEAR_WRIST_L   13				// на левом запястье
#define WEAR_ARMS      14				// на кистях
#define WEAR_FINGER_R  15				// правый палец
#define WEAR_FINGER_L  16				// левый палец
#define WEAR_LEGS      17				// на ногах
#define WEAR_FEET      18				// как обувь
#define WEAR_SHIELD    19				// как щит
#define WEAR_WIELD     20				// в правой руке
#define WEAR_HOLD      21				// в левой руке
#define WEAR_BOTHS     22				// в обеих руках

#define NUM_WEARS      23	// This must be the # of eq positions

/* object-related defines ********************************************/


/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Unimplemented		*/
#define ITEM_MISSILE    7		/* Unimplemented		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
#define ITEM_BOOK	   24		/* Книга для магов, убийц, монахов */
#define ITEM_INGRADIENT 25      /* Инградиент */	
#define ITEM_MING      26	    /* Магический ингредиент  перенести в линух ОЛЦ 13.04.2005*/
#define ITEM_TABLE	   27		/* табличка для друидов, следопыт */
#define ITEM_MANUS	   28		/* Манускрипт для лекарей, паладинов, монахов */
#define ITEM_SKIN	   29       // Шкура от мобов!!!

/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  // Может браться с земли  		
#define ITEM_WEAR_FINGER	(1 << 1)  // Может носится на пальце    	
#define ITEM_WEAR_NECK		(1 << 2)  // Может одет быть вокруг шеи 	
#define ITEM_WEAR_BODY		(1 << 3)  // Может носиться на теле			
#define ITEM_WEAR_HEAD		(1 << 4)  // Может носиться на голове   	
#define ITEM_WEAR_LEGS		(1 << 5)  // Может носиться на ногах		
#define ITEM_WEAR_FEET		(1 << 6)  // Может носиться как обувь	
#define ITEM_WEAR_HANDS		(1 << 7)  // Может носиться как наручи
#define ITEM_WEAR_ARMS		(1 << 8)  // Может носится на кистях руки	
#define ITEM_WEAR_SHIELD	(1 << 9)  // Может использоваться как щит
#define ITEM_WEAR_ABOUT		(1 << 10) // Может носиться на плечах 	
#define ITEM_WEAR_WAIST 	(1 << 11) // Может носиться вокруг талии
#define ITEM_WEAR_WRIST		(1 << 12) // Может носится на запястье 	
#define ITEM_WEAR_WIELD		(1 << 13) // Можно вооружится			
#define ITEM_WEAR_HOLD		(1 << 14) // Можно держать				
#define ITEM_WEAR_BOTHS     (1 << 15) // Можно вооружиться в обе руки
#define ITEM_WEAR_EYES		(1 << 16) // Может одеваться на глаза 
#define ITEM_WEAR_EAR		(1 << 17) // Может вдеваться в уши 
#define ITEM_WEAR_BACKPACK	(1 << 18) // Может одеваться за спиной 

// РАсширенное одевание персонажа


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW          (1 << 0)		/* Item is glowing				*/
#define ITEM_HUM           (1 << 1)		/* Item is humming				*/
#define ITEM_NORENT        (1 << 2)		/* Item cannot be rented		*/
#define ITEM_NODONATE      (1 << 3)		/* Item cannot be donated		*/
#define ITEM_NOINVIS	   (1 << 4)		/* Item cannot be made invis	*/
#define ITEM_INVISIBLE     (1 << 5)		/* Item is invisible			*/
#define ITEM_MAGIC         (1 << 6)		/* Item is magical				*/
#define ITEM_NODROP        (1 << 7)		/* Item is cursed: can't drop	*/
#define ITEM_BLESS         (1 << 8)		/* Item is blessed				*/
#define ITEM_NOSELL		   (1 << 9)		/* Предмет нельзя продать или купить*/ 	
#define ITEM_DECAY		   (1 << 10)	/* Предмет развалится,если его выбросить*/
#define ITEM_ZONEDECAY	   (1 << 11)	/* Предмет развалится при выходе из зоны*/
#define ITEM_NODISARM	   (1 << 12)	/* Предмет нельзя выбить из рук */
#define ITEM_NODECAY       (1 << 13)    // Не рассыпется если на земле
#define ITEM_POISONED      (1 << 14)    // Предмет отравлен
#define ITEM_SHARPEN       (1 << 15)    // Предмет заточен
#define ITEM_ARMORED       (1 << 16)    // Укреплен
#define ITEM_DAY		   (1 << 17)    // Загрузится днем
#define ITEM_NIGHT         (1 << 18)    // Загрузится ночью
#define ITEM_FULLMOON      (1 << 19)    // Загрузится в полнолунье
#define ITEM_WINTER        (1 << 20)    // Загрузится зимой
#define ITEM_SPRING        (1 << 21)    // Загрузится весной
#define ITEM_SUMMER        (1 << 22)    // Загрузится летом
#define ITEM_AUTUMN        (1 << 23)    // Загрузится осенью
#define ITEM_SWIMMING      (1 << 24)    // Предмет не тонет
#define ITEM_FLYING        (1 << 25)    // Предмет летает
#define ITEM_THROWING      (1 << 26)    // Предмет можно метнуть
#define ITEM_TICKTIMER     (1 << 27)    // Таймер запущен
#define ITEM_FIRE          (1 << 28)    // Предмет горит
#define ITEM_ZONERESET	   (1 << 29)    // Предмет рассыпися при репопе зоны

#define ITEM_RANDLOAD	   (INT_ONE | (1 << 0)) //генерит автоматически характеристики уровень берется из поля в редакторе "тек"
#define ITEM_NOLOCATE	   (INT_ONE | (1 << 1)) // Предмет нельзя локейтить
#define ITEM_SKIPLOG       (INT_ONE | (1 << 2)) // Подавление сообщений, что предмет на земле (для тех предметов, что и должны
						// загружатся на землю, что бы не спамило в сислог)

#define ITEM_LIFTED	   (INT_TWO | (1 << 10)) //предмет поднятый
#define ITEM_PC_CORPSE	   (INT_TWO | (1 << 29))

        // Флаги запретов 
#define ITEM_AN_GOOD		(1 << 0)		// Предмет запрещен для добрых
#define ITEM_AN_EVIL		(1 << 1)		// Предмет запрещен для злых
#define ITEM_AN_NEUTRAL	     	(1 << 2)		// Предмет запрещен для нейтральных
#define ITEM_AN_MAGIC        	(1 << 3)		// Предмет запрещен для магов
#define ITEM_AN_CLERIC	     	(1 << 4)		// Предмет запрещен для клириков
#define ITEM_AN_THIEF		(1 << 5)		// Предмет запрещен для воров
#define ITEM_AN_WARRIOR		(1 << 6)		// Предмет запрещен для воинов
#define ITEM_AN_ASSASINE	(1 << 7)		// Предмет запрещен для ассасинов
#define ITEM_AN_MONAH		(1 << 8)		// Предмет запрещен для монахов
#define ITEM_AN_TAMPLIER	(1 << 9)		// Предмет запрещен для паладинов
#define ITEM_AN_SLEDOPYT	(1 << 10)		// Предмет запрещен для следопытов
#define ITEM_AN_VARVAR		(1 << 11)		// Предмет запрещен для варваров 
#define ITEM_AN_DRUID		(1 << 13)		// Предмет запрещен для друидов
#define ITEM_AN_WEDMAK		(1 << 14)		// Предмет запрещен для ведьмаков


      // Флаги неудобств
#define ITEM_NO_GOOD		(1 << 0)		// Предмет не могут использовать добрые	
#define ITEM_NO_EVIL		(1 << 1)		// Предмет не могут использовать злые	
#define ITEM_NO_NEUTRAL	    	(1 << 2)		// Предмет не могут использовать нейтральные	
#define ITEM_NO_MAGIC	    	(1 << 3)		// Предмет не могут использовать маги	
#define ITEM_NO_CLERIC		(1 << 4)		// Предмет не могут использовать клирики
#define ITEM_NO_THIEF		(1 << 5)		// Предмет не могут использовать воры
#define ITEM_NO_WARRIOR		(1 << 6)		// Предмет не могут использовать воины
#define ITEM_NO_ASSASINE	(1 << 7)		// Предмет не могут использовать ассасины
#define ITEM_NO_MONAH		(1 << 8)		// Предмет не могут использовать монахи
#define ITEM_NO_TAMPLIER	(1 << 9)		// Предмет не могут использовать паладины
#define ITEM_NO_SLEDOPYT	(1 << 10)		// Предмет не могут использовать следопыты
#define ITEM_NO_VARVAR		(1 << 11)		// Предмет не могут использовать варвары
#define ITEM_NO_DRUID		(1 << 13)		// Предмет не могут использовать друиды
#define ITEM_NO_WEDMAK	    	(1 << 14)		// Предмет не могут использовать ведьмаки


/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to constitution	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_MANAREG          APPLY_MANA    	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/

#define APPLY_SAVING_PARA      20	// Apply to save throw: paralz	APPLY_SAVING_WILL
#define APPLY_SAVING_WILL      20
#define APPLY_SAVING_ROD       21	// Apply to save throw: rods	APPLY_RESIST_FIRE
#define APPLY_RESIST_FIRE      21
#define APPLY_SAVING_PETRI     22	// Apply to save throw: petrif	APPLY_RESIST_AIR
#define APPLY_RESIST_AIR       22
#define APPLY_SAVING_BREATH    23	// Apply to save throw: breath	APPLY_SAVING_CRITICAL
#define APPLY_SAVING_CRITICAL  23
#define APPLY_SAVING_SPELL     24	// Apply to save throw: spells	APPLY_SAVING_STABILITY
#define APPLY_SAVING_STABILITY 24

#define APPLY_HITREG           25
#define APPLY_MOVEREG          26
#define APPLY_C1               27
#define APPLY_C2               28
#define APPLY_C3               29
#define APPLY_C4               30
#define APPLY_C5               31
#define APPLY_C6               32
#define APPLY_C7               33
#define APPLY_C8               34
#define APPLY_C9               35
#define APPLY_SIZE             36
#define APPLY_ARMOUR           37
#define APPLY_POISON           38
#define APPLY_SAVING_BASIC     39
#define APPLY_SAVING_REFLEX    39
#define APPLY_CAST_SUCCESS     40
#define APPLY_MORALE           41
#define APPLY_INITIATIVE       42
#define APPLY_RELIGION	       43
#define APPLY_ABSORBE          44
#define APPLY_LIKES	       45
#define APPLY_RESIST_WATER	46	// Apply to RESIST throw: water  сопротивление воде
#define APPLY_RESIST_EARTH	47	// Apply to RESIST throw: earth  сопротивление земле
#define APPLY_RESIST_VITALITY	48	// Apply to RESIST throw: light, dark, critical damage 
#define APPLY_RESIST_MIND	49	// Apply to RESIST throw: mind magic сопротивление затуманиванию сознания
#define APPLY_RESIST_IMMUNITY	50	// Apply to RESIST throw: poison, disease etc. сопротивляемость ядам
#define APPLY_ARESIST		51	// Apply to Magic affect resist  сопротивление магическим аффектам
#define APPLY_DRESIST		52	// Apply to Magic damage resist  сопротивление магическим повреждениям
#define NUM_APPLIES		53



/* Material flags - 11 строка число 4 */
#define MAT_NONE               0        /*Нет*/  
#define MAT_BULAT              1	    /*Булат*/
#define MAT_BRONZE             2		/*Бронза*/	
#define MAT_IRON               3		/*Железо*/
#define MAT_STEEL              4		/*Сталь*/
#define MAT_SWORDSSTEEL        5		/*Мифрил*/
#define MAT_COLOR              6		/*Цветной металл*/
#define MAT_CRYSTALL           7		/*Кристалл*/
#define MAT_WOOD               8		/*Тонкое дерево*/
#define MAT_SUPERWOOD          9		/*Крепкое дерево*/
#define MAT_FARFOR             10		/*Керамика*/
#define MAT_GLASS              11		/*Стекло*/
#define MAT_ROCK               12		/*Камень*/
#define MAT_BONE               13		/*Кость*/
#define MAT_MATERIA            14		/*Ткань*/
#define MAT_SKIN               15		/*Кожа*/
#define MAT_ORGANIC            16		/*Органика*/
#define MAT_PAPER              17		/*Пергамент*/
#define MAT_DIAMOND            18		/*Призрачное*/


/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0            /*вода*/
#define LIQ_BEER       1			/*пиво*/
#define LIQ_WINE       2			/*вино*/
#define LIQ_ALE        3			/*эль*/
#define LIQ_DARKALE    4			/*темный эль*/
#define LIQ_WHISKY     5			/*виски*/
#define LIQ_LEMONADE   6			/*лимонад*/
#define LIQ_FIREBRT    7			/*огненная вода*/
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10			/*молоко*/
#define LIQ_TEA        11			/*чай*/
#define LIQ_COFFE      12			/*кофе*/
#define LIQ_BLOOD      13			/*кровь*/
#define LIQ_SALTWATER  14			/*соленая вода*/
#define LIQ_CLEARWATER 15           /*чистая вода*/


/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2


/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3


/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5
#define RENT_CHEST	6

#define EXTRA_FAILHIDE       (1 << 0)
#define EXTRA_FAILSNEAK      (1 << 1)
#define EXTRA_FAILCAMOUFLAGE (1 << 2)
/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1.
 */
#define LVL_IMPL	50
#define LVL_GLGOD	49
#define LVL_BUILDER 	49
#define LVL_GRGOD	48
#define LVL_GOD		47
#define LVL_IMMORT	46
#define LVL_REMORT	31

/* Level of the 'freeze' command */
#define LVL_FREEZE	LVL_GRGOD

#define NUM_OF_DIRS	6	/* number of directions in a room (nsewud) */
#define MAGIC_NUMBER	(0x06)	/* Arbitrary number that won't be in a string */

#define OPT_USEC	100000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (10 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)
#define PULSE_AUCTION	(9 RL_SEC)


/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (12 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       256          /* Max length of prompt        */
#define GARBAGE_SPACE			32          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE			1024        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE			(MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)
#define HISTORY_SIZE			5	/* Keep last 5 commands. */
#define MAX_STRING_LENGTH		8192
#define MAX_INPUT_LENGTH		300	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	512	/* Max size of *raw* input */
#define MAX_MESSAGES			60
#define MAX_NAME_LENGTH			20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH			10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH		80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH				30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH			2048//768 /* Used in char_file_u *DO*NOT*CHANGE* тест */
#define MAX_TONGUE				3   /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS				200 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SPELLS       		200 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT				32  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT			6 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_POOF_LENGTH			120 /*used for afk message*/
#define MAX_TIMED_SKILLS 		16 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_EXTEND_LENGTH		0xFFFF
/*

*/

/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */
#if defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif

/**********************************************************************
* Structures                                                          *
**********************************************************************/
//разбор по флагам, протестить.
#if defined (WIN32)
typedef __int64		flag64_t;	/* For MSVC4.2/5.0 - flags */
typedef __int32		flag32_t;	/* short flags (less memory usage) */
#else
typedef int64_t		flag64_t;	/* For GNU C compilers - flags */
typedef int32_t		flag32_t;	/* short flags (less memory usage) */
#endif 

typedef signed char			sbyte;
typedef unsigned char		ubyte;
typedef short int			sh_int;
typedef unsigned short int	ush_int;
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char			bool;
#endif

typedef	unsigned short int	Uint16;
typedef signed short   int	Sint16;
typedef unsigned       int  	Uint32;
typedef signed         int	Sint32;


#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	/* Hm, sysdep.h? */
typedef char			byte;
#endif

typedef int room_vnum;	/* A room's vnum type */
typedef int	obj_vnum;	/* An object's vnum type */
typedef int	mob_vnum;	/* A mob's vnum type */
typedef int	zone_vnum;	/* A virtual zone number.	*/

typedef int	room_rnum;	/* A room's real (internal) number type */
typedef int	obj_rnum;	/* An object's real (internal) num type */
typedef int	mob_rnum;	/* A mobile's real (internal) num type */
typedef int	zone_rnum;	/* A zone's real (array index) number.	*/


/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 * 'unsigned long long' will give you at least 64 bits if you have GCC.
 *
 * Since we don't want to break the pfiles, you'll have to search throughout
 * the code for "bitvector_t" and change them yourself if you'd like this
 * extra flexibility.
 */
typedef unsigned long int	bitvector_t;
struct ExtendFlag {
	 int flags[4];
};

#define INT_ZERRO (0 << 30)
#define INT_ONE   (1 << 30)
#define INT_TWO   (2 << 30)
#define INT_THREE (3 << 30)
#define GET_FLAG(value,flag)  ((unsigned long)flag < (unsigned long)INT_ONE   ? value.flags[0] : \
                               (unsigned long)flag < (unsigned long)INT_TWO   ? value.flags[1] : \
                               (unsigned long)flag < (unsigned long)INT_THREE ? value.flags[2] : value.flags[3])


/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};


/* object-related structures ******************************************/


/* object flags; used in obj_data */
#define NUM_OBJ_VAL_POSITIONS 4

struct obj_flag_data {
   int		value[NUM_OBJ_VAL_POSITIONS];				// Значение предмета (see list)    
   byte		type_flag;						// Тип предмета			   
   int		tren_skill;						// Тренируемый скилл предмета
   int		cred_point;						// Сколько выдержит предмет попаданий
   int		ostalos;						// Сколько осталось попаданий	    
   ubyte    	material;					    	// Материал предмета 
   ubyte	pol;							// Пол предмета
   int		spell;							// Спелл предмета
   ubyte	level_spell;						// Уровень кастуемого спелла 
   ubyte	levels;							// уровень рандома перенести
   long		wear_flags;						// Как вы можете использовать предмет? 
   
   FLAG_DATA	extra_flags;						// Если это светится, шумит.
   FLAG_DATA    affects;
   FLAG_DATA    anti_flag;
   FLAG_DATA    no_flag;
   FLAG_DATA	bitvector;	
   
   int		weight;							// Вес объекта                  
   int		cost;							// Value when sold (gp.)            
   int		cost_per_day;						// Cost to keep pr. real day        
   int		cost_per_inv;						// рента за предмет есть он не одет
   int		timer;							// Timer for object                 
   int      	Obj_owner;
   int		Obj_zone;						// Обьект в зоне 
   int		Obj_destroyer;
   int	    	Obj_maker;						// Уникальный номер создателя 
   int	    	Obj_parent;						// Виртуальный номер родителя 
};


/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
   int location;       /* Which ability to change (APPLY_XXX) */
   sbyte modifier;     /* How much it changes by              */
};


// ================== Memory Structure for Objects ================== 
struct obj_data {
   obj_data() : clin(0), cont(0) {}
   obj_vnum item_number;							// Номер в базе данных
   room_rnum in_room;								// In what room -1 when conta/carr

   struct obj_flag_data obj_flags;						// Object information 
   struct obj_affected_type affected[MAX_OBJ_AFFECT];				// affects 

   char	*name;									// Название предмета :держать, взять и т.д.        
   char	*description;								// Описание когда лежит в комнате                     
   char	*short_description;							// Имя в именительном падеже         
   char	*short_rdescription;							// Имя в родительном падеже
   char	*short_ddescription;							// И так далее
   char	*short_vdescription;
   char	*short_tdescription;
   char	*short_pdescription;
   char	*action_description;							// Выдается сообщение при использовании          
   struct extra_descr_data *ex_description;					// дополнительное описание   
   
   sbyte  worn_on;								// Где одет?
                                                                                
   CHAR_DATA *carried_by;							// Кто несет :Ноль если в комнате или неизвестном месте 
   CHAR_DATA *worn_by;								// Worn by?			    
   OBJ_DATA *in_obj;								// In what object NULL when none    
   OBJ_DATA *contains;								// Контейнер  
 
   long id;									// used by DG triggers              
   struct trig_proto_list *proto_script;					// list of default triggers  
   struct script_data *script;							// script info for the object   

   
   OBJ_DATA *next_content;							// Список контейнера   
   OBJ_DATA *next;								// Список обьектов  
   int    room_was_in;
   int    obj_in_vroom;								// В какой комнате остался лежать труп
   int	  max_in_world;								// Количество предметов в Мире

   bool	  chest;								// Указывает, где хранится предмет? (на складе или на чаре)

   unsigned int    clin;
   unsigned int    cont;
};
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent files		   */
/*struct obj_file_elem { 
   obj_vnum item_number;
   sh_int location;
   int    ostalos;	 // Сколько осталось попаданий
   int    live;		 
   int	  value[4];
   long	  extra_flags;
   int	  weight;
   int    owner;
   int	  timer;    // Жизнь предмета в тиках
   ubyte    matery;
   long   bitvector;
   long   wear_flags;
   struct obj_affected_type affected[MAX_OBJ_AFFECT];
};
*/

/* header block for rent files.  BEWARE: Changing it will ruin rent files  */

/* ======================================================================= */


/* room-related structures ************************************************/

struct weather_control
{ int  rainlevel;
  int  snowlevel;
  int  icelevel;
  int  sky;
  int  weather_type;
  int  duration;
};


struct room_direction_data {
   char			*general_description;   // When look DIR.
   char			*keyword;		// for open/close
   char			*vkeyword;              // открывание дверей в винительном падеже                             
   sh_int		exit_info;		// Exit info
   obj_vnum		key;			// Key's number (-1 for no key)	
   room_rnum	to_room;			// Where direction leads (NOWHERE)
};

struct   track_data
{ int    track_info;                 // bitvector
  int    who;                        // real_number for NPC, IDNUM for PC 
  int    time_income[6];             // time bitvector
  int    time_outgone[6];
  struct track_data *next;
};


/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;		    /* Rooms number	(vnum)		      */
   zone_rnum zone;              /* Room zone (for resetting)          */
   ubyte	sector_type;            /* sector type (move/hide)       */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   struct  weather_control weather;   /* Weather state for room */
   FLAG_DATA room_flags;	/* DEATH,DARK ... etc */
   byte    light;                  /* Number of lightsources in room     */
   byte    glight;                    /* Number of lightness person     */
   byte    gdark;                     /* Number of darkness  person     */

   SPECIAL(*func);

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data     *script;       /* script info for the object*/
   struct track_data      *track;

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */
   ubyte  fires;                /* Time when fires                    */
   ubyte  forbidden;            /* Time when room forbidden for mobs  */
   ubyte  ices;                 /* Time when ices restore */

   int    portal_room;
   ubyte  portal_time;
   long   id;
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long	  id;
   long   time;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct save_rent_info {
   int	time;
   int	rentcode;
   int	net_cost_per_diem;
   int	nitems;
   int  m_nCostChest;
   int  m_nServis;

};

struct save_time_info {
   int vnum;
   int timer;
};


struct save_info {
   struct save_rent_info rent;
   struct save_time_info time[2];
};
/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   int	played;     /* This is the total accumulated time played in secs а тут в секундах нужно то всего таймер отсоединения? нафиг много столько инт?*/
};


/* general player-related info, usually PC's and NPC's				*/
struct char_player_data {
   char	passwd[MAX_PWD_LENGTH+1]; /* character's password				*/
   char	*name;	       /* PC / NPC имя (kill ...  )						*/
   char *rname;        /* Родительный падеж имени						*/
   char *dname;        /* Дательный падеж имени							*/
   char *vname;		   /* Винительный падеж имени						*/	
   char *tname;		   /* Тварительный падеж имени						*/ 
   char *pname;        /* Предложный падеж имени						*/
   char	*short_descr;  /* для  NPC 'короткое описание					*/
   char	*long_descr;   /* описание персонажа в комнате (Пьяница стоит)  */
   char	*description;  /* Смотрим на пьяницу и видим описание	    	*/
   char	*title;        /* титул 										*/
   char *ptitle;       /* Предтитул										*/	
   byte sex;           // PC / NPC пол	
   byte chclass;       // PC / NPC класс								
   byte race;          // PC / NPC расса								
   byte level;         // PC / NPC уровень								
   sh_int weight;      // PC / NPC Вес									
   ubyte height;       // PC / NPC Рост									

};


/* Характеристики персонажа. */
struct char_ability_data {
   ubyte Skills[MAX_SKILLS+1];	/* Матрица прокачки скиллов			*/
   ubyte SplKnw[MAX_SPELLS+1];  /* array of SPELL_KNOW_TYPE         */
   ubyte SplMem[MAX_SPELLS+1];  /* array of MEMed SPELLS            */
   sbyte str;
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
   
   
   sbyte size; /* Размер игрока*/
   sbyte hitroll;
   sh_int damroll;
   sh_int armor;

};
/* Char's additional abilities. Used only while work */
struct char_played_ability_data
{  sh_int str_add;
   sh_int intel_add;
   sh_int wis_add;
   sh_int dex_add;
   sh_int con_add;
   sh_int cha_add;
   sh_int mana_add;
   sh_int weight_add;
   sh_int height_add;
   sh_int size_add;
   sh_int age_add;
   sh_int hit_add;
   sh_int move_add;
   sh_int hitreg;
   sh_int movereg;
   sh_int manareg;
   sbyte  slot_add[10]; 
   sh_int  armour;
   sh_int  ac_add;
   sh_int  hr_add;
   sh_int  dr_add;
   sh_int  absorb;
   sh_int  morale_add;
   sh_int  cast_success;
   sh_int  initiative_add;
   sh_int  poison_add;
   sh_int apply_saving_throw[6]; /* Saving throw (Bonuses)	*/
   sh_int apply_resistance_throw[7];	// Сопротивление (резисты) к магии, ядам и крит. ударам 
   ubyte  d_resist;					// Сопротивляемость магическому дамагу
   ubyte  a_resist;					// Сопротивляемость магическим аффектам
};
/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
   sh_int mana;
   sh_int max_mana;     /* Max mana for PC/NPC			   */
   sh_int hit;
   sh_int max_hit;      /* Max hit for PC/NPC                      */
   sh_int move;
   sh_int max_move;     /* Max move for PC/NPC                     */

   int	gold;           /* Money carried                           */
   long	bank_gold;		/* Gold the char has in a bank account	   */
   long	exp;            /* The experience of the player            */
   sh_int pk_counter;   /* pk counter list тожк нафиг от сюда выбросить в спец_плаер*/
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
   short int	alignment;		/* +-1000 for alignments                */
   long	idnum;			/* player's idnum; -1 for mobiles	*/
   FLAG_DATA Act;		/* act flag for NPC's; player flag for PC's */
   FLAG_DATA affected_by;
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
   struct char_data *fighting;	/* Opponent				*/
   struct char_data *hunting;	/* Char hunted by this char		*/

   byte position;				/* Standing, fighting, sleeping, etc.	*/
   byte ConPos;					//по какой части тела наносится удар 

   sh_int	carry_weight;		/* Carried weight			*/
   sh_int   carry_items;		/* Number of items carried		*/
   int	    timer;				/* Timer for update			*/

   struct char_special_data_saved saved; /* constants saved in plrfile	*/
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
   bool			talks[MAX_TONGUE];			/* PC s язык 0 for NPC				*/
   sh_int		wimp_level;					/* Below this # of hit points, flee!*/
   byte			freeze_level;				/* Level of god who froze char, if any	*/
   sh_int		invis_level;				/* level of invisibility			*/
   sh_int	    open_krug;					/* сколько взято умений	*/
   bitvector_t	pref;						/* привелигированные флаги для PC's.*/
   ubyte		bad_pws;					/* number of bad password attemps	*/
   sbyte		conditions[3];				/* Drunk, full, thirsty				*/
 
   int			clan;						// Номер клана  26.11.2004г. 
   int			load_room;					/* Which room to place char in	 */
   char			spare0[1];				    // Для символа-разделителя 
   ubyte		spare1;						// БОнус экспа за уровень
   ubyte		spare2;						// Бонус для мувов
   bool			spells_to_learn[200];		// Количество практик для скиллов//протестить  (bool)	
   int			spare9;						// Количество убитых игроков		
   int			spare12;					// Количество Ваших смертей
   //ubyte		HouseRank;	//                в дальнейшем выкинуть, так как система кланов новая. 26.11.2004г.
   int			DrunkState;
   int			glory;						//Количество используемой славы
   int		    olc_zone;//перенести в линух ОЛЦ 12.04.2005
   int			unique;
   ubyte		clan_rank;
   int			hometown;					// PC Родной город для постоя (zone)
   ubyte		Remorts;
   bool			Prelimit;
   ubyte		Religion; 
   int			questpoints;				//общее количество славы потраченное на трены
   int			Questmob;
   int			Questobj;
   bool			QuestOk;
   ubyte		Countdown;	
   ubyte		QuestGloryMax;
   ubyte		nextquest;
   long			NameDuration;
   long			GodsLike;
   long			GodsDuration;
   long			MuteDuration;
   long			FreezeDuration;
   long			HellDuration;
   long			IndPower;	//накопительная индивидуальная  мощность 01.03.2006г.
   long			IndShopPower;	//покупательская индивидуальная мощность 01.03.2006г.	
   long			PowerStoreChar; //индивидуальная мощьность до вступления в клан 01.03.2006г
   time_t		LastLogon;
   long			DumbDuration;
   char			EMail[128];
   char			LastIP[128];
   int			Utimer;							//таймер усталости 
   sh_int		Ustalost;						//значение усталости
   long			spare19;						//количество убитых мобов
   long			spare20;						//Время счетчика ПК
   
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */

//Warehouse -  склад.
//хранилище, перенести в линух, после доработки.
//class WareHouse;	//класс для хранилища, должно присутствовать только для ироков,
					//мобы тут не причем, поэтому скорее всего в нижестоящюю структуру и надо засунуть.
					//динамически-выделяемую память сделать однозначно, назвать поле WareHouse Whouse
/*
 * Здесь я нарисую складской класс, что бы можно было хранить предметы					*
 * уходя в зону или на постой, то есть юзать его в том случае, если предметы,			*
 * которые там находятся понадобятся, причем склад будет расположен в специальной		*
 * руме, и за сохранность предметов будет отвечать специальный моб, выдающий по запросу	*
 * вещи, а так же, он назначает и сумму за хранение вещей. Склад хранится в структуре	*
 * игрока, только разнесена обработка, эмулируется состояние раздельности.
 */


typedef  vector < OBJ_DATA * > Whouse;

class WareHouse
{
public:
	WareHouse (){};
	~WareHouse (){};
   void PushObj( OBJ_DATA *obj );
   void ListObj( CHAR_DATA *ch );
   void GetObj ( CHAR_DATA *ch, char *argument);
   void TransferObj( CHAR_DATA *ch, char *argument );
   int size() const { return m_chest.size(); }
   OBJ_DATA* operator[](int index) const { return m_chest[index]; } 
   int FindObj(const char* name);
   void RemoveObj(int index);
 
  Whouse m_chest; 
};


struct player_special_data {
	struct	player_special_data_saved saved;
	struct	alias_data *aliases;/* Character's aliases				*/
	struct time_data time;  // PC's AGE in days
	char	*poofin;			/* Description on arrival of a god. */
	char	*poofout;			/* Description upon a god's exit.   */
	long	last_tell;			/* idnum of last tell from			*/
	void	*last_olc_targ;		/* olc control						*/
	int		last_olc_mode;		/* olc control						*/
	int		may_rent;           /* PK control						*/
	ubyte   CRbonus;			//количество бонусов класс и расс 
	char   * MuteReason; 
    	char   * DumbReason;
    	char   * HellReason;
	char   *afk_message;	 // PC 	афк	
	char	remember[MAX_REMEMBER_TELLS][MAX_RAW_INPUT_LENGTH]; 
	ubyte	lasttell; 
	char	temphost[HOST_LENGTH+1];
	sbyte str_roll;
	sbyte intel_roll;
	sbyte wis_roll;
	sbyte dex_roll;
	sbyte con_roll;
	sbyte cha_roll;
	ubyte point_stat[6];
	WareHouse mChest;
	
};


/* Specials used by NPCs, not PCs */
struct mob_special_data {
   byte		last_direction;     /* Последнее направление куда пошел Монстр   */
   ubyte	attack_type;        /* Номер атаки Bitvector для NPC         */
   byte		default_pos;        /* Исходная позиция для монстра (NPC)        */
   memory_rec *memory;			/* Флаг на тех кто нападал (память)	         */
   ubyte	damnodice;          /* Номер повреждения	       */
   ubyte	damsizedice;        /* Размер повреждения           */
   ubyte	like_work;          /* Вероятность использования умений или заклинаний*/
   ubyte	max_factor;			/* Количество убийств моба, при котором не будет замаксивания*/
   ubyte	extra_attack;       /* Количество атак в раунд моба */
   FLAG_DATA    Npc_Flags;	/* новый флаг блин*/
   char		*Questor;
  // int	GoldNoDs;
  // int	GoldSiDs;
   sh_int	HorseState;		
   int		LastRoom;
   int		dest[MAX_DEST];
   sh_int	dest_dir;
   sh_int	dest_pos;
   sh_int	dest_count;
   int		activity;
   bool		isquest;			// Учавствующий в квесте
   int		speed;
};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
   sh_int type;            /* Тип заклинания, который вызвал эффект   */
   int duration;        /* Длительность действия эффекта	        */
   sbyte  battleflag;        /* This is added to apropriate ability     */
   byte   location;        /* Tells which ability to change(APPLY_XXX)*/
   long   modifier;      /**** SUCH AS HOLD,SIELENCE etc **/
   bitvector_t	bitvector; /* Tells which bits to set (AFF_XXX) */

   struct affected_type *next;
};

// Структура для заклинаний и умений по времени
struct timed_type
{ ubyte      skill;               /* Number of used skill/spell */
  ubyte      time;                /* Time for next using        */
  struct     timed_type *next;
};

struct mob_kill_data
{ int count;
  int *howmany;
  int *vnum;
};


/* Structure used for helpers */
struct helper_data_type
{ int mob_vnum;
   struct helper_data_type *next_helper;
};

struct extra_attack_type
{ ubyte  used_skill;//255 для скиллов.
  struct char_data *victim;
};

struct cast_attack_type
{ sh_int    spellnum;
  struct char_data *tch;
  struct obj_data  *tobj;
};


/* Structure used for questing */
struct quest_data
{ int count;
  int *quests;
};

/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};

class MemoryOfSpells
{
public:
    MemoryOfSpells();
    int size() const;    
    int getid(int index);   
    int count(int id) const;
    void add(int id);
    void dec(int id);
    void clear();
    int get_next_spell(struct char_data *ch);    
    int get_mana_cost(struct char_data *ch, int id);  
    int calc_time(struct char_data *ch, int index);            // calc time in seconds
    int calc_mana_stored(char_data *ch);

private:
    std::vector<ubyte> mem_spells;    
    time_t m_time;
    float m_mana_per_second;
    float m_mana_stored;
};
/* ================== Structure for player/non-player =================== */
struct char_data {
   int						pfilepos;			/* playerfile pos		          */
   mob_rnum					nr;					/* Mob's rnum			          */
   room_rnum					in_room;			/* Location (real room number)	  */
   room_rnum					was_in_room;		/* location for linkdead people   */
   int						wait;				/* wait for how many loops	      */
   int						punctual_wait;		// wait for how many loops (punctual style)    
   struct char_player_data	player;				/* Normal data                    */
   struct char_played_ability_data add_abils;
   struct char_ability_data real_abils;			/* Характеристики без модификаторов*/
   struct char_point_data	points;				/* Points                         */
   struct char_special_data char_specials;		/* PC/NPC specials				  */
   struct player_special_data *player_specials; /* PC specials					  */
   struct mob_special_data	mob_specials;		/* NPC specials					  */
   
   struct affected_type		*affected;			/* affected by what spells        */
   struct obj_data			*equipment[NUM_WEARS];/* Массив позиции экипировки    */
   struct obj_data			*carrying;			/* Head of list                   */
   struct descriptor_data	*desc;				/* NULL for mobiles               */

   struct char_data			*next_in_room;     /* Список персонажей в комнате     */
   struct char_data			*next;             /* For either monster or ppl-list  */
   struct char_data			*next_fighting;    /* Список дерущихся                */
   

   long id;									   /* used by DG triggers             */
   struct trig_proto_list	*proto_script;	   /* list of default triggers        */
   struct script_data		*script;				   /* script info for the object      */
   struct script_memory		*memory;			   /* for mob memory triggers		  */
   struct timed_type		*timed;          /* use which timed skill/spells  */
  
   //ubyte                     Memory[MAX_SPELLS+1]; //заклинания, которые запоминаются в данное время

   MemoryOfSpells            Memory;            //заклинания, которые запоминаются в данное время

   struct follow_type		*followers;        /* Список следующих                */
   struct char_data			*master;           /* Лидер группы					  */
   struct char_data*         Touching;
   struct char_data*         Protecting;
   long						 Temporary;
   int                       ManaMemNeeded;  /*необходимое количество маны для полного заучивания*/
   int                       ManaMemStored;  /*сколько уже накопили маны для окончания заучивания*/  
   long					     BattleAffects;
   sh_int					 CasterLevel;
   sh_int                    DamageLevel;
   int                       Poisoner;
   sh_int                    Initiative;
   ubyte	                 BattleCounter;
   struct PK_Memory_type     *pk_list;
   struct helper_data_type   *helpers;
   struct quest_data         Questing;
   struct mob_kill_data      MobKill;
   struct extra_attack_type  extra_attack;
   struct cast_attack_type   cast_attack;
   sh_int                    track_dirs;
   bool					     CheckAggressive;
   int                       ExtractTimer;
   char						 *prompt;
   char						 *fprompt;
   char                      *exchange_filter;
};
/* ====================================================================== */




/* descriptor-related structures =========================================*/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};


struct descriptor_data {
   socket_t	descriptor;	/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte idle_tics;		/* tics idle at password prompt		*/
   ubyte	connected;		/* mode of 'connectedness'		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   time_t input_time;
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   char *backstr;		/* backup string for modify-str system	*/
   size_t max_str;	        /*		-			*/
   long	mail_to;		/* name for mail system			*/
   ubyte	has_prompt;		/* is the user at a prompt?           */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE]; /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for ! mostly.	*/
   int	history_pos;		/* Circular array position.		*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   sh_int  AntiSpamming;
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   //struct olc_data *olc;	     /*. OLC info - defined in olc.h   .*/
   ubyte	codepage;
   int wait; 
  // int prompt_mode;
   int  options;		 /* descriptor flags			*/
#if defined(HAVE_ZLIB)
   ubyte mccp_version;
   z_stream *deflate;		 /* compression engine			*/
#endif
 //   struct olc_data *olc;//перенести в линух ОЛЦ 12.04.2005г.
   void *olc;
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* сообщение нападавшему  */
   char	*victim_msg;    /* сообщение жертве       */
   char	*room_msg;      /* сообщение в комнату    */
};


struct message_type {
   struct msg_type die_msg;	    /* сообщение о смерти			*/
   struct msg_type miss_msg;	/* сообщение о промахе			*/
   struct msg_type hit_msg;	    /* сообщение при попадании		*/
   struct msg_type god_msg;	    /* сообщение при атаке на Бога	*/
   struct message_type *next;	/* переход к следующим атакам этого вида.	*/
};


struct message_list {
   int	a_type;					/* Номер атаки				*/
   int	number_of_attacks;   	/* How many attack messages to chose from. */
   struct message_type *msg;	/* Список сообщений.			*/
};


struct dex_skill_type {
   sh_int p_pocket;
   sh_int p_locks;
   sh_int traps;
   sh_int sneak;
   sh_int hide;
};


struct dex_app_type {
   sh_int reaction;
   sh_int miss_att;
   sh_int defensive;
};


struct str_app_type {
   int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
   int todam;    /* Damage Bonus/Penalty                */
   int carry_w;  /* Maximum weight that can be carrried */
   int wield_w;  /* Maximum weight that can be wielded  */
   int hold_w;   /* Добавление для левой руки*/
};


struct wis_app_type {
   ubyte  max_learn_l20;        /* MAX SKILL on LEVEL 20 */
   sh_int spell_success;        /* spell using success        */
   sh_int char_savings;         /* saving spells (damage)     */
   ubyte  max_skills;   
   ubyte  slot_bonus;		//бонус к слотам 
   sh_int addshot;			// зависимость допвыстрела
 };

struct int_app_type
{  ubyte spell_aknowlege;    /* chance to know spell               */
   sbyte to_skilluse;        /* ADD CHANSE FOR USING SKILL         */
   ubyte mana_per_tic;
   ubyte spell_success;      /*  max count of spell on 1s level    */
   ubyte improove;           /* chance to improove skill           */
   sbyte observation;        /* chance to use SKILL_AWAKE/CRITICAL */
   sh_int maxkrug;				// показатель открытия кругов!
};


struct con_app_type {
   sbyte hitp;
   ubyte addhit;
   sbyte affect_saving;      // савингспел (affects) 
   sbyte poison_saving;     //бонус к савигнпара
   ubyte critic_saving;	  //защита от критудара
   ubyte ressurection;     //воскрешение
};



struct cha_app_type
{  sbyte  leadership;
   sh_int charms;
   ubyte morale;
   sbyte illusive;
};


struct size_app_type
{  sbyte  ac;           //ADD VALUE FOR AC           
   sh_int interpolate;  // ADD VALUE FOR SOME SKILLS  
   sbyte initiative;
   ubyte shocking;
};

struct weapon_app_type
{  sbyte shocking;
   sbyte bashing;
   sbyte parrying;
};


struct extra_affects_type
{ int affect;
  int set_or_clear;
};


struct class_app_type
{ ubyte    unknown_weapon_fault;
  ubyte    koef_con; //свободно
  ubyte    base_con; //свободно
  ubyte    min_con;  //свободно
  ubyte    max_con;  //свободно

  struct extra_affects_type *extra_affects;
  struct obj_affected_type  *extra_modifiers;
};

struct race_app_type
{ struct extra_affects_type *extra_affects;
  struct obj_affected_type  *extra_modifiers;
};

struct weather_data
{  int  hours_go;              /* Time life from reboot */

   int	pressure;	           /* How is the pressure ( Mb )            */
   int  press_last_day;        /* Average pressure last day             */
   int  press_last_week;       /* Average pressure last week            */

   int  temperature;           /* How is the temperature (C)            */
   int  temp_last_day;         /* Average temperature last day          */
   int  temp_last_week;        /* Average temperature last week         */

   int  rainlevel;             /* Level of water from rains             */
   int  snowlevel;             /* Level of snow                         */
   int  icelevel;              /* Level of ice                          */

   int  weather_type;          /* bitvector - some values for month     */

   int	change;  	/* How fast and what way does it change. */
   int	sky;	    /* How is the sky.   */
   int	sunlight;	/* And how much sun. */
   int  moon_day;   /* And how much moon */
   int  season;
   int  week_day_mono;
   int  week_day_poly;
};

struct weapon_affect_types
{      int    aff_pos;
       int    aff_bitvector;
       sh_int aff_spell;
};

struct title_type {
   char	*title_m;
   char	*title_f;
   int	exp;
};


/* element in monster and object index-tables   */
struct index_data {
   int	    vnum;	    /* virtual number of this mob/obj		    */
   int		number;		/* number of existing units of this mob/obj	*/
   int		stored;     /* number of things in rent file            */
   SPECIAL(*func);
   char     *farg;         /* string argument for special function     */
   struct   trig_data *proto;     /* for triggers... the trigger     */
   int      zone;	/* mob/obj zone rnum*/
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list
{ int vnum;                             /* vnum of the trigger   */
  struct trig_proto_list *next;         /* next trigger          */
};
//****************************************************************************
struct social_messg
{ 
  int  ch_min_pos, ch_max_pos, vict_min_pos, vict_max_pos;
  char *char_self;		//мессага на себя
  char *char_self_room;	//мессага на себя, которую видят в руме
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;		/* if NULL, read no further, ignore args */
  char *others_found;
  char *vict_found;

  /* An argument was there, but no victim was found */
  char *not_found;
};

struct social_keyword
{ char *keyword;
  int  social_message;
};

extern struct social_messg   *soc_mess_list;
extern struct social_keyword *soc_keys_list;

//******************************************************************************




#define  DAY_EASTER    	-1

struct gods_celebrate_type
{ int  unique;         // Uniqum ID
  const char *name;    // Celebrate
  int  from_day;       // Day of month, -1 and less if in range
  int  from_month;     // Month, -1 and less if relative
  int  duration;
  ubyte  religion;       // Religion type
};

#define 	GAPPLY_NONE                 0
#define 	GAPPLY_SKILL_SUCCESS        1
#define 	GAPPLY_SPELL_SUCCESS        2
#define 	GAPPLY_SPELL_EFFECT         3
#define     GAPPLY_MODIFIER             4
#define     GAPPLY_AFFECT               5

struct gods_celebrate_apply_type
{ int    unique;
  int    gapply_type;
  int    what;
  int    modi;
  struct gods_celebrate_apply_type *next;
};

struct cheat_list_type
{  char *name;
   struct cheat_list_type *next_name;
};

struct auction_lot_type
{	int		item_id;
	struct	obj_data *item;
	int		seller_unique;
	struct	char_data *seller;
	int		buyer_unique;
	struct  char_data *buyer;
	int		prefect_unique;
	struct  char_data *prefect;
	int		cost;
	int		tact;
};


/* pclean_criteria_data структура которая определяет через какой время 
   неактивности будет удален чар.
*/
struct pclean_criteria_data {
  sbyte level;			/* max уровень для этого временного лимита */
  int   days;			/* временной лимит в днях 		   */
};

struct show_struct
{
  const char  *cmd;
        char  level;
};


struct set_struct
{
  const char  *cmd;
        char  level;
        char  pcnpc;
        char  type;
};



struct edit_files
{
    const char *cmd;
    char level;
    char **buffer;
    int  size;
    const char *filename;
};


//рандомная таблица лоада предметов в зависимости от условий.
struct TablObjRand
{ ubyte	m_A;		//первое выражение дамага
  ubyte	m_B;		//второе выражение дамага, будет считаться как m_A d m_B (пример 3d6)= 3*(6+1)/2
  ubyte m_base;		//базовое значение очков
  ubyte	m_weight;	//вес оружия
  int	m_AppleAff; //таблица аффектов
  ubyte m_Cost;		//цена бонуса
  sbyte m_Add;		//корректировка к бонусам нужно что бы обработка была с минусами 13.12.2007г.
  ubyte m_Coeff;	//коэфициент к значениям
};

#endif
