/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "screen.h"
#include "pk.h"

extern room_rnum r_mortal_start_room;
extern struct room_data *world;
extern struct obj_data *object_list, *obj_proto;
extern struct char_data *character_list;
extern struct index_data *obj_index;
//extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
extern char cast_argument[MAX_INPUT_LENGTH];
extern int mini_mud;
extern int pk_allowed;
extern int num_of_houses;
extern const  char *material_name[];
extern const  char *weapon_affects[];
extern struct time_info_data time_info;
int  what_sky = SKY_CLOUDLESS;


void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
void name_to_drinkcon(struct obj_data * obj, int type);
void name_from_drinkcon(struct obj_data * obj);
void go_create_weapon(struct char_data *ch, struct obj_data *obj, int obj_type, int skill);
//void go_create_weapon(struct char_data *ch, struct obj_data *obj, int obj_type);
char *diag_weapon_to_char(struct obj_data *obj, int show_wear);
int  compute_armor_class(struct char_data *ch);
int  calc_loadroom(struct char_data *ch);

/*
 * Special spells appear below.
 */

#define SUMMON_FAIL "Вы потерпели неудачу.\r\n"
#define SUMMON_FAIL2 "Ваша жертва сопротивляется.\r\n"

ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("Вы наполнили $3 водой.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}


#define MIN_NEWBIE_ZONE  9
#define MAX_NEWBIE_ZONE  10
#define MAX_SUMMON_TRIES 2000

ASPELL(spell_recall)
{ room_rnum to_room = NOWHERE, fnd_room = NOWHERE;
  int       modi = 0;
  struct follow_type *k;


  if (!victim || IS_NPC(victim) || IN_ROOM(ch) != IN_ROOM(victim))
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_GOD(ch) &&
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT)
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }


   if (victim != ch) 
     {if (WAITLESS(ch) && !WAITLESS(victim))
         modi = 0;  // always success
      else
      if (same_group(ch,victim))
         modi = 25;   // 75% chance to success
      else
      if (!IS_NPC(ch) || (ch->master && !IS_NPC(ch->master)))
         modi  = 100; // always fail

      if (number(0,99) < modi)
         {send_to_char(SUMMON_FAIL,ch);
          return;
         }
     }
   

  if ((to_room = real_room(GET_LOADROOM(victim))) == NOWHERE)
     to_room = real_room(calc_loadroom(victim));

  if (to_room == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  for (modi = 0; modi < MAX_SUMMON_TRIES; modi++)
      {fnd_room = number(0,top_of_world);
       if ( world[to_room].zone == world[fnd_room].zone              &&
            SECT(fnd_room)      != SECT_SECRET                       &&
            !ROOM_FLAGGED(fnd_room, ROOM_DEATH      | ROOM_TUNNEL)   &&
            !ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORT)                 &&
	    !ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)                  &&
	    (!ROOM_FLAGGED(fnd_room, ROOM_GODROOM) || IS_IMMORTAL(victim)) &&
	    (!ROOM_FLAGGED(fnd_room, ROOM_PRIVATE))
           )
	   break;
      }

  if (modi >= MAX_SUMMON_TRIES)
     {send_to_char(SUMMON_FAIL,ch);
      return;
     }

   if (FIGHTING(victim) && (victim != ch))
    pk_agro_action(ch,FIGHTING(victim));

//Изменения Карда
  /* Тут вставка, если недавно убивал, и реколишься в зону любого замка,
  то тебя перенесет в комнату atrium */
/*  if (RENTABLE(victim))
     {for (j = 0; j < num_of_houses; j++)
          {if (get_name_by_id(house_control[j].owner) == NULL)
              continue;
          if (world[fnd_room].zone == world[real_room(house_control[j].vnum[0])].zone)
             fnd_room = real_room(house_control[j].atrium);
         }
     }  */
//
  act("$n исчез$q.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, fnd_room);
  check_horse(victim);  
  act("$n появил$u в центре комнаты.", TRUE, victim, 0, 0, TO_ROOM);

   for (k =victim->followers; k; k = k->next)
     {if (AFF_FLAGGED(k->follower,AFF_CHARM) && k->follower->master == victim && IN_ROOM(k->follower) == IN_ROOM(victim))
         {
		  act("$n исчез$q.", TRUE, k->follower, 0, 0, TO_ROOM);
		  char_from_room(k->follower);
		  char_to_room(k->follower, fnd_room);
		  check_horse(k->follower);  
		  act("$n появил$u в центре комнаты.", TRUE, k->follower, 0, 0, TO_ROOM);
         }
     }

  
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim, NULL, -1);
  greet_mtrigger(victim, NULL, -1);
  greet_otrigger(victim,-1);
  greet_memory_mtrigger(victim, NULL, -1);
}

extern room_vnum mortal_start_room;
ASPELL(spell_group_recall)
{
   if (IS_NPC(ch))
        return;
   if (!IS_IMMORTAL(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORT))
   {
        send_to_char(SUMMON_FAIL, ch);
        return;
   }
   
   // Если указана цель, то телепортим группу цели
   char_data *caster = ch;
   if (victim != NULL)
       ch = victim;
   // берем самого главного ведущего
   while (ch->master)
       ch = ch->master;
   // если вдруг это npc - то выходим
   if (IS_NPC(ch))
   {
       send_to_char(SUMMON_FAIL, caster);
       return;
   }
   
   int chance = GET_LEVEL(caster) * 2 + GET_REAL_WIS(caster);
   if (number(0,99) > chance && !IS_IMMORTAL(caster))
   {
       send_to_char(SUMMON_FAIL,caster);
       return;
   }

   std::vector<char_data*> to_recall;
   to_recall.push_back(ch);   
   for (int i = 0;; ++i)
   {
       char_data *f = to_recall[i];
       for (follow_type *k =f->followers; k; k = k->next)
           to_recall.push_back(k->follower);
       int last = to_recall.size() - 1;
       if (i == last) break;
   }

   if (victim && FIGHTING(caster) && !IS_IMMORTAL(caster))
   {
       bool caster_in_group = false;
       for (int i=0,e=to_recall.size(); i<e; ++i)
       {
           if (to_recall[i] == caster) { caster_in_group = true; break; }
       }
       if (!caster_in_group)
       {
           send_to_char(SUMMON_FAIL, caster);
           return;
       }
   }

   room_rnum load_room = GET_LOADROOM(ch);   
   if (load_room == NOWHERE)
       load_room = mortal_start_room;

   //select random room in zone
   room_rnum room = NOWHERE;
   do 
   {
       int zone = load_room / 100;
       for (int i = 0; i<=99; ++i)
       {
           room_rnum rr =  real_room(zone * 100 + i);
           if (rr != NOWHERE) 
             { room = rr; break; }
       }
       if (room != NOWHERE)
           break;
       else { if (load_room == mortal_start_room) { send_to_char(SUMMON_FAIL, caster); return; }}
       load_room = mortal_start_room;
   } while(true);

   zone_rnum z = world[room].zone;
   std::vector<room_rnum> rooms;
   while (z == world[room].zone)
   {
       if (SECT(room) != SECT_SECRET &&
           !ROOM_FLAGGED(room, ROOM_DEATH) &&
           !ROOM_FLAGGED(room, ROOM_TUNNEL) &&
           !ROOM_FLAGGED(room, ROOM_NOTELEPORT) &&
           !ROOM_FLAGGED(room, ROOM_SLOWDEATH) &&
           !ROOM_FLAGGED(room, ROOM_GODROOM) &&
           !ROOM_FLAGGED(room, ROOM_PRIVATE))
       {
           rooms.push_back(room);
       }
       room++;
   }
 
   if (rooms.empty()) {  send_to_char(SUMMON_FAIL,caster);   return;   }
   while(true)
   {
      int rnd = number(0, rooms.size()-1);
      load_room = rooms[rnd];
      if (!ROOM_FLAGGED(load_room, ROOM_PEACEFUL))
          break;
      // chance for peaceful room
      if (number(0, 99) < GET_GLORY(caster) + GET_LEVEL(caster))
          break;
   }

   act("Возникла яркая вспышка, которая на мнговение ослепила Вас.", TRUE, ch, 0, 0, TO_ROOM);
   for (int i=0, e=to_recall.size(); i<e; ++i)
   {
       char_data *rch = to_recall[i];
       char_from_room(rch);
       char_to_room(rch, load_room);
       act("$n появил$u в центре комнаты.", TRUE, rch, 0, 0, TO_ROOM);
   }

   for (int i=0, e=to_recall.size(); i<e; ++i)
   {
       char_data *rch = to_recall[i];
       if (IS_NPC(rch)) continue;
       check_horse(rch);
       send_to_char(rch, "\r\n");
       look_at_room(rch, 0);
       entry_memory_mtrigger(rch, NULL, -1);
       greet_mtrigger(rch, NULL, -1);
       greet_otrigger(rch,-1);
       greet_memory_mtrigger(rch, NULL, -1);       
   }
}

ASPELL(spell_restore_magic)
{
    if (!victim)
    {
        send_to_char("Укажите цель для заклинания.", ch);
        return;
    }
    if (IS_IMMORTAL(victim))
    {
        if (ch == victim)
            send_to_char("Вам это не нужно.\r\n", ch);
        else
        {
            if (GET_SEX(victim) == SEX_FEMALE)
                send_to_char("Ей это не нужно.\r\n", ch);
            else 
                send_to_char("Ему это не нужно.\r\n", ch);
        }
        return;
    }

    for (int i=0,e=victim->Memory.size(); i<e; ++i)
    {
        int spell = victim->Memory.getid(i);
        GET_SPELL_MEM(victim, spell) += 1;
    }
    GET_MANA_STORED(victim) = 0;
    GET_MANA_NEED(victim) = 0;
    CLR_MEMORY(victim);
    
    sprintf(buf, "Глаза %s засветились ярким светом.\r\n", GET_RNAME(victim));
    send_to_char(buf, ch);

    send_to_char("Вы почувствовали себя умнее.\r\n", victim);
}

ASPELL(spell_teleport)
{ room_rnum fnd_room = NOWHERE;
  int       modi = 0;

  if (victim == NULL)
     victim = ch;

  if (IN_ROOM(victim) == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (victim != ch)
     {if (same_group(ch,victim))
         modi += 25;
      if (general_savingthrow(victim, SAVING_WILL, modi))
         {send_to_char(SUMMON_FAIL,ch);
          return;
         }
     }

  if (!IS_GOD(ch) &&
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT)
     )
    {send_to_char("Телепортирование невозможно.\r\n", ch);
     return;
    }

  for (modi = 0; modi < MAX_SUMMON_TRIES; modi++)
      {fnd_room = number(0,top_of_world);
       if ( world[fnd_room].zone == world[IN_ROOM(victim)].zone      &&
            SECT(fnd_room)       != SECT_SECRET                      &&
            !ROOM_FLAGGED(fnd_room, ROOM_DEATH      | ROOM_TUNNEL)   &&
            !ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORT)                 &&
	    !ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)                  &&
	    (!ROOM_FLAGGED(fnd_room, ROOM_GODROOM) || IS_IMMORTAL(victim)) &&
	    (!ROOM_FLAGGED(fnd_room, ROOM_PRIVATE))
           )
	   break;
      }

  if (modi >= MAX_SUMMON_TRIES)
     {send_to_char(SUMMON_FAIL,ch);
      return;
     }

    if (FIGHTING(victim) && (victim != ch))
     pk_agro_action(ch,FIGHTING(victim));

  act("$n медленно исчез$q из виду.",FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, fnd_room);
  check_horse(victim);
  act("$n медленно появил$u откуда-то.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
 /* entry_memory_mtrigger(victim);
  greet_mtrigger(victim,-1);
  greet_memory_mtrigger(victim);  */

  entry_memory_mtrigger(victim, NULL, -1);
  greet_mtrigger(victim, NULL, -1);
  greet_otrigger(victim,-1);
  greet_memory_mtrigger(victim, NULL, -1);
}

ASPELL(spell_relocate)
{ room_rnum to_room, fnd_room;
  int       is_newbie;

  if (victim == NULL)
     return;

  int delta = GET_LEVEL(victim)  - GET_LEVEL(ch);

  if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(victim) && !GET_COMMSTATE(ch))
  {
      if (delta <= 0)
      {
          send_to_char(SUMMON_FAIL, ch);
          return;
      }
  }

  if (delta > 1 && !GET_COMMSTATE(ch))
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if ((fnd_room = IN_ROOM(victim)) == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_GOD(ch) &&
      (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORT) ||
       SECT(fnd_room) == SECT_SECRET              ||
       ROOM_FLAGGED(fnd_room, ROOM_DEATH)         ||
       ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)     ||
       ROOM_FLAGGED(fnd_room, ROOM_TUNNEL)        ||
       ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORT)    ||
       (ROOM_FLAGGED(fnd_room, ROOM_PRIVATE)) ||
       (ROOM_FLAGGED(fnd_room, ROOM_GODROOM) && !IS_IMMORTAL(ch))
      )
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_NPC(ch))
     {if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE)
         to_room = real_room(calc_loadroom(ch));
      is_newbie = (world[to_room].zone >= MIN_NEWBIE_ZONE && world[to_room].zone <= MAX_NEWBIE_ZONE);
      if ((is_newbie  && (world[fnd_room].zone < MIN_NEWBIE_ZONE || world[fnd_room].zone > MAX_NEWBIE_ZONE)) ||
          (!is_newbie && world[fnd_room].zone >= MIN_NEWBIE_ZONE && world[fnd_room].zone <= MAX_NEWBIE_ZONE)
	 )
         {send_to_char(SUMMON_FAIL, ch);
          return;
         }
     }
  act("$n медленно исчез$q из виду.",FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, fnd_room);
  check_horse(ch);
  act("$n медленно появил$u откуда-то.", FALSE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
 /* entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);*/
  
  entry_memory_mtrigger(ch, NULL, -1);
  greet_mtrigger(ch, NULL, -1);
  greet_otrigger(ch, -1);
  greet_memory_mtrigger(ch, NULL, -1);

  if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE)))
     WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
}

ASPELL(spell_portal)
{ room_rnum to_room,fnd_room;
  int       is_newbie;

  if (victim == NULL)
     return;
  //if (GET_LEVEL(victim) > GET_LEVEL(ch) && !GET_COMMSTATE(ch))
  //   return;

  fnd_room  = IN_ROOM(victim);
  if (fnd_room == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }
  if (!IS_GOD(ch) && 
	  (IS_NPC(victim)							  ||	
       ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORT) ||
       SECT(fnd_room) == SECT_SECRET              ||
       ROOM_FLAGGED(fnd_room, ROOM_DEATH)         ||
       ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)     ||
       ROOM_FLAGGED(fnd_room, ROOM_TUNNEL)        ||
       ROOM_FLAGGED(fnd_room, ROOM_PRIVATE)       ||
       ROOM_FLAGGED(fnd_room, ROOM_GODROOM)       ||
       ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORT)
      )
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_NPC(ch))
     {if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE)
         to_room = real_room(calc_loadroom(ch));
      is_newbie = (world[to_room].zone >= MIN_NEWBIE_ZONE && world[to_room].zone <= MAX_NEWBIE_ZONE);
      if ((is_newbie  && (world[fnd_room].zone < MIN_NEWBIE_ZONE || world[fnd_room].zone > MAX_NEWBIE_ZONE)) ||
          (!is_newbie && world[fnd_room].zone >= MIN_NEWBIE_ZONE && world[fnd_room].zone <= MAX_NEWBIE_ZONE))
         {send_to_char(SUMMON_FAIL, ch);
          return;
         }
     }

  if (IN_ROOM(ch) == IN_ROOM(victim))
	{ send_to_char("Но Вы же и так находитесь в одной комнате!\r\n", ch);
		return;
	}
  to_room                     = IN_ROOM(ch);
  world[fnd_room].portal_room = to_room;
  world[fnd_room].portal_time = 1;
  act("Мерцающий портал появился в воздухе.",FALSE,world[fnd_room].people,0,0,TO_CHAR);
  act("Мерцающий портал появился в воздухе.",FALSE,world[fnd_room].people,0,0,TO_ROOM);
  world[to_room].portal_room   = fnd_room;
  world[to_room].portal_time   = 1;
  act("Мерцающий портал появился в воздухе.",FALSE,world[to_room].people,0,0,TO_CHAR);
  act("Мерцающий портал появился в воздухе.",FALSE,world[to_room].people,0,0,TO_ROOM);
  
}



ASPELL(spell_chain_lightning)
{ mag_char_areas(level, ch, victim, SPELL_CHAIN_LIGHTNING, SAVING_STABILITY);
}




ASPELL(spell_summon)
{ room_rnum to_room, fnd_room;
  int       is_newbie;

  if (ch == NULL || victim == NULL || ch == victim)
     return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3) &&
      !GET_COMMSTATE(ch) && !IS_NPC(ch)
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  fnd_room = IN_ROOM(ch);
  if (fnd_room == NOWHERE || IN_ROOM(victim) == NOWHERE)
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_GOD(ch) && !IS_NPC(victim) &&
      (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOSUMMON) ||
       ROOM_FLAGGED(fnd_room, ROOM_NOSUMMON)        ||
       SECT(IN_ROOM(ch)) == SECT_SECRET             ||
       ROOM_FLAGGED(fnd_room, ROOM_DEATH)           ||
       ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH)       ||
       ROOM_FLAGGED(fnd_room, ROOM_TUNNEL)          ||
       (ROOM_FLAGGED(fnd_room, ROOM_PRIVATE)) ||
       ROOM_FLAGGED(fnd_room, ROOM_GODROOM)
     )
    )
    {send_to_char(SUMMON_FAIL, ch);
     return;
    }

  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_AGGRESSIVE))
     {act("Вы произнесли древнее заклинание и $N предстал$y в тот же миг перед Вами.\r\n"
          "Почувствовав недоброжелательный характер $R, Вы поторопились отправить $S назад.",
          FALSE, ch, 0, victim, TO_CHAR);
      return;
     }

 
   if (!IS_IMMORTAL(ch) && !IS_NPC(victim) && !IS_NPC(ch) && 
      !PRF_FLAGGED(victim, PRF_SUMMONABLE)    &&
      !same_group(ch,victim)                  &&
      !may_kill_here(ch,victim)
     )
     {
       send_to_char(SUMMON_FAIL, ch);
       return;
     }

 if (!IS_IMMORTAL(ch) && !IS_NPC(victim) && 
      (PRF_FLAGGED(victim, PRF_SUMMONABLE) || same_group(ch,victim) ) && 
      (number(1,100) < 40)
     )
     {
       send_to_char(SUMMON_FAIL, ch);
       return;
     }

if (!same_group(ch,victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
     (pk_action_type(ch,victim) <= PK_ACTION_REVENGE )
     )
     {pk_agro_action(ch,victim);
     }
 
 if ( !IS_IMMORTAL(ch) &&
      ( (IS_NPC(ch) && IS_NPC(victim) ) ||
        (!IS_NPC(ch) && (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOSUMMON))) ||
        ( (pk_action_type(ch,victim) == PK_ACTION_KILL) && 
          !PRF_FLAGGED(victim, PRF_SUMMONABLE) && !same_group(ch,victim)
        )
      )
     )
     {
      send_to_char(SUMMON_FAIL2, ch);
      return;
     }

 

  if (!IS_IMMORTAL(ch) &&
      ((IS_NPC(ch) && IS_NPC(victim)) ||
       (!IS_NPC(ch) && ((IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOSUMMON)) ||
                        general_savingthrow(victim, SAVING_WILL, 0))
       )
      )
     )
     {send_to_char(SUMMON_FAIL, ch);
      return;
     }

  if (!IS_NPC(victim))
     {if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE)
         to_room = real_room(calc_loadroom(ch));
      is_newbie = (world[to_room].zone >= MIN_NEWBIE_ZONE && world[to_room].zone <= MAX_NEWBIE_ZONE);
      if ((is_newbie  && (world[fnd_room].zone < MIN_NEWBIE_ZONE || world[fnd_room].zone > MAX_NEWBIE_ZONE))  ||
          (!is_newbie && world[fnd_room].zone >= MIN_NEWBIE_ZONE && world[fnd_room].zone <= MAX_NEWBIE_ZONE)
         )
        {send_to_char(SUMMON_FAIL, ch);
         return;
        }
     }


  act("$n внезапно исчез$q.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, fnd_room);
  check_horse(victim);
  act("$n внезапно появл$u перед Вами.", TRUE, victim, 0, 0, TO_ROOM);
  act("Вас призвал$y $n!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
 /* entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim); */
  
  greet_otrigger(victim, -1);
  entry_memory_mtrigger(victim, NULL, -1);
  greet_mtrigger(victim, NULL, -1);
  greet_memory_mtrigger(victim, NULL, -1);
}


ASPELL(spell_locate_object)
{
  OBJ_DATA *i;
  char name[MAX_INPUT_LENGTH];
  int j;

  /*
   * FIXME: This is broken.  The spell parser routines took the argument
   * the player gave to the spell and located an object with that keyword.
   * Since we're passed the object and not the keyword we can only guess
   * at what the player originally meant to search for. -gg
   */
	 if (!obj)
 	  return;

  strcpy(name, cast_argument);

  j = level / 2;

  for (i = object_list; i && (j > 0); i = i->next) {
      if (IS_CORPSE(i))
          continue;
	  
      if (!isname(name, i->name))
          continue;

	  if (IN_ROOM(i) != NOWHERE && SECT(IN_ROOM(i)) == SECT_SECRET)
          continue;

     if (i->carried_by)
		if (SECT(IN_ROOM(i->carried_by)) == SECT_SECRET ||
		    (OBJ_FLAGGED(i, ITEM_NOLOCATE)				&&
			IS_NPC(i->carried_by))						||
				IS_IMMORTAL(i->carried_by))
					continue;

   /* if (i->carried_by)
      sprintf(buf, "%s находится в инвентаре у %s.\r\n", i->short_description, RPERS(i->carried_by, ch));
    else if (i->in_room != NOWHERE)
      sprintf(buf, "%s находится в %s.\r\n", i->short_description, world[i->in_room].name);
    else if (i->in_obj)
      sprintf(buf, "%s находится в %s.\r\n", i->short_description, i->in_obj->short_pdescription);
    else if (i->worn_by)
      sprintf(buf, "%s использует %s.\r\n",  i->short_description, PERS(i->worn_by, ch));
    else 
      sprintf(buf, "'%s' место нахождения неизвестно.\r\n", OBJS(i,ch));*/

			if (i->carried_by) {
			if (!IS_NPC(i->carried_by))
				sprintf(buf, "'%s' находится у %s в инвентаре.\r\n",
					i->short_description, RPERS(i->carried_by, ch));
			else
				continue;
		} else if (IN_ROOM(i) != NOWHERE && IN_ROOM(i)) {
			if (!OBJ_FLAGGED(i, ITEM_NOLOCATE))
				sprintf(buf, "'%s' находится в  %s.\r\n", i->short_description, world[IN_ROOM(i)].name);
			else
				continue;
		} else if (i->in_obj) {
			if (i->in_obj->carried_by)
				if (IS_NPC(i->in_obj->carried_by) && (OBJ_FLAGGED(i, ITEM_NOLOCATE)) )
					continue;
			if (IN_ROOM(i->in_obj) != NOWHERE && IN_ROOM(i->in_obj))
				if (OBJ_FLAGGED(i, ITEM_NOLOCATE))
					continue;
			if (i->in_obj->worn_by)
				if (IS_NPC(i->in_obj->worn_by)
				    && (OBJ_FLAGGED(i, ITEM_NOLOCATE)))
					continue;
			 else
				sprintf(buf, "'%s' находится в %s.\r\n", i->short_description, i->in_obj->short_pdescription);
		} else if (i->worn_by) {
			if (IS_NPC(i->worn_by) && !OBJ_FLAGGED(i, ITEM_NOLOCATE)
			      || !IS_NPC(i->worn_by))
				sprintf(buf, "'%s' надет%s на %s.\r\n", i->short_description,
					GET_OBJ_SUF_6(i), PPERS(i->worn_by, ch));
			else
				continue;
		} else
			sprintf(buf, "Местонахождение '%s' неизвестно.\r\n", VOBJS(i, ch));


    CAP(buf);
    send_to_char(buf, ch);
    j--;
  }

  if (j == level / 2)
    send_to_char("Вы ничего не нашли.\r\n", ch);
}



ASPELL(spell_create_weapon)
{ //go_create_weapon(ch,NULL,what_sky); в стадии разработки, пока не сделано 26.02.2006
}



inline float GetFamagePerRound(struct char_data * victim)
{
    float dam_per_round;
    dam_per_round = (GET_DR(victim) + str_app[GET_REAL_STR(victim)].todam +
		     victim->mob_specials.damnodice * (victim->mob_specials.damsizedice + 1) / 2) *
                    (1 +victim->mob_specials.extra_attack);

    if (MOB_FLAGGED (victim, (MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH |
			    MOB_ACIDBREATH | MOB_LIGHTBREATH)))
    dam_per_round = dam_per_round * 1.2;

  return dam_per_round;
}

inline int ReformedCharmHp(struct char_data *ch)
{    int r_hp;
     
     r_hp = GET_LEVEL(ch) * 10 + GET_REAL_CHA(ch)   * 10 +
            wis_app[GET_REAL_WIS(ch)].spell_success * 10 + 
	    wis_app[GET_REAL_WIS(ch)].slot_bonus    * 5  +
            MAX(GET_REAL_INT(ch) - 17, 0)           * 20;
    return(r_hp);
}

inline int NeedCharmHp(struct char_data *victim)
{   
    return GET_REAL_MAX_HIT(victim);
}


int check_charmee(struct char_data *ch, struct char_data *victim, int spellnum)
{struct follow_type *k;
 int    cha_summ = 0, hp_summ = 0;

 for (k = ch->followers; k; k = k->next)
     {if (AFF_FLAGGED(k->follower,AFF_CHARM) && k->follower->master == ch)
         {cha_summ++;
          hp_summ += NeedCharmHp(k->follower);
         }
     }

 if ( spellnum==SPELL_CLONE &&
      cha_summ >= MAX(1,(GET_LEVEL(ch)+4)/5-2) )
    {send_to_char("Невозможно! Вы не в состоянии уследить за последователями!\r\n",ch);
     return (FALSE);
    }

 if ( spellnum!=SPELL_CLONE &&
      cha_summ >= (GET_LEVEL(ch) + 10) / 10 )
    {send_to_char("Невозможно! Вы не в состоянии управлять последователями!\r\n",ch);
     return (FALSE);
    }
 if ( spellnum!=SPELL_CLONE &&
     !WAITLESS(ch) &&
      hp_summ + NeedCharmHp(victim) + MAX(0, (int)GetFamagePerRound(victim) - GET_LEVEL(ch) * 2) * 
                                              GET_LEVEL(victim) / 3 >= ReformedCharmHp(ch) )
    {send_to_char("Вы не в состоянии управлять такой группой!\r\n",ch);
     return (FALSE);
    }
 return (TRUE);
}

ASPELL(spell_charm)
{ struct affected_type af;

  if (victim == NULL || ch == NULL)
     return;

  if (victim == ch)
     send_to_char("Вы и так себя любите больше, чем кого либо!\r\n", ch);
  else
  if (!IS_NPC(victim))
	{ send_to_char("Вы не можете очаровывать игроков!\r\n", ch);
      pk_agro_action(ch,victim);
    }
  else
  if (!IS_IMMORTAL(ch) && 
      (AFF_FLAGGED(victim, AFF_SANCTUARY) ||
       MOB_FLAGGED(victim, MOB_PROTECT)
      )
     )
     send_to_char("Ваша цель под божественной защитой!\r\n", ch);
  else
  if (!IS_IMMORTAL(ch) && MOB_FLAGGED(victim, MOB_CHARM))
     send_to_char("Ваша жертва сопротивляется!\r\n", ch);
  else
  if (AFF_FLAGGED(ch, AFF_CHARM))
     send_to_char("Вы сами находитесь под очарованием!\r\n", ch);
  else
  if (AFF_FLAGGED(victim, AFF_CHARM)         ||
      MOB_FLAGGED(victim, MOB_AGGRESSIVE)    ||
      MOB_FLAGGED(victim, MOB_AGGRMONO)      ||
      MOB_FLAGGED(victim, MOB_AGGRPOLY)      ||
      MOB_FLAGGED(victim, MOB_AGGR_DAY)      ||
      MOB_FLAGGED(victim, MOB_AGGR_NIGHT)    ||
      MOB_FLAGGED(victim, MOB_AGGR_FULLMOON) ||
      MOB_FLAGGED(victim, MOB_AGGR_WINTER)   ||
      MOB_FLAGGED(victim, MOB_AGGR_SPRING)   ||
      MOB_FLAGGED(victim, MOB_AGGR_SUMMER)   ||
      MOB_FLAGGED(victim, MOB_AGGR_AUTUMN)	 ||
	  GET_QUESTOR(victim)
     )
     send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
  else
  if (IS_HORSE(victim))
     act("Я скакун боевой, а ты мне глазки строишь! - завопил$Y $N.",FALSE,ch,0,victim,TO_CHAR);
  else
  if (!WAITLESS(ch) && IS_MAGIC_USER(ch) && GET_CLASS(victim) != CLASS_HUMAN)
	  send_to_char("Вы можете зачаровывать только человекоподобные существа!\r\n", ch);
  else
  if (!WAITLESS(ch) && IS_DRUID(ch) && !IS_ANIMALS(victim))
      send_to_char("Вы можете зачаровывать только животных!\r\n", ch);
  else
  if (!WAITLESS(ch) && IS_CLERIC(ch) && !IS_UNDEADS(victim))
      send_to_char("Вы можете подчинить себе только нежить!\r\n", ch);
  else
    if (FIGHTING(victim) || GET_POS(victim) < POS_RESTING)
     act("$M сейчас, похоже, не до Вас.",FALSE,ch,0,victim,TO_CHAR);
  else
  if (circle_follow(victim, ch))
     send_to_char("Следование по кругу запрещено.\r\n", ch);
  else
  if (!IS_IMMORTAL(ch) &&
      general_savingthrow(victim, SAVING_WILL, (GET_REAL_CHA(ch) - 10) * 3))
     send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
  else
     {if (!check_charmee(ch,victim,SPELL_CHARM))
         return;

      if (victim->master)
         {if (stop_follower(victim,SF_MASTERDIE))
             return;
         }
  //     sprintf(buf,"%d - требуется    %d - имеется",NeedCharmHp(victim),ReformedCharmHp(ch));
//	   send_to_char(buf,ch);

	  if (GET_LEVEL(ch) < GET_LEVEL(victim) ||
		  check_charmee(ch,victim,SPELL_CHARM) > ReformedCharmHp(ch))
	  {   send_to_char("Вы недостаточно сильны для этого!\r\n", ch);
		  return;	
	  }
	  affect_from_char(victim, SPELL_CHARM);
      add_follower(victim, ch);
      af.type = SPELL_CHARM;
      if (GET_REAL_INT(victim) > GET_REAL_INT(ch))
         af.duration = pc_duration(victim,GET_REAL_CHA(ch),0,0,0,0);
      else
         af.duration = pc_duration(victim,GET_REAL_CHA(ch)+number(1,10),0,0,0,0);
      af.modifier    = 0;
      af.location    = 0;
      af.bitvector   = AFF_CHARM;
      af.battleflag  = 0;
      affect_to_char(victim, &af);

      act("$n очаровал$y Вас так, что Вы готовы исполнить любой $s приказ.", FALSE, ch, 0, victim, TO_VICT);
  
    if (IS_NPC(victim))
    { REMOVE_BIT(MOB_FLAGS(victim, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
      REMOVE_BIT(MOB_FLAGS(victim, MOB_SPEC), MOB_SPEC);
      SET_BIT(MOB_FLAGS(victim, MOB_NOTRAIN), MOB_NOTRAIN);
      SET_SKILL(victim, SKILL_PUNCTUAL, 0);
    }
  }
}



ACMD(do_findhelpee)
{ struct char_data     *helpee;
  struct follow_type   *k;
  struct affected_type af;
  int    cost, times;

  if (IS_NPC(ch) ||
      (!WAITLESS(ch)// && GET_CLASS(ch) != CLASS_MERCHANT)
     ))
     {send_to_char("Вы не можете этим воспользоваться!\r\n",ch);
      return;
     }

  if (subcmd == SCMD_FREEHELPEE)
     {for (k = ch->followers; k; k=k->next)
          if (AFF_FLAGGED(k->follower, AFF_HELPER) &&
              AFF_FLAGGED(k->follower, AFF_CHARM))
             break;
      if (k)
         {act("Вы отказались от услуг $R.",FALSE,ch,0,k->follower,TO_CHAR);
          affect_from_char(k->follower, SPELL_CHARM);
          stop_follower(k->follower, SF_CHARMLOST);
         }
      else
         act("У Вас нет в настоящее время наемников!",FALSE,ch,0,0,TO_CHAR);
      return;
     }

  argument = one_argument(argument, arg);

  if (!*arg)
     {for (k = ch->followers; k; k=k->next)
          if (AFF_FLAGGED(k->follower, AFF_HELPER) &&
              AFF_FLAGGED(k->follower, AFF_CHARM))
             break;
      if (k)
         act("Вашим наемником является $N.",FALSE,ch,0,k->follower,TO_CHAR);
      else
         act("У Вас нет наемников!",FALSE,ch,0,0,TO_CHAR);
      return;
     }

  if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char("Здесь нет никого с таким именем.\r\n", ch);
      return;
     }
  for (k = ch->followers; k; k=k->next)
      if (AFF_FLAGGED(k->follower, AFF_HELPER) &&
          AFF_FLAGGED(k->follower, AFF_CHARM))
         break;
  if (k)
     {act("Вы уже наняли $V.",FALSE,ch,0,k->follower,TO_CHAR);
      return;
     }

  if (helpee == ch)
     send_to_char(" Это Вы здорово придумали! Себя нанять и себе же заплатить!\r\n", ch);
  else
  if (!IS_NPC(helpee))
     send_to_char("Вы не можете нанимать игроков!\r\n", ch);
  else
  if (!WAITLESS(ch) && !NPC_FLAGGED(helpee, NPC_HELPED))
     act("$N не может быть нанят!",FALSE,ch,0,helpee,TO_CHAR);
  else
  if (AFF_FLAGGED(helpee, AFF_CHARM))
     act("$N находится в чей-то власти.",FALSE,ch,0,helpee,TO_CHAR);
  else
  if (IS_HORSE(helpee))
     send_to_char("Вы не можете нанимать скакунов!\r\n", ch);
  else
  if (FIGHTING(helpee) || GET_POS(helpee) < POS_RESTING)
     act("$N сражается и ему явно не до Вас!.",FALSE,ch,0,helpee,TO_CHAR);
  else
  if (circle_follow(helpee, ch))
     send_to_char("Следование по кругу запрещено.\r\n", ch);
  else
     {one_argument(argument, arg);
      if (!arg)
          times = 0;
      else
      if ((times = atoi(arg)) < 0)
         {act("На какое время Вы хотите нанять $V?.",FALSE,ch,0,helpee,TO_CHAR);
          return;
         }
      if (!times)
         {cost = GET_LEVEL(helpee) * TIME_KOEFF;
          sprintf(buf,"$n сказал$y Вам: \"За один час эта услуга будет стоить %d %s\".\r\n",
                  cost,desc_count(cost, WHAT_MONEYu));
          act(buf,FALSE,helpee,0,ch,TO_VICT);
          return;
         }
      cost =  (WAITLESS(ch) ? 0 : 1) * GET_LEVEL(helpee) * TIME_KOEFF * times;
      if (cost > GET_GOLD(ch))
         {sprintf(buf,"$n сказал$y Вам: \"За %d %s для Вас будет стоить %d %s - и у Вас явно не хватает денег!\"",
                  times, desc_count(times, WHAT_HOUR),
                  cost, desc_count(cost, WHAT_MONEYu));
          act(buf,FALSE,helpee,0,ch,TO_VICT);		
          return;
         }
      if (GET_LEVEL(ch) < GET_LEVEL(helpee))
         {sprintf(buf,"$n сказал$y Вам: \" Вы еще не доросли, чтобы я служил$y Вам.\"");
          act(buf,FALSE,helpee,0,ch,TO_VICT);		
          return;
         }	 
       if (helpee->master)
          {if (stop_follower(helpee,SF_MASTERDIE))
              return;
          }
       GET_GOLD(ch)  -= cost;
       affect_from_char(helpee, AFF_CHARM);
       sprintf(buf,"$n сказал$y Вам: \"%s, я жду Ваших приказаний!\"",
               GET_SEX(ch) == SEX_FEMALE ? "Повелительница" : "Повелитель");
       act(buf,FALSE,helpee,0,ch,TO_VICT);
       add_follower(helpee, ch);
       af.type        = SPELL_CHARM;
       af.duration    = pc_duration(helpee,times*TIME_KOEFF,0,0,0,0);
       af.modifier    = 0;
       af.location    = 0;
       af.bitvector   = AFF_CHARM;
       af.battleflag  = 0;
       affect_to_char(helpee, &af);
       SET_BIT(AFF_FLAGS(helpee, AFF_HELPER), AFF_HELPER);
		if (IS_NPC(helpee))
		{ REMOVE_BIT(MOB_FLAGS(helpee, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
		  REMOVE_BIT(MOB_FLAGS(helpee, MOB_SPEC), MOB_SPEC);
 		  SET_BIT(MOB_FLAGS(helpee, MOB_NOTRAIN), MOB_NOTRAIN);
		}
     }
}
void show_weapon(struct char_data *ch, struct obj_data *obj)
{
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
    {*buf = '\0';
     if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
        sprintf(buf, "Можно взять %s в правую руку.\r\n", VOBJS(obj,ch));
     if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
        sprintf(buf+strlen(buf), "Можно взять %s в левую руку.\r\n", VOBJS(obj,ch));
     if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
        sprintf(buf+strlen(buf), "Можно взять %s в обе руки.\r\n", VOBJS(obj,ch));
     if (*buf)
        send_to_char(buf,ch);
    }
}


void mort_show_obj_values(struct obj_data *obj, struct char_data *ch,
                          int fullness)
{int i, found, drndice = 0, drsdice = 0, count = 0;

 send_to_char("Вы узнали следующее:\r\n", ch);
 sprintf(buf, "Предмет \'&C%s&n\', Тип:&G ", obj->short_description);
 sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
 strcat(buf, buf2);
 strcat(buf, "&n\r\n");
 send_to_char(buf, ch);
 if (GET_OBJ_RLVL(obj))
 { sprintf(buf, "Уровень предмета для генерации рандомстатов: %d\r\n", GET_OBJ_RLVL(obj));
   send_to_char(buf, ch);
 }
 strcpy(buf, diag_weapon_to_char(obj, TRUE));
 if (*buf)
    send_to_char(buf, ch);

 if (fullness < 20)
    return;

 //show_weapon(ch, obj);

 sprintf(buf, "Вес: %d, Цена: %d, Рента: Экипирован/в инвентаре %d/%d)\r\n",
         GET_OBJ_WEIGHT(obj),
         GET_OBJ_COST(obj),
         GET_OBJ_RENTE(obj),
         GET_OBJ_RENT(obj));
 send_to_char(buf, ch);

 if (fullness < 30)
    return;

 send_to_char("Материал: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprinttype(obj->obj_flags.material, material_name, buf);
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 sprintf(buf,"Таймер  : %d\r\n", GET_OBJ_TIMER(obj));
 send_to_char(buf, ch);

 if (fullness < 40)
    return;

 send_to_char("Флаги неудобств: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.no_flag, no_bits, buf, ", ");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 50)
    return;

 send_to_char("Флаги запретов : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.anti_flag, anti_bits, buf, ", ");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 60)
    return;

 send_to_char("Флаги предмета : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ", ");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 75)
    return;

 switch (GET_OBJ_TYPE(obj))
 {
 case ITEM_SCROLL:
 case ITEM_POTION:
      sprintf(buf, "Содержит заклинания: ");
      if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
	     sprintf(buf + strlen(buf)," %s", spell_name(GET_OBJ_VAL(obj, 1)));
	  if (GET_OBJ_VAL(obj, 2) >= 1 && GET_OBJ_VAL(obj, 2) < MAX_SPELLS)
	     sprintf(buf + strlen(buf)," %s", spell_name(GET_OBJ_VAL(obj, 2)));
	  if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
	     sprintf(buf + strlen(buf)," %s", spell_name(GET_OBJ_VAL(obj, 3)));
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
 case ITEM_WAND:
 case ITEM_STAFF:
      sprintf(buf, "Вызывает заклинания: ");
      if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
         sprintf(buf + strlen(buf), " %s\r\n", spell_name(GET_OBJ_VAL(obj, 3)));
      sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n",
	          GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
 case ITEM_WEAPON:
      drndice = GET_OBJ_VAL(obj, 1);
      drsdice = GET_OBJ_VAL(obj, 2);
      sprintf(buf, "Наносимые повреждения: %dD%d", drndice,drsdice);
      sprintf(buf + strlen(buf), " (среднее %.1f.)\r\n",
	          ((drsdice + 1) * drndice / 2.0));
      send_to_char(buf, ch);
      break;
 case ITEM_ARMOR:
      drndice = GET_OBJ_VAL(obj, 0);
      drsdice = GET_OBJ_VAL(obj, 1);
      sprintf(buf, " &KЗащита (AC)&n: &y%d&n\r\n", drndice);
      send_to_char(buf, ch);
      sprintf(buf, " &KКласс брони&n: &y%d&n\r\n", drsdice);
      send_to_char(buf, ch);
      break;  
 case ITEM_BOOK://изменить 
      found = FALSE;
     *buf = '\0';
 if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
		{ for (i = 0; i < NUM_CLASSES; i++)
		   if (spell_info[GET_OBJ_VAL(obj,1)].slot_forc[i]!=12)
		   {   sprintf(buf + strlen(buf), "Круг изучения для %sа: &C%d&n\r\n",pc_class_types[i], spell_info[GET_OBJ_VAL(obj,1)].slot_forc[i]);
		       found = TRUE;
		   }    
				send_to_char(buf,ch); 
				sprintf(buf, "Содержит заклинание: &C\"%s\"&n\r\n", spell_info[GET_OBJ_VAL(obj,1)].name);
				send_to_char(buf,ch);
		}
			if (!found)
	 send_to_char("Данная магия не определена для персонажей.\r\n", ch);
	 break;
 case ITEM_INGRADIENT:

      sprintbit(GET_OBJ_SKILL(obj),ingradient_bits,buf2);
      sprintf(buf,"%s\r\n",buf2);
      send_to_char(buf,ch);

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_USES))
         {sprintf(buf,"можно применить %d раз\r\n",GET_OBJ_VAL(obj,2));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_LAG))
         {sprintf(buf,"можно применить 1 раз в %d сек",(i = GET_OBJ_VAL(obj,0) & 0xFF));
          if (GET_OBJ_VAL(obj,3) == 0 ||
              GET_OBJ_VAL(obj,3) + i < time(NULL))
             strcat(buf,"(можно применять).\r\n");
          else
             sprintf(buf + strlen(buf),"(осталось %ld сек).\r\n",GET_OBJ_VAL(obj,3) + i - time(NULL));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_LEVEL))
         {sprintf(buf,"можно применить c %d уровня.\r\n",(GET_OBJ_VAL(obj,0) >> 8) & 0x1F);
          send_to_char(buf,ch);
         }

      if ((i = real_object(GET_OBJ_VAL(obj,1))) >= 0)
         {sprintf(buf,"прототип %s%s%s.\r\n",
                      CCCYN(ch, C_NRM),
                      (obj_proto+i)->short_vdescription,
                      CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
         }
      break;

 }


 if (fullness < 90)
    return;

 send_to_char("Аффекты: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.affects, weapon_affects, buf, ", ");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 if (fullness < 100)
    return;
 found = FALSE;
	count = MAX_OBJ_AFFECT;
	for (i = 0; i < count; i++) {
		drndice = obj->affected[i].location;
		drsdice = obj->affected[i].modifier;
		if ((drndice != APPLY_NONE) && (drsdice != 0)) {
			if (!found) {
				send_to_char("Дополнительные свойства:\r\n", ch);
				found = TRUE;
			}
			sprinttype(drndice, apply_types, buf2);
			int negative = 0;
			for (int j = 0; *apply_negative[j] != '\n'; j++)
				if (!str_cmp(buf2, apply_negative[j])) {
					negative = TRUE;
					break;
				}
			switch (negative) {
			case FALSE:
				if (obj->affected[i].modifier < 0)
					negative = TRUE;
				break;
			case TRUE:
				if (obj->affected[i].modifier < 0)
					negative = FALSE;
				break;
			}
			sprintf(buf, " &c%-35s%s%d&n\r\n",
				buf2, negative ? " &R -" : " &G +",
				abs(obj->affected[i].modifier));
			send_to_char(buf, ch);
		}
	}
 }

void imm_show_obj_values(struct obj_data *obj, struct char_data *ch)
{int i, found, drndice = 0, drsdice = 0;

 send_to_char("Вы узнали следующее:\r\n", ch);
 sprintf(buf, "Предмет \'&C%s&n\', Тип:&G ", obj->short_description);
 sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
 strcat(buf, buf2);
 strcat(buf, "&n\r\n");
 send_to_char(buf, ch);

 if (GET_OBJ_RLVL(obj))
 { sprintf(buf, "Уровень предмета для генерации рандомстатов: %d\r\n", GET_OBJ_RLVL(obj));
   send_to_char(buf, ch);
 }
 strcpy(buf, diag_weapon_to_char(obj, TRUE));
 if (*buf)
    send_to_char(buf, ch);

// show_weapon(ch, obj);

send_to_char("Материал: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprinttype(obj->obj_flags.material, material_name, buf);
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 sprintf(buf,"Таймер  : %d\r\n", GET_OBJ_TIMER(obj));
 send_to_char(buf, ch);

 send_to_char("Аффекты        : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.affects, weapon_affects, buf, ", ");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Флаги предмета : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ", ");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Флаги запретов : ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.anti_flag, anti_bits, buf, ", ");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Флаги неудобств: ", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(obj->obj_flags.no_flag, no_bits, buf, ", ");
 strcat(buf, "\r\n");
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);

 sprintf(buf, "Вес: %d, Цена: %d, Рента: Экипирован/в инвентаре %d/%d\r\n",
              GET_OBJ_WEIGHT(obj),
	          GET_OBJ_COST(obj),
	          GET_OBJ_RENTE(obj),
	          GET_OBJ_RENT(obj));
 send_to_char(buf, ch);

 switch (GET_OBJ_TYPE(obj))
 {
 case ITEM_SCROLL:
 case ITEM_POTION:
      sprintf(buf, "Содержит заклинания: ");
      if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 1)));
      if (GET_OBJ_VAL(obj, 2) >= 1 && GET_OBJ_VAL(obj, 2) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 2)));
      if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
	     sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 3)));
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
 case ITEM_WAND:
 case ITEM_STAFF:
      sprintf(buf, "Вызывает заклинания: ");
      if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
         sprintf(buf + strlen(buf), " %s\r\n", spell_name(GET_OBJ_VAL(obj, 3)));
      sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n",
	          GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
 case ITEM_WEAPON:
      sprintf(buf, "Наносимые повреждения: %dD%d",
              GET_OBJ_VAL(obj, 1),GET_OBJ_VAL(obj, 2));
      sprintf(buf + strlen(buf), " (среднее %.1f.)\r\n",
	          (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
      send_to_char(buf, ch);
      break;
 case ITEM_ARMOR:
      sprintf(buf, " &KЗащита (AC)&n: &y%d&n\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
      sprintf(buf, " &KКласс брони&n: &y%d&n\r\n", GET_OBJ_VAL(obj, 1));
      send_to_char(buf, ch);
      break;
 case ITEM_BOOK://изменить
    found = FALSE;
     *buf = '\0';
 if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
		{ for (i = 0; i < NUM_CLASSES; i++)
		   if (spell_info[GET_OBJ_VAL(obj,1)].slot_forc[i]!=12)
		   {   sprintf(buf + strlen(buf), "Круг изучения для %sа: &C%d&n\r\n",pc_class_types[i], spell_info[GET_OBJ_VAL(obj,1)].slot_forc[i]);
		       found = TRUE;
		   }    
				send_to_char(buf,ch); 
				sprintf(buf, "Содержит заклинание: &C\"%s\"&n\r\n", spell_info[GET_OBJ_VAL(obj,1)].name);
				send_to_char(buf,ch);
		}
			if (!found)
	 send_to_char("Данная магия не определена для персонажей.\r\n", ch);
	 break;
 case ITEM_INGRADIENT:

      sprintbit(GET_OBJ_SKILL(obj),ingradient_bits,buf2);
      sprintf(buf,"%s\r\n",buf2);
      send_to_char(buf,ch);

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_USES))
         {sprintf(buf,"можно применить %d раз\r\n",GET_OBJ_VAL(obj,2));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_LAG))
         {sprintf(buf,"можно применить 1 раз в %d сек",(i = GET_OBJ_VAL(obj,0) & 0xFF));
          if (GET_OBJ_VAL(obj,3) == 0 ||
              GET_OBJ_VAL(obj,3) + i < time(NULL))
             strcat(buf,"(можно применять).\r\n");
          else
             sprintf(buf + strlen(buf),"(осталось %ld сек).\r\n",GET_OBJ_VAL(obj,3) + i - time(NULL));
          send_to_char(buf,ch);
         }

      if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_LEVEL))
         {sprintf(buf,"можно применить c %d уровня.\r\n",(GET_OBJ_VAL(obj,0) >> 8) & 0x1F);
          send_to_char(buf,ch);
         }

      if ((i = real_object(GET_OBJ_VAL(obj,1))) >= 0)
         {sprintf(buf,"прототип %s%s%s.\r\n",
                      CCCYN(ch, C_NRM),
                      (obj_proto+i)->short_rdescription,
                      CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
         }
      break;
 }
 found = FALSE;
for (i = 0; i < MAX_OBJ_AFFECT; i++) {
		if ((obj->affected[i].location != APPLY_NONE) && (obj->affected[i].modifier != 0)) {
			if (!found) {
				send_to_char("Дополнительные свойства:\r\n", ch);
				found = TRUE;
			}
			sprinttype(obj->affected[i].location, apply_types, buf2);
			int negative = FALSE;
			for (int j = 0; *apply_negative[j] != '\n'; j++) {
				if (!str_cmp(buf2, apply_negative[j])) {
					negative = TRUE;
					break;
				}
			}
			switch (negative) {
			case FALSE:
				if (obj->affected[i].modifier < 0)
					negative = TRUE;
				break;
			case TRUE:
				if (obj->affected[i].modifier < 0)
					negative = FALSE;
				break;
			}
			sprintf(buf, " &c%-35s%s%d&n\r\n",
				buf2, negative ? " &R -" : " &G +",
				abs(obj->affected[i].modifier));
			send_to_char(buf, ch);
		}
	}  
 
 }

#define IDENT_SELF_LEVEL 6

void mort_show_char_values(struct char_data *victim, struct char_data *ch, int fullness)
{struct affected_type *aff;
 int    found, val0, val1, val2;

 sprintf(buf, "Имя: %s\r\n", GET_NAME(victim));
 send_to_char(buf, ch);

 if (!IS_NPC(victim) && victim == ch)
    {sprintf(buf, "Возраст %s : %d %s, %d %s, %d %s и %d %s.\r\n",
              GET_RNAME(victim),
              age(victim)->year, desc_count(age(victim)->year, WHAT_YEAR), 
              age(victim)->month,desc_count(age(victim)->month, WHAT_MONTH),
	          age(victim)->day,  desc_count(age(victim)->day, WHAT_DAY),
	          age(victim)->hours,desc_count(age(victim)->hours, WHAT_HOUR));
      send_to_char(buf, ch);
     }
 if (fullness < 20 && ch != victim)
    return;

 val0 = GET_HEIGHT(victim);
 val1 = GET_WEIGHT(victim);
 val2 = GET_SIZE(victim);
 sprintf(buf, "Рост %d , Вес %d, Размер %d\r\n", val0, val1, val2);
 send_to_char(buf, ch);
 if (fullness < 60 && ch != victim)
    return;

 val0 = GET_LEVEL(victim);
 val1 = GET_HIT(victim);
 val2 = GET_REAL_MAX_HIT(victim);
 sprintf(buf, "Уровень : %d, может выдержать повреждений : %d(%d)\r\n",
         val0, val1, val2);
 send_to_char(buf,ch);	
 if (fullness < 90 && ch != victim)
    return;

 val0 = compute_armor_class(victim);
 val1 = GET_REAL_HR(victim); // + str_app[GET_REAL_STR(victim)].tohit;
 val2 = GET_REAL_DR(victim); // + str_app[GET_REAL_STR(victim)].todam;
 sprintf(buf, "Защита : %d, Хитролл : %d, Дамролл : %d, Броня : %d\r\n",
         val0, val1, val2, GET_ARMOUR(victim));
 send_to_char(buf, ch);

 if (fullness < 100 || (ch != victim && !IS_NPC(victim)))
    return;

 val0 = GET_STR(victim);
 val1 = GET_INT(victim);
 val2 = GET_WIS(victim);
 sprintf(buf, "Сила: %d, Интеллект: %d, Мудрость: %d, ", val0, val1, val2);
 val0 = GET_DEX(victim);
 val1 = GET_CON(victim);
 val2 = GET_CHA(victim);
 sprintf(buf+strlen(buf),"Ловкость: %d, Тело: %d, Удача: %d\r\n",val0,val1,val2);
 send_to_char(buf, ch); 	

 sprintf(buf,
	"Сопротивления: [&RОгонь:%2d &WВоздух:%2d &BВода:%2d &cЗемля:%2d &GЖивучесть:%2d &mРазум:%2d &YИммунитет:%2d&n]\r\n",
	GET_RESIST(victim, 0), GET_RESIST(victim, 1), GET_RESIST(victim, 2), GET_RESIST(victim, 3),
	GET_RESIST(victim, 4), GET_RESIST(victim, 5), GET_RESIST(victim, 6));
 send_to_char(buf, ch);
 sprintf(buf,
	"Защита от магических аффектов : [%d], Защита от магических повреждений : [%d]\r\n", GET_ARESIST(victim), GET_DRESIST(victim));
 send_to_char(buf, ch);

 if (fullness < 120 || (ch != victim && !IS_NPC(victim)))
    return;

 for (aff = victim->affected, found = FALSE; aff; aff = aff->next)
     {if (aff->location != APPLY_NONE &&
          aff->modifier != 0)
         {if (!found)
             {send_to_char("Дополнительные свойства:\r\n", ch);
              found = TRUE;
              send_to_char(CCRED(ch, C_NRM), ch);
             }
          sprinttype(aff->location, apply_types, buf2);
          sprintf(buf, "   %s изменяет на %s%ld\r\n", buf2,
                  aff->modifier > 0 ? "+" : "", aff->modifier);
          send_to_char(buf, ch);
         }
     }
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Аффекты :\r\n", ch);
 send_to_char(CCCYN(ch, C_NRM), ch);
 sprintbits(victim->char_specials.saved.affected_by, affected_bits, buf2, "\r\n ");
 sprintf(buf, "%s\r\n", buf2);
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);
}

void imm_show_char_values(struct char_data *victim, struct char_data *ch)
{struct affected_type *aff;
 int    found;

 sprintf(buf, "Имя: %s\r\n", GET_NAME(victim));
 send_to_char(buf, ch);

 if (!IS_NPC(victim))
    {sprintf(buf, "Возраст %s : %d %s, %d %s, %d %s и %d %s.\r\n",
              GET_RNAME(victim),
              age(victim)->year, desc_count(age(victim)->year, WHAT_YEAR), 
              age(victim)->month,desc_count(age(victim)->month, WHAT_MONTH),
	          age(victim)->day,  desc_count(age(victim)->day, WHAT_DAY),
	          age(victim)->hours,desc_count(age(victim)->hours, WHAT_HOUR));
      send_to_char(buf, ch);
     }

 sprintf(buf, "Рост %d(%s%d%s), Вес %d(%s%d%s), Размер %d(%s%d%s)\r\n",
         GET_HEIGHT(victim),
         CCRED(ch, C_NRM), GET_REAL_HEIGHT(victim), CCNRM(ch, C_NRM),
         GET_WEIGHT(victim),
         CCRED(ch, C_NRM), GET_REAL_WEIGHT(victim), CCNRM(ch, C_NRM),
         GET_SIZE(victim),
         CCRED(ch, C_NRM), GET_REAL_SIZE(victim), CCNRM(ch, C_NRM)
         );
 send_to_char(buf, ch);

 sprintf(buf, "Уровень : %d, может выдержать повреждений : %d(%d,%s%d%s)\r\n",
	          GET_LEVEL(victim), GET_HIT(victim), GET_MAX_HIT(victim),
	          CCRED(ch, C_NRM), GET_REAL_MAX_HIT(victim), CCNRM(ch, C_NRM)
	          );
 send_to_char(buf,ch);	

 sprintf(buf, "Защита : %d(%s%d%s), Хитролл : %d(%s%d%s), Дамролл : %d(%s%d%s), Броня : %d\r\n",
         GET_AC(victim),
         CCRED(ch, C_NRM), compute_armor_class(victim), CCNRM(ch, C_NRM),
         GET_HR(victim),
         CCRED(ch, C_NRM), GET_REAL_HR(victim), CCNRM(ch, C_NRM),
         GET_DR(victim),
         CCRED(ch, C_NRM), GET_REAL_DR(victim), CCNRM(ch, C_NRM), GET_ARMOUR(victim));
 send_to_char(buf, ch);

 sprintf(buf, "Сила: %d(%s%d%s), Интеллект: %d(%s%d%s), Мудрость: %d(%s%d%s), Ловкость: %d(%s%d%s), Тело: %d(%s%d%s), Удача: %d(%s%d%s)\r\n",
     	                    GET_STR(victim), CCRED(ch, C_NRM), GET_REAL_STR(victim), CCNRM(ch, C_NRM),
                            GET_INT(victim), CCRED(ch, C_NRM), GET_REAL_INT(victim), CCNRM(ch, C_NRM),
                            GET_WIS(victim), CCRED(ch, C_NRM), GET_REAL_WIS(victim), CCNRM(ch, C_NRM),
	                        GET_DEX(victim), CCRED(ch, C_NRM), GET_REAL_DEX(victim), CCNRM(ch, C_NRM),
                            GET_CON(victim), CCRED(ch, C_NRM), GET_REAL_CON(victim), CCNRM(ch, C_NRM),
                            GET_CHA(victim), CCRED(ch, C_NRM), GET_REAL_CHA(victim), CCNRM(ch, C_NRM)
                            );
 send_to_char(buf, ch);
 
 for (aff = victim->affected, found = FALSE; aff; aff = aff->next)
     {if (aff->location != APPLY_NONE &&
          aff->modifier != 0)
         {if (!found)
             {send_to_char("Дополнительные свойства:\r\n", ch);
              found = TRUE;
              send_to_char(CCRED(ch, C_NRM), ch);
             }
          sprinttype(aff->location, apply_types, buf2);
          sprintf(buf, "   %s изменяет на %s%ld\r\n", buf2,
                  aff->modifier > 0 ? "+" : "", aff->modifier);
          send_to_char(buf, ch);
         }
     }
 send_to_char(CCNRM(ch, C_NRM), ch);

 send_to_char("Аффекты :\r\n", ch);
 send_to_char(CCMAG(ch, C_NRM), ch);
 sprintbits(victim->char_specials.saved.affected_by, affected_bits, buf2, "\r\n ");
 sprintf(buf, "%s\r\n", buf2);
 if (victim->followers)
    sprintf(buf + strlen(buf), "За ним следуют.\r\n");
 else
 if (victim->master)
    sprintf(buf + strlen(buf), "Следует за %s.\r\n", GET_TNAME(victim->master));
 sprintf(buf + strlen(buf), "Уровень повреждений %d, уровень заклинаний %d.\r\n", GET_DAMAGE(victim), GET_CASTER(victim));
 send_to_char(buf, ch);
 send_to_char(CCNRM(ch, C_NRM), ch);
}

ASPELL(skill_identify)
{
  if (obj)
     if (IS_IMMORTAL(ch))
        imm_show_obj_values(obj,ch);
     else
        mort_show_obj_values(obj,ch,train_skill(ch,SKILL_IDENTIFY,skill_info[SKILL_IDENTIFY].max_percent,0));
  else
  if (victim)
    { if (IS_IMMORTAL(ch))
         imm_show_char_values(victim,ch);
      else
	  { if (GET_LEVEL(victim) < 4 )
         {send_to_char("Вы можете опознать только персонажа, достигнувшего четвертого уровня.\r\n", ch);
          return;
         }
         mort_show_char_values(victim,ch,train_skill(ch,SKILL_IDENTIFY,skill_info[SKILL_IDENTIFY].max_percent,victim));
	  }
     }
}

ASPELL(spell_identify)
{
  if (obj)
     mort_show_obj_values(obj,ch,100);
  else
  if (victim){
	  /*
	  if (victim != ch){
		  send_to_char("С помощью магии нельзя опознать другое существо.\r\n", ch);
          return;
	  }
	  */
	  /*
     if (GET_LEVEL(victim) < 4 )
         {send_to_char("Вы можете опознать себя только достигнув четвертого уровня.\r\n", ch);
          return;
         }
		 */
      mort_show_char_values(victim,ch,100);
     }
}


/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */

ASPELL(spell_enchant_weapon)
{
  int i;
  int rnd, tip, score;
  int count;
  int aff_count;

  if (ch == NULL || obj == NULL)
    return;

  // Если не оружие или уже магическое то ничего не делаем. 
  //if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC)){
  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || GET_OBJ_RLVL(obj)>0){
  act("$o совсем не изменился.", FALSE, ch, obj, 0, TO_CHAR);
    return;
	}

  // Если есть на оружии аффекты, то ничего не делаем.
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].location == APPLY_NONE) break;

  SET_BIT(GET_OBJ_EXTRA(obj, ITEM_MAGIC), ITEM_MAGIC);

  aff_count = number(1, 3); // количество эффектов в зависимости от скила
  
  while(aff_count--) {
  
	  score = number(5, 60); // потом сюда зависимость от скила
	  
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
		
		
		count=0;
		
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
		
	}

	GET_OBJ_RLVL(obj)= GET_OBJ_CUR(obj);
	
  if (!WAITLESS(ch))
  GET_OBJ_TIMER(obj) = MAX(GET_OBJ_TIMER(obj), 4 * 24 * 60);

   if (IS_GOOD(ch))
  { SET_BIT(GET_FLAG(GET_OBJ_NO(obj), ITEM_AN_EVIL), ITEM_AN_EVIL);
    act("$o светится голубым.", FALSE, ch, obj, 0, TO_CHAR);
  } 
  else
  if (IS_EVIL(ch))
  { SET_BIT(GET_FLAG(GET_OBJ_NO(obj), ITEM_AN_GOOD), ITEM_AN_GOOD);
    act("$o светится красным.", FALSE, ch, obj, 0, TO_CHAR);
  }
 else
    act("$o светится желтым.", FALSE, ch, obj, 0, TO_CHAR);
}

ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (AFF_FLAGGED(victim, AFF_POISON))
        send_to_char("ЯД жжёт Ваши вены и Вы страдаете.\r\n", ch);
      else
        send_to_char("Вы чувствуете себя здоровым.\r\n", ch);
    } else {
      if (AFF_FLAGGED(victim, AFF_POISON))
        act("Вы чувствуете $N отравлен$Y.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("Вы чувствуете $N здоров$Y.", FALSE, ch, 0, victim, TO_CHAR); 
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("Вы чувствуете $o отравлен$y.",FALSE,ch,obj,0,TO_CHAR); 
      else
	act("Вы чувствуете $o не отравлен$y.", FALSE, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char("Вы чувствуете, этот предмет нельзя отравить.\r\n", ch);
    }
  }
}

ASPELL(spell_control_weather)
{ const char *sky_info=NULL;
 int  i, duration, zone, sky_type = 0;

 if (what_sky > SKY_LIGHTNING)
    what_sky = SKY_LIGHTNING;

 switch (what_sky)
 {case SKY_CLOUDLESS:
    sky_info = "На небе появились облака.";
    break;
  case SKY_CLOUDY:
    sky_info = "Небо заволокло дождевой тучей.";
    break;
  case SKY_RAINING:
    if (time_info.month >= MONTH_MAY      &&   time_info.month <= MONTH_OCTOBER)
       {sky_info = "Начался ливень.";
        create_rainsnow(&sky_type,WEATHER_LIGHTRAIN,0,50,50);
       }
    else
    if (time_info.month >= MONTH_DECEMBER ||   time_info.month <= MONTH_FEBRUARY)
       {sky_info = "Начался снегопад.";
        create_rainsnow(&sky_type,WEATHER_LIGHTSNOW,0,50,50);
       }
    else
    if (time_info.month == MONTH_MART || time_info.month == MONTH_NOVEMBER)
       {if (weather_info.temperature > 2)
           {sky_info = "Начался проливной дождь.";
            create_rainsnow(&sky_type,WEATHER_LIGHTRAIN,0,50,50);
           }
        else
           {sky_info = "Повалил снег крупными хлопьями.";
            create_rainsnow(&sky_type,WEATHER_LIGHTSNOW,0,50,50);
           }
       }
    break;
  case SKY_LIGHTNING:
    sky_info = "На небе не осталось ни единого облачка.";
    break;
  default:
    break;
  }

 if (sky_info)
    { duration = MAX(GET_LEVEL(ch) / 8, 2);
      zone     = world[ch->in_room].zone;
      for (i = FIRST_ROOM; i <= top_of_world; i++)
          if (world[i].zone == zone && SECT(i) != SECT_INSIDE && SECT(i) != SECT_CITY)
             {world[i].weather.sky          = what_sky;
              world[i].weather.weather_type = sky_type;
              world[i].weather.duration     = duration;
              if (world[i].people)
                 {act(sky_info,FALSE,world[i].people,0,0,TO_ROOM);
                  act(sky_info,FALSE,world[i].people,0,0,TO_CHAR);
                 }
             }
    }
}
