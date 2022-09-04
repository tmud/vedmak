/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
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
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "dg_scripts.h"
#include "constants.h"
#include "screen.h"
#include "clan.h"

// external vars 
extern struct gods_celebrate_apply_type  *Poly_apply;
extern struct gods_celebrate_apply_type  *Mono_apply;
extern int    supress_godsapply;
extern struct room_data *world;
extern struct obj_data *object_list;
extern CHAR_DATA *character_list;
extern struct obj_data *obj_proto;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern CHAR_DATA *mob_proto;
extern struct descriptor_data *descriptor_list;
extern const char *MENU;

// local functions 

int apply_ac(CHAR_DATA * ch, int eq_pos);
void update_object(struct obj_data * obj, int use);
void update_char_objects(CHAR_DATA * ch);
void do_entergame(struct descriptor_data *d);
int  invalid_unique(CHAR_DATA *ch, struct obj_data *obj);
int apply_armour(CHAR_DATA * ch, int eq_pos);


// Локальные константы
const int money_destroy_timer  = 60;
const int death_destroy_timer  = 5;
const int room_destroy_timer   = 10;
const int room_nodestroy_timer = -1;
const int script_destroy_timer = 1;

// external functions 
int  extra_damroll(int class_num, int level);
int  invalid_no_class(CHAR_DATA *ch, struct obj_data *obj);
int  invalid_anti_class(CHAR_DATA *ch, struct obj_data *obj);
void remove_follower(CHAR_DATA * ch);
void clearMemory(CHAR_DATA * ch);
void die_follower(CHAR_DATA * ch);
int  delete_char(char *name);
void free_script(struct script_data *sc);

ACMD(do_return);

char *fname(const char *namelist)
{
  static char holder[30];
  register char *point;

  for (point = holder; is_alpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}


int char_saved_aff[] =
{AFF_GROUP,
 AFF_HORSE,
 0
};

int isname(const char *str, const char *namelist)
{ int   once_ok = FALSE;
  const char *curname, *curstr, *laststr;

  if (!namelist || !*namelist)
     return (FALSE);

  curname = namelist;
  curstr  = laststr = str;
  for (;;)
      {once_ok = FALSE;
       for (;;curstr++, curname++)
           {if (!*curstr)
	       return (once_ok);
	    if (curstr != laststr && *curstr == '!')
	       if (is_alpha(*curname))
	          {curstr = laststr;
	           break;
	          }
	    if (!is_alpha(*curstr))
	       {for(; !is_alpha(*curstr); curstr++)
	           {if (!*curstr)
	               return (once_ok);
                   }
                laststr = curstr;
  	        break;
	       }
            if (!*curname)
  	       return (FALSE);
            if (!is_alpha(*curname))
  	       {curstr = laststr;
	        break;
	       }
            if (LOWER(*curstr) != LOWER(*curname))
	       {curstr = laststr;
	        break;
	       }
	    else
	       once_ok = TRUE;
           }
     // skip to next name
     for (; is_alpha(*curname); curname++);
     for (; !is_alpha(*curname); curname++)
         {if (!*curname)
             return (FALSE);
         }
     }
}

void set_quested(CHAR_DATA *ch, int quest)
{ int i;
  if (IS_NPC(ch) || IS_IMMORTAL(ch))
     return;
  if (ch->Questing.quests)
     {for (i = 0; i < ch->Questing.count; i++)
          if (*(ch->Questing.quests+i) == quest)
             return;
      if (!(ch->Questing.count % 10L))
         RECREATE(ch->Questing.quests, int, (ch->Questing.count / 10L + 1) * 10L);
     }
  else
     {ch->Questing.count  = 0;
      CREATE(ch->Questing.quests, int, 10);
     }
  *(ch->Questing.quests+ch->Questing.count++) = quest;
}

int get_quested(CHAR_DATA *ch, int quest)
{ int i;
  if (IS_NPC(ch) || IS_IMMORTAL(ch))
     return (FALSE);
  if (ch->Questing.quests)
     {for (i = 0; i < ch->Questing.count; i++)
          if (*(ch->Questing.quests+i) == quest)
             return (TRUE);
     }
  return (FALSE);
}

void check_light(CHAR_DATA *ch, int was_equip, int was_single, int was_holylight, int was_holydark, int koef)
{ int light_equip = FALSE;

  if (IN_ROOM(ch) == NOWHERE)
     return;

  //if (IS_IMMORTAL(ch))
  //   {sprintf(buf,"%d %d %d (%d)\r\n",world[IN_ROOM(ch)].light,world[IN_ROOM(ch)].glight,world[IN_ROOM(ch)].gdark,koef);
  //    send_to_char(buf,ch);
  //   }

  if (GET_EQ(ch, WEAR_LIGHT))
     {if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
         {if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 0)) // изменяю с двойки на ноль
             {//send_to_char("Light OK!\r\n",ch);
              light_equip = TRUE;
	     }
         }
     }
  // In equipment
  if (light_equip)
     {if (was_equip == LIGHT_NO)
         world[ch->in_room].light = MAX(0,world[ch->in_room].light + koef);
     }
  else
     {if (was_equip == LIGHT_YES)
         world[ch->in_room].light = MAX(0,world[ch->in_room].light - koef);
     }
  // Singleligt affect
  if (AFF_FLAGGED(ch, AFF_SINGLELIGHT))
     {if (was_single == LIGHT_NO)
         world[ch->in_room].light = MAX(0,world[ch->in_room].light + koef);
     }
  else
     {if (was_single == LIGHT_YES)
         world[ch->in_room].light = MAX(0,world[ch->in_room].light - koef);
     }
  // Holyligh affect
  if (AFF_FLAGGED(ch, AFF_HOLYLIGHT))
     {if (was_holylight == LIGHT_NO)
         world[ch->in_room].glight = MAX(0,world[ch->in_room].glight + koef);
     }
  else
     {if (was_holylight == LIGHT_YES)
         world[ch->in_room].glight = MAX(0,world[ch->in_room].glight - koef);
     }
  // Holydark affect
  // if (IS_IMMORTAL(ch))
  //    {sprintf(buf,"holydark was %d\r\n",was_holydark);
  //     send_to_char(buf,ch);
  //    }
  if (AFF_FLAGGED(ch, AFF_HOLYDARK))
     {// if (IS_IMMORTAL(ch))
      //    send_to_char("holydark on\r\n",ch);
      if (was_holydark == LIGHT_NO)
         world[ch->in_room].gdark = MAX(0,world[ch->in_room].gdark + koef);
     }
  else
     {// if (IS_IMMORTAL(ch))
      //   send_to_char("HOLYDARK OFF\r\n",ch);
      if (was_holydark == LIGHT_YES)
         world[ch->in_room].gdark = MAX(0,world[ch->in_room].gdark - koef);
     }
  //if (IS_IMMORTAL(ch))
  //   {sprintf(buf,"%d %d %d (%d)\r\n",world[IN_ROOM(ch)].light,world[IN_ROOM(ch)].glight,world[IN_ROOM(ch)].gdark,koef);
  //    send_to_char(buf,ch);
  //   }
}

void affect_modify(CHAR_DATA * ch, sbyte loc, int mod, bitvector_t bitv, bool add)
{
   if (add)
     {  if (bitv)
        SET_BIT (AFF_FLAGS (ch, bitv), bitv);
     }
   else
     { REMOVE_BIT (AFF_FLAGS (ch, bitv), bitv);
       mod = -mod;
     }
 
  switch (loc) {
  case APPLY_NONE:
    break;
 case APPLY_STR:
	if (!IS_NPC(ch)) {
		int x = GET_STR_ADD(ch);
		int y = 1;
	}
	
    GET_STR_ADD(ch) += mod;
    break;
  case APPLY_DEX:
    GET_DEX_ADD(ch) += mod;
    break;
  case APPLY_INT:
    GET_INT_ADD(ch) += mod;
    break;
  case APPLY_WIS:
    GET_WIS_ADD(ch) += mod;
    break;
  case APPLY_CON:
    GET_CON_ADD(ch) += mod;
    break;
  case APPLY_CHA:
    GET_CHA_ADD(ch) += mod;
    break;
  case APPLY_CLASS:
    break;

  /*
   * My personal thoughts on these two would be to set the person to the
   * value of the apply.  That way you won't have to worry about people
   * making +1 level things to be imp (you restrict anything that gives
   * immortal level of course).  It also makes more sense to set someone
   * to a class rather than adding to the class number. -gg
   */

  case APPLY_LEVEL:
    break;
  case APPLY_AGE:
    GET_AGE_ADD(ch)    += mod;
    break;
  case APPLY_CHAR_WEIGHT:
    GET_WEIGHT_ADD(ch) += mod;
    break;
  case APPLY_CHAR_HEIGHT:
    GET_HEIGHT_ADD(ch) += mod;
    break;
  case APPLY_MANAREG:
    GET_MANAREG(ch) += mod;
    break;
  case APPLY_HIT:
    GET_HIT_ADD(ch) += mod;
    break;
  case APPLY_MOVE:
    GET_MOVE_ADD(ch) += mod;
    break;
  case APPLY_GOLD:
    break;
  case APPLY_EXP:
    break;
  case APPLY_AC:
    GET_AC_ADD(ch) += mod;
    break;
  case APPLY_HITROLL:
    GET_HR_ADD(ch) += mod;
    break;
  case APPLY_DAMROLL:
    GET_DR_ADD(ch) += mod;
    break;
  case APPLY_SAVING_WILL://APPLY_SAVING_PARA
    GET_SAVE(ch, SAVING_WILL) += mod;
    break;
  case APPLY_RESIST_FIRE://APPLY_SAVING_ROD
    GET_RESIST(ch, FIRE_RESISTANCE) += mod;// GET_SAVE(ch, SAVING_ROD)    += mod;
    break;
  case APPLY_RESIST_AIR:// APPLY_SAVING_PETRI
    GET_RESIST(ch, AIR_RESISTANCE) += mod;//GET_SAVE(ch, SAVING_PETRI)  += mod;
    break;
  case APPLY_SAVING_CRITICAL://APPLY_SAVING_BREATH
    GET_SAVE(ch, SAVING_CRITICAL) += mod;
    break;
  case APPLY_SAVING_STABILITY://APPLY_SAVING_SPELL
    GET_SAVE(ch, SAVING_STABILITY)  += mod;
    break;
  case APPLY_SAVING_REFLEX://APPLY_SAVING_BASIC
    GET_SAVE(ch, SAVING_REFLEX)  += mod;
    break;
  case APPLY_HITREG:
    GET_HITREG(ch) += mod;
    break;
  case APPLY_MOVEREG:
    GET_MOVEREG(ch) += mod;
    break;
  case APPLY_C1:
  case APPLY_C2:
  case APPLY_C3:
  case APPLY_C4:
  case APPLY_C5:
  case APPLY_C6:
  case APPLY_C7:
  case APPLY_C8:
  case APPLY_C9:
  //  if(!IS_NPC(ch))
  GET_SLOT(ch,loc-APPLY_C1) += mod;
    break;
  case APPLY_SIZE:
    GET_SIZE_ADD(ch) += mod;
    break;
  case APPLY_ARMOUR:
    GET_ARMOUR(ch)   += mod;
    break;
  case APPLY_POISON:
    GET_POISON(ch)   += mod;
    break;
  case APPLY_CAST_SUCCESS:
    GET_CAST_SUCCESS(ch) += mod;
    break;
  case APPLY_MORALE:
    GET_MORALE(ch)       += mod;
    break;
  case APPLY_INITIATIVE:
    GET_INITIATIVE(ch)   += mod;
    break;
/*  case APPLY_RELIGION:
    if (add)
       GET_PRAY(ch)      |= mod;
    else
       GET_PRAY(ch)      &= mod;
    break;*/
  case APPLY_ABSORBE:
    GET_ABSORBE(ch)      += mod;
    break;
	
  	case APPLY_LIKES:
		GET_LIKES(ch) += mod;
		break;
	case APPLY_RESIST_WATER:
		GET_RESIST(ch, WATER_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_EARTH:
		GET_RESIST(ch, EARTH_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_VITALITY:
		GET_RESIST(ch, VITALITY_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_MIND:
		GET_RESIST(ch, MIND_RESISTANCE) += mod;
		break;
	case APPLY_RESIST_IMMUNITY:
		GET_RESIST(ch, IMMUNITY_RESISTANCE) += mod;
		break;
	case APPLY_ARESIST:
		GET_ARESIST(ch) += mod;
		break;
	case APPLY_DRESIST:
		GET_DRESIST(ch) += mod;
		break;
  
  default:
 log("SYSERR: Unknown apply adjust %d, (name %s), attempt (%s, affect_modify).", loc, GET_NAME(ch),  __FILE__);    
    break;
  } 
}

void total_gods_affect(CHAR_DATA *ch)
{
 struct gods_celebrate_apply_type  *cur = NULL;

 if (IS_NPC(ch) || supress_godsapply)
    return;
 // Set new affects
 if (GET_RELIGION(ch) == RELIGION_POLY)
    cur = Poly_apply;
 else
    cur = Mono_apply;
 // log("[GODAFFECT] Start function...");
 for (; cur; cur=cur->next)
     if (cur->gapply_type == GAPPLY_AFFECT)
        {affect_modify(ch,0,0,cur->modi,TRUE);
        }
     else
     if (cur->gapply_type == GAPPLY_MODIFIER)
        {affect_modify(ch,cur->what,cur->modi,0,TRUE);
        };
 // log("[GODAFFECT] Stop function...");
}




int char_stealth_aff[] =
{AFF_HIDE,
 AFF_SNEAK,
 AFF_CAMOUFLAGE,
 0
};

// This updates a character by subtracting everything he is affected by 
// restoring original abilities, and then affecting all again
          
void affect_total(struct char_data * ch)
{ struct affected_type *af;
  struct obj_data      *obj;
  struct extra_affects_type *extra_affect   = NULL;
  struct obj_affected_type *extra_modifier = NULL;
  FLAG_DATA saved = {0,0,0,0};
  int i, j;

  // Clear all affect, because recalc one
  memset((char *)&ch->add_abils, 0, sizeof(struct char_played_ability_data));

  // PC's clear all affects, because recalc one
    
 if (!IS_NPC(ch))
     {saved = ch->char_specials.saved.affected_by;
      ch->char_specials.saved.affected_by = clear_flags;
      for (i = 0; (j = char_saved_aff[i]); i++)
          if (IS_SET(GET_FLAG(saved,j),j))
             SET_BIT (AFF_FLAGS (ch, j), j);
     }
  
  if (IS_NPC(ch))
  (ch)->add_abils = (&mob_proto[GET_MOB_RNUM(ch)])->add_abils;
  
  total_gods_affect(ch);

   if (GET_SKILL(ch, SKILL_SINGLE_WEAPON) && GET_EQ(ch, WEAR_WIELD))
	 {	if(!GET_EQ(ch, WEAR_HOLD) && !GET_EQ(ch,WEAR_SHIELD)  &&
		   !GET_EQ(ch,WEAR_BOTHS))
		    GET_AC_ADD(ch) -= GET_SKILL(ch, SKILL_SINGLE_WEAPON);
                  GET_SAVE(ch, SAVING_REFLEX)    -= GET_SKILL(ch, SKILL_SINGLE_WEAPON)/9;
	          GET_SAVE(ch, SAVING_CRITICAL)  -= GET_SKILL(ch, SKILL_SINGLE_WEAPON)/6;
	 }

   if (GET_SKILL(ch, SKILL_BOTH_WEAPON) && GET_EQ(ch, WEAR_BOTHS))
	  {	   GET_INITIATIVE(ch) += GET_SKILL(ch, SKILL_BOTH_WEAPON) / 10;
		   GET_HR_ADD(ch) += GET_SKILL(ch, SKILL_BOTH_WEAPON) / 6;
		   GET_DR_ADD(ch) += GET_SKILL(ch, SKILL_BOTH_WEAPON) / 4;
	  }
   if (GET_SKILL(ch, SKILL_TWO_WEAPON) && GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_HOLD))
	    GET_AC_ADD(ch) -= GET_SKILL(ch, SKILL_TWO_WEAPON)/2;

   if (GET_SKILL(ch, SKILL_SHIELD_MASTERY) && GET_EQ(ch,WEAR_SHIELD))
	 	   GET_AC_ADD(ch) -= GET_SKILL(ch, SKILL_SHIELD_MASTERY)/2;
   
    for (i = 0; i < NUM_WEARS; i++)
      {if ((obj = GET_EQ(ch, i)))
          {
                 /* Update weapon applies */
           for (j = 0; j < MAX_OBJ_AFFECT; j++)
				{ if (!GET_EQ(ch, i)->affected[j].location)
			        continue;
					affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
		                         GET_EQ(ch, i)->affected[j].modifier,
								 0, TRUE);
				}
		   /* Update weapon bitvectors */
          for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
               {if (weapon_affect[j].aff_bitvector == 0 || !IS_OBJ_AFF (obj, weapon_affect[j].aff_pos))
                    continue;
                affect_modify(ch, APPLY_NONE, 0, weapon_affect[j].aff_bitvector,TRUE);
               }
		  }
      }
  
  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);

 // move race and class modifiers 
  if (!IS_NPC(ch))
     {if ((int) GET_CLASS(ch) >= 0 && (int) GET_CLASS(ch) < NUM_CLASSES)
         {extra_affect   = class_app[(int) GET_CLASS(ch)].extra_affects;
          extra_modifier = class_app[(int) GET_CLASS(ch)].extra_modifiers;
	
          for (i = 0; extra_affect && (extra_affect+i)->affect != -1; i++)
              affect_modify(ch,APPLY_NONE,0,(extra_affect+i)->affect,(extra_affect+i)->set_or_clear ? true : false);
          for (i = 0; extra_modifier && extra_modifier[i].location != -1; i++)
              affect_modify(ch, extra_modifier[i].location,(extra_modifier+i)->modifier,0,TRUE);
         }

      if (GET_RACE(ch) < NUM_RACES)
         {extra_affect   = race_app[(int) GET_RACE(ch)].extra_affects;
          extra_modifier = race_app[(int) GET_RACE(ch)].extra_modifiers;
          for (i = 0; extra_affect && (extra_affect+i)->affect != -1; i++)
              affect_modify(ch,APPLY_NONE,0,(extra_affect+i)->affect,(extra_affect+i)->set_or_clear ? true : false);
          for (i = 0; extra_modifier && (extra_modifier + i)->location != -1; i++)
              affect_modify(ch,(extra_modifier+i)->location,(extra_modifier+i)->modifier,0,TRUE);
         }
  

  // Make certain values are between 0..25, not < 0 and not > 25! 
   
         //Клан_бонусы
     if (!supress_godsapply && GET_CLAN_RANK(ch))
        ClansAffect(ch);

    
	  if (IS_WEDMAK(ch))
	     if (GET_WIS_ADD(ch))
	    GET_MANA_ADD(ch) += MAX(wis_app[GET_REAL_WIS(ch)].spell_success * GET_WIS_ADD(ch),0);

// Apply other PC modifiers
       switch (IS_CARRYING_W(ch) * 10 / MAX(1,CAN_CARRY_W(ch)))
       {case 10: case 9: case 8:
             GET_DEX_ADD(ch) -= 2;
             break;
        case 7: case 6: case 5:
             GET_DEX_ADD(ch) -= 1;
             break;
       }
       GET_DR_ADD(ch) += extra_damroll((int)GET_CLASS(ch),(int)GET_LEVEL(ch));
       GET_HITREG(ch) += ((int)GET_LEVEL(ch) + 4) / 5 * 10;

       
       if (GET_CON_ADD(ch))
          {i = con_app[GET_REAL_CON(ch)].addhit * GET_CON_ADD(ch);
           GET_HIT_ADD(ch) += i;
           if ((i = GET_MAX_HIT(ch) + GET_HIT_ADD(ch)) < 1)
              GET_HIT_ADD(ch) -= (i-1);
          }
       if (!WAITLESS(ch) && on_horse(ch))
          {  REMOVE_BIT (AFF_FLAGS (ch, AFF_HIDE), AFF_HIDE);
	     REMOVE_BIT (AFF_FLAGS (ch, AFF_SNEAK), AFF_SNEAK);
	     REMOVE_BIT (AFF_FLAGS (ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
	     REMOVE_BIT (AFF_FLAGS (ch, AFF_INVISIBLE), AFF_INVISIBLE);
	  }
   
       	// correctize all weapon 
   if ((obj=GET_EQ(ch,WEAR_BOTHS)) && !IS_IMMORTAL(ch) && !OK_BOTH(ch,obj))
     {act("Вам слишком тяжело держать $3 в обоих руках!",FALSE,ch,obj,0,TO_CHAR);
      act("$n прекратил$y использовать $3.",FALSE,ch,obj,0,TO_ROOM);
      obj_to_char(unequip_char(ch,WEAR_BOTHS),ch);
      return;
     }
  if ((obj=GET_EQ(ch,WEAR_WIELD)) && !IS_IMMORTAL(ch) && !OK_WIELD(ch,obj))
     {act("Вам слишком тяжело держать $3 в правой руке!",FALSE,ch,obj,0,TO_CHAR);
      act("$n прекратил$y использовать $3.",FALSE,ch,obj,0,TO_ROOM);
      obj_to_char(unequip_char(ch,WEAR_WIELD),ch);
      return;
     }
  if ((obj=GET_EQ(ch,WEAR_HOLD)) && !IS_IMMORTAL(ch) && !OK_HELD(ch,obj))
     {act("Вам слишком тяжело держать $3 в левой руке!",FALSE,ch,obj,0,TO_CHAR);
      act("$n прекратил$y использовать $3.",FALSE,ch,obj,0,TO_ROOM);
      obj_to_char(unequip_char(ch,WEAR_HOLD),ch);
      return;
     }
  }
    // calculate DAMAGE value 
  GET_DAMAGE(ch) = (str_app[STRENGTH_APPLY_INDEX(ch)].todam + GET_REAL_DR(ch)) * 2;
  if ((obj = GET_EQ(ch,WEAR_BOTHS)) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
     GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2)+1)) >> 1;
  else
     {if ((obj = GET_EQ(ch,WEAR_WIELD)) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
         GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2)+1)) >> 1;
      if ((obj = GET_EQ(ch,WEAR_HOLD)) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
         GET_DAMAGE(ch) += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2)+1)) >> 1;
     }

  // Calculate CASTER value
  for (GET_CASTER(ch) = 0, i = 1; !IS_NPC(ch) && i <= MAX_SPELLS; i++)
      if (IS_SET(GET_SPELL_TYPE(ch,i), SPELL_KNOW | SPELL_TEMP))
         GET_CASTER(ch) += (spell_info[i].danger * GET_SPELL_MEM(ch,i));

 // Check steal affects
  for (i = 0; (j = char_stealth_aff[i]); i++)
	{if ( IS_SET(GET_FLAG(saved,j),j) &&
           !IS_SET(AFF_FLAGS(ch, j), j)
	  )
          CHECK_AGRO(ch) = TRUE;
      }


		if (FIGHTING(ch))
		{   REMOVE_BIT (AFF_FLAGS (ch, AFF_HIDE), AFF_HIDE);
		    REMOVE_BIT (AFF_FLAGS (ch, AFF_SNEAK), AFF_SNEAK);
		    REMOVE_BIT (AFF_FLAGS (ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
		    REMOVE_BIT (AFF_FLAGS (ch, AFF_INVISIBLE), AFF_INVISIBLE);
		}
}




// Insert an affect_type in a char_data structure
//   Automatically sets apropriate bits and apply's
void affect_to_char(CHAR_DATA * ch, struct affected_type * af)
{
	long   was_lgt = AFF_FLAGGED(ch,AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
           was_hlgt= AFF_FLAGGED(ch,AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
           was_hdrk= AFF_FLAGGED(ch,AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO;

  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
  affect_total(ch);
check_light(ch, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}



/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(CHAR_DATA * ch, struct affected_type * af)
{ int    was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
         duration = 0;

  struct affected_type *temp;
  int    change = 0;

  // if (IS_IMMORTAL(ch))
  //   {sprintf(buf,"<%d>\r\n",was_hdrk);
  //    send_to_char(buf,ch);
  //   }

  if (ch->affected == NULL)
     {log("SYSERR: affect_remove(%s) when no affects...",GET_NAME(ch));
      // core_dump();
      return;
     }

  affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
  if (af->type == SPELL_ABSTINENT)
     {GET_DRUNK_STATE(ch) = GET_COND(ch, DRUNK) = MIN(GET_COND(ch, DRUNK), CHAR_DRUNKED - 1);
     }
  else
  if (af->type == SPELL_DRUNKED)
     {duration = pc_duration(ch, 3, MAX(0,GET_DRUNK_STATE(ch) - CHAR_DRUNKED), 0, 0, 0);
      if (af->location == APPLY_AC)
         {af->type      = SPELL_ABSTINENT;
          af->duration  = duration;
          af->modifier  = 20;
          af->bitvector = AFF_ABSTINENT;
          change        = 1;
         }
      else
      if (af->location == APPLY_HITROLL)
         {af->type     = SPELL_ABSTINENT;
          af->duration  = duration;
          af->modifier  = -2;
          af->bitvector = AFF_ABSTINENT;
          change        = 1;
         }
      else
      if (af->location == APPLY_DAMROLL)
         {af->type      = SPELL_ABSTINENT;
          af->duration  = duration;
          af->modifier  = -2;
          af->bitvector = AFF_ABSTINENT;
          change        = 1;
         }
     }
  if (change)
     affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
  else
     {REMOVE_FROM_LIST(af, ch->affected, next);
      free(af);
     }
  //log("[AFFECT_REMOVE->AFFECT_TOTAL] Start");
  affect_total(ch);
  //log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Stop");
  check_light(ch, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}

// Call affect_remove with every spell of spelltype "skill"
void affect_from_char(struct char_data * ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type)
      affect_remove(ch, hjp);
  }
	if (IS_NPC(ch) && type == SPELL_CHARM)
     EXTRACT_TIMER(ch) = 5;
}



/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */

bool affected_by_spell(CHAR_DATA * ch, int type)
{ AFFECT_DATA *hjp;
	//проверить, разные ли эти заклы или одинаковые, а то уже не помню, ежели что разобраться и протестить. 31.07.2007г.
	if (type == SPELL_POWER_SIELENCE)
	   type   = SPELL_SIELENCE;
	else
	if (type == SPELL_POWER_BLINDNESS)
	   type   = SPELL_BLINDNESS;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == type)
      return (TRUE);
  return (FALSE);
}

void affect_join_fspell(CHAR_DATA * ch, AFFECT_DATA * af)
{
	AFFECT_DATA *hjp;
	bool found = FALSE;

	for (hjp = ch->affected; !found && hjp; hjp = hjp->next) {
		if ((hjp->type == af->type) && (hjp->location == af->location)) {

			if (hjp->modifier < af->modifier)
				hjp->modifier = af->modifier;
			if (hjp->duration < af->duration)
				hjp->duration = af->duration;
			affect_total(ch);
			found = TRUE;
		}
	}
	if (!found) 
    affect_to_char(ch, af);
	
}

void affect_join(CHAR_DATA * ch, AFFECT_DATA * af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{ AFFECT_DATA *hjp;
  bool found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = hjp->next) {

    if ((hjp->type == af->type) && (hjp->location == af->location)) {
      if (add_dur)
	af->duration += hjp->duration;
      if (avg_dur)
	af->duration /= 2;

      if (add_mod)
	af->modifier += hjp->modifier;
      if (avg_mod)
	af->modifier /= 2;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}


// Insert an timed_type in a char_data structure
void timed_to_char(CHAR_DATA * ch, struct timed_type * timed)
{ struct timed_type *timed_alloc;

  CREATE(timed_alloc, struct timed_type, 1);

  *timed_alloc      = *timed;
  timed_alloc->next = ch->timed;
  ch->timed         = timed_alloc;
}

void timed_from_char(CHAR_DATA * ch, struct timed_type * timed)
{
  struct timed_type *temp;

  if (ch->timed == NULL)
     {log("SYSERR: timed_from_char(%s) when no timed...",GET_NAME(ch));
        return;
     }

  REMOVE_FROM_LIST(timed, ch->timed, next);
  free(timed);
}

int timed_by_skill(CHAR_DATA * ch, int skill)
{
  struct timed_type *hjp;

  for (hjp = ch->timed; hjp; hjp = hjp->next)
      if (hjp->skill == skill)
         return (hjp->time);

  return (0);
}



// перемещение игрока из комнаты
void char_from_room(CHAR_DATA * ch)
{ CHAR_DATA *temp;

  if (ch == NULL || ch->in_room == NOWHERE) {
    log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
    exit(1);
  }

  if (FIGHTING(ch) != NULL)
    stop_fighting(ch, TRUE);

  check_light(ch,LIGHT_NO,LIGHT_NO,LIGHT_NO,LIGHT_NO,-1);
  REMOVE_FROM_LIST(ch, world[ch->in_room].people, next_in_room);
  ch->in_room = NOWHERE;
  ch->next_in_room = NULL;
}



// размещение персонажа в комнате
void char_to_room(CHAR_DATA * ch, room_rnum room)
{
  if (ch == NULL || room < 0 || room > top_of_world)
     log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p",
		  room, top_of_world, ch);
  else
     {ch->next_in_room   = world[room].people;
      world[room].people = ch;
      ch->in_room        = room;
      check_light(ch,LIGHT_NO,LIGHT_NO,LIGHT_NO,LIGHT_NO,1);
      REMOVE_BIT(EXTRA_FLAGS(ch), EXTRA_FAILHIDE);
      REMOVE_BIT(EXTRA_FLAGS(ch), EXTRA_FAILSNEAK);
      REMOVE_BIT(EXTRA_FLAGS(ch), EXTRA_FAILCAMOUFLAGE);
      if (IS_GRGOD(ch))// && PRF_FLAGGED(ch,PRF_CODERINFO)
         {sprintf(buf,"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d \r\n"
	                  "%sТьма=%s%d %sЗапрет=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d.\r\n",
	              CCNRM(ch,C_NRM), CCNRM(ch, C_NRM), room,
	              CCRED(ch,C_NRM), CCRED(ch, C_NRM), world[room].light,
	              CCGRN(ch,C_NRM), CCGRN(ch, C_NRM), world[room].glight,
	          	  CCYEL(ch,C_NRM), CCYEL(ch, C_NRM), world[room].fires,
	          	  CCBLU(ch,C_NRM), CCBLU(ch, C_NRM), world[room].gdark,
	          	  CCMAG(ch,C_NRM), CCMAG(ch, C_NRM), world[room].forbidden,
	          	  CCCYN(ch,C_NRM), CCCYN(ch, C_NRM), weather_info.sky,
	          	  CCWHT(ch,C_NRM), CCWHT(ch, C_NRM), weather_info.sunlight,
	          	  CCYEL(ch,C_NRM), CCYEL(ch, C_NRM), weather_info.moon_day	          	
	          	  );
          send_to_char(buf,ch);
         }

      // Stop fighting now, if we left.
      if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
         {stop_fighting(FIGHTING(ch), FALSE); 
          stop_fighting(ch,TRUE);
         }
     }
}


void restore_object(struct obj_data *obj, CHAR_DATA *ch)
{int i, j;

 if ((i = GET_OBJ_RNUM(obj)) < 0)
    	return;

 if (GET_OBJ_OWNER(obj) && OBJ_FLAGGED(obj, ITEM_NODONATE) && (!ch || GET_UNIQUE(ch) != GET_OBJ_OWNER(obj)))
   { GET_OBJ_VAL(obj,0) = GET_OBJ_VAL(obj_proto+i, 0);
     GET_OBJ_VAL(obj,1) = GET_OBJ_VAL(obj_proto+i, 1);
     GET_OBJ_VAL(obj,2) = GET_OBJ_VAL(obj_proto+i, 2);
     GET_OBJ_VAL(obj,3) = GET_OBJ_VAL(obj_proto+i, 3);
     GET_OBJ_MATER(obj) = GET_OBJ_MATER(obj_proto+i);
     GET_OBJ_MAX(obj)   = GET_OBJ_MAX(obj_proto+i);
     GET_OBJ_CUR(obj)   = 1;
     GET_OBJ_WEIGHT(obj)= GET_OBJ_WEIGHT(obj_proto+i);
     GET_OBJ_TIMER(obj) = 24*60;
     obj->obj_flags.extra_flags = (obj_proto+i)->obj_flags.extra_flags;
     obj->obj_flags.affects     = (obj_proto+i)->obj_flags.affects;
     GET_OBJ_WEAR(obj)  = GET_OBJ_WEAR(obj_proto+i);
     GET_OBJ_OWNER(obj) = 0;
     for (j = 0; j < MAX_OBJ_AFFECT; j++)
         obj->affected[j] = (obj_proto+i)->affected[j];
   }
}


// give an object to a char 
void obj_to_char(struct obj_data * object, CHAR_DATA * ch)
{ int may_carry = TRUE;

   if (object && ch)
     { restore_object(object,ch);
      if (invalid_anti_class(ch, object) || invalid_unique(ch, object))
         may_carry = FALSE;

      if (!may_carry)
         {act("В Вас ударила молния при попытке взять $3.", FALSE, ch, object, 0, TO_CHAR);
          act("$n попытал$u взять $3, но содрогнул$u от удара молнии.", FALSE, ch, object, 0, TO_ROOM);
          obj_to_room(object, IN_ROOM(ch));
          return;
         }
	
      if (!IS_NPC(ch))
        SET_BIT(GET_OBJ_EXTRA(object, ITEM_TICKTIMER), ITEM_TICKTIMER);
	
      object->next_content = ch->carrying;
      ch->carrying         = object;
      object->carried_by   = ch;
      object->in_room      = NOWHERE;
      IS_CARRYING_W(ch)   += GET_OBJ_WEIGHT(object);
      IS_CARRYING_N(ch)++;

      // set flag for crash-save system, but not on mobs!
      if (!IS_NPC(ch))
       SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
     }
  else
     log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}

// убираем предмет из персонажа 
void obj_from_char(struct obj_data * object)
{
  struct obj_data *temp;

  if (object == NULL) {
    log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  if (object->carried_by == NULL)
     return;

  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  // set flag for crash-save system, but not on mobs!
  if (!IS_NPC(object->carried_by))
     SET_BIT(PLR_FLAGS(object->carried_by, PLR_CRASH), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;
}



// Return the effect of a piece of armor in position eq_pos
int apply_ac(CHAR_DATA * ch, int eq_pos)
{ int factor;

  if (GET_EQ(ch, eq_pos) == NULL) 
     return (0);
     
  if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
    return (0);

  switch (eq_pos) {

  case WEAR_BODY:
         factor = 10;
      break;		
    case WEAR_HEAD:
    case WEAR_FEET:
         factor = 4;
      break;		
    case WEAR_LEGS:
    case WEAR_HANDS:
         factor = 5;
      break;
    case WEAR_ARMS:
    case WEAR_WAIST:
		 factor = 2;
      break;
    case WEAR_SHIELD:
         factor = 7;
      break;		
    case WEAR_ABOUT:
         factor = 3;
      break;
    default:
         factor = 1;
      break;			// all others 10% 
  }

  return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

int apply_armour(CHAR_DATA * ch, int eq_pos)
{ int    factor = 1;
  struct obj_data *obj = GET_EQ(ch,eq_pos);

  if (!obj)
     {log("SYSERR: apply_armor(%s,%d) when no equip...",GET_NAME(ch),eq_pos);
        return (0);
     }

  if (!(GET_OBJ_TYPE(obj) == ITEM_ARMOR))
     return (0);

  switch (eq_pos)
         {
 case WEAR_BODY:
         factor = 4;
      break;		
    case WEAR_HEAD:
    case WEAR_FEET:
         factor = 2;
      break;		
    case WEAR_LEGS:
    case WEAR_HANDS:
         factor = 3;
      break;
    case WEAR_ARMS:
    case WEAR_WAIST:
		 factor = 1;
      break;
    case WEAR_SHIELD:
         factor = 3;
      break;		
    case WEAR_ABOUT:
         factor = 2;
      break;
    default:
         factor = 1;
      break;		// all others 10%
          }


  return ( factor * GET_OBJ_VAL(obj, 1) * GET_OBJ_CUR(obj) / MAX(1,GET_OBJ_MAX(obj)) );
}

int invalid_align(CHAR_DATA *ch, OBJ_DATA *obj)
{
  if (IS_OBJ_ANTI(obj, ITEM_AN_EVIL) && IS_EVIL(ch))
    return TRUE;
  if (IS_OBJ_ANTI(obj, ITEM_AN_GOOD) && IS_GOOD(ch))
    return TRUE;
  if (IS_OBJ_ANTI(obj, ITEM_AN_NEUTRAL) && IS_NEUTRAL(ch))
    return TRUE;
  return FALSE;
}

int preequip_char(CHAR_DATA * ch, OBJ_DATA * obj, int pos)
{ int    was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
         was_lamp= FALSE;

  if (pos < 0 || pos >= NUM_WEARS)
     {log("SYSERR: preequip(%s,%d) in unknown pos...",GET_NAME(ch),pos);
         return(FALSE);
     }

  if (GET_EQ(ch, pos))
     {log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	      obj->short_description);
      return(FALSE);
     }
  if (obj->carried_by)
     {log("SYSERR: EQUIP: %s - Obj is carried_by when equip.", OBJN(obj,ch));
      return(FALSE);
     }
  if (obj->in_room != NOWHERE)
     {log("SYSERR: EQUIP: %s - Obj is in_room when equip.", OBJN(obj,ch));
      return(FALSE);
     }

  if (invalid_anti_class(ch, obj))
     {act("Вас ударило током, при попытке надеть $3.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n попытал$u использовать $3 - и чудом остал$u жив$y.", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_room(obj, IN_ROOM(ch));
      obj_decay(obj);
      return(FALSE);
     }

  if ((!IS_NPC(ch) && invalid_align(ch, obj)) ||
      invalid_no_class(ch, obj)               ||
      (IS_NPC(ch) && (OBJ_FLAGGED(obj,ITEM_SHARPEN) ||
                      OBJ_FLAGGED(obj,ITEM_ARMORED)))
     )
     {act("$o явно не предназначен$Y для Вас.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n попытал$u использовать $3, но у н$s ничего не получилось.", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_char(obj, ch);
      return(FALSE);
     }

  if (GET_EQ(ch,WEAR_LIGHT) &&
      GET_OBJ_TYPE(GET_EQ(ch,WEAR_LIGHT)) == ITEM_LIGHT &&
      GET_OBJ_VAL(GET_EQ(ch,WEAR_LIGHT), 0)) // была двойка, ставлю 0 для света
     was_lamp = TRUE;

  GET_EQ(ch, pos)   = obj;
  CHECK_AGRO(ch)    = TRUE;
  obj->worn_by      = ch;
  obj->worn_on      = pos;
  obj->next_content = NULL;

  if (ch->in_room == NOWHERE)
     log("SYSERR: ch->in_room = NOWHERE when equipping char %s.", GET_NAME(ch));

  //log("[PREEQUIP_CHAR(%s)->AFFECT_TOTAL] Start",GET_NAME(ch));
  affect_total(ch);
  //log("[PREEQUIP_CHAR(%s)->AFFECT_TOTAL] Stop",GET_NAME(ch));
  check_light(ch,was_lamp,was_lgt,was_hlgt,was_hdrk,1);
  return (TRUE);
}

void postequip_char(CHAR_DATA * ch, OBJ_DATA * obj)
{ int j;

  if (IN_ROOM(ch) != NOWHERE)
     for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
         {if (weapon_affect[j].aff_spell == 0 || !IS_OBJ_AFF(obj, weapon_affect[j].aff_pos))
             continue;
          if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
             {act("Магия $1 потерпела неудачу и развеялась по ветру",FALSE,ch,obj,0,TO_ROOM);
              act("Магия $1 потерпела неудачу и развеялась по ветру",FALSE,ch,obj,0,TO_CHAR);
             }
          else
             mag_affects(obj->obj_flags.level_spell, ch, ch, weapon_affect[j].aff_spell, SAVING_WILL);
             
         }
}


void equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int pos)
{
    int  was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
         was_lamp= FALSE;

  int j, skip_total = IS_SET(pos,0x80), no_cast = IS_SET(pos,0x40);

  REMOVE_BIT(pos,(0x80|0x40));
  
  	if (pos < 0 || pos >= NUM_WEARS) 
    	 { log("SYSERR: equip_char(%s,%d) in unknown pos...", GET_NAME(ch), pos);
        	return;
    	 }

  	if (GET_EQ(ch, pos)) 
  	 { log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch), obj->short_description);
       	 	return;
 	 }

  	if (obj->carried_by) 
	{ log("SYSERR: EQUIP: Obj is carried_by when equip.");
    	  	return;
  	}

  	if (obj->in_room != NOWHERE)
       { log("SYSERR: EQUIP: Obj is in_room when equip.");
    		return;
       }
 
  if (invalid_anti_class(ch, obj))
     {act("Вас ударило током при попытке использовать $3.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n попытал$u использовать $3, но пошатнул$u от электрического разряда.", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_room(obj, IN_ROOM(ch));
      obj_decay(obj);
      return;
     }
  
  if ((!IS_NPC(ch) && invalid_align(ch, obj)) || invalid_no_class(ch, obj))
     {act("$3 не может быть Вами использован$Y.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n попытал$u использовать $3, но у н$s ничего не получилось.", FALSE, ch, obj, 0, TO_ROOM);
      obj_to_char(obj, ch);
      return;
     }


if (GET_EQ(ch,WEAR_LIGHT) &&
      GET_OBJ_TYPE(GET_EQ(ch,WEAR_LIGHT)) == ITEM_LIGHT &&
      GET_OBJ_VAL(GET_EQ(ch,WEAR_LIGHT), 0))// изменяю двойку на 0 для света
     was_lamp = TRUE;

  GET_EQ(ch, pos) 	= obj;
  obj->worn_by 		= ch;
  obj->worn_on 		= pos;
  obj->next_content 	= NULL;
  CHECK_AGRO(ch)    	= TRUE;
  
  if (ch->in_room == NOWHERE)
     log("SYSERR: ch->in_room = NOWHERE when equipping char %s.", GET_NAME(ch));
  
 	 for (j = 0; j < MAX_OBJ_AFFECT; j++)
	   { if (!obj->affected[j].location) 		
			continue;	
		affect_modify(ch, obj->affected[j].location,
		        obj->affected[j].modifier, 0, TRUE);
	   }

    if (IN_ROOM(ch) != NOWHERE)
       for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
             { if (weapon_affect[j].aff_spell == 0 ||
	        !IS_OBJ_AFF (obj, weapon_affect[j].aff_pos))
                 continue;
        
          if (!no_cast)
             {log("[EQUIPPING] Casting magic...");
              if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
                 {act("Магия $1 потерпела неудачу и развеялась по воздуху",FALSE,ch,obj,0,TO_ROOM);
                  act("Магия $1 потерпела неудачу и развеялась по воздуху",FALSE,ch,obj,0,TO_CHAR);
                 }
              else
                 {mag_affects(obj->obj_flags.level_spell, ch, ch,
                              weapon_affect[j].aff_spell, SAVING_SPELL);
                 }
             }
         }

  if (!skip_total)
     {//log("[EQUIP_CHAR(%s)->AFFECT_TOTAL] Start",GET_NAME(ch));
      affect_total(ch);
      //log("[EQUIP_CHAR(%s)->AFFECT_TOTAL] Stop",GET_NAME(ch));
      check_light(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
     }
		  
		 
}


OBJ_DATA *unequip_char(CHAR_DATA * ch, int pos)
{ int    was_lgt = AFF_FLAGGED(ch, AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hlgt= AFF_FLAGGED(ch, AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO,
         was_hdrk= AFF_FLAGGED(ch, AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO,
         was_lamp= FALSE;

  int    j, skip_total = IS_SET(pos,0x80);
  struct obj_data *obj;

  REMOVE_BIT(pos,(0x80|0x40));

  	if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL)
     	{ log("SYSERR: unequip_char(%s,%d) - unused pos or no equip...", GET_NAME(ch), pos);
         	return (NULL);
     	}

  if (GET_EQ(ch,WEAR_LIGHT) &&
      GET_OBJ_TYPE(GET_EQ(ch,WEAR_LIGHT)) == ITEM_LIGHT &&
      GET_OBJ_VAL(GET_EQ(ch,WEAR_LIGHT), 0))//изменяю двойку на 0 для света
       was_lamp = TRUE;

  obj               = GET_EQ(ch, pos);
  obj->worn_by      = NULL;
  obj->next_content = NULL;
  obj->worn_on      = -1;
  GET_EQ(ch, pos)   = NULL;

  if (ch->in_room == NOWHERE)
     log("SYSERR: ch->in_room = NOWHERE when unequipping char %s.", GET_NAME(ch));

  
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
     { if (!obj->affected[j].location)
			continue;
	  affect_modify(ch, obj->affected[j].location,
	            obj->affected[j].modifier,
	            0, FALSE);
     }
		
  
  if (IN_ROOM(ch) != NOWHERE)		
     for (j = 0; weapon_affect[j].aff_bitvector >= 0; j++)
         { if (weapon_affect[j].aff_bitvector == 0 || !IS_OBJ_AFF (obj, weapon_affect[j].aff_pos))
             continue;
          affect_modify(ch, APPLY_NONE, 0, weapon_affect[j].aff_bitvector, FALSE);
         }	

  if (!skip_total)
     {//log("[UNEQUIP_CHAR->AFFECT_TOTAL] Start");
      affect_total(ch);
      //log("[UNEQUIP_CHAR->AFFECT_TOTAL] Stop");
      check_light(ch, was_lamp, was_lgt, was_hlgt, was_hdrk, 1);
     }

  return (obj);
}


int get_number(char **name)

{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr(*name, '.')) != NULL)
     {*ppos = '\0';
      strcpy(number, *name);
      for (i = 0; *(number + i); i++)
          {if (!isdigit((unsigned char)*(number + i)))
	          {*ppos = '.';
	           return (1);
	          }
	      }
      strcpy(*name, ppos+1);
      return (atoi(number));
     }
  return (1);
}


// Search a given list for an object number, and return a ptr to that obj 
OBJ_DATA *get_obj_in_list_num(int num, OBJ_DATA * list)
{ OBJ_DATA *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return (i);

  return (NULL);
}



// search the entire world for an object number, and return a pointer 
OBJ_DATA *get_obj_num(obj_rnum nr)
{ OBJ_DATA *i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return (i);

  return (NULL);
}



// search a room for a char, and return a pointer if found.. 
CHAR_DATA *get_char_room(char *name, room_rnum room)
{
  CHAR_DATA *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return (NULL);

  for (i = world[room].people; i && (j <= number); i = i->next_in_room)
    if (isname(tmp, i->player.name))
      if (++j == number)
	return (i);

  return (NULL);
}



// search all over the world for a char num, and return a pointer if found 
CHAR_DATA *get_char_num(mob_rnum nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return (i);

  return (NULL);
}



// кладем предмет в комнату 
void obj_to_room(OBJ_DATA * object, room_rnum room)
{ int sect = 0;

  if (!object || room < FIRST_ROOM || room > top_of_world)
     {log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
	      room, top_of_world, object);
      if (object)
         extract_obj(object);
     }
  else
     {restore_object(object,0);
      object->next_content = world[room].contents;
      world[room].contents = object;
      object->in_room      = room;
      object->carried_by   = NULL;
      object->worn_by      = NULL;
      sect                 = real_sector(room);
      if (ROOM_FLAGGED(room, ROOM_HOUSE))
         SET_BIT(world[room].room_flags.flags[0], ROOM_HOUSE_CRASH);
      if (object->proto_script || object->script)
         GET_OBJ_DESTROY(object)  = script_destroy_timer;
      else
      if (OBJ_FLAGGED(object, ITEM_NODECAY))
         GET_OBJ_DESTROY(object)  = room_nodestroy_timer;
      else
 
      if (GET_OBJ_TYPE(object) == ITEM_MONEY)
         GET_OBJ_DESTROY(object)  = money_destroy_timer;
      else
      if (ROOM_FLAGGED(room, ROOM_DEATH))
         GET_OBJ_DESTROY(object)  = death_destroy_timer;
      else
         GET_OBJ_DESTROY(object)  = room_destroy_timer;
     }
}
 

// Функция для удаления обьектов после лоада в комнату
//   результат работы - 1 если посыпался, 0 - если остался 
int obj_decay(OBJ_DATA * object)
{ int room, sect;
  room = object->in_room;

	if (room == NOWHERE)
   	  return (0);

 sect = real_sector(room);


 if (((sect == SECT_WATER_SWIM || sect == SECT_WATER_NOSWIM) &&
      !IS_CORPSE(object) &&
      !OBJ_FLAGGED(object, ITEM_SWIMMING))) {
	   act("$o медленно утонул$Y.", FALSE,
             world[room].people, object, 0, TO_ROOM);
       act("$o медленно утонул$Y.", FALSE,
             world[room].people, object, 0, TO_CHAR);
	        extract_obj(object); 
	return (1);
  }

 if (((sect == SECT_FLYING ) &&
      !IS_CORPSE(object) &&
      !OBJ_FLAGGED(object, ITEM_FLYING))) {
        act("$o упал$Y вниз и рассыпал$U.", FALSE,
              world[room].people, object, 0, TO_ROOM);
		act("$o упал$Y вниз и рассыпал$U.", FALSE,
              world[room].people, object, 0, TO_CHAR);
		     extract_obj(object);
	return (1);
  }


//дописать функцию рассыпания при репопе зоны...
  if (OBJ_FLAGGED(object, ITEM_DECAY) ||
     (OBJ_FLAGGED(object, ITEM_ZONEDECAY) &&
     GET_OBJ_ZONE(object) != NOWHERE &&
     GET_OBJ_ZONE(object) != world[room].zone))
	{ act("$o рассыпал$U в мелкую пыль, которую развеял ветер.", FALSE,
            world[room].people, object, 0, TO_ROOM);
      act("$o рассыпал$U в мелкую пыль, которую развеял ветер.", FALSE,
           world[room].people, object, 0, TO_CHAR);
      	  extract_obj(object);
	 return (1);
	}
  
  return (0);
}
// Убираем предметы из комнаты 
void obj_from_room(OBJ_DATA * object)
{ OBJ_DATA *temp;

  if (!object || object->in_room == NOWHERE)
   { log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room", object, object->in_room);
   	 return;
   }

  REMOVE_FROM_LIST(object, world[object->in_room].contents, next_content);

  object->in_room 	= NOWHERE;
  object->next_content 	= NULL;
}


// Кладем предмет в контейнер (рюкзак, сумка и ect)
void obj_to_obj(OBJ_DATA * obj, OBJ_DATA * obj_to)
{ OBJ_DATA *tmp_obj;

  if (!obj || !obj_to || obj == obj_to) {
    log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.", obj, obj, obj_to);
    return;
  }

  obj->next_content = obj_to->contains;
  obj_to->contains  = obj;
  obj->in_obj 	    = obj_to;

  for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

  // top level object.  Subtract weight from inventory if necessary. 
  GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->carried_by)
    IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
}


// Заберем предмет из контейнера (рюкзак, сумка и ect) и вычтим реальный вес
void obj_from_obj(OBJ_DATA * obj)
{ OBJ_DATA *temp, *obj_from;

  if (obj->in_obj == NULL)
    { log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    	return;
    }

  obj_from = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  // Вычитаем вес контейнера (забрали предмет из сумки, рюкзака и ect)
  for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

  // Вычитаем вес предметов из персонажа, который их несет
  GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (temp->carried_by)
    IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);

  obj->in_obj = NULL;
  obj->next_content = NULL;
}


// Set all carried_by to point to new owner 
void object_list_new_owner(OBJ_DATA * list, CHAR_DATA * ch)
{
  if (list) {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}


// Удаление объекта из мира 
void extract_obj(OBJ_DATA * obj)
{
char   name[MAX_STRING_LENGTH];
  struct obj_data *temp;

  strcpy(name,obj->short_description);
//  log("Start extract obj %s",name);

  
  // Обработка содержимого контейнера при его уничтожении 
   while (obj->contains)
  { temp = obj->contains;
	if (world[obj->in_room].people && obj->in_room != -1)
        act("$o вывалил$U на землю.", FALSE, world[obj->in_room].people, temp, world[obj->in_room].people, TO_ROOM);
	   
	obj_from_obj(temp);
    
    	if (obj->carried_by)
	{ if (IS_NPC(obj->carried_by) || (IS_CARRYING_N(obj->carried_by) >= CAN_CARRY_N(obj->carried_by)))
	    { obj_to_room(temp, IN_ROOM(obj->carried_by));
	      obj_decay(temp);
	    } 
	  else
	      obj_to_char(temp, obj->carried_by);
        }
	else
    	if (obj->worn_by != NULL)
	 { if (IS_NPC(obj->worn_by) || (IS_CARRYING_N(obj->worn_by) >= CAN_CARRY_N(obj->worn_by)))
	     { obj_to_room(temp, IN_ROOM(obj->worn_by));
	       obj_decay(temp);
	     }
	   else
	      obj_to_char(temp, obj->worn_by);
	 }
	else
    	if (obj->in_room != NOWHERE)
	 { obj_to_room(temp, obj->in_room);
	   obj_decay(temp);
	 }
	else
        if (obj->in_obj)
	 extract_obj(temp);
	else
     	extract_obj(temp);
  }
  // Содержимое контейнера удалено
  
           
  if (obj->worn_by != NULL)
     if (unequip_char(obj->worn_by, obj->worn_on) != obj)
        log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj->in_room != NOWHERE)
     obj_from_room(obj);
  else
  if (obj->carried_by)
     obj_from_char(obj);
  else
  if (obj->in_obj)
     obj_from_obj(obj);

  check_auction(NULL,obj);
  REMOVE_FROM_LIST(obj, object_list, next);

  if (GET_OBJ_RNUM(obj) >= 0)
     (obj_index[GET_OBJ_RNUM(obj)].number)--;
   
  free_script(SCRIPT(obj)); 
  free_obj(obj);
 // log("Stop extract obj %s",name);
}


void update_object(OBJ_DATA * obj, int use)
{

  if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && GET_OBJ_TIMER(obj) > 0  && OBJ_FLAGGED(obj, ITEM_TICKTIMER))
            GET_OBJ_TIMER(obj) -= use;	  
  if (obj->contains)
    update_object(obj->contains, use);
  
  if (obj->next_content)
    update_object(obj->next_content, use);
}


void update_char_objects(CHAR_DATA * ch)
{ int i;

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
     {if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
         {if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 0) > 0) // ставлю 0 вместо 2
             {i = --GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 0);
  	      if (i == 1)
	         {act("Ваш$Y $o стал$Y мерцать и угасать.",
	              FALSE, ch, GET_EQ(ch, WEAR_LIGHT), 0, TO_CHAR);
                  act("$o $r стал$Y мерцать и угасать.",
	              FALSE, ch, GET_EQ(ch, WEAR_LIGHT), 0, TO_ROOM);
                 }
              else
              if (i == 0)
                 {act("Ваш$Y $o погас$Q.",
                      FALSE, ch, GET_EQ(ch, WEAR_LIGHT), 0, TO_CHAR);
                  act("$o у $r погас$Q.",
                      FALSE, ch, GET_EQ(ch, WEAR_LIGHT), 0, TO_ROOM);
		  if (IN_ROOM(ch) != NOWHERE)
                     {if (world[IN_ROOM(ch)].light > 0)
                         world[IN_ROOM(ch)].light -= 1;
                     }
                  if (OBJ_FLAGGED(GET_EQ(ch,WEAR_LIGHT), ITEM_DECAY))
                     extract_obj(GET_EQ(ch,WEAR_LIGHT));
                 }
             }
         }
     }

  for (i = 0; i < NUM_WEARS; i++)
     {  if (GET_EQ(ch, i))
	   if (!IS_NPC(ch) || OBJ_FLAGGED(GET_EQ(ch, i), ITEM_LIFTED))
		 update_object(GET_EQ(ch, i), 1);
      
     }
	
   if (ch->carrying)
	  { if (!IS_NPC(ch) || OBJ_FLAGGED(ch->carrying, ITEM_LIFTED))
		 update_object(ch->carrying, 1);
	  }	
}



void change_fighting(CHAR_DATA *ch, int need_stop)
{ CHAR_DATA *k, *j, *temp;

  for (k = character_list; k; k = temp)
      {temp = k->next;
       if (PROTECTING(k) == ch)
          {PROTECTING(k) = NULL;
           CLR_AF_BATTLE(k,EAF_PROTECT);
          }
       if (TOUCHING(k) == ch)
          {TOUCHING(k) = NULL;
           CLR_AF_BATTLE(k,EAF_PROTECT);
          }
       if (GET_EXTRA_VICTIM(k) == ch)
	   SET_EXTRA(k, 0, NULL);
           
       if (GET_CAST_CHAR(k) == ch)
          SET_CAST(k, 0, NULL, NULL);
	   	
       if (FIGHTING(k) == ch && IN_ROOM(k) != NOWHERE)
          {//log("[Change fighting] Change victim");
           for (j = world[IN_ROOM(ch)].people; j; j = j->next_in_room)
               if (FIGHTING(j) == k)
                  {act("Вы переключили внимание на $V.",FALSE,k,0,j,TO_CHAR);
                   act("$n переключил$u на Вас !",FALSE,k,0,j,TO_VICT);
                   FIGHTING(k) = j;
                   break;
                  }
           if (!j && need_stop)
              stop_fighting(k,FALSE);
          }
      }
}


// Уберите персонаж из игры и оставте стуфф на земле
void extract_char(CHAR_DATA * ch, int clear_objs)
{ char   name[MAX_STRING_LENGTH];
  struct descriptor_data *t_desc;
  OBJ_DATA *obj, *obj_eq;
  int    i, freed = 0;
  CHAR_DATA *ch_w, *temp = NULL;


  if (MOB_FLAGGED(ch,MOB_FREE) || MOB_FLAGGED(ch,MOB_DELETE))
     return;

  strcpy(name,GET_NAME(ch));

 // log("[Extract char] Start function for char %s",name);
  if (!IS_NPC(ch) && !ch->desc)
     {log("[Extract char] Extract descriptors");
      for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
          if (t_desc->original == ch)
	         do_return(t_desc->character, NULL, 0, 0);
     }

  if (ch->in_room == NOWHERE)
     // перенести в линух лишний спамм 29.04.2005г. log("SYSERR: NOWHERE extracting char %s. (%s, extract_char)",GET_NAME(ch), __FILE__);
      return;
          

 // log("[Extract char] Die followers");
  if (ch->followers || ch->master)
     die_follower(ch);

  // Forget snooping, if applicable 
 
  if (ch->desc)
	{  log("[Extract char] Stop snooping");
	  if (ch->desc->snooping)
         {ch->desc->snooping->snoop_by = NULL;
          ch->desc->snooping = NULL;
         }
      if (ch->desc->snoop_by)
         {SEND_TO_Q("Ваша жертва наблюдений теперь недоступна.\r\n",
		            ch->desc->snoop_by);
          ch->desc->snoop_by->snooping = NULL;
          ch->desc->snoop_by = NULL;
         }
     }

   // transfer equipment to room, if any 
 // log("[Extract char] Drop equipment"); исправление баги с предметом, который в инвентаре локейтится а его нер
 
  for (i = 0; i < NUM_WEARS; i++)//связан был, что если снимался предмет который добавлял силу а силы у чара не хватало
      if (GET_EQ(ch, i))//то предмет попадал в инвенторий, а инвенторий снимался до снятия экипировки - что вызвало багу 15.05.2005г.
         { act("Вы сняли $3 и выбросили на землю.", FALSE, ch, GET_EQ(ch,i), 0, TO_CHAR);
      
          if (!IS_NPC(ch) || !MOB_FLAGGED(ch, MOB_CORPSE))    
	    act("$n снял$y $3 и бросил$y на землю.", FALSE, ch, GET_EQ(ch,i), 0, TO_ROOM);

	  obj_eq = unequip_char(ch, i);
          obj_to_room(obj_eq, ch->in_room);
          obj_decay(obj_eq);
         }

  // transfer objects to room, if any
  //log("[Extract char] Drop objects");
  while (ch->carrying)
        {obj = ch->carrying;
         obj_from_char(obj);
         act("Вы выбросили $3 на землю.", FALSE, ch, obj, 0, TO_CHAR);
         if (!IS_NPC(ch) || !MOB_FLAGGED(ch, MOB_CORPSE))
		 act("$n бросил$y $3 на землю.", FALSE, ch, obj, 0, TO_ROOM);
         obj_to_room(obj, ch->in_room);
	 obj_decay(obj);
        }

  // log("[Extract char] Stop fighting self");
  if (FIGHTING(ch))
     stop_fighting(ch,TRUE);

  //log("[Extract char] Stop all fight for opponee");
  change_fighting(ch,TRUE);


  //log("[Extract char] Remove char from room");
  char_from_room(ch);

  // pull the char from the list
 
    SET_BIT(MOB_FLAGS(ch, MOB_DELETE), MOB_DELETE);
   // REMOVE_FROM_LIST(ch, character_list, next);
   
  if (ch->desc && ch->desc->original)
     do_return(ch, NULL, 0, 0);

  if (!IS_NPC(ch))
     {log("[Extract char] All save for PC");
      check_auction(ch,NULL);
      save_char(ch,NOWHERE);

    //удаляются рент-файлы, если пуст склад
	if (!GET_CHEST(ch).m_chest.size())      
          Crash_delete_crashfile(ch);

        if(clear_objs) 
       Crash_crashsave(ch);
   }   
  else
     {//log("[Extract char] All clear for NPC");
      if (GET_MOB_RNUM(ch) > -1)		// if mobile 
         mob_index[GET_MOB_RNUM(ch)].number--;
         clearMemory(ch);		// Only NPC's can have memory 
     
	  free_script(SCRIPT(ch)); 
          SCRIPT(ch) = NULL;  
     
	  if (SCRIPT_MEM(ch))
	  { extract_script_mem(SCRIPT_MEM(ch));
	    SCRIPT_MEM(ch) = NULL; 
	  }
      
      SET_BIT(MOB_FLAGS(ch, MOB_FREE), MOB_FREE);
       freed = 1;
     }

  if (!freed && ch->desc != NULL)
     { 
        if (GET_MAX_HIT(ch) <= 16)
		  {   //send_to_char("Ваш персонаж умер по настоящему.\r\n\r\n", ch);
			  //delete_char(GET_NAME(ch));
			  //STATE(ch->desc) = CON_CLOSE;
			  //return ;
              GET_MAX_HIT(ch) = 21;
		  }
		if (PLR_FLAGGED(ch, PLR_QUESTOR)) 
		{ ch_w = get_mob_by_id(GET_QUESTMOB(ch));
		 if (ch_w)
		 GET_QUESTOR(ch_w) = 0;
                 GET_COUNTDOWN(ch) = 0;
                 GET_QUESTMOB(ch)  = 0;
		}

	STATE(ch->desc) = CON_MENU;
        SEND_TO_Q(MENU, ch->desc); 
        if (!IS_NPC(ch) && RENTABLE(ch) && clear_objs) 
	  { ch_w = ch->next;
            do_entergame(ch->desc);
            ch->next = ch_w;
 	  }
       
     }
  else
     {// if a player gets purged from within the game
      if (!freed)
        SET_BIT(MOB_FLAGS(ch, MOB_FREE), MOB_FREE);
         
     }
  //log("[Extract char] Stop function for char %s",name);
}


// Extract a MOB completely from the world, and destroy his stuff
void extract_mob(CHAR_DATA * ch)
{ int    i;
 // CHAR_DATA *temp; 

  if (MOB_FLAGGED(ch,MOB_FREE) || MOB_FLAGGED(ch,MOB_DELETE))
     return;

  if (ch->in_room == NOWHERE)
     { log("SYSERR: NOWHERE extracting char %s. (%s, extract_mob)",GET_NAME(ch), __FILE__);
       return;
     }
  
 if (ch->followers || ch->master)
     die_follower(ch);

  // Forget snooping, if applicable 
  if (ch->desc)
     {if (ch->desc->snooping)
         {ch->desc->snooping->snoop_by = NULL;
          ch->desc->snooping = NULL;
         }
      if (ch->desc->snoop_by)
         {SEND_TO_Q("Ваша жертва наблюдений теперь недоступна.\r\n",
		            ch->desc->snoop_by);
          ch->desc->snoop_by->snooping = NULL;
          ch->desc->snoop_by = NULL;
         }
     }

 
   // transfer equipment to room, if any 
  for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
         extract_obj(GET_EQ(ch,i));
  
  
  
  // extract objects, if any
  while (ch->carrying)
        extract_obj(ch->carrying);

   if (FIGHTING(ch))
     stop_fighting(ch,TRUE);

 // log("[Extract mob] Stop all fight for opponee");
  change_fighting(ch,TRUE);

  char_from_room(ch);

  // pull the char from the list
  
  SET_BIT(MOB_FLAGS(ch, MOB_DELETE), MOB_DELETE);
  //REMOVE_FROM_LIST(ch, character_list, next);


  if (ch->desc && ch->desc->original)
     do_return(ch, NULL, 0, 0);

  if (GET_MOB_RNUM(ch) > -1)
     mob_index[GET_MOB_RNUM(ch)].number--;
  
  clearMemory(ch);  
  free_script(SCRIPT(ch));		// см. выше
  SCRIPT(ch) = NULL; 

  if (SCRIPT_MEM(ch))
  { extract_script_mem(SCRIPT_MEM(ch));
    SCRIPT_MEM(ch) = NULL;
  }
    
  SET_BIT(MOB_FLAGS(ch, MOB_FREE), MOB_FREE);
}



/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */
CHAR_DATA *get_player_vis(CHAR_DATA * ch, char *name, int inroom)
{
  struct char_data *i;
    for (i = character_list; i; i = i->next)
	{	if (IS_NPC(i))
			continue;
		if (!HERE(i))
			continue;
		if ((inroom & FIND_CHAR_ROOM) && i->in_room != ch->in_room)
			continue;
   		if (!CAN_SEE(ch, i))
			continue;
		if (!isname(name, i->player.name))
			continue;
	return (i);
	};
    return (NULL);
}

CHAR_DATA *get_char_room_vis(CHAR_DATA * ch, char *name)
{
  CHAR_DATA *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;
  
  
	if (!str_cmp(name, "self")	|| !str_cmp(name, "me")		||
	    !str_cmp(name, "я")		|| !str_cmp(name, "меня")	|| !str_cmp(name, "себя"))
		return (ch);

  // 0.<name> means PC with name 
  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return (get_player_vis(ch, tmp, FIND_CHAR_ROOM));

  for (i = world[ch->in_room].people; i && j <= number; i = i->next_in_room)
    if (HERE(i) && CAN_SEE(ch, i) && isname(tmp, i->player.name))
   		if (++j == number)
		 return (i);
   return (NULL);
}

CHAR_DATA *get_char_vis(CHAR_DATA * ch, char *name, int where)
{
  CHAR_DATA *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  // check the room first 
  if (where == FIND_CHAR_ROOM)
    return get_char_room_vis(ch, name);
  else if (where == FIND_CHAR_WORLD) {
    if ((i = get_char_room_vis(ch, name)) != NULL)
      return (i);

    strcpy(tmp, name);
    if (!(number = get_number(&tmp)))
      return get_player_vis(ch, tmp, 0);

    for (i = character_list; i && (j <= number); i = i->next)
      if (HERE(i) && CAN_SEE(ch, i) && isname(tmp, i->player.name))
        if (++j == number)
          return (i);
  }

  return (NULL);
}


OBJ_DATA *get_obj_in_list_vis(CHAR_DATA * ch, char *name, OBJ_DATA * list)
{
  if (!list)
	  return (NULL);
  OBJ_DATA *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
   return (NULL);

  for (i = list; i && (j <= number); i = i->next_content)
  {
	  if (isname(tmp, i->name))
		  if (CAN_SEE_OBJ(ch, i))
			  if (++j == number)
				  return (i);
  }
  return (NULL);
}




// search the entire world for an object, and return a pointer
OBJ_DATA *get_obj_vis(CHAR_DATA * ch, char *name)
{ OBJ_DATA *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  // scan items carried
  
  if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
    return (i);

  // scan room 
  if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) != NULL)
    return (i);

  strcpy(tmp, name);
  if ((number = get_number(&tmp)) == 0)
    return (NULL);

  // ok.. no luck yet. scan the entire obj list 
  for (i = object_list; i && (j <= number); i = i->next)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (++j == number)
	  return (i);

  return (NULL);
}



OBJ_DATA *get_object_in_equip_vis(CHAR_DATA * ch, char *arg, OBJ_DATA * equipment[], int *j)
{ int  l, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, arg);
  if ((number = get_number(&tmp)) == 0)
     return (NULL);

  for ((*j) = 0, l = 0; (*j) < NUM_WEARS; (*j)++)
      if (equipment[(*j)])
         if (CAN_SEE_OBJ(ch, equipment[(*j)]))
	        if (isname(arg, equipment[(*j)]->name))
	           if (++l == number)
                  return (equipment[(*j)]);

  return (NULL);
}
 
OBJ_DATA *get_obj_in_eq_vis(CHAR_DATA * ch, char *arg)
{ int  l, number, j;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, arg);
  if ((number = get_number(&tmp)) == 0)
     return (NULL);

  for (j = 0, l = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch,j))
         if (CAN_SEE_OBJ(ch, GET_EQ(ch,j)))
	        if (isname(arg, GET_EQ(ch,j)->name))
	           if (++l == number)
                      return (GET_EQ(ch,j));

  return (NULL);
}

char *money_desc(int amount, int padis)
{
  static char buf[128];
  static const char *single[6][2] =
         { {"а",  "а"},
           {"ой", "ы"},
           {"ой", "е"},
           {"у",  "у"},
           {"ой", "ой"},
           {"ой", "е"} },
              *plural[6][3] =
          { {"ая", "а", "а"},
            {"ой", "и", "ы"},
            {"ой", "е", "е"},
            {"ую", "у", "у"},
            {"ой", "ой","ой"},
            {"ой", "е", "е"}};

  if (amount <= 0)
     {log("SYSERR: Try to create negative or 0 money (%d).", amount);
      return (NULL);
     }
  if (amount == 1)
     { sprintf(buf, "одн%s монет%s",
              single[padis][0], single[padis][1]);
     }
  else
  if (amount <= 10)
     sprintf(buf, "маленьк%s горстк%s монет",
             plural[padis][0], plural[padis][1]);
  else
  if (amount <= 20)
     sprintf(buf, "небольш%s горстк%s монет",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 75)
     sprintf(buf, "горстк%s монет", plural[padis][1]);
  else
  if (amount <= 200)
     sprintf(buf, "маленьк%s кучк%s монет",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 1000)
     sprintf(buf, "небольш%s кучк%s монет",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 5000)
     sprintf(buf, "кучк%s монет",
            plural[padis][1]);
  else
  if (amount <= 10000)
     sprintf(buf, "больш%s кучк%s монет",
            plural[padis][0], plural[padis][1]);
  else
  if (amount <= 20000)
     sprintf(buf, "куч%s монет",
            plural[padis][2]);
  else
  if (amount <= 75000)
     sprintf(buf, "больш%s куч%s монет",
            plural[padis][0], plural[padis][2]);
  else
  if (amount <= 150000)
     sprintf(buf, "гор%s монет",
            plural[padis][1]);
  else
  if (amount <= 250000)
     sprintf(buf, "больш%s гор%s монет",
            plural[padis][0], plural[padis][2]);
  else
    sprintf(buf, "огромн%s гор%s монет",
           plural[padis][0], plural[padis][2]);

  return (buf);
}


OBJ_DATA *create_money(int amount)
{ OBJ_DATA *obj;
  struct extra_descr_data *new_descr;
  char buf[200];

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money. (%d)", amount);
    return (NULL);
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);
    
  if (amount == 1)
   {obj->short_description  = str_dup(money_desc(amount,0));
    obj->short_rdescription = str_dup(money_desc(amount,1));
    obj->short_ddescription = str_dup(money_desc(amount,2));
    obj->short_vdescription = str_dup(money_desc(amount,3));
    obj->short_tdescription = str_dup(money_desc(amount,4));
    obj->short_pdescription = str_dup(money_desc(amount,5));
    obj->name = str_dup("монета coin gold"); 
    obj->description = str_dup("Одна монета лежит здесь."); 
    new_descr->keyword = str_dup("монет coin gold");
    new_descr->description = str_dup("Здесь лишь одна монета."); 
   }
   else
   {obj->name = str_dup("монет coin gold");
    obj->short_description  = str_dup(money_desc(amount,0));
    obj->short_rdescription = str_dup(money_desc(amount,1));
    obj->short_ddescription = str_dup(money_desc(amount,2));
    obj->short_vdescription = str_dup(money_desc(amount,3));
    obj->short_tdescription = str_dup(money_desc(amount,4));
    obj->short_pdescription = str_dup(money_desc(amount,5));
    
    sprintf(buf, "%s находится здесь.", money_desc(amount, 0)); 
    obj->description = str_dup(CAP(buf));

    new_descr->keyword = str_dup("монет coin gold");
   
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_SEX(obj)			 = SEX_FEMALE;
  GET_OBJ_TYPE(obj)			 = ITEM_MONEY;
  GET_OBJ_WEAR(obj)			 = ITEM_WEAR_TAKE;
  GET_OBJ_VAL(obj, 0)			 = amount;
  GET_OBJ_COST(obj)			 = amount;
  GET_OBJ_MAX(obj)                   	 = 100;
  GET_OBJ_CUR(obj)                   	 = 100;
  GET_OBJ_TIMER(obj)                	 = 24*60*7;
  GET_OBJ_WEIGHT(obj)               	 = 1;
  GET_OBJ_EXTRA(obj, ITEM_NODONATE) 	|= ITEM_NODONATE;
  GET_OBJ_EXTRA(obj, ITEM_NOSELL)   	|= ITEM_NOSELL;
  
  obj->item_number = NOTHING;

  return (obj);
}


/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(char *arg, bitvector_t bitvector, CHAR_DATA * ch, CHAR_DATA ** tar_ch, OBJ_DATA ** tar_obj)
{ char name[256];

  *tar_ch = NULL;
  *tar_obj = NULL;

  one_argument(arg, name);

  if (!*name)
    return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) {	// Find person in room
    if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_ROOM)) != NULL)
      return (FIND_CHAR_ROOM);
  }
  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_WORLD)) != NULL)
      return (FIND_CHAR_WORLD);
  }
  if (IS_SET(bitvector, FIND_OBJ_EQUIP))
  { if ((*tar_obj = get_obj_in_eq_vis(ch, name)) != NULL)
         return (FIND_OBJ_EQUIP);
     }
    
  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
      return (FIND_OBJ_INV);
  }
  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) != NULL)
      return (FIND_OBJ_ROOM);
  }
  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name)))
      return (FIND_OBJ_WORLD);
  }
  return (0);
}


// a function to scan for "all" or "all.x"
int find_all_dots(char *arg)
{
  if (!strcmp(arg, "all")||!strcmp(arg, "все"))
    return (FIND_ALL);
  else 
    if (!strncmp(arg, "все.", 4)) 
     { strcpy(arg, arg + 4);
       return (FIND_ALLDOT);
     }
    else
    return (FIND_INDIV);
}
