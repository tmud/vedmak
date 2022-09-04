/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"

extern CHAR_DATA *character_list;
extern struct index_data *mob_index;
extern struct room_data *world;
extern int no_specials;

// external structs
void go_bash(CHAR_DATA *ch, CHAR_DATA *vict);
void go_stupor(CHAR_DATA *ch, CHAR_DATA *vict);
void go_mighthit(CHAR_DATA *ch, CHAR_DATA *vict);
void npc_wield(CHAR_DATA *ch);
void npc_steal(CHAR_DATA *ch);
void hunt_victim(CHAR_DATA * ch);
void go_backstab(CHAR_DATA *ch, CHAR_DATA *vict);
void npc_armor(CHAR_DATA *ch);
void npc_group(CHAR_DATA *ch);
void npc_light(CHAR_DATA *ch);
void npc_groupbattle(CHAR_DATA *ch);
int  legal_dir(CHAR_DATA *ch, int dir, int need_specials_check, int show_msg);
int  npc_walk(CHAR_DATA *ch);
int  npc_track(CHAR_DATA *ch);
int  npc_move(CHAR_DATA *ch, int dir, int need_specials_check);
int  may_kill_here(CHAR_DATA *ch, CHAR_DATA *victim);
int  skip_sneaking(CHAR_DATA *ch, CHAR_DATA *vict);
int  skip_hiding(CHAR_DATA *ch, CHAR_DATA *vict);
int  npc_scavenge(CHAR_DATA *ch);
int  npc_loot(CHAR_DATA *ch);
int  skip_camouflage(CHAR_DATA *ch, CHAR_DATA *vict);

ACMD(do_get);
ACMD(do_gen_comm);
ACMD(do_rescue);

// local functions
int do_npc_rescue(struct char_data * ch_hero, struct char_data * ch_victim);
void mobile_activity(int activity_level, int missed_pulses);
void clearMemory(struct char_data * ch);
void go_disarm(struct char_data *ch, struct char_data *vict);
void go_chopoff(struct char_data *ch, struct char_data *vict);
void pulse_affect_update (CHAR_DATA * ch);

extern struct descriptor_data *descriptor_list;


#define KILL_FIGHTING   (1 << 0)
#define CHECK_HITS      (1 << 10)
#define SKIP_HIDING     (1 << 11)
#define SKIP_CAMOUFLAGE (1 << 12)
#define SKIP_SNEAKING   (1 << 13)


int extra_aggressive(CHAR_DATA *ch, CHAR_DATA *victim)
{
 
	int time_ok=FALSE, no_time = TRUE, month_ok = FALSE, no_month = TRUE, agro = FALSE;

	if (!IS_NPC(ch))
		return (FALSE);

 if (MOB_FLAGGED(ch, MOB_AGGRESSIVE)){
//    log("Агрессивен...%s", GET_NAME(ch));
	 return (TRUE);
 }
 if (victim && MOB_FLAGGED(ch, MOB_AGGRMONO) && //нападает на добрых
     !IS_NPC(victim) && IS_GOOD(victim))//разобраться ... временно раз нет религии отключил
     //GET_RELIGION(victim) == RELIGION_MONO)
    agro = TRUE;

 if (victim && MOB_FLAGGED(ch, MOB_AGGRPOLY) && // нападает на злых
     !IS_NPC(victim) && IS_EVIL(victim))
    // GET_RELIGION(victim) == RELIGION_POLY*/)
    agro = TRUE;
  
 if (victim && NPC_FLAGGED(ch, NPC_CITYGUARD) &&
	 !IS_NPC(victim) && PLR_FLAGGED(victim, PLR_KILLER))
/*	{  
	for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) == CON_PLAYING && i != victim->desc && i->character &&
	!PLR_FLAGGED(i->character, PLR_WRITING) &&
	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF))
      if ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
	      !AWAKE(i->character))
	        continue;
      act(buf, FALSE, ch, 0, i->character, TO_VICT);
     }*/
	  agro = TRUE;	
	
 
	if (MOB_FLAGGED(ch, MOB_AGGR_DAY))
    {no_time = FALSE;
     if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)
        time_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_NIGHT))
    {no_time = FALSE;
     if (weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET)
        time_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_WINTER))
    {no_month = FALSE;
     if (weather_info.season == SEASON_WINTER)
        month_ok = TRUE;
    }


 if (MOB_FLAGGED(ch, MOB_AGGR_SPRING))
    {no_month = FALSE;
     if (weather_info.season == SEASON_SPRING)
        month_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_SUMMER))
    {no_month = FALSE;
     if (weather_info.season == SEASON_SUMMER)
        month_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_AUTUMN))
    {no_month = FALSE;
     if (weather_info.season == SEASON_AUTUMN)
        month_ok = TRUE;
    }

 if (MOB_FLAGGED(ch, MOB_AGGR_FULLMOON))
    {no_time = FALSE;
     if (weather_info.moon_day >= 12 && weather_info.moon_day <= 15 &&
         (weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET))
        time_ok = TRUE;
    }
 if (agro || !no_time || !no_month)
    return ((no_time || time_ok) && (no_month || month_ok));
 else
    return (FALSE);

}


int attack_best(CHAR_DATA *ch, CHAR_DATA *victim)
{ if (victim)
     {if (GET_SKILL(ch, SKILL_BACKSTAB) && !FIGHTING(victim))
         {go_backstab(ch, victim);
          return (TRUE);
         }
      if (GET_SKILL(ch, SKILL_MIGHTHIT))
         {go_mighthit(ch,victim);
          return (TRUE);
         }
      if (GET_SKILL(ch, SKILL_STUPOR))
         {go_stupor(ch,victim);
          return (TRUE);
         }
      if (GET_SKILL(ch, SKILL_BASH))
         {go_bash(ch, victim);
          return (TRUE);
         }
      if (GET_SKILL(ch, SKILL_DISARM))
         {go_disarm(ch, victim);
         }
      if (GET_SKILL(ch, SKILL_CHOPOFF))
         {go_chopoff(ch, victim);
         }
      if (!FIGHTING(ch))
         hit(ch,victim,TYPE_UNDEFINED,1);
      return (TRUE);
     }
  else
     return (FALSE);
}

bool MobRandomAttack (CHAR_DATA *ch)
{ CHAR_DATA *vict;
   bool yes = false;


     for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
		{ if ((IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_CHARM))	   ||
	           PRF_FLAGGED(vict, PRF_NOHASSLE) || !MAY_SEE(ch,vict)||
	           !may_kill_here(ch,vict))										
						continue;

	 	// Если мало жизней, то не вписываемся в качестве хелпера.
		if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict) && 
			GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch))
						continue;

		if (FIGHTING(vict) && number(0, 1)	&&
			IS_NPC(FIGHTING(vict))			&&
			CAN_SEE(ch, vict)				&& 
			!AFF_FLAGGED(FIGHTING(vict),AFF_CHARM))
				{ yes = true;
				   break;
				}
		}

	 if (yes)
	 { if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING)
		{ act("$n поднял$u.",FALSE,ch,0,0,TO_ROOM);
          GET_POS(ch) = POS_STANDING;
        }

       if (FIGHTING(vict))
         act("&W$n, выискав цель, присоединил$u к бою на стороне $R.&n", FALSE, ch, 0, FIGHTING(vict), TO_ROOM);


	    if (!attack_best(ch, vict) && !FIGHTING(ch))
                hit(ch, vict, TYPE_UNDEFINED, 1);

       return true;
    }
 return false;
}


int perform_best_mob_attack (CHAR_DATA *ch, int extmode)
{ CHAR_DATA *vict, *victim = NULL, *use_light = NULL, *min_hp = NULL,
            *min_lvl = NULL, *caster = NULL,   *best   = NULL, *rch = NULL;
  int   extra_aggr = false;
  bool  kill_this = false/*, yes = false*/;

  void pk_translate_pair(CHAR_DATA * * pkiller, CHAR_DATA * * pvictim);
  
  
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
  {      if ((IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_CHARM))	||
	        PRF_FLAGGED(vict, PRF_NOHASSLE) || !MAY_SEE(ch,vict)||
	        !may_kill_here(ch,vict))										
						continue;

       kill_this = FALSE;

       // Mobile too damage Если у хелпера мало хитов, то желание помочь проходим дальше. 
       if (IS_SET(extmode, CHECK_HITS) &&
           MOB_FLAGGED(ch, MOB_WIMPY)  &&
           AWAKE(vict)                 &&
           GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch))
						continue;
/*
1. Сюда приходит vict в качестве игрока или же зачармленного игроком моба, при условии что
   не стоит флаг PRF_NOHASSLE (моб не может напасть на игрока с таким флагом),
   моб, который рассматривает возможность помочь может видеть цель vict и 
   может проводить боевые действия в данной местности (комнате).

2. Проверяем на флаги и разделяем на действия по отношению к vict со стороны моба (ch),
   который согласно флагов хочет помочь.

3. Проверяем действие, если у моба стоит флаг NPC_RANDHELP, то надо выбрать цель (рандомно)
   что бы в эту цель вписаться и больше не проводить проверок.

4. Обработаем цели: 
	а) Если они в группе и хотя бы один из согрупников дерется нужно нападать
       вне зависимости дерутся ли остальные согруппники или нет.

	б) Если игроки вне группы и не дерутся, нападать нельзя, может они
       просто так рядом стоят и ничего плохого делать мобам не хотят.

vict->master -  хозяин чармиста, а сам чармисит vict
*/
//(!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM)) &&  
//!!! Возможно вот как реализовать, 
	   /*1. если кидать рандом, (number(0, 1)) и он условию не удовлетворяет, надо стедать continue,
		    что бы перебором взять другую цель vict, после обрабатываем по тому же алгоритму.
			Если кинулся рандом положительно, то проверяем vict на выполнение условий, при
			котором ch сможет вписаться в vict.
	   */
		
/*
1.  Здесь собственно говоря попытка реализовать хелп моба (ch), который просматривает
	исходя из его флага рандомхелп, в кого вписываться, если кто то дерется с его СОБРАТОМ МОБОМ.
2.	Мы должны проверить принадлежность всех находящихся в комнате к лидеру. В зависимости от возможности
	иметь право вступится помогающему мобу в драку на стороне СОБРАТА МОБА.
3.	Нет необходимости проверять список лидера, кто у него в группе, ведь игроки могут быть
	совсем в другом месте или другой зоне, а проверяя, нагружаем движок и уменьшаем эффективность работы кода.
	а мы должны проверить лишь только тех, кто находится именно в этой комнате.

*/
		
	/*	  if (AFF_FLAGGED(vict, AFF_GROUP))//имеет ли цель группу?
		  { for (f = vict->followers; f; f = f->next)//кто является последователем? крутим список
				if ((AFF_FLAGGED(f->follower, AFF_GROUP) ||
					AFF_FLAGGED(f->follower, AFF_CHARM)) &&
					may_kill_here(ch, f->follower))
		              if (!number(0, 2))
					  {	 rch = f->follower;
					    kill_this = TRUE;
					  }
		  }*/
 	 
		

		 if (IS_SET(extmode, KILL_FIGHTING)            &&
			  FIGHTING(vict) && FIGHTING(vict) != ch   &&
			  IS_NPC(FIGHTING(vict))				   &&
              !AFF_FLAGGED(FIGHTING(vict),AFF_CHARM))
			{ if (number(0, 1))
				{ pk_translate_pair( &vict , &vict );
					if (!vict || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
						!CAN_SEE(ch, vict) || !may_kill_here(ch, vict) )
					    continue;
				}
				kill_this = TRUE;
			} 
	   
			else
       // ... but no aggressive for this char
			if (!(extra_aggr =  extra_aggressive(ch,vict)))
				continue;
		
	   
	   
	   // skip sneaking, hiding and camouflaging pc
       if (IS_SET(extmode, SKIP_SNEAKING))
		{ skip_sneaking(vict,ch);
           if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK))
			   REMOVE_BIT(AFF_FLAGS(vict, AFF_SNEAK), AFF_SNEAK);
               if (AFF_FLAGGED(vict, AFF_SNEAK)) 
					continue;
        }

       if (IS_SET(extmode, SKIP_HIDING))
		{ skip_hiding(vict,ch);
           if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE))
			   REMOVE_BIT(AFF_FLAGS(vict, AFF_HIDE), AFF_HIDE);
        }

       if (IS_SET(extmode, SKIP_CAMOUFLAGE))
		{ skip_camouflage(vict,ch);
           if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE))
			   REMOVE_BIT(AFF_FLAGS(vict, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
        }

       if (!CAN_SEE(ch, vict))
          continue;				

       // Mobile aggresive на будущее для одного из классов персонажей
       if (!kill_this && extra_aggr)
          {if (GET_CLASS(vict) == number(0,10) &&
               number(1,GET_LEVEL(vict) * GET_REAL_CHA(vict)) >
               number(1,GET_LEVEL(ch)   * GET_REAL_INT(ch))
              )
              continue;
           kill_this = TRUE;
          }
	
       if (!kill_this)
          continue;

       // define victim if not defined
       if (!victim)
          victim = vict;
	
       if (IS_DEFAULTDARK(IN_ROOM(ch)) &&
           ((GET_EQ(vict,ITEM_LIGHT) && GET_OBJ_VAL(GET_EQ(vict,ITEM_LIGHT), 0)) ||
            ((AFF_FLAGGED(vict, AFF_SINGLELIGHT) || 
              AFF_FLAGGED(vict, AFF_HOLYLIGHT)) &&
              !AFF_FLAGGED(vict, AFF_HOLYDARK))
           ) &&
           (!use_light || GET_REAL_CHA(use_light) > GET_REAL_CHA(vict))
          )
          use_light = vict;

       if (!min_hp ||
           GET_HIT(vict)   + GET_REAL_CHA(vict)   * 10 < 
	   GET_HIT(min_hp) + GET_REAL_CHA(min_hp) * 10
          )
          min_hp = vict;
	  
       if (!min_lvl ||
           GET_LEVEL(vict)    + number(1,GET_REAL_CHA(vict)) < 
	   GET_LEVEL(min_lvl) + number(1,GET_REAL_CHA(min_lvl))
          )
          min_lvl = vict;
	  
       if (IS_CASTER(vict) && 
           (!caster || 
            GET_CASTER(caster) * GET_REAL_CHA(vict) < 
	    GET_CASTER(vict)   * GET_REAL_CHA(caster)
           )
          )
          caster = vict;
        }
    
  if (GET_REAL_INT(ch) < 5+number(1,6))
     best = victim;
  else
  if (GET_REAL_INT(ch) < 10+number(1,6))
     best = use_light ? use_light : victim;
  else
  if (GET_REAL_INT(ch) < 15+number(1,6))
     best = min_lvl ? min_lvl : (use_light ? use_light : victim);
  else
  if (GET_REAL_INT(ch) < 25+number(1,6))
     best = caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim));
  else
     best = min_hp ? min_hp : (caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim)));

  if (best)
     {/*
      sprintf(buf,"B-%s,V-%s,L-%s,Lev-%s,H-%s,C-%s",
              GET_NAME(best),
	      GET_NAME(victim),
	      use_light ? GET_NAME(use_light) : "None",
	      min_lvl   ? GET_NAME(min_lvl) : "None",	      
	      min_hp    ? GET_NAME(min_hp) : "None",
	      caster    ? GET_NAME(caster) : "None");
      act(buf,FALSE,ch,0,0,TO_ROOM);
      */
      if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING)
         {act("$n поднял$u.",FALSE,ch,0,0,TO_ROOM);
          GET_POS(ch) = POS_STANDING;
         }

      if (IS_SET(extmode,KILL_FIGHTING) && FIGHTING(best))
         act("$n присоединил$u к бою на стороне $R.",FALSE,ch,0,FIGHTING(best),TO_ROOM);

      if (!IS_NPC(best))
         {struct follow_type *f;
	  for (f = best->followers; f; f = f->next)
	      if (IS_NPC(f->follower)                   &&
	          MOB_FLAGGED(f->follower, MOB_CLONE)   &&
		  IN_ROOM(f->follower) == IN_ROOM(best) &&
		  number(1,30) <= GET_LEVEL(best)
		 )
		 {best = f->follower;
		  break;
		 }
		}

      if (!attack_best(ch, best) && !FIGHTING(ch))
         hit(ch,best,TYPE_UNDEFINED,1);
      return (TRUE);
     }
  return (FALSE);
}


int perform_best_horde_attack(CHAR_DATA *ch, int extmode)
{CHAR_DATA *vict;
 if (perform_best_mob_attack(ch, extmode))
    return (TRUE);

 for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict)                   || 
          !MAY_SEE(ch,vict)               ||
          MOB_FLAGGED(vict,MOB_PROTECT)
         )
         continue;
      if (!SAME_ALIGN(ch,vict))	  
         {if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) > POS_SLEEPING)
             {act("$n вскочил$y.",FALSE,ch,0,0,TO_ROOM);
              GET_POS(ch) = POS_STANDING;
             }
          if (!attack_best(ch, vict) && !FIGHTING(ch))
             hit(ch,vict,TYPE_UNDEFINED,1);
          return (TRUE);
         }
     }
  return (FALSE);
}



void do_aggressive_mob(CHAR_DATA *ch, bool check_sneak)
{ struct char_data *vict, *next_ch, *next_vict, *victim;
  int    mode = check_sneak ? SKIP_SNEAKING : 0;
  memory_rec *names;

  if (IN_ROOM(ch) == NOWHERE)
     return;

  for (ch = world[IN_ROOM(ch)].people; ch; ch = next_ch)
      { next_ch = ch->next_in_room;
        if (!IS_NPC(ch)                ||
	    !MAY_ATTACK(ch)            ||
            AFF_FLAGGED(ch, AFF_BLIND)
	   )
           continue;

        /****************  Horde */
	if (MOB_FLAGGED(ch,MOB_HORDE))
	   {perform_best_horde_attack(ch,mode | SKIP_HIDING | SKIP_CAMOUFLAGE);
            continue;
           }	       
	       
        /****************  Aggressive Mobs */
        if (extra_aggressive(ch,NULL))
           {if (perform_best_mob_attack(ch,mode | SKIP_HIDING | SKIP_CAMOUFLAGE | CHECK_HITS))
               continue;
           }

        
		/***************** При ВХОДЕ В КОМНАТУ МОБ ВСТАЕТ */
        		if (GET_POS(ch) < POS_FIGHTING &&
			    GET_POS(ch) > POS_SLEEPING && !GET_WAIT(ch))
		          for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
			    if (!IS_NPC(vict) && IN_ROOM(ch) == IN_ROOM(vict) &&
				CAN_SEE(ch, vict) && GET_DEFAULT_POS(ch) == POS_STANDING)
			     { GET_POS(ch) = POS_STANDING; 
			       act("$n прекратил$y отдыхать и встал$y.",FALSE,ch,0,0,TO_ROOM);	
			       break;
			     } 

        /*****************  Mob Memory      */
        if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch))
           {victim = NULL;
            // Find memory in room
            for (vict = world[ch->in_room].people; vict && !victim; vict = next_vict)
                {next_vict = vict->next_in_room;
                 if (IS_NPC(vict) || 
		     PRF_FLAGGED(vict, PRF_NOHASSLE)
		    )
	            continue;
                 for (names = MEMORY(ch); names && !victim; names = names->next)
                     if (names->id == GET_IDNUM(vict))
                        {if (!MAY_SEE(ch,vict) ||
                             !may_kill_here(ch,vict)
			    )
			    continue;
			 if (check_sneak)
	                    {skip_sneaking(vict,ch);
			     if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK))
                                 REMOVE_BIT(AFF_FLAGS(vict, AFF_SNEAK), AFF_SNEAK);
	                     if (AFF_FLAGGED(vict, AFF_SNEAK))
			         continue;
			        
	                    }
	                  skip_hiding(vict,ch);
                          if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE))
                              REMOVE_BIT(AFF_FLAGS(vict, AFF_HIDE), AFF_HIDE);
	                  skip_camouflage(vict,ch);
                          if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE))
                              REMOVE_BIT(AFF_FLAGS(vict, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
                          if (CAN_SEE(ch,vict))
	                     victim = vict;
	                 }
                 }
            // Is memory found ?
            if (victim)
               {if (GET_POS(ch) < POS_FIGHTING && 
	            GET_POS(ch) > POS_SLEEPING
		   )
	           {act("$n вскочил$y.",FALSE,ch,0,0,TO_ROOM);
	            GET_POS(ch) = POS_STANDING;
	           }
	        if (IS_ANIMALS(ch))
	            act("$n вспомнил$y $V.",FALSE,ch,0,victim,TO_ROOM);
	        else
                    act("\"$N, ты хотел$Y УБИТЬ меня, ты будешь убит$Y!\" - закричал$y $n.",
	                   FALSE, ch, 0, victim, TO_ROOM);

	        if (!attack_best(ch, victim))
		   {hit(ch, victim, TYPE_UNDEFINED, 1);
		   }
	        continue;
	     }
      }
	/****************  Helper Mobs     */
         if (NPC_FLAGGED(ch, NPC_RANDHELP))
	   if (MobRandomAttack(ch))
		continue;

	 if (MOB_FLAGGED(ch, MOB_HELPER))
	   if (perform_best_mob_attack(ch,mode | KILL_FIGHTING | CHECK_HITS))
		continue;
			
  }
}

#define  MAY_LIKES(ch)   ((!AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HELPER) && \
                          AWAKE(ch) && GET_WAIT(ch) <= 0)
void    mob_casting (struct char_data * ch);

void mobile_activity(int activity_level)
{
  struct char_data *ch, *next_ch, *vict, *first, *victim;
  struct room_direction_data * rdata=NULL;
  int    door, found, max,/* start_battle,*/ was_in =0;
  memory_rec *names;

  activity_level = activity_level % PULSE_MOBILE;
 

  for (ch = character_list; ch; ch = next_ch)
  {
     next_ch = ch->next;
     
     if (!IS_MOB(ch))
         continue;
     
        pulse_affect_update (ch);

       if (GET_WAIT(ch) > 0)
          GET_WAIT(ch)--;
       else
          GET_WAIT(ch) = 0;
	  
       if (GET_ACTIVITY(ch) != activity_level ||
           (was_in = IN_ROOM(ch)) == NOWHERE  ||
           GET_ROOM_VNUM(IN_ROOM(ch)) % 100 == 99
	  )
          continue;

       
       // Examine call for special procedure
       if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
          { if (mob_index[GET_MOB_RNUM(ch)].func == NULL)
		{ log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
                  GET_NAME(ch), GET_MOB_VNUM(ch));
		  REMOVE_BIT(MOB_FLAGS(ch, MOB_SPEC), MOB_SPEC);
          	}
           else
	     { if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, ""))
                  continue; // go to next char
             }
          }

   
       // Extract uncharmed mobs
       if (EXTRACT_TIMER(ch) > 0)
          {if (ch->master)
	         EXTRACT_TIMER(ch) = 0;
	       else     
              {EXTRACT_TIMER(ch)--;
	       if (!EXTRACT_TIMER(ch))
	          {
                extract_char(ch,FALSE);
	           continue;
	          }
	       }   
	  }

       // If the mob has no specproc, do the default actions
       if (FIGHTING(ch)                   ||
           GET_POS(ch) <= POS_STUNNED     ||
	       GET_WAIT(ch) > 0               ||
           AFF_FLAGGED(ch, AFF_CHARM)     ||
           AFF_FLAGGED(ch, AFF_HOLD)      ||
           AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
	       AFF_FLAGGED(ch, AFF_HOLDALL)	  ||
		   AFF_FLAGGED(ch, AFF_SLEEP)     
	  )
	  continue;
	  
       if (IS_HORSE(ch))
          {if (GET_POS(ch) < POS_FIGHTING)
	      GET_POS(ch) = POS_STANDING;
	   continue;
	  }
	  
       if (!AFF_FLAGGED(ch,AFF_SLEEP) &&
		   GET_POS(ch) == POS_SLEEPING &&
           GET_DEFAULT_POS(ch) > POS_SLEEPING
	  )
	  {GET_POS(ch) = GET_DEFAULT_POS(ch);
	   act("$n проснул$u.",FALSE,ch,0,0,TO_ROOM);
          }
	  	   
       if (!AWAKE(ch))
          continue;
	  
       for (vict = world[ch->in_room].people, max = FALSE;
            vict; vict = vict->next_in_room)
           {if (ch == vict)
               continue;
            if (FIGHTING(vict) == ch)
               break;
            if (!IS_NPC(vict) && CAN_SEE(ch,vict))
               max = TRUE;
           }

       // Mob is under attack
       if (vict)
          continue;

       
   // доорелизовать, что бы мобы лечили себя когда стоит вне боя и есть чем лечить.
/*	       if (!FIGHTING(ch)				  &&
		   GET_HIT(ch) < GET_MAX_HIT(ch) &&
		  ((GET_SPELL_MEM(ch,SPELL_EXTRA_HITS) && IS_SET(spell_info[SPELL_EXTRA_HITS].routines,NPC_CALCULATE))		  ||
           (GET_SPELL_MEM(ch,SPELL_HEAL)	   && IS_SET(spell_info[SPELL_HEAL].routines,NPC_CALCULATE))			  ||
		   (GET_SPELL_MEM(ch,SPELL_CURE_CRITIC)&& IS_SET(spell_info[SPELL_CURE_CRITIC].routines,NPC_CALCULATE))		  ||
		   (GET_SPELL_MEM(ch,SPELL_GROUP_HEAL) && IS_SET(spell_info[SPELL_GROUP_HEAL].routines,NPC_CALCULATE))		  ||
		   (GET_SPELL_MEM(ch,SPELL_CURE_CRITIC)&& IS_SET(spell_info[SPELL_CURE_CRITIC].routines,NPC_CALCULATE))		  ||
		   (GET_SPELL_MEM(ch,SPELL_CURE_SERIOUS)&& IS_SET(spell_info[SPELL_CURE_SERIOUS].routines,NPC_CALCULATE))	  ||
		   (GET_SPELL_MEM(ch,SPELL_CURE_LIGHT) && IS_SET(spell_info[SPELL_CURE_LIGHT].routines,NPC_CALCULATE))))
		   { 
			    if(GET_HIT(ch) >= GET_MAX_HIT(ch))
				{ act("$n прекратил$y отдыхать.",FALSE,ch,0,0,TO_ROOM);
	              GET_POS(ch) = POS_STANDING;
				}
		     mob_casting(ch);
		   }*/
	  
	   // Mob attemp rest
	   if (!max &&
           GET_HIT(ch) < (2 * GET_REAL_MAX_HIT(ch)/3) &&
           GET_POS(ch) > POS_RESTING)
		{ act("$n присел$y отдохнуть.",FALSE,ch,0,0,TO_ROOM);
          GET_POS(ch) = POS_RESTING;
        }	  
	  
	  
       // Mob return to default pos if full rested
       if (GET_HIT(ch) >= (2 * GET_REAL_MAX_HIT(ch)/3) &&
           GET_POS(ch) != GET_DEFAULT_POS(ch)
	  )
          switch (GET_DEFAULT_POS(ch))
          {case POS_STANDING:
                act("$n встал$y на ноги.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch) = POS_STANDING;
                break;
           case POS_SITTING:
                act("$n сел$y.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch) = POS_SITTING;
                break;
           case POS_RESTING:
                act("$n присел$y отдохнуть.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch) = POS_RESTING;
                break;
           case POS_SLEEPING:
                act("$n уснул$y.",FALSE,ch,0,0,TO_ROOM);
                GET_POS(ch) = POS_SLEEPING;
                break;
          }
	  
       // look at room before moving
       do_aggressive_mob(ch, FALSE);

       // if mob attack something
       if (FIGHTING(ch) || 
           GET_WAIT(ch) >  0)
          continue;
       
	   
	   //моб выискивает жертву дописал 20.07.2002 г.  
	  // 	 hunt_victim(ch); //временно отключил

       /* Scavenger (picking up objects) */
       if (MOB_FLAGGED(ch, MOB_SCAVENGER))
          npc_scavenge(ch);

       /* лутит трупы            */
       if (MOB_FLAGGED(ch, MOB_LOOTER))
          npc_loot(ch);
// вооружаем моба если есть чем и установлен флаг
       if (NPC_FLAGGED(ch, NPC_WIELDING))
     	   npc_wield(ch);
// одевается типа если что-)
       if (NPC_FLAGGED(ch, NPC_ARMORING))
      	   npc_armor(ch);
// ну это воровство чтоли -)
       if (NPC_FLAGGED(ch, NPC_STEALING))
    	   npc_steal(ch);

       if (GET_POS(ch) == POS_STANDING && NPC_FLAGGED(ch, NPC_INVIS))
           SET_BIT(AFF_FLAGS(ch, AFF_INVISIBLE), AFF_INVISIBLE);

       if (GET_POS(ch) == POS_STANDING && NPC_FLAGGED(ch, NPC_MOVEFLY))
           SET_BIT(AFF_FLAGS(ch, AFF_FLY), AFF_FLY);

       if (GET_POS(ch) == POS_STANDING &&
           NPC_FLAGGED(ch, NPC_SNEAK))
          {if (calculate_skill(ch,SKILL_SNEAK,100,0) >= number(0,100))
               SET_BIT(AFF_FLAGS(ch, AFF_SNEAK), AFF_SNEAK);
           else
               REMOVE_BIT(AFF_FLAGS(ch, AFF_SNEAK), AFF_SNEAK);
           //log("[MOBILE_ACTIVITY->AFFECT_TOTAL] Sneak start");
           affect_total(ch);
           //log("[MOBILE_ACTIVITY->AFFECT_TOTAL] Sneak stop");
          }

       if (GET_POS(ch) == POS_STANDING &&
           NPC_FLAGGED(ch, NPC_CAMOUFLAGE))
          {if (calculate_skill(ch,SKILL_CAMOUFLAGE,100,0) >= number(0,100))
               SET_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
           else
               REMOVE_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
           //log("[MOBILE_ACTIVITY->AFFECT_TOTAL] Camouflage start");
           affect_total(ch);
           //log("[MOBILE_ACTIVITY->AFFECT_TOTAL] Camouflage stop");
          }

       door = BFS_ERROR;

       // Helpers go to some dest
       if (door == BFS_ERROR &&
           MOB_FLAGGED(ch, MOB_HELPER)     /*|| 
		   NPC_FLAGGED(ch, NPC_RANDHELP)*/   &&  
           !MOB_FLAGGED(ch, MOB_SENTINEL)  &&
           !AFF_FLAGGED(ch,AFF_BLIND)      &&
           !ch->master                     &&
           GET_POS(ch) == POS_STANDING
          )
          {for (found = FALSE, door = 0; door < NUM_OF_DIRS; door++)
               {for(rdata = EXIT(ch, door), max = MAX(1,GET_REAL_INT(ch) / 10);
                    max > 0 && !found; max--)
                   {if (!rdata ||
                        rdata->to_room == NOWHERE ||
                        IS_SET(rdata->exit_info, EX_CLOSED) ||
                        !legal_dir(ch,door,TRUE,FALSE) ||
                        world[rdata->to_room].forbidden ||
                        IS_DARK(rdata->to_room) ||
                        (MOB_FLAGGED(ch, MOB_STAY_ZONE) &&
                         world[IN_ROOM(ch)].zone != world[rdata->to_room].zone
                        )
                       )
                       break;
                    for (first = world[rdata->to_room].people; first; first = first->next_in_room)
                        if (IS_NPC(first)      &&
                            !AFF_FLAGGED(first, AFF_CHARM) &&
                            !IS_HORSE(first)   &&
                            CAN_SEE(ch, first) &&
                            FIGHTING(first)    &&
			    SAME_ALIGN(ch,first)//тут еще разобраться с алигментом, будет ли искать при таком условии
                           )
                           {found = TRUE;
                            break;
                           }
                    rdata = world[rdata->to_room].dir_option[door];
                   }
                if (found)
                   break;
               }
           if (!found)
              door = BFS_ERROR;
          }

       if (GET_DEST(ch) != NOWHERE &&
           GET_POS(ch) > POS_FIGHTING &&
           door == BFS_ERROR)
          {npc_group(ch);
           door = npc_walk(ch);
          }

       if (GET_SKILL(ch, SKILL_TRACK) &&
           GET_POS(ch) > POS_FIGHTING &&
           MEMORY(ch) &&
           door == BFS_ERROR)
          door = npc_track(ch);

       if (door == BFS_ALREADY_THERE)
          {do_aggressive_mob(ch,FALSE);
           continue;
          }

       if (door == BFS_ERROR)
          door = number(0, 18);

       // Mob Movement 
       if (!MOB_FLAGGED(ch, MOB_SENTINEL) &&
           GET_POS(ch) == POS_STANDING &&
           (door >= 0 && door < NUM_OF_DIRS) &&
           EXIT(ch, door) &&
           EXIT(ch,door)->to_room != NOWHERE &&
           (legal_dir(ch,door,TRUE,FALSE) ||
		    (EXIT_FLAGGED (EXIT (ch, door), EX_CLOSED) &&
			MOB_FLAGGED (ch, MOB_OPENDOOR))) &&
		    !world[EXIT(ch, door)->to_room].forbidden &&
           (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
            world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone
           )
          )
          {  
             npc_move(ch, door, 1);
             npc_group(ch);
             npc_groupbattle(ch);
          }

       npc_light(ch);

       /*****************  Mob Memory      */
       if (MOB_FLAGGED(ch, MOB_MEMORY) &&
           MEMORY(ch)                  &&
           GET_POS(ch) > POS_SLEEPING  &&
	   !AFF_FLAGGED(ch, AFF_BLIND) &&
           !FIGHTING(ch))
          { victim = NULL;
           // Find memory in world

	    for (names = MEMORY(ch); names && !victim && (GET_SPELL_MEM(ch,SPELL_SUMMON) > 0 ||
                  GET_SPELL_MEM(ch,SPELL_RELOCATE) > 0 );
                names = names->next)
               for (vict = character_list; vict && !victim; vict = vict->next)
                   if (names->id == GET_IDNUM(vict)      &&
                       CAN_SEE(ch, vict)                 &&
                       !PRF_FLAGGED(vict, PRF_NOHASSLE))
                      { if (GET_SPELL_MEM(ch,SPELL_SUMMON) > 0 )
			  { cast_spell(ch,vict,0,SPELL_SUMMON);
  				         break;
			  } 
			else
                         if (GET_SPELL_MEM(ch,SPELL_RELOCATE) > 0 )
                          { cast_spell(ch,vict,0,SPELL_RELOCATE);
  					 break;
 			  }
		       }
 	   }
		   
       if (was_in != IN_ROOM(ch))
          do_aggressive_mob(ch, FALSE);
       /* Add new mobile actions here */

  }				// end for() 
}


void remember(CHAR_DATA * ch, CHAR_DATA * victim)
{ struct timed_type timed;
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch)    ||
      IS_NPC(victim) ||
      PRF_FLAGGED(victim,PRF_NOHASSLE) ||
      !MOB_FLAGGED(ch, MOB_MEMORY))
     return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
      if (tmp->id == GET_IDNUM(victim))
         {if (tmp->time > 0)
             tmp->time = time(NULL) + SECS_PER_MUD_HOUR * GET_REAL_INT(ch);
               present = TRUE;
         }

  if (!present)
     {CREATE(tmp, memory_rec, 1);
      tmp->next  = MEMORY(ch);
      tmp->id    = GET_IDNUM(victim);
      tmp->time  = time(NULL) + SECS_PER_MUD_HOUR * GET_REAL_INT(ch);
      MEMORY(ch) = tmp;
     }
     
  if (!timed_by_skill(victim, SKILL_HIDETRACK))
     {timed.skill = SKILL_HIDETRACK;
      timed.time  = GET_SKILL(ch,SKILL_TRACK) ? 6 : 3;
      timed_to_char(victim,&timed);
     }
}

// Заставим чара забыть жертву 
void forget(CHAR_DATA * ch, CHAR_DATA * victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim)) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return;			/* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}


// Стираем память чара 
void clearMemory(CHAR_DATA * ch)
{
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}
