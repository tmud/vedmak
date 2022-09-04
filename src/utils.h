/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* external declarations and prototypes **********************************/

extern struct weather_data weather_info;
extern FILE *logfile;

#define log			basic_mud_log
extern char AltToKoi[];
extern char KoiToAlt[];
extern char WinToKoi[];
extern char KoiToWin[];
extern char KoiToWin2[];
extern char AltToLat[];

//exern funcions
#define WHAT_DAY      0
#define WHAT_HOUR     1
#define WHAT_YEAR     2
#define WHAT_POINT    3
#define WHAT_MINa     4
#define WHAT_MINu     5
#define WHAT_MONEYa   6
#define WHAT_MONEYu   7
#define WHAT_THINGa   8
#define WHAT_THINGu   9
#define WHAT_LEVEL    10
#define WHAT_MOVEa    11
#define WHAT_MOVEu    12
#define WHAT_ONEa     13
#define WHAT_ONEu     14
#define WHAT_SEC      15
#define WHAT_DEGREE   16
#define WHAT_MONTH    17
#define WHAT_GLORY    18 

/* public functions in utils.c */
char	*str_dup(const char *source);
char	*str_add(char *dst, const char *src);
char	*rustime(const struct tm *timeptr);
char    *race_or_title(struct char_data *ch);
char    *only_title(struct char_data *ch);
const char    *desc_count (int how_many, int of_what);
char    *noclan_title(struct char_data *ch);

bool cmpstr(const char *arg1, const char *arg2);
int	str_cmp(const char *arg1, const char *arg2);
int	strn_cmp(const char *arg1, const char *arg2, int n);
int	is_ascii(char c);
int     is_alpha(char c);
int     is_space(char c);
int	touch(const char *path);
int	number(int from, int to);
int	dice(int number, int size);
int  	sprintbit(bitvector_t vektor, const char *names[], char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *orig_name, char *filename, int mode);
int	num_pc_in_room(struct room_data *room);
int     pc_duration(struct char_data *ch,int cnst, int level, int level_divisor, int min, int max);
int     same_group(struct char_data *ch, struct char_data *tch);
int    	complex_spell_modifier(struct char_data *ch, int spellnum, int type, int value);
int    	on_horse(struct char_data *ch);
int    	has_horse(struct char_data *ch, int same_room);
int    	god_skill_modifier(struct char_data *ch, int skillnum, int type, int value);
int    	day_skill_modifier(struct char_data *ch, int skillnum, int type, int value);
int    	weather_skill_modifier(struct char_data *ch, int skillnum, int type, int value);
int    	complex_skill_modifier(struct char_data *ch, int skillnum, int type, int value);
int    	awaking(struct char_data *ch, int mode);
int    	check_awake(struct char_data *ch, int what);
int    	Convers_Flags(struct char_data *ch, unsigned long flags, int type);
int    	obj_on_auction(struct obj_data *obj);
int    	valid_email(const char *address);

void	pers_log(char *orig_name, const char *format, ...);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void	mudlog(const char *str, int type, int level, int file);
void	log_death_trap(struct char_data *ch);
void	sprinttype(int type, const char *names[], char *result);
void	sprintbits(FLAG_DATA flags, const char *names[], char *result, const char *div);
void	core_dump_real(const char *, int);
void    set_afk(struct char_data *ch, char *afk_message, int result);
void    horse_drop(struct char_data *ch);
void    make_horse(struct char_data *horse, struct char_data *ch);
void    check_horse(struct char_data *ch);
void    check_auction(struct char_data *ch, struct obj_data *obj);
void    clear_auction(int lot);
void    sell_auction(int lot);
void    tact_auction(void);
void    paste_mobiles(int zone);
void    circle_srandom(unsigned long initial_seed);
void    ustalost_timer(struct char_data *ch, int mod);
struct  time_info_data *age(struct char_data *ch);
struct  char_data *get_horse(struct char_data *ch);
struct  auction_lot_type *free_auction(int *lotnum);

#define core_dump()		core_dump_real(__FILE__, __LINE__)

/* random functions in random.c */

unsigned long circle_random(void);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
#define MMIN(a,b) ((a<b)?a:b)
#define MMAX(a,b) ((a<b)?b:a)


char *CAP(char *txt);

//в objsave.c

int    GetClanzone(struct char_data *ch);

/* in magic.c */
bool	circle_follow(struct char_data *ch, struct char_data * victim);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following, int checkmob);
int awake_hide(struct char_data *ch);
/* in limits.c */
int awake_sneak(struct char_data *ch);
int	    mana_gain(struct char_data *ch);
int	    hit_gain(struct char_data *ch);
int  	move_gain(struct char_data *ch);
void	advance_level(struct char_data *ch);
void	set_title(struct char_data *ch, char *title);
void	set_ptitle(struct char_data *ch, char *ptitle);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	update_pos(struct char_data *victim);
int     real_sector(int room);
int     check_moves(struct char_data *ch, int how_moves);
int     awake_camouflage(struct char_data *ch);
/* various constants *****************************************************/
#define STR 0
#define DEX 1
#define CON 2
#define INT 3
#define WIS 4
#define CHA 5



#define KtoW(c) ((ubyte)(c) < 128 ? (c) : KoiToWin[(ubyte)(c)-128])
#define KtoW2(c) ((ubyte)(c) < 128 ? (c) : KoiToWin2[(ubyte)(c)-128])
#define KtoA(c) ((ubyte)(c) < 128 ? (c) : KoiToAlt[(ubyte)(c)-128])
#define WtoK(c) ((ubyte)(c) < 128 ? (c) : WinToKoi[(ubyte)(c)-128])
#define AtoK(c) ((ubyte)(c) < 128 ? (c) : AltToKoi[(ubyte)(c)-128])
#define AtoL(c) ((ubyte)(c) < 128 ? (c) : AltToLat[(ubyte)(c)-128])

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

//Резисты для мобов по стихиям.
#define PROTECT_AIR			0	//воздух
#define PROTECT_FIRE			1	//огонь
#define PROTECT_WATER			2	//вода
#define PROTECT_EARTH			3	//земля
#define PROTECT_LIVE			4	//живучесть
#define PROTECT_INT			5	//защита разума
#define PROTECT_IMUNE			6	//иммунитет

/* get_filename() */
#define CRASH_FILE		  0
#define ETEXT_FILE	 	  1
#define ALIAS_FILE		  2
#define SCRIPT_VARS_FILE  3
#define PLAYERS_FILE      4
#define PKILLERS_FILE     5
#define PQUESTS_FILE      6
#define PMKILL_FILE       7
#define TEXT_CRASH_FILE   8
#define TIME_CRASH_FILE   9
#define PFILE_FILE	  10
#define PLAYERS_LOG       11
#define NAME_CHEST_FILE   12
#define PLAYER_EXCHANGE   13
#define PLAYER_MAIL       14
	
#define MAX_EXP_PERCENT   80

#define SF_EMPTY       (1 << 0)
#define SF_FOLLOWERDIE (1 << 1)
#define SF_MASTERDIE   (1 << 2)
#define SF_CHARMLOST   (1 << 3)

/* some awaking cases */
#define AWAKE_HIDE       (1 << 0)
#define AWAKE_INVIS      (1 << 1)
#define AWAKE_CAMOUFLAGE (1 << 2)
#define AWAKE_SNEAK      (1 << 3)

#define ACHECK_AFFECTS (1 << 0)
#define ACHECK_LIGHT   (1 << 1)
#define ACHECK_HUMMING (1 << 2)
#define ACHECK_GLOWING (1 << 3)
#define ACHECK_WEIGHT  (1 << 4)
/* breadth-first searching */
#define BFS_ERROR		-1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH		-3

#define MAX_AUCTION_LOT            3
#define MAX_AUCTION_TACT_BUY       3
#define MAX_AUCTION_TACT_PRESENT   (MAX_AUCTION_TACT_BUY + 3)
#define AUCTION_PULSES             30
#define GET_LOT(value) ((auction_lots+value))
/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time * Мудовское время тика и жизни*/
#define SECS_PER_MUD_HOUR		60
#define SECS_PER_MUD_DAY		(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH		(35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR		(17*SECS_PER_MUD_MONTH)
#define HOURS_PER_DAY			24
#define MONTHS_PER_YEAR			12
#define DAYS_PER_MONTH             	30


#define NEWMOONSTART               	27
#define NEWMOONSTOP                	1
#define HALFMOONSTART              	7
#define FULLMOONSTART              	13
#define FULLMOONSTOP               	15
#define LASTHALFMOONSTART          	21
#define MOON_CYCLE			28
#define WEEK_CYCLE			7
#define POLY_WEEK_CYCLE			9

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)
#define TIME_KOEFF             1
#define SECS_PER_PLAYER_AFFECT 2
/* string utils **********************************************************/
#define CHAR_DRUNKED               10
#define CHAR_MORTALLY_DRUNKED      16


#define YESNO(a) ((a) ? "&CДА&n " : "&RНЕТ&n")
#define ONOFF(a) ((a) ? "&CВКЛ&n" : "&RВЫК&n")

//ДКЪ ЙНХ-8 Б КНБЕПЕ МЕНАУНДХЛН +32 ОНЛЕМЪРЭ МЮ -32, Ю Б СООЕПЕ МЮНАНПНР-)
//ю
#define LOWER(c)        (((c)>='Ю'  && (c) <= 'Ъ') ? ((c)-(32)) : (c))

#define UPPER(c)      (((c)>='ю'  && (c) <= 'ъ') ? ((c)+(32)) : (c))

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 
#define IF_STR(st) ((st) ? (st) : "\0")

#define IS_NULLSTR(str)		(!(str) || *(char*)(str) == '\0') 

//#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

/* memory utils **********************************************************/

#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("SYSERR: realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/

#define IS_SET_IN		0    // используемые в новой функции определения
#define SET_BIT_IN		1
#define REMOVE_BIT_IN		2
#define TOGGLE_BIT_IN		3

#define IS_SET(flag,bit)  ((flag & 0x3FFFFFFF) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit & 0x3FFFFFFF))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit & 0x3FFFFFFF))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit & 0x3FFFFFFF))
/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

/*

#define PRF_FLAGS(ch,flag)  (GET_FLAG((ch)->player_specials->saved.pref, flag))


#define EXTRA_FLAGS(ch,flag)(GET_FLAG((ch)->Temporary, flag))*/

#define DESC_FLAGS(d)	((d)->options)

#define PLR_FLAGGED(ch, flag)   (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch,flag), (flag)))
#define MOB_FLAGGED(ch, flag)   (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch,flag), (flag)))
#define NPC_FLAGGED(ch, flag)   (IS_SET(NPC_FLAGS(ch,flag), (flag)))
#define AFF_FLAGGED(ch, flag)   (IS_SET(AFF_FLAGS(ch,flag), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS((loc),(flag)), (flag)))
#define DESC_FLAGGED(d, flag)   (IS_SET(DESC_FLAGS(d), (flag)))

#define MOB_FLAGS(ch,flag)      (GET_FLAG((ch)->char_specials.saved.Act,flag))
#define PLR_FLAGS(ch,flag)      (GET_FLAG((ch)->char_specials.saved.Act,flag))

#define NPC_FLAGS(ch,flag)	(GET_FLAG((ch)->mob_specials.Npc_Flags, flag))
#define AFF_FLAGS(ch,flag)      (GET_FLAG((ch)->char_specials.saved.affected_by, flag))
#define ROOM_FLAGS(loc,flag)    (GET_FLAG(world[(loc)].room_flags, flag))
//#define AFF_FLAGS(ch,flag)   ((unsigned long)flag < INT_ONE ? ch->char_specials.saved.affected_by.flags[0] : \
//						  ch->char_specials.saved.affected_by.flags[1])



/* See http://www.circlemud.org/~greerga/todo.009 to eliminate MOB_ISNPC. */

#define SET_EXTRA(ch,skill,vict)   {(ch)->extra_attack.used_skill = skill; (ch)->extra_attack.victim     = vict;}

									
#define EXTRA_FLAGGED(ch, flag)      (IS_SET(EXTRA_FLAGS(ch), (flag)))
#define GET_EXTRA_SKILL(ch)          ((ch)->extra_attack.used_skill)
#define GET_EXTRA_VICTIM(ch)         ((ch)->extra_attack.victim)
#define SET_CAST(ch,snum,dch,dobj)   (ch)->cast_attack.spellnum  = snum; \
                                     (ch)->cast_attack.tch       = dch; \
                                     (ch)->cast_attack.tobj      = dobj;
#define GET_CAST_SPELL(ch)	     ((ch)->cast_attack.spellnum)
#define GET_CAST_CHAR(ch)	     ((ch)->cast_attack.tch)
#define GET_CAST_OBJ(ch)	     ((ch)->cast_attack.tobj)
#define GET_CASTER_KRUG(ch)	     ((int)(GET_LEVEL(ch) * 10/int_app[POSI(GET_REAL_INT(ch))].maxkrug))


/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED(ch, skill))


#define EXTRA_FLAGS(ch)     		((ch)->Temporary)
#define PRF_FLAGS(ch)			((ch)->player_specials->saved.pref)
#define SPELL_ROUTINES(spl)		(spell_info[spl].routines)

/* See http://www.circlemud.org/~greerga/todo.009 to eliminate MOB_ISNPC. */
#define IS_NPC(ch)			(IS_SET(MOB_FLAGS(ch, MOB_ISNPC), MOB_ISNPC))
#define IS_MOB(ch)  			(IS_NPC(ch) && GET_MOB_RNUM(ch) >= 0)

#define PRF_FLAGGED(ch, flag) 		(IS_SET(PRF_FLAGS(ch), (flag)))
#define EXIT_FLAGGED(exit, flag) 	(IS_SET((exit)->exit_info, (flag)))
#define OBJVAL_FLAGGED(obj, flag) 	(IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag) 	(IS_SET((obj)->obj_flags.wear_flags, (flag)))
#define OBJ_FLAGGED(obj, flag)		(IS_SET(GET_OBJ_EXTRA(obj,flag), (flag)))
#define HAS_SPELL_ROUTINE(spl, flag) 	(IS_SET(SPELL_ROUTINES(spl), (flag)))


#define AFF_TOG_CHK(ch,flag) 		((TOGGLE_BIT(AFF_FLAGS(ch, flag), (flag))) & (flag))
#define PLR_TOG_CHK(ch,flag) 		((TOGGLE_BIT(PLR_FLAGS(ch, flag), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) 		((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))



/* room utils ************************************************************/

#define LIKE_ROOM(ch) ((IS_CLERIC(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_CLERIC)) || \
                       (IS_MAGIC_USER(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_MAGE)) || \
                       (IS_WARRIOR(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_WARRIOR)) || \
                       (IS_THIEF(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_THIEF)) || \
                       (IS_ASSASIN(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_ASSASINE)) || \
                       (IS_TAMPLIER(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_TAMPLIER)) || \
                       (IS_MONAH(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_MONAH)) || \
                       (IS_SLEDOPYT(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_SLEDOPYT)) || \
                       (IS_VARVAR(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_VARVAR)) || \
                       (IS_DRUID(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_DRUID)))

#define GET_ROOM_SKY(room) (world[room].weather.duration > 0 ? \
                            world[room].weather.sky : weather_info.sky)

#define SECT(room)	(world[(room)].sector_type)

#define IS_DEFAULTDARK(room) (ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 weather_info.sunlight == SUN_DARK )) )
#define IS_MOONLIGHT(room) ((GET_ROOM_SKY(room) == SKY_LIGHTNING && \
                             weather_info.moon_day >= FULLMOONSTART && \
                             weather_info.moon_day <= FULLMOONSTOP))


#define IS_DARK(room)      ((world[room].gdark > world[room].glight) || \
                            (!(world[room].gdark < world[room].glight) && \
                             !(world[room].light+world[room].fires) && \
                              (ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 (weather_info.sunlight == SUN_DARK && \
                                  !IS_MOONLIGHT(room)) )) ) ) )

#define IS_TIMEDARK(room)  ((world[room].gdark > world[room].glight) || \
                            (!(world[room].light+world[room].fires+world[room].glight) && \
                              (ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 weather_info.sunlight == SUN_DARK )) ) ) )

#define IS_LIGHT(room)  (!IS_DARK(room))
#define DARK			400 // темнота в соседней комнате


#define VALID_RNUM(rnum)	((rnum) >= 0 && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) (VALID_RNUM(room) ? world[(room)].func : NULL)

/* char utils ************************************************************/


#define IN_ROOM(ch)				((ch)->in_room)
#define GET_WAS_IN(ch)			((ch)->was_in_room)
#define GET_AGE(ch)				(age(ch)->year)
#define GET_REAL_AGE(ch)		(age(ch)->year + GET_AGE_ADD(ch))
								
#define GET_PC_NAME(ch)			((ch)->player.name)
#define GET_NAME(ch)			(IS_NPC(ch) ? \
								(ch)->player.short_descr : GET_PC_NAME(ch))
#define GET_RNAME(ch)			((ch)->player.rname)
#define GET_DNAME(ch)			((ch)->player.dname)
#define GET_VNAME(ch)			((ch)->player.vname)
#define GET_TNAME(ch)			((ch)->player.tname)
#define GET_PNAME(ch)			((ch)->player.pname)
	
#define GET_PTITLE(ch)			((ch)->player.ptitle)
#define GET_TITLE(ch)			((ch)->player.title)
#define GET_LEVEL(ch)			((ch)->player.level)
#define GET_PASSWD(ch)			((ch)->player.passwd)
#define GET_PFILEPOS(ch)		((ch)->pfilepos)
#define GET_REMORT(ch)			((ch)->player_specials->saved.Remorts)
//ДОбваление флага для обработки 
#define TOUCHING(ch)		   	((ch)->Touching)
#define PROTECTING(ch)		   	((ch)->Protecting)
#define GET_AF_BATTLE(ch,flag) 		(IS_SET((ch)->BattleAffects, flag))
#define SET_AF_BATTLE(ch,flag) 		(SET_BIT((ch)->BattleAffects,flag))
#define CLR_AF_BATTLE(ch,flag) 		(REMOVE_BIT((ch)->BattleAffects,flag))
#define NUL_AF_BATTLE(ch)      		((ch)->BattleAffects = 0)
/*#define GET_AF_BATTLE(ch,flag) 	(IS_SET(GET_FLAG((ch)->BattleAffects, flag),flag))
#define SET_AF_BATTLE(ch,flag) 		(SET_BIT(GET_FLAG((ch)->BattleAffects,flag),flag))
#define CLR_AF_BATTLE(ch,flag) 		(REMOVE_BIT(GET_FLAG((ch)->BattleAffects, flag),flag))
#define NUL_AF_BATTLE(ch)      		(GET_FLAG((ch)->BattleAffects, 0) = \
                                	GET_FLAG((ch)->BattleAffects, INT_ONE) = \
                                	GET_FLAG((ch)->BattleAffects, INT_TWO) = \
                                	GET_FLAG((ch)->BattleAffects, INT_THREE) = 0)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */

#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))
#define POSI(val)			((val < 50) ? ((val > 0) ? val : 1) : 50)
#define VPOSI(val,min,max)      	((val < max) ? ((val > min) ? val : min) : max)


#define GET_CLASS(ch)			((ch)->player.chclass)
#define GET_RACE(ch)			((ch)->player.race)
#define GET_SEX(ch)			((ch)->player.sex)
#define GET_HEIGHT(ch)			((ch)->player.height)
#define GET_HEIGHT_ADD(ch)		((ch)->add_abils.height_add)
#define GET_REAL_HEIGHT(ch)		(GET_HEIGHT(ch) + GET_HEIGHT_ADD(ch))
#define GET_WEIGHT(ch)			((ch)->player.weight)
#define GET_WEIGHT_ADD(ch)		((ch)->add_abils.weight_add)
#define GET_REAL_WEIGHT(ch)		(GET_WEIGHT(ch) + GET_WEIGHT_ADD(ch))
#define GET_SIZE(ch)			((ch)->real_abils.size)
#define GET_SIZE_ADD(ch)		((ch)->add_abils.size_add)
#define GET_REAL_SIZE(ch)		(VPOSI(GET_SIZE(ch) + GET_SIZE_ADD(ch), 1, 70))
#define GET_POS_SIZE(ch)		(POSI(GET_REAL_SIZE(ch) >> 1))
#define GET_STR(ch)			((ch)->real_abils.str)
#define GET_REAL_STR(ch)		(POSI(GET_STR(ch) + GET_STR_ADD(ch)))
#define GET_STR_ADD(ch)			((ch)->add_abils.str_add)
#define GET_DEX(ch)			((ch)->real_abils.dex)
#define GET_DEX_ADD(ch)			((ch)->add_abils.dex_add)
#define GET_REAL_DEX(ch)		(POSI(GET_DEX(ch)+GET_DEX_ADD(ch)))
#define GET_INT(ch)			((ch)->real_abils.intel)
#define GET_INT_ADD(ch)			((ch)->add_abils.intel_add)
#define GET_REAL_INT(ch)		(POSI(GET_INT(ch) + GET_INT_ADD(ch)))
#define GET_WIS(ch)			((ch)->real_abils.wis)
#define GET_WIS_ADD(ch)			((ch)->add_abils.wis_add)
#define GET_REAL_WIS(ch)		(POSI(GET_WIS(ch) + GET_WIS_ADD(ch)))
#define GET_CON(ch)			((ch)->real_abils.con)
#define GET_CON_ADD(ch)			((ch)->add_abils.con_add)
#define GET_REAL_CON(ch)		(POSI(GET_CON(ch) + GET_CON_ADD(ch)))
#define GET_CHA(ch)			((ch)->real_abils.cha)
#define GET_CHA_ADD(ch)			((ch)->add_abils.cha_add)
#define GET_REAL_CHA(ch)	    	(POSI(GET_CHA(ch) + GET_CHA_ADD(ch)))
#define GET_REAL_MAX_HIT(ch)		(GET_MAX_HIT(ch) + GET_HIT_ADD(ch))
#define GET_HR(ch)			((ch)->real_abils.hitroll)
#define GET_HR_ADD(ch)			((ch)->add_abils.hr_add)
#define GET_REAL_HR(ch)			(VPOSI(GET_HR(ch)+GET_HR_ADD(ch), -50, 100))
#define GET_MANA_ADD(ch)		((ch)->add_abils.mana_add)
#define GET_REAL_MAX_MANA(ch)		(GET_MAX_MANA(ch) + GET_MANA_ADD(ch)) 
#define GET_AC(ch)			((ch)->real_abils.armor)
#define GET_AC_ADD(ch)			((ch)->add_abils.ac_add)
#define GET_REAL_AC(ch)			(GET_AC(ch)+GET_AC_ADD(ch))
#define GET_DR(ch)			((ch)->real_abils.damroll)
#define GET_DR_ADD(ch)			((ch)->add_abils.dr_add)
#define GET_REAL_DR(ch)			(VPOSI(GET_DR(ch)+GET_DR_ADD(ch), -50, 100))
#define GET_CAST_SUCCESS(ch)    	((ch)->add_abils.cast_success)
#define GET_POISON(ch)		    	((ch)->add_abils.poison_add)
#define GET_MORALE(ch)          	((ch)->add_abils.morale_add)
#define GET_DRUNK_STATE(ch)  		((ch)->player_specials->saved.DrunkState)
// points structur
#define GET_EXP(ch)			((ch)->points.exp)
#define GET_HIT(ch)			((ch)->points.hit)
#define GET_MAX_HIT(ch)			((ch)->points.max_hit)
#define GET_MOVE(ch)			((ch)->points.move)
#define GET_MAX_MOVE(ch)		((ch)->points.max_move)
#define GET_MANA(ch)			((ch)->points.mana)
#define GET_MAX_MANA(ch)		((ch)->points.max_mana)
#define GET_GOLD(ch)			((ch)->points.gold)
#define GET_BANK_GOLD(ch)		((ch)->points.bank_gold)
#define RENTABLE(ch)		        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->may_rent))
#define GET_REAL_MAX_MOVE(ch)   	(GET_MAX_MOVE(ch) + GET_MOVE_ADD(ch))

#define CHECK_AGRO(ch)          	((ch)->CheckAggressive)
#define GET_MANA_NEED(ch)       	((ch)->ManaMemNeeded)
#define GET_MANA_STORED(ch)	    	((ch)->ManaMemStored)
#define GET_CASTER(ch)			((ch)->CasterLevel)
#define GET_DAMAGE(ch)			((ch)->DamageLevel)
#define GET_POS(ch)			((ch)->char_specials.position)
#define GET_CONPOS(ch)			((ch)->char_specials.ConPos)
#define GET_IDNUM(ch)			((ch)->char_specials.saved.idnum)
#define IS_CARRYING_W(ch)		((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch)		((ch)->char_specials.carry_items)
#define FIGHTING(ch)			((ch)->char_specials.fighting)
#define HUNTING(ch)			((ch)->char_specials.hunting)

#define GET_LASTIP(ch)         		((ch)->player_specials->saved.LastIP)
#define GET_EMAIL(ch)           	((ch)->player_specials->saved.EMail)
#define GET_USTALOST(ch)        	((ch)->player_specials->saved.Ustalost)
#define GET_UTIMER(ch)          	((ch)->player_specials->saved.Utimer)
#define GET_GLORY(ch)           	((ch)->player_specials->saved.glory)
#define GET_ALIGNMENT(ch)		((ch)->char_specials.saved.alignment)
#define IS_GOOD(ch)			(GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)			(GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch)			(!IS_GOOD(ch) && !IS_EVIL(ch))
#define CALC_ALIGNMENT(ch)      	((GET_ALIGNMENT(ch) <= ALIG_EVIL_LESS) ? ALIG_EVIL : \
					(GET_ALIGNMENT(ch) >= ALIG_GOOD_MORE) ? ALIG_GOOD : ALIG_NEUT)
#define ALIG_GOOD 0
#define ALIG_NEUT 1
#define ALIG_EVIL 2

#define ALIG_EVIL_LESS			-350
#define ALIG_GOOD_MORE			350                                
/*#define SAME_ALIGN(ch,vict)		((IS_GOOD(ch) && IS_GOOD(vict)) ||\
					(IS_EVIL(ch) && IS_EVIL(vict)) ||\
					(IS_NEUTRAL(ch) && IS_NEUTRAL(vict))) */
#define ALIGN_DELTA  10
#define SAME_ALIGN(ch,vict) 		 (GET_ALIGNMENT(ch) > GET_ALIGNMENT(vict)?\
                             		 (GET_ALIGNMENT(ch) - GET_ALIGNMENT(vict))<=ALIGN_DELTA:\
                              		(GET_ALIGNMENT(vict) - GET_ALIGNMENT(ch))<=ALIGN_DELTA\
                             		)

#define GET_RELIGION(ch)		((ch)->player_specials->saved.Religion)
#define GET_ID(x)			((x)->id)
#define GET_HELPER(ch)			((ch)->helpers)
#define GET_LIKES(ch)		    	((ch)->mob_specials.like_work)

#define GET_MANAREG(ch)			((ch)->add_abils.manareg)
#define GET_HITREG(ch)			((ch)->add_abils.hitreg)
#define GET_MOVEREG(ch)			((ch)->add_abils.movereg)
#define GET_ARMOUR(ch)			((ch)->add_abils.armour)
#define GET_ABSORBE(ch)			((ch)->add_abils.absorb)
#define GET_AGE_ADD(ch)			((ch)->add_abils.age_add)
#define GET_HIT_ADD(ch)			((ch)->add_abils.hit_add)
#define GET_MOVE_ADD(ch)		((ch)->add_abils.move_add)
#define GET_SAVE(ch,i)			((ch)->add_abils.apply_saving_throw[i])
#define GET_RESIST(ch,i)		((ch)->add_abils.apply_resistance_throw[i])
#define GET_DRESIST(ch)			((ch)->add_abils.d_resist)
#define GET_ARESIST(ch)			((ch)->add_abils.a_resist)
#define GET_SLOT(ch,i)			((ch)->add_abils.slot_add[i])

#define EXTRACT_TIMER(ch)     		((ch)->ExtractTimer)

#define GET_DEST(ch)        		(((ch)->mob_specials.dest_count ? \
                              		(ch)->mob_specials.dest[(ch)->mob_specials.dest_pos] : \
                              		NOWHERE))
// Для квестеров
//GET_QUESTMOB(ch) - моб, участвующий в квесте
//GET_QUESTOBJ(ch) - объект,участвующий в квесте
//GET_QUESTGIVER(ch) - моб, дающий квест (пока не используется)
//GET_NEXTQUEST(ch) - времени до следующего квеста (в тиках, 1 тик - минута)
//GET_QUESTPOINTS(ch -  очки за полученный квест
//GET_COUNTDOWN(ch) - время на выполнение квеста.
#define GET_QUESTMOB(ch)     		((ch)->player_specials->saved.Questmob)
#define GET_QUESTOBJ(ch)     		((ch)->player_specials->saved.Questobj)
#define GET_QUESTOK(ch)	     		((ch)->player_specials->saved.QuestOk)
#define GET_NEXTQUEST(ch)    		((ch)->player_specials->saved.nextquest) 
#define GET_QUESTPOINTS(ch)  		((ch)->player_specials->saved.questpoints)
#define GET_COUNTDOWN(ch)		((ch)->player_specials->saved.Countdown)
#define GET_QUESTMAX(ch)     		((ch)->player_specials->saved.QuestGloryMax)
#define GET_TIME_KILL(ch)    		((ch)->player_specials->saved.spare20)	
#define GET_HOME(ch)		 	((ch)->player_specials->saved.hometown)                             	
#define GET_CLAN(ch)			((ch)->player_specials->saved.clan)
#define GET_CLAN_RANK(ch)		((ch)->player_specials->saved.clan_rank)

#define GET_COND(ch, i)			((ch)->player_specials->saved.conditions[(i)])
#define GET_LOADROOM(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
#define GET_PRACTICES(ch, num)		((ch)->player_specials->saved.spells_to_learn[num])
#define GET_INVIS_LEV(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
#define GET_WIMP_LEV(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))
#define GET_FREEZE_LEV(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.freeze_level))
#define GET_BAD_PWS(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
#define GET_TALK(ch, i)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))
#define POOFIN(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_LAST_OLC_TARG(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_CRBONUS(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->CRbonus))
#define AFK_DEFAULT			"Я в АФК, меня нет рядом с клавиатурой!"
#define GET_AFK(ch)			((ch)->player_specials->afk_message)
#define GET_STR_ROLL(ch)		((ch)->player_specials->str_roll)
#define GET_INT_ROLL(ch)		((ch)->player_specials->intel_roll)
#define GET_WIS_ROLL(ch)		((ch)->player_specials->wis_roll)
#define GET_DEX_ROLL(ch)		((ch)->player_specials->dex_roll)
#define GET_CON_ROLL(ch)		((ch)->player_specials->con_roll)
#define GET_CHA_ROLL(ch)		((ch)->player_specials->cha_roll)
#define GET_POINT_STAT(ch, i)		((ch)->player_specials->point_stat[i])
#define GET_LASTTELL(ch)	        ((ch)->player_specials->lasttell)
#define GET_TELL(ch,i)	                ((ch)->player_specials->remember)[i]
#define GET_CHEST(ch)			((ch)->player_specials->mChest)

#define MUTE_REASON(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->MuteReason))
#define DUMB_REASON(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->DumbReason))
#define HELL_REASON(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->HellReason))


#define BATTLECNTR(ch)			 ((ch)->BattleCounter)
#define INITIATIVE(ch)			 ((ch)->Initiative)
#define GET_INITIATIVE(ch)		 ((ch)->add_abils.initiative_add)
#define GET_SKILL(ch, i)		 ((ch)->real_abils.Skills[i])
#define SET_SKILL(ch, i, pct)		 ((ch)->real_abils.Skills[i] = pct)
#define GET_SPELL_TYPE(ch, i)		 ((ch)->real_abils.SplKnw[i])
#define GET_SPELL_MEM(ch, i)		 ((ch)->real_abils.SplMem[i])
#define SET_SPELL(ch, i, pct)		 ((ch)->real_abils.SplMem[i] = pct)
#define GET_COUNT_SKILL(ch)		 ((ch)->player_specials->saved.open_krug)

#define GET_EQ(ch, i)			 ((ch)->equipment[i])

#define GET_MOB_SPEC(ch)		 (IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define GET_MOB_RNUM(mob)		 ((mob)->nr)
#define GET_MOB_VNUM(mob)		 (IS_MOB(mob) ? \
								 mob_index[GET_MOB_RNUM(mob)].vnum : -1)

#define GET_DEFAULT_POS(ch)		((ch)->mob_specials.default_pos)
#define MEMORY(ch)				((ch)->mob_specials.memory)

#define STRENGTH_APPLY_INDEX(ch)  (GET_REAL_STR(ch))
        

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
#define CAN_CARRY_N(ch) (10 + (GET_REAL_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING && !AFF_FLAGGED(ch,AFF_SLEEP))
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

#define GET_GOD_FLAG(ch,flag)   (IS_SET((ch)->player_specials->saved.GodsLike,flag))
#define SET_GOD_FLAG(ch,flag)   (SET_BIT((ch)->player_specials->saved.GodsLike,flag))
#define CLR_GOD_FLAG(ch,flag)	(REMOVE_BIT((ch)->player_specials->saved.GodsLike,flag))
#define GET_COMMSTATE(ch)       ((ch)->player_specials->saved.Prelimit)
#define SET_COMMSTATE(ch,val)   ((ch)->player_specials->saved.Prelimit = (val))
#define IND_POWER_CHAR(ch)      ((ch)->player_specials->saved.IndPower)		//накопительная индивидуальная  мощность 01.03.2006г.
#define IND_SHOP_POWER(ch)	((ch)->player_specials->saved.IndShopPower)	//покупательская индивидуальная мощность 01.03.2006г.
#define POWER_STORE_CHAR(ch)	((ch)->player_specials->saved.PowerStoreChar)//индивидуальная мощьность до вступления в клан 01.03.2006г
#define LAST_LOGON(ch)          ((ch)->player_specials->saved.LastLogon)
#define MUTE_DURATION(ch)	    ((ch)->player_specials->saved.MuteDuration)
#define DUMB_DURATION(ch)		((ch)->player_specials->saved.DumbDuration)
#define FREEZE_DURATION(ch)		((ch)->player_specials->saved.FreezeDuration)
#define HELL_DURATION(ch)		((ch)->player_specials->saved.HellDuration)
#define GET_UNIQUE(ch)          ((ch)->player_specials->saved.unique)
#define NAME_DURATION(ch)		((ch)->player_specials->saved.NameDuration)

#define WAITLESS(ch)          (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
#define IS_IMMORTAL(ch)     (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_IMMORT || GET_COMMSTATE(ch)))
#define IS_GOD(ch)          (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GOD    || GET_COMMSTATE(ch)))
#define IS_GRGOD(ch)        (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GRGOD  || GET_COMMSTATE(ch)))
#define IS_GLGOD(ch)	    (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GLGOD  || GET_COMMSTATE(ch)))
#define IS_IMPL(ch)         (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_IMPL))
#define IS_HIGHGOD(ch)      (IS_IMPL(ch) && (GET_GOD_FLAG(ch,GF_HIGHGOD) || GET_COMMSTATE(ch)))
#define IS_KILLER(ch)       ((ch)->points.pk_counter)
#define GODS_DURATION(ch)  ((ch)->player_specials->saved.GodsDuration)

/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)                ((ch)->wait > 0)
#define GET_WAIT(ch)          GET_WAIT_STATE(ch)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
//#define GET_MOB_HOLD(ch)      ((AFF_FLAGGED((ch),AFF_HOLD) || AFF_FLAGGED(ch, AFF_HOLDALL)) ? 1 : 0)
#define GET_MOB_HOLD(ch)      ((AFF_FLAGGED((ch),AFF_HOLD)) ? 1 : 0)

/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    			((ch)->wait)
#define GET_PUNCTUAL_WAIT_STATE(ch)		((ch)->punctual_wait)//перенести в линух 03.01.2007 г. новая обработка
#define GET_PUNCTUAL_WAIT(ch)          		GET_PUNCTUAL_WAIT_STATE(ch)

//#define FORGET_ALL(ch) (CLR_MEMORY(ch); (memset((ch)->real_abils.SplMem,0,MAX_SPELLS+1)))

#define CLR_MEMORY(ch)  ((ch)->Memory.clear())
#define GET_ACTIVITY(ch)    ((ch)->mob_specials.activity)
#define GET_HORSESTATE(ch)  ((ch)->mob_specials.HorseState)
#define GET_LASTROOM(ch)    ((ch)->mob_specials.LastRoom)
#define GET_QUESTOR(ch)		((ch)->mob_specials.isquest)
#define OK_GAIN_EXP(ch,victim) ((!IS_NPC(ch) ? (!IS_NPC(victim) ? 0 : 1) : 1))


/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)	((d)->connected)
//#define IS_GOD(ch)		(!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GOD))

/* object utils **********************************************************/


#define GET_OBJ_TYPE(obj)		((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)		((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)		((obj)->obj_flags.cost_per_day)
#define GET_OBJ_RENTE(obj)		((obj)->obj_flags.cost_per_inv)
#define GET_OBJ_EXTRA(obj,flag)		(GET_FLAG((obj)->obj_flags.extra_flags,flag))
#define GET_OBJ_AFF(obj,flag)		(GET_FLAG((obj)->obj_flags.affects,flag))
#define GET_OBJ_WEAR(obj)		((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val)		((obj)->obj_flags.value[(val)])
#define GET_OBJ_SKILL(obj)		((obj)->obj_flags.tren_skill)
#define GET_OBJ_SPELL(obj)	    	((obj)->obj_flags.spell)
#define GET_OBJ_ZONE(obj)		((obj)->obj_flags.Obj_zone)
#define GET_OBJ_VROOM(obj)      	((obj)->obj_in_vroom)
#define OBJ_GET_LASTROOM(obj)		((obj)->room_was_in)
#define GET_OBJ_WEIGHT(obj)		((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)		((obj)->obj_flags.timer)
#define GET_OBJ_MATER(obj)		((obj)->obj_flags.material)
#define GET_OBJ_CUR(obj)		((obj)->obj_flags.ostalos)
#define GET_OBJ_MAX(obj)		((obj)->obj_flags.cred_point)
#define GET_OBJ_OWNER(obj)      	((obj)->obj_flags.Obj_owner)
#define GET_OBJ_MAKER(obj)		((obj)->obj_flags.Obj_maker)
#define GET_OBJ_PARENT(obj)		((obj)->obj_flags.Obj_parent)
#define GET_OBJ_DESTROY(obj)    	((obj)->obj_flags.Obj_destroyer)
#define GET_OBJ_ALIAS(obj)      	((obj)->short_description)
#define GET_OBJ_DESC(obj)       	((obj)->description)
#define GET_OBJ_ACT(obj)        	((obj)->action_description)
#define GET_OBJ_RLVL(obj)		((obj)->obj_flags.levels)
#define GET_OBJ_LEVEL(obj)	    	((obj)->obj_flags.level_spell)
#define GET_OBJ_AFFECTS(obj)       	((obj)->obj_flags.affects) 
#define GET_OBJ_ANTI(obj)       	((obj)->obj_flags.anti_flag)
#define GET_OBJ_NO(obj)         	((obj)->obj_flags.no_flag)
#define GET_OBJ_RNUM(obj)		((obj)->item_number)
#define GET_OBJ_VNUM(obj)		(GET_OBJ_RNUM(obj) >= 0 ? \
					 obj_index[GET_OBJ_RNUM(obj)].vnum : -1)

#define IS_CORPSE(obj)			(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), 3) == 1)
#define IS_OBJ_STAT(obj,stat)		(IS_SET(GET_FLAG((obj)->obj_flags.extra_flags, \
                                                  stat), stat))
#define IS_OBJ_ANTI(obj,stat) 		(IS_SET(GET_FLAG((obj)->obj_flags.anti_flag, \
                                                  stat), stat))
#define IS_OBJ_NO(obj,stat)       	(IS_SET(GET_FLAG((obj)->obj_flags.no_flag, \
                                                  stat), stat))
#define IS_OBJ_AFF(obj,stat)    	(IS_SET(GET_FLAG((obj)->obj_flags.affects, \
                                                  stat), stat))
#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
	(obj_index[(obj)->item_number].func) : NULL)

#define OBJ_WHERE(obj) ((obj)->worn_by    ? IN_ROOM(obj->worn_by) : \
                        (obj)->carried_by ? IN_ROOM(obj->carried_by) : (obj)->in_room)
#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))

#define OK_BOTH(ch,obj)  (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].wield_w + str_app[STRENGTH_APPLY_INDEX(ch)].hold_w)

#define OK_WIELD(ch,obj) (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)

#define OK_HELD(ch,obj)  (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].hold_w)


// Various macros building up to CAN_SEE
#define MAY_SEE(sub,obj) (!AFF_FLAGGED((sub),AFF_BLIND) && \
                         (!IS_DARK(IN_ROOM(sub)) || AFF_FLAGGED((sub),AFF_INFRAVISION)) && \
						 (!AFF_FLAGGED((obj),AFF_MENTALLS)  || AFF_FLAGGED((sub),AFF_DETECT_MENTALLS)) && \
						 (!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED((sub),AFF_DETECT_INVIS)))
			  
#define MAY_ATTACK(sub)  (!AFF_FLAGGED((sub),AFF_CHARM)     && \
                          !IS_HORSE((sub))                  && \
			  !AFF_FLAGGED((sub),AFF_STOPFIGHT) && \
			  !AFF_FLAGGED((sub),AFF_HOLD)      && \
			  !AFF_FLAGGED((sub),AFF_HOLDALL)   && \
			  !MOB_FLAGGED((sub),MOB_NOFIGHT)   && \
			  !PLR_FLAGGED((sub), PLR_IMMKILL)  && \
			  GET_WAIT(sub) <= 0                && \
			  !FIGHTING(sub)                    && \
			  GET_POS(sub) >= POS_RESTING)
/* compound utilities and other macros **********************************/
#define SENDOK(ch)	(((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
		         (to_sleeping || AWAKE(ch)) && \
	                  !PLR_FLAGGED((ch), PLR_WRITING))

#define HERE(ch)  ((IS_NPC(ch) || (ch)->desc || RENTABLE(ch)))
/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "его": (GET_SEX(ch) == SEX_FEMALE ? "ее" : "их")) :"его")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "он": (GET_SEX(ch) == SEX_FEMALE ? "она" : "они")) :"оно")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "ему": (GET_SEX(ch) == SEX_FEMALE ? "ей" : "им")) :"ему")

#define OSHR(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "его": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "ее" : "их")) :"его")
#define OSSH(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "он": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "она" : "они")) :"оно")
#define OMHR(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "ему": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "ей" : "им")) :"ему")


#define GET_OBJ_MIW(obj) ((obj)->max_in_world)
#define GET_OBJ_SEX(obj) ((obj)->obj_flags.pol)


#define GET_CH_SUF_1(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "о" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "а" : "и")
#define GET_CH_SUF_2(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "ось" :\
                          GET_SEX(ch) == SEX_MALE ? "ся"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ась" : "ись")
#define GET_CH_SUF_3(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "ое" :\
                          GET_SEX(ch) == SEX_MALE ? "ый"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ая" : "ые")
#define GET_CH_SUF_4(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "ло" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ла" : "ли")
#define GET_CH_SUF_5(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "ло" :\
                          GET_SEX(ch) == SEX_MALE ? "ел"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ла" : "ли")
#define GET_CH_SUF_6(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "о" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "а" : "ы")
#define GET_CH_SUF_7(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "ет" :\
                          GET_SEX(ch) == SEX_MALE ? "ет"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ет" : "ут")
#define GET_CH_SUF_8(ch) (GET_SEX(ch) == SEX_NEUTRAL ? "ет" :\
                          GET_SEX(ch) == SEX_MALE ? "ет"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ет" : "ют")

#define GET_CH_VIS_SUF_1(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "о" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "а" : "и")
#define GET_CH_VIS_SUF_2(ch,och) (!CAN_SEE(och,ch) ? "ся" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "ось" :\
                          GET_SEX(ch) == SEX_MALE ? "ся"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ась" : "ись")
#define GET_CH_VIS_SUF_3(ch,och) (!CAN_SEE(och,ch) ? "ый" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "ое" :\
                          GET_SEX(ch) == SEX_MALE ? "ый"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ая" : "ые")
#define GET_CH_VIS_SUF_4(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "ло" :\
                          GET_SEX(ch) == SEX_MALE ? ""  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ла" : "ли")
#define GET_CH_VIS_SUF_5(ch,och) (!CAN_SEE(och,ch) ? "ел" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "ло" :\
                          GET_SEX(ch) == SEX_MALE ? "ел"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ла" : "ли")
#define GET_CH_VIS_SUF_6(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "ым" :\
                          GET_SEX(ch) == SEX_MALE ? "ым"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ой" : "ыми")
#define GET_CH_VIS_SUF_7(ch,och) (!CAN_SEE(och,ch) ? "ет" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "ет" :\
                          GET_SEX(ch) == SEX_MALE ? "ет"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ет" : "ут")

#define GET_CH_VIS_SUF_8(ch,och) (!CAN_SEE(och,ch) ? "ет" :\
                          GET_SEX(ch) == SEX_NEUTRAL ? "ет" :\
                          GET_SEX(ch) == SEX_MALE ? "ет"  :\
                          GET_SEX(ch) == SEX_FEMALE ? "ет" : "ют")



#define GET_OBJ_SUF_1(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "о" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "а" : "и")
#define GET_OBJ_SUF_2(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ось" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ся"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ась" : "ись")
#define GET_OBJ_SUF_3(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ое" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ый"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ая" : "ые")
#define GET_OBJ_SUF_4(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ло" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ла" : "ли")
#define GET_OBJ_SUF_5(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ло" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ла" : "ли")
#define GET_OBJ_SUF_6(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "о" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "а" : "ы")
#define GET_OBJ_SUF_7(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ет" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ет"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ет" : "ут")
#define GET_OBJ_SUF_8(obj) (GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ет" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ет"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ет" : "ют")


#define GET_OBJ_VIS_SUF_1(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "о" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "о" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "а" : "и")
#define GET_OBJ_VIS_SUF_2(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ось" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ось" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ся"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ась" : "ись")
#define GET_OBJ_VIS_SUF_3(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ый" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ое" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ый"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ая" : "ые")
#define GET_OBJ_VIS_SUF_4(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ло" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ло" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ла" : "ли")
#define GET_OBJ_VIS_SUF_5(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ло" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ло" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? ""  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ла" : "ли")
#define GET_OBJ_VIS_SUF_6(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ым" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ым"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ой" : "ыми")
#define GET_OBJ_VIS_SUF_7(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ет" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ет" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ет"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ет" : "ут")
#define GET_OBJ_VIS_SUF_8(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ет" :\
                            GET_OBJ_SEX(obj) == SEX_NEUTRAL ? "ет" :\
                            GET_OBJ_SEX(obj) == SEX_MALE ? "ет"  :\
                            GET_OBJ_SEX(obj) == SEX_FEMALE ? "ет" : "ют")
#define SIELENCE ("Вы не в состоянии произнести ни слова.\r\n")
/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || AFF_FLAGGED((sub), AFF_INFRAVISION)))

#define INVIS_OK(sub, obj) \
 (!AFF_FLAGGED((sub), AFF_BLIND) && \
  (  (!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED((sub),AFF_DETECT_INVIS)) && \
  (!AFF_FLAGGED((obj),AFF_MENTALLS) || AFF_FLAGGED((sub),AFF_DETECT_MENTALLS)) && \
	((!AFF_FLAGGED((obj), AFF_HIDE) && !AFF_FLAGGED((obj), AFF_CAMOUFLAGE)) || \
    AFF_FLAGGED((sub), AFF_SENSE_LIFE) \
   ) \
  ) \
 )



#define MORT_CAN_SEE(sub, obj) (HERE(obj) && \
                                INVIS_OK(sub, obj) && \
                                (IS_LIGHT((obj)->in_room) || \
                                 AFF_FLAGGED((sub), AFF_INFRAVISION) \
                                ) \
                               )


#define IMM_CAN_SEE(sub, obj) \
   (IS_GLGOD(sub) || MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= ((IS_NPC(obj)) ? 0 : GET_INVIS_LEV(obj))) && \
   IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */

#define INVIS_OK_OBJ(sub, obj) \
  (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj) \
  (INVIS_OK_OBJ(sub, obj) && !AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT(IN_ROOM(sub)) || OBJ_FLAGGED(obj, ITEM_GLOW) || \
    (IS_CORPSE(obj) && AFF_FLAGGED(sub, AFF_INFRAVISION))))

#define CAN_SEE_OBJ(sub, obj) \
   (obj->worn_by    == sub || \
    obj->carried_by == sub || \
    (obj->in_obj && (obj->in_obj->worn_by == sub || obj->in_obj->carried_by == sub)) || \
    MORT_CAN_SEE_OBJ(sub, obj) || \
    (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define MORT_CAN_SEE_CHAR(sub, obj) (HERE(obj) && \
                                     INVIS_OK(sub,obj) \
				    )
#define CAN_SEE_CHAR(sub, obj) (SELF(sub, obj) || \
        ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
         IMM_CAN_SEE_CHAR(sub, obj)))
#define IMM_CAN_SEE_CHAR(sub, obj) \
        (MORT_CAN_SEE_CHAR(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))



#define CAN_WEAR_ANY(obj) (((obj)->obj_flags.wear_flags>0) && ((obj)->obj_flags.wear_flags != ITEM_WEAR_TAKE))

#define PERS(ch, vict)    (CAN_SEE(vict, ch) ? GET_NAME(ch) : "кто-то")
#define RPERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_RNAME(ch) : "кого-то") 
#define DPERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_DNAME(ch) : "кому-то")
#define VPERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_VNAME(ch) : "кого-то")
#define TPERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_TNAME(ch) : "кем-то")
#define PPERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_PNAME(ch) : "ком-то")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "что-то")
#define ROBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_rdescription  : "чего-то")
#define DOBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_ddescription  : "чему-то")
#define VOBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_vdescription  : "чего-то")
#define TOBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_tdescription  : "чем-то")
#define POBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_pdescription  : "чем-то")


#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "что-то")

#define EXITDATA(room,door) ((room >= 0 && room < top_of_world) ? \
                             world[room].dir_option[door] : NULL)

#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])
#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])


#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))


#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : pc_class_types[(int)GET_CLASS(ch)])

#define IS_MAGIC_USER(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGIC_USER))
#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_CLERIC))
#define IS_THIEF(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_THIEF))
#define IS_WARRIOR(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WARRIOR))
#define IS_VARVAR(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_VARVAR))
#define IS_DRUID(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_DRUID))
#define IS_SLEDOPYT(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_SLEDOPYT))
#define IS_TAMPLIER(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_TAMPLIER))
#define IS_ASSASIN(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_ASSASINE))
#define IS_TITAN(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_TITAN))
#define IS_MONAH(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MONAH))
#define IS_ASSASINE(ch)        IS_ASSASIN(ch)

#define IS_WEDMAK(ch)		(!IS_NPC(ch) &&\
				(GET_CLASS(ch) == CLASS_WEDMAK))

#define IS_HUMAN(ch)	(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_HUMAN))
#define IS_ELF(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_ELF))
#define IS_GNOME(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_GNOME))
#define IS_DWARF(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_DWARF))
#define IS_HOBBIT(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_HOBBIT))
#define IS_POLUELF(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_POLUELF))
#define IS_OGR(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_OGR))
#define IS_LESELF(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_LESELF))

#define IS_CHARMICE(ch)    (IS_NPC(ch) && \
            (AFF_FLAGGED(ch,AFF_HELPER) && AFF_FLAGGED(ch,AFF_CHARM)))


#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))

#define IS_HORSE(ch) (IS_NPC(ch) && (ch->master) && AFF_FLAGGED(ch, AFF_HORSE))
/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE 1
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
//#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
#define CRYPT(a,b) (a)
//#else
//#define CRYPT(a,b) ((char *) crypt((a),(b)))
//#endif



