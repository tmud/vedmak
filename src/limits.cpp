/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
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
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "dg_scripts.h"
#include "constants.h"
#include "exchange.h"

extern struct zone_data *zone_table;
extern INDEX_DATA *obj_index;
extern unsigned long dg_global_pulse;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct char_data *mob_proto;
extern struct obj_data *object_list;
extern struct room_data *world;
extern int max_exp_gain;
extern int max_exp_loss;
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern int use_autowiz;
extern int min_wizlist_lev;
extern int free_rent;
int    calc_loadroom(struct char_data *ch);
int    get_room_sky(int rnum);

/* local functions */
void   check_autowiz(struct char_data * ch);
void   decrease_level(struct char_data *ch);
void   quest_update(struct char_data *ch);
int    Crash_rentsave(struct char_data *ch, int cost, int num);
void   update_char_objects(struct char_data * ch);	/* handler.c */
void   reboot_wizlists(void);
int    max_exp_loss_pc(struct char_data *ch);
int    graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
int    max_exp_gain_pc(struct char_data *ch);
int    level_exp(int chclass, int level);
int    average_day_temp(void);
struct char_data *find_char(long n);



#define NO_DESTROY(obj) ((obj)->carried_by   			|| \
                         (obj)->worn_by      			|| \
			 (obj)->in_room == NOWHERE		|| \
                         (obj)->in_obj       			|| \
                         (obj)->proto_script 			|| \
                         (obj)->script       			|| \
                         GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN 	|| \
	             	 OBJ_FLAGGED(obj, ITEM_NODECAY))

#define NO_TIMER(obj)    (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) 
#define UPDATE_PC_ON_BEAT TRUE

int  up_obj_where(struct obj_data *obj)
{if (obj->in_obj)
    return up_obj_where(obj->in_obj);
 else
    return OBJ_WHERE(obj);
}
/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (age < 15)
    return (p0);		/* < 15   */
  else if (age <= 29)
    return (int) (p1 + (((age - 20) * (p2 - p1)) / 10));	/* 15..29 */
  else if (age <= 44)
    return (int) (p2 + (((age - 30) * (p3 - p2)) / 5));	/* 30..44 */
  else if (age <= 59)
    return (int) (p3 + (((age - 35) * (p4 - p3)) / 15));	/* 45..59 */
  else if (age <= 79)
    return (int) (p4 + (((age - 50) * (p5 - p4)) / 25));	/* 60..79 */
  else
    return (p6);		/* >= 80 */
}


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(struct char_data * ch)
{
  int gain = 0, restore = int_app[GET_REAL_INT(ch)].mana_per_tic, percent = 100;
  int stopmem = FALSE;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
			if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
				return (0);
//    gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);
gain = graf(age(ch)->year, restore-8, restore-4, restore, restore+5, restore, restore-4, restore-8);
    
		if (LIKE_ROOM(ch))
	     percent += 25;
	  if (average_day_temp() < -20)
         percent -= 10;
      else
      if (average_day_temp() < -10)
         percent -= 5;
     }
  
  
		/* Skill/Spell calculations */

	if (world[IN_ROOM(ch)].fires)
		 percent += MAX(50, 10+world[IN_ROOM(ch)].fires * 5);

    /* Position calculations    */
  if (FIGHTING(ch))
      percent -= 90;
else switch (GET_POS(ch)) {
    case POS_SLEEPING:
     if (IS_WEDMAK(ch))
		  percent += 170;
	else
		{ percent = 0;
		  stopmem = TRUE;
		}
        break;
    case POS_RESTING:
      percent += 100;
		break;
    case POS_SITTING:
      percent += 50;
    	break;
    case POS_STANDING:
      break;
    default:
      stopmem = TRUE;
      percent = 0;
      break;
}

	if (IN_ROOM(ch) != NOWHERE && IS_DARK(IN_ROOM(ch))) 
		if (!IS_WEDMAK(ch))
			{ percent = 0;
		      stopmem = TRUE;
			}
		
    
	if (AFF_FLAGGED(ch,AFF_HOLD || AFF_BLIND || AFF_SLEEP) ||
        AFF_FLAGGED(ch, AFF_HOLDALL))
		  	{stopmem = TRUE;
			 percent = 0;
			}

	 if (!IS_NPC(ch))
		 {if (GET_COND(ch, FULL) == 0)
		   percent -= 85;
		  if (GET_COND(ch, THIRST) == 0)
                   percent -= 45;
		  if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
                   percent -= 10;
		}
		percent += GET_MANAREG(ch);

if (AFF_FLAGGED(ch, AFF_POISON) && percent > 0)
     percent /= 4;
  percent  = MAX(0, MIN(250, percent));
  gain    = gain * percent / 100;
  gain = MAX(3, gain);
  return (stopmem ? 0 : gain);
}

/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data * ch)
{
  int gain,
      restore = MAX(10, GET_REAL_CON(ch) + con_app[GET_REAL_CON(ch)].hitp),
      percent = 100;

  if (IS_NPC(ch)) {
     gain = GET_LEVEL(ch) + GET_REAL_CON(ch);
  } else 
  { if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
         return (0);
   // gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);
 gain = graf(age(ch)->year, restore-3, restore, restore, restore-2, restore-3, restore-5, restore-7);
     if (LIKE_ROOM(ch))
         percent += 25;
      /* Weather specification */
      if (average_day_temp() < -20)
         percent -= 15;
      else
      if (average_day_temp() < -10)
         percent -= 10;
     }

  if (world[IN_ROOM(ch)].fires)
     percent  += MAX(50, 10+world[IN_ROOM(ch)].fires * 5);

	 /* Skill/Spell calculations */

    /* Position calculations    */

  switch (GET_POS(ch))
  { case POS_SLEEPING:
      percent += 25;
      break;
    case POS_RESTING:
      percent += 15;
      break;
    case POS_SITTING:
      percent += 10;
      break;
  }

  if (!IS_NPC(ch))
     { if (GET_COND(ch, FULL) == 0)
          percent -= 50;
       if (GET_COND(ch, THIRST) == 0)
          percent -= 25;
     }

  percent += GET_HITREG(ch);
  if (AFF_FLAGGED(ch, AFF_POISON) && percent > 0)
     percent /= 4;
  percent  = MAX(-200, MIN(3000, percent));//???????
  gain     = gain * percent / 100;
  if (!IS_NPC(ch))
     {if (GET_POS(ch) == POS_INCAP ||
          GET_POS(ch) == POS_MORTALLYW)
         gain = 0;
     }
  if (AFF_FLAGGED(ch, AFF_POISON))
     gain -= (GET_POISON(ch) * (IS_NPC(ch) ? 3 : 3));
	
 return (gain);
}

/* move gain pr. game hour */
int move_gain(struct char_data * ch)
{
  int gain, restore = con_app[GET_REAL_CON(ch)].hitp, percent = 100;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else 
  {if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
         return (0);
      gain = graf(age(ch)->year, 15+restore, 20+restore, 25+restore, 20+restore, 16+restore, 12+restore, 8+restore);
      /* Room specification    */
      if (LIKE_ROOM(ch))
         percent += 25;
      /* Weather specification */
      if (average_day_temp() < -20)
         percent -= 10;
      else
      if (average_day_temp() < -10)
         percent -= 5;
     }

  if (world[IN_ROOM(ch)].fires)
     percent += MAX(50, 10+world[IN_ROOM(ch)].fires * 5);
    //gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);

    /* Class/Level calculations */

    /* Skill/Spell calculations */


    /* Position calculations    */
  
 switch (GET_POS(ch))
  { case POS_SLEEPING:
      percent += 25;
      break;
    case POS_RESTING:
      percent += 15;
      break;
    case POS_SITTING:
      percent += 10;
      break;
  }

  if (!IS_NPC(ch))
     {if (GET_COND(ch, FULL) == 0)
         percent -= 50;
      if (GET_COND(ch, THIRST) == 0)
         percent -= 25;
      if (!IS_IMMORTAL(ch) && affected_by_spell(ch, SPELL_HIDE))
         percent -= 20;
      if (!IS_IMMORTAL(ch) && affected_by_spell(ch, SPELL_CAMOUFLAGE))
         percent -= 30;
     }

  percent += GET_MOVEREG(ch);
  if (AFF_FLAGGED(ch, AFF_POISON) && percent >  0)
     percent /= 4;
  percent  = MAX(0, MIN(250, percent));
  gain     = gain * percent / 100;
  return (gain);
}

#define MINUTE            60
int interpolate(int min_value, int pulse)
{int sign = 1, int_p, frac_p, i, carry,
     x, y;

 if (min_value < 0)
    {sign      = -1;
     min_value = -min_value;
    }
 int_p  = min_value / MINUTE;
 frac_p = min_value % MINUTE;
 if (!frac_p)
    return(sign * int_p);
 pulse = time(NULL) % MINUTE + 1;
 x     = MINUTE;
 y     = 0;
 for (i = 0, carry = 0; i <= pulse; i++)
     {y += frac_p;
      if (y >= x)
         {x += MINUTE;
          carry = 1;
         }
      else
         carry = 0;
     }
 return (sign * (int_p + carry));
}

void beat_points_update(int pulse)
{ struct char_data *i, *next_char;
  int    restore;

  if (!UPDATE_PC_ON_BEAT)
     return;

  /* only for PC's */
  for (i = character_list; i; i = next_char)
     {next_char = i->next;
      if (IS_NPC(i))
         continue;

      if (RENTABLE(i) < time(NULL))
         RENTABLE(i) = 0;

       	 	
      if (PLR_FLAGGED(i, PLR_HELLED) &&
         HELL_DURATION(i)            &&
         HELL_DURATION(i) <= time(NULL))
         {restore = PLR_TOG_CHK(i, PLR_HELLED);
          send_to_char("&G??? ????????? ?? ???.&n\r\n",i);
          if ((restore = GET_LOADROOM(i)) == NOWHERE)
             restore = calc_loadroom(i);
          restore = real_room(restore);
          if (restore == NOWHERE)
             {if (GET_LEVEL(i) >= LVL_IMMORT)
                 restore = r_immort_start_room;
              else
                 restore = r_mortal_start_room;
             }
          char_from_room(i);
          char_to_room(i,restore);
          look_at_room(i,restore);
          act("$n ??????$u ? ?????? ???????, ? ????????? ?????????? ????!",FALSE,i,0,0,TO_ROOM);
         }
      if (PLR_FLAGGED(i, PLR_NAMED) &&
          NAME_DURATION(i)            &&
          NAME_DURATION(i) <= time(NULL)
	 )
         {restore = PLR_TOG_CHK(i, PLR_NAMED);
          send_to_char("&G??? ????????? ?? ??????? ?????.&n\r\n",i);
          if ((restore = GET_LOADROOM(i)) == NOWHERE)
             restore = calc_loadroom(i);
          restore = real_room(restore);
          if (restore == NOWHERE)
             {if (GET_LEVEL(i) >= LVL_IMMORT)
                 restore = r_immort_start_room;
              else
                 restore = r_mortal_start_room;
             }
          char_from_room(i);
          char_to_room(i,restore);
          look_at_room(i,restore);
          act("? ???????????? ??????, $v ????????? ?? \"??????? ?????\".",FALSE,i,0,0,TO_ROOM);
         }
      if (PLR_FLAGGED(i, PLR_MUTE) &&
          MUTE_DURATION(i) != 0 && MUTE_DURATION(i) <= time(NULL)
	 )
         {restore = PLR_TOG_CHK(i, PLR_MUTE);
          send_to_char("?? ?????? ?????.\r\n",i);
         }
      if (PLR_FLAGGED(i, PLR_DUMB) &&
          DUMB_DURATION(i) != 0 && DUMB_DURATION(i) <= time(NULL)
	 )
         {restore = PLR_TOG_CHK(i, PLR_DUMB);
          send_to_char("?? ?????? ????????.\r\n",i);
         }
      if (GET_GOD_FLAG(i, GF_GODSLIKE) &&
          GODS_DURATION(i) != 0 && GODS_DURATION(i) <= time(NULL))
         {CLR_GOD_FLAG(i, GF_GODSLIKE);
          send_to_char("?? ????? ?? ??? ??????? ?????.\r\n",i);
          }
       if (GET_GOD_FLAG(i, GF_GODSCURSE) &&
           GODS_DURATION(i) != 0 && GODS_DURATION(i) <= time(NULL))
          {CLR_GOD_FLAG(i, GF_GODSCURSE);
           send_to_char("???? ????? ?? ? ????? ?? ???.\r\n",i);
          }
       if (PLR_FLAGGED(i, PLR_FROZEN) &&
           FREEZE_DURATION(i) != 0 && FREEZE_DURATION(i) <= time(NULL))
          {restore = PLR_TOG_CHK(i, PLR_FROZEN);
           send_to_char("?? ???????.\r\n",i);
          }
	
       if (GET_POS(i) <= POS_STUNNED && GET_HIT(i) < 0)
             continue;
		// Update PC position
        if (GET_POS(i) <= POS_STUNNED)
             update_pos(i);
        // Restore hitpoints
        restore = hit_gain(i);
        restore = interpolate(restore, pulse);
        
              //Update mana for wedmak
        if (IS_WEDMAK(i))
		{  
			 
			// ?????
			if (affected_by_spell(i, SPELL_ZNAK_IRGEN)) {
				  if (GET_MANA(i) > 0 && GET_POS(i) > POS_SITTING)
		              GET_MANA(i) -=2;
	                else
			    { affect_from_char(i, SPELL_ZNAK_IRGEN);
                             send_to_char("?????? ????? &W?????&n ???????.\r\n", i);	
			    }
			}
			 
			// ????
		    if (affected_by_spell(i, SPELL_ZNAK_AKSY))
				  if (GET_MANA(i) > 0 && GET_POS(i) > POS_SITTING)
				      GET_MANA(i) -=3;
	                 else
			    { affect_from_char(i, SPELL_ZNAK_AKSY);
			      send_to_char("?????? ????? &W?????&n ???????.\r\n", i);
			    }
				
			// ?????????
			if (affected_by_spell(i, SPELL_ZNAK_GELIOTROP)) 
			{
				  if (GET_MANA(i) > 0 && GET_POS(i) > POS_SITTING)
				      GET_MANA(i) -=7;
	                else
			   { affect_from_char(i, SPELL_ZNAK_GELIOTROP);
			     send_to_char("?????? ????? &W?????????&n ???????.\r\n", i);
			   }
			}
			   
			   
			// ????
			if (affected_by_spell(i, SPELL_ZNAK_KVEN))
			{
				  if (GET_MANA(i) > 0 && GET_POS(i) > POS_SITTING)
				      GET_MANA(i) -=5;
	                else
			   { affect_from_char(i, SPELL_ZNAK_KVEN);
			     send_to_char("?????? ????? &W????&n ???????.\r\n", i);
			   }
			}
			   
			   
        }
		
            if (restore < 0)
                continue;
            else
            if (GET_HIT(i) < GET_REAL_MAX_HIT(i))
               GET_HIT(i) = MIN(GET_HIT(i) + restore, GET_REAL_MAX_HIT(i));
                                  
		  if (IS_WEDMAK(i) && !FIGHTING(i))
		  { restore = mana_gain(i);
                    restore = interpolate(restore, pulse);
		    GET_MANA(i) = MIN(GET_MANA(i) + restore, GET_REAL_MAX_MANA(i));		  
		  } 
       
       // Restore PC caster mem
        if (GET_MANA_NEED(i)) // && !FIGHTING(i))
        {            
            //restore = mana_gain(i);
            //restore = interpolate(restore, pulse);
            //GET_MANA_STORED(i) = MIN( GET_MANA_STORED(i) + restore, GET_MANA_NEED(i) );

            int mana_stored = i->Memory.calc_mana_stored(i);
            GET_MANA_STORED(i) = MIN( mana_stored, GET_MANA_NEED(i) );
                
            int next_spell = i->Memory.get_next_spell(i);
            if (next_spell != -1)
            {
                GET_SPELL_MEM(i, next_spell) += 1;
                int mana_cost = i->Memory.get_mana_cost(i, next_spell);
                GET_MANA_STORED(i) = MAX(0, GET_MANA_STORED(i)-mana_cost);
                GET_MANA_NEED(i) = MAX(0, GET_MANA_NEED(i)-mana_cost);

                sprintf(buf2,"?? ????? ?????? ??????????????? ??????????? '%s%s%s'!\r\n", CCRED(i,C_NRM), 
                        spell_info[next_spell].name,
                        CCNRM(i,C_NRM));                
                send_to_char(buf2, i);
            }

            if (GET_MANA_STORED(i) == GET_MANA_NEED(i))
            {
                GET_MANA_STORED(i) = 0;
                GET_MANA_NEED(i) = 0;
                CLR_MEMORY(i);
            }

            //affect_total(i); // need ???

            // old mem code
            /*if (GET_MANA_STORED(i) >= GET_MANA_NEED(i))
               {GET_MANA_STORED(i) = 0;
                GET_MANA_NEED(i)   = 0;
                //if (GET_RELIGION(i) == RELIGION_MONO)
                  if ((GET_CLASS(i) == CLASS_CLERIC) || (GET_CLASS(i) == CLASS_SLEDOPYT))  
				     sprintf(buf2,"???? ??????? ????????. ?? ? ??????? ?????? ???? ???????????.\r\n");
                else
                  if((GET_CLASS(i) == CLASS_MAGIC_USER) || (GET_CLASS(i) == CLASS_TAMPLIER) || (GET_CLASS(i) == CLASS_ASSASINE))
				    sprintf(buf2,"???? ??????? ????????. ?? ? ??????? ?????????? ???? ????? ??????????.\r\n");
                else
				  if((GET_CLASS(i) == CLASS_DRUID) || (GET_CLASS(i) == CLASS_MONAH))
					sprintf(buf2,"???? ??????? ????????. ?? ? ??????? ???????? ???? ????????.\r\n");
				 
				  send_to_char(buf2,i);
                
                  for (restore = 1; restore <= MAX_SPELLS; restore++)
                    {if (i->Memory[restore])
                        GET_SPELL_MEM(i,restore) += i->Memory[restore];
                     i->Memory[restore] = 0;
                     }
                 //log("[BEAT_POINT_UPDATE->AFFECT_TOTAL] Start");
                 affect_total(i);
                 //log("[BEAT_POINT_UPDATE->AFFECT_TOTAL] Stop");
                 CLR_MEMORY(i);

                 // if (GET_RELIGION(i) == RELIGION_MONO)
                 //   sprintf(buf2,"??????? ???????, $n ? ??????? ?????????$y ????? ??????????.");
                 //else
                 //  sprintf(buf2,"??????? ???????, $n ? ??????? ?????$y ????? ??????????.");

                 if ((GET_CLASS(i) == CLASS_CLERIC) || (GET_CLASS(i) == CLASS_SLEDOPYT))  
				     sprintf(buf2,"???????? ???????, $n ? ??????? ?????$y ???? ??????????.");
                else
                  if((GET_CLASS(i) == CLASS_MAGIC_USER) || (GET_CLASS(i) == CLASS_TAMPLIER) || (GET_CLASS(i) == CLASS_ASSASINE))
				    sprintf(buf2,"???????? ???????, $n ? ??????? ?????????$y ???? ????? ??????????.");
                else
				  if((GET_CLASS(i) == CLASS_DRUID) || (GET_CLASS(i) == CLASS_MONAH))
					sprintf(buf2,"???????? ???????, $n ? ??????? ???????$y ???? ????????.");

				 act(buf2,FALSE,i,0,0,TO_ROOM);
                }*/

            } // END of Restore PC caster mem


        // Restore moves
        restore = move_gain(i);
        restore = interpolate(restore, pulse);
        GET_MOVE(i) = MIN(GET_MOVE(i) + restore, GET_REAL_MAX_MOVE(i));
     }
}

void set_ptitle(struct char_data * ch, char *ptitle)
{
   if (strlen(ptitle) > MAX_TITLE_LENGTH)
    ptitle[MAX_TITLE_LENGTH] = '\0';


   if (GET_PTITLE(ch) != NULL)
   { free(GET_PTITLE(ch));
     GET_PTITLE(ch) = NULL;     
   }
   
    if (ptitle && *ptitle)
   GET_PTITLE(ch) = str_dup(ptitle);  


}


void set_title(struct char_data * ch, char *title)
{
  
	if (strlen(title) > MAX_TITLE_LENGTH)
            title[MAX_TITLE_LENGTH] = '\0';

        if (GET_TITLE(ch) != NULL)
	  { free(GET_TITLE(ch));
            GET_TITLE(ch) = NULL;    
	  }

        if (title && *title)
          GET_TITLE(ch) = str_dup(title);
 
}


void check_autowiz(struct char_data * ch)
{
char buf[128];
    if (use_autowiz && GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "autowiz %d %s %d %s", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE);
/*#endif /* CIRCLE_WINDOWS */

    mudlog("Initiating autowiz.", CMP, LVL_IMMORT, FALSE);
    system(buf);
    reboot_wizlists();
  }

}


#define MAX_REMORT 15
void gain_exp(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;
  char buf[128];

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
      return;

  if (PLR_FLAGGED(ch, PLR_NOEXP) && gain>=0 )
      return;
	 
  if (IS_NPC(ch)) { GET_EXP(ch) += gain/3; return; }

  if (gain > 0)
  {
      gain = MIN(max_exp_gain_pc(ch), gain);
      GET_EXP(ch) += gain;

      // ???? ?? ???? ???? ???????? ?????? ??????? - ?????????
      //GET_EXP(ch) = MIN(GET_EXP(ch), level_exp(GET_CLASS(ch),LVL_REMORT + GET_REMORT(ch)) - 1);
            
      // ???? ????? ??? ???? ???????? ????? ?? ??????, ?? ????? ????
      if (!GET_GOD_FLAG(ch, GF_REMORT))      
      {
          while (GET_LEVEL(ch) < (LVL_REMORT + GET_REMORT(ch) - 1) &&
                 GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1))
          {
              GET_LEVEL(ch) += 1;
              num_levels++;
              sprintf(buf,"&G???????????, ?? ???????? ???????!&n\r\n");
              send_to_char(buf,ch);
    	    
              if (GET_LEVEL(ch)==16)
	             send_to_char("&W?? ???????? ????? ?????? ?????!&n\r\n", ch);
		      if (GET_LEVEL(ch)==24)
	             send_to_char("&W?? ???????? ????? ?????? ?????????!&n\r\n", ch);           
              advance_level(ch);
              is_altered = TRUE;
          }

          if (GET_EXP(ch) >= level_exp(GET_CLASS(ch), LVL_REMORT + GET_REMORT(ch)))
          {
              if (GET_REMORT(ch) < MAX_REMORT )
              {
                  sprintf(buf,"%s&M?? ???????? ????? ?? ??????????????!&n%s\r\n", CCGRN(ch,C_NRM), CCNRM(ch,C_NRM));
                  send_to_char(buf,ch);
                  SET_GOD_FLAG(ch, GF_REMORT);
              }
          }

          if (is_altered)
          {
              sprintf(buf, "%s advanced %d level%s to level %d.",
		              GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s",
		              GET_LEVEL(ch));
              mudlog(buf, BRF, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE);
              check_autowiz(ch);
          }
      }      
  }
  else if (gain < 0)
  {
      gain = MAX(-max_exp_loss_pc(ch), gain); /* Cap max exp lost per death */
      GET_EXP(ch) += gain;
      if (GET_EXP(ch) < 0) GET_EXP(ch) = 0;
                              
      while (GET_LEVEL(ch) > 1 &&
             (GET_EXP(ch)+ GET_EXP(ch)/3) < level_exp(GET_CLASS(ch), GET_LEVEL(ch)))
      {
          GET_LEVEL(ch) -= 1;
          num_levels++;
          sprintf(buf,"%s?? ???????? ???????!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
          send_to_char(buf,ch);
          decrease_level(ch);
          is_altered = TRUE;
      }

      if ( GET_GOD_FLAG(ch, GF_REMORT) && GET_EXP(ch) < level_exp(GET_CLASS(ch), LVL_REMORT + GET_REMORT(ch)) )
      {
          sprintf(buf,"%s?? ???????? ????? ?? ??????????????!%s\r\n", CCGRN(ch,C_NRM),CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
          CLR_GOD_FLAG(ch, GF_REMORT);
      }

      if (is_altered)
      {
          sprintf(buf, "%s decreases %d level%s to level %d.",
		          GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s",
		          GET_LEVEL(ch));
          mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
          check_autowiz(ch);			
      }
      /*if (GET_LEVEL(ch) == 1 && ch->player_specials->saved.spare12 > 10)
	  { if (ch->points.max_hit > 2)
	         ch->points.max_hit --;
	  }*/
  }
}

void gain_exp_regardless(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;
  struct char_file_u;
  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    if (gain > 0)
	{  while (GET_LEVEL(ch) < LVL_IMPL &&
	   GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
		}

    if (is_altered) {
      sprintf(buf, "%s ???????? ?? %d ??????%s ? ?????? ????? ??????? - %d.",
		GET_NAME(ch), num_levels, num_levels == 1 ? "?" : "??",
		GET_LEVEL(ch));
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      if (num_levels == 1)
        send_to_char("&G?? ????????? ?? ???????!&n\r\n", ch);
      else {
	sprintf(buf, "?? ????????? ?? %d %s!\r\n", num_levels, desc_count(num_levels, WHAT_LEVEL));
	send_to_char(buf, ch);
			}
	  /*    set_title(ch, NULL);*/
      check_autowiz(ch);
		}
	}
else
      if (gain < 0)
         {gain = MAX(-max_exp_loss_pc(ch), gain);
          GET_EXP(ch) += gain;
          if (GET_EXP(ch) < 0)
              GET_EXP(ch) = 0;
          while (GET_LEVEL(ch) > 1 &&
                 GET_EXP(ch)   < level_exp(GET_CLASS(ch), GET_LEVEL(ch)))
                {GET_LEVEL(ch) -= 1;
                 num_levels++;
                 sprintf(buf,"%s?? ???????? ???????.%s\r\n",
                             CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
                 send_to_char(buf,ch);
                 decrease_level(ch);
                 is_altered = TRUE;
                }
          if (is_altered)
             {sprintf(buf, "%s decreases %d level%s to level %d.",
		                   GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s",
		                   GET_LEVEL(ch));
              mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
              check_autowiz(ch);
             }
         }
	// ??????????? ? ????????!!!
/*	if(!ch->player.unbonus && GET_LEVEL(ch) == 16)
		   { ch->player.unbonus =1;
			 ch->player.bonus++;
			 sprintf(buf, "%s??? ??????????? ????????????? ???????!%s\r\n",CCGRN(ch,CMP), CCNRM(ch,CMP));
			 send_to_char(buf, ch);
		   }
			if(ch->player.unbonus == 1 && GET_LEVEL(ch) ==24)
			{ ch->player.unbonus =2;
			  ch->player.bonus++;
			  sprintf(buf, "%s??? ??????????? ????????????? ???????!%s\r\n",CCGRN(ch,CMP), CCNRM(ch,CMP));
			  send_to_char(buf, ch);
			}      
*/

	}
 }


void gain_condition(struct char_data * ch, int condition, int value)
{
  bool intoxicated;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case FULL:
    send_to_char("?? ?????? ????.\r\n", ch); 
    return;
  case THIRST:
    send_to_char("?? ?????? ????.\r\n", ch); 
    return;
  case DRUNK:
    if (AFF_FLAGGED(ch,AFF_ABSTINENT) || AFF_FLAGGED(ch,AFF_DRUNKED))
       GET_COND(ch,DRUNK) = MAX(CHAR_DRUNKED,GET_COND(ch,DRUNK));
    if (intoxicated && GET_COND(ch,DRUNK) < CHAR_DRUNKED)
       send_to_char("?? ???????????.\r\n", ch);
       GET_DRUNK_STATE(ch) = MAX(GET_DRUNK_STATE(ch), GET_COND(ch,DRUNK));
	  return;
  default:
    break;
  }

}

void dt_activity(void)
{ int    i;
  struct char_data *ch, *next;
  char   msg[MAX_STRING_LENGTH];

  for (i = FIRST_ROOM; i <= top_of_world; i++)
      {if (ROOM_FLAGGED(i, ROOM_SLOWDEATH) || ROOM_FLAGGED(i, ROOM_ICEDEATH))
          for (ch = world[i].people; ch; ch = next)
              {next = ch->next_in_room;
               if (!IS_NPC(ch))
                  {sprintf(msg,"Player %s died in slow DT (room %d)",GET_NAME(ch),GET_ROOM_VNUM(i));
                   if (damage(ch,ch,MAX(1,GET_REAL_MAX_HIT(ch) >> 2),TYPE_ROOMDEATH,FALSE) < 0)
                      log(msg);
                  }
              }
       }
}

void check_idling(struct char_data * ch)
{
  if (!RENTABLE(ch)) {
	if (++(ch->char_specials.timer) > idle_void) {
    if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE) {
      GET_WAS_IN(ch) = ch->in_room;
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch),FALSE );
	stop_fighting(ch, TRUE);
      }
      act("$n ???????? ?????????$u ? ???????.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("?? ???????? ???????????? ? ???????.\r\n", ch);
      save_char(ch, NOWHERE);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, STRANGE_ROOM);
    } else if (ch->char_specials.timer > idle_rent_time) {
      if (ch->in_room != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, STRANGE_ROOM);
      if (ch->desc) {
	STATE(ch->desc) = CON_DISCONNECT;
	/*
	 * For the 'if (d->character)' test in close_socket().
	 * -gg 3/1/98 (Happy anniversary.)
	 */
	ch->desc->character = NULL;
	ch->desc = NULL;
      }
      if (free_rent)
	Crash_rentsave(ch, 0, 0);
      else
	Crash_idlesave(ch);

      sprintf(buf, "%s ???????? ????????????? ???????? ? ?????? (??????????).", GET_NAME(ch));
      mudlog(buf, CMP, LVL_GOD, TRUE);
      extract_char(ch, FALSE);
			}
		}
	}
}

void hour_update(void)
{ DESCRIPTOR_DATA *i;

  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) != CON_PLAYING || i->character == NULL ||
           PLR_FLAGGED(i->character, PLR_WRITING))
          continue;
        SEND_TO_Q("&c?????? ???.&n", i);
      }
}



// Update PCs, NPCs, and objects 
void point_update(void)
{ memory_rec *mem,*nmem,*pmem;
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;
  struct track_data *track, *next_track, *temp;
 // int restore, mana, count=0;
 int    count, mob_num, spellnum, mana, restore, cont = 0;
 char   buffer_mem[MAX_SPELLS+1], real_spell[MAX_SPELLS+1];
 void pk_temp_timer(struct char_data *ch);
  for (count = 0; count <= MAX_SPELLS; count++)
      {buffer_mem[count] = count; real_spell[count] = 0;}
  for (spellnum = MAX_SPELLS; spellnum > 0; spellnum--)
      {count                           = number(1,spellnum);
       real_spell[MAX_SPELLS-spellnum] = buffer_mem[count];
       for (;count < MAX_SPELLS; buffer_mem[count] = buffer_mem[count+1], count++);
      }

 // characters 


 for (i = character_list; i; i = next_char) {
    next_char = i->next;

            
	
	if (AFF_FLAGGED(i, AFF_SLEEP) && GET_POS(i) > POS_SLEEPING)
		{ GET_POS(i) = POS_SLEEPING;
		  send_to_char("????? ??????, ? ??? ?? ??????????!\r\n", i);
		  act("$n ????????????? ???????$u ? ?????? ????????$y, ???????????? ?? ?????? ???.", TRUE, i, 0, 0, TO_ROOM);
		}
	
				if (!IS_NPC(i))
				{   gain_condition(i, FULL, -1);
    				gain_condition(i, DRUNK, -1);
    				gain_condition(i, THIRST, -1);
                                quest_update(i);

                   if (GET_USTALOST(i))
	               ustalost_timer(i, -1); // ?????????
                     pk_temp_timer(i);
				
			     
	if (PLR_FLAGGED(i, PLR_KILLER) && !(ROOM_FLAGGED(IN_ROOM(i), ROOM_ARENA) ||
		ROOM_FLAGGED(IN_ROOM(i), ROOM_PEACEFUL)) && GetClanzone(i))       
	  {   GET_TIME_KILL(i) --;
		
			if (GET_TIME_KILL(i) <=0 )
				{ GET_TIME_KILL(i) = 0;
			      REMOVE_BIT(PLR_FLAGS(i, PLR_KILLER), PLR_KILLER);
				  send_to_char("???? ?????? ??????.\r\n",i);
				}
      }
	}

			if (GET_POS(i) >= POS_STUNNED)
			{ count = hit_gain(i);
				  if (count < 0)
					{ if (damage(i, i, -count, SPELL_POISON, FALSE) < 0)
						continue;
					} 
				  else
					if (IS_NPC(i) && GET_HIT(i) < GET_REAL_MAX_HIT(i))
					GET_HIT(i) = MIN(GET_HIT(i) + count, GET_REAL_MAX_HIT(i));
		                 else
					if (!IS_NPC(i) && GET_POS(i) == POS_STUNNED)
					{    GET_HIT(i) ++;
					    continue;
					}
	 
 // Restore mobs
          if (IS_NPC(i) && !FIGHTING(i))
             {// Restore horse
              if (IS_HORSE(i))
                 {switch(real_sector(IN_ROOM(i)))
                  {case SECT_FLYING:
                   case SECT_UNDERWATER:
                   case SECT_SECRET:
                   case SECT_WATER_SWIM:
                   case SECT_WATER_NOSWIM:
                   case SECT_THICK_ICE:
                   case SECT_NORMAL_ICE:
                   case SECT_THIN_ICE:
                        mana = 0;
                        break;
                  case SECT_CITY:
                        mana = 20;
                        break;
                  case SECT_FIELD:
                  case SECT_FIELD_RAIN:
                        mana = 100;
                        break;
                  case SECT_FIELD_SNOW:
                        mana = 40;
                        break;
                  case SECT_FOREST:
                  case SECT_FOREST_RAIN:
                        mana = 80;
                        break;
                  case SECT_FOREST_SNOW:
                        mana = 30;
                        break;
                  case SECT_HILLS:
                  case SECT_HILLS_RAIN:
                        mana = 70;
                        break;
                  case SECT_HILLS_SNOW:
                        mana = 25;
                        break;
                  case SECT_MOUNTAIN:
                        mana = 25;
                        break;
                  case SECT_MOUNTAIN_SNOW:
                        mana = 10;
                        break;
                  default:
                        mana = 10;
                  }
                  if (on_horse(i->master))
                     mana /= 2;
                  GET_HORSESTATE(i) = MIN(200, GET_HORSESTATE(i) + mana);
                }    
		   // Forget PC's
                for (mem = MEMORY(i),pmem=NULL; mem; mem=nmem)
                  {nmem = mem->next;
                   if (mem->time <= 0 && FIGHTING(i))
                     { pmem = mem;//??????? - ????? ?? ????? ???????. ??????????.
					   continue;
					 }
					 if (mem->time < time(NULL) || mem->time <= 0)
                      {if (pmem)
                          pmem->next = nmem;
                       else
                          MEMORY(i)  = nmem;
                       free(mem);
                      }
                   else
                      pmem = mem;
                  }
                
			  // Remember some spells ??? ???? ????????? ?????, ??? ?????? ??? ???. ??? ?? ?? ??????? ??????
              if ((mob_num = GET_MOB_RNUM(i)) >= 0)
                 for (count = 0, mana = 0;
                      count < MAX_SPELLS && mana < GET_REAL_INT(i) * 10; count ++)
					 { spellnum = real_spell[count];
                      if (GET_SPELL_MEM(mob_proto+mob_num, spellnum) > GET_SPELL_MEM(i, spellnum))
					  { GET_SPELL_MEM(i, spellnum)++;
                        mana += ((spell_info[spellnum].mana_max + spell_info[spellnum].mana_min) / 2);
                        GET_CASTER(i) += (IS_SET(spell_info[spellnum].routines,NPC_CALCULATE) ? 1 : 0);
			 //  sprintf(buf,"Remember spell %s for mob %s.\r\n",spell_info[spellnum].name,GET_NAME(i));
			 //  send_to_gods(buf);
                       }
                     }
			} //end Restore mobs
	
	    // Restore moves
          if (IS_NPC(i) || !UPDATE_PC_ON_BEAT)
			  if (GET_MOVE(i) < GET_REAL_MAX_MOVE(i))
				GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_REAL_MAX_MOVE(i));

		  
	// Update PC/NPC position
          if (GET_POS(i) <= POS_STUNNED)
             update_pos(i);
      }  //end poss stuned
	  
     
	  	else
		  if (GET_POS(i) == POS_INCAP) 
			{       GET_HIT(i)++;
				update_pos(i); 
			//	if (damage(i, i, 1, TYPE_SUFFERING,FALSE) == -1)
							continue;
			}
         	else
			if (GET_POS(i) == POS_MORTALLYW)
			{   if (damage(i, i, 1, TYPE_SUFFERING, FALSE) == -1)
							continue;
			}
        	

      update_char_objects(i);
      if (!IS_NPC(i) && GET_LEVEL(i) < idle_max_level)
         check_idling(i);
     }	  
  

 
// rooms 
  for (count = FIRST_ROOM; count <= top_of_world; count++)
      {if (world[count].fires)
          {switch (get_room_sky(count))
           { case SKY_CLOUDY:
             case SKY_CLOUDLESS:
                  mana = number(1,2);
                  break;
             case SKY_RAINING:
                  mana = 2;
                  break;
             default:
                  mana = 1;
           }
           world[count].fires -= MIN(mana, world[count].fires);
           if (world[count].fires <= 0)
              {act("?????? ?????? ?????.",FALSE,world[count].people,0,0,TO_ROOM);
               act("?????? ?????? ?????.",FALSE,world[count].people,0,0,TO_CHAR);
               world[count].fires = 0;
              }
          }
       if (world[count].forbidden)
          world[count].forbidden--;
       if (world[count].portal_time) 
	   { world[count].portal_time--;
			if (world[count].portal_time <= 0)
            {act("?????????? ?????? ???????? ? ??????????? ? ????????????.",FALSE,world[count].people,0,0,TO_ROOM);
             act("?????????? ?????? ???????? ? ??????????? ? ????????????.",FALSE,world[count].people,0,0,TO_CHAR);
            }  
		}
       if (world[count].ices)
          if (!--world[count].ices)
		  REMOVE_BIT(ROOM_FLAGS(count, ROOM_ICEDEATH), ROOM_ICEDEATH);
	    
       world[count].glight = MAX(0, world[count].glight);
       world[count].gdark  = MAX(0, world[count].gdark);

       for (track = world[count].track, temp = NULL; track; track = next_track)
           {next_track = track->next;
            switch (real_sector(count))
            {case SECT_FLYING:
             case SECT_UNDERWATER:
             case SECT_SECRET:
             case SECT_WATER_SWIM:
             case SECT_WATER_NOSWIM:
                       spellnum = 31;
                       break;
             case SECT_THICK_ICE:
             case SECT_NORMAL_ICE:
             case SECT_THIN_ICE:
                        spellnum = 16;
                        break;
             case SECT_CITY:
                        spellnum = 4;
                        break;
             case SECT_FIELD:
             case SECT_FIELD_RAIN:
                        spellnum = 2;
                        break;
             case SECT_FIELD_SNOW:
                        spellnum = 1;
                        break;
             case SECT_FOREST:
             case SECT_FOREST_RAIN:
                        spellnum = 2;
                        break;
             case SECT_FOREST_SNOW:
                        spellnum = 1;
                        break;
             case SECT_HILLS:
             case SECT_HILLS_RAIN:
                        spellnum = 3;
                        break;
             case SECT_HILLS_SNOW:
                        spellnum = 1;
                        break;
             case SECT_MOUNTAIN:
                        spellnum = 4;
                        break;
             case SECT_MOUNTAIN_SNOW:
                        spellnum = 1;
                        break;
             default:
                        spellnum = 2;
            }

            for (mana = 0, restore = FALSE; mana < NUM_OF_DIRS; mana++)
                {if ((track->time_income[mana]  <<= spellnum))
		    restore = TRUE;
		 if ((track->time_outgone[mana] <<= spellnum))
		    restore = TRUE;
                }
            if (!restore)
               {if (temp)
	           temp->next = next_track;
		else
		   world[count].track = next_track;
		free(track);
               }
	    else
	       temp = track;
           }
      }

  
  // ???????? 
  for (j = object_list; j; j = next_thing)
  {   next_thing = j->next;	// ????????? ??????? ?? ??????
    // If this is a corpse 

    if (IS_CORPSE(j))
	{ // timer count down
      if (GET_OBJ_TIMER(j) > 0)
	      GET_OBJ_TIMER(j)--;
      if (GET_OBJ_TIMER(j) <= 0)
	  { for (jj = j->contains; jj; jj = next_thing2)
		{ next_thing2 = jj->next_content;	// Next in inventory 
				obj_from_obj(jj);
			if (j->in_obj)
				obj_to_obj(jj, j->in_obj);
			else if (j->carried_by)
				obj_to_char(jj, j->carried_by);
			else if (j->in_room != NOWHERE)
				obj_to_room(jj, j->in_room);
			else { 	log("SYSERR: extract %s from %s to NOTHING !!!",jj->short_description,j->short_description);
		  		extract_obj(jj);
		 }
		}
	  	if (j->carried_by)
			{ act("$3 ??????$U ? ????? ?????.", FALSE, j->carried_by, j, 0, TO_CHAR);
			  obj_from_char(j);
			}
		else
	  	if ((j->in_room != NOWHERE))
	   	{ if (world[j->in_room].people)
			{ act("?????????? ??? ??? ?????? ?? ??????? ?? $1.",
				TRUE, world[j->in_room].people, j, 0, TO_ROOM);
			 act("?????????? ??? ??? ?????? ?? ??????? ?? $1.",
				TRUE, world[j->in_room].people, j, 0, TO_CHAR);
			}
	    obj_from_room(j);//?????????? ?? ?????? ????? ??? ???????? ???????????, ????? ??? ????
	  	}					//??????? ???????? ??????????. ?????????, ?????? ???? ? ????????? ? ??????????? ??? ?????.
	  	else
		  if (j->in_obj)
		   obj_from_obj(j);
	           extract_obj(j);
	  }
  }
 /* If the timer is set, count it down and at 0, try the trigger */
    /* note to .rej hand-patchers: make this last in your point-update() */
       else
          {if (SCRIPT_CHECK(j, OTRIG_TIMER))
              {if (GET_OBJ_TIMER(j) > 0 && OBJ_FLAGGED(j,ITEM_TICKTIMER))

			     GET_OBJ_TIMER(j)--;
               if (!GET_OBJ_TIMER(j))
                  {timer_otrigger(j);
                   j = NULL;
                  }
              }
           else // ???????????? ???????? ?? ??????? ?? ?????, ?? ??????????? ???? ???? ???????????? ??? ?????. 
           if (GET_OBJ_DESTROY(j) > 0 && !NO_DESTROY(j) && !ROOM_FLAGGED(j->in_room, ROOM_HOUSE))
              GET_OBJ_DESTROY(j)--;

            if ((j->in_room != NOWHERE) &&
	              GET_OBJ_TIMER(j) > 0 && !NO_DESTROY(j))
 	              GET_OBJ_TIMER(j)--;
           
           if (j &&
               ((OBJ_FLAGGED(j, ITEM_ZONEDECAY)						&&
                 GET_OBJ_ZONE(j) != NOWHERE							&&
                 up_obj_where(j) != NOWHERE							&&
                 GET_OBJ_ZONE(j) != world[up_obj_where(j)].zone)	||
				(GET_OBJ_TIMER(j)   <= 0 && !NO_TIMER(j)   )		||
                (GET_OBJ_DESTROY(j) == 0 && !NO_DESTROY(j) )))
              {//**** ?????????? ???????
               for (jj = j->contains; jj; jj = next_thing2)
                   {next_thing2 = jj->next_content;
                    obj_from_obj(jj);
                    if (j->in_obj)
                       obj_to_obj(jj, j->in_obj);
                    else if (j->worn_by)
                       obj_to_char(jj, j->worn_by);
                    else if (j->carried_by)
                       obj_to_char(jj, j->carried_by);
                    else if (j->in_room != NOWHERE)
                       obj_to_room(jj, j->in_room);
                    else {
			  log("SYSERR: extract %s from %s to NOTHING !!!",jj->short_description,j->short_description);
                        // core_dump();
                        extract_obj(jj);
			  }
		  }

               if (j->worn_by)
                  {switch (j->worn_on)
                   {case WEAR_LIGHT :
                    case WEAR_SHIELD :
                    case WEAR_WIELD :
                    case WEAR_HOLD :
                    case WEAR_BOTHS :
                         act("$o ????????$U ? ????? ?????, ????????????? ? ????.",FALSE,j->worn_by,j,0,TO_CHAR);
                         break;
                    default :
                         act("$o ????????$U ?? ????? ????...",FALSE,j->worn_by,j,0,TO_CHAR);
                         break;
                   }
		   unequip_char(j->worn_by,j->worn_on);
                  }
               else   if (j->carried_by)
			{ act("$o ????????$U ? ????? ?????, ????????????? ? ????.",FALSE,j->carried_by,j,0,TO_CHAR);
			  obj_from_char(j);
			}
		  else
		     if (j->in_room != NOWHERE)
	              {if (world[j->in_room].people)
			{act("$o ?????????$U ? ????, ??????? ??????? ?????.",
                           FALSE, world[j->in_room].people, j, 0, TO_CHAR);
			 act("$o ?????????$U ? ????, ??????? ??????? ?????.",
                           FALSE, world[j->in_room].people, j, 0, TO_ROOM);
			}
                         obj_from_room(j);
                      }
	       		else
			   if (j->in_obj)
                  	     obj_from_obj(j);
      		          extract_obj(j);
			}
		   else
            // decay poision && other affects 
            for (count = 0; count < MAX_OBJ_AFFECT; count++)
                if (j->affected[count].location == APPLY_POISON)
                   {j->affected[count].modifier--;
                    if (j->affected[count].modifier <= 0)
                       {j->affected[count].location = APPLY_NONE;
                        j->affected[count].modifier = 0;
                       }
                   }
          }
       }
 // ???????, ????????, ? ????????? ???????.
  for (j = object_list; j; j = next_thing)
  { next_thing = j->next;	// ????????? ?????? ?? ??????
    if (j->contains)
	{
     cont = TRUE;
    } else
	{
     cont = FALSE;
    }
    if (obj_decay(j)) {
     if (cont) {
      next_thing = object_list;
     }
    }
  }

  exchange_tick_timer_objects();
}

void set_afk(struct char_data * ch, char *afk_message, int result)
{
  if (!*afk_message)
    sprintf(buf1, "%s", AFK_DEFAULT);
  else
    sprintf(buf1, "%s", afk_message);

  if (strlen(buf1) > MAX_POOF_LENGTH) {
    send_to_char("????????? ??? ??????? ???????, ???????? ??? ??????.\r\n", ch);
    sprintf(buf1, "%s", AFK_DEFAULT);
  }

  if (GET_AFK(ch))
    free(GET_AFK(ch));
  GET_AFK(ch) = NULL;
  GET_AFK(ch) = str_dup(buf1);

  if (result) {
     sprintf(buf, "?? ???? ? ????? AFK:\\c02 %s\\c00", CAP(GET_AFK(ch)));
		act(buf, FALSE, ch, 0, 0, TO_CHAR);
  } else {
		act("$n ??????$u ?? AFK!", FALSE, ch, 0, 0, TO_ROOM);
		act("?? ????????? ?? AFK!", FALSE, ch, 0, 0, TO_CHAR);
		}
}

void ItemDecayZrepop(zone_rnum zone)
{ struct obj_data  *obj, *obj_next;
  bool cont = FALSE;
	for (obj = object_list; obj; obj = obj_next)
		{ obj_next = obj->next;
	    
	         if (obj->contains)
			     cont = TRUE;
		     else
			     cont = FALSE;
		
	      if ((int)GET_OBJ_VNUM(obj)/100 == zone_table[zone].number && OBJ_FLAGGED(obj, ITEM_ZONERESET))
		  	{  
			  if (obj->worn_by)
				{ act("$o ????????$U ?? ?????? ? ????.", FALSE,
				  obj->worn_by, obj, 0, TO_ROOM);
				  act("$o ????????$U ? ????? ?????.", FALSE,
				  obj->worn_by, obj, 0, TO_CHAR);
               	}
		      else
				if(obj->carried_by)
				  act("$o ????????$U ? ????? ?????.", FALSE,
				  obj->carried_by, obj, 0, TO_CHAR);
		      else 
			    if (obj->in_room != NOWHERE)
				{ if (world[obj->in_room].people)
					{ act ("$o ????????$U, ????? ??? ?????...",
		              FALSE, world[obj->in_room].people, obj, 0, TO_CHAR);
		              act ("$o ????????$U, ????? ??? ?????...",
		              FALSE, world[obj->in_room].people, obj, 0, TO_ROOM);
					}
				}
			extract_obj(obj);

				if (cont)
					obj_next = object_list;
			}
		}
}

