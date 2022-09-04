/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <fstream>


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"

extern struct descriptor_data *descriptor_list;
extern struct time_data time_info;
extern struct room_data *world;
extern struct char_data *mob_proto;

int  attack_best(struct char_data *ch, struct char_data *victim);
char *House_rank(struct char_data *ch);
char *House_sname(struct char_data *ch);
char *House_name(struct char_data *ch);
const char *tiny_affect_loc_name(int location);
/* local functions */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void die_follower(struct char_data * ch);
void add_follower(struct char_data * ch, struct char_data * leader);
void prune_crlf(char *txt);
int valid_email(const char *address);


/* creates a random number in interval [from;to] */
int number(int from, int to)
{
  /* error checking in case people call number() incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
    log("SYSERR: number() should be called with lowest, then highest. number(%d, %d), not number(%d, %d).", from, to, to, from);
  }

  return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int number, int size)
{
  int sum = 0;

  if (size <= 0 || number <= 0)
    return (0);

  while (number-- > 0)
    sum += ((circle_random() % size) + 1);

  return (sum);
}


int MIN(int a, int b)
{
  return (a < b ? a : b);
}


int MAX(int a, int b)
{
  return (a > b ? a : b);
}


char *CAP(char *txt)
{
	if(*txt<='ю')
		*txt=(*txt + 32);
	  else
	*txt = UPPER(*txt);
  return (txt);
}


char *str_add(char *dst, const char *src)
{
	if (dst == NULL) {
		dst = (char *) malloc(strlen(src) + 1);
		strcpy(dst, src);
	} else {
		dst = (char *) realloc(dst, strlen(dst) + strlen(src) + 1);
		strcat(dst, src);
	};
	return dst;
}


char *str_dup(const char *source)
{
	char *new_z = NULL;
	if (source) {
		CREATE(new_z, char, strlen(source) + 1);
		return (strcpy(new_z, source));
	}
	CREATE(new_z, char, 1);
	return (strcpy(new_z, ""));
}


/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}

/* cmpstr a case-insensitive version of strcmp 
 * but checks only first letters, if it exists */

bool cmpstr(const char *arg1, const char *arg2)
{
  if (arg1 == NULL || arg2 == NULL) 
        return false;

  if (!*arg1 || !*arg2)
  {
      return (*arg1 == *arg2) ? true : false;
  }
  
  for (int i = 0; arg1[i] && arg2[i]; i++) 
  {
      if (LOWER(arg1[i]) != LOWER(arg2[i]))
		return false;
  }
  return true;
}

/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
 /* Программа проверки входного пароля персонажа*/
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
  //  log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

       for (i = 0; arg1[i] || arg2[i]; i++) 
  if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      
		return (chk);	/* not equal */

  return (0);
}


/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}

/*функции предназначенные для обработки русских символов*/

int is_ascii(char c)
{
  return (isascii((unsigned char)c) || ((unsigned char)c >= 0xA8));	
}

int is_alpha(char c)
{
  return (((unsigned char)c >= 0xC0) || isalpha((unsigned char)c));
}

int is_space(char c)
{
  return (isspace((unsigned char)c) && ((unsigned char)c < 0xC0));
}

int is_alnum(char c)
{ 
	return (isalnum((unsigned char)c) && ((unsigned char)c < 0xC0));
}

/* log a death trap hit */
void log_death_trap(struct char_data * ch)
{
  char buf[256];

  sprintf(buf, "%s death trap #%d (%s)", GET_NAME(ch),
	  GET_ROOM_VNUM(IN_ROOM(ch)), world[ch->in_room].name);
  mudlog(buf, BRF, LVL_IMMORT, TRUE);
}

/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_log(const char *format, ...)
{
  va_list args;
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL)
    puts("SYSERR: Using log() before stream was initialized!");
  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);

  va_start(args, format);
  vfprintf(logfile, format, args);
  va_end(args);

  fprintf(logfile, "\n");
  fflush(logfile);
}


void pers_log(char *orig_name, const char *format, ...)
{
	char filename[MAX_NAME_LENGTH + 1];
	if (orig_name == NULL || *orig_name == '\0') {
		log("SYSERR: NULL pointer passed to pers_log(), %p.", orig_name);
		return;
	}

	get_filename(orig_name, filename, ETEXT_FILE);

	FILE *perslog;

	if (!(perslog = fopen(filename, "a"))) {
		log("SYSERR: error open %s", filename);
		return;
	}

	va_list args;
	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));

	if (format == NULL)
		format = "SYSERR: perslog() received a NULL format.";

	time_s[strlen(time_s) - 1] = '\0';

	fprintf(perslog, "%-15.15s :: ", time_s + 4);

	va_start(args, format);
	vfprintf(perslog, format, args);
	va_end(args);

	fprintf(perslog, "\n");
	fclose(perslog);
}

/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(const char *str, int type, int level, int file)
{
  char buf[MAX_STRING_LENGTH], tp;
  struct descriptor_data *i;

  if (str == NULL)
    return;	/* eh, oh well. */
  if (file)
    log(str, "");
  if (level < 0)
    return;

  sprintf(buf, "[ %s ]\r\n", str);

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_LEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
	  (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));
    if (tp < type)
      continue;

    send_to_char(CCGRN(i->character, C_NRM), i->character);
    send_to_char(buf, i->character);
    send_to_char(CCNRM(i->character, C_NRM), i->character);
  }
}



/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
int sprintbit(bitvector_t bitvector, const char *names[], char *result)
{
  long nr=0, fail=0, divider=FALSE;
  *result = '\0';

  if ((unsigned long)bitvector < (unsigned long)INT_ONE)
      ;
  else
  if ((unsigned long)bitvector < (unsigned long)INT_TWO)
     fail = 1;
  else
  if ((unsigned long)bitvector < (unsigned long)INT_THREE)
     fail = 2;
  else
     fail = 3;
  bitvector &= 0x3FFFFFFF;
  while (fail)
        {if (*names[nr] == '\n')
            fail--;
         nr++;
        }
 
  for (; bitvector; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      if (*names[nr] != '\n') {
	strcat(result, names[nr]);
	strcat(result, ", ");
      divider = TRUE;
	  } else {
	strcat(result, "UNDEFINED ");
    strcat(result, ",");
	divider = TRUE;
	  }
	  }
    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result) {
    strcpy(result, "НЕ УСТАНОВЛЕН ");
  return FALSE;
  }
 else
  if (divider)
     *(result + strlen(result) - 1) = '\0';
  return TRUE; 
}



void sprinttype(int type, const char *names[], char *result)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  if (*names[nr] != '\n')
    strcpy(result, names[nr]);
  else
    strcpy(result, "UNDEFINED");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / (SECS_PER_MUD_HOUR * TIME_KOEFF)) % HOURS_PER_DAY;	/* 0..23 hours */
  secs     -= SECS_PER_MUD_HOUR * TIME_KOEFF * now.hours;

  now.day = (secs / (SECS_PER_MUD_DAY * TIME_KOEFF)) % DAYS_PER_MONTH; /* 0..29 days  */
  secs -= SECS_PER_MUD_DAY * TIME_KOEFF * now.day;

  now.month = (secs / (SECS_PER_MUD_MONTH * TIME_KOEFF)) % MONTHS_PER_YEAR; /* 0..11 months */
  secs -= SECS_PER_MUD_MONTH * TIME_KOEFF * now.month;

  now.year = (secs / (SECS_PER_MUD_YEAR * TIME_KOEFF)); /* 0..XX? years */

  return (&now);
}
  
  
  /*  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	// 0..23 hours 
   secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;	// 0..34 days  
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17;	// 0..16 months 
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	// 0..XX? years 

  return (&now);
}*/



struct time_info_data *age(struct char_data * ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->player_specials->time.birth);

  player_age.year += 17;	/* All players start at 17 */

  return (&player_age);
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data * ch, struct char_data * victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}

void make_horse(struct char_data *horse, struct char_data *ch)
{
	SET_BIT(AFF_FLAGS(horse, AFF_HORSE), AFF_HORSE);
	add_follower(horse, ch);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_WIMPY), MOB_WIMPY);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_SENTINEL), MOB_SENTINEL);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_HELPER), MOB_HELPER);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
	REMOVE_BIT(MOB_FLAGS(horse, MOB_MOUNTING), MOB_MOUNTING);
	REMOVE_BIT(AFF_FLAGS(horse, AFF_TETHERED), AFF_TETHERED);
}

int  on_horse(struct char_data *ch)
{ return (AFF_FLAGGED(ch, AFF_HORSE) && has_horse(ch,TRUE));
}

int  has_horse(struct char_data *ch, int same_room)
{ struct follow_type *f;

  if (IS_NPC(ch))
     return (FALSE);

  for (f = ch->followers; f; f = f->next)
      {if ((IS_NPC(f->follower) || IS_IMPL(ch)) && AFF_FLAGGED(f->follower, AFF_HORSE) &&
           (!same_room || IN_ROOM(ch) == IN_ROOM(f->follower)))
          return (TRUE);
      }
  return (FALSE);
}

struct char_data *get_horse(struct char_data *ch)
{ struct follow_type *f;

  if (IS_NPC(ch))
     return (NULL);

  for (f = ch->followers; f; f = f->next)
      {if ((IS_NPC(f->follower) || IS_IMPL(ch)) && AFF_FLAGGED(f->follower, AFF_HORSE))
          return (f->follower);
      }
  return (NULL);
}

void horse_drop(struct char_data *ch)
{ if (ch->master)
     {act("$N сбросил$Y Вас со своей спины.",FALSE,ch->master,0,ch,TO_CHAR);
      REMOVE_BIT(AFF_FLAGS(ch->master, AFF_HORSE), AFF_HORSE);
      WAIT_STATE(ch->master, 3 * PULSE_VIOLENCE);
      if (GET_POS(ch->master) > POS_SITTING)
         GET_POS(ch->master) = POS_SITTING;
     }
}

void check_horse(struct char_data *ch)
{ if (!IS_NPC(ch) && !has_horse(ch, TRUE))
     REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);
}


/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
int  stop_follower(struct char_data * ch, int mode)
{ CHAR_DATA *master;
  struct follow_type *j, *k;
  int    i;

  
  if (!ch->master)
	{ log("SYSERR: stop_follower(%s) without master",GET_NAME(ch));
       return (FALSE);
    }

  act("Вы прекратили следовать за $T.", FALSE, ch, 0, ch->master, TO_CHAR);
  act("$n прекратил$y следовать за $T.", TRUE, ch, 0, ch->master, TO_NOTVICT);

//  log("[Останов последователей] Stop horse");
  if (get_horse(ch->master) == ch && on_horse(ch->master))
     horse_drop(ch);
  else
     act("$n прекратил$y следовать за Вами.", TRUE, ch, 0, ch->master, TO_VICT);

  //log("[Останов последователей] Remove from followers list");
  if (!ch->master->followers)
     log("[Stop follower] SYSERR: Followers absent for %s (master %s).",GET_NAME(ch),GET_NAME(ch->master));
  else
  if (ch->master->followers->follower == ch)
     {// Head of follower-list? 
      k                     = ch->master->followers;
      if (!(ch->master->followers = k->next) &&
          !ch->master->master)
		  REMOVE_BIT(AFF_FLAGS(ch->master, AFF_GROUP), AFF_GROUP);
          free(k);
     }
  else
     {// locate follower who is not head of list 
      for (k = ch->master->followers; k->next && k->next->follower != ch; k = k->next);
      if (!k->next)
         log("[Stop follower] SYSERR: Undefined %s in %s followers list.",GET_NAME(ch), GET_NAME(ch->master));
      else
         {j       = k->next;
          k->next = j->next;
          free(j);
         }
     }
  master     = ch->master;
  ch->master = NULL;

        REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);

  if (IS_NPC(ch) && AFF_FLAGGED(ch,AFF_HORSE))
	  	REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);
   
  //log("[Останов последователей] Освобождение от чарма");
  if (AFF_FLAGGED(ch, AFF_CHARM)  ||
      AFF_FLAGGED(ch, AFF_HELPER) ||
      IS_SET(mode, SF_CHARMLOST)
     )
    {
      if (affected_by_spell(ch,SPELL_CHARM))
         affect_from_char(ch,SPELL_CHARM);

      EXTRACT_TIMER(ch) = 5;
	  REMOVE_BIT(AFF_FLAGS(ch, AFF_CHARM), AFF_CHARM);
  
    //   log("[Останов последователей] Остановка драки чарма");
      if (FIGHTING(ch))
         stop_fighting(ch,TRUE);
     
      //log("[Останов последователей] Реакция чармов");
      if (IS_NPC(ch))
      {     if (MOB_FLAGGED(ch, MOB_CORPSE) || MOB_FLAGGED(ch, MOB_CLONE))
             {act("Налетевший ветер развеял $v, не оставив и следа.",TRUE,ch,0,0,TO_ROOM);
              GET_LASTROOM(ch) = IN_ROOM(ch);
              extract_char(ch,FALSE);
              return (TRUE);
             }
          else
          if (AFF_FLAGGED(ch, AFF_HELPER))
             REMOVE_BIT(AFF_FLAGS(ch, AFF_HELPER), AFF_HELPER);
          else
             {if (master &&
                  !IS_SET(mode, SF_MASTERDIE) &&
                  IN_ROOM(ch) == IN_ROOM(master) &&
                  CAN_SEE(ch,master) &&
                  !FIGHTING(ch)) //изменить диалог
                 {if (number(1,GET_REAL_INT(ch) * 2) > GET_REAL_CHA(master))
                     {act("$n посчитал$y, что Вы заслуживаете смерти!",FALSE,ch,0,master,TO_VICT);
                      act("$n заорал$y : \"Ты долго водил$Y меня за нос, но дальше так не пойдет!\""
                          "              \"Теперь только твоя смерть может искупить твой обман!\"",
                          TRUE, ch, 0, master, TO_NOTVICT);
                      set_fighting(ch,master);
                     }
                 }
              else
              if (master &&
                  !IS_SET(mode, SF_MASTERDIE) &&
                  CAN_SEE(ch,master) &&
                  MOB_FLAGGED(ch, MOB_MEMORY))
                 remember(ch,master);
             }
         }
     }
  //log("[Останов последователей] Переустанавливаем флаги мобам");
  if (IS_NPC(ch) && (i = GET_MOB_RNUM(ch)) >= 0)
     { 	MOB_FLAGS(ch, INT_ZERRO) = MOB_FLAGS(mob_proto + i, INT_ZERRO);
		MOB_FLAGS(ch, INT_ONE) = MOB_FLAGS(mob_proto + i, INT_ONE);
		MOB_FLAGS(ch, INT_TWO) = MOB_FLAGS(mob_proto + i, INT_TWO);
		MOB_FLAGS(ch, INT_THREE) = MOB_FLAGS(mob_proto + i, INT_THREE);
      }
   //log("[Останов последователей] Работа функции завершена");
   return (FALSE);
}


/* Called when a character that follows/is followed dies */
void die_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (ch->master)
  {
      char_data *master = ch->master;
      stop_follower(ch, FALSE);

      for (k = ch->followers; k; k = j) 
      {
        j = k->next;
        stop_follower(k->follower, FALSE);
      }

      if (IS_NPC(ch))
          return;

      int pc_alive = 0;
      for (k = master->followers; k; k = j) 
      {
         j = k->next;
         if (!IS_NPC(k->follower))
            pc_alive++;
      }

      if (pc_alive == 0)
      {
          REMOVE_BIT(AFF_FLAGS(master, AFF_GROUP), AFF_GROUP);
      }
      return;
  }

  std::vector<char_data*> alive_group_chars;
  for (k = ch->followers; k; k = j) 
  {
    j = k->next;
    if (IS_SET(AFF_FLAGS(k->follower, AFF_GROUP), AFF_GROUP) && !IS_NPC(k->follower))
    {
        alive_group_chars.push_back(k->follower);
    }
    else
    {
        stop_follower(k->follower, FALSE);
    }
  }
  
  if (alive_group_chars.empty())
     return;

  int count = alive_group_chars.size();
  if (count == 1)
  {
      stop_follower(alive_group_chars[0], FALSE);
      return;
  }
  
  // clear followers list of leader
  for (k = ch->followers; k; k = j) 
  {
     j = k->next;
     free(k);
  }
  ch->followers = NULL;
  REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);
  
  // select new leader
  char_data *new_leader = alive_group_chars[0];
  new_leader->master = NULL;
  
  sprintf(buf,"&GВы теперь новый лидер группы!&n\r\n");
  send_to_char(buf, new_leader);

  sprintf(buf,"&G%s теперь является лидером группы!&n\r\n", GET_NAME(new_leader));
  for (int i=(count-1); i>=1; --i)
  {
      alive_group_chars[i]->master = new_leader;
      CREATE(k, struct follow_type, 1);
      k->follower = alive_group_chars[i];
      k->next = new_leader->followers;
      new_leader->followers = k;
      send_to_char(buf, alive_group_chars[i]);
  }
}


/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data * ch, struct char_data * leader)
{
  struct follow_type *k;

  if (ch->master)
  { log ("SYSERR: add_follower(%s->%s) when master existing(%s)...",//перенести в линух 22.06.2005г.
	GET_NAME (ch), leader ? GET_NAME (leader) : "",
	GET_NAME (ch->master));
    return;
  }

 if (ch == leader)
     return;
  
  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  if (!IS_HORSE(ch))
  { act("Вы начали следовать за $T.", FALSE, ch, 0, leader, TO_CHAR);
//  if (CAN_SEE(leader, ch))
    act("$n начал$y следовать за Вами.", TRUE, ch, 0, leader, TO_VICT);
    act("$n начал$y следовать за $T.", TRUE, ch, 0, leader, TO_NOTVICT);

  }
}
/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do {
    fgets(temp, 256, fl);
    if (feof(fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  int len = strlen(temp);
  if (len == 0)
    return 0;

  int x2 = (len > 1) ? temp[len-2] : -1;
  if (x2 == '\r' || x2 == '\n')
  {
    len = len - 2;
  }
  else
  {
    len = len - 1;
  }

  temp[len] = '\0';
  strcpy(buf, temp);
  return (lines);
}

int get_filename(char *orig_name, char *filename, int mode)
{
  const char *prefix, *middle, *suffix;
  char name[64], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		orig_name, filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case TEXT_CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = TEXT_SUF_OBJS;
    break;
  case TIME_CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = TIME_SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case PLAYERS_FILE:
    prefix = LIB_PLRS;
    suffix = SUF_PLAYER;
    break;
  case PKILLERS_FILE:
    prefix = LIB_PLRS;
    suffix = SUF_PKILLER;
    break;
  case PQUESTS_FILE:
    prefix = LIB_PLRS;
    suffix = SUF_QUESTS;
    break;
  case PMKILL_FILE:
    prefix = LIB_PLRS;
    suffix = SUF_PMKILL;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  case PFILE_FILE:
	prefix = LIB_PLRS;
	suffix = SUF_PFILE;
	break;
  case PLAYERS_LOG:
	prefix = LIB_PLRS;
	suffix = SUF_PLOG;
	break;
  case NAME_CHEST_FILE:
	prefix = LIB_PLROBJS;
	suffix = SUF_CHEST;
	  break;
  case PLAYER_EXCHANGE:
    prefix = LIB_PLROBJS;
    suffix = SUF_EXCHANGE;
      break;
  case PLAYER_MAIL:
    prefix = LIB_PLRS;
    suffix = SUF_MAIL;
      break;
  default:
    return (0);
  }

  strcpy(name, orig_name);
  *name = LOWER(*name);
  for (ptr = name; *ptr; ptr++)
   *ptr = AtoL(*ptr);

  switch (*name) {
  case 'a':  middle = "a"; break;
  case 'b':  middle = "b"; break;
  case 'c':  middle = "c"; break;
  case 'd':  middle = "d"; break;
  case 'e':  middle = "e"; break;
  case 'f':  middle = "f"; break;
  case 'g':  middle = "g"; break;
  case 'h':  middle = "h"; break;
  case 'i':  middle = "i"; break;
  case 'j':  middle = "j"; break;
  case 'k':  middle = "k"; break;
  case 'l':  middle = "l"; break;
  case 'm':  middle = "m"; break;
  case 'n':  middle = "n"; break;
  case 'o':  middle = "o"; break;
  case 'p':  middle = "p"; break;
  case 'q':  middle = "q"; break;
  case 'r':  middle = "r"; break;
  case 's':  middle = "s"; break;
  case 't':  middle = "t"; break;
  case 'u':  middle = "u"; break;
  case 'v':  middle = "v"; break;
  case 'w':  middle = "w"; break;
  case 'x':  middle = "x"; break;
  case 'y':  middle = "y"; break;
  case 'z':  middle = "z"; break;
  case '0':  middle = "0"; break;
  case '1':  middle = "1"; break;
  case '2':  middle = "2"; break;
  case '3':  middle = "3"; break;
  case '4':  middle = "4"; break;
  case '5':  middle = "5"; break;
  case '6':  middle = "6"; break;
  case '7':  middle = "7"; break;
  case '8':  middle = "8"; break;
  case '9':  middle = "9"; break;
  default:
    middle = "zzz";
    break;
  }

  sprintf(filename, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
  return (1);
}

int check_moves(struct char_data *ch, int how_moves)
{
 if (IS_IMMORTAL(ch) || IS_NPC(ch))
    return (TRUE);
 if (GET_MOVE(ch) < how_moves)
    {send_to_char("Вы слишком устали.\r\n", ch);
     return (FALSE);
    }

 GET_MOVE(ch) -= how_moves;
 return (TRUE);
}

int real_sector(int room)
{ int sector = SECT(room);

  if (ROOM_FLAGGED(room,ROOM_NOWEATHER))
     return sector;
  switch (sector)
  {case SECT_INSIDE:
   case SECT_CITY:
   case SECT_FLYING:
   case SECT_UNDERWATER:
   case SECT_SECRET:
        return sector;
        break;
   case SECT_FIELD:
        if (world[room].weather.snowlevel > 20)
           return SECT_FIELD_SNOW;
        else
        if (world[room].weather.rainlevel > 20)
           return SECT_FIELD_RAIN;
        else
           return SECT_FIELD;
        break;
   case SECT_FOREST:
        if (world[room].weather.snowlevel > 20)
           return SECT_FOREST_SNOW;
        else
        if (world[room].weather.rainlevel > 20)
           return SECT_FOREST_RAIN;
        else
           return SECT_FOREST;
        break;
   case SECT_HILLS:
        if (world[room].weather.snowlevel > 20)
           return SECT_HILLS_SNOW;
        else
        if (world[room].weather.rainlevel > 20)
           return SECT_HILLS_RAIN;
        else
           return SECT_HILLS;
        break;
   case SECT_MOUNTAIN:
        if (world[room].weather.snowlevel > 20)
           return SECT_MOUNTAIN_SNOW;
        else
           return SECT_MOUNTAIN;
        break;
   case SECT_WATER_SWIM:
   case SECT_WATER_NOSWIM:
        if (world[room].weather.icelevel > 30)
           return SECT_THICK_ICE;
        else
        if (world[room].weather.icelevel > 20)
           return SECT_NORMAL_ICE;
        else
        if (world[room].weather.icelevel > 10)
           return SECT_THIN_ICE;
        else
           return sector;
        break;
  }
  return SECT_INSIDE;
}


void message_auction(char *message, struct char_data *ch)
{ struct descriptor_data *i;

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) == CON_PLAYING &&
           (!ch || i != ch->desc) &&
           i->character &&
	   !PRF_FLAGGED(i->character, PRF_NOAUCT) &&
	   !PLR_FLAGGED(i->character, PLR_WRITING) &&
	   !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) &&
	   GET_POS(i->character) > POS_SLEEPING)
	  {if (COLOR_LEV(i->character) >= C_NRM)
	      send_to_char(CCYEL(i->character,C_NRM), i->character);
           act(message, FALSE, i->character, 0, 0, TO_CHAR | TO_SLEEP);
           if (COLOR_LEV(i->character) >= C_NRM)
	      send_to_char(CCNRM(i->character,C_NRM), i->character);
          }
      }
}

struct auction_lot_type auction_lots[MAX_AUCTION_LOT] =
{ {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
  {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
  {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0} /*,
  {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0},
  {-1, NULL, -1, NULL, -1, NULL, -1, NULL, 0, 0}  */
};

void clear_auction(int lot)
{ if (lot < 0 || lot >= MAX_AUCTION_LOT)
     return;
  GET_LOT(lot)->seller        = GET_LOT(lot)->buyer       = GET_LOT(lot)->prefect = NULL;
  GET_LOT(lot)->seller_unique = GET_LOT(lot)->buyer_unique= GET_LOT(lot)->prefect_unique = -1;
  GET_LOT(lot)->item      = NULL;
  GET_LOT(lot)->item_id   = -1;
}

void sell_auction(int lot)
{ struct char_data *ch, *tch;
  struct obj_data  *obj;

  if (lot < 0 || lot >= MAX_AUCTION_LOT ||
      !(ch = GET_LOT(lot)->seller) || GET_UNIQUE(ch) != GET_LOT(lot)->seller_unique ||
      !(tch= GET_LOT(lot)->buyer)  || GET_UNIQUE(tch)!= GET_LOT(lot)->buyer_unique  ||
      !(obj= GET_LOT(lot)->item)   || GET_ID(obj)    != GET_LOT(lot)->item_id)
     return;

   if (obj->carried_by != ch)
      {sprintf(buf,"Аукцион: лот #%d '%s' снят, ввиду смены владельца", lot,
               obj->short_description);
       message_auction(buf,NULL);
       clear_auction(lot);
       return;
      }

    if (obj->contains)
       {sprintf(buf,"Опустошите %s перед продажей.\r\n",
                obj->short_vdescription);
        send_to_char(buf,ch);
        if (GET_LOT(lot)->tact >= MAX_AUCTION_TACT_PRESENT)
           {sprintf(buf,"Аукцион: лот #%d '%s' снят с аукциона распорядителем торгов.", lot,
                    obj->short_description);
            message_auction(buf,NULL);
            clear_auction(lot);
            return;
           }
       }

    if (GET_GOLD(tch) + GET_BANK_GOLD(tch) < GET_LOT(lot)->cost)
       {sprintf(buf,"У Вас не хватает денег на покупку %s.\r\n",
                obj->short_rdescription);
        send_to_char(buf,tch);
        sprintf(buf,"У покупателя %s не хватает денег.\r\n",
                obj->short_rdescription);
        send_to_char(buf,ch);
        sprintf(buf,"Аукцион: лот #%d '%s' снят с аукциона распорядителем торгов.", lot,
                    obj->short_description);
        message_auction(buf,NULL);
        clear_auction(lot);
        return;
       }

    if (/*IN_ROOM(ch) != IN_ROOM(tch)*/ !ROOM_FLAGGED(IN_ROOM(ch), ROOM_AUCTION) || !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
       {if (GET_LOT(lot)->tact >= MAX_AUCTION_TACT_PRESENT)
           {sprintf(buf,"Аукцион: лот #%d '%s' снят с аукциона распорядителем торгов.", lot,
                    obj->short_description);
            message_auction(buf,NULL);
            clear_auction(lot);
            return;
           }
        sprintf(buf,"Вам необходимо прибыть в комнату аукциона к $d для получения '%s'.",
                GET_LOT(lot)->item->short_rdescription);
        act(buf,FALSE,GET_LOT(lot)->seller,0,GET_LOT(lot)->buyer,TO_VICT|TO_SLEEP);
        sprintf(buf,"Вам необходимо прибыть в комнату аукциона к $D для получения денег за '%s'.",
                GET_LOT(lot)->item->short_vdescription);
        act(buf,FALSE,GET_LOT(lot)->seller,0,GET_LOT(lot)->buyer,TO_CHAR|TO_SLEEP);
        GET_LOT(lot)->tact = MAX(GET_LOT(lot)->tact,MAX_AUCTION_TACT_BUY);
        return;
       }

    sprintf(buf,"Вы продали '%s' с аукциона.\r\n&WНа Ваш счет поступило %d %s.&n\r\n",obj->short_vdescription, GET_LOT(lot)->cost, desc_count(GET_LOT(lot)->cost, WHAT_MONEYa));
    send_to_char(buf,ch);
    sprintf(buf,"Вы купили '%s' на аукционе.\r\n",obj->short_vdescription);
    send_to_char(buf,tch);
    obj_from_char(obj);
    obj_to_char(obj,tch);
    GET_BANK_GOLD(ch) += GET_LOT(lot)->cost;
    if ((GET_BANK_GOLD(tch)-= GET_LOT(lot)->cost) < 0)
       {GET_GOLD(tch)      += GET_BANK_GOLD(tch);
        GET_BANK_GOLD(tch)  = 0;
       }
    clear_auction(lot);
    return;
}

void check_auction(struct char_data *ch, struct obj_data *obj)
{int i;
 if (ch)
    {for (i = 0; i < MAX_AUCTION_LOT; i++)
         {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
               continue;
          if (GET_LOT(i)->seller == ch || GET_LOT(i)->seller_unique == GET_UNIQUE(ch) ||
              GET_LOT(i)->buyer  == ch || GET_LOT(i)->buyer_unique  == GET_UNIQUE(ch) ||
              GET_LOT(i)->prefect== ch || GET_LOT(i)->prefect_unique== GET_UNIQUE(ch))
             {sprintf(buf,"Аукцион: лот #%d '%s' снят с аукциона распорядителем.", i,
                      GET_LOT(i)->item->short_description);
              message_auction(buf,ch);
              clear_auction(i);
             }
          }
    }
 else
 if (obj)
     {for (i = 0; i < MAX_AUCTION_LOT; i++)
          {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
               continue;
           if (GET_LOT(i)->item == obj || GET_LOT(i)->item_id == GET_ID(obj))
              {sprintf(buf,"Аукцион : лот %d (%s) снят с аукциона распорядителем.", i,
                       GET_LOT(i)->item->short_description);
               message_auction(buf,obj->carried_by);
               clear_auction(i);
              }
           }
      }
   else
      {for (i = 0; i < MAX_AUCTION_LOT; i++)
           {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
               continue;
            if (GET_LOT(i)->item->carried_by != GET_LOT(i)->seller ||
                (GET_LOT(i)->buyer &&
                 (GET_GOLD(GET_LOT(i)->buyer) + GET_BANK_GOLD(GET_LOT(i)->buyer) <
                  GET_LOT(i)-> cost)))
               {sprintf(buf,"Аукцион: лот #%d '%s' снят с аукциона распорядителем.", i,
                        GET_LOT(i)->item->short_description);
                message_auction(buf,NULL);
                clear_auction(i);
               }
            }
      }
}

const char *tact_message[] =
{ "(выставляется первый раз)",
  "(выставляется второй раз)",
  "(выставляется третий раз)",
  "(выставляется четвертый раз)",
  "(выставляется пятый раз)",
  "\n"
};

void tact_auction(void)
{int i;
 check_auction(NULL,NULL);

 for (i = 0; i < MAX_AUCTION_LOT; i++)
     {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
         continue;
     if (GET_LOT(i)->buyer)
      { if(!ROOM_FLAGGED(IN_ROOM(GET_LOT(i)->buyer), ROOM_AUCTION) ||
	!ROOM_FLAGGED(IN_ROOM(GET_LOT(i)->buyer), ROOM_PEACEFUL))
	{ sprintf(buf,"Аукцион: лот #%d '%s' снят с торгов.",
              i, GET_LOT(i)->item->short_description);
              message_auction(buf,NULL);
              clear_auction(i);
              continue;
             }
      }
   if (++GET_LOT(i)->tact < MAX_AUCTION_TACT_BUY)
         {sprintf(buf,"Аукцион: лот #%d '%s', %d %s, %s", i, GET_LOT(i)->item->short_description,
                  GET_LOT(i)->cost, desc_count(GET_LOT(i)->cost, WHAT_MONEYa),
                  tact_message[GET_LOT(i)->tact]);
          message_auction(buf,NULL);
          continue;
         }
      else
      if (GET_LOT(i)->tact < MAX_AUCTION_TACT_PRESENT)
         {if (!GET_LOT(i)->buyer)
             {sprintf(buf,"Аукцион: лот #%d '%s' снят распорядителем ввиду отсутствия спроса.",
                      i, GET_LOT(i)->item->short_description);
              message_auction(buf,NULL);
              clear_auction(i);
              continue;
             }
          if (!GET_LOT(i)->prefect)
             {sprintf(buf,"Аукцион: лот #%d '%s', %d %s, &CПРОДАНО&n.", i, GET_LOT(i)->item->short_description,
                      GET_LOT(i)->cost, desc_count(GET_LOT(i)->cost, WHAT_MONEYa));
                     // tact_message[GET_LOT(i)->tact]);
              message_auction(buf,NULL);
              GET_LOT(i)->prefect        = GET_LOT(i)->buyer;
              GET_LOT(i)->prefect_unique = GET_LOT(i)->buyer_unique;
             }
          sell_auction(i);
         }
      else
         sell_auction(i);
     }
}

struct auction_lot_type *free_auction(int *lotnum)
{int i;
 for (i = 0; i < MAX_AUCTION_LOT; i++)
     {if (!GET_LOT(i)->seller && !GET_LOT(i)->item)
         {*lotnum = i;
          return (GET_LOT(i));
         }
     }

 return (NULL);
}

int obj_on_auction(struct obj_data *obj)
{int i;
 for (i = 0; i < MAX_AUCTION_LOT; i++)
     {if (GET_LOT(i)->item == obj && GET_LOT(i)->item_id == GET_ID(obj))
         return (TRUE);
     }

 return (FALSE);
}

char *noclan_title(struct char_data *ch)
{
  static char title[MAX_STRING_LENGTH];
  char * title_stop, * prefix;

  if ( !GET_TITLE(ch) || !*GET_TITLE(ch) )
  {
    sprintf( title,
             "%s %s",
             pc_race_types[(int)GET_RACE(ch)][(int)GET_SEX(ch)], 
             GET_NAME(ch)
           );
  }
  else
  {
    if ( (title_stop = strchr(GET_TITLE(ch),'/')) != NULL ) *title_stop = 0;
    if ( (prefix = strchr(GET_TITLE(ch),';')) != NULL ) *prefix = 0;

    sprintf( title,
             "%s%s%s%s%s",
             prefix ? prefix + 1 : "",
             prefix ? " " : "", 
             GET_NAME(ch),
             *GET_TITLE(ch) ? ", " : "", 
             GET_TITLE(ch)
           );

    if ( prefix ) *prefix = ';';
    if ( title_stop ) *title_stop = '/';
  }
  return title;
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
void core_dump_real(const char *who, int line)
{
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise... */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);

  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
  if (fork() == 0)
    abort();
#endif
}

const char *empty_string = "ничего";
int sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div)
{
	long nr = 0, fail = 0, divider = FALSE;

	*result = '\0';

	if ((unsigned long) bitvector < (unsigned long) INT_ONE);
	else if ((unsigned long) bitvector < (unsigned long) INT_TWO)
		fail = 1;
	else if ((unsigned long) bitvector < (unsigned long) INT_THREE)
		fail = 2;
	else
		fail = 3;
	bitvector &= 0x3FFFFFFF;
	while (fail) {
		if (*names[nr] == '\n')
			fail--;
		nr++;
	}


	for (; bitvector; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			if (*names[nr] != '\n') {
				strcat(result, names[nr]);
				strcat(result, div);
				divider = TRUE;
			} else {
				strcat(result, "UNDEF");
				strcat(result, div);
				divider = TRUE;
			}
		}
		if (*names[nr] != '\n')
			nr++;
	}

	if (!*result) {
		strcat(result, empty_string);
		return FALSE;
	} else if (divider)
		*(result + strlen(result) - 2) = '\0';//стоит 2, отнимает позиции, что бы выравнение сделать.
	return TRUE;
}

void sprintbits(FLAG_DATA flags, const char *names[], char *result, const char *div)
{
 char buffer[MAX_STRING_LENGTH];
 int i;
 *result='\0';
 for(i=0; i < 4; i++)
 {if (sprintbitwd(flags.flags[i]|(i << 30),names, buffer, div))
  {if (strlen(result))
    strcat(result,div);
   strcat(result,buffer);
  }
 }
 if (!strlen(result))
  strcat(result,buffer);
}


const char *some_pads[3][19] =
{ {"дней","часов","лет","очков","минут","минут","монет","монет",
   "штук","штук","уровней","верст","верст","единиц","единиц","секунд","градусов", "месяцев", "славы"},
  {"день","час","год","очко","минута","минуту","монета","монету",
   "штука","штуку","уровень","верста","версту","единица","единицу","секунду","градус", "месяц", "слава"},
  {"дня","часа","года","очка","минуты","минуты","монеты","монеты",
   "штуки","штуки","уровня","версты","версты","единицы","единицы","секунды","градуса", "месяца", "славы"}
};

const char * desc_count (int how_many, int of_what)
{if (how_many < 0)
    how_many = -how_many;
 if ( (how_many % 100 >= 11 && how_many % 100 <= 14) ||
       how_many % 10 >= 5 ||
       how_many % 10 == 0)
    return some_pads[0][of_what];
 if ( how_many % 10 == 1)
    return some_pads[1][of_what];
 else
    return some_pads[2][of_what];
}

int  same_group(struct char_data *ch, struct char_data *tch)
{if (!ch || !tch)
    return (FALSE);

 if (IS_NPC(ch)  && ch->master   && !IS_NPC(ch->master))
    ch  = ch->master;
 if (IS_NPC(tch) && tch->master  && !IS_NPC(tch->master))
    tch = tch->master;

 // NPC's always in same group
 if ((IS_NPC(ch) && IS_NPC(tch)) || ch == tch)
    return (TRUE);

 if (!AFF_FLAGGED(ch, AFF_GROUP) ||
     !AFF_FLAGGED(tch, AFF_GROUP))
    return (FALSE);

 if (ch->master == tch ||
     tch->master == ch ||
     (ch->master && ch->master == tch->master)
    )
    return (TRUE);

 return (FALSE);
}
/*char *only_title(struct char_data *ch)
{static char title[MAX_STRING_LENGTH], *hname, *hrank;
 static char clan[MAX_STRING_LENGTH];
 char   *pos;

 if ((hname = House_name(ch)) &&
     (hrank = House_rank(ch)) &&
     GET_HOUSE_RANK(ch)
    )
    {sprintf (clan, " (%s %s)",hrank,House_sname(ch));
    }
 else
    {clan[0] = 0;
    }

 if (!GET_TITLE(ch) || !*GET_TITLE(ch))
    {sprintf(title,"%s%s",GET_NAME(ch),clan);
    }
 else
 if ((pos = strchr(GET_TITLE(ch),'*')))
    {*(pos++) = '\0';
     sprintf(title,"%s %s%s%s%s",pos, GET_NAME(ch),
             *GET_TITLE(ch) ? " " : " ", GET_TITLE(ch),clan);
     *(--pos) = '*';
    }
 else
    sprintf(title,"%s%s%s",GET_NAME(ch),GET_TITLE(ch),clan);
 return title;
}
 */

char *race_or_title(struct char_data *ch)
{static char title[MAX_STRING_LENGTH];
 static char race[30];

 if (IS_IMMORTAL(ch))
     sprintf (title,"%s%s%s%s", (!GET_PTITLE(ch) || !*GET_PTITLE(ch)) ? "" : GET_PTITLE(ch), GET_NAME(ch),
	                          (!GET_TITLE(ch)  || !*GET_TITLE(ch))  ? "": GET_TITLE(ch), "");
 else
 if (AFF_FLAGGED(ch, AFF_PRISMATICAURA))
	 sprintf (title,"%s %s",
	pc_race_types[RACE_VAMPIRE][(int) GET_SEX(ch)], GET_NAME(ch));
 else
 { sprintf(race, "%s ",pc_race_types[(int) GET_RACE(ch)][(int) GET_SEX(ch)]);
	 sprintf (title,"%s%s%s",
	 GET_PTITLE(ch) ?  GET_PTITLE(ch) : ((!GET_TITLE(ch) || !*GET_TITLE(ch))  ?
	 race : ""), GET_NAME(ch),
	(!GET_TITLE(ch)  || !*GET_TITLE(ch)) ? "": GET_TITLE(ch));     
 }
 return title;
}
 


void ustalost_timer(struct char_data *ch, int mod)
{int t = 0;
   
   if (!mod || IS_NPC(ch))
	   return;
   
   if (mod < 0 && GET_UTIMER(ch) > 0 && GET_USTALOST(ch) > 0)
   {   t = GET_UTIMER(ch) + mod;
	   if (t > 0)
	       GET_UTIMER(ch) -= 1;
	   else
		{   GET_USTALOST(ch) -= 1;
			t = ((GET_USTALOST(ch)*GET_USTALOST(ch) - (GET_USTALOST(ch) - 1)*(GET_USTALOST(ch) - 1))/10);
			t = MAX(t,1);
			GET_UTIMER(ch) = t;
		}		   
		return;
   }

   if (mod > 0)
   {  GET_USTALOST(ch) += mod;
	  t = ((GET_USTALOST(ch)*GET_USTALOST(ch) - (GET_USTALOST(ch) - 1)*(GET_USTALOST(ch) - 1))/10);
	  t = MAX(t,1);
	  GET_UTIMER(ch) = t;
   }
	 	 
if (GET_USTALOST(ch) < 0)
	GET_USTALOST(ch) = 0;
if (GET_UTIMER(ch) < 0)
	GET_UTIMER(ch) = 0;
}

//Проверка на корректность вводимой почты при создании персонажа
//или же изменения почты иморталом, имеющим соответствующие привелегии.
int valid_email(const char *address)
{
        int i, size, count = 0;
	static string special_symbols("\r\n ()<>,;:\\\"[]|/&'`$");
        string addr = address;
	string::size_type dog_pos = 0, pos = 0;

        /* Наличие запрещенных символов или кириллицы */
	if (addr.find_first_of(special_symbols) != string::npos)
		return 0;
        size = 	addr.size();
	for (i = 0; i < size; i++)
		if (addr[i] <= ' ' || addr[i] >= 127)
		    return 0;
	/* Собака должна быть только одна и на второй и далее позиции */
	while ((pos = addr.find_first_of('@', pos)) != string::npos) {
	    dog_pos = pos;
	    ++count;
	    ++pos;
	}
	if (count != 1 || dog_pos == 0)
	    return 0;
	/* Проверяем правильность синтаксиса домена */
	/* В доменной части должно быть как минимум 4 символа, считая собаку */
	if (size - dog_pos <= 3)
	    return 0;
	/* Точка отсутствует, расположена сразу после собаки, или на последнем месте */
	if (addr[dog_pos + 1] == '.' || addr[size - 1] == '.' || addr.find('.', dog_pos) == string::npos)
	    return 0;

        return 1;
}	    


char *rustime(const struct tm *timeptr)
{
	static char mon_name[12][10] = {
		"Января\0", "Февраля\0", "Марта\0", "Апреля\0", "Мая\0", "Июня\0",
		"Июля\0", "Августа\0", "Сентября\0", "Октября\0", "Ноября\0", "Декабря\0"
	};
	static char result[100];

	sprintf(result, "%.2d:%.2d:%.2d %2d %s %d года",
		timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec, timeptr->tm_mday, mon_name[timeptr->tm_mon], 1900 + timeptr->tm_year);
	return result;
}

//перенести в линух доработать, получение аппов для использования в функции ОЧКИ do_score
const char *tiny_affect_loc_name(int location)
{
	switch (location) {
	case APPLY_NONE:		return "NIL";
	case APPLY_STR:			return " STR  ";
	case APPLY_DEX:			return " DEX  ";
	case APPLY_INT:			return " INT  ";
	case APPLY_WIS:			return " WIS  ";
	case APPLY_CON:			return " CON  ";
	case APPLY_CHA:			return " CHA  ";
//	case APPLY_LCK:			return " LCK  ";
//	case APPLY_SEX:			return " SEX  ";
	case APPLY_CLASS:		return " CLASS";
	case APPLY_LEVEL:		return " LVL  ";
	case APPLY_AGE:			return " AGE  ";
	case APPLY_MANA:		return " MANA ";
	case APPLY_HIT:			return " HV   ";
	case APPLY_MOVE:		return " MOVE ";
	case APPLY_GOLD:		return " GOLD ";
	case APPLY_EXP:			return " EXP  ";
	case APPLY_AC:			return " AC   ";
	case APPLY_HITROLL:		return " HITRL";
	case APPLY_DAMROLL:		return " DAMRL";
//	case APPLY_SAVING_POISON:	return "SV POI";
	case APPLY_SAVING_ROD:		return "SV ROD";
	case APPLY_SAVING_PARA:		return "SV PARA";
	case APPLY_SAVING_BREATH:	return "SV BRTH";
	case APPLY_SAVING_SPELL:	return "SV SPLL";
/*	case APPLY_HEIGHT:		return "HEIGHT";
	case APPLY_WEIGHT:		return "WEIGHT";
	case APPLY_AFFECT:		return "AFF BY";
	case APPLY_RESISTANT:		return "RESIST";
	case APPLY_IMMUNE:		return "IMMUNE";
	case APPLY_SUSCEPTIBLE:		return "SUSCEPT";
	case APPLY_WEAPONSPELL:		return " WEAPON";
	case APPLY_BACKSTAB:		return "BACKSTB";
	case APPLY_PICK:		return " PICK  ";
	case APPLY_TRACK:		return " TRACK ";
	case APPLY_STEAL:		return " STEAL ";
	case APPLY_SNEAK:		return " SNEAK ";
	case APPLY_HIDE:		return " HIDE  ";
	case APPLY_PALM:		return " PALM  ";
	case APPLY_DETRAP:		return " DETRAP";
	case APPLY_DODGE:		return " DODGE ";
	case APPLY_PEEK:		return " PEEK  ";
	case APPLY_SCAN:		return " SCAN  ";
	case APPLY_GOUGE:		return " GOUGE ";
	case APPLY_SEARCH:		return " SEARCH";
	case APPLY_MOUNT:		return " MOUNT ";
	case APPLY_DISARM:		return " DISARM";
	case APPLY_KICK:		return " KICK  ";
	case APPLY_PARRY:		return " PARRY ";
	case APPLY_BASH:		return " BASH  ";
	case APPLY_STUN:		return " STUN  ";
	case APPLY_PUNCH:		return " PUNCH ";
	case APPLY_CLIMB:		return " CLIMB ";
	case APPLY_GRIP:		return " GRIP  ";
	case APPLY_SCRIBE:		return " SCRIBE";
	case APPLY_BREW:		return " BREW  ";
	case APPLY_WEARSPELL:		return " WEAR  ";
	case APPLY_REMOVESPELL:		return " REMOVE";
	case APPLY_EMOTION:		return "EMOTION";
	case APPLY_MENTALSTATE:		return " MENTAL";
	case APPLY_STRIPSN:		return " DISPEL";
	case APPLY_REMOVE:		return " REMOVE";
	case APPLY_DIG:			return " DIG   ";
	case APPLY_FULL:		return " HUNGER";
	case APPLY_THIRST:		return " THIRST";
	case APPLY_DRUNK:		return " DRUNK ";
	case APPLY_BLOOD:		return " BLOOD ";*/
	}

//	bug("Affect_location_name: unknown location %d.", location);
	return "(?)";
}


