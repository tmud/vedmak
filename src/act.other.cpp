/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

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
#include "pk.h"


// extern variables
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern int auto_save;
extern struct char_data *ch_selling;
extern struct char_data *ch_buying;

// extern functions 
char *color_value(struct char_data *ch,int real, int max);
void appear(struct char_data * ch);
void write_aliases(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
void print_group(struct char_data *ch);
void die(struct char_data * ch, struct char_data *killer);
int  Crash_rentsave(struct char_data * ch, int cost, int num);
int  perform_group(struct char_data *ch, struct char_data *vict);
int  posi_value(int real, int max);

SPECIAL(shop_keeper);
// local functions 
ACMD(do_quit);
ACMD(do_save);
ACMD(do_courage);
ACMD(do_color);
ACMD(do_not_here);
ACMD(do_sneak);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_ptitle);
ACMD(do_visible);
ACMD(do_tren_skill);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_gen_comm);
ACMD(do_afk);
ACMD(do_prompt);

int awake_others(struct char_data * ch)
{int i;

 if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
    return (FALSE);

 if (IS_GOD(ch))
    return (FALSE);

 if (AFF_FLAGGED(ch, AFF_STAIRS)      || 
     AFF_FLAGGED(ch, AFF_SANCTUARY)   ||
     AFF_FLAGGED(ch, AFF_SINGLELIGHT) ||
     AFF_FLAGGED(ch, AFF_HOLYLIGHT)
    )
    return (TRUE);

 for (i = 0; i < NUM_WEARS; i++)
     {if (GET_EQ(ch,i))
         if ((GET_OBJ_TYPE(GET_EQ(ch,i))        == ITEM_ARMOR &&
              GET_EQ(ch,i)->obj_flags.material < MAT_COLOR) ||
             OBJ_FLAGGED(GET_EQ(ch,i), ITEM_HUM) ||
             OBJ_FLAGGED(GET_EQ(ch,i), ITEM_GLOW)
            )
            return (TRUE);
      }
 return (FALSE);
}


int check_awake(struct char_data *ch, int what)
{int i, retval = 0, wgt = 0;

 if (!IS_GOD(ch))
    {if (IS_SET(what, ACHECK_AFFECTS) &&
         (AFF_FLAGGED(ch, AFF_STAIRS) ||
          AFF_FLAGGED(ch, AFF_SANCTUARY)
         )
        )
        SET_BIT(retval, ACHECK_AFFECTS);

     if (IS_SET(what, ACHECK_LIGHT)  &&
         IS_DEFAULTDARK(IN_ROOM(ch)) &&
         (AFF_FLAGGED(ch, AFF_SINGLELIGHT) ||
          AFF_FLAGGED(ch, AFF_HOLYLIGHT)
         )
        )
        SET_BIT(retval, ACHECK_LIGHT);

     for (i = 0; i < NUM_WEARS; i++)
         {if (!GET_EQ(ch,i))
             continue;

          if (IS_SET(what,ACHECK_HUMMING) &&
              OBJ_FLAGGED(GET_EQ(ch,i), ITEM_HUM)
             )
             SET_BIT(retval, ACHECK_HUMMING);

          if (IS_SET(what,ACHECK_GLOWING) &&
              OBJ_FLAGGED(GET_EQ(ch,i), ITEM_GLOW)
             )
             SET_BIT(retval, ACHECK_GLOWING);

          if (IS_SET(what, ACHECK_LIGHT)  &&
              IS_DEFAULTDARK(IN_ROOM(ch)) &&
              GET_OBJ_TYPE(GET_EQ(ch,i)) == ITEM_LIGHT &&
              GET_OBJ_VAL(GET_EQ(ch,i), 0)
             )
             SET_BIT(retval, ACHECK_LIGHT);

          if (GET_OBJ_TYPE(GET_EQ(ch,i))  == ITEM_ARMOR &&
              GET_OBJ_MATER(GET_EQ(ch,i)) < MAT_COLOR
             )
             wgt += GET_OBJ_WEIGHT(GET_EQ(ch,i));
         }

     if (IS_SET(what, ACHECK_WEIGHT) &&
         wgt > GET_REAL_STR(ch) * 2
        )
        SET_BIT(retval, ACHECK_WEIGHT);
    }
 return (retval);
}

int equip_in_metall(struct char_data * ch)
{int i, wgt = 0;

 if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
    return (FALSE);
 if (IS_GOD(ch))
    return (FALSE);

 for (i = 0; i < NUM_WEARS; i++)
     {if (GET_EQ(ch,i) &&
          GET_OBJ_TYPE(GET_EQ(ch,i))  == ITEM_ARMOR &&
          GET_OBJ_MATER(GET_EQ(ch,i)) < MAT_COLOR
         )
         wgt += GET_OBJ_WEIGHT(GET_EQ(ch,i));
     }

 if (wgt > GET_REAL_STR(ch))
    return (TRUE);

 return (FALSE);
}


ACMD(do_afk)
{
  int result;

  if (IS_NPC(ch)) {
    send_to_char("Монстры не могут быть в АФК :)\r\n", ch);
    return;
  }

  skip_spaces(&argument);

  if (PRF_FLAGGED(ch, PRF_AFK)) {
    result = FALSE;
    REMOVE_BIT(PRF_FLAGS(ch), PRF_AFK);
  } else 
  {
    result = TRUE;
    SET_BIT(PRF_FLAGS(ch), PRF_AFK);
  }
  set_afk(ch, argument, result);
}

int awake_hide(struct char_data *ch)
{if (IS_GOD(ch))
    return (FALSE);
 return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING | ACHECK_WEIGHT);
}


int awake_invis(struct char_data *ch)
{if (IS_GOD(ch))
    return (FALSE);
 return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING);
}

int awake_camouflage(struct char_data *ch)
{if (IS_GOD(ch))
    return (FALSE);
 return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING);
}

int awake_sneak(struct char_data *ch)
{if (IS_GOD(ch))
    return (FALSE);
 return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING | ACHECK_GLOWING | ACHECK_WEIGHT);
}

int awaking(struct char_data *ch, int mode)
{if (IS_GOD(ch))
    return (FALSE);
 if (IS_SET(mode,AWAKE_HIDE) && awake_hide(ch))
    return (TRUE);
 if (IS_SET(mode,AWAKE_INVIS) && awake_invis(ch))
    return (TRUE);
 if (IS_SET(mode,AWAKE_CAMOUFLAGE) && awake_camouflage(ch))
    return (TRUE);
 if (IS_SET(mode,AWAKE_SNEAK) && awake_sneak(ch))
    return (TRUE);
 return (FALSE);
}

ACMD(do_quit)
{
  struct descriptor_data *d, *next_d;

  
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char("Чтобы выйти из игры Вам необходимо набрать \'конец\'!\r\n", ch);
  else if (FIGHTING(ch))//(GET_POS(ch) == POS_FIGHTING)
    send_to_char("Не получится, Вы сражаетесь за свою жизнь!\r\n", ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("Вы попали в Рай...\r\n", ch); 
    die(ch, NULL);
  } else 
	  if (AFF_FLAGGED(ch,AFF_SLEEP))
			return;
    else 
  {    if (RENTABLE(ch))
         {send_to_char("Похоже, Вы оставили незаконченные боевые приключения.\r\n",ch);
	      return;
         }

	   if (!GET_INVIS_LEV(ch))
      act("$n покинул$y Мир.", TRUE, ch, 0, 0, TO_ROOM);
    sprintf(buf, "%s выход из игры.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    send_to_char("Успеха и удачи! Пока!\r\n", ch);
	
        if (PLR_FLAGGED(ch, PLR_QUESTOR))
         GET_NEXTQUEST(ch) = number(4, 10);   

    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc)
        continue;
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
        STATE(d) = CON_DISCONNECT;
    }

    if (free_rent || IS_GOD(ch))
          Crash_rentsave(ch, 0, 0);
     
  // Записываем и освобождаем память от персонажа
       extract_char(ch, FALSE);		

  // Сохраняем состояние склада и наличных вещей персонажа при выходе по команде конец.
        Crash_crashsave(ch);      

    /* If someone is quitting in their house, let them load back here
    if (ROOM_FLAGGED(loadroom, ROOM_HOUSE))
      save_char(ch, loadroom);   */
  }
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. This code assumes
     * that guest immortals aren't trustworthy. If you've disabled guest
     * immortal advances from mortality, you may want < instead of <=.
     */
    if (auto_save && GET_LEVEL(ch) <= LVL_IMMORT) {
       send_to_char("Записываю алиасы (синонимы).\r\n", ch);
      write_aliases(ch);
      return;
    }
    sprintf(buf, "Запись алиасов для %s.\r\n", GET_VNAME(ch));
    send_to_char(buf, ch);
  }

  write_aliases(ch);
  save_char(ch, NOWHERE);
  Crash_crashsave(ch);
 
// if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
 //   House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Извините, но Вы не можете сделать это здесь!\r\n", ch); 
}



ACMD(do_sneak)
{
  struct affected_type af;
  ubyte percent, prob;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK)) {
    send_to_char("Вы понятия не имеете как это делать.\r\n", ch);
    return;
  }
 
    if (on_horse(ch))
     {act("Все бы ничего, Вам только бы научить $V этому умению прежде!", FALSE, ch, 0, get_horse(ch), TO_CHAR);
      return;
     }
  
    affect_from_char(ch, SKILL_SNEAK);

  if (affected_by_spell(ch, SPELL_SNEAK))
     {send_to_char("Вы уже и так крадетесь.\r\n", ch);
      return;
     }
	
  send_to_char("Вы попытаетесь передвигаться бесшумно.\r\n", ch);
  REMOVE_BIT(EXTRA_FLAGS(ch), EXTRA_FAILSNEAK);
  percent = number(1,skill_info[SKILL_SNEAK].max_percent);
  prob    = calculate_skill(ch,SKILL_SNEAK,skill_info[SKILL_SNEAK].max_percent,0);

  
  af.type = SPELL_SNEAK;
  af.duration = pc_duration(ch,2,GET_LEVEL(ch),4,0,1);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.battleflag = 0;
  if (percent > prob)
     af.bitvector = 0;
  else
    af.bitvector = AFF_SNEAK;
    affect_to_char(ch, &af);
     
  
}

ACMD(do_camouflage)
{
  struct affected_type af;
  struct timed_type    timed;
  ubyte  prob, percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_CAMOUFLAGE))
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }

  if (on_horse(ch))
     {send_to_char("Ваш скакун мешает Вам замаскироваться!\r\n", ch);
      return;
     }

  if (timed_by_skill(ch, SKILL_CAMOUFLAGE))
     {send_to_char("Вы еще не отдохнули от предыдущей маскировки!\r\n", ch);
      return;
     }

  if (IS_IMMORTAL(ch))
     affect_from_char(ch, SPELL_CAMOUFLAGE);

  if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
     {send_to_char("Вы уже маскируетесь.\r\n", ch);
      return;
     }

  send_to_char("Вы попытались замаскироваться.\r\n", ch);
  REMOVE_BIT(EXTRA_FLAGS(ch), EXTRA_FAILCAMOUFLAGE);
  percent = number(1,skill_info[SKILL_CAMOUFLAGE].max_percent);
  prob    = calculate_skill(ch, SKILL_CAMOUFLAGE, skill_info[SKILL_CAMOUFLAGE].max_percent, 0);

 
// send_to_char("Вам удалось замаскироваться.\r\n", ch);
  af.type       = SPELL_CAMOUFLAGE;
  af.duration   = pc_duration(ch,2,GET_LEVEL(ch),6,0,2);
  af.modifier   = world[IN_ROOM(ch)].zone;
  af.location   = APPLY_NONE;
  af.battleflag = 0;
 if (percent > prob)
     af.bitvector = 0;
  else
     af.bitvector = AFF_CAMOUFLAGE;
  affect_to_char(ch, &af);
  if (!IS_IMMORTAL(ch))
     {timed.skill = SKILL_CAMOUFLAGE;
      timed.time  = 2;
      timed_to_char(ch, &timed);
     }
     
    //  send_to_char("Вам неудалось замаскироваться.\r\n", ch);
}

ACMD(do_hide)
{
  struct affected_type af;
  ubyte  prob, percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE))
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }
   

 // if (AFF_FLAGGED(ch, AFF_HIDE))
  //    REMOVE_BIT(ch->char_specials.saved.affected_by.flags[0], AFF_HIDE);
 
	 if (on_horse(ch))
     {act("А $V Вы будете прятать в карман?", FALSE, ch, 0, get_horse(ch), TO_CHAR);
      return;
     }
  affect_from_char(ch, SPELL_HIDE);

    if (affected_by_spell(ch, SPELL_HIDE))
     {send_to_char("Вы уже пытаетесь спрятаться.\r\n", ch);
      return;
     }
  send_to_char("Вы попытались спрятаться.\r\n", ch);
  REMOVE_BIT(EXTRA_FLAGS(ch), EXTRA_FAILHIDE);
  percent = number(1,skill_info[SKILL_HIDE].max_percent);
  prob    = calculate_skill(ch, SKILL_HIDE, skill_info[SKILL_HIDE].max_percent, 0);

  
  af.type       = SPELL_HIDE;
  af.duration   = pc_duration(ch,2,GET_LEVEL(ch),4,0,1);
  af.modifier   = 0;
  af.location   = APPLY_NONE;
  af.battleflag = 0;
  if (percent > prob)
     af.bitvector = 0;
  else
     af.bitvector = AFF_HIDE;
     affect_to_char(ch, &af);

WAIT_STATE(ch, PULSE_VIOLENCE);
}

void go_steal(struct char_data *ch, struct char_data *vict, char *obj_name)
{ int    percent, gold, eq_pos, ohoh = 0, success=0, prob;
  struct obj_data *obj;

  if (!vict)
     return;

  if (!WAITLESS(ch) && FIGHTING(vict))
     {act("$N с Вами дерется. Вы не можете красть в бою!",FALSE,ch,0,vict,TO_CHAR);
      return;
     }

  if (!WAITLESS(ch) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA))
     { send_to_char("Вы не можете украсть на арене.\r\n",ch);
       return;
     }

   /* 101% is a complete failure */
  percent = number(1,skill_info[SKILL_SNEAK].max_percent);

  if (WAITLESS(ch) ||
      (GET_POS(vict) <= POS_SLEEPING && !AFF_FLAGGED(vict,AFF_SLEEP)))
     success  = 1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
     percent = MAX(percent - 50, 0);

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (IS_IMMORTAL(vict)                ||
      GET_GOD_FLAG(vict, GF_GODSLIKE)  ||
      GET_MOB_SPEC(vict) == shop_keeper)
     success = 0;		/* Failure */

  if (str_cmp(obj_name, "монеты") &&
      str_cmp(obj_name, "coins")  &&
      str_cmp(obj_name, "gold")
	  )
     {if (!(obj = get_obj_in_list_vis(ch, obj_name, vict->carrying)))
         {for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
              if (GET_EQ(vict, eq_pos) &&
                  (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
                  CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos)))
                 {obj = GET_EQ(vict, eq_pos);
                  break;
                 }
          if (!obj)
             {act("У н$S этого нет!", FALSE, ch, 0, vict, TO_CHAR);
              return;
             }
          else
             {/* It is equipment */
              if (!success)
                 {send_to_char("Вы не можете сейчас этого сделать!\r\n", ch);
                  return;
                 }
              else
              if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
                 {send_to_char("Вы не сможете унести столько вещей!\r\n", ch);
                  return;
                 }
              else
              if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
                 {send_to_char("Вы не сможете унести такой вес.\r\n", ch);
                  return;
                 }
              else
                 {act("Вы сняли с $R $3 и забрали себе!", FALSE, ch, obj, vict, TO_CHAR);
                  act("$n украл$y $3 у $R.", FALSE, ch, obj, vict, TO_NOTVICT);
                  obj_to_char(unequip_char(vict, eq_pos), ch);
                 }
             }
         }
      else {/* obj found in inventory */
            percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */
	    prob     = calculate_skill(ch, SKILL_STEAL, percent, vict);
	    if (CAN_SEE(vict,ch))
	       improove_skill(ch,SKILL_STEAL,0,vict);
	    if (!WAITLESS(ch) && AFF_FLAGGED(vict,AFF_SLEEP))
	       prob = 0;
            if (percent > prob && !success)
               {ohoh = TRUE;
                send_to_char("УПС...Вы кажется попали!!!\r\n", ch);
                act("Вы обнаружили, что $n засунул$y руку в Ваш карман!", FALSE, ch, 0, vict, TO_VICT);
                act("$n попытал$u что-то украсть у $R.", TRUE, ch, 0, vict, TO_NOTVICT);
               }
            else
               {/* Steal the item */
                if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))
                    {if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch))
                        {obj_from_char(obj);
                         obj_to_char(obj, ch);
                         act("Вы украли $3 у $R!", FALSE, ch, obj, vict, TO_CHAR);
                        }
                    }
                 else
                    {send_to_char("Вы не можете столько нести.\r\n", ch);
		     return;
		    }
               }
           }
     }
  else
     {/* Steal some coins */
      prob     = calculate_skill(ch, SKILL_STEAL, percent, vict);
      if (CAN_SEE(vict,ch))
         improove_skill(ch,SKILL_STEAL,0,vict);
      if (!WAITLESS(ch) && AFF_FLAGGED(vict,AFF_SLEEP))
         prob = 0;
      if (percent > prob && !success)
         {ohoh = TRUE;
          send_to_char("УПС...Вы кажется попали!!!\r\n", ch);
          act("Вы обнаружили руку $r в своем кармане.", FALSE, ch, 0, vict, TO_VICT);
          act("$n пытал$u украсть деньги из кармана $R.", TRUE, ch, 0, vict, TO_NOTVICT);
         }
      else
         {/* Steal some gold coins */
	  if (!GET_GOLD(vict))
             {act("У н$S денег отродясь наверное не было!",FALSE,ch,0,vict,TO_CHAR);
	      return;
	     }
	  else
	     {gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
              gold = MIN(GET_LEVEL(ch) * 100, gold);
              if (gold > 0)
                 {GET_GOLD(ch)   += gold;
                  GET_GOLD(vict) -= gold;
                  if (gold > 1)
                     {sprintf(buf, "Вам повезло! Вы украли %d %s!\r\n", gold, desc_count(gold, WHAT_MONEYu));
                      send_to_char(buf, ch);
                     }
                  else
                     send_to_char("Вы смогли украсть всего лишь одну монету.\r\n", ch);
                 }
              else
                 send_to_char("Вы ничего не смогли украсть.\r\n", ch);
             }
         }
     }
  if (!WAITLESS(ch) && ohoh)
   {  WAIT_STATE(ch, 5 * PULSE_VIOLENCE);
      pk_thiefs_action(ch,vict);
   }  

    if (ohoh      &&
     IS_NPC(vict) &&
      AWAKE(vict) &&
      CAN_SEE(vict, ch) &&
      MAY_ATTACK(vict)
     )
     hit(vict, ch, TYPE_UNDEFINED, 1);
}


ACMD(do_steal)
{
  struct char_data *vict;
  char   vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL))
     {send_to_char("Но Вы не знаете как воровать!\r\n", ch);
      return;
     }
  if (PLR_FLAGGED(ch, PLR_IMMKILL))
    { send_to_char("У Вас отсохли руки, очень жаль -(\r\n", ch);
      return;
    }


  if (!WAITLESS(ch) && on_horse(ch))
     {send_to_char("Вы не можете этого сделать верхом на скакуне!\r\n", ch);
      return;
     }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, FIND_CHAR_ROOM)))
     {send_to_char("У кого Вы хотите своровать?\r\n", ch);
      return;
     }
  else
  if (vict == ch)
     {send_to_char("Воровать у себя?\r\n", ch);
      return;
     }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) &&
      !(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
     {send_to_char("Здесь тихо и мирно. Попробуйте свое деяние сделать в другом месте!\r\n", ch);
      return;
     }

  go_steal(ch, vict, obj_name);
}





ACMD(do_tren_skill)
{ 
   if (IS_NPC(ch))
    return;
one_argument(argument, arg);

  if (*arg)
    send_to_char("Вы можете тренировать характеристики только в своей гильдии.\r\n", ch); /*You can only practice skills in your guild*/
  else
    send_to_char("Что Вы хотите тренировать?\r\n", ch);

}



ACMD(do_practice)
{ 
  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (*arg)
    send_to_char("Вы можете учиться только в своей гильдии.\r\n", ch);
  else 
    send_to_char("Что Вы хотите учить?\r\n", ch);
}



ACMD(do_visible)
{ 	bool appear_msg = AFF_FLAGGED(ch, AFF_INVISIBLE)  ||
					  AFF_FLAGGED(ch, AFF_CAMOUFLAGE) ||
					  AFF_FLAGGED(ch, AFF_MENTALLS)   ||
					  AFF_FLAGGED(ch, AFF_HIDE);
   
  if (appear_msg)
  {	if (affected_by_spell(ch, SPELL_INVISIBLE))
		affect_from_char(ch, SPELL_INVISIBLE);
	if (affected_by_spell(ch, SPELL_HIDE))
		affect_from_char(ch, SPELL_HIDE);
	if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
		affect_from_char(ch, SPELL_CAMOUFLAGE);
	if (affected_by_spell(ch, SPELL_MENTALLS))
		affect_from_char(ch, SPELL_MENTALLS);

	REMOVE_BIT(AFF_FLAGS(ch, AFF_INVISIBLE), AFF_INVISIBLE);
	REMOVE_BIT(AFF_FLAGS(ch, AFF_HIDE), AFF_HIDE);
	REMOVE_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);
	REMOVE_BIT(AFF_FLAGS(ch, AFF_MENTALLS), AFF_MENTALLS);
	
		if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)
			  act("$n медленно появил$u из пустоты.", FALSE, ch, 0, 0, TO_ROOM);
		else
			{ act("$n внезапно появил$u из пустоты.", FALSE, ch, 0, 0, TO_ROOM);
			  GET_INVIS_LEV(ch) = 0;
			}
		send_to_char("Вы теперь полностью видимы.\r\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch))
	{ if (GET_INVIS_LEV(ch) == 0)
		{ send_to_char("Вы и так были видимы для всех!\r\n", ch);
		  return;
		}
	  GET_INVIS_LEV(ch) = 0;
	  act("$n внезапно появил$u из пустоты.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("Вы теперь полностью видимы.\r\n", ch);
	  return;
	}
    send_to_char("Вы и так видимы.\r\n", ch);
}



int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) ||  AFF_FLAGGED(vict, AFF_CHARM) || IS_HORSE(vict))
    return (0);

  SET_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);

  if (ch != vict) {
    act("$N теперь является членом Вашей группы.", FALSE, ch, 0, vict, TO_CHAR);
  act("Вы теперь являетесь членом группы $r.", FALSE, ch, 0, vict, TO_VICT);
  act("$N теперь является членом группы $r.", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  return (1);
}

int low_charm(struct char_data *ch)
{struct affected_type *aff;
 for (aff = ch->affected; aff; aff = aff->next)
     if (aff->type == SPELL_CHARM && aff->duration <= 1)
        return (TRUE);
 return (FALSE);
}

void print_one_line(struct char_data *ch, struct char_data *k, int leader, int header)
{ int  ok, ok2, div;
  const char *WORD_STATE[] =
       {"Умирает     ",
        "Ужасное     ",
        "О.Плохое    ",
        "Плохое      ",
        "Плохое      ",
        "Среднее     ",
        "Среднее     ",
        "Хорошее     ",
        "Хорошее     ",
        "Хорошее     ",
        "О.Хорошее   ",
        "Превосходное"};
  const char *MOVE_STATE[] =
       {"Истощен",
        "Истощен",
        "О.устал",
        " Устал ",
        " Устал ",
        "Л.устал",
        "Л.устал",
        "Хорошо ",
        "Хорошо ",
        "Хорошо ",
        "Отдохн.",
        "Превос."};
  const char *POS_STATE[] =
        {"Умер",
         "Истекает кровью",
         "При смерти",
         "Без сознания",
         "Спит",
         "Отдыхает",
         "Сидит",
         "Сражается",
         "Стоит"};
         
  if (IS_NPC(k))
     {if (!header)
		{ send_to_char("|Имя          |   Жизни    |В комнате| Доп |Положение|Таймер|Владелец     \r\n", ch);
          send_to_char("---------------------------------------------------------------------------\r\n", ch);
		}
		 sprintf(buf, "%s%-14s%s|",CCWHT(ch, C_NRM), CAP(GET_NAME(k)), CCNRM(ch, C_NRM));
         sprintf(buf+strlen(buf), "%s%s%s|",
              color_value(ch,GET_HIT(k),GET_REAL_MAX_HIT(k)),
              WORD_STATE[posi_value(GET_HIT(k),GET_REAL_MAX_HIT(k))+1],
              CCNRM(ch,C_NRM));      
              
      ok = IN_ROOM(ch) == IN_ROOM(k);
      sprintf(buf+strlen(buf), "%s%s%s|",
              ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM),
              ok ? "   Да    " : "   Нет   ",
              CCNRM(ch, C_NRM));

      sprintf(buf+strlen(buf)," %s%s%s%s%s%s%s |",
              CCYEL(ch, C_NRM),
              (AFF_FLAGGED(k,AFF_SINGLELIGHT) ||
               AFF_FLAGGED(k,AFF_HOLYLIGHT)   ||
               (GET_EQ(k,WEAR_LIGHT) && GET_OBJ_VAL(GET_EQ(k,WEAR_LIGHT),0))) ? "С" : " ", //заменил 2 на ноль
              CCBLU(ch, C_NRM),
              AFF_FLAGGED(k,AFF_FLY) ? "П" : " ",
              CCYEL(ch, C_NRM),
              low_charm(k) ? "Т" : " ",
              CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf),"%-9s|",POS_STATE[(int) GET_POS(k)]);

      if (AFF_FLAGGED(k, AFF_CHARM))
      {
          int timer = 0;
          affected_type *at;
          for (at = k->affected; at; at = at->next)
          {
              if (at->type == SPELL_CHARM)
                { timer = at->duration; break; }
          }
          if (timer > 0)
            sprintf(buf+strlen(buf), "  %-3d |%s", timer, GET_NAME(k->master));
          else if (timer == 0)
            sprintf(buf+strlen(buf), "%s  ~0%s  |%s", CCRED(ch, C_NRM), CCNRM(ch,C_NRM), GET_NAME(k->master));
      }
      act(buf, FALSE, ch, 0, k, TO_CHAR);
     }
  else
     {if (!header)
		{ send_to_char("|Имя         |   Жизни    |Энергия|В комнате|Запом|Сведения|Лидер| Позиция   |\r\n", ch);
         send_to_char("------------------------------------------------------------------------------\r\n", ch);
		}      
		 sprintf(buf, "%s%-13s%s|",CCWHT(ch, C_NRM), CAP(GET_NAME(k)), CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf), "%s%s%s|",
              color_value(ch,GET_HIT(k),GET_REAL_MAX_HIT(k)),
              WORD_STATE[posi_value(GET_HIT(k),GET_REAL_MAX_HIT(k))+1],
              CCNRM(ch,C_NRM));

      sprintf(buf+strlen(buf), "%s%s%s|",
              color_value(ch,GET_MOVE(k),GET_REAL_MAX_MOVE(k)),
              MOVE_STATE[posi_value(GET_MOVE(k),GET_REAL_MAX_MOVE(k))+1],
              CCNRM(ch,C_NRM));

      ok = IN_ROOM(ch) == IN_ROOM(k);
      sprintf(buf+strlen(buf), "%s%s%s|",
              ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM),
              ok ? "   Да    " : "   Нет   ",
              CCNRM(ch, C_NRM));

    
 if ((ok = GET_MANA_NEED(k)))
	  { div = mana_gain(k);
          if (div > 0)
		  { ok = MAX(0, 1+ok - GET_MANA_STORED(k));
		      ok2 = ok * 60;
			  ok = ok / div; 
			  ok2 = MAX(0, ok2/div - ok * 60); 
			  if (ok > 99)
			   sprintf(buf+strlen(buf), "&g%5d&n|",ok);
			  else
				{ sprintf(buf+strlen(buf), "&g%2d:%s%d&n|",ok,
		          ok2 > 9 ? "" : "0", ok2);
				} 
              
          }
          else
       sprintf(buf+strlen(buf), "&r    -&n|");
     } 
	  else
      sprintf(buf+strlen(buf), "    -|"); 

      sprintf(buf+strlen(buf)," %s%s%s%s%s%s%s%s%s   |",
              CCYEL(ch, C_NRM),
              (AFF_FLAGGED(k,AFF_SINGLELIGHT) ||
               AFF_FLAGGED(k,AFF_HOLYLIGHT)   ||
              (GET_EQ(k,WEAR_LIGHT) && GET_OBJ_VAL(GET_EQ(k,WEAR_LIGHT),0))) ? "С" : " ", // заменил 2 на ноль
               CCBLU(ch, C_NRM),
               AFF_FLAGGED(k,AFF_FLY) ? "П" : " ",
			   CCGRN(ch, C_NRM),
               AFF_FLAGGED(k,AFF_INVISIBLE) ? "Н" : " ",
               CCYEL(ch, C_NRM),
               on_horse(k) ? "В" : " ",
               CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf),"%5s|",
              leader ? "ДА" : "НЕТ"
             );
      sprintf(buf+strlen(buf)," %s",POS_STATE[(int) GET_POS(k)]);
      act(buf, FALSE, ch, 0, k, TO_CHAR);
     }
}

void print_group(struct char_data *ch)
{ int    gfound = 0, cfound = 0;
  struct char_data *k;
  struct follow_type *f;
  if (AFF_FLAGGED(ch, AFF_GROUP))
  {   send_to_char("Состав и состояние Вашей группы:\r\n", ch);
      k = (ch->master ? ch->master : ch);
      if (AFF_FLAGGED(k, AFF_GROUP))
         print_one_line(ch,k,TRUE,gfound++);     
      
      std::vector<char_data*> charms;
      for (f = k->followers; f; f = f->next)
      {
          if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower,AFF_CHARM))
            { charms.push_back(f->follower); continue; }
          if (!AFF_FLAGGED(f->follower, AFF_GROUP))
              continue;
           print_one_line(ch,f->follower,FALSE,gfound++);
           for (follow_type *gc = f->follower->followers; gc; gc = gc->next)
           {
               if (AFF_FLAGGED(gc->follower,AFF_CHARM))                   
                   charms.push_back(gc->follower);
           }
      }
      if (!charms.empty())
          send_to_char("За Вами следуют:\r\n",ch);
      for (int i=0,e=charms.size(); i<e; ++i)
      {
         print_one_line(ch,charms[i],FALSE,i);
      }
      return;
   }
   for (f = ch->followers; f; f = f->next)
   {
       if (!AFF_FLAGGED(f->follower,AFF_CHARM))
           continue;
       if (!cfound)           
           send_to_char("За Вами следуют:\r\n",ch);
       print_one_line(ch,f->follower,FALSE,cfound++);
   }
   if (!gfound && !cfound)
      send_to_char("Вы не член группы!\r\n", ch);
}

void new_leader(struct char_data *ch, const char* leader_name)
{ 
    if (!IS_SET(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP))
    {
        send_to_char("Вы не член группы!\r\n", ch);
        return;
    }

    if (ch->master)
    {
        send_to_char("Вы должны быть лидером группы!\r\n", ch);
        return; 
    }

    int len = strlen(leader_name);
    if (len == 0)
    {
        send_to_char("Кто должен быть лидером?\r\n", ch);
        return;
    }

    char *leader = new char[len+1];
    strcpy(leader, leader_name);    

    char_data* vict = get_char_vis(ch, leader, FIND_CHAR_ROOM);
    if (!vict)    
        vict = get_char_vis(ch, leader, FIND_CHAR_WORLD);
    if (ch == vict)
    {
        delete []leader;
        send_to_char("Вы и так лидер группы.\r\n", ch);
        return;
    }
    delete []leader;

    if (vict && IS_NPC(vict))
        vict = NULL;

    if (vict)
    {
         bool exist = false;
         struct follow_type *j, *k;
         for (k = ch->followers; k; k = j) 
         {
           j = k->next;
           if (k->follower == vict) { exist = true; break; }        
         }
         if (!exist) 
             vict = NULL;
    }
        
    if (!vict)
    {
        send_to_char("Нет такого члена группы.\r\n", ch);
        return;
    }

    // vict - new leader
    // ch - old leader
    std::vector<char_data*> group;
    
    // clear followers list of leader and collect followers
    group.push_back(ch);
    
    follow_type * npcs = NULL;   
    struct follow_type *j, *k;
    for (k = ch->followers; k; k = j) 
    {
       j = k->next;
       if (IS_NPC(k->follower))   // collect npc, which following, but will still follow owner (not new leader)
       {
           k->next = NULL;
           if (!npcs)
               npcs = k;
           else
           {
               follow_type *p = npcs; 
               while (p->next) p=p->next;
               p->next = k;
           }
       }
       else
       {
           if (k->follower != vict)
                group.push_back(k->follower);
           free(k);
       }
    }

    ch->followers = npcs;    
    vict->master = NULL;

    sprintf(buf,"&GВы теперь новый лидер группы!&n\r\n");
    send_to_char(buf, vict);
    sprintf(buf,"&G%s теперь является лидером группы!&n\r\n", GET_NAME(vict));
    
    int count = group.size();
    for (int i=(count-1); i>=0; --i)
    {
      group[i]->master = vict;
      CREATE(k, struct follow_type, 1);
      k->follower = group[i];
      k->next = vict->followers;
      vict->followers = k;      
      send_to_char(buf, group[i]);
    }
}

ACMD(do_group)
{
  CHAR_DATA *vict;
  struct follow_type *f;
  int found;

  char* next_arg = one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }
  
  if (!str_cmp(buf, "лидер"))
  {
     one_argument(next_arg, buf);
     new_leader(ch, buf);
     return;
  }

  if (ch->master) 
  {
     if (!strn_cmp(buf, "лидер", 2))
     {
         if (!IS_SET(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP))
         {
            send_to_char("Вы не член группы!\r\n", ch);
            return;
         }
     }
     act("Принимать и исключать в группе может только ее лидер.", FALSE, ch, 0, 0, TO_CHAR);
     return;
  }
  
  if (!ch->followers)
  {
      send_to_char("За Вами никто не следует.\r\n",ch);
      return;
  }
  
  if (!str_cmp(buf, "all") || !str_cmp(buf, "все"))
  {
     perform_group(ch, ch);
     for (found = 0, f = ch->followers; f; f = f->next)
       found += perform_group(ch, f->follower);
     if (!found)
       send_to_char("Здесь нет никого подходящего для принятия в группу.\r\n", ch);
     return;
  }
  
  if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
  {
      if (!strn_cmp(buf, "лидер", 2))
      {
          one_argument(next_arg, buf);
          if (strlen(buf) != 0)
          {
              new_leader(ch, buf);
              return;
          }
      }
      send_to_char(NOPERSON, ch);
  }
  else if ((vict->master != ch) && (vict != ch))
    act("$N должен следовать за Вами, что бы стать членом группы.", FALSE, ch, 0, vict, TO_CHAR);
  else {
	  if (!AFF_FLAGGED(vict, AFF_GROUP)) 
      {
		  if (AFF_FLAGGED(vict, AFF_CHARM) || IS_HORSE(vict))
		  {send_to_char("Не равноценная группа.\r\n",ch);
	       send_to_char("Не равноценная группа.\r\n",vict);
		  }
		  perform_group(ch, ch);
		  perform_group(ch, vict);
	  }
    else 
	{ if (ch != vict)
	   { act("$N больше не член Вашей группы.", FALSE, ch, 0, vict, TO_CHAR);
	     act("$n исключил$y Вас только что из группы!", FALSE, ch, 0, vict, TO_VICT);
	     act("$n исключил$y только что из группы $V!", FALSE, ch, 0, vict, TO_NOTVICT);
             REMOVE_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);
	   }	
       else
 	   { act("Вы больше не член группы!", FALSE, ch, 0, vict, TO_VICT);
         REMOVE_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);
	   }
	}
  }
}

ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  CHAR_DATA *tch;

  one_argument(argument, buf);

 
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP)))
		{ send_to_char("Но у Вас нет группы!\r\n", ch);
			 return;
		}

	 if (!*buf)
	 { sprintf(buf2, "Вы были исключены из группы %s.\r\n", GET_RNAME(ch));
      for (f = ch->followers; f; f = next_fol)
          {next_fol = f->next;
           if (AFF_FLAGGED(f->follower, AFF_GROUP))
              {REMOVE_BIT(AFF_FLAGS(f->follower, AFF_GROUP), AFF_GROUP);
	           send_to_char(buf2, f->follower);
               if (!AFF_FLAGGED(f->follower, AFF_CHARM) &&
                   !(IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_HORSE)))
	              stop_follower(f->follower, SF_EMPTY);
              }
          }
      REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);
      send_to_char("Вы распустили группу.\r\n", ch);
      return;
     }


 	for (f = ch->followers; f; f = next_fol)
	{ next_fol = f->next; 
	  tch = f->follower;
	 if (isname(buf, tch->player.name) && !AFF_FLAGGED(tch, AFF_CHARM) && !IS_HORSE(tch))
		{ REMOVE_BIT(AFF_FLAGS(tch, AFF_GROUP), AFF_GROUP);
			act("$N больше не член Вашей группы.", FALSE, ch, 0, tch, TO_CHAR);
			act("Вы только что были исключены из группы $r!", FALSE, ch, 0, tch, TO_VICT);
			act("$N только что был$Y отпущен$Y из группы $r!", FALSE, ch, 0, tch, TO_NOTVICT);
            stop_follower(tch, SF_EMPTY);
           return;
		}
	}
	send_to_char("Такого игрока нет в составе Вашей группы.\r\n", ch);
return;
}



ACMD(do_report)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("Вы не член группы!\r\n", ch);
    return;
  }
  sprintf(buf, "%s сообщил$y свое состояние: %d/%dH, %d/%dV",
	  GET_NAME(ch), GET_HIT(ch), GET_REAL_MAX_HIT(ch),
	  GET_MOVE(ch), GET_REAL_MAX_MOVE(ch));

  CAP(buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      act(buf, FALSE, ch, 0, f->follower, TO_VICT);
	  if (k != ch)
act(buf, FALSE, ch, 0, k, TO_VICT);
    send_to_char("Вы сообщили свое состояние группе.\r\n", ch);
}



ACMD(do_split)
{
  int amount, num, share, rest;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  
 if (AFF_FLAGGED(ch, AFF_BLIND))
     {send_to_char("Вы слепы!\r\n", ch);
      return;
     }

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char("Вы не можете этого сделать.\r\n", ch);
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char("У Вас нет столько монет.\r\n", ch);
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char("С кем Вы хотите разделить Ваши монеты?\r\n", ch);
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);
    sprintf(buf, "%s разделил%s %d %s, Вам досталось %d.\r\n",
              GET_NAME(ch), GET_CH_SUF_1(ch),
              amount, desc_count(amount,WHAT_MONEYu), share);
	
	if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
      GET_GOLD(k) += share;
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room) &&
	  f->follower != ch) {
	GET_GOLD(f->follower) += share;
	send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "Вы разделили %d %s среди %d членов - по %d %s каждому.\r\n",
	    amount,desc_count(amount,WHAT_MONEYu), num, share, desc_count(share,WHAT_MONEYu));
    if (rest) {
      sprintf(buf + strlen(buf), "Неделимый остаток %d %s, Вы оставили у себя.\r\n",
		  rest, desc_count(rest,WHAT_MONEYu));
    //  GET_GOLD(ch) += rest;
    }
    send_to_char(buf, ch);
  } else {
    send_to_char("Сколько монет Вы хотите разделить с группой?\r\n", ch);
    return;
  }
}



ACMD(do_use)
{
  struct obj_data *mag_item;
  int    do_hold = 0;

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "Что Вы хотите %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		send_to_char("У Вас этого нет!\r\n", ch);
	return;
      }
      break;
    case SCMD_USE:
      sprintf(buf2, "Вы кажется, не держите %s.\r\n", arg);
      send_to_char(buf2, ch);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char("Это можно только осушить.\r\n", ch);
      return;
    }
    do_hold = 1;
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char("Вы можете зачитывать только свитки.\r\n", ch);
      return;       	
    }
    if (affected_by_spell(ch, SPELL_COURAGE))
	{ send_to_char("Вам не до этих мелочей!\r\n", ch);
	 return;
	}
    do_hold = 1;
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char("Использовать можно только магические предметы!\r\n", ch);
      return;
    }
    break;
  }
if (do_hold && GET_EQ(ch,WEAR_HOLD) != mag_item)
     {do_hold = GET_EQ(ch, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_HOLD;
      if (GET_EQ(ch,do_hold))
         {act("Вы прекратили использовать $3.",FALSE,ch,GET_EQ(ch,do_hold),0,TO_CHAR);
          act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,do_hold),0,TO_ROOM);
          obj_to_char(unequip_char(ch,do_hold), ch);
         }
      if (GET_EQ(ch,WEAR_HOLD))
         obj_to_char(unequip_char(ch,WEAR_HOLD), ch);
      act("Вы взяли $3 в левую руку.", FALSE, ch, mag_item, 0, TO_CHAR);
      act("$n взял$y $3 в левую руку.", FALSE, ch, mag_item, 0, TO_ROOM);
      obj_from_char(mag_item);
      equip_char(ch,mag_item,WEAR_HOLD);
     }

  mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_courage)
{ OBJ_DATA      *obj;
  AFFECT_DATA  af[2];
  struct timed_type    timed;

  if (IS_NPC(ch))
     return;

  
  if (!GET_SKILL(ch, SKILL_COURAGE))
     {send_to_char("Вам это не по силам.\r\n", ch);
      return;
     }

  if (timed_by_skill(ch, SKILL_COURAGE))
     {send_to_char("У Вас нет сил так часто впадать в неистовство.\r\n", ch);
      return;
     }
   if (!FIGHTING(ch))
	{ send_to_char("Только в бою вы можете впадать в неистовство!\r\n", ch);
		return;
	}

  timed.skill = SKILL_COURAGE;
  timed.time  = 5;
  timed_to_char(ch, &timed);
  
  int prob = calculate_skill(ch, SKILL_COURAGE, skill_info[SKILL_COURAGE].max_percent, 0) / 6;

	af[0].type      = SPELL_COURAGE;
	af[0].duration  = pc_duration(ch,3,0,0,0,0);
	af[0].modifier  = 20;
	af[0].location  = APPLY_AC;
	af[0].bitvector = AFF_NOFLEE;
	af[0].battleflag= 0;
	af[1].type		= SPELL_COURAGE;
	af[1].duration	= pc_duration(ch, 3, 0, 0, 0, 0);
	af[1].modifier	= MAX(1, prob * 3);
	af[1].location	= APPLY_ABSORBE;
	af[1].bitvector = AFF_NOFLEE;
	af[1].battleflag= 0;

	for (prob = 0; prob < 2; prob++)
       affect_join(ch,&af[prob],TRUE,FALSE,TRUE,FALSE);
   

   send_to_char("Красный туман застелил глаза, ваш разум помутнел, теперь никто не в силах остановить вас!\r\n",ch);
   if ((obj = GET_EQ(ch,WEAR_WIELD)) || (obj = GET_EQ(ch,WEAR_BOTHS)))
      strcpy(buf, "Глаза $r налились кровью и $e бешенно сжал$y в руках $3.");
   else
      strcpy(buf, "Глаза $r налились кровью.");
   act(buf, FALSE, ch, obj, 0, TO_ROOM);
}



ACMD(do_wimpy)
{
  int wimp_lev;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {
      sprintf(buf, "Хорошо, Вы попытаетесь убежать, когда у Вас останется %d единиц жизни.\r\n",
	      GET_WIMP_LEV(ch));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("Вы будете сражаться на смерть.\r\n", ch);
      return;
    }
  }
  if (isdigit((unsigned char)*arg)) {
    if ((wimp_lev = atoi(arg)) != 0) {
      if (wimp_lev < 0)
	send_to_char("В таком состоянии останется лишь молиться!\r\n", ch);
      else if (wimp_lev > GET_REAL_MAX_HIT(ch))
	send_to_char("У вас нет столько жизней!\r\n", ch);
      else if (wimp_lev > (GET_REAL_MAX_HIT(ch) / 2))
	  { send_to_char("Вы убежите из боя, если у Вас останется 50% жизни.\r\n", ch);
      GET_WIMP_LEV(ch) = GET_REAL_MAX_HIT(ch) / 2;
	  }else {
	sprintf(buf, "Хорошо, Вы попытаетесь убежать из боя, когда останется %d хитов.\r\n",
		wimp_lev);
	send_to_char(buf, ch);
	GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char("Хорошо, Вы будете сражаться до последней капли крови.\r\n", ch);
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char("Режим автоматического убегания из боя.(0 - режим выключен)\r\n", ch);
}

ACMD(do_prompt)
{
	const char *prompt;
    char DEFAULT_PROMPT[] = "%hж/%HЖ %vэ/%VЭ %Oо %gм %wm %f %#C%e%%>%#n ";
    char DEFAULT_PROMPT_FIGHTING[] = "%f %hж %vэ %wm %#C%e%%>%#n ";

	skip_spaces(&argument);

	if (argument[0] == '\0'
	||  !str_cmp(argument, "показать")) {
		sprintf(buf, "Текущая строка дисплея: \"%s\".\r\n", ch->prompt);
		send_to_char(buf, ch);
		sprintf(buf, "Текущая боевая строка дисплея: \"%s\".\r\n", ch->fprompt);
		send_to_char(buf, ch);
		return;
	}

	if (!subcmd) 
	{if (!str_cmp(argument, "все") || !str_cmp(argument, "default"))
		prompt = str_dup(DEFAULT_PROMPT);
	else 
		{ sprintf(buf,"%s ", argument);
		  prompt = str_dup(buf);
		}
	ch->prompt = str_dup(prompt);
	sprintf(buf, "Строка установлена в \"%s\"\r\n", ch->prompt);
	send_to_char(buf, ch);
	}
	else
	{ if (!str_cmp(argument, "все") || !str_cmp(argument, "default"))
		prompt = str_dup(DEFAULT_PROMPT_FIGHTING);
	else 
		{ sprintf(buf,"%s ", argument);
		  prompt = str_dup(buf);
		}
	ch->fprompt = str_dup(prompt);
	sprintf(buf, "Строка установлена в \"%s\"\r\n", ch->fprompt);
	send_to_char(buf, ch);
	}
} 


ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp, buf[MAX_STRING_LENGTH];
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char("У монстра не может быть идей.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("Это должно быть ошибка...\r\n", ch);
    return;
  }
  sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  mudlog(buf, CMP, LVL_IMMORT, FALSE);

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Извините, в данный момент файл переполнен, попытайтесь позже.\r\n", ch);
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: do_gen_write");
    send_to_char("Не могу открыть файл. Извините.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  GET_ROOM_VNUM(IN_ROOM(ch)), argument);
  fclose(fl);
  send_to_char("Хорошо... Спасибо!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1


//#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"Вы теперь не можете быть призваны другими игроками.\r\n",
    "Вы теперь снова можете быть призваны другими игроками.\r\n"},
    {"Мирный режим отключен.\r\n",
    "Мирный режим включен.\r\n"},
    {"Краткий режим выключен.\r\n",
    "Краткий режим включен.\r\n"},
    {"Компактный режим выключен.\r\n",
    "Компактный режим включен.\r\n"},
    {"Вы теперь можете слышать сообщения.\r\n",
    "Вы теперь глухи к сообщениям.\r\n"},
    {"Вы теперь будете видеть сообщения с аукциона.\r\n",
    "Вы теперь не будете видеть сообщения с аукциона.\r\n"},
    {"Вы теперь снова можете слышать крики.\r\n",
    "Вы теперь не можете слышать крики.\r\n"},
    {"Вы теперь снова можете слышать болтовню.\r\n",
    "Вы теперь не можете слышать болтовню.\r\n"},
    {"Вы включились на канал оффтопика.\r\n",
    "Вы отключились от канала оффтопика\r\n"},
    {"Вы теперь снова можете слышать Wiz-канал.\r\n",
    "Вы теперь не можете слышать Wiz-канал.\r\n"},
    {"Вы больше не участвуете в квестах.\r\n",
    "Хорошо, Вы снова будете участвовать в квестах!\r\n"},
    {"Вы теперь не будете видеть флаги комнаты и номера монстров.\r\n",
    "Вы теперь будете видеть флаги комнаты и номера монстров.\r\n"}, /*You will now see the room flags*/
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"Всевидение выключено.\r\n",
    "Всевидение включено.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Авто выход выключен.\r\n",
    "Авто выход включен.\r\n"},
    {"Вы теперь не сможете выслеживать сквозь двери.\r\n",
    "Вы теперь сможете выслеживать сквозь двери.\r\n"},
	{"\r\n",
     "\r\n"},
    {"Режим показа дополнительной информации выключен.\r\n",
     "Режим показа дополнительной информации включен.\r\n"},
    {"Автозаучивание заклинаний выключено.\r\n",
     "Автозаучивание заклинаний включено.\r\n"},
    {"Сжатие запрещено.\r\n",
     "Сжатие разрешено.\r\n"},
    {"Автосжатие выключено\r\n",
     "Автосжатие включено\r\n"},
    {"Вы слышите все крики.\r\n",
     "Вы глухи к крикам.\r\n"},
    {"Режим автозавершения (IAC GA) выключен.\r\n",
     "Режим автозавершения (IAC GA) включен.\r\n"},
    {"Режим боевой строки выключен.\r\n",
     "Режим боевой строки включен.\r\n"},
    {"Режим агродействий выключен.\r\n",
     "&RВНИМАНИЕ! РЕЖИМ АГРО ВКЛЮЧЕН!&n\r\n"},
    {"Режим направление выключен.\r\n",
     "Режим направление включен.\r\n"},
    {"За Вами теперь можно следовать.\r\n",
     "За Вами теперь нельзя следовать.\r\n"},
	{"Вы снова &Gполучаете&n опыт.\r\n",
     "Вы больше &Rне получаете&n опыт.\r\n"},
    {"Теперь сообщения в журналах будут отображаться по порядку.\r\n",
     "Теперь сообщения в журналах будут отображаться в обратном порядке.\r\n"}
  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
   case SCMD_NOSHOUT:
    result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
    break; 
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (nameserver_is_slow = !nameserver_is_slow);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_TRACK:
  //  result = (track_through_doors = !track_through_doors);
    break;
  case SCMD_CODERINFO:
//    result = PRF_TOG_CHK(ch, PRF_CODERINFO);
    break;
  case SCMD_AUTOMEM:
    result = PRF_TOG_CHK(ch, PRF_AUTOMEM);
    break;
  case SCMD_COLOR:
    do_color(ch,argument,0,0);
    return;
    break;
#if defined(HAVE_ZLIB)
   case SCMD_COMPRESS:
     result = toggle_compression(ch->desc);
     break;
   case SCMD_AUTOZLIB:
     result = PRF_TOG_CHK(ch, PRF_AUTOZLIB);
     break;
#else
   case SCMD_COMPRESS:
   case SCMD_AUTOZLIB:
     send_to_char("Сжатие не установлено.\r\n", ch);
     return;
#endif
   case SCMD_GOAHEAD:
     result = PLR_TOG_CHK(ch, PLR_GOAHEAD);
     break;
   case SCMD_DISPHP:
     result = PRF_TOG_CHK(ch, PRF_DISPHP);
     break;
   case SCMD_AGRO:
    if (!strn_cmp(argument, "вкл", 2) || PRF_FLAGGED(ch, PRF_AGRO))
	{ SET_BIT(PRF_FLAGS(ch), PRF_AGRO);
	  result = TRUE;//PRF_TOG_CHK(ch, PRF_AGRO);
	}	
	   
    if (!strn_cmp(argument, "выкл", 3) || !PRF_FLAGGED(ch, PRF_AGRO))
       { REMOVE_BIT(PRF_FLAGS(ch), PRF_AGRO);
         REMOVE_BIT(PRF_FLAGS(ch), PRF_AGRO_AUTO);
         result = FALSE; 
       }
   break;
   case SCMD_DIRECTION:
       result = PRF_TOG_CHK(ch, PRF_DIRECTION);
	break;
   case SCMD_NOFOLLOW:
       result = PLR_TOG_CHK(ch, PLR_NOFOLLOW);
	break; 
	case SCMD_NOEXP:
       result = PLR_TOG_CHK(ch, PLR_NOEXP);
	break;

    case SCMD_JOURNAL:
       result = PRF_TOG_CHK(ch, PRF_JOURNAL);
    break;
	
      default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}
