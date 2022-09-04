/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
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
#include "constants.h"
#include "dg_scripts.h"
#include "clan.h"

/* external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern struct zone_data *zone_table;
extern vector < ClanRec * > clan;

const int Reverse[NUM_OF_DIRS] = {2,3,0,1,5,4};
/* external functs */
void    make_corpse(struct char_data * ch);
void	add_follower(struct char_data *ch, struct char_data *leader);
int		special(struct char_data *ch, int cmd, char *arg);
void	death_cry(struct char_data *ch);
int		find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
int		awake_others(struct char_data *ch);
int		may_kill_here(struct char_data *ch, struct char_data *victim);
void    do_aggressive_mob(struct char_data *ch, bool check_sneak);
int     attack_best(struct char_data *ch, struct char_data *victim);

/* local functions */
void	die(struct char_data * ch);
int		has_boat(struct char_data *ch);
int		find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
int		has_key(struct char_data *ch, obj_vnum key);
void	do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
int		ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd);

ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);
ACMD(do_givehorse);


/* simple function to determine if char can walk on water */
int has_boat(struct char_data *ch)
{
  struct obj_data *obj;
  int i;
/*
  if (ROOM_IDENTITY(ch->in_room) == DEAD_SEA)
    return (1);
*/

  if (GET_LEVEL(ch) > LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK))
    return (1);

  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return (1);

  return (0);
}

/* check some death situation */
int check_death_trap(struct char_data *ch)
{if (IN_ROOM(ch) != NOWHERE)
    if ((ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && !IS_IMMORTAL(ch)) ||
        (real_sector(IN_ROOM(ch)) == SECT_FLYING && !IS_NPC(ch) && !IS_GOD(ch) &&
        !AFF_FLAGGED(ch,AFF_FLY)) ||
        (real_sector(IN_ROOM(ch)) == SECT_WATER_NOSWIM && !IS_NPC(ch) && !IS_GOD(ch) &&
        !has_boat(ch))            ||
        (real_sector(IN_ROOM(ch)) == SECT_UNDERWATER && !IS_NPC(ch) && !IS_GOD(ch) &&
        !AFF_FLAGGED(ch,AFF_WATERBREATH)))
       {log_death_trap(ch);
        death_cry(ch);
        make_corpse(ch);
	    GET_HIT(ch) = GET_MOVE(ch) = 0;
        extract_char(ch,TRUE);
        return (TRUE);
       }
 return (FALSE);
}

/* check ice in room */
int check_death_ice(int room, struct char_data *ch)
{int sector, mass=0, result = FALSE;
 struct char_data *vict/*, *next_vict */;

 if (room == NOWHERE)
    return (FALSE);
 sector = SECT(room);
 if (sector != SECT_WATER_SWIM && sector != SECT_WATER_NOSWIM)
    return (FALSE);
 if ((sector = real_sector(room)) != SECT_THIN_ICE && sector != SECT_NORMAL_ICE)
    return (FALSE);
 for (vict = world[room].people; vict; vict = vict->next_in_room)
     if (!IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_FLY))
        mass += GET_WEIGHT(vict) + IS_CARRYING_W(vict);
 if (!mass)
    return (FALSE);
 if ((sector == SECT_THIN_ICE    && mass > 500) ||
     (sector == SECT_NORMAL_ICE  && mass > 1500))
    {act("Лед проломился под Вашей тяжестью.",FALSE,world[room].people,0,0,TO_ROOM);
     act("Лед проломился под Вашей тяжестью.",FALSE,world[room].people,0,0,TO_CHAR);
     world[room].weather.icelevel = 0;
     world[room].ices             = 2;
     SET_BIT(ROOM_FLAGS(room, ROOM_ICEDEATH), ROOM_ICEDEATH);
    }
 else
    return (FALSE);
 return (result);
}

void make_visible(struct char_data *ch, long int affect)
{char to_room[MAX_STRING_LENGTH], to_char[MAX_STRING_LENGTH];

 *to_room = *to_char = 0;

 switch (affect)
 {case AFF_HIDE:
       strcpy(to_char, "Вы прекратили прятаться.\r\n");
       strcpy(to_room, "$n прекратил$y прятаться.");
       break;
  case AFF_CAMOUFLAGE:
       strcpy(to_char, "Вы прекратили маскироваться.\r\n");
       strcpy(to_room, "$n прекратил$y маскироваться.");
       break;
 }
 REMOVE_BIT(AFF_FLAGS(ch, affect), affect);
 CHECK_AGRO(ch) = TRUE;
 if (*to_char)
    send_to_char(to_char,ch);
 if (*to_room)
    act(to_room,FALSE,ch,0,0,TO_ROOM);
}


int skip_hiding (struct char_data *ch, struct char_data *vict)
{ int percent, prob;

  if (MAY_SEE(vict, ch) &&
      (AFF_FLAGGED(ch, AFF_HIDE) || affected_by_spell(ch, SPELL_HIDE))
     )
     {if (awake_hide(ch))
         {//if (affected_by_spell(ch, SPELL_HIDE))
             send_to_char("Вы попытались спрятаться, но Ваша экипировка выдала Вас.\r\n",ch);
          affect_from_char(ch, SPELL_HIDE);
          make_visible(ch, AFF_HIDE);
          SET_BIT(ch->Temporary, EXTRA_FAILHIDE);
         }
      else
      if (affected_by_spell(ch, SPELL_HIDE))
         {percent = number(1,82 + GET_REAL_INT(vict));
          prob    = calculate_skill(ch,SKILL_HIDE,percent,vict);
          if (percent > prob)
             {affect_from_char(ch, SPELL_HIDE);
              if (!AFF_FLAGGED(ch,AFF_HIDE))
                 {improove_skill(ch,SKILL_HIDE,FALSE,vict);
                  act("Вы не сумели остаться незаметным.", FALSE, ch, 0, vict, TO_CHAR);
                 }
             }
          else
	     {improove_skill(ch,SKILL_HIDE,TRUE,vict);
              act("Вам удалось остаться незаметным.",FALSE, ch, 0, vict, TO_CHAR);
              return (TRUE);
	     }
         }
     }
   return (FALSE);
}

int skip_camouflage (struct char_data *ch, struct char_data *vict)
{ int percent, prob;

  if (MAY_SEE(vict,ch) &&
      (AFF_FLAGGED(ch, AFF_CAMOUFLAGE) || affected_by_spell(ch, SPELL_CAMOUFLAGE))
     )
     {if (awake_camouflage(ch))
         {//if (affected_by_spell(ch,SPELL_CAMOUFLAGE))
             send_to_char("Вы попытались замаскироваться, но Ваша экипировка выдала Вас.\r\n",ch);
          affect_from_char(ch, SPELL_CAMOUFLAGE);
          make_visible(ch,AFF_CAMOUFLAGE);
          SET_BIT(EXTRA_FLAGS(ch), EXTRA_FAILCAMOUFLAGE);
         }
      else
      if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
         {percent = number(1,82 + GET_REAL_INT(vict));
          prob    = calculate_skill(ch,SKILL_CAMOUFLAGE,percent,vict);
          if (percent > prob)
             {affect_from_char(ch, SPELL_CAMOUFLAGE);
              if (!AFF_FLAGGED(ch,AFF_CAMOUFLAGE))
                 {improove_skill(ch,SKILL_CAMOUFLAGE,FALSE,vict);
                  act("Вы не сумели правильно замаскироваться.", FALSE, ch, 0, vict, TO_CHAR);
                 }
             }
          else
	     {improove_skill(ch,SKILL_CAMOUFLAGE,TRUE,vict);
              act("Ваша маскировка оказалась на высоте.\r\n",FALSE, ch, 0, vict, TO_CHAR);
              return (TRUE);
	     }
         }
     }
   return (FALSE);
}

int skip_sneaking(struct char_data *ch, struct char_data *vict)
{ int percent, prob;

  if (MAY_SEE(vict,ch) &&
      (AFF_FLAGGED(ch,AFF_SNEAK) || affected_by_spell(ch, SPELL_SNEAK))
     )
     {if (awake_sneak(ch))
         {//if (affected_by_spell(ch,SPELL_SNEAK))
             send_to_char("Вы попытались подкрасться, но Ваша экипировка выдала Вас.\r\n", ch);
          affect_from_char(ch, SPELL_SNEAK);
          if (affected_by_spell(ch,SPELL_HIDE))
             affect_from_char(ch, SPELL_HIDE);
          make_visible(ch,AFF_SNEAK);
          SET_BIT(EXTRA_FLAGS(ch), EXTRA_FAILSNEAK);
         }
      else
      if (affected_by_spell(ch,SPELL_SNEAK))
         {percent = number(1, 82+GET_REAL_INT(vict));
          prob    = calculate_skill(ch,SKILL_SNEAK,percent,vict);
          if (percent > prob)
             {affect_from_char(ch, SPELL_SNEAK);
              if (affected_by_spell(ch,SPELL_HIDE))
                 affect_from_char(ch, SPELL_HIDE);
              if (!AFF_FLAGGED(ch,AFF_SNEAK))
                 {improove_skill(ch,SKILL_SNEAK,FALSE,vict);
                  act("Вы не сумели пробраться незаметно.", FALSE, ch, 0, vict, TO_CHAR);
                 }
             }
          else
             {improove_skill(ch,SKILL_SNEAK,TRUE,vict);
              act("Вам подкрались.\r\n",FALSE, ch, 0, vict, TO_CHAR);
	      return (TRUE);
	     }
         }
     }
  return (FALSE);
}
 

 
int legal_dir(struct char_data *ch, int dir, int need_specials_check, int show_msg)
{ int    need_movement = 0;
  struct char_data *tch;


  if (need_specials_check && special(ch, dir + 1, ""))
     return (FALSE);

  if (!CAN_GO(ch,dir))
     return (FALSE);

  /* не пускать в ванрумы после пк */
  if (!IS_NPC(ch) && ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) && RENTABLE(ch)) {
     if (show_msg)
         send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
      return (FALSE);
  }

  /* charmed */
  if (AFF_FLAGGED(ch, AFF_CHARM) &&
      ch->master                 &&
      ch->in_room == ch->master->in_room)
     {
       if (!IS_NPC(ch))
       {
           if (show_msg)
           { send_to_char("Вы не можете покинуть свой идеал.\r\n", ch);
             act("$N попытал$U покинуть Вас.", FALSE, ch->master, 0, ch, TO_CHAR);
           }
           return (FALSE);
       }
     } 
  
  /* check NPC's */
  if (IS_NPC(ch))
     {if (GET_DEST(ch) != NOWHERE)
         return (TRUE);
      //  if this room or the one we're going to needs a boat, check for one */
      if (!MOB_FLAGGED(ch, MOB_SWIMMING) &&
          !MOB_FLAGGED(ch, MOB_FLYING)   &&
          !AFF_FLAGGED(ch, AFF_FLY)      &&
          (real_sector(ch->in_room) == SECT_WATER_NOSWIM ||
           real_sector(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM))
         {if (!has_boat(ch))
             return (FALSE);
         }
      if (!MOB_FLAGGED(ch, MOB_FLYING) &&
          !AFF_FLAGGED(ch, AFF_FLY) &&
          SECT(EXIT(ch,dir)->to_room) == SECT_FLYING)
         return (FALSE);

      if (MOB_FLAGGED(ch, MOB_ONLYSWIMMING) &&
          !(real_sector(EXIT(ch, dir)->to_room) == SECT_WATER_SWIM ||
            real_sector(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM ||
            real_sector(EXIT(ch, dir)->to_room) == SECT_UNDERWATER))
         return (FALSE);

      if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_NOMOB) &&
          !IS_HORSE(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
         return (FALSE);

      if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_DEATH) &&
          !IS_HORSE(ch))
         return (FALSE);

      if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM))
         return (FALSE);

      if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_NOHORSE) &&
          IS_HORSE(ch))
         return (FALSE);

      if (!entry_mtrigger(ch, &world[EXIT(ch, dir)->to_room], dir))
         return (FALSE);
      if (!enter_wtrigger(&world[EXIT(ch, dir)->to_room], ch, dir))
          return (FALSE);
     }
  else
     {if (real_sector(ch->in_room)            == SECT_WATER_NOSWIM ||
          real_sector(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM)
          {if (!has_boat(ch))
              {if (show_msg)
                  send_to_char("Вам нужна лодка, чтобы плыть в этом направлении.\r\n", ch);
               return (FALSE);
              }
          }

      // move points needed is avg. move loss for src and destination sect type
      need_movement = (AFF_FLAGGED(ch,AFF_FLY) || on_horse(ch)) ? 1 :
                      (movement_loss[real_sector(ch->in_room)] + movement_loss[real_sector(EXIT(ch, dir)->to_room)]) / 2;
	
      if (IS_IMMORTAL(ch))
         need_movement  = 0;
      else
      if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
         need_movement += CAMOUFLAGE_MOVES;
      else
      if (affected_by_spell(ch, SPELL_SNEAK))
         need_movement += SNEAK_MOVES;

      if (GET_MOVE(ch) < need_movement)
         {if (need_specials_check && ch->master)
             {if (show_msg)
                 send_to_char("У Вас нет сил следовать дальше.\r\n", ch);
             }
          else
             {if (show_msg)
                 send_to_char("У Вас нет сил идти дальше...\r\n", ch);
             }
          return (FALSE);
         }

      if (ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM))
             {if (show_msg)
                 send_to_char("Частная собственность ! Вход воспрещен !\r\n", ch);
              return (FALSE);
             }

      if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) &&
          (num_pc_in_room(&(world[EXIT(ch, dir)->to_room])) > 0 ||
           on_horse(ch)))
         {if (show_msg)
             send_to_char("Слишком мало места.\r\n", ch);
          return (FALSE);
         }

      if (on_horse(ch) && !legal_dir(get_horse(ch), dir, need_specials_check, FALSE))
         { if (show_msg)
              act("Ваш$Y $N отказывается туда идти.",FALSE,ch,0,get_horse(ch),TO_CHAR);
           return (FALSE);
         }

       if (on_horse(ch) && GET_HORSESTATE(get_horse(ch)) <= 0)
         {if (show_msg)
             act("Ваш$Y $N загнан$Y настолько, что не может нести Вас на себе.",FALSE,ch,0,get_horse(ch),TO_CHAR);
          return (FALSE);
         }

      if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM) &&
	      !IS_IMMORTAL(ch))
         {if (show_msg)
	         send_to_char("Вы не столь Божественны, как Вам кажется!\r\n", ch);
          return (FALSE);
         }

      if (!entry_mtrigger(ch, &world[EXIT(ch, dir)->to_room], dir))
         return (FALSE);
      if (!enter_wtrigger(&world[EXIT(ch, dir)->to_room], ch, dir))
         return (FALSE);

      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
          { if (!IS_NPC(tch))
               continue;
            if (NPC_FLAGGED(tch, 1 << dir) &&
	        AWAKE(tch) &&
		GET_POS(tch) > POS_SLEEPING &&
	        CAN_SEE(tch,ch) &&
		!AFF_FLAGGED(tch, AFF_CHARM)
               )
               {if (show_msg)
               sprintf(buf, "$N не пускает Вас %s.", dirs2[dir]);   
			   act(buf, FALSE,ch,0,tch,TO_CHAR);
                return (FALSE);
               }
           }
     }

  return (need_movement ? need_movement : 1);
}

/*#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

#define MAX_DRUNK_SONG 6
const char *drunk_songs[MAX_DRUNK_SONG] =
{"\"Шумел камыш, и-к-к..., деревья гнулися\"",
 "\"Куда ты, тропинка, меня завела\"",
 "\"Бабам, пара пабабам\"",
 "\"А мне любое море по колено\"",
 "\"Не жди меня мама, хорошего сына\"",
 "\"Бываааали дниии, весеееелыя\"",
};
#define MAX_DRUNK_VOICE 5
const char *drunk_voice[MAX_DRUNK_VOICE] =
{" - затянул$g $n",
 " - запел$g $n.",
 " - прохрипел$g $n.",
 " - зычно заревел$g $n.",
 " - разухабисто протянул$g $n.",
};

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail	разобрать
 */
/*int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  int need_movement;
  register struct char_data *k;
  const int guild[] = {1034,1040,1037,1039,1044,1038,1041,1042,1035,1036};
  const char *movies[] = {
"приш$i",
"приш$i",
"приш$i",
"приш$i",
"подкрал$u",
"приш$i",
"приш$i",
"приполз$q",	
"прилетел$y",
"приплыл$y",
"прибежал$y",
"прискакал$y",
"появил$u",
"притопал$y",
"материализовал$u",
"возник$y",
"подош$i",
"прихромал$y"
};

const char *movies_1[] = {
"уш$i",
"уш$i",
"уш$i",
"уш$i",
"скрыл$u",
"уш$i",
"уш$i",
"уполз$q",	
"улетел$y",
"уплыл$y",
"убежал$y",
"ускакал$y",
"растворил$u",
"потопал$y",
"материализовал$u",
"уш$i",
"уш$i",
"похромал$y"
};


 /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  


#define MAX_DRUNK_SONG 6
const char *drunk_songs[MAX_DRUNK_SONG] =
{"\"Шумел камыш, и-к-к..., деревья гнулись\"",
 "\"Куда ты, тропинка, меня завела\"",
 "\"Бабам, пара пабабам\"",
 "\"А мне любое море по колено\"",
 "\"Не жди меня мама, хорошего сына\"",
 "\"Бываааали дниии, весеееелыя\"",
};
#define MAX_DRUNK_VOICE 5
const char *drunk_voice[MAX_DRUNK_VOICE] =
{" - затянул$y $n",
 " - запел$y $n.",
 " - прохрипел$y $n.",
 " - зычно заревел$y $n.",
 " - разухабисто протянул$y $n.",
};
/*
const char *movies[] = {
"приш$i",
"приш$i",
"приш$i",
"приш$i",
"подкрал$u",
"приш$i",
"приш$i",
"приполз$q",	
"прилетел$y",
"приплыл$y",
"прибежал$y",
"прискакал$y",
"появил$u",
"притопал$y",
"материализовал$u",
"возник$y",
"подош$i",
"прихромал$y"
};

const char *movies_1[] = {
"уш$i",
"уш$i",
"уш$i",
"уш$i",
"скрыл$u",
"уш$i",
"уш$i",
"уполз$q",	
"улетел$y",
"уплыл$y",
"убежал$y",
"ускакал$y",
"растворил$u",
"потопал$y",
"материализовал$u",
"уш$i",
"уш$i",
"похромал$y"
};
*/

const int guildo[] = 
{
	1071,1065,1066,1068,1067,1070,1069,1073,1074,1072,1075
};

int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{ struct track_data *track;
  room_rnum was_in, go_to;
  int    need_movement, i, ndir=-1, nm, invis=0, use_horse=0, is_horse=0, direction = 0;
  int    IsFlee = dir & 0x80, mob_rnum=-1, odir;
  struct char_data *vict, *horse=NULL;
  odir = dir = dir & 0x7f;

  
  if (!(need_movement = legal_dir(ch, dir, need_specials_check, TRUE)))
     return (FALSE);

  /* Mortally drunked - it is loss direction */
  if (GET_COND(ch, DRUNK) >= CHAR_MORTALLY_DRUNKED && !on_horse(ch) &&
      GET_COND(ch, DRUNK) >= number(CHAR_DRUNKED, 50))
     for (i = 0; i < NUM_OF_DIRS && ndir < 0; i++)
         {ndir = number(0,5);
          if (!EXIT(ch, ndir) || EXIT(ch, ndir)->to_room == NOWHERE ||
              EXIT_FLAGGED(EXIT(ch, ndir), EX_CLOSED) ||
              !(nm = legal_dir(ch,ndir,need_specials_check, TRUE)))
             ndir = -1;
          else
             {if (dir != ndir)
                 {sprintf(buf,"Вы не в состоянии дальше идти...\r\n");
                  send_to_char(buf,ch);
                 }
              if (!FIGHTING(ch) && number(10,24) < GET_COND(ch,DRUNK))
                 {sprintf(buf,"%s",drunk_songs[number(0,MAX_DRUNK_SONG-1)]);
                  send_to_char(buf,ch);
                  send_to_char("\r\n",ch);
                  strcat(buf,drunk_voice[number(0,MAX_DRUNK_VOICE-1)]);
                  act(buf,FALSE,ch,0,0,TO_ROOM);
                  affect_from_char(ch,SPELL_SNEAK);
                  affect_from_char(ch,SPELL_CAMOUFLAGE);
                 };
              dir           = ndir;
              need_movement = nm;
             }
         }

  /* Now we know we're allow to go into the room. */
  if (!IS_IMMORTAL(ch) && !IS_NPC(ch))
     GET_MOVE(ch) -= need_movement;

  
  
  i = skill_info[SKILL_SNEAK].max_percent;
  if (AFF_FLAGGED(ch, AFF_SNEAK) && !IsFlee)
     {if (IS_NPC(ch))
         invis = 1;
      else
      if (awake_sneak(ch))
         {affect_from_char(ch,SPELL_SNEAK);
         }
      else
      if (!affected_by_spell(ch,SPELL_SNEAK) ||
          calculate_skill(ch,SKILL_SNEAK,i,0) >= number(1,i)
         )
        invis = 1;
     }

  i = skill_info[SKILL_CAMOUFLAGE].max_percent;
  if (AFF_FLAGGED(ch, AFF_CAMOUFLAGE) && !IsFlee)
     {if (IS_NPC(ch))
         invis = 1;
      else
      if (awake_camouflage(ch))
         {affect_from_char(ch,SPELL_CAMOUFLAGE);
         }
      else
      if (!affected_by_spell(ch,SPELL_CAMOUFLAGE) ||
          calculate_skill(ch,SKILL_CAMOUFLAGE,i,0) >= number(1,i)
         )
        invis = 1;
     }


   if (!IsFlee && PRF_FLAGGED(ch, PRF_DIRECTION))
    { sprintf (buf, "Вы направились %s.", dirs2[dir]);
      act (buf, FALSE, ch, 0, NULL, TO_CHAR);
    }

   
  was_in    = ch->in_room;
  go_to     = world[was_in].dir_option[dir]->to_room;
  direction = dir + 1;
  use_horse = AFF_FLAGGED(ch, AFF_HORSE) && has_horse(ch,FALSE) &&
              (IN_ROOM(get_horse(ch)) == was_in || IN_ROOM(get_horse(ch)) == go_to);
  is_horse  = IS_HORSE(ch) && (ch->master) && !AFF_FLAGGED(ch->master, AFF_INVISIBLE) &&
              (IN_ROOM(ch->master) == was_in || IN_ROOM(ch->master) == go_to);

  if (!invis && !is_horse)
     {if (IsFlee)
         strcpy(buf1,"убежал$y");
      else
      if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVERUN))
         strcpy(buf1,"убежал$y");
      else
      if (AFF_FLAGGED(ch, AFF_FLY) ||
          (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEFLY)))
         strcpy(buf1,"улетел$y");
      else
      if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVESWIM) &&
          (real_sector(was_in) == SECT_WATER_SWIM   ||
           real_sector(was_in) == SECT_WATER_NOSWIM ||
           real_sector(was_in) == SECT_UNDERWATER))
         strcpy(buf1,"уплыл$y");
      else
      if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEJUMP))
         strcpy(buf1,"ускакал$y");
      else
      if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVECREEP))
         strcpy(buf1,"уполз$q");
      else
      if (real_sector(was_in) == SECT_WATER_SWIM   ||
          real_sector(was_in) == SECT_WATER_NOSWIM ||
          real_sector(was_in) == SECT_UNDERWATER)
         if (use_horse)
	      sprintf(buf1,"уплыл$y верхом на %s", GET_PNAME(get_horse(ch)));
	  else
		  strcpy(buf1,"уплыл$y");
      else
      if (use_horse)
	  sprintf(buf1,"ускакал$y верхом на %s", GET_PNAME(get_horse(ch)));
      else
         strcpy(buf1,"уш$i");

      if (IsFlee && !IS_NPC(ch) && GET_CLASS(ch) == CLASS_ASSASINE)
         sprintf(buf2, "$n %s.", buf1);
      else	
        if (has_horse(ch,TRUE) && !use_horse)
		{sprintf(buf2,"$n %s %s, уводя %s за собой.", buf1, dirs2[dir], GET_VNAME(get_horse(ch)));
         GET_HORSESTATE(get_horse(ch)) -= 1;
		}
		else
		  sprintf(buf2,"$n %s %s.", buf1, dirs2[dir]);
		  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    	}

  if (invis && !is_horse) 
      act("Кто-то тихо удалился отсюда.", TRUE, ch, 0, 0, TO_ROOM_HIDE);
  
  if (on_horse(ch))
     horse = get_horse(ch);
  
      if (IsFlee)
      stop_fighting (ch, TRUE);
     

  char_from_room(ch);
  char_to_room(ch, go_to);
  if (horse)
     { GET_HORSESTATE(horse) -= 1;
       char_from_room(horse);
       char_to_room(horse,go_to);

     if (real_sector(was_in) != SECT_INSIDE &&
           real_sector(was_in) != SECT_CITY  &&
           number(1, skill_info[SKILL_RIDING].max_percent) >
		   train_skill(ch, SKILL_RIDING, skill_info[SKILL_RIDING].max_percent, NULL))
	   if (number(1,10) == 5)
	     { GET_POS(ch) = POS_SITTING;
               act("Вы упали с $R.",FALSE,ch,0,get_horse(ch),TO_CHAR);
	       act("$n свалил$u с $R.",FALSE,ch,0,get_horse(ch),TO_ROOM);
               REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);
	       WAIT_STATE(ch, PULSE_VIOLENCE *2);
	     }
    }

  if (!invis && !is_horse)
     {if (IsFlee || (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVERUN)))
         strcpy(buf1,"прибежал$y");
      else
      if (AFF_FLAGGED(ch, AFF_FLY) || (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEFLY)))
         strcpy(buf1,"прилетел$y");
      else
      if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVESWIM) &&
          (real_sector(go_to) == SECT_WATER_SWIM   ||
           real_sector(go_to) == SECT_WATER_NOSWIM ||
           real_sector(go_to) == SECT_UNDERWATER))
         strcpy(buf1,"приплыл$y");
      else
      if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEJUMP))
         strcpy(buf1,"прискакал$y");
      else
      if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVECREEP))
         strcpy(buf1,"приполз$q");
      else
      if (real_sector(go_to) == SECT_WATER_SWIM   ||
          real_sector(go_to) == SECT_WATER_NOSWIM ||
          real_sector(go_to) == SECT_UNDERWATER)
         if (use_horse)
	       sprintf(buf1,"приплыл$y верхом на %s", GET_PNAME(get_horse(ch)));
		else
			strcpy(buf1,"приплыл$y");
      else
      if (use_horse)
         sprintf(buf1,"прискакал$y верхом на %s", GET_PNAME(get_horse(ch)));
		  //strcpy(buf1,"приехал$y");
      else
         strcpy(buf1,"приш$i");

      if (has_horse(ch,TRUE) && !use_horse)
	  {  sprintf(buf2,"$n %s %s, ведя %s за собой.", buf1, dirs1[dir], GET_VNAME(get_horse(ch)));
         GET_HORSESTATE(get_horse(ch)) -= 1;
	  }
	  else
		  sprintf(buf2,"$n %s %s.", buf1, dirs1[dir]);
		  
      act(buf2, TRUE, ch, 0, 0, TO_ROOM);
      
     };

  if (invis && !is_horse) {
      act("Кто-то тихо подкрался сюда.", TRUE, ch, 0, 0, TO_ROOM_HIDE);
  }

if (!IS_NPC(ch) && ch->in_room == real_room(1051)) {
  act("$n прош$i к своему гильдмастеру.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, real_room(guildo[GET_CLASS(ch)]));
  act("$n появил$u в центре комнаты.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  return(1);
	} 

  if (ch->desc != NULL)
     look_at_room(ch, 0);

  if (check_death_trap(ch))
     {if (horse)
         extract_char(horse,FALSE);
      return (FALSE);
     }
  if (check_death_ice(go_to, ch))
     return (FALSE);

 entry_memory_mtrigger(ch, &world[world[was_in].dir_option[dir]->to_room], dir); 

  if (!greet_mtrigger(ch, &world[world[was_in].dir_option[dir]->to_room], dir) ||
      !greet_otrigger(ch, dir))
     { char_from_room(ch);
       char_to_room(ch, was_in);
       if (horse)
          {char_from_room(horse);
           char_to_room(horse,was_in);
          }
       look_at_room(ch, 0);
       return (FALSE);
     }
  else
     { 
      greet_memory_mtrigger(ch, &world[world[was_in].dir_option[dir]->to_room], dir);
      // add track info
      if (!AFF_FLAGGED(ch, AFF_NOTRACK) &&
          (!IS_NPC(ch) || (mob_rnum = GET_MOB_RNUM(ch)) >= 0)
         )
         {for (track = world[go_to].track; track; track = track->next)
              if ((IS_NPC(ch)  && IS_SET(track->track_info, TRACK_NPC) && track->who == mob_rnum) ||
                  (!IS_NPC(ch) && !IS_SET(track->track_info,TRACK_NPC) && track->who == GET_IDNUM(ch)))
                 break;

          if (!track && !ROOM_FLAGGED(go_to,ROOM_NOTRACK))
             {
                int count = 0;
                track_data* t0 = NULL;
                track_data* t = world[go_to].track;
                while (t) { count++; t0=t; t=t->next; }
                if (count < 5)
                {
                     CREATE(track, struct track_data, 1);
                }
                else
                {
                    track_data* t = world[go_to].track;
                    while (t->next != t0) t=t->next;
                    t->next = NULL;
                    for (int i=0;i<6;++i) {
                        t0->time_income[i] = 0;
                        t0->time_outgone[i] = 0;
                    }
                    track = t0;
                }
                track->track_info  = IS_NPC(ch) ? TRACK_NPC : 0;
                track->who         = IS_NPC(ch) ? mob_rnum  : GET_IDNUM(ch);
                track->next        = world[go_to].track;
                world[go_to].track = track;
             }

          if (track)
             {SET_BIT(track->time_income[Reverse[dir]],1);
              REMOVE_BIT(track->track_info, TRACK_HIDE);
             }

         for (track = world[was_in].track; track; track = track->next)
             if ((IS_NPC(ch)  && IS_SET(track->track_info, TRACK_NPC) && track->who == mob_rnum) ||
                 (!IS_NPC(ch) && !IS_SET(track->track_info,TRACK_NPC) && track->who == GET_IDNUM(ch)))
                break;

         if (!track && !ROOM_FLAGGED(was_in, ROOM_NOTRACK))
          {
              int count = 0;
              track_data* t0 = NULL;
              track_data* t = world[was_in].track;
              while (t) { count++; t0 = t; t = t->next; }
              if (count < 5)
              {
                  CREATE(track, struct track_data, 1);
              }
              else
              {
                  track_data* t = world[was_in].track;
                  while (t->next != t0) t = t->next;
                  t->next = NULL;
                  for (int i = 0; i < 6; ++i) {
                      t0->time_income[i] = 0;
                      t0->time_outgone[i] = 0;
                  }
                  track = t0;
              }
             track->track_info = IS_NPC(ch) ? TRACK_NPC : 0;
             track->who        = IS_NPC(ch) ? mob_rnum  : GET_IDNUM(ch);
             track->next       = world[was_in].track;
             world[was_in].track = track;
          }
         if (track)
            {SET_BIT(track->time_outgone[dir], 1);
             REMOVE_BIT(track->track_info, TRACK_HIDE);
            }
        }
     }

  /* track improovment */
  // if (ch->track.dirs)
  //     {sprintf(buf,"[DEBUG] Direction mask %d, dir %d\r\n",ch->track.dirs,dir);
  //      send_to_char(buf,ch);
  //     }
  if (!IS_NPC(ch) && IS_BITS(ch->track_dirs, dir))
     {send_to_char("Вы двинулись по следу.\r\n",ch);
      improove_skill(ch,SKILL_TRACK,TRUE,0);
     }
  ch->track_dirs = 0;

  /* hide improovment */
  if (IS_NPC(ch))
     for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
         {if (!IS_NPC(vict))
             skip_hiding(vict,ch);
         }

  income_mtrigger(ch,&world[world[was_in].dir_option[dir]->to_room],direction-1);

  /* char income, go mobs action */
  for (vict = world[IN_ROOM(ch)].people; !IS_NPC(ch) && vict; vict = vict->next_in_room)
      {if (!vict && !IS_NPC(vict))
          continue;

       if (!CAN_SEE(vict, ch)              ||
           AFF_FLAGGED(ch, AFF_SNEAK)      ||
           AFF_FLAGGED(ch, AFF_CAMOUFLAGE) ||
           FIGHTING(vict)                  ||
           GET_POS(vict) < POS_RESTING
          )
          continue;

       /* AWARE mobs */
       if (MOB_FLAGGED(vict,MOB_AWARE)  &&
           GET_POS(vict) < POS_FIGHTING &&
           GET_POS(vict) > POS_SLEEPING)
          {act("$n поднял$u.",FALSE,vict,0,0,TO_ROOM);
           GET_POS(vict) = POS_STANDING;
          }
      }

  /* If flee - go agressive mobs */
  if (!IS_NPC(ch) && IsFlee)
     do_aggressive_mob(ch, FALSE);

  return (direction);
}


int perform_move(struct char_data *ch, int dir, int need_specials_check, int checkmob)
{
  room_rnum was_in;
  struct follow_type *k, *next;
  
  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
     return (0);
  else
  if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE)
     send_to_char("Вы не можете идти в этом направлении...\r\n", ch);
  else
  if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
     {if (EXIT(ch, dir)->keyword)
         {sprintf(buf2, "Закрыто (%s).\r\n",EXIT(ch, dir)->keyword);
          send_to_char(buf2, ch);
         }
      else
         send_to_char("Закрыто.\r\n", ch);
     }
  else
     {if (!ch->followers)
         {if (!do_simple_move(ch, dir, need_specials_check))
             return(FALSE);
         }
      else
         {was_in = ch->in_room;
          // When leader mortally drunked - he change direction
          // So returned value set to FALSE or DIR + 1
          if (!(dir = do_simple_move(ch, dir, need_specials_check)))
             return (FALSE);
          dir--;
          for (k = ch->followers; k && k->follower->master; k = next)
              {next = k->next;
               if (k->follower->in_room == was_in &&
                   !FIGHTING(k->follower)         &&
                   HERE(k->follower)              &&
	            !GET_MOB_HOLD(k->follower)        &&
		 !AFF_FLAGGED(k->follower, AFF_HOLDALL)   &&
                   AWAKE(k->follower)             &&
                   (IS_NPC(k->follower)    ||
		    (!PLR_FLAGGED(k->follower,PLR_MAILING) &&
		     !PLR_FLAGGED(k->follower,PLR_WRITING)
		    )
		   )                              &&
	           (!IS_HORSE(k->follower) ||
               !IS_SET(AFF_FLAGS(k->follower, AFF_TETHERED),AFF_TETHERED)     
			       )
                  )
	          {if (GET_POS(k->follower) < POS_STANDING)
                      {if (IS_NPC(k->follower)         &&
                           IS_NPC(k->follower->master) &&
                           GET_POS(k->follower) > POS_SLEEPING
                          )
                          {act("$n поднял$u.",FALSE,k->follower,0,0,TO_ROOM);
                           GET_POS(k->follower) = POS_STANDING;
                          }
                       else
                          continue;
						}
                   act("Вы последовали за $T.",FALSE,k->follower,0,ch,TO_CHAR);
  	           perform_move(k->follower, dir, 1, FALSE);
                  }
              }
          }
      if (checkmob)
         do_aggressive_mob(ch, TRUE);
         
      return (TRUE);
     }
  return (FALSE);
}



ACMD(do_move)
{
  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
 
	perform_move(ch, subcmd - 1, 0, TRUE);
}

ACMD(do_hidemove)
{int    dir = 0, sneaking = affected_by_spell(ch, SPELL_SNEAK);
 struct affected_type af;

 skip_spaces(&argument);
 if (!GET_SKILL(ch, SKILL_SNEAK))
    {send_to_char("Вы не умеете этого.\r\n",ch);
     return;
    }

 if (!*argument)
    {send_to_char("И куда это Вы направляетесь?\r\n",ch);
     return;
    }


    if ((dir = search_block(argument, dirs, FALSE)) < 0)
    {send_to_char("Неизвестное направление.\r\n",ch);
     return;
    }
     
 if (on_horse(ch))
    {act("Вам мешает $N.",FALSE,ch,0,get_horse(ch),TO_CHAR);
     return;
    }
 if (!sneaking)
    {af.type      = SPELL_SNEAK;
     af.location  = 0;
     af.modifier  = 0;
     af.duration  = 1;
     af.bitvector =   (number(1,skill_info[SKILL_SNEAK].max_percent) <
                       calculate_skill(ch, SKILL_SNEAK, skill_info[SKILL_SNEAK].max_percent, 0))
                      ? AFF_SNEAK : 0;
     af.battleflag= 0;
     affect_join(ch,&af,FALSE,FALSE,FALSE,FALSE);
    }
  perform_move(ch, dir, 0, TRUE);
  if (!sneaking)
     affect_from_char(ch,SPELL_SNEAK);
}

#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
			(EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))

#define DOOR_IS(ch, door)	((EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))

#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))

#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))

#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
			(OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : \
			(EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))

#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))

#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, 2)) : \
					(EXIT(ch, door)->key))
#define DOOR_LOCK(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, 1)) : \
					(EXIT(ch, door)->exit_info))

int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
  int door;

  if (*dir)
{			//Определяем направление
    if ((door = search_block(dir, dirs, FALSE)) == -1)
	{ send_to_char("Нет такого направления.\r\n", ch);
      return (-1);
    }
    if (EXIT(ch, door))
		{ if (EXIT(ch, door)->keyword && EXIT(ch, door)->vkeyword)
		 { if (isname(type, EXIT(ch, door)->keyword) || isname(type, EXIT(ch, door)->vkeyword))
	  		return (door);
		   else
		   { sprintf(buf2, "Вы не видите здесь \"%s\".\r\n", type);
	  		 send_to_char(buf2, ch);
	  			return (-1);
		   }
         }
		else
		 return (door);
		}
   else
 	{ sprintf(buf2, "Вы не можете \"%s\" это здесь.\r\n", cmdname);
        send_to_char(buf2, ch);
     	  return (-1);
    	}
  }
  else
 	{			// попробуйте найти ключевое слово 
       if (!*type)
        { sprintf(buf2, "Что Вы хотите %s?\r\n", cmdname);
          send_to_char(buf2, ch);
          return (-1);
        }

  	for (door = 0; door < NUM_OF_DIRS; door++)
          {if (EXIT(ch, door))
	          {if (EXIT(ch, door)->keyword && EXIT(ch, door)->vkeyword)
	             {if (isname(type, EXIT(ch, door)->keyword) && EXIT(ch, door)->vkeyword)
	                  return (door);
  	             }
 	          else
	          if (DOOR_IS(ch,door) &&
	              (is_abbrev(type, "дверь") || is_abbrev(type, "door")))
	             return (door);
	         }
	      }

    sprintf(buf2, "Вы не видите здесь \"%s\"!\r\n", type);
    send_to_char(buf2, ch);
    return (-1);
  }
}


int has_key(struct char_data *ch, obj_vnum key)
{
  struct obj_data *o;
 
    if (key ==-1)
       return (1);

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return (1);

  if (GET_EQ(ch, WEAR_HOLD))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
      return (1);

  return (0);
}



#define NEED_OPEN		(1 << 0)
#define NEED_CLOSED		(1 << 1)
#define NEED_UNLOCKED	(1 << 2)
#define NEED_LOCKED		(1 << 3)

const char *cmd_door[] =
{
  "открыл$y",
  "закрыл$y",
  "отпер$q",
  "запер$q",
  "взломал$y"
};

const char *ch_cmd_door[] =
{
  "открыл$Y",
  "закрыл$Y",
  "отпер$Q",
  "запер$Q",
  "взломал$Y"
};


const char *a_cmd_door[] =
{
  "открыть",
  "закрыть",
  "отпереть",
  "запереть",
  "взломать"
};

const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};


#define EXITN(room, door)		(world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  int other_room = 0;
  struct room_direction_data *back = 0;
  int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
  const char *dirs_3[] = {
	  "на севере.",
        "на востоке.",
	  "на юге.",
	  "на западе.",
	  "вверху.",
	  "внизу."
  };
  
  sprintf(buf, "$n %s ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
    if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
          if ((back->to_room != ch->in_room)||
            ((ch->in_room >= 0 &&
		ch->in_room <= top_of_world ? world[ch->in_room].dir_option[door] : NULL)->exit_info^
             (other_room >= 0 &&
		other_room <= top_of_world ? world[other_room].dir_option[rev_dir[door]] : NULL)->exit_info)&
		(EX_ISDOOR|EX_CLOSED|EX_LOCKED))
            /* ((EXITDATA(ch->in_room,door)->exit_info ^
               EXITDATA(other_room,rev_dir[door])->exit_info) &
              (EX_ISDOOR|EX_CLOSED|EX_LOCKED))) */
	back = 0;

  switch (scmd) {
  case SCMD_OPEN:
  case SCMD_CLOSE:
    if (scmd == SCMD_OPEN  && obj  && !open_otrigger(obj,ch,FALSE))
       return;
    if (scmd == SCMD_CLOSE && obj  && !close_otrigger(obj,ch,FALSE))
       return;
    if (scmd == SCMD_OPEN  && !obj &&
        !open_wtrigger(&world[ch->in_room], ch, door, FALSE))
       return;
    if (scmd == SCMD_CLOSE && !obj &&
        !close_wtrigger(&world[ch->in_room], ch, door, FALSE))
       return;
    if (scmd == SCMD_OPEN  && !obj && back &&
        !open_wtrigger(&world[other_room], ch, rev_dir[door], FALSE))
       return;
    if (scmd == SCMD_CLOSE && !obj && back &&
        !close_wtrigger(&world[other_room], ch, rev_dir[door], FALSE))
       return;
    OPEN_DOOR(ch->in_room, obj, door);
    if (back)
      OPEN_DOOR(other_room, obj, rev_dir[door]);
    send_to_char("Хорошо.\r\n", ch);
    break;
  case SCMD_UNLOCK:
  case SCMD_LOCK:
	if (scmd == SCMD_UNLOCK  && obj  && !open_otrigger(obj,ch,TRUE))
       return;
    if (scmd == SCMD_LOCK && obj  && !close_otrigger(obj,ch,TRUE))
       return;
    if (scmd == SCMD_UNLOCK  && !obj &&
        !open_wtrigger(&world[ch->in_room], ch, door, TRUE))
       return;
    if (scmd == SCMD_LOCK && !obj &&
        !close_wtrigger(&world[ch->in_room], ch, door, TRUE))
       return;
    if (scmd == SCMD_UNLOCK  && !obj && back &&
        !open_wtrigger(&world[other_room], ch, rev_dir[door], TRUE))
       return;
    if (scmd == SCMD_LOCK && !obj && back &&
        !close_wtrigger(&world[other_room], ch, rev_dir[door], TRUE))
       return;
    LOCK_DOOR(ch->in_room, obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char("*Щелк*\r\n", ch);
    break;
   case SCMD_PICK:
	if (obj  && !pick_otrigger(obj,ch))
       return;
    if (!obj && !pick_wtrigger(&world[ch->in_room], ch, door))
       return;
    if (!obj && back && !pick_wtrigger(&world[other_room], ch, rev_dir[door]))
       return;
    LOCK_DOOR(ch->in_room, obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char("Вы умело взломали замок.\r\n", ch);
    strcpy(buf, "$n умело взломал$y замок ");
    break;
  }

  // Notify the room 
sprintf(buf + strlen(buf), "%s %s", (obj) ? "$3." :
(EXIT(ch, door)->vkeyword ? EXIT(ch, door)->vkeyword : "дверь"), (door !=-1) ? dirs_3[door] : "");
  if (!(obj) || (obj->in_room != NOWHERE))
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->vkeyword, TO_ROOM);

  // Notify the other room
  if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back) {
    sprintf(buf, "$N %s %s %s",
	           ch_cmd_door[scmd],
	           (back->vkeyword ? back->vkeyword : "дверь"),dirs_3[rev_dir[door]]);
    if (world[EXIT(ch, door)->to_room].people) {
      act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, ch, TO_ROOM);
      act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, ch, TO_CHAR);
    }
  }
}

int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
{
  int percent;

 // percent = number(1, 101);
percent = number(1, skill_info[SKILL_PICK_LOCK].max_percent);
  if (scmd == SCMD_PICK) {
    if (keynum < 0)
      send_to_char("Вы не можете найти замочную скважину.\r\n", ch);
    else if (pickproof)
      send_to_char("Вы не можете взломать это.\r\n", ch);
    else if (!check_moves(ch, PICKLOCK_MOVES));
    else if (percent > train_skill(ch, SKILL_PICK_LOCK,skill_info[SKILL_PICK_LOCK].max_percent,NULL))//GET_SKILL(ch, SKILL_PICK_LOCK))
      send_to_char("Вы не сумели взломать замок.\r\n", ch);
    else
      return (1);
    return (0);
  }
  return (1);
}

ACMD(do_gen_door)
{
  int door = -1;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

 if (subcmd == SCMD_PICK && !GET_SKILL(ch,SKILL_PICK_LOCK))
     {send_to_char("Это умение Вам недоступно.\r\n",ch);
      return;
     }
  
  skip_spaces(&argument);
  if (!*argument) {
    sprintf(buf, "%s что?", a_cmd_door[subcmd]);
    /*send_to_char(CAP(buf), ch);*/
	act(CAP(buf), FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, a_cmd_door[subcmd]);

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(ch, obj, door);
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      act("Вы не можете $F это!", FALSE, ch, 0, a_cmd_door[subcmd], TO_CHAR);
    else 
	if (!DOOR_IS_OPEN(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char("Но здесь уже закрыто!\r\n", ch);
    else 
	if (!DOOR_IS_CLOSED(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char("Это можно только закрыть!\r\n", ch);
    else 
	if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char("Здесь нечего отпирать!\r\n", ch);
    else
	if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_UNLOCKED))
      send_to_char("Здесь, кажется, заперто.\r\n", ch); 
    else 
	if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
	     ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char("У Вас нет нужного ключа.\r\n", ch);
    else 
	if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd))
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}


//Получаем истину, если это территория клан-замка
//лож - если это не территория клан-замка
bool GetClanHome (sh_int zone)
{
		 for(int i=0; i < num_of_clans; i++)
			 if (zone == clan[i]->zone)	
			 	 return true;
return false;
}
//На будущее, здесь реализовать при наличии альянса, что бы можно было 
//соклановцам в клан содружественный телепортится.
bool GetClanHome (sh_int ClanNr, sh_int ToZone)
{
  		//Не клановый игрок не может попасть в замки кланов. НО с территории замка сможет
		//свободно порталится в неклановую местность.
			if (ClanNr < 0)
			{  if (GetClanHome (ToZone))
				  return true;
			  return false;
			}	
	
			//Обработка действий кланов по отношению к замкам

			//разрешаем внутри клана порталится соклановцам
			//и возвращение в свой клан
				if (ToZone == clan[ClanNr]->zone)
				            return false;
						
			//попытка зайти в чужой клан_замок
			//если не клан замок, свободное перемещение по МИРу
				 if (GetClanHome (ToZone))
					  return true;
return false;
}


ACMD(do_enter)
{
  int door, from_room, zone;
  void set_wait (CHAR_DATA * ch, int waittime, int victim_in_room);

  one_argument(argument, buf);
  if (*buf) 			
  { if (!str_cmp("портал", buf))
        { if (!world[IN_ROOM(ch)].portal_time)
             send_to_char("Вы не видите здесь портала.\r\n",ch);
          else
          {
              from_room = IN_ROOM(ch);
              door      = world[IN_ROOM(ch)].portal_room;
              zone      = zone_table[(world[door].zone)].number;
    
              if (!IS_IMMORTAL (ch) && GetClanHome (find_clan_by_id(GET_CLAN(ch)), zone))
		      { send_to_char("Вы не можете пройти на чужую территорию.\r\n",ch);
			        return;
		      }
             
 
              act("$n исчез$q в портале.",TRUE,ch,0,0,TO_ROOM);
              char_from_room(ch);
              char_to_room(ch,door);
              set_wait (ch, 2, FALSE);
              if (ch->desc != NULL)
                 look_at_room(ch, 0);
              act("$n появил$u из портала.",TRUE,ch,0,0,TO_ROOM);
          }
      }
      else
      {
	     if (!str_cmp("школа", buf) && ch->in_room == real_room(3625)) 
         {
		     if (GET_LEVEL(ch) <= 3) 
             {
                 int rnum = real_room(1000);
                 if (rnum == -1)
                 {
                     send_to_char("Школа закрыта.\r\n",ch);
                     return;
                 }

                  act("$n прош$i в Мад-школу.", TRUE, ch, 0, 0, TO_ROOM);
                  char_from_room(ch);
                  char_to_room(ch, rnum);
                  act("$n появил$u в центре комнаты.", TRUE, ch, 0, 0, TO_ROOM);
                  look_at_room(ch, 0);
             }
	         else
             {
                 act("Вы слишком стары чтобы учиться!", TRUE, ch, 0, 0, TO_CHAR);
             }
	        return;	
        } 
	    else
        {   /* an argument was supplied, search for door
  	           * keyword */
              for (door = 0; door < NUM_OF_DIRS; door++)
              if (EXIT(ch, door))
	             if (EXIT(ch, door)->keyword)
	                if (isname(buf, EXIT(ch, door)->keyword) || isname(buf, EXIT(ch, door)->vkeyword))
	                {
                        perform_move(ch, door, 1, TRUE);
	                    return;
	                }
              sprintf(buf2, "Вы не можете никуда войти.\r\n");
              send_to_char(buf2, ch);
        }
    }

		
  } else if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
    send_to_char("Вы и так в закрытом помещении.\r\n", ch);
  else {
    // try to locate an entrance 
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	      ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1, TRUE);
	    return;
	  }
    send_to_char("Вы ничего не можете найти, что бы войти.\r\n", ch);
  }
}


ACMD(do_leave)
{
  int door;

  if (OUTSIDE(ch))
    send_to_char("Вы с другой стороны.. куда Вы хотите идти?\r\n", ch);
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	    !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1, TRUE);
	    return;
	  }
    send_to_char("Я не вижу никаких выходов с обратной стороны.\r\n", ch);
  }
}


ACMD(do_stand)
{
   if (on_horse(ch))
     {act("Для этого Вам сначала необходимо соскочить с $R.",FALSE,ch,0,get_horse(ch),TO_CHAR);
      return;
     }
	switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char("Вы уже стоите.\r\n", ch);
    break;
  case POS_SITTING:
    send_to_char("Вы встали.\r\n", ch); 
    act("$n встал$y на ноги.", TRUE, ch, 0, 0, TO_ROOM);
    /* Will be sitting after a successful bash and may still be fighting. */
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
    break;
  case POS_RESTING:
    send_to_char("Вы прекратили отдыхать и встали на ноги.\r\n", ch);
    act("$n прекратил$y отдыхать и поднял$u на ноги.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_SLEEPING:
    send_to_char("Вы это хотите сделать во сне?\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Вы и так стоите.\r\n", ch);
    break;
  default:
    send_to_char("Вы перестали летать и встали на землю.\r\n", ch);
    act("$n перестал$y летать и встал$y на землю.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}


ACMD(do_sit)
{
if (on_horse(ch))
     {act("Для этого Вам сначала необходимо соскочить с $R.",FALSE,ch,0,get_horse(ch),TO_CHAR);
      return;
     }	
	switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char("Вы сели.\r\n", ch);
    act("$n сел$y.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SITTING:
    send_to_char("Вы уже и так сидите.\r\n", ch);
    break;
  case POS_RESTING:
    send_to_char("Вы прекратили отдыхать и сели.\r\n", ch);
    act("$n прекратил$y отдыхать и сел$y.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SLEEPING:
    send_to_char("Вы это хотите сделать во сне?\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Вы хотите сесть в бою? Вам что, себя не жалко???\r\n", ch);
    break;
  default:
    send_to_char("Вы прекратили летать и сели.\r\n", ch);
    act("$n прекратил$y летать и сел$y.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_rest)
{

	if (on_horse(ch))
     {act("Для этого Вам сначала необходимо соскочить с $R.",FALSE,ch,0,get_horse(ch),TO_CHAR);
      return;
     }

switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char("Вы присели, дав своим уставшим косточкам отдохнуть.\r\n", ch);
    act("$n сел$y и начал$y отдыхать.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    send_to_char("Вы прекратили сидеть и начали отдыхать.\r\n", ch);
    act("$n начал$y отдыхать.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    send_to_char("Вы уже и так отдыхаете.\r\n", ch); 
    break;
  case POS_SLEEPING:
    send_to_char("Вы это прикажете делать во сне?.\r\n", ch);
	break;
  case POS_FIGHTING:
    send_to_char("Отдыхать во время боя? Вы в своём УМЕ?\r\n", ch);
    break;
  default:
    send_to_char("Вы прекратили летать и сели отдохнуть.\r\n", ch);
    act("$n прекратил$y летать и сел$y отдыхать.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_sleep)
{
 if (on_horse(ch))
     {act("Для этого Вам сначала необходимо соскочить с $R.",FALSE,ch,0,get_horse(ch),TO_CHAR);
      return;
     }
	switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
    send_to_char("Вы уснули.\r\n", ch);
    act("$n лег$q и уснул$y.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  case POS_SLEEPING:
    send_to_char("Вы и так уже спите.\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Спать во время боя? Вы в своём УМЕ?\r\n", ch);
    break;
  default:
    send_to_char("Вы прекратили летать, лягли и уснули.\r\n", ch);
    act("$n прекратил$y летать, лег$q и уснул$y.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
}

ACMD(do_horseon)
{ struct char_data *horse;

  if (IS_NPC(ch))
     return;

  if (!get_horse(ch))
     {send_to_char("У Вас нет скакуна.\r\n",ch);
      return;
     }

  if (on_horse(ch))
     {send_to_char("Вы уже и так на скакуне.\r\n",ch);
      return;
     }

  one_argument(argument, arg);
  if (*arg)
     horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  else
     horse = get_horse(ch);

  if (horse == NULL)
     send_to_char(NOPERSON, ch);
  else
  if (IN_ROOM(horse) != IN_ROOM(ch))
     send_to_char("Ваш скакун далеко от Вас.\r\n",ch);
  else
  if (!IS_HORSE(horse) && !IS_IMPL(ch))
     send_to_char("Это не скакун.\r\n",ch);
  else
  if (horse->master != ch)
     send_to_char("Это не Ваш скакун.\r\n",ch);
  else
  if (GET_POS(horse) < POS_FIGHTING)
     act("$N не сможет Вас нести в таком состоянии.",FALSE,ch,0,horse,TO_CHAR);
  else
  if (AFF_FLAGGED(horse, AFF_TETHERED))
     act("Вам сначала необходимо отвязать $V.",FALSE,ch,0,horse,TO_CHAR);
  else
  if (ROOM_FLAGGED (ch->in_room, ROOM_TUNNEL))
    send_to_char ("Здесь слишком мало места.\r\n", ch);
  else
     {if (affected_by_spell(ch, SPELL_SNEAK))
         affect_from_char(ch, SPELL_SNEAK);
      if (affected_by_spell(ch,SPELL_CAMOUFLAGE))
         affect_from_char(ch, SPELL_CAMOUFLAGE);
      if (affected_by_spell(ch, SPELL_FLY))
         affect_from_char(ch, SPELL_FLY);
      act("Вы вскочили на спину $R.",FALSE,ch,0,horse,TO_CHAR);
      act("$n вскочил$y на $V.",FALSE,ch,0,horse,TO_ROOM);
      SET_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);
     }
}

ACMD(do_horseoff)
{ struct char_data *horse;

  if (IS_NPC(ch))
     return;
  if (!(horse = get_horse(ch)))
     {send_to_char("У Вас нет скакуна.\r\n",ch);
      return;
     }

  if (!on_horse(ch))
     {send_to_char("Вы ведь и так не на лошади.\r\n",ch);
      return;
     }

  act("Вы спешились с $R.",FALSE,ch,0,horse,TO_CHAR);
  act("$n спешил$u с $R.",FALSE,ch,0,horse,TO_ROOM);
  REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);
}

ACMD(do_horseget)
{ struct char_data *horse;

  if (IS_NPC(ch))
     return;

  if (!get_horse(ch))
     {send_to_char("У Вас нет скакуна.\r\n",ch);
      return;
     }

  if (on_horse(ch))
     {send_to_char("Вы уже сидите на скакуне.\r\n",ch);
      return;
     }

  one_argument(argument, arg);
  if (*arg)
     horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  else
     horse = get_horse(ch);

  if (horse == NULL)
     send_to_char(NOPERSON, ch);
  else
  if (IN_ROOM(horse) != IN_ROOM(ch))
     send_to_char("Ваш скакун далеко от Вас.\r\n",ch);
  else
  if (!IS_HORSE(horse) && !IS_IMPL(ch))
     send_to_char("Это не скакун.\r\n",ch);
  else
  if (horse->master != ch)
     send_to_char("Это не Ваш скакун.\r\n",ch);
  else
  if (!AFF_FLAGGED(horse, AFF_TETHERED))
     act("А $N не привязан$Y.",FALSE,ch,0,horse,TO_CHAR);
  else
     {act("Вы отвязали $V.",FALSE,ch,0,horse,TO_CHAR);
      act("$n отвязал$y $V.",FALSE,ch,0,horse,TO_ROOM);
      REMOVE_BIT(AFF_FLAGS(horse, AFF_TETHERED), AFF_TETHERED);
     }
}


ACMD(do_horseput)
{ struct char_data *horse;

  if (IS_NPC(ch))
     return;
  if (!get_horse(ch))
     {send_to_char("У Вас нет скакуна.\r\n",ch);
      return;
     }

  if (on_horse(ch))
     {send_to_char("Вам сначала стоит соскочить со своего скакуна.\r\n",ch);
      return;
     }

  one_argument(argument, arg);
  if (*arg)
     horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  else
     horse = get_horse(ch);
  if (horse == NULL)
     send_to_char(NOPERSON, ch);
  else
  if (IN_ROOM(horse) != IN_ROOM(ch))
     send_to_char("Ваш скакун далеко от Вас.\r\n",ch);
  else
  if (!IS_HORSE(horse))
     send_to_char("Это не скакун.\r\n",ch);
  else
  if (horse->master != ch)
     send_to_char("Это не Ваш скакун.\r\n",ch);
  else
  if (AFF_FLAGGED(horse, AFF_TETHERED))
     act("А $N уже и так привязан$Y.",FALSE,ch,0,horse,TO_CHAR);
  else
     {act("Вы привязали $V.",FALSE,ch,0,horse,TO_CHAR);
      act("$n привязал$y $V.",FALSE,ch,0,horse,TO_ROOM);
      SET_BIT(AFF_FLAGS(horse, AFF_TETHERED), AFF_TETHERED);
     }
}


ACMD(do_horsetake)
{ int    percent, prob;
  struct char_data *horse = NULL;

  if (IS_NPC(ch))
     return;

  if (get_horse(ch))
     {send_to_char("Зачем Вам столько скакунов?\r\n",ch);
      return;
     }

  one_argument(argument, arg);
	if (!*arg)
		{send_to_char("Кого Вы хотите оседлать?\r\n",ch);
		return;
		}
  if (*arg)
     horse = get_char_vis(ch, arg, FIND_CHAR_ROOM);

  if (horse == NULL)
     {send_to_char(NOPERSON, ch);
      return;
     }
  else
  if (!IS_NPC(horse) && !IS_IMPL(ch))
     {send_to_char("Вы не можете этого сделать...\r\n",ch);
      return;
     }
  else
  if (!IS_GOD(ch) && !MOB_FLAGGED(horse, MOB_MOUNTING))
     {act("Вы не сможете оседлать $V.",FALSE,ch,0,horse,TO_CHAR);
      return;
     }
  else
  if (get_horse(ch))
     {if (get_horse(ch) == horse)
         act("Не стоит седлать $S еще раз.",FALSE,ch,0,horse,TO_CHAR);
      else
         send_to_char("Вам не усидеть сразу на двух скакунах.\r\n",ch);
      return;
     }
  else
  if (GET_POS(horse) < POS_STANDING)
     {act("$N не сможет стать Вашим скакуном.",FALSE,ch,0,horse,TO_CHAR);
      return;
     }
  else
  if (IS_HORSE(horse))
     {if (!IS_IMMORTAL(ch) && !(GET_CLASS(ch) == CLASS_THIEF))
         {send_to_char("Это не Ваш скакун.\r\n",ch);
          return;
         }
      if (on_horse(horse->master))
         {send_to_char("Вы не сможете увести скакуна из-под седока.\r\n",ch);
          return;
         }
      if (!IS_IMMORTAL(ch))
         {if (!GET_SKILL(ch,SKILL_STEAL))
             {send_to_char("Вы не умеете воровать.\r\n",ch);
              return;
             }
          percent = number(1,skill_info[SKILL_STEAL].max_percent);
          if (AWAKE(horse->master))
             percent += 50;
          if (AFF_FLAGGED(horse,AFF_TETHERED))
             percent += 10;
          prob    = train_skill(ch, SKILL_STEAL, skill_info[SKILL_STEAL].max_percent, 0);
          if (percent > prob)
             {act("Вы неудачно попытались украсть скакуна у $R.",FALSE,ch,0,horse->master,TO_CHAR);
              act("$n неудачно попытал$u украсть скакуна у $R.",TRUE,ch,0,horse->master,TO_NOTVICT);
              act("$n пытал$u украсть Вашего скакуна!",FALSE,ch,0,horse->master,TO_VICT);
              WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
              return;
             }
          WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
         }
      else
         stop_follower(horse,SF_EMPTY);
     }

   act("Вы оседлали $V.",FALSE,ch,0,horse,TO_CHAR);
   act("$n оседлал$y $V.",FALSE,ch,0,horse,TO_ROOM);
   make_horse(horse,ch);
}

ACMD(do_givehorse)
{struct char_data *horse, *victim;

 if (IS_NPC(ch))
    return;

 if (!(horse = get_horse(ch)))
    {send_to_char("У Вас нет скакуна!\r\n",ch);
     return;
    }
 if (!has_horse(ch,TRUE))
    {send_to_char("Вашего скакуна нет рядом.\r\n",ch);
     return;
    }
  one_argument(argument,arg);
  if (!*arg)
     {send_to_char("Кому Вы хотите отдать скакуна ?\r\n",ch);
      return;
     }
  if (!(victim = get_char_vis(ch,arg,FIND_CHAR_ROOM)))
     {send_to_char("Вам некому отдать скакуна.\r\n",ch);
      return;
     }
  else
  if (IS_NPC(victim))
     {send_to_char("Он и без этого обойдется.\r\n",ch);
      return;
     }
  if (get_horse(victim))
     {act("У $R уже есть скакун.\r\n",FALSE,ch,0,victim,TO_CHAR);
      return;
     }
  if (on_horse(ch))
     {send_to_char("Вам сначала стоит соскочить со своего скакуна.\r\n",ch);
      return;
     }
  if (AFF_FLAGGED(horse,AFF_TETHERED))
     {send_to_char("Вам стоит прежде отвязать своего скакуна.\r\n",ch);
      return;
     }
  stop_follower(horse,SF_EMPTY);
  act("Вы передали своего скакуна $D.",FALSE,ch,0,victim,TO_CHAR);
  act("$n передал$y Вам своего скакуна.",FALSE,ch,0,victim,TO_VICT);
  act("$n передал$y своего скакуна $D.",TRUE,ch,0,victim,TO_NOTVICT);
  make_horse(horse,victim);
}

ACMD(do_stophorse)
{struct char_data *horse;

 if (IS_NPC(ch))
    return;

 if (!(horse = get_horse(ch)))
    {send_to_char("У Вас нет скакуна.\r\n",ch);
     return;
    }
 if (!has_horse(ch,TRUE))
    {send_to_char("Ваш скакун далеко от Вас.\r\n",ch);
     return;
    }
 if (on_horse(ch))
     {send_to_char("Вам сначала стоит соскочить со своего скакуна.\r\n",ch);
      return;
     }
 if (AFF_FLAGGED(horse,AFF_TETHERED))
    {send_to_char("Вам стоит сначала отвязать своего скакуна.\r\n",ch);
     return;
    }
  stop_follower(horse,SF_EMPTY);
  act("Вы отпустили $V.",FALSE,ch,0,horse,TO_CHAR);
  act("$n отпустил$y $V.",FALSE,ch,0,horse,TO_ROOM);
  if (GET_MOB_VNUM(horse) == HORSE_VNUM)
     {act("$n ускакал$y в свою конюшню.\r\n",FALSE,horse,0,0,TO_ROOM);
      extract_char(horse,FALSE);
     }
}


ACMD(do_wake)
{
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if ((*arg) && subcmd == 1) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char("Возможно, Вы должны сначала проснуться.\r\n", ch);
    else if ((vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)) == NULL)
      send_to_char(NOPERSON, ch);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$N и не спал$Y.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("Вы не можете разбудить $V!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$N в плохом состоянии!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("Вы разбудили $V.", FALSE, ch, 0, vict, TO_CHAR);
      act("Вас разбудил$y $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
    }
    if (!self)
      return;
  }
  
  if (*arg) {
    send_to_char("Вы можете этой командой только себя этой будить!\r\n", ch);
	  return;
  }
	if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char("Вы не можете проснуться!\r\n", ch);
  else if (AWAKE(ch))
    send_to_char("А Вы и так не спали...\r\n", ch);
  else {
    send_to_char("Вы проснулись и встали.\r\n", ch);
    act("$n проснул$u и встал$y.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
  }
}


ACMD(do_follow)
{
  struct char_data *leader;
  struct follow_type *f;

  one_argument(argument, buf);

  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && FIGHTING(ch))
		return;

  if (*buf)
     {if (!str_cmp(buf,"я")    ||
          !str_cmp(buf,"self") ||
          !str_cmp(buf,"me"))
         {if (!ch->master)
             send_to_char("Но Вы ни за кем не следуете!\r\n",ch);
          else
             stop_follower(ch, SF_EMPTY);
          return;
         }
      if (!(leader = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
         {send_to_char(NOPERSON, ch);
          return;
         }
      }
   else
      {send_to_char("За кем Вы хотите следовать?\r\n", ch);
       return;
      }

  if (PLR_FLAGGED(leader, PLR_NOFOLLOW) && !IS_IMMORTAL(leader))
		{act("Вы не можете следовать за $T.", FALSE, ch, 0, leader, TO_CHAR);
        	return;
    	}
//если я следую, автоматом выхожу из режима нельзя следовать
     REMOVE_BIT(PLR_FLAGS(ch, PLR_NOFOLLOW), PLR_NOFOLLOW);


  if (ch->master == leader)
     {act("Вы уже следуете за $T.", FALSE, ch, 0, leader, TO_CHAR);
      return;
     }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master))
     { act("Вы можете следовать в данный момент только за $T!", FALSE, ch, 0, ch->master, TO_CHAR);
     }
  else
     {/* Not Charmed follow person */
      if (leader == ch)
         {if (!ch->master)
             {send_to_char("Вы и так следуете за собой.\r\n", ch);
	          return;
             }
          stop_follower(ch, SF_EMPTY);
         }
      else
        {
         if (circle_follow(ch, leader))
            {send_to_char("Следовать по кругу невозможно!\r\n", ch);
	         return;
            }
         
               if (ch->master)
	         stop_follower(ch, SF_EMPTY);
                  REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);

            //кто следует и в группе, исключаем
                for (f = ch->followers; f; f = f->next)
	REMOVE_BIT(AFF_FLAGS(f->follower, AFF_GROUP), AFF_GROUP); 

         add_follower(ch, leader);

        }
     }
}
