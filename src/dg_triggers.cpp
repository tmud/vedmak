/**************************************************************************
*  File: triggers.c                                                       *
*                                                                         *
*  Usage: contains all the trigger functions for scripts.                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: galion $
*  $Date: 1996/08/05 23:32:08 $
*  $Revision: 3.9 $
**************************************************************************/

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "olc.h"

extern struct index_data **trig_index;
extern struct room_data *world;


#ifndef LVL_BUILDER
#define LVL_BUILDER LVL_GOD
#endif


/* external functions from scripts.c */
//void add_var(struct trig_var_data **var_list, char *name, char *value, long id); триггера
extern int script_driver(void *go, trig_data *trig, int type, int mode);
char *matching_quote(char *p);
char *str_str(char *cs, char *ct);


static char *dirs[] = 
{
  "Север",
  "Восток",
  "Юг",
  "Запад",
  "Вверх",
  "Вниз",
  "\n"
};


/* mob trigger types */
const char *trig_types[] =
{ "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Damage",
  "Great PC",
  "Great-All PC",
  "Income",
  "Income PC",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "Auto",
  "\n"
};


/* obj trigger types */
const char *otrig_types[] =
{ "Global",
  "Random",
  "Command",
  "UNUSED",
  "UNUSED",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED",
  "Remove",
  "UNUSED",
  "Load",
  "Unlock",
  "Open",
  "Lock",
  "Close",
  "Взломать",
  "Вход PC",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "Auto",
  "\n"
};


/* wld trigger types */
const char *wtrig_types[] =
{ "Global",
  "Random",
  "Command",
  "Speech",
  "Enter PC",
  "Zone Reset",
  "Enter",
  "Drop",
  "Unlock",
  "Open",
  "Lock",
  "Close",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "Auto",
  "\n"
};


/*
 *  General functions used by several triggers
 */


/*
 * Copy first phrase into first_arg, returns rest of string
 */
char *one_phrase(char *arg, char *first_arg)
{   skip_spaces(&arg);

    if (!*arg)
	   *first_arg = '\0';
    else
    if (*arg == '"')
       {char *p, c;
	    p = matching_quote(arg);
	    c = *p;
	    *p = '\0';
	    strcpy(first_arg, arg + 1);
	    if (c == '\0')
	       return p;
	    else
	       return p + 1;
       }
    else
       {char *s, *p;

	    s = first_arg;
	    p = arg;
	
	    while (*p && !is_space(*p) && *p != '"')
	          *s++ = *p++;

	    *s = '\0';
	    return p;
       }

    return arg;
}


int is_substring(char *sub, char *string)
{
   // char *s;
    int sublen = strlen(sub);
  /*  if ((s = str_str(string, sub)))
       {int len    = strlen(string);
	
	
	    /* check front */
/*	    if ((s == string || !is_space(*(s - 1)) || ispunct(*(s - 1))) &&
	    /* check end */
	  /*      ((s + sublen == string + len) || !is_space(s[sublen]) ||
	          ispunct(s[sublen])))
	       return 1;
       }*/

	if (isname(sub, string) && strlen(sub) > 1)
		return 1;

    return 0;
}


/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
int word_check(char *str, char *wordlist)
{
    char words[MAX_INPUT_LENGTH], phrase[MAX_INPUT_LENGTH], *s;

    if (*wordlist=='*') return 1;

    strcpy(words, wordlist);

    for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase))
	    if (is_substring(phrase, str))
	       return 1;

    return 0;
}

	

/*
 *  mob triggers
 */

void random_mtrigger(char_data *ch)
{
  trig_data *t;

  if (!SCRIPT_CHECK(ch, MTRIG_RANDOM) || AFF_FLAGGED(ch, AFF_CHARM))
     return;

  for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      {if (TRIGGER_CHECK(t, MTRIG_RANDOM) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
           break;
          }
      }
}

void bribe_mtrigger(char_data *ch, char_data *actor, int amount)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(ch, MTRIG_BRIBE) || AFF_FLAGGED(ch, AFF_CHARM))
     return;

  for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      {if (TRIGGER_CHECK(t, MTRIG_BRIBE) && (amount >= GET_TRIG_NARG(t)))
          {sprintf(buf, "%d", amount);
           add_var_cntx(&GET_TRIG_VARS(t), "amount", buf, 0);//триггера
           ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
           break;
          }
      }
}

struct char_node {
       struct char_data *ch;
       struct char_node *next;
};
// Вызывается перед заходом персонажа в комнату (legal_dir)
int entry_mtrigger(char_data *ch, room_data *entry_room, int entry_dir) {
    char buf[MAX_INPUT_LENGTH];
    trig_data *t;

    if (!SCRIPT_CHECK(ch, MTRIG_ENTRY) || AFF_FLAGGED(ch, AFF_CHARM))
       return 1;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_ENTRY) &&
            (number(1, 100) <= GET_TRIG_NARG(t))) {
           if (entry_dir >= 0)
           add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[entry_dir], 0);//триггера
           ADD_UID_ROOM_VAR(buf, t, entry_room, "room", 0);
           return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
        }
    }
    return 1;
}
// Вызывается когда персонаж вошел в комнату
int entry_memory_mtrigger(char_data *ch, room_data *enter_from, int enter_dir) {
    trig_data *t;
    char_data *actor;
    struct script_memory *mem;
    char buf[MAX_INPUT_LENGTH];
    int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };

    struct char_node *char_top = NULL, *char_cur = NULL, *char_end = NULL;
    int inroom = IN_ROOM(ch);

    if (!SCRIPT_MEM(ch) || AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch,AFF_HORSE))
       return 1;
   
  
    // составим список всех в комнате
    for (actor = world[IN_ROOM(ch)].people; actor; actor = actor->next_in_room) {
        CREATE(char_cur,struct char_node,1);
        char_cur->ch = ch;
        char_cur->next = NULL;
        if (!char_top) {
           char_top = char_cur;
           char_end = char_cur;
        }
        else {
           char_end->next = char_cur;
           char_end = char_cur;
        }
    }

    for (char_cur = char_top; char_cur; char_cur = char_top) {
        char_top = char_cur->next;
        actor = char_cur->ch;
        free(char_cur);
        
        if (actor==ch || IN_ROOM(actor) != IN_ROOM(ch) || IN_ROOM(ch) != inroom)
           continue;
        for (mem = SCRIPT_MEM(ch); mem; mem = mem->next) {
            if (GET_ID(actor)==mem->id) {
               struct script_memory *prev;
               if (mem->cmd)
                  command_interpreter(ch, mem->cmd);
               else {
                  for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
                      if (TRIGGER_CHECK(t, MTRIG_MEMORY) &&
                          number(1, 100) <= GET_TRIG_NARG(t)) {
                         ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
						 if (enter_from)
                      ADD_UID_ROOM_VAR(buf, t, enter_from, "room", 0);
                         if (enter_dir >= 0)//entry_dir -  ошибка нет такого здесь
                           add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[enter_dir]], 0);
                         script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
                         break;
                     }
                  }
               }
               /* delete the memory */
               if (SCRIPT_MEM(ch)==mem) {
                  SCRIPT_MEM(ch) = mem->next;
               }
               else {
                  prev = SCRIPT_MEM(ch);
                  while (prev->next != mem)
                        prev = prev->next;
                  prev->next = mem->next;
               }
               if (mem->cmd)
                  free(mem->cmd);
               free(mem);
            }
        }
    }
    return inroom == IN_ROOM(ch);
}
// Вызывается когда actor уже вошел в комнату
int greet_mtrigger(char_data *actor, room_data *enter_from, int enter_dir) { 
    trig_data *t;
    char_data *ch;
    char buf[MAX_INPUT_LENGTH];
    int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
    int intermediate, final=TRUE;

   

	for (ch = world[IN_ROOM(actor)].people; ch; ch = ch->next_in_room) {
        if (!SCRIPT_CHECK(ch, MTRIG_GREET | MTRIG_GREET_ALL | MTRIG_GREET_PC | MTRIG_GREET_PC_ALL) ||
            !AWAKE(ch)    ||
            (ch == actor) ||
            AFF_FLAGGED(ch, AFF_CHARM) ||
            AFF_FLAGGED(ch, AFF_HORSE))
          continue;
        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (((IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET) && CAN_SEE(ch, actor)) ||
                  IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_ALL) ||
                  (IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_PC) && !IS_NPC(actor) && CAN_SEE(ch,actor)) ||
                  (IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_PC_ALL) && !IS_NPC(actor))) &&
                !GET_TRIG_DEPTH(t) && 
                number(1, 100) <= GET_TRIG_NARG(t)) {
                if (enter_dir>=0)
                   add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[enter_dir]], 0);
                ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				if(enter_from)
                ADD_UID_ROOM_VAR(buf, t, enter_from, "room", 0);
                intermediate =  script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
                if (!intermediate)
                   final = FALSE;
                continue;
            }
        }
  }
  return final;
}
// Вызывается когда actor уже вошел в комнату
int greet_memory_mtrigger(char_data *actor, room_data *enter_from, int enter_dir) { 
    trig_data *t;
    char_data *ch;
    struct script_memory *mem;
    struct char_node *char_top = NULL, *char_cur = NULL, *char_end = NULL;
    int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
    int command_performed;//ошибка не инициализирована была;
    char buf[MAX_INPUT_LENGTH];
    int inroom = IN_ROOM(actor);
  
	if (IN_ROOM(actor)==NOWHERE)
         return 0;
    // составим список всех в комнате
    for (ch = world[IN_ROOM(actor)].people; ch; ch = ch->next_in_room) {
        if (!SCRIPT_MEM(ch) || 
            !AWAKE(ch)      || 
            (ch == actor)   ||
            AFF_FLAGGED(ch, AFF_CHARM) ||
            AFF_FLAGGED(ch, AFF_HORSE))
           continue;
        CREATE(char_cur,struct char_node,1);
        char_cur->ch = ch;
        char_cur->next = NULL;
        if (!char_top) {
           char_top = char_cur;
           char_end = char_cur;
        }
        else {
           char_end->next = char_cur;
           char_end = char_cur;
        }
    }
    
    // бум работать с ЭТИМ списком
    for (char_cur = char_top; char_cur; char_cur = char_top) {
        command_performed = 0;
        char_top = char_cur->next;
        ch = char_cur->ch;
       
        // очистим уже отработанный узел
        free(char_cur);
        // actor не находится на месте - отменим обработку
        if (IN_ROOM(actor) != inroom || IN_ROOM(ch) != IN_ROOM(actor))
           continue;
        for (mem = SCRIPT_MEM(ch); mem && SCRIPT_MEM(ch); mem=mem->next) {
            if (GET_ID(actor)!=mem->id)
               continue;
            if (mem->cmd) {
               command_interpreter(ch, mem->cmd);
               command_performed = 1;
            }
            break;
        }
        if (mem && !command_performed) {
           for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
               if (IS_SET(GET_TRIG_TYPE(t), MTRIG_MEMORY) &&
                   CAN_SEE(ch, actor) &&
                   !GET_TRIG_DEPTH(t) &&
                   number(1, 100) <= GET_TRIG_NARG(t)) {
                   if (enter_dir>=0)
                   add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[enter_dir]], 0);
                   ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
				   if (enter_from)
                   ADD_UID_ROOM_VAR(buf, t, enter_from, "room", 0);
                   script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
				                    break;
               }
            }
         }
         if (mem) {
             if (SCRIPT_MEM(ch)==mem) {
                SCRIPT_MEM(ch) = mem->next;
             }
             else {
                struct script_memory *prev;
                prev = SCRIPT_MEM(ch);
                while (prev->next != mem)
                      prev = prev->next;
                prev->next = mem->next;
             }
             if (mem->cmd)
                free(mem->cmd);
             free(mem);
        }
    }
    return inroom == IN_ROOM(actor);
}
int income_mtrigger(char_data *ch, room_data *enter_from, int enter_dir) { 
    trig_data *t;
    int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };

    if (!SCRIPT_CHECK(ch, MTRIG_INCOME) || AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HORSE))
       return 1;
    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_INCOME) && (number(1, 100) <= GET_TRIG_NARG(t))) {
           if (enter_dir>=0)// dir - ошибка
           add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[enter_dir]], 0);
           ADD_UID_ROOM_VAR(buf, t, enter_from, "room", 0);
           return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
        }
    }
    return 1;
}

int compare_cmd(int mode, char *source, char *dest)
{ int result = FALSE;
  if (!source || !*source || !dest || !*dest)
     return (FALSE);
  if (*source == '*')
     return (TRUE);

  switch (mode)
  {case 0: result = word_check(dest, source);

           break;
   case 1: result = is_substring(dest, source);

           break;
   default:if (!str_cmp(source, dest))
              return (TRUE);
   }
   return (result);
}


int command_mtrigger(char_data *actor, char *cmd, char *argument)
{
  char_data *ch, *ch_next;
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  for (ch = world[IN_ROOM(actor)].people; ch; ch = ch_next)
      {ch_next = ch->next_in_room;

       if (SCRIPT_CHECK(ch, MTRIG_COMMAND) && !AFF_FLAGGED(ch, AFF_CHARM) &&
           (actor!=ch))
          {for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
               {if (!TRIGGER_CHECK(t, MTRIG_COMMAND))
                   continue;

                if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
                   {sprintf(buf,"SYSERR: Command Trigger #%d has no text argument!",
                                GET_TRIG_VNUM(t));
                    mudlog(buf, NRM, LVL_BUILDER, TRUE);
                    continue;
                   }
                if (compare_cmd(GET_TRIG_NARG(t),GET_TRIG_ARG(t),cmd))
                   {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
	                skip_spaces(&argument);
	                add_var_cntx(&GET_TRIG_VARS(t), "arg", argument, 0);
	                skip_spaces(&cmd);
	                add_var_cntx(&GET_TRIG_VARS(t), "cmd", cmd, 0);
	
	                if (script_driver(ch, t, MOB_TRIGGER, TRIG_NEW))
	                   return 1;
	               }
               }
          }
      }

  return 0;
}


void speech_mtrigger(char_data *actor, char *str)
{
  char_data *ch, *ch_next;
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  for (ch = world[IN_ROOM(actor)].people; ch; ch = ch_next)
      {ch_next = ch->next_in_room;

       if (SCRIPT_CHECK(ch, MTRIG_SPEECH) && AWAKE(ch) &&
           !AFF_FLAGGED(ch, AFF_CHARM) && (actor!=ch))
          for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
              {if (!TRIGGER_CHECK(t, MTRIG_SPEECH))
                  continue;

               if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
                  {sprintf(buf,"SYSERR: Speech Trigger #%d has no text argument!",
                   GET_TRIG_VNUM(t));
                   mudlog(buf, NRM, LVL_BUILDER, TRUE);
                   continue;
                  }
               
               if (compare_cmd(GET_TRIG_NARG(t), GET_TRIG_ARG(t), str))
                  { ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
                    add_var_cntx(&GET_TRIG_VARS(t), "speech", str, 0);
                    script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
                   break;
                  }
              }
      }
}


void act_mtrigger(char_data *ch, char *str, char_data *actor,
		  char_data *victim, obj_data *object,
		  obj_data *target, char *arg)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (SCRIPT_CHECK(ch, MTRIG_ACT) && !AFF_FLAGGED(ch, AFF_CHARM) &&
      (actor!=ch))
     for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
         {if (!TRIGGER_CHECK(t, MTRIG_ACT))
             continue;

          if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
             {sprintf(buf,"SYSERR: Act Trigger #%d has no text argument!",
                      GET_TRIG_VNUM(t));
              mudlog(buf, NRM, LVL_BUILDER, TRUE);
              continue;
             }

          if (compare_cmd(GET_TRIG_NARG(t),GET_TRIG_ARG(t),str)
             /* ((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
                (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str))) */ )
             {if (actor)
	             ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
	          if (victim)
	             ADD_UID_CHAR_VAR(buf, t, victim, "victim", 0);
	          if (object)
	             ADD_UID_OBJ_VAR(buf, t, object, "object", 0);
	          if (target)
	             ADD_UID_CHAR_VAR(buf, t, target, "target", 0);
	          if (arg)
	             {skip_spaces(&arg);
	              add_var_cntx(&GET_TRIG_VARS(t), "arg", arg, 0);
	             }	
	          script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
	          break;
             }	
         }
}


void fight_mtrigger(char_data *ch)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(ch, MTRIG_FIGHT) || !FIGHTING(ch) ||
      AFF_FLAGGED(ch, AFF_CHARM))
     return;

  for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      {if (TRIGGER_CHECK(t, MTRIG_FIGHT) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {ADD_UID_CHAR_VAR(buf, t, FIGHTING(ch), "actor", 0)
           script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
           break;
          }
      }
}

int damage_mtrigger(char_data *damager, char_data *victim)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(victim, MTRIG_DAMAGE) ||
      AFF_FLAGGED(victim, AFF_CHARM))
     return 0;

  for (t = TRIGGERS(SCRIPT(victim)); t; t = t->next)
      {if (TRIGGER_CHECK(t, MTRIG_DAMAGE) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {ADD_UID_CHAR_VAR(buf, t, damager, "damager", 0)
           return script_driver(victim, t, MOB_TRIGGER, TRIG_NEW);
          }
      }
  return 0;
}


void hitprcnt_mtrigger(char_data *ch)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(ch, MTRIG_HITPRCNT) || !FIGHTING(ch) ||
      AFF_FLAGGED(ch, AFF_CHARM))
    return;

  for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      {if (TRIGGER_CHECK(t, MTRIG_HITPRCNT) && GET_MAX_HIT(ch) &&
	       (((GET_HIT(ch) * 100) / GET_MAX_HIT(ch)) <= GET_TRIG_NARG(t)))
	      {ADD_UID_CHAR_VAR(buf, t, FIGHTING(ch), "actor", 0)
           script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
           break;
          }
      }
}


int receive_mtrigger(char_data *ch, char_data *actor, obj_data *obj)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(ch, MTRIG_RECEIVE) || AFF_FLAGGED(ch, AFF_CHARM))
     return 1;

  for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      {if (TRIGGER_CHECK(t, MTRIG_RECEIVE) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
               ADD_UID_OBJ_VAR(buf, t, obj, "object", 0);
               return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
              }
      }

  return 1;
}


int death_mtrigger(char_data *ch, char_data *actor)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(ch, MTRIG_DEATH) || AFF_FLAGGED(ch, AFF_CHARM))
     return 1;

  for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      {if (TRIGGER_CHECK(t, MTRIG_DEATH))
          {if (actor)
	       ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}

void load_mtrigger(char_data *ch)
{
  trig_data *t;

  if (!SCRIPT_CHECK(ch, MTRIG_LOAD))
    return;

  for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      {if (TRIGGER_CHECK(t, MTRIG_LOAD) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {script_driver(ch, t, MOB_TRIGGER, TRIG_NEW);
           break;
          }
      }
}


/*
 *  object triggers
 */

void random_otrigger(obj_data *obj)
{
  trig_data *t;

  if (!SCRIPT_CHECK(obj, OTRIG_RANDOM))
    return;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_RANDOM) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
           break;
          }
      }
}


void timer_otrigger(struct obj_data *obj)
{
  trig_data *t;

  if (!SCRIPT_CHECK(obj, OTRIG_TIMER))
    return;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_TIMER))
          script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
      }

  return;
}


int get_otrigger(obj_data *obj, char_data *actor)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(obj, OTRIG_GET))
     return 1;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_GET) && (number(1, 100) <= GET_TRIG_NARG(t)))
          {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}


/* checks for command trigger on specific object. assumes obj has cmd trig */
int cmd_otrig(obj_data *obj, char_data *actor, char *cmd,
	      char *argument, int type)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND))
     for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
         {if (!TRIGGER_CHECK(t, OTRIG_COMMAND))
             continue;

          if (IS_SET(GET_TRIG_NARG(t), type) &&
              (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)))
             {sprintf(buf,"SYSERR: O-Command Trigger #%d has no text argument!",
                          GET_TRIG_VNUM(t));
              mudlog(buf, NRM, LVL_BUILDER, TRUE);
              continue;
             }
       //изменить - типа сделать что бы по части слова как и мобы и комнаты срабатывало!!!
          if (IS_SET(GET_TRIG_NARG(t), type) &&
              (*GET_TRIG_ARG(t)=='*' ||
               !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(GET_TRIG_ARG(t))) ))
             {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
	          skip_spaces(&argument);
	          add_var_cntx(&GET_TRIG_VARS(t), "arg", argument, 0);
	          skip_spaces(&cmd);
	          add_var_cntx(&GET_TRIG_VARS(t), "cmd", cmd, 0);
	
	          if (script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW))
	             return 1;
             }
         }

  return 0;
}


int command_otrigger(char_data *actor, char *cmd, char *argument)
{
  obj_data *obj;
  int i;

  for (i = 0; i < NUM_WEARS; i++)
      if (cmd_otrig(GET_EQ(actor, i), actor, cmd, argument, OCMD_EQUIP))
         return 1;

  for (obj = actor->carrying; obj; obj = obj->next_content)
      if (cmd_otrig(obj, actor, cmd, argument, OCMD_INVEN))
         return 1;

  for (obj = world[IN_ROOM(actor)].contents; obj; obj = obj->next_content)
      if (cmd_otrig(obj, actor, cmd, argument, OCMD_ROOM))
         return 1;

  return 0;
}


int wear_otrigger(obj_data *obj, char_data *actor, int where)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(obj, OTRIG_WEAR))
     return 1;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_WEAR))
          {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}


int remove_otrigger(obj_data *obj, char_data *actor)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(obj, OTRIG_REMOVE))
     return 1;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_REMOVE))
          {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}


int drop_otrigger(obj_data *obj, char_data *actor)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(obj, OTRIG_DROP))
    return 1;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_DROP) && (number(1, 100) <= GET_TRIG_NARG(t)))
          {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}


int give_otrigger(obj_data *obj, char_data *actor, char_data *victim)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(obj, OTRIG_GIVE))
     return 1;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_GIVE) && (number(1, 100) <= GET_TRIG_NARG(t)))
          {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           ADD_UID_CHAR_VAR(buf, t, victim, "victim", 0);
           return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}

void load_otrigger(obj_data *obj)
{
  trig_data *t;

  if (!SCRIPT_CHECK(obj, OTRIG_LOAD))
     return;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_LOAD) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
           break;
          }
      }
}

int pick_otrigger(obj_data *obj, char_data *actor)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(obj, OTRIG_PICK))
    return 1;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, OTRIG_PICK) && (number(1, 100) <= GET_TRIG_NARG(t)))
          {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}



int open_otrigger(obj_data *obj, char_data *actor, bool unlock)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];
  int  open_mode = unlock ? OTRIG_UNLOCK : OTRIG_OPEN;

  if (!SCRIPT_CHECK(obj, open_mode))
    return 1;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, open_mode) && (number(1, 100) <= GET_TRIG_NARG(t)))
          {add_var_cntx(&GET_TRIG_VARS(t), "mode", unlock ? (char* )"1" : (char* )"0", 0);
           ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}

int close_otrigger(obj_data *obj, char_data *actor, bool lock)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];
  int  close_mode = lock ? OTRIG_LOCK : OTRIG_CLOSE;

  if (!SCRIPT_CHECK(obj, close_mode))
    return 1;

  for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
      {if (TRIGGER_CHECK(t, close_mode) && (number(1, 100) <= GET_TRIG_NARG(t)))
          {add_var_cntx(&GET_TRIG_VARS(t), "mode", lock ? (char* )"1" : (char* )"0", 0);
           ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}


int greet_otrigger(char_data *actor, int dir)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];
  obj_data *obj;
  int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
  int intermediate, final=TRUE;

  if (IS_NPC(actor))
   return (TRUE);

  for (obj = world[IN_ROOM(actor)].contents; obj; obj = obj->next_content) {
    if (!SCRIPT_CHECK(obj, OTRIG_GREET_ALL_PC))
       continue;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
       if (TRIGGER_CHECK(t, OTRIG_GREET_ALL_PC)) {
           ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           if (dir>=0)
			 add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]], 0);
           intermediate = script_driver(obj, t, OBJ_TRIGGER, TRIG_NEW);
           if (!intermediate)
             final = FALSE;
           continue;
          }
      }
  }
  return final;
}



/*
 *  world triggers
 */

void reset_wtrigger(struct room_data *room)
{
  trig_data *t;

  if (!SCRIPT_CHECK(room, WTRIG_RESET))
     return;
 //log("Triggers rnum %d", room);
  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      {if (TRIGGER_CHECK(t, WTRIG_RESET) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
           break;
          }
      }
}

void random_wtrigger(struct room_data *room, int num,
                     void * s, int types, void * list, void * next)
{
  trig_data *t;

  if (!SCRIPT_CHECK(room, WTRIG_RANDOM))
    return;

  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      {if (TRIGGER_CHECK(t, WTRIG_RANDOM) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
           break;
          }
      }
}
int enter_wtrigger(struct room_data *room, char_data *actor, int dir)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];
  int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };

  if (!SCRIPT_CHECK(room, WTRIG_ENTER | WTRIG_ENTER_PC))
     return 1;

  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      {if ((TRIGGER_CHECK(t, WTRIG_ENTER) ||
            (TRIGGER_CHECK(t, WTRIG_ENTER_PC) && !IS_NPC(actor))) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {if (dir>=0)
                 add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]], 0);
               ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}


int command_wtrigger(char_data *actor, char *cmd, char *argument)
{
  struct room_data *room;
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!actor                    ||
      IN_ROOM(actor) == NOWHERE ||
      !SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_COMMAND)
     )
     return 0;

  room = &world[IN_ROOM(actor)];
  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      {if (!TRIGGER_CHECK(t, WTRIG_COMMAND))
          continue;

       if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
          {sprintf(buf,"SYSERR: W-Command Trigger #%d has no text argument!",
           GET_TRIG_VNUM(t));
           mudlog(buf, NRM, LVL_BUILDER, TRUE);
           continue;
          }

       if (compare_cmd(GET_TRIG_NARG(t),GET_TRIG_ARG(t),cmd)
           /* *GET_TRIG_ARG(t)=='*' ||
           !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(GET_TRIG_ARG(t)))*/)
          {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           skip_spaces(&argument);
           add_var_cntx(&GET_TRIG_VARS(t), "arg", argument, 0);
           skip_spaces(&cmd);
           add_var_cntx(&GET_TRIG_VARS(t), "cmd", cmd, 0);

           return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
          }
      }

  return 0;
}


void speech_wtrigger(char_data *actor, char *str)
{ struct room_data *room;
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!actor || !SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_SPEECH))
     return;

  room = &world[IN_ROOM(actor)];
  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      {if (!TRIGGER_CHECK(t, WTRIG_SPEECH))
          continue;

       if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))
          {sprintf(buf,"SYSERR: W-Speech Trigger #%d has no text argument!",
                       GET_TRIG_VNUM(t));
           mudlog(buf, NRM, LVL_BUILDER, TRUE);
           continue;
          }

       if (compare_cmd(GET_TRIG_NARG(t),GET_TRIG_ARG(t),str))
          {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           add_var_cntx(&GET_TRIG_VARS(t), "speech", str, 0);
           script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
           break;
          }
      }
}

int drop_wtrigger(obj_data *obj, char_data *actor)
{
  struct room_data *room;
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!actor || !SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_DROP))
     return 1;

  room = &world[IN_ROOM(actor)];
  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      if (TRIGGER_CHECK(t, WTRIG_DROP) &&
	      (number(1, 100) <= GET_TRIG_NARG(t)))
	     {ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
              ADD_UID_OBJ_VAR(buf, t, obj, "object", 0);
          return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
          break;
         }	

  return 1;
}

int pick_wtrigger(struct room_data *room, char_data *actor, int dir)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!SCRIPT_CHECK(room, WTRIG_PICK))
     return 1;

  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      {if (TRIGGER_CHECK(t, WTRIG_PICK) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[dir], 0);
           ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}



int open_wtrigger(struct room_data *room, char_data *actor, int dir, bool unlock)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];
  int open_mode = unlock ? WTRIG_UNLOCK : WTRIG_OPEN;

  if (!SCRIPT_CHECK(room, open_mode))
     return 1;

  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      {if (TRIGGER_CHECK(t, open_mode) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {add_var_cntx(&GET_TRIG_VARS(t), "mode", unlock ? (char* )"1" : (char* )"0", 0);
	       add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[dir], 0);
           ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}

int close_wtrigger(struct room_data *room, char_data *actor, int dir, bool lock)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];
  int close_mode = lock ? WTRIG_LOCK : WTRIG_CLOSE;

  if (!SCRIPT_CHECK(room, close_mode))
     return 1;

  for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
      {if (TRIGGER_CHECK(t, close_mode) &&
	       (number(1, 100) <= GET_TRIG_NARG(t)))
	      {add_var_cntx(&GET_TRIG_VARS(t), "mode", lock ? (char* )"1" : (char* )"0", 0);
	       add_var_cntx(&GET_TRIG_VARS(t), "direction", dirs[dir], 0);
           ADD_UID_CHAR_VAR(buf, t, actor, "actor", 0);
           return script_driver(room, t, WLD_TRIGGER, TRIG_NEW);
          }
      }

  return 1;
}
