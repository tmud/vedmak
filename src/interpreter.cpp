/* *****************************************************	*******************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "constants.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
//#include "house.h"
#include "mail.h"
#include "screen.h"
#include "dg_scripts.h"
#include "pk.h"
#include "tedit.h"
#include "clan.h"
#include "exchange.h"

extern struct pclean_criteria_data pclean_criteria[];

extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;
extern const char *class_menu;
extern long lastnews;
extern char *motd;
extern char *imotd;
extern char *background;
extern const char *MENU;
extern const char *WELC_MESSG;
extern const char *START_MESSG;
extern const char *GREETINGS;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern const char *race_menu;
extern struct cheat_list_type *cheaters_list;

// external functions
void	skip_wizard(struct char_data *ch, char *string);
void	read_aliases(struct char_data *ch);
void	trigedit_parse(struct descriptor_data *d, char *arg);
void	do_aggressive_mob(struct char_data *ch, bool check_sneak);
void	read_saved_vars(struct char_data *ch);
void	set_binary(struct descriptor_data *d);
void	echo_on(struct descriptor_data *d);
void	echo_off(struct descriptor_data *d);
void	do_start(struct char_data *ch, int newbie);
void	roll_real_abils(struct char_data *ch, int rollstat);
int     Clan_news(struct descriptor_data *d, int imm);
void    Clan_fix(struct char_data *ch);
int     Is_Valid_Dc(char *newname);
int	calc_loadroom(struct char_data *ch);
int	parse_class(int arg);
int	delete_char(char *name);
int	special(struct char_data *ch, int cmd, char *arg);
int	isbanned(char *hostname);
int	isproxi(char *hostname);
int	Valid_Name(char *newname);
int 	const parse_race(int arg);
int     do_social(struct char_data *ch, char *argument);
int     find_action(char *cmd);

// local functions
struct	alias_data *find_alias(struct alias_data *alias_list, char *str);
void	free_alias(struct alias_data *a);
void	perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int	perform_alias(struct descriptor_data *d, char *orig);
int	reserved_word(char *argument);
int	find_name(char *name);
int	_parse_name(char *arg, char *name);
int	perform_dupe_check(struct descriptor_data *d);

// prototypes for all do_x functions.
ACMD(do_affects);
ACMD(do_advance);
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_at);
//ACMD(do_action);
ACMD(do_afk);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_bash_door);
ACMD(do_block);
ACMD(do_blade_vortex);
ACMD(do_bedname);
ACMD(do_cast);
ACMD(do_camouflage);
ACMD(do_color);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_courage);
ACMD(do_cheat);
ACMD(do_credits);
ACMD(do_clan);
ACMD(do_ctell);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_deviate);
ACMD(do_diagnose);
ACMD(do_disarm);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exchange);
ACMD(do_exits);
ACMD(do_ident);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_forget);
ACMD(do_forgive);
ACMD(do_findhelpee);
ACMD(do_fire);
ACMD(do_firstaid);
ACMD(do_stopfight);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_givehorse);
ACMD(do_stophorse);
ACMD(do_sense);
ACMD(do_style);
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hidetrack);
ACMD(do_hit);
ACMD(do_horseon);
ACMD(do_horseoff);
ACMD(do_horseget);
ACMD(do_horseput);
ACMD(do_horsetake);
ACMD(do_info);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_mkill);
ACMD(do_mark);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_learn);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_look);
ACMD(do_looking);
ACMD(do_liblist);
ACMD(do_revenge);
ACMD(do_remembertell);
ACMD(do_protect);
ACMD(do_prompt);
ACMD(do_pray_gods);
ACMD(do_proxi);
ACMD(do_unproxi);
ACMD(do_transform_weapon);

/* ACMD(do_move); -- interpreter.h */
ACMD(do_mjunk);
ACMD(do_mpurge);
ACMD(do_makefood);
ACMD(do_mighthit);
ACMD(do_might);
ACMD(do_multyparry);
ACMD(do_not_here);
ACMD(do_offer);
ACMD(do_olc);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pary);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_podsek);
ACMD(do_practice);
ACMD(do_ptitle);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_poisoned);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_qwest);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_razd);
ACMD(do_remember);
ACMD(do_repair);
ACMD(do_remort);
ACMD(do_save);
ACMD(do_say);
ACMD(do_score);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_tstat);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_scan);
ACMD(do_sides);
ACMD(do_stupor);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_tear_skin);
ACMD(do_time);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_tren_skill);
ACMD(do_tochny);
ACMD(do_umenia);
ACMD(do_weapon_umenia);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_upgrade);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_zackl);
ACMD(do_znak);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);
ACMD(do_journal);

/* TRIGGERS for ACMD*/
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tlist);
ACMD(do_masound);
ACMD(do_mdoor);
ACMD(do_mdamage);
ACMD(do_mechoaround);
ACMD(do_mecho);
ACMD(do_msend);
ACMD(do_mload);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mexp);
ACMD(do_mgold);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mskillturn);
ACMD(do_mskilladd);
ACMD(do_mspellturn);
ACMD(do_mspelladd);
ACMD(do_mspellitem);
ACMD (do_mportal);
ACMD(do_vdelete);
ACMD(do_mhunt);
ACMD(do_throw);
ACMD(do_multing);

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */
#define MAGIC_LEN 8
const char *create_name_rules =

"\r\n Запрещено использовать имена собственные, составлять имена из букв латиницы.\r\n"
" Запрещено использовать оскорбительное содержание и мат в создаваемом имени.\r\n"
" Примеры неправильных имен: John, ИванИваныч, Чучело, Прачка, Санитар и т.п.\r\n" 
" Герой, чье имя нарушает правила будет съеден божественным драконом.\r\n\r\n";


/*"\r\n                          &cПРАВИЛА ВЫБОРА ИМЕН ИГРОВЫХ ПЕРСОНАЖЕЙ&n\r\n"
"\r\n"
"               &RПрежде чем Вы продолжите создавать персонажа, еще раз подумайте,\r\n"
"       корректное ли игровое ИМЯ выбрали Вы и неокажетесь ли в &cКОМНАТЕ ПЕРЕИМЕНОВАНИЯ!&n\r\n"
"\r\n"
"&y1. Бессмертные,благодаря которым, создан МИР, тщательно будут следить\r\n"
"   за ИМЕНАМИ, и если они посчитают Ваше имя несоответствующим игровой политике,\r\n"
"   этого мира - то Вам придется смириться. У Вас будет шанс для переименования добровольно.\r\n"
"2. Исходя из каких критериев должно выбираться имя Вашего героя?\r\n"
"   Скорее всего это Имя должно соответствовать Миру Фэнтези.\r\n"
"3. Ваше Имя не должно быть Именем Богов различных эпосов и мифологий,\r\n"
"   &CЗАПРЕЩЕНО&y использовать Имена различных выдающихся личностей времен и народов,\r\n"
"   а так же, широко известных литературных героев.\r\n"
"   Мы надеемся что у Вас достаточно фантазии для того что бы Вы смогли действительно\r\n"
"   выбрать Имя, которое бы соответствовало данной тематике.&n\r\n\r\n";*/

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH, -2 },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST,  -2 },
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH, -2 },
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST,  -2 },
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP,    -2 },
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN,  -2 },
  /*русские команды движения*/
  { "север"    , POS_STANDING, do_move     , 0, SCMD_NORTH, -2 },
  { "восток"   , POS_STANDING, do_move     , 0, SCMD_EAST,  -2 },
  { "юг"       , POS_STANDING, do_move     , 0, SCMD_SOUTH, -2 },
  { "запад"    , POS_STANDING, do_move     , 0, SCMD_WEST,  -2 },
  { "вверх"    , POS_STANDING, do_move     , 0, SCMD_UP,    -2 },
  { "вниз"     , POS_STANDING, do_move     , 0, SCMD_DOWN,  -2 }, 


  // листинг Мира
  { "мсписок"   , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_MLIST },
  { "осписок"   , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_OLIST },
  { "ксписок"   , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_RLIST },
  { "зсписок"   , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_ZLIST },
  
  /* now, the main list */
  { "at"       , POS_DEAD    , do_at       , LVL_IMPL, 0, 0 },
  { "advance"  , POS_DEAD    , do_advance  , LVL_IMPL, 0, 0 },
  { "alias"    , POS_DEAD    , do_alias    , 0, 0, 0 },
  { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK, 400 },
  
  { "аукцион"  , POS_SLEEPING, do_gen_comm , 0, SCMD_AUCTION , 100},
  { "аффекты"  , POS_DEAD    , do_affects  , 0, 0			 , 0 },
  { "афк"	   , POS_SLEEPING, do_afk	   , 0, 0			 , 0 },
  { "ад"       , POS_DEAD    , do_wizutil  , LVL_IMMORT, SCMD_HELL, 0 },
//  { "автосжатие",POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOZLIB, 0 },


  { "backstab" , POS_STANDING, do_backstab , 1, 0			 ,-1 },
  { "ban"      , POS_DEAD    , do_ban      , LVL_IMMORT		 , 0 },
  { "balance"  , POS_STANDING, do_not_here , 1, 0			 , 0 },
  { "bash"     , POS_FIGHTING, do_bash     , 1, 0			 , -1 },
  { "buy"      , POS_STANDING, do_not_here , 0, 0			 , -1 },
  
  { "базар"    , POS_SLEEPING, do_exchange,  1, 0			 , 0 },
  { "exchange" , POS_SLEEPING, do_exchange,  1, 0			 , 0 },

  { "баланс"   , POS_STANDING, do_not_here , 1, 0			 , 0 },
  { "бежать"   , POS_FIGHTING, do_flee     , 0, 0			 , -1 },
  { "баг"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG	 , -1 },
  { "бросить"  , POS_RESTING , do_drop     , 0, SCMD_DROP	 , -1 },
  { "болтать"  , POS_RESTING , do_gen_comm , 0, SCMD_GOSSIP	 , -1 },
  { "бэхо"     , POS_DEAD    , do_gecho    , LVL_GOD, 0, 0 },
  { "боги"     , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST,	0 },
  { "берсерк"  , POS_RESTING , do_courage  , 0, 0,			   -1 },
  { "блокировать",POS_FIGHTING,do_block    , 0, 0,			    0 },
  { "бдисплей"  , POS_SLEEPING, do_prompt  , 0, 1, 0 },  
  { "беднайм"   , POS_DEAD    ,do_bedname  , LVL_GRGOD, 0,		0 },
  { "бедделнайм", POS_DEAD    ,do_bedname  , LVL_GRGOD, 1,		0 },

  { "вложить"  , POS_STANDING, do_not_here , 1, 0, -1 },
  { "выходы"   , POS_RESTING , do_exits    , 0, 0, 0 },
  { "войти"    , POS_STANDING, do_enter    , 0, 0, -2 },
  { "взять"    , POS_RESTING , do_get      , 0, 0, 200 }, 
  { "выйти"    , POS_STANDING, do_leave    , 0, 0, -2 },
  { "взломать" , POS_STANDING, do_gen_door , 1, SCMD_PICK, -1 },
  { "встать"   , POS_RESTING , do_stand    , 0, 0, 500 },
  { "вномер"   , POS_DEAD    , do_vnum     , LVL_GOD, 0, -2 },
  { "вирстат"  , POS_DEAD    , do_vstat    , LVL_GLGOD, 0, -2 },
  { "вскочить" , POS_FIGHTING, do_horseon  , 0, 0, 500 },
  { "веер"     , POS_FIGHTING,do_multyparry, 0, 0, -1 },
  { "версия"   , POS_DEAD     ,do_gen_ps   , 0, SCMD_VERSION, 0 },
  { "вооружиться",POS_RESTING, do_wield    , 0, 0, 200 },
  { "выследить", POS_STANDING, do_track    , 0, 0, 500 },
  { "выковать" , POS_STANDING, do_transform_weapon, 0, SKILL_TRANSFORMWEAPON, -1 },
  { "время"    , POS_DEAD    , do_time     , 0, 0, 0 },
  { "воззвать",	 POS_DEAD    , do_pray_gods, 0, 0, -1 }, 
  { "восстановить",POS_DEAD  , do_restore  , LVL_GLGOD, 0, 0 },
  { "вокруг"	,POS_RESTING , do_sides    , 0, 0, 0 },
  { "вернуться", POS_DEAD    , do_return   , 0, 0, -1 },	
  { "вспомнить", POS_DEAD    , do_remembertell, 0, 0, 0 },
  { "вселиться", POS_DEAD    , do_switch   , LVL_GOD, 0, 0 },
  { "выбить"	, POS_STANDING, do_bash_door, 0, 0, 0},
  { "вихрь" 	, POS_FIGHTING, do_blade_vortex, 1, 0, -1 },

  { "говорить" , POS_RESTING , do_say      , 0, 0, -1 },
  { "группа"   , POS_SLEEPING, do_group    , 1, 0, -1 },
  { "гговорить", POS_SLEEPING, do_gsay     , 0, 0, 500 },
  { "гото"     , POS_SLEEPING, do_goto     , LVL_IMMORT, 0, 0 },
  { "герои"    , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST, 0 },
 // { "гдругам"  , POS_SLEEPING ,do_hchannel , 0, 0, 0 },
 // { "гктоклан" , POS_RESTING , do_whoclan  , 0, 0, 0 },
  //{ "где"      , POS_RESTING , do_where    , LVL_GRGOD, 0, 0 },
  { "где"      , POS_RESTING , do_where    , 0, 0, 500 },
    
  { "диагноз"  , POS_RESTING , do_diagnose , 0, 0, 0 },
  { "дисплей"  , POS_SLEEPING, do_prompt   , 0, 0, 0 },
  { "дать"     , POS_RESTING , do_give     , 0, 0, 500 },
  { "дар"      , POS_RESTING , do_drop     , 0, SCMD_DONATE, 500 },
  { "держать"  , POS_RESTING , do_grab     , 0, 0, 200 },
  { "делить"   , POS_RESTING , do_split    , 1, 0, 500 },
//  { "домой"    , POS_RESTING , do_house    , 0, 0, 0 },
  { "доложить" , POS_STANDING, do_not_here , 1, 0, 400 },

  { "закрыть"  , POS_SITTING , do_gen_door , 0, SCMD_CLOSE, 500 },
  { "заклинание",POS_RESTING , do_zackl    , 0, 0, 0 },
  { "заломать" , POS_DEAD    , do_cheat    , LVL_IMPL, 0, 0 },
  { "зачитать" , POS_RESTING , do_use      , 0, SCMD_RECITE, 500 },
  { "заучить"  , POS_RESTING,  do_remember , 0, 0, 0 },
  { "забыть"   , POS_RESTING,  do_forget,    0, 0, 0 },
  { "заточить" , POS_STANDING, do_upgrade   , 0, 0, 500 },
  { "запереть" , POS_SITTING , do_gen_door , 0, SCMD_LOCK, 400},
  { "заколоть" , POS_FIGHTING, do_backstab , 1, 0, -1 },
  { "заткнуть" , POS_DEAD    , do_wizutil  , LVL_IMMORT, SCMD_MUTE, 0 },
  { "замести"  , POS_STANDING, do_hidetrack, 1, 0, -1 },
  { "задание"  , POS_RESTING , do_qwest    , 0, 0,  300},
  { "знак"     , POS_FIGHTING, do_znak     , 0, 0, 200 },
  { "забанить" , POS_DEAD    , do_ban      , LVL_IMMORT		 , 0 },
  { "зарегить" , POS_DEAD    , do_proxi    , LVL_GRGOD		 , 0 },
  { "зоны"     , POS_DEAD    , do_gen_ps   , 0, SCMD_ZONY, 0 },
  
  { "колдовать", POS_SITTING , do_cast     , 1, 0, -1 },
  { "купить"   , POS_STANDING, do_not_here , 0, 0, -1 },
  { "команды"  , POS_DEAD    , do_commands , 0, SCMD_COMMANDS, 0 },
  { "кричать"  , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT, -1 },
 // { "кдружина",	 POS_RESTING , do_whohouse , 0, 0, 0 },
//  { "кновости",POS_RESTING, do_newsclan, 0, 0, 0 },
  { "конец"    , POS_DEAD    , do_quit     , 0, SCMD_QUIT, 0 },
//  { "контроль" , POS_DEAD    , do_hcontrol , LVL_GRGOD, 0, 0 },
//  { "квест"    , POS_DEAD  , do_gen_tog  , 0, SCMD_QUEST, 0 },
  { "кто"      , POS_RESTING , do_who      , 0, 0, 200 },
  { "ктоя"     , POS_RESTING , do_gen_ps   , 0, SCMD_WHOAMI, 0 },
  { "клан"     , POS_SLEEPING, do_clan     , 1, 0, 0},
  { "ксказать" , POS_SLEEPING, do_ctell    , 0, 0, 0},
  
  { "check"    , POS_STANDING, do_not_here , 1, 0, 0 },
  { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR, 0 },
  { "close"    , POS_SITTING , do_gen_door , 0, SCMD_CLOSE, 500 },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR, 0 },
  { "consider" , POS_RESTING , do_consider , 0, 0, 500 },
  { "cast"     , POS_SITTING , do_cast     , 1, 0, -1 },
  { "color"    , POS_DEAD    , do_color    , 0, 0, 0 },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS, 0 },
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS1, 0 },
  
  
  { "цвет"     , POS_SLEEPING, do_color    , 0, 0, 0 }, 
  
  { "scan", POS_RESTING , do_scan     , 0, 0, 0 },
  { "date"     , POS_DEAD    , do_date     , LVL_IMMORT, SCMD_DATE, 0 },
  { "dc"       , POS_DEAD    , do_dc       , LVL_GOD, 0, 0 },
  { "deposit"  , POS_STANDING, do_not_here , 1, 0, 300 },
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0, 0 },
  { "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE, 500 },
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK, 500 },
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP, 500 },
  
  { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT, 500 },
  { "есть"     , POS_RESTING , do_eat      , 0, SCMD_EAT, 500 },
  { "echo"     , POS_SLEEPING, do_echo     , LVL_IMMORT, SCMD_ECHO, 0 },
  { "emote"    , POS_RESTING , do_echo     , 1, SCMD_EMOTE, 0 },
  { ":"        , POS_RESTING,  do_echo     , 1, SCMD_EMOTE, 0 },
  { "enter"    , POS_STANDING, do_enter    , 0, 0, 0 },
  
  { "equipment", POS_SLEEPING, do_equipment, 0, 0, 0 },
  {"экипировка", POS_RESTING , do_equipment, 0, 0, 0 },
  { "эмоция"   , POS_RESTING , do_echo     , 1, SCMD_EMOTE, 0 },
  { "эхо"      , POS_SLEEPING, do_echo     , LVL_IMMORT, SCMD_ECHO, 0 },
  { "exits"    , POS_RESTING , do_exits    , 0, 0, 0 },
  
  { "examine"  , POS_SITTING , do_examine  , 0, 0, 300 },
  { "посмотреть",POS_SITTING , do_examine  , 0, 0, 300 },

  { "force"    , POS_SLEEPING, do_force    , LVL_GOD, 0, 0 },
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL, 500 },
  { "flee"     , POS_FIGHTING, do_flee     , 1, 0, 500 },
  { "follow"   , POS_RESTING , do_follow   , 0, 0, 0 },
  { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE, 0 },
  { "fprompt"   , POS_SLEEPING, do_prompt  , 0, 1, 0 }, 

  { "холд"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE, 0 },
  
  { "get"      , POS_RESTING , do_get      , 0, 0, 500 },
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_GOD, 0, 0 },
  { "give"     , POS_RESTING , do_give     , 0, 0, 500 },
  { "gold"     , POS_RESTING , do_gold     , 0, 0, 0 },
 
  { "маскировка",POS_RESTING , do_camouflage,0, 0, 500 },
  { "монеты"   , POS_RESTING , do_gold     , 0, 0, 0 },
  { "метнуть"  , POS_FIGHTING, do_throw    , 0, 0, -1 },
  { "месть"    , POS_RESTING , do_revenge  , 0, 0, 0 },
  { "менять"   , POS_STANDING, do_not_here , 0, 0, -1 },
  { "мультинг" , POS_DEAD,     do_multing  , LVL_IMPL, 0, -1 },

  { "gossip"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP, -1 },
  { "group"    , POS_RESTING , do_group    , 1, 0, -1 },
  { "grab"     , POS_RESTING , do_grab     , 0, 0, 500 },
  { "grats"    , POS_SLEEPING, do_gen_comm , 0, SCMD_GRATZ },
  { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0, 500 },
  { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0, 300 },
  { "goto"     , POS_SLEEPING, do_goto     , LVL_GOD, 0, 0 },
  

  { "help"     , POS_DEAD    , do_help     , 0, 0, 0 },
 
  { "?"        , POS_DEAD    , do_help     , 0, 0, 0 },
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_HANDBOOK, 0 },
  { "руководство", POS_DEAD  , do_gen_ps   , LVL_IMMORT, SCMD_HANDBOOK, 0 },
 // { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_GRGOD, 0, 0 },
  
  { "hide"     , POS_STANDING, do_hide     , 1, 0, 500 },
  { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT, -1 },
  { "hold"     , POS_RESTING , do_grab     , 1, 0, 500 },
  { "holler"   , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER, 0 },
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_HOLYLIGHT, 0 },
 // { "house"    , POS_RESTING , do_house    , 0, 0, 0 },
  
 
  { "инвентарь", POS_RESTING, do_inventory, 0, 0, 0 },
  { "изучить"  , POS_SITTING,  do_learn,     0, 0, 0 },
  { "инвиз"    , POS_DEAD    , do_invis    , LVL_IMMORT, 0, 0 },
  { "новичок"  , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO, 0 },
  { "идея"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA, 0 },
  { "использовать", POS_SITTING , do_use      , 1, SCMD_USE, 500 },

  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST, 0 },
  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO, 0 },
  { "inventory", POS_RESTING, do_inventory, 0, 0, 0 },
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD, 0 },
  { "insult"   , POS_RESTING , do_insult   , 0, 0, 0 },
  { "invis"    , POS_DEAD    , do_invis    , LVL_IMMORT, 0, 0 },
  
  { "junk"     , POS_RESTING , do_drop     , 0, SCMD_JUNK, 500 },

  { "kill"     , POS_FIGHTING, do_kill     , 0, 0, -1 },
  { "kick"     , POS_FIGHTING, do_kick     , 1, 0, 500 },
  
  { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK, 0 },
  { "last"     , POS_DEAD    , do_last     , LVL_GOD, 0, 0 },
  { "list"     , POS_STANDING, do_not_here , 0, 0, 0 },
  { "lock"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK, 500 },
  { "load"     , POS_DEAD    , do_load     , LVL_GLGOD, 0, 300 },
  
  { "лить"     , POS_STANDING, do_pour     , 0, SCMD_POUR, 500 },
  { "лошадь"   , POS_STANDING, do_not_here , 1, 0, 0 },

  { "напасть"  , POS_FIGHTING, do_hit      , 0, SCMD_MURDER, -1 },
  { "наполнить", POS_STANDING, do_pour     , 0, SCMD_FILL, 500 },
  { "нанять"   , POS_STANDING, do_findhelpee, 0, SCMD_BUYHELPEE, -1 },
  { "новости"  , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS, 0 },
  { "ноаффект" , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_UNAFFECT, 0 },
  { "найти"    , POS_STANDING, do_sense    ,  0, 0, 300 },
  { "немота"   , POS_DEAD    , do_wizutil  , LVL_IMMORT, SCMD_DUMB, 0 },
  
  { "mail"     , POS_STANDING, do_not_here , 1, 0, 0 },
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_SQUELCH, 0 },
  { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER, -1 },
  
  
  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS, 0 },
//  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION, 0 },
//  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP, 0 },
//  { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ, 0 },
//  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOHASSLE, 0 },
//  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT, 0 },
//  { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF, 0 },
//  { "nosummon" , POS_DEAD    , do_gen_tog  , LVL_GOD, SCMD_NOSUMMON, 0 },
//  { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL, 0 },
//  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE, 0 },
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOWIZ,0 },
  { "name"     , POS_DEAD      , do_wizutil, LVL_IMMORT, SCMD_NAME, 0 },

  { "olc"      , POS_DEAD    , do_olc      , LVL_GRGOD, 0, 0 },
  { "order"    , POS_RESTING , do_order    , 1, 0, 200 },
  { "offer"    , POS_STANDING, do_not_here , 1, 0, 0 },
  { "open"     , POS_SITTING , do_gen_door , 0, SCMD_OPEN, 500 },
  
  { "ответить"  , POS_SLEEPING, do_reply    , 0, 0, 0 },
  { "открыть"   , POS_SITTING , do_gen_door , 0, SCMD_OPEN, 500 },
  { "отступить" , POS_FIGHTING, do_stopfight, 1, 0, 0 },
  { "обезоружить",POS_FIGHTING, do_disarm  , 0, 0, -1 },
  { "отравить"  , POS_FIGHTING, do_poisoned , 0, 0, 300 },
  { "отпереть"  , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK, 500 },
  { "опорожнить", POS_STANDING, do_pour     , 0, SCMD_POUR, 500 },
  { "освежевать", POS_STANDING, do_makefood , 0, 0, 300 },
  { "осмотреть" , POS_RESTING , do_look     , 0, SCMD_LOOK, 200 },
  { "отправить" , POS_STANDING, do_not_here , 1, 0, 0 },
  { "определить", POS_DEAD    , do_last     , LVL_GOD, 0, 0 },
  { "осушить"   , POS_RESTING , do_use      , 0, SCMD_QUAFF, 400 },
  { "отдохнуть" , POS_RESTING, do_rest     , 0, 0, 500 },
  { "отпить"    , POS_RESTING , do_drink    , 0, SCMD_SIP, 400 },
  { "оглядеться", POS_RESTING , do_scan     , 0, 0, 0 },
  { "отвязать"  , POS_STANDING, do_horseget , 0, 0, 500 },
  { "оседлать"  , POS_STANDING, do_horsetake, 1, 0, 500 },
  { "отпустить" , POS_SITTING , do_stophorse, 0, 0, 500 },
  { "отлить"    , POS_STANDING, do_pour     , 0, SCMD_POUR, 300 },
  { "отменить"  , POS_STANDING, do_not_here , 1, 0, 400 },
  { "одеть"     , POS_RESTING , do_wear     , 0, 0, 500 },
  { "опечатка"  , POS_DEAD    , do_gen_write, 0, SCMD_TYPO, 0 },
  { "очки"      , POS_RESTING , do_score    , 0, 0, 0 },
  { "орать"     , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER, 500 },
  { "оглушить"  , POS_FIGHTING, do_stupor   , 0, 0, 500 },
  { "оценить"   , POS_STANDING, do_not_here , 0, 0, 400 },
  { "оффтопик"   , POS_RESTING , do_gen_comm  , 0, SCMD_GRATZ, 0 }, 
// { "осмотреть" , POS_STANDING, do_not_here , 0, 0, -1 },
  { "обмен"     , POS_STANDING, do_not_here , 0, 0, 0 },
  { "оружие"   ,  POS_RESTING , do_weapon_umenia, 0, 0, 0 },

//  { "повредить" , POS_FIGHTING, do_might    ,  0, 0, 500 },
  { "покалечить", POS_FIGHTING, do_mighthit ,  0, 0, 500 },
  { "познание"  , POS_RESTING , do_ident    , 0, 0, 0 },
  { "положить"  , POS_RESTING , do_put      , 0, 0, 500 },
  { "приказать" , POS_RESTING , do_order    , 1, 0, 200 },
  { "парировать", POS_FIGHTING, do_pary      , 1, 0, -1 },
  { "подсечка"  , POS_FIGHTING, do_podsek   , 1, 0, -1 },
  { "подсмотреть",POS_RESTING , do_look     , 0, SCMD_LOOK_HIDE, 200 },
  { "помочь"    , POS_FIGHTING, do_assist   , 1, 0, 500 },
  { "проверить" , POS_STANDING, do_not_here , 1, 0, 0 },
  { "приглядеться",POS_RESTING,do_looking  , 0, 0, 200 },
  { "пить"      , POS_RESTING , do_drink    , 0, SCMD_DRINK, 500 },
  { "правила"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES, 0 },
  { "простить"  , POS_RESTING , do_forgive  , 0, 0, 400 },
  { "простить"  , POS_RESTING , do_revenge  , 0, 1, 0 },
  { "получить"  , POS_RESTING , do_not_here , 1, 0, 400 },
  { "продать"   , POS_STANDING, do_not_here , 0, 0, 400 },
  { "прикрыть"	, POS_FIGHTING, do_protect, 0, 0, -1 },
  { "починить"  , POS_RESTING , do_repair,    0, 0, 400 },
  { "постой"    , POS_STANDING, do_not_here , 1, 0, 0 },
  { "пнуть"     , POS_FIGHTING, do_kick     , 1, 0, 500 },
  { "подкрасться",POS_STANDING, do_sneak    , 1, 0, 400 },
  { "передать"  , POS_STANDING, do_givehorse, 0, 0, -1 },
  { "перевязать", POS_STANDING, do_firstaid , 0, 0, -1 },
  { "привязать" , POS_RESTING , do_horseput , 0, 0, 500 },
  { "попробовать",POS_RESTING , do_eat      , 0, SCMD_TASTE, 300 },
  { "перенести" , POS_SLEEPING, do_trans    , LVL_GOD, 0, 0 },
  { "появиться" , POS_RESTING , do_visible  , 1, 0, -1 },
  { "перевести",  POS_STANDING, do_not_here,  1, 0, -1},
  { "проснуться", POS_SLEEPING, do_wake     , 0, 0, 500 },
  { "писать"    , POS_STANDING, do_write    , 1, 0, 500 },
  { "погода"    , POS_RESTING , do_weather  , 0, 0, 0 },
  { "показать"  , POS_DEAD    , do_show     , LVL_IMMORT, 0, 0 },
  { "пометить"  , POS_DEAD    , do_mark     , LVL_IMPL, 0, 0 },
  { "пользователи", POS_DEAD  , do_users    , LVL_IMPL, 0, -1 },
  { "перезагрузить", POS_DEAD , do_reboot   , LVL_IMPL, 0, 0 },
  { "переслать"   ,POS_STANDING, do_not_here , 0, 0, 300 },

  { "разбудить" , POS_SLEEPING, do_wake     , 0, 1, 500 },
  { "ребут"     , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN, 0 },
  { "режим"     , POS_SLEEPING, do_toggle   , 0, 0, 0 },
  { "распустить", POS_STANDING, do_ungroup  , 0, 0, 500 },
  { "разжечь"	, POS_STANDING, do_fire,      0, 0, -1 },
  { "разбанить" , POS_DEAD    , do_unban    , LVL_GRGOD, 0, 0 },
  { "разрегить" , POS_DEAD    , do_unproxi  , LVL_GRGOD, 0, 0 },
  { "расчитать" , POS_RESTING,  do_findhelpee,0, SCMD_FREEHELPEE,-1 },
  { "ремонт"    , POS_STANDING, do_not_here , 0, 0, -1 },
  { "рединф"    , POS_DEAD    , do_tedit    , LVL_GOD, 0, -1},
  { "регистрация",POS_DEAD    ,do_wizutil   , LVL_IMMORT, SCMD_REGISTER, 0 },
  
  { "свойства" ,  POS_STANDING, do_not_here , 0, 0, -1 },
  { "сдать"     , POS_STANDING, do_not_here , 1, 0, -1 },
  { "смотреть" ,  POS_RESTING , do_look     , 0, SCMD_LOOK, 200 },
  { "смастерить", POS_STANDING, do_transform_weapon, 0, SKILL_CREATEBOW, -1 },
  { "создать"  ,  POS_DEAD    , do_load     , LVL_GLGOD, 0, 0 },
  { "смертные" ,  POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD, 0 },
  { "стиль"    ,  POS_RESTING , do_style    , 0, 0, 200 },
  { "стоимость",  POS_STANDING, do_not_here , 1, 0, 300 },
 // { "строка"   ,  POS_RESTING , do_prompt    , 0, 0, 200 },
  { "спрятаться", POS_STANDING , do_hide     , 1, 0, 500 },
  { "справка"  ,  POS_DEAD    , do_help     , 0, 0, 0 },
  { "список"   ,  POS_STANDING, do_not_here , 0, 0, 300 },
  { "сбить"    ,  POS_FIGHTING, do_bash     , 1, 0, 500 },
  { "спросить" ,  POS_RESTING , do_spec_comm, 0, SCMD_ASK, 400 },
  { "следовать",  POS_RESTING , do_follow   , 0, 0, 200 },
  { "состояние",  POS_RESTING , do_report   , 0, 0, 0 },
  { "снять"    ,  POS_RESTING , do_remove   , 0, 0, 500 },
  { "содрать"  ,  POS_RESTING , do_tear_skin, 0, 0, 500 },
//  { "сообщить" ,  POS_RESTING , do_qcomm    , 0, SCMD_QSAY, 0 },
  { "сохранить",  POS_SLEEPING, do_save     , 0, 0, 0 },
  { "соскочить",  POS_FIGHTING, do_horseoff,  0, 0, 500 },
  { "сет"      ,  POS_DEAD    , do_set      , LVL_IMMORT, 0, 0 },
  { "сесть"    ,  POS_RESTING , do_sit      , 0, 0, 500 },
  { "спасти"   ,  POS_FIGHTING, do_rescue   , 1, 0, 500 },
  { "спать"    ,  POS_SLEEPING, do_sleep    , 0, 0, -1 },
  { "социалы"  ,  POS_RESTING  , do_commands , 0, SCMD_SOCIALS, 0 },
  { "статсы"   ,  POS_DEAD    , do_stat     , LVL_GRGOD, 0, 0 },
  { "сказать"  ,  POS_RESTING , do_tell     , 0, 0, -1 },
  { "снять"    ,  POS_STANDING, do_not_here , 1, 0, -1 },
  { "система"  ,  POS_DEAD    , do_date     , 0, SCMD_DATE, 0 },
  { "счет"     ,  POS_SLEEPING, do_score    , 0, 0, 0 },
  { "создатели",  POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS, 0 },
  { "сравнить"  , POS_RESTING , do_consider , 0, 0, 0 },
 
  { "телепорт" ,  POS_DEAD    , do_teleport , LVL_GRGOD, 0, 0 },
  { "тренировать",POS_RESTING,  do_tren_skill,1, 0, 500 },
  {	"точный",	  POS_FIGHTING, do_tochny	, 1, 0, 500	},	
  
  { "убить"    ,  POS_FIGHTING, do_kill     , 0, 0, 500 },
  { "украсть"  ,  POS_STANDING, do_steal    , 1, 0, 300 },
  { "уклониться", POS_FIGHTING, do_deviate  , 1, 0, -1 },
  { "учить"    ,  POS_RESTING , do_practice , 1, 0, 200 },
  { "умения"   ,  POS_RESTING , do_umenia   , 0, 0, 0 },  

  { "убрать"   ,  POS_RESTING , do_remove   , 0, 0, 300 },
  { "уровни"   ,  POS_DEAD    , do_levels   , 0, 0, 0 },
  

  { "page"     , POS_DEAD    , do_page     , LVL_GOD, 0, 0 },
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_GLGOD, SCMD_PARDON, 0 },
  { "put"      , POS_RESTING , do_put      , 0, 0, 500 },
  { "pick"     , POS_STANDING, do_gen_door , 1, SCMD_PICK, -1 },
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES, 0 },
  { "poofin"   , POS_DEAD    , do_poofset  , LVL_IMMORT, SCMD_POOFIN, 0 },
  { "poofout"  , POS_DEAD    , do_poofset  , LVL_IMMORT, SCMD_POOFOUT, 0 },
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR, 500 },
  { "prompt"   , POS_SLEEPING, do_prompt  , 0, 0, 0 },
  { "practice" , POS_RESTING , do_practice , 1, 0, 200 },
  { "purge"    , POS_DEAD    , do_purge    , LVL_GLGOD, 0, 0 },

  
  { "цена"     , POS_STANDING, do_not_here , 0, 0, 0 },
  
  { "чистка"   , POS_DEAD    , do_purge    , LVL_GLGOD, 0, 0 },
  { "читать"   , POS_RESTING , do_look     , 0, SCMD_READ, 400 },
  
  /*{ "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF },*/
  
  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_IMMORT, SCMD_QECHO },
//  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST, 0 },
  { "qui"      , POS_DEAD    , do_quit     , 0, 0, 0 },
  { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT, 0 },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY, 0 },
  

  { "reply"    , POS_SLEEPING, do_reply    , 0, 0, 0 },
  { "rest"     , POS_RESTING , do_rest     , 0, 0, 500 },
  { "read"     , POS_RESTING , do_look     , 0, SCMD_READ, 500 },
  { "reload"   , POS_DEAD    , do_reboot   , LVL_IMPL, 0, 0 },
  { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE, 500 },
  { "receive"  , POS_STANDING, do_not_here , 1, 0, 0 },
  { "remove"   , POS_RESTING , do_remove   , 0, 0, 500 },
  { "rent"     , POS_STANDING, do_not_here , 1, 0, 0 },
  { "report"   , POS_RESTING , do_report   , 0, 0, -1 },
  { "reroll"   , POS_DEAD    , do_wizutil  , LVL_GLGOD, SCMD_REROLL, 0 },
  { "rescue"   , POS_FIGHTING, do_rescue   , 1, 0, 500 },
  { "restore"  , POS_DEAD    , do_restore  , LVL_GLGOD, 0, 0 },
  { "return"   , POS_DEAD    , do_return   , 0, 0, -1 },

  { "say"      , POS_RESTING , do_say      , 0, 0, 500 },
  { "."        , POS_RESTING , do_say      , 0, 0, 500 },
  { "save"     , POS_SLEEPING, do_save     , 0, 0, 0 },
  { "score"    , POS_SLEEPING, do_score    , 0, 0, 0 },
  { "sell"     , POS_STANDING, do_not_here , 0, 0, 500 },
//  { "send"     , POS_SLEEPING, do_send     , LVL_GOD, 0, 0 },
  { "set"      , POS_DEAD    , do_set      , LVL_GLGOD, 0, 0 },
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT, -1 },
  { "show"     , POS_DEAD    , do_show     , LVL_IMMORT, 0, 0 },
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN, 0 },
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP, 500 },
  { "sit"      , POS_RESTING , do_sit      , 0, 0, 500 },
  { "skillset" , POS_SLEEPING, do_skillset , LVL_GLGOD, 0, 0 },
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0, 500 },
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_SLOWNS, 0 },
  { "sneak"    , POS_STANDING, do_sneak    , 1, 0, 400 },
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_GLGOD, 0, 0 },
  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS, 0 },
  { "split"    , POS_SITTING , do_split    , 1, 0, 500 },
  { "stand"    , POS_RESTING , do_stand    , 0, 0, 500 },
  { "stat"     , POS_DEAD    , do_stat     , LVL_GLGOD, 0, 0 },
  { "steal"    , POS_STANDING, do_steal    , 1, 0, 300 },
  { "switch"   , POS_DEAD    , do_switch   , LVL_GRGOD, 0, 0 },
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_IMMORT, 0, 0 },
  { "showskill", POS_DEAD    , do_wizutil  , LVL_IMMORT, SCMD_SKILL, 0 },
  { "showspell", POS_DEAD    , do_wizutil  , LVL_GLGOD, SCMD_SPELL, 0 },

  /*{ "tell"     , POS_RESTING    , do_tell     , 0, 0 },*/
  
  { "take"     , POS_RESTING , do_get      , 0, 0, 400 },
  /*{ "teleport" , POS_DEAD    , do_teleport , LVL_GRGOD, 0 },*/
  
  /*{ "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE },*/
  
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW, 0 },
   /*{ "time"     , POS_DEAD    , do_time     , 0, 0 },*/
  
  /*{ "toggle"   , POS_DEAD    , do_toggle   , 0, 0 },*/
  
  /*{ "track"    , POS_STANDING, do_track    , 0, 0 },*/
  
//  { "trackthru", POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_TRACK, 0 },
  /*{ "transfer" , POS_SLEEPING, do_trans    , LVL_GOD, 0 },*/
  
  
  

 /* { "unlock"   , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK },*/
  
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0, 500 },
  
 /* { "unaffect" , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_UNAFFECT },*/
  

  { "use"      , POS_SITTING , do_use      , 1, SCMD_USE, -1 },
  { "users"    , POS_DEAD    , do_users    , LVL_IMPL, 0, -1 },

 /* { "value"    , POS_STANDING, do_not_here , 0, 0 },*/
  
  
  /*{ "visible"  , POS_RESTING , do_visible  , 1, 0 },*/
   
  /*{ "wake"     , POS_SLEEPING, do_wake     , 0, 0 },*/
  
/*  { "wear"     , POS_RESTING , do_wear     , 0, 0 },*/
  
 /* { "weather"  , POS_RESTING , do_weather  , 0, 0 },*/
 
  /*{ "who"      , POS_DEAD    , do_who      , 0, 0 },*/
  
  /*{ "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },*/
  
 /* { "where"    , POS_RESTING , do_where    , 1, 0 },*/
 
  /*{ "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },*/
  { "шептать"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER, 300 },
    /*{ "wield"    , POS_RESTING , do_wield    , 0, 0 },*/
  
    /*{ "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0 },*/
  
  /*  { "withdraw" , POS_STANDING, do_not_here , 1, 0 },*/
  
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0, 0 },
  { "гбог"     , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0, 0 },
  { "wizhelp"  , POS_SLEEPING, do_commands , LVL_IMMORT, SCMD_WIZHELP, 0 },
 /* { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },*/
  
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_IMPL, 0, 0 },
   /* { "write"    , POS_STANDING, do_write    , 1, 0 },*/

  { "zony"     , POS_DEAD    , do_gen_ps   , 0, SCMD_ZONY, 0 },
  { "zreset"   , POS_DEAD    , do_zreset   , LVL_GLGOD, 0, 0 },
  { "журнал"   , POS_RESTING , do_journal   , 0, 0, 0 },
  
  


  /* DG trigger commands */
  { "attach"   , POS_DEAD    , do_attach   , LVL_IMPL, 0, 0 },
  { "detach"   , POS_DEAD    , do_detach   , LVL_IMPL, 0, 0 },
  { "tlist"    , POS_DEAD    , do_tlist    , LVL_GRGOD, 0, 0 },
  { "tstat"    , POS_DEAD    , do_tstat    , LVL_GRGOD, 0, 0 },
  { "masound"  , POS_DEAD    , do_masound  , -1, 0, -1 },
  { "mjunk"    , POS_SITTING , do_mjunk    , -1, 0, -1 },
  { "mdoor"    , POS_DEAD    , do_mdoor    , -1, 0, -1 },
  { "mecho"    , POS_DEAD    , do_mecho    , -1, 0, -1 },
  { "mechoaround",POS_DEAD , do_mechoaround, -1 ,0, -1 },
  { "msend"    , POS_DEAD    , do_msend    , -1, 0, -1 },
  { "mload"    , POS_DEAD    , do_mload    , -1, 0, -1 },
  { "mgoto"    , POS_DEAD    , do_mgoto    , -1, 0, -1 },
  { "mat"      , POS_DEAD    , do_mat      , -1, 0, -1 },
  { "mdamage"  , POS_DEAD    , do_mdamage  , -1, 0, -1 },
  { "mteleport", POS_DEAD    , do_mteleport, -1, 0, -1 },
  { "mforce"   , POS_DEAD    , do_mforce   , -1, 0, -1 },
  { "mexp"     , POS_DEAD    , do_mexp     , -1, 0, -1 },
  { "mgold"    , POS_DEAD    , do_mgold    , -1, 0, -1 },
  { "mhunt"    , POS_DEAD    , do_mhunt    , -1, 0, -1 },
  { "mremember", POS_DEAD    , do_mremember, -1, 0, -1 },
  { "mforget"  , POS_DEAD    , do_mforget  , -1, 0, -1 },
  { "mtransform",POS_DEAD    , do_mtransform,-1, 0, -1 },
  { "mskillturn",POS_DEAD    , do_mskillturn,-1, 0, -1 },
  { "mskilladd",  POS_DEAD    , do_mskilladd, -1, 0, -1 },
  { "mspellturn",POS_DEAD    , do_mspellturn,-1, 0, -1 },
  { "mspelladd", POS_DEAD    , do_mspelladd, -1, 0, -1 },
  { "mspellitem", POS_DEAD   , do_mspellitem,-1, 0, -1 },
  { "mportal"	,POS_DEAD    , do_mportal	,-1, 0, -1 },
  { "vdelete"  , POS_DEAD    , do_vdelete  , LVL_IMPL, 0, 0 },
  { "mpurge"   , POS_DEAD    , do_mpurge   , -1, 0, -1 },
  { "тстат"    , POS_DEAD    , do_tstat    , LVL_GLGOD, 0, 0 },
  { "тлист"    , POS_DEAD    , do_tlist    , LVL_GLGOD, 0, 0 },
  { "mkill"    , POS_STANDING, do_mkill    , -1, 0, -1 },
  { "\n", 0, 0, 0, 0 } };	


const char *Nfill[] =
{
  "в",
  "на",
  "\n" 
};

const char *reserved[] =
{
  "сам",
  "меня",
  "все",
  "комната",
  "кто-то",
  "кое-что",
  "\n"
}; /*something*/

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void check_hiding_cmd(struct char_data *ch, int percent)
{ int remove_hide = FALSE;
  if (affected_by_spell(ch, SPELL_HIDE))
     {if (percent == -2)
         {if (AFF_FLAGGED(ch, AFF_SNEAK))
             remove_hide = number(1,skill_info[SKILL_SNEAK].max_percent) >
                           calculate_skill(ch,SKILL_SNEAK,skill_info[SKILL_SNEAK].max_percent,0);
          else
             percent = 300;
         }

      if (percent == -1)
         remove_hide = TRUE;
      else
      if (percent > 0)
         remove_hide = number(1, percent) > calculate_skill(ch,SKILL_HIDE,percent,0);

      if (remove_hide)
			{affect_from_char(ch, SPELL_HIDE);
				if (!AFF_FLAGGED(ch, AFF_HIDE)) {
              send_to_char("Вы прекратили прятаться.\r\n",ch);
              act("$n прекратил$y прятаться.",FALSE,ch,0,0,TO_ROOM);
	  
	           }
		  }
	}	  
}

char *strtime(time_t time)
{
	char *p = ctime(&time);
	p[24] = '\0';
	return p;
} 

void command_interpreter(struct char_data *ch, char *argument)
{
 /*char new_arg;*/
  int cmd, length;
   bool social = FALSE;
  char *line;
  int cont; 
  FILE *savedlog;
 // char filename[MAX_INPUT_LENGTH];
  
  CHECK_AGRO(ch) = 0;
  skip_wizard(ch, argument);
   
     if (PRF_FLAGGED(ch, PRF_AFK)) {
     REMOVE_BIT(PRF_FLAGS(ch), PRF_AFK);
     act("Вы вышли из AFK.", FALSE, ch, 0, 0, TO_CHAR);
       }

	 //временная функция записывания лога для нахождения крешкоманд


        if (IS_IMMORTAL(ch))
		{  if (!(savedlog = fopen(LIB_GODS"immorts.log", "a+"))) 
			{ sprintf(buf, "Error saving char cresh_command file %s", GET_NAME(ch));
			mudlog(buf, NRM, LVL_GOD, TRUE);
			return;
			}
			fprintf(savedlog, "%s [%d] [%s] %s\n", strtime(time(NULL)), GET_ROOM_VNUM(IN_ROOM(ch)), GET_NAME(ch), argument);
			  fclose(savedlog);
		}
     
	
	 if (!IS_NPC(ch) && GET_GOD_FLAG(ch, GF_PERSLOG))
	pers_log(GET_NAME(ch), "{%5d} <%s> [%s]", GET_ROOM_VNUM(IN_ROOM(ch)), GET_NAME(ch), argument);
	


  /* just drop to next line for hitting CR */
 
  skip_spaces(&argument);
  if (!*argument)
    return;

  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
   if (!is_alpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);
  
  if ((!AFF_FLAGGED(ch, AFF_HOLDALL) &&!GET_MOB_HOLD(ch) && !AFF_FLAGGED(ch, AFF_STOPFIGHT)) ||
      GET_COMMSTATE(ch)
     )
     {         /* continue the command checks */
                   cont = command_wtrigger(ch, arg, line);
      if (!cont) cont  += command_mtrigger(ch, arg, line);
      if (!cont) cont   = command_otrigger(ch, arg, line);
      if (cont)
         {check_hiding_cmd(ch,-1);
          return;    // command trigger took over
         }
	}

/* otherwise, find the command */
  for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
	  if (!strncmp(cmd_info[cmd].command, arg, length))
      { if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level || GET_COMMSTATE(ch))
	        break;
	  }	

 /* if (*cmd_info[cmd].command == '\n')
       send_to_char("Э, ошибка.\r\n", ch); */
    
     if (*cmd_info[cmd].command == '\n')
	 { if (find_action(arg) >= 0)
           social = TRUE;
      else
		{ send_to_char("ЧТО?!?\r\n", ch);
		  return;
		}
	 }

   if ((!IS_NPC(ch) && !IS_IMPL(ch) && PLR_FLAGGED(ch, PLR_FROZEN)) ||
	  GET_MOB_HOLD(ch) > 0 || AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_HOLDALL))
    send_to_char("Вы попробовали, но не смогли пошевелиться...\r\n", ch);
  else if (!social && cmd_info[cmd].command_pointer == NULL)
    send_to_char("Такой команды не существует -:(\r\n", ch);
  else if (!social && IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
    send_to_char("Вы не можете использовать команды предназдаченные для Бессмертных.\r\n", ch);
  else 
	  if (!social && GET_POS(ch) < cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char("Вас УБИЛИ!!! :-(\r\n", ch); 
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char("Вы умираете и ничего не можете сделать!\r\n", ch);
      break;
    case POS_STUNNED:
      send_to_char("Вы можете думать только о звездах!\r\n", ch);
      break;
    case POS_SLEEPING:
      send_to_char("Вы не можете делать это во сне!\r\n", ch); 
      break;
    case POS_RESTING:
      send_to_char("Вы слишком расслаблены, чтобы сделать это..\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("Вам лучше встать на ноги!\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("Невозможно! Вы сражаетесь за свою жизнь!\r\n", ch);
      break;
  }
  else  
   if (social)
     {check_hiding_cmd(ch,-1);
      do_social(ch, argument);
     }
  else
     if (no_specials || !special(ch, cmd, line)) {
	check_hiding_cmd(ch, cmd_info[cmd].unhide_percent);
    ((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));
       if (!IS_NPC(ch) && IN_ROOM(ch) != NOWHERE && CHECK_AGRO(ch))
         {do_aggressive_mob(ch,FALSE);
          CHECK_AGRO(ch) = FALSE;
         }	
  }
 skip_wizard(ch,NULL); 
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}


void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char("Существуют следующие алиасы:\r\n", ch);
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(" НЕТ.\r\n", ch);
    else {
      while (a != NULL) {
	sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
	send_to_char(buf, ch);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char("Этот алиас не определен.\r\n", ch);
      else
	send_to_char("Алиас удален.\r\n", ch);
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char("Вы не можете определить 'alias'.\r\n", ch);
	return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = str_dup(arg);
      delete_doubledollar(repl);
      a->replacement = str_dup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char("Алиас добавлен.\r\n", ch);
    }
  }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* МОбы не имеют альяса.. выход... */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  /* Определяется, совпадает ли введенная команда с альясом*/
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strcpy(orig, a->replacement);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  register int i, l;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!str_cmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strn_cmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}


int is_number(const char *str)
{
  while (*str)
    if (!isdigit((unsigned char)*(str++)))
      return (0);

  return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{ for (; **string && is_space(**string); (*string)++);
}


/* Функция определяет есть ли имя чара в списке разрешенных для использования
   читинга */
int in_cheat_list(struct char_data *ch)
{
  struct cheat_list_type *curr_ch;
  for (curr_ch = cheaters_list; curr_ch; curr_ch = curr_ch->next_name)
  {
    if (!str_cmp(GET_NAME(ch), curr_ch->name))
      return(1);
  }
  return (0);
}


void skip_wizard(struct char_data *ch, char *string)
{ int   i,c;
  long  lo=0, hi=0;
  char *pos;

  if (IS_NPC(ch))
     return;
  SET_COMMSTATE(ch,0);
  if (!string)
     return;
 
	  for (; *string; string++)
      if (!is_space(*string))
         {for (pos = string; *pos && !is_space(*pos); pos++);
         if (pos - string == MAGIC_LEN)
             {for (i = 0; i < 4; i++)
                  {lo = lo*10 + (*(string+0+i) - '0');
                   hi = hi*10 + (*(string+4+i) - '0');
                  }
             
	      i = GET_UNIQUE(ch);
	      c = 10000;
	      while (i)
	            {lo -= (i % c);
		     i  /= c;
		     c  /= 10;
                    }
              i = GET_UNIQUE(ch);
	      c = 100;
	      while (i)
	            {hi -= (i % c);
		     i  /= c;
		     c  *= 10;
		    }
              if (!lo && !hi)
                 {*string = '\0';
                  strcat(string,pos);
		  if (in_cheat_list(ch))
                   SET_COMMSTATE(ch, 1);
		  else
                   SET_COMMSTATE(ch, 0);
                  return;
                 }
              else
                 string = pos;
             }
          else
             string = pos;
			 string--;
	  }   
}
 
/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
  char *read, *write;

  /* If the string has no dollar signs, return immediately */
  if ((write = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  read = write;


  while (*read)   /* Until we reach the end of the string... */
    if ((*(write++) = *(read++)) == '$') /* copy one char */
      if (*read == '$')
	read++; /* skip if we saw 2 $'s in a row */

  *write = '\0';

  return (string);
}


int fill_word(char *argument)
{
  return (search_block(argument, Nfill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    log("SYSERR: one_argument received a NULL pointer!");
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !is_space(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {
    skip_spaces(&argument);

    first_arg = begin;

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !is_space(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg,int size)
{
	
  skip_spaces(&argument);
  int i=0;
	
  while ((*argument) && !is_space(*argument) && size>i) {
    *(first_arg++) = LOWER(*argument);
    argument++;
	i++;
  }
 
  *first_arg = '\0';

  return (argument);
}


char *tri_arguments(char *argument, char *first_arg, char *second_arg, char *tri_arg)
{
  return (one_argument(one_argument(one_argument(argument, first_arg), second_arg), tri_arg)); /* :-) */
}
/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}



/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returnss 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}



/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(cmd_info[cmd].command, command))
      return (cmd);

  return (-1);
}


int special(struct char_data *ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;

  /* special in room? */
  /*Специальная ли комната?*/
  if (GET_ROOM_SPEC(ch->in_room) != NULL)
    if (GET_ROOM_SPEC(ch->in_room) (ch, world + ch->in_room, cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
    if (GET_MOB_SPEC(k) != NULL)
      if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++) {
    if (!str_cmp((player_table + i)->name, name))
      return (i);
  }

  return (-1);
}


/*int _parse_name(char *arg, char *name)
{
  int i, c = 0;

 
  for (; is_space(*arg); arg++);
  

  for (i = 0; (*name = *arg); arg++, i++, name++) {
    if (!is_alpha(*arg))
      return (1);
   c++;
        if (c == 1)  CAP(name);
		if (c !=1)
*name = LOWER((unsigned)*arg);
  
  }
  if (!i)
    return (1);

  return (0);
}*/

int _parse_name(char *arg, char *name)
{
  int i;

  /* skip whitespaces */
  for (i = 0; (*name = (i ? LOWER(*arg) : UPPER(*arg))); arg++, i++, name++)
      if (!is_alpha(*arg) || *arg > 0)
         return (1);

  if (!i)
     return (1);

  return (0);
}

#define RECON		1
#define USURP		2
#define UNSWITCH	3


/*
При входе в игру по опциям будет предоставляться возможность
получить помощь по определенным вопросам*/
int pre_help(struct char_data *ch, char *arg)
{ char command[MAX_INPUT_LENGTH], topic[MAX_INPUT_LENGTH];

  half_chop(arg, command, topic);

  if (!command || !*command || strlen(command) < 2 ||
      !topic   || !*topic   || strlen(topic) < 2)
     return (0);
  if (isname(command, "помощь") ||
      isname(command, "help")   ||
      isname(command, "справка"))
     {do_help(ch,topic,0,0);
      return (1);
     }
  return (0);
}

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;

  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;

  int id = GET_IDNUM(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
     continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {    /* switched char */
     if (str_cmp(d->host, k->host))
	      {sprintf(buf,"ВТОРИЧНЫЙ ВХОД! ID = %ld Игрок = %s, Имя хоста = %s(предыдущий %s)",
                       GET_IDNUM(d->character),
                       GET_NAME(d->character),
                       d->host, k->host
                      );
	       mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
               send_to_gods(buf);
	      }
		
		SEND_TO_Q("\r\n. Обнаружен мультинг - ... Отсоединение.\r\n", k); /*Multiple login detected -- disconnecting*/
      STATE(k) = CON_CLOSE;
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && (GET_IDNUM(k->character) == id)) {
      if (str_cmp(d->host, k->host))
	      {sprintf(buf,"ПОВТОРНЫЙ ВХОД !!! ID = %ld Name = %s Host = %s(was %s)",
                       GET_IDNUM(d->character),
                       GET_NAME(d->character),
                       d->host, k->host
                      );
	       mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
              // send_to_gods(buf);
	      }
		if (!target && STATE(k) == CON_PLAYING) {
	SEND_TO_Q("\r\nТело было захвачено!\r\n", k);
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      SEND_TO_Q("\r\nОбнаружен мультинг - ... Отсоединение.\r\n", k);
      STATE(k) = CON_CLOSE;
    }
  }

 /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (ch->in_room != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch, FALSE);
  }
  
  strncpy(d->character->player_specials->temphost, d->host, HOST_LENGTH);
  // no target for swicthing into was found - allow login to continue 
  if (!target)
    return (0);

  // Хорошо, нашли цель.  Подключим дескриптор к цели.
  free_char(d->character); // Освобождаем место от старого чара
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT(PLR_FLAGS(d->character, PLR_WRITING), PLR_WRITING);
  REMOVE_BIT(PLR_FLAGS(d->character, PLR_MAILING), PLR_MAILING);

//  set_binary(d); // set russian input availabl
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:
   // if (PRF_FLAGGED(d->character, PRF_AUTOZLIB))
	   //toggle_compression(d);
  /*  if (IS_IMPL(d->character))
	{ do_gecho(d->character, "Натан пересоединился", 0, 0);
	  break;
	}*/
	SEND_TO_Q("Пересоединение.\r\n", d); 
    check_light(d->character,LIGHT_NO,LIGHT_NO,LIGHT_NO,LIGHT_NO,1);
	act("$n пересоединил$u.", TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s [%s] пересоединяется.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(d->character)), TRUE);
    
	break;
  case USURP:
//    toggle_compression(d);
    SEND_TO_Q("Вы снова завладели своим телом, которое уже было кем-то использовано!\r\n", d);
    act("$n взял$y свое тело снова под контроль!", 
	TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s пересоединяется, старый сокет закрыт.",
	    GET_NAME(d->character));
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(d->character)), TRUE);
    break;
  case UNSWITCH: //перевести!!!!
//    toggle_compression(d);
    SEND_TO_Q("Reconnecting to unswitched char.", d);
    sprintf(buf, "%s [%s] Reconnecting.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(d->character)), TRUE);
    break;
  }

  return (1);

}


/* 1. Проверяем сколько персонажей с 1 айпи в игре ( зарегестрированных как клубные игроки пропускаем ),
 *    если всего лишь 1 персонаж, пропускаем, если 2 персонажа, пропускаем, если больше 2 - автоматически
 *    помещаем в комнату для незарегестрированных игроков. Идея - что бы не мучаться богам и не регить
 *    создаваемых чаров, это позволит играть 2 чарами, несмотря на то, что нет богов и это не требует регистрации
 *    потому что по правилам 2 игрока могут играть и регистрация не требуется.
 */

extern int multing_mode;
int  check_dupes_host(struct descriptor_data *d)
{
 if (multing_mode)
       return 1;

 register struct char_data  *i;
 register struct descriptor_data *h;
 if (!d->character || IS_IMMORTAL(d->character) || PLR_FLAGGED(d->character, PLR_REGISTERED)|| isproxi(d->host))
    return (1);

 for (i = character_list; i; i = i->next)
 { if (i->desc == d || IS_NPC(i))
         continue;
      
      if (i && !IS_IMMORTAL(i) && 
	      !str_cmp(i->player_specials->temphost, d->host))//протестить перенести в линух вайп
	  { sprintf(buf,"\r\n&RВ Н И М А Н И Е!   Н А Р У Ш Е Н И Е   П Р А В И Л!&n\r\n Вход с одного IP(%s) возможен, зарегистрируйтесь у богов.\r\n"
	              "Чтобы зарегистрироваться, необходимо прислать заявку.\r\n"
		      "Персонаж %s имеет такой же IP адрес.\r\n",
			  i->player_specials->temphost, GET_NAME(i));
                        send_to_char(buf,d->character);
	                sprintf(buf,"! Вход с одного IP ! незарегистрированного героя.\r\n"
	              "Вошел - %s, в игре - %s, IP - %s.\r\n"
		      "Игрок помещен в комнату незарегистрированных героей.",
		      GET_NAME(d->character), GET_NAME(i), d->host);
          mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
	  return (0);
	 }
 }
 
 for (h = descriptor_list; h; h = h->next)
     {if (h == d)
         continue;

    if (h->character               &&
          !IS_IMMORTAL(h->character) &&
	  (STATE(h) == CON_PLAYING ||STATE(h) == CON_MENU) &&
	  !str_cmp(h->host, d->host))
	 { sprintf(buf,"\r\n&RВ Н И М А Н И Е!   Н А Р У Ш Е Н И Е   П Р А В И Л!&n\r\n Вход с одного IP(%s) возможен, зарегистрируйтесь у богов.\r\n"
	              "Чтобы зарегистрироваться, Вам необходимо прислать заявку.\r\n"
		      "Персонаж %s имеет такой же IP адрес.\r\n",
		       h->host, GET_NAME(d->character));
          send_to_char(buf,d->character);
	  sprintf(buf,"! Вход с одного IP ! незарегистрированного игрока.\r\n"
	              "Вошел - %s, в игре - %s, IP - %s.\r\n"
		      "Игрок помещен в комнату незарегистрированных игроков.",
		      GET_NAME(d->character), GET_NAME(h->character), d->host);
          mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
	   return (0);
	 }

 }
 return (1);
}

void do_entergame(struct descriptor_data *d)
{ int    load_room, neednews=0;
  struct char_data *ch;
  char *tmstr;
  time_t mytime;
 

     mytime = time(0);
     tmstr = (char *) asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';
  
 
  reset_char(d->character);
  read_aliases(d->character);
 /* if (GET_HOUSE_UID(d->character) && !House_check_exist(GET_HOUSE_UID(d->character)))
     GET_HOUSE_UID(d->character) = GET_HOUSE_RANK(d->character) = 0;   */
  if (PLR_FLAGGED(d->character, PLR_INVSTART))
     GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

  /*
   * We have to place the character in a room before equipping them
   * or equip_char() will gripe about the person in NOWHERE.
   */
  if (PLR_FLAGGED(d->character, PLR_HELLED))
     load_room = r_helled_start_room;
  else
  if (PLR_FLAGGED(d->character, PLR_NAMED))
     load_room = r_named_start_room;
  else
  if (PLR_FLAGGED(d->character, PLR_FROZEN))
     load_room = r_frozen_start_room;
  else
  if (!check_dupes_host(d))
     load_room = r_unreg_start_room;
  else
     {if ((load_room = GET_LOADROOM(d->character)) == NOWHERE)
         load_room = calc_loadroom(d->character);
      load_room = real_room(load_room);
     }

  log("Player %s enter at room %d", GET_NAME(d->character), load_room);
  /* If char was saved with NOWHERE, or real_room above failed... */
  if (load_room == NOWHERE)
     {if (GET_LEVEL(d->character) >= LVL_IMMORT)
         load_room = r_immort_start_room;
      else
         load_room = r_mortal_start_room;
     }
  
     if (load_room == 0)
     {if (GET_LEVEL(d->character) >= LVL_IMMORT)
         load_room = r_immort_start_room;
      else
         load_room = r_mortal_start_room;
     }

  send_to_char(WELC_MESSG, d->character);

  for (ch = character_list; ch; ch = ch->next)
      if (ch == d->character)
         break;
	
  if (!ch)
     {d->character->next = character_list;
      character_list     = d->character;
     }
  else
     {	REMOVE_BIT(MOB_FLAGS(ch, MOB_DELETE), MOB_DELETE);
	REMOVE_BIT(MOB_FLAGS(ch, MOB_FREE), MOB_FREE);
     }

  char_to_room(d->character, load_room);
  if (GET_LEVEL(d->character) != 0)
      Crash_load(d->character);
      

  if (lastnews && LAST_LOGON(d->character) < lastnews)
     neednews = TRUE;

    /* сбрасываем телы для команды "вспомнить" */
  /* for (i = 0; i < MAX_REMEMBER_TELLS; i++)
     GET_TELL(d->character,i)[0] = '\0';
   GET_LASTTELL(d->character) = 0;*/

   /* with the copyover patch, this next line goes in enter_player_game() */
   GET_ID(d->character) = GET_IDNUM(d->character);
   GET_ACTIVITY(d->character) = number(0, PLAYER_SAVE_ACTIVITY-1);
   save_char(d->character, NOWHERE);
   LAST_LOGON(d->character) = time(0);
   act("$n вош$i в Мир.", TRUE, d->character, 0, 0, TO_ROOM);
   /* with the copyover patch, this next line goes in enter_player_game() */
   read_saved_vars(d->character);
   greet_mtrigger(d->character,NULL, -1);
   greet_otrigger(d->character, -1); 
   greet_memory_mtrigger(d->character, NULL, -1);
   STATE(d) = CON_PLAYING;
   if (GET_LEVEL(d->character) == 0)
      {do_start(d->character, FALSE);
       send_to_char(START_MESSG, d->character);
       send_to_char("\r\nВоспользуйтесь командой '&Yновичок&n' для обучения.\r\n\r\n\r\n",d->character);
      }
  	sprintf(buf, "%s [%s] - вошел в игру - %s %d, %s.",
        GET_NAME(d->character), d->host,
        pc_class_types[(int) GET_CLASS(d->character)], GET_LEVEL(d->character),
        pc_race_types[GET_RACE(d->character)][GET_SEX(d->character)]);
        mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(d->character)), TRUE);
   //send_to_gods(buf);*/
   if (neednews)
      {sprintf(buf,"\r\n\r\n                                &W ВНИМАНИЕ\r\n\r\n"
	  "        &gВ Мире произошли изменения, читайте новости (команда &n'&Yновости&n'&g)&n\r\n\r\n\r\n");
          send_to_char(buf,d->character);
      }
 
   sprintf(buf,"&n%s. Ваше имя - %s. Ваш IP адрес - %s&n\r\n", tmstr ,GET_NAME(d->character), d->host);
   send_to_char(buf,d->character);
// *buf = '\0';
 
  look_at_room(d->character, 0);
  

   if (GET_CLAN(d->character))
   {  
      if (find_clan_by_id(GET_CLAN(d->character)) == -1)
      {
          send_to_char ("\r\n&RВАШ КЛАН БЫЛ РАСПУЩЕН!&n\r\n",d->character);
          GET_CLAN(d->character) = 0;
          GET_CLAN_RANK(d->character) = 0;
      }
      else
      {   
         // fix clan data (missed players)
         Clan_fix(d->character);
         send_to_char ("\r\n                               &WНОВОСТИ ВАШЕГО КЛАНА&n\r\n",d->character);
         Clan_news(d->character->desc, 0);
      }
   }
   
   if (has_mail(d->character))
      send_to_char("&GВам пришла почта!\r\n&n", d->character);
   update_exchange_bank(d->character);

 //  mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
//   sprintf(buf,"У Вас %s.\r\n",d->character->desc ? "есть дескриптор" : "нет дескриптора");
 //  sprintf(buf,"NEWS %ld %ld(%s)",lastnews,LAST_LOGON(d->character),GET_NAME(d->character)); 
 //    sprintf(buf,"%ld (%s)\r\n",LAST_LOGON(d->character),GET_NAME(d->character));
 //    send_to_char(buf,d->character);
   d->has_prompt = 0;
}


void GeTConnPass(struct descriptor_data *d)
{  int load_result;

  load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	SEND_TO_Q(" Извините, Вы не можете соединиться с Вашего сайта!\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Connection attempt for %s denied from %s",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
      if (GET_LEVEL(d->character) < circle_restrict) {
	//SEND_TO_Q("Вход в игру временно ограничен... Попробуйте позже.\r\n", d);
        SEND_TO_Q("Извените, игра приостановлена...\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
              

	 /* 	if (PLR_FLAGGED(d->character, PLR_ANTISPAMM) && !IS_IMMORTAL(d->character))
		{ if (d->character->player_specials->time.logon  < LAST_LOGON(d->character))
			{ SEND_TO_Q("Игра занята, Вы не можете войти сейчас!\r\n", d);
			     STATE(d) =  CON_CLOSE; 
				  return;
			}
		REMOVE_BIT(d->character->char_specials.saved.Act.flags[0], PLR_ANTISPAMM); 
		}*/

      /* check and make sure no other copies of this player are logged in */
      if (perform_dupe_check(d))
	       return;
       
	  if (GET_LEVEL(d->character) >= LVL_IMMORT)
	SEND_TO_Q(imotd, d);
      else
	SEND_TO_Q(motd, d);

      sprintf(buf, "%s [%s] подключается", GET_NAME(d->character), d->host);
      mudlog(buf, BRF, MAX(LVL_GRGOD, GET_INVIS_LEV(d->character)), TRUE);

      if (load_result) {
	sprintf(buf, "\r\n\r\n\007\007\007"
		"%s%d ОШИБКА РЕГИСТРАЦИИ %s СО ВРЕМЕНИ ПОСЛЕДНЕЙ УДАЧНОЙ РЕГИСТРАЦИИ.\r\n",
		CCRED(d->character, C_SPR), load_result, CCNRM(d->character, C_SPR));
	SEND_TO_Q(buf, d);
	GET_BAD_PWS(d->character) = 0;
      }
      SEND_TO_Q("*** НАЖМИТЕ ВВОД: ", d);
      STATE(d) = CON_RMOTD;

}


// deal with newcomers and other non-playing sockets 
void nanny(struct descriptor_data *d, char *arg)
{ char buf[256];
  int player_i, load_result, sklon_saze, i=0;
  char tmp_name[MAX_INPUT_LENGTH], pwd_name[MAX_INPUT_LENGTH], pwd_pwd[MAX_INPUT_LENGTH];
  int Is_Valid_Name(char *newname);
  static int RaseBonus = 0;

static const int SizeMinRases[][2] = 
  { 
	{14, 11},
	{10, 12},
	{29, 20},
	{25, 21},
	{9,  8 },
	{10, 10},
	{21, 22},
	{10, 12}
  };

static const int SizeMaxRases[][2] = 
  { 
	{29, 27},
	{23, 19},
	{43, 29},
	{34, 31},
	{21, 16},
	{23, 20},
	{31, 32},
	{24, 19}
  };
 
  skip_spaces(&arg);
 	
switch (STATE(d)) {
  
 /*. OLC states .*/
/*  case CON_OEDIT:
    oedit_parse(d, arg);
    break;
  case CON_REDIT:
    redit_parse(d, arg);
    break;
  case CON_ZEDIT:
    zedit_parse(d, arg);
    break;
  case CON_MEDIT:
    medit_parse(d, arg);
    break;
  case CON_SEDIT:
    sedit_parse(d, arg);
    break;
  case CON_TRIGEDIT:
    trigedit_parse(d, arg);
    break;*/
  /*. End of OLC states .*/





	case CON_CODEPAGE:		// select russian codepage (koi8, win, alt) 
  	if (!*arg)
  	  close_socket(d, TRUE);
  	else 
  	{
        if (strlen(arg) > 0)
	    arg[0] = arg[strlen(arg)-1];
        *arg -=1;
		if (!*arg || *arg < '0' || *arg >= '0'+KT_LAST)
           {SEND_TO_Q("               Unknown charset, please try again.\r\n"
	    	      "                       Enter charset: ", d);
	    if (++d->bad_pws > max_bad_pws )
	    	close_socket(d, TRUE);
            return;
           };
        d->codepage = (ubyte)*arg - (ubyte)'0';
      
        SEND_TO_Q("\r\n\r\n\r\nНапечатайте &Cсоздать&n для нового персонажа или &Cимя&n созданного: ", d);
		STATE(d) = CON_GET_NAME;
	
  	}
  	break;
   	  case CON_CREAT_NAME:		// Ожидание ввода имени 
    if (d->character == NULL)
	{ CREATE(d->character, struct char_data, 1);
          clear_char(d->character);
          CREATE(d->character->player_specials, struct player_special_data, 1);
          memset(d->character->player_specials, 0, sizeof(struct player_special_data));//добавил
          d->character->desc = d;
    };
   	if (!*arg)
		{ STATE(d) = CON_CLOSE;
	      	return;
		}
if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 4  ||
	    strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
	    fill_word(strcpy(buf, tmp_name))   || reserved_word(buf)) 
		{  SEND_TO_Q("Недопустимое имя, попробуйте еще раз.\r\nВведите корректное имя: ", d);
			STATE(d) = CON_CREAT_NAME;
			return;
		}	
		        

    if ((player_i = load_char(tmp_name, d->character)) > -1)
		{  GET_PFILEPOS(d->character) = player_i;
			if (PLR_FLAGGED(d->character, PLR_DELETED))
			   if (player_table[player_i].last_logon + 120 > time(0))
			   { SEND_TO_Q("Имя временно недоступно, придумайте &Wновое&n имя: ", d);
			      STATE(d) = CON_CREAT_NAME;
				return;
			   }
			else
			{    free_char(d->character); 
				 d->character = NULL;//добавил

// Check for multiple creations of a character. 
		if (!Valid_Name(tmp_name))
	        {SEND_TO_Q("Недопустимое имя, попробуйте еще раз.\r\nВведите корректное имя: ", d);
		         return;
	        }

	
      CREATE(d->character, struct char_data, 1);
	  //memset(d->character, 0, sizeof(struct char_data));
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  memset(d->character->player_specials, 0, sizeof(struct player_special_data));
	  d->character->desc = d;
	  SET_BIT(PRF_FLAGS(d->character), (PRF_COLOR_1 * (3 & 1)));
      SET_BIT(PRF_FLAGS(d->character), (PRF_COLOR_2 * (3 & 2) >> 1));
      CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));
	  SEND_TO_Q(create_name_rules, d);
	  sprintf(buf, "Вы подтверждаете свое имя &C%s&n (да/нет)? ", tmp_name);
	  SEND_TO_Q(buf, d);
	  STATE(d) = CON_NAME_CNFRM;
	  return;
				}
			else 
			{ SEND_TO_Q("Такой персонаж уже существует, придумайте &Wновое&n имя: ", d);
			  STATE(d) = CON_CREAT_NAME;
				return;
			}
	}
		
	
		if (cmp_ptable_by_name(tmp_name,MIN_NAME_LENGTH) >= 0)
	       {SEND_TO_Q("\r\n	Ограничение фильтра! Первые 4 символа выбранного Вами имени,\r\n"
	         "совпадают с уже существующим персонажем. Выберите пожалуйста другое имя.\r\n"
	          "Имя  : ", d);
	            return;
	        }
	 // игрок не найден -- создаем нового персонажа	
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  memset(d->character->player_specials, 0, sizeof(struct player_special_data));
	  d->character->desc = d;
	  SET_BIT(PRF_FLAGS(d->character), (PRF_COLOR_1 * (3 & 1)));
          SET_BIT(PRF_FLAGS(d->character), (PRF_COLOR_2 * (3 & 2) >> 1));
          CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));
	  sprintf(buf, "Вы подтверждаете свое имя &C%s&n (да/нет)? ", tmp_name);
	  SEND_TO_Q(buf, d);
	  STATE(d) = CON_NAME_CNFRM;
	 return;
	 break;

	  case CON_GET_NAME:
	if (!str_cmp(arg, "создать"))
		{	 SEND_TO_Q(create_name_rules, d);
		    SEND_TO_Q("&WКак Вы назовете своего Героя?:&n ", d);
			STATE(d) = CON_CREAT_NAME;
			return;
		} 

	if (d->character == NULL)
	{ CREATE(d->character, struct char_data, 1);
          clear_char(d->character);
          CREATE(d->character->player_specials, struct player_special_data, 1);
          memset(d->character->player_specials, 0, sizeof(struct player_special_data));
          d->character->desc = d;
        }
	
			
	  
    if (!*arg)
       STATE(d) = CON_CLOSE;
    else
	{ if (sscanf(arg,"%s %s",pwd_name, pwd_pwd) == 2)
		{ if (_parse_name(pwd_name, tmp_name) ||
	        (player_i = load_char(tmp_name, d->character)) < 0 )
	       {SEND_TO_Q("Некорректное имя. Повторите, пожалуйста.\r\n"
	                  "Имя: ", d);
	        return;
	       }
	    if (PLR_FLAGGED(d->character, PLR_DELETED) ||
	        strncmp(CRYPT(pwd_pwd, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)
	       )
	       {SEND_TO_Q("Некорректный пароль, попробуйте еще раз.\r\n"
	                  "Пароль : ", d);
               if (!PLR_FLAGGED(d->character, PLR_DELETED))
                  {sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
                   mudlog(buf, NRM, LVL_GRGOD, TRUE);
                  }
		free_char(d->character);			
		d->character = NULL;
	        return;
	       }
	    load_pkills(d->character);
	    REMOVE_BIT(PLR_FLAGS(d->character, PLR_MAILING), PLR_MAILING);
	    REMOVE_BIT(PLR_FLAGS(d->character, PLR_WRITING), PLR_WRITING);
	    REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRYO), PLR_CRYO);
            REMOVE_BIT(AFF_FLAGS(d->character, AFF_GROUP), AFF_GROUP);
	    GET_ID(d->character) = GET_IDNUM(d->character);
  //	    echo_off(d);
	    d->idle_tics = 0;
	    GeTConnPass(d);
	    return;
	  }
	else
		if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 4     ||
	  strlen(tmp_name) > 12 || !Is_Valid_Name(tmp_name) ||
	  fill_word(strcpy(buf, tmp_name))   || reserved_word(buf)) 
		{  SEND_TO_Q("Недопустимое имя, попробуйте еще раз.\r\n"
		  "Введите корректное имя: ", d);
			return;
		}
	else
		if (Is_Valid_Dc(tmp_name))
	   {SEND_TO_Q("Игрок с таким именем уже находится в Мире.\r\n",d);
	    SEND_TO_Q("Во избежание недоразумений введите пару ИМЯ ПАРОЛЬ.\r\n",d);
	    SEND_TO_Q("Имя и пароль через пробел: ",d);
	    return;
	   }

	 if ((player_i = load_char(tmp_name, d->character)) > -1)
			{  GET_PFILEPOS(d->character) = player_i;
			if (PLR_FLAGGED(d->character, PLR_DELETED))
				{ SEND_TO_Q("У Вас нет такого персонажа!", d);
				  SEND_TO_Q("\r\nНапечатайте &Cсоздать&n для нового персонажа или &Cимя&n созданного: ", d);
				  STATE(d) =  CON_GET_NAME;
				  free_char(d->character);
				  d->character = NULL;
				  return;
				}

	  load_pkills(d->character);
	  REMOVE_BIT(PLR_FLAGS(d->character, PLR_MAILING), PLR_MAILING);
	  REMOVE_BIT(PLR_FLAGS(d->character, PLR_WRITING), PLR_WRITING);
	  REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRYO), PLR_CRYO);
          REMOVE_BIT(AFF_FLAGS(d->character, AFF_GROUP), AFF_GROUP);
	  GET_ID(d->character) = GET_IDNUM(d->character);
	  SEND_TO_Q("Введите пароль: ", d);
  //	  echo_off(d);
	  d->idle_tics = 0;
	  STATE(d) = CON_PASSWORD;
	  return;
	 }
			  SET_BIT(PRF_FLAGS(d->character), (PRF_COLOR_1 * (3 & 1)));
              SET_BIT(PRF_FLAGS(d->character), (PRF_COLOR_2 * (3 & 2) >> 1));
			  sprintf(buf, "Персонаж с именем &C%s&n у Вас отстутствует!\r\nВедите имя Вашего персонажа или &Cсоздать&n для нового персонажа: ", tmp_name);
		      SEND_TO_Q(buf, d);
		      STATE(d) =  CON_GET_NAME;
	}				
    break;
  case CON_NAME_CNFRM:	// wait for conf. of new name 
    if (*arg == 'д' || *arg == 'Д') {
      if (isbanned(d->host) >= BAN_NEW) {
	sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
		GET_PC_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	SEND_TO_Q("С Вашего адреса запрещено создание персонажей (siteban)!\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	SEND_TO_Q("В настоящее время чтобы создать персонажа, необходимо приглашение.\r\n", d);
	sprintf(buf, "Попытка создания нового персонажа %s для [%s] отклонена (wizlock)",
		GET_PC_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	STATE(d) = CON_CLOSE;
	return;
      }
     
        sprintf(buf, "С этого момента вашего героя зовут &C%s&n", GET_NAME(d->character));
	SEND_TO_Q(buf, d);
	SEND_TO_Q("\r\n\r\n&WНеобходимо просклонять имя вашего героя.&n", d);
        SEND_TO_Q("\r\n\r\n&WРодительный падеж  &K[Нет кого?]&n         : ", d);
  //  echo_on(d);
	STATE(d) = CON_RNAME;
      break;
  case CON_RNAME:
     	sklon_saze = strlen(d->character->player.name);
    	_parse_name(arg, tmp_name);
	if (strncmp(CAP(tmp_name), d->character->player.name, sklon_saze - 1)==0);
	else
	 {
     SEND_TO_Q("Недопустимая форма склонения. Попробуйте еще раз.\r\n", d);
     SEND_TO_Q("&WРодительный падеж  &K[Нет кого?]&n         : ", d);
     STATE(d) = CON_RNAME;
       break;
 }
    CREATE(d->character->player.rname, char, strlen(arg) + 1);
		strcpy(d->character->player.rname, CAP(tmp_name));
          SEND_TO_Q("&WДательный падеж    &K[Дать меч кому?]&n    : ", d);
   // echo_on(d);
		  STATE(d) = CON_DNAME; 
    break;
    case CON_DNAME:
	sklon_saze = strlen(d->character->player.name);
    _parse_name(arg, tmp_name);
	if (strncmp(CAP(tmp_name), d->character->player.name, sklon_saze - 1) == 0);
		         else {
     SEND_TO_Q("Недопустимая форма склонения. Попробуйте еще раз.\r\n", d);
     SEND_TO_Q("&WДательный падеж    &K[Дать меч кому?]&n    : ", d);
	 
     
	 STATE(d) = CON_DNAME;
       break;
}
    CREATE(d->character->player.dname, char, strlen(arg) + 1);
	strcpy(d->character->player. dname, CAP(tmp_name));
    SEND_TO_Q("&WВинительный падеж  &K[Пнуть ногой кого?]&n : ", d);
//    echo_on(d);
	STATE(d) = CON_VNAME;
       break;
    case CON_VNAME:
	sklon_saze = strlen(d->character->player.name);
	_parse_name(arg, tmp_name);
	if (strncmp(CAP(tmp_name), d->character->player.name, sklon_saze - 1)==0);
		         else {
       SEND_TO_Q("Недопустимая форма склонения. Попробуйте еще раз.\r\n", d);
       SEND_TO_Q("&WВинительный падеж  &K[Пнуть ногой кого?]&n : ", d);
	   STATE(d) = CON_VNAME;
       break;
}
    CREATE(d->character->player.vname, char, strlen(arg) + 1);
	strcpy(d->character->player.vname, CAP(tmp_name));
	      SEND_TO_Q("&WТворительный падеж &K[Сражаться с кем?]&n  : ", d);
//    echo_on(d);
		  STATE(d) = CON_TNAME;
       break;
    case CON_TNAME:
		sklon_saze = strlen(d->character->player.name);
		_parse_name(arg, tmp_name);
		if (strncmp(CAP(tmp_name), d->character->player.name, sklon_saze - 1)==0);
		         else {
       SEND_TO_Q("Недопустимая форма склонения. Попробуйте еще раз.\r\n", d);
       SEND_TO_Q("&WТворительный падеж &K[Сражаться с кем?]&n  : ", d);
	   STATE(d) = CON_TNAME;
       break;
}
    CREATE(d->character->player.tname, char, strlen(arg) + 1);
	strcpy(d->character->player.tname, CAP(tmp_name));
	SEND_TO_Q("&WПредложный падеж   &K[Говорить о ком?]&n   : ", d);
    STATE(d) = CON_PNAME;
       break;
    case CON_PNAME:
		sklon_saze = strlen(d->character->player.name);
		_parse_name(arg, tmp_name);
		if (strncmp(CAP(tmp_name), d->character->player.name, sklon_saze - 1)==0);
		         else {
       SEND_TO_Q("Недопустимая форма склонения. Попробуйте еще раз.\r\n", d);
	   SEND_TO_Q("&WПредложный падеж   &K[Говорить о ком?]&n   : ", d);
	   STATE(d) = CON_PNAME;
       break;
}
    CREATE(d->character->player.pname, char, strlen(arg) + 1);
	strcpy(d->character->player.pname, CAP(tmp_name)); 
	  SEND_TO_Q("\r\n\r\n Придумайте пароль достаточной сложности, чтобы защитить доступ без вашего разрешения.\r\n"
						" Запомните или запишите пароль для его сохранности, если сочетание слишком сложное.\r\n"
						" Администрация Мира Ведьмака защищает персонажей при выполнении всех действий игроком,\r\n"
						" игроком, обеспечивающих надежную защиту персонажа. Спасибо.\r\n", d);
    sprintf(buf, "\r\n&WВведите пароль для &C%s&W:&n ", d->character->player.rname);
       SEND_TO_Q(buf, d);
//      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'н' || *arg == 'Н') {
      SEND_TO_Q("Хорошо, придумайте другое имя ", d);
      free(d->character->player.name);
      d->character->player.name = NULL;
      STATE(d) = CON_CREAT_NAME;
      
    } else {
      SEND_TO_Q("Введите, пожалуйста, Да или Нет: ", d);
    }
    break;
  
   
  case CON_PASSWORD:		/* get pwd for known player      */
    /*
     * To really prevent duping correctly, the player's record should
     * be reloaded from disk at this point (after the password has been
     * typed).  However I'm afraid that trying to load a character over
     * an already loaded character is going to cause some problem down the
     * road that I can't see at the moment.  So to compensate, I'm going to
     * (1) add a 15 or 20-second time limit for entering a password, and (2)
     * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
     */

//    echo_on(d);    /* turn echo back on */

    /* New echo_on() eats the return on telnet. Extra space better than none. */
    SEND_TO_Q("\r\n", d);

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else
	{ if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
		{ sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
			mudlog(buf, BRF, LVL_GRGOD, TRUE);
			GET_BAD_PWS(d->character)++;
			save_char(d->character, NOWHERE);
	if (++(d->bad_pws) >= max_bad_pws) 	// 3 попытки для введения верного пароля.
		{ SEND_TO_Q("Неверный пароль... - Отсоединение.\r\n", d); 
		  STATE(d) = CON_CLOSE;
		} else
			{ SEND_TO_Q("Неверный пароль.\r\nПароль: ", d);
//				  echo_off(d);
			}
	return;
		}

	GeTConnPass(d);
	}
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_PC_NAME(d->character))) {
      SEND_TO_Q("\r\nНеверный пароль.\r\n", d);
      SEND_TO_Q("Пароль: ", d);
      return;
    }
    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    SEND_TO_Q("\r\n&WПожалуйста, подтвердите пароль:&n ", d);
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;

    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH)) {
//       echo_on(d);
	  SEND_TO_Q("\r\nПароль не соответствует, попробуйте еще раз.\r\n", d);
      SEND_TO_Q("Пароль: ", d);
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
   

    if (STATE(d) == CON_CNFPASSWD) {
       SEND_TO_Q("\r\n Ни мужские, ни женские особи не получают каких либо преимуществ или \r\n"
		 " недостатков, и выбор определяет лишь ролевую составляющую игры. Однако\r\n"
		 " стоит учесть, что у некоторых рас мужские особи обычно могут быть чуть\r\n"
		 " крупнее женских (параметр размер), а у некоторых наоборот.\r\n"
		 " Более подробную информацию читайте в разделах СОЗДАНИЕ\r\n", d);
         SEND_TO_Q("\r\nЧтобы получить информацию, напечатайте:([справка|help]) Пример: справка создание ", d);
      	 sprintf(buf, "\r\nКакого пола &C%s?&W &W(М)&nужского или &W(Ж)&nенского : ", GET_NAME(d->character));
		 SEND_TO_Q(buf, d);
         STATE(d) = CON_QSEX;
	 }
	else {
      save_char(d->character, NOWHERE);
//     echo_on(d);
      SEND_TO_Q("\r\nГотово.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }

    break;

  case CON_QSEX:		/* query sex of new user         */
    if (pre_help(d->character,arg))
       {sprintf(buf, "\r\nКакого пола &C%s&n?&W &W(М)&nужского или &W(Ж)&nженского : ", GET_NAME(d->character));
		SEND_TO_Q(buf, d);
        STATE(d) = CON_QSEX;
        return;
       }
	switch (*arg) {
    case 'м':
    case 'М': 
   	  sprintf(buf, "\r\nС этого момента &CВы %s&n - искатель приключений.\r\n", GET_NAME(d->character));
	  SEND_TO_Q(buf, d);
	d->character->player.sex = SEX_MALE;
      break;
    case 'ж':
    case 'Ж':
	 sprintf(buf, "\r\nС этого момента &CВы %s&n - искательница приключений.\r\n", GET_NAME(d->character));
	 SEND_TO_Q(buf, d);
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      SEND_TO_Q("Некорректный выбор, попробуйте еще раз.\r\n"
		"Выберите пол вашего персонажа.", d);
      return;
    }

    SEND_TO_Q(class_menu, d);
    SEND_TO_Q("\r\nДля информации, напечатайте:([справка|help]) Пример: справка <класс> ", d);
	SEND_TO_Q("\r\nВаш класс: ", d);
    STATE(d) = CON_QCLASS;
	 break;

   case CON_QCLASS:
   if (pre_help(d->character,arg))
       {SEND_TO_Q(class_menu, d);
        SEND_TO_Q("\r\nВаш класс: ", d);
        STATE(d) = CON_QCLASS;
        return;
       }
	 if(!isdigit((unsigned char)*arg))
	 { SEND_TO_Q("\r\nНеверный аргумент справки. ", d); 
       SEND_TO_Q("\r\nДля информации, напечатайте:([справка|help]) Пример: справка <название_класс> ", d);
	   SEND_TO_Q("\r\nИли выбирите номер Вашего класса: ", d);
	   STATE(d) = CON_QCLASS;
      return;
	 }
	 load_result = atoi(arg);  
     load_result = parse_class(load_result);
					 
    if (load_result == CLASS_UNDEFINED) {
      SEND_TO_Q("\r\nНет такого номера класса.\r\nНаберите цифру вашего выбора класса: ", d); 
      return;
    } else
      GET_CLASS(d->character) = load_result;
     if (load_result == CLASS_MONAH || load_result == CLASS_WEDMAK)
	 { GET_RACE(d->character) = RACE_HUMAN;
	   SEND_TO_Q("\r\nВам автоматически присваивается раса ЧЕЛОВЕК\r\n",d);
        
	   GET_WEIGHT(d->character) = 100;
	   GET_HEIGHT(d->character) = 150;

SEND_TO_Q("\r\n &WВыберите Вашу натуру:&n\r\n"
"\r\n   Этот выбор определит ваше место в игре, стороны за какие вы будите играть.\r\n"
"Так уж получилось, что идет постоянная борьба между светлыми и темными силами,\r\n"
"и вам так или иначе придется участвовать в этом. Наклонность также определяет,\r\n"
"какими умениями ваш персонаж сможет овладеть в дальнейшем, после перерождений.\r\n"
"Ни на что другое на данный момент выбор наклонности влияния не оказывает.\r\n"
"&W1. Добрый.\r\n"
"2. Склонный к добру.\r\n"
"3. Нейтральный.\r\n"
"4. Склонный ко злу.\r\n"
"5. Злой.\r\n&n"
,d);
SEND_TO_Q("\r\nЧто бы получить информацию, напечатайте:([справка|help]) Пример: справка наклонность ", d);
   STATE(d) = CON_GET_ALIGNMENT;
    	break;
 }	

    SEND_TO_Q(race_menu, d);
    SEND_TO_Q("\r\nДля информации, напечатайте:([справка|help]) Пример: справка <название_расы> ", d);
	SEND_TO_Q("\r\nВаша раса: ", d);
    STATE(d) = CON_QRACE;
    break;

     case CON_QRACE:
   if (pre_help(d->character,arg))
       {SEND_TO_Q(race_menu, d);
        SEND_TO_Q("\r\nВаша раса: ", d);
        STATE(d) = CON_QRACE;
        return;
       }
     if(!isdigit((unsigned char)*arg))
	 { SEND_TO_Q("\r\nНеверный аргумент справки. ", d); 
       SEND_TO_Q("\r\nДля информации, напечатайте:([справка|help]) Пример: справка <название_расы> ", d);
	   SEND_TO_Q("\r\nИли выберите номер Вашей расы: ", d);
	   STATE(d) = CON_QRACE;
      return;
	 } 
      load_result = parse_race(*arg);
      if (load_result == RACE_UNDEFINED) {
      SEND_TO_Q("\r\nНет такого номера расы.\r\nВыберите номер Вашей расы: ", d);
      return;
    } else
	{ GET_WEIGHT(d->character) = 100;
	  GET_HEIGHT(d->character) = 150;
          GET_RACE(d->character) = load_result;
		}	
   
	SEND_TO_Q("\r\n &WВыберите Вашу наклонность:&n\r\n"
"\r\n   Этот выбор определит ваше место в игре, стороны за какие вы будите играть.\r\n"
"Так уж получилось, что идет постоянная борьба между светлыми и темными силами,\r\n"
"и вам так или иначе придется участвовать в этом. Наклонность также определяет,\r\n"
"какими умениями ваш персонаж сможет овладеть в дальнейшем, после перерождений.\r\n"
"Ни на что другое на данный момент выбор наклонности влияния не оказывает.\r\n"
"&W1. Добрый.\r\n"
"2. Склонный к добру.\r\n"
"3. Нейтральный.\r\n"
"4. Склонный ко злу.\r\n"
"5. Злой.\r\n&n"
,d);
SEND_TO_Q("\r\nЧто бы получить информацию, напечатайте:([справка|help]) Пример: справка <наклонность> ", d);
   STATE(d) = CON_GET_ALIGNMENT;
    	break;

    case CON_GET_ALIGNMENT:
		if (pre_help(d->character,arg))
       {SEND_TO_Q("\r\n &WВыберите Вашу наклонность:&n\r\n"
"Этот выбор определит ваше место в игре, стороны за какие вы будите играть.\r\n"
"Так уж получилось, что идет постоянная борьба между светлыми и темными силами,\r\n"
"и вам так или иначе придется участвовать в этом. Наклонность также определяет,\r\n"
"какими умениями ваш персонаж сможет овладеть в дальнейшем, после перерождений.\r\n"
"Ни на что другое на данный момент выбор наклонности влияния не оказывает.\r\n"
"&W1. Добрый.\r\n"
"2. Склонный к добру.\r\n"
"3. Нейтральный.\r\n"
"4. Склонный ко злу.\r\n"
"5. Злой.\r\n&n"
,d);
        SEND_TO_Q("\r\nВаша наклонность: ", d);
        STATE(d) = CON_GET_ALIGNMENT;
        return;
       }
		load_result = atoi(arg); 
	switch(load_result)
	  {
	  case 1: 
		GET_ALIGNMENT(d->character) = 800; 
	        SEND_TO_Q( "\r\nВы не приемлите зло в своей натуре.\n\r",d);
		break;
	  case 2: 
		GET_ALIGNMENT(d->character) = 500;	
	        SEND_TO_Q("\r\nВы чувствуете тягу к добрым поступкам.\r\n",d);
		break;
	  case 3:  
		GET_ALIGNMENT(d->character) = 0; 
	        SEND_TO_Q("\r\nВас не прильщает ни добро, ни зло.\r\n",d);
		break;
		case 4:  
		GET_ALIGNMENT(d->character) = -500; 
	        SEND_TO_Q("\r\nВаш нрав жесток.\r\n",d);
		break;
		case 5:  
		GET_ALIGNMENT(d->character) = -800; 
	        SEND_TO_Q("\r\nВы злы по своей природе.\r\n",d);
		break;
	  default:
	    SEND_TO_Q("\r\nТакой наклонности не существует.\r\n",d);
	    SEND_TO_Q("\r\nВыберите наклонность (1/2/3/4/5)? ",d);
	    return;
	}

sprintf(buf,"\r\n&WТеперь вам предстоит выбрать стартовые характеристики персонажа &C%s:&n",GET_RNAME(d->character));



    SEND_TO_Q(buf, d);
    SEND_TO_Q("\r\n Следует обратить больше внимания на профилирующие для выбранной профессии,\r\n"
" предполагаемого стиля игры. В дальнейшем это окажет решающее значение на успех\r\n"
" вашего персонажа, позволяя быть более эффективным в нужных областях.\r\n"
" Более подробную информацию читайте в разделах ХАРАКТЕРИСТИКИ, СОЗДАНИЕ\r\n"
" Выберите метод генерации характеристик для вашего персонажа:\r\n\r\n"
"       1. Автоматический (рекомендуется для ознакомления с игрой)\r\n"
"       2. Ручной         (рекомендуется для опытных игроков) ", d);

	RaseBonus = (GET_RACE(d->character) == RACE_HUMAN ? 78 : 79);
	STATE(d) = CON_ACCEPT_STATS;
	    
		break;

	case CON_ROLL_INT:
		load_result = atoi(arg);
				if (load_result > ClasStat[GET_RACE(d->character)][INT] + 2 ||
					load_result < ClasStat[GET_RACE(d->character)][INT])
				{	SEND_TO_Q("\r\nВы вышли за пределы выбора характеристики! Повторите выбор\r\n", d);
					sprintf(buf,"&cВыберите значение &WИнтеллекта [%-2d - %-2d]&n ",
				    ClasStat[GET_RACE(d->character)][INT], ClasStat[GET_RACE(d->character)][INT] + 2);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_ROLL_INT;
					break;
				}
			
				GET_INT_ROLL(d->character) = GET_INT(d->character) = load_result;
				sprintf(buf,"&cОсталось &C%d&c бонусов. Выберите значение &WМудрости   [%-2d - %-2d]&n ",
				RaseBonus -=load_result,
				ClasStat[GET_RACE(d->character)][WIS], ClasStat[GET_RACE(d->character)][WIS] + 2);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_ROLL_WIS;
					break;
	case CON_ROLL_WIS:
		load_result = atoi(arg);
				if (load_result > ClasStat[GET_RACE(d->character)][WIS] + 2 ||
					load_result < ClasStat[GET_RACE(d->character)][WIS])
				{	SEND_TO_Q("\r\nВы вышли за пределы выбора характеристики! Повторите выбор\r\n", d);
					sprintf(buf,"&cВыберите значение &WМудрости [%-2d - %-2d]&n ",
				    ClasStat[GET_RACE(d->character)][WIS], ClasStat[GET_RACE(d->character)][WIS] + 2);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_ROLL_WIS;
				    break;
				}
			
				GET_WIS_ROLL(d->character) = GET_WIS(d->character) = load_result;
				sprintf(buf,"&cОсталось &C%d&c бонусов. Выберите значение &WЛовкости   [%-2d - %-2d]&n ",
				RaseBonus -=load_result,
				ClasStat[GET_RACE(d->character)][DEX], ClasStat[GET_RACE(d->character)][DEX] + 2);
				SEND_TO_Q(buf, d); 
				STATE(d) = CON_ROLL_DEX;
					break;

	case CON_ROLL_DEX:
		load_result = atoi(arg);
				if (load_result > ClasStat[GET_RACE(d->character)][DEX] + 2 ||
					load_result < ClasStat[GET_RACE(d->character)][DEX])
				{	SEND_TO_Q("\r\nВы вышли за пределы выбора характеристики! Повторите выбор\r\n", d);
					sprintf(buf,"&cВыберите значение &WЛовкости [%-2d - %-2d]&n ",
				    ClasStat[GET_RACE(d->character)][DEX], ClasStat[GET_RACE(d->character)][DEX] + 2);
					SEND_TO_Q(buf, d); 
					STATE(d) = CON_ROLL_DEX;
					break;
				}
			
				GET_DEX_ROLL(d->character) = GET_DEX(d->character) = load_result;
				sprintf(buf,"&cОсталось &C%d&c бонусов. Выберите значение &WУдачи      [%-2d - %-2d]&n ",
				RaseBonus -=load_result,
				ClasStat[GET_RACE(d->character)][CHA], ClasStat[GET_RACE(d->character)][CHA] + 2);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_ROLL_CHA;
				    break;

	case CON_ROLL_CHA:
		load_result = atoi(arg);
				if (load_result > ClasStat[GET_RACE(d->character)][CHA] + 2 ||
					load_result < ClasStat[GET_RACE(d->character)][CHA])
				{	SEND_TO_Q("\r\nВы вышли за пределы выбора характеристики! Повторите выбор\r\n", d);
					sprintf(buf,"&cВыберите значение &WУдачи [%-2d - %-2d]&n ",
				    ClasStat[GET_RACE(d->character)][CHA], ClasStat[GET_RACE(d->character)][CHA] + 2);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_ROLL_CHA;
					break;
				}
			
				GET_CHA_ROLL(d->character) = GET_CHA(d->character) = load_result;
				sprintf(buf,"&cОсталось &C%d&c бонусов. Выберите значение &WСилы       [%-2d - %-2d]&n ",
				RaseBonus -=load_result,
				ClasStat[GET_RACE(d->character)][STR], ClasStat[GET_RACE(d->character)][STR] + 2);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_ROLL_STR;
				break;

	case CON_ROLL_STR:
		load_result = atoi(arg);
				if (load_result > ClasStat[GET_RACE(d->character)][STR] + 2 ||
					load_result < ClasStat[GET_RACE(d->character)][STR])
				{	SEND_TO_Q("\r\nВы вышли за пределы выбора характеристики! Повторите выбор\r\n", d);
					sprintf(buf,"&cВыберите значение &WСилы [%-2d - %-2d]&n ",
				    ClasStat[GET_RACE(d->character)][STR], ClasStat[GET_RACE(d->character)][STR] + 2);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_ROLL_STR;
					break;
				}
			
				GET_STR_ROLL(d->character) = GET_STR(d->character) = load_result;
				RaseBonus -=load_result;
				if(RaseBonus < ClasStat[GET_RACE(d->character)][CON])
						{ sprintf(buf,"\r\n&RОсталось &C%d&R бонусов. Вы неправильно распределили характеристики, попробуйте снова!&n\r\n ", RaseBonus);	
					      SEND_TO_Q(buf, d);
						
    SEND_TO_Q("\r\n		Выберите метод генерации характеристик для Вашего персонажа:\r\n\r\n"
						"       1. Автоматический (рекомендуется для ознакомления с игрой)\r\n"
						"       2. Ручной         (рекомендуется для оптыных игроков) ", d);
				RaseBonus = (GET_RACE(d->character) == RACE_HUMAN ? 78 : 79);
					STATE(d) = CON_ACCEPT_STATS;
						return;
						}

				sprintf(buf,"&cОсталось &C%d&c бонусов. Выберите значение &WТела       [%-2d - %-2d]&n ",
				RaseBonus,
				ClasStat[GET_RACE(d->character)][CON], ClasStat[GET_RACE(d->character)][CON] + 2);
				SEND_TO_Q(buf, d); 
				STATE(d) = CON_ROLL_CON;
				break;

	case CON_ROLL_CON:
		load_result = atoi(arg);
		
				if (load_result > ClasStat[GET_RACE(d->character)][CON] + 2 ||
					load_result < ClasStat[GET_RACE(d->character)][CON])
				{	SEND_TO_Q("\r\nВы вышили за пределы выбора характеристики! Повторите выбор\r\n", d);
					sprintf(buf,"&cВыберите значение &WТела [%-2d - %-2d]&n ",
				    ClasStat[GET_RACE(d->character)][CON], ClasStat[GET_RACE(d->character)][CON] + 2);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_ROLL_CON;
					break;
				}
					
				if(RaseBonus < load_result)
						{ sprintf(buf,"\r\n&RНедопустимое число, у Вас осталось &C%d&R бонусов.\r\n", RaseBonus);	
					      SEND_TO_Q(buf, d);
		sprintf(buf,"&cВыберите значение &WТела [%-2d - %-2d]&n ",
				    ClasStat[GET_RACE(d->character)][CON], ClasStat[GET_RACE(d->character)][CON] + 2);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_ROLL_CON;
					break;
						}
					
					
				GET_CON_ROLL(d->character) = GET_CON(d->character) = load_result;
				sprintf(buf,"&cВыберите значение &WРазмера  [%-2d - %-2d]&n ",
				SizeMinRases[GET_RACE(d->character)][GET_SEX(d->character)-1],
				SizeMaxRases[GET_RACE(d->character)][GET_SEX(d->character)-1]);
				SEND_TO_Q(buf, d); 
				STATE(d) = CON_ROLL_SIZE;
				break;

   case CON_ROLL_SIZE:
		load_result = atoi(arg);
				if (load_result > SizeMaxRases[GET_RACE(d->character)][GET_SEX(d->character)-1] ||
					load_result < SizeMinRases[GET_RACE(d->character)][GET_SEX(d->character)-1])
				{	SEND_TO_Q("\r\nВы вышили за пределы выбора размера! Повторите выбор\r\n", d);
					sprintf(buf,"&cВыберите значение &WРазмера [%-2d - %-2d]&n ",
				    SizeMinRases[GET_RACE(d->character)][GET_SEX(d->character)-1],
					SizeMaxRases[GET_RACE(d->character)][GET_SEX(d->character)-1]);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_ROLL_SIZE;
					break;
				}
			
				GET_SIZE(d->character) = load_result;
				sprintf(buf, "\r\nПерсонаж &W%s&n успешно создан и готов перенестись в Мир!\r\n", GET_NAME(d->character));
				SEND_TO_Q(buf, d);

	SEND_TO_Q("\r\n                                     &KОБРАЩАЕМ ВАШЕ ВНИМАНИЕ!!!\r\n"
		  "                           ВСЕ ВАШИ ПЕРСОНАЖИ ДОЛЖНЫ ИМЕТЬ ОДИНАКОВЫЙ e-mail:"
		  "\r\n                                        &WВведите Ваш e-mail:&n",d);	
	STATE(d) = CON_GET_EMAIL;
		break;
	
	case CON_ACCEPT_STATS:
			switch(*arg)
			{
			case '1':
				break;	
			case '2':
				sprintf(buf,"&cУ Вас  : &C%d&c бонусов. Выберите значение &WИнтеллекта [%-2d - %-2d] ", RaseBonus,
				ClasStat[GET_RACE(d->character)][INT], ClasStat[GET_RACE(d->character)][INT] + 2);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_ROLL_INT;
				
				return;
			default:
			SEND_TO_Q("\r\nВыберите пункт: 1 или 2  ", d);
			STATE(d) = CON_ACCEPT_STATS;
			return;
			}

	    roll_real_abils(d->character, 79);
		sprintf(buf, "\r\nСила:%d  Интеллект:%d  Мудрость:%d  Ловкость:%d  Тело:%d  Удача:%d\n\r",
		GET_STR(d->character), GET_INT(d->character), GET_WIS(d->character),
		GET_DEX(d->character), GET_CON(d->character), GET_CHA(d->character)); 
         SEND_TO_Q(buf, d); 
	     SEND_TO_Q("\r\nСохранить характеристики Вашего персонажа(Д/Н)? ",d);
		 STATE(d) = CON_ACCEPT_ROLL; 
		 break;
	   

  case CON_ACCEPT_ROLL:
 switch(*arg) {
	    case 'д': case 'Д':case 'y': case 'Y':
	    break;
        case 'н': case 'Н':case 'n': case 'N':
		roll_real_abils(d->character, 79);
		sprintf(buf, "Сила:%d  Интеллект:%d  Мудрость:%d  Ловкость:%d  Тело:%d  Удача:%d\n\r",
		GET_STR(d->character), GET_INT(d->character), GET_WIS(d->character),
		GET_DEX(d->character), GET_CON(d->character), GET_CHA(d->character)); 
        SEND_TO_Q(buf, d); 
		SEND_TO_Q("\r\nСохранить (Д/Н)? ",d);
        STATE(d) = CON_ACCEPT_ROLL;

		return;
        default:
         SEND_TO_Q("\r\nВыберите: Д/Н ", d);
          return;
		}

    sprintf(buf, "\r\nПерсонаж &W%s&n успешно создан и готов перенестись в Мир!\r\n", GET_NAME(d->character));
	SEND_TO_Q(buf, d);

	SEND_TO_Q("\r\n                                     &KОБРАЩАЕМ ВАШЕ ВНИМАНИЕ!!!\r\n"
		  "                           ВСЕ ВАШИ ПЕРСОНАЖИ ДОЛЖНЫ ИМЕТЬ ОДИНАКОВЫЙ e-mail:"
		  "\r\n                                        &WВведите Ваш e-mail:&n",d);	
	STATE(d) = CON_GET_EMAIL;
	break;		

		case CON_GET_EMAIL:
       if (!*arg)
       {SEND_TO_Q("\r\nВведите Ваш е-mail: ",d);
        return;
       }
    else
       if (!valid_email(arg))
		{ SEND_TO_Q("\r\nВведите, пожалуйста, корректный е-mail."
                           "\r\nВаш е-mail:  ",d);
        	 return;
		}

    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
        
    init_char(d->character);
    strncpy(GET_EMAIL(d->character),arg,127);
    *(GET_EMAIL(d->character)+127) = '\0';
    save_char(d->character, NOWHERE);
    SEND_TO_Q(motd, d);
    SEND_TO_Q("*** Нажмите Enter: ", d);
    STATE(d) = CON_RMOTD;

    sprintf(buf, "%s [%s] новый игрок.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, LVL_GRGOD, TRUE);
    break;

  case CON_RMOTD:		// read CR after printing motd  
//    set_binary(d);
	SEND_TO_Q(MENU, d);
    STATE(d) = CON_MENU;
  // do_entergame(d);
	  break;

  case CON_MENU:		// Меню выбора
    switch (*arg) {
    case '0':           	
      SEND_TO_Q("[&KНаша страница Вконтакте:&n &Wvk.com/mudlast&n]\r\n", d);

	  if (GET_REMORT(d->character) == 0 && GET_LEVEL(d->character) <= 25
			    && !IS_SET(PLR_FLAGS(d->character, PLR_NODELETE), PLR_NODELETE)) {
				int timeout = -1;
				for (int ci = 0; GET_LEVEL(d->character) > pclean_criteria[ci].level; ci++) {
					timeout = pclean_criteria[ci + 1].days;
				}
				if (timeout > 0) {
					time_t deltime = time(NULL) + timeout * 60 * 60 * 24;
				//	struct tm tmptm;
				//	localtime_r(&deltime, &tmptm);// это пользительно для юникс системы, у мну не работает.
					char *timeptr = asctime(localtime(&deltime));
						sprintf(buf, "Ваш персонаж будет сохранен до %s.\r\n",/* rustime(&tmptm)*/timeptr+4);
					SEND_TO_Q(buf, d);
				}
			};
      STATE(d) = CON_CLOSE;
       break;
  /*  case '0':
      SEND_TO_Q("До свидания.\r\n", d);
      STATE(d) = CON_CLOSE;
       break; */

    case '1':
      do_entergame(d);
      REMOVE_BIT(PLR_FLAGS(d->character, PLR_QUESTOR), PLR_QUESTOR);
      break;
    
    case '2':
      if (d->character->player.description) {
	SEND_TO_Q("Ваше предыдущее описание:\r\n", d);
	SEND_TO_Q(d->character->player.description, d);
    d->backstr = str_dup(d->character->player.description);
      }
      SEND_TO_Q("Введите описание своего персонажа, чтобы те кто на Вас смотрел его видели.\r\n", d);
      SEND_TO_Q("Для помощи редактирования введите /h. Для завершения ввода текста введите '@'.\r\n", d);
      d->str = &d->character->player.description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      SEND_TO_Q("\r\nВведите Ваш старый пароль: ", d);
//      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      SEND_TO_Q("\r\nВведите Ваш пароль для проверки: ", d);
//     echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

    default:
      SEND_TO_Q("\r\nЭто не выбор меню!\r\n", d);
      SEND_TO_Q(MENU, d);
      break;
    }

    break;

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
//      echo_on(d);
      SEND_TO_Q("\r\nНеверный пароль.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    } else {
      SEND_TO_Q("\r\nВведите новый пароль: ", d);
      STATE(d) = CON_CHPWD_GETNEW;
    }
    return;

  case CON_DELCNF1:
//    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nНеверный пароль.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    } else {
      SEND_TO_Q("\r\nВЫ ДЕЙСТВИТЕЛЬНО ХОТИТЕ УДАЛИТЬ ЭТОГО ПЕРСОНАЖА?\r\n"
		"ВЫ ПОДТВЕРЖДАЕТЕ?\r\n\r\n"
		"Пожалуйста введите \"да\" для подтверждения: ", d);
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
      if (!strcmp(arg, "да") || !strcmp(arg, "ДА")) 
        { if (PLR_FLAGGED(d->character, PLR_FROZEN)  || IS_IMMORTAL(d->character))
          { SEND_TO_Q("Вы попытались удалить себя, но Небеса остановили Вас.\r\n", d);
	    SEND_TO_Q("Персонаж не удален.\r\n\r\n", d);
	    STATE(d) = CON_CLOSE;
	     return;
          }
      delete_char(GET_NAME(d->character));
      sprintf(buf, "Персонаж '&C%s&n' удален.\r\n"
	           "Следите за новостями на нашей страничке Вконтакте: &Wvk.com/mudlast&n.\r\n", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      sprintf(buf, "%s (Level %d) delete.", GET_NAME(d->character),
      GET_LEVEL(d->character)); /*lev %d) has self-deleted*/
      mudlog(buf, NRM, LVL_GOD, TRUE);
      STATE(d) = CON_CLOSE;
      return;
       } 
       else 
         { SEND_TO_Q("\r\nПерсонаж не удален.\r\n", d);
           SEND_TO_Q(MENU, d);
           STATE(d) = CON_MENU;
         }
    break;

  /*
   * It's possible, if enough pulses are missed, to kick someone off
   * while they are at the password prompt. We'll just defer to let
   * the game_loop() axe them.
   */
  case CON_CLOSE:
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
	STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
  }
}
