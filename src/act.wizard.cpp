/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "mobmax.h"
#include "pk.h"
#include "clan.h"


// external vars
extern room_rnum r_helled_start_room;
extern FILE *player_fl;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct attack_hit_type attack_hit_text[];
extern time_t boot_time;
extern zone_rnum top_of_zone_table;
extern int circle_shutdown, circle_reboot, reboot_time, reboot_state;
extern int circle_restrict;
extern int load_into_inventory;
extern int max_char_to_boot;
extern int buf_switches, buf_largecount, buf_overflows;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern int top_of_p_table;
extern struct player_index_element *player_table;
extern const char *weapon_affects[];
extern unsigned long int number_of_bytes_read;
extern unsigned long int number_of_bytes_written;
extern vector < ClanRec * > clan;

// for chars
extern const char *pc_class_types[];
extern const char *npc_class_types[];
extern const char *pc_race_types[][3];
extern const char *npc_race_types[];

// extern functions
int     slot_for_char1(struct char_data *ch, int slotnum);
int     check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int extract);
int     compute_armor_class(struct char_data *ch);
int	level_exp(int chclass, int level);
void	show_shops(struct char_data * ch, char *value);
void	list_skills(CHAR_DATA * ch, CHAR_DATA * vict, bool, int);
void	list_spells(CHAR_DATA * ch, CHAR_DATA * vict);
void	appear(struct char_data *ch);
void	reset_zone(zone_rnum zone);
void	roll_real_abils(struct char_data *ch, int rollstat);
int	parse_class(int arg);
int const	parse_race(int arg);
struct  char_data *find_char(long n);
void	new_save_quests(struct char_data * ch);
int     calc_loadroom(struct char_data *ch);
void    decrease_level(struct char_data *ch);
int    _parse_name(char *arg, char *name);
int     Valid_Name(char *name);
int     reserved_word(char *argument);
long    get_ptable_by_name(char *name);
const char *how_good(int percent);

// local functions
void   rename_char(struct char_data *ch, char *oname);
int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
void do_stat_room(struct char_data * ch);
void do_stat_object(struct char_data * ch, struct obj_data * j);
void do_stat_character(struct char_data * ch, struct char_data * k);
void perform_immort_invis(struct char_data *ch, int level);
void stop_snooping(struct char_data * ch);
void perform_immort_vis(struct char_data *ch);
void print_zone_to_buf(char *bufptr, zone_rnum zone);
room_rnum find_target_room(struct char_data * ch, char *rawroomstr);

ACMD(do_echo);
ACMD(do_send);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
ACMD(do_stat);
ACMD(do_shutdown);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_liblist);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
ACMD(do_remort);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
ACMD(do_show);
ACMD(do_set);

#define MAX_TIME 0x7fffffff

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Да... но что?\r\n", ch); /*Yes.. but what*/
  else {
    if (subcmd == SCMD_EMOTE)
      sprintf(buf, "$n %s", argument);
    else
      strcpy(buf, argument);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char("OK\r\n", ch);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}


ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Что и кому вы хотите сообщить?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  send_to_char(buf, vict);
  send_to_char("\r\n", vict);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char("Сообщение отправлено.\r\n", ch);
  else {
    sprintf(buf2, "Вы сообщаете \"%s\" %s.\r\n", buf, GET_TNAME(vict));
    send_to_char(buf2, ch);
  }
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data * ch, char *rawroomstr)
{
  room_vnum tmp;
  room_rnum location;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char("Вы должны назвать номер комнаты или имя.\r\n", ch);
    return (NOWHERE);
  }
  if (isdigit((unsigned char)*roomstr) && !strchr(roomstr, '.')) {
    tmp = atoi(roomstr);
    if ((location = real_room(tmp)) < 0) {
      send_to_char("Такой номер комнаты не существует.\r\n", ch);
      return (NOWHERE);
    }
  } else if ((target_mob = get_char_vis(ch, roomstr, FIND_CHAR_WORLD)) != NULL)
    location = target_mob->in_room;
  else if ((target_obj = get_obj_vis(ch, roomstr)) != NULL) {
    if (target_obj->in_room != NOWHERE)
      location = target_obj->in_room;
    else {
      send_to_char("Такого объекта нет.\r\n", ch);
      return (NOWHERE);
    }
  } else {
    send_to_char("Здесь нет никого с таким именем.\r\n", ch);
    return (NOWHERE);
  }

  /* a location has been found -- if you're < GRGOD, check restrictions. */
  if (!IS_IMMORTAL(ch)) {
    if (ROOM_FLAGGED(location, ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GOD) {
      send_to_char("Вы не достигли такой Божественности, что бы использовать эту команду!\r\n", ch);
      return (NOWHERE);
    }
    if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
	world[location].people && world[location].people->next_in_room) {
      send_to_char("В этой комнате Вы явно лишний.\r\n", ch);
      return (NOWHERE);
    }
    if (ROOM_FLAGGED(location, ROOM_HOUSE))
    { send_to_char("Это чужой дом!\r\n", ch);
      return (NOWHERE);
    }
  }
  return (location);
}



ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char("Вы должны назвать номер комнаты или имя.\r\n", ch);
    return;
  }

  if (!*command) {
    send_to_char("Что вы хотите сделать?\r\n", ch);
    return;
  }

  if ((location = find_target_room(ch, buf)) < 0)
    return;

  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}


ACMD(do_goto)
{
  room_rnum location;

  if ((location = find_target_room(ch, argument)) < 0)
    return;

  if (!GET_COMMSTATE(ch))
	 { if (POOFOUT(ch))
			sprintf(buf, "$n %s",POOFOUT(ch));
		else
			strcpy(buf, "$n растворил$u в клубах дыма.");
	 }
  else	 
	  strcpy(buf, "$n взял$y свиток возврата в левую рукую.\r\n"
						  "$n зачитал$y свиток возврата.\r\n"
						  "$n исчез$q.");


  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);
  check_horse(ch);
  
  if (!GET_COMMSTATE(ch) && POOFIN(ch))
    sprintf(buf, "$n %s", POOFIN(ch));
     else
        strcpy(buf, "$n появил$u посреди комнаты.");
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
        look_at_room(ch, 0);
}



ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Кого Вы хотите перенести?\r\n", ch);
  else if (str_cmp("все", buf)) {
    if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
      send_to_char(NOPERSON, ch);
    else if (victim == ch)
      send_to_char("Какой смысл в этом?\r\n", ch);
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char("Передача своих полномочий.\r\n", ch);
	return;
      }
      act("$n растворил$u на атомы.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, ch->in_room);
      act("$n появил$u в клубах дыма.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n перенес$q Вас!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(victim, 0);
    }
  } else {			/* Trans All */
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char("Я думаю нет.\r\n", ch);
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
	victim = i->character;
	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	  continue;
	act("$n растворил$u на атомы.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, ch->in_room);
	act("$n появил$u в клубах дыма.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n перенес$y Вас!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
      }
    send_to_char(OK, ch);
  }
}



ACMD(do_teleport)
{
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char("Кого Вы хотите телепортировать?\r\n", ch);
  else if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
    send_to_char(NOPERSON, ch);
  else if (victim == ch)
    send_to_char("Используйте команду 'goto', что бы телепортировать себя.\r\n", ch);
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char("Вы не можете это сделать.\r\n", ch);
  else if (!*buf2)
    send_to_char("Куда Вы хотите телепортировать этот персонаж?\r\n", ch);
  else if ((target = find_target_room(ch, buf2)) >= 0) {
   send_to_char(OK, ch);
    act("$n растворил$u в клубах дыма.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n появил$u в клубах дыма.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n телепортировал$y Вас!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(victim, 0);
  }
}



ACMD(do_vnum)
{
  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "моб") && !is_abbrev(buf, "пре")
	  && !is_abbrev(buf, "найти") && !is_abbrev(buf, "аффект"))) {
    send_to_char("Формат: вном { пре(дмет) | моб | найти | аффект} <имя>\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "моб"))
    if (!vnum_mobile(buf2, ch))
      send_to_char("Здесь нет моба с таким именем.\r\n", ch);

  if (is_abbrev(buf, "пре"))
    if (!vnum_object(buf2, ch))
      send_to_char("Здесь нет предмета с таким названием.\r\n", ch);
  if (is_abbrev(buf, "найти"))
    if (!vnum_search(buf2, ch))
      send_to_char("Нет предмета с таким модификатором.\r\n", ch);
 if (is_abbrev(buf, "аффект"))
    if (!vnum_affect(buf2, ch))
      send_to_char("Нет предмета с таким аффектом.\r\n", ch);
}


void do_stat_room(struct char_data * ch)
{
  struct extra_descr_data *desc;
  struct room_data *rm = &world[ch->in_room];
  int i, found;
  struct obj_data *j;
  struct char_data *k;

  sprintf(buf, "Название комнаты: %s%s%s.\r\n", CCCYN(ch, C_NRM), rm->name,
	  CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprinttype(rm->sector_type, sector_types, buf2);
  sprintf(buf, "Зона: [%3d], VNum: [%s%5d%s], RNum: [%5d], Тип: %s\r\n",
	  zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number,
	  CCNRM(ch, C_NRM), ch->in_room, buf2);
  send_to_char(buf, ch);

   sprintbits(rm->room_flags, room_bits, buf2, ", ");
  sprintf(buf, "Спецпрограмма: %s. Флаги: %s\r\n",
	  (rm->func == NULL) ? "Нет" : "Выходы", buf2);
  send_to_char(buf, ch);

  send_to_char("                        &BОписание комнаты:&n\r\n", ch);
  if (rm->description)
    send_to_char(rm->description, ch);
  else
    send_to_char("Нет.\r\n", ch);

  if (rm->ex_description) {
    sprintf(buf, "Дополнительное описание:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  sprintf(buf, "Персонажи в комнате:%s", CCYEL(ch, C_NRM));
  for (found = 0, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;
    sprintf(buf2, "%s %s (%s)", found++ ? "," : "", GET_NAME(k),
	    (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (k->next_in_room)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);
  send_to_char(CCNRM(ch, C_NRM), ch);

  if (rm->contents) {
    sprintf(buf, "Предметы:%s", CCGRN(ch, C_NRM));
    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;
      sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (rm->dir_option[i]) {
      if (rm->dir_option[i]->to_room == NOWHERE)
	sprintf(buf1, " %sНЕТ%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
      else
	sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
	GET_ROOM_VNUM(rm->dir_option[i]->to_room), CCNRM(ch, C_NRM));
        sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
        sprintf(buf, "Выход %s%-5s%s:  К: [%s]. Ключ: [%5d]. Название: %s (%s). Тип: %s\r\n ",
	CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1, rm->dir_option[i]->key,
	rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "Нет(дверь)",
	rm->dir_option[i]->vkeyword ? rm->dir_option[i]->vkeyword : "Нет(дверь)", buf2);
      send_to_char(buf, ch);
      if (rm->dir_option[i]->general_description)
	strcpy(buf, rm->dir_option[i]->general_description);
      else
	strcpy(buf, "  Нет описания выходов.\r\n");
      send_to_char(buf, ch);
    }
  }
/* check the room for a script */
 do_sstat_room(ch);
}



void do_stat_object(struct char_data * ch, struct obj_data * j)
{
  int i, found;
  obj_vnum vnum, rnum;
  struct obj_data *j2;
  struct extra_descr_data *desc;

  vnum = GET_OBJ_VNUM(j);
  rnum = GET_OBJ_RNUM(j);
  sprinttype(GET_OBJ_SEX(j), genders, buf1);
  sprintf(buf, "Имя: \"%s%s%s\". Алиасы: \"\\c09%s\\c00\".  Пол: &C%s&n\r\n", CCYEL(ch, C_NRM),
	  ((j->short_description) ? j->short_description : "<\\c08Нет\\c00>"),
	  CCNRM(ch, C_NRM), j->name, buf1);
  send_to_char(buf, ch);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
  if (GET_OBJ_RNUM(j) >= 0)
    strcpy(buf2, (obj_index[GET_OBJ_RNUM(j)].func ? "\\c11Есть\\c00" : "\\c08Нет\\c00"));
  else
    strcpy(buf2, "Нет");
  sprintf(buf, "VNum: [%s%5d%s], RNum: [%5d], Тип: %s. Спец программа: %s\r\n",
   CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), buf1, buf2);
  send_to_char(buf, ch);

  if (GET_OBJ_RLVL(j))
	{ sprintf(buf, "Уровень предмета для генерации рандомстатов: %d\r\n", GET_OBJ_RLVL(j));
	  send_to_char(buf, ch);
	}

  	if (GET_OBJ_OWNER(j))
	{ sprintf(buf, "Владелец : %s, ", get_name_by_unique(GET_OBJ_OWNER(j)));
	  send_to_char(buf, ch);
	}

  sprintf(buf, "L-Des: %s\r\n", ((j->description) ? j->description : "\\c08Нет\\c00"));
  send_to_char(buf, ch);

  if (j->ex_description) {
    sprintf(buf, "Расширенное описание:%s", CCCYN(ch, C_NRM));
    for (desc = j->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
       if (desc->keyword)
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  send_to_char("Может использоваться как: ", ch);
  sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
   
  if (str_cmp(skill_name(j->obj_flags.tren_skill),"UNDEFINED"))
  { sprintf(buf, "Тренируемый скилл: %s. \r\n", skill_name(j->obj_flags.tren_skill)); 
   send_to_char(buf,ch);
  }
  sprintbits(j->obj_flags.no_flag, no_bits, buf, ", ");
  sprintf(buf1, "Флаги неудобств :&C%24s\r\n&n", buf);
  send_to_char(buf1, ch);
 
  sprintbits(j->obj_flags.anti_flag, no_bits, buf, ", ");
  sprintf(buf1, "Флаги запретов :&C%25s\r\n&n", buf);
  send_to_char(buf1, ch);
 
  sprintbits(j->obj_flags.affects, weapon_affects, buf, ", ");
  sprintf(buf1, "Устанавливает аффекты :&C%18s\r\n&n", buf);
  send_to_char(buf1, ch);
  
  sprintbits(j->obj_flags.bitvector, affected_bits, buf, ", ");
  sprintf(buf1, "Флаги на игрока :&C%24s\r\n&n", buf);
  send_to_char(buf1, ch);

  sprintbits(j->obj_flags.extra_flags, extra_bits, buf, ", ");
  sprintf(buf1, "Дополнительный флаг :&C%20s\r\n&n", buf);
  send_to_char(buf1, ch);

  sprintf(buf, "Вес: %d. Цена: %d. Экипирован/в инвентаре (в день): %d/%d.\r\nТаймер: %d. Время до исчезновения %d\r\n",
     GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENTE(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), GET_OBJ_DESTROY(j));
  send_to_char(buf, ch);

  
  strcpy(buf, "В комнате: ");
  if (j->in_room == NOWHERE)
    strcat(buf, "нигде");
  else {
    sprintf(buf2, "%d", GET_ROOM_VNUM(IN_ROOM(j)));
    strcat(buf, buf2);
  }
  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  strcat(buf, ". В объекте: ");
  strcat(buf, j->in_obj ? j->in_obj->short_description : "Ничего");
  strcat(buf, ". В инвентаре: ");
  strcat(buf, j->carried_by ? GET_RNAME(j->carried_by) : "Нет");
  strcat(buf, ". В экипировке: ");
  strcat(buf, j->worn_by ? GET_RNAME(j->worn_by) : "Нет");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, 0) == -1)
      strcpy(buf, "Оставшееся время света: Вечный свет");
    else
      sprintf(buf, "Оставшееся время света: [%d]", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    sprintf(buf, "Заклинания: (Уровень %d) %s, %s, %s", GET_OBJ_VAL(j, 0),
	    spell_name(GET_OBJ_VAL(j, 1)), spell_name(GET_OBJ_VAL(j, 2)),
	    spell_name(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    sprintf(buf, "Заклинания: %s (Уровень %d), %d (из %d) осталось зарядов",
	    spell_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 0),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_WEAPON:
      sprintf(buf, "Наносимое повреждение: %dd%d. Среднее наносимое повреждение : %.1f.\r\nВыдержит повреждений : %d. Осталось выдержать повреждений: %d.\r\nСообщение атаки: %d (%s)",
	    GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), (((GET_OBJ_VAL(j, 2) + 1) / 2.0) * GET_OBJ_VAL(j, 1)),
	    GET_OBJ_MAX(j), GET_OBJ_CUR(j), GET_OBJ_VAL(j, 3), attack_hit_text[GET_OBJ_VAL(j, 3)]);
       break;
  
 case ITEM_ARMOR:
    sprintf(buf, "добавляет к защите (AC): [%d], добавляет к броне (ARMOUR) [%d]", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
     
    break;
  case ITEM_TRAP:
    sprintf(buf, "Spell: %d, - Hitpoints: %d",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_CONTAINER:
    sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
    sprintf(buf, "Вместимость контейнера: %d. Тип блокировки: %s. Номер ключа: %d. Corpse: %s",
	    GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2),
	    YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
    sprintf(buf, "Емкость: %d. Контейнер: %d. Яд: %s. Жидкость: %s",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), YESNO(GET_OBJ_VAL(j, 3)),
	    buf2);
    break;
  case ITEM_NOTE:
    sprintf(buf, "Язык: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY:
    strcpy(buf, "");
    break;
  case ITEM_FOOD:
    sprintf(buf, "Степень насыщения: %d. Отравленный: &G%d&n", GET_OBJ_VAL(j, 0),
	    /*YESNO*/(GET_OBJ_VAL(j, 1))); // заменил с 3 на 1 - в редакторе для еды уровень яда
    break;
  case ITEM_MONEY:
    sprintf(buf, "Монет: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_BOOK:
      if (GET_OBJ_VAL(j, 1) >= 1 && GET_OBJ_VAL(j, 1) < MAX_SPELLS)
         {  sprintf(buf, "содержит заклинание        : \"%s\" [%d]\r\n",
		     spell_info[GET_OBJ_VAL(j,1)].name, GET_OBJ_VAL(j,1));
            //изменить
	        sprintf(buf+ strlen(buf), "Для Вас это заклинание &C%d&n круга.\r\n",
			 spell_info[GET_OBJ_VAL(j,1)].slot_forc[(int) GET_CLASS(ch)]);
          }
      break;
  default:
        sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (j->contains) {
    sprintf(buf, "\r\nКонтейнер:%s", CCGRN(ch, C_NRM));
    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j2->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  found = 0;
  send_to_char("Аффекты:", ch);
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      sprinttype(j->affected[i].location, apply_types, buf2);
      sprintf(buf, "%s %+d - %s", found++ ? "," : "",
	      j->affected[i].modifier, buf2);
      send_to_char(buf, ch);
    }
  if (!found)
    send_to_char(" Нет", ch);

  send_to_char("\r\n", ch);
  sprintf(buf,"Всего объектов: [&Wв игре &c%d&n][&Wна сохранении &R%d&n]\r\nМаксимальное количество объектов в Мире : [&C%d&n].\r\n",
  rnum >= 0 ? obj_index[rnum].number : -1, rnum >= 0 ? obj_index[rnum].stored : -1, GET_OBJ_MIW(j));
  send_to_char(buf,ch);	

// проверка объекта на наличие скрипта (триггера)
  do_sstat_object(ch, j);
}


void do_stat_character(struct char_data * ch, struct char_data * k)
{
  int i, i2, found = 0;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;

  sprinttype(GET_SEX(k), genders, buf);
  sprintf(buf2, " %s '%s'  IDNum: [%5ld], In room [%5d]\r\n",
	  (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
	  GET_NAME(k), GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)));
  send_to_char(strcat(buf, buf2), ch);
  if (IS_MOB(k)) {
    sprintf(buf, "Алиасы: \\c06%s\\c00. VNum: [%5d], RNum: [%5d]\r\n",
	    k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
    send_to_char(buf, ch);
  	sprintf(buf, "Описание: %s", (k->player.long_descr ? k->player.long_descr : "<нет>\r\n"));
    send_to_char(buf, ch);
	strcpy(buf, "Класс Монстра: \\c06");
    sprinttype(k->player.chclass, npc_class_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\\c00. Раса Монстра: \\c06"); 
    sprinttype(GET_RACE(k), npc_race_types, buf2);
    sprintf(buf2 + strlen(buf2),"\\c00. Размер: \\c06[%2d]\\c00.", GET_REAL_SIZE(k));
  } else {
          sprintf(buf,"Номер : %d. Где родился %d. E-mail почта : %s\r\n",
		  GET_UNIQUE(k), GET_LOADROOM(k), GET_EMAIL(k));
          send_to_char(buf,ch);
	  if (PLR_FLAGGED(k, PLR_HELLED) && HELL_DURATION(k))
		{ sprintf(buf, "Находится в АДУ : %ld час. [%s]\r\n",
	      (HELL_DURATION(k) - time(NULL)) / 3600, HELL_REASON(k)?HELL_REASON(k):"-");
          send_to_char(buf, ch);
		}
	if (PLR_FLAGGED(k, PLR_NAMED) && NAME_DURATION(k))
		{ sprintf(buf, "Находится в комнате имени : %ld час\r\n",
	          (NAME_DURATION(k) - time(NULL)) / 3600);
          send_to_char(buf, ch);
		}
      if (PLR_FLAGGED(k, PLR_MUTE) && MUTE_DURATION(k))
         {sprintf(buf, "Будет молчать : %ld час. %ld мин. [%s]\r\n",
	          (MUTE_DURATION(k) - time(NULL)) / 3600, (HELL_DURATION(k) - time(NULL) % 3600 + 59) / 60, 
			   MUTE_REASON(k)?MUTE_REASON(k):"-");
          send_to_char(buf, ch);
	 }
      if (PLR_FLAGGED(k, PLR_DUMB) && DUMB_DURATION(k))
         {sprintf(buf, "Будет нем : %ld мин. [%s]\r\n",
	          (DUMB_DURATION(k) - time(NULL)) / 60, MUTE_REASON(k)?MUTE_REASON(k):"-");
          send_to_char(buf, ch);
	 }
      if (GET_GOD_FLAG(k, GF_GODSLIKE) && GODS_DURATION(k))
         {sprintf(buf, "Под защитой Богов : %ld час.\r\n",
	          (GODS_DURATION(k) - time(NULL)) / 3600);
          send_to_char(buf, ch);
	 }
      if (GET_GOD_FLAG(k, GF_GODSCURSE) && GODS_DURATION(k))
         {sprintf(buf, "Проклят Богами : %ld час.\r\n",
	          (GODS_DURATION(k) - time(NULL)) / 3600);
          send_to_char(buf, ch);
	 }
	sprintf(buf, "Титул: %s&n\r\n", (k->player.title ? k->player.title : "<нет>"));
    send_to_char(buf,ch);
	sprintf(buf, "Предтитул: %s&n\r\n", (k->player.ptitle ? k->player.ptitle : "<нет>"));
    send_to_char(buf,ch);
    sprintf(buf, "Имя в падежах: %s%s/%s/%s/%s/%s%s\r\n",CCCYN(ch,C_CMP),
    k->player.rname, k->player.dname, k->player.vname, k->player.tname,
    k->player.pname, CCNRM(ch,C_CMP));
    sprintf(buf + strlen(buf),"Убитых мобов - \\c06%d\\c00. ", k->player_specials->saved.spare19); 
    sprintf(buf + strlen(buf),"Убитых игроков - \\c06%d\\c00. ", k->player_specials->saved.spare9);
    sprintf(buf + strlen(buf),"Смертей - \\c06%d\\c00.\r\n", k->player_specials->saved.spare12);
	strcat(buf, "Класс: ");
    sprinttype(GET_CLASS(k), pc_class_types, buf2);
    strcat(buf, buf2);
    strcat(buf, ". Раса: ");
    sprintf(buf2, "%s", pc_race_types[GET_RACE(k)][GET_SEX(k)]);
	sprintf(buf2 + strlen(buf2),". Размер: [%2d]. Ремортов: [%2d]. Право на реморт: [%d].", GET_REAL_SIZE(k), GET_REMORT(k),
         !GET_GOD_FLAG(k, GF_REMORT) ? 0 : 1);
        if((i=find_clan_by_id(GET_CLAN(k)))>=0)
	  sprintf(buf2 + strlen(buf2),"\r\nПринадлежит к клану: %s, Ранг в клане: %d, ИД клана: %d", clan[i]->name, GET_CLAN_RANK(k), GET_CLAN(k));
  }
  strcat(buf, buf2);
  sprintf(buf2, "\r\nУровень: [%s%2d%s]. EXP: [%s%7d%s]. Наклонность: [&Y%4d&n]. Максфактор [&Y%d&n]\r\n",
	  CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
	  CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
	  GET_ALIGNMENT(k), k->mob_specials.max_factor);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
      strcpy(buf1, (char *) asctime(localtime(&(k->player_specials->time.birth))));
      strcpy(buf2, (char *) asctime(localtime(&(k->player_specials->time.logon))));
      buf1[10] = buf2[10] = '\0';

    sprintf(buf, "Создан: [%s]. Вход в систему: [%s]. Играет: [%dh %dm]. Возраст: [%d]\r\n",
	    buf1, buf2, k->player_specials->time.played / 3600,
	    ((k->player_specials->time.played % 3600) / 60), age(k)->year);
    send_to_char(buf, ch);

	if (IS_IMPL(ch) && !strncmp(GET_NAME(ch),"Натан",5)) {
	   sprintf(buf,"\\c04Пароль %s: [%s].\\c00\r\n", GET_RNAME(k), GET_PASSWD(k));
		send_to_char(buf, ch);
	}
         sprintf(buf, "Родной город: [%d], Speaks: [%d/%d/%d], (STL[%d]/NSTL[%d]), &W[ОИМ/ПИМ/МДВК %d&C/&W%d&C/&W%d]&n\r\n",
	 GET_HOME(k), GET_TALK(k, 0), GET_TALK(k, 1), GET_TALK(k, 2),
	    int_app[GET_INT(k)].observation, wis_app[GET_WIS(k)].max_skills,
		IND_POWER_CHAR(k), IND_SHOP_POWER(k), POWER_STORE_CHAR(k));
    send_to_char(buf, ch);
  }
  sprintf(buf, "Str: [%s%d/%d/%d%s] Int: [%s%d/%d/%d%s] Wis: [%s%d/%d/%d%s] "
	  "Dex: [%s%d/%d/%d%s] Con: [%s%d/%d/%d%s] Lck: [%s%d/%d/%d%s]\r\n",
	  CCCYN(ch, C_NRM), GET_STR(k), GET_REAL_STR(k), GET_STR_ROLL(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_INT(k), GET_REAL_INT(k), GET_INT_ROLL(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_WIS(k), GET_REAL_WIS(k), GET_WIS_ROLL(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_DEX(k), GET_REAL_DEX(k), GET_DEX_ROLL(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_CON(k), GET_REAL_CON(k), GET_CON_ROLL(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_CHA(k), GET_REAL_CHA(k), GET_CHA_ROLL(k), CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprintf(buf, "Жизни p.:[%s%d/%d+%d%s] Mana p.:[%s%d/%d+%d%s] Энергии p.:[%s%d/%d+%d%s] Усталость/Таймер p.:[&C%d/%d&n]\r\n",
	  CCGRN(ch, C_NRM), GET_HIT(k), GET_REAL_MAX_HIT(k), hit_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k), mana_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM),
	  GET_USTALOST(k), GET_UTIMER(k));
  send_to_char(buf, ch);

  sprintf(buf, "Монет: [%9d], Банк: [%9d] (Всего: %d), Слава: [%9d/%d]\r\n",
	  GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k), GET_GLORY(k), GET_QUESTPOINTS(k));
          send_to_char(buf, ch);

 	      sprintf(buf, "AC: [%d/%d(%d)], Броня: [%d], Попадания: [%2d/%2d/%d], Повреждения: [%2d/%2d/%d],\r\n",
	      GET_AC(k), GET_REAL_AC(k), compute_armor_class(k),
	      GET_ARMOUR(k),//по сути армор не используется, подумать, может для чего нить другого поюзать
	      GET_HR(k), GET_REAL_HR(k), GET_REAL_HR(k) + str_app[GET_REAL_STR(k)].tohit,
	      GET_DR(k), GET_REAL_DR(k), GET_REAL_DR(k) + str_app[GET_REAL_STR(k)].todam);
	      send_to_char(buf, ch);


  	sprintf(buf,
		"Спас броски: [Воля:%2d Живучесть:%2d Стойкость:%2d Реакция:%2d], Мораль: %2d, Инициатива: %2d, Шанкаст: %2d\r\n",
		GET_SAVE(k, SAVING_WILL), GET_SAVE(k, SAVING_CRITICAL), GET_SAVE(k, SAVING_STABILITY), GET_SAVE(k, SAVING_REFLEX),
		GET_MORALE(k), GET_INITIATIVE(k), GET_CAST_SUCCESS(k));
	send_to_char(buf, ch);
	sprintf(buf,
		"Сопротивления: [&RОгонь:%2d &WВоздух:%2d &bВода:%2d &cЗемля:%2d &GЖивучесть:%2d &yРазум:%2d &YИммунитет:%2d&n]\r\n",
		GET_RESIST(k, 0), GET_RESIST(k, 1), GET_RESIST(k, 2), GET_RESIST(k, 3),
		GET_RESIST(k, 4), GET_RESIST(k, 5), GET_RESIST(k, 6));
	send_to_char(buf, ch);
	sprintf(buf,
		"Защита от магических аффектов : [%d], Защита от магических повреждений : [%d]\r\n", GET_ARESIST(k), GET_DRESIST(k));
	send_to_char(buf, ch);

	sprintf(buf, "ПлюсМем: [%d], Хитрег: [%d], Муврег: [%d], Поглощение: [%d]\r\n",
		          GET_MANAREG(k), GET_HITREG(k), GET_MOVEREG(k), GET_ABSORBE(k));
	send_to_char(buf, ch);

          
  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf, "Положение при загрузке: %s. Сражается: %s.", buf2,
	  (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Нет"));

  if (IS_NPC(k)) {
    strcat(buf, " Описание Атаки: ");
    strcat(buf, attack_hit_text[k->mob_specials.attack_type].plural);
    strcat(buf, "$Y");
  }  
  act(buf, FALSE, ch, 0, k, TO_CHAR);

  strcpy(buf, "Положение по умолчанию: ");
  sprinttype((k->mob_specials.default_pos), position_types, buf2);
  strcat(buf, buf2);

  sprintf(buf2, ", Таймер разъединения игрока (в тиках) [%d]\r\n", k->char_specials.timer);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (IS_NPC(k)) {
    sprintbits(k->char_specials.saved.Act, action_bits, buf2, ", ");
    sprintf(buf, "Флаги монстра (NPC): %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbits(k->mob_specials.Npc_Flags, special_bits, buf2, ", ");
    sprintf(buf, "Спецфлаги монстра (NPC): %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintf(buf, "Для скакуна, количество ходов - %d\r\n", GET_HORSESTATE(k));
    send_to_char(buf, ch);
  } else {
    sprintbits(k->char_specials.saved.Act, player_bits, buf2, ", ");
    sprintf(buf, "Флаги игрока (PLR): %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbit(PRF_FLAGS(k), preference_bits, buf2);
    sprintf(buf, "Флаг режима игрока (PRF): %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
     if (IS_IMPL(ch))
		{ sprintf(buf, "Персональный лог игрока - %s&n\r\n", GET_GOD_FLAG(k, GF_PERSLOG) ? "&CВКЛ" : "&RВЫКЛ");
		  send_to_char(buf, ch);
		}
  }

  if (IS_MOB(k)) {
       sprintf(buf, "Спецпрограмма: %s. Наносимое повреждение руками: %dd%d\r\n",
	    (mob_index[GET_MOB_RNUM(k)].func ? "Есть" : "Нет"),
	    k->mob_specials.damnodice, k->mob_specials.damsizedice);
    send_to_char(buf, ch);
  }
  sprintf(buf, "Несет: Вес: %d. Предметов: %d. ",
	  IS_CARRYING_W(k), IS_CARRYING_N(k));

  for (i = 0, j = k->carrying; j; j = j->next_content, i++);
  sprintf(buf + strlen(buf), "Предметы в инвентаре: %d. ", i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      i2++;
  sprintf(buf2, "Экипировка: %d. \r\n", i2);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    sprintf(buf, "Голод: %d. Жажда: %d. Опъянение: %d\r\n",
	  GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
    send_to_char(buf, ch);
  }

  sprintf(buf, "Лидер группы: %s. Состав группы:",
	  ((k->master) ? GET_NAME(k->master) : "<никого>"));

  for (fol = k->followers; fol; fol = fol->next) {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (fol->next)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);

  // Showing the bitvector 
  sprintbits(k->char_specials.saved.affected_by, affected_bits, buf2, ", ");
  sprintf(buf, "Аффекты: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  // Routine to show what spells a char is affected by
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      *buf2 = '\0';
      sprintf(buf, "Спеллы: (%3dhr) %s%-21s%s ", aff->duration + 1,
	      CCCYN(ch, C_NRM), spell_name(aff->type), CCNRM(ch, C_NRM));
      if (aff->modifier) {
	sprintf(buf2, "%+d на %s", aff->modifier, apply_types[(int) aff->location]);
	strcat(buf, buf2);
      }
      if (aff->bitvector) {
	if (*buf2)
	  strcat(buf, ", установлен ");
	else
	  strcat(buf, "установлен ");
	sprintbit(aff->bitvector, affected_bits, buf2);
	strcat(buf, buf2);
      }
      send_to_char(strcat(buf, "\r\n"), ch);
    }
  }

  /* check mobiles for a script */
  if (IS_NPC(k))
     {do_sstat_character(ch, k);
      if (MEMORY(k))
		{ struct memory_rec_struct *memchar;
          send_to_char("&CПомнит:&n\r\n",ch);
	  for (memchar = MEMORY(k); memchar; memchar = memchar->next)
			{ struct char_data *mchar = find_char(memchar->id);
				if (mchar)
				{ i = ((memchar->time - time(NULL)) % 3600 + 59) / 60;	
				sprintf(buf,"&R%s&n - %d\r\n",GET_VNAME(mchar), i);
				send_to_char(buf,ch);
				}
			}
	  }
      if (SCRIPT_MEM(k))
         {struct script_memory *mem = SCRIPT_MEM(k);
          send_to_char("Память (скрипт):\r\n  Игрок                Команда\r\n", ch);
          while (mem)
                {struct char_data *mc = find_char(mem->id);
                 if (!mc)
                    send_to_char("  ** Разрушено!\r\n", ch);
                 else
                    {if (mem->cmd)
                        sprintf(buf,"  %-20.20s%s\r\n",GET_NAME(mc),mem->cmd);
                     else
                        sprintf(buf,"  %-20.20s <default>\r\n",GET_NAME(mc));
                     send_to_char(buf, ch);
                    }
                 mem = mem->next;
                }
         }
     }
  else
     {/* this is a PC, display their global variables */
      if (k->script && k->script->global_vars)
         {struct trig_var_data *tv;
          char name[MAX_INPUT_LENGTH];
          void find_uid_name(char *uid, char *name);
           send_to_char("Глобальные переменные:\r\n", ch);
          /* currently, variable context for players is always 0, so it is */
          /* not displayed here. in the future, this might change */
          for (tv = k->script->global_vars; tv; tv = tv->next)
              {if (*(tv->value) == UID_CHAR)
                  {find_uid_name(tv->value, name);
                   sprintf(buf, "    %10s:  [UID]: %s\r\n", tv->name, name);
                  }
               else
                  sprintf(buf, "    %10s:  %s\r\n", tv->name, tv->value);
               send_to_char(buf, ch);
              }
         }
		if (k->Questing.count)
         {send_to_char("Выпоненные квесты © : \r\n",ch);
          *buf = '\0';
              for (i = 0; i < k->Questing.count && strlen(buf) + 80 < MAX_STRING_LENGTH; i++)
                sprintf(buf + strlen(buf), "%-8d", k->Questing.quests[i]);
             
          strcat(buf,"\r\n");
          send_to_char(buf,ch);
         }

        pk_list_sprintf(k, buf);
        send_to_char(buf,ch);
  
  
  }
}

ACMD(do_stat)
{
  struct char_data *victim;
  struct obj_data *object;
 // struct char_file_u tmp_store;
  int tmp;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char("Информация на кого или что?\r\n", ch);
    return;
  } else if (is_abbrev(buf1, "рум")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "моб")) {
    if (!*buf2)
      send_to_char("Характеристики на какого моба?\r\n", ch); 
    else {
      if ((victim = get_char_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char("Здесь нет моба с таким именем.\r\n", ch); 
    }
  } else if (is_abbrev(buf1, "игрок")) {
    if (!*buf2) {
      send_to_char("Характеристика на какого игрока?\r\n", ch); 
    } else {
      if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char("Здесь нет игрока с таким именем.\r\n", ch);
     }
  } else if (is_abbrev(buf1, "файл")) {
    if (!*buf2) {
      send_to_char("Характеристика на какого игрока?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
    if (load_char(buf2, victim) > -1) {
	      load_pkills(victim);
	  /* if (load_char(buf2, &tmp_store) > -1) {
	store_to_char(&tmp_store, victim);
	victim->player.time.logon = tmp_store.last_logon;
	char_to_room(victim, 0);*/
	if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char("Извините, Вы не можете этого сделать.\r\n", ch); 
	else
	  do_stat_character(ch, victim);
	extract_char(victim, FALSE);
      } else {
	send_to_char("Нет такого игрока.\r\n", ch);
	free(victim);
      }
    }
  } else if (is_abbrev(buf1, "пре")) {
    if (!*buf2)
      send_to_char("Характеристика на какой объект?\r\n", ch); 
    else {
      if ((object = get_obj_vis(ch, buf2)) != NULL)
	do_stat_object(ch, object);
      else
	send_to_char("Здесь нет такого объекта.\r\n", ch); 
    }
  } else {
    if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)) != NULL)
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_ROOM)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room].contents)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, buf1)) != NULL)
      do_stat_object(ch, object);
    else
      send_to_char("Нет никого с таким именем.\r\n", ch); 
  }
}


ACMD(do_shutdown)
{
    int times = 0;
	
	if (subcmd != SCMD_SHUTDOWN) {
    send_to_char("Если вы хотите что-либо закрыть, так и скажите!\r\n", ch); 
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Завершение работы.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Перезагрузка...... подождите 5 секунд.\r\n"); 
    touch(FASTBOOT_FILE);
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Завершение работы для сохранения.\r\n"); 
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Приостановление работы.\r\n"); 
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "таймер")) {
    sprintf(buf, "(GC) Timed Reboot by %s.", GET_NAME(ch));
    log(buf);
    reboot_state = 1;
sprintf(buf, "\\c04                   [ВНИМАНИЕ!!! СЕЙЧАС БУДЕТ ПРОИЗВЕДЕНА ПЕРЕЗАГРУЗКА ИГРЫ!]\\c00\r\n", reboot_time);
			send_to_all(buf);
  } else if (!str_cmp(arg, "остановить")) {
	  if (!reboot_state) {
		  send_to_char ("Уже все остановлено! \r\n", ch);
		return;
	  }	  
		  reboot_state = 0;
			  reboot_time  = 6;
sprintf(buf, "\\c04                       [!!! ПЕРЕЗАГРУЗКА ОСТАНОВЛЕНА !!!]\\c00\r\n", reboot_time);
			send_to_all(buf);
  } else
    send_to_char("Неизвестный параметр для завершения работы.\r\n", ch);
}


void stop_snooping(struct char_data * ch)
{
  if (!ch->desc->snooping)
    send_to_char("Вы ни за кем не следите.\r\n", ch);
  else {
    send_to_char("Вы прекратили следить.\r\n", ch);
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}


ACMD(do_snoop)
{
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
    send_to_char("Здесь нет никого с таким именем.\r\n", ch);
  else if (!victim->desc)
    send_to_char("Сейчас нет связи... Не за кем следить.\r\n", ch);
  else if (victim == ch)
    stop_snooping(ch);
  else if (victim->desc->snoop_by)
    send_to_char("Уже занят. \r\n", ch);
  else if (victim->desc->snooping == ch->desc)
    send_to_char("Не будте так глупы!!!\r\n", ch);
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
      send_to_char("Вы не можете.\r\n", ch);
      return;
    }
  send_to_char(OK, ch);

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}



ACMD(do_switch)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char("Вы и так уже в другом теле.\r\n", ch);
  else if (!*arg)
    send_to_char("Вселиться в кого?\r\n", ch);
  else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
    send_to_char("Нет такого персонажа.\r\n", ch);
  else if (ch == victim)
    send_to_char("Кхе кхе... Мы что сегодня прикалываемся, а?\r\n", ch); 
  else if (victim->desc)
    send_to_char("Вы не можете этого сделать, так как тело уже используется!\r\n", ch);
  else if (!IS_IMPL(ch) && !IS_NPC(victim))
    send_to_char("Вы не достаточно божествены, что бы вселится в игрока.\r\n", ch); 
  else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
    send_to_char("Это могут делать только Боги!\r\n", ch);
  else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE))
    send_to_char("Вы не можете попасть на частную собственность!\r\n", ch);
  else {
    send_to_char(OK, ch);

    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}


ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char("Вы вновь вернулись в свое тело.\r\n", ch);

    /*
     * If someone switched into your original body, disconnect them.
     *   - JE 2/22/95
     *
     * Zmey: here we put someone switched in our body to disconnect state
     * but we must also NULL his pointer to our character, otherwise
     * close_socket() will damage our character's pointer to our descriptor
     * (which is assigned below in this function). 12/17/99
     */
    if (ch->desc->original->desc) {
      ch->desc->original->desc->character = NULL;
      STATE(ch->desc->original->desc) = CON_DISCONNECT;
    }

    /* Now our descriptor points to our original body. */
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    /* And our body's pointer to descriptor now points to our descriptor. */
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}



ACMD(do_load)
{
  struct char_data *mob;
  struct obj_data *obj;
  mob_vnum number;
  mob_rnum r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit((unsigned char)*buf2)) {
    send_to_char("Формат: создать { пре(дмет) | моб } <номер>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("ОТРИЦАТЕЛЬНЫЙ НОМЕР??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob") || is_abbrev(buf, "моб")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("Нет монстра с таким номером.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, ch->in_room);

    act("$n сделал$y странный магический жест рукой.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n создал$y $V!", FALSE, ch, 0, mob, TO_ROOM);
    act("Вы создали $V.", FALSE, ch, 0, mob, TO_CHAR);
    load_mtrigger(mob);
  } else if (is_abbrev(buf, "obj") || is_abbrev(buf, "пре")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("Нет предмета с таким номером.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    if (load_into_inventory)
      obj_to_char(obj, ch);
    else
      obj_to_room(obj, ch->in_room);
    act("$n сделал$y странный магический жест рукой.", TRUE, ch, 0, 0, TO_ROOM); 
	act("$n создал$y $3!", FALSE, ch, obj, 0, TO_ROOM);
    act("Вы создали $3.", FALSE, ch, obj, 0, TO_CHAR);
    load_otrigger(obj);
    obj_decay(obj);
  } else
    act("Вы должны указать \"номер предмета\" или \"номер монстра\".",FALSE, ch, 0, 0, TO_CHAR);
}

ACMD(do_vstat)
{
  struct char_data *mob;
  struct obj_data *obj;
  mob_vnum number;	// виртуальный номер предмета
  mob_rnum r_num;	// реальный номер предмета

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit((unsigned char)*buf2)) {
    send_to_char("Формат: вирстат { пре(дмет) | моб } <номер>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("ОТРИЦАТЕЛЬНЫЙ НОМЕР??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "моб")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("Нет монстра с таким номером.\r\n", ch); 
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob, FALSE);
  }
  else
	  if (is_abbrev(buf, "пре"))
	  { if ((real_object(number)) < 0)
			{ send_to_char("Нет объекта с таким номером.\r\n", ch); 
			  return;
			}
	    r_num = number;
		obj = read_object(r_num, VIRTUAL);
		do_stat_object(ch, obj);
		extract_obj(obj);
	  } 
	  else
    send_to_char("Это должен быть \"пре(дмет)\" или \"моб\".\r\n", ch);
}



// clean a room of all mobiles and objects
ACMD(do_purge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  one_argument(argument, buf);

  if (*buf) // Есть аргумент, удаляем цель (предмет или моба)
  { if ((vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)) != NULL) {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
	send_to_char("Фууууууууууу!\r\n", ch);
	return;
      }
      act("$n отправил$y $V в безну.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
	sprintf(buf, "(GC) %s clear %s.", GET_NAME(ch), GET_NAME(vict));
	mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	if (vict->desc) {
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict, FALSE);
    } else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents)) != NULL) {
      act("$n превратил$y в пыль $3.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char("Здесь никого нет с таким именем.\r\n", ch);
      return;
    }

   send_to_char(OK, ch);
  } 
 else   //без аргумента, очищаем от мобов и предметов комнату.
      { act("$n прошептал$y заклинание... Вас окружило огненное пламя!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("Мир стал немного чище.\r\n", ch->in_room, FALSE);

    for (vict = world[ch->in_room].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (IS_NPC(vict))
	extract_char(vict, FALSE);
    }

    for (obj = world[ch->in_room].contents; obj; obj = next_o) {
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
}



const char *logtypes[] = {
  "выключен", "краткий", "нормальный", "полный", "\n"
};

ACMD(do_syslog)
{
  int tp;

  one_argument(argument, arg);

  if (!*arg) {
    tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
	  (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
    sprintf(buf, "Ваш системный лог %s.\r\n", logtypes[tp]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
    send_to_char("Формат: сислог { выключен | краткий | нормальный | полный }\r\n", ch);
    return;
  }
  
  REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG2);
  REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG2);
  SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)));
  SET_BIT(PRF_FLAGS(ch), (PRF_LOG2 * (tp & 2) >> 1));
  
  sprintf(buf, "Ваш системный лог: %s.\r\n", logtypes[tp]); 
  send_to_char(buf, ch);
}



ACMD(do_advance)
{
  struct char_data *victim;
  char *name = arg, *level = buf2;
  int newlevel, oldlevel, i=0;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
      send_to_char("Такого персонажа в данный момент нет.\r\n", ch);
      return;
    }
  } else {
    send_to_char("Изменить кого?\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("Возможно это не очень хорошая идея.\r\n", ch);
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char("Нет! не для мобов!!!.\r\n", ch);
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char("Это не уровень!\r\n", ch); 
    return;
  }
  if (newlevel > LVL_IMPL) {
    sprintf(buf, "%d имеет самый высокий уровень.\r\n", LVL_IMPL);
	send_to_char(buf, ch);
    return;
  }
  if (newlevel > GET_LEVEL(ch)) {
    send_to_char("Да, правильно.\r\n", ch);
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char("Уже на этом уровне.\r\n", ch); 
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    i = oldlevel - newlevel; 
	 
    while (i--)
	{
	GET_LEVEL(victim) -=1;// newlevel;
    decrease_level(victim);   /*изменил функцию роления при изменении уровня*/
    }
	send_to_char("У Вас на мгновенье потемнело в глазах!\r\n"
		 "Вы ощутили как уменьшились.\r\n", victim); 
  } else {
    act("$n сделал$y странные магические жесты.\r\n"    
	"Вы ощютили незабываемое чувство, как будто Вас взяла\r\n"         
	"\\c12ГИГАНТСКАЯ ГОСПОДНЯ\\c00 рука и, захватив Ваше тело,\r\n" 
	"понесла его через звездную мглу, которая пронзила всю\r\n"                     
	"Вашу сущность. Ваша голова, кажется, заполнилась блаженством,\r\n"      
  	"пришедшим из другого мира. Ваше тело наполнилось космической энергией.\r\n"  
	"Внезапно яркая вспышка вернула Вас к земной реальности в новом качестве.\r\n\r\n"       
	"Вы почувствовали себя увереннее.", FALSE, ch, 0, victim, TO_VICT);
    
  }

    send_to_char(OK, ch);

  if (newlevel < oldlevel)
    log("(GC) %s demoted %s from level %d to %d.",
		GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
    log("(GC) %s has advanced %s to level %d (from %d)",
		GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  gain_exp_regardless(victim,
	 level_exp(GET_CLASS(victim), newlevel) - GET_EXP(victim));
  save_char(victim, NOWHERE);
}



ACMD(do_restore)
{
  struct char_data *vict;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Кого Вы хотите восстановить?\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
    send_to_char(NOPERSON, ch);
  else {
    GET_HIT(vict) = GET_REAL_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_REAL_MAX_MOVE(vict);
    GET_MANA_STORED(vict) = GET_MANA_NEED(vict);
    
	if ((GET_LEVEL(ch) >= LVL_GRGOD) && (GET_LEVEL(vict) >= LVL_IMMORT)) 
	{ for (i = 1; i <= MAX_SKILLS; i++)
	  { SET_SKILL(vict, i, 100);
	    GET_SPELL_TYPE(vict, i) |= SPELL_KNOW;
	  }     

     	vict->real_abils.intel = 25;
	vict->real_abils.wis = 25;
	vict->real_abils.dex = 25;
	vict->real_abils.str = 25;
	vict->real_abils.con = 25;
	vict->real_abils.cha = 25;
      
    
      	}
	update_pos(vict);
    send_to_char(OK, ch);
	if (ch == vict)
       act("Вы наполнили свое тело жизнью!", FALSE, vict, 0, ch, TO_CHAR);
   else
	   act("$N наполнил$Y жизнью Ваше тело!", FALSE, vict, 0, ch, TO_CHAR);
  }
}



void perform_immort_vis(struct char_data *ch)
{
    
     if (GET_INVIS_LEV(ch) == 0			&&
      !AFF_FLAGGED(ch, AFF_HIDE)		&&
      !AFF_FLAGGED(ch, AFF_INVISIBLE)	&&
      !AFF_FLAGGED(ch, AFF_CAMOUFLAGE)  &&
	  !AFF_FLAGGED(ch, AFF_MENTALLS))
     {send_to_char("Вы и так были видимы для всех!\r\n", ch);
      return;
     }
 
  GET_INVIS_LEV(ch) = 0;
  appear(ch);
  send_to_char("Вы теперь полностью видимы.\r\n", ch); 
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  if (IS_NPC(ch))
    return;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
      act("Вы пришли в ужас, когда $n растворил$u на Ваших глазах.", FALSE, ch, 0,
	  tch, TO_VICT);
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
      act("$n призван$y на Небеса.", FALSE, ch, 0,
	  tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  sprintf(buf, "Ваш уровень невидимости %d.\r\n", level);
  send_to_char(buf, ch);
}
  

ACMD(do_invis)
{
  int level;

  if (IS_NPC(ch)) {
    send_to_char("Вы не можете это сделать!\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    level = atoi(arg);
    if (level > GET_LEVEL(ch))
      send_to_char("Вы не можете быть невидимым выше своего уровня!\r\n", ch);
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char("Это должно быть ошибка...\r\n", ch);
  else {
    sprintf(buf, "%s\r\n", argument);
    for (pt = descriptor_list; pt; pt = pt->next)
      if (STATE(pt) == CON_PLAYING && pt->character && pt->character != ch)
	send_to_char(buf, pt->character);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      send_to_char(buf, ch);
  }
}


ACMD(do_poofset)
{
  char **msg;

  switch (subcmd) {
  case SCMD_POOFIN:    msg = &(POOFIN(ch));    break;
  case SCMD_POOFOUT:   msg = &(POOFOUT(ch));   break;
  default:    return;
  }

  skip_spaces(&argument);

  if (*msg)
    free(*msg);

  if (!*argument)
    *msg = NULL;
  else
    *msg = str_dup(argument);

  send_to_char(OK, ch);
}



ACMD(do_dc)
{
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char("Формат: DC <номер пользователя> (type USERS for a list)\r\n", ch);
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char("Данное соединение отсутствует.\r\n", ch);
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char("Такое подключение отсутствует.\r\n", ch);
    else
      send_to_char("Хмм...  Возможно это не очень хорошая идея...\r\n", ch);
    return;
  }

  /* We used to just close the socket here using close_socket(), but
   * various people pointed out this could cause a crash if you're
   * closing the person below you on the descriptor list.  Just setting
   * to CON_CLOSE leaves things in a massively inconsistent state so I
   * had to add this new flag to the descriptor. -je
   *
   * It is a much more logical extension for a CON_DISCONNECT to be used
   * for in-game socket closes and CON_CLOSE for out of game closings.
   * This will retain the stability of the close_me hack while being
   * neater in appearance. -gg 12/1/97
   *
   * For those unlucky souls who actually manage to get disconnected
   * by two different immortals in the same 1/10th of a second, we have
   * the below 'if' check. -gg 12/17/99
   */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
    send_to_char("Соединение уже отключено.\r\n", ch);
  else {
    /*
     * Remember that we can disconnect people not in the game and
     * that rather confuses the code when it expected there to be
     * a character context.
     */
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DISCONNECT;
    else
      STATE(d) = CON_CLOSE;

    sprintf(buf, "Соединение #%d закрыто.\r\n", num_to_dc);
    send_to_char(buf, ch);
    log("(GC) Соединение закрыто для %s.", GET_VNAME(ch));
  }
}



ACMD(do_wizlock)
{
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char("Неверное значение wizlock.\r\n", ch);
      return;
    }
    circle_restrict = value;
    when = "теперь";
  } else
    when = "в настоящее время";

  switch (circle_restrict) {
  case 0:
    sprintf(buf, "Игра %s полностью открыта.\r\n", when);
    break;
  case 1:
    sprintf(buf, "Игра %s закрыта для новых игроков.\r\n", when);
    break;
  default:
    sprintf(buf, "Только уровнем %d и выше можно зайти в игру %s.\r\n",
	    circle_restrict, when);
    break;
  }
  send_to_char(buf, ch);
}

extern const char* get_month(const tm* t);
ACMD(do_date)
{ 
  int d, h, m;

  time_t mytime = time(0);
  tm* td = localtime(&mytime);
  sprintf(buf, "Текущее время сервера               : %d %s %d %02d:%02d\r\n", 
      td->tm_mday, get_month(td), td->tm_year+1900, td->tm_hour, td->tm_min);

  td = localtime(&boot_time);
  sprintf(buf + strlen(buf), "Последнее время перезагрузки сервера: %d %s %d %02d:%02d\r\n",
      td->tm_mday, get_month(td), td->tm_year+1900, td->tm_hour, td->tm_min);

    mytime = time(0) - boot_time;
   	d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;
         sprintf(buf + strlen(buf), "С момента перезагрузки прошло       : %d %s, %d %s, %02d %s\r\n", d,
	 desc_count(d, WHAT_DAY), h, desc_count(h, WHAT_HOUR),  m, desc_count(m, WHAT_MINa));
	 sprintf(buf + strlen(buf), "В мире сейчас находится             : %d комнат в %d зонах\r\n",
	 top_of_world + 1, top_of_zone_table + 1);
  send_to_char(buf, ch);
}



ACMD(do_last)
{ CHAR_DATA *chdata;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Кого Вы хотите найти?\r\n", ch);
    return;
  }

 
  CREATE(chdata, CHAR_DATA, 1);
  clear_char(chdata);
  if (load_char(arg, chdata) < 0) {
      send_to_char("Нет такого игрока.\r\n", ch);
      free(chdata);
      return;
  }
  
  if ((GET_LEVEL(chdata) > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_IMPL)) {
    send_to_char("Это под силу только Богам!!!\r\n", ch);
    return;
  }
          sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
          GET_IDNUM(chdata), (int) GET_LEVEL(chdata),
          CLASS_ABBR(chdata), GET_NAME(chdata), 
	  GET_LASTIP(chdata)[0] ? GET_LASTIP(chdata) : "Unknown",
	  ctime(&LAST_LOGON(chdata)));
          send_to_char(buf, ch);

   free_char(chdata);
}


ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  CHAR_DATA *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n заставил$y Вас \"%s\".", to_force);

  if (!*arg || !*to_force)
    send_to_char("Что Вы хотите заставить сделать?\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GRGOD) || (str_cmp("все", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
      send_to_char(NOPERSON, ch);
    else if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char("НЕТ, Нет, нет!!!\r\n", ch);
    else {
    send_to_char(OK, ch);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (vict = world[ch->in_room].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
   send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}



ACMD(do_wiznet)
{
  struct descriptor_data *d;
  char emote = FALSE;
  char any = FALSE;
  int level = LVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("Формат: wiznet <text> | #<level> <text> | *<emotetext> |\r\n "
		 "       wiznet @<level> *<emotetext> | wiz @\r\n", ch);
    return;
  }
  switch (*argument) {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), LVL_IMMORT);
      if (level > GET_LEVEL(ch)) {
	send_to_char("Вы не можете это сделать.\r\n", ch);
	return;
      }
    } else if (emote)
      argument++;
    break;
  case '@':
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) == CON_PLAYING && GET_LEVEL(d->character) >= LVL_IMMORT &&
	  !PRF_FLAGGED(d->character, PRF_NOWIZ) &&
	  (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL)) {
	if (!any) {
	  strcpy(buf1, "Боги online:\r\n");
	  any = TRUE;
	}
	sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
	if (PLR_FLAGGED(d->character, PLR_WRITING))
	  strcat(buf1, " (пишет)\r\n");
	else if (PLR_FLAGGED(d->character, PLR_MAILING))
	  strcat(buf1, " (отправляет почту)\r\n");
	else
	  strcat(buf1, "\r\n");

      }
    }
    any = FALSE;
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) == CON_PLAYING && GET_LEVEL(d->character) >= LVL_IMMORT &&
	  PRF_FLAGGED(d->character, PRF_NOWIZ) &&
	  CAN_SEE(ch, d->character)) {
	if (!any) {
	  strcat(buf1, "Боги offline:\r\n");
	  any = TRUE;
	}
	sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
      }
    }
    send_to_char(buf1, ch);
    return;
  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char("Вы offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Не беспокойте Богов по мелочам!!!\r\n", ch);
    return;
  }
  if (level > LVL_IMMORT) {
    sprintf(buf1, "%s: <%d> %s%s\r\n", GET_NAME(ch), level,
	    emote ? "<--- " : "", argument);
    sprintf(buf2, "Кто-то: <%d> %s%s\r\n", level, emote ? "<--- " : "",
	    argument);
  } else {
    sprintf(buf1, "%s: %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
	    argument);
    sprintf(buf2, "Кто-то: %s%s\r\n", emote ? "<--- " : "", argument);
  }

  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && (GET_LEVEL(d->character) >= level) &&
	(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
	(!PLR_FLAGGED(d->character, (PLR_WRITING | PLR_MAILING) ))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(CCCYN(d->character, C_NRM), d->character);
      if (CAN_SEE(d->character, ch))
	send_to_char(buf1, d->character);
      else
	send_to_char(buf2, d->character);
      send_to_char(CCNRM(d->character, C_NRM), d->character);
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
}



ACMD(do_zreset)
{
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Вы должны определить зону.\r\n", ch);
    return;
  }
  if (*arg == '*') {
    for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i);
    send_to_char("Перезапуск Мира.\r\n", ch);
    sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
    return;
  } else if (*arg == '.')
    i = world[ch->in_room].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }
  if (i >= 0 && i <= top_of_zone_table) {
    reset_zone(i);
    sprintf(buf, "Перезапуск зоны %d (#%d): %s.\r\n", i, zone_table[i].number,
	    zone_table[i].name);
    send_to_char(buf, ch);
    sprintf(buf, "(GC) %s reset entire world %d (%s)", GET_NAME(ch), i, zone_table[i].name);
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
  } else
    send_to_char("Несуществующий номер зоны.\r\n", ch);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
  CHAR_DATA *vict;
  char   *reason;
  char   num[MAX_INPUT_LENGTH];
  long result;
  int times = 0;

      reason = two_arguments(argument, arg, num);

  if (!*arg)
    send_to_char("Да, но кого?!?\r\n", ch);
  else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
    send_to_char("В данный момент игрок с таким именем отсутствует.\r\n", ch);
  else if (IS_NPC(vict) && !(subcmd == SCMD_SKILL || subcmd == SCMD_SPELL))
    send_to_char("Вы не можете это делать с мобом!\r\n", ch);
  else if (GET_LEVEL(vict) > GET_LEVEL(ch))
    send_to_char("Хммм...У Вас на это нет права.\r\n", ch);
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      send_to_char("Перерождение...\r\n", ch);
      roll_real_abils(vict, MIN(85, atoi(num)));
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      sprintf(buf, "Новые характеристики: Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Lck: %d, Size: %d\r\n",
	      GET_STR(vict), GET_STR_ADD(vict), GET_INT(vict), GET_WIS(vict),
	      GET_DEX(vict), GET_CON(vict), GET_CHA(vict), GET_SIZE(vict));
      send_to_char(buf, ch);
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, (PLR_THIEF | PLR_KILLER) )) {
	send_to_char("Ваша жертва не имеет флага.\r\n", ch);
	return;
      }
      REMOVE_BIT(PLR_FLAGS(vict, PLR_THIEF), PLR_THIEF);
	  REMOVE_BIT(PLR_FLAGS(vict, PLR_KILLER), PLR_KILLER);
      send_to_char(OK, ch);
      send_to_char("Вас простили Боги!!!\r\n", vict);
      sprintf(buf, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      GET_TIME_KILL(vict) = 0; 
	  break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_MUTE);
      sprintf(buf, "(GC) Squelch %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
   case SCMD_MUTE:
      if ( *num && reason && *reason )
      {
        skip_spaces(&reason);
        times = atol(num);
        MUTE_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
		SET_BIT(PLR_FLAGS(vict, PLR_MUTE), PLR_MUTE);
        sprintf( num, "%s : %s", GET_NAME(ch), reason );
		
        if ( MUTE_REASON(vict) ) free( MUTE_REASON(vict) );
           	MUTE_REASON(vict) = strcpy((char*)(malloc(strlen(num)+1)), num );
      }
      else
      {
         if (!PLR_FLAGGED(vict, PLR_MUTE))
            {send_to_char("Ваша жертва и так может кричать.\r\n", ch);
             return;
            }
         else
           {MUTE_DURATION(vict) = 0;
            REMOVE_BIT(PLR_FLAGS(vict, PLR_MUTE), PLR_MUTE);
            if ( MUTE_REASON(vict) ) free( MUTE_REASON(vict) );
            MUTE_REASON(vict) = NULL;
           }
      }
      result = PLR_FLAGGED(vict, PLR_MUTE);
      sprintf(buf,"%s$N %s$Y Вам кричать.%s",
                  CCRED(vict,C_NRM),
                  (result ? "запретил" : "разрешил"),
                  CCNRM(vict,C_NRM));
      act(buf,FALSE,vict,0,ch,TO_CHAR);
      sprintf(buf, "(GC) Mute %s for %s by %s(%dh).", ONOFF(result),
              GET_NAME(vict), GET_NAME(ch), times);
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_DUMB:
      if ( *num && reason && *reason )
      {
        skip_spaces(&reason);
        times = atol(num);
        DUMB_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
		SET_BIT(PLR_FLAGS(vict, PLR_DUMB), PLR_DUMB);
        sprintf( num, "%s : %s", GET_NAME(ch), reason );
        if ( DUMB_REASON(vict) ) free( DUMB_REASON(vict) );
        DUMB_REASON(vict) = strcpy( (char*)(malloc(strlen(num)+1)), num );
      }
      else
      {
         if (!PLR_FLAGGED(vict, PLR_DUMB))
            {send_to_char("Ваша жертва и так может говорить.\r\n", ch);
             return;
            }
         else
           {DUMB_DURATION(vict) = 0;
            REMOVE_BIT(PLR_FLAGS(vict, PLR_DUMB), PLR_DUMB);
            if ( DUMB_REASON(vict) ) free( DUMB_REASON(vict) );
            DUMB_REASON(vict) = NULL;
           }
      }
      result = PLR_FLAGGED(vict, PLR_DUMB);
      sprintf(buf,"%s$N %s$Y Вам разговаривать.%s",
                  CCRED(vict,C_NRM),
                  (result ? "запретил" : "разрешил"),
                  CCNRM(vict,C_NRM));
      act(buf,FALSE,vict,0,ch,TO_CHAR);
      sprintf(buf, "(GC) Dumb %s for %s by %s(%dm).", ONOFF(result),
              GET_NAME(vict), GET_NAME(ch), times);
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
	
	case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char("ОХ, ДА, это действительно КРУТО...\r\n", ch); 
	return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Ваша цель уже и так пофризина.\r\n", ch);
	return;
      }
      if (sscanf(argument,"%s %d",arg,&times) > 0)
         FREEZE_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
      else
         FREEZE_DURATION(vict) = 0;
	  SET_BIT(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char("Вы почувствовали, как холод сковал Ваше тело!\r\n", vict);
      send_to_char("Заморожен.\r\n", ch);
      act("Лед сковал все  движения $n!", FALSE, vict, 0, 0, TO_ROOM);
      sprintf(buf, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
   
case SCMD_HELL:
      if ( *num && reason && *reason )
      {
        skip_spaces(&reason);
        times = atol(num);
        HELL_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
        sprintf( num, "%s : %s", GET_NAME(ch), reason );
       if ( HELL_REASON(vict) ) free( HELL_REASON(vict) ); 
        HELL_REASON(vict) = strcpy( (char*)(malloc(strlen(num)+1)), num );
      }
      else
      {
         if ( GET_LEVEL(ch) < LVL_GOD ) 
         {
           send_to_char("Вы не имеете права сделать это.\r\n", ch);
           return;
         }
         HELL_DURATION(vict) = 0;
        if ( HELL_REASON(vict) ) free( HELL_REASON(vict) );
         HELL_REASON(vict) = NULL;
      }


      if (HELL_DURATION(vict))
         {if (PLR_FLAGGED(vict, PLR_HELLED))
             {send_to_char("Ваша жертва и так в темнице.\r\n", ch);
	          return;
             }
          SET_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);
          sprintf(buf,"%s$N поместил$Y Вас в АД.%s",
                  CCRED(vict,C_NRM),CCNRM(vict,C_NRM));
          act(buf,FALSE,vict,0,ch,TO_CHAR);
          act("Вы посадили в АД $V!", FALSE, ch, 0, vict, TO_CHAR);
          act("$n посажен$y в АД!", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, r_helled_start_room);
          look_at_room(vict, r_helled_start_room);
          act("$n посажен$y в АД!", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed to hell by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      else
         {if (!PLR_FLAGGED(vict, PLR_HELLED))
             {send_to_char("Ваша жертва и так на свободе.\r\n", ch);
	          return;
             }
          REMOVE_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);
          send_to_char("Вас выпустили из АДА.\r\n", vict);
          act("Вы выпустили из АДА $V!", FALSE, ch, 0, vict, TO_CHAR);
          if ((result = GET_LOADROOM(vict)) == NOWHERE)
             result = calc_loadroom(vict);
          result = real_room(result);
          if (result == NOWHERE)
             {if (GET_LEVEL(vict) >= LVL_IMMORT)
                 result = r_immort_start_room;
              else
                 result = r_mortal_start_room;
             }
          act("$n выпущен$y из АДА!", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, result);
          look_at_room(vict, result);
          act("$n выпущен$y из АДА!", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed from hell by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      break;

    case SCMD_NAME:
      if (sscanf(argument,"%s %d",arg,&times) > 1)
         NAME_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
      else
         NAME_DURATION(vict) = 0;

      if (NAME_DURATION(vict))
         {if (PLR_FLAGGED(vict, PLR_NAMED))
             {send_to_char("Ваша жертва и так в комнате имени.\r\n", ch);
	          return;
             }
          SET_BIT(PLR_FLAGS(vict, PLR_NAMED), PLR_NAMED);
          sprintf(buf,"%s$N поместил$Y Вас в комнату имен.%s",
                  CCRED(vict,C_NRM),CCNRM(vict,C_NRM));
          act(buf,FALSE,vict,0,ch,TO_CHAR);
          send_to_char("Перемещен.\r\n", ch);
          act("$n посажен$y в КОМНАТУ ИМЕНИ !", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, r_named_start_room);
          look_at_room(vict, r_named_start_room);
          act("$n посажен$y в КОМНАТУ ИМЕНИ !", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed to NAMES ROOM by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      else
         {if (!PLR_FLAGGED(vict, PLR_NAMED))
             {send_to_char("В комнате &c\"имени\"&n этого персонажа нет.\r\n", ch);
	          return;
             }
          REMOVE_BIT(PLR_FLAGS(vict, PLR_NAMED), PLR_NAMED);
          send_to_char("Вас выпустили из КОМНАТЫ ИМЕНИ.\r\n", vict);
          send_to_char("Выпустили.\r\n", ch);
          if ((result = GET_LOADROOM(vict)) == NOWHERE)
             result = calc_loadroom(vict);
          result = real_room(result);
          if (result == NOWHERE)
             {if (GET_LEVEL(vict) >= LVL_IMMORT)
                 result = r_immort_start_room;
              else
                 result = r_mortal_start_room;
             }
          act("$n выпущен$y из КОМНАТЫ ИМЕНИ !", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, result);
          look_at_room(vict, result);
          act("$n выпущен$y из КОМНАТЫ ИМЕНИ !", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed from NAMES ROOM by %s(%ds).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      break;

    case SCMD_REGISTER:
      if (ch == vict)
		{ send_to_char("Ты че пьяный сегодня?!\r\n", ch);
	      return;
        }
   	  if (PLR_TOG_CHK(vict, PLR_REGISTERED))
		 { send_to_char("Вас зарегистрировали.\r\n", vict);
           send_to_char("Зарегистрирован.\r\n", ch);
			if (IN_ROOM(vict) == r_unreg_start_room)
				{if ((result = GET_LOADROOM(vict)) == NOWHERE)
					result = calc_loadroom(vict);
					result = real_room(result);
				if (result == NOWHERE)
					{if (GET_LEVEL(vict) >= LVL_IMMORT)
						result = r_immort_start_room;
					 else
						result = r_mortal_start_room;
					}
						char_from_room(vict);
						char_to_room(vict, result);
						look_at_room(vict, result);
					act("$n появил$u в центре комнаты с правом играть после регистрации!", FALSE, vict, 0, 0, TO_ROOM);
				}
              sprintf(buf, "(GC) %s зарегистрирован %s.", GET_NAME(vict), GET_TNAME(ch));
		}
		else
			{ send_to_char("Разрегистрирован.\r\n", ch);
			  sprintf(buf, "(GC) %s разрегистрирован %s.", GET_NAME(vict), GET_TNAME(ch));
			}
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
	case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Этот персонаж не находится в фризе.\r\n", ch);
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
	sprintf(buf, "Извините, но уровень %d зафиксирован Богом %s... Вы ничего не можете сделать %s.\r\n",
	   GET_FREEZE_LEV(vict), GET_TNAME(vict), HMHR(vict));
	send_to_char(buf, ch);
	return;
      }
      sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      REMOVE_BIT(PLR_FLAGS(vict, PLR_FROZEN), PLR_FROZEN);
      send_to_char("Огненный шар внезапно окутал Вас!\r\nВы начинаете оттаивать.\r\n", vict);
      send_to_char("Тепло.\r\n", ch);
      act("Огненный шар внезапно окутал $v!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected) {
	while (vict->affected)
	  affect_remove(vict, vict->affected);
	send_to_char("Появилась кратковременная световая вспышка!\r\n"
		     "Вы почувствовали изменение.\r\n", vict);
	send_to_char("Все аффекты удалены.\r\n", ch);
      } else {
	send_to_char("Ваша цель не имеет никаких аффектов!\r\n", ch);
	return;
      }
      break;
    case SCMD_SKILL:
			list_skills(vict, ch, true, 0);
	   break;
    case SCMD_SPELL:
		list_spells(vict, ch);
	   break;
	default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      break;
    }
    save_char(vict, NOWHERE);
  }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, zone_rnum zone)
{
  sprintf(bufptr, "%s%3d %-30.31s Возраст: %3d; Перезапуск: %3d (%1d); Топ: %5d\r\n",
	  bufptr, zone_table[zone].number, zone_table[zone].name,
	  zone_table[zone].age, zone_table[zone].lifespan,
	  zone_table[zone].reset_mode, zone_table[zone].top);
}



ACMD(do_show)
{ int i, j, k, l, con = 0;
  zone_rnum zrn;
  zone_vnum zvn;
  char self = 0;
  struct char_data *vict;
  struct obj_data *obj;
  struct descriptor_data *d;
  show_struct fields[] = 
  {
    { "ничего",		NULL  },				// 0 
    { "зоны",		LVL_IMMORT },	// 1 
    { "игроки",		LVL_GOD },
    { "рента",		LVL_GLGOD },
    { "статсы",		LVL_IMMORT },
    { "ошибки",		LVL_IMPL },		// 5 
    { "смерть",		LVL_GRGOD },
    { "годрумс",	LVL_GOD },
    { "магазины",	LVL_IMMORT },
    { "постой",		LVL_GOD },
    { "следить",	LVL_IMPL },		// 10 
   { "описание",	LVL_IMMORT },	// 11 
    { "дроплинк",   LVL_GRGOD },    // 12 
    { "наказанные", LVL_IMMORT },	// 13 
    { "\n", 0 }
  };

  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH];
  char buf[MAX_EXTEND_LENGTH];

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "Лист опций:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch) || GET_COMMSTATE(ch))
	sprintf(buf + strlen(buf), "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("Это могут только боги!\r\n", ch);
    return;
  }
  if (!strcmp(value, "."))
    self = 1;
  buf[0] = '\0';
  switch (l) {
  case 1:			/* zone */
       if (self)
      print_zone_to_buf(buf, world[ch->in_room].zone);
    else if (*value && is_number(value)) {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
      if (zrn <= top_of_zone_table)
	print_zone_to_buf(buf, zrn);
      else {
	send_to_char("Это неверная зона.\r\n", ch); 
	return;
      }
    } else
		if (*value && !is_number(value))
			{ for (zrn = 0; zrn <= top_of_zone_table; zrn++)
			  if (isname(value, zone_table[zrn].name))
			  { print_zone_to_buf(buf, zrn);
			    con =  true;
			  }
			  if (!con)
			  send_to_char("Нет зоны с таким именем.\r\n", ch); 
			}
       else
		   for (zrn = 0; zrn <= top_of_zone_table; zrn++)
				print_zone_to_buf(buf, zrn);
    page_string(ch->desc, buf, TRUE);
    break;
  case 2:			/* player */
	send_to_char("База игроков:\r\n", ch);
	for (j = 0, i = 0; i <= top_of_p_table; i++, j++)
	{ sprintf(buf + strlen(buf), "%-12s",player_table[i].name);
              if(j == 7)
	{	strcat(buf, "\r\n");  
		j = 0;
	}
	} 
	strcat(buf, "\r\n");
	page_string(ch->desc, buf, 1);
    break;
  case 3:
    if (!*value) {
      send_to_char("Имя помогло бы.\r\n", ch);
      return;
    }
    Crash_listrent(ch, value);
    break;
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    strcpy(buf, "Текущая информация:\r\n");
    sprintf(buf + strlen(buf), "  %5d игроков в игре    %5d соединений\r\n",
		i, con);
    sprintf(buf + strlen(buf), "  %5d всего игроков в базе\r\n",
		top_of_p_table + 1);
    sprintf(buf + strlen(buf), "  %5d мобов             %5d прототипов\r\n",
		j, top_of_mobt + 1);
    sprintf(buf + strlen(buf), "  %5d объектов          %5d прототипов\r\n",
		k, top_of_objt + 1);
    sprintf(buf + strlen(buf), "  %5d комнат            %5d зон\r\n",
		top_of_world + 1, top_of_zone_table + 1);
    sprintf(buf + strlen(buf), "  %5d больших буферов\r\n",
		buf_largecount);
    sprintf(buf + strlen(buf), "  %5d переключенных буферов      %5d переполненных буферов\r\n",
		buf_switches, buf_overflows);
    sprintf (buf + strlen (buf), "  Отправлено кбайт - %lu\r\n",
	       number_of_bytes_written/1024);
    sprintf (buf + strlen (buf), "  Принято кбайт    - %lu\r\n",
	       number_of_bytes_read/1024);
    send_to_char(buf, ch);
    break;
  case 5:
    strcpy(buf, "Пустых выходов:\r\n------------\r\n");
    for (i = FIRST_ROOM, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++)
	if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0)
	  sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k, GET_ROOM_VNUM(i),
		  world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 6:
    strcpy(buf, "Комнаты смерти (ДТ)\r\n-----------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j,
		GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 7:
    strcpy(buf, "Комнаты для богов\r\n--------------------------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
    if (ROOM_FLAGGED(i, ROOM_GODROOM))
      sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n",
		++j, GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 8:
    show_shops(ch, value);
    break;
  case 9:
   send_to_char("Извените, эта опция пока не поддерживается.\r\n", ch);    
//hcontrol_list_houses(ch);
    break;
  case 10:
    *buf = '\0';
    send_to_char("Текущий негласный контроль:\r\n", ch);
    send_to_char("---------------------------\r\n", ch);
    for (d = descriptor_list; d; d = d->next) {
      if (d->snooping == NULL || d->character == NULL)
	continue;
      if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
	continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	continue;
      sprintf(buf + strlen(buf), "%-10s - подслушивается %s.\r\n",
               GET_NAME(d->snooping->character), GET_TNAME(d->character));
    }
    send_to_char(*buf ? buf : "Никто в настоящее время не подслушивает.\r\n", ch); 
    break; 
	case 11: 
    buf[0] = '\0';
    if (!(vict = get_char_vis(ch, value, FIND_CHAR_WORLD))){
        send_to_char("Нет такого игрока или моба.\r\n", ch);
	return;
       }
    if (IS_NPC(vict))
       sprintf(buf, "%sЭто описание МОБА с именем: %s%s\r\n",CCYEL(ch,C_SPR),GET_NAME(vict),CCNRM(ch,C_SPR));
    else 
       sprintf(buf, "%sОписание ИГРОКА с именем: %s%s\r\n",CCYEL(ch,C_SPR),GET_NAME(vict),CCNRM(ch,C_SPR));    
    sprintf(buf + strlen(buf), "%s\r\n", (vict->player.description));
    send_to_char(buf, ch);
    break;    
  case 12: // show linkdrop
    send_to_char("  Список игроков в состоянии 'link drop'\r\n",ch);
    sprintf(buf,"%-50s%s %s\r\n","   Имя","Комната","Таймер бездействия (тики)");
    send_to_char(buf,ch);
    for ( i = 0, vict = character_list; vict; vict = vict->next )
    {
      if ( IS_NPC(vict)               ||
           vict->desc != NULL         ||
           IN_ROOM( vict ) == NOWHERE
         ) continue;
      ++i;
      sprintf(buf,"%-50s[% 5d] %d\r\n",noclan_title(vict),GET_ROOM_VNUM(IN_ROOM(vict)),vict->char_specials.timer );
      send_to_char(buf, ch);
    }
    sprintf(buf,"Всего - %d\r\n",i);
    send_to_char(buf, ch);
    break;
  case 13: // show punishment
    send_to_char("  Список наказанных игроков.\r\n",ch);
    for (d = descriptor_list; d; d = d->next)
    {
      if (d->snooping != NULL && d->character != NULL) continue;
      if (STATE(d) != CON_PLAYING ||
          (GET_LEVEL(ch) < GET_LEVEL(d->character) && !GET_COMMSTATE(ch))
         )
        continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	    continue;
      buf[0] = 0;

      if (PLR_FLAGGED(d->character, PLR_MUTE) && MUTE_DURATION(d->character))
        sprintf(buf+strlen(buf), "Будет молчать : %ld час [%s].\r\n",
	          (MUTE_DURATION(d->character) - time(NULL)) / 3600,
              MUTE_REASON(d->character)?MUTE_REASON(d->character):"-"
                );

      if (PLR_FLAGGED(d->character, PLR_DUMB) && DUMB_DURATION(d->character))
        sprintf(buf+strlen(buf), "Будет нем : %ld час [%s].\r\n",
	          (DUMB_DURATION(d->character) - time(NULL)) / 3600,
              DUMB_REASON(d->character)?DUMB_REASON(d->character):"-"
                );

      if (PLR_FLAGGED(d->character, PLR_HELLED) && HELL_DURATION(d->character))
        sprintf(buf+strlen(buf), "Будет в аду : %ld час [%s].\r\n",
	          (HELL_DURATION(d->character) - time(NULL)) / 3600,
              HELL_REASON(d->character)?HELL_REASON(d->character):"-"
                );

      if ( buf[0] )
      {
        send_to_char(GET_NAME(d->character),ch);
        send_to_char("\r\n",ch);
        send_to_char(buf,ch);
      }
    }
    break;
  default:
    send_to_char("Извините, это не известная команда.\r\n", ch); 
    break;
  }
}


/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

  set_struct  set_fields[] = {
   { "brief",		LVL_GOD, 	PC, 	BINARY },  // 0 
   { "invstart", 	LVL_GOD, 	PC, 	BINARY },  
   { "титул",		LVL_IMMORT,	PC, 	MISC },
   { "nosummon", 	LVL_GRGOD, 	PC, 	BINARY },
   { "maxhit",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "maxmana", 	LVL_GRGOD, 	BOTH, 	NUMBER },  // 5 
   { "maxmove", 	LVL_IMPL, 	BOTH, 	NUMBER },
   { "hit", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "mana",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "move",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "align",		LVL_GRGOD, 	BOTH, 	NUMBER },  // 10 
   { "str",	    	LVL_IMPL, 	BOTH, 	NUMBER },
   { "размер",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "int", 		LVL_IMPL, 	BOTH, 	NUMBER },
   { "wis", 		LVL_IMPL, 	BOTH, 	NUMBER },
   { "dex", 		LVL_IMPL, 	BOTH, 	NUMBER },  // 15
   { "con", 		LVL_IMPL, 	BOTH, 	NUMBER },
   { "cha",	    	LVL_IMPL, 	BOTH, 	NUMBER },
   { "ac", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "монеты",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "банк",		LVL_IMPL, 	PC, 	NUMBER },  // 20 
   { "exp", 		LVL_IMPL, 	BOTH, 	NUMBER },
   { "хитрол",  	LVL_GRGOD, 	BOTH, 	NUMBER },
   { "дамрол",  	LVL_GRGOD, 	BOTH, 	NUMBER },
   { "invis",		LVL_IMPL, 	PC, 	NUMBER },
   { "nohassle", 	LVL_GRGOD, 	PC, 	BINARY },  // 25 
   { "холд",		LVL_FREEZE,     PC, 	BINARY },
   { "право",   	LVL_IMPL, 	PC, 	BINARY },
   { "колигрок", 	LVL_GRGOD, 	PC, 	NUMBER },
   { "опьянение",	LVL_GRGOD, 	BOTH, 	MISC },
   { "голод",		LVL_GRGOD, 	BOTH, 	MISC },    // 30 
   { "жажда",		LVL_GRGOD, 	BOTH, 	MISC },
   { "убийца",		LVL_GLGOD, 	PC, 	BINARY },
   { "вор",		LVL_GOD, 	PC, 	BINARY },
   { "уровень",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "room",		LVL_IMPL, 	BOTH, 	NUMBER },  // 35
   { "румфлаг", 	LVL_GRGOD, 	PC, 	BINARY },
   { "siteok",		LVL_GRGOD, 	PC, 	BINARY },
   { "deleted", 	LVL_GRGOD, 	PC, 	BINARY },
   { "класс",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "размакс", 	LVL_GOD, 	PC, 	BINARY },  // 40 
   { "quest",		LVL_GOD, 	PC, 	BINARY },
   { "лоадрум", 	LVL_GRGOD, 	PC, 	MISC },
   { "color",		LVL_GOD, 	PC, 	BINARY },
   { "idnum",		LVL_IMPL, 	PC, 	NUMBER },
   { "пароль",		LVL_IMPL, 	PC, 	MISC },    // 45
   { "nodelete", 	LVL_GOD, 	PC, 	BINARY },
   { "пол", 		LVL_GOD, 	BOTH, 	MISC },
   { "возраст",	    	LVL_GRGOD,	BOTH,	NUMBER },
   { "рост",		LVL_GOD,	BOTH,	NUMBER },
   { "вес",		LVL_GOD,	BOTH,	NUMBER },  // 50 
   { "раса",       	LVL_GRGOD,  	PC,     MISC },
   { "предтитул",	LVL_IMMORT, 	PC, 	MISC },
   { "ренайм",  	LVL_IMMORT, 	PC, 	MISC },    
   { "усталость",	LVL_GRGOD, 	PC,     NUMBER },  
   { "ад",          	LVL_IMMORT, 	PC,     MISC},     // 55
   { "тригквест",   	LVL_IMPL,   	PC,     MISC},     
   { "слава",       	LVL_IMPL,   	PC,     MISC},		
   { "rstr",    	LVL_IMPL, 	BOTH, 	NUMBER },	
   { "rint", 		LVL_IMPL, 	BOTH, 	NUMBER },	
   { "rwis", 		LVL_IMPL, 	BOTH, 	NUMBER },  // 60
   { "rdex", 		LVL_IMPL, 	BOTH, 	NUMBER },   
   { "rcon", 		LVL_IMPL, 	BOTH, 	NUMBER },	
   { "rcha",    	LVL_IMPL, 	BOTH, 	NUMBER },	
   { "email",       	LVL_IMMORT, 	PC,     MISC   },
   { "хслава",          LVL_IMPL,   	PC,     MISC},	    // 65
   { "кланканал",       LVL_GLGOD,  	BOTH,   NUMBER},
   { "реморт",          LVL_IMPL,  	BOTH,   MISC},
   { "оим",		LVL_IMPL,   	PC, 	NUMBER	},	//общая индивидуальная мощь (накопительная) 
   { "пим",     	LVL_IMPL,   	PC, 	NUMBER	},  //покупательная индивидуальная мощь
   { "мдвк",            LVL_IMPL,   	PC, 	NUMBER	},  //70 индивидуальная мощь накопленная до вступления в клан.
   { "кланид",		LVL_GLGOD,  	BOTH,   NUMBER	},  //установка чару идешника клана, в случае изменения.
   { "кланранг",	LVL_GLGOD,  	BOTH,   NUMBER	},  //установка чару ранга клана.
   { "иммкаст",     	LVL_IMPL,   	PC, 	BINARY  },
   { "иммбой",     	LVL_IMPL,   	PC, 	BINARY  },
   { "перслог",		LVL_IMPL,	PC,	BINARY	}, 
   { "квест",	    	LVL_IMMORT, 	BOTH,   MISC	},
   { "доска",	 	LVL_GRGOD, 	PC, 	BINARY  },
   { "\n", 0, BOTH, MISC }
  };


int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
  int i, j, on = 0, off = 0, value = 0, return_code = 1, ptnum, times, result;
  char npad[6][256];
  char *reason; 
  room_rnum rnum;
  room_vnum rvnum;
  char output[MAX_STRING_LENGTH], num[MAX_INPUT_LENGTH];
  
   void generate_quest(struct char_data *ch, struct char_data *questman);
 
  /* Check to make sure all the levels are correct */
  if (GET_LEVEL(ch) != LVL_IMPL) {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
      send_to_char("Возможно, это не очень хорошая идея...\r\n", ch); 
      return (0);
    }
  }
  if (GET_LEVEL(ch) < set_fields[mode].level) {
    send_to_char("У Вас нет права доступа чтобы использовать эту команду!\r\n", ch); 
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
    send_to_char("Вы не можете делать это по отношению к Монстру!\r\n", ch); 
    return (0);
  } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char("Это может быть сделано только по отношению к Монстру!\r\n", ch); /*That can only be done to a beast*/
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "вкл"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "выкл"))
      off = 1;
    if (!(on || off)) {
      send_to_char("Значение должно быть 'on' или 'off'.\r\n", ch);
      return (0);
    }
    sprintf(output, "%s %s для %s.", set_fields[mode].cmd, ONOFF(on),
	    GET_RNAME(vict));
  } else if (set_fields[mode].type == NUMBER) {
    value = atoi(val_arg);
    sprintf(output, "%s присвоен %s %d.", GET_DNAME(vict),
	    set_fields[mode].cmd, value);
  } else {
    strcpy(output, "Хорошо.");
  }

  switch (mode) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict, PLR_INVSTART), PLR_INVSTART);
    break;
  case 2:
    set_title(vict, val_arg);
    if (!GET_TITLE(vict))
      { sprintf(output, "Титул %s удален.", GET_RNAME(vict));
        break;
      }
    sprintf(output, "Новый титул %s: \"%s\"", GET_RNAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    sprintf(output, "Призвать %s для %s.\r\n", ONOFF(!on), GET_RNAME(vict));
    break;
  case 4:
    vict->points.max_hit = RANGE(1, 30000);
    affect_total(vict);
    break;
  case 5:
    vict->points.max_mana = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 6:
    vict->points.max_move = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 7:
    vict->points.hit = RANGE(-9, vict->points.max_hit);
    affect_total(vict);
    break;
  case 8:
    vict->points.mana = RANGE(0, vict->points.max_mana);
    affect_total(vict);
    break;
  case 9:
    vict->points.move = RANGE(0, vict->points.max_move);
    affect_total(vict);
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;
  case 11:
    RANGE(3, 36);
    vict->real_abils.str = value;
    affect_total(vict);
    break;
  case 12:
    vict->real_abils.size = RANGE(0, 100);
    affect_total(vict);
    break;
  case 13:
    RANGE(6, 36);
    vict->real_abils.intel = value;
    affect_total(vict);
    break;
  case 14:
    RANGE(6, 36);
    vict->real_abils.wis = value;
    affect_total(vict);
    break;
  case 15:
    RANGE(6, 36);
    vict->real_abils.dex = value;
    affect_total(vict);
    break;
  case 16:
    RANGE(6, 36);
    vict->real_abils.con = value;
    affect_total(vict);
    break;
  case 17:
    RANGE(6, 36);
    vict->real_abils.cha = value;
    affect_total(vict);
    break;
  case 18:
    vict->real_abils.armor = RANGE(-30, 20);
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 21:
    vict->points.exp = RANGE(0, 250000000);
    break;
  case 22:
    vict->real_abils.hitroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 23:
    vict->real_abils.damroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 24:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char("Вы пока не достигли такого уровня божественности!\r\n", ch);
      return (0);
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
    break;
  case 25:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char("Это могут сделать только Боги!\r\n", ch);
      return (0);
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict && on) {
      send_to_char("Вам этого лучше не делать!\r\n", ch); /*Better not -- could be a long winter*/
      return (0);
    }
    SET_OR_REMOVE(vict->char_specials.saved.Act.flags[0], PLR_FROZEN);
    break;
  case 27:
       SET_OR_REMOVE(vict->player_specials->saved.GodsLike, GF_REMORT);

	  if (GET_GOD_FLAG(vict, GF_REMORT))
	     sprintf(output, "Вы установили право на реморт %s.", GET_DNAME(vict));
          else
       	    sprintf(output, "Вы отключили право на реморт у %s.", GET_VNAME(vict));
     break;
  case 28:
     max_char_to_boot = atoi(val_arg);
        break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "вык")) {
      GET_COND(vict, (mode - 29)) = (char) -1; /* warning: magic number here */
      sprintf(output, "У %s значение \"%s\" выключено .", GET_RNAME(vict), set_fields[mode].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (mode - 29)) = (char) value; /* and here too */
      sprintf(output, "%s установить значение \"%s\" в %d.", GET_DNAME(vict),
	      set_fields[mode].cmd, value);
    } else {
      send_to_char("Может быть \"вык\" или значение от 0 до 24.\r\n", ch);
      return (0);
    }
    break;
  case 32:
    SET_OR_REMOVE(vict->char_specials.saved.Act.flags[0], PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(vict->char_specials.saved.Act.flags[0], PLR_THIEF);
    break;
  case 34:
    if (value > GET_LEVEL(ch) || value > LVL_IMPL) {
      send_to_char("Вы не можете это сделать.\r\n", ch);
      return (0);
    }
    RANGE(0, LVL_IMPL);
    vict->player.level = (byte) value;
    break;
  case 35:
    if ((rnum = real_room(value)) < 0) {
      send_to_char("Нет комнаты с таким номером.\r\n", ch); 
      return (0);
    }
    if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
      char_from_room(vict);
    char_to_room(vict, rnum);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(vict->char_specials.saved.Act.flags[0], PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(vict->char_specials.saved.Act.flags[0], PLR_DELETED);
    break;
  case 39:
        value = atoi(val_arg);
	    if (parse_class(value) == CLASS_UNDEFINED) {
      send_to_char("Такого класса пока не существует.\r\n", ch);
      return (0);
    }
    GET_CLASS(vict) = RANGE(0, 10);
    break;
  case 40:
      free_mkill(vict);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (!str_cmp(val_arg, "вык")) {
      REMOVE_BIT(vict->char_specials.saved.Act.flags[0], PLR_LOADROOM);
    } else if (is_number(val_arg)) {
      rvnum = atoi(val_arg);
      if (real_room(rvnum) != NOWHERE) {
    SET_BIT(vict->char_specials.saved.Act.flags[0], PLR_LOADROOM);
//	SET_BIT(PLR_FLAGS(vict, PLR_LOADROOM), PLR_LOADROOM);
	GET_LOADROOM(vict) = rvnum;
	sprintf(output, "%s будет входить в игру из #%d комнаты.", GET_NAME(vict),
		GET_LOADROOM(vict));
      } else {
	send_to_char("Этой комнаты не существует!\r\n", ch);
	return (0);
      }
    } else {
      send_to_char("Может быть \"вык\" или комнаты с виртуальным номером.\r\n", ch);
      return (0);
    }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
    break;
  case 44:
    if (!IS_IMPL(ch) || !IS_NPC(vict))
		return (0);
		GET_IDNUM(vict) = value;
    break;
  case 45:
       if (!IS_IMPL(ch) && !GET_COMMSTATE(ch) && ch != vict)
       {send_to_char("Не используйте эту команду.\r\n", ch);
        return (0);
       }
       if (IS_IMPL(vict) && ch != vict && !GET_COMMSTATE(ch))
       {send_to_char("Вы не можете это изменить.\r\n", ch);
        return (0);
       }
    strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);
    *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
    sprintf(output, "Пороль изменен на '%s'.", val_arg);
    break;
  case 46:
    SET_OR_REMOVE(vict->char_specials.saved.Act.flags[0], PLR_NODELETE);
    break;
  case 47:
    if ((i = search_block(val_arg, genders, FALSE)) < 0) {
      send_to_char("Должно быть 'мужчина', 'женщина', или 'нентральный'.\r\n", ch);
      return (0);
    }
    GET_SEX(vict) = i;
    break;
  case 48:	/* set age */
    if (value < 2 || value > 200) {	/* Arbitrary limits. */
      send_to_char("Возраст от 2 до 200 принятый.\r\n", ch);
      return (0);
    }
    /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
    vict->player_specials->time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
    break;

  case 49:	/* Blame/Thank Rick Glover. :) */
    GET_HEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 50:
    GET_WEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 51:
    if ((i = parse_race(*val_arg)) == CLASS_UNDEFINED) {
      send_to_char("Такой расы не существует.\r\n", ch);
      return (0);
    }
    GET_RACE(vict) =i;
    break;

  case 52:
    set_ptitle(vict, val_arg);
    if (!GET_PTITLE(vict))
	{ sprintf(output, "Предтитул %s удален.", GET_RNAME(vict));
          break;
	}
    sprintf(output, "Новый предтитул %s: %s.\r\n", GET_RNAME(vict), GET_PTITLE(vict));
     break;
 case 53:
    /* изменение имени !!! */

    if ((i = sscanf(val_arg,"%s %s %s %s %s %s",npad[0],npad[1],npad[2],npad[3],npad[4],npad[5])) != 6)
       {sprintf(buf,"Для переименования, необходимо 6 падежей, вписано %d\r\n",i);
        send_to_char(buf,ch);
        return (0);
       }

    if (*npad[0] == '*')
       {// Only change pads
       if (!_parse_name(npad[0],npad[0]))
			{if (GET_NAME(vict))
			free(GET_NAME(vict));
			CREATE(vict->player.name,char,strlen(npad[0])+1);
			strcpy(GET_NAME(vict),npad[0]);
			}
	  if (!_parse_name(npad[1],npad[1]))
			{if (GET_RNAME(vict))
			free(GET_RNAME(vict));
			CREATE(GET_RNAME(vict),char,strlen(npad[1])+1);
			strcpy(GET_RNAME(vict),npad[1]);
			}
	  if (!_parse_name(npad[2],npad[2]))
			{if (GET_DNAME(vict))
			free(GET_DNAME(vict));
			CREATE(GET_DNAME(vict),char,strlen(npad[2])+1);
			strcpy(GET_DNAME(vict),npad[2]);
			}
	  if (!_parse_name(npad[3],npad[3]))
			{if (GET_VNAME(vict))
			free(GET_VNAME(vict));
			CREATE(GET_VNAME(vict),char,strlen(npad[3])+1);
			strcpy(GET_VNAME(vict),npad[3]);
			}
	  if (!_parse_name(npad[4],npad[4]))
			{if (GET_TNAME(vict))
			free(GET_TNAME(vict));
			CREATE(GET_TNAME(vict),char,strlen(npad[4])+1);
			strcpy(GET_TNAME(vict),npad[4]);
			}
	  if (!_parse_name(npad[5],npad[5]))
			{if (GET_PNAME(vict))
			free(GET_PNAME(vict));	
			CREATE(GET_PNAME(vict),char,strlen(npad[5])+1);
			strcpy(GET_PNAME(vict),npad[5]);
			}
	  sprintf(buf,"Произведена замена падежей.\r\n");
      send_to_char(buf,ch);	
       }
    else
       {if (_parse_name(npad[0],npad[0])      ||
            strlen(npad[0]) < MIN_NAME_LENGTH ||
	    strlen(npad[0]) > MAX_NAME_LENGTH ||
            !Valid_Name(npad[0])              ||
	    reserved_word(npad[0])            ||
	    fill_word(npad[0])
           )
           {send_to_char("Некорректное имя.\r\n",ch);
            return (0);
           }
        
      if (get_id_by_name(npad[0]) >= 0) {
            send_to_char("Такой персонаж уже существует или не удалился из таблицы.\r\n"
                         "Для исключения различного рода недоразумений имя отклонено.\r\n", ch);
            return (0);
         }

        ptnum = get_ptable_by_name(GET_NAME(vict));
        if (ptnum < 0)
           return (0);

	/*	free(player_table[ptnum].name);
        CREATE(player_table[ptnum].name,char,strlen(npad[0])+1);
	    strcpy(player_table[ptnum].name, npad[0]);*/

      if (!_parse_name(npad[0],npad[0]))
			{if (GET_NAME(vict))
			free(GET_NAME(vict));
			CREATE(vict->player.name,char,strlen(npad[0])+1);
			strcpy(GET_NAME(vict),npad[0]);
			}
	  if (!_parse_name(npad[1],npad[1]))
			{if (GET_RNAME(vict))
			free(GET_RNAME(vict));
			CREATE(GET_RNAME(vict),char,strlen(npad[1])+1);
			strcpy(GET_RNAME(vict),npad[1]);
			}
	  if (!_parse_name(npad[2],npad[2]))
			{if (GET_DNAME(vict))
			free(GET_DNAME(vict));
			CREATE(GET_DNAME(vict),char,strlen(npad[2])+1);
			strcpy(GET_DNAME(vict),npad[2]);
			}
	  if (!_parse_name(npad[3],npad[3]))
			{if (GET_VNAME(vict))
			free(GET_VNAME(vict));
			CREATE(GET_VNAME(vict),char,strlen(npad[3])+1);
			strcpy(GET_VNAME(vict),npad[3]);
			}
	  if (!_parse_name(npad[4],npad[4]))
			{if (GET_TNAME(vict))
			free(GET_TNAME(vict));
			CREATE(GET_TNAME(vict),char,strlen(npad[4])+1);
			strcpy(GET_TNAME(vict),npad[4]);
			}
	  if (!_parse_name(npad[5],npad[5]))
			{if (GET_PNAME(vict))
			free(GET_PNAME(vict));	
			CREATE(GET_PNAME(vict),char,strlen(npad[5])+1);
			strcpy(GET_PNAME(vict),npad[5]);
			}
        
			if (GET_NAME(vict))
           free(GET_NAME(vict));
        CREATE(vict->player.name,char,strlen(npad[0])+1);
        strcpy(GET_NAME(vict),npad[0]);

        free(player_table[ptnum].name);
        CREATE(player_table[ptnum].name,char,strlen(npad[0])+1);
        for (i=0, player_table[ptnum].name[i] = '\0'; npad[0][i]; i++)
            player_table[ptnum].name[i] = LOWER(npad[0][i]);

        return_code = 2;
       SET_BIT(PLR_FLAGS(vict, PLR_CRASH), PLR_CRASH);
         }
		break;
	case 54:
	 GET_USTALOST(vict) = RANGE(10, 200);
	break;
    case 55:
      reason = one_argument(val_arg, num);
		if (num && *num)
		times=atol(num);

	 if ( *num && reason && *reason )
      {
        skip_spaces(&reason);
        times = atol(num);
        HELL_DURATION(vict) = (times > 0) ? time(NULL) + times * 60 * 60 : MAX_TIME;
        sprintf( num, "%s : %s", GET_NAME(ch), reason );
       if ( HELL_REASON(vict) ) free( HELL_REASON(vict) ); 
        HELL_REASON(vict) = strcpy( (char*)(malloc(strlen(num)+1)), num );
      }
      else
      {
         if ( GET_LEVEL(ch) < LVL_GOD ) 
         {
           send_to_char("Вы не имеете права сделать это.\r\n", ch);
           return false;
         }
         HELL_DURATION(vict) = 0;
        if ( HELL_REASON(vict) ) free( HELL_REASON(vict) );
         HELL_REASON(vict) = NULL;
      }


      if (HELL_DURATION(vict))
         {if (PLR_FLAGGED(vict, PLR_HELLED))
             {send_to_char("Ваша жертва и так в темнице.\r\n", ch);
	          return false;
             }
          SET_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);
          sprintf(buf,"%s$N поместил$Y Вас в АД.%s",
                  CCRED(vict,C_NRM),CCNRM(vict,C_NRM));
          act(buf,FALSE,vict,0,ch,TO_CHAR);
          act("Вы посадили в АД $V!", FALSE, ch, 0, vict, TO_CHAR);
          act("$n посажен$y в АД!", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, r_helled_start_room);
          look_at_room(vict, r_helled_start_room);
          act("$n посажен$y в АД!", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed to hell by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      else
         {if (!PLR_FLAGGED(vict, PLR_HELLED))
             {send_to_char("Ваша жертва и так на свободе.\r\n", ch);
	          return false;
             }
          REMOVE_BIT(PLR_FLAGS(vict, PLR_HELLED), PLR_HELLED);
          send_to_char("Вас выпустили из АДА.\r\n", vict);
          act("Вы выпустили из АДА $V!", FALSE, ch, 0, vict, TO_CHAR);
         
		 if ((result = GET_LOADROOM(vict)) == NOWHERE)
                result = calc_loadroom(vict);

          result = real_room(result);

          if (result == NOWHERE)
             {if (GET_LEVEL(vict) >= LVL_IMMORT)
                 result = r_immort_start_room;
              else
                 result = r_mortal_start_room;
             }

          act("$n выпущен$y из АДА!", FALSE, vict, 0, 0, TO_ROOM);
          char_from_room(vict);
          char_to_room(vict, result);
          look_at_room(vict, result);
          act("$n выпущен$y из АДА!", FALSE, vict, 0, 0, TO_ROOM);
          sprintf(buf, "(GC) %s removed from hell by %s(%dh).", GET_NAME(vict), GET_NAME(ch), times);
          mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
         }
      break;

    case 56:
    if (sscanf(val_arg,"%d %s", &ptnum, npad[0]) != 2)
       {send_to_char("Формат : set <имя> trgquest <quest_num> <on|off>\r\n", ch);
        return (0);
       }
    if (!str_cmp(npad[0],"off") || !str_cmp(npad[0],"вык"))
       {for (i = j = 0; j < vict->Questing.count; i++, j++)
            {if (vict->Questing.quests[i] == ptnum)
                j++;
             vict->Questing.quests[i] = vict->Questing.quests[j];
            }
        if (j > i)
           vict->Questing.count--;
        else
           {act("$N не выполнял$Y этого квеста.",FALSE,ch,0,vict,TO_CHAR);
            return (0);
           }
       }
    else
    if (!str_cmp(npad[0],"on") || !str_cmp(npad[0],"вкл"))
       {set_quested(vict,ptnum);
       }
    else
       {
        send_to_char("Требуется on или off (\"вкл\" или \"вык\").\r\n",ch);
        return (0);
       }
    break;

case 57:
    skip_spaces(&val_arg);
    if (!val_arg || !*val_arg || ((j = atoi(val_arg)) == 0 && str_cmp("ноль", val_arg)))
       {sprintf(output,"%s заработал%s %d %s славы.", GET_NAME(vict),
                GET_CH_SUF_1(vict), GET_GLORY(vict), desc_count(GET_GLORY(vict), WHAT_POINT));
        return_code = 0;
       }
    else
       {if (*val_arg == '-' ||
            *val_arg == '+'
	   )
           GET_GLORY(vict) = MAX(0,GET_GLORY(vict) + j);
        else
           GET_GLORY(vict) = j;
        sprintf(output,"Количество славы, которое заработал%s %s установлено в %d %s.",
                GET_CH_SUF_1(vict), GET_NAME(vict), GET_GLORY(vict), desc_count(GET_GLORY(vict), WHAT_POINT));
       }
    break;
   case 58:
   RANGE(3, 36);
    GET_STR_ROLL(vict) = value;
    affect_total(vict);
    break;
   case 59:
    RANGE(6, 36);
    GET_INT_ROLL(vict) = value;
    affect_total(vict);
    break;
   case 60:
    RANGE(6, 36);
    GET_WIS_ROLL(vict) = value;
    affect_total(vict);
    break;
   case 61:
    RANGE(6, 36);
    GET_DEX_ROLL(vict) = value;
    affect_total(vict);
    break;
   case 62:
    RANGE(6, 36);
    GET_CON_ROLL(vict) = value;
    affect_total(vict);
    break;
   case 63:
    RANGE(6, 36);
    GET_CHA_ROLL(vict) = value;
    affect_total(vict);
    break;
  case 64:
      if (valid_email(val_arg))
	 { strncpy(GET_EMAIL(vict), val_arg, 127);
	  *(GET_EMAIL(vict)+127) = '\0';
	 }
      else	
         {send_to_char("Неверный формат E-Mail.\r\n",ch);
	  return (0);
	 }
      break;
  case 65:
    skip_spaces(&val_arg);
    if (!val_arg || !*val_arg || ((j = atoi(val_arg)) == 0 && str_cmp("ноль", val_arg)))
       {sprintf(output,"%s заработал%s %d %s славы.", GET_NAME(vict),
                GET_CH_SUF_1(vict), GET_QUESTPOINTS(vict), desc_count(GET_QUESTPOINTS(vict), WHAT_POINT));
        return_code = 0;
       }
    else
       {if (*val_arg == '-' ||
            *val_arg == '+'
	   )
           GET_QUESTPOINTS(vict) = MAX(0,GET_QUESTPOINTS(vict) + j);
        else
           GET_QUESTPOINTS(vict) = j;
        sprintf(output,"Количество славы, которое заработал%s %s установлено в %d %s.",
                GET_CH_SUF_1(vict), GET_NAME(vict), GET_QUESTPOINTS(vict), desc_count(GET_QUESTPOINTS(vict), WHAT_POINT));
       }
    break;
  case 66:
  if (IS_NPC(vict))
	  { send_to_char("Вы не можете это делать с мобом!\r\n", ch);
	    break;
	  }
	
    GET_CLAN(vict) = value;   
   break;
  case 67:
   if (!GET_GOD_FLAG(vict, GF_REMORT))
        sprintf(output,"%s не имеет право на перевоплощение!",GET_NAME(vict));
        do_remort(vict, NULL, 0,0);
        break;
  case 68:
	IND_POWER_CHAR(vict) = value;//накопительная индивидуальная мощность 18.03.2006г.
   break;
  case 69:
	IND_SHOP_POWER(vict) = value;//покупательская индивидуальная мощность 18.03.2006г.
   break;
  case 70:
	POWER_STORE_CHAR(vict) = value;//индивидуальная мощность до вступления в клан 18.03.2006г.
   break;
  case 71:
	GET_CLAN(vict) = value;
  break;
  case 72:
	GET_CLAN_RANK(vict) = value;
  break;
  case 73:
        SET_OR_REMOVE (PLR_FLAGS (vict, PLR_IMMCAST), PLR_IMMCAST);
  break;
  case 74:
	SET_OR_REMOVE (PLR_FLAGS (vict, PLR_IMMKILL), PLR_IMMKILL);  
  break;
  case 75:
	if (on)
	  { if (GET_GOD_FLAG(vict, GF_PERSLOG))
	       { sprintf(output,"&6Персональный лог для %s уже включен.&n",GET_VNAME(vict));
                 break;
	       }
	    SET_GOD_FLAG(vict, GF_PERSLOG);
	   }
        else
	if (off) 
	   CLR_GOD_FLAG(vict, GF_PERSLOG);
	break;
  case 76:
	  generate_quest(vict, NULL);
	  sprintf(output,"%s получил$y в задание уничтожить %s!", GET_NAME(vict),  GET_VNAME(get_mob_by_id(GET_QUESTMOB(vict))));
	break;
  case 77:// не может писать на досках
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOWBOARD);
    break;

  default:
    send_to_char("Это присвоить Невозможно!!!\r\n", ch);
    return (0);
  }
  strcat(output, "\r\n");
  send_to_char(CAP(output), ch);
  return (return_code);
}


ACMD(do_set)
{
  CHAR_DATA *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH],
       val_arg[MAX_INPUT_LENGTH],OName[MAX_INPUT_LENGTH];
  int  mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop(argument, name, buf);

  if (!strcmp(name, "файл")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "игрок")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "моб"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);
  strcpy(val_arg, buf);

  if (!*name || !*field) {
    send_to_char("Usage: set <victim> <field> <value>\r\n", ch);
    return;
  }

  // Ищем цель
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, FIND_CHAR_WORLD))) {
	send_to_char("В данный момент игрок с таким именем отсутствует.\r\n", ch); 
	return;
      }
    } else { // Это моб? 
      if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
	send_to_char("Такого моба нет.\r\n", ch);
	return;
      }
    }
  } else if (is_file) {
    // Загружаем персонажа из файла 
    CREATE(cbuf, CHAR_DATA, 1);
    clear_char(cbuf);
	
     if ((player_i = load_char(name, cbuf)) > -1) {
      if (GET_LEVEL(cbuf) >= GET_LEVEL(ch) && !GET_COMMSTATE(ch)) {
	free_char(cbuf);
	send_to_char("Жаль, Вы не можете этого сделать.\r\n", ch);
	return;
      }
      load_pkills(cbuf);
	  vict = cbuf;
    } else {
      free(cbuf);
      send_to_char("В данный момент времени игрок с таким именем отсутствует.\r\n", ch);
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strncmp(field, set_fields[mode].cmd, len))
      break;

  /* perform the set */
  strcpy(OName,GET_NAME(vict));
  retval = perform_set(ch, vict, mode, val_arg);

  /* save the character if a change was made */
 if (retval && !IS_NPC(vict))
     {if (retval == 2)
         {rename_char(vict,OName);
         }
      else
         {if (!is_file && !IS_NPC(vict))
             {save_char(vict, NOWHERE);
             }
          if (is_file) {
	      save_char(vict,GET_LOADROOM(vict));
              send_to_char("Файл сохранен.\r\n", ch);
          }
         }
     }
  
  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

ACMD(do_liblist)
{

  int first, last, nr, found = 0;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2) {
    switch (subcmd) {
      case SCMD_RLIST:
        send_to_char("Использование: ксписок <начальный номер> <конечный номер>\r\n", ch);
        break;
      case SCMD_OLIST:
        send_to_char("Использование: осписок <начальный номер> <конечный номер>\r\n", ch);
        break;
      case SCMD_MLIST:
        send_to_char("Использование: мсписок <начальный номер> <конечный номер>\r\n", ch);
        break;
      case SCMD_ZLIST:
        send_to_char("Использование: зсписок <начальный номер> <конечный номер>\r\n", ch);
        break;
      default:
        sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
        mudlog(buf, BRF, LVL_GOD, TRUE);
        break;
    }
    return;
  }

  first = atoi(buf);
  last = atoi(buf2);

  if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
    send_to_char("Значения должны быть между 0 и 99999.\n\r", ch);
    return;
  }

  if (first >= last) {
    send_to_char("Второе значение должно быть больше первого.\n\r", ch);
    return;
  }

  if (first + 200 < last) {
    send_to_char("Максимальный показываемый промежуток - 200.\n\r", ch);
    return;
  }


  switch (subcmd) {
    case SCMD_RLIST:
      sprintf(buf, "Список комнат от Vnum %d до %d\r\n", first, last);
      for (nr = FIRST_ROOM; nr <= top_of_world && (world[nr].number <= last); nr++) {
        if (world[nr].number >= first) {
          sprintf(buf, "%s%5d. [%5d] (%3d) %s\r\n", buf, ++found,
                  world[nr].number, world[nr].zone,
                  world[nr].name);
        }
      }
      break;
    case SCMD_OLIST:
      sprintf(buf, "Список объектов Vnum %d до %d\r\n", first, last);
      for (nr = 0; nr <= top_of_objt; nr++)
      { if (obj_index[nr].vnum >= first && obj_index[nr].vnum <= last)
       {  switch (obj_proto[nr].obj_flags.type_flag)
			{ case ITEM_WEAPON:
				sprintf(buf, "%s%5d. [%5d] %-30s Повреждение: [%2dd%-2d]\r\n", buf, ++found,
                obj_index[nr].vnum,
                obj_proto[nr].short_description,
		obj_proto[nr].obj_flags.value[1],
		obj_proto[nr].obj_flags.value[2]);
		        break;
			  case ITEM_ARMOR:
                sprintf(buf, "%s%5d. [%5d] %-30s Защита (АС): [%2d] Броня: (ARMOUR) [%d]\r\n", buf, ++found,
                obj_index[nr].vnum,
                obj_proto[nr].short_description,
		obj_proto[nr].obj_flags.value[0],
		obj_proto[nr].obj_flags.value[1]);
				break;
			  default:
                sprintf(buf, "%s%5d. [%5d] %s\r\n", buf, ++found,
                obj_index[nr].vnum,
                obj_proto[nr].short_description);
				  break;
  
			}

      /*    sprintf(buf, "%s%5d. [%5d] %s\r\n", buf, ++found,
                  obj_index[nr].vnum,
                  obj_proto[nr].short_description);*/
        }
      }
      break;
    case SCMD_MLIST:
      sprintf(buf, "Список мобов от %d до %d\r\n", first, last);
      for (nr = 0; nr <= top_of_mobt; nr++)
     {  if (mob_index[nr].vnum >= first && mob_index[nr].vnum <= last) {
          sprintf(buf, "%s%5d. [%5d] %-25s Ур: [%2d]  H: [%5d] Dam: [%2d] Руки: [%2dd%-2d] M: [%2d] Gld: [%d]\r\n", buf, ++found,
                    mob_index[nr].vnum,
                    mob_proto[nr].player.short_descr,
		    mob_proto[nr].player.level,
		    mob_proto[nr].points.hit,
		    mob_proto[nr].real_abils.damroll +
		    str_app[mob_proto[nr].real_abils.str + mob_proto[nr].add_abils.str_add].todam,
		    mob_proto[nr].mob_specials.damnodice,
		    mob_proto[nr].mob_specials.damsizedice,
		    mob_proto[nr].mob_specials.max_factor,
		    mob_proto[nr].points.gold);
        } 
      }     
       break;
    case SCMD_ZLIST:
      sprintf(buf, "Список зон от %d до %d\r\n", first, last);
      for (nr = 0; nr <= top_of_zone_table && (zone_table[nr].number <= last);
 nr++) {
        if (zone_table[nr].number >= first) {
          sprintf(buf, "%s%5d. [%5d] (%3d) %s\r\n", buf, ++found,
                  zone_table[nr].number, zone_table[nr].lifespan,
                  zone_table[nr].name);
        }
      }
      break;
    default:
      sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
      mudlog(buf, BRF, LVL_GOD, TRUE);
      return;
  }

  if (!found) {
    switch (subcmd) {
      case SCMD_RLIST:
        send_to_char("Нет комнат в этом промежутке.\r\n", ch);
        break;
      case SCMD_OLIST:
        send_to_char("Нет объектов в этом промежутке.\r\n", ch);
        break;
      case SCMD_MLIST:
        send_to_char("Нет мобов в этом промежутке.\r\n", ch);
        break;
      case SCMD_ZLIST:
        send_to_char("Нет зон в этом промежутке.\r\n", ch);
        break;
      default:
        sprintf(buf, "SYSERR:: invalid SCMD passed to do_build_list!");
        mudlog(buf, BRF, LVL_GOD, TRUE);
        break;
    }
    return;
  }

  page_string(ch->desc, buf, 1);
}

#ifdef MULTING
int multing_mode = 1;
#else
int multing_mode = 0;
#endif

ACMD(do_multing)
{
   one_argument(argument, buf);

   char *x = &buf[0];
   skip_spaces(&x);
   
   if (cmpstr(x, "off") || !str_cmp(x, "0") || cmpstr(x, "выкл"))
   {
       multing_mode = 0;
   }
   else if (cmpstr(x, "on") || !str_cmp(x, "1") || cmpstr(x, "вкл"))
   {
       multing_mode = 1;    
   }
   else
   {
       if (strlen(x))
       {
           send_to_char("Неверный параметр (выкл/вкл 0/1 on/off)\r\n", ch); 
           return;
       }
   }
   sprintf(buf, "Мультинг (вход с одного ip-адреса): %s\r\n", multing_mode ? "&Gразрешен&n" : "&Rзапрещен&n");
   send_to_char(buf, ch);
}
