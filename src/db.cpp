/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
//#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "mobmax.h"
#include "pk.h"
#include "sys/stat.h"
#include "clan.h"
#include "diskio.h"

#include "exchange.h"
/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

#if defined(CIRCLE_MACINTOSH)
  long beginning_of_time = -1561789232;
#else
  long beginning_of_time = 650336715;
#endif



struct room_data *world = NULL;			// array of rooms	
room_rnum top_of_world = 0;			// ref to top element of world
const FLAG_DATA clear_flags = { {0,0,0,0} }; 	//(инициализация доп.флагов
struct char_data *character_list = NULL;	// глобальный связной список для *char
struct index_data **trig_index; 		// таблица индексов для триггеров     
int   top_of_trigt = 0;           		// top of trigger index table  
long max_id = MOBOBJ_ID_BASE;   		// for unique mob/obj id's      
struct index_data *mob_index;			// index table for mobile file	
struct char_data *mob_proto;			// prototypes for mobs		
mob_rnum top_of_mobt = 0;			// top of mobile index table	 
struct cheat_list_type *cheaters_list; 		// Список разрешенных читеров 
struct obj_data *object_list = NULL;		// global linked list of objs	 
struct index_data *obj_index;			// index table for object file	
struct obj_data *obj_proto;			// prototypes for objs		
obj_rnum top_of_objt = 0;			// top of object index table

struct zone_data *zone_table;	/* zone table			 */
zone_rnum top_of_zone_table = 0;/* top element of zone tab	 */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages	 */
struct player_index_element *player_table = NULL;	/* index to plr file	 */
FILE *player_fl = NULL;		/* file desc of player file	 */
int top_of_p_table = 0;		/* ref to top of table		 */
int top_of_p_file = 0;		/* ref of size of p file	 */
long top_idnum = 0;		/* highest idnum in use		 */
int supress_godsapply = FALSE;
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
int now_entrycount = FALSE;
time_t boot_time = 0;		/* time of mud boot		 */
int circle_restrict = 0;	/* level of game restriction	 */
room_rnum r_helled_start_room;
room_rnum r_mortal_start_room;	/* rnum of mortal start room	 */
room_rnum r_immort_start_room;	/* rnum of immort start room	 */
room_rnum r_frozen_start_room;	/* rnum of frozen start room	 */
room_rnum r_named_start_room;
room_rnum r_unreg_start_room;

extern room_vnum unreg_start_room;
extern room_vnum named_start_room;

char *credits = NULL;		/* game credits			 */
long lastnews = 0;
char *news = NULL;		/* mud news			 */
char *motd = NULL;		/* message of the day - mortals */
char *imotd = NULL;		/* message of the day - immorts */
char *GREETINGS = NULL;		/* opening credits screen	*/
char *help = NULL;		/* help screen			 */
char *info = NULL;		/* info page			 */
char *wizlist = NULL;		/* list of higher gods		 */
char *immlist = NULL;		/* list of peon gods		 */
char *background = NULL;	/* background story		 */
char *handbook = NULL;		/* handbook for new immortals	 */
char *policies = NULL;		/* policies page		 */
char *zony = NULL;		/* zony page		 */

struct help_index_element *help_table = 0;	/* the help table	 */
int top_of_helpt = 0;		/* top of help index table	 */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	*/
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

/* local functions */
extern void calc_god_celebrate();
void new_save_char(struct char_data * ch, room_rnum load_room);
void new_build_player_index(void);
void add_follower(struct char_data * ch, struct char_data * leader);
int  mobs_in_room(int m_num, int r_num); /*(количество мобов в комнате)*/
int check_object_spell_number(struct obj_data *obj, int val);
int check_object_level(struct obj_data *obj, int val);
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode, char *filename);
int check_object(struct obj_data *);
void parse_trigger(FILE *fl, int virtual_nr);
void parse_room(FILE * fl, int virtual_nr);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void load_help(FILE *fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void reboot_wizlists(void);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE *fl);
int count_hash_records(FILE * fl);
int count_social_records(FILE *fl, int *messages, int *keywords);
bitvector_t asciiflag_conv(char *flag);
void asciiflag_conv1(char *flag, void *value);
void parse_simple_mob(FILE *mob_f, int i, int nr);
void interpret_espec(const char *keyword, char *value, int i, int nr);
void parse_espec(char *buf, int i, int nr);
void parse_enhanced_mob(FILE *mob_f, int i, int nr);
void get_one_line(FILE *fl, char *buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
void load_corpses(void);
long get_ptable_by_name(char *name);
void load_socials(FILE *fl);

/* external functions */
extern void tascii(int *pointer, int num_planes, char *ascii);
extern void ItemDecayZrepop(zone_rnum zone);
void   name_from_drinkcon(struct obj_data * obj);
void   name_to_drinkcon(struct obj_data * obj, int type);
void   create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void   extract_mob(struct char_data *ch);
void   Crash_clear_objects(int index);
int    Crash_read_timer(int index, int temp);
int    calc_loadroom(struct char_data *ch);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void   free_alias(struct alias_data *a);
void   load_messages(void);
void   weather_and_time(int mode);
void   mag_assign_spells(void);
//void   boot_social_messages(void);
void   update_obj_file(void);	/* In objsave.c */
void   sort_commands(void);
void   sort_spells(void);
void   load_banned(void);
void   load_proxi(void);
void   Read_Invalid_List(void);
void   boot_the_shops(FILE * shop_f, char *filename, int rec_count);
int    find_name(char *name);
int    hsort(const void *a, const void *b);
int    csort(const void *a, const void *b);
void   prune_crlf(char *txt);
void   new_save_quests(struct char_data * ch);
void   save_char_vars(struct char_data *ch);
void   new_load_quests(struct char_data * ch);
void   calc_easter(void);
void   die_follower(struct char_data * ch);
void   do_start(struct char_data *ch, int newbie);
int    real_zone(int number);
void   renum_obj_zone(void);
void   renum_mob_zone(void);
int    get_zone_rooms( int, int *, int * );

/* external vars */
extern struct month_temperature_type year_temp[];
extern const int sunrise[][2];
extern room_vnum helled_start_room;
extern int no_specials;
extern int scheck;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern struct descriptor_data *descriptor_list;
extern const char *weapon_affects[];
//extern struct pclean_criteria_data pclean_criteria[];
extern const char *unused_spellname;
extern int top_of_socialm;
extern int top_of_socialk;

#define READ_SIZE 256


/* Separate a 4-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for(i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);
  *ttag = '\0';
  
  while(*tmp == ':' || *tmp == ' ')
    tmp++;

  while(*tmp)
    *(wrt++) = *(tmp++);
  *wrt = '\0';
}
/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
  struct stat sb;
  int i;

  one_argument(argument, arg);

  if (!str_cmp(arg, "все") || *arg == '*') {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
    file_to_string_alloc(NEWS_FILE, &news);
    if (stat(LIB""NEWS_FILE,&sb) >= 0)
         lastnews = sb.st_mtime;
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(HELP_PAGE_FILE, &help);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(ZONY_FILE, &zony);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
  } else if (!str_cmp(arg, "wizlist"))
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
  else if (!str_cmp(arg, "immlist"))
    file_to_string_alloc(IMMLIST_FILE, &immlist);
  else if (!str_cmp(arg, "news")){
    file_to_string_alloc(NEWS_FILE, &news);
		if (stat(NEWS_FILE,&sb) >= 0)
         lastnews = sb.st_mtime;
		}
  else if (!str_cmp(arg, "credits"))
    file_to_string_alloc(CREDITS_FILE, &credits);
  else if (!str_cmp(arg, "motd"))
    file_to_string_alloc(MOTD_FILE, &motd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string_alloc(IMOTD_FILE, &imotd);
  else if (!str_cmp(arg, "help"))
    file_to_string_alloc(HELP_PAGE_FILE, &help);
  else if (!str_cmp(arg, "info"))
    file_to_string_alloc(INFO_FILE, &info);
  else if (!str_cmp(arg, "policy"))
    file_to_string_alloc(POLICIES_FILE, &policies);
  else if (!str_cmp(arg, "zony"))
    file_to_string_alloc(ZONY_FILE, &zony);
  else if (!str_cmp(arg, "handbook"))
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else if (!str_cmp(arg, "background"))
    file_to_string_alloc(BACKGROUND_FILE, &background);
  else if (!str_cmp(arg, "greetings")) {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
  } else if (!str_cmp(arg, "xhelp")) {
    if (help_table) {
      for (i = 0; i <= top_of_helpt; i++) {
        if (help_table[i].keyword)
	  free(help_table[i].keyword);
        if (help_table[i].entry && !help_table[i].duplicate)
	  free(help_table[i].entry);
      }
      free(help_table);
    }
    top_of_helpt = 0;
    index_boot(DB_BOOT_HLP);
  } else
    if (!str_cmp(arg, "socials"))
     {if (soc_mess_list)
         {for (i = 0; i <= top_of_socialm; i++)
              {if (soc_mess_list[i].char_self)
                  free(soc_mess_list[i].char_self);
               if (soc_mess_list[i].char_self_room)
                  free(soc_mess_list[i].char_self_room);
               if (soc_mess_list[i].others_no_arg)
                  free(soc_mess_list[i].others_no_arg);
	       if (soc_mess_list[i].char_no_arg)
                  free(soc_mess_list[i].char_no_arg);
               if (soc_mess_list[i].char_found)
                  free(soc_mess_list[i].char_found);
               if (soc_mess_list[i].others_found)
                  free(soc_mess_list[i].others_found);
               if (soc_mess_list[i].vict_found)
                  free(soc_mess_list[i].vict_found);
               if (soc_mess_list[i].not_found)
                  free(soc_mess_list[i].not_found);
               }
          free (soc_mess_list);
         }
      if (soc_keys_list)
         {for (i = 0; i <= top_of_socialk; i++)
              if (soc_keys_list[i].keyword)
                  free(soc_keys_list[i].keyword);
          free (soc_keys_list);
         }
      top_of_socialm = -1;
      top_of_socialk = -1;
      index_boot(DB_BOOT_SOCIAL);
     }
     else 
     { send_to_char("Неизвестная опция перезагрузки.\r\n", ch);
       return;
  }

  send_to_char(OK, ch);
}

void init_cheaters(void)
{ 
  FILE *ch_file;
  char name[300];
  struct cheat_list_type *curr_ch, *prev_ch;
  int i;
  
  cheaters_list = NULL;
  prev_ch = NULL;
  
  /* читаем файл */
  if (!(ch_file = fopen(LIB_MISC"cheaters.lst","r"))) {
   log("Can not open cheaters.lst");
   return;
  }
  while (get_line(ch_file,name)) {
    if (!name[0] || name[0] == ';')
      continue;
    /* добавляем имя в список */
    CREATE(curr_ch, struct cheat_list_type, 1);
    CREATE(curr_ch->name, char, strlen(name) + 1);
    for (i = 0, curr_ch->name[i] = '\0'; name[i]; i++)
      curr_ch->name[i] = LOWER(name[i]);
    curr_ch->name[i] = '\0';
    curr_ch->next_name = NULL;
    if (!cheaters_list)
      cheaters_list = curr_ch;
    else
      prev_ch->next_name = curr_ch;

    prev_ch = curr_ch;
  }
  fclose(ch_file);
}

void boot_world(void)
{
  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);
  
  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Count mob qwantity by level");
  mob_lev_count();

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  log("Renumbering zone table.");
  renum_zone_table();

  log("Renumbering Obj_zone.");
  renum_obj_zone();

  log("Renumbering Mob_zone.");
  renum_mob_zone();

  if (!no_specials) {
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }
}

  

/* body of the booting system */
void boot_db(void)
{  struct stat sb;
  zone_rnum i;

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  log("Reading news, credits, help, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  if (stat(NEWS_FILE,&sb) >= 0)
     lastnews = sb.st_mtime;
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(ZONY_FILE, &zony);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
    prune_crlf(GREETINGS);

  log("Loading spell definitions.");
  mag_assign_spells();

  boot_world();

  log("Loading help entries.");
  index_boot(DB_BOOT_HLP);

  log("Loading social entries.");
  index_boot(DB_BOOT_SOCIAL);  

  log("Generating player index.");
  new_build_player_index();

  log("Load Exchange db.");
  load_exchange_db();

  log("Loading fight messages.");
  load_messages();


  log("Assigning function pointers:");

  if (!no_specials) {
    log("   Mobiles.");
    assign_mobiles();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
  }

  log("Assigning spell and skill levels.");
  init_spell_levels();
  
  
  log("Sorting command list and spells.");
  sort_commands();
  sort_spells();

  log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();
  log("Reading proxi site list.");   
  load_proxi();

 /* if (!no_rent_check) {
    log("Deleting timed-out crash and rent files:");
    update_obj_file();
    log("   Done.");
  }*/

  /* Moved here so the object limit code works. -gg 6/24/98 */
  
 
   // Изменено Ладником
  log("Booting rented objects info");
  for (i = 0; i <= top_of_p_table; i++)
      {(player_table + i)->timer=NULL;
       Crash_read_timer(i, FALSE);
       log ("Player %s", (player_table + i)->name);
      }

    log("Booting corpse for room");
       load_corpses();


  for (i = 0; i <= top_of_zone_table; i++)
      {log("Перезагрузка %s (комнаты %d-%d).", zone_table[i].name,
               (i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
       reset_zone(i);
      }

  reset_q.head = reset_q.tail = NULL;

  log("Booting clans.");
  init_clans();
  boot_time = time(0);
 

  log("Booting allowed cheaters");
  init_cheaters();
  
  log("Boot db -- DONE.");
}

extern vector < ClanRec * > clan;
std::map<int, int> m_type_count;
typedef std::map<int, int>::iterator tc_iterator;

void free_social_messg(social_messg* s)
{
    free(s->char_self_room);
    free(s->char_no_arg);
    free(s->others_no_arg);
    free(s->char_found);
    free(s->others_found);
    free(s->vict_found);
    free(s->not_found);
}

void free_social_keyword(social_keyword* s)
{
    free(s->keyword);
}

void free_help_index(help_index_element *h)
{
    free(h->keyword);
    //free(h->entry);
}

void free_obj_data(struct obj_data * obj);
void free_char_data(struct char_data* ch);
void trig_data_free_data(struct trig_data *this_data);

void free_index(struct index_data *index)
{
    free(index->farg);
    if (index->proto)
        trig_data_free_data(index->proto);
}
void free_zone(struct zone_data* zd)
{
    free(zd->aftor);
    free(zd->name);
    free(zd->cmd);
}
void free_room(struct room_data* r)
{
   free(r->name);
   free(r->description);           /* Shown when entered                 */
   extra_descr_data* ed = r->ex_description;
   while (ed) {
      free(ed->description);
      free(ed->keyword);
      extra_descr_data* next = ed->next;
      free (ed);
      ed = next;
   }
   for (int i=0,e=NUM_OF_DIRS;i<e;++i) {
       room_direction_data* dd = r->dir_option[i];
       if (!dd) continue;
       free (dd->general_description);
       free (dd->keyword);
       free (dd->vkeyword);   
       free (dd);
   }
   trig_proto_list *tl = r->proto_script;
   while (tl) {
        trig_proto_list* next = tl->next;
        free(tl);
        tl = next;
   }

    struct script_data     *script;       // script info for the object
    
    /*struct track_data      *track;

    struct obj_data *contents;   // List of items in room
    struct char_data *people;    // List of NPC / PC in room
    ubyte  fires;                // Time when fires        
    ubyte  forbidden;            // Time when room forbidden for mobs
    ubyte  ices;                 // Time when ices restore

    int    portal_room;
    ubyte  portal_time;
    long   id;*/
}

void unload_db()
{
  log("Unloading db");
  while (cheaters_list)
  {
      free(cheaters_list->name);
      cheat_list_type *new_cl = cheaters_list->next_name;
      free(cheaters_list);
      cheaters_list = new_cl;
  }
  for (int i=0,e=clan.size(); i<e; ++i)
      delete clan[i];
  clan.clear();

  tc_iterator tc = m_type_count.find(DB_BOOT_SOCIAL);
  if (tc != m_type_count.end())
  {
      for (int i = 0, e=tc->second; i<e; ++i)
         free_social_messg(&soc_mess_list[i]);
      free(soc_mess_list);
  }  
  tc = m_type_count.find(DB_BOOT_SOCIAL2);
  if (tc != m_type_count.end())
  {
      for (int i = 0, e = tc->second; i < e; ++i)
          free_social_keyword(&soc_keys_list[i]);
      free(soc_keys_list);
  }
  tc = m_type_count.find(DB_BOOT_HLP);
  if (tc != m_type_count.end())
  {
      for (int i = 0, e = tc->second; i < e; ++i)
      {
          help_index_element *h = &help_table[i];       
          free_help_index(h);
      }
      free(help_table);
  }
  tc = m_type_count.find(DB_BOOT_OBJ);
  if (tc != m_type_count.end())
  {
      for (int i = 0, e = tc->second; i < e; ++i)
      {
          free_obj_data(&obj_proto[i]);
          free_index(&obj_index[i]);
      }
      free(obj_proto);
      free(obj_index);
  }
  tc = m_type_count.find(DB_BOOT_MOB);
  if (tc != m_type_count.end())
  {
      for (int i = 0, e = tc->second; i < e; ++i)
      {
          free_char_data(&mob_proto[i]);
          free_index(&mob_index[i]);
      }
      free(mob_proto);
      free(mob_index);
  }
  tc = m_type_count.find(DB_BOOT_ZON);
  if (tc != m_type_count.end())
  {
      for (int i = 0, e = tc->second; i < e; ++i)
      {
          free_zone(&zone_table[i]);
      }
      free(zone_table);
  }
  tc = m_type_count.find(DB_BOOT_WLD);
  if (tc != m_type_count.end())
  {
      for (int i = 0, e = tc->second; i < e; ++i)
      {
          free_room(&world[i]);
      }
      free(world);
  }

}



/* reset the time in the game from file */
void reset_time(void)
{
  time_info = *mud_time_passed(time(0), beginning_of_time);

 // Calculate moon day
  weather_info.moon_day      = ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % MOON_CYCLE;
  weather_info.week_day_mono = ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % WEEK_CYCLE;
  weather_info.week_day_poly = ((time_info.year * MONTHS_PER_YEAR + time_info.month) * DAYS_PER_MONTH + time_info.day) % POLY_WEEK_CYCLE;
  // Calculate Easter
  calc_easter();
  calc_god_celebrate();

  if (time_info.hours <  sunrise[time_info.month][0])
     weather_info.sunlight = SUN_DARK;
  else
  if (time_info.hours == sunrise[time_info.month][0])
     weather_info.sunlight = SUN_RISE;
  else
  if (time_info.hours <  sunrise[time_info.month][1])
     weather_info.sunlight = SUN_LIGHT;
  else
  if (time_info.hours == sunrise[time_info.month][1])
     weather_info.sunlight = SUN_SET;
  else
     weather_info.sunlight = SUN_DARK;

  log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
          time_info.day, time_info.month, time_info.year);
        
  weather_info.temperature = (year_temp[time_info.month].med * 4 +
                              number(year_temp[time_info.month].min,year_temp[time_info.month].max)) / 5;
  weather_info.pressure = 960;
  if ((time_info.month >= MONTH_MAY) && (time_info.month <= MONTH_NOVEMBER))
     weather_info.pressure += dice(1, 50);
  else
     weather_info.pressure += dice(1, 80);

  weather_info.change       = 0;
  weather_info.weather_type = 0;

  if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_MAY)
     weather_info.season = SEASON_SPRING;
  else
  if (time_info.month == MONTH_MART && weather_info.temperature >= 3)
     weather_info.season = SEASON_SPRING;
  else
  if (time_info.month >= MONTH_JUNE  && time_info.month <= MONTH_AUGUST)
     weather_info.season = SEASON_SUMMER;
  else
  if (time_info.month >= MONTH_SEPTEMBER && time_info.month <= MONTH_OCTOBER)
     weather_info.season = SEASON_AUTUMN;
  else
  if (time_info.month == MONTH_NOVEMBER && weather_info.temperature >= 3)
     weather_info.season = SEASON_AUTUMN;
  else
     weather_info.season = SEASON_WINTER;

  if (weather_info.pressure <= 980)
     weather_info.sky = SKY_LIGHTNING;
  else
  if (weather_info.pressure <= 1000)
     {weather_info.sky = SKY_RAINING;
      if (time_info.month >= MONTH_APRIL && time_info.month <= MONTH_OCTOBER)
         create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 40, 40, 20);
      else
      if (time_info.month >= MONTH_DECEMBER || time_info.month <= MONTH_FEBRUARY)
         create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 50, 40, 10);
      else
      if (time_info.month == MONTH_NOVEMBER || time_info.month == MONTH_MART)
         {if (weather_info.temperature >= 3)
             create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTRAIN, 70, 30, 0);
          else
             create_rainsnow(&weather_info.weather_type, WEATHER_LIGHTSNOW, 80, 20, 0);
         }
     }
  else
  if (weather_info.pressure <= 1020)
     weather_info.sky = SKY_CLOUDY;
  else
     weather_info.sky = SKY_CLOUDLESS;
}
  



/* generate index table for the player file */

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$') {
    /* skip the text */
    do {
      get_one_line(fl, line);
      if (feof(fl))
	goto ackeof;
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do {
      scan = one_word(scan, next_key);
      if (*next_key)
        ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      goto ackeof;
  }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof:	
  log("SYSERR: Unexpected end of help file.");
  exit(1);	/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}


int count_social_records(FILE *fl, int *messages, int *keywords)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$')
        {/* skip the text */
         do {get_one_line(fl, line);
             if (feof(fl))
                    goto ackeof;
            } while (*line != '#');

         /* now count keywords */
         scan = key;
         ++(*messages);
         do {scan = one_word(scan, next_key);
             if (*next_key)
                ++(*keywords);
            } while (*next_key);

         /* get next keyword line (or $) */
         get_one_line(fl, key);

         if (feof(fl))
            goto ackeof;
        }

  return (TRUE);

  /* No, they are not evil. -gg 6/24/98 */
ackeof: 
  log("SYSERR: Unexpected end of socials file.");
  exit(1);      /* Some day we hope to handle these things better... */
}





//разобраться с влдешником
void index_boot(int mode)
{
  const char *index_filename, *prefix = NULL;	/* NULL or egcs 1.1 complains */
  FILE *index, *db_file;
  int rec_count = 0, size[2], count;

  switch (mode) {
  case DB_BOOT_TRG:
    prefix = TRG_PREFIX;
    break;
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  case DB_BOOT_SOCIAL:
    prefix = SOC_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand %d to index_boot!", mode);
    exit(1);
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  sprintf(buf2, "%s%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
	  index_filename, strerror(errno));
      fscanf(index, "%s\n", buf1);
      continue;
    } else {
      if (mode == DB_BOOT_ZON)
	rec_count++;
      else 
      if (mode == DB_BOOT_SOCIAL)
           rec_count += count_social_records(db_file, &top_of_socialm, &top_of_socialk);
        else
        if (mode == DB_BOOT_HLP)
           rec_count += count_alias_records(db_file);
        else
        if (mode == DB_BOOT_WLD)
           {
            count = count_hash_records(db_file);
            if (count > 100)
               {
                 log("WARN: File '%s, %d' list more than 100 rooms",buf2, count);
               }
            rec_count += (count+1);
           }
        else
           rec_count += count_hash_records(db_file);
       }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (!rec_count) {
    if (mode == DB_BOOT_SHP)
      return;
    log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
	index_filename);
    exit(1);
  }

  /* Any idea why you put this here Jeremy? */
  rec_count++;

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  switch (mode) {
  case DB_BOOT_TRG:
    CREATE(trig_index, struct index_data *, rec_count);
    break;
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    size[0] = sizeof(struct room_data) * rec_count;
    log("   %d rooms, %d bytes.", rec_count, size[0]);
    m_type_count[mode] = rec_count;
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct char_data) * rec_count;
    log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    m_type_count[mode] = rec_count;
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct obj_data) * rec_count;
    log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    m_type_count[mode] = rec_count;
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    size[0] = sizeof(struct zone_data) * rec_count;
    log("   %d zones, %d bytes.", rec_count, size[0]);
    m_type_count[mode] = rec_count;
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    size[0] = sizeof(struct help_index_element) * rec_count;
    log("   %d entries, %d bytes.", rec_count, size[0]);
    for (int i=0;i<rec_count;++i) {
        help_index_element &h = help_table[i];
        h.entry = NULL; h.keyword = NULL;
    }
    m_type_count[mode] = rec_count;
    break;
  case DB_BOOT_SOCIAL:
    CREATE(soc_mess_list, struct social_messg, top_of_socialm+1);
    CREATE(soc_keys_list, struct social_keyword, top_of_socialk+1);
    size[0] = sizeof(struct social_messg) * (top_of_socialm+1);
    size[1] = sizeof(struct social_keyword) * (top_of_socialk+1);
    log("   %d entries(%d keywords), %d(%d) bytes.", top_of_socialm+1,
        top_of_socialk+1, size[0], size[1]);
    m_type_count[DB_BOOT_SOCIAL] = top_of_socialm+1;
    m_type_count[DB_BOOT_SOCIAL2] = top_of_socialk+1;
  }

  rewind(index);
 
  fscanf(index, "%s\n", buf1);

  while (*buf1 != '$') {
    sprintf(buf2, "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      log("SYSERR: %s: %s", buf2, strerror(errno));
      exit(1);
    }
    switch (mode) {
    case DB_BOOT_TRG:
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
      discrete_load(db_file, mode, buf2);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      /*
       * If you think about it, we have a race here.  Although, this is the
       * "point-the-gun-at-your-own-foot" type of race.
       */
      load_help(db_file);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
   case DB_BOOT_SOCIAL:
      load_socials(db_file);
      break;  
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  fclose(index);

  /* sort the help index */
  if (mode == DB_BOOT_HLP) {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
    top_of_helpt--;
  }
  	 // sort the social index
  if (mode == DB_BOOT_SOCIAL)
     qsort(soc_keys_list, top_of_socialk+1, sizeof(struct social_keyword), csort);
}


void discrete_load(FILE * fl, int mode, char *filename)
{
  int nr = -1, last;
  char line[256];

  const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg", "qst"};

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
	if (nr == -1) {
	  log("SYSERR: %s file %s is empty!", modes[mode], filename);
	} else {
	  log("SYSERR: Format error in %s after %s #%d\n"
	      "...expecting a new %s, but file ended!\n"
	      "(maybe the file is not terminated with '$'?)", filename,
	      modes[mode], nr, modes[mode]);
	}
	exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	log("SYSERR: Format error after %s #%d", modes[mode], last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	switch (mode) {
    case DB_BOOT_TRG:
      parse_trigger(fl, nr);
      break;
	case DB_BOOT_WLD:
	  parse_room(fl, nr);
	  break;
	case DB_BOOT_MOB:
	  parse_mobile(fl, nr);
	   break;
	case DB_BOOT_OBJ:
	  strcpy(line, parse_object(fl, nr));
	  break;
	case DB_BOOT_QST:
   //    parse_quest(fl, nr);
      break;
	  }
    } else {
      log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
	  filename, modes[mode], nr);
      log("SYSERR: ... offending line: '%s'", line);
      exit(1);
    }
  }
}


void asciiflag_conv1(char *flag, void *value)
{
  int       *flags  = (int *) value;
  int is_number     = 1, block = 0, i;
  register char *p;

  for (p = flag; *p; p += i+1)
      {i = 0;
       if (islower((unsigned char)*p))
          {if (*(p+1) >= '0' && *(p+1) <= '9')
              {block = (int) *(p+1) - '0';
               i     = 1;
              }
           else
              block = 0;
           *(flags+block) |= (0x3FFFFFFF & (1 << (*p - 'a')));
          }
       else
       if (isupper((unsigned char)*p))
          {if (*(p+1) >= '0' && *(p+1) <= '9')
              {block = (int) *(p+1) - '0';
               i     = 1;
              }
           else
              block  = 0;
           *(flags+block) |=  (0x3FFFFFFF & (1 << (26 + (*p - 'A'))));
          }
       if (!isdigit((unsigned char)*p))
          is_number = 0;
      }

  if (is_number)
	{is_number = atol(flag);
      block     = is_number < INT_ONE ? 0 : is_number < INT_TWO ? 1 : is_number < INT_THREE ? 2 : 3;
      *(flags+block) = is_number & 0x3FFFFFFF;
	} 
}




bitvector_t asciiflag_conv(char *flag)
{
  bitvector_t flags = 0;
  int is_number = 1;
  register char *p;

  for (p = flag; *p; p++) {
    if (islower((unsigned char)*p))
      flags |= 1 << (*p - 'a');
    else if (isupper((unsigned char)*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit((unsigned char)*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return (flags);
}

char fread_letter(FILE *fp)
{
  char c;
  do {c = getc(fp);
     } while (is_space(c));
  return c;
}

/* load the rooms */
void parse_room(FILE * fl, int virtual_nr)
{
  static int room_nr = FIRST_ROOM, zone = 0;
  int t[10], i;
  char line[256], flags[128];
  struct extra_descr_data *new_descr;
  char letter;

  sprintf(buf2, "room #%d", virtual_nr);

  if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
    log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      log("SYSERR: Room %d is outside of any zone.", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

      world[room_nr].room_flags.flags[0]  = 0;
      world[room_nr].room_flags.flags[1]  = 0;
      world[room_nr].room_flags.flags[2]  = 0;
      world[room_nr].room_flags.flags[3]  = 0;

  if (!get_line(fl, line)) {
    log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
	virtual_nr);
    exit(1);
  }

  if (sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
    log("SYSERR: Format error in roomflags/sector type of room #%d",
	virtual_nr);
    exit(1);
  }
  /* t[0] is the zone number; ignored with the zone-file system */
//  world[room_nr].room_flags = asciiflag_conv(flags);
 asciiflag_conv1(flags, &world[room_nr].room_flags);
  world[room_nr].sector_type = t[2];

  world[room_nr].func		= NULL;
  world[room_nr].contents	= NULL;
  world[room_nr].people		= NULL;
  world[room_nr].track      	= NULL;
  world[room_nr].light		= 0;	// Zero light sources
  world[room_nr].fires      	= 0;
  world[room_nr].gdark      	= 0;
  world[room_nr].glight     	= 0;
  world[room_nr].forbidden  	= 0;
  world[room_nr].proto_script	= NULL;

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

//  sprintf(buf,"SYSERR: Format error in room #%d (expecting D/E/S)",virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      log(buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
 
	case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = NULL;
      new_descr->description = NULL;
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      if (new_descr->keyword && new_descr->description)
      {
        new_descr->next = world[room_nr].ex_description;
        world[room_nr].ex_description = new_descr;
      }
      else
      {
        sprintf(buf,"SYSERR: Format error in room #%d (Corrupt extradesc)",virtual_nr);
        log(buf);
        free(new_descr);
      }
      break;

	case 'S':			/* end of room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter=='T')
            {dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
             letter = fread_letter(fl);
             ungetc(letter, fl);
            }
		top_of_world = room_nr++;
      return;
    default:
      log(buf);
      exit(1);
    }
  }
}



 

// read direction data
void setup_dir(FILE * fl, int room, int dir)
{
  int t[5];
  char line[256];

  sprintf(buf2, "room #%d, direction D%d", GET_ROOM_VNUM(room), dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  //world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  char *alias = fread_string(fl, buf2);
	if (alias && *alias)
	{	std::string buffer(alias);
		std::string::size_type i = buffer.find('|');
		if (i != std::string::npos)
			{ world[room].dir_option[dir]->keyword = str_dup(buffer.substr(0,i).c_str());
			  world[room].dir_option[dir]->vkeyword = str_dup(buffer.substr(++i).c_str());
			}
		else
			{ world[room].dir_option[dir]->keyword = str_dup(buffer.c_str());
			  world[room].dir_option[dir]->vkeyword = str_dup(buffer.c_str());
			}
	}
	free(alias);

  if (!get_line(fl, line)) {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else
    world[room].dir_option[dir]->exit_info = 0;

  if (t[0] == 4)
    world[room].dir_option[dir]->exit_info |= EX_HIDDEN;

  world[room].dir_option[dir]->key		= t[1];
  world[room].dir_option[dir]->to_room  = t[2];
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
    if ((r_helled_start_room = real_room(helled_start_room)) < 0)
     {if (!mini_mud)
         log("SYSERR:  Warning: Hell start room does not exist.  Change in config.c.");
      r_helled_start_room = r_mortal_start_room;
     }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
	}
  if ((r_named_start_room = real_room(named_start_room)) < 0)
     {if (!mini_mud)
         log("SYSERR:  Warning: NAME start room does not exist.  Change in config.c.");
      r_named_start_room = r_mortal_start_room;
     }
  if ((r_unreg_start_room = real_room(unreg_start_room)) < 0)
    {if (!mini_mud)
         log("SYSERR:  Warning: UNREG start room does not exist.  Change in config.c.");
      r_unreg_start_room = r_mortal_start_room;
     }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  register int room, door;

  for (room = FIRST_ROOM; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door])
	if (world[room].dir_option[door]->to_room != NOWHERE)
	  world[room].dir_option[door]->to_room =
	    real_room(world[room].dir_option[door]->to_room);
}


// Установка принадлежности к зоне в прототипах 
void renum_obj_zone( void )
{
   int i;
   for ( i = 0; i <= top_of_objt; ++i )
   {
      obj_index[i].zone = real_zone(obj_index[i].vnum);
   }
}

// Установкапринадлежности к зоне в индексе 
void renum_mob_zone( void )
{
   int i;
   for ( i = 0; i <= top_of_mobt; ++i )
   {
      mob_index[i].zone = real_zone(mob_index[i].vnum);
   }
}

#define ZCMD zone_table[zone].cmd[cmd_no]
#define ZCMD_CMD(cmd_nom) zone_table[zone].cmd[cmd_nom]
/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
  int cmd_no, a, b, c, olda, oldb, oldc;
  zone_rnum zone;
  char buf[128];

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = c = 0;
      olda = ZCMD.arg1;
      oldb = ZCMD.arg2;
      oldc = ZCMD.arg3;
      switch (ZCMD.command) {
      case 'M':
	a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
	c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
	  case 'Q':
    a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
    break;
      case 'F':
	    a = ZCMD.arg1 = real_room(ZCMD.arg1);
        b = ZCMD.arg2 = real_mobile(ZCMD.arg2);
        c = ZCMD.arg3 = real_mobile(ZCMD.arg3);
	break;
	  case 'O':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	if (ZCMD.arg3 != NOWHERE)
	  c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'G':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'E':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'P':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	c = ZCMD.arg3 = real_object(ZCMD.arg3);
	break;
      case 'D':
	a = ZCMD.arg1 = real_room(ZCMD.arg1);
	break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
	b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      case 'T': /* a trigger */
       /* designer's choice: convert this later */
   // b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
      b = real_trigger(ZCMD.arg2); /* leave this in for validation */
      break;
      case 'V': /* trigger variable assignment */
    if (ZCMD.arg1 == WLD_TRIGGER)
       b = ZCMD.arg2 = real_room(ZCMD.arg2);
      break;
	  }
      if (a < 0 || b < 0 || c < 0) {
	if (!mini_mud) {
	  sprintf(buf,  "Invalid vnum %d, cmd disabled",
			 (a < 0) ? olda : ((b < 0) ? oldb : oldc));
	  log_zone_error(zone, cmd_no, buf);
	}
	ZCMD.command = '*';
      }
    }
}



void parse_simple_mob(FILE *mob_f, int i, int nr)
{
  int j, t[10];
  char line[256];

  mob_proto[i].real_abils.str = 11;
  mob_proto[i].real_abils.intel = 11;
  mob_proto[i].real_abils.wis = 11;
  mob_proto[i].real_abils.dex = 11;
  mob_proto[i].real_abils.con = 11;
  mob_proto[i].real_abils.cha = 11;

  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    exit(1);
  }

  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
	  t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    log("SYSERR: Format error in mob #%d, first line after S flag\n"
	"...expecting line of form '# # # #d#+# #d#+#'", nr);
    exit(1);
  }

  GET_LEVEL(mob_proto + i)        	= t[0];
  mob_proto[i].real_abils.hitroll = 20 - t[1] + MAX(0, t[0]*2/3);
  mob_proto[i].real_abils.armor   = 10 * t[2];
//разобраться (впрочем все ясно, но если что не забыть заглянуть)
  /* max hit = 0 is a flag that H, M, V is xdy+z */
  //gold = dice(t[3], t[4]);
  mob_proto[i].points.max_hit  		= 0;
  mob_proto[i].ManaMemNeeded   		= t[3];
  mob_proto[i].ManaMemStored   		= t[4];
  mob_proto[i].points.hit      		= t[5];

  mob_proto[i].points.move     		= 100;
  mob_proto[i].points.max_move 		= 100;
    

  mob_proto[i].mob_specials.damnodice	= t[6];
  mob_proto[i].mob_specials.damsizedice = t[7];
  mob_proto[i].real_abils.damroll	= t[8];

  if (!get_line(mob_f, line)) {
      log("SYSERR: Format error in mob #%d, second line after S flag\n"
	  "...expecting line of form '# #', but file ended!", nr);
      exit(1);
    }

  if (sscanf(line, " %dd%d+%d %d ", t, t + 1, t + 2, t + 3) != 4) {
    log("SYSERR: Format error in mob #%d, second line after S flag\n"
	"...expecting line of form '# #'", nr);
    exit(1);
  }
 
  GET_GOLD(mob_proto + i)	 = t[2] + dice(t[0], t[1]);
  GET_EXP(mob_proto + i)	 = t[3];

  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # #', but file ended!", nr);
    exit(1);
  }

  /*if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) != 3) {
    log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # #'", nr);
    exit(1);
  }*/

  switch (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3)) {
	case 3:
		mob_proto[i].mob_specials.speed = -1;
		break;
	case 4:
		mob_proto[i].mob_specials.speed = t[3];
		break;
	default:
		log("SYSERR: Format error in 3th line of mob #%d\n" "...expecting line of form '# # # #'", nr);
		exit(1);
	}

  mob_proto[i].char_specials.position   	= t[0];
  mob_proto[i].mob_specials.default_pos 	= t[1];
  mob_proto[i].player.sex			= t[2];

  mob_proto[i].player.chclass			= 0;
  mob_proto[i].player.weight			= 200;
  mob_proto[i].player.height			= 198;

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 6; j++)
    GET_SAVE(mob_proto + i, j) = 0;
}


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, char *value, int i, int nr)
{
  struct helper_data_type *helper;
  int num_arg, matched = 0;
  int t[7];
  char *ptr, orig[1024];

  ptr = any_one_arg(value, orig);
  num_arg = atoi(orig);


  CASE("Resistances") {
		if (sscanf(value, "%d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6) != 7) {
			log("SYSERROR : Excepted format <# # # # # # #> for RESISTANCES in MOB #%d", i);
			return;
		}
		for (int k = 0; k < MAX_NUMBER_RESISTANCE; k++)
				GET_RESIST(mob_proto + i, k) = MIN(300, MAX(-1000, t[k]));
	}

	CASE("Saves") {
		if (sscanf(value, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
			log("SYSERROR : Excepted format <# # # #> for SAVES in MOB #%d", i);
			return;
		}
		for (int k = 0; k < SAVING_COUNT; k++)
			GET_SAVE(mob_proto + i, k) = MIN(200, MAX(-200, t[k]));
	}

	CASE("HPreg")
	{	RANGE(-200, 200);
		mob_proto[i].add_abils.hitreg = num_arg;
	}

	CASE("Armour")
	{	RANGE(0, 100);
		mob_proto[i].add_abils.armour = num_arg;
	}

	CASE("PlusMem")
	{	RANGE(-200, 200);
		mob_proto[i].add_abils.manareg = num_arg;
	}

	CASE("CastSuccess")
	{	RANGE(-200, 200);
		mob_proto[i].add_abils.cast_success = num_arg;
	}

	CASE("Success")
	{	RANGE(-200, 200);
		mob_proto[i].add_abils.morale_add = num_arg;
	}

	CASE("Initiative")
	{	RANGE(-200, 200);
		mob_proto[i].add_abils.initiative_add = num_arg;
	}

	CASE("Absorbe")
	{	RANGE(-200, 200);
		mob_proto[i].add_abils.absorb = num_arg;
	}
	CASE("AResist")
	{	RANGE(0, 100);
		mob_proto[i].add_abils.a_resist = num_arg;//сопротивлячемость магическим афффектам
	}
	CASE("MResist")
	{ 	RANGE(0, 100);
		mob_proto[i].add_abils.d_resist = num_arg;//сопротивлячемость магическому дамагу
	}
 
  CASE("Destination")
  { if (mob_proto[i].mob_specials.dest_count < MAX_DEST)
       {mob_proto[i].mob_specials.dest[mob_proto[i].mob_specials.dest_count] = num_arg;
        mob_proto[i].mob_specials.dest_count++;
       }
  }

  CASE("LikeWork") {
	  RANGE(0,100);
	  mob_proto[i].mob_specials.like_work = num_arg;  //Вероятность использования умений моба
  }

  CASE("MaxFactor") {
	  RANGE(0, 255);
	  mob_proto[i].mob_specials.max_factor = num_arg; //Количество убитых мобов до замаксивания
  }

  CASE("ExtraAttack") {
	  RANGE(0,10);
	  mob_proto[i].mob_specials.extra_attack = num_arg; //Кол-во дополнительных ударов
  }

  CASE("BareHandAttack") {
    RANGE(0, 99);
    mob_proto[i].mob_specials.attack_type = num_arg;
  }
  
  CASE("Special_Bitvector") {
     asciiflag_conv1((char *)value, &mob_proto[i].mob_specials.Npc_Flags);
  }

  CASE("Str") {
    RANGE(3, 50);
    mob_proto[i].real_abils.str = num_arg;
  }

  
  CASE("Int") {
    RANGE(3, 50);
    mob_proto[i].real_abils.intel = num_arg;
  }

  CASE("Wis") {
    RANGE(3, 50);
    mob_proto[i].real_abils.wis = num_arg;
  }

  CASE("Dex") {
    RANGE(3, 50);
    mob_proto[i].real_abils.dex = num_arg;
  }

  CASE("Con") {
    RANGE(3, 50);
    mob_proto[i].real_abils.con = num_arg;
  }

  CASE("Cha") {
    RANGE(3, 50);
    mob_proto[i].real_abils.cha = num_arg;
  }

  CASE("Class") {
    RANGE(0, 120);
	  mob_proto[i].player.chclass = num_arg - 100;
	 }
  
  CASE("Height") {
    RANGE(0, 300);
     mob_proto[i].player.height = num_arg;
	 }
//вес
  CASE("Weight") {
	RANGE(0, 300);
     mob_proto[i].player.weight = num_arg;
	 }
// Рассы в редакторе не реализованы, пока не используется!!!
  CASE("Race") {
    RANGE(0, 15);
     mob_proto[i].player.race = num_arg;
	 }

  CASE("Size") {
    RANGE(1, 100);
     mob_proto[i].real_abils.size = num_arg;
	 }
  CASE("Spell") {
  if (sscanf(value, "%d", t+0) != 1)
      {log("SYSERROR : Excepted format <#> for SPELL in MOB #%d",i);
       return;
      }
   if (t[0] > MAX_SPELLS && t[0] < 1)
      {log("SYSERROR : Unknown spell No %d for MOB #%d",t[0],i);
       return;
      }
   GET_SPELL_MEM(mob_proto+i,t[0]) += 1;
   GET_CASTER(mob_proto+i) += (IS_SET(spell_info[t[0]].routines,NPC_CALCULATE) ? 1 : 0);

  
  }
  CASE("Skill") {
	RANGE(130, 200);
	  mob_proto[i].real_abils.Skills[num_arg] = atoi(ptr);
}
  
 CASE("Helper")
  { CREATE(helper, struct helper_data_type, 1);
    helper->mob_vnum = num_arg;
    helper->next_helper = GET_HELPER(mob_proto+i);
    GET_HELPER(mob_proto+i) = helper;
  }

  if (!matched) {
    log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
	    keyword, nr);
  }    
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (is_space(*ptr))
      ptr++;
#if 0	/* Need to evaluate interpret_espec()'s NULL handling. */
  }
#else
  } else
    ptr = "";
#endif
    interpret_espec(buf, ptr, i, nr);
}


void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
  char line[256];
 
  parse_simple_mob(mob_f, i, nr);

  while (get_line(mob_f, line)) {
     if (!strcmp(line, "E")) /* end of the enhanced section */
      return;
     else if (*line == '#') {  /* we've hit the next mob, maybe? */
      log("SYSERR: Unterminated E section in mob #%d", nr);
      exit(1);
    } else 
      parse_espec(line, i, nr);
  }

  log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  exit(1);
}


void parse_mobile(FILE * mob_f, int nr)
{
  static int i = 0;
  int j, t[10], c;
  char line[256], letter, *ptr = NULL;
  char f1[128], f2[128];
  char buf[EXDSCR_LENGTH];
  char buf2[EXDSCR_LENGTH];

  mob_index[i].vnum = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  *buf = '\0';
  *buf2 = '\0';
  /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */
  mob_proto[i].player_specials = &dummy_mob;
    
  /***** String data *****/
  mob_proto[i].player.name = fread_string(mob_f, buf2);
  mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
  mob_proto[i].player.rname = fread_string(mob_f, buf2);
  mob_proto[i].player.dname = fread_string(mob_f, buf2);
  mob_proto[i].player.vname = fread_string(mob_f, buf2);
  mob_proto[i].player.tname = fread_string(mob_f, buf2);
  mob_proto[i].player.pname = fread_string(mob_f, buf2);
  mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
 // mob_proto[i].player.description = fread_string(mob_f, buf2);   //заремил
  mob_proto[i].player.title = NULL;
  mob_proto[i].mob_specials.Questor = NULL;
  mob_proto[i].player.description = NULL;//добавил

  ptr = fread_string(mob_f, buf2);
 
  if (ptr)
  strcpy(buf, ptr);
  if ((ptr = strchr(buf, '@')) != NULL)	
	{ ++ptr;
	  skip_spaces(&ptr); 
      mob_proto[i].mob_specials.Questor = str_dup(ptr);
	  --ptr;
	  
	  *ptr = '\0';
	 j = strlen(buf);
	  for (c=0; c <= j; c++)
	  {  
		  if (buf[c]!= '@')
		   buf2[c] = buf[c];   
		  else
		  { buf2[c] = '\0';
			  break;			  
		  }
	  }
		  mob_proto[i].player.description = str_dup(buf2);
	}
   else 
    mob_proto[i].player.description = str_dup(buf);
   

  /*  if (ptr)
  strcpy(buf, ptr);
  if ((ptr = strchr(buf, '@')) != NULL)	
	{ ++ptr;
	  skip_spaces(&ptr); 
      mob_proto[i].mob_specials.Questor = str_dup(ptr);
	  --ptr;
	  *ptr = '\0';
      mob_proto[i].player.description = str_dup(buf);
	}
   else 
    mob_proto[i].player.description = str_dup(buf);
    
   */

 /* *** Numeric data *** */
  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}', but file ended!", nr);
    exit(1);
  }

#ifdef CIRCLE_ACORN	/* Ugh. */
  if (sscanf(line, "%s %s %d %s", f1, f2, t + 2, &letter) != 4) {
#else
  if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4) {
#endif
    log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}'", nr);
    exit(1);
  }
    asciiflag_conv1(f1, &MOB_FLAGS(mob_proto + i, 0));
    SET_BIT(MOB_FLAGS(mob_proto + i, MOB_ISNPC), MOB_ISNPC);
    asciiflag_conv1(f2, &AFF_FLAGS(mob_proto + i, 0));
    GET_ALIGNMENT(mob_proto + i) = t[2];

  switch (UPPER((unsigned)letter)) {
  case 'S':	// Simple monsters
    parse_simple_mob(mob_f, i, nr);
    break;
  case 'E':	// Circle3 Enhanced monsters
    parse_enhanced_mob(mob_f, i, nr);
    break;
  // add new mob types here..
  default:
    log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
    exit(1);
  }

  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter=='T')
        {dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
         letter = fread_letter(mob_f);
         ungetc(letter, mob_f);
        }
  
//  mob_proto[i].aff_abils = mob_proto[i].real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  top_of_mobt = i++;
  
}

#define SEVEN_DAYS 60*24*30
/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr)
{
  static int i = 0;
  static char line[256];
  int t[10], j, retval;
  char *tmpptr;
  char f1[256], f2[256], f[256];
  struct extra_descr_data *new_descr;

  obj_index[i].vnum		   = nr;
  obj_index[i].number	   = 0;
  obj_index[i].stored      = 0;
  obj_index[i].func		   = NULL;
  
  clear_object(obj_proto + i);
  obj_proto[i].item_number = i;

  // Доработать разобрать нужно будет инициализацию при необходимости
  obj_proto[i].obj_flags.tren_skill 	= 0;
  obj_proto[i].obj_flags.cred_point	= 100;
  obj_proto[i].obj_flags.material	= 0;
  obj_proto[i].obj_flags.timer		= SEVEN_DAYS; 
  obj_proto[i].obj_flags.Obj_destroyer 	= 60;
  obj_proto[i].obj_flags.spell		= 0;
  obj_proto[i].obj_flags.level_spell	= 1;
  obj_proto[i].obj_flags.affects	= clear_flags; 
  obj_proto[i].obj_flags.no_flag    	= clear_flags;
  obj_proto[i].obj_flags.anti_flag	= clear_flags;
  obj_proto[i].obj_flags.pol	    	= 1;
  

  sprintf(buf2, "object #%d", nr);

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  obj_proto[i].short_description =  fread_string(obj_f, buf2);
  obj_proto[i].short_rdescription = fread_string(obj_f, buf2);
  obj_proto[i].short_ddescription = fread_string(obj_f, buf2);
  obj_proto[i].short_vdescription = fread_string(obj_f, buf2);
  obj_proto[i].short_tdescription = fread_string(obj_f, buf2);
  obj_proto[i].short_pdescription = fread_string(obj_f, buf2);
  
  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    CAP(tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }
    if ((retval = sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    log("SYSERR: Format error in first numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
	}                /* ПЕРВАЯ ПОЗИЦИЯ*/
  obj_proto[i].obj_flags.tren_skill 	= t[0];	  /*тренируемый скилл*/
  obj_proto[i].obj_flags.cred_point 	= t[1];   /*сколько выдержит ударов*/
  obj_proto[i].obj_flags.ostalos	= t[2];   /*сколько осталось выдержать*/
  obj_proto[i].obj_flags.material	= t[3];   /*материал из которого сделан предмет*/
  
 if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }
   if ((retval = sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    log("SYSERR: Format error in first numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
   }                       /* ВТОРАЯ*/
  obj_proto[i].obj_flags.pol		= t[0];		 			// Пол предмета  
  obj_proto[i].obj_flags.timer		= t[1] > 0 ? t[1] : SEVEN_DAYS;      	// Таймер предмета
  obj_proto[i].obj_flags.spell		= t[2];		 			// Спелл, кастуемый предметом
  obj_proto[i].obj_flags.level_spell	= t[3];		 			// Уровень кастуемого спелла. Для книг - сложность заучивания
 
 if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  if ((retval = sscanf(line, " %s %s %s", f, f1, f2)) != 3) {
    log("SYSERR: Format error in first numeric line (expecting 3 args, got %d), %s", retval, buf2);
    exit(1);
  }                 
  	// ТРЕТЬЯ  Ща тут правильно!!! больше не проверять!!!
  asciiflag_conv1(f, &obj_proto[i].obj_flags.affects);  		// аффекты при одевании
  asciiflag_conv1(f1, &obj_proto[i].obj_flags.no_flag);			// нельзя одеть
  asciiflag_conv1(f2, &obj_proto[i].obj_flags.anti_flag);		// нельзя взять



   if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  if ((retval = sscanf(line, " %d %s %s", t, f1, f2)) != 3) {
    log("SYSERR: Format error in first numeric line (expecting 3 args, got %d), %s", retval, buf2);
    exit(1);
  }         
  					// ЧЕТВЕРТАЯ
  obj_proto[i].obj_flags.type_flag	= t[0];
  asciiflag_conv1(f1, &obj_proto[i].obj_flags.extra_flags);
  obj_proto[i].obj_flags.wear_flags 	= asciiflag_conv(f2);
                  
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    log("SYSERR: Format error in second numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
  }						/* ПЯТАЯ*/
  obj_proto[i].obj_flags.value[0] = t[0]; /*что за предмет, магический, клерик*/
  //asciiflag_conv1(f1,&obj_proto[i].obj_flags.value); 
  obj_proto[i].obj_flags.value[1] = t[1]; /*Действия свитков, наименьший уровень*/
  obj_proto[i].obj_flags.value[2] = t[2]; /* номер спелла заклинания*/
  obj_proto[i].obj_flags.value[3] = t[3]; /*уровень*/

  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    log("SYSERR: Format error in third numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
  }                    /* ШЕСТАЯ*/
  obj_proto[i].obj_flags.weight		  = t[0];
  obj_proto[i].obj_flags.cost		  = t[1];
  obj_proto[i].obj_flags.cost_per_day = t[2];
  obj_proto[i].obj_flags.cost_per_inv = t[3];
 
  /* check to make sure that weight of containers exceeds curr. quantity */
  if (obj_proto[i].obj_flags.type_flag == ITEM_DRINKCON ||
      obj_proto[i].obj_flags.type_flag == ITEM_FOUNTAIN) {
    if (obj_proto[i].obj_flags.weight < obj_proto[i].obj_flags.value[1])
      obj_proto[i].obj_flags.weight = obj_proto[i].obj_flags.value[1] + 5;
  }

  /* *** extra descriptions and affect fields ****/

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  strcat(buf2, ", after numeric constants\n"
	 "...expecting 'E', 'A', '$', or next object number");
  j = 0;

  for (;;) {
    if (!get_line(obj_f, line)) {
      log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
  //  switch (*line) {
   /* case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = obj_proto[i].ex_description;
      obj_proto[i].ex_description = new_descr;
      break;
*/

/*      case 'E':
          CREATE (new_descr, EXTRA_DESCR_DATA, 1);
	  new_descr->keyword = NULL;
	  new_descr->description = NULL;
	  new_descr->keyword = fread_string (obj_f, buf2);
	  new_descr->description = fread_string (obj_f, buf2);
	  if (new_descr->keyword && new_descr->description)
	    {
	      new_descr->next = obj_proto[i].ex_description;
	      obj_proto[i].ex_description = new_descr;
	    }
	  else
	    {
	      sprintf (buf, "SYSERR: Format error in %s (Corrupt extradesc)",
		       buf2);
	      log (buf);
	      free (new_descr);
	    }
	  break;
    
      case 'A':
      if (j >= MAX_OBJ_AFFECT) {
	log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
	exit(1);
      }
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }

      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
      }
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      j++;
      break;
     case 'T':  // DG triggers 
      dg_obj_trigger(line, &obj_proto[i]);
     break;
	
	case '$':
    case '#':
      check_object(&obj_proto[i]);
      top_of_objt = i++;
      return (line);
    default:
      log("SYSERR: Format error in %s", buf2);
      exit(1);
    }*/
       switch (*line)
	{
	case 'E':
	  CREATE (new_descr, EXTRA_DESCR_DATA, 1);
	  new_descr->keyword = NULL;
	  new_descr->description = NULL;
	  new_descr->keyword = fread_string (obj_f, buf2);
	  new_descr->description = fread_string (obj_f, buf2);
	  if (new_descr->keyword && new_descr->description)
	    {
	      new_descr->next = obj_proto[i].ex_description;
	      obj_proto[i].ex_description = new_descr;
	    }
	  else
	    {
	      sprintf (buf, "SYSERR: Format error in %s (Corrupt extradesc)",
		       buf2);
	      log (buf);
	      free (new_descr);
	    }
	  break;
	case 'A':
	  if (j >= MAX_OBJ_AFFECT)
	    {
	      log ("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT,
		   buf2);
	      exit (1);
	    }
	  if (!get_line (obj_f, line))
	    {
	      log ("SYSERR: Format error in 'A' field, %s\n"
		   "...expecting 2 numeric constants but file ended!", buf2);
	      exit (1);
	    }
	  if ((retval = sscanf (line, " %d %d ", t, t + 1)) != 2)
	    {
	      log ("SYSERR: Format error in 'A' field, %s\n"
		   "...expecting 2 numeric arguments, got %d\n"
		   "...offending line: '%s'", buf2, retval, line);
	      exit (1);
	    }
	  obj_proto[i].affected[j].location = t[0];
	  obj_proto[i].affected[j].modifier = t[1];
	  j++;
	  break;
	case 'T':		/* DG triggers */
	  dg_obj_trigger (line, &obj_proto[i]);
	  break;
	case 'M':
	  GET_OBJ_MIW (&obj_proto[i]) = atoi (line + 1);
	  break;

	case '$':
	case '#':
	  check_object (&obj_proto[i]);
	  top_of_objt = i++;
	  return (line);
	default:
	  log ("SYSERR: Format error in %s", buf2);
	  exit (1);
	}

  }
}


 

#define Z	zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
  static zone_rnum zone = 0;
  int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, arg_num=0;
  char *ptr, buf[256], zname[256];
  char t1[80], t2[80];
  
  strcpy(zname, zonename);

  while (get_line(fl, buf))
    num_of_cmds++;	
  rewind(fl);

  if (num_of_cmds == 0) {
    log("SYSERR: %s is empty!", zname);
    exit(1);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if (sscanf(buf, "#%hd", &Z.number) != 1)
    { log("SYSERR: Ошибка формата в %s, на линии %d", zname, line_num);
      exit(1);
    }

  sprintf(buf2, "загрузка зоны #%d", Z.number);

  line_num += get_line(fl, buf);

  if ((ptr = strchr(buf, '/')) != NULL)
      *ptr = '\0';
  else
    if ((ptr = strchr(buf, '~')) != NULL)
          *ptr = '\0';



  Z.name = str_dup(buf);


  line_num += get_line(fl, buf);
   if (sscanf(buf, "#%hd", &Z.level) != 1) {
    log("SYSERR: Неверный уровень зоны в %s, на линии %d", zname, line_num);
    exit(1);
  }
   

  line_num += get_line(fl, buf);

  if (sscanf(buf, " %d %d %d ", &Z.top, &Z.lifespan, &Z.reset_mode) != 3)
    { log("SYSERR: Ошибка формата, должно быть 3 поля в %s", zname);
      exit(1);
    }

  cmd_no = 0;

  for (;;) {
    if ((tmp = get_line(fl, buf)) == 0)
    { log("SYSERR: Ошибка формата в %s - преждевременный конец файла", zname);
      exit(1);
    }

    line_num += tmp;

    ptr = buf;

    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;
     ptr++;

    if (ZCMD.command == 'S' || ZCMD.command == '$') {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
      ZCMD.arg4 = -1;
      if (strchr("MOEGPDTVQF", ZCMD.command) == NULL)
         {/* a 3-arg command */
          if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
                 error = 1;
         }
      else
      if (ZCMD.command=='V')
         {/* a string-arg command */
          if (sscanf(ptr, " %d %d %d %d %s %s", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                             &ZCMD.arg3, t1, t2) != 6)
                 error = 1;
          else
            {ZCMD.sarg1 = str_dup(t1);
             ZCMD.sarg2 = str_dup(t2);
            }
         }
      else
      if (ZCMD.command=='Q')
         {/* a number command */
          if (sscanf(ptr, " %d %d", &tmp, &ZCMD.arg1) != 2)
             error = 1;
          else
             tmp   = 0;
         }
      else
         {if (sscanf(ptr, " %d %d %d %d %d", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                             &ZCMD.arg3, &ZCMD.arg4) < 4)
              error = 1;
         }

    	if (tmp)
	   ZCMD.if_flag = true;
	else
	   ZCMD.if_flag = false;

    if (error) {
      log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  top_of_zone_table = zone++;
}

#undef Z


void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == NULL) {
    log("SYSERR: ошибка чтения файла помощи: нет окончания терминаторас $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}


void load_help(FILE *fl)
{
#if defined(CIRCLE_MACINTOSH)
  static char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384]; /* ? */
#else
  char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384];
#endif
  char line[READ_SIZE+1], *scan;
  struct help_index_element el;

  /* get the first keyword line */
  get_one_line(fl, key);
  while (*key != '$') {
    /* read in the corresponding help entry */
    strcpy(entry, strcat(key, "\r\n"));
    get_one_line(fl, line);
    while (*line != '#') {
      strcat(entry, strcat(line, "\r\n"));
      get_one_line(fl, line);
    }

    /* now, add the entry to the index with each keyword on the keyword line */
    el.duplicate = 0;
    el.entry = str_dup(entry);
    scan = one_word(key, next_key);
    while (*next_key) {
      el.keyword = str_dup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }

    /* get next keyword line (or $) */
    get_one_line(fl, key);
  }
}


int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}

int csort(const void *a, const void *b)
{
  const struct social_keyword *a1, *b1;

  a1 = (const struct social_keyword *) a;
  b1 = (const struct social_keyword *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}



/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/



int vnum_mobile(char *searchname, struct char_data * ch)
{ int nr, found = 0;  for (nr = 0; nr <= top_of_mobt; nr++)
  { if (isname(searchname, mob_proto[nr].player.name))
	{  sprintf(buf, "%3d. [%5d] %-25s L: [%2d]  Hp: [%5d] Dam: [%2d] At: [%2dd%-2d] M: [%2d] G: [%d]\r\n", ++found,
                    mob_index[nr].vnum,
                    mob_proto[nr].player.short_descr,
		    mob_proto[nr].player.level,
		    mob_proto[nr].points.hit,
		    mob_proto[nr].real_abils.damroll +
		    str_app[mob_proto[nr].real_abils.str +
		    mob_proto[nr].add_abils.str_add].todam,
		    mob_proto[nr].mob_specials.damnodice,
		    mob_proto[nr].mob_specials.damsizedice,
		    mob_proto[nr].mob_specials.max_factor,
		    mob_proto[nr].points.gold);
          send_to_char(buf, ch); 
       }
  }
return (found);
}


int vnum_affect(char *searchname, struct char_data * ch) 
{ int nr, found = 0;
  char temp[MAX_INPUT_LENGTH];
  *buf = '\0';
    	
	for (nr = 0; nr <= top_of_objt; nr++)
	{ sprintbits(obj_proto[nr].obj_flags.affects, weapon_affects, temp, ", ");
	//	if (str_cmp(temp,"НЕ УСТАНОВЛЕН "))
		if (isname(searchname, temp))
		{ sprintf(buf + strlen(buf), "&G%3d. [%5d] %-35s %s&n\r\n", ++found, obj_index[nr].vnum,
                  obj_proto[nr].short_description, temp);	
			if (found > 100)
			  { strcat(buf, "\r\n\r\n");
			    send_to_char(buf, ch);
			    *buf = '\0';
			    found = 0;
                         }
		}
			
	 }
   
   send_to_char(buf, ch);

 return (found);
}






int vnum_search(char *searchname, struct char_data * ch) 
{ int nr, found = 0, i;
  char temp[MAX_INPUT_LENGTH];
  *buf = '\0';
    	for (nr = 0; nr <= top_of_objt; nr++)
		 { for (i = 0; i < MAX_OBJ_AFFECT; i++)
			{ sprinttype(obj_proto[nr].affected[i].location, apply_types, temp);
		 		if (isname(searchname, temp))
				{ sprintf(buf + strlen(buf), "&G%3d. [%5d] %-35s %s %d&n\r\n", ++found, obj_index[nr].vnum,
                  obj_proto[nr].short_description, searchname,  obj_proto[nr].affected[i].modifier);	
					if (found > 100)
					{  strcat(buf, "\r\n\r\n");
					   send_to_char(buf, ch);
					  *buf = '\0';
					   found = 0;


					}
				}
			}
		 }
   
   send_to_char(buf, ch);
 return (found);
}
int vnum_object(char *searchname, struct char_data * ch)
{  int nr, found = 0, drndice, drsdice, i = 0;
   bool key; 
 
 for (nr = 0; nr <= top_of_objt; nr++)
  { if (isname(searchname, obj_proto[nr].name))
	{  	sprintf(buf, "%3d. [%5d] %-30s ", ++found, obj_index[nr].vnum,
                     obj_proto[nr].short_description);

		switch (obj_proto[nr].obj_flags.type_flag)
			{ case ITEM_WEAPON:
	            sprintf(buf + strlen(buf), "Повреждение: [%2dd%-2d]  Вес: [%-2d]\r\n",
               	                obj_proto[nr].obj_flags.value[1],
				                obj_proto[nr].obj_flags.value[2],
                                obj_proto[nr].obj_flags.weight);
			
				sprintbits(obj_proto[nr].obj_flags.affects, weapon_affects, buf1, ", ");
				if (str_cmp(buf1,"ничего"))
				{ sprintf(buf + strlen(buf), "&C%3d. [%5d] %-30s %s&n\r\n", found,
                  obj_index[nr].vnum,
                  obj_proto[nr].short_description, buf1);
				}
				break;
			  case ITEM_ARMOR:
                sprintf(buf + strlen(buf), "Защита (АС): [%2d] Броня: (ARMOUR) [%d]  Вес: [%-2d]\r\n",
				                obj_proto[nr].obj_flags.value[0],
				                obj_proto[nr].obj_flags.value[1],
                                obj_proto[nr].obj_flags.weight);
				
				sprintbits(obj_proto[nr].obj_flags.affects, weapon_affects, buf1, ", ");
				if (str_cmp(buf1,"ничего"))
				{ sprintf(buf + strlen(buf), "&C%3d. [%5d] %-30s %s&n\r\n", found,
                  obj_index[nr].vnum,
                  obj_proto[nr].short_description, buf1);
				}
				break;
			  default:
			    sprintf(buf + strlen(buf), "Защита (АС): [%2d] Броня: (ARMOUR) [%d]  Вес: [%-2d]\r\n",
				                obj_proto[nr].obj_flags.value[0],
				                obj_proto[nr].obj_flags.value[1],
                                obj_proto[nr].obj_flags.weight);
				
			    sprintbits(obj_proto[nr].obj_flags.affects, weapon_affects, buf1, ", ");
				if (str_cmp(buf1,"ничего"))
				{ sprintf(buf + strlen(buf), "&C%3d. [%5d] %-30s %s&n", found,
                  obj_index[nr].vnum,
                  obj_proto[nr].short_description, buf1);
				}
				strcat(buf, "\r\n");
				  break;
  			}
         send_to_char(buf, ch);
		 key = found > 0 ? 1 : 0;

		  found = FALSE;
	*buf = '\0';

	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{ drndice = obj_proto[nr].affected[i].location;
	  drsdice = obj_proto[nr].affected[i].modifier;
		if ((drndice != APPLY_NONE) && (drsdice != 0))
		{ if (!found)
			{ send_to_char("Дополнительные свойства :\r\n", ch);
			  found = TRUE;
			}
			sprinttype(drndice, apply_types, buf2);
			int negative = 0;
			for (int j = 0; *apply_negative[j] != '\n'; j++)
				if (!str_cmp(buf2, apply_negative[j]))
				{ negative = TRUE;
				  break;
				}
			switch (negative)
			{ case FALSE:
			     if (obj_proto[nr].affected[i].modifier < 0)
					      negative = TRUE;
				 break;
			case TRUE:
				if (obj_proto[nr].affected[i].modifier < 0)
					     negative = FALSE;
				break;
			}
			sprintf(buf  + strlen(buf), "&G%3d. [%5d]%-30s %s %d&n\r\n",found, obj_index[nr].vnum,
				buf2, negative ? " &Rухудшает на&W " : " &Cулучшает на&W ",
				abs(obj_proto[nr].affected[i].modifier));
			
		}
	}
  send_to_char(buf, ch);
    } 
 } 
 return (key);
}

// создаем персонажа и добавляем в чарЛист.
struct char_data *create_char(void)
{
  struct char_data *ch;
  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;
  GET_ID(ch) = max_id++;

  return (ch);
}

// create a new mobile from a prototype 
struct char_data *read_mobile(mob_vnum nr, int type)
{ int is_corpse = 0;
  mob_rnum i;
  struct char_data *mob;

   if (nr < 0)
     { is_corpse = 1;
        nr = -nr;
     }
   
  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) < 0) {
      log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return (NULL);
    }
  } else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob			= mob_proto[i];
  mob->proto_script 	= NULL;
  mob->next		= character_list;
  character_list 	= mob;

  if (!mob->points.max_hit) {
    mob->points.max_hit = dice(GET_MANA_NEED(mob),GET_MANA_STORED(mob)) +
                                 mob->points.hit;
  } else
    mob->points.max_hit = number(mob->points.hit, GET_MANA_NEED(mob));

  mob->points.hit		  = mob->points.max_hit;
  GET_MANA_NEED(mob)	  = GET_MANA_STORED(mob) = 0;
  GET_HORSESTATE(mob)	  = 200;
  GET_LASTROOM(mob)	  = NOWHERE;
  GET_PFILEPOS(mob)	  = -1;
  //GET_ACTIVITY(mob)	  = number(0,PULSE_MOBILE-1);

  if (mob->mob_specials.speed <= -1)
		GET_ACTIVITY(mob) = number(0, PULSE_MOBILE - 1);
  else
		GET_ACTIVITY(mob) = number(0, mob->mob_specials.speed);


  EXTRACT_TIMER(mob)	  = 0;
  mob->points.move	  = mob->points.max_move;
    
  mob_index[i].number++;
  GET_ID(mob) = max_id++;
    if (!is_corpse)
  assign_triggers(mob, MOB_TRIGGER);
  return (mob);
}


//  создаем объект и добавляем к общему списку объектов
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next		= object_list;
  object_list		= obj;
  GET_ID(obj)           = max_id++;
  GET_OBJ_ZONE(obj)     = NOWHERE;
  OBJ_GET_LASTROOM(obj) = NOWHERE;
  //assign_triggers(obj, OBJ_TRIGGER);  ибо креш
  return (obj);
}

void set_obj_aff(struct obj_data *itemobj, int bitv)
{
	int i;

	for (i = 0; weapon_affect[i].aff_bitvector != -1; i++)
	{ if (weapon_affect[i].aff_bitvector == bitv)
		{ SET_BIT(GET_OBJ_AFF(itemobj, weapon_affect[i].aff_pos), weapon_affect[i].aff_pos);
			break;
		}
	}
}

void set_obj_eff(struct obj_data *itemobj, int type, int mod)
{ int i;

	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (itemobj->affected[i].location == type)
		 { itemobj->affected[i].modifier += mod;
			break;
		 }
		else
		if (itemobj->affected[i].location == APPLY_NONE)
		 { itemobj->affected[i].location = type;
		   itemobj->affected[i].modifier = mod;
			break;
		 }
}

// возвращаемое значение истина - умножение, в противном случае - сумма.
bool ToIncreaseOrPlus (int rnd)
{ bool result = false;

	switch (ObjRand[rnd].m_AppleAff)
	{ case APPLY_STR: 
	  case APPLY_DEX: 
	  case APPLY_INT: 
	  case APPLY_WIS:
	  case APPLY_CON:
	  case APPLY_CHA:
	  case APPLY_HITROLL:
	  case APPLY_DAMROLL:
	  case APPLY_CAST_SUCCESS:
	   result = true;
	   break;
	  default:
	   break;
	}
return result;
}

// создаем новый объект из прототипа (опытного образца (эталон)) перенести в линух 03.04.2007г.
struct obj_data *read_object(obj_vnum nr, int type)
{
  OBJ_DATA *obj;
  obj_rnum i;
 
  if (nr < 0) {
    log("SYSERR: Trying to create obj with negative (%d) num!", nr);
    return (NULL);
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      log("Объект (V) %d такого предмета нет в базе данных.", nr);
      return (NULL);
    }
  } else
    i = nr;

  CREATE(obj, OBJ_DATA, 1);
  clear_object(obj);
  *obj			= obj_proto[i];
  obj->next		= object_list;
  object_list		= obj;
  GET_OBJ_ZONE(obj)     = NOWHERE;
  OBJ_GET_LASTROOM(obj) = NOWHERE;
  GET_ID(obj)           = max_id++;
  obj_index[i].number++;

 
  //Здесь написана часть функции связанная с рандомной генерацией предметов в зонах.
  //GET_OBJ_CUR - здесь передается уровень, который используется для расчета генерации характеристик
  // тут же буду присваивать новое значение GET_OBJ_CUR, которое будет равно GET_OBJ_MAX
//так же здесь сделать проверку на уровень зоны, если score будет выше максимально допустимого
//значения, то есть у нас макс для уровня предмета должно стоять 50 уровень,
//если у нас будет значение GET_OBJ_CUR(obj) выше 50, то в редакторе неверно присвоено значение
//уровню рандомной генерации вещи, взять в расчет общий уровень зоны.Пока ставлю принудительное ограничение
// 50
// так же, учесть ситуацию, если у нас предмет может браться как в прайм так и в офф, какой при такой
//ситуации брать коэффициент?:??


		if ( IS_OBJ_STAT(obj, ITEM_RANDLOAD) && !type)
			{ int rnd, tip, score = MIN(GET_OBJ_CUR(obj), 50) + 10;//очки, полученные от уровня предмета плюс константа 10
              float koeff = 1.0;
			  bool start = true;
			  ubyte count = 0;

			do
			{ rnd = number(0, 42);
		     switch (GET_OBJ_TYPE(obj))
			 { case  ITEM_WEAPON: 
				if ( CAN_WEAR(obj, ITEM_WEAR_WIELD) )
					      koeff = 1.5;
				if ( CAN_WEAR(obj, ITEM_WEAR_HOLD) )	
					      koeff = 2;
				if ( (tip = static_cast<int>(ObjRand[rnd].m_base * koeff)) <= score)
					{ start = false;
					  GET_OBJ_VAL(obj, 1) = ObjRand[rnd].m_A;//присваиваем новый дамаг,полученный в результате обработки
					  GET_OBJ_VAL(obj, 2) = ObjRand[rnd].m_B;
					  GET_OBJ_WEIGHT(obj) = ObjRand[rnd].m_weight;
					  score -= tip; 
					  score += number(0, 10);// даем бонус, если оружие
					}
				break;

			   case  ITEM_ARMOR:
					int armor, ac;
					armor	= number(10, 40);
					tip	= armor - 10;
					ac	= number(10, MIN(40, score - 10));
					if (tip <= score)
					{ start = false;
					  GET_OBJ_VAL(obj, 0) = ac;
					  GET_OBJ_VAL(obj, 1) = armor;
					  score -= tip;
					  score += number(0, 20);//даем бонус, если броня
					}
				break;
					default:
			    break;
				}
			}
			while (start);

		 score = MAX(5, MIN(60, score)); // оставшееся количество очков,но не более 60, не менее 5, а то
						 // при типе 2, если меньше 5 - вечный цикл (мин. данные из таблицы).

		

		rnd = number(1, 100);//выбираем тип генерации, если 50 и выше то 1 тип, если 16 до 50 2 тип, если 1 до 15 3 тип.


		if (rnd >= 50)
			tip = 1;
		else
		if (rnd > 15)
			tip = 2;
		else
		if (rnd >= 1)
			tip = 3;

		switch(tip)
		{ case 1:
		// рандом 0 - 56 - выбирается номер аффекта из массива,
		// если номер аффекта от 1 до 23 проверяется хватает ли очков (3-й столбик) на этот аффект
		//		если очков хватает - аффект (1-й столбик) присваивается, если нет - генерация прекращается,
		//		никаких аффектов более не вешается
		// если номер аффекта от 25 до 56 - выбирается случайно сила воздействия аффекта, проверяется его стоимость
		//      и если хватает очков то он присваивается, если не хватает - генерация прекращается
				if ((rnd = number(0, 56)) < 23)
				{ if (ObjRand[rnd].m_Cost >= score)
					{ set_obj_aff (obj, ObjRand[rnd].m_AppleAff);
					  break;
					}
				}
				 else 
				  if (ObjRand[rnd].m_Cost >= score)
				   	  set_obj_eff (obj, ObjRand[rnd].m_AppleAff, ObjRand[rnd].m_Add);
							
		 	break;
		
		  case 2:
		// рандом 1-56 - выбирается номер аффекта из массива,
		// если количество очков на этот аффект (3-й столбик) нехватает, то рандом кидается вновь
		// если количество очков на этот аффект хватает то:
		//		если номер аффекта от 1 до 24 то он присваивается и выход из генерации
		//		если номер аффекта от 25 до 56 то вычисляется максимально возможное значение, укладывающиеся в эти очки
		//		и аффект данной силы присваивается и выход
//ToIncreaseOrPlus(rnd)
			  do 
			   rnd = number(0, 56);
			  
			  while (ObjRand[rnd].m_Cost > score);

			  if (rnd < 23)
				  set_obj_aff (obj, ObjRand[rnd].m_AppleAff);
			  else 
				{ set_obj_eff (obj, ObjRand[rnd].m_AppleAff, ObjRand[rnd].m_Add);
			      if (ToIncreaseOrPlus(rnd)) 
				  { if ((ObjRand[rnd].m_Cost * ObjRand[rnd].m_Coeff) <= score)
				   set_obj_eff (obj, ObjRand[rnd].m_AppleAff, ObjRand[rnd].m_Add);
				  }
			      else
				   if ((ObjRand[rnd].m_Cost + ObjRand[rnd].m_Coeff) <= score)
				  set_obj_eff (obj, ObjRand[rnd].m_AppleAff, ObjRand[rnd].m_Add);
		
				}
			break;
			
	      case 3:
		// label: рандом 1-56 - выбирается номер аффекта из массива,
		//		если количество очков на этот аффект (3-й столбик) нехватает, то выход из генерации
		//		если количество очков на аффект хватает то аффект присваивается (в случае аффектов 25-56 сила выбирается рандомно)
		// если оставшееся количество очков меньше 5 то выход из генерации,
		// если оставшееся количество очков больше 5 то запускается цикл
		// label1:  bonus = number (0,1)
		//			if (bonus) 
        //				{ очки += bonus
		//				идем на label1} 
        //		идем на label (не больше трех раз)
			
			while (ObjRand[rnd = number(0, 56)].m_Cost < score)
			{ if (rnd < 23)
				  set_obj_aff (obj, ObjRand[rnd].m_AppleAff);
			  else
				  set_obj_eff (obj, ObjRand[rnd].m_AppleAff, ObjRand[rnd].m_Add);
			  score -= ObjRand[rnd].m_Cost;

			   if (score < 5)
				   break;

			   while (number(0, 1))
			          score ++;
                 
			   if (count ++ > 3)
			       break;
			   
			} 

			break;
		
		default:
			break;
		
		}

	  GET_OBJ_RLVL(obj)= GET_OBJ_CUR(obj);
	  GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);
	  REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_RANDLOAD), ITEM_RANDLOAD);//снимаю флаг ранддомага, он уже больше не нужен.
	
	} //end randoms obj

	
 
	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
		{ name_from_drinkcon(obj);
			if (GET_OBJ_VAL(obj,1) && GET_OBJ_VAL(obj,2))
				name_to_drinkcon(obj, GET_OBJ_VAL(obj,2));
		}
  
  assign_triggers(obj, OBJ_TRIGGER);
  return (obj);
}

#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i=0;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;
    
// jelson 10/22/92

  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    // since one minute has passed, increment zone ages

    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
	  zone_table[i].reset_mode)
	(zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
	  zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
	/* enqueue zone */

	CREATE(update_u, struct reset_q_element, 1);

	update_u->zone_to_reset = i;
	update_u->next = 0;

	if (!reset_q.head)
	  reset_q.head = reset_q.tail = update_u;
	else {
	  reset_q.tail->next = update_u;
	  reset_q.tail = update_u;
	}

	zone_table[i].age = ZO_DEAD;
      }
    }
  }	// end - one minute has passed


  // dequeue zones (if possible) and reset 
  // this code is executed every 10 seconds (i.e. PULSE_ZONE)
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2  ||
        is_empty(update_u->zone_to_reset))
	{      reset_zone(update_u->zone_to_reset);
                sprintf(buf, "Репоп зоны %d. Название зоны: %s",
	        zone_table[update_u->zone_to_reset].number,
	        zone_table[update_u->zone_to_reset].name);
                mudlog(buf, CMP, LVL_GOD, FALSE);
      // dequeue 
      if (update_u == reset_q.head)
	reset_q.head = reset_q.head->next;
      else {
	for (temp = reset_q.head; temp->next != update_u;
	     temp = temp->next);

	if (!update_u->next)
	  reset_q.tail = temp;

	temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
  char buf[256];

  sprintf(buf, "SYSERR: zone file: %s", message);
  mudlog(buf, NRM, LVL_GOD, TRUE);

  sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	  ZCMD.command, zone_table[zone].number, ZCMD.line);
  mudlog(buf, NRM, LVL_GOD, TRUE);
}

/*#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }
*/

#define ZONE_ERROR(message) \
        { log_zone_error(zone, cmd_no, message); }

/* execute the reset command table of a given zone */

void reset_zone(zone_rnum zone)
{
  int cmd_no, last_cmd = 0, nr = 0, i;
  int mob_load = FALSE; 
  int obj_load = FALSE;
  int room_vnum, room_rnum;
  struct char_data *mob = NULL, *leader=NULL, *ch;
  struct obj_data *obj, *obj_to;
  struct char_data *tmob=NULL; // for trigger assignment 
  struct obj_data *tobj=NULL;  // for trigger assignment 


  ItemDecayZrepop(zone); //сдикеем вещь с флагом зонресет.

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) 
    {i = 0;
     
      if (ZCMD.if_flag && !last_cmd && !mob_load && !obj_load)
           continue;
     
	if (!ZCMD.if_flag) { /* ### */
       mob_load = FALSE;
       obj_load = FALSE;
     }


    switch (ZCMD.command) {
    case '*':			// игнорируем команду  
      last_cmd = 0;
      break;

    case 'M':			// зачитаем моба 
	mob = NULL;
     	if (mob_index[ZCMD.arg1].number < ZCMD.arg2 &&
	   (ZCMD.arg4 < 0 || mobs_in_room(ZCMD.arg1,ZCMD.arg3) < ZCMD.arg4)) {
		   mob = read_mobile(ZCMD.arg1, REAL);
		    char_to_room(mob, ZCMD.arg3);
			 last_cmd = 1;
               load_mtrigger(mob);
             tmob = mob;
	     mob_load = TRUE;
      } else
	last_cmd = 0;
	tobj = NULL;
      break;

     case 'F':           //Мобы, которые следуют за хозяином
      last_cmd = 0;
      leader   = NULL;
      if (ZCMD.arg1 >= FIRST_ROOM && ZCMD.arg1 <= top_of_world)
         {for (ch = world[ZCMD.arg1].people; ch && !leader; ch = ch->next_in_room)
              if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg2)
                 leader = ch;
          for (ch = world[ZCMD.arg1].people; ch && leader; ch = ch->next_in_room)
              if (IS_NPC(ch)                    &&
                  GET_MOB_RNUM(ch) == ZCMD.arg3 &&
                  leader     != ch              &&
                  ch->master != leader
                 )
                 {if (ch->master)
                     stop_follower(ch, SF_EMPTY);
                     add_follower(ch, leader);
                  last_cmd = 1;
                 }
         }
      break;

    case 'Q':                   // удалим всех мобов
      last_cmd = 0;
      for (ch = character_list; ch; ch=leader)
          {leader = ch->next;
           if (IS_NPC(ch) && GET_MOB_RNUM(ch) == ZCMD.arg1)
              {extract_mob(ch);
               last_cmd = 1;
              }
          }
      tobj = NULL;
      tmob = NULL;
      break;

	  
	case 'O':	// зачитаем объект
	if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored < ZCMD.arg2 &&
          (ZCMD.arg4 < 0 || number(1, 100) <= ZCMD.arg4))
		{ obj = read_object(ZCMD.arg1, REAL);
		  if (ZCMD.arg3 >= 0)
		  { GET_OBJ_ZONE(obj) = world[ZCMD.arg3].zone;
		    obj_to_room(obj, ZCMD.arg3);
		    load_otrigger(obj);
            	  }
		  else
		    IN_ROOM(obj) = NOWHERE;
					   
		   tobj = obj;
		   last_cmd = 1;
		   obj_load = TRUE;
			
			if (!OBJ_FLAGGED(obj, ITEM_NODECAY) && !OBJ_FLAGGED(obj, ITEM_SKIPLOG))
			{ sprintf(buf, "WARN: В комнату загружен объект без флага NODECAY : %s (VNUM = %d)",
		 	  obj->short_description, GET_OBJ_VNUM(obj));
			  mudlog(buf, BRF, LVL_BUILDER, TRUE);
			}
		}
	else
	 last_cmd = 0;
    
	tmob = NULL;
      break;

    case 'P':			// загрузим объект в объект
      if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored < ZCMD.arg2 &&
          (ZCMD.arg4 < 0 || number(1, 100) <= ZCMD.arg4)) {
	
	if (!(obj_to = get_obj_num(ZCMD.arg3))) {
				ZONE_ERROR("target obj not found, command disabled");
				ZCMD.command = '*';
				break;
			 }
    if (GET_OBJ_TYPE(obj_to) != ITEM_CONTAINER) {
                ZONE_ERROR("attempt put obj to non container, omited");
                ZCMD.command = '*';
                break;
             }
			obj = read_object(ZCMD.arg1, REAL);
	   	 if (obj_to->in_room != NOWHERE)
             GET_OBJ_ZONE(obj) = world[obj_to->in_room].zone;
    else if (obj_to->worn_by)
             GET_OBJ_ZONE(obj) = world[IN_ROOM(obj_to->worn_by)].zone;
    else if (obj_to->carried_by)
             GET_OBJ_ZONE(obj) = world[IN_ROOM(obj_to->carried_by)].zone;
				obj_to_obj(obj, obj_to);
	            load_otrigger(obj);
                tobj = obj; 
				last_cmd = 1;
      } else
	last_cmd = 0;
    tmob = NULL;
      break;

    case 'G':			// загрузим объект в инвентарь моба
      if (!mob) 
	break;
     
      if ((obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored < ZCMD.arg2) &&
     (number(1,100) <= ZCMD.arg3))
	  {
	// разобраться
    obj = read_object(ZCMD.arg1, REAL);
    obj_to_char(obj, mob);
	GET_OBJ_ZONE(obj) = world[IN_ROOM(mob)].zone;
	tobj = obj;
    load_otrigger(obj);
	last_cmd = 1;
      } else
	last_cmd = 0;
    tmob = NULL;
      break;

    case 'E':			// загрузка обьъектов в инвентарь моба
      if (!mob) 
	break;
     
   
      if (obj_index[ZCMD.arg1].number + obj_index[ZCMD.arg1].stored < ZCMD.arg2 &&
          (ZCMD.arg4 <= 0 || number(1, 100) <= ZCMD.arg4)) {
	    if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  GET_OBJ_ZONE(obj) = world[IN_ROOM(mob)].zone;
	  IN_ROOM(obj) = IN_ROOM(mob);
	  load_otrigger(obj);
              if (wear_otrigger(obj, mob, ZCMD.arg3))
                {IN_ROOM(obj) = NOWHERE;
				  equip_char(mob, obj, ZCMD.arg3); 
				}
			  else
				 obj_to_char(obj, mob);
              if (!(obj->carried_by == mob) && !(obj->worn_by == mob))
                 {extract_obj(obj);
                  tobj     = NULL; 
				  last_cmd = 0;
				}
			  else 
				{
				last_cmd = 0;
			    tobj = obj;
				}
			}
		}	
		else
			last_cmd = 0;
			tmob = NULL;
			break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL) {
        obj_from_room(obj);
        extract_obj(obj);
      }
      last_cmd	= 1;
      tmob		= NULL;
      tobj		= NULL;
      break;


    case 'D':			/* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
	  (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
	ZONE_ERROR("door does not exist, command disabled");
	ZCMD.command = '*';
      } else
	switch (ZCMD.arg3) {
	case 0:
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_CLOSED);
          break;
        case 1:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_CLOSED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          break;
        case 2:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_CLOSED);
          break;
        case 3:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_HIDDEN);
          break;        
        case 4: 
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_HIDDEN);
          break;
         }
      last_cmd = 1;
      tmob = NULL;
      tobj = NULL;
       break;
     
	case 'T': // trigger command; details to be filled in later 
      if (ZCMD.arg1==MOB_TRIGGER && tmob)
         {if (!SCRIPT(tmob))
             CREATE(SCRIPT(tmob), struct script_data, 1);
          add_trigger(SCRIPT(tmob), read_trigger(real_trigger(ZCMD.arg2)), -1);
          last_cmd = 1;
         }
      else
      if (ZCMD.arg1==OBJ_TRIGGER && tobj)
         {if (!SCRIPT(tobj))
             CREATE(SCRIPT(tobj), struct script_data, 1);
          add_trigger(SCRIPT(tobj), read_trigger(real_trigger(ZCMD.arg2)), -1);
          last_cmd = 1;
         }
      break;

    case 'V':
      if (ZCMD.arg1==MOB_TRIGGER && tmob)
         {if (!SCRIPT(tmob))
             {ZONE_ERROR("Attempt to give variable to scriptless mobile");
             }
          else
             add_var_cntx(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                                                   ZCMD.arg3);
          last_cmd = 1;
         }
      else
      if (ZCMD.arg1==OBJ_TRIGGER && tobj)
         {if (!SCRIPT(tobj))
             {ZONE_ERROR("Attempt to give variable to scriptless object");
             }
          else
             add_var_cntx(&(SCRIPT(tobj)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                                                   ZCMD.arg3);
          last_cmd = 1;
         }
      else
      if (ZCMD.arg1 == WLD_TRIGGER)
         {if (ZCMD.arg2 < FIRST_ROOM || ZCMD.arg2 > top_of_world)
             {ZONE_ERROR("Invalid room number in variable assignment");
             }
          else
             {if (!(world[ZCMD.arg2].script))
                 {ZONE_ERROR("Attempt to give variable to scriptless object");
                 }
              else
                 add_var_cntx(&(world[ZCMD.arg2].script->global_vars),
                           ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
              last_cmd = 1;
             }
         }
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
    }
  }

  zone_table[zone].age = 0;

/* handle reset_wtrigger's */
 
  room_vnum = zone_table[zone].number * 100;
  while (room_vnum <= zone_table[zone].top)
        {room_rnum = real_room(room_vnum);
         if (room_rnum != NOWHERE)
		 {// log("Triggers rnum %d", room_vnum);
            reset_wtrigger(&world[room_rnum]);
         }
			room_vnum++;
        }

  paste_mobiles(zone);
  
}


void paste_mobiles(int zone)
{struct char_data *ch,  *ch_next;
 struct obj_data  *obj, *obj_next;//, *pop, *pop_next;
 int    time_ok, month_ok, need_move, no_month, no_time, room = -1;

 for (ch = character_list; ch; ch = ch_next)
     {ch_next = ch->next;
      if (!IS_NPC(ch))
         continue;
      if (FIGHTING(ch))
         continue;
      if (GET_POS(ch) < POS_STUNNED)
         continue;
      if (AFF_FLAGGED(ch,AFF_CHARM) ||
          AFF_FLAGGED(ch,AFF_HORSE) ||
          AFF_FLAGGED(ch,AFF_HOLD)	||
		  AFF_FLAGGED(ch,AFF_HOLDALL))
         continue;
      if (MOB_FLAGGED(ch, MOB_CORPSE))
         continue;
      if ((room = IN_ROOM(ch)) == NOWHERE)
         continue;
      if (zone >= 0 && world[room].zone != zone)
         continue;
        
      time_ok  = FALSE;
      month_ok = FALSE;
      need_move= FALSE;
      no_month = TRUE;
      no_time  = TRUE;

      if (MOB_FLAGGED(ch,MOB_LIKE_DAY))
         {if (weather_info.sunlight == SUN_RISE ||
              weather_info.sunlight == SUN_LIGHT
             )
             time_ok = TRUE;
           need_move = TRUE;
           no_time   = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_NIGHT))
         {if (weather_info.sunlight == SUN_SET ||
              weather_info.sunlight == SUN_DARK
             )
               time_ok = TRUE;
             need_move = TRUE;
             no_time   = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_FULLMOON))
         {if ((weather_info.sunlight == SUN_SET ||
               weather_info.sunlight == SUN_DARK) &&
              (weather_info.moon_day >= 12 &&
               weather_info.moon_day <= 15)
             )
               time_ok = TRUE;
             need_move = TRUE;
             no_time   = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_WINTER))
         {if (weather_info.season == SEASON_WINTER)
              month_ok = TRUE;
             need_move = TRUE;
             no_month  = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_SPRING))
         {if (weather_info.season == SEASON_SPRING)
              month_ok = TRUE;
             need_move = TRUE;
             no_month  = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_SUMMER))
         {if (weather_info.season == SEASON_SUMMER)
              month_ok = TRUE;
             need_move = TRUE;
             no_month  = FALSE;
         }
      if (MOB_FLAGGED(ch,MOB_LIKE_AUTUMN))
         {if (weather_info.season == SEASON_AUTUMN)
              month_ok = TRUE;
             need_move = TRUE;
             no_month  = FALSE;
         }
      if (need_move)
         {month_ok |= no_month;
          time_ok  |= no_time;
          if (month_ok && time_ok)
             {if (world[room].number != zone_table[world[room].zone].top)
                 continue;
              if (GET_LASTROOM(ch) == NOWHERE)
                 {extract_mob(ch);
                  continue;
                 }
              char_from_room(ch);
              char_to_room(ch,GET_LASTROOM(ch));
              //log("Put %s at room %d",GET_NAME(ch),world[IN_ROOM(ch)].number);
             }
          else
             {if (world[room].number == zone_table[world[room].zone].top)
                 continue;
              GET_LASTROOM(ch) = room;
              char_from_room(ch);
              room = real_room(zone_table[world[room].zone].top);
              if (room == NOWHERE)
                 room = GET_LASTROOM(ch);
              char_to_room(ch,room);
              //log("Remove %s at room %d",GET_NAME(ch),world[IN_ROOM(ch)].number);
             }
         }
     }
 
 for (obj = object_list; obj; obj = obj_next)
     {obj_next = obj->next;
        
	if (obj->carried_by || obj->worn_by ||
          (room = obj->in_room) == NOWHERE)
         continue;
      if (zone >= 0 && world[room].zone != zone)
         continue;
      time_ok  = FALSE;
      month_ok = FALSE;
      need_move= FALSE;
      no_time  = TRUE;
      no_month = TRUE;
      if (OBJ_FLAGGED(obj,ITEM_DAY))
         {if (weather_info.sunlight == SUN_RISE ||
              weather_info.sunlight == SUN_LIGHT
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_NIGHT))
         {if (weather_info.sunlight == SUN_SET ||
              weather_info.sunlight == SUN_DARK
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_FULLMOON))
         {if ((weather_info.sunlight == SUN_SET ||
               weather_info.sunlight == SUN_DARK) &&
              (weather_info.moon_day >= 12 &&
               weather_info.moon_day <= 15)
             )
             time_ok = TRUE;
          need_move = TRUE;
          no_time   = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_WINTER))
         {if (weather_info.season == SEASON_WINTER)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_SPRING))
         {if (weather_info.season == SEASON_SPRING)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_SUMMER))
         {if (weather_info.season == SEASON_SUMMER)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (OBJ_FLAGGED(obj,ITEM_AUTUMN))
         {if (weather_info.season == SEASON_AUTUMN)
             month_ok = TRUE;
          need_move = TRUE;
          no_month  = FALSE;
         }
      if (need_move)
         {month_ok |= no_month;
          time_ok  |= no_time;
          if (month_ok && time_ok)
             {if (world[room].number != zone_table[world[room].zone].top)
                 continue;
              if (OBJ_GET_LASTROOM(obj) == NOWHERE)
                 {extract_obj(obj);
                  continue;
                 }
              obj_from_room(obj);
              obj_to_room(obj,OBJ_GET_LASTROOM(obj));
              
             }
          else
             {if (world[room].number == zone_table[world[room].zone].top)
                 continue;
              OBJ_GET_LASTROOM(obj) = room;
              obj_from_room(obj);
              room = real_room(zone_table[world[room].zone].top);
              if (room == NOWHERE)
                 room = OBJ_GET_LASTROOM(obj);
              obj_to_room(obj,room);
              
             }
         }
     }
}


// Ищет RNUM первой и последней комнаты зоны !!!!!!!!! 
// Еси возвращает 0 - комнат в зоне нету
int get_zone_rooms(int zone_nr, int * start, int * stop)
{
  int first_room_vnum, rnum;
  first_room_vnum = zone_table[zone_nr].top;
  rnum = real_room( first_room_vnum );
  if ( rnum == -1 ) return 0; 
  *stop = rnum;
  rnum = -1;
  while ( zone_nr )
  {
    first_room_vnum = zone_table[--zone_nr].top;
    rnum = real_room( first_room_vnum );
    if ( rnum != -1 ) 
    {
      ++rnum;
      break;
    }
  }
  if ( rnum == -1 ) rnum = 0; // самая первая зона начинается с 0
  *start = rnum;
  return 1;
}

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
  DESCRIPTOR_DATA *i;
  CHAR_DATA *c;
  int rnum_start, rnum_stop;
  

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (IN_ROOM(i->character) == NOWHERE)
      continue;
    if (GET_LEVEL(i->character) >= LVL_IMMORT)
      continue;
    if (world[i->character->in_room].zone != zone_nr)
      continue;

    return 0;
  }

	rnum_start = zone_table[zone_nr].number * 100;
    rnum_stop = zone_table[zone_nr].top;

		for ( ; rnum_start <= rnum_stop; ++rnum_start )
		{   
            int r = real_room(rnum_start);
            if (r == NOWHERE)
                continue;

            // portal exists in zone
            if (world[r].portal_time)
				        return 0;

            c = world[r].people;
			for (; c != NULL; c = c->next_in_room)
				if ( !IS_NPC(c) && (GET_LEVEL(c) < LVL_IMMORT)) 
						return 0;
		}	     


	// теперь проверю всех товарищей в void комнате STRANGE_ROOM
 	 for ( c = world[STRANGE_ROOM].people; c; c = c->next_in_room )
  	   { int was = GET_WAS_IN(c);
    	     if (was == NOWHERE) 
                   continue;
    	      if (GET_LEVEL(c) >= LVL_IMMORT) 
                   continue;
    		if (world[was].zone != zone_nr) 
                   continue;
    		     return 0;
  	}
  return 1;
}


int mobs_in_room(int m_num, int r_num)
{ CHAR_DATA *ch;
  int count = 0;

	for (ch = world[r_num].people; ch; ch = ch->next_in_room)
		 if (m_num == GET_MOB_RNUM(ch))
			count++;
 return count;
}


/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/

long cmp_ptable_by_name(char *name, int len)
{ int i;

  len = MIN(len,strlen(name));
  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
      if (!strn_cmp(player_table[i].name, arg, MIN(len,strlen(player_table[i].name))))
         return (i);
  return (-1);
}



long get_ptable_by_name(char *name)
{ int i;
  one_argument(name, arg);
  
  for (i = 0; i <= top_of_p_table; i++)
     if (!str_cmp(player_table[i].name, arg))
              return (i);
  sprintf(buf,"Персонаж %s(%s) в базе отсутствует.",name, arg);
  mudlog(buf, NRM, LVL_IMMORT, TRUE);
  return (-1);
}

long get_id_by_name(char *name)
{  int i;
   one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, arg))
      return (player_table[i].id);
  return (-1);
}

CHAR_DATA *get_char_by_id(int id)
{
	for (CHAR_DATA *i = character_list; i; i = i->next)
		if (!IS_NPC(i) && GET_IDNUM(i) == id)
			return (i);
	return 0;
}

char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == id)
      return (player_table[i].name);

  return (NULL);
}

struct char_data *get_mob_by_id(long id)
{ struct char_data *ch;

  for (ch = character_list; ch; ch = ch->next)
    if (ch->id == id)
      return (ch);

  return (NULL);
}

struct char_data *get_char_cmp_id(long id)
{  struct char_data *i;


  for (i = character_list; i; i = i->next)
  {    if (IS_NPC(i))
		continue;
	  if (GET_QUESTMOB(i) == id)
      return (i);
  }

  return (NULL);
}

char *get_name_by_unique(long unique)
{ int i;

  for (i = 0; i <= top_of_p_table; i++)
      if (player_table[i].unique == unique)
         return (player_table[i].name);

  return (NULL);
}

int correct_unique(int unique)
{int i;

 for (i = 0; i <= top_of_p_table; i++)
     if (player_table[i].unique == unique)
        return (TRUE);

 return (FALSE);
}

int create_unique(void)
{int unique;

 do {unique = (number(0,64) << 24) + (number(0,255) << 16) +
              (number(0,255) << 8) + (number(0,255));
    } while (correct_unique(unique));
 return (unique);
}



/* new load_char reads ascii pfiles */
/* Load a char, TRUE if loaded, FALSE if not */

int load_char_ascii(char *name, struct char_data *ch)
{
  int id, num = 0, num1 = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, i;
  long int lnum;
  FBFILE *fl;
  char filename[40];
  char buf[128+10000], line[MAX_INPUT_LENGTH + 10000], tag[6];
  char line1[MAX_INPUT_LENGTH + 10000];
  struct affected_type af;
  struct timed_type timed;  

  *filename = '\0';
  //log("Load ascii char %s", name);
  if (now_entrycount) {
    id = 1;
  } else {
    id = find_name(name);
  }
  if (!(id >= 0 &&
       get_filename(name,filename,PLAYERS_FILE) &&
      (fl=fbopen(filename, FB_READ)))) {
     log("Cann't load ascii %d %s", id, filename);
     return (-1);
  }
							  
    
    // character init 
    // initializations necessary to keep some things straight 

    if (ch->player_specials == NULL)
       CREATE(ch->player_specials, struct player_special_data,1);
    ch->player.short_descr = NULL;
    ch->player.long_descr = NULL;


    for(i = 1; i <= MAX_SKILLS; i++)
      GET_SKILL(ch, i) = 0; 
    for(i = 1; i <= MAX_SPELLS; i++)
      GET_SPELL_TYPE(ch, num) = 0;
    for(i = 1; i <= MAX_SPELLS; i++)
      GET_SPELL_MEM(ch, num) = 0;
    ch->char_specials.saved.affected_by = clear_flags;
	ch->player_specials->saved.spare0[0]= 0;
    POOFIN(ch)                          = NULL;
    POOFOUT(ch)                         = NULL;
    GET_LAST_TELL(ch)                   = NOBODY;
//    GET_RSKILL(ch)                      = NULL;    // рецептов не знает
    ch->char_specials.carry_weight      = 0;
    ch->char_specials.carry_items       = 0;
    ch->real_abils.armor                = 100;
    //ch->player.bonus			= 0;
    //ch->player.unbonus			= 0;
//    GET_MEM_TOTAL(ch)                   = 0;
  //  GET_MEM_COMPLETED(ch)               = 0;
  //  MemQ_init(ch);
    GET_CRBONUS(ch)			= 0;
    GET_AC(ch)				= 10;
    GET_ALIGNMENT(ch)			= 0;
    GET_BAD_PWS(ch)			= 0;
    GET_BANK_GOLD(ch)			= 0;
    ch->player_specials->time.birth 	= time(0);
    GET_CHA(ch)				= 10;
    GET_CHA_ROLL(ch)			= 10;
    GET_CLASS(ch)			= 1;
    GET_CON(ch)				= 10;
	GET_CON_ROLL(ch)		= 10;
    GET_COMMSTATE(ch)			= 0;
    GET_DEX(ch)				= 10;
	GET_DEX_ROLL(ch)		= 10;
    GET_COND(ch, DRUNK)			= 0;
    GET_DRUNK_STATE(ch)			= 0;
    DUMB_DURATION(ch)			= 0;
    DUMB_REASON(ch)			= 0;
    GET_DR(ch)				= 0;
    GET_EXP(ch)				= 0;
    GET_FREEZE_LEV(ch)			= 0;
    FREEZE_DURATION(ch)			= 0;
    GET_GOLD(ch)			= 0;
	GET_QUESTPOINTS(ch)		= 0;
    GODS_DURATION(ch)			= 0;
    GET_GLORY(ch)			= 0;
    ch->player_specials->saved.GodsLike = 0;
    GET_HIT(ch)				= 21;
    GET_MAX_HIT(ch)			= 21;
    GET_HEIGHT(ch)			= 50;
    GET_LOADROOM(ch)        		= 0;
	GET_HOME(ch)			= 0;
    GET_HR(ch)				= 0;
    GET_COND(ch, FULL)			= 0;
    HELL_DURATION(ch)			= 0;
    HELL_REASON(ch)			= 0;
    IND_POWER_CHAR(ch)			= 0;
    IND_SHOP_POWER(ch)			= 0;
    POWER_STORE_CHAR(ch)		= 0;
    GET_IDNUM(ch)			= 0;
    GET_INT(ch)				= 10;
	GET_INT_ROLL(ch)		= 10;
    GET_INVIS_LEV(ch)			= 0;
    LAST_LOGON(ch)			= time(0);
    GET_LEVEL(ch)			= 1;
    ch->player_specials->time.logon 	= time(0);
    GET_MOVE(ch)			= 82;
    GET_MAX_MOVE(ch)			= 82;
    MUTE_DURATION(ch)			= 0;
    MUTE_REASON(ch)			= 0;
    NAME_DURATION(ch)			= 0;
	//АВтоквестер
    GET_NEXTQUEST(ch)			= 0;
    GET_CLAN_RANK(ch)			= '\0'; 
	//ПК таймер             	
    GET_TIME_KILL(ch)       		= 0;

//    NAME_GOD(ch) = 0;
//    NAME_ID_GOD(ch) = 0;
//    GET_OLC_ZONE(ch) = 0;
    ch->player_specials->time.played 	= 0;
    IS_KILLER(ch)			= 0;
    GET_LOADROOM(ch)		= NOWHERE;
    GET_RELIGION(ch)		= 1;
    GET_RACE(ch)			= 1;
    GET_REMORT(ch)			= 0;
    GET_SEX(ch)				= 0;
    GET_STR(ch)				= 10;
    GET_STR_ROLL(ch)		= 10;
    GET_COND(ch, THIRST)	= 0;
    GET_WEIGHT(ch)			= 50;
    GET_WIMP_LEV(ch)		= 0;
    GET_WIS(ch)				= 10;
	GET_WIS_ROLL(ch)		= 10;
    GET_UNIQUE(ch) = 0;

    asciiflag_conv1("", &ch->player_specials->saved.pref);
    //asciiflag_conv1("", &ch->char_specials.saved.Act.flags[0]);//PLR_FLAGS(ch, 0));
    //asciiflag_conv1("", &ch->char_specials.saved.affected_by.flags[0]);
    asciiflag_conv1("", &PLR_FLAGS (ch, 0));
    asciiflag_conv1("", &AFF_FLAGS (ch, 0));
	
	ch->Questing.quests = NULL;
    ch->Questing.count  = 0;
//    GET_PORTALS(ch) = NULL;
     GET_LASTIP(ch)[0] = 0;
//    CREATE( GET_LOGS(ch), int, NLOG );

    while(fbgetline(fl, line, sizeof(line))) {
      tag_argument(line, tag);
      for (i = 0; !(line[i] == ' ' || line[i] == '\0'); i++) {
        line1[i] = line[i];
      }
      line1[i] =  '\0';
      num = atoi(line1);
      lnum = atol(line1);

      switch (*tag) {
      case 'A':
	if(!strcmp(tag, "Ac  "))
	  GET_AC(ch) = num;
	else if(!strcmp(tag, "Act "))
	 // asciiflag_conv1(line, &ch->char_specials.saved.Act.flags[0]);
            asciiflag_conv1(line, &PLR_FLAGS (ch, 0));
	else if(!strcmp(tag, "Aff ")) 
	    asciiflag_conv1(line, &AFF_FLAGS(ch,0));
	//asciiflag_conv1(line, &ch->char_specials.saved.affected_by.flags[0]);
	else if(!strcmp(tag, "Affs")) {
	  i = 0;
	  do {
	    fbgetline(fl, line , sizeof(line));
	    sscanf(line, "%d %d %d %d %d", &num, &num2, &num3, &num4, &num5);
	    if(num > 0) {
	      af.type = num;
	      af.duration = num2;
	      af.modifier = num3;
	      af.location = num4;
	      af.bitvector = num5;
	      affect_to_char(ch, &af);
	      i++;
	    }
	  } while (num != 0);
	} else if(!strcmp(tag, "Alin"))
	  GET_ALIGNMENT(ch) = num;
	break;
  
      case 'B':
	if(!strcmp(tag, "Badp"))
	  GET_BAD_PWS(ch) = num;
	else if(!strcmp(tag, "Bank"))
	  GET_BANK_GOLD(ch) = lnum;
	else if(!strcmp(tag, "Brth"))
	  ch->player_specials->time.birth = lnum;
       /* else if (!strcmp(tag, "Bons"))
          ch->player.bonus = num;*/
	break;
  
      case 'C':
	if(!strcmp(tag, "Cha "))
	  GET_CHA(ch) = num;
	else if(!strcmp(tag, "Clas"))
	  GET_CLASS(ch) = num;
	else if(!strcmp(tag, "Con "))
	  GET_CON(ch) = num;
	else if(!strcmp(tag, "ComS"))
          GET_COMMSTATE(ch) = num ? true : false;
	else if(!strcmp(tag, "Clan"))
	  GET_CLAN(ch) = num;
        break;
  
      case 'D':
	if(!strcmp(tag, "Desc")) {
	  ch->player.description = fbgetstring(fl);
	} else if(!strcmp(tag, "Dex "))
	  GET_DEX(ch) = num;
 	else if(!strcmp(tag, "Drnk"))
	  GET_COND(ch, DRUNK) = num;
 	else if(!strcmp(tag, "DrSt"))
	  GET_DRUNK_STATE(ch) = num;
 	else if(!strcmp(tag, "DmbD"))
    {
	  DUMB_DURATION(ch) = lnum;
      while ( line[i] && is_space( line[i] ) ) ++i;
      if ( line[i] ) DUMB_REASON(ch) = strcpy((char* )malloc(strlen(line+i)+1),line+i);
    }
	else if(!strcmp(tag, "Drol"))
	  GET_DR(ch) = num;
	break;
  
      case 'E':
	if(!strcmp(tag, "Exp "))
	  GET_EXP(ch) = lnum;
	else if(!strcmp(tag, "EMal"))
	  strcpy(GET_EMAIL(ch),line);
	break;
  
      case 'F':
	if(!strcmp(tag, "Frez"))
	  GET_FREEZE_LEV(ch) = num;
 	else if(!strcmp(tag, "FrzD"))
	  FREEZE_DURATION(ch) = lnum;
        else if(!strcmp(tag, "Fprm"))
	  ch->fprompt = str_dup(line);
	break;
  
      case 'G':
	if(!strcmp(tag, "Gold"))
	  GET_GOLD(ch) = num;
 	else if(!strcmp(tag, "GodD"))
	  GODS_DURATION(ch) = lnum;
 	else if(!strcmp(tag, "Glor"))
	{  sscanf(line, "%d/%d", &num, &num2);
	   GET_GLORY(ch) = num;
	   GET_QUESTPOINTS(ch) = num2;
	}
 	else if(!strcmp(tag, "GdFl"))
	  ch->player_specials->saved.GodsLike = lnum;
	break;
  
      case 'H':
	if(!strcmp(tag, "Hit ")) {
	  sscanf(line, "%d/%d", &num, &num2);
	  GET_HIT(ch) = num;
	  GET_MAX_HIT(ch) = num2;
	} else if(!strcmp(tag, "Hite"))
	  GET_HEIGHT(ch) = num;
	else if(!strcmp(tag, "Home"))
	  GET_LOADROOM(ch) = num;
	else if(!strcmp(tag, "Hrol"))
	  GET_HR(ch) = num;
	else if(!strcmp(tag, "Hung"))
  	  GET_COND(ch, FULL) = num;
 	else if(!strcmp(tag, "HelD"))
    {
	  HELL_DURATION(ch) = lnum;
      while ( line[i] && is_space( line[i] ) ) ++i;
      if ( line[i] ) HELL_REASON(ch) = strcpy((char* )malloc(strlen(line+i)+1),line+i);
    }
 	else if(!strcmp(tag, "Host"))
	  strcpy(GET_LASTIP(ch),line);
	break;

     case 'I':
	if(!strcmp(tag, "Id  "))
	  GET_IDNUM(ch) = lnum;
	else if(!strcmp(tag, "Int "))
	  GET_INT(ch) = num;
	else if(!strcmp(tag, "Invs"))
	  GET_INVIS_LEV(ch) = num;
        else if(!strcmp(tag, "Ipch"))
	  IND_POWER_CHAR(ch) = lnum;
	else if(!strcmp(tag, "Ispw"))
	  IND_SHOP_POWER(ch) = lnum;
	else if(!strcmp(tag, "Ipsc"))
	  POWER_STORE_CHAR(ch) = lnum;	
       break;
  
     case 'K':
     if(!strcmp(tag, "Kmob"))
	 ch->player_specials->saved.spare19 = num;//кол-во убитых мобов
     else if (!strcmp(tag, "Kcha"))
	 ch->player_specials->saved.spare9  = num;//кол-во убитых чаров
    else if (!strcmp(tag, "Kdie"))
	 ch->player_specials->saved.spare12 = num;//кол-во смертей своих
     break;

	 case 'L':
	if(!strcmp(tag, "LstL"))
	  LAST_LOGON(ch) = lnum;
	else if(!strcmp(tag, "Levl"))
	  GET_LEVEL(ch) = num;
	break;
  
      case 'M':
	if(!strcmp(tag, "Mana")) {
	  sscanf(line, "%d/%d", &num, &num2);
	  GET_MANA(ch)      = num;
	  GET_MAX_MANA(ch) = num2;
	}
	else if(!strcmp(tag, "Move")) {
	  sscanf(line, "%d/%d", &num, &num2);
	  GET_MOVE(ch) = num;
	  GET_MAX_MOVE(ch) = num2;
	}
 	else if(!strcmp(tag, "MutD"))
    {
	  MUTE_DURATION(ch) = lnum;
      while ( line[i] && is_space( line[i] ) ) ++i;
      if ( line[i] ) MUTE_REASON(ch) = strcpy((char* )malloc(strlen(line+i)+1),line+i);
    }
	else if(!strcmp(tag, "Mob ")) {
	  sscanf(line, "%d %d", &num, &num2);
	  inc_kill_vnum(ch, num, num2);
	}
	break;
  
      case 'N':
	if(!strcmp(tag, "Name"))
	 GET_PC_NAME(ch) = str_dup(line);
	else if(!strcmp(tag, "NmR "))
	  GET_RNAME(ch) = str_dup(line);
	else if(!strcmp(tag, "NmD "))
	  GET_DNAME(ch) = str_dup(line);
	else if(!strcmp(tag, "NmV "))
	  GET_VNAME(ch) = str_dup(line);
	else if(!strcmp(tag, "NmT "))
	  GET_TNAME(ch) = str_dup(line);
	else if(!strcmp(tag, "NmP "))
	  GET_PNAME(ch) = str_dup(line);
 	else if(!strcmp(tag, "NamD"))
	  NAME_DURATION(ch) = lnum;
	else if(!strcmp(tag, "Need"))
	 GET_MANA_NEED(ch)  = lnum;
	else if(!strcmp(tag, "Nqws"))
	 GET_NEXTQUEST(ch)  = lnum;
    
 /*	else if(!strcmp(tag, "NamG"))
	  NAME_GOD(ch) = num;
 	else if(!strcmp(tag, "NaID"))
	  NAME_ID_GOD(ch) = lnum;*/
	break;

  /*    case 'O':
	if(!strcmp(tag, "Olc "))
	  GET_OLC_ZONE(ch) = num;
	break;*/

  
      case 'P':
	if(!strcmp(tag, "Pass"))
	  strcpy(GET_PASSWD(ch), line);
	else if(!strcmp(tag, "Plyd"))
	  ch->player_specials->time.played = num;
	else if(!strcmp(tag, "PfIn"))
	  POOFIN(ch) = str_dup(line);
	else if(!strcmp(tag, "PfOt"))
	  POOFOUT(ch) = str_dup(line);
        else if(!strcmp(tag, "Prom"))
	  ch->prompt = str_dup(line);
	else if(!strcmp(tag, "Pref"))
	  asciiflag_conv1(line, &PRF_FLAGS(ch));
	else if(!strcmp(tag, "PK  "))
	  IS_KILLER(ch) = lnum;
	else if(!strcmp(tag, "Ptim"))
	  GET_TIME_KILL(ch) = lnum;
	else if(!strcmp(tag, "Ptit"))
	  GET_PTITLE(ch) = str_dup(line);
	else if(!strcmp(tag, "Prac")) {
	  do {
	    fbgetline(fl, line, sizeof(line));
	    sscanf(line, "%d %d", &num, &num2);
	      if(num != 0)
		GET_PRACTICES(ch, num) = true;
	  } while (num != 0);
	}
/*	else if(!strcmp(tag, "Prtl"))
	  add_portal_to_char(ch, num);*/
	break;

      case 'Q':
	if(!strcmp(tag, "Qst "))
          set_quested(ch, num);
	break;

      case 'R':
	if(!strcmp(tag, "Room"))
	  GET_LOADROOM(ch) = num;
	else if(!strcmp(tag, "Reli"))
	  GET_RELIGION(ch) = num;
	else if(!strcmp(tag, "Race"))
	  GET_RACE(ch) = num;
	else if(!strcmp(tag, "Rmrt"))
	  GET_REMORT(ch) = num;
	else if(!strcmp(tag, "Rank"))
	  GET_CLAN_RANK(ch) = num;
	else if(!strcmp(tag, "Rstr"))
	  GET_STR_ROLL(ch) = num;
	else if(!strcmp(tag, "Rint"))
	  GET_INT_ROLL(ch) = num;
	else if(!strcmp(tag, "Rwis"))
	  GET_WIS_ROLL(ch) = num;
	else if(!strcmp(tag, "Rdex"))
	  GET_DEX_ROLL(ch) = num;
	else if(!strcmp(tag, "Rcon"))
	  GET_CON_ROLL(ch) = num;
	else if(!strcmp(tag, "Rcha"))
	  GET_CHA_ROLL(ch) = num;
	
	else if(!strcmp(tag, "Razd"))
     strcpy(ch->player_specials->saved.spare0 ,line);

/*	else if(!strcmp(tag, "Rcps")) 
    {
      im_rskill * last = NULL;
	  for(;;) 
      {
        im_rskill * rs;
	    fbgetline(fl, line);
	    sscanf(line, "%d %d", &num, &num2);
	    if ( num < 0 ) break;
        num = im_get_recipe( num );
        if ( num < 0 ) continue;
        CREATE( rs, im_rskill, 1 );
        rs->rid = num;
        rs->perc = num2;
        rs->link = NULL;
        if ( last ) last->link = rs;
        else GET_RSKILL(ch) = rs;
        last = rs;
	  } 
    }*/
	break;
  
      case 'S':
	if(!strcmp(tag, "Size"))
	  GET_SIZE(ch)			= num;
	else if(!strcmp(tag, "Sex "))
	  GET_SEX(ch)			= num;
	else if(!strcmp(tag,"Stor"))
	  GET_MANA_STORED(ch)	= num;
       	else if (!strcmp(tag,"Stat"))
	{ fbgetline(fl, line, sizeof(line));
	    sscanf(line, "%d %d %d %d %d %d", &num, &num1, &num2, &num3, &num4, &num5);
		GET_POINT_STAT(ch, STR) = num;
		GET_POINT_STAT(ch, DEX) = num1;
		GET_POINT_STAT(ch, CON) = num2;
		GET_POINT_STAT(ch, INT) = num3;
		GET_POINT_STAT(ch, WIS) = num4;
		GET_POINT_STAT(ch, CHA) = num5;
	} 
	else if(!strcmp(tag, "Skil")) {
	  do {
	    fbgetline(fl, line ,sizeof(line));
	    sscanf(line, "%d %d", &num, &num2);
	      if(num != 0)
		GET_SKILL(ch, num) = num2;
	  } while (num != 0);
	} else if(!strcmp(tag, "SkTm")) {
	  do {
	    fbgetline(fl, line, sizeof(line));
	    sscanf(line, "%d %d", &num, &num2);
	      if(num != 0) {
		timed.skill = num;
		timed.time = num2;
		timed_to_char(ch,&timed);
	      }
	  } while (num != 0);
	} else if(!strcmp(tag, "Spel")) {
	  do {
	    fbgetline(fl, line, sizeof(line));
	    sscanf(line, "%d %d", &num, &num2);
	      if(num != 0 && spell_info[num].name)
		GET_SPELL_TYPE(ch, num) = num2;
	  } while (num != 0);
	} else if(!strcmp(tag, "SpMe")) {
	  do {
	    fbgetline(fl, line, sizeof(line));
	    sscanf(line, "%d %d", &num, &num2);
	      if(num != 0)
		GET_SPELL_MEM(ch, num) = num2;
	  } while (num != 0);
	} else if(!strcmp(tag, "SpXm")) {
	  do {
	    fbgetline(fl, line, sizeof(line));
	    sscanf(line, "%d", &num);
	    if(num != 0)
           ch->Memory.add(num);
	  } while (num != 0);
	} else if(!strcmp(tag, "Str "))
	  GET_STR(ch) = num;
	  
	break;

      case 'T':
	if(!strcmp(tag, "Thir"))
	  GET_COND(ch, THIRST) = num;
	else if(!strcmp(tag, "Titl"))
	  GET_TITLE(ch) = str_dup(line);
	break;
  
      case 'W':
	if(!strcmp(tag, "Wate"))
	  GET_WEIGHT(ch) = num;
	else if(!strcmp(tag, "Wimp"))
	  GET_WIMP_LEV(ch) = num;
	else if(!strcmp(tag, "Wis "))
	  GET_WIS(ch) = num;
	break;

      case 'U':
	if(!strcmp(tag, "UIN "))
	  GET_UNIQUE(ch)    = num;
        else if(!strcmp(tag, "Ubns"))
	   GET_CRBONUS(ch)  = num;
        else if(!strcmp(tag, "Ust "))// усталость
	   GET_USTALOST(ch) = num;
	else if(!strcmp(tag, "Utim"))
	   GET_UTIMER(ch)   = num;
	break;
 
      default:
	sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
    }
  }
  
  // affect_total(ch);  
  
  // initialization for imms 
  if(GET_LEVEL(ch) >= LVL_IMMORT) {
    for(i = 1; i <= MAX_SKILLS; i++)
      GET_SKILL(ch, i) = 100;
    GET_COND(ch, FULL) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
    GET_LOADROOM(ch) = NOWHERE;
  }
  if (IS_GRGOD(ch)) {
    for (i = 0; i <= MAX_SPELLS; i++) 
        GET_SPELL_TYPE(ch,i) = GET_SPELL_TYPE(ch,i) |
                               SPELL_ITEMS          |
                               SPELL_KNOW           |
                               SPELL_RUNES          |
                               SPELL_SCROLL         |
                               SPELL_POTION         |
                               SPELL_WAND;
    }
  else if (!IS_IMMORTAL(ch)) {
     for (i = 0; i <= MAX_SPELLS; i++)
        if (spell_info[i].slot_forc[(int)GET_CLASS(ch)] == MAX_SLOT)
           REMOVE_BIT(GET_SPELL_TYPE(ch,i),SPELL_KNOW);
  }

 
  if (!AFF_FLAGGED(ch, AFF_POISON) &&
      (((long) (time(0) - LAST_LOGON(ch))) >= SECS_PER_REAL_HOUR))
     { GET_HIT(ch)  = GET_REAL_MAX_HIT(ch);
       GET_MOVE(ch) = GET_REAL_MAX_MOVE(ch);
     }
  else
     GET_HIT(ch) = MIN(GET_HIT(ch), GET_REAL_MAX_HIT(ch));



  fbclose(fl);
  return(id);
}




// Загрузка игроков, TRUE загрузили удачно, FALSE не смогли загрузить 

int load_char(char *name, struct char_data * char_element)
{
  int player_i;

  player_i = load_char_ascii(name, char_element);
  if (player_i > -1) {
    GET_PFILEPOS(char_element) = player_i;
  }
  return (player_i);
}


/*
 * write the vital data of a player to the player file
 *
 * NOTE: load_room should be an *RNUM* now.  It is converted to a vnum here.
 */
void save_char(struct char_data * ch, room_rnum load_room)
{
//  struct char_file_u st;
  
  if (!now_entrycount)
    if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
        return;

  if (USE_SINGLE_PLAYER)
     {new_save_char(ch,load_room);
      return;
     }
  
}

#define CASE(test) if (!matched && !str_cmp(buf2, test) && (matched = 1))

void char_to_newload(char *name)
{  FILE *wpfile;
  char filename[MAX_STRING_LENGTH], *ptr;
  int matched = 0;

   
  *buf2 = '\0';
  
	  get_filename(name,filename,PFILE_FILE);
      if (!(wpfile = fopen(filename, "r"))) 
	  { sprintf(buf, "Error saving char file %s", name);
       mudlog(buf, NRM, LVL_GOD, TRUE);
       return;
      }
	 
	   while(get_line(wpfile, buf2))
	   { if ((ptr = strchr(buf2, ':')) != NULL)
			{ *(ptr++) = '\0';
				while (is_space(*ptr))
				ptr++;
				CASE("Prompt")
				{break;
				}
			}
	         
	  }

fclose(wpfile);
}


int create_entry(char *name)
{
  int i, pos;

  if (top_of_p_table == -1) {	/* no table */
    CREATE(player_table, struct player_index_element, 1);
    pos = top_of_p_table = 0;
  } else 
	if ((pos = get_ptable_by_name(name)) == -1) {	/* new name */
    i = ++top_of_p_table + 1;
    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++)
  player_table[pos].activity = number(0, OBJECT_SAVE_ACTIVITY-1);
  player_table[pos].timer    = NULL;
	  /* Nothing */;

  return (pos);
}



/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/


/* Читать и выделить память буфера для '~'-terminated строки из данного файла */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      log("SYSERR: fread_string: format error at or near %s", error);
      exit(1);
    }
    
	/* Если находим '~', то конец строки; 
	если встречаем "\r\n" или '\n' записываем и тоже конец строки.*/
    
	if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      log(error);
      exit(1);
    } else {
      strcat(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* память буфера для новой строки и копировать это */
  if (strlen(buf) > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return (rslt);
}


/* release memory allocated for a char struct */
void free_char_data(struct char_data * ch)
{ int i, id = -1;
  struct alias_data *a;
  struct helper_data_type *temp;
    
	if (!IS_NPC(ch))
     { id = get_ptable_by_name(GET_NAME(ch));
              if (id >= 0)
             { player_table[id].level       = (GET_REMORT(ch) ? (30+GET_REMORT(ch)) : GET_LEVEL(ch));
               player_table[id].last_logon  = time(0);
              player_table[id].activity    = number(0, OBJECT_SAVE_ACTIVITY-1);
            } 
   }

   if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1)) {
    /* if this is a player, or a non-prototyped non-player, free all */
      if (GET_NAME(ch))
      free(GET_NAME(ch));
      if (GET_RNAME(ch))
      free(GET_RNAME(ch));
      if (GET_DNAME(ch))
      free(GET_DNAME(ch));
      if (GET_VNAME(ch))
      free(GET_VNAME(ch));
      if (GET_TNAME(ch))
      free(GET_TNAME(ch));
      if (GET_PNAME(ch))
      free(GET_PNAME(ch));
      if (ch->player.title)
      free(ch->player.title);
      if (ch->player.ptitle)
      free(ch->player.ptitle);
      if (ch->player.short_descr)
      free(ch->player.short_descr);
      if (ch->player.long_descr)
      free(ch->player.long_descr);
  //    if (ch->player.description)
 //      delete ch->player.description;
      if (IS_NPC(ch) && ch->mob_specials.Questor)
         free(ch->mob_specials.Questor);
      if (ch->Questing.quests)
         free(ch->Questing.quests);
	      
	    free_mkill(ch);
		 pk_free_list(ch);

          while (ch->helpers)
            REMOVE_FROM_LIST(ch->helpers,ch->helpers,next_helper);
     
  } else if ((i = GET_MOB_RNUM(ch)) >= 0) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.name && ch->player.name != mob_proto[i].player.name)
      free(ch->player.name);
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
      free(ch->player.short_descr);
	if (ch->player.rname && ch->player.rname != mob_proto[i].player.rname)
      free(ch->player.rname);
    if (ch->player.dname && ch->player.dname != mob_proto[i].player.dname)
      free(ch->player.dname);
	if (ch->player.vname && ch->player.vname != mob_proto[i].player.vname)
      free(ch->player.vname);
    if (ch->player.tname && ch->player.tname != mob_proto[i].player.tname)
      free(ch->player.tname);
	if (ch->player.pname && ch->player.pname != mob_proto[i].player.pname)
      free(ch->player.pname);
    if (ch->player.title && ch->player.title != mob_proto[i].player.title)
      free(ch->player.title);
    if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
      free(ch->player.long_descr);
	if (ch->player.description && ch->player.description != mob_proto[i].player.description)
      free(ch->player.description);
	if (ch->mob_specials.Questor && ch->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
         free(ch->mob_specials.Questor);
    }
  supress_godsapply = TRUE;
  while (ch->affected)
    affect_remove(ch, ch->affected);
  supress_godsapply = FALSE;

   while (ch->timed)
        timed_from_char(ch, ch->timed);

  if (ch->desc)
    ch->desc->character = NULL;

   if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
    while ((a = GET_ALIASES(ch)) != NULL) {
      GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
      free_alias(a);
    }
    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
	if (GET_AFK(ch))
	  free(GET_AFK(ch));

	if (ch->prompt)
	  free(ch->prompt);
	if (ch->fprompt)
	  free(ch->fprompt);
    if (ch->exchange_filter)
      free(ch->exchange_filter);

	 if  ( MUTE_REASON(ch) )
		    free( MUTE_REASON(ch) );
      if ( DUMB_REASON(ch) )
		   free( DUMB_REASON(ch) );
      if ( HELL_REASON(ch) )
		   free( HELL_REASON(ch) );
       
	  free(ch->player_specials);
	 ch->player_specials = NULL; // чтобы словить ACCESS VIOLATION !!! 

	   if (IS_NPC(ch))
      log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
  }
}

void free_char(struct char_data * ch)
{
    free_char_data(ch);
    free(ch);
}


/* release memory allocated for an obj struct */
void free_obj_data(struct obj_data * obj)
{
  int nr;
  struct extra_descr_data *thisd, *next_one;

  if ((nr = GET_OBJ_RNUM(obj)) == -1) {
    if (obj->name)
      free(obj->name);
    if (obj->description)
      free(obj->description);
    if (obj->short_description)
      free(obj->short_description);
    if (obj->short_rdescription)
      free(obj->short_rdescription);
    if (obj->short_ddescription)
      free(obj->short_ddescription);
    if (obj->short_vdescription)
      free(obj->short_vdescription);
    if (obj->short_tdescription)
      free(obj->short_tdescription);
    if (obj->short_pdescription)
      free(obj->short_pdescription);
    if (obj->action_description)
      free(obj->action_description);
    if (obj->ex_description) {
    for (thisd = obj->ex_description; thisd; thisd = next_one) {
	   next_one = thisd->next;
	if (thisd->keyword)
	  free(thisd->keyword);
	if (thisd->description)
	  free(thisd->description);
	free(thisd);
      }
    }
  }
  else
  { if (obj->name && obj->name != obj_proto[nr].name)
      free(obj->name);
    if (obj->description && obj->description != obj_proto[nr].description)
      free(obj->description);
    if (obj->short_description && obj->short_description != obj_proto[nr].short_description)
      free(obj->short_description);
    if (obj->short_rdescription && obj->short_rdescription != obj_proto[nr].short_rdescription)
      free(obj->short_rdescription);
	if (obj->short_ddescription && obj->short_ddescription != obj_proto[nr].short_ddescription)
      free(obj->short_ddescription);
    if (obj->short_vdescription && obj->short_vdescription != obj_proto[nr].short_vdescription)
      free(obj->short_vdescription);
    if (obj->short_tdescription && obj->short_tdescription != obj_proto[nr].short_tdescription)
      free(obj->short_tdescription);
    if (obj->short_pdescription && obj->short_pdescription != obj_proto[nr].short_pdescription)
      free(obj->short_pdescription);

	if (obj->action_description && obj->action_description != obj_proto[nr].action_description)
      free(obj->action_description);
    if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description)
   
  for (thisd = obj->ex_description; thisd; thisd = next_one) {
	    next_one = thisd->next;
	if (thisd->keyword)
	  free(thisd->keyword);
	if (thisd->description)
	  free(thisd->description);
	free(thisd);
      }
  }
}

void free_obj(struct obj_data * obj)
{
    free_obj_data(obj);
    free(obj);
}

/* Load a char, TRUE if loaded, FALSE if not */
void new_load_quests(struct char_data * ch)
{FILE *loaded;
 char filename[MAX_STRING_LENGTH];
 int  intload = 0;

 if (ch->Questing.quests)
    {log("QUESTING ERROR for %s - attempt load when not empty - call to Coder",
         GET_NAME(ch));
     free(ch->Questing.quests);
    }
 ch->Questing.count = 0;
 ch->Questing.quests= NULL;

 log("Load quests %s", GET_NAME(ch));
 if (get_filename(GET_NAME(ch),filename,PQUESTS_FILE) &&
     (loaded = fopen(filename,"r+b")))
    {intload = fread(&ch->Questing.count, sizeof(int), 1, loaded);
     if (ch->Questing.count && intload)
        {CREATE(ch->Questing.quests, int, (ch->Questing.count / 10L + 1) * 10L);
         intload = fread(ch->Questing.quests, sizeof(int), ch->Questing.count, loaded);
         if (intload < ch->Questing.count)
            ch->Questing.count = intload;
         //sprintf(buf,"SYSINFO: Questing file for %s loaded(%d)...",GET_NAME(ch),intload);
         //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
        }
     else
        {//sprintf(buf,"SYSERR: Questing file for %s empty...",GET_NAME(ch));
         //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         ch->Questing.count = 0;
         CREATE(ch->Questing.quests, int, 10);
        }
     fclose(loaded);
    }
 else
    {//sprintf(buf,"SYSERR: Questing file not found for %s...",GET_NAME(ch));
     //mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
     CREATE(ch->Questing.quests, int, 10);
    }
}


void new_save_quests(struct char_data *ch)
{ FILE *saved;
  char filename[MAX_STRING_LENGTH];

  if (!ch->Questing.count || !ch->Questing.quests)
     {//if (!IS_IMMORTAL(ch))
      //   {sprintf(buf,"SYSERR: Questing list for %s empty...",GET_NAME(ch));
      //    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      //   }
     }


  if (get_filename(GET_NAME(ch),filename,PQUESTS_FILE) &&
      (saved = fopen(filename,"w+b")))
      {fwrite(&ch->Questing.count, sizeof(int), 1, saved);
       fwrite(ch->Questing.quests, sizeof(int), ch->Questing.count, saved);
       fclose(saved);
      }
}


/* remove ^M's from file output */

void kill_ems(char *str)
{
  char *ptr1, *ptr2, *tmp;
  
  tmp = str;
  ptr1 = str;
  ptr2 = str;
  
  while(*ptr1) {
    if((*(ptr2++) = *(ptr1++)) == '\r')
      if(*ptr1 == '\r')
        ptr1++;
  }
  *ptr2 = '\0';
}

void new_save_char(struct char_data * ch, room_rnum load_room)
{ FILE   *saved;
  char   filename[MAX_STRING_LENGTH];
  char str[2];
  room_rnum location;
  int i, tmp;
  time_t li;
  struct affected_type *aff, tmp_aff[MAX_AFFECT];
  struct obj_data *char_eq[NUM_WEARS];
  struct timed_type *skj;
  char DEFAULT_PROMPT[] = "%hж/%HЖ %vэ/%VЭ %Oо %gм %wm %f %#C%e%%>%#n "; 
  char DEFAULT_PROMPT_FIGHTING[] = "%f %hж %vэ %wm %#C%e%%>%#n "; 
//  struct char_portal_type *prt;

  str[0] = 0;
  str[1] = 0;

  if (!now_entrycount)
    if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
       return;

  if (!PLR_FLAGGED(ch, PLR_LOADROOM)) {
     if (load_room > NOWHERE)
     {
        GET_LOADROOM(ch)                   = GET_ROOM_VNUM(load_room);
        log("Player %s save at room %d", GET_NAME(ch), GET_ROOM_VNUM(load_room));
     } 
  }

  //log("Save char %s", GET_NAME(ch));
  new_save_pkills(ch);
  save_char_vars(ch);
  
// Запись чара в новом формате 
    get_filename(GET_NAME(ch),filename,PLAYERS_FILE);
    if (!(saved = fopen(filename,"w"))) {
      perror("Unable open charfile");
      return;
    }
// подготовка
  //снимаем все возможные аффекты  
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      char_eq[i] = unequip_char(ch, i | 0x80 | 0x40);
#ifndef NO_EXTRANEOUS_TRIGGERS
      remove_otrigger(char_eq[i], ch);
#endif
    } else
      char_eq[i] = NULL;
  }

  for (aff = ch->affected, i=0;i < MAX_AFFECT; i++) {
    if (aff) {
      if (aff->type == SPELL_ARMAGEDDON || aff->type < 1 ||
               aff->type > LAST_USED_SPELL)
         i--;
      else {
        tmp_aff[i] = *aff;
        tmp_aff[i].next = 0;
      }
      aff = aff->next;
    } else {
      tmp_aff[i].type      = 0;       // Zero signifies not used 
      tmp_aff[i].duration  = 0;
      tmp_aff[i].modifier  = 0;
      tmp_aff[i].location  = 0;
      tmp_aff[i].bitvector = 0;
      tmp_aff[i].next      = 0;
    }
  }

 
  supress_godsapply = TRUE;
  while (ch->affected)
    affect_remove(ch, ch->affected);
  supress_godsapply = FALSE;

  if ((i >= MAX_AFFECT) && aff && aff->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

// запись 
    if(GET_NAME(ch)) 
      fprintf(saved,"Name: %s\n",GET_NAME(ch));
    if(GET_NAME(ch)) 
      fprintf(saved,"NmR : %s\n",GET_RNAME(ch));
    if(GET_NAME(ch)) 
      fprintf(saved,"NmD : %s\n",GET_DNAME(ch));
    if(GET_NAME(ch)) 
      fprintf(saved,"NmV : %s\n",GET_VNAME(ch));
    if(GET_NAME(ch)) 
      fprintf(saved,"NmT : %s\n",GET_TNAME(ch));
    if(GET_NAME(ch)) 
      fprintf(saved,"NmP : %s\n",GET_PNAME(ch));
    if(GET_PASSWD(ch)) 
      fprintf(saved,"Pass: %s\n",GET_PASSWD(ch));
    if(GET_EMAIL(ch)) 
      fprintf(saved,"EMal: %s\n",GET_EMAIL(ch));
    if(GET_TITLE(ch)) 
      fprintf(saved,"Titl: %s\n",GET_TITLE(ch));
	if(GET_PTITLE(ch)) 
      fprintf(saved,"Ptit: %s\n",GET_PTITLE(ch));
    if(ch->player.description && *ch->player.description) {
      strcpy(buf, ch->player.description);
      kill_ems(buf);
      fprintf(saved,"Desc:\n%s~\n",buf);
    }
    if(POOFIN(ch)) 
      fprintf(saved,"PfIn: %s\n",POOFIN(ch));
    if(POOFOUT(ch)) 
      fprintf(saved,"PfOt: %s\n",POOFOUT(ch));
    fprintf(saved,"Sex : %d %s\n",GET_SEX(ch),genders[(int) GET_SEX(ch)]);
    fprintf(saved,"Clas: %d %s\n",GET_CLASS(ch), pc_class_types[(int) GET_CLASS(ch)]);
    fprintf(saved,"Levl: %d\n",GET_LEVEL(ch));
    if ((location = real_room(GET_LOADROOM(ch))) != NOWHERE)
      fprintf(saved,"Home: %d %s\n",GET_LOADROOM(ch),world[(location)].name);
    li = ch->player_specials->time.birth; 
    fprintf(saved,"Brth: %ld %s\n", li, ctime(&li));
    // Gunner
    tmp = time(0) - ch->player_specials->time.logon;
    tmp += ch->player_specials->time.played; 
    fprintf(saved,"Plyd: %d\n", tmp);
    // Gunner end
   // li = ch->player.time.logon;
   // fprintf(saved,"Last: %ld %s\n", li, ctime(&li));
    if (ch->desc)
      strcpy(buf,ch->desc->host);
    else
      strcpy(buf,"Unknown");
    fprintf(saved,"Host: %s\n", buf);
    fprintf(saved,"Hite: %d\n",GET_HEIGHT(ch));
    fprintf(saved,"Wate: %d\n",GET_WEIGHT(ch));
    fprintf(saved,"Size: %d\n",GET_SIZE(ch));
    
   if (ch->prompt) 
	 fprintf(saved, "Prom: %s\n", ch->prompt); 
    else 
	 fprintf(saved, "Prom: %s\n", DEFAULT_PROMPT); 
   
   if (ch->fprompt) 
	 fprintf(saved, "Fprm: %s\n", ch->fprompt); 
    else 
	 fprintf(saved, "Fprm: %s\n", DEFAULT_PROMPT_FIGHTING); 
 
	// структуры 
    fprintf(saved,"Alin: %d\n",GET_ALIGNMENT(ch));
    fprintf(saved,"Id  : %ld\n",GET_IDNUM(ch));
    fprintf(saved,"UIN : %d\n",GET_UNIQUE(ch));
    *buf = '\0';
    //tascii(&ch->char_specials.saved.Act.flags[0], 1, buf);
    tascii (&PLR_FLAGS (ch, 0), 4, buf);
    fprintf(saved,"Act : %s\n",buf);
    *buf = '\0';
    //tascii(&ch->char_specials.saved.affected_by.flags[0], 1, buf);
    tascii (&AFF_FLAGS (ch, 0), 4, buf);
    fprintf(saved,"Aff : %s\n",buf);
	
    
    // дальше не по порядку 
    // статсы 
    fprintf(saved,"Str : %d\n",GET_STR(ch));
    fprintf(saved,"Int : %d\n",GET_INT(ch));
    fprintf(saved,"Wis : %d\n",GET_WIS(ch));
    fprintf(saved,"Dex : %d\n",GET_DEX(ch));
    fprintf(saved,"Con : %d\n",GET_CON(ch));
    fprintf(saved,"Cha : %d\n",GET_CHA(ch));

	//рол_статсы
	fprintf(saved,"Rstr: %d\n",GET_STR_ROLL(ch));
    fprintf(saved,"Rint: %d\n",GET_INT_ROLL(ch));
    fprintf(saved,"Rwis: %d\n",GET_WIS_ROLL(ch));
    fprintf(saved,"Rdex: %d\n",GET_DEX_ROLL(ch));
    fprintf(saved,"Rcon: %d\n",GET_CON_ROLL(ch));
    fprintf(saved,"Rcha: %d\n",GET_CHA_ROLL(ch));

	//клан, кланранг.
    fprintf(saved,"Clan: %d\n",GET_CLAN(ch));
    fprintf(saved,"Rank: %d\n",GET_CLAN_RANK(ch));
    
    // скилы 
    if (GET_LEVEL(ch) < LVL_IMMORT) {
      fprintf(saved,"Skil:\n");
      for (i = 1; i <= MAX_SKILLS; i++) {
        if (GET_SKILL(ch, i))
          fprintf(saved,"%d %d %s\n", i, GET_SKILL(ch, i), skill_info[i].name);
      }
      fprintf(saved,"0 0\n");
    }

     	//статы 
      fprintf(saved,"Stat:\n");
	  *buf = 0;
      for (i = 0; i <= 5; i++)
	  sprintf(buf + strlen(buf), "%d ", GET_POINT_STAT(ch, i)); 
      fprintf(saved,"%s\n", buf);
      

	//практик
	if (GET_LEVEL(ch) < LVL_IMMORT) {
      fprintf(saved,"Prac:\n");
      for (i = 1; i <= MAX_SKILLS; i++) {
        if (GET_PRACTICES(ch, i))
          fprintf(saved,"%d %d %s\n", i, GET_PRACTICES(ch, i), skill_info[i].name);
      }
      fprintf(saved,"0 0\n");
    }
    // Задержки на скилы 
    if (GET_LEVEL(ch) < LVL_IMMORT) {
      fprintf(saved,"SkTm:\n");
      for (skj = ch->timed; skj; skj = skj->next) {
        fprintf(saved,"%d %d %s\n", skj->skill, skj->time, 
	                            skill_info[skj->skill].name);
      }
      fprintf(saved,"0 0\n");
    }
    
    // спелы 
    if (GET_LEVEL(ch) < LVL_IMMORT) {
      fprintf(saved,"Spel:\n");
      for (i = 1; i <= MAX_SPELLS; i++) {
        if (GET_SPELL_TYPE(ch, i))
          fprintf(saved,"%d %d %s\n", i, GET_SPELL_TYPE(ch, i), spell_info[i].name);
      }
      fprintf(saved,"0 0\n");
    }

    // Замемленые спелы 
    if (GET_LEVEL(ch) < LVL_IMMORT) {
      fprintf(saved,"SpMe:\n");
      for (i = 1; i <= MAX_SPELLS; i++) {
        if (GET_SPELL_MEM(ch, i))
          fprintf(saved,"%d %d %s\n", i, GET_SPELL_MEM(ch, i), spell_info[i].name);
      }
      fprintf(saved,"0 0\n");
    }

    //запись спелов, находящихся в меме.
	if (GET_LEVEL(ch) < LVL_IMMORT) {
      fprintf(saved,"SpXm:\n");     
      int e = ch->Memory.size();
      for (i = 1; i <= e; i++) 
      {
          int id = ch->Memory.getid(i-1);
          fprintf(saved,"%d %s\n", id, spell_info[id].name);
      }
      fprintf(saved,"0\n");
    } 


    // Рецепты 
//    if (GET_LEVEL(ch) < LVL_IMMORT) 
 /*   {
      im_rskill * rs;
      im_recipe * r;
      fprintf(saved,"Rcps:\n");
      for (rs = GET_RSKILL(ch);rs;rs=rs->link)
      {
        if ( rs->perc <= 0 ) continue;
        r = &imrecipes[rs->rid];
        fprintf(saved,"%d %d %s\n", r->id, rs->perc, r->name);        
      }
      fprintf(saved,"-1 -1\n");
    }
*/
	
	fprintf(saved,"Kmob: %ld\n",ch->player_specials->saved.spare19);//кол-во убитых мобов
    fprintf(saved,"Kcha: %d\n",ch->player_specials->saved.spare9 );//кол-во убитых чаров
    fprintf(saved,"Kdie: %d\n",ch->player_specials->saved.spare12);//кол-во смертей своих

	fprintf(saved,"Nqws: %d\n",GET_NEXTQUEST(ch));
	fprintf(saved,"Ptim: %ld\n",GET_TIME_KILL(ch));
	
	fprintf(saved,"Need: %d\n",GET_MANA_NEED(ch));
    fprintf(saved,"Stor: %d\n",GET_MANA_STORED(ch));

    fprintf(saved,"Hrol: %d\n",GET_HR(ch));
    fprintf(saved,"Drol: %d\n",GET_DR(ch));
    fprintf(saved,"Ac  : %d\n",GET_AC(ch));

    fprintf(saved,"Hit : %d/%d\n",GET_HIT(ch),  GET_MAX_HIT(ch));
    fprintf(saved,"Mana: %d/%d\n",GET_MANA(ch), GET_MAX_MANA(ch));
    fprintf(saved,"Move: %d/%d\n",GET_MOVE(ch), GET_MAX_MOVE(ch));
    fprintf(saved,"Gold: %d\n",GET_GOLD(ch));
    fprintf(saved,"Bank: %ld\n",GET_BANK_GOLD(ch));
    fprintf(saved,"Exp : %ld\n",GET_EXP(ch));
    fprintf(saved,"PK  : %d\n",IS_KILLER(ch));
    //fprintf(saved,"Bons: %d\n", ch->player.bonus);
    fprintf(saved,"Ubns: %d\n", GET_CRBONUS(ch));

    fprintf(saved,"Wimp: %d\n",GET_WIMP_LEV(ch));
    fprintf(saved,"Frez: %d\n",GET_FREEZE_LEV(ch));
    fprintf(saved,"Invs: %d\n",GET_INVIS_LEV(ch));
    fprintf(saved,"Room: %d\n",GET_LOADROOM(ch));

    str[0] = *ch->player_specials->saved.spare0;
    fprintf(saved,"Razd: %s\n", str);

    fprintf(saved,"Ust : %d\n",GET_USTALOST(ch));// усталость
    fprintf(saved,"Utim: %d\n",GET_UTIMER(ch));
    
    fprintf(saved,"Ipch: %ld\n",IND_POWER_CHAR(ch));
    fprintf(saved,"Ispw: %ld\n",IND_SHOP_POWER(ch));
    fprintf(saved,"Ipsc: %ld\n",POWER_STORE_CHAR(ch));
    

    fprintf(saved,"Badp: %d\n",GET_BAD_PWS(ch));
    if (GET_LEVEL(ch) < LVL_IMMORT)
      fprintf(saved,"Hung: %d\n",GET_COND(ch, FULL));
    if (GET_LEVEL(ch) < LVL_IMMORT)
      fprintf(saved,"Thir: %d\n",GET_COND(ch, THIRST));
    if (GET_LEVEL(ch) < LVL_IMMORT)
      fprintf(saved,"Drnk: %d\n",GET_COND(ch, DRUNK));

   // fprintf(saved,"Reli: %d %s\n",GET_RELIGION(ch),
   //                       religion_name[GET_RELIGION(ch)][(int) GET_SEX(ch)]);
    fprintf(saved,"Race: %d %s\n",GET_RACE(ch),
                          pc_race_types[GET_RACE(ch)][(int) GET_SEX(ch)]);
    fprintf(saved,"DrSt: %d\n",GET_DRUNK_STATE(ch));
    fprintf(saved,"ComS: %d\n",GET_COMMSTATE(ch));
    fprintf(saved,"Glor: %d/%d\n",GET_GLORY(ch), GET_QUESTPOINTS(ch));
   
    if (GET_REMORT(ch) > 0)
      fprintf(saved,"Rmrt: %d\n",GET_REMORT(ch));
    *buf = '\0';
    tascii((int *)&ch->player_specials->saved.pref, 1, buf);
    fprintf(saved,"Pref: %s\n",buf);
    if (NAME_DURATION(ch) > 0)
      fprintf(saved,"NamD: %ld\n",NAME_DURATION(ch));
    if (GODS_DURATION(ch) > 0)
      fprintf(saved,"GodD: %ld\n",GODS_DURATION(ch));
    if (MUTE_DURATION(ch) > 0)
    {
      if ( MUTE_REASON(ch) )
        fprintf(saved,"MutD: %ld %s\n",MUTE_DURATION(ch),MUTE_REASON(ch));
      else
        fprintf(saved,"MutD: %ld\n",MUTE_DURATION(ch));
    }
    if (FREEZE_DURATION(ch) > 0)
      fprintf(saved,"FrzD: %ld\n",FREEZE_DURATION(ch));
    if (HELL_DURATION(ch) > 0)
    {
      if ( HELL_REASON(ch) )
        fprintf(saved,"HelD: %ld %s\n",HELL_DURATION(ch),HELL_REASON(ch));
      else
        fprintf(saved,"HelD: %ld\n",HELL_DURATION(ch));
    }
    if (DUMB_DURATION(ch) > 0)
    {
      if ( DUMB_REASON(ch) )
        fprintf(saved,"DmbD: %ld %s\n",DUMB_DURATION(ch),DUMB_REASON(ch));
      else
        fprintf(saved,"DmbD: %ld\n",DUMB_DURATION(ch));
    }
    fprintf(saved,"LstL: %ld\n",LAST_LOGON(ch));
    fprintf(saved,"GdFl: %ld\n",ch->player_specials->saved.GodsLike);
//    fprintf(saved,"NamG: %d\n",NAME_GOD(ch));
//    fprintf(saved,"NaID: %ld\n",NAME_ID_GOD(ch));

    // affected_type 
    if (tmp_aff[0].type > 0) {
      fprintf(saved, "Affs:\n");
      for(i = 0; i < MAX_AFFECT; i++) {
        aff = &tmp_aff[i];
        if(aff->type)
          fprintf(saved, "%d %d %ld %d %ld %s\n", aff->type, aff->duration,
          aff->modifier, aff->location, (int)aff->bitvector,
	  spell_name(aff->type));
      }
      fprintf(saved, "0 0 0 0 0\n");
    }
    
    // порталы 
  /*  for (prt = GET_PORTALS(ch); prt; prt = prt->next) {
      fprintf(saved,"Prtl: %d\n", prt->vnum);
    }
    for ( i = 0; i < NLOG; ++i )
    {
      fprintf(saved,"Logs: %d %d\n", i, GET_LOGS(ch)[i] );
    }*/

    // Квесты 
    if (ch->Questing.quests) {
      for (i = 0; i < ch->Questing.count; i++) {
        fprintf(saved,"Qst : %d\n",*(ch->Questing.quests + i));
      }
    }
    
    save_mkill(ch, saved);

    fclose(saved);
    
  // восстанавливаем аффекты 
  // add spell and eq affections back in now 
  for (i = 0; i < MAX_AFFECT; i++) {
    if (tmp_aff[i].type)
          affect_to_char(ch,&tmp_aff[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i]) {
#ifndef NO_EXTRANEOUS_TRIGGERS
           if (wear_otrigger(char_eq[i], ch, i))
#endif
             equip_char(ch, char_eq[i], i | 0x80 | 0x40);
#ifndef NO_EXTRANEOUS_TRIGGERS
           else
             obj_to_char(char_eq[i], ch);
#endif
    }
  }
  affect_total(ch);

  if ((i = get_ptable_by_name(GET_NAME(ch))) >= 0) {
   //   log("[CHAR TO STORE] Change logon time");
      player_table[i].last_logon = -1;
      player_table[i].level      = GET_LEVEL(ch);
  }
  
}

void rename_char(struct char_data *ch, char *oname)
{ char filename[MAX_INPUT_LENGTH], ofilename[MAX_INPUT_LENGTH];

 // 1) Rename(if need) char and pkill file - directly
 log("Rename char %s->%s", GET_NAME(ch), oname);
 get_filename(oname,ofilename,PLAYERS_FILE);
 get_filename(GET_NAME(ch),filename,PLAYERS_FILE);
 rename(ofilename,filename);

 save_char(ch, GET_LOADROOM(ch)); 

 get_filename(oname,ofilename,PKILLERS_FILE);
 get_filename(GET_NAME(ch),filename,PKILLERS_FILE);
 rename(ofilename,filename);

 // 2) Rename all other files
 get_filename(oname,ofilename,CRASH_FILE);
 get_filename(GET_NAME(ch),filename,CRASH_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,TEXT_CRASH_FILE);
 get_filename(GET_NAME(ch),filename,TEXT_CRASH_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,TIME_CRASH_FILE);
 get_filename(GET_NAME(ch),filename,TIME_CRASH_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,ETEXT_FILE);
 get_filename(GET_NAME(ch),filename,ETEXT_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,ALIAS_FILE);
 get_filename(GET_NAME(ch),filename,ALIAS_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,SCRIPT_VARS_FILE);
 get_filename(GET_NAME(ch),filename,SCRIPT_VARS_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,PQUESTS_FILE);
 get_filename(GET_NAME(ch),filename,PQUESTS_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,PMKILL_FILE);
 get_filename(GET_NAME(ch),filename,PMKILL_FILE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,PLAYER_EXCHANGE);
 get_filename(GET_NAME(ch),filename,PLAYER_EXCHANGE);
 rename(ofilename,filename);

 get_filename(oname,ofilename,PLAYER_MAIL);
 get_filename(GET_NAME(ch),filename,PLAYER_MAIL);
 rename(ofilename,filename);
}

int delete_char(char *name)
{  char   filename[MAX_INPUT_LENGTH];
   struct char_data *st;
   int    id, retval=TRUE;

 
     CREATE(st, CHAR_DATA, 1);
     clear_char(st);
  if ((id = load_char(name,st)) >= 0) { 
    //1) Метим игрока как удаленного
      SET_BIT(PLR_FLAGS(st, PLR_DELETED), PLR_DELETED);      
      save_char(st, GET_LOADROOM(st));
      extract_char(st, FALSE);

      Crash_clear_objects(id);
      player_table[id].unique     = -1;
      player_table[id].level      = -1;
      player_table[id].last_logon = -1;
      player_table[id].activity   = -1;
     }
  else
     retval = FALSE;

  free(st);
  
  // 2) Remove all other files
  Crash_delete_files(id);

 get_filename(name,filename,SCRIPT_VARS_FILE);
  remove(filename);


 
  return (retval);
}

/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */

int file_to_string_alloc(const char *name, char **buf)
{
  char temp[MAX_EXTEND_LENGTH];
  struct descriptor_data *in_use;

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
    if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
      return (-1);

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
    return (-1);

  if (*buf)
    free(*buf);

  *buf = str_dup(temp);
  return (0);
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE+3];

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    log("SYSERR: reading %s: %s", name, strerror(errno));
    return (-1);
  }
  do {
    fgets(tmp, READ_SIZE, fl);
    tmp[strlen(tmp) - 1] = '\0'; /* take off the trailing \n */
    strcat(tmp, "\r\n");

    if (!feof(fl)) {
      if (strlen(buf) + strlen(tmp) + 1 > MAX_EXTEND_LENGTH) {
        log("SYSERR: %s: string too big (%d max)", name,
		MAX_STRING_LENGTH);
	*buf = '\0';
	return (-1);
      }
      strcat(buf, tmp);
    }
  } while (!feof(fl));

  fclose(fl);

  return (0);
}



/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;

  memset((void *) &ch->add_abils, 0, sizeof(struct char_played_ability_data));

  ch->followers		= NULL;
  ch->master		= NULL;
  ch->in_room		= NOWHERE;
  ch->carrying		= NULL;
  ch->next			= NULL;
  ch->next_fighting = NULL;
  ch->Protecting	= NULL;
  ch->next_in_room	= NULL;
  FIGHTING(ch)		= NULL;
  ch->Touching      = NULL;
  ch->Poisoner      = 0;
  ch->BattleAffects = 0;
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;
  
 
 // CLR_MEMORY(ch);
  GET_CASTER(ch)    = 0;
  GET_DAMAGE(ch)    = 0;
  
  GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  ch->in_room = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  GET_CASTER(ch)   = 0;
  GET_DAMAGE(ch)   = 0;
  CLR_MEMORY(ch);
  ch->Poisoner     = 0;
  ch->mob_specials.default_pos = POS_STANDING;

//  GET_AC(ch) = 20;		/* Basic Armor */
 /* if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;*/
}


void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
  obj->worn_on = NOWHERE;
}




/* initialize a new character only if class is set */
void init_char(struct char_data * ch)
{
  int i, start_room = NOWHERE;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */

  if (top_of_p_table == 0) 
  {     GET_EXP(ch) 		= 550000000;
        GET_LEVEL(ch)	    	= LVL_IMPL;
        ch->points.max_hit	= 8000;
    	ch->points.max_mana 	= 500; 
        ch->points.max_move 	= 500;
	start_room          	= immort_start_room;
	ch->real_abils.intel= 25;
	ch->real_abils.wis	= 25;
	ch->real_abils.dex	= 25;
	ch->real_abils.str	= 25;
	ch->real_abils.con	= 25;
	ch->real_abils.cha	= 25;
	ch->real_abils.size	= 50;
    
  }
  GET_LASTIP(ch)[0] 		= 0;
  ch->player.title 		= NULL;
  ch->player.ptitle		= NULL;
  ch->player.short_descr 	= NULL;
  ch->player.long_descr 	= NULL;
  ch->player.description 	= NULL;
  GET_HOME(ch) 			= 1;
  ch->player_specials->time.birth =  time(0);
  ch->player_specials->time.played = 0;
  ch->player_specials->time.logon = time(0);
 strcpy(ch->player_specials->saved.spare0,"'");

  for (i = 0; i < MAX_TONGUE; i++)
    GET_TALK(ch, i) = 0;
 

  ch->points.max_mana = 100;
  ch->points.mana = GET_MAX_MANA(ch);
  ch->points.hit = GET_MAX_HIT(ch);
  ch->points.max_move = 82;
  ch->points.move = GET_MAX_MOVE(ch);
  ch->real_abils.armor = 100;

  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
   { player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
     player_table[i].unique     = GET_UNIQUE(ch) = create_unique();
     player_table[i].level      = 0;
     player_table[i].last_logon = -1;
   }
  else
    log("SYSERR: инициализация персонажа: Персонаж '%s' не найдет в таблице игроков.", GET_NAME(ch));   
  
  for (i = 1; i <= MAX_SKILLS; i++) {
    if (GET_LEVEL(ch) < LVL_IMMORT)
      SET_SKILL(ch, i, 0);
    else
      SET_SKILL(ch, i, 100);
  }

	for (i=1; i <= MAX_SPELLS; i++) { 
	if (GET_LEVEL(ch) < LVL_GRGOD)
      GET_SPELL_TYPE(ch, i) = 0;
    else
      GET_SPELL_TYPE(ch, i) = SPELL_KNOW;
      }

 ch->char_specials.saved.affected_by = clear_flags;

  for (i = 0; i < 6; i++)
    GET_SAVE(ch, i) = 0;

  for (i = 0; i < 3; i++)
   GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);

  
  if (GET_LEVEL(ch) >= LVL_IMMORT)
	GET_LOADROOM(ch) = immort_start_room;
  else  
	GET_LOADROOM(ch) = mortal_start_room;
  
  SET_BIT(PLR_FLAGS(ch, PLR_LOADROOM), PLR_LOADROOM);
  SET_BIT(PRF_FLAGS(ch), PRF_AUTOEXIT);
  SET_BIT(PRF_FLAGS(ch), PRF_SUMMONABLE);
  SET_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
  save_char(ch, NOWHERE);

}
//IND_POWER_CHAR IND_SHOP_POWER
ACMD(do_remort)
{ char filename[MAX_INPUT_LENGTH];
  int  i, load_room = NOWHERE;
  struct helper_data_type *temp;

  if (IS_NPC(ch))
      return; 
  if (IS_IMMORTAL(ch)){ 
      send_to_char("Это Вам ни к чему!\r\n",ch);
      return;
  }
  if (!GET_GOD_FLAG(ch, GF_REMORT)) {
      send_to_char("Вы недостигли возможности для перевоплощения!\r\n",ch);
      return;
   }
 
  log("Remort %s", GET_NAME(ch));
  get_filename(GET_NAME(ch), filename, PQUESTS_FILE);
  remove(filename);
  get_filename(GET_NAME(ch), filename, PMKILL_FILE);
  remove(filename);

  GET_REMORT(ch)++;
  CLR_GOD_FLAG(ch, GF_REMORT);
  GET_STR(ch) = GET_STR_ROLL(ch) += 1;
  GET_CON(ch) = GET_CON_ROLL(ch) += 1;
  GET_DEX(ch) = GET_DEX_ROLL(ch) += 1;
  GET_INT(ch) = GET_INT_ROLL(ch) += 1;
  GET_WIS(ch) = GET_WIS_ROLL(ch) += 1;
  GET_CHA(ch) = GET_CHA_ROLL(ch) += 1;

  //возвращаем бонусы после реморта для классов.
  	switch (GET_CLASS(ch))
	{ case CLASS_WEDMAK:
		GET_CRBONUS(ch)=15;
	 break;
	  case CLASS_MONAH:
	  GET_CRBONUS(ch)=13;
   	 break;
	  case CLASS_TAMPLIER:
	    GET_CRBONUS(ch)=8;
        if (IS_HUMAN(ch))
		{ GET_CRBONUS(ch)=11;
		  ch->real_abils.cha+=1;
		}
	 break;
   default:
	   GET_CRBONUS(ch)=8;
      if (IS_HUMAN(ch))
	   GET_CRBONUS(ch)=11;
	  break;
  }  
 
  if (FIGHTING(ch))
     stop_fighting(ch,TRUE);

  die_follower(ch);
  free_mkill(ch);
  
  ch->Questing.count = 0;
  if (ch->Questing.quests)
  {
     free(ch->Questing.quests);
     ch->Questing.quests = 0;
  }

  while (ch->helpers)
    REMOVE_FROM_LIST(ch->helpers,ch->helpers,next_helper);
        
  supress_godsapply = TRUE;
  while (ch->affected)
     affect_remove(ch, ch->affected);
  supress_godsapply = FALSE;

  //Убираем набранные очки для тренировки
  for (i = 0; i <= 5; i++)
     GET_POINT_STAT(ch, i) = 0;

  while (ch->timed)
        timed_from_char(ch, ch->timed);
  for (i = 1; i <= MAX_SKILLS; i++)
      SET_SKILL(ch, i, 0);
  for (i = 1; i <= MAX_SPELLS; i++)
  {
      GET_SPELL_TYPE(ch, i) = 0;
      GET_SPELL_MEM(ch, i)  = 0;
  }
  GET_HIT(ch)       = GET_MAX_HIT(ch)     = 60;
  GET_MOVE(ch)      = GET_MAX_MOVE(ch)    = 82;
  GET_MANA_NEED(ch) = GET_MANA_STORED(ch) = 0;
  GET_MANA(ch)      = GET_MAX_MANA(ch)    = MAX(wis_app[GET_WIS(ch)+2].spell_success, 10);
  GET_LEVEL(ch)     = 0;
  GET_WIMP_LEV(ch)  = 0;
  GET_AC(ch)        = 100;
  GET_LOADROOM(ch)  = calc_loadroom(ch);
  GET_GLORY(ch)    += GET_QUESTPOINTS(ch);
  GET_GLORY(ch)    += 1000;
  GET_QUESTPOINTS(ch) =0;
 
  REMOVE_BIT(PRF_FLAGS(ch), PRF_AWAKE);

  set_title(ch, "");
  set_ptitle(ch, "");

  do_start(ch, FALSE);
  save_char(ch, NOWHERE);
  if (PLR_FLAGGED(ch, PLR_HELLED))
     load_room = r_helled_start_room;
  else
  if (PLR_FLAGGED(ch, PLR_NAMED))
     load_room = r_named_start_room;
  else
  if (PLR_FLAGGED(ch, PLR_FROZEN))
     load_room = r_frozen_start_room;
  else
     {if ((load_room = GET_LOADROOM(ch)) == NOWHERE)
         load_room = calc_loadroom(ch);
      load_room = real_room(load_room);
     }
  if (load_room == NOWHERE)
     {if (GET_LEVEL(ch) >= LVL_IMMORT)
         load_room = r_immort_start_room;
      else
         load_room = r_mortal_start_room;
     }
  char_from_room(ch);
  char_to_room(ch, load_room);
  look_at_room(ch, 0);
  act("$n вош$i в Мир.", TRUE, ch, 0, 0, TO_ROOM);
  act("Поздравляем Вас, Вы перевоплотились",FALSE, ch, 0, 0, TO_CHAR);
}


// returns the real number of the room with given virtual number 
room_rnum real_room(room_vnum vnum)
{
  room_rnum bot, top, mid;

  bot = 0;
  top = top_of_world;

  // perform binary search on world-table
  for (;;) {
    mid = (bot + top) / 2;

    if ((world + mid)->number == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if ((world + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  mob_rnum bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(struct obj_data *obj)
{
  int error = FALSE;

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative weight (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

  sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf);
  if (strstr(buf, "UNDEFINED") && (error = TRUE))
    log("SYSERR: Object #%d (%s) has unknown wear flags.",
	GET_OBJ_VNUM(obj), obj->short_description);

  sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ", ");
  if (strstr(buf, "UNDEFINED") && (error = TRUE))
    log("SYSERR: Object #%d (%s) has unknown extra flags.",
	GET_OBJ_VNUM(obj), obj->short_description);

  sprintbits(obj->obj_flags.bitvector, affected_bits, buf, ", ");
  if (strstr(buf, "UNDEFINED") && (error = TRUE))
    log("SYSERR: Object #%d (%s) has unknown affection flags.",
	GET_OBJ_VNUM(obj), obj->short_description);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON:
   // Fall through. 
  case ITEM_FOUNTAIN:
    if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
      log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
      log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
    break;
 }

  return (error);
}

int check_object_spell_number(struct obj_data *obj, int val)
{
  int error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1)	/* i.e.: no spell */
    return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (GET_OBJ_VAL(obj, val) < 0)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > MAX_SPELLS && GET_OBJ_VAL(obj, val) <= MAX_SKILLS)
    error = TRUE;
  if (error)
    log("SYSERR: Object #%d (%s) has out of range spell #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
    log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	GET_OBJ_VNUM(obj),
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
	spell_name(GET_OBJ_VAL(obj, val)));
#endif

  if (scheck)		/* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = spell_name(GET_OBJ_VAL(obj, val));
//разобраться после!!!
 // if ((spellname == unused_spellname || str_cmp("UNDEFINED", spellname)) && (error = TRUE))
//		GET_OBJ_VNUM(obj), obj->short_description, spellname,
//		GET_OBJ_VAL(obj, val));

  return (error);
}

int check_object_level(struct obj_data *obj, int val)
{
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) && (error = TRUE))
    log("SYSERR: Object #%d (%s) has out of range level #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  return (error);
}




/* Система автоудаления 
   В этой структуре хранится информация которая определяет какаие игроки
   автоудалятся при перезагрузке мада. Уровни должны следовать в
   возрастающем порядке с невозможно малым уровнем в конце массива.
   Уровень -1 определяет удаленных угроков - т.е. через сколько времени
   они физически попуржатся.
   Если количество дней равно -1, то не удалять никогда  перенести линух
*/
struct pclean_criteria_data pclean_criteria[] = {
/*     УРОВЕНЬ           ДНИ   */
  {	-1		,0    }, /* Удаленные чары - удалять сразу */
  {	0		,0    }, /* Чары 0го уровня никогда не войдут в игру
  {	1		,5    }, /* 1 через 5 */
  {	2		,7    }, /* 2 через 7 */
  {	3		,10   }, /* 3 через 10 */
  {	4		,14   }, /* 4 через 14 */
  {	5		,18   }, // 5 через 18     
  {	10		,20   },
  {	15		,30   },
  {	20		,60   },
  {	24		,90   },
  {     LVL_IMPL	,-1   }, /* c 11го и дальше живут вечно */
  {	-2		,0    }  /* Последняя обязательная строка */
};

int must_be_deleted(struct char_data *cfu)
{ int ci, timeout;

 if (IS_SET(cfu->char_specials.saved.Act.flags[0], PLR_NODELETE))
   return (0);

 if (GET_REMORT(cfu))
   return (0);

 if (IS_SET(cfu->char_specials.saved.Act.flags[0], PLR_DELETED))
 { timeout = pclean_criteria[0].days;
   if (timeout >= 0)
   {  timeout *=SECS_PER_REAL_DAY;
     if ((time(0) - LAST_LOGON(cfu)) > timeout)
	  return (1);
      return (0);
   } 
      return (0);
   
 }
 
 timeout = -1;
 for (ci = 0; ci == 0 || pclean_criteria[ci].level > 
      pclean_criteria[ci - 1].level; ci++)
 {  if (GET_LEVEL(cfu) <= pclean_criteria[ci].level)
	{ timeout = pclean_criteria[ci].days;
      break;
	}   
 }
 if (timeout >= 0)
 { timeout *=SECS_PER_REAL_DAY;
   if ((time(0) - LAST_LOGON(cfu)) > timeout)
      return (1);
 }

 return (0);
}

/*
void convert_char(char *name) {
  FILE *player_file;
  struct char_file_u cfd;
  char filename[MAX_STRING_LENGTH];
  long size;
  int conv = 0;
  struct char_data *chr;
  
  if (get_filename(name,filename,PLAYERS_FILE) &&
  (player_file = fopen(filename,"r+b"))) {
    fseek(player_file, 0L, SEEK_END);
    size = ftell(player_file);
    if (size == sizeof(struct char_file_u)) {
      // Необходима конвертация 
      conv = 1;
      fseek(player_file, 0L, SEEK_SET);
      fread(&cfd, sizeof(struct char_file_u), 1, player_file);
    }
    fclose(player_file);
    if (conv) {
      log("Yeeee - convert char to ascii needed: %s", name);
      CREATE(chr, struct char_data, 1);
      clear_char(chr);
      store_to_char(&cfd, chr);
      new_load_mkill(chr);
      load_pkills(chr);
      // Почему то спелы скидываются при конвертации. 
      chr->real_abils = cfd.abilities;
      
      save_char(chr, GET_LOADROOM(chr));
      extract_char(chr,FALSE);
      free_char(chr);
    }
  }
}
 */


void entrycount(char *name)
{

  int  i, deleted;
  struct char_data *dummy;
  char filename[MAX_STRING_LENGTH];
        
  if (get_filename(name,filename,PLAYERS_FILE)) {

      CREATE(dummy, struct char_data, 1);
      clear_char(dummy);
      deleted = 1;
     // convert_char(name);
      if (load_char(name, dummy) > -1) {
        /* если чар удален или им долго не входили, то не создаем для него запись */
        if (!must_be_deleted(dummy)) {
	  deleted = 0;

    /* new record */
          if (player_table)
             RECREATE(player_table, struct player_index_element, top_of_p_table+2);
          else
             CREATE(player_table, struct player_index_element, 1);
          top_of_p_file++;
          top_of_p_table++;

          CREATE(player_table[top_of_p_table].name, char, strlen(GET_NAME(dummy)) + 1);
          for (i = 0, player_table[top_of_p_table].name[i] = '\0';
               (player_table[top_of_p_table].name[i] = LOWER(GET_NAME(dummy)[i])); i++);
          player_table[top_of_p_table].id     = GET_IDNUM(dummy);
          player_table[top_of_p_table].unique = GET_UNIQUE(dummy);
          player_table[top_of_p_table].level  = (GET_REMORT(dummy) ? 
	                                         30 : GET_LEVEL(dummy));
          player_table[top_of_p_table].timer  = NULL;
          if (IS_SET(dummy->char_specials.saved.Act.flags[0], PLR_DELETED))
             {player_table[top_of_p_table].last_logon = -1;
              player_table[top_of_p_table].activity   = -1;
             }
          else
             {player_table[top_of_p_table].last_logon = LAST_LOGON(dummy);
              player_table[top_of_p_table].activity   = number(0, OBJECT_SAVE_ACTIVITY - 1);
             }
          top_idnum = MAX(top_idnum, GET_IDNUM(dummy));
          log("Add new player %s",player_table[top_of_p_table].name);
        }
	free_char(dummy);
      }
	  else
	  {free(dummy);
      
	  }
      /* если чар уже удален, то стираем с диска его файл */
      if (deleted)
      {
        log("Player %s already deleted - kill player all files", name);
        remove(filename);
 get_filename(name,filename,PKILLERS_FILE);
 remove(filename);

  get_filename(name,filename,ETEXT_FILE);
  remove(filename);

  get_filename(name,filename,ALIAS_FILE);
  remove(filename);

  get_filename(name,filename,SCRIPT_VARS_FILE);
  remove(filename);   

  get_filename(name,filename,PQUESTS_FILE);
  remove(filename);

  delete_mkill_file(name);
      }
     }
  return;
}

void new_build_player_index(void)
{
  FILE *players;
  char name[MAX_INPUT_LENGTH], playername[MAX_INPUT_LENGTH];
  int  c;

  player_table  = NULL;
  top_of_p_file = top_of_p_table = -1;
  if (!(players = fopen(LIB_PLRS"players.lst","r")))
     {
       log("Players list empty...");
       return;
     }
  
  now_entrycount = TRUE;
    while (get_line(players,name))
        {if (!*name || *name == ';')
            continue;
         if (sscanf(name, "%s ", playername) == 0)
            continue;
		for (c = 0; c < top_of_p_table; c++)
             if (!str_cmp(playername, player_table[c].name))
                break;
         if (c < top_of_p_table)
            continue;   
         entrycount(playername);
        }
        
  fclose(players);
  now_entrycount = FALSE;
}


void flush_player_index(void)
{ FILE *players;
  char name[MAX_STRING_LENGTH];
  int  i, c;

  if (!(players = fopen(LIB_PLRS"players.lst","w+")))
     {log("Cann't save players list...");
      return;
     }
  for (i = 0; i <= top_of_p_table; i++)
      {if (!player_table[i].name || !*player_table[i].name)
          continue;

        // check double
      for (c = 0; c < i; c++)
           if (!str_cmp(player_table[c].name, player_table[i].name))
              break;
       if (c < i)
          continue;
      
           sprintf(name,"%s %ld %ld %d %d\n",
               player_table[i].name,
               player_table[i].id,
               player_table[i].unique,
               player_table[i].level,
               player_table[i].last_logon
              );
       fputs(name, players);
      }
  fclose(players);
  //log("Сохранено индексов %d (считано при загрузке %d)",i,top_of_p_file+1);
}

void dupe_player_index(void)
{ FILE *players;
  char name[MAX_STRING_LENGTH];
  int  i, c;
							  
  sprintf(name,LIB_PLRS"players.dup");//%ld",time(NULL));

  if (!(players = fopen(name,"w+")))
     {log("Cann't save players list...");
      return;
     }
  for (i = 0; i <= top_of_p_table; i++)
      {if (!player_table[i].name || !*player_table[i].name)
          continue;
        
       // check double
       for (c = 0; c < i; c++)
           if (!str_cmp(player_table[c].name, player_table[i].name))
              break;
       if (c < i)
          continue;
        
       sprintf(name,"%s %ld %ld %d %d\n",
               player_table[i].name,
               player_table[i].id,
               player_table[i].unique,
               player_table[i].level,
               player_table[i].last_logon
              );
        fputs(name, players);
       }
  fclose(players);
  log("Продублировано индексов %d (считано при загрузке %d)",i,top_of_p_file+1);
}

