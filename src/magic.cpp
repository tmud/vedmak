/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
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
#include "interpreter.h"
#include "dg_scripts.h"
#include "constants.h"
#include "pk.h"
#include "screen.h"
#include "clan.h"

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *obj_index;
extern struct spell_create_type spell_create[];
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
extern struct obj_data *obj_proto;
extern int mini_mud;
extern int pk_allowed;
extern const char *spell_wear_off_msg[];
extern struct char_data *mob_proto;
extern int supress_godsapply;



int  check_charmee(struct char_data *ch, struct char_data *victim, int spellnum);
int  slot_for_char1(struct char_data *ch, int slotnum);
int  attack_best(struct char_data *ch, struct char_data *victim);
void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
byte extend_saving_throws(int class_num, int type, int level);
void cast_reaction(struct char_data *victim, struct char_data *caster, int spellnum);
void alterate_object(struct obj_data *obj, int dam, int chance);


// local functions
void go_flee(struct char_data * ch, bool mode);
void perform_mag_groups(int level, struct char_data * ch, struct char_data * tch, int spellnum, int savetype);
int  check_death_trap(struct char_data *ch);
int  check_death_ice(int room, struct char_data *ch);
int  equip_in_metall(struct char_data *ch);
void battle_affect_update(struct char_data * ch);
void pulse_affect_update (CHAR_DATA * ch);

/*
 * Saving throws are now in class.c as of bpl13.
 */


/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */
//#define SpINFO spell_info[spellnum]
int spell_use_success1(struct char_data *ch, int spellnum)
{int prob = 100;
 if  (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
     return (TRUE);

        prob = int_app[GET_REAL_INT(ch)].spell_aknowlege + wis_app[GET_REAL_WIS(ch)].spell_success;

       if ((IS_MAGIC_USER(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_MAGE)) ||
           (IS_CLERIC(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_CLERIC)) ||
           (IS_TAMPLIER(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_TAMPLIER)) ||
           (IS_DRUID(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_DRUID)))
          prob += 10;
       if (IS_MAGE(ch) && equip_in_metall(ch))
          prob -= 50;
//       break;

 prob = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_SUCCESS,prob);
 prob -=100;
 return (prob);
}

int calc_anti_savings(struct char_data *ch, int spellnum)
{ int modi = 0;

  if (WAITLESS(ch))
     modi = 150;
  else
  if (GET_GOD_FLAG(ch, GF_GODSLIKE))
     modi = 50;
  else
  if (GET_GOD_FLAG(ch, GF_GODSCURSE))
     modi = -50;
  else
     modi = GET_CAST_SUCCESS(ch);
     
     modi -= GET_USTALOST(ch);
//  log("[EXT_APPLY] Name==%s modi==%d",GET_NAME(ch), modi);
 modi += spell_use_success1(ch, spellnum);
  return modi;
}

int general_savingthrow(struct char_data * ch, int type, int ext_apply)
{
  /* NPCs use warrior tables according to some book */
  int save;
  int class_sav = GET_CLASS(ch);
  int luck_faktor = GET_LEVEL(ch)/6 + GET_REMORT(ch)/2; 
  
  luck_faktor += number(0,MAX(1,(GET_REAL_CHA(ch)-8)/4));
  if (luck_faktor > number(1,100))
	  return (TRUE);
  else 
  if (number(1,95+luck_faktor) < 6 && type != SAVING_BREATH)
	  return (FALSE);

  if (IS_NPC(ch))
     {if (class_sav < CLASS_HUMAN || class_sav >= CLASS_LAST_NPC)
         class_sav = CLASS_WARRIOR;
     }
  else
  if (class_sav < 0 || class_sav >= NUM_CLASSES)
     class_sav = CLASS_WARRIOR;

 save  = extend_saving_throws(class_sav, type, GET_LEVEL(ch));
 

   
  if (PRF_FLAGGED(ch,PRF_AWAKE))
      save -= GET_SKILL(ch,SKILL_AWAKE);


  switch(type)
  {case SAVING_BASIC:
   case SAVING_CRITICAL:
       save -= dex_app[GET_REAL_DEX(ch)].reaction;
	   save += con_app[GET_REAL_CON(ch)].critic_saving;
	   break;
   case SAVING_SPELL:
	   save += wis_app[GET_REAL_WIS(ch)].char_savings;
        if (IS_WEDMAK(ch)) 
         save -= GET_LEVEL(ch);
	   break;
   case SAVING_WILL://SAVING_PARA - заменил на вилл
	   save += con_app[GET_REAL_CON(ch)].poison_saving;
	   save += wis_app[GET_REAL_WIS(ch)].char_savings;	
	   break;
   case SAVING_REFLEX://приведение к единому пониманию сейвов SAVING_BREATH
	   if ((save > 0) && (GET_CLASS(ch) == CLASS_THIEF))
			save >>= 1;
	   save -= dex_app[GET_REAL_DEX(ch)].reaction;		
	   save -= con_app[GET_REAL_CON(ch)].critic_saving;
	   save += 10;
	   save -= GET_REAL_SIZE(ch);

	   if (IS_WEDMAK(ch)) 
           save += GET_LEVEL(ch) * 2 / 3;	  
	   break;
   case SAVING_STABILITY:// SAVING_PETRI
       save += con_app[GET_REAL_CON(ch)].affect_saving;	
	   break;
   default:
	   break;
   }

  	// Ослабление магических атак
	if (type != SAVING_REFLEX) {
		if ((save > 0) &&
		    (AFF_FLAGGED(ch, AFF_AIRAURA) || AFF_FLAGGED(ch, AFF_FIREAURA) || AFF_FLAGGED(ch, AFF_ICEAURA)))
			save >>= 1;
	}

	 
  save -= GET_REMORT(ch)*10;  
  save -= GET_LEVEL(ch);
  save += GET_SAVE(ch, type);
  save += ext_apply;

  if (IS_GOD(ch))
     save = -150;
  else
  if (GET_GOD_FLAG(ch, GF_GODSLIKE))
     save -= 50;
  else
  if (GET_GOD_FLAG(ch, GF_GODSCURSE))
     save += 50;
 // if (!IS_NPC(ch))
//     log("[SAVING] Name==%s type==%d save==%d ext_apply==%d",GET_NAME(ch),type,save, ext_apply);
  /* Throwing a 0 is always a failure. */
  if (MAX(0, save) <= number(1, 100))
       return (TRUE);
  
  /* Oops, failed. Sorry. */
  return (FALSE);
}


//Функия уменьшающая время действия во время боя

void show_spell_off(int aff, char_data *ch)
{
  if (!IS_NPC(ch) && PLR_FLAGGED(ch,PLR_WRITING))
    return;
  
   send_to_char(spell_wear_off_msg[aff], ch);
   send_to_char("\r\n", ch);
  
}

void battle_affect_update(struct char_data *ch)
{
  struct affected_type *af, *next;

  supress_godsapply = TRUE;
  for (af = ch->affected; af; af = next)
      {next         = af->next;
       if (!IS_SET(af->battleflag,AF_BATTLEDEC))
          continue;
       if (af->duration > 0)
          { 
              af->duration -= MIN(af->duration, SECS_PER_MUD_HOUR / SECS_PER_PLAYER_AFFECT);
          }
       else
       if (af->duration == -1)	/* No action */
          af->duration = -1;	    /* GODs only! unlimited */
       else
         {if ((af->type > 0) && (af->type <= MAX_SPELLS))
             {if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))
                 {if (af->type > 0 &&
                      af->type <= LAST_USED_SPELL &&
                      *spell_wear_off_msg[af->type])
                     {show_spell_off(af->type, ch);
	             }
	         }
	     }
	  affect_remove(ch, af);
         }
      }
  supress_godsapply = FALSE;
  //log("[BATTLE_AFFECT_UPDATE->AFFECT_TOTAL] Start");
  affect_total(ch);
  //log("[BATTLE_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
}

void pulse_affect_update (CHAR_DATA * ch)
{
  AFFECT_DATA *af, *next;
  bool pulse_aff = FALSE;

  if (FIGHTING (ch))
    return;

  supress_godsapply = TRUE;
  for (af = ch->affected; af; af = next)
    { next = af->next;
      if (!IS_SET (af->battleflag, AF_PULSEDEC))
	             continue;
      pulse_aff = TRUE;
      if (af->duration > 0)
	{
	    af->duration--;
	}
    else
		if (af->duration == -1)	// No action
			af->duration = -1;	// GODs only! unlimited
		else
			{ if ((af->type > 0) && (af->type <= MAX_SPELLS))
				{ if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))
					{ if (af->type > 0 && af->type <= LAST_USED_SPELL && *spell_wear_off_msg[af->type])
						 show_spell_off (af->type, ch);
					}
				}
			affect_remove (ch, af);
			}
   }
  supress_godsapply = FALSE;
  if (pulse_aff)
    affect_total (ch);
}


/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(struct char_data * ch, int item0, int item1, int item2,
		      int extract, int verbose)
{
  struct obj_data *tobj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

  for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
    if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
      obj0 = tobj;
      item0 = -1;
    } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
      obj1 = tobj;
      item1 = -1;
    } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
      obj2 = tobj;
      item2 = -1;
    }
  }
  if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
    if (verbose) {
      switch (number(0, 2)) {
      case 0:
	send_to_char("На Вашем лице ростет большая бородавка.\r\n", ch);
	break;
      case 1:
	send_to_char("Ваши волосы выпадают клочками.\r\n", ch);
	break;
      case 2:
	send_to_char("Огромная мозоль растет на Вашем большом пальце ноги.\r\n", ch);/*A huge corn develops on your big toe.*/
	break;
      }
    }
    return (FALSE);
  }
  if (extract) {
    if (item0 < 0) {
      obj_from_char(obj0);
      extract_obj(obj0);
    }
    if (item1 < 0) {
      obj_from_char(obj1);
      extract_obj(obj1);
    }
    if (item2 < 0) {
      obj_from_char(obj2);
      extract_obj(obj2);
    }
  }
  if (verbose) {
    send_to_char("A puff of smoke rises from your pack.\r\n", ch);
    act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  return (TRUE);
}


int mag_item_ok(struct char_data *ch, struct obj_data *obj, int spelltype)
{  int num = 0;

   if ((!IS_SET(GET_OBJ_SKILL(obj), ITEM_RUNES) && spelltype == SPELL_RUNES) ||
      (IS_SET(GET_OBJ_SKILL(obj), ITEM_RUNES) && spelltype != SPELL_RUNES))
      return (FALSE);

   if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES) &&
       GET_OBJ_VAL(obj,2) <= 0)
      return (FALSE);

   if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LAG))
      { num = 0;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG1s))
           num += 1;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG2s))
           num += 2;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG4s))
           num += 4;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG8s))
           num += 8;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG16s))
           num += 16;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG32s))
           num += 32;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG64s))
           num += 64;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LAG128s))
           num += 128;
        if (GET_OBJ_VAL(obj,3) + num - 5 * GET_REMORT(ch) >= time(NULL))
           return (FALSE);
      }

   if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LEVEL))
      { num = 0;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL1))
           num += 1;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL2))
           num += 2;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL4))
           num += 4;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL8))
           num += 8;
        if (IS_SET(GET_OBJ_VAL(obj,0), MI_LEVEL16))
           num += 16;
        if (GET_LEVEL(ch) + GET_REMORT(ch) < num)
           return (FALSE);
      }

 return (TRUE);
}

void extract_item(struct char_data *ch, struct obj_data *obj, int spelltype)
{  int extract = FALSE;
   if (!obj)
      return;

   GET_OBJ_VAL(obj,3) = time(NULL);

   if (IS_SET(GET_OBJ_SKILL(obj),ITEM_CHECK_USES))
      {GET_OBJ_VAL(obj,2)--;
       if (GET_OBJ_VAL(obj,2) <= 0 && IS_SET(GET_OBJ_SKILL(obj), ITEM_DECAY_EMPTY))
          extract = TRUE;
      }
   else
   if (spelltype != SPELL_RUNES)
      extract = TRUE;

   if (extract)
      {if (spelltype == SPELL_RUNES)
          act("$o рассыпал$U у Вас в руках.",FALSE,ch,obj,0,TO_CHAR);
       obj_from_char(obj);
       extract_obj(obj);
      }
}

int check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int extract)
{
  struct obj_data *obj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL, *obj3 = NULL, *objo=NULL;
  int    item0=-1, item1=-1, item2=-1, item3=-1;
  int    create=0, obj_num=-1, skillnum=-1, percent=0, num=0;
  struct spell_create_item *items;

  if (spellnum <= 0 || spellnum > MAX_SPELLS)
     return (FALSE);
  if (spelltype == SPELL_ITEMS)
     {items = &spell_create[spellnum].items;
     }
  else
  if (spelltype == SPELL_POTION)
     {items    = &spell_create[spellnum].potion;
      skillnum = SKILL_CREATE_POTION;
      create   = 1;
     }
  else
  if (spelltype == SPELL_WAND)
     {items    = &spell_create[spellnum].wand;
      skillnum = SKILL_CREATE_WAND;
      create   = 1;
     }
  else
  if (spelltype == SPELL_SCROLL)
     {items    = &spell_create[spellnum].scroll;
      skillnum = SKILL_CREATE_SCROLL;
      create   = 1;
     }
  else
  if (spelltype == SPELL_RUNES)
     {items    = &spell_create[spellnum].runes;
     }
  else
     return (FALSE);

  if (((spelltype == SPELL_RUNES || spelltype == SPELL_ITEMS) &&
       (item3=items->rnumber)  +
       (item0=items->items[0]) +
       (item1=items->items[1]) +
       (item2=items->items[2]) < -3) ||
      ((spelltype == SPELL_SCROLL || spelltype == SPELL_WAND || spelltype == SPELL_POTION) &&
        ((obj_num = items->rnumber) < 0 ||
         (item0 = items->items[0]) +
         (item1 = items->items[1]) +
         (item2 = items->items[2]) < -2))
     )
     return (FALSE);

  for (obj = ch->carrying; obj; obj = obj->next_content)
      {if (item0 >= 0 && (percent = real_object(item0)) >= 0 &&
           GET_OBJ_VAL(obj,1) == GET_OBJ_VAL(obj_proto+percent,1) &&
           mag_item_ok(ch,obj,spelltype))
          {obj0  = obj;
           item0 = -2;
           objo  = obj0;
           num++;
          }
       else
       if (item1 >= 0 && (percent = real_object(item1)) >= 0 &&
           GET_OBJ_VAL(obj,1) == GET_OBJ_VAL(obj_proto+percent,1) &&
           mag_item_ok(ch,obj,spelltype))
          {obj1  = obj;
           item1 = -2;
           objo  = obj1;
           num++;
          }
       else
       if (item2 >= 0 && (percent = real_object(item2)) >= 0 &&
           GET_OBJ_VAL(obj,1) == GET_OBJ_VAL(obj_proto+percent,1) &&
           mag_item_ok(ch,obj,spelltype))
          {obj2  = obj;
           item2 = -2;
           objo  = obj2;
           num++;
          }
       else
       if (item3 >= 0 && (percent = real_object(item3)) >= 0 &&
           GET_OBJ_VAL(obj,1) == GET_OBJ_VAL(obj_proto+percent,1) &&
           mag_item_ok(ch,obj,spelltype))
          {obj3  = obj;
           item3 = -2;
           objo  = obj3;
           num++;
          }
      };

//  log("%d %d %d %d",items->items[0],items->items[1],items->items[2],items->rnumber);
//  log("%d %d %d %d",item0,item1,item2,item3);
  if (!objo ||
      (items->items[0] >= 0 && item0 >= 0) ||
      (items->items[1] >= 0 && item1 >= 0) ||
      (items->items[2] >= 0 && item2 >= 0) ||
      (items->rnumber  >= 0 && item3 >= 0))
     return (FALSE);
//  log("OK!");
  if (extract)
     {if (spelltype == SPELL_RUNES)
         strcpy(buf,"Вы сложили ");
      else
         strcpy(buf,"Вы взяли ");
      if (create)
         {if (!(obj = read_object(obj_num, VIRTUAL)))
             return (FALSE);
          else
             {percent = number(1,100);
              if (skillnum > 0 && percent > train_skill(ch,skillnum,percent,0))
                 percent = -1;
             }
         }
      if (item0 == -2)
         {strcat(buf,CCWHT(ch, C_NRM));
          strcat(buf,obj0->short_vdescription);
          strcat(buf,", ");
         }
      if (item1 == -2)
         {strcat(buf, CCWHT(ch, C_NRM));
          strcat(buf,obj1->short_vdescription);
          strcat(buf,", ");
         }
      if (item2 == -2)
         {strcat(buf, CCWHT(ch, C_NRM));
          strcat(buf,obj2->short_vdescription);
          strcat(buf,", ");
         }
      if (item3 == -2)
         {strcat(buf, CCWHT(ch, C_NRM));
          strcat(buf,obj3->short_vdescription);
          strcat(buf,", ");
         }
      strcat(buf, CCNRM(ch, C_NRM));
      if (create)
         {if (percent >= 0)
             {strcat(buf," и создали $3.");
              act(buf,FALSE,ch,obj,0,TO_CHAR);
              act("$n создал$y $3.",FALSE,ch,obj,0,TO_ROOM);
              obj_to_char(obj,ch);
             }
          else
             {strcat(buf," и попытались создать $3.\r\n"
                         "Ничего не вышло.");
              act(buf,FALSE,ch,obj,0,TO_CHAR);
              extract_obj(obj);
             }
         }
      else
         {if (spelltype == SPELL_ITEMS)
             {strcat(buf, "и создали магическую смесь.\r\n");
              act(buf,FALSE,ch,0,0,TO_CHAR);
              act("$n смешал$y что-то в своей ноше.\r\n"
                  "Вы почувствовали резкий запах.", TRUE, ch, NULL, NULL, TO_ROOM);
             }
          else
          if (spelltype == SPELL_RUNES)
             {sprintf(buf+strlen(buf), "котор%s вспыхнул%s ярким светом.\r\n",
                      num > 1 ? "ые" : GET_OBJ_SUF_3(objo),
                      num > 1 ? "и"  : GET_OBJ_SUF_1(objo));
              act(buf,FALSE,ch,0,0,TO_CHAR);
              act("$n сложил$y руны, которые вспыхнули ярким пламенем.",
                  TRUE, ch, NULL, NULL, TO_ROOM);
             }
         }
       extract_item(ch,obj0,spelltype);
       extract_item(ch,obj1,spelltype);
       extract_item(ch,obj2,spelltype);
       extract_item(ch,obj3,spelltype);
     }
  return (TRUE);
}

int check_recipe_values(struct char_data * ch, int spellnum, int spelltype, int showrecipe)
{
  int    item0=-1, item1=-1, item2=-1, obj_num=-1;
  struct spell_create_item *items;

  if (spellnum <= 0 || spellnum > MAX_SPELLS)
     return (FALSE);
  if (spelltype == SPELL_ITEMS)
     {items    = &spell_create[spellnum].items;
     }
  else
  if (spelltype == SPELL_POTION)
     {items = &spell_create[spellnum].potion;
     }
  else
  if (spelltype == SPELL_WAND)
     {items = &spell_create[spellnum].wand;
     }
  else
  if (spelltype == SPELL_SCROLL)
     {items = &spell_create[spellnum].scroll;
     }
  else
  if (spelltype == SPELL_RUNES)
     {items = &spell_create[spellnum].runes;
     }
  else
     return (FALSE);

  if (((obj_num = real_object(items->rnumber)) < 0 &&
       spelltype != SPELL_ITEMS && spelltype != SPELL_RUNES)  ||
      ((item0 = real_object(items->items[0])) +
       (item1 = real_object(items->items[1])) +
       (item2 = real_object(items->items[2])) < -2)
     )
     {if (showrecipe)
         send_to_char("Боги хранят в секрете этот рецепт.\n\r",ch);
      return (FALSE);
     }

  if (!showrecipe)
     return (TRUE);
  else
     {strcpy(buf,"Вы при себе должны иметь:\r\n");
      if (item0 >= 0)
         {strcat(buf, CCRED(ch, C_NRM));
          strcat(buf,obj_proto[item0].short_description);
          strcat(buf,"\r\n");
         }
      if (item1 >= 0)
         {strcat(buf, CCYEL(ch, C_NRM));
          strcat(buf,obj_proto[item1].short_description);
          strcat(buf,"\r\n");
         }
      if (item2 >= 0)
         {strcat(buf, CCGRN(ch, C_NRM));
          strcat(buf,obj_proto[item2].short_description);
          strcat(buf,"\r\n");
         }
      if (obj_num >= 0 && (spelltype == SPELL_ITEMS || spelltype == SPELL_RUNES))
         {strcat(buf, CCBLU(ch, C_NRM));
          strcat(buf,obj_proto[obj_num].short_description);
          strcat(buf,"\r\n");
         }

      strcat(buf, CCNRM(ch, C_NRM));
      if (spelltype == SPELL_ITEMS || spelltype == SPELL_RUNES)
         {strcat(buf,"для создания магии '");
          strcat(buf,spell_name(spellnum));
          strcat(buf,"'.");
         }
      else
         {strcat(buf,"для создания ");
          strcat(buf, obj_proto[obj_num].short_rdescription);
         }
      act(buf,FALSE,ch,0,0,TO_CHAR);
     }

  return (TRUE);
}
/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
void do_sacrifice(struct char_data *ch, int dam)
{GET_HIT(ch) = MIN(GET_HIT(ch)+MAX(1,dam), GET_REAL_MAX_HIT(ch) +  GET_LEVEL(ch)*5);
 update_pos(ch);
}

#define SpINFO   spell_info[spellnum]

int mag_damage(int level, struct char_data * ch, struct char_data * victim,
  		       int spellnum, int savetype, int use_glory)
{
  int    dam = 0, rand = 0, count=1, modi=0, ndice = 0, sdice = 0, adice = 0,
         no_savings = FALSE;
  double vych = 0;
  struct obj_data *obj=NULL;
  struct affected_type af;

  if (victim == NULL || IN_ROOM(victim) == NOWHERE ||
      ch     == NULL
     )
     return (0);


      pk_agro_action(ch,victim);


//  log("[MAG DAMAGE] %s damage %s (%d)",GET_NAME(ch),GET_NAME(victim),spellnum);

    //Magic resistance
if (ch != victim          &&
    spellnum < MAX_SPELLS &&                //тут разобраться, что савинг петри на сопротивление магии что сопротивление аффектам!
    ((GET_SAVE(victim, SAVING_PETRI) + GET_DRESIST(victim)) -!
	 MAX(GET_LEVEL(ch) - GET_LEVEL(victim),0)) > number(1,100))  
   { act("Заклинание рассеялось около $r.",FALSE,victim,0,0,TO_ROOM);
     act("Заклинание не подействовало на Вас.",FALSE,victim,0,0,TO_CHAR);
     return (0);  
 }


  // Magic glass
  if (ch != victim &&
      spellnum < MAX_SPELLS &&
      ((AFF_FLAGGED(victim,AFF_MAGICGLASS) && number(1,100) < GET_LEVEL(victim)
       ) ||
       (IS_GOD(victim) && (IS_NPC(ch) || GET_LEVEL(victim) > GET_LEVEL(ch))
       )
      )
     )
    {act("&WМагическое зеркало $R отразило Вашу магию!&n",FALSE,ch,0,victim,TO_CHAR);
     act("&WВаше магическое зеркало отразило магию $r!&n",FALSE,ch,0,victim,TO_VICT);
     return(mag_damage(level,ch,ch,spellnum,savetype));
    }

  if (SPELL_FIRE_BREATH <= spellnum  && spellnum <= SPELL_LIGHTNING_BREATH)
	  modi  = 110;
  else
  if (ch != victim)
     {modi  = calc_anti_savings(ch, spellnum);
      modi  += wis_app[GET_REAL_WIS(victim)].char_savings;
      modi  += number(GET_REAL_CHA(ch)/8,GET_REAL_CHA(ch)/2);
      modi  += MAX(0, GET_LEVEL(ch) - GET_REAL_INT(ch));
      modi -= 25;
      modi += GET_LEVEL(ch);
     }

 /* if (IS_WARRIOR(victim) && 
	  clan[find_clan_by_id(GET_CLAN(victim))].alignment == 1 &&
	  GET_CLAN_RANK(victim) &&
	  IS_NPC(ch))
	  modi -= (MAX(0,GET_LEVEL(victim) - 15) * 4); */

  switch (spellnum)
  {
  /******** ДЛЯ ВСЕХ МАГОВ *********/
  // магическая стрела - для всех с 1го левела 1го круга(8 слотов)
  // *** мин 5 макс 63 (360)
  // нейтрал
  case SPELL_MAGIC_MISSILE:
    savetype = SAVING_REFLEX;
    ndice = 2;
    sdice = 4;
    adice = level / 5;
    count = (level+9) / 6;
    break;
  // ледяное прикосновение - для всех с 7го левела 3го круга(7 слотов)
  // *** мин 29.5 макс 55.5  (390)
  // нейтрал
  case SPELL_CHILL_TOUCH:
    savetype = SAVING_STABILITY;
    ndice = 12;
    sdice = 2;
    adice = level;
    break;
   // кислота - для всех с 18го левела 5го круга (6 слотов)
   // *** мин 48 макс 70 (420)
   // нейтрал
  case SPELL_ACID:
	savetype = SAVING_REFLEX;
    obj = NULL;
    
	if (IS_NPC(victim))
       {rand = number(1,50);
        if (rand <= WEAR_BOTHS)
           obj = GET_EQ(victim, rand);
        else
           for (rand -= WEAR_BOTHS, obj = victim->carrying; rand && obj;
               rand--, obj = obj->next_content);
       }
    if (obj && !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
       {ndice = 6;
        sdice = 10;
        adice = level;//добавить: кислота, пущенная ИМЯ, покрыла ...
        act("Кислота покрыла $3.",FALSE,victim,obj,0,TO_CHAR);
        alterate_object(obj,number(level*2,level*4),100);
       }
    else
       {ndice = 6;
        sdice = 15;
        adice = (level - 18) * 2;
       }
if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  || !general_savingthrow(victim, savetype, modi+10))  &&
        !WAITLESS(victim)
       )
       { act("Кислота разъедает Ваше тело.",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_FAST_REGENERATION;
        af.location  = APPLY_HITREG;
        af.modifier  = -50;
        af.duration  = pc_duration(victim,2,0,0,0,0);
        af.bitvector = 0;
        af.battleflag= 0;
        affect_join(victim,&af,FALSE,FALSE,FALSE,FALSE);
        
       }
	    break;

  // землетрясение чернокнижники 22 уровень 7 круг (4)
  // *** мин 48 макс 60 (240)
  // нейтрал
 /* if (ch == victim ||
       ((rand = number(1,100)) < GET_LEVEL(ch) &&
        !general_savingthrow(victim, SAVING_ROD,modi)
       )
      )*/
  case SPELL_EARTHQUAKE:
    savetype = SAVING_REFLEX + EARTH_RESISTANCE;
    ndice = 8;
    sdice = 15;
    adice = (level - 22) * 2;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         GET_MOB_HOLD(victim)               ||
         AFF_FLAGGED(victim, AFF_HOLDALL)	||
		 !general_savingthrow(victim, savetype, modi-20)
		 //number(0,100) <= 30
        ) &&
        GET_POS(victim) > POS_SITTING &&
        !WAITLESS(victim)
       )
       {act("&c$v повалило на землю.&n",FALSE,victim,0,0,TO_ROOM);
        act("&cВас повалило на землю.&n",FALSE,victim,0,0,TO_CHAR);
        GET_POS(victim) = POS_SITTING;
        update_pos(victim);
        WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
       }
    break;
  // Высосать жизнь - некроманы - уровень 18 круг 6й (5)
  // *** мин 54 макс 66 (330)
  case SPELL_SACRIFICE:
    if (victim == ch)
       return (0);
    if (!WAITLESS(victim))
       {savetype = MIND_RESISTANCE;
        ndice = 8;
        sdice = 8;
        adice = level;
       }
    break;

  /********** ДЛЯ ФРАГЕРОВ **********/
  // горящие руки - с 1го левела 1го круга (8 слотов)
  // *** мин 21 мах 30 (240)
  // ОГОНЬ
  case SPELL_BURNING_HANDS:
    savetype = SAVING_STABILITY;
    ndice = 10;
    sdice = 3;
    adice = (level + 2) / 3;//SAVING_STABILITY - стойкость
   if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  || !general_savingthrow(victim, SAVING_STABILITY, modi-90))  &&
        !WAITLESS(victim)
       )
       {act("&rПламя оставило Вам небольшой ожог.&n",FALSE,victim,0,0,TO_CHAR);
        
        af.type      = SPELL_FAST_REGENERATION;
        af.location  = APPLY_HITREG;
        af.modifier  = -20;
        af.duration  = pc_duration(victim,2,0,0,0,0);
        af.bitvector = 0;
       
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }
    break;
  // обжигающая хватка - с 4го левела 2го круга (8 слотов)
  // *** мин 36 макс 45 (360) - возможно дать следаку или еще кому из их команды
  // ОГОНЬ
   case SPELL_SHOCKING_GRASP:
    savetype = SAVING_REFLEX;
    ndice = 10;
    sdice = 6;
    adice = (level + 2) / 3;
    break;
  // молния - с 7го левела 3го круга (7 слотов)
  // *** мин 18 - макс 45 (315)
  // ВОЗДУХ
  case SPELL_LIGHTNING_BOLT:
    savetype = SAVING_REFLEX;
    ndice = level;
    sdice = 5;
    adice = level/2;
    count = 1;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  || !general_savingthrow(victim, SAVING_STABILITY, modi-70))  &&
        !WAITLESS(victim)
       )
       {act("&K$n потерял$y зрение.&n",FALSE,victim,0,0,TO_ROOM);
        act("&KЯркая вспышка молнии ослепила Вас.&n",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_BLINDNESS;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.duration  = pc_duration(victim,2,0,0,0,0);
        af.bitvector = AFF_BLIND;
        af.battleflag= AF_BATTLEDEC;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }
    break;
  // яркий блик - с 7го 3го круга (7 слотов)
  // *** мин 33 - макс 40 (280)
  // ОГОНЬ
  case SPELL_SHINEFLASH:
    savetype = SAVING_STABILITY;//стойкость
    ndice = 10;
    sdice = 5;
    adice = (level + 2) / 3;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         number(0,100) <= GET_LEVEL(ch) * 3
        )                               &&
        !AFF_FLAGGED(victim, AFF_BLIND) &&
        !WAITLESS(victim)
       )
       {act("&K$n ослеп$q!&n",FALSE,victim,0,0,TO_ROOM);
        act("&KВы ослепли!&n",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_BLINDNESS;
        af.location  = APPLY_HITROLL;
        af.modifier  = 0;
        af.duration  = pc_duration(victim,2,level+7,8,0,0);
        af.bitvector = AFF_BLIND;
        af.battleflag= AF_BATTLEDEC;
        affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
       }
    break;
  // шаровая молния - с 10го левела 4го круга (6 слотов)
  // *** мин 35 макс 55 (330)
  // ВОЗДУХ
  case SPELL_CALL_LIGHTNING:
    savetype = SAVING_REFLEX;//реакция
    ndice = 3;
    sdice = 4;
    adice = level;
    break;
  // огненная стрела - уровень 14 круг 5 (6 слотов)
  // *** мин 72 макс 102 (360)
  // ОГОНЬ
  case SPELL_COLOR_SPRAY:
    savetype = SAVING_STABILITY + AIR_RESISTANCE;//стойкость
    ndice = 10 + level/3;
    sdice = 14;
    adice = level/2;
   if ((GET_GOD_FLAG(victim, GF_GODSCURSE) 							||
     WAITLESS(ch)  													||
     !general_savingthrow(victim, SAVING_REFLEX, modi-20-40/adice))	&&
     !WAITLESS(victim))
	{   af.type      = SPELL_COLOR_SPRAY;
        af.location  = 0;
        af.modifier  = 0;
        af.duration  = pc_duration(victim,2+adice/3,0,0,0,0);
	    af.battleflag= 1;

  switch(number(0, 11))
	{  case 1:
		act("&YОранжевая искра попала в Вас, Вы потеряли сознание.&n",FALSE,victim,0,0,TO_CHAR);
		act("&Y$n вспыхнул$y оранжевым светом и упал$y на землю!&n",FALSE,victim,0,0,TO_ROOM);
        af.bitvector = AFF_STOPFIGHT;
		GET_POS(victim) = POS_STUNNED;
		break;
   
		case 2:
		act("&mФиолетовая искра попала в Вас, Вы потеряли сознание.&n",FALSE,victim,0,0,TO_CHAR);
		act("&m$n вспыхнул$y фиолетовым светом и упал$y на землю!&n",FALSE,victim,0,0,TO_ROOM);
        af.bitvector = AFF_STOPFIGHT;
		GET_POS(victim) = POS_SLEEPING;
		break;

		case 3:
		act("&cЗеленая искра парализовала Вас.&n",FALSE,victim,0,0,TO_CHAR);
		act("&c$n вспыхнул$y зеленым светом и застыл$y на месте.&n",FALSE,victim,0,0,TO_ROOM);
        af.bitvector = AFF_HOLDALL;
		break;

	case 4: case 5: case 6:
		act("&KКрасная искра попала в Вас, Вы потеряли способность разговаривать.&n",FALSE,victim,0,0,TO_CHAR);
		act("&K$n вспыхнул$y красным светом и замолчал$y.&n",FALSE,victim,0,0,TO_ROOM);
        af.bitvector = AFF_SIELENCE;
		break;
   
	case 7: case 8: case 9: case 10:
		act("&KЖелтая искра ослепила Вас.&n",FALSE,victim,0,0,TO_CHAR);
		act("&K$n вспыхнул$y желтым светом и ослеп$q.&n",FALSE,victim,0,0,TO_ROOM);
        af.type      = SPELL_BLINDNESS;
		af.bitvector = AFF_BLIND;
		break;
   
	default:
		act("&KСиняя искра замедлила Ваши движения.&n",FALSE,victim,0,0,TO_CHAR);
		act("&K$n вспыхнул$y синим светом и покрыл$u льдом.&n",FALSE,victim,0,0,TO_ROOM);
        af.bitvector = AFF_SLOW;
		break;
	}
	  affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
	  adice = MAX(level, level / (adice + 4) * 10);
	}

	break;



  // сфера холода - уровень 14 круг 5 (6 слотов)
  // *** мин 44 макс 70 (360)
  // ВОДА
  case SPELL_CONE_OF_COLD:
    savetype = SAVING_STABILITY;//стойкость
    ndice = level;
    sdice = 7;
    adice = level / 2;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  || !general_savingthrow(victim, WATER_RESISTANCE, modi-50))  &&//воля, что бы не замедляло.
        !WAITLESS(victim)
       )
       {
    if (affected_by_spell(victim, SPELL_HASTE))
       {affect_from_char(victim, SPELL_HASTE);
       // success = FALSE;
        break;
       }
        act("&KВаши движения замедлились.&n",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_SLOW;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.duration  = pc_duration(victim,10,0,0,0,0);
        af.bitvector = AFF_SLOW;
        af.battleflag= AF_BATTLEDEC;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }
    break;
  // Огненный шар - уровень 18 круг 6 (5 слотов)
  // *** мин 66 макс 80 (400)
  // ОГОНЬ
  case SPELL_FIREBALL:
    savetype = SAVING_REFLEX;
    ndice = (level * 2) / 3;
    sdice = 6;
    adice = 1;
if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  || !general_savingthrow(victim, SAVING_CRITICAL, modi-50))  &&
        !WAITLESS(victim)
       )
       {act("&rПламя оставило Вам сильный ожог.&n",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_FAST_REGENERATION;
        af.location  = APPLY_HITREG;
        af.modifier  = -65;
        af.duration  = pc_duration(victim,2,0,0,0,0);
        af.bitvector = 0;
        af.battleflag= 0;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }
    break;
  // слонечный луч массфраг масслеп,сильно по нежити
  // ***  мин 38 макс 50 (250)

  
    case SPELL_FIREBLAST:
    savetype = SAVING_STABILITY;//стойкость
    ndice = 1;
    sdice = 1;
    adice = level * 2 / 3;
      if(IS_NPC(victim) &&  IS_UNDEADS(victim))
 { 
	if ((GET_GOD_FLAG(victim, GF_GODSCURSE) || 
		WAITLESS(ch)  ||
		!general_savingthrow(victim, savetype ,modi - GET_LEVEL(victim)))  &&
		!WAITLESS(victim))
	{   ndice = level*2;
		sdice = 10;
		adice = level+50;
	}
else
		{ndice = level;
		 sdice = 10;
		 adice = level+10;
		}
	}
  else
	{  
	  if ((GET_GOD_FLAG(victim, GF_GODSCURSE) || 
         WAITLESS(ch)  || !general_savingthrow(victim, SAVING_REFLEX, modi-20))  &&
        !WAITLESS(victim)
       )
       {act("&KЯркий солнечный свет ослепил Вас.&n",FALSE,victim,0,0,TO_CHAR);
	    act("&KЯркий солнечный свет ослепил $v&n.",FALSE,victim,0,0,TO_ROOM);
        af.type      = SPELL_BLINDNESS;
        af.location  = APPLY_AC;
        af.modifier  = 50;
		af.duration  = pc_duration(victim,2,0,0,0,0);
		af.bitvector = AFF_BLIND;
		af.battleflag= 1;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
    }
 }
    break;
  // метеоритный шторм - уровень 22 круг 7 (4 слота)
  // *** мин 66 макс 80  (240)
  // нейтрал, ареа
  case SPELL_METEORSTORM:
    savetype = SAVING_REFLEX;
	ndice = 10;
    sdice = 1;  
    adice = 1;
	
	if(IS_NPC(victim) && IS_UNDEADS(victim))  // лечит мертвых до полного
      GET_HIT(victim) = GET_REAL_MAX_HIT(victim);
      if(GET_LEVEL(victim)< GET_LEVEL(ch)/2)  // если уровень в два раза ниже - то хиты цели
	  adice = GET_HIT(victim);
	  else
	  if(GET_LEVEL(victim)< GET_LEVEL(ch)/2-4)// если уровень в два раза ниже-4 то половина хитов
      adice =  GET_HIT(victim)/2;
  break;
  // поток молний - уровень 22 круг 7 (4 слота)
  // *** мин 76 макс 100 (400)
  // ВОЗДУХ, ареа
  case SPELL_CHAIN_LIGHTNING:
	savetype = SAVING_STABILITY;//стойкость
    ndice = level;
    sdice = 7;
    adice = 5;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE)				||
          WAITLESS(ch)							||
	  (!general_savingthrow(victim, SAVING_CRITICAL, modi + level * 2 - 40) 		&& 
	  number(1,4) == 1))					 	&&
         !WAITLESS(victim))
        {   WAIT_STATE(victim, number(1,4) * PULSE_VIOLENCE);
          act("&CРазряд молнии ошеломил Вас!&n",FALSE,victim,0,0,TO_CHAR);
          act("&CРазряд молнии ошеломил $v.&n",FALSE,victim,0,0,TO_ROOM);
	}    
    break;
  // гнев богов - уровень 26 круг 8 (2 слота)
  // *** мин 226 макс 250 (500)
  // ВОДА
  case SPELL_IMPLOSION:
	savetype = SAVING_WILL;// воля
    ndice = 1;
    sdice = 1;
    adice = 180;
    break;
  // ледяной шторм - 26 левела 8й круг (2)
  // *** мин 55 макс 75 (150)
  // ВОДА, ареа
  case SPELL_ICESTORM:
    savetype = SAVING_STABILITY;//стойкость
    ndice = 7;
    sdice = 7;
    adice = (level - 18) * 3;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  || !general_savingthrow(victim, SAVING_CRITICAL, modi - 10))  &&//здоровье
        !WAITLESS(victim)
       )
       {act("$v оглушило.",FALSE,victim,0,0,TO_ROOM);
        act("Вас оглушило.",FALSE,victim,0,0,TO_CHAR);
        WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
        af.type      = SPELL_ICESTORM;
        af.location  = 0;
        af.modifier  = 0;
        af.duration  = pc_duration(victim,2,0,0,0,0);
        af.bitvector = AFF_STOPFIGHT;
        af.battleflag= AF_BATTLEDEC | AF_PULSEDEC;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }
    break;
  // суд богов - уровень 28 круг 9 (1 слот)
  // *** мин 188 макс 200 (200)
  // ВОЗДУХ, ареа
  case SPELL_ARMAGEDDON:
    savetype = SAVING_WILL;//воля
    ndice = level;
    sdice = 7;
    adice = level / 2;
    break;



  /******* ХАЙЛЕВЕЛ СУПЕРДАМАДЖ МАГИЯ ******/
  // истощить энергию - круг 28 уровень 9 (1)
  // для всех
  case SPELL_ENERGY_DRAIN:
   dam        = -1;
   no_savings = TRUE;
   if (ch == victim ||
       (number(1,100) <= 33 &&
        !general_savingthrow(victim, SAVING_WILL, modi)
       )
      )
      {int i;
       for (i = 0; i <= MAX_SPELLS; GET_SPELL_MEM(victim,i++) = 0);
      };
   break;
  // каменное проклятие - круг 28 уровень 9 (1)
  // для всех
  case SPELL_STUNNING:
   dam        = -1;
   no_savings = TRUE;
   if (ch == victim ||
       ((rand = number(1,100)) < GET_LEVEL(ch) &&
        !general_savingthrow(victim, SAVING_WILL, modi)
       )
      )
      {dam = MAX(1, GET_HIT(victim) + 1);
      }
   else
   if (rand > 50 && rand < 60 && !WAITLESS(ch))
      {send_to_char("Ваша магия обратилась против Вас.\r\n",ch);
       GET_HIT(ch) = 1;
      }
   break;
  // круг пустоты - круг 28 уровень 9 (1)
  // для всех
  case SPELL_VACUUM:
   dam        = -1;
   no_savings = TRUE;
   if (ch == victim ||
       (number(1,100) <= 33 &&
        !general_savingthrow(victim, SAVING_WILL, modi)
       )
      )
      {dam = MAX(1, GET_HIT(victim) + 1);
      };
   break;


  /********* СПЕЦИФИЧНАЯ ДЛЯ КЛЕРИКОВ МАГИЯ **********/
  case SPELL_DAMAGE_LIGHT:
    savetype = SAVING_CRITICAL;//здоровье
    ndice = 3;
    sdice = 8;
    adice = (level) / 3;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  || !general_savingthrow(victim, SAVING_WILL, modi - 70))  &&
        !WAITLESS(victim)
       )
       {act("&rУ $r открылась небольшая рана!&n",FALSE,victim,0,0,TO_ROOM);
	    act("&rУ Вас начала кровоточить небольшая рана!&n",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_FAST_REGENERATION;
        af.location  = APPLY_HITREG;
        af.modifier  = -40;
        af.duration  = pc_duration(victim,2,0,0,0,0);
        af.bitvector = 0;
        af.battleflag= 0;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }
    break;
  case SPELL_DAMAGE_SERIOUS:
    savetype = SAVING_CRITICAL;//здоровье
    ndice = 6;
    sdice = 8;
    adice = (level) / 2;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE) ||
         WAITLESS(ch)  || !general_savingthrow(victim, SAVING_WILL, modi - 80))  &&
        !WAITLESS(victim)
       )
       {act("&rУ $r открылась ужасная рана!&n",FALSE,victim,0,0,TO_ROOM);
	    act("&rУ Вас появилась ужасная рана!&n",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_FAST_REGENERATION;
        af.location  = APPLY_HITREG;
        af.modifier  = -80;
        af.duration  = pc_duration(victim,2,0,0,0,0);
        af.bitvector = 0;
        af.battleflag= 0;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }
    break;
  case SPELL_DAMAGE_CRITIC:
    savetype = SAVING_CRITICAL;//здоровье
    ndice = 12;
    sdice = 8;
    adice = (level + 10) / 2;
    if ((GET_GOD_FLAG(victim, GF_GODSCURSE)				|| 
		   WAITLESS(ch)						||
		 (!general_savingthrow(victim, SAVING_WILL, modi - 90) 	&&
		 number(1,3) == 1					&& 
		 !IS_UNDEADS(victim)					&& 
		 !AFF_FLAGGED(victim,AFF_HOLD))				&&
        	!WAITLESS(victim))
       )
       {act("&c$v парализовало от боли!&n",FALSE,victim,0,0,TO_ROOM);
	    act("&cВас парализовало от боли!&n",FALSE,victim,0,0,TO_CHAR);
        af.type      = SPELL_HOLD;
        af.location  = 0;
        af.modifier  = 0;
        af.duration  = pc_duration(victim,number(1,3),0,0,0,0);
        af.battleflag= 1;
        af.bitvector = AFF_HOLD;
        affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
       }  
  break;
  case SPELL_DISPEL_EVIL:
    ndice = 4;
    sdice = 4;
    adice = level;
    if (ch != victim  &&
        IS_EVIL(ch)   &&
        !WAITLESS(ch) &&
        GET_HIT(ch) > 1
       )
       {send_to_char("Ваша магия обратилась против Вас.", ch);
        GET_HIT(ch) = 1;
       }
    if (!IS_EVIL(victim))
       {if (victim != ch)
           act("Боги защитили $V от Вашей магии.", FALSE, ch, 0, victim, TO_CHAR);
        return (0);
       };
    break;
  case SPELL_DISPEL_GOOD:
    ndice = 4;
    sdice = 4;
    adice = level;
    if (ch != victim  &&
        IS_GOOD(ch)   &&
        !WAITLESS(ch) &&
        GET_HIT(ch) > 1
       )
       {send_to_char("Ваша магия обратилась против Вас.", ch);
        GET_HIT(ch) = 1;
       }
    if (!IS_GOOD(victim))
       {if (victim != ch)
           act("Боги защитили $V от Вашей магии.", FALSE, ch, 0, victim, TO_CHAR);
        return (0);
       };
    break;
  case SPELL_HARM:
    savetype = SAVING_CRITICAL;//здоровье
    ndice = 10;
    sdice = 5;
    adice = (level + 4) * 5;
    break;

  case SPELL_FIRE_BREATH:
  case SPELL_LIGHTNING_BREATH:
  case SPELL_FROST_BREATH:
  case SPELL_ACID_BREATH:
	  savetype = SAVING_REFLEX;//реакция
    case SPELL_GAS_BREATH:
      savetype = SAVING_STABILITY;//стойкость
	  
    if (!IS_NPC(ch))
       return (0);

    ndice = ch->mob_specials.damnodice;
    sdice = ch->mob_specials.damsizedice/3;
    adice = (GET_REAL_DR(ch) + str_app[STRENGTH_APPLY_INDEX(ch)].todam)/3;
    break;
  case SPELL_FEAR:
    if (!general_savingthrow(victim, savetype, calc_anti_savings(ch, spellnum)) &&
        !MOB_FLAGGED(victim, MOB_NOFEAR)
       )
       {go_flee(victim, 1);
        return (0);
       }
    dam        = -1;
    no_savings = TRUE;
    break;
  case SPELL_ZNAK_IGNI:
    savetype = SAVING_REFLEX;//реакция
    ndice = 4;
    sdice = 8;
    adice = level;

        act("&YЯрко-огненная вспышка полетела в сторону $R.&n",FALSE,ch,0,victim,TO_CHAR);
	act("&rИз руки $r вырвалась ярко-огненная вспышка и ударила Вам в лицо.&n", FALSE,ch,0,victim,TO_VICT);
        act("Из руки $r вырвалась ярко-огненная вспышка и ударила в лицо $D.",TRUE,ch,0,victim,TO_NOTVICT);

    if (MOB_FLAGGED(victim,MOB_NOBLIND) ||
        ((ch != victim) &&		
		general_savingthrow(victim, SAVING_STABILITY, modi)) ||
		affected_by_spell(victim,SPELL_BLINDNESS))
		 //success = FALSE;
          break;
		
 
    af.type      = SPELL_BLINDNESS;
    af.location  = APPLY_HITROLL;
    af.modifier  = 0;
    af.duration  = pc_duration(victim,2,level,4,0,0);
    af.bitvector = AFF_BLIND;
    af.battleflag= AF_BATTLEDEC;
   	affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);

	act("&K$n потерял$y зрение!&n",FALSE,victim,0,0,TO_ROOM);
	act("&KВы потеряли зрение!&n",FALSE,victim,0,0,TO_CHAR);
	spellnum = SPELL_BLINDNESS;

    break;
  case SPELL_ZNAK_AARD:
	savetype = SAVING_STABILITY;//здоровье
    ndice = 1;
    sdice = 1;
    adice = 1;	  
  if (ch != victim &&
		 general_savingthrow(victim, SAVING_REFLEX, modi+20))
		{ send_to_char(NOEFFECT, ch);
              break;
		};

	 GET_HIT(victim) -= GET_LEVEL(ch);
		  
    if (GET_HIT(victim) > 0)
	{ if (GET_POS(victim) != POS_SITTING)
		{  act("$n отлетел$y в сторону и грохнул$u об земь!",FALSE,victim,0,0,TO_ROOM);
	         act("&rВы отлетели в сторону и грохнулись об земь!&n",FALSE,victim,0,0,TO_CHAR);
		}
	else
		{  act("$v еще сильнее вдавило в землю!",FALSE,victim,0,0,TO_ROOM);
	         act("&rВас еще сильнее вдавило в землю!&n",FALSE,victim,0,0,TO_CHAR);
		}
		WAIT_STATE(victim, PULSE_VIOLENCE *2);
		 GET_POS(victim)  = POS_SITTING;	   
         stop_fighting(victim,TRUE);
		return (0);
	}
      GET_HIT(victim) = 1;
	  act("$v размазало по земле насмерть!",FALSE,victim,0,0,TO_ROOM);
	  act("&rВас размазало по земле и Вы умерли!&n",FALSE,victim,0,0,TO_CHAR);

    ndice = 1;
    sdice = 1;
    adice = level;
break;
} // end switch(spellnum) 

  if (!dam && !no_savings)
     {double koeff = 1;
      if (IS_NPC(victim))
         {if (NPC_FLAGGED(victim,NPC_FIRECREATURE))
             {if (IS_SET(SpINFO.violent,MTYPE_FIRE))
                 koeff /= 2;
              if (IS_SET(SpINFO.violent,MTYPE_WATER))
                 koeff *= 2;
             }
          if (NPC_FLAGGED(victim,NPC_AIRCREATURE))
             {if (IS_SET(SpINFO.violent,MTYPE_EARTH))
                 koeff *= 2;
              if (IS_SET(SpINFO.violent,MTYPE_AIR))
                 koeff /= 2;
             }
          if (NPC_FLAGGED(victim,NPC_WATERCREATURE))
             {if (IS_SET(SpINFO.violent,MTYPE_FIRE))
                 koeff *= 2;
              if (IS_SET(SpINFO.violent,MTYPE_WATER))
                 koeff /= 2;
             }
          if (NPC_FLAGGED(victim,NPC_EARTHCREATURE))
             {if (IS_SET(SpINFO.violent,MTYPE_EARTH))
                 koeff /= 2;
              if (IS_SET(SpINFO.violent,MTYPE_AIR))
                 koeff *= 2;
             }
         }
     if (AFF_FLAGGED(victim, AFF_SANCTUARY))
         koeff *= 2;

      vych = MAX(50, modi + 100);
     
               if (!IS_WEDMAK(ch))
	  vych += GET_CAST_SUCCESS(ch) * 3 / 4;
	  
            koeff *= vych/100;
            no_savings = FALSE;

             if (number(con_app[GET_REAL_CON(ch)].critic_saving,500) < (cha_app[GET_REAL_CHA(ch)].morale / 2 +
	        GET_MORALE(ch) / 2 + 
			wis_app[GET_REAL_WIS(ch)].spell_success / 3 + 
			(int_app[GET_REAL_INT(ch)].spell_aknowlege - 50) / 4) && 
			!general_savingthrow(victim, SAVING_STABILITY, modi))
		{ dam = ndice * sdice + adice;
		 modi +=100;
                  no_savings = TRUE;
		  adice = (int) (dam * koeff);
      	     
              if (!IS_WEDMAK(ch))
      		{ act("&mЗаклинание $r набрало ужасную мощь!&n",FALSE,ch,0,0,TO_ROOM);
		  act("&mВаше заклинание набрало ужасную мощь!&n",FALSE,ch,0,0,TO_CHAR);
		}
	}
	else
  dam = dice(ndice,sdice) + adice;

      //sprintf(buf,"&GБазовое магический дамаг - %d хп&n",dam);
      //act(buf, FALSE, ch, 0, 0, TO_CHAR);

      dam = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_EFFECT,dam);

      //sprintf(buf,"&CВ сумме с погодным модификатором - %d хп&n",dam);
      //act(buf,FALSE,ch,0,0,TO_CHAR);
    

            if (ch != victim && general_savingthrow(victim, savetype, modi) &&
	     (spellnum != SPELL_MAGIC_MISSILE || spellnum != SPELL_IMPLOSION))
              koeff /= 2;
	  
 if (ch != victim)
		if (AFF_FLAGGED(victim,AFF_FIREAURA) && IS_SET(SpINFO.violent,MTYPE_FIRE))//Висит заклинашка "сопротивление огню"
			dam = dam * number(30,70) / 100;//и заклинание относится к типу "огонь"
		if (AFF_FLAGGED(victim,AFF_ICEAURA) && IS_SET(SpINFO.violent,MTYPE_WATER))//Висит заклинашка "сопротивление холоду"
			dam = dam * number(30,70) / 100;//и заклинание относится к типу "вода"
        if (AFF_FLAGGED(victim,AFF_AIRAURA) && IS_SET(SpINFO.violent,MTYPE_AIR))// Висит заклинашка "сопротивление воздуху"
			dam = dam * number(30,70) / 100;//и заклинание относится к типу "воздух"
        if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && IS_SET(SpINFO.violent,MTYPE_EARTH))
			dam = dam * number(30,70) / 100;

      if (dam > 0)
		{ koeff *= 1000;
          dam    = (int) MMAX(1, dam * MMAX(300,MMIN(koeff,10000)) / 1000);
		}
           if (no_savings)
              dam = MAX(adice,MIN(dam,GET_REAL_MAX_HIT(victim) * 6 / 10));
          
		   if (spellnum == SPELL_SACRIFICE && ch != victim)
			{ int sacrifice = MAX(1,MIN(dam*count,GET_HIT(victim)));
			    do_sacrifice(ch,sacrifice);
              if (!IS_NPC(ch))
				{ struct follow_type *f;
                  for (f = ch->followers; f; f = f->next)
                      if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower,AFF_CHARM) &&
                          MOB_FLAGGED(f->follower,MOB_CORPSE))
                         do_sacrifice(f->follower,sacrifice);
                 }
             }
         }

   if (IS_NPC(victim)) //дамаг от позиции
  switch(GET_POS(victim))
  {case POS_SITTING:
   case POS_RESTING:
   case POS_SLEEPING:
     dam = dam * 4 / 3;
   break;
   case POS_STUNNED:
     dam *= 2;
   break;
  default:
	   break;
  }

//dam -= GET_ABSORBE(victim)/5; убрать, есть GET_DRESIST(victim) сопротивление магдамагу.
// сопротивление магадмагу в процентах, если набрали 100, то дамаг не проходит вооще.
// хорошо ли это, пока не знаю, думу думать буду

dam -= dam * GET_DRESIST(victim) / 100;

dam = MAX(0,dam);



		//sprintf(buf, "&WРезультирующий магический дамаг %d&n\r\n\r\n", dam);
		//act(buf,FALSE,ch,0,0,TO_CHAR);

  // And finally, inflict the damage and call fight_mtrg because hit() not called
  for (; count > 0 && rand >= 0; count--)
      {if (IN_ROOM(ch)     != NOWHERE    &&
           IN_ROOM(victim) != NOWHERE    &&
	   GET_POS(ch)     > POS_STUNNED &&
	   GET_POS(victim) > POS_DEAD
	  )
//	  fight_mtrigger(ch);
        rand = damage(ch, victim, dam, spellnum, count <= 1);
      }
  return rand;
}

          
          
      



/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
*/

#define MAX_SPELL_AFFECTS 6	/* change if more needed */

int pc_duration(struct char_data *ch,int cnst, int level, int level_divisor, int min, int max)
{int result = 0;
 result = cnst  * SECS_PER_MUD_HOUR;
 if (level > 0 && level_divisor > 0)
    level  = level * SECS_PER_MUD_HOUR / level_divisor;
 else
    level  = 0;
 if (min > 0)
    level = MIN(level, min * SECS_PER_MUD_HOUR);
 if (max > 0)
    level = MAX(level, max * SECS_PER_MUD_HOUR);
 result = (level + result) / SECS_PER_PLAYER_AFFECT;
 return ( result );
}

void mag_affects(int level,    struct char_data * ch, struct char_data * victim,
		         int spellnum, int savetype, int use_glory)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool   accum_affect  = FALSE, accum_duration = FALSE, success = TRUE, update_spell = FALSE;
  const  char *to_vict = NULL, *to_room = NULL;
  int    i, modi=0, heal = FALSE;


  if (victim == NULL || IN_ROOM(victim) == NOWHERE ||
      ch == NULL
     )
     return;

  //Magic resistance
   if (ch != victim 		&&
    spellnum < MAX_SPELLS	&&
    ((GET_SAVE(victim, SAVING_STABILITY) + GET_ARESIST(victim)) -
	 MAX(GET_LEVEL(ch) - GET_LEVEL(victim),0)) > number(1,100) &&
	!IS_SET(SpINFO.routines, NPC_AFFECT_NPC | NPC_UNAFFECT_NPC))  
	{ act("Заклинание рассеялось около $r.",FALSE,victim,0,0,TO_ROOM);
      act("Заклинание не подействовало на Вас.",FALSE,victim,0,0,TO_CHAR);
      pk_agro_action(ch,victim);
	  return;  
	}


       if (ch != victim)
	{  if (IS_SET(SpINFO.routines, NPC_AFFECT_PC))
           pk_agro_action(ch,victim);
       else
       if (IS_SET(SpINFO.routines, NPC_AFFECT_NPC) && FIGHTING(victim))
           pk_agro_action(ch,FIGHTING(victim));
     }


	   /* if (ch != victim && clan[find_clan_by_id(GET_CLAN(victim))].alignment == 1 &&
                GET_CLAN_RANK(victim) && IS_SET(SpINFO.routines, NPC_AFFECT_NPC) &&
                spellnum != SPELL_FIRE_SHIELD)
		{ act("Заклинание рассеялось около $r.",FALSE,victim,0,0,TO_ROOM);
                  act("Заклинание не подействовало на Вас.",FALSE,victim,0,0,TO_CHAR);
		  return;
		}
        */
  // Magic glass
  if (ch != victim   &&
      SpINFO.violent &&
      ((AFF_FLAGGED(victim,AFF_MAGICGLASS) && number(1,100) < GET_LEVEL(victim)
       ) ||
       (IS_GOD(victim) && (IS_NPC(ch) || GET_LEVEL(victim) > GET_LEVEL(ch))
       )
      )
     )
    {act("&WМагическое зеркало $R отразило Вашу магию!&n",FALSE,ch,0,victim,TO_CHAR);
     act("Ваше магическое зеркало отразило магию $r!",FALSE,ch,0,victim,TO_VICT);
     mag_affects(level,ch,ch,spellnum,savetype);
     return;
    }


  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      {af[i].type       = spellnum;
       af[i].bitvector  = 0;
       af[i].modifier   = 0;
       af[i].battleflag = 0;
       af[i].location   = APPLY_NONE;
      }

  /* decreese modi for failing, increese fo success */
  if (ch != victim)
     {modi  = con_app[GET_REAL_CON(victim)].affect_saving;
      modi  = modi + number(GET_REAL_CHA(ch)/8,GET_REAL_CHA(ch));
      modi += calc_anti_savings(ch, spellnum);
      modi -= MAX(0,GET_LEVEL(victim) - GET_LEVEL(ch))*5;
      modi -= MIN(0,GET_LEVEL(victim) - GET_LEVEL(ch));
      }

  /*if (IS_WARRIOR(victim) && 
	  clan[find_clan_by_id(GET_CLAN(victim))].alignment == 1 &&
	  GET_CLAN_RANK(victim) &&
	  IS_NPC(ch))
	  modi -= (MAX(0,GET_LEVEL(victim) - 15) * 4);
*/
//  log("[MAG Affect] Modifier value for %s (caster %s) = %d(spell %d)",
//      GET_NAME(victim), GET_NAME(ch), modi, spellnum);

  switch (spellnum)
  {
  case SPELL_CHILL_TOUCH:
    savetype = SAVING_STABILITY;
    success  = FALSE;

    if (ch != victim && general_savingthrow(victim, savetype, modi))
      break;
       
    af[0].location    = APPLY_STR;
    af[0].duration    = pc_duration(victim,2,level,4,6,0);
    af[0].modifier    = -1;
    af[0].battleflag  = AF_BATTLEDEC;

    to_room           = "Боевый пыл $r несколько остыл.";
    to_vict           = "Вы почувствовали упадок сил!";
    break;

  case SPELL_ENERGY_DRAIN:
    savetype = SAVING_WILL;

    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT,ch);
        success = FALSE;
        break;
       }
    af[0].duration = pc_duration(victim,2,level,4,6,0);
    af[0].location = APPLY_STR;
    af[0].modifier    = -2;
    af[0].battleflag  = AF_BATTLEDEC;
    accum_duration    = TRUE;
    to_room           = "$n стал$y немного слабее.";
    to_vict           = "&KВы почувствовали себя слабее!&n";
    break;

  case SPELL_WEAKNESS:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT,ch);
        success = FALSE;
        break;
       }
    if (affected_by_spell(victim,SPELL_STRENGTH))
       {affect_from_char(victim,SPELL_STRENGTH);
        success = FALSE;
        break;
       }
    af[0].duration    = pc_duration(victim,5,level,6,4,0);
    af[0].location    = APPLY_STR;
    af[0].modifier    = -3;
    af[0].battleflag  = AF_BATTLEDEC;
    accum_duration    = TRUE;
    to_room           = "$n стал$y немного слабее.";
    to_vict           = "&KВы почувствовали себя слабее!&n";
    break;

  case SPELL_STONESKIN:
    af[0].location = APPLY_ABSORBE;
    af[0].modifier = (level + 30) / 2;
    af[0].duration = pc_duration(victim,3,0,1,0,0);
//    accum_duration = TRUE;
    to_room        = "Кожа $r покрылась каменными пластинами.";
    to_vict        = "Вы стали менее чувствительны к ударам.";
    break;

  case SPELL_FAST_REGENERATION:
    af[0].location  = APPLY_HITREG;
    af[0].modifier  = 100+level*20+GET_REMORT(ch)*200;
    af[0].duration  = pc_duration(victim,0,level,10,0,0);
//    accum_duration  = TRUE;
    to_room          = "$n расцвел$y на Ваших глазах.";
    to_vict          = "Вас наполнила живительная сила.";
    break;

  case SPELL_AIR_SHIELD:
    af[0].bitvector  = AFF_AIRSHIELD;
    af[0].battleflag = TRUE;
    af[0].duration   = pc_duration(victim,8,0,0,0,0);
    to_room         = "$v окружил воздушный щит."; 
    to_vict          = "Вас окружил воздушный щит.";
    break;

  case SPELL_FIRE_SHIELD:
    af[0].bitvector  = AFF_FIRESHIELD;
    af[0].battleflag = TRUE;
    af[0].duration   = pc_duration(victim,8,0,0,0,0);
    to_room          = "$v окутал огненный щит.";
    to_vict          = "Вас окутал огненный щит.";
    break;

  case SPELL_ICE_SHIELD:
    af[0].bitvector  = AFF_ICESHIELD;
    af[0].battleflag = TRUE;
    af[0].duration   = pc_duration(victim,8,0,0,0,0);
    to_room          = "$v окутал ледяной щит.";
    to_vict          = "Вас окутал ледяной щит.";
    break;

  case SPELL_AIR_AURA:
    af[0].location   = APPLY_RESIST_AIR;
    af[0].bitvector  = AFF_AIRAURA;
    af[0].duration   = pc_duration(victim,level-8,0,0,0,0);
    to_room          = "По телу $r побежал ветерок, превратившись в упругую оболочку.";
    to_vict          = "Вокруг Вас образовалась воздушная оболочка.";
    break;

  case SPELL_FIRE_AURA:
    af[0].location   = APPLY_RESIST_WATER;
    af[0].modifier   = level;
    af[0].bitvector  = AFF_FIREAURA;
    af[0].duration   = pc_duration(victim,level-8,0,0,0,0);

    to_room          = "По телу $r побежал ветерок, превратившись в упругую оболочку.";
    to_vict          = "Вокруг Вас образовалась воздушная оболочка.";
    break;

  case SPELL_ICE_AURA:
    af[0].location   = APPLY_RESIST_FIRE;
    af[0].modifier   = level;
    af[0].bitvector  = AFF_ICEAURA;
    af[0].duration   = pc_duration(victim,level-8,0,0,0,0);
    to_room	     = "$v окружила ледяная аура.";
    to_vict	     = "Вас окружила ледяная аура.";
    break;


  case SPELL_CLOUDLY:
    af[0].location = APPLY_AC;
    af[0].modifier = -10;
    af[0].duration = pc_duration(victim,10,level,6,0,0);
    af[1].location = APPLY_RESIST_FIRE;
    af[1].modifier = 5;
    af[1].duration = af[0].duration;

    to_room = "Очертания $r расплылись и стали менее отчетливыми.";
    to_vict = "Очертания Вашего тела стали менее отчетливы.";
    break;

  case SPELL_ARMOR:
    af[0].location   = APPLY_ABSORBE;
    af[0].modifier   = 4 + level/4;
    af[0].duration   = pc_duration(victim,15,level + 20,4,0,0);
    af[0].battleflag = AF_BATTLEDEC; 
    
    af[1].location   = APPLY_SAVING_REFLEX;
    af[1].modifier   = -5;
    af[1].duration   = af[0].duration;
    af[1].battleflag = AF_BATTLEDEC;

    af[2].location   = APPLY_SAVING_STABILITY;
    af[2].modifier   = -5;
    af[2].duration   = af[0].duration;
    af[2].battleflag = AF_BATTLEDEC; 
  
    to_room = "Вокруг $r на мгновенье возник магический свет и тут же пропал.";
    to_vict = "Вокруг Вас возник магический невидимый щит.";
    break;

   case SPELL_GROUP_BLESS:
   case SPELL_BLESS:
    af[0].location = APPLY_SAVING_STABILITY;
    af[0].modifier = -5;
    af[0].duration = pc_duration(victim,10,0,0,0,0);
    af[0].bitvector= AFF_BLESS;
    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = -5;
    af[1].duration = pc_duration(victim,6,0,0,0,0);
    af[1].bitvector= AFF_BLESS;
    to_room = "$n осветил$u на миг неземным светом.";
    to_vict = "На Вас снизошло благословение.";
    break;

  case SPELL_AWARNESS:
    af[0].duration = pc_duration(victim,6,0,0,0,0);
    af[0].bitvector= AFF_AWARNESS;
    to_room        = "$n начал$y внимательно осматриваться по сторонам.";
    to_vict        = "Вы стали более внимательны к окружающему.";
    break;

  case SPELL_SHIELD:
    af[0].duration   = pc_duration(victim,4,0,0,0,0);
    af[0].bitvector  = AFF_SHIELD;
    af[0].location   = APPLY_SAVING_STABILITY;
    af[0].modifier   = -10;
    af[0].battleflag = AF_BATTLEDEC;
    af[1].duration   = pc_duration(victim,4,0,0,0,0);
    af[1].bitvector  = AFF_SHIELD;
    af[1].location   = APPLY_SAVING_WILL;
    af[1].modifier   = -10;
    af[1].battleflag = AF_BATTLEDEC;
    af[2].duration   = pc_duration(victim,4,0,0,0,0);
    af[2].bitvector  = AFF_SHIELD;
    af[2].location   = APPLY_SAVING_REFLEX;
    af[2].modifier   = -10;
    af[2].battleflag = AF_BATTLEDEC;

    to_room 	     = "$n покрыл$u сверкающим коконом.";
    to_vict 	     = "Вас покрыл голубой кокон.";
    break;

  case SPELL_GROUP_HASTE:
  case SPELL_HASTE:
    if (affected_by_spell(victim, SPELL_SLOW))
       {affect_from_char(victim, SPELL_SLOW);
        success = FALSE;
       }
    else
       {af[0].duration = pc_duration(victim,12,0,0,0,0);
        af[0].bitvector= AFF_HASTE;
        to_vict = "Ваши передвижения стали быстрее.";
        to_room = "$n начал$y передвигаться быстрее.";
       }
    break;

   case SPELL_PROTECT_MAGIC:
    af[0].location = APPLY_RESIST_AIR;
    af[0].modifier = GET_LEVEL(ch);
    af[0].duration = pc_duration(victim,0,level,3,3,9);
    af[0].battleflag  = 0;

    to_room = "$n осветил$u на миг изумрудным светом.";
    to_vict = "Боги взяли Вас под свою защиту.";
    break;

  case SPELL_ENLARGE:
    if (affected_by_spell(victim, SPELL_ENLESS))
       {affect_from_char(victim, SPELL_ENLESS);
        success = FALSE;
       }
    else
       {af[0].location = APPLY_SIZE;
        af[0].modifier = 6 + level/4;
        af[0].duration = pc_duration(victim,0,level,4,0,0);

        to_room  = "$n начал$y расти, увеличиваясь в размерах.";
        to_vict  = "Вы стали крупнее.";
       }
    break;

  case SPELL_ENLESS:
    if (affected_by_spell(victim, SPELL_ENLARGE))
       {affect_from_char(victim, SPELL_ENLARGE);
        success = FALSE;
       }
    else
       {af[0].location = APPLY_SIZE;
        af[0].modifier = -(6 + level/4);
        af[0].duration = pc_duration(victim,0,level,4,0,0);
        accum_duration = TRUE;
        to_room  = "$n уменьшил$u в размере.";
        to_vict  = "Вы почувствовали, как уменьшелись в размерах.";
       }
    break;

  case SPELL_MAGICGLASS:
    af[0].bitvector= AFF_MAGICGLASS;
    af[0].duration = pc_duration(victim,1,GET_REAL_INT(ch),9,0,0);
    accum_duration = TRUE;
    to_room  = "$n покрыла зеркальная пелена.";
    to_vict  = "Вас окружила зеркальная магия.";
    break;

  case SPELL_STONEHAND:
    af[0].bitvector= AFF_STONEHAND;
    af[0].duration = pc_duration(victim,30,GET_LEVEL(ch),3,0,0);
//    accum_duration = TRUE;
    to_room  = "Кулаки $r стали крепки как камни.";
    to_vict  = "Ваши кулаки стали крепки как камни.";
    break;

  case SPELL_PRISMATICAURA:
       if (!IS_NPC(ch) && !same_group(ch,victim))
       {send_to_char("Это заклинание Вы можете произносить на себя или согруппников!\r\n",ch);
        return;
       }
   
    to_room  = "У $r побледнела кожа и выросли клыки.";
    to_vict  = "Вы почувствовали жажду крови.";
    
    af[0].location = APPLY_CHA;
    af[0].modifier = 2;
    af[0].duration = pc_duration(victim,16000,0,0,0,0);
    af[0].battleflag= AF_DEADKEEP;
     
    af[1].location = APPLY_DEX;
    af[1].modifier = 6;
    af[1].duration = af[0].duration;
    af[1].battleflag= AF_DEADKEEP;

    af[2].bitvector= AFF_PRISMATICAURA;
    af[2].location = APPLY_INT;
    af[2].modifier = -10;
    af[2].duration = af[0].duration;
    af[2].battleflag= AF_DEADKEEP;

    af[3].bitvector= AFF_INFRAVISION;
    af[3].location = APPLY_WIS;
    af[3].modifier = -10;
    af[3].duration = af[0].duration;
	af[3].battleflag= AF_DEADKEEP;

    af[4].bitvector= AFF_SENSE_LIFE;
    af[4].location = APPLY_STR;
    af[4].modifier = 8;
    af[4].duration = af[0].duration;
    af[4].battleflag= AF_DEADKEEP;
  
    af[5].location = APPLY_CON;
    af[5].modifier = 4;
    af[5].duration = af[0].duration;
    af[5].battleflag= AF_DEADKEEP;
  	break;
   
    	break;

  case SPELL_STAIRS:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].duration  = pc_duration(victim,0,level,3,0,0);
    af[0].bitvector = AFF_STAIRS;
    to_room = "Контуры тела $r отчетливо засветились!";
    to_vict = "Контуры Вашего тела отчетливо засветились!";
    break;

  case SPELL_MINDLESS:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].location  = APPLY_MANAREG;
    af[0].modifier  = -30;
    af[0].duration  = pc_duration(victim,0,GET_REAL_WIS(ch) + GET_REAL_INT(ch),10,0,0);
    af[1].location  = APPLY_CAST_SUCCESS;
    af[1].modifier  = -30;
    af[1].duration  = af[0].duration;
    af[2].location  = APPLY_HITROLL;
    af[2].modifier  = -20;
    af[2].duration  = af[0].duration;

    to_room = "&KФизиономия $r расплылась в идиотской улыбке!&n";
    to_vict = "&KВы ощутили, как способность думать ослабла!&n";
    break;


  case SPELL_POWER_BLINDNESS:
  case SPELL_BLINDNESS:
     savetype = SAVING_STABILITY;
    if (MOB_FLAGGED(victim, MOB_NOBLIND) ||
        ((ch != victim) && general_savingthrow(victim, savetype, modi))
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].type      = SPELL_BLINDNESS;
    af[0].location  = APPLY_HITROLL;
    af[0].modifier  = -4;
    af[0].duration  = spellnum == SPELL_POWER_BLINDNESS ?
                      pc_duration(victim,3,level,6,0,0) :
                      pc_duration(victim,2,level,8,0,0);
    af[0].bitvector = AFF_BLIND;
    af[0].battleflag= AF_BATTLEDEC;
    to_room = "&K$n ослеп$q!&n";
    to_vict = "&KВы ослепли!&n";
    break;

  case SPELL_MADNESS:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].duration  = pc_duration(victim,3,0,0,0,0);
    af[0].bitvector = AFF_NOFLEE;
    to_room = "Теперь $n не сможет сбежать из боя!";
    to_vict = "&KВас обуяло безумие боя!&n";
    break;

  case SPELL_WEB:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].location  = APPLY_HITROLL;
    af[0].modifier  = -3;
    af[0].duration  = pc_duration(victim,3,level,6,0,0);
    af[0].battleflag= AF_BATTLEDEC;
    af[0].bitvector = AFF_NOFLEE;
    af[1].location  = APPLY_AC;
    af[1].modifier  = 20;
    af[1].duration  = af[0].duration;
    af[1].battleflag= AF_BATTLEDEC;
    af[1].bitvector = AFF_NOFLEE;
    to_room = "&g$r покрыла невидимая паутина, сковывая $s движения!&n";
    to_vict = "&gВас покрыла невидимая паутина!&n";
    break;


  case SPELL_CURSE:
    if (ch != victim && general_savingthrow(victim, SAVING_WILL, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }
    af[0].location  = APPLY_INITIATIVE;
    af[0].duration  = pc_duration(victim,1,level,2,0,0);
    af[0].modifier  = -20;
    af[0].bitvector = AFF_CURSE;
    af[1].location = APPLY_CAST_SUCCESS;
    af[1].duration = af[0].duration;
    af[1].modifier = -1 * (level/3 + GET_REMORT(ch));
    af[1].bitvector = AFF_CURSE;
   // accum_duration = TRUE;
   // accum_affect   = TRUE;
    to_room = "&KМраморное сияние вспыхнуло и тут же погасло над головой $r!&n";
    to_vict = "&KВы почувствовали себя неуютно.&n";
    break;

  case SPELL_SLOW:
	  savetype = SAVING_STABILITY;
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    if (affected_by_spell(victim, SPELL_HASTE))
       {affect_from_char(victim, SPELL_HASTE);
        success = FALSE;
        break;
       }

    af[0].duration  = pc_duration(victim,9,0,0,0,0);
    af[0].bitvector = AFF_SLOW;
    to_room         = "&KДвижения $r стали неуклюжими.&n";
    to_vict         = "&KВаши движения замедлились.&n";
    break;

  case SPELL_DETECT_ALIGN:
    af[0].duration  = pc_duration(victim,12,level,2,0,0);
    af[0].bitvector = AFF_DETECT_ALIGN;
//    accum_duration  = TRUE;
    to_vict         = "Ваши глаза приобрели зеленый оттенок.";
    to_room         = "Глаза $r приобрели зеленый оттенок.";
    break;

  case SPELL_DETECT_INVIS:
    af[0].duration  = pc_duration(victim,16,level,2,0,0);
    af[0].bitvector = AFF_DETECT_INVIS;
//    accum_duration  = TRUE;
    to_vict         = "Ваши глаза приобрели золотистый оттенок.";
    to_room         = "Глаза $r приобрели золотистый оттенок.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].duration  = pc_duration(victim,16,level,2,0,0);
    af[0].bitvector = AFF_DETECT_MAGIC;
//    accum_duration = TRUE;
    to_vict        = "Ваши глаза приобрели желтый оттенок.";
    to_room        = "Глаза $r приобрели желтый оттенок.";
    break;

  case SPELL_INFRAVISION:
    af[0].duration  = pc_duration(victim,16,level,2,0,0);
    af[0].bitvector = AFF_INFRAVISION;
//    accum_duration  = TRUE;
    to_vict         = "Ваши глаза приобрели красноватый оттенок.";
    to_room         = "Глаза $r приобрели красноватый оттенок.";
    break;

  case SPELL_DETECT_POISON:
    af[0].duration  = pc_duration(victim,16,level,2,0,0);
    af[0].bitvector = AFF_DETECT_POISON;
//    accum_duration  = TRUE;
    to_vict         = "Ваши глаза приобрели карий оттенок.";
    to_room         = "Глаза $r приобрели карий оттенок.";
    break;

  case SPELL_GROUP_INVISIBLE:
  case SPELL_INVISIBLE:
    if (!victim)
       victim = ch;

    af[0].duration  = pc_duration(victim,10,level,6,0,0);
    af[0].modifier  = -10;
    af[0].location  = APPLY_AC;
    af[0].bitvector = AFF_INVISIBLE;
//    accum_duration  = TRUE;
    to_vict         = "Вы медленно растворились в воздухе.";
    to_room         = "$n медленно растворил$u в пустоте.";
    break;

	//новое заклинание.протестить
  case SPELL_MENTALLS:
     if (!victim)
       victim = ch;
//pc_duration(victim,1,level + 20,5,0,0); взял из защиты продолжительность, протестить
    af[0].duration  = pc_duration(victim,12,level,4,0,0);
    af[0].modifier  = -40;
    af[0].location  = APPLY_AC;
    af[0].bitvector = AFF_MENTALLS;
	af[0].battleflag= AF_BATTLEDEC;
//    accum_duration  = TRUE;
    to_vict         = "Вы перешли в ментальное поле.";
    to_room         = "$n растворил$u в воздухе.";

	  break;

  case SPELL_DETECT_MENTALLS:
    af[0].duration  = pc_duration(victim,16,level,1,0,0);
    af[0].bitvector = AFF_DETECT_MENTALLS;
//    accum_duration  = TRUE;
    to_vict         = "У Вас появилась способность видеть ментальное поле.";
    to_room         = "Глаза $r приобрели прозрачный оттенок.";
    break;

  case SPELL_PLAQUE:
    savetype = SAVING_STABILITY;
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].location  = APPLY_HITREG;
    af[0].duration  = pc_duration(victim,0,level,2,0,0);
    af[0].modifier  = -95;
    af[1].location  = APPLY_MANAREG;
    af[1].duration  = pc_duration(victim,0,level,2,0,0);
    af[1].modifier  = -95;
    af[2].location  = APPLY_MOVEREG;
    af[2].duration  = pc_duration(victim,0,level,2,0,0);
    af[2].modifier  = -95;
    to_vict         = "&gВас скрутило в жестокой лихорадке.&n";
    to_room         = "&g$v скрутило в жестокой лихорадке.&n";
    break;


  case SPELL_POISON:
	  savetype = SAVING_CRITICAL;
    if (ch != victim &&
        general_savingthrow(victim, savetype, modi + con_app[GET_REAL_CON(victim)].poison_saving)
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].location  = APPLY_STR;
    af[0].duration  = pc_duration(victim,level/4,0,0,0,0);
    af[0].modifier  = -4;
    af[0].bitvector = AFF_POISON;
    af[1].location  = APPLY_POISON;
    af[1].duration  = af[0].duration;
    af[1].modifier  = level;
    af[1].bitvector = AFF_POISON;
    to_vict         = "&rВы почувствовали себя отравленн$g.&n";
    to_room         = "&K$n почернел$y и начал$y судорожно дрожать от действия яда.&n";
    break;

  case SPELL_PROT_FROM_EVIL:
 // af[0].duration  = pc_duration(victim,24,0,0,0,0);
    af[0].duration  = pc_duration(victim,level-8,0,0,0,0);
    af[0].bitvector = AFF_PROTECT_EVIL;
    accum_duration  = TRUE;
    to_vict         = "Вокруг Вас вырос щит, защищающий от тьмы.";
    break;

  case SPELL_EVILESS:
    af[0].duration  = pc_duration(victim,6,0,0,0,0);
    af[0].location  = APPLY_DAMROLL;
    af[0].modifier  = 20;
    af[0].bitvector = AFF_EVILESS;
    af[1].duration  = pc_duration(victim,6,0,0,0,0);
    af[1].location  = APPLY_HITROLL;
    af[1].modifier  = 5;
    af[1].bitvector = AFF_EVILESS;
    af[2].duration  = pc_duration(victim,6,0,0,0,0);
    af[2].location  = APPLY_HIT;
    af[2].modifier  = GET_REAL_MAX_HIT(victim);
    af[2].bitvector = AFF_EVILESS;
    heal            = TRUE;
    to_vict         = "Черное облако покрыло Вас.";
    to_room         = "Черное облако покрыло $v.";
    break;

  case SPELL_SANCTUARY:
    if (!IS_NPC(ch) && !same_group(ch,victim))
       {send_to_char("Только на себя или одногруппника!\r\n",ch);
        return;
       }
    af[0].duration  = pc_duration(victim,8,level,4,0,0);
    af[0].bitvector = AFF_SANCTUARY;
    to_vict         = "Белая аура мгновенно окружила Вас.";
    to_room         = "Белая аура окутала все тело $r.";
    break;


  case SPELL_SLEEP:
	  savetype = SAVING_WILL;

	  if (MOB_FLAGGED(victim, MOB_NOSLEEP)  ||
		  AFF_FLAGGED(victim, AFF_HOLDALL)  ||
		  GET_MOB_HOLD(victim)              ||
		  //IS_UNDEADS(victim)		        ||
        (ch != victim && general_savingthrow(victim, savetype, modi))
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       };  

	  if (FIGHTING(victim))
		stop_fighting(victim,TRUE);
		//modi -= (50 + MAX(0,GET_LEVEL(victim)-15));
		//af[0].duration  = pc_duration(victim,0,0,0,0,0);
		
		//else
	af[0].duration  = pc_duration(victim,1,level,6,1,6);
	af[0].bitvector = AFF_SLEEP;
    af[0].battleflag= AF_BATTLEDEC;
	
	
    if (GET_POS(victim) > POS_SLEEPING && success)
		{ if (on_horse(victim))
			{ sprintf(buf, "%s свалил%s со своего скакуна.", GET_NAME(victim), GET_CH_SUF_2(victim));
				act(buf, FALSE, victim, 0, 0, TO_ROOM);
				REMOVE_BIT(AFF_FLAGS(victim, AFF_HORSE), AFF_HORSE);
			}
			send_to_char("&YВас резко потянуло в сон..&n\r\n", victim);
			act("&Y$n впал$y в магический сон..&n", TRUE, victim, 0, 0, TO_ROOM);
			GET_POS(victim) = POS_SLEEPING;
		}
    break;

  case SPELL_GROUP_STRENGTH:
  case SPELL_STRENGTH:
    if (affected_by_spell(victim, SPELL_WEAKNESS))
       {affect_from_char(victim, SPELL_WEAKNESS);
        success = FALSE;
        break;
       }
    af[0].location = APPLY_STR;
    af[0].duration = pc_duration(victim,12,level,4,0,0);
    if (ch == victim)
       af[0].modifier = (level + 10) / 10;
    else
       af[0].modifier = (level + 15) / 15;
//    accum_duration = TRUE;
 //   accum_affect   = TRUE;
    to_vict        = "Вы почувствовали себя сильнее.";
    to_room        = "Вы увидели, как тело $r начало наливаться силой!";
    break;
    spellnum = SPELL_STRENGTH;

  case SPELL_SENSE_LIFE:
    to_vict         = "У Вас возникла способность чувствовать жизнь.";
    af[0].duration  = pc_duration(victim,8,level,0,0,0);
    af[0].bitvector = AFF_SENSE_LIFE;
//    accum_duration  = TRUE;
    break;

  case SPELL_WATERWALK:
    af[0].duration  = pc_duration(victim,24,0,0,0,0);
    af[0].bitvector = AFF_WATERWALK;
//    accum_duration  = TRUE;
    to_vict         = "Вы обрели способность плавать.";
    break;

  case SPELL_WATERBREATH:
    af[0].duration  = pc_duration(victim,12,level,4,0,0);
    af[0].bitvector = AFF_WATERBREATH;
//    accum_duration  = TRUE;
    to_vict         = "У Вас выросли жабры.";
    to_room         = "У $r выросли жабры.";
    break;

  //слово замереть, холд не любого
  case SPELL_OWER_HOLD:
    if (MOB_FLAGGED(victim, MOB_NOHOLD) ||
        (ch != victim && 
general_savingthrow(victim, SAVING_WILL, modi - ((GET_HIT(victim) - 700) / 7)))
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].type       = SPELL_OWER_HOLD;
    af[0].duration   = pc_duration(victim,0,level+2,3,0,0);
    af[0].bitvector  = AFF_HOLDALL;
    af[0].battleflag = AF_BATTLEDEC;
    to_room          = "&C$n застыл$y на месте, не в силах пошевелиться!&n";
    to_vict          = "&CВы застыли на месте, не в силах пошевелиться.&n";
    break;


  case SPELL_POWER_HOLD:
    if (MOB_FLAGGED(victim, MOB_NOHOLD) ||
        (ch != victim && general_savingthrow(victim, SAVING_STABILITY, modi)) ||
         ((IS_NPC(victim)&&GET_CLASS(victim) != CLASS_HUMAN)  && !(IS_IMMORTAL(ch) ||
	   GET_GOD_FLAG(ch,GF_GODSLIKE))))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }
    af[0].type       = SPELL_POWER_HOLD;
    af[0].duration   =  pc_duration(victim,0,level,3,0,0);
    af[0].bitvector  = AFF_HOLD;
    af[0].battleflag = AF_BATTLEDEC|AF_PULSEDEC;
    to_room          = "&C$n замер$q на месте, не в силах шевельнуться!&n";
    to_vict          = "&CВы замерли на месте, не в силах пошевелиться.&n";
    break;


  case SPELL_UNDEAD_HOLD:
    if (MOB_FLAGGED(victim, MOB_NOHOLD) ||
	 !IS_NPC(victim)			    ||	
       (ch != victim && general_savingthrow(victim, SAVING_WILL, modi)) ||
	 (!IS_UNDEADS(victim) && !(IS_IMMORTAL(ch) ||
	 GET_GOD_FLAG(ch,GF_GODSLIKE))))	
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].type       = SPELL_UNDEAD_HOLD;
    af[0].duration   = pc_duration(victim,0,level,4,0,0);
    af[0].bitvector  = AFF_HOLDALL;
    af[0].battleflag = AF_BATTLEDEC;
    to_room          = "&C$n застыл$y на месте, не в силах сдвинуться с места!&n";
    to_vict          = "&CВы застыли на месте, не в силах сдвинуться с места!&n";
    break;

  case SPELL_HOLD:
    if (MOB_FLAGGED(victim, MOB_NOHOLD) ||
        (ch != victim && general_savingthrow(victim, SAVING_WILL, modi)) ||
        ((IS_NPC(victim) && IS_UNDEADS(victim)) && !(IS_IMMORTAL(ch) ||
	GET_GOD_FLAG(ch,GF_GODSLIKE))))
	{send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }
    af[0].type       = SPELL_HOLD;
    af[0].duration   = pc_duration(victim,0,level+9,4,0,0);
    af[0].bitvector  = AFF_HOLD;
    af[0].battleflag = AF_BATTLEDEC;
    to_room          = "&c$v сковал паралич!&n";
    to_vict          = "&cВас сковал паралич.&n";
    break;

  case SPELL_POWER_SIELENCE:
  case SPELL_SIELENCE:
    if (MOB_FLAGGED(victim, MOB_NOSIELENCE) ||
        (ch != victim && general_savingthrow(victim, SAVING_WILL, modi - GET_LEVEL(victim)  / 2))
       )
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].type       = SPELL_SIELENCE;
    af[0].duration   = spellnum == SPELL_POWER_SIELENCE ?
                       pc_duration(victim,2,level+3,4,6,0) :
                       pc_duration(victim,2,level+7,8,3,0);
    af[0].bitvector  = AFF_SIELENCE;
    af[0].battleflag = AF_BATTLEDEC;
    to_room          = "&K$n потерял$y способность разговаривать!&n";
    to_vict          = "&KВы потеряли способность разговаривать.&n";
    break;

  case SPELL_FLY:
    if (on_horse(victim))
       {send_to_char(NOEFFECT,ch);
        success = FALSE;
        break;
       }
    af[0].duration   = pc_duration(victim,10,level,4,0,0);
    af[0].bitvector  = AFF_FLY;
    to_room          = "$n медленно поднял$u в воздух.";
    to_vict          = "Вы медленно поднялись в воздух.";
    break;

  case SPELL_BLINK:
    af[0].duration   = pc_duration(victim,0,level,4,0,0);
    af[0].bitvector  = AFF_BLINK;
    to_room          = "$n начал$y мигать.";
    to_vict          = "Вы начали мигать.";
    break;

  case SPELL_NOFLEE:
    if (ch != victim && general_savingthrow(victim, savetype, modi))
       {send_to_char(NOEFFECT, ch);
        success = FALSE;
        break;
       }

    af[0].duration   = pc_duration(victim,0,level,3,0,0);
    af[0].bitvector  = AFF_NOFLEE;
    to_room          = "$n теперь не сможет убежать от $R.";
    to_vict          = "&KВы не сможете убежать от $R.&n";
    break;

  case SPELL_LIGHT:
    af[0].duration   = pc_duration(victim,12,level,10,4,0);
    af[0].bitvector  = AFF_HOLYLIGHT;
    to_room          = "$n засветил$u ярким светом изнутри.";
    to_vict          = "Вы засветились изнутри ярким светом, освещая комнату.";
    break;

  case SPELL_DARKNESS:
    af[0].duration   = pc_duration(victim,6,level,4,4,0);
    af[0].bitvector  = AFF_HOLYDARK;
    to_room          = "$n погрузил$y комнату во мрак.";
    to_vict          = "Вы погрузили комнату во тьму.";
    break;
   
  case SPELL_ZNAK_IRGEN:
    af[0].type	     = SPELL_ZNAK_IRGEN;
    af[0].location   = APPLY_SAVING_STABILITY;
    af[0].modifier  -= level*2+10*GET_REMORT(ch);
    af[0].duration   = pc_duration(victim,50,0,0,0,0);
    af[0].battleflag = 0; 

    af[1].type	     = SPELL_ZNAK_IRGEN;
    af[1].location   = APPLY_SAVING_WILL;
    af[1].modifier  -= level*2+10*GET_REMORT(ch);
    af[1].duration   = af[0].duration;
    af[1].battleflag = 0;

    to_room = "Вокруг $r появилась некая субстанция!";
    to_vict = "Вокруг Вас появилась защитная субстанция!";
    break;

  case SPELL_ZNAK_AKSY:
    af[0].type	     = SPELL_ZNAK_AKSY;
    af[0].location   = APPLY_RESIST_FIRE;
    af[0].modifier   = level*2+10*GET_REMORT(ch);
    af[0].duration   = pc_duration(victim,50,0,0,0,6);
    af[0].battleflag = 0;
	
    af[1].type	     = SPELL_ZNAK_AKSY;
    af[1].location   = APPLY_SAVING_REFLEX;
    af[1].modifier  -= level*2+10*GET_REMORT(ch);
    af[1].duration   = af[0].duration;
    af[1].battleflag = 0;

    to_room = "Вокруг $r появилась упругая оболочка!";
    to_vict = "Вокруг Вас появилась упругая оболочка!";
    break;

	case SPELL_ZNAK_KVEN:
    af[0].type	     = SPELL_ZNAK_KVEN;
    af[0].location   = APPLY_ABSORBE;
	//af[0].modifier  += 70;
	af[0].modifier  += level*2+10*GET_REMORT(ch);
    af[0].duration   = pc_duration(victim,50,0,0,0,6);
    af[0].battleflag = 0;

    to_room = "Вокруг $r появилась защитная оболочка!";
    to_vict = "Вокруг Вас появилась защитная оболочка!";
    break;

	
  case SPELL_ZNAK_GELIOTROP:
    af[0].type	     = SPELL_ZNAK_GELIOTROP;
    af[0].bitvector  = AFF_AIRSHIELD;
    af[0].battleflag = 0;
    af[0].duration   = pc_duration(victim,50,0,0,0,6);

    to_room = "Вокруг $r появился серебристый щит!";
    to_vict = "Вокруг Вас появился серебристый щит!";
  break;    
}

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (IS_NPC(victim) && success)
     for (i = 0; i < MAX_SPELL_AFFECTS && success; i++)
         if (AFF_FLAGGED(victim, af[i].bitvector))
		 { if (IN_ROOM(ch) == IN_ROOM(victim))
			 send_to_char(NOEFFECT, ch);
	         success = FALSE;
         }

  /*
   * If the victim is already affected by this spell
   */

  		 //если спел дружественный, указываем, что спелл будет обновлен по времени ниже
 	if (!SpINFO.violent && affected_by_spell(victim, spellnum) && success)
	    	update_spell = TRUE;
	

	if ((ch != victim) && affected_by_spell(victim, spellnum) && success && !(update_spell || accum_duration || accum_affect))
		{ if (IN_ROOM(ch) == IN_ROOM(victim))
			  send_to_char(NOEFFECT, ch);
		      success = FALSE;
		}



   for (i = 0; success && i < MAX_SPELL_AFFECTS; i++)
   {       af[i].type = spellnum;
	   if (af[i].bitvector || af[i].location != APPLY_NONE)
		{ af[i].duration = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_EFFECT,af[i].duration);
          if (update_spell)//обновляем время действия заклинашки
				affect_join_fspell(victim, af + i);
		  else
				affect_join(victim, af + i, accum_duration, FALSE, accum_affect, FALSE);
		}
   }  

   if (success)
      {if (heal)
          GET_HIT(victim) = MAX(GET_HIT(victim), GET_REAL_MAX_HIT(victim));
       if (spellnum == SPELL_POISON)
          victim->Poisoner = GET_ID(ch);
       if (to_vict != NULL)
          act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
       if (to_room != NULL)
          act(to_room, TRUE, victim, 0, ch, TO_ROOM);
      }
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */

void perform_mag_groups(int level, struct char_data * ch,
			struct char_data * tch, int spellnum, int savetype)
{
  switch (spellnum) {
   case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, SPELL_HEAL, savetype);
    break;
   case SPELL_GROUP_CURE_CRITIC:
	mag_points(level, ch, tch, SPELL_CURE_CRITIC, savetype);
    break;
  case SPELL_GROUP_BLESS:
    mag_affects(level, ch, tch, SPELL_BLESS, savetype);
    break;
  case SPELL_GROUP_HASTE:
    mag_affects(level, ch, tch, SPELL_HASTE, savetype);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
    break; 
  case SPELL_STRENGTH:
  case SPELL_GROUP_STRENGTH:
    mag_affects(level, ch, tch, SPELL_STRENGTH, savetype);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL);
    break;
  case SPELL_SANCTUARY:
    mag_affects(level, ch, tch, SPELL_SANCTUARY, savetype);
    break;
  case SPELL_GROUP_INVISIBLE:
    mag_affects(level, ch, tch, SPELL_INVISIBLE, savetype);
    break;
  case SPELL_GROUP_REFRESH:
    mag_points(level, ch, tch, SPELL_REFRESH, savetype);
	break;
  }
}

ASPELL(spell_eviless)
{struct char_data *tch;

 for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    if (tch && IS_NPC(tch) && tch->master == ch && MOB_FLAGGED(tch,MOB_CORPSE))
       mag_affects(GET_LEVEL(ch),ch,tch,SPELL_EVILESS,0);
}

/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */

void mag_groups(int level, CHAR_DATA * ch, int spellnum, int savetype, int use_glory)
{
  if (!ch || !AFF_FLAGGED(ch, AFF_GROUP)) return;

  CHAR_DATA *k = (ch->master ? ch->master : ch);
  std::vector<char_data*> followers;
  followers.push_back(k);
  for (follow_type *f = k->followers; f; f = f->next)
  {
      if (IS_NPC(f->follower))
      {
          if (AFF_FLAGGED(f->follower,AFF_CHARM))
                followers.push_back(f->follower);
          continue;
      }
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
          continue;
      followers.push_back(f->follower);
      for (follow_type *gc = f->follower->followers; gc; gc = gc->next)
      {
          if (IS_NPC(gc->follower) && (AFF_FLAGGED(gc->follower,AFF_CHARM)))
              followers.push_back(gc->follower);
      }
  }

  for (int i=0,e=followers.size(); i<e; ++i)
  {
      if (ch != followers[i] && ch->in_room == followers[i]->in_room)
          perform_mag_groups(level, ch, followers[i], spellnum, savetype);
  }
  perform_mag_groups(level, ch, ch, spellnum, savetype);

  /*CHAR_DATA *tch, *k;
  struct follow_type *f, *f_next;

  if (ch == NULL)
    return;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    return;
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (tch->in_room != ch->in_room)
      continue;
    if (!AFF_FLAGGED(tch, AFF_GROUP))
      continue;
    if (ch == tch)
      continue;
    perform_mag_groups(level, ch, tch, spellnum, savetype);
  }

  if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room) )
    perform_mag_groups(level, ch, k, spellnum, savetype);
  perform_mag_groups(level, ch, ch, spellnum, savetype);*/
}


/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented as of Circle 3.0.
 */

struct char_list_data
{ struct char_data      *ch;
  struct char_list_data *next;
};


void mag_masses(int level, struct char_data * ch, int spellnum, int savetype, int use_glory)
{
  struct char_data *tch;
  struct char_list_data *char_list = NULL, *char_one;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {CREATE(char_one, struct char_list_data, 1);
       char_one->ch   = tch;
       char_one->next = char_list;
       char_list      = char_one;
      }
      switch (spellnum)
      {case SPELL_MASS_BLINDNESS:
            send_to_char("У Вас над головой возникла яркая вспышка, которая ослепила все живое.\r\n",ch);
            break;
        case SPELL_MASS_HOLD:
            send_to_char("Вы сжали зубы от боли, когда из Вашего тела вырвалось множество невидимых каменных лучей.\r\n",ch);
            break;

        case SPELL_MASS_CURSE:
            send_to_char("Медленно оглянувшись, Вы прошептали древние слова.\r\n",ch);
            break;
        case SPELL_MASS_SIELENCE:
            send_to_char("Поведя вокруг грозным взгядом, Вы заставили всех замолчать.\r\n",ch);
            break;
        case SPELL_MASS_SLOW:
           send_to_char("Вы заставили замедлить движения находящихся с Вами рядом.\r\n",ch);
            break;
        default:
            break;
      
      }

  for (char_one = char_list; char_one; char_one = char_one->next)
      {
       tch = char_one->ch;
       if (IS_IMMORTAL(tch))                            /* immortal   */
          {char_one->ch = NULL;
           continue;
          }
       	if (!HERE(tch))
  	  continue;

       if (same_group(ch,tch) && !IS_SET(SpINFO.routines, MAG_ALL))
          {char_one->ch = NULL;
           continue;                                     /* same groups */
          }

       switch (spellnum)
       { case SPELL_MASS_BLINDNESS:
           // act("&KВспышка света, вызванная $t, ослепила Вас.&n",TRUE,ch,0,tch,TO_ROOM);
            mag_affects(level, ch, tch, SPELL_BLINDNESS, SAVING_STABILITY);
            break;
        case SPELL_MASS_HOLD:
           // act("&CВаше тело сковала неведомая сила, вызванная $t.&n",TRUE,ch,0,tch,TO_ROOM);
            mag_affects(level, ch, tch, SPELL_HOLD,  SAVING_WILL);
            break;
        case SPELL_MASS_CURSE:
          //  act("&K$n злобно посмотрел$y на Вас и начал$y шептать древние слова.&n",TRUE,ch,0,tch,TO_ROOM);
            mag_affects(level, ch, tch, SPELL_CURSE,  SAVING_WILL);
            break;
        case SPELL_MASS_SIELENCE:
         //   act("&KВы встретились взглядом с $t, и Ваше горло что-то сковало.&n",TRUE,ch,0,tch,TO_ROOM);
            mag_affects(level, ch, tch, SPELL_SIELENCE,  SAVING_WILL);
            break;
        case SPELL_MASS_SLOW:
          //  act("&K$n прошептал$y проклятия и Ваши движения замедлились.&n",TRUE,ch,0,tch,TO_ROOM);
            mag_affects(level, ch, tch, SPELL_SLOW,  SAVING_WILL);
            break;
        case SPELL_GROUP_FLY:
		//	act("&K$n что-то произнес$y и Вы потеряли всякую волю и надежду.&n",TRUE,ch,0,tch,TO_VICT);
            mag_affects(level, ch, tch, SPELL_FLY,  SAVING_WILL); 
			break;
		default:
            break;
        }
      }

  for (char_one = char_list; char_one; char_one = char_list)
      {char_list = char_one->next;
       cast_reaction(char_one->ch,ch,spellnum);
       free(char_one);
      }
}

/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
*/
void mag_areas(int level, struct char_data * ch, int spellnum, int savetype, int use_glory)
{
  struct char_data *tch;
  const  char *to_char = NULL, *to_room = NULL;
  struct char_list_data *char_list = NULL, *char_one;

  if (ch == NULL)
     return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum)
  {
  case SPELL_FORBIDDEN:
    if (world[IN_ROOM(ch)].forbidden)
       {send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
        return;
       }
    world[IN_ROOM(ch)].forbidden = 1 + (GET_LEVEL(ch) + 14) / 15;
    send_to_char("Вы запечатали магией все входы в комнату.\r\n", ch);
    return;
  case SPELL_EARTHQUAKE:
    to_char = "Ваши чары произвели &cземлетрясение&n, содрогнувшее все живое!&n";
    to_room = "Чары $r произвели &cземлетрясение&n, содрогнувшее все живое!";
    break;
  case SPELL_FIREBALL:
    to_char = "Обжигая врагов ваш &Rогненный шар&n сорвался в их сторону!&n";
    to_room = "Обжигающий &Rогненный шар&n $r опалил все живое!&n";
    break;
  case SPELL_ARMAGEDDON:
    to_char = "Прошептав слово &Wиссушение&n вы объявили Судный День!";
    to_room = "Ужасающий вихрь принес &Wиссушение&n, подчиняющееся $d!";
    break;
  case SPELL_ICESTORM:
    to_char = "Вы вызвали &Bледяной шторм&n из тысяч холодных камней!";
    to_room = "$n вызвал$y &Bледяной шторм&n и жестом направил$y его!";
    break;
  case SPELL_METEORSTORM:
    to_char = "Вы нарисовали в воздухе круг смерти!";
    to_room = "$n нарисовал$y в воздухе круг смерти!";
    break;
  case SPELL_FIREBLAST:
    to_char = "Вы прошептали древнюю молитву и все осветило ярким солнечным светом!";
    to_room = "$n что-то пробормотал$y. Ослепляющий солнечный луч ударил с неба!";
    break;
  default:
    return;
  }
  if (to_char != NULL)
     act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
     act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {CREATE(char_one, struct char_list_data, 1);
       char_one->ch   = tch;
       char_one->next = char_list;
       char_list      = char_one;
      }

  for (char_one = char_list; char_one; char_one = char_one->next)
      {tch = char_one->ch;
       if (IS_IMMORTAL(tch))               /* immortal    */
          continue;
       if (IN_ROOM(ch) == NOWHERE ||       /* Something killed in process ... */
           IN_ROOM(tch) == NOWHERE
          )
          continue;
	   if (!HERE(tch))
		  continue;

       switch (spellnum)
       {case SPELL_DISPEL_EVIL:
        case SPELL_DISPEL_GOOD:
             mag_damage(level, ch, tch, spellnum, SAVING_WILL);
             break;
        default:
             if (tch != ch && !same_group(ch, tch))
                mag_damage(level, ch, tch, spellnum, SAVING_WILL);
             break;
       }
      }

  for (char_one = char_list; char_one; char_one = char_list)
      {char_list = char_one->next;
       free(char_one);
      }
}



void mag_char_areas(int level, struct char_data * ch,
                    struct char_data *victim, int spellnum, int savetype, int use_glory)
{
  struct char_data *tch;
  const char *to_char = NULL, *to_room = NULL;
  int   decay=2;
  struct char_list_data *char_list = NULL, *char_one;
  int may_cast_here (struct char_data *caster, struct char_data *victim, int spellnum); 

  if (ch == NULL)
      return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum)
  {
  case SPELL_CHAIN_LIGHTNING:
    to_char = "&yВы подняли руки к небу и оно осветилось яркими вспышками!&n";
    to_room = "&y$n поднял$y руки к небу и оно осветилось яркими вспышками!&n";
   if (level > 20)
      decay   = level  / 2;
   else
  		decay   = 3; 
   break;
  default:
    return;
  }

  if (to_char != NULL)
     act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
     act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      {CREATE(char_one, struct char_list_data, 1);
       char_one->ch   = tch;
       char_one->next = char_list;
       char_list      = char_one;
      }

  /* First - damage victim */
  mag_damage(level, ch, victim, spellnum, savetype);
  level -= decay;

  /* Damage other room members */
  for (char_one = char_list; char_one && level > 0; char_one = char_one->next)
      {tch = char_one->ch;
       if (IS_IMMORTAL(tch) || tch == victim || tch == ch)
          continue;
       if (IN_ROOM(ch) == NOWHERE || IN_ROOM(tch) == NOWHERE)
          continue;
        if (!HERE(tch))
	  continue;
       if (same_group(ch, tch))
          continue;
       if (!may_cast_here(ch, tch, spellnum))
		   continue;
       mag_damage(level, ch, tch, spellnum, savetype);
       level -= decay;
      }

  for (char_one = char_list; char_one; char_one = char_list)
      {char_list = char_one->next;
       free(char_one);
      }
}

/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = {
  "\r\n",
  "$n сделал$y магический жест рукой и Вы ощутили дуновенье сильного ветра!",
  "$n поднял$y труп!",
  "$N появил$y из густого синего дыма!",
  "$N появил$y из густого зеленого дыма!",
  "$N появил$y из густого красного дыма!",
  "$n сделал$y магический жест рукой и Вы почувствовали сильный холодный ветер!",
  "$n сделал$y магический жест рукой и Ваши волосы на голове встали дыбом!",
  "$n сделал$y магический жест рукой и кровь в Ваших жилах застыла от ужаса!",
  "$n сделал$y магический жест рукой и Вас закружил сильный вихрь!",
  "$n волшебно раздвоил$u!",
  "$n оживил$y труп!",
  "Преданное существо поклонилось хозяину.",
  "Славное существо начало служить своему хозяину."
};

/*
 * Keep the \r\n because these use send_to_char.
 *There are no such creatures.*/
const char *mag_summon_fail_msgs[] = {
  "\r\n",
  "Здесь нет никого с таким именем.\r\n",
  "Ох, ох, сорвалось...\r\n",
  "У Вас ничего не вышло.\r\n",
  "Черт! Ничего не вышло\r\n",
  "У Вас это не получилось!\r\n",
  "Вы потерпели неудачу.\r\n",
  "Здесь нет нужного трупа!\r\n"
};

/* These mobiles do not exist. */
#define MOB_MONSUM_I		130
#define MOB_MONSUM_II		140
#define MOB_MONSUM_III		150
#define MOB_GATE_I		    160
#define MOB_GATE_II		    170
#define MOB_GATE_III		180

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE	20	/* Only one for now. */
#define MOB_BONEDOG		    10 //моб до 24 уровня
#define MOB_ZOMBIE		    11 //моб до 15 уровня
#define MOB_BONEDRAGON	    12 //моб выше 24 уровня
#define MOB_KEEPER          13 //по спелу "защитник" вызывается
#define MOB_FIREKEEPER      14 //по спелу "огненый защитнки" вызывается
#define MOB_SKELETON		15 //моб до 9 уровня
#define MOB_ANIMAL_1	1803 //животное уровня
#define MOB_ANIMAL_2	1800 //животное уровня
#define MOB_ANIMAL_3	1801 //животное уровня
#define MOB_ANIMAL_4	1804 //животное уровня
#define MOB_ANIMAL_5	1802 //животное уровня
void mag_summons(int level, struct char_data * ch, struct obj_data * obj,
		      int spellnum, int savetype, int use_glory)
{
  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  struct affected_type af;
  int pfail = 0, msg = 0, fmsg = 0, num = 1, handle_corpse = FALSE, keeper = FALSE;
  int glory_price = 0;
  mob_vnum mob_num;

  if (ch == NULL)
    return;

  switch (spellnum) {
  case SPELL_CLONE:
    msg = 10;
    fmsg = number(3, 5);	/* Random fail message. */
    mob_num = MOB_BONEDRAGON;
    pfail = 50;	/* 50% failure, should be based on something later. */
    keeper  = TRUE;
	break;

  case SPELL_SUMMON_KEEPER:
  case SPELL_SUMMON_FIREKEEPER:
    msg     = 12;
    fmsg    = number(2, 6);
    mob_num = spellnum == SPELL_SUMMON_KEEPER ? MOB_KEEPER : MOB_FIREKEEPER;
    pfail   = 50;
    keeper  = TRUE;
    break;
	
   case SPELL_SUMMON_ANIMAL:
	msg     = 12;
    fmsg    = number(2, 6);
	
	if ( GET_LEVEL(ch) < 14 ) {
		mob_num = MOB_ANIMAL_1;
	}	else if ( GET_LEVEL(ch) < 17 ) {
		mob_num = MOB_ANIMAL_2;
    }	else if ( GET_LEVEL(ch) < 20 ) {
		mob_num = MOB_ANIMAL_3;
    }	else if ( GET_LEVEL(ch) < 26 ) {
		mob_num = MOB_ANIMAL_4;
    }	else {
		mob_num = MOB_ANIMAL_5;
    }

    if (use_glory)
    {
        int level = GET_LEVEL(ch);
        if (level < 16)
        {
            send_to_char("Вы еще не готовы к призыву славных мобов.\r\n", ch);
            return;
        }

        pfail = -1;
        msg = 13;
        int mobs[15] = { 1817,1829,1827,1826,1820,1824,1825,1821,1822,1816,1823,1828,1815,1819,1818 };
        if (level > 30) level = 30;

        int index = level - 16;
        mob_num = mobs[index];
    }
    else
	    pfail   = 100 - (2 * GET_LEVEL(ch) + GET_WIS(ch));
    keeper  = TRUE;
    break;

  case SPELL_ANIMATE_DEAD:
    if (obj == NULL || !IS_CORPSE(obj)) {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    mob_num = GET_OBJ_VAL(obj,2);
    if (mob_num <= 0 )
       mob_num = MOB_SKELETON;
    else
       {pfail = 110 - con_app[GET_CON(mob_proto+real_mobile(mob_num))].ressurection -
                number(1,GET_LEVEL(ch));
        if (GET_LEVEL(mob_proto+real_mobile(mob_num)) <= 9)
           mob_num = MOB_SKELETON;
        else
        if (GET_LEVEL(mob_proto+real_mobile(mob_num)) <= 15)
           mob_num = MOB_ZOMBIE;
        else
        if (GET_LEVEL(mob_proto+real_mobile(mob_num)) <= 24)
           mob_num = MOB_BONEDOG;
        else
           mob_num = MOB_BONEDRAGON;
       }
	handle_corpse = TRUE;
    msg           = number(1, 9);
    fmsg          = number(2, 6);	
    //  pfail = 10;
    break;
	

    case SPELL_RESSURECTION:
    if (obj == NULL || !IS_CORPSE(obj))
       {act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
        return;
       }
    if ((mob_num = GET_OBJ_VAL(obj,2)) <= 0)
       {send_to_char("Вы не можете поднять труп этого монстра!\r\n",ch);
        return;
       }
    handle_corpse = TRUE;
    msg           = 11;
    fmsg          = number(2, 6);
    pfail = 110 - con_app[GET_CON(mob_proto+real_mobile(mob_num))].ressurection -
            number(1,GET_LEVEL(ch));
    break;

  default:
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    send_to_char("Вы не можете этого сделать в таком состоянии!\r\n", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && number(0, 101) < pfail)
     {send_to_char(mag_summon_fail_msgs[fmsg], ch);
      if (handle_corpse)
         extract_obj(obj);
      return;
     }
  
    if (!(mob = read_mobile(mob_num, VIRTUAL))) 
		{ send_to_char("Вы не знаете как это сделать.\r\n", ch);
			return;
		}
    
    if (spellnum == SPELL_SUMMON_ANIMAL)
    {
        struct follow_type *k;
        int cha_summ = 0;
        for (k = ch->followers; k; k = k->next)
        {
            if (AFF_FLAGGED(k->follower,AFF_CHARM) && k->follower->master == ch)  { cha_summ++; }            
        }
        if (cha_summ != 0)
        {
            send_to_char("Вы не можете призвать еще одно животное.\r\n", ch);
            return;
        }
        if (use_glory && GET_GLORY(ch) < glory_price)
        {
            send_to_char("В Мире совсем мало кому известно о вашей славе.\r\n", ch);
            return;
        }
        
        char_to_room(mob, ch->in_room);
        if (use_glory)
        {
            GET_GLORY(ch) -= glory_price;
            sprintf(buf, "Ваша слава заставила %s покорно служить вам.\r\n", GET_RNAME(mob) );
            send_to_char(buf, ch);
        }
    }
    else
    {
        char_to_room(mob, ch->in_room);
        if (!check_charmee(ch,mob,spellnum))
         {extract_char(mob,FALSE);
          if (handle_corpse)
             extract_obj(obj);
          return;
         }
    }

    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;    

  if (spellnum == SPELL_SUMMON_ANIMAL)
  {
      af.duration = MAX(1, GET_WIS(ch) / 3);
  }
  else
  {
    if (weather_info.moon_day < 14)
         af.duration     = pc_duration(mob,GET_REAL_CHA(ch) + number(0,weather_info.moon_day % 14),0,0,0,0);
    else
         af.duration     = pc_duration(mob,GET_REAL_CHA(ch) + number(0,14 - weather_info.moon_day % 14),0,0,0,0);
  }

  af.type            = SPELL_CHARM;
  af.modifier        = 0;
  af.location        = 0;
  af.bitvector       = AFF_CHARM;
  af.battleflag      = 0;
  affect_to_char(mob, &af);
  if (keeper)
     {af.bitvector                = AFF_HELPER;
      affect_to_char(mob, &af);
      GET_SKILL(mob,SKILL_RESCUE) = 100;
     }
	 
  if (spellnum == SPELL_SUMMON_ANIMAL) {
	SET_BIT(MOB_FLAGS(mob, MOB_CLONE), MOB_CLONE);
  }
  
	
	// SET_BIT(mob->char_specials.saved.affected_by.flags[0], AFF_CHARM);
    if (spellnum == SPELL_CLONE) 
	{ sprintf(buf2, "клон %s", GET_NAME(ch));
	  mob->player.name = str_dup(buf2);
      
	  sprintf(buf2, "клон %s", GET_NAME(ch));
	  mob->player.short_descr = str_dup(buf2);
      
      sprintf(buf2, "клон %s", GET_RNAME(ch));
	  mob->player.rname = str_dup(buf2);
	 
	  sprintf(buf2, "клон %s", GET_DNAME(ch));
	  mob->player.dname = str_dup(buf2);
      
      sprintf(buf2, "клон %s", GET_VNAME(ch));
	  mob->player.vname = str_dup(buf2);
	  
      sprintf(buf2, "клон %s", GET_TNAME(ch));
	  mob->player.tname = str_dup(buf2);
	  
      sprintf(buf2, "клон %s", GET_PNAME(ch));
	  mob->player.pname = str_dup(buf2);
	  	
      GET_STR(mob)       = GET_STR(ch);
      GET_INT(mob)       = GET_INT(ch);
      GET_WIS(mob)       = GET_WIS(ch);
      GET_DEX(mob)       = GET_DEX(ch);
      GET_CON(mob)       = GET_CON(ch);
      GET_CHA(mob)       = GET_CHA(ch);

      GET_LEVEL(mob)     = GET_LEVEL(ch);
      GET_HR(mob)		 = GET_HR(ch);
      GET_AC(mob)        = GET_AC(ch);
      GET_DR(mob)		 = GET_DR(ch);

      GET_MAX_HIT(mob)   = GET_MAX_HIT(ch);
      GET_HIT(mob)       = GET_MAX_HIT(ch);
      mob->mob_specials.damnodice   = 0;
      mob->mob_specials.damsizedice = 0;
      GET_GOLD(mob)      = 0;
      GET_EXP(mob)       = 0;

      GET_POS(mob)         = POS_STANDING;
      GET_DEFAULT_POS(mob) = POS_STANDING;
      GET_SEX(mob)         = SEX_MALE;

      GET_CLASS(mob)       = GET_CLASS(ch);
      GET_WEIGHT(mob)      = GET_WEIGHT(ch);
      GET_HEIGHT(mob)      = GET_HEIGHT(ch);
      GET_SIZE(mob)        = GET_SIZE(ch);
      SET_BIT(MOB_FLAGS(mob, MOB_CLONE), MOB_CLONE);
      REMOVE_BIT(MOB_FLAGS(mob, MOB_MOUNTING), MOB_MOUNTING);// клоны не могут быть оседланы
	
	}
    act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
    load_mtrigger(mob);
    add_follower(mob, ch);

   SET_BIT(MOB_FLAGS(mob, MOB_NOTRAIN), MOB_NOTRAIN);

  
  if (handle_corpse) {
    for (tobj = obj->contains; tobj; tobj = next_obj) {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
    }
    extract_obj(obj);
  }
}

void mag_points(int level, struct char_data * ch, struct char_data * victim,
		     int spellnum, int savetype, int use_glory)
{
  int hit = 0, move = 0;

  if (victim == NULL)
     return;

              /* if (ch != victim && clan[find_clan_by_id(GET_CLAN(victim))].alignment == 1 &&
	           GET_CLAN_RANK(victim))  
 	 	{ act("Заклинание рассеялось около $r.",FALSE,victim,0,0,TO_ROOM);
                  act("Заклинание не подействовало на Вас.",FALSE,victim,0,0,TO_CHAR);
		  return;
		}
                */

  switch (spellnum)
  {
  case SPELL_CURE_LIGHT://исправлены
    hit = dice(5, 8) + (level+2) / 3;
    send_to_char("Вы почувствовали себя немного лучше.\r\n", victim);
    if (ch != victim)
      { sprintf(buf,"Вы слегка восстановили здоровье %s.\r\n", CAP(GET_DNAME(victim)));
        send_to_char(buf, ch);
      }
    break;
  case SPELL_CURE_SERIOUS:
    hit = dice(10,8) + (level+6) / 2;
    send_to_char("Вы почувствовали себя лучше.\r\n", victim);
    if (ch != victim)
     { sprintf(buf,"Вы заметно восстановили здоровье %s.\r\n", CAP(GET_DNAME(victim)));
       send_to_char(buf, ch);
     }
    break;
  case SPELL_CURE_CRITIC:
    hit = dice(19,8) + level * 2;
    send_to_char("Вы почувствовали себя намного лучше.\r\n", victim);
    if (ch != victim)
     { sprintf(buf,"Вы достаточно заметно восстановили здоровье %s.\r\n", CAP(GET_DNAME(victim)));
       send_to_char(buf, ch);
     }
    break;
  case SPELL_HEAL:
    hit = GET_REAL_MAX_HIT(victim);
    send_to_char("У Вас полностью восстановилось здоровье.\r\n", victim);
    if (ch != victim)
     { sprintf(buf,"Вы полностью восстановили здоровье %s.\r\n", CAP(GET_DNAME(victim)));
       send_to_char(buf, ch);
     }
    break;
  case SPELL_REFRESH:
    move = GET_REAL_MAX_MOVE(victim);
    send_to_char("Ваша энергия полностью восстановилась.\r\n", victim);
    if (ch != victim)
     { sprintf(buf,"Вы восстановили энергию %s.\r\n", CAP(GET_DNAME(victim)));
       send_to_char(buf, ch);
     }
    break;
  case SPELL_EXTRA_HITS:
    hit = dice(10,level/3) + level;
    send_to_char("Вы ощутили приток жизненной энергии.\r\n",victim);
    if (ch != victim)
     { sprintf(buf,"Вы увеличили жизненную энергию %s.\r\n", CAP(GET_DNAME(victim)));
       send_to_char(buf, ch);
     }
    break;
  case SPELL_FULL:
    if (!IS_NPC(victim) && !WAITLESS(victim))
       {GET_COND(victim, THIRST) = 24;
        GET_COND(victim, FULL)   = 24;
        send_to_char("Вы полноcтью насытились.\r\n", victim);
        if (ch != victim)
          {  sprintf(buf,"Вы насытили %s.\r\n", CAP(GET_VNAME(victim)));
            send_to_char(buf, ch);
	    }       
	 }
    else
	send_to_char("А Вы и не были голодны!\r\n", victim);
    break;	  
}

  hit = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_EFFECT,hit);

  if (hit && FIGHTING(victim) && ch != victim)
       pk_agro_action(ch,FIGHTING(victim));


  if (spellnum == SPELL_EXTRA_HITS)
     GET_HIT(victim) = MAX(GET_HIT(victim),
                           MIN(GET_HIT(victim)+hit, 
			       GET_REAL_MAX_HIT(victim) + GET_REAL_MAX_HIT(victim) * 33 / 100
			      )
			  );
  else
  if (GET_HIT(victim) < GET_REAL_MAX_HIT(victim))
     GET_HIT(victim) = MIN(GET_HIT(victim)+hit, GET_REAL_MAX_HIT(victim));

  GET_MOVE(victim) = MIN(GET_REAL_MAX_MOVE(victim), GET_MOVE(victim) + move);
  update_pos(victim);
}


void mag_unaffects(int level, struct char_data * ch, struct char_data * victim,
		        int spellnum, int type, int use_glory)
{ struct affected_type *hjp;
  int    spell = 0, remove = 0;
  const  char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum)
  {
  case SPELL_HEAL:
    return;
  case SPELL_CURE_BLIND:
    spell   = SPELL_BLINDNESS;
    to_vict = "Вы снова можете видеть!";
    to_room = "$n прозрел$y.";
    break;
  case SPELL_REMOVE_POISON:
    spell   = SPELL_POISON;
    to_vict = "Яд Вас больше не беспокоит.";
    to_room = "$n больше не отравлен$y.";
    break;
  case SPELL_CURE_PLAQUE:
    spell   = SPELL_PLAQUE;
    to_vict = "Вас перестала мучить лихорадка.";
    to_room = "$n перестала мучить лихорадка.";
    break;
  case SPELL_REMOVE_CURSE:
    spell   = SPELL_CURSE;
    to_vict = "Боги вернули Вам свое покровительство.";
    to_room = "Жизнерадостная улыбка вновь озарила лицо $v.";
    break;
  case SPELL_FAST_REGENERATION:
    spell   = SPELL_BATTLE;
    to_vict = "Ваши раны затянулись!";
    to_room = "$n выглядит лучше.";
    break;
  case SPELL_REMOVE_HOLD:
    if (affected_by_spell(victim, SPELL_POWER_HOLD))
        spell   = SPELL_POWER_HOLD;
    else
        spell   = SPELL_HOLD;
    to_vict = "Вы снова обрели способность двигаться.";
    to_room = "$n снова может двигаться.";
    break;
  case SPELL_REMOVE_SIELENCE:
    spell   = SPELL_SIELENCE;
    to_vict = "Вы снова можете разговаривать.";
    to_room = "$n обрел$y способность разговаривать.";
    break;
  case SPELL_DISPELL_MAGIC:
    for (spell=0, hjp = victim->affected; hjp; hjp = hjp->next,spell++);
     if (!spell)
       {send_to_char(NOEFFECT,ch);
        return;
       }
      spell = number(1,spell);
    
    for (hjp=victim->affected; spell > 1 && hjp; hjp = hjp->next,spell--);
    
     for (hjp=victim->affected; spell > 1 && hjp; hjp = hjp->next,spell--);
      if ((!hjp							|| 
        hjp->bitvector == AFF_CHARM				|| 
	    hjp->type      == SPELL_CHARM			||
		hjp->bitvector == AFF_PRISMATICAURA		|| 
	    hjp->type	   == SPELL_PRISMATICAURA		||
        !spell_info[hjp->type].name				||
        *spell_info[hjp->type].name == '!'
       ) && !IS_GRGOD(ch))
       {send_to_char(NOEFFECT,ch);
        return;
       }
       
    spell  = hjp->type;
    remove = TRUE;
    break;
  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
    return;
  }

  if (!affected_by_spell(victim, spell))
     { if (spell == SPELL_BATTLE)
	  return;
       if (spellnum != SPELL_HEAL)
          send_to_char(NOEFFECT, ch);
          return;
     }
  spellnum = spell;
  if (ch != victim && !remove)
     { if (IS_SET(SpINFO.routines, NPC_AFFECT_NPC))
          pk_agro_action(ch,victim);
       else
       if (IS_SET(SpINFO.routines, NPC_AFFECT_PC) && FIGHTING(victim))
          pk_agro_action(ch,FIGHTING(victim));
       }
  
  affect_from_char(victim, spell);
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

void mag_alter_objs(int level, struct char_data * ch, struct obj_data * obj,
		            int spellnum, int savetype, int use_glory)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  /*	if (IS_OBJ_STAT(obj, ITEM_NOALTER)) { разобраться и , сделать заклу устойчивая магия.
		act("$o устойчив$y к Вашей магии.", TRUE, ch, obj, 0, TO_CHAR);
		return 0;
	}*/


  switch (spellnum)
  { case SPELL_BLESS:
      if (!IS_OBJ_STAT(obj, ITEM_BLESS) &&
          (GET_OBJ_WEIGHT(obj) <= 5 * GET_LEVEL(ch))
         )
	 {SET_BIT(GET_OBJ_EXTRA(obj, ITEM_BLESS), ITEM_BLESS);
	  if (IS_OBJ_STAT(obj,ITEM_NODROP))
			{ REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
              if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                 GET_OBJ_VAL(obj, 2)++;
             }
          GET_OBJ_MAX(obj) += MAX(GET_OBJ_MAX(obj) >> 2, 1);
	  GET_OBJ_CUR(obj)  = GET_OBJ_MAX(obj);
	  to_char = "$o вспыхнул$Y нежно-голубым светом и тут же погас$Q.";
         }
      break;
    case SPELL_CURSE:
      if (!IS_OBJ_STAT(obj, ITEM_NODROP))
         {SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
	  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
             {if (GET_OBJ_VAL(obj,2) > 0)
                 GET_OBJ_VAL(obj, 2)--;
             }
          else
          if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
             {GET_OBJ_VAL(obj,0)--;
              GET_OBJ_VAL(obj,1)--;
             }
	  to_char = "$o вспыхнул$Y красным светом и тут же погас$Q.";
         }
      break;
    case SPELL_INVISIBLE:
      if (!IS_OBJ_STAT(obj, ITEM_NOINVIS) && !IS_OBJ_STAT(obj, ITEM_INVISIBLE))
		{ SET_BIT(GET_OBJ_EXTRA(obj, ITEM_INVISIBLE), ITEM_INVISIBLE);
	      to_char = "$o растворил$U в воздухе.";
        }
      break;
    case SPELL_POISON: //изменить!! в зависимости от уровня игрока
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
           (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)) && 
		   !GET_OBJ_VAL(obj, 3))
			{GET_OBJ_VAL(obj, 3) = GET_LEVEL(ch)/3;
	         to_char = "$o отравлен$Y.";
			}
		
	  if (GET_OBJ_TYPE(obj) == ITEM_FOOD && 
			!GET_OBJ_VAL(obj, 1))
           {GET_OBJ_VAL(obj, 1) = GET_LEVEL(ch)/3;
            to_char = "$o отравлен$Y.";
           }
      break;
    case SPELL_REMOVE_CURSE:
      if (IS_OBJ_STAT(obj,ITEM_NODROP))
		{ REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
          if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
             GET_OBJ_VAL(obj, 2)++;
          to_char = "$o вспыхнул$Y розовым светом и тут же погас$Q.";
         }
      break;
    case SPELL_REMOVE_POISON:
        if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
           (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)) && 
		   !GET_OBJ_VAL(obj, 3))
			{GET_OBJ_VAL(obj, 3) = 0;
	         to_char = "$o вновь можно использовать для питья.";
			}
		if (GET_OBJ_TYPE(obj) == ITEM_FOOD && 
			!GET_OBJ_VAL(obj, 1))
           {GET_OBJ_VAL(obj, 1) = 0;
            to_char = "$o вновь можно употреблять в пищу!";
           }
      break;
	
    case SPELL_ACID:
      alterate_object(obj, number(GET_LEVEL(ch) * 2, GET_LEVEL(ch) * 4), 100);
      break;
    case SPELL_REPAIR:
      GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);
      to_char = "Вы полностью восстановили $3.";
      break;
  }

  if (to_char == NULL)
    send_to_char(NOEFFECT, ch);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
}

void mobile_affect_update(void)
{   
  struct timed_type    *timed, *timed_next;
  AFFECT_DATA	*af, *next;
  CHAR_DATA	 *i, *i_next;

  int    was_charmed = 0, count, charmed_msg = FALSE;

  for (i = character_list; i; i = i_next)
      {i_next            = i->next;
       charmed_msg       = FALSE;
       was_charmed       = FALSE;
       supress_godsapply = TRUE;
       for (af = i->affected; IS_NPC(i) && af; af = next)
           {next = af->next;
            if (af->duration >= 1)
               {af->duration--;
                if (af->type == SPELL_CHARM &&
			 !charmed_msg 		  &&
			 af->duration <= 1	  &&
			 GET_POS(i) > POS_SLEEPING)
                   {act("$n начал$y растерянно оглядываться по сторонам.",FALSE,i,0,0,TO_ROOM);
                    charmed_msg = TRUE;
                   }
               }
            else
            if (af->duration == -1)	
                af->duration = -1;  // GODS - unlimited
            else
               {if ((af->type > 0) && (af->type <= MAX_SPELLS))
	           {if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))
  	               {if (af->type > 0 &&
	                    af->type <= LAST_USED_SPELL &&
			    *spell_wear_off_msg[af->type]
			   )
	                   {show_spell_off(af->type, i);
	                    if (af->type == SPELL_CHARM || af->bitvector == AFF_CHARM)
	                       was_charmed = TRUE;
	                   }
	               }
	           }
	        affect_remove(i, af);
               }
           }
       supress_godsapply = FALSE;
       //log("[MOBILE_AFFECT_UPDATE->AFFECT_TOTAL] (%s) Start",GET_NAME(i));
       affect_total(i);
       //log("[MOBILE_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
       for (timed = i->timed; timed; timed = timed_next)
           {timed_next = timed->next;
            if (timed->time >= 1)
	       timed->time--;
            else
               timed_from_char(i, timed);
           }

       if (check_death_trap(i))
          continue;
       if (was_charmed)
          stop_follower(i, SF_CHARMLOST);
          
      }

  for (count = 0; count <= top_of_world; count++)
      check_death_ice(count,NULL);
}


void player_affect_update(void)
{
 AFFECT_DATA	*af, *next;
 CHAR_DATA	*i,  *i_next;   
 int    slots[MAX_SLOT], cnt, dec, sloti, slotn;

  for (i = character_list; i; i = i_next)
      {i_next = i->next;
       if (IS_NPC(i))
          continue;
       pulse_affect_update (i);     

  supress_godsapply = TRUE;
       for (af = i->affected; af; af = next)
           {next = af->next;
            if (af->duration >= 1)
	           af->duration--;
            else
            if (af->duration == -1)	
                af->duration = -1;
            else
               {if ((af->type > 0) && (af->type <= MAX_SPELLS))
                   {if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))  	
		           {if (af->type > 0 &&
			        af->type <= LAST_USED_SPELL &&
			        *spell_wear_off_msg[af->type]
			       )
	                       {show_spell_off(af->type, i);
	                       }
	                   }
	               }
	            affect_remove(i, af);
               }
           }
       for (cnt = 0; cnt < MAX_SLOT; slots[cnt++] = 0);

       for (cnt = 1; cnt <= LAST_USED_SPELL; cnt++)
           {
            if (IS_SET(GET_SPELL_TYPE(i,cnt), SPELL_TEMP) ||
                !GET_SPELL_MEM(i,cnt)                     ||
                WAITLESS(i)
               )
               continue;//изменить
            //if (spell_info[cnt].min_level[(int) GET_CLASS(i)] > GET_LEVEL(i))
            if (1+GET_CASTER_KRUG(i) < spell_info[cnt].slot_forc[(int) GET_CLASS(i)])  
			GET_SPELL_MEM(i,cnt) = 0;
            else
               { sloti = spell_info[cnt].slot_forc[(int) GET_CLASS(i)];
                 slotn = slot_for_char1(i,sloti);
                 slots[sloti] += GET_SPELL_MEM(i,cnt);
                 if (slotn < slots[sloti])
                    {dec = MIN(GET_SPELL_MEM(i,cnt), slots[sloti] - slotn);
                     GET_SPELL_MEM(i,cnt) -= dec;
                     slots[sloti]         -= dec;
                    }
               }
           }
       supress_godsapply = FALSE;
       //log("[PLAYER_AFFECT_UPDATE->AFFECT_TOTAL] Start");
       affect_total(i);
       //log("[PLAYER_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
      }
}




void mag_creations(int level, struct char_data * ch, int spellnum, int use_glory)
{
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */

  switch (spellnum) {
  case SPELL_CREATE_FOOD:
    z = 10;
    break;
 case  SPELL_CREATE_LIGHT: 
	 z = 25;
	 break;
  default:
    send_to_char("Такого заклинания не существует.\r\n", ch);
    return;
  }

  if (!(tobj = read_object(z, VIRTUAL))) {
    send_to_char("Я не могу это создать -(.\r\n", ch);
    log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
	    spellnum, z);
    return;
  }
  
  act("$n создал$y $3.", FALSE, ch, tobj, 0, TO_ROOM);
  act("Вы создали $3.", FALSE, ch, tobj, 0, TO_CHAR); 
  load_otrigger(tobj);

  
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
     {send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
      obj_to_room(tobj, IN_ROOM(ch));
      obj_decay(tobj);
     }
  else
  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(tobj) > CAN_CARRY_W(ch))
     {send_to_char("Вы не сможете унести такой вес.\r\n", ch);
      obj_to_room(tobj, IN_ROOM(ch));
      obj_decay(tobj);
     }
  else
     obj_to_char(tobj, ch);
  
  return;
}

