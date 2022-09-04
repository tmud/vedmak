/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "mobmax.h"
#include "pk.h"
#include "clan.h"

// Structures
struct char_data *combat_list = NULL;			// Список дерущихся чаров
struct char_data *next_combat_list = NULL;

// External structures
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern struct obj_data  *obj_proto;
extern int r_helled_start_room;
extern int pk_allowed;		/* see config.c */
extern int max_exp_gain_npc;	/* see config.c */
extern int max_exp_loss;	/* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;
extern const int material_value[];
extern int    supress_godsapply;
extern vector < ClanRec * > clan;

// External procedures 
int    get_room_sky(int rnum);
int  max_exp_loss_pc(struct char_data *ch);
int  may_kill_here(struct char_data *ch, struct char_data *victim);
char *fread_action(FILE * fl, int nr);
ACMD(do_flee);
int backstab_mult(int level);
int thaco(int ch_class, int level);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
int    awake_others(struct char_data * ch);
// local functions 
int  extra_aco(int class_num, int level);
int  level_exp(int chclass, int chlevel);
int  max_exp_gain_pc(struct char_data *ch);
int  check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim);
int  compute_armor_class(struct char_data *ch);
int  compute_thaco(struct char_data *ch);
int  level_exp(int chclass, int level);
int  equip_in_metall(struct char_data *ch);
int  npc_battle_scavenge(struct char_data *ch);
int  armour_absorbe(struct char_data * ch, struct char_data * victim, obj_data *weapon, int dam, bool too_shit);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void change_fighting(struct char_data *ch, int need_stop);
void perform_group_gain(struct char_data * ch, struct char_data * victim, int members, float koef);
void dam_message(int dam, struct char_data * ch, struct char_data * victim, int w_type);
void appear(struct char_data * ch);
void load_messages(void);
void check_killer(struct char_data * ch, struct char_data * vict);
void make_corpse(struct char_data * ch);
void change_alignment(struct char_data * ch, struct char_data * victim);
void death_cry(struct char_data * ch);
void die(struct char_data * ch, struct char_data *killer);
void group_gain(struct char_data * ch, struct char_data * victim);
void perform_violence(void);
void perform_wan(void);
void alterate_object(struct obj_data *obj, int dam, int chance);
void npc_groupbattle(struct char_data *ch);
void npc_wield(struct char_data *ch);
void npc_armor(struct char_data *ch);
void go_rescue(struct char_data *ch, struct char_data *vict, struct char_data *tmp_ch);
void go_touch(struct char_data *ch, struct char_data *vict);
void go_protect(struct char_data *ch, struct char_data *vict);
void go_chopoff(struct char_data *ch, struct char_data *vict);
void go_disarm(struct char_data *ch, struct char_data *vict);
void go_throw(struct char_data *ch, struct char_data *vict);
void go_bash(struct char_data *ch, struct char_data *vict);
void go_kick(struct char_data *ch, struct char_data *vict);
void battle_affect_update(struct char_data * ch);
void raw_kill(struct char_data * ch, struct char_data * killer);

// Weapon attack texts 
struct attack_hit_type attack_hit_text[] =
{
  {"ударили",	"ударил$y"		},	/* 0 */
  {"ужалили",	"ужалил$y"		},
  {"хлестнули",	"хлестнул$y"	},
  {"рубанули",	"рубанул$y"		},
  {"укусили",	"укусил$y"		},
  {"огрели",	"огрел$y"		},	/* 5 */
  {"сокрушили", "сокрушил$y"	},
  {"резанули",	"резанул$y"		},
  {"поцарапали","поцарапал$y"	},
  {"подстрелили","подстрелил$y"	},
  {"пырнули",	"пырнул$y"		},	/* 10 */
  {"укололи",	"уколол$y"		},
  {"ткнули",	"ткнул$y"		},
  {"лягнули",	"лягнул$y"		},
  {"боднули",	"боднул$y"		},
  {"клюнули",   "клюнул$y"		},	/* 15 */
  {"ободрали",	"ободрал$y"		},
  {"*","*"},
  {"*","*"},
  {"*","*"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_MAGIC))


int  calc_leadership(struct char_data *ch)
{ int    prob, percent;
  struct char_data *leader = 0;

  if (IS_NPC(ch)                                 ||
      !AFF_FLAGGED(ch, AFF_GROUP)                ||
      (!ch->master && !ch->followers)
     )
     return (FALSE);
     
  if (ch->master)
     {if (IN_ROOM(ch) != IN_ROOM(ch->master))
         return (FALSE);
      leader = ch->master;	 
     }
  else
     leader = ch;
     
  if (!GET_SKILL(leader, SKILL_LEADERSHIP))
     return (FALSE);
     
  percent = number(1,101);
  prob    = calculate_skill(leader,SKILL_LEADERSHIP,121,0);
  if (percent > prob)
     return (FALSE);
  else
     return (TRUE);
}




// The Fight related routines 
void appear(struct char_data * ch)
{   
  bool appear_msg = AFF_FLAGGED(ch, AFF_INVISIBLE)  ||
                   AFF_FLAGGED(ch, AFF_CAMOUFLAGE) ||
                   AFF_FLAGGED(ch, AFF_HIDE);

  if (affected_by_spell(ch, SPELL_INVISIBLE))
  { affect_from_char(ch, SPELL_INVISIBLE);
    //appear_msg =1;
  }
  if (affected_by_spell(ch, SPELL_HIDE))
  {  affect_from_char(ch, SPELL_HIDE);
     //appear_msg =1;
  }
  if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
  { affect_from_char(ch, SPELL_CAMOUFLAGE);
    //appear_msg =1;
  }

 	REMOVE_BIT(AFF_FLAGS(ch, AFF_INVISIBLE), AFF_INVISIBLE);
	REMOVE_BIT(AFF_FLAGS(ch, AFF_HIDE), AFF_HIDE);
	REMOVE_BIT(AFF_FLAGS(ch, AFF_CAMOUFLAGE), AFF_CAMOUFLAGE);


	if (appear_msg)
    { if (GET_LEVEL(ch) < LVL_IMMORT)
		{ act("$n медленно появил$u из пустоты.", FALSE, ch, 0, 0, TO_ROOM);
		  act("Вы медленно появились из пустоты.", FALSE, ch, 0, 0, TO_CHAR);
		}
	   else 
		   act("$n внезапно появил$u из пустоты", FALSE, ch, 0, 0, TO_ROOM); 
	}	
}


int compute_armor_class(struct char_data *ch)
{
  int armorclass = GET_REAL_AC(ch);

  if (AWAKE(ch)  && !(GET_MOB_HOLD(ch) || AFF_FLAGGED(ch, AFF_HOLDALL)))
    armorclass += dex_app[GET_DEX(ch)].defensive * 10;
    armorclass += extra_aco((int)GET_CLASS(ch),(int)GET_LEVEL(ch));

   if (!IS_NPC(ch))
   armorclass += (size_app[GET_REAL_SIZE(ch)].ac * 10);

  armorclass = MIN(200, armorclass);

  if (GET_AF_BATTLE(ch, EAF_PUNCTUAL))
     armorclass -= 40;


  return (MAX(-400, armorclass));      /* -100 is lowest*/
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data * victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    GET_POS(victim) = GET_POS(victim);
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;

  if (AFF_FLAGGED(victim,AFF_SLEEP) &&
      GET_POS(victim) != POS_SLEEPING
     )
     affect_from_char(victim,SPELL_SLEEP);

  if (on_horse(victim) &&
      GET_POS(victim) < POS_FIGHTING
     )
     horse_drop(get_horse(victim));

  if (IS_HORSE(victim) &&
      GET_POS(victim) < POS_FIGHTING &&
      on_horse(victim->master)
     )
     horse_drop(victim);
}

void check_killer(struct char_data * ch, struct char_data * vict)
{ struct PK_Memory_type *pk;
  struct Pk_Date *Pk;

  char buf[256];
  int a, clan_num;
 

  if (PLR_FLAGGED(vict, PLR_KILLER)			||
	  PLR_FLAGGED(vict, PLR_THIEF)			||
	  ROOM_FLAGGED(IN_ROOM(vict),ROOM_ARENA)||
	  (GET_CLAN_RANK(ch) && GET_CLAN_RANK(vict))//если 2 чара в клане, то флаг убийцы не устанавливается.
	  )
    return;
  
  if (PLR_FLAGGED(ch, PLR_KILLER) ||
	  IS_NPC(vict)				  ||
	  ch == vict)
    return;

  
  if (GET_LEVEL(ch) > GET_LEVEL(vict))
	a = GET_LEVEL(ch) - GET_LEVEL(vict);
  else
	a = GET_LEVEL(vict) - GET_LEVEL(ch);

   if (a <= 5)
	  return;

  
		for (pk = vict->pk_list; pk; pk = pk->next)
           if (pk->unique == GET_UNIQUE(ch)) 
	      {if (pk->kill_num) 
	          return;
		  }


	if ((clan_num=find_clan_by_id(GET_CLAN(ch))) >= 0)	 
		for (Pk = clan[clan_num]->CharPk; Pk; Pk = Pk->next)
			if (Pk->PkUid == GET_UNIQUE(vict))
					return;

   	if ((clan_num=find_clan_by_id(GET_CLAN(vict))) >= 0)	 
		for (Pk = clan[clan_num]->CharPk; Pk; Pk = Pk->next)
			if (Pk->PkUid == GET_UNIQUE(ch))
					return;
         if (!IS_NPC(ch))
  	{ SET_BIT(PLR_FLAGS(ch, PLR_KILLER), PLR_KILLER);
    	  GET_TIME_KILL(ch) = 60  * a;
    	  sprintf(buf, "PC Флаг убийцы был установлен %s за убийство %s в комнате - %s.",
    	  GET_DNAME(ch), GET_RNAME(vict), world[vict->in_room].name);
    	  mudlog(buf, BRF, LVL_IMMORT, TRUE);
    	  send_to_char("Вы получили флаг УБИЙЦЫ...\r\n", ch);
  	}
}






void set_battle_pos(struct char_data *ch)
{ switch (GET_POS(ch))
  {case POS_STANDING:
        GET_POS(ch)  = POS_FIGHTING;
        break;
   case POS_RESTING:
   case POS_SITTING:
   case POS_SLEEPING:
        if (GET_WAIT(ch) <= 0			&&
            !GET_MOB_HOLD(ch)			&&
            !AFF_FLAGGED(ch, AFF_SLEEP) &&
		  !AFF_FLAGGED(ch, AFF_HOLDALL) &&
            !AFF_FLAGGED(ch, AFF_CHARM))
		{ if (IS_NPC(ch))
 
			{ if (NPC_FLAGGED(ch, NPC_MOVEJUMP))
			    act("$n поднял$u на лапы.",FALSE,ch,0,0,TO_ROOM);
			else
			if (NPC_FLAGGED(ch, NPC_MOVERUN))
			    act("$n встал$y на лапы.",FALSE,ch,0,0,TO_ROOM);
			else
			if (AFF_FLAGGED(ch, AFF_FLY) || NPC_FLAGGED(ch, NPC_MOVEFLY))
			   act("$n взлетел$y снова над землей.",FALSE,ch,0,0,TO_ROOM);
			else
			if (NPC_FLAGGED(ch, NPC_MOVESWIM))
			   act("$n всплыл$y над водой.",FALSE,ch,0,0,TO_ROOM);
			else  
                        if (NPC_FLAGGED(ch, NPC_MOVECREEP))
			   act("$n приш$i в себя.",FALSE,ch,0,0,TO_ROOM);
			else
			   act("$n встал$y на ноги.",FALSE,ch,0,0,TO_ROOM);
			   GET_POS(ch)  = POS_FIGHTING;
			}
        else
           if (!IS_NPC(ch) && GET_POS(ch) == POS_SLEEPING)
		   { GET_POS(ch)  = POS_SITTING;
		 	 act("Вы проснулись и сели.",FALSE,ch,0,0,TO_CHAR);
             act("$n проснул$u и сел$y.",FALSE,ch,0,0,TO_ROOM);
                
           }
        }
        break;
  }
}

//CLASS_HUMAN        2 //гуманоид
//CLASS_ANIMAL       3 //животное
//CLASS_DRAGON       4 //драконы
//CLASS_UNDEAD       1 //нежить

void restore_battle_pos(struct char_data *ch)
{switch (GET_POS(ch))
 {case POS_FIGHTING:
        GET_POS(ch)  = POS_STANDING;
        break;
  case POS_RESTING:
  case POS_SITTING:
  case POS_SLEEPING:
       if (IS_NPC(ch)                  &&
           GET_WAIT(ch) <= 0           &&
           !GET_MOB_HOLD(ch)           &&
           !AFF_FLAGGED(ch, AFF_SLEEP) &&
		 !AFF_FLAGGED(ch, AFF_HOLDALL) &&
           !AFF_FLAGGED(ch, AFF_CHARM))
          {act("$n встал$y на ноги.",FALSE,ch,0,0,TO_ROOM);
           GET_POS(ch)  = POS_STANDING;
          }
       break;
 }
 if (AFF_FLAGGED(ch,AFF_SLEEP))
    GET_POS(ch) = POS_SLEEPING;
}



/* Начало нападения одного персонажа на другого (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    log("SYSERR: присвоение боя(%s->%s)уже и так при сражении(%s)...",
          GET_NAME(ch),GET_NAME(vict),GET_NAME(FIGHTING(ch)));
	 // core_dump();
    return;
  }

   if (PLR_FLAGGED(ch, PLR_IMMKILL) || PLR_FLAGGED(vict, PLR_IMMKILL))//запрещение боевых действий Имму
		  return;
   
   if ((IS_NPC(ch)   && MOB_FLAGGED(ch, MOB_NOFIGHT)) ||
      (IS_NPC(vict) && MOB_FLAGGED(ch, MOB_NOFIGHT)))
     return;

  ch->next_fighting = combat_list;
  combat_list = ch;

 if (AFF_FLAGGED(ch, AFF_SLEEP) && !IS_NPC(ch))
       affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  NUL_AF_BATTLE(ch);
  PROTECTING(ch) = 0;
  TOUCHING(ch)   = 0;
  INITIATIVE(ch) = 0;
  BATTLECNTR(ch) = 0;
  ch->extra_attack.used_skill = 0;
  ch->extra_attack.victim = 0;
  set_battle_pos(ch);
  
 if (AFF_FLAGGED(ch, AFF_SLEEP) && IS_NPC(ch))
    affect_from_char(ch, SPELL_SLEEP);

 if (!AFF_FLAGGED(ch,AFF_COURAGE) &&
      !AFF_FLAGGED(ch,AFF_DRUNKED) &&
      !AFF_FLAGGED(ch,AFF_ABSTINENT))
     {/*if (PRF_FLAGGED(ch, PRF_PUNCTUAL))
         SET_AF_BATTLE(ch, EAF_PUNCTUAL);
      else*/
      if (PRF_FLAGGED(ch, PRF_AWAKE))
         SET_AF_BATTLE(ch, EAF_AWAKE);
     }
 
}



/*  remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch, int switch_others)
{
  struct char_data *temp;
    
  if (ch == next_combat_list)
  next_combat_list = ch->next_fighting;

  if (PRF_FLAGGED(ch, PRF_AGRO) && PRF_FLAGGED(ch, PRF_AGRO_AUTO))
  {
      REMOVE_BIT(PRF_FLAGS(ch), PRF_AGRO);
      REMOVE_BIT(PRF_FLAGS(ch), PRF_AGRO_AUTO);
      send_to_char("\r\n&KРЕЖИМ &RАГРО&K &Wвыключен&K АВТОМАТИЧЕСКИ!&n\r\n\r\n",ch);
  }

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting  = NULL;
  FIGHTING(ch)		 = NULL;
  PROTECTING(ch)     = NULL;
  TOUCHING(ch)       = NULL;
  INITIATIVE(ch)	 = 0;
  BATTLECNTR(ch)     = 0;
  SET_EXTRA (ch, 0, NULL);
  SET_CAST (ch, 0, NULL, NULL);
  restore_battle_pos(ch);
  NUL_AF_BATTLE(ch);

  update_pos(ch);

}

void make_arena_corpse(struct char_data * ch, struct char_data *killer)
{struct obj_data *corpse;
 struct extra_descr_data *exdesc;

 corpse              = create_obj();
 corpse->item_number = NOTHING;
 corpse->in_room     = NOWHERE;
 GET_OBJ_SEX(corpse) = SEX_POLY;

 sprintf(buf2, "Останки %s лежат на земле.", GET_RNAME(ch));
 corpse->description = str_dup(buf2);

 sprintf(buf2, "труп %s", GET_RNAME(ch));
 corpse->short_description = str_dup(buf2);

 sprintf(buf2, "останки %s", GET_RNAME(ch));
 corpse->short_description = str_dup(buf2);
 corpse->name      = str_dup(buf2);

 sprintf(buf2, "останков %s", GET_RNAME(ch));
 corpse->short_rdescription = str_dup(buf2);
 sprintf(buf2, "останкам %s", GET_RNAME(ch));
 corpse->short_ddescription = str_dup(buf2);
 sprintf(buf2, "останки %s", GET_RNAME(ch));
 corpse->short_vdescription = str_dup(buf2);
 sprintf(buf2, "останками %s", GET_RNAME(ch));
 corpse->short_tdescription = str_dup(buf2);
 sprintf(buf2, "останках %s", GET_RNAME(ch));
 corpse->short_pdescription = str_dup(buf2);

 GET_OBJ_TYPE(corpse)                 		= ITEM_CONTAINER;
 GET_OBJ_WEAR(corpse)                 		= ITEM_WEAR_TAKE;
 SET_BIT(GET_OBJ_EXTRA(corpse, ITEM_NODONATE), ITEM_NODONATE);
 GET_OBJ_VAL(corpse, 0)				= 0;
 GET_OBJ_VAL(corpse, 2)				= IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
 GET_OBJ_VAL(corpse, 3)				= 1;
 GET_OBJ_WEIGHT(corpse)				= GET_WEIGHT(ch);
 GET_OBJ_RENT(corpse)				= 100000;
 GET_OBJ_TIMER(corpse) = max_pc_corpse_time * 2;
 CREATE(exdesc, struct extra_descr_data, 1);
 exdesc->keyword     = strdup(corpse->name);
 if (killer)
    sprintf(buf,"Убит на арене %s.\r\n",GET_TNAME(killer));
 else
    sprintf(buf,"Умер на арене.\r\n");
 exdesc->description = strdup(buf);
 exdesc->next = corpse->ex_description;
 corpse->ex_description = exdesc;
 obj_to_room(corpse, ch->in_room);
}

void make_corpse(struct char_data * ch)
{
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i;
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_CORPSE))
    return;
  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;
  GET_OBJ_SEX(corpse) = SEX_MALE;
  
  corpse->name = str_dup("труп"); 
  sprintf(buf2, "Труп %s лежит здесь.", GET_RNAME(ch));
  corpse->description = str_dup(buf2);
  sprintf(buf2, "труп %s", GET_RNAME(ch));
  corpse->short_description = str_dup(buf2);
  sprintf(buf2, "трупа %s", GET_RNAME(ch));
  corpse->short_rdescription = str_dup(buf2);
  sprintf(buf2, "трупу %s", GET_RNAME(ch));
  corpse->short_ddescription = str_dup(buf2);
  sprintf(buf2, "труп %s", GET_RNAME(ch));
  corpse->short_vdescription = str_dup(buf2);
  sprintf(buf2, "трупом %s", GET_RNAME(ch));
  corpse->short_tdescription = str_dup(buf2);
  sprintf(buf2, "трупе %s", GET_RNAME(ch));
  corpse->short_pdescription = str_dup(buf2);

  
  SET_BIT(GET_OBJ_EXTRA(corpse, ITEM_NOSELL), ITEM_NOSELL);
  SET_BIT(GET_OBJ_EXTRA(corpse, ITEM_NODONATE), ITEM_NODONATE);
  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 2) = IS_NPC(ch) ? GET_MOB_VNUM(ch) : -1;
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;


        //снимаем экипировку с персонажа и помещаем в труп
  for (i = 0; i < NUM_WEARS; i++)
	  if (GET_EQ(ch, i))
	  { 
        OBJ_DATA *obj = unequip_char(ch, i);
        remove_otrigger(obj, ch);
        if (OBJ_FLAGGED(obj, ITEM_DECAY))
        {
            obj_to_room(obj, IN_ROOM(ch));
            obj_decay(obj);
        }
        else
        {
	        obj_to_char(obj, ch);
        }
	  }
  	// Считаем вес шмоток после того как разденем чара перенести в линух 03.03.2007 коррекция веса трупа
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
                                                        
     //помещаем инвентарь в труп
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
  {
	  o->in_obj = corpse;
  }
  object_list_new_owner(corpse, NULL);


//	GET_HIT(ch) = 1;
  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;
  GET_OBJ_VROOM(corpse) = 0;
 if (!IS_NPC(ch))
  GET_OBJ_VROOM(corpse) = GET_ROOM_VNUM(ch->in_room);//(сохранение трупов в файле)
  obj_to_room(corpse, ch->in_room);
}


/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
	if (!GET_CLAN_RANK(ch) && !GET_REMORT(ch))
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 200;
}



void death_cry(struct char_data * ch)
{
  int door;

  act("Ваша кровь застыла, когда Вы услышали предсмертный крик $r.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (CAN_GO(ch, door))
      send_to_room("Ваша кровь застыла, когда Вы услышали чей-то предсмертный крик.\r\n", 
		world[ch->in_room].dir_option[door]->to_room, TRUE);
}


void raw_kill(struct char_data * ch, struct char_data * killer)
{ struct char_data *hitter, *vict=NULL;
  struct affected_type *af, *naf;
  int to_room;

  if (FIGHTING(ch))
     stop_fighting(ch,TRUE);

  for (hitter = combat_list; hitter; hitter = hitter->next_fighting)
      if (FIGHTING(hitter) == ch)
         WAIT_STATE(hitter, 0);

  supress_godsapply = TRUE;
  for (af = ch->affected; af; af = naf)
      {naf = af->next;
       if (!IS_SET(af->battleflag, AF_DEADKEEP))
          affect_remove(ch, af);
      }
  supress_godsapply = FALSE;
  affect_total(ch);

      if (killer)
         death_mtrigger(ch, killer);

  if (IN_ROOM(ch) != NOWHERE)
     {if (!IS_NPC(ch) && ROOM_FLAGGED(IN_ROOM(ch),ROOM_ARENA))
         {make_arena_corpse(ch,killer);
	      change_fighting(ch,TRUE);
          GET_HIT(ch) = 1;
	      GET_POS(ch) = POS_SITTING;
          char_from_room(ch);
        if ((to_room = real_room(GET_LOADROOM(ch))) == NOWHERE)
	   { SET_BIT(PLR_FLAGS(ch, PLR_HELLED), PLR_HELLED);
	     HELL_DURATION(ch) = time(0) + 6;
             to_room = r_helled_start_room;
	   }
          char_to_room(ch,to_room);
          look_at_room(ch,to_room);
          act("$v на крыльях принесли ангелы с небес!",FALSE,ch,0,0,TO_ROOM);
         }
      else
         {make_corpse(ch);
          if (!IS_NPC(ch))
    	  { GET_MANA_STORED(ch) = 0;
            GET_MANA_NEED(ch)   = 0;
			  CLR_MEMORY(ch);
		     memset(ch->real_abils.SplMem,0,MAX_SPELLS+1);
	         for ( hitter = character_list; hitter; hitter = hitter->next )
                if (IS_NPC(hitter) && MEMORY(hitter))
                   forget(hitter,ch);
			 
          }
          /* Если убит в бою - то может выйти из игры */
          if (!IS_NPC(ch)) 
	    RENTABLE(ch) = 0;
          extract_char(ch,TRUE);
         }
     }

}
void die(struct char_data * ch, struct char_data * killer)
{ int    is_pk = 0, dec_exp, cnt = 0, e;
  struct char_data *master=NULL;
  struct follow_type *f;

  if (IS_NPC(ch) || !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
     {if (!(IS_NPC(ch) || IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE)))
         {e = GET_EXP(ch);
	  dec_exp = number(GET_EXP(ch)/100, GET_EXP(ch) / 20) +
          (level_exp(GET_CLASS(ch),GET_LEVEL(ch)+1) - level_exp(GET_CLASS(ch),GET_LEVEL(ch))) / 3;
          gain_exp(ch, -dec_exp);
          dec_exp = e - GET_EXP(ch);
          sprintf(buf,"Вы потеряли %d %s опыта.\r\n",dec_exp,desc_count(dec_exp,WHAT_POINT));
          send_to_char(buf, ch); 
	}
  
     // Вычисляем замакс по мобам 
      if (IS_NPC(ch) && killer)
         {if (IS_NPC(killer) &&
	      AFF_FLAGGED(killer, AFF_CHARM) &&
	      killer->master
	     )
             master = killer->master;
          else
          if (!IS_NPC(killer))
             master = killer;
	  if (master)
	     {struct char_data *leader = master->master ? master->master : master;
	                
                       if (leader->in_room == ch->in_room)
				     if (AFF_FLAGGED( master, AFF_GROUP) && leader == master->master )
					 {  inc_kill_vnum(leader, GET_MOB_VNUM(ch), 1);
				         cnt = 1;
					 }
			   
		    for (f = leader->followers; f; f = f->next)
                       if (!IS_NPC(f->follower)            &&
		       AFF_FLAGGED(f->follower, AFF_GROUP) &&
                       f->follower->in_room == ch->in_room  )
		       inc_kill_vnum(f->follower, GET_MOB_VNUM(ch), 1);
						
	     

       	      if (!IS_NPC(master)&& !cnt) 
                  inc_kill_vnum(master, GET_MOB_VNUM(ch), 1);		 
         }
       }

      // train LEADERSHIP 
      if  (IS_NPC(ch) && killer)
       if (!IS_NPC(killer)                                &&
          AFF_FLAGGED(killer,AFF_GROUP)                  &&
          killer->master                                 &&
          GET_SKILL(killer->master,SKILL_LEADERSHIP) > 0 &&
          IN_ROOM(killer) == IN_ROOM(killer->master)
         )
         improove_skill(killer->master,SKILL_LEADERSHIP,number(0,1),ch);

//вызов функции для обработки событий клан_системы.  
	         if(killer)
	  ClanSobytia(killer, ch, is_pk);    
			    
			
	  if (!IS_NPC(ch) && killer)
         {// decrease LEADERSHIP 
          if (IS_NPC(killer)            &&
              AFF_FLAGGED(ch,AFF_GROUP) &&
              ch->master                &&
             IN_ROOM(ch) == IN_ROOM(ch->master))
             {if (GET_SKILL(ch->master, SKILL_LEADERSHIP) > 1)
                 GET_SKILL(ch->master, SKILL_LEADERSHIP)--;
             }
          }
        pk_revenge_action(killer,ch);
     }
 
  raw_kill(ch, killer);
  // if (killer)
  //   log("Killer lag is %d", GET_WAIT(killer));
}




int  get_extend_exp(int exp, struct char_data *ch, struct char_data *victim)
{int base,diff;
 int koef;

 if (!IS_NPC(victim) || IS_NPC(ch))
    return (exp);

 for (koef = 100, base = victim->mob_specials.max_factor, diff = get_kill_vnum(ch,GET_MOB_VNUM(victim));
      base < diff && koef > 5; base++, koef = koef * 95 / 100);

 koef -= 20 * MAX(0, MAX(GET_LEVEL(victim), GET_LEVEL(ch)) - MIN(GET_LEVEL(victim), GET_LEVEL(ch))-5);
 koef = MAX(1, koef);
 exp = exp * MAX(1, 100 - GET_REMORT(ch) * 7) / 100;

 exp = exp * koef / 100;


 if (AFF_FLAGGED(ch, AFF_PRISMATICAURA))
 exp /=4; 
	
//if (!(base = victim->mob_specials.max_factor))
//   return (exp);
 
 /* if ((diff = get_kill_vnum(ch,GET_MOB_VNUM(victim)) - base) <= 0)
     return (exp);
  exp = exp * base / (base+diff);*/

 return (exp);
}


void perform_group_gain(struct char_data * ch,
                        struct char_data * victim,
			int members, float koef)
{
  int exp;
  
 
if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
	  return;
  if (!OK_GAIN_EXP(ch,victim) || NPC_FLAGGED(victim, NPC_NOEXP))
     {send_to_char("Ваше деяние никто не оценил.\r\n",ch);
      return;
     }

      //расчет индивидуальной мощи и заморозка при вступлении в клан
//после выхода из клана, восстанавливается мощь, полученная до вступления в клан.
		if (!IS_NPC(ch) && (IS_GOOD(ch) && !IS_GOOD(victim) ||
				    IS_EVIL(ch) && !IS_EVIL(victim) ||
				    IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)))	   
		{
		    exp = (MAX(0,((GET_LEVEL(victim) - GET_LEVEL(ch)) +20)/2) *
			MAX(1,(GET_LEVEL(victim)-10)/4)*MAX(1,(GET_LEVEL(victim)-18)/3))/members;
			
			IND_SHOP_POWER(ch) += exp;
			IND_POWER_CHAR(ch) += exp;	
			   if (!GET_CLAN_RANK(ch))
			POWER_STORE_CHAR(ch) += exp;
		}
		else
		{
		    exp = (MAX(0,((GET_LEVEL(victim) - GET_LEVEL(ch)) +10)/2) * 
			MAX(1,(GET_LEVEL(victim)-10)/4)*MAX(1,(GET_LEVEL(victim)-18)/3))/members;
			
			IND_SHOP_POWER(ch) += exp;
			IND_POWER_CHAR(ch) += exp;	
			   if (!GET_CLAN_RANK(ch))
			POWER_STORE_CHAR(ch) += exp;
		}
  
    if (PLR_FLAGGED(ch, PLR_NOEXP))
     {send_to_char("Ваше деяние никто не оценил (Режим noexp).\r\n",ch);
      return;
     }
	 
  exp = GET_EXP(victim) / MAX(members,1) + GET_EXP(victim)/MAX(3,(members+2)/2);

  if (IS_NPC(ch))
     exp = MIN(max_exp_gain_npc, exp);
  else
     exp = MIN(max_exp_gain_pc(ch), get_extend_exp(exp, ch, victim));
  exp   = (int)(exp * koef / 100);

  if (!IS_NPC(ch))
     exp = MIN(max_exp_gain_pc(ch),exp);
  exp   = MAX(1,exp);

  exp *= SERVER_RATE;
  
  if ((IS_GOOD(ch) && IS_GOOD(victim)) ||
	   (IS_EVIL(ch) && IS_EVIL(victim)))
       if(!(exp = exp *2 /3))
		  exp =1;	
		  
  if (exp > 1)
     {sprintf(buf2, "Вы получили свою часть опыта - %d %s.\r\n", exp, desc_count(exp, WHAT_POINT));
      send_to_char(buf2, ch);
     }
  else
    send_to_char("Вы получили свою часть опыта - маленькую единичку!\r\n", ch);

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}

void group_gain(struct char_data * ch, struct char_data * victim)
{
  int  leader_inroom, inroom_members, koef = 100, maxlevel, minlevel;
  struct char_data *k;
  struct follow_type *f;

  maxlevel = minlevel = GET_LEVEL(ch);

  if (!(k = ch->master))
     k = ch;

  leader_inroom = (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room ==
  ch->in_room));


  if (leader_inroom)
     {inroom_members = 1;
      maxlevel = minlevel = GET_LEVEL(k);
     }
  else
     inroom_members = 0;

  for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
          f->follower->in_room == ch->in_room
	 )
		{ ++inroom_members;
		  //tot_members++;
          if (!IS_NPC(f->follower))
             {minlevel = MIN(minlevel, GET_LEVEL(f->follower));
              maxlevel = MAX(maxlevel, GET_LEVEL(f->follower));
             }
         }

  koef -= 20 * MAX(0, maxlevel - minlevel - 6);
 
  /* prevent illegal xp creation when killing players */
  //if (maxlevel - minlevel > 5)
  //   koef -= 50;

  
   if (leader_inroom && (inroom_members>1) && calc_leadership(k))
       koef += 20;


  koef = MAX(1, koef);

  if (leader_inroom)
      perform_group_gain( k, victim, inroom_members, koef );


  for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
          f->follower->in_room == ch->in_room
	 )
         perform_group_gain(f->follower, victim, inroom_members, koef);
}




char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
	    }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
/*сообщения о повреждении от оружия*/ 
/* локация, класс моба, модификатор от оружия*/
// 8,16 голову,  
// 0,1,9,10,11,12,13,14,15 в голову
// 2,3,5,6,7 по голове
// 4 за голову

static const char *ConPos[][7][4] = 
{       //голова 0
	{
		{" голову."," в голову."," по голове.",	" за голову."}, //человек
		{" голову."," в голову."," по голове.",	" за голову."}, //животное с лапами
		{" голову."," в голову."," по голове.",	" за голову."}, //копытное животное
		{" голову."," в голову."," по голове.",	" за голову."}, //летающее насекомое, мона и птицу
		{" голову."," в голову."," по голове.",	" за голову."}, //рыбы
		{" голову."," в голову."," по голове.",	" за голову."}, //ползающее
		{".",".",".","."},
	},  //аморфное 
		//глаз 1
	{
		{" глаз.",	" в глаз.",	" по глазу."," за глаз."},
		{" глаз.",	" в глаз.",	" по глазу."," за глаз."},
		{" глаз.",	" в глаз.",	" по глазу."," за глаз."},
		{" глаз.",	" в глаз.",	" по глазу."," за глаз."},
		{" глаз.",	" в глаз.",	" по глазу."," за глаз."},
		{" глаз.",	" в глаз.",	" по глазу."," за глаз."},
		{".",".",".","."},
		},
		//правое ухо 2
	{
		{" ухо.",	" в ухо.",		" по уху.",		" за ухо."},
		{" ухо.",	" в ухо.",		" по уху.",		" за ухо."},
		{" ухо.",	" в ухо.",		" по уху.",		" за ухо."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{".",".",".","."},
	},
		//левое ухо 3
	{
		{" ухо.",	" в ухо.",		" по уху.",		" за ухо."},
		{" ухо.",	" в ухо.",		" по уху.",		" за ухо."},
		{" ухо.",	" в ухо.",		" по уху.",		" за ухо."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{".",".",".","."},
	},
		//шея 4
	{
		{" шею.",	" в шею.",		" по шее.",		" за шею."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{" голову."," в голову.",	" по голове.",	" за голову."},
		{".",".",".","."},
	},
		//грудь 5
	{
		{" грудь.",	" в грудь.",	" по груди.",	 " за грудь."},
		{" ребро."," в ребро."," по ребрам."," за ребро."},
		{" ребро."," в ребро."," по ребрам."," за ребро."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{".",".",".","."},
	},
		//спина 6
	{
		{" спину.",	" в спину.",	" по спине.",	" за спину."},
		{" спину.",	" в спину.",	" по спине.",	" за спину."},
		{" спину.",	" в спину.",	" по спине.",	" за спину."},
		{" нервный узел.",	" в нервный узел.",	" по нервному узлу.",	" за нервный узел."},
		{" спину.",	" в спину.",	" по спине.",	" за спину."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{".",".",".","."},
	},
		//тело 7
	{
		{" тело.",	  " в тело.",	" по телу.",	" за тело."},
		{" тело.",	  " в тело.",	" по телу.",	" за тело."},
		{" тело.",	  " в тело.",	" по телу.",	" за тело."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{".",".",".","."},
	},
		//плечи(хвост) 8
	{
		{" плечо.",	" в плечо.",	" по плечу.",	" за плечо."},
		{" ребро."," в ребро."," по ребрам."," за ребро."},
		{" ребро."," в ребро."," по ребрам."," за ребро."},
		{" нервный узел.",	" в нервный узел.",	" по нервному узлу.",	" за нервный узел."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{".",".",".","."},
	},
		//живот 9
	{
		{" мышцу."," в мышцу."," по мышце."," за мышцу."},
		{" ребро."," в ребро."," по ребрам."," за ребро."},
		{" ребро."," в ребро."," по ребрам."," за ребро."},
		{" нервный узел.",	" в нервный узел.",	" по нервному узлу.",	" за нервный узел."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{".",".",".","."},
	},
		//руки 10
	{
		{" руку.",			" в руку.",				" по руке.",			" за руку."},
		{" верхние конечности."," в верхние конечности."," по верхним конечностям."," за верхние конечности."},
		{" верхние конечности."," в верхние конечности."," по верхним конечностям."," за верхние конечности."},
		{" нервный узел.",	" в нервный узел.",	" по нервному узлу.",	" за нервный узел."},
		{" туловище.",		" в туловище.",			" по туловищу.",		" за туловище."},
		{" туловище.",		" в туловище.",			" по туловищу.",		" за туловище."},
		{".",".",".","."},
	},
		//правое запястье 11
	{
		{" правое запястье.",		" в правое запястье.",		" по правому запястью.",	" за правое запястье."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" хрящ."," в хрящ."," по хрящу."," за туловище."},
		{".",".",".","."},
	},
		//левое запястье 12
	{
		{" левое запястье.",		" в левое запястье.",		" по левому запястью.",		" за левое запястье."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" хрящ."," в хрящ."," по хрящу."," за туловище."},
		{".",".",".","."},
	},
		//кисти 13
	{
		{" кисть."," в кисть."," по кистям."," за кисть."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" нервный узел.",	" в нервный узел.",	" по нервному узлу.",	" за нервный узел."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" хрящ."," в хрящ."," по хрящу."," за туловище."},
		{".",".",".","."},
	},
		//правый палец 14
	{
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" хрящ."," в хрящ."," по хрящу."," за туловище."},
		{".",".",".","."}
	},
		//левый палец 15
	{	{" сустав."," в сустав."," по суставу."," за сустав."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" хрящ."," в хрящ."," по хрящу."," за туловище."},
		{".",".",".","."}
	},
		//ноги 16
	{	{" голень."," в голень."," по голени."," за голень."},
		{" нижние конечности."," в нижние конечности."," по нижним конечностям."," за нижние конечности."},
		{" нижние конечности."," в нижние конечности."," по нижним конечностям."," за нижние конечности."},
		{" нервный узел.",	" в нервный узел.",	" по нервному узлу.",	" за нервный узел."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" хрящ."," в хрящ."," по хрящу."," за туловище."},
		{".",".",".","."}
	},
		//ступни 17
	{	{" сустав."," в сустав."," по суставу."," за сустав."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" сустав."," в сустав."," по суставу."," за сустав."},
		{" нервный узел.",	" в нервный узел.",	" по нервному узлу.",	" за нервный узел."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" хрящ."," в хрящ."," по хрящу."," за туловище."},
		{".",".",".","."}
	},
		//щит 18
	{	{" тело.",	  " в тело.",	" по телу.",	" за тело."},
		{" хребет.",	  " в хребет.",	" по хребту.",	" за туловище."},
		{" хребет.",	  " в хребет.",	" по хребту.",	" за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{" туловище."," в туловище."," по туловищу."," за туловище."},
		{".",".",".","."}
	}
		
};


void dam_message(int dam, struct char_data * ch, struct char_data * victim,
		      int w_type)
{
    char *buf;
    int posic, rases;
    //int msgnum, rand;

  /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

  /* static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } const dam_weapons[][2] =
{ 

	{{
      "$n попытал$u #W $V, но промахнул$u.",	
      "Вы попытались #w $V, но промахнулись.",
      "$n попытал$u #W Вас, но промахнул$u."
     }, 
	 {
      "$n попытал$u #W $V, но удар прошел мимо.",	//0 0 
      "Вы попытались #w $V, но удар прошел мимо.",
      "$n попытал$u #W Вас, но удар прошел мимо."
    }},
	{{
      "$n еле заметно #W $V",
      "Вы еле заметно #w $V",
      "$n еле заметно #W"
	  },
      {
      "$n еле заметно #W $V",					//1  1..2 
      "Вы еле заметно #w $V",
      "$n еле заметно #W"
	}},
	{{
      "$n еле заметно #W $V",	 
      "Вы еле заметно #w $V",
      "$n еле заметно #W"
	  },
      {										//2  3..4
      "$n еле заметно #W $V",	 
      "Вы еле заметно #w $V",
      "$n еле заметно #W"
	}},
	{{
      "$n слегка #W $V",	
      "Вы слегка #w $V",
      "$n слегка #W"
	  },
      {										//3  5..6 
      "$n слегка #W $V",		
      "Вы слегка #w $V",
      "$n слегка #W"
	}},
	{{
      "$n легонько #W $V",
      "Вы легонько #w $V",
      "$n легонько #W"
	  },
	  {
      "$n легонько #W $V",						//4  7..8  
      "Вы легонько #w $V",
      "$n легонько #W"
	}},
	{{
      "$n слабо #W $V",	
      "Вы слабо #w $V",
      "$n слабо #W"
	  },
	  {
      "$n слабо #W $V",					//5  9..10
      "Вы слабо #w $V",
      "$n слабо #W"
	}},
	{{
      "$n #W $V",   
      "Вы #w $V",
      "$n #W"
	  },
	  {										//6  11..13
      "$n #W $V",	
      "Вы #w $V",
      "$n #W"
	}},
	{{
      "$n неприятно #W $V",	 
      "Вы неприятно #w $V",
      "$n неприятно #W"
	  },
	  {										//7  14..17
      "$n неприятно #W $V",	
      "Вы неприятно #w $V",
      "$n неприятно #W"
	}},
	{{
      "$n сильно #W $V", 
      "Вы сильно #w $V",
      "$n сильно #W"
	  },
      {
      "$n сильно #W $V",			//8  18..21 
      "Вы сильно #w $V",
      "$n сильно #W"
	}}, 
	{{
      "$n очень сильно #W $V",
      "Вы очень сильно #w $V",
      "$n очень сильно #W"
	  },  
	  {
      "$n очень сильно #W $V",						//9  22..25
      "Вы очень сильно #w $V",
      "$n очень сильно #W"
	}}, 
	{{
      "$n чрезвычайно сильно #W $V",
      "Вы чрезвычайно сильно #w $V",
      "$n чрезвычайно сильно #W"	
	  },  
      {
      "$n чрезвычайно сильно #W $V",			//10  26..29 
      "Вы чрезвычайно сильно #w $V",
      "$n чрезвычайно сильно #W"
	}},
	{{
      "$n чрезвычайно сильно #W $V", 
      "Вы чрезвычайно сильно #w $V",
      "$n чрезвычайно сильно #W"
	  },
      {
      "$n чрезвычайно сильно #W $V",					//11  30..33
      "Вы чрезвычайно сильно #w $V",
      "$n чрезвычайно сильно #W"
	}},
	{{
      "$n смертельно #W $V", 
      "Вы смертельно #w $V",
      "$n смертельно #W"
	  },
      {
      "$n смертельно #W $V",			//12  34..38 
      "Вы смертельно #w $V",
      "$n смертельно #W"
	}},
	{{
      "$n смертельно #W $V",	
      "Вы смертельно #w $V",
      "$n смертельно #W"
	  },
      {
      "$n смертельно #W $V",					//13 39..43  
      "Вы смертельно #w $V",
      "$n смертельно #W"
	}},
	{{
      "$n БОЛЬНО #W $V",
      "Вы БОЛЬНО #w $V",
      "$n БОЛЬНО #W"
	  },
      {
      "$n БОЛЬНО #W $V",					//14  44..48
      "Вы БОЛЬНО #w $V",
      "$n БОЛЬНО #W"
	}},
	{{
      "$n БОЛЬНО #W $V", 
      "Вы БОЛЬНО #w $V",
      "$n БОЛЬНО #W"
      }, 
      {
      "$n БОЛЬНО #W $V",					//15  49..53  
      "Вы БОЛЬНО #w $V",
      "$n БОЛЬНО #W"
	}},
	{{
      "$n ОЧЕНЬ БОЛЬНО #W $V",
      "Вы ОЧЕНЬ БОЛЬНО #w $V",
      "$n ОЧЕНЬ БОЛЬНО #W"
      },
	  {
      "$n ОЧЕНЬ БОЛЬНО #W $V",					//16  54..58 
      "Вы ОЧЕНЬ БОЛЬНО #w $V",
      "$n ОЧЕНЬ БОЛЬНО #W"
	}},
	{{
      "$n НЕВЫНОСИМО БОЛЬНО #W $V",		
      "Вы НЕВЫНОСИМО БОЛЬНО #w $V",
      "$n НЕВЫНОСИМО БОЛЬНО #W"
	  },
	  {
      "$n НЕВЫНОСИМО БОЛЬНО #W $V",					//17  62+ 
      "Вы НЕВЫНОСИМО БОЛЬНО #w $V",
      "$n НЕВЫНОСИМО БОЛЬНО #W"
    }},
	{{
	  "$n ЗАПРЕДЕЛЬНО БОЛЬНО #W $V",  
	  "Вы ЗАПРЕДЕЛЬНО БОЛЬНО #w $V",		//18: >300
      "$n ЗАПРЕДЕЛЬНО БОЛЬНО #W"
	 },
     {
	  "$n ЗАПРЕДЕЛЬНО БОЛЬНО #W $V",  
	  "Вы ЗАПРЕДЕЛЬНО БОЛЬНО #w $V",	
      "$n ЗАПРЕДЕЛЬНО БОЛЬНО #W"
	}}
};*/

  char to_room[48];
  char to_char[48];
  char to_victim[48];

  static const char* msgs[] = {
  "ОШИБКА ",
  "еле заметно ",   //1-4
  "слегка ",        //5-6
  "легонько ",      //7-8
  "слабо ",         //9-10
  "",               //11-13
  "неприятно ",     //14-17
  "сильно ",        //18-21
  "очень сильно ",  //22-25
  "чрезвычайно сильно ",//26-33
  "смертельно ",        //34-43
  "БОЛЬНО ",            //44-58
  "ОЧЕНЬ БОЛЬНО ",      //59-85
  "НЕВЫНОСИМО БОЛЬНО ", // 86-114
  "УЖАСНО БОЛЬНО ",     //115-145
  "НЕРЕАЛЬНО БОЛЬНО ",  //146-199
  "ЗАПРЕДЕЛЬНО БОЛЬНО ",//200-299
  "<вырезано цензурой> "//300+
  };

  if (dam == 0)
  {
      strcpy(to_room, "$n попытал$u #W $V, но промахнул$u.");
      strcpy(to_char, "Вы попытались #w $V, но промахнулись."),
      strcpy(to_victim, "$n попытал$u #W Вас, но промахнул$u.");
  }
  else
  {
      int msg = 0;
      if (dam<=4) msg = 1;
      else if (dam <= 6) msg = 2;
      else if (dam <= 8) msg = 3;
      else if (dam <= 10) msg = 4;
      else if (dam <= 13) msg = 5;
      else if (dam <= 17) msg = 6;
      else if (dam <= 21) msg = 7;
      else if (dam <= 25) msg = 8;
      else if (dam <= 33) msg = 9;
      else if (dam <= 43) msg = 10;
      else if (dam <= 58) msg = 11;
      else if (dam <= 85) msg = 12;
      else if (dam <= 114) msg = 13;
      else if (dam <= 145) msg = 14;
      else if (dam <= 199) msg = 15;
      else if (dam <= 299) msg = 16;
      else msg = 17;

      sprintf(to_room, "$n %s#W $V", msgs[msg]);
      sprintf(to_char, "Вы %s#w $V", msgs[msg]);
      sprintf(to_victim, "$n %s#W", msgs[msg]);
  }

  if (w_type >= TYPE_HIT && w_type < TYPE_MAGIC)
     w_type -= TYPE_HIT;		/* Change to base of table with text */
  else
     w_type  = TYPE_HIT;
  
  switch(w_type)
  { case 4:
	 posic = 3;
    break;
    case 8:
    case 16:
	 posic = 0;
    break;
    case 2:
    case 3:
    case 5:
    case 6:
    case 7:
	 posic = 2;
    break;
    
	default:
	 posic = 1;
	  break;
  
  }
  
if  (IS_NPC(victim))
 switch(GET_CLASS(victim))
  { case 0:
    case 7:
    case 14:
	 rases = 0;
    break;
    case 1:
    case 8:
    case 15:
	 rases = 1;
    break;
    case 2:
    case 9:
    case 16:
     rases = 2;  
    break;
	case 3:
    case 10:
    case 17:
	 rases = 3;  
    break;
	case 4:
    case 11:
    case 18:
	 rases = 4;  
    break;
	case 5:
    case 12:
    case 19:
	 rases = 5;  
    break;
	case 6:
    case 13:
    case 20:
	 rases = 6;  
    break;
    
	default:
	 rases = 1;
	  break;
  
  }
  
else
   rases = 0;
 
  //rand = number(0,1);
 
  // Сообщения, которые видят все кто в комнате
  buf = replace_string(to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
   
  strcpy(buf + strlen(buf), ConPos[GET_CONPOS(ch)][rases][posic]);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);
 
  // Сообщения видит бьющий, разобраться и добавить жизни цвету сообщений 
  send_to_char("&Y", ch);
  buf = replace_string(to_char,
	    attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
          //sprintf(buf + strlen(buf), " (%d)", dam);
          sprintf(buf + strlen(buf), ConPos[GET_CONPOS(ch)][rases][posic]);
		  act(buf, FALSE, ch, NULL, victim, TO_CHAR);
          send_to_char(CCNRM(ch, C_CMP), ch);
		   
  // damage message to damagee 
  send_to_char("&R", victim);
  buf = replace_string(to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
   //sprintf(buf + strlen(buf), " (%d)", dam);
  sprintf(buf + strlen(buf), "%s%s",posic ? " Вас": " Вам", ConPos[GET_CONPOS(ch)][rases][posic]);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(victim, C_CMP), victim);
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */

#define DUMMY_KNIGHT 3030
#define DUMMY_SHIELD 3024
#define DUMMY_WEAPON 3001

static const char *KickDamag[] =
{
	"еле заметно ",		// 1-4
	"слегка ",			// 5-6
	"легонько ",		// 7-8
	"слабо ",			// 9-10
	"",			        // 11..13
	"неприятно ",		// 14..17
	"сильно ",			// 18..21
	"очень сильно ",    // 22..25
    "чрезвычайно сильно ", // 26..33
	"смертельно ",		// 34-43
	"БОЛЬНО ",	        // 44..58
    "ОЧЕНЬ БОЛЬНО ",    // 59..85
    "НЕВЫНОСИМО БОЛЬНО ", // 86..114
    "УЖАСНО БОЛЬНО ",	// 115..145
    "НЕРЕАЛЬНО БОЛЬНО ",  // 146..199
    "ЗАПРЕДЕЛЬНО БОЛЬНО ", // 200..299
    "<вырезано цензурой> " // >300+
};


int skill_message(int dam, struct char_data * ch, struct char_data * vict,
 	          int attacktype)
{
  int    i, j, nr, weap_i;
  struct message_type *msg;
  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD) ? GET_EQ(ch, WEAR_WIELD) : GET_EQ(ch, WEAR_BOTHS);

  // log("[SKILL MESSAGE] Message for skill %d",attacktype);
  for (i = 0; i < MAX_MESSAGES; i++)
      {if (fight_messages[i].a_type == attacktype)
          {
           nr = dice(1, fight_messages[i].number_of_attacks);
           // log("[SKILL MESSAGE] %d(%d)",fight_messages[i].number_of_attacks,nr);
           for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	           msg = msg->next;

           switch (attacktype)
           {case SKILL_BACKSTAB+TYPE_HIT:
                 if (!(weap=GET_EQ(ch, WEAR_WIELD))  && (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
                    weap = (obj_proto + weap_i);
		 break;
		    case SKILL_BLADE_VORTEX+TYPE_HIT:
					 if (!(weap=GET_EQ(ch, WEAR_WIELD))  && (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
						weap = (obj_proto + weap_i);
			 break;
            case SKILL_THROW+TYPE_HIT:
                 if (!(weap=GET_EQ(ch, WEAR_WIELD))  && (weap_i = real_object(DUMMY_KNIGHT)) >= 0)
                    weap = (obj_proto + weap_i);
		 break;
            case SKILL_BASH+TYPE_HIT:
                 if (!(weap=GET_EQ(ch, WEAR_SHIELD)) && (weap_i = real_object(DUMMY_SHIELD)) >= 0)
                    weap = (obj_proto + weap_i);
		 break;
            case SKILL_KICK + TYPE_HIT:
				// weap - мессажное сообщение
				if (dam <= 4)
					weap = (OBJ_DATA *) KickDamag[0];
				else if (dam <= 6)
					weap = (OBJ_DATA *) KickDamag[1];
				else if (dam <= 8)
					weap = (OBJ_DATA *) KickDamag[2];
				else if (dam <= 10)
					weap = (OBJ_DATA *) KickDamag[3];
				else if (dam <= 13)
					weap = (OBJ_DATA *) KickDamag[4];
				else if (dam <= 17)
					weap = (OBJ_DATA *) KickDamag[5];
				else if (dam <= 21)
					weap = (OBJ_DATA *) KickDamag[6];
				else if (dam <= 25)
					weap = (OBJ_DATA *) KickDamag[7];
				else if (dam <= 33)
					weap = (OBJ_DATA *) KickDamag[8];
				else if (dam <= 43)
					weap = (OBJ_DATA *) KickDamag[9];
				else if (dam <= 58)
					weap = (OBJ_DATA *) KickDamag[10];
				else if (dam <= 85)
					weap = (OBJ_DATA *) KickDamag[11];
				else if (dam <= 114)
					weap = (OBJ_DATA *) KickDamag[12];
				else if (dam <= 145)
					weap = (OBJ_DATA *) KickDamag[13];
				else if (dam <= 199)
					weap = (OBJ_DATA *) KickDamag[14];
				else if (dam <= 299)
					weap = (OBJ_DATA *) KickDamag[15];				
				else
					weap = (OBJ_DATA *) KickDamag[16];
				break;
            case TYPE_HIT:
                 weap = NULL;
		 break;
            default:
                 if (!weap && (weap_i = real_object(DUMMY_WEAPON)) >= 0)
                    weap = (obj_proto + weap_i);
           }
	
           if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT))
              {	switch (attacktype)
                {case SKILL_BACKSTAB+TYPE_HIT:
				 case SKILL_BLADE_VORTEX+TYPE_HIT:
                 case SKILL_THROW+TYPE_HIT:
                 case SKILL_BASH+TYPE_HIT:
                 case SKILL_KICK+TYPE_HIT:
                      send_to_char(CCWHT(ch, C_CMP), ch);
                      break;

                 default:
                      send_to_char(CCYEL(ch, C_CMP), ch);
                      break;
                }
                act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                send_to_char(CCNRM(ch, C_CMP), ch);

                act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
                act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
              }
           else
           if (dam != 0)
              {if (GET_POS(vict) == POS_DEAD)
                  {send_to_char(CCYEL(ch, C_CMP), ch);
                   act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                   send_to_char(CCNRM(ch, C_CMP), ch);

                   send_to_char(CCRED(vict, C_CMP), vict);
                   act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                   send_to_char(CCNRM(vict, C_CMP), vict);
                   act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
                  }
               else
                  {send_to_char(CCYEL(ch, C_CMP), ch);
                   act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                   send_to_char(CCNRM(ch, C_CMP), ch);
                   send_to_char(CCRED(vict, C_CMP), vict);
                   act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                   send_to_char(CCNRM(vict, C_CMP), vict);
                   act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
                  }
              }
           else
           if (ch != vict)
              {	/* Dam == 0 */
                switch (attacktype)
                {case SKILL_BACKSTAB+TYPE_HIT:
				 case SKILL_BLADE_VORTEX+TYPE_HIT:
                 case SKILL_THROW+TYPE_HIT:
                 case SKILL_BASH+TYPE_HIT:
                 case SKILL_KICK+TYPE_HIT:
                      send_to_char(CCWHT(ch, C_CMP), ch);
                      break;
                 default:
                      send_to_char(CCYEL(ch, C_CMP), ch);
                      break;
                }
                act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                send_to_char(CCNRM(ch, C_CMP), ch);

                send_to_char(CCRED(vict, C_CMP), vict);
                act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                send_to_char(CCNRM(vict, C_CMP), vict);

                act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
              }
          return (1);
      }
  }
  return (0);
}

		// ЗДесь идет обработка повреждений для материала экипировки

void alterate_object(struct obj_data *obj, int dam, int chance)
{if (!obj)
    return;
 dam = number (0, dam * (material_value[GET_OBJ_MATER(obj)]) /
                  MAX(1, GET_OBJ_MAX(obj) *
		         (IS_OBJ_STAT(obj,ITEM_NODROP) ? 5 : IS_OBJ_STAT(obj,ITEM_BLESS) ? 15 : 10) *
			 (GET_OBJ_SKILL(obj) == SKILL_BOWS ? 5 : 2)
		      )
              );

 if (dam > 0 && chance >= number(1, 100))
    {if ((GET_OBJ_CUR(obj) -= dam) <= 0)
        {if (obj->worn_by)
            act("$o рассыпал$U от повреждений.", FALSE, obj->worn_by, obj, 0, TO_CHAR);
         else
         if (obj->carried_by)
            act("$o рассыпал$U от повреждений.", FALSE, obj->carried_by, obj, 0, TO_CHAR);
         extract_obj(obj);
        }
    }
}



void alt_equip(struct char_data *ch, int pos, int dam, int chance)
{
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
		return;
  // calculate chance if
 if (pos == NOWHERE)
    {pos = number(0,100);
     if (pos < 3)
        pos = WEAR_FINGER_R + number(0,1);
     else
     if (pos < 6)
        pos = WEAR_NECK_1 + number(0,1);
     else
     if (pos < 10)
        pos = WEAR_EAR_R + number(0,1);
     else
     if (pos < 20)
        pos = WEAR_BODY;
     else
     if (pos < 26)
        pos = WEAR_BODY;
     else
     if (pos < 31)
        pos = WEAR_BACKPACK;
     else
     if (pos < 38)
        pos = WEAR_EYES;
     else
     if (pos < 45)
        pos = WEAR_LEGS;
     else
     if (pos < 50)
        pos = WEAR_FEET;
     else
     if (pos < 58)
        pos = WEAR_HANDS;
     else
     if (pos < 66)
        pos = WEAR_ARMS;
     else
     if (pos < 76)
        pos = WEAR_SHIELD;
     else
     if (pos < 86)
        pos = WEAR_ABOUT;
     else
     if (pos < 90)
        pos = WEAR_WAIST;
     else
     if (pos < 94)
        pos = WEAR_WRIST_R + number(0,1);
     else
        pos = WEAR_HOLD;
    }

 if (pos <= 0 || pos > WEAR_BOTHS || !GET_EQ(ch,pos) || dam < 0)
    return;
 alterate_object(GET_EQ(ch,pos), dam, chance);
}

// Сильное кровотечение / ПОКА НЕ ПОДКЛЮЧЕНА

bool was_point = FALSE; 
bool was_critic = FALSE;
ubyte dam_critic = 0;

void haemorragia(struct char_data *ch, int percent)
{struct affected_type af[3];
 int    i;

 af[0].type      = SPELL_HAEMORRAGIA;
 af[0].location  = APPLY_HITREG;
 af[0].modifier  = -percent;
 af[0].duration  = pc_duration(ch,number(1, MAX(2,10-con_app[GET_REAL_CON(ch)].critic_saving)),0,0,0,0);
 af[0].bitvector = 0;
 af[0].battleflag= 0;
 af[1].type      = SPELL_HAEMORRAGIA;
 af[1].location  = APPLY_MOVEREG;
 af[1].modifier  = -percent;
 af[1].duration  = af[0].duration;
 af[1].bitvector = 0;
 af[1].battleflag= 0;
 af[2].type      = SPELL_HAEMORRAGIA;
 af[2].location  = APPLY_MANAREG;
 af[2].modifier  = -percent;
 af[2].duration  = af[0].duration;
 af[2].bitvector = 0;
 af[2].battleflag= 0;

 for (i = 0; i < 3; i++)
     affect_join(ch, &af[i], TRUE, FALSE, TRUE, FALSE);
}

// разобрать по срокам действия яда
void poison_victim(struct char_data *ch, struct char_data *vict, int modifier)
{ struct    affected_type af[4];
  int       i;

  /* change strength */
  af[0].type         = SPELL_POISON;

  af[0].location     = APPLY_STR;
  af[0].duration     = pc_duration(vict,MAX(2, GET_LEVEL(ch)-GET_LEVEL(vict)),0,0,0,0);
  af[0].modifier     = - MIN(2, (modifier + 29) / 40);
  af[0].bitvector    = AFF_POISON;
  af[0].battleflag   = 0;
  /* change damroll */
  af[1].type        = SPELL_POISON;
  af[1].location    = APPLY_DAMROLL;
  af[1].duration    = af[0].duration;
  af[1].modifier    = - MIN(2, (modifier + 29) / 30);
  af[1].bitvector   = AFF_POISON;
  af[1].battleflag  = 0;
  /* change hitroll */
 af[2].type        = SPELL_POISON;
 af[2].location    = APPLY_HITROLL;
 af[2].duration    = af[0].duration;
 af[2].modifier    = -MIN(2, (modifier + 19) / 20);
 af[2].bitvector   = AFF_POISON;
 af[2].battleflag  = 0;
  /* change poison level */
 af[3].type        = SPELL_POISON;
 af[3].location    = APPLY_POISON;
 af[3].duration    = af[0].duration;
 af[3].modifier    = GET_LEVEL(ch);
 af[3].bitvector   = AFF_POISON;
 af[3].battleflag  = 0;

 for (i = 0; i < 4; i++)
     affect_join(vict, af+i, FALSE, FALSE, FALSE, FALSE);
 vict->Poisoner    = GET_ID(ch);
 act("Вы отравили $V.", FALSE, ch, 0, vict, TO_CHAR);
 act("$n отравил$y Вас.", FALSE, ch,0, vict, TO_VICT);
}

bool check_punch_eq(char_data *ch)
{
    if (GET_EQ(ch, WEAR_SHIELD))
        return false;
    obj_data *obj = GET_EQ(ch, WEAR_BOTHS);
    if (obj && GET_OBJ_SKILL(obj) != SKILL_PUNCH)
        return false;
    obj = GET_EQ(ch, WEAR_HOLD);
    if (obj && GET_OBJ_SKILL(obj) != SKILL_PUNCH)
        return false;
    obj = GET_EQ(ch, WEAR_WIELD);
    if (obj && GET_OBJ_SKILL(obj) != SKILL_PUNCH)
        return false;
    return true;
}

int extdamage(struct char_data * ch,
              struct char_data * victim,
	      int dam,
              int attacktype,
	      struct obj_data *wielded,
	      int mayflee)
{int    prob, percent=0, lag = 0, i, mem_dam = dam;
 struct affected_type af;

 if (!victim)
    return false;
    

 if (dam < 0)
    dam     = 0;

 // MIGHT_HIT
 if (attacktype == TYPE_HIT &&
     GET_AF_BATTLE(ch, EAF_MIGHTHIT) //&&
     //GET_WAIT(ch) <= 0
    )
   {
     CLR_AF_BATTLE(ch, EAF_MIGHTHIT);
     if (IS_NPC(ch) || IS_IMMORTAL(ch)  ||
         (check_punch_eq(ch) && !GET_AF_BATTLE(ch, EAF_TOUCH)) )
     {
         percent = number(1,skill_info[SKILL_MIGHTHIT].max_percent);
         prob    = train_skill(ch, SKILL_MIGHTHIT, skill_info[SKILL_MIGHTHIT].max_percent, victim);
	 if (GET_MOB_HOLD(victim) || AFF_FLAGGED(victim, AFF_HOLDALL))
	    prob = MAX(prob, percent);
         if (prob * 100 / percent < 20 || dam == 0)
            {sprintf(buf,"%sВаш калечащий удар не получился.%s\r\n",
                     CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag = 3;
             dam = 0;
            }
         else
         if (prob * 100 / percent < 220)
            {sprintf(buf,"%sВаш калечащий удар зацепил %s.%s\r\n",
                     CCBLU(ch, C_NRM), VPERS(victim,ch), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag  = 3;
             dam += (dam / 2);
            }
         else
         if (prob * 100 / percent < 480)
            {sprintf(buf,"%sВаш сильнейший удар потряс %s и пустил %s кровь.%s\r\n",
                     CCGRN(ch, C_NRM), VPERS(victim,ch), HMHR(victim), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag  = 2;
             dam += (dam / 2);
             WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
             af.type         = SPELL_BATTLE;
             af.bitvector    = AFF_STOPFIGHT;
             af.location     = 0;
             af.modifier     = 0;
             af.duration     = pc_duration(victim,2,0,0,0,0);
             af.battleflag   = AF_BATTLEDEC;
             affect_join(victim, &af, TRUE, FALSE, TRUE, FALSE);
             sprintf(buf,"%sВаше сознание затуманилось после удара %s.\r\nВаши раны кровоточат!%s\r\n",
                     CCRED(victim, C_NRM), RPERS(ch,victim), CCNRM(victim, C_NRM));
             send_to_char(buf, victim);
             act("Сильнейший удар $r потряс и пустил кровь $D.", FALSE, ch, 0, victim, TO_ROOM);

             if (!affected_by_spell(victim, SPELL_HAEMORRAGIA))
             {
                int h = GET_SKILL(ch, SKILL_PUNCH) / 7;
                haemorragia(victim, MIN(h, 8));
             }
         }
         else
            {sprintf(buf,"%sВаш сильнейший удар покалечил %s и пустил %s кровь.%s\r\n",
                     CCGRN(ch, C_NRM), VPERS(victim,ch), HMHR(victim), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag  = 2;
             dam *= 2;
             WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
             af.type         = SPELL_BATTLE;
             af.bitvector    = AFF_STOPFIGHT;
             af.location     = 0;
             af.modifier     = 0;
             af.duration     = pc_duration(victim,3,0,0,0,0);
             af.battleflag   = AF_BATTLEDEC;
             affect_join(victim, &af, TRUE, FALSE, TRUE, FALSE);
             sprintf(buf,"%sВаше сознание на некоторое время померкло после удара %s.\r\nВаши раны кровоточат!%s\r\n",
                     CCRED(victim, C_NRM), RPERS(ch,victim), CCNRM(victim, C_NRM));
             send_to_char(buf, victim);
             act("Сильнейший удар $r покалечил и пустил кровь $D.", FALSE, ch, 0, victim, TO_ROOM);

             if (!affected_by_spell(victim, SPELL_HAEMORRAGIA))
             {
                int h = GET_SKILL(ch, SKILL_PUNCH) / 5;
                haemorragia(victim, MIN(h, 15));
             }
         }
         if (!WAITLESS(ch))
            WAIT_STATE(ch, lag * PULSE_VIOLENCE);
        }
    }
 // STUPOR
 else
 if (GET_AF_BATTLE(ch,EAF_STUPOR) &&  GET_WAIT(ch) <= 0 )
 {
     CLR_AF_BATTLE(ch, EAF_STUPOR);
     if (IS_NPC(ch)       ||
         IS_IMMORTAL(ch)  ||
         (wielded &&
          GET_OBJ_SKILL(wielded) != SKILL_BOWS &&
          !GET_AF_BATTLE(ch, EAF_PARRY) &&
          !GET_AF_BATTLE(ch, EAF_MULTYPARRY)
         )
        )
     {  percent = number(1,skill_info[SKILL_STUPOR].max_percent);
        prob    = train_skill(ch, SKILL_STUPOR, skill_info[SKILL_STUPOR].max_percent, victim);
	    if (GET_MOB_HOLD(victim) || AFF_FLAGGED(victim, AFF_HOLDALL))
	        prob = MAX(prob, percent * 150 / 100 + 1);

         if (prob * 100 / percent < 130 || dam == 0)
            {sprintf(buf,"%sВы попытались оглушить %s, но не смогли.%s\r\n",
                     CCCYN(ch, C_NRM), VPERS(victim,ch), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             lag = 3;
             dam = 0;
            }
         else
         if (prob * 100 / percent < 320)
            {sprintf(buf,"%sВаш мощнейший удар на время оглушил %s.%s\r\n",
                     CCBLU(ch, C_NRM), VPERS(victim,ch), CCNRM(ch, C_NRM));
             send_to_char(buf,ch);
             act("Мощнейший удар $r на мгновенье оглушил $V.", FALSE, ch, 0, victim, TO_ROOM);
             lag  = 2;
             WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
             sprintf(buf,"%sОт удара %s Ваше сознание несколько померкло.%s\r\n",
                     CCRED(victim, C_NRM), RPERS(ch,victim), CCNRM(victim, C_NRM));
             send_to_char(buf, victim);
            }
         else
         {
             WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
			 if (GET_POS(victim) > POS_SITTING)
                {GET_POS(victim)        = POS_SITTING;
                sprintf(buf,"&GВаш мощнейший удар на время оглушил и свалил с ног %s.&n\r\n",VPERS(victim,ch));
                send_to_char(buf,ch);
                act("Мощнейший удар $r на мгновенье оглушил и свалил с ног $V.", FALSE, ch, 0, victim, TO_ROOM);
                lag  = 2;
                sprintf(buf,"&R%s своим мощнейшим ударом оглушил и сбил Вас с ног!&n\r\n", PERS(ch,victim));
                send_to_char(buf, victim);

                if (affected_by_spell(ch, SPELL_COURAGE)) 
                {                    
                    int prob2 = (GET_SKILL(ch, SKILL_COURAGE) + GET_STR(ch)) / 3 + GET_REMORT(ch);
                    if (number(0, 100) < prob2 && !affected_by_spell(victim, SPELL_HOLD))
                    {                    
                        struct affected_type af;
                        af.type = SPELL_HOLD;
                        af.modifier = 0;
                        af.duration   = pc_duration(victim,2,0,0,0,0);
                        af.bitvector  = AFF_HOLD;
                        af.battleflag = AF_BATTLEDEC;
                        affect_to_char(victim, &af);
                        sprintf(buf,"%s сковал паралич!&n\r\n", VPERS(victim,ch));
                        sprintf(buf2,"&c%s&n", CAP(buf));
                        send_to_char(buf2, ch);
                        if (GET_SEX(victim) == SEX_FEMALE)
                            act("&c$N обмякла и рухнула на землю парализованной!&n\r\n", FALSE, ch, 0, victim, TO_ROOM);
                        else
                            act("&c$N обмяк и рухнул на землю парализованным!&n\r\n", FALSE, ch, 0, victim, TO_ROOM);
                        sprintf(buf,"&RВы упали парализованными!&n\r\n");
                        send_to_char(buf, victim);
                    }
                }
             }
             else
             {   sprintf(buf,"%sУ Вас из глаз посыпались искры после удара %s.%s\r\n",
                         CCRED(victim, C_NRM), RPERS(ch,victim), CCNRM(victim, C_NRM));
                 send_to_char(buf, victim);
             }             
         }
         if (!WAITLESS(ch))
            WAIT_STATE(ch, lag * PULSE_VIOLENCE);
        }
    }
 // Calculate poisoned weapon
 else
 if (dam && wielded && timed_by_skill(ch, SKILL_POISONED))
    {for (i = 0; i < MAX_OBJ_AFFECT; i++)
         if (wielded->affected[i].location == APPLY_POISON)
            break;
     if (i < MAX_OBJ_AFFECT                &&
         wielded->affected[i].modifier > 0 &&
         !AFF_FLAGGED(victim, AFF_POISON)  &&
         !WAITLESS(victim)
	)
        {percent = number(1,skill_info[SKILL_POISONED].max_percent);
         prob    = calculate_skill(ch, SKILL_POISONED, skill_info[SKILL_POISONED].max_percent, victim);
         if (prob >= percent &&
             !general_savingthrow(victim, SAVING_CRITICAL, con_app[GET_REAL_CON(victim)].poison_saving)
            )
            {improove_skill(ch, SKILL_POISONED, TRUE, victim);
             poison_victim(ch,victim, prob - percent);
             wielded->affected[i].modifier--;
            }
        }
    }
 // Calculate mob-poisoner
 else
 if (dam &&
     IS_NPC(ch) &&
     NPC_FLAGGED(ch, NPC_POISON) &&
     !AFF_FLAGGED(ch,AFF_CHARM)  &&
     GET_WAIT(ch) <= 0           &&
     !AFF_FLAGGED(victim, AFF_POISON) &&
     number(0,100) < GET_LIKES(ch) + GET_LEVEL(ch) - GET_LEVEL(victim) &&
     !general_savingthrow(victim, SAVING_CRITICAL, con_app[GET_REAL_CON(victim)].poison_saving)
    )
    poison_victim(ch,victim,MAX(1,GET_LEVEL(ch) - GET_LEVEL(victim)) * 10);

 return damage(ch, victim, mem_dam >= 0 ? dam : -1, attacktype, mayflee);
}

// Повреждение частей тела и попадание по броне и оружию

int compute_critical(struct char_data *ch, struct char_data *victim, int dam)
{ const char   *to_char=NULL, *to_vict=NULL;
 struct affected_type af[2];
 struct obj_data *obj=NULL;
 int    i, unequip_pos = 0, uses_skill=0, rases, position, conpos=0;

 for (i = 0; i < 2; i++)
     {af[i].type       = 0;
      af[i].location   = APPLY_NONE;
      af[i].bitvector  = 0;
      af[i].modifier   = 0;
      af[i].battleflag = 0;
      af[i].duration   = pc_duration(victim,2,0,0,0,0);
     }


  if  (IS_NPC(victim))
 switch(GET_CLASS(victim))
  { case 0:
    case 7:
    case 14:
	 rases = 0;
      break;
    case 1:
    case 8:
    case 15:
	 rases = 1;
      break;
    case 2:
    case 9:
    case 16:
     rases = 2;
      break;
	case 3:
    case 10:
    case 17:
	   rases = 3; 
      break;
	case 4:
    case 11:
    case 18:
	 rases = 4;  
	 
    break;
	case 5:
    case 12:
    case 19:
	 rases = 5; 
	 	
    break;
	case 6:
    case 13:
    case 20:
	 rases = 6; 
//	 return (dam);
    break;
    
	default:
	 rases = 1;
	  break;
  }
 else
       rases = 0; 
   
   switch(rases)
   { case 0:
     	switch (number(1,6))
		{case 1://голова
		position = 1;
			break;
		case 2://глаз
		position = 2;
			break;
		case 3://грудь
		position = 6;
			break;
		case 4://живот
		position = 10;
			break;
		case 5://рука
		position = 11;
			break;
		case 6://нога
		position = 17;
			break;			 
		} 
     break;
     case 1: case 2:
        switch (number(1,5))
		{	case 1://голову
			position = 1;
			break;
			case 2://глаз
			position = 2;
			break;
			case 3://грудь
			position = 6;
			break;
			case 4://живот
			position = 10;
			break;		
			case 5://ногу
			if (!number(0,1)) 
			position = 17;
			else
			position = 11;
			break;	
		}	
	  break;
	 case 3:
		 switch (number(1,5))
	   {case 1://голову
		position = 1;
			break;
		case 2://глаз
		position = 2;
			break;
		case 3://грудь
		position = 6;
			break;
		case 4://брюхо
		position = 10;
			break;		
		case 5://лапу        
		position = 17;
			break;	
	   }
     break;
	 case 4: case 5:
		 switch (number(1,3))
	   {case 1://голову
		position = 1;
			break;
		case 2://глаз
		position = 2;
			break;
		case 3://туловище
		position = 6;
			break;		
	   }	
     break;
	 default:
	  break;
   }
  

  /* Find weapon for attack number weapon */

      //GET_OBJ_TYPE(obj) == ITEM_WEAPON
     if (!GET_EQ(ch,WEAR_WIELD) && !GET_EQ(ch,WEAR_HOLD))
	 { if (GET_EQ(ch,WEAR_BOTHS))
            obj = GET_EQ(ch,WEAR_BOTHS);
	 }   
     
  else
     if (GET_EQ(ch,WEAR_WIELD))
     obj = GET_EQ(ch,WEAR_WIELD);
     if (GET_EQ(ch,WEAR_HOLD))
     obj = GET_EQ(ch,WEAR_HOLD);

	if (!obj)
		uses_skill = GET_SKILL(ch, SKILL_PUNCH);
	else
        uses_skill = GET_SKILL(ch, GET_OBJ_SKILL(obj));
  


 was_critic = FALSE;
 
 
 
 dam_critic = number(0,dam_critic);
   

   switch (position)  
   {case WEAR_HEAD://голова
   if (dam_critic < 1)
   {WAIT_STATE(victim, number(2,6) *PULSE_VIOLENCE);
	to_char = "оцарапало $D";
    to_vict = "оцарапало Вам";}                   
   else
   if (dam_critic < 6)
   {WAIT_STATE(victim, 2*PULSE_VIOLENCE);
    if (GET_EQ(victim,WEAR_HEAD))
       unequip_pos = WEAR_HEAD;
    else
       {af[0].type     = SPELL_BATTLE;
        af[0].location = APPLY_HITROLL;
        af[0].modifier = -2;
		}
     dam *= MAX(2,(uses_skill-40)/15);
     to_char = "зацепило $D";
     to_vict = "зацепило Вашу";
   }           
   else
   if (dam_critic < 13)
   {WAIT_STATE(victim, 4*PULSE_VIOLENCE);    
    dam *= MAX(2,(uses_skill-40)/10);
    to_char = "повредило $D";
    to_vict = "повредило Вам";
    haemorragia(victim,20);				
   }
   else
   if (dam_critic < 20)
   {af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_STOPFIGHT;
    af[0].duration  = pc_duration(victim,2,0,0,0,0);
	af[0].battleflag= 1;
    haemorragia(victim,30);
    dam *= MAX(2,(uses_skill-40)/16);
    to_char = "проломило $D";
    to_vict = "проломило Вам";
	}
   else     
   {dam *= MAX(2,(uses_skill-40)/10);
    af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_STOPFIGHT;
    af[0].duration  = pc_duration(victim,4,0,0,0,0);
	af[0].battleflag= 1;
    haemorragia(victim,60);
    to_char = "размозжило $D";
    to_vict = "размозжило Вашу";
   }
    break;
   case WEAR_EYES://глаза
	   if (dam_critic < 1)
   {WAIT_STATE(victim, number(2,6) *PULSE_VIOLENCE);
    dam *= MAX(2,(uses_skill-40)/13);
	to_char = "повредило $D";
    to_vict = "повредило Ваш";}                   
   else   
   if (dam_critic < 13)
   {WAIT_STATE(victim, 4*PULSE_VIOLENCE);    
    dam *= MAX(2,(uses_skill-40)/10);
	af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_BLIND;
    af[0].duration  = pc_duration(victim,2,0,0,0,0);
	af[0].battleflag= 1;
    to_char = "ослепило $D";
    to_vict = "ослепило Ваш";
    haemorragia(victim,20);				
   }
   else   
   {dam *= MAX(2,(uses_skill-40)/10);
    af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_BLIND;
    af[0].duration  = pc_duration(victim,6,0,0,0,0);
	af[0].battleflag= 1;
    haemorragia(victim,60);
    to_char = "выбило $D";
    to_vict = "выбило Вам";
   }
    break;
	case WEAR_NECK_2://грудь
   if (dam_critic < 1)
   {WAIT_STATE(victim, number(2,6) *PULSE_VIOLENCE);
	to_char = "оцарапало $D";
    to_vict = "оцарапало Вам";}                   
   else
   if (dam_critic < 8)
   {dam *= MAX(2,(uses_skill-40)/13);
	WAIT_STATE(victim, 2*PULSE_VIOLENCE);
    to_char = "повредило $D";
    to_vict = "повредило Вам";
    af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_NOFLEE;
    SET_AF_BATTLE(victim, EAF_SLOW);                   
   }           
   else
   if (dam_critic < 16)
   {dam *= MAX(2,(uses_skill-40)/15);
	conpos = 1;//в грудь
	to_char = "ранило $V";
    to_vict = "ранило Вас";
    af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_STOPFIGHT;
    af[0].duration  = pc_duration(victim,1,0,0,0,0);
	af[0].battleflag= 1;
    af[1].type      = SPELL_BATTLE;
    af[1].bitvector = AFF_NOFLEE;
    haemorragia(victim,50);
    SET_AF_BATTLE(victim, EAF_SLOW);
   }
   else   
   {af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_STOPFIGHT;
    af[0].duration  = pc_duration(victim,2,0,0,0,0);
	af[0].battleflag= 1;
    dam *= MAX(2,(uses_skill-40)/11);
    haemorragia(victim,90);
    to_char = "разорвало $D";
    to_vict = "разорвало Вам";
	}
   break;
   	case WEAR_WAIST://живот
   if (dam_critic < 1)
   {WAIT_STATE(victim, number(2,6) *PULSE_VIOLENCE);
	to_char = "оцарапало $D";
    to_vict = "оцарапало Вам";}                   
   else
   if (dam_critic < 8)
   {dam *= MAX(2,(uses_skill-40)/13);
	WAIT_STATE(victim, 2*PULSE_VIOLENCE);
    to_char = "повредило $D";
    to_vict = "повредило Вам";
    af[0].modifier  = -2;
    af[0].bitvector = AFF_NOFLEE;
    af[0].type      = SPELL_BATTLE;    
	af[0].duration  = pc_duration(victim,7,0,0,0,0);
   }           
   else
   if (dam_critic < 16)
   {dam *= MAX(2,(uses_skill-40)/11);
	conpos = 1;//в живот
	to_char = "ранило $V";
    to_vict = "ранило Вас";
	af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_STOPFIGHT;
    af[0].duration  = pc_duration(victim,2,0,0,0,0);
	af[0].battleflag= 1;
    af[1].type      = SPELL_BATTLE;
    af[1].bitvector = AFF_NOFLEE;
	af[1].duration  = pc_duration(victim,10,0,0,0,0);
	af[1].battleflag= 1;
    haemorragia(victim,70);
	SET_AF_BATTLE(victim, EAF_SLOW);
   }
   else   
   {af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_STOPFIGHT;
    af[0].duration  = pc_duration(victim,4,0,0,0,0);
	af[0].battleflag= 1;
    af[1].type      = SPELL_BATTLE;
    af[1].bitvector = AFF_NOFLEE;
	af[1].duration  = pc_duration(victim,6,0,0,0,0);    
	haemorragia(victim,70);
	SET_AF_BATTLE(victim, EAF_SLOW);
    dam *= MAX(2,(uses_skill-40)/10);
    haemorragia(victim,90);
    to_char = "разорвало $D";
    to_vict = "разорвало Вам";
	}
   break;
   case WEAR_HANDS://рука
   if (!rases) //хуманоид
   { 
		if (dam_critic < 1)
		{to_char = "ранило $D";
         to_vict = "ранило Вам";
         if (GET_EQ(victim,WEAR_BOTHS))
			unequip_pos = WEAR_BOTHS;
         else
         if (GET_EQ(victim,WEAR_WIELD))
			unequip_pos = WEAR_WIELD;
         else
         if (GET_EQ(victim,WEAR_HOLD))
			unequip_pos = WEAR_HOLD;
         else
         if (GET_EQ(victim,WEAR_SHIELD))
			unequip_pos = WEAR_SHIELD;
         break;
		}                   
		else
		if (dam_critic < 8)
		{WAIT_STATE(victim, number(2,4) * PULSE_VIOLENCE);
         if (GET_EQ(victim,WEAR_BOTHS))
			unequip_pos = WEAR_BOTHS;
         else
         if (GET_EQ(victim,WEAR_WIELD))
            unequip_pos = WEAR_WIELD;
         else
         if (GET_EQ(victim,WEAR_HOLD))
            unequip_pos = WEAR_HOLD;
         dam *= MAX(2,(uses_skill-40)/18);
         to_char = "повредило $D";
         to_vict = "повредило Вам";
         break;
		}           
		else
		if (dam_critic < 16)
		{dam *= MAX(2,(uses_skill-40)/14);
			if (!AFF_FLAGGED(victim, AFF_STOPRIGHT))
                {to_char = "изуродовало $D правую";
                to_vict = "изуродовало Вам правую";
                af[0].type      = SPELL_BATTLE;
                af[0].bitvector = AFF_STOPRIGHT;
                af[0].duration  = pc_duration(victim,4,0,0,0,0);
				af[0].battleflag= 1;
                }
             else
             if (!AFF_FLAGGED(victim, AFF_STOPLEFT))
                {to_char = "изуродовало $D левую";
                to_vict = "изуродовало Вам левую";
                af[0].type      = SPELL_BATTLE;
                af[0].bitvector = AFF_STOPLEFT;
                af[0].duration  = pc_duration(victim,4,0,0,0,0);
				af[0].battleflag= 1;
                }
             else
                {conpos = 1;
				 to_char = "БОЛЬНО ранило $V";
                to_vict = "БОЛЬНО ранило Вас";
                af[0].type      = SPELL_BATTLE;
                af[0].bitvector = AFF_STOPFIGHT;
                af[0].duration  = pc_duration(victim,4,0,0,0,0);
				af[0].battleflag= 1;
                }
           haemorragia(victim,50);
		}
		else   
		{dam *= MAX(2,(uses_skill-40)/12);
			if (!AFF_FLAGGED(victim, AFF_STOPRIGHT))
                {to_char = "сломало $D правую";
                to_vict = "сломало Вам правую";
                af[0].type      = SPELL_BATTLE;
                af[0].bitvector = AFF_STOPRIGHT;
                af[0].duration  = pc_duration(victim,4,0,0,0,0);				
                }
             else
             if (!AFF_FLAGGED(victim, AFF_STOPLEFT))
                {to_char = "сломало $D левую";
                to_vict = "сломало Вам левую";
                af[0].type      = SPELL_BATTLE;
                af[0].bitvector = AFF_STOPLEFT;
                af[0].duration  = pc_duration(victim,4,0,0,0,0);				
                }
             else
                 to_char = "раздробило $D";
                to_vict = "раздробило Вам";
                af[0].type      = SPELL_BATTLE;
                af[0].bitvector = AFF_STOPFIGHT;
                af[0].duration  = pc_duration(victim,4,0,0,0,0);				
                }
           haemorragia(victim,90);
		}
   else //негуманоид
	{	if (dam_critic < 1)
			{GET_POS(victim) = POS_SITTING;
			WAIT_STATE(victim, 2*PULSE_VIOLENCE);
			to_char = "повредило $D";
			to_vict = "повредило Вам";
			}                   
		else
		if (dam_critic < 8)
			{dam *= MAX(2,(uses_skill-40)/15);
			to_char = "изуродовало $D";
			to_vict = "изуродовало Вам";
			af[0].type      = SPELL_BATTLE;
			af[0].bitvector = AFF_NOFLEE;
			SET_AF_BATTLE(victim, EAF_SLOW);
			GET_POS(victim) = POS_SITTING;
			WAIT_STATE(victim, 2*PULSE_VIOLENCE);
			}           
		else
		if (dam_critic < 20)
			{dam *= MAX(2,(uses_skill-40)/15);
			to_char = "сломало $D";
			to_vict = "сломало Вам";
			af[0].type      = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration  = pc_duration(victim,2,0,0,0,0);
			af[0].battleflag= 1;
			af[1].type      = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim,60);
			SET_AF_BATTLE(victim, EAF_SLOW);
			GET_POS(victim) = POS_SITTING;
			}
		else   
			{dam *= MAX(2,(uses_skill-40)/15);
			to_char = "оторвало $D";
			to_vict = "оторвало Вам";
			af[0].type      = SPELL_BATTLE;
			af[0].bitvector = AFF_STOPFIGHT;
			af[0].duration  = pc_duration(victim,3,0,0,0,0);	
			af[1].type      = SPELL_BATTLE;
			af[1].bitvector = AFF_NOFLEE;
			haemorragia(victim,90);
			SET_AF_BATTLE(victim, EAF_SLOW);
			GET_POS(victim) = POS_SITTING;
			}
   }
   break;
   case WEAR_LEGS://нога-лапа
   if (dam_critic < 1)
   {GET_POS(victim) = POS_SITTING;
    WAIT_STATE(victim, 2*PULSE_VIOLENCE);
    to_char = "повредило $D";
    to_vict = "повредило Вам";
   }                   
   else
   if (dam_critic < 8)
   {dam *= MAX(2,(uses_skill-40)/15);
    to_char = "изуродовало $D";
    to_vict = "изуродовало Вам";
    af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_NOFLEE;
    SET_AF_BATTLE(victim, EAF_SLOW);
	GET_POS(victim) = POS_SITTING;
    WAIT_STATE(victim, 2*PULSE_VIOLENCE);
   }           
   else
   if (dam_critic < 20)
   {dam *= MAX(2,(uses_skill-40)/15);
	to_char = "сломало $D";
	to_vict = "сломало Вам";
    af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_STOPFIGHT;
    af[0].duration  = pc_duration(victim,2,0,0,0,0);
	af[0].battleflag= 1;
    af[1].type      = SPELL_BATTLE;
    af[1].bitvector = AFF_NOFLEE;
    haemorragia(victim,60);
    SET_AF_BATTLE(victim, EAF_SLOW);
	GET_POS(victim) = POS_SITTING;
    }
   else   
   {dam *= MAX(2,(uses_skill-40)/15);
	to_char = "оторвало $D";
	to_vict = "оторвало Вам";
    af[0].type      = SPELL_BATTLE;
    af[0].bitvector = AFF_STOPFIGHT;
    af[0].duration  = pc_duration(victim,3,0,0,0,0);	
    af[1].type      = SPELL_BATTLE;
    af[1].bitvector = AFF_NOFLEE;
    haemorragia(victim,90);
    SET_AF_BATTLE(victim, EAF_SLOW);
	GET_POS(victim) = POS_SITTING;
	}
   break;
   }
   --position;

   
  for (i = 0; i < 2; i++)
     if (af[i].type)
        affect_join(victim, af+i, TRUE, FALSE, TRUE, FALSE);
  
 if (position >= 0)
  {  if (to_char)
    {sprintf(buf,"&GВаше критическое попадание %s%s&n",
             to_char, ConPos[position][rases][conpos]);
     act(buf, FALSE, ch, 0, victim, TO_CHAR);
    }
 if (to_vict)
    {sprintf(buf,"&RКритическое попадание $r %s%s&n",
             to_vict, ConPos[position][rases][conpos]);
     act(buf, FALSE, ch, 0, victim, TO_VICT);
   
    }
 if (to_vict && to_char)
	{sprintf(buf,"&GКритическое попадание $r %s%s&n", to_char, ConPos[position][rases][conpos]);
	 act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
	}
}

 else
 { dam *= MAX(2,(uses_skill-40)/11);
   sprintf(buf,"&GВаше критическое попадание нанесло повреждение $D.&n");
   act(buf, FALSE, ch, 0, victim, TO_CHAR);
   sprintf(buf,"&RКритическое попадание $r нанесло повреждение $D.&n");
   act(buf, FALSE, ch, 0, victim, TO_VICT);
   sprintf(buf,"&GКритическое попадание $r нанесло повреждение $D.&n");
   act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
  }

 
if (unequip_pos && GET_EQ(victim,unequip_pos))
    {obj = unequip_char(victim,unequip_pos);
     if (!IS_NPC(victim) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
	  obj_to_char(obj,victim);
	  else
        obj_to_room(obj,IN_ROOM(victim));
	   act("$n не смог$q удержать $3.", FALSE, victim, obj, 0, TO_ROOM); 
	   act("Вы не смогли удержать $3.", FALSE, victim, obj, 0, TO_CHAR);
           obj_decay(obj);
    }

 return dam;
}


/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.           (жертва мертва)
 *	= 0	No damage.            (нет повреждений)
 *	> 0	How much damage done. (сколько нанесено повреждений)
 */

void char_dam_message(int dam, struct char_data *ch, struct char_data *victim, int attacktype, int mayflee)
{ 
  //int clan_num;
	switch (GET_POS(victim))//разобраться со склонением в множественном числе...
  {case POS_MORTALLYW:
     sprintf (buf, "$n смертельно ранен%s и умр%s, если $m не помогут.", GET_CH_SUF_6(victim), GET_CH_SUF_6(victim)=="ы"? "ут": "ет");
    act(buf, TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("Вы смертельно ранены и умрете, если Вам не помогут.\r\n", victim);
    break;
   case POS_INCAP:
    sprintf(buf, "$n без сознания и медленно умира%s.", GET_CH_SUF_6(victim)=="ы"? "ют": "ет");
	act(buf, TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("Вы без сознания и медленно умираете, истекая кровью без помощи.\r\n", victim);
    break;
   case POS_STUNNED:
    act("$n без сознания, но у н$s еще есть возможность прийти в себя.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("Вы находитесь без сознания, но еще можете прийти в себя самостоятельно.\r\n", victim);
    break;
   case POS_DEAD:
          if (!IS_NPC(victim))
	    { if (IS_NPC(ch) &&  AFF_FLAGGED(ch, AFF_CHARM))
	          check_killer(ch->master, victim);
	      else
                  check_killer(ch, victim);

	     if (!IS_NPC(ch) || IS_NPC(ch) &&  AFF_FLAGGED(ch, AFF_CHARM))
		{ REMOVE_BIT(PLR_FLAGS(victim, PLR_KILLER), PLR_KILLER);
		  GET_TIME_KILL(victim) = 0;
		}	                    	
	    }
		if (IN_ROOM(victim) != NOWHERE)
	         death_cry(victim);

             
		sprintf(buf, "$n мертв%s. R.I.P.", GET_CH_SUF_6(victim));
		act(buf, FALSE, victim, 0, 0, TO_ROOM);
                send_to_char("Вас убили. R.I.P.\r\n", victim);

    if (!IS_NPC(victim) && victim != ch && !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA)) 
	{  GET_NEXTQUEST(victim)= 0;
	   victim->player_specials->saved.spare12++;
	   GET_MAX_HIT(victim) -= number(0,1);
        }

   if (!IS_NPC(victim) && victim != ch  && !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
	   ch->player_specials->saved.spare9++;		
   break;

   default:			// >= POSITION SLEEPING 
    if (dam > (GET_REAL_MAX_HIT(victim) / 4))
        send_to_char("Это действительно БОЛЬНО!\r\n", victim);

    if (GET_POS(victim) != POS_DEAD && dam > 0 && GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4))
       {sprintf(buf2, "%sВы желаете, чтобы Ваши раны не кровоточили так сильно! %s\r\n",
	            CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
        send_to_char(buf2, victim);
       }
    if (ch != victim    &&
        IS_NPC(victim) &&
        GET_HIT(victim) < (GET_REAL_MAX_HIT(victim) / 4)  &&
        MOB_FLAGGED(victim, MOB_WIMPY)                    &&
	mayflee                                           &&
        GET_POS(victim) > POS_SITTING
       )
	{  do_flee(victim, NULL, 0, 0);
	  
	}
    if (ch != victim          &&
        !IS_NPC(victim)       &&
	     HERE(victim)         &&
         GET_WIMP_LEV(victim) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim)            &&
	mayflee                                           &&
	GET_POS(victim) > POS_SITTING
       )
       {send_to_char("Вы запаниковали и попытались убежать!\r\n", victim);
        do_flee(victim, NULL, 0, 0);
       }
    break;
   }
}


//функция для проверки на выпонение квеста.

void Quest_Ap(struct char_data * ch, struct char_data * victim) 
{ struct char_data *k, *tch;
  struct follow_type *f;
  k = (ch->master ? ch->master : ch);//если лидер,то к -  лидер,
									//если не лидер, то к ==сh

     if (GET_ID(victim) == GET_QUESTMOB(ch))  
			{ GET_QUESTMOB(ch) = 0;	
			  GET_QUESTOK(ch) = 1; 
			  sprintf (buf, "&CВы выполнили задание, осталось только успеть доложить!&n\r\n");
			  send_to_char (buf, ch);
			  return;
        	}
     //внизу пропускаю либо я лидер и в группе, либо моба (может быть абьюз, если кто со
	 //своим чармистом придет и заставит убить квест, он засчитается на того, кто его получил
	 //совершенно пофиг если этот чармист не того, у кого квест) пока оставляю, если
	 //кто найдет, еще поставлю дополнительную проверку на пренадлженость следования чармиста за хозяином.
	 //щас думать ламы, хотя фигли думать, к-фолловер = лидеру.
	  if((AFF_FLAGGED(k, AFF_GROUP) || IS_NPC(ch)) && (k != ch) &&
		     GET_ID(victim) == GET_QUESTMOB(k))
	  {   if ((IN_ROOM(ch)== IN_ROOM(k))         &&
	         ((GET_LEVEL(k) - GET_LEVEL(ch) <= 3 &&
		      GET_LEVEL(k) - GET_LEVEL(ch) >= - 3) || IS_NPC(ch)))
			{ GET_QUESTMOB(k) = 0;
			  GET_QUESTOK(k) = 1;
			  sprintf(buf, "&CВаш согрупник помог Вам выполнить задание!&n\r\n");
		      send_to_char(buf,k);
              sprintf(buf, "&CВы помогли согрупнику выполнить задание!&n\r\n");
		      send_to_char(buf,ch);
			  return;
	  		}
		 else		 		 //помешали выполнить квест
			{ REMOVE_BIT(k->char_specials.saved.Act.flags[0], PLR_QUESTOR);
			  GET_QUESTMOB(k) = 0;
			  GET_COUNTDOWN(k)= 0;
			  GET_NEXTQUEST(k)= 0; //после помехи время на получение следующего
			  sprintf(buf, "&GВам помешали выполнить задание!&n\r\n");
			  send_to_char (buf, k);
			  sprintf (buf, "&CВы помешали выполнить задание %s!&n\r\n", GET_DNAME(k));
			  send_to_char (buf, ch);
			  return;
			}
		
		}
	  for (f = k->followers; f; f = f->next)
	    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch &&
		  GET_ID(victim) == GET_QUESTMOB(f->follower))  
		if((IN_ROOM(ch)== IN_ROOM(f->follower))         && 
		  (GET_LEVEL(f->follower) - GET_LEVEL(ch)) <= 3 &&
		   GET_LEVEL(f->follower) - GET_LEVEL(ch) >= - 3 )
		{  GET_QUESTMOB(f->follower) = 0;	
		   GET_QUESTOK(f->follower) = 1;
		   sprintf(buf, "&CВаш согрупник помог Вам выполнить задание!&n\r\n");
		   send_to_char(buf,f->follower);
           sprintf(buf, "&CВы помогли согрупнику выполнить задание!&n\r\n");
		   send_to_char(buf,ch);
		    return;
		}
	     else
		 		 //помешали выполнить квест
			{ REMOVE_BIT(f->follower->char_specials.saved.Act.flags[0], PLR_QUESTOR);
			  GET_QUESTMOB(f->follower) = 0;
			  GET_COUNTDOWN(f->follower)= 0;
			  GET_NEXTQUEST(f->follower)= 0; //после помехи время на получение следующего
			  sprintf(buf, "&GВам помешали выполнить задание!&n\r\n");
			  send_to_char (buf, f->follower);
			  sprintf (buf, "&CВы помешали выполнить задание %s!&n\r\n", GET_DNAME(f->follower));
			  send_to_char (buf, ch);
			  return;
			}
	  

  
	   if (tch = get_char_cmp_id(GET_ID(victim)))
			 //помешали выполнить квест
			{ REMOVE_BIT(tch->char_specials.saved.Act.flags[0], PLR_QUESTOR);
			  GET_QUESTMOB(tch) = 0;
			  GET_COUNTDOWN(tch)= 0;
			  GET_NEXTQUEST(tch)= 1; //после помехи время на получение следующего
			  sprintf(buf, "&GВам кто-то помешал выполнить задание!&n\r\n");
			  send_to_char (buf, tch);
			  sprintf (buf, "&CВы помешали кому-то выполнить задание!&n\r\n");
			  send_to_char (buf, ch);
			}
            
}


int mindamage;
int armour_absorbe(struct char_data * ch, struct char_data * victim, obj_data *weapon, int dam, bool too_shit)
{
int stat = 0, chance, decr = 100, decrease = 0,ac = 0, arm_pos;
struct obj_data *armour = 0;

	mindamage = dam / 5; 

    if (weapon)
          stat = 110 - (GET_SKILL(ch,GET_OBJ_SKILL(weapon))/3);
    else
	{ if (!too_shit)//бьем правой рукой, 
           stat = 110 - (GET_SKILL(ch, SKILL_PUNCH)/3);
      else
		   stat = 110 - (GET_SKILL(ch, SKILL_SHIT)/3);//бьем ударом левой руки
	}
  
  stat -= dex_app[GET_REAL_DEX(ch)].miss_att*4;
  
  if (GET_REAL_DEX(victim) > GET_REAL_SIZE(victim))
      stat += dex_app[GET_REAL_DEX(victim)].reaction;
  else
      stat += size_app[GET_REAL_SIZE(victim)].ac;
  
  
  

  if (GET_EQ(victim, WEAR_SHIELD) && GET_OBJ_TYPE(GET_EQ(victim, WEAR_SHIELD)) == ITEM_ARMOR)
  {chance = -dex_app[GET_REAL_DEX(victim)].defensive;
   chance += GET_SKILL(victim, SKILL_SHIELD_MASTERY) / 2;
	if (chance > number(1,100))
	{  decrease += MIN(40,GET_OBJ_VAL(GET_EQ(victim, WEAR_SHIELD),1))* 
       GET_OBJ_CUR(GET_EQ(victim, WEAR_SHIELD))/GET_OBJ_MAX(GET_EQ(victim, WEAR_SHIELD));  
		if (MIN(25,GET_OBJ_VAL(GET_EQ(victim,WEAR_SHIELD ),0)) > number(0,99))
		{	act("Вы ловко подставили щит под удар $r.",FALSE,ch,0,victim,TO_VICT);
			act("$N ловко подставил свой щит под Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
			act("$N ловко подставил свой щит под удар $r.",TRUE,ch,0,victim,TO_NOTVICT);
			alterate_object(GET_EQ(victim, WEAR_SHIELD), dam*3, 100);
			was_point = TRUE;		
			return (0);
		}	   
	    alterate_object(GET_EQ(victim, WEAR_SHIELD), dam, 50);	  
	}
  }

	chance = number(0,MAX(0, stat));

  if (chance < 30)
   arm_pos = WEAR_BODY;//тело  
  else
  if (chance < 43)
  {arm_pos = WEAR_LEGS;dam = dam * 9 / 10;}//ноги  
  else
  if (chance < 53)
  {arm_pos = WEAR_HANDS;dam = dam * 8 / 10;}//руки
  else
  if (chance < 56)
  {arm_pos = WEAR_NECK_1;dam = dam * 12 / 10;}//шея
  else  
  if (chance < 64)
  arm_pos = WEAR_HEAD;//голова
  else
  if (chance < 65)
  {arm_pos = WEAR_EYES;dam = dam * 15 / 10;}//глаза
  else
  if (chance < 71)
  {arm_pos = WEAR_ABOUT;dam = dam * 9 / 10;}//плечи
  else
  if (chance < 76)
  {arm_pos = WEAR_WAIST;;dam = dam * 11 / 10;}//пояс
  else
  if (chance < 83)
  arm_pos = WEAR_NECK_2;//грудь
  else
  if (chance < 87)
  {arm_pos = WEAR_FEET;dam = dam * 7 / 10;}//ступни
  else  
  if (chance < 93)
  {arm_pos = WEAR_BACKPACK;dam = dam * 7 / 10;}//за спионой
  else  
  if (chance < 96)
  {arm_pos = WEAR_ARMS;dam = dam * 5 / 10;}//кисти
  else
  if (chance < 99)
  {arm_pos = WEAR_WRIST_R;dam = dam * 5 / 10;}//правое запястье
  else
  if (chance < 102)
  {arm_pos = WEAR_WRIST_L;dam = dam * 5 / 10;}//правое запястье
  else  
  if (chance < 104)
  {arm_pos = WEAR_FINGER_R;dam = dam * 4 / 10;}//правый палец
  else
  if (chance < 106)
  {arm_pos = WEAR_FINGER_L;dam = dam * 4 / 10;}//левый палец
  else
  if (chance < 108)
  {arm_pos = WEAR_EAR_R;dam = dam * 3 / 10;}//левое ухо
  else  
  {arm_pos = WEAR_EAR_L;dam = dam * 3 / 10;}//правое ухо
  
  

  if (!dam)
	  return 0;

  GET_CONPOS(ch) = arm_pos-1;

  if (GET_EQ(victim, arm_pos)) 
  {   alterate_object(GET_EQ(victim, arm_pos), dam, 50);	  
    if (GET_EQ(victim, arm_pos) && GET_OBJ_TYPE(GET_EQ(victim, arm_pos)) == ITEM_ARMOR)
   {   armour = GET_EQ(victim, arm_pos); 
	   decrease += GET_OBJ_VAL(GET_EQ(victim, arm_pos),1) *
	               GET_OBJ_CUR(GET_EQ(victim, arm_pos))/GET_OBJ_MAX(GET_EQ(victim, arm_pos));
       		  ac = MIN(50,GET_OBJ_VAL(GET_EQ(victim, arm_pos),0))*
	               GET_OBJ_CUR(GET_EQ(victim, arm_pos))/GET_OBJ_MAX(GET_EQ(victim, arm_pos));
	}  
  
  if (ac > number(0,99))
     {   act("Ваши доспехи отразили удар $r.",FALSE,ch,0,victim,TO_VICT);
        act("Доспехи $R отразили Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
        act("Доспехи $R отразили удар $r.",TRUE,ch,0,victim,TO_NOTVICT);
		was_point = TRUE;
		alterate_object(GET_EQ(victim, arm_pos), dam*2, 100);
		return (0);
	 }
  }
  stat = 0;
  stat -= dex_app[GET_REAL_DEX(victim)].reaction;
  if (armour)
  {
  stat -= (1000 /(105 - GET_OBJ_MATER(armour)) * GET_OBJ_TIMER(armour) / GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(armour)));  
  stat += (GET_OBJ_MAX(armour) - GET_OBJ_CUR(armour) - 20);	
  }
  if (weapon)    
  stat += weapon_app[GET_OBJ_WEIGHT(weapon)].shocking * weapon_app[GET_OBJ_WEIGHT(weapon)].shocking / 10;

	if (weapon)
    switch (GET_OBJ_SKILL(weapon))
	{case SKILL_CLUBS: //дубины
	stat += str_app[GET_REAL_STR(ch)].tohit;
		chance = 31;
		decr = number(30,60);
	break;
	case SKILL_BOTHHANDS: //двуручи
		stat += str_app[GET_REAL_STR(ch)].tohit;
		chance = 15;
		decr = number(5,30);
	break;
	case SKILL_AXES: //топоры
		stat += str_app[GET_REAL_STR(ch)].tohit;
		chance = 10;
		decr = number(8,25);
	break;
	case SKILL_LONGS: //длинные
		stat += str_app[GET_REAL_STR(ch)].tohit / 2 + dex_app[GET_REAL_DEX(ch)].reaction / 2;
		chance = 13;
		decr = number(15,40);
	break;
    case SKILL_SPADES:// копья
		stat += dex_app[GET_REAL_DEX(ch)].reaction;
		chance = 9;
		decr = number(3,25);
	break;
	case SKILL_SHORTS:// короткие
		stat += str_app[GET_REAL_STR(ch)].tohit / 2 + dex_app[GET_REAL_DEX(ch)].reaction / 2;
		chance = 6;
		decr = number(20,50);
	break;
    case SKILL_BOWS: // луки
		stat += dex_app[GET_REAL_DEX(ch)].reaction;
		chance = 5;
		decr = number(25,60);
	break;
	case SKILL_PICK:// проники
	    stat += dex_app[GET_REAL_DEX(ch)].reaction;
		chance = 4;
		decr = number(1,10);
	break;
	default:
		stat += str_app[GET_REAL_STR(ch)].tohit * 2 / 3;
		chance = 3;
		decr = number(50,75);
	break;
	}
	else
	switch (ch->mob_specials.attack_type)
   {case 5: // огрели
		stat += str_app[GET_REAL_STR(ch)].tohit;
		chance = 31;
		decr = number(30,60);
	break;
	case 6: // сокрушили 
		stat += str_app[GET_REAL_STR(ch)].tohit;
		chance = 15;
		decr = number(5,40);
	break;
	case 3: // рубанули
		stat += str_app[GET_REAL_STR(ch)].tohit;
		chance = 10;
		decr = number(10,25);
	break;
	case 7: // резанули
		stat += str_app[GET_REAL_STR(ch)].tohit / 2 + dex_app[GET_REAL_DEX(ch)].reaction / 2;
		chance = 13;
		decr = number(15,40);
	break;
	case 12:// ткнули
		stat += dex_app[GET_REAL_DEX(ch)].reaction;
		chance = 9;
		decr = number(3,25);
	break;
	case 10:// пырнули 
		stat += str_app[GET_REAL_STR(ch)].tohit / 2 + dex_app[GET_REAL_DEX(ch)].reaction / 2;
		chance = 6;
		decr = number(20,50);
	break;
	case 9: // подстрелили 
		stat += dex_app[GET_REAL_DEX(ch)].reaction;
		chance = 5;
		decr = number(30,60);
	break;
	case 11:// укололи 
	case 2: // ужалили
		stat += dex_app[GET_REAL_DEX(ch)].reaction;
		chance = 4;
		decr = number(1,10);
	break;
	case 13:// лягнули
	case 14:// боднули 
		stat += str_app[GET_REAL_STR(ch)].tohit + 10;
		chance = 8;
		decr = number(20,60);
	break;
	case 16:// ободрали 
		stat += str_app[GET_REAL_STR(ch)].tohit + 25;
		chance = 11;
		decr = number(15,50);
	break;
	default:
		stat += str_app[GET_REAL_STR(ch)].tohit * 2 / 3;
		chance = 3;
		decr = number(40,70);
	break;
	}

   
   if ((chance < number(1,100)) || (stat < 1) || 
	  (number(0,stat) < number(0,cha_app[GET_REAL_CHA(victim)].morale + GET_REAL_CHA(victim))) )
       decr = 100;

 decrease = MIN(decrease,decr);

              if (decrease >= number(dam, dam * 50))
                 {act("Ваши доспехи полностью поглотили удар $r.",FALSE,ch,0,victim,TO_VICT);
                  act("Доспехи $R полностью поглотили Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
                  act("Доспехи $R полностью поглотили удар $r.",TRUE,ch,0,victim,TO_NOTVICT);
				   was_point = TRUE;
                  return (0);
                 }
			 dam -=(dam * MIN(99,decrease) / 100);//либо доубле сделать - а то инт пофиг. не работает щас
    
 return (dam);
}
 
int damage(struct char_data * ch, struct char_data * victim, int dam, int attacktype, int mayflee)
{ int FS_damage = 0; int decrease = 0;

 

  if (!ch || !victim)
     return (0);
     


  if (IN_ROOM(victim) == NOWHERE || IN_ROOM(ch) == NOWHERE ||
      IN_ROOM(ch) != IN_ROOM(victim)
     )
     {log("SYSERR: Attempt to damage '%s' in room NOWHERE by '%s'.",
          GET_NAME(victim), GET_NAME(ch));
      return (0);
     }

  if (GET_POS(victim) <= POS_DEAD)
     {log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		  GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
      die(victim,NULL);
      return (0);			/* -je, 7/7/92 */
     }

  //
  if (dam >=0 && damage_mtrigger(ch,victim))
      return (0);
     

  // Shopkeeper protection
  if (!ok_damage_shopkeeper(ch, victim))
     return (0);
     
  
  //запрещение боевых действий Имму
  if (PLR_FLAGGED(ch, PLR_IMMKILL) || PLR_FLAGGED(victim, PLR_IMMKILL))
	  return false;  
  
  // No fight mobiles
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
       return (0);
     

  if (IS_NPC(victim) && MOB_FLAGGED(ch, MOB_NOFIGHT))
     {act("Вы не можете причинить вред $D.",FALSE,ch,0,victim,TO_CHAR);
      return (0);
     }

  // You can't damage an immortal!
  if (IS_GOD(victim))
     dam = 0;
  else
  if (IS_IMMORTAL(victim) ||
      GET_GOD_FLAG(victim,GF_GODSLIKE))
     dam /= 4;
  else
  if (GET_GOD_FLAG(victim,GF_GODSCURSE))
     dam *= 2;

 
  	if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MEMORY)) {
		if (!IS_NPC(ch))
			remember(victim, ch);
		else if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !IS_NPC(ch->master)) {
			if (MOB_FLAGGED(ch, MOB_CLONE))
				remember(victim, ch->master);
			else if (IN_ROOM(ch->master) == IN_ROOM(victim) && CAN_SEE(victim, ch->master))
				remember(victim, ch->master);
		}
	}

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MEMORY)) {
		if (!IS_NPC(victim))
			remember(ch, victim);
		else if (AFF_FLAGGED(victim, AFF_CHARM) && victim->master && !IS_NPC(victim->master)) {
			if (MOB_FLAGGED(victim, MOB_CLONE))
				remember(ch, victim->master);
			else if (IN_ROOM(victim->master) == IN_ROOM(ch) && CAN_SEE(ch, victim->master))
				remember(ch, victim->master);
		}
	}


  //*************** If the attacker is invisible, he becomes visible
  appear(ch);
  appear(victim);

     //**************** If you attack a pet, it hates your guts 

  if ( !same_group( ch, victim ) )
  {
    // атака не в группе
    if ( victim->master == ch )
    {
      // лидер атакует своего последователя - выгнать последователя
      if (stop_follower(victim, SF_EMPTY))
        return (-1);
    }
    else
    if ( ch->master == victim )
    {
      // последователь атакует своего лидера - выгнать последователя
      if (stop_follower(ch, SF_EMPTY))
        return (-1);
    }
    else
    if ( ch->master && ch->master == victim->master )
    {
      // один последователь атакует другого последователя
      // выгнать того, кто не в группе
      if (stop_follower(AFF_FLAGGED(victim, AFF_GROUP)?ch:victim, SF_EMPTY))
        return (-1);
    }
  } 


  if (victim != ch)
    {//**************** Start the attacker fighting the victim
     if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
		{ pk_agro_action(ch,victim); 	
		  set_fighting(ch, victim);
                  npc_groupbattle(ch);
                }


  //**************** If you attack a pet, it hates your guts  
 
     if (victim->master == ch)
         {if (stop_follower(victim, SF_EMPTY))
             return (-1);
         }
      else
      if (ch->master == victim || victim->master == ch ||
          (ch->master && ch->master == victim->master))
        { if (stop_follower(ch, SF_EMPTY))
            return (-1);
        }
      

 
	 //***************** Start the victim fighting the attacker
     if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL))
        {set_fighting(victim, ch);
         npc_groupbattle(victim);
        }
    }

 

  


  //*************** If negative damage - return
  if (dam < 0                    ||
      IN_ROOM(ch) == NOWHERE     ||
      IN_ROOM(victim) == NOWHERE ||
      IN_ROOM(ch) != IN_ROOM(victim)
     )
     return (0);



//  check_killer(ch, victim);
  if (victim != ch)
     {if (dam && AFF_FLAGGED(victim, AFF_SHIELD))
         {act("Магический кокон полностью поглотил удар $R.",FALSE,victim,0,ch,TO_CHAR);
          act("Магический кокон вокруг $R полностью поглотил Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
          act("Магический кокон вокруг $R полностью поглотил удар $r.",TRUE,ch,0,victim,TO_NOTVICT);
          return (0);
         }

      if (dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_FIRESHIELD))
         {FS_damage = dam * 20 / 100;
	  dam      -= (dam * number(10,30) / 100);
	 }
	
      if (dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_ICESHIELD))
     	 {act("Ледяной щит принял часть удара на себя.",FALSE,ch,0,victim,TO_VICT);
          act("Ледяной щит вокруг $R смягчил Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
          act("Ледяной щит вокруг $R смягчил удар $r.",TRUE,ch,0,victim,TO_NOTVICT);
	  dam -= (dam * number(30,50) / 100);
	 }

      if (dam > 0 && !was_critic && AFF_FLAGGED(victim, AFF_AIRSHIELD))
     	 {act("Серебристый щит смягчил удар $r.",FALSE,ch,0,victim,TO_VICT);
          act("Серебристый щит вокруг $R ослабил Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
          act("Серебристый щит вокруг $R ослабил удар $r.",TRUE,ch,0,victim,TO_NOTVICT);
	  dam -= (dam * number(70,80) / 100);
	 }
     
	
       if (dam < 1) 
     mindamage = dam; 

	

           if (dam > 0 && GET_ABSORBE(victim) > 0 && IS_WEAPON(attacktype))
	  { int mdamag = number(GET_ABSORBE(victim)/2, GET_ABSORBE(victim));
            if (AFF_FLAGGED(victim, AFF_SANCTUARY))
            dam *= 2;
	      
		  if (GET_SKILL(ch, SKILL_BOTH_WEAPON) && GET_EQ(ch, WEAR_BOTHS))
			dam += mdamag - 1;

      if (dam <= mdamag && !mindamage)
		{	act("Защитная магия отразила удар $r.",FALSE,ch,0,victim,TO_VICT);
			act("Защитная магия $R отразила Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
			act("Защитная магия $R отразила удар $r.",TRUE,ch,0,victim,TO_NOTVICT);
			return (0);   
		}
			dam -=mdamag;
	  	  

	  if (affected_by_spell(victim, SPELL_ARMOR))  
		{ act("Магический щит смягчил удар $r.",FALSE,ch,0,victim,TO_VICT);
          act("Магический щит вокруг $R ослабил Ваш удар.",FALSE,ch,0,victim,TO_CHAR);
          act("Магический щит вокруг $R ослабил удар $r.",TRUE,ch,0,victim,TO_NOTVICT);
	      
		}
	  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
            dam /= 2;
	  }

	  if (dam && IS_WEAPON(attacktype))
         alt_equip(victim,NOWHERE,dam,50);


     }
 /* else
  
  if (MOB_FLAGGED(victim, MOB_PROTECT))
      return false; */
     

  //*************** Set the maximum damage per round and subtract the hit points
  if (MOB_FLAGGED(victim, MOB_PROTECT))
     {act("$n находится под защитой Богов.",FALSE,victim,0,0,TO_ROOM);
      return false;
     }
  // log("[DAMAGE] Compute critic...");
  dam = MAX(dam, 0);

  if (GET_AF_BATTLE(ch, EAF_AWAKE) && !IS_NPC(ch)) 
       dam = dam/2;


  if (attacktype == SPELL_FIRE_SHIELD)
     {if ((GET_HIT(victim) -= dam) < 1)
         GET_HIT(victim) = 1;
     }
  else
     GET_HIT(victim) -= dam;

  
  if (AFF_FLAGGED(ch, AFF_PRISMATICAURA) && 
      !IS_UNDEADS(victim)		 && 
	   dam > 2			 &&
	  GET_HIT(ch) < GET_REAL_MAX_HIT(ch))
      GET_HIT(ch) += dam / 2;

    if (dam >= 0)
  dam = MAX(mindamage, dam);   
  
  //*************** Gain exp for the hit
  if (ch != victim &&
      OK_GAIN_EXP(ch, victim)				&&
     !AFF_FLAGGED(victim, AFF_CHARM)			&& 
     !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) 		&&
     !NPC_FLAGGED(victim, NPC_NOEXP) && !IS_NPC(ch))
     gain_exp(ch, IS_NPC(ch) ? GET_LEVEL(victim) * dam : (GET_LEVEL(victim) * dam + 4) / 5);
  // log("[DAMAGE] Updating pos...");
     update_pos(victim);


  // * skill_message sends a message from the messages file in lib/misc.
  //  * dam_message just sends a generic "You hit $n extremely hard.".
  // * skill_message is preferable to dam_message because it is more
  // * descriptive.
  // *
  // * If we are _not_ attacking with a weapon (i.e. a spell), always use
  // * skill_message. If we are attacking with a weapon: If this is a miss or a
  // * death blow, send a skill_message if one exists; if not, default to a
  // * dam_message. Otherwise, always send a dam_message.
  // log("[DAMAGE] Attack message...");

    if (!IS_WEAPON(attacktype) && !was_point)
     skill_message(dam, ch, victim, attacktype);
     else
    {if (GET_POS(victim) == POS_DEAD || dam == 0)
		{ if (!was_point && !skill_message(dam, ch, victim, attacktype))
          	dam_message(dam, ch, victim, attacktype);
        }
     else
		{ if (!was_point) 
		   dam_message(dam, ch, victim, attacktype);
		}
    }

  // log("[DAMAGE] Victim message...");
  //******** Use send_to_char -- act() doesn't send message if you are DEAD.
  char_dam_message(dam,ch,victim,attacktype,mayflee);
 
 
  // Проверить, что жертва все еще тут. Может уже сбежала по трусости.
  // Думаю, простой проверки достаточно.
  // Примечание, если сбежал в FIRESHIELD, 
  // то обратного повреждения по атакующему не будет

  if (IN_ROOM(ch) != IN_ROOM(victim))
     return dam;
  
  // log("[DAMAGE] Flee etc...");
  // *********** Если нападение на того кто без связи, авторекол */
  //отключено, что бы не читили.
/*  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED)
     {
      do_flee(victim, NULL, 0, 0);
      if (!FIGHTING(victim))
         {act("$n был$y спасен$y Богами.", FALSE, victim, 0, 0, TO_ROOM);
          GET_WAS_IN(victim) = victim->in_room;
          char_from_room(victim);
          char_to_room(victim, STRANGE_ROOM);
         }
       
     }*/

  // *********** Stop someone from fighting if they're stunned or worse
  if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
         stop_fighting(victim,GET_POS(victim) <= POS_DEAD);
     

  // *********** Uh oh.  Victim died.
  if (GET_POS(victim) == POS_DEAD)
     {struct char_data *killer = NULL;

      if (IS_NPC(victim) || victim->desc)
         {if (victim == ch && IN_ROOM(victim) != NOWHERE)
             {if (attacktype == SPELL_POISON)
                 {struct char_data *poisoner;
                  for (poisoner = world[IN_ROOM(victim)].people; poisoner; poisoner = poisoner->next_in_room)
                      if (poisoner != victim && GET_ID(poisoner) == victim->Poisoner)
                         killer = poisoner;
                 }
              else
              if (attacktype == TYPE_SUFFERING)
                 {struct char_data *attacker;
                  for (attacker = world[IN_ROOM(victim)].people; attacker; attacker = attacker->next_in_room)
                      if (FIGHTING(attacker) == victim)
                         killer = attacker;
                 }
             }
          if (ch != victim)
             killer = ch;
         }

       if (killer)
	  { if (AFF_FLAGGED(killer, AFF_GROUP))
	     group_gain(killer, victim);
	    else
	      if (AFF_FLAGGED(killer,AFF_CHARM) && killer->master)
	         {   if (IN_ROOM(killer) == IN_ROOM(killer->master))
			{ if (!IS_NPC(killer->master) && AFF_FLAGGED(killer->master,AFF_GROUP))
	                      group_gain(killer->master,victim);
                          else
			      perform_group_gain( killer->master, victim, 1, 100 ); //разобраться
		        }
              	   }
           else
             perform_group_gain( killer, victim, 1, 100 );
			 
          }
      
	  if (!IS_NPC(victim))
         {sprintf(buf2, "%s:  %s убит%s %s",
	          IN_ROOM(victim) != NOWHERE ? world[victim->in_room].name : "NOWHERE", GET_NAME(victim),
			  GET_SEX(victim) == SEX_FEMALE ? "а" : "", GET_TNAME(ch));
          mudlog(buf2, BRF, LVL_IMMORT, TRUE);

          if (IS_NPC(ch) && 
              (AFF_FLAGGED(ch,AFF_CHARM) || IS_HORSE(ch)) &&
               ch->master &&
               !IS_NPC(ch->master)
             )
             {sprintf(buf2, "%s подчиняется %s.", GET_NAME(ch),GET_DNAME(ch->master));
              mudlog(buf2, BRF, LVL_IMMORT, TRUE);
             }
          if (MOB_FLAGGED(ch, MOB_MEMORY))
	         forget(ch, victim);
         }
	  if (IS_NPC(victim)) // количество убитых мобов
	   { ch->player_specials->saved.spare19++;
	 	if (GET_QUESTOR(victim))
	          { Quest_Ap(ch, victim); 
	   	    GET_QUESTOR(victim) = 0;
		  } 
           }
     	
         if (killer)
	  ch=killer;
     
	  die(victim,ch);
	  return (-1);
     }
  if (FS_damage                     &&
      FIGHTING(victim)              &&
      GET_POS(victim) > POS_STUNNED &&

      IN_ROOM(victim) != NOWHERE
     )
     damage(victim,ch,FS_damage,SPELL_FIRE_SHIELD,FALSE);
  return (dam);
}



/**** This function realize second shot for bows *******/
void exthit(struct char_data * ch, int type, int weapon)
{ struct obj_data *wielded = NULL;
  int    percent=0, prob = 0;

  if (IS_NPC(ch))
     {if (MOB_FLAGGED(ch,MOB_EADECREASE) &&
          weapon > 1)
         {if (ch->mob_specials.extra_attack * GET_HIT(ch) * 2 <
              weapon                       * GET_REAL_MAX_HIT(ch))
          return;
         }
      if (MOB_FLAGGED(ch,(MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH |
                          MOB_ACIDBREATH | MOB_LIGHTBREATH)))
         {for (prob = 18, percent = -1; prob <= 22; prob++) 
			{   percent++;
		         if (MOB_FLAGGED(ch, (INT_ONE | (1 << prob))))
                     break;
			}
				 
          mag_damage(GET_LEVEL(ch),ch,FIGHTING(ch),SPELL_FIRE_BREATH+MIN(percent,4), SAVING_CRITICAL);
          return;
         }
     
  }

  if (weapon == 1)
     {if (!(wielded = GET_EQ(ch,WEAR_WIELD)))
         wielded = GET_EQ(ch,WEAR_BOTHS);
     }
  else
  if (weapon == 2)
     wielded = GET_EQ(ch,WEAR_HOLD);
  percent = number(1,skill_info[SKILL_ADDSHOT].max_percent);
  if (wielded &&
      GET_OBJ_SKILL(wielded) == SKILL_BOWS &&
      ((prob = train_skill(ch,SKILL_ADDSHOT,skill_info[SKILL_ADDSHOT].max_percent,0)) >= percent ||
       WAITLESS(ch)
      )
     )
     {hit(ch, FIGHTING(ch), type, weapon);

      if (prob > (percent * 2  - wis_app[GET_REAL_WIS(ch)].addshot)  && FIGHTING(ch))
         hit(ch, FIGHTING(ch), type, weapon);
     }
  hit(ch, FIGHTING(ch), type, weapon);
}


#define GET_HP_PERC(ch) ((int)(GET_HIT(ch) * 100 / GET_MAX_HIT(ch)))
#define POOR_DAMAGE  15
#define POOR_CASTER  5
#define MAX_PROBES   0

void hit(struct char_data * ch, struct char_data * victim, int type, int weapon)
{
  struct obj_data *wielded=NULL;
  struct char_data *vict;
  int w_type = 0, victim_ac, calc_thaco, dam, diceroll, prob, range, skill = 0,
      weapon_pos = WEAR_WIELD, percent;
  
  bool is_shit = (weapon == 2) ? 1 : 0;
  
  if (!victim)
     return;

  /* check if the character has a fight trigger */
//  fight_mtrigger(ch);

  /* Do some sanity checking, in case someone flees, etc. */
  if (ch->in_room != victim->in_room || ch->in_room == NOWHERE)
     {if (FIGHTING(ch) && FIGHTING(ch) == victim)
         stop_fighting(ch, TRUE);
      return;
     }

  /* Stand awarness mobs */
  if (CAN_SEE(victim, ch) &&
      !FIGHTING(victim)   &&
      ((IS_NPC(victim)    &&
        (GET_HIT(victim) < GET_MAX_HIT(victim) ||
         MOB_FLAGGED(victim, MOB_AWARE)
        )
       ) ||
       AFF_FLAGGED(victim, AFF_AWARNESS)
      )                     &&
      !GET_MOB_HOLD(victim) &&
      !AFF_FLAGGED(victim, AFF_HOLDALL) &&
      GET_WAIT(victim) <= 0)
     set_battle_pos(victim);

  /* Find weapon for attack number weapon */
  if (weapon == 1)
     {if (!(wielded=GET_EQ(ch,WEAR_WIELD)))
         {wielded    = GET_EQ(ch,WEAR_BOTHS);
          weapon_pos = WEAR_BOTHS;
         }
     }
  else
  if (weapon == 2)
     {wielded    = GET_EQ(ch,WEAR_HOLD);
      weapon_pos = WEAR_HOLD;
     }

  calc_thaco = 0;
  victim_ac  = 0;
  dam        = 0;

  /* Find the weapon type (for display purposes only) */
  if (type == SKILL_THROW)
     {//diceroll = 100;
      weapon   = 100;
      skill    = SKILL_THROW;
      w_type   = type + TYPE_HIT;
     }
  else
  if (type == SKILL_BACKSTAB)
     {//diceroll = 100;
      weapon   = 100;
      skill    = SKILL_BACKSTAB;
      w_type   = type + TYPE_HIT;
     }
  else
  if (type == SKILL_BLADE_VORTEX)
     {//diceroll = 100;
      weapon   = 100;
      skill    = SKILL_BLADE_VORTEX;
      w_type   = type + TYPE_HIT;
     }
  
  else//for wedmak
   if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
   {    if (GET_SKILL(ch, SKILL_SINGLE_WEAPON)			  			&&
		 !GET_EQ(ch, WEAR_HOLD)							&&
		 !GET_EQ(ch,WEAR_SHIELD)						&&
		 !GET_EQ(ch,WEAR_BOTHS))
		{ //diceroll = number(1,skill_info[SKILL_SINGLE_WEAPON].max_percent);
		  weapon   = train_skill(ch,SKILL_SINGLE_WEAPON,
		  skill_info[SKILL_SINGLE_WEAPON].max_percent,victim);
		  skill    = GET_OBJ_SKILL(wielded);
          train_skill(ch,skill,skill_info[skill].max_percent,victim);
		}
		else
		if (GET_SKILL(ch, SKILL_BOTH_WEAPON)			  &&
		    GET_EQ(ch,WEAR_BOTHS))
		{ //diceroll = number(1,skill_info[SKILL_BOTH_WEAPON].max_percent);
		  weapon   = train_skill(ch,SKILL_BOTH_WEAPON,
		  skill_info[SKILL_BOTH_WEAPON].max_percent,victim);
		  skill    = GET_OBJ_SKILL(wielded);
          train_skill(ch,skill,skill_info[skill].max_percent,victim);
		}
		else
        if (GET_SKILL(ch, SKILL_TWO_WEAPON)						  &&
		 GET_EQ(ch, WEAR_HOLD)							  &&
                 GET_EQ(ch,WEAR_WIELD))
		 {//diceroll = number(1,skill_info[SKILL_TWO_WEAPON].max_percent);
		  weapon   = train_skill(ch,SKILL_TWO_WEAPON, skill_info[SKILL_TWO_WEAPON].max_percent,victim);
		  skill    = GET_OBJ_SKILL(wielded);
          train_skill(ch,skill,skill_info[skill].max_percent,victim);
		 }
		else
		{ skill    = GET_OBJ_SKILL(wielded);
		  //diceroll = number(1,skill_info[skill].max_percent);
		  weapon   = train_skill(ch,skill,skill_info[skill].max_percent,victim);
		}
		if (GET_SKILL(ch, SKILL_SHIELD_MASTERY)	&& GET_EQ(ch,WEAR_SHIELD))
		 train_skill(ch,SKILL_SHIELD_MASTERY, skill_info[SKILL_SHIELD_MASTERY].max_percent,victim);
		if (!IS_NPC(ch))
         {// Two-handed attack - decreace TWO HANDS
          if (!is_shit &&
              GET_EQ(ch,WEAR_HOLD) &&
              GET_OBJ_TYPE(GET_EQ(ch,WEAR_HOLD)) == ITEM_WEAPON)
             {if (GET_SKILL(ch, skill) < EXPERT_WEAPON)
                 calc_thaco += 6;
              else
                 calc_thaco += 2;
             }
          else
          if (is_shit && GET_EQ(ch,WEAR_WIELD) &&
              GET_OBJ_TYPE(GET_EQ(ch,WEAR_WIELD)) == ITEM_WEAPON)
             {if (GET_SKILL(ch, skill) < EXPERT_WEAPON)
                 calc_thaco += 8;
              else
                 calc_thaco += 4;
             }

          // Apply HR for light weapon
          percent = 0;
          switch (weapon_pos)
          {case WEAR_WIELD: percent = (str_app[STRENGTH_APPLY_INDEX(ch)].wield_w  - GET_OBJ_WEIGHT(wielded) + 1) / 2;
           case WEAR_HOLD : percent = (str_app[STRENGTH_APPLY_INDEX(ch)].hold_w  - GET_OBJ_WEIGHT(wielded) + 1)  / 2;
           case WEAR_BOTHS: percent = (str_app[STRENGTH_APPLY_INDEX(ch)].wield_w +
                                       str_app[STRENGTH_APPLY_INDEX(ch)].hold_w  - GET_OBJ_WEIGHT(wielded) + 1)  / 2;
          }
          calc_thaco -= MIN(5, MAX(percent,0));

                  // Optimize weapon  
             switch((int) GET_CLASS(ch))
          {case CLASS_CLERIC:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 1; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 2; dam += 2; break;
                  case SKILL_PICK: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 0; dam += 0; break;
                }
                break;
           case CLASS_BATTLEMAGE:
               switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco += 2; dam -= 1; break;
                  case SKILL_LONGS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 2; dam += 2; break;
                  case SKILL_PICK: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 0; dam += 0; break;
                }
                break;
           case CLASS_WARRIOR:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 2; break;
                  case SKILL_SHORTS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 2; dam += 1; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 0; dam += 2; break;
                  case SKILL_PICK: 		calc_thaco += 1; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 2; dam += 3; break;
                  case SKILL_BOWS: 		calc_thaco -= 0; dam += 0; break;
                }
                break;

           case CLASS_VARVAR:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 6; dam += 4; break;
                  case SKILL_AXES: 		calc_thaco -= 6; dam += 4; break;
                  case SKILL_LONGS: 		calc_thaco -= 6; dam += 4; break;
                  case SKILL_SHORTS: 		calc_thaco -= 6; dam += 4; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 6; dam += 4; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 6; dam += 4; break;
                  case SKILL_PICK: 		calc_thaco -= 6; dam += 4; break;
                  case SKILL_SPADES: 		calc_thaco -= 6; dam += 4; break;
                  case SKILL_BOWS: 		calc_thaco -= 6; dam += 4; break;
                }
                break;
                 case CLASS_SLEDOPYT:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_SHORTS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 1; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 1; dam += 1; break;
                  case SKILL_PICK: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 4; dam += 3; break;
                }
                break;
           case CLASS_THIEF:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam -= 1; break;
                  case SKILL_LONGS: 		calc_thaco -= 2; dam += 1; break;
                  case SKILL_SHORTS: 		calc_thaco -= 4; dam += 3; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 1; dam += 1; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 1; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco -= 6; dam += 4; break;
                  case SKILL_SPADES: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_BOWS: 		calc_thaco -= 1; dam += 0; break;
                }
                break;
           case CLASS_ASSASINE:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco += 1; dam += 1; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 2; break;
                  case SKILL_SHORTS: 		calc_thaco -= 2; dam += 5; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 1; dam += 2; break;
                  case SKILL_BOTHHANDS: 	calc_thaco += 1; dam += 0; break;
                  case SKILL_PICK: 		calc_thaco -= 3; dam += 5; break;
                  case SKILL_SPADES: 		calc_thaco -= 1; dam += 4; break;
                  case SKILL_BOWS: 		calc_thaco += 1; dam -= 1; break;
                }
           case CLASS_TAMPLIER:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_SHORTS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 4; dam += 8; break;
                  case SKILL_PICK: 		calc_thaco -= 1; dam -= 1; break;
                  case SKILL_SPADES: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 1; dam -= 1; break;
                }
                break;
           case CLASS_MONAH:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_AXES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 1; dam += 1; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 1; dam += 4; break;
                  case SKILL_PICK: 		calc_thaco -= 1; dam += 1; break;
                  case SKILL_SPADES: 		calc_thaco -= 1; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 1; dam += 0; break;
                }
                break;
           case CLASS_DRUID:
                switch (skill)
                { case SKILL_CLUBS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_AXES: 		calc_thaco += 1; dam += 0; break;
                  case SKILL_LONGS: 		calc_thaco += 1; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco += 0; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco += 1; dam += 0; break;
                  case SKILL_BOTHHANDS: 	calc_thaco -= 2; dam += 2; break;
                  case SKILL_PICK: 		calc_thaco += 1; dam += 0; break;
                  case SKILL_SPADES: 		calc_thaco += 1; dam += 0; break;
                  case SKILL_BOWS: 		calc_thaco -= 1; dam += 0; break;
                }
                break;
			 case CLASS_WEDMAK://ведьмак
                switch (skill)
                { case SKILL_LONGS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_SHORTS: 		calc_thaco -= 0; dam += 0; break;
                  case SKILL_NONSTANDART: 	calc_thaco -= 0; dam += 0; break;
		  case SKILL_SINGLE_WEAPON:     calc_thaco -= 2; dam += 3; break; 	
		}
                break;

	}

          // Штраф за неизвестный тип оружия
          if (GET_SKILL(ch, skill) == 0)
             weapon   = 0;
                        

          // Бонус для эксперта в этом виде умения
          if (GET_SKILL(ch, skill) >= EXPERT_WEAPON)
             calc_thaco -= 1;
          
         } //конец обработки для ИГРОКОВ
      w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
     }        
  else
     {skill      = SKILL_PUNCH;
      weapon_pos = 0;
      //diceroll   = number(0,skill_info[skill].max_percent);
      weapon     = train_skill(ch,skill,skill_info[skill].max_percent,victim);

      if (!IS_NPC(ch))
         {// is this SHIT
          if (is_shit)
              weapon   += train_skill(ch,SKILL_SHIT,skill_info[SKILL_SHIT].max_percent,victim);
             
          // Bonus for expert PUNCH
          if (weapon >= EXPERT_WEAPON)
             calc_thaco -= 2;
          // Bonus for leadership
          if (calc_leadership(ch))
             calc_thaco -= 2;

	  if (GET_CLASS(ch)==CLASS_MONAH)
             calc_thaco -= ((GET_LEVEL(ch) - 12) / 6 + 2);
         }

      if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
         w_type  = ch->mob_specials.attack_type + TYPE_HIT;
      else
         w_type += TYPE_HIT;
     }

  // courage
  if (affected_by_spell(ch, SPELL_COURAGE)) 
  {
       train_skill(ch,SKILL_COURAGE,skill_info[SKILL_COURAGE].max_percent,victim);
          dam        += ((GET_SKILL(ch,SKILL_COURAGE)+7) / 7);    //при прокачке 70 плюс 11 дамрол
          calc_thaco += ((GET_SKILL(ch, SKILL_COURAGE) + 9) / 20);// корркция берсерка.
	   if (wielded && (weapon_pos == WEAR_BOTHS))
	      dam    += GET_SKILL(ch,SKILL_COURAGE) / 10;     
  }

  // PUNCTUAL style - decrease PC damage
 	if (GET_AF_BATTLE(ch, EAF_AWAKE) && (IS_NPC(ch) || 
		GET_CLASS(ch) != CLASS_ASSASINE) &&
		skill != SKILL_THROW &&
		skill != SKILL_BACKSTAB &&
		skill != SKILL_BLADE_VORTEX)
		{ calc_thaco += ((GET_SKILL(ch, SKILL_AWAKE) + 15) / 20) + 2;
			if (GET_SKILL(ch, SKILL_AWAKE) > 50 && !IS_NPC(ch))
				dam = dam / (GET_SKILL(ch, SKILL_AWAKE) / 50);
		}
  
     
	if (!IS_NPC(ch) && skill != SKILL_THROW && skill != SKILL_BACKSTAB && skill != SKILL_BLADE_VORTEX)
     {// Casters use weather, int and wisdom  
      if (IS_CASTER(ch))
	{ calc_thaco -= (int) ((GET_REAL_INT(ch) - 13) / GET_LEVEL(ch));
          calc_thaco -= (int) ((GET_REAL_WIS(ch) - 13) / GET_LEVEL(ch));
        }


  // Horse modifier for attacker and modifier for riding
       if (on_horse(ch))
	{ prob = train_skill(ch, SKILL_HORSE, skill_info[SKILL_HORSE].max_percent, victim);
         if (number(1, 100) > (100 - train_skill(ch, SKILL_RIDING, skill_info[SKILL_RIDING].max_percent, NULL)))
	  { if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)			 	    
	        dam += ((prob + 19) / 10);
            else
	        dam += ((prob + 19) / 20);
	
            calc_thaco -= (number(1, prob) + 19) / 20;
          }
	  else
	     { if (number(1, 10) == 5)
		{ GET_POS(ch) = POS_SITTING;
                  act("Вы упали с $R.", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		  act("$n неуклюже размахнул$u и упал$y с $R.", FALSE, ch, 0, get_horse(ch), TO_ROOM);
                  REMOVE_BIT(AFF_FLAGS(ch, AFF_HORSE), AFF_HORSE);
		  WAIT_STATE(ch, PULSE_VIOLENCE *2);
		}
		calc_thaco += (number(1, prob) + 19) / 20;
	     }
	}
   }		

  // not can see (blind, dark, etc)
  if (!CAN_SEE(ch, victim))
     calc_thaco += 4;

  if (!CAN_SEE(victim, ch))
     calc_thaco -= 4;

  // bless
  if (AFF_FLAGGED(ch, AFF_BLESS))
     calc_thaco -= 2;
     
  // curse
  if (AFF_FLAGGED(ch, AFF_CURSE))
     {calc_thaco += 4;
      dam        -= 5;
     }

  // some protects
  if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch))
     calc_thaco  += 2;
  if (AFF_FLAGGED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch))
     calc_thaco  += 2;

  // "Dirty" methods for battle
  if (skill != SKILL_THROW && skill != SKILL_BACKSTAB && skill != SKILL_BLADE_VORTEX)
      calc_thaco -= MAX(-5,(GET_SKILL(ch,skill) - GET_SKILL(victim,skill)) / 20);
     
  // AWAKE style for victim
  if (GET_AF_BATTLE(victim, EAF_AWAKE)    &&
      !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
      !GET_MOB_HOLD(victim)               &&
      !AFF_FLAGGED(victim, AFF_HOLDALL)	  &&
      train_skill(victim,SKILL_AWAKE,skill_info[SKILL_AWAKE].max_percent,ch) >=
                  number(1,skill_info[SKILL_AWAKE].max_percent))
     {prob = GET_SKILL(victim, SKILL_AWAKE);
	  dam        -= IS_NPC(ch) ? prob/10 : prob/7;
      calc_thaco += IS_NPC(ch) ? prob/15 : prob/11;
     }

      calc_thaco += size_app[GET_REAL_SIZE(ch) ? 0 : 30].ac / 3;

  // Calculate the THAC0 of the attacker
  if (!IS_NPC(ch))
     {calc_thaco += thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
      calc_thaco -= (dex_app[GET_REAL_DEX(ch)].miss_att + GET_REAL_HR(ch));
      calc_thaco -= weapon/20;
     }
  else		
     {calc_thaco += (20 - weapon/20);
      calc_thaco -= (dex_app[GET_REAL_DEX(ch)].miss_att + GET_REAL_HR(ch));
     }
       //  log("Attacker : %s", GET_NAME(ch));
       //  log("THAC0    : %d ", calc_thaco);

  // Calculate the raw armor including magic armor.  Lower AC is better.
  
  //Модификаторы попадания на луки от мудры 
  if (wielded && (weapon_pos == WEAR_BOTHS))
  { if(GET_OBJ_SKILL(wielded) == SKILL_BOWS)
	 { victim_ac +=(wis_app[GET_REAL_WIS(ch)].addshot);
	   dam += (int)(str_app[STRENGTH_APPLY_INDEX(ch)].todam / 4);
	 }
     else
       dam += str_app[STRENGTH_APPLY_INDEX(ch)].todam;
  }  
  
  victim_ac += compute_armor_class(victim);
  victim_ac /= 10;
  if (GET_POS(victim) < POS_FIGHTING)
     victim_ac += 4;
  if (GET_POS(victim) < POS_RESTING)
     victim_ac += 2;
  
  if (AFF_FLAGGED(victim, AFF_STAIRS) &&
	  victim_ac < 10)
     victim_ac = (victim_ac + 10) >> 1;


       //  log("Target : %s", GET_NAME(victim));
       //  log("AC     : %d ", victim_ac);

  // roll the die and take your chances...
  diceroll   = number(1, 20);

     
  // decide whether this is a hit or a miss
  if (( ((diceroll < 20) && AWAKE(victim)) &&
        ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac))) )
    {/* the attacker missed the victim */
     extdamage(ch, victim, 0, w_type, wielded, TRUE);
    }
  else
   {// blink
    if ((AFF_FLAGGED(victim, AFF_BLINK)
	|| (GET_CLASS(victim) == CLASS_THIEF))
	&& !GET_AF_BATTLE(ch, EAF_MIGHTHIT)
	&& !GET_AF_BATTLE(ch, EAF_STUPOR)
	&& (!(type == SKILL_BACKSTAB && GET_CLASS(ch) == CLASS_THIEF))
	&& number(1, 100) <= 20) 
	 { sprintf(buf,"%sНа мгновенье Вы исчезли из поля зрения противника.%s\r\n",
                CCNRM(victim,C_NRM),CCNRM(victim,C_NRM));
           send_to_char(buf,victim);
          extdamage(ch, victim, 0, w_type, wielded, TRUE);
	return;
       }

   // okay, we know the guy has been hit.  now calculate damage.

   // Start with the damage bonuses: the damroll and strength apply
   dam += str_app[STRENGTH_APPLY_INDEX(ch)].todam;
   dam += GET_REAL_DR(ch);

       if (IS_NPC(ch))
   	{ if (!AFF_FLAGGED(ch, AFF_CHARM))
        	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
     	  else
        	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice)/2;
   	}  

   if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
      {// Add weapon-based damage if a weapon is being wielded
       
	   percent = dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
           percent = MIN(percent, percent * GET_OBJ_CUR(wielded) /
	                         MAX(1, GET_OBJ_MAX(wielded)));

          if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
			percent /= 2;

       dam += MAX(1,percent);
      }
   else
      {// If no weapon, add bare hand damage instead
	   if (GET_EQ(ch,WEAR_ARMS))
	   { if (GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_ARMS)) < 
	         str_app[GET_REAL_STR(ch)].wield_w)
	         dam +=  GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_ARMS)) / 2;
	     else 
	         calc_thaco += (GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_ARMS)) -
        	 str_app[GET_REAL_STR(ch)].wield_w);
	   }

        if (AFF_FLAGGED(ch, AFF_STONEHAND))
	  { if (IS_MONAH(ch))
              dam += number(7, 12 + GET_LEVEL(ch)/5);
	    else
              dam += number(5, 10);
	  }
       else
	  { if (IS_MONAH(ch))
              dam += number(3, 5 + GET_LEVEL(ch)/5);
	    else
              dam += number(1, 3);
	  }	
      }

     // Skill level increase damage
          	 dam *= MAX(49, GET_SKILL(ch, skill));
		 dam /= 100;
		 dam += ((GET_SKILL(ch, skill)) / 7); 

       
   // Change victim, if protector present
   for (vict = world[IN_ROOM(victim)].people; vict && type != TYPE_NOPARRY; vict = vict->next_in_room)
       { if (PROTECTING(vict) == victim 	&&
             !AFF_FLAGGED(vict, AFF_STOPFIGHT)	&&
             GET_WAIT(vict) <= 0		&&
             !GET_MOB_HOLD(vict)		&&
	     !AFF_FLAGGED(vict, AFF_HOLDALL)	&&
             GET_POS(vict) >= POS_FIGHTING)
            {percent = number(1,skill_info[SKILL_PROTECT].max_percent);
             prob    = calculate_skill(vict,SKILL_PROTECT,skill_info[SKILL_PROTECT].max_percent,victim);
	     improove_skill(vict,SKILL_PROTECT,prob >= percent,0);
             if (WAITLESS(vict))
                percent = prob;
             if (GET_GOD_FLAG(vict, GF_GODSCURSE))
                percent = 0;
                CLR_AF_BATTLE(vict, EAF_PROTECT);
                PROTECTING(vict) = NULL;
             if (prob < percent)
                {act("Вы не смогли прикрыть $V.", FALSE, vict, 0, victim, TO_CHAR);
                 act("$N не смог$Q прикрыть Вас.", FALSE, victim, 0, vict, TO_CHAR);
                 act("$n не смог$q прикрыть $V.", TRUE, vict, 0, victim, TO_NOTVICT);
                 prob = 3;
                }
             else
                {pk_agro_action(vict,ch);
                 act("Вы героически прикрыли $V, приняв удар на себя", FALSE, vict, 0, victim, TO_CHAR);
                 act("$N героически прикрыл$Y Вас, приняв удар на себя", FALSE, victim, 0, vict, TO_CHAR);
                 act("$n героически прикрыл$y $V, приняв удар на себя", TRUE, vict, 0, victim, TO_NOTVICT);
                 victim = vict;
                 prob   = 2;
                }
                if (!WAITLESS(vict))
                WAIT_STATE(vict, 2 * PULSE_VIOLENCE);

             if (victim == vict)
                break;
            }
       }


     // Include a damage multiplier if victim isn't ready to fight:
     // Position sitting  1.5 x normal
     // Position resting  2.0 x normal
     // Position sleeping 2.5 x normal
     // Position stunned  3.0 x normal
     // Position incap    3.5 x normal
     // Position mortally 4.0 x normal
     //
     // Note, this is a hack because it depends on the particular
     // values of the POSITION_XXX constants.
     //
    if (GET_POS(ch) < POS_FIGHTING)
       dam -= (dam * (POS_FIGHTING - GET_POS(ch)) / 4);

    if (GET_POS(victim) < POS_FIGHTING)
       dam += (dam * (POS_FIGHTING - GET_POS(victim)) / 3);

    if (GET_MOB_HOLD(victim) && dam >= 2) //наносимый дамаг при параличе
       dam += dam/2;

    // Cut damage in half if victim has sanct, to a minimum 1

    if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
       dam /= 2;

    // at least 1 hp damage min per hit
	 
    dam = MAX(1, dam);
  
    
	if (weapon_pos) 
       alt_equip(ch, weapon_pos, dam, 10);
    was_critic = FALSE;
    dam_critic = 0;

    if (type == SKILL_BACKSTAB)
       {dam *= (GET_COMMSTATE(ch) ? 25 : backstab_mult(GET_LEVEL(ch)));
        /* если критбакстаб, то дамаж равен 95% хитов жертвы 
	   вероятность критстабба - стабб/20+ловкость-20 (кард) */
	if (IS_NPC(victim) && (number(1,100) < (GET_SKILL(ch, SKILL_BACKSTAB) / 20 + GET_REAL_DEX(ch) - 20)))
	{ dam = MAX(dam, GET_REAL_MAX_HIT(victim) - (GET_REAL_MAX_HIT(victim) / 20));
	  send_to_char("&GПрямо в сердце!&n\r\n",ch);
	}
        extdamage(ch, victim, dam, w_type, 0, TRUE);
        return;
       }
    else
	if (type == SKILL_BLADE_VORTEX)
       {dam *= (GET_COMMSTATE(ch) ? 10 : (calculate_skill(ch,SKILL_BLADE_VORTEX,skill_info[SKILL_BLADE_VORTEX].max_percent,victim) + 15) / 15);
        if (IS_NPC(ch))
	    dam = MIN(300,dam);
        extdamage(ch, victim, dam, w_type, 0, TRUE);
        return;
       }
    else
    if (type == SKILL_THROW)
       {dam *= (GET_COMMSTATE(ch) ? 10 : (calculate_skill(ch,SKILL_THROW,skill_info[SKILL_THROW].max_percent,victim) + 15) / 15);
        if (IS_NPC(ch))
	    dam = MIN(300,dam);
        extdamage(ch, victim, dam, w_type, 0, TRUE);
        return;
       }
    else
       {// Critical hits !!!доработать  с холдом и паралой разобраться
        if ( diceroll >= 20 - GET_LEVEL(ch)/8 + GET_LEVEL(victim)/15)
	{         percent  = cha_app[GET_REAL_CHA(ch)].morale;
		  percent += GET_MORALE(ch);
		  percent += dex_app[GET_REAL_DEX(ch)].reaction;
		  percent += str_app[GET_REAL_STR(ch)].tohit;
		  percent -= 50;

		  wielded = NULL;
		  if (weapon == 1)
		  {
			  if (!(wielded = GET_EQ(ch, WEAR_WIELD)))
			  {
				  wielded = GET_EQ(ch, WEAR_BOTHS);
				  weapon_pos = WEAR_BOTHS;
			  }
		  }
		  else
			  if (weapon == 2)
			  {
				  wielded = GET_EQ(ch, WEAR_HOLD);
				  weapon_pos = WEAR_HOLD;
			  }

           if (wielded && (weapon_pos == WEAR_BOTHS) && GET_OBJ_SKILL(wielded) != SKILL_BOWS)
              percent += (GET_OBJ_WEIGHT(wielded) - 25) * (GET_OBJ_WEIGHT(wielded) - 25);
            if (!general_savingthrow(victim, SAVING_REFLEX, percent) &&
		calc_thaco - diceroll < victim_ac - 5)
	      {   was_critic = TRUE;
               	if (wielded)
	  	   if (GET_OBJ_SKILL(wielded) != SKILL_BOWS)
			dam_critic = MAX(1,number(0,weapon_app[GET_OBJ_WEIGHT(wielded)].shocking));
		   else 
		      dam_critic = MAX(1,number(0,weapon_app[GET_OBJ_WEIGHT(wielded)].shocking) * 6 / 10);
		 else
		  dam_critic = 1 + GET_SKILL(ch,SKILL_PUNCH)/10;
                }
           }
	if (dam && was_critic)
	 {  was_point = TRUE;
           dam = compute_critical(ch,victim,dam);
       	 }
       else
	dam = armour_absorbe(ch, victim, GET_EQ(ch,weapon_pos), dam, is_shit);

		if ((GET_AF_BATTLE (ch, EAF_STUPOR) ||
		    GET_AF_BATTLE (ch, EAF_MIGHTHIT)) &&
		    GET_WAIT (ch) > 0)
		 {  CLR_AF_BATTLE (ch, EAF_STUPOR);
		    CLR_AF_BATTLE (ch, EAF_MIGHTHIT);
		 }
       
        /**** обработаем ситуацию ЗАХВАТ */
        for (vict = world[IN_ROOM(ch)].people;
             vict && dam >= 0 && type != TYPE_NOPARRY;
             vict = vict->next_in_room)
            { if (TOUCHING(vict) == ch &&
                  !AFF_FLAGGED(vict, AFF_STOPFIGHT) &&
                  !AFF_FLAGGED(vict, AFF_STOPRIGHT) &&
                  GET_WAIT(vict) <= 0				&&
                  !GET_MOB_HOLD(vict)				&&
				  !AFF_FLAGGED(vict, AFF_HOLDALL)	&&
                  (IS_IMMORTAL(vict) ||
                   IS_NPC(vict)      ||
                   GET_GOD_FLAG(vict, GF_GODSLIKE) ||
                   !(GET_EQ(vict,WEAR_WIELD)|| GET_EQ(vict,WEAR_BOTHS))
                  ) &&
                  GET_POS(vict) > POS_SLEEPING)
                 {percent = number(1,skill_info[SKILL_TOUCH].max_percent);
                  prob    = train_skill(vict,SKILL_TOUCH,skill_info[SKILL_TOUCH].max_percent,ch);
                  if (IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE))
                     percent = prob;
                  if (GET_GOD_FLAG(vict, GF_GODSCURSE))
                     percent = 0;
                  CLR_AF_BATTLE(vict, EAF_TOUCH);
                  SET_AF_BATTLE(vict, EAF_USEDRIGHT);
                  TOUCHING(vict) = NULL;
                  if (prob < percent)
                     {act("Вы не смогли перехватить атаку $R.", FALSE, vict, 0, ch, TO_CHAR);
                      act("$N не смог$Q перехватить Вашу атаку.", FALSE, ch, 0, vict, TO_CHAR);
                      act("$n не смог$q перехватить атаку $R.", TRUE, vict, 0, ch, TO_NOTVICT);
                      prob = 2;
                     }
                  else
                     {act("Вы перехватили атаку $R.", FALSE, vict, 0, ch, TO_CHAR);
                      act("$N перехватил$Y Вашу атаку.", FALSE, ch, 0, vict, TO_CHAR);
                      act("$n перехватил$y атаку $R.", TRUE, vict, 0, ch, TO_NOTVICT);
                      dam    = -1;
                      prob   = 1;
                     }
                  if (!WAITLESS(vict))
                     WAIT_STATE(vict, prob*PULSE_VIOLENCE);
                 }
            }


        /**** Обработаем команду   УКЛОНИТЬСЯ */
			 //дописать что бы при сне не уклонялись!!!
        if (dam > 0 && type != TYPE_NOPARRY	&&
            !GET_AF_BATTLE(ch, EAF_MIGHTHIT)    &&
	    !GET_AF_BATTLE(ch, EAF_STUPOR)      &&
            GET_AF_BATTLE(victim, EAF_DEVIATE)	&&
            GET_WAIT(victim) <= 0		&&
            !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
            GET_MOB_HOLD(victim) == 0		&&
			!AFF_FLAGGED(victim, AFF_HOLDALL))
           {range = number(1,skill_info[SKILL_DEVIATE].max_percent);
            prob  = train_skill(victim,SKILL_DEVIATE,skill_info[SKILL_DEVIATE].max_percent,ch);
            if (GET_GOD_FLAG(victim,GF_GODSCURSE))
               prob = 0;
            prob  = (int)(prob * 100 / range);
            if (prob < 60)
               {act("Вы не смогли уклонитьcя от атаки $R", FALSE, victim, 0, ch, TO_CHAR);
                act("$N не сумел$Y уклониться от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                act("$n не сумел$y уклониться от атаки $R", TRUE, victim, 0, ch, TO_NOTVICT);
               }
            else
            if (prob < 100)
               {act("Вы немного уклонились от атаки $R", FALSE, victim, 0, ch, TO_CHAR);
                act("$N немного уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                act("$n немного уклонил$u от атаки $R", TRUE, victim, 0, ch, TO_NOTVICT);
                dam  = (int)(dam/1.5);
               }
            else
            if (prob < 200)
               {act("Вы частично уклонились от атаки $R", FALSE, victim, 0, ch, TO_CHAR);
                act("$N частично уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                act("$n частично уклонил$u от атаки $R", TRUE, victim, 0, ch, TO_NOTVICT);
                dam  = (int)(dam/2);
               }
            else
               {act("Вы уклонились от атаки $R", FALSE, victim, 0, ch, TO_CHAR);
                act("$N уклонил$U от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                act("$n уклонил$u от атаки $R", TRUE, victim, 0, ch, TO_NOTVICT);
                dam  = -1;
               }
	            // BATTLECNTR(victim)++;
           //      if (!WAITLESS(ch))
             //    WAIT_STATE(victim, PULSE_VIOLENCE);
                CLR_AF_BATTLE(victim, EAF_DEVIATE);
           }
        else
        /**** обработаем команду  ПАРИРОВАТЬ */
        if (dam > 0 && type != TYPE_NOPARRY	&&
            !GET_AF_BATTLE(ch, EAF_MIGHTHIT)    &&
	    !GET_AF_BATTLE(ch, EAF_STUPOR)      && 
            GET_AF_BATTLE(victim, EAF_PARRY)	&&
            !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
            !AFF_FLAGGED(victim, AFF_STOPRIGHT) &&
            !AFF_FLAGGED(victim, AFF_STOPLEFT)	&&
             GET_WAIT(victim) <= 0		&&
	    !AFF_FLAGGED(victim, AFF_HOLDALL)	&&
            GET_MOB_HOLD(victim) == 0)
           {		
			if (!(
                  (GET_EQ(victim,WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(victim,WEAR_WIELD)) == ITEM_WEAPON &&
                   GET_EQ(victim,WEAR_HOLD)  && GET_OBJ_TYPE(GET_EQ(victim,WEAR_HOLD))  == ITEM_WEAPON    ) ||
                   IS_NPC(victim) ||
                   IS_IMMORTAL(victim) ||
                   GET_GOD_FLAG(victim,GF_GODSLIKE)))
               {send_to_char("У Вас нечем парировать атаку противника.\r\n",victim);
	            CLR_AF_BATTLE(victim,EAF_PARRY);
               }
        
			else// (нельзя парировать кнуты, оружие из кожи
				if (!(IS_NPC(victim)									 ||
                      IS_IMMORTAL(victim)								 ||
                      GET_GOD_FLAG(victim,GF_GODSLIKE))					 &&
					  GET_OBJ_MATER(GET_EQ(victim,WEAR_WIELD)) == MAT_SKIN)
				{ send_to_char("Этим оружием Вы не можете парировать атаки противника.\r\n",victim);
	              CLR_AF_BATTLE(victim,EAF_PARRY);
				}
			else
               {range = number(1,skill_info[SKILL_PARRY].max_percent);
                prob  = train_skill(victim,SKILL_PARRY,skill_info[SKILL_PARRY].max_percent,ch);
                prob  = (int)(prob * 100 / range);
          
		//	wielded	 
		     if (GET_EQ(ch,WEAR_WIELD) && GET_OBJ_MATER(GET_EQ(ch,WEAR_WIELD)) == MAT_SKIN)
			     prob = 0;
				if (prob < 70 ||
                    ((skill == SKILL_BOWS || w_type == TYPE_MAUL)  &&
					!IS_IMMORTAL(victim)))
                   {//act("Вы не смогли парировать $4 атаку $R", FALSE,victim,GET_EQ(victim,WEAR_WIELD),ch,TO_CHAR);
					act("Вы не смогли парировать атаку $R", FALSE,victim,0,ch,TO_CHAR);
                    act("$N не сумел$Y парировать Вашу атаку", FALSE,ch,0,victim,TO_CHAR);
                    act("$n не сумел$y парировать атаку $R", TRUE,victim,0,ch,TO_NOTVICT);
                    prob = 0;
                    SET_AF_BATTLE(victim, EAF_USEDLEFT);		
                   }
                else
                if (prob < 120)
                   {act("Вы немного парировали атаку $R",FALSE,victim,0,ch,TO_CHAR);
                    act("$N немного парировал$Y Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n немного парировал$y атаку $R",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 10);
                    prob = 0;
                    dam  = (int)(dam / 1.5);
                    SET_AF_BATTLE(victim, EAF_USEDLEFT);		
                   }
                else
                if (prob < 170)
                   {act("Вы частично парировали атаку $R",FALSE,victim,0,ch,TO_CHAR);
                    act("$N частично парировал$Y Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n частично парировал$y атаку $R",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 15);
                    prob = 0;
                    dam  = (int)(dam / 2);
                   // SET_AF_BATTLE(victim, EAF_USEDLEFT);		
                   }
                else
                   {act("Вы полностью парировали атаку $R",FALSE,victim,0,ch,TO_CHAR);
                    act("$N полностью парировал$Y Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n полностью парировал$y атаку $R",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 25);
                    prob = 0;
                    dam  = -1;
                   }
                if (!WAITLESS(ch) && prob)
                   WAIT_STATE(victim, PULSE_VIOLENCE * prob);

                   CLR_AF_BATTLE(victim, EAF_PARRY);
               }
           }
       
		else
        /**** обработаем команду  ВЕЕРНАЯ ЗАЩИТА */
        if (dam > 0 && type != TYPE_NOPARRY 		&&
            !GET_AF_BATTLE(ch, EAF_MIGHTHIT)   		&&
	    !GET_AF_BATTLE(ch, EAF_STUPOR)      	&&
            GET_AF_BATTLE(victim, EAF_MULTYPARRY) 	&&
            !AFF_FLAGGED(victim, AFF_STOPFIGHT) 	&&
            !AFF_FLAGGED(victim, AFF_STOPRIGHT) 	&&
            !AFF_FLAGGED(victim, AFF_STOPLEFT) 		&&
            BATTLECNTR(victim) < (GET_LEVEL(victim) + 4) / 5 &&
            GET_WAIT(victim) <= 0 &&
			!AFF_FLAGGED(victim, AFF_HOLDALL) &&
            GET_MOB_HOLD(victim) == 0)
           {if (!(
                  (GET_EQ(victim,WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(victim,WEAR_WIELD)) == ITEM_WEAPON &&
                   GET_EQ(victim,WEAR_HOLD)  && GET_OBJ_TYPE(GET_EQ(victim,WEAR_HOLD))  == ITEM_WEAPON    ) ||
                   IS_NPC(victim) ||
                   IS_IMMORTAL(victim) ||
                   GET_GOD_FLAG(victim,GF_GODSLIKE)))
               send_to_char("У Вас нечем отклонять атаки противников\r\n",victim);
            else
               {range = number(1,skill_info[SKILL_MULTYPARRY].max_percent) + 10 * BATTLECNTR(victim);
                prob  = train_skill(victim,SKILL_MULTYPARRY,skill_info[SKILL_MULTYPARRY].max_percent + BATTLECNTR(ch)*10,ch);
                prob  = (int)(prob * 100 / range);
                if ((skill == SKILL_BOWS || w_type == TYPE_MAUL) && !IS_IMMORTAL(victim))
                   prob = 0;
                else
                   BATTLECNTR(victim)++;

                if (prob < 50)
                   {act("Вы не смогли отбить атаку $R", FALSE,victim,0,ch,TO_CHAR);
                    act("$N не сумел$Y отбить Вашу атаку", FALSE,ch,0,victim,TO_CHAR);
                    act("$n не сумел$y отбить атаку $R", TRUE,victim,0,ch,TO_NOTVICT);
                   }
                else
                if (prob < 90)
                   {act("Вы немного отбили атаку $R",FALSE,victim,0,ch,TO_CHAR);
                    act("$N немного отбил$Y Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n немного отбил$y атаку $R",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 10);
                    dam  = (int)(dam / 1.5);
                   }
                else
                if (prob < 180)
                   {act("Вы частично отбили атаку $R",FALSE,victim,0,ch,TO_CHAR);
                    act("$N частично отбил$Y Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n частично отбил$y атаку $R",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 15);
                    dam  = (int)(dam / 2);
                   }
                else
                   {act("Вы полностью отбили атаку $R",FALSE,victim,0,ch,TO_CHAR);
                    act("$N полностью отбил$Y Вашу атаку",FALSE,ch,0,victim,TO_CHAR);
                    act("$n полностью отбил$y атаку $R",TRUE,victim,0,ch,TO_NOTVICT);
                    alt_equip(victim, number(0,2) ? WEAR_WIELD : WEAR_HOLD, dam, 25);
                    dam  = -1;
                   }
               }
           }
        else
        /**** Обработаем команду   БЛОКИРОВАТЬ */
        if (dam > 0 && type != TYPE_NOPARRY	&&
            !GET_AF_BATTLE(ch, EAF_MIGHTHIT)    &&
	    !GET_AF_BATTLE(ch, EAF_STUPOR)      &&
            GET_AF_BATTLE(victim, EAF_BLOCK)	&&
            !AFF_FLAGGED(victim, AFF_STOPFIGHT) &&
            !AFF_FLAGGED(victim, AFF_STOPLEFT)	&&
            GET_WAIT(victim) <= 0		&&
            GET_MOB_HOLD(victim) == 0		&&
	    !AFF_FLAGGED(victim, AFF_HOLDALL)	//&&
          //  BATTLECNTR(victim) < (GET_LEVEL(victim) + 8) / 9
           )
           {
            if (!(GET_EQ(victim, WEAR_SHIELD) ||
                  IS_NPC(victim)              ||
                  IS_IMMORTAL(victim)         ||
                  GET_GOD_FLAG(victim,GF_GODSLIKE)))
				{ send_to_char("Вам нечем прикрыться от атаки противника\r\n",ch);
				  CLR_AF_BATTLE(victim, EAF_BLOCK);
				}
			else
               {range = number(1,skill_info[SKILL_BLOCK].max_percent);
                prob  = train_skill(victim,SKILL_BLOCK,skill_info[SKILL_BLOCK].max_percent,ch);
                prob  = (int)(prob * 100 / range);
		      //  BATTLECNTR(victim)++;
                if (prob < 100)
                   {act("Вы не смогли прикрыться щитом от атаки $R", FALSE, victim, 0, ch, TO_CHAR);
                    act("$N не смог$Q прикрыться щитом от Вашей атаки", FALSE, ch, 0, victim, TO_CHAR);
                    act("$n не смог$q прикрыться щитом от атаки $R", TRUE, victim, 0, ch, TO_NOTVICT);
                   }
                else
                if (prob < 150)
                   {act("Вы слегка смогли блокировать атаку $R", FALSE, victim, 0, ch, TO_CHAR);
                    act("$N слегка блокировал$Y Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
                    act("$n слегка блокировал$y атаку $R", TRUE, victim, 0, ch, TO_NOTVICT);
                    alt_equip(victim,WEAR_SHIELD,dam,10);
                    dam  = (int)(dam/1.5);

                   }
                else
                if (prob < 250)
                   {act("Вы частично блокировали атаку $R", FALSE, victim, 0, ch, TO_CHAR);
                    act("$N частично блокировал$Y Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
                    act("$n частично блокировал$y атаку $R", TRUE, victim, 0, ch, TO_NOTVICT);
                    alt_equip(victim,WEAR_SHIELD,dam,15);
                    dam  = (int)(dam/2);
                   }
                else
                   {act("Вы полностью блокировали атаку $R", FALSE, victim, 0, ch, TO_CHAR);
                    act("$N полностью блокировал$Y Вашу атаку", FALSE, ch, 0, victim, TO_CHAR);
                    act("$n полностью блокировал$y атаку $R", TRUE, victim, 0, ch, TO_NOTVICT);
                    alt_equip(victim,WEAR_SHIELD,dam,25);
                    dam  = -1;
                   }
             //  if (!WAITLESS(ch) && prob)
                //   WAIT_STATE(victim, PULSE_VIOLENCE * prob);
                   CLR_AF_BATTLE(victim, EAF_BLOCK);
				}
           };

        extdamage(ch, victim, dam, w_type, wielded, TRUE);
        was_critic = FALSE;
		was_point = FALSE;
        dam_critic = 0;
       }
   }

  /* check if the victim has a hitprcnt trigger */

  hitprcnt_mtrigger(victim);
}


int GET_MAXDAMAGE(struct char_data *ch)
{
  if (AFF_FLAGGED(ch,AFF_HOLD) || AFF_FLAGGED(ch,AFF_HOLDALL))
     return 0;
  else
     return GET_DAMAGE(ch);
}

int GET_MAXCASTER(struct char_data *ch)
{ if (AFF_FLAGGED(ch,AFF_HOLD) || AFF_FLAGGED(ch,AFF_HOLDALL) || AFF_FLAGGED(ch,AFF_SIELENCE) || GET_WAIT(ch) > 0)
     return 0;
  else
     return IS_IMMORTAL(ch) ? 1 : GET_CASTER(ch);
}

int in_same_battle(struct char_data *npc, struct char_data *pc, int opponent)
{ int ch_friend_npc, ch_friend_pc, vict_friend_npc, vict_friend_pc;
  struct  char_data *ch, *vict, *npc_master, *pc_master, *ch_master, *vict_master;

  if (npc == pc)
     return (!opponent);
  if (FIGHTING(npc) == pc)    // NPC fight PC - opponent
     return (opponent);
  if (FIGHTING(pc)  == npc)   // PC fight NPC - opponent
     return (opponent);
  if (FIGHTING(npc) && FIGHTING(npc) == FIGHTING(pc))
     return (!opponent);     // Fight same victim - friend
  if (AFF_FLAGGED(pc,AFF_HORSE) || AFF_FLAGGED(pc,AFF_CHARM))
     return (opponent);

  npc_master = npc->master ? npc->master : npc;
  pc_master  = pc->master  ? pc->master : pc;

  for (ch = world[IN_ROOM(npc)].people; ch; ch = ch->next)
      {if (!FIGHTING(ch))
          continue;
       ch_master = ch->master ? ch->master : ch;
       ch_friend_npc = (ch_master == npc_master) ||
                       (IS_NPC(ch) && IS_NPC(npc) &&
                        !AFF_FLAGGED(ch, AFF_CHARM) && !AFF_FLAGGED(npc, AFF_CHARM) &&
                        !AFF_FLAGGED(ch, AFF_HORSE) && !AFF_FLAGGED(npc, AFF_HORSE)
                       );
       ch_friend_pc  = (ch_master == pc_master) ||
                       (IS_NPC(ch) && IS_NPC(pc) &&
                        !AFF_FLAGGED(ch, AFF_CHARM) && !AFF_FLAGGED(pc, AFF_CHARM) &&
                        !AFF_FLAGGED(ch, AFF_HORSE) && !AFF_FLAGGED(pc, AFF_HORSE)
                       );
       if (FIGHTING(ch) == pc &&
           ch_friend_npc)             // Friend NPC fight PC - opponent
          return (opponent);
       if (FIGHTING(pc) == ch &&
           ch_friend_npc)             // PC fight friend NPC - opponent
          return (opponent);
       if (FIGHTING(npc) == ch &&
           ch_friend_pc)              // NPC fight friend PC - opponent
          return (opponent);
       if (FIGHTING(ch)  == npc &&
           ch_friend_pc)              // Friend PC fight NPC - opponent
          return (opponent);
       vict        = FIGHTING(ch);
       vict_master = vict->master ? vict->master : vict;
       vict_friend_npc = (vict_master == npc_master) ||
                         (IS_NPC(vict) && IS_NPC(npc) &&
                          !AFF_FLAGGED(vict, AFF_CHARM) && !AFF_FLAGGED(npc, AFF_CHARM) &&
                          !AFF_FLAGGED(vict, AFF_HORSE) && !AFF_FLAGGED(npc, AFF_HORSE)
                         );
       vict_friend_pc  = (vict_master == pc_master) ||
                         (IS_NPC(vict) && IS_NPC(pc) &&
                          !AFF_FLAGGED(vict, AFF_CHARM) && !AFF_FLAGGED(pc, AFF_CHARM) &&
                          !AFF_FLAGGED(vict, AFF_HORSE) && !AFF_FLAGGED(pc, AFF_HORSE)
                         );
       if (ch_friend_npc && vict_friend_pc)
          return (opponent);          // Friend NPC fight friend PC - opponent
       if (ch_friend_pc && vict_friend_npc)
          return (opponent);          // Friend PC fight friend NPC - opponent
      }

  return (!opponent);
}

struct char_data *find_friend_cure(struct char_data *caster, int spellnum)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0, AFF_USED = 0;
 switch (spellnum)
 {case SPELL_CURE_LIGHT :
       AFF_USED = 80;
       break;
  case SPELL_CURE_SERIOUS :
       AFF_USED = 70;
       break;
  case SPELL_EXTRA_HITS :
  case SPELL_CURE_CRITIC :
  case SPELL_GROUP_CURE_CRITIC :
       AFF_USED = 50;
       break;
  case SPELL_HEAL :
  case SPELL_GROUP_HEAL :
       AFF_USED = 30;
       break;
 }

 if (AFF_FLAGGED(caster,AFF_CHARM) && AFF_FLAGGED(caster,AFF_HELPER))
    {if (GET_HP_PERC(caster) < AFF_USED)
        return (caster);
     else
     if (caster->master                             &&
   //      !IS_NPC(caster->master)                    &&
	 CAN_SEE(caster, caster->master)            &&
	 IN_ROOM(caster->master) == IN_ROOM(caster) &&
         FIGHTING(caster->master)                   &&
	 GET_HP_PERC(caster->master) < AFF_USED
	)
        return (caster->master);
     return (NULL);
    }

 for (vict = world[IN_ROOM(caster)].people; AFF_USED && vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict)               ||
          AFF_FLAGGED(vict,AFF_CHARM) ||
	  !CAN_SEE(caster, vict)
	 )
         continue;
      if (!FIGHTING(vict) && !(MOB_FLAGGED(vict, MOB_HELPER) || NPC_FLAGGED(vict, NPC_RANDHELP)))
         continue;
      if (GET_HP_PERC(vict) < AFF_USED && (!victim || vict_val > GET_HP_PERC(vict)))
         {victim   = vict;
          vict_val = GET_HP_PERC(vict);
          if (GET_REAL_INT(caster) < number(10,20))
             break;
         }
     }
 return (victim);
}

struct char_data *find_friend(struct char_data *caster, int spellnum)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0, AFF_USED = 0, spellreal = -1;
 switch (spellnum)
 {case SPELL_CURE_BLIND :
       SET_BIT(AFF_USED, AFF_BLIND);
       break;
  case SPELL_REMOVE_POISON :
       SET_BIT(AFF_USED, AFF_POISON);
       break;
  case SPELL_REMOVE_HOLD :
       SET_BIT(AFF_USED, AFF_HOLD); 
	   SET_BIT(AFF_USED, AFF_HOLDALL);
       break;
  case SPELL_REMOVE_CURSE :
       SET_BIT(AFF_USED, AFF_CURSE);
       break;
  case SPELL_REMOVE_SIELENCE :
       SET_BIT(AFF_USED, AFF_SIELENCE);
       break;
  case SPELL_CURE_PLAQUE :
       spellreal = SPELL_PLAQUE;
       break;

  }
 if (AFF_FLAGGED(caster,AFF_CHARM) && AFF_FLAGGED(caster,AFF_HELPER))
    {if (AFF_FLAGGED(caster, AFF_USED) ||
         affected_by_spell(caster,spellreal)
	)
        return (caster);
     else
     if (caster->master                             &&
        // !IS_NPC(caster->master)                    &&
	 CAN_SEE(caster, caster->master)            &&
	 IN_ROOM(caster->master) == IN_ROOM(caster) &&
        // FIGHTING(caster->master)                   &&
         (AFF_FLAGGED(caster->master, AFF_USED) ||
	  affected_by_spell(caster->master,spellreal)
	 )
        )
        return (caster->master);
     return (NULL);
    }

  for (vict = world[IN_ROOM(caster)].people; AFF_USED && vict; vict = vict->next_in_room)
      {if (!IS_NPC(vict) || AFF_FLAGGED(vict,AFF_CHARM) || !CAN_SEE(caster, vict))
          continue;
       if (!AFF_FLAGGED(vict, AFF_USED))
          continue;
       if (!FIGHTING(vict) && !(MOB_FLAGGED(vict, MOB_HELPER) || NPC_FLAGGED(vict, NPC_RANDHELP)))
          continue;
       if (!victim || vict_val < GET_MAXDAMAGE(vict))
          {victim   = vict;
           vict_val = GET_MAXDAMAGE(vict);
           if (GET_REAL_INT(caster) < number(10,20))
              break;
           }
      }
  return (victim);
}

struct char_data  *find_caster(struct char_data *caster, int spellnum)
{struct char_data *vict = NULL, *victim = NULL;
 int    vict_val = 0, AFF_USED, spellreal = -1;
 AFF_USED = 0;
 switch (spellnum)
 {case SPELL_CURE_BLIND :
       SET_BIT(AFF_USED, AFF_BLIND);
       break;
  case SPELL_REMOVE_POISON :
       SET_BIT(AFF_USED, AFF_POISON);
       break;
  case SPELL_REMOVE_HOLD :
       SET_BIT(AFF_USED, AFF_HOLD);
       SET_BIT(AFF_USED, AFF_HOLDALL);
	   break;
  case SPELL_REMOVE_CURSE :
       SET_BIT(AFF_USED, AFF_CURSE);
       break;
  case SPELL_REMOVE_SIELENCE :
       SET_BIT(AFF_USED, AFF_SIELENCE);
       break;
  case SPELL_CURE_PLAQUE :
       spellreal = SPELL_PLAQUE;
       break;
 }

 if (AFF_FLAGGED(caster,AFF_CHARM) && AFF_FLAGGED(caster,AFF_HELPER))
    {if (AFF_FLAGGED(caster, AFF_USED) ||
         affected_by_spell(caster,spellreal)
	)
        return (caster);
     else
     if (caster->master                             &&
        // !IS_NPC(caster->master)                    &&
	 CAN_SEE(caster, caster->master)            &&
	 IN_ROOM(caster->master) == IN_ROOM(caster) &&
        // FIGHTING(caster->master)                   &&
         (AFF_FLAGGED(caster->master, AFF_USED) ||
	  affected_by_spell(caster->master,spellreal)
	 )
        )
        return (caster->master);
     return (NULL);
    }

 for (vict = world[IN_ROOM(caster)].people; AFF_USED && vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict) || AFF_FLAGGED(vict,AFF_CHARM) || !CAN_SEE(caster, vict))
         continue;
      if (!AFF_FLAGGED(vict, AFF_USED))
         continue;
      if (!FIGHTING(vict) && !(MOB_FLAGGED(vict, MOB_HELPER) || NPC_FLAGGED(vict, NPC_RANDHELP)))
         continue;
      if (!victim || vict_val < GET_MAXCASTER(vict))
         {victim   = vict;
          vict_val = GET_MAXCASTER(vict);
          if (GET_REAL_INT(caster) < number(10,20))
             break;
         }
     }
 return (victim);
}


struct  char_data *find_affectee(struct char_data *caster, int spellnum)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0, spellreal = spellnum;

 if (spellreal == SPELL_GROUP_FLY)
    spellreal = SPELL_FLY;
 else
 if (spellreal == SPELL_GROUP_ARMOR)
    spellreal = SPELL_ARMOR;
 else
 if (spellreal == SPELL_GROUP_STRENGTH)
    spellreal = SPELL_STRENGTH;
 else
 if (spellreal == SPELL_GROUP_BLESS)
    spellreal = SPELL_BLESS;
 else
 if (spellreal == SPELL_GROUP_HASTE)
    spellreal = SPELL_HASTE;
else
 if (spellreal == SPELL_GROUP_INVISIBLE)
    spellreal = SPELL_INVISIBLE;

 if (AFF_FLAGGED(caster,AFF_CHARM) && AFF_FLAGGED(caster,AFF_HELPER))
    {if (!affected_by_spell(caster,spellreal))
        return (caster);
     else
     if (caster->master                             &&
        // !IS_NPC(caster->master)                    &&
	 CAN_SEE(caster, caster->master)            &&
	 IN_ROOM(caster->master) == IN_ROOM(caster) &&
         FIGHTING(caster->master)                   &&
	 !affected_by_spell(caster->master,spellreal)
	)
        return (caster->master);
     return (NULL);
    }

 if (GET_REAL_INT(caster) > number(5,15))
    for (vict = world[IN_ROOM(caster)].people; vict; vict = vict->next_in_room)
        {if (!IS_NPC(vict) || AFF_FLAGGED(vict,AFF_CHARM) || !CAN_SEE(caster, vict))
            continue;
         if (!FIGHTING(vict) || AFF_FLAGGED(vict, AFF_HOLD) ||
							 AFF_FLAGGED(vict, AFF_HOLDALL) ||
             affected_by_spell(vict,spellreal))
            continue;
         if (!victim || vict_val < GET_MAXDAMAGE(vict))
            {victim = vict;
             vict_val = GET_MAXDAMAGE(vict);
            }
        }
 if (!victim && !affected_by_spell(caster,spellreal))
    victim = caster;

 return (victim);
}

struct  char_data *find_opp_affectee(struct char_data *caster, int spellnum)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0, spellreal = spellnum;

 if (spellreal == SPELL_GROUP_FLY)
    spellreal = SPELL_FLY;
 else
 if (spellreal == SPELL_POWER_BLINDNESS || spellreal == SPELL_MASS_BLINDNESS)
    spellreal = SPELL_BLINDNESS;
 else
 if (spellreal == SPELL_POWER_SIELENCE || spellreal == SPELL_MASS_SIELENCE)
    spellreal = SPELL_SIELENCE;
 else
 if (spellreal == SPELL_MASS_CURSE)
    spellreal = SPELL_CURSE;
 else
 if (spellreal == SPELL_MASS_SLOW)
    spellreal = SPELL_SLOW;

 if (GET_REAL_INT(caster) > number(10,20))
    for (vict = world[caster->in_room].people; vict; vict = vict->next_in_room)
        {if ((IS_NPC(vict) && !AFF_FLAGGED(vict,AFF_CHARM)) ||
	     !CAN_SEE(caster, vict)
	    )
            continue;
         if ((!FIGHTING(vict) && (GET_REAL_INT(caster) < number(20,27) || !in_same_battle(caster,vict,TRUE))) ||
             AFF_FLAGGED(vict, AFF_HOLD)    || 
			 AFF_FLAGGED(vict, AFF_HOLDALL) ||
			 affected_by_spell(vict,spellreal))
            continue;
         if (!victim || vict_val < GET_MAXDAMAGE(vict))
            {victim   = vict;
             vict_val = GET_MAXDAMAGE(vict);
            }
        }
	
 if (!victim && FIGHTING(caster) && !affected_by_spell(FIGHTING(caster),spellreal))
    victim = FIGHTING(caster);
 return (victim);
}

struct  char_data *find_opp_caster(struct char_data *caster)
{ struct char_data *vict=NULL, *victim = NULL;
  int    vict_val = 0;

  for (vict = world[IN_ROOM(caster)].people; vict; vict = vict->next_in_room)

      {if (IS_NPC(vict))// && !AFF_FLAGGED(vict,AFF_CHARM))
          continue;
       if ((!FIGHTING(vict) && (GET_REAL_INT(caster) < number(15, 25) || !in_same_battle(caster,vict,TRUE))) ||
           AFF_FLAGGED(vict, AFF_HOLD)    ||
	       AFF_FLAGGED(vict,AFF_SIELENCE) ||
		   AFF_FLAGGED(vict, AFF_HOLDALL) ||
	   (!CAN_SEE(caster, vict) && FIGHTING(caster) != vict)
	  )
          continue;
       if (vict_val < GET_MAXCASTER(vict))
          {victim   = vict;
           vict_val = GET_MAXCASTER(vict);
          }
      }
  return (victim);
}
CHAR_DATA *find_damagee(struct char_data *caster)
{struct char_data *vict, *victim = NULL;
 int    vict_val = 0;

 if (GET_REAL_INT(caster) > number(10, 20))
    for (vict = world[IN_ROOM(caster)].people; vict; vict = vict->next_in_room)
        {if ((IS_NPC(vict) && !(AFF_FLAGGED(vict,AFF_CHARM) &&
		(vict->master && !IS_NPC(vict->master)))) ||// ПЕРЕНЕСЕНО
	     !CAN_SEE(caster, vict))
            continue;
         if ((!FIGHTING(vict) && (GET_REAL_INT(caster) < number(20,27) ||
			 !in_same_battle(caster,vict,TRUE)))					   ||
             AFF_FLAGGED(vict, AFF_HOLD)							   || 
			 AFF_FLAGGED(vict, AFF_HOLDALL))
            continue;
         if (GET_REAL_INT(caster) >= number(25,30))
            {if (!victim || vict_val < GET_MAXCASTER(vict))
                {victim = vict;
                 vict_val = GET_MAXCASTER(vict);
                }
            }
         else
         if (!victim || vict_val < GET_MAXDAMAGE(vict))
            {victim   = vict;
             vict_val = GET_MAXDAMAGE(vict);
            }
        }
 if (!victim)
    victim = FIGHTING(caster);

 return (victim);
}


struct  char_data *find_minhp(struct char_data *caster)
{ struct char_data *vict, *victim = NULL;
  int    vict_val = 0;

  if (GET_REAL_INT(caster) > number(10,20))
     for (vict = world[IN_ROOM(caster)].people; vict; vict = vict->next_in_room)
         {if ((IS_NPC(vict) && !(AFF_FLAGGED(vict,AFF_CHARM) &&
		(vict->master && !IS_NPC(vict->master)))) ||
	      !CAN_SEE(caster, vict)
	     )
            continue;
          if (!FIGHTING(vict) &&
              (GET_REAL_INT(caster) < number(20,27) || !in_same_battle(caster, vict, TRUE))
	     )
             continue;
          if (!victim || vict_val > GET_HIT(vict))
             {victim = vict;
              vict_val = GET_HIT(vict);

             }
         }
  if (!victim)
     victim = FIGHTING(caster);

  return (victim);
}

struct  char_data *find_cure(struct char_data *caster, struct char_data *patient, int *spellnum)
{if (GET_HP_PERC(patient) <= number(20,33))
    {if (GET_SPELL_MEM(caster,SPELL_EXTRA_HITS))
        *spellnum = SPELL_EXTRA_HITS;
     else
     if (GET_SPELL_MEM(caster,SPELL_HEAL))
        *spellnum = SPELL_HEAL;
     else
     if (GET_SPELL_MEM(caster,SPELL_CURE_CRITIC))
        *spellnum = SPELL_CURE_CRITIC;
     else
	 if (GET_SPELL_MEM(caster,SPELL_GROUP_CURE_CRITIC))
        *spellnum = SPELL_GROUP_CURE_CRITIC;
     else
     if (GET_SPELL_MEM(caster,SPELL_GROUP_HEAL))
        *spellnum = SPELL_GROUP_HEAL;
    }
 else
 if (GET_HP_PERC(patient) <= number(50,65))
    {if (GET_SPELL_MEM(caster,SPELL_CURE_CRITIC))
        *spellnum = SPELL_CURE_CRITIC;
     else
     if (GET_SPELL_MEM(caster,SPELL_CURE_SERIOUS))
        *spellnum = SPELL_CURE_SERIOUS;
     else
     if (GET_SPELL_MEM(caster,SPELL_CURE_LIGHT))
        *spellnum = SPELL_CURE_LIGHT;
    }
 if (*spellnum)
    return (patient);
 else
    return (NULL);
}

void    mob_casting (struct char_data * ch)
{ struct char_data  *victim;
  char   battle_spells[MAX_STRING_LENGTH];
  int    lag=GET_WAIT(ch), i, spellnum, spells, sp_num;
  struct obj_data   *item;

  if (AFF_FLAGGED(ch, AFF_CHARM)    ||
      AFF_FLAGGED(ch, AFF_HOLD)     ||
      AFF_FLAGGED(ch, AFF_SIELENCE) ||
	  AFF_FLAGGED(ch, AFF_HOLDALL)  ||
      lag > 0
     )
     return;

  memset(&battle_spells,0,sizeof(battle_spells));
  for (i=1,spells=0;i<=MAX_SPELLS;i++)
      if (GET_SPELL_MEM(ch,i) && IS_SET(spell_info[i].routines,NPC_CALCULATE))
         battle_spells[spells++]  = i;

  for (item = ch->carrying;
       spells < MAX_STRING_LENGTH    &&
       item                          &&
	   !IS_ANIMALS(ch)				 &&
       !AFF_FLAGGED(ch, AFF_CHARM);
       item = item->next_content)
      switch(GET_OBJ_TYPE(item))
      { case ITEM_WAND:
        case ITEM_STAFF: if (GET_OBJ_VAL(item,2) > 0 &&
                             IS_SET(spell_info[GET_OBJ_VAL(item,3)].routines,NPC_CALCULATE))
                            battle_spells[spells++] = GET_OBJ_VAL(item,3);
                         break;
        case ITEM_POTION:
                         for (i = 1; i <= 3; i++)
                             if (IS_SET(spell_info[GET_OBJ_VAL(item,i)].routines,NPC_AFFECT_NPC|NPC_UNAFFECT_NPC|NPC_UNAFFECT_NPC_CASTER))
                                battle_spells[spells++] = GET_OBJ_VAL(item,i);
                         break;
        case ITEM_SCROLL:
                     //    for (i = 1; i <= 3; i++)
                       //      if (IS_SET(spell_info[GET_OBJ_VAL(item,i)].routines,NPC_CALCULATE))
                        //        battle_spells[spells++] = GET_OBJ_VAL(item,i);
                         break;
      }

  // перво-наперво  -  лечим себя
  spellnum = 0;
  victim   = find_cure(ch,ch,&spellnum);
  // Ищем рандомную заклинашку и цель для нее
  for (i = 0; !victim && spells && i < GET_REAL_INT(ch) / 5; i++)
      if (!spellnum && (spellnum = battle_spells[(sp_num = number(0,spells-1))]) &&
          spellnum > 0 && spellnum <= MAX_SPELLS)
         {// sprintf(buf,"$n using spell '%s', %d from %d",
          //         spell_name(spellnum), sp_num, spells);
          // act(buf,FALSE,ch,0,FIGHTING(ch),TO_VICT);
          if (spell_info[spellnum].routines & NPC_DAMAGE_PC_MINHP)
             {if (!AFF_FLAGGED(ch, AFF_CHARM))
                 victim = find_minhp(ch);
             }
          else
          if (spell_info[spellnum].routines & NPC_DAMAGE_PC)
             {if (!AFF_FLAGGED(ch, AFF_CHARM))
                 victim  = find_damagee(ch);
             }
          else
          if (spell_info[spellnum].routines & NPC_AFFECT_PC_CASTER)
             {if (!AFF_FLAGGED(ch, AFF_CHARM))
                 victim  = find_opp_caster(ch);
             }
          else
          if (spell_info[spellnum].routines & NPC_AFFECT_PC)
             {if (!AFF_FLAGGED(ch, AFF_CHARM))
                 victim  = find_opp_affectee(ch, spellnum);
             }
          else
          if (spell_info[spellnum].routines & NPC_AFFECT_NPC)
             victim  = find_affectee(ch, spellnum);
          else
          if (spell_info[spellnum].routines & NPC_UNAFFECT_NPC_CASTER)
             victim  = find_caster(ch, spellnum);
          else
          if (spell_info[spellnum].routines & NPC_UNAFFECT_NPC)
             victim  = find_friend(ch, spellnum);
          else
          if (spell_info[spellnum].routines & NPC_DUMMY)
             victim  = find_friend_cure(ch, spellnum);
          else
             spellnum = 0;
         }
  if (spellnum && victim)
     { // Is this object spell ?
       for (item = ch->carrying;
        !AFF_FLAGGED(ch, AFF_CHARM) &&
	    item                        &&
	    !IS_ANIMALS(ch);
	    item = item->next_content
           )
           switch(GET_OBJ_TYPE(item))
           { case ITEM_WAND:
             case ITEM_STAFF: if (GET_OBJ_VAL(item,2) > 0 &&
                                  GET_OBJ_VAL(item,3) == spellnum)
                                 {mag_objectmagic(ch,item,GET_NAME(victim));
                                  return;
                                 }
                              break;
             case ITEM_POTION:
                              for (i = 1; i <= 3; i++)
                                  if (GET_OBJ_VAL(item,i) == spellnum)
                                     {if (ch != victim)
                                         {obj_from_char(item);
                                          act("$n передал$y $3 $D.",FALSE,ch,item,victim,TO_ROOM);
                                          obj_to_char(item,victim);
                                         }
                                      else
                                         victim = ch;
                                      mag_objectmagic(victim,item,GET_NAME(victim));
                                      return;
                                     }
                              break;
             case ITEM_SCROLL:
                            /*  for (i = 1; i <= 3; i++)
                                   if (GET_OBJ_VAL(item,i) == spellnum)
                                      {mag_objectmagic(ch,item,GET_NAME(victim));
                                       return;
                                      }*/
                              break;
            }

      cast_spell(ch,victim,NULL,spellnum);
	    WAIT_STATE(ch, PULSE_VIOLENCE);
     }
}

#define  MAY_LIKES(ch)   ((!AFF_FLAGGED(ch, AFF_CHARM) || AFF_FLAGGED(ch, AFF_HELPER)) && \
                          AWAKE(ch) && GET_WAIT(ch) <= 0)

/* control the fights going on.  Called every 2 seconds from comm.c. */
/* Управляет длительностью поединка, установелнным 2 секунды в comm.c*/

void improove_skill(struct char_data *ch, int skill_no, int success,
                    struct char_data *victim)
{int    skill_is, diff=0, how_many=0, prob, koeff;

	if (IS_NPC(ch))
    	   return;
    if (victim && (IS_HORSE(victim) || MOB_FLAGGED(victim, MOB_NOTRAIN)))
		return;

 if (IS_IMMORTAL(ch) ||
     ((!victim ||(OK_GAIN_EXP(ch, victim) && GET_EXP(victim))) &&
      IN_ROOM(ch) != NOWHERE                       &&
      !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)    && // Стрибог
      !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA)       &&

      (diff = wis_app[GET_REAL_WIS(ch)].max_learn_l20 * GET_LEVEL(ch) / 25 + 10 - //временно 6.. было 20
              GET_SKILL(ch,skill_no))
        > 0                                      &&
      GET_SKILL(ch,skill_no) < MAX_EXP_PERCENT + GET_REMORT(ch) * 10
     )
	)
 { 
 //Natan
	how_many = GET_COUNT_SKILL(ch);
	switch(skill_no)
	{
	case SKILL_MIGHTHIT:
		koeff = 21;
		break;
	case SKILL_STUPOR:
		koeff = 14;
		break;
	case SKILL_POISONED:
		koeff = 30;
		break;
	case SKILL_HIDETRACK:
		koeff = 24;
		break;
	case SKILL_MULTYPARRY:
		koeff = 4;
		break;
	case SKILL_MAKEFOOD:
		koeff = 20;
		break;
	case SKILL_LEADERSHIP:
		koeff = 40;
		break;
    	case SKILL_IDENTIFY:
		koeff = 30;
		break;
	case SKILL_AID:
		koeff = 45;
		break;
	case SKILL_TEAR:
		koeff = 25;
		break;
	case SKILL_TOCHNY:
		koeff = 8;
		break;
	case SKILL_THROW:
		koeff = 9;
		break;
	case SKILL_BACKSTAB:
		koeff = 15;
		break;
	case SKILL_BLADE_VORTEX:
		koeff = 3;
		break;
	case SKILL_BASH:
		koeff = 13;
		break;
	case SKILL_HIDE:
		koeff = 18;
		break;
	case SKILL_KICK:
		koeff = 18;
		break;
	case SKILL_PICK_LOCK:
		koeff = 16;
		break;
	case SKILL_PUNCH:
		koeff = 5;
		break;
	case SKILL_RESCUE:
		koeff = 16;
		break;
	case SKILL_SNEAK:
    case SKILL_CAMOUFLAGE:
		koeff = 35;
		break;
	case SKILL_STEAL:
		koeff = 19;
		break;
	case SKILL_TRACK:
		koeff = 14;
		break;
	case SKILL_CLUBS:
		koeff = 5;
		break;
	case SKILL_AXES:
		koeff = 6;
		break;
	case SKILL_LONGS:
		koeff = 4;
		break;
    	case SKILL_HORSE:
		koeff = 4;
		break; 
	case SKILL_SHORTS:
		koeff = 6;
		break;
	case SKILL_NONSTANDART:
		koeff = 4;
		break;
	case SKILL_BOTHHANDS:
		koeff = 9;
		break;
	case SKILL_BOTH_WEAPON:
        koeff = 5;
		break;
	case SKILL_PICK:
		koeff = 7;
		break;
	case SKILL_SPADES:
		koeff = 7;
		break;
	case SKILL_PRIGL:
		koeff = 14;
		break;
	case SKILL_DISARM:
		koeff = 12;
		break;
	case SKILL_PARRY:
		koeff = 15;
		break;
	case SKILL_BOWS:
		koeff = 4;
		break;
	case SKILL_DEVIATE:
		koeff = 19;
		break;
	case SKILL_BLOCK:
		koeff = 14;
		break;
	case SKILL_LOOKING:
		koeff = 14;
		break;
	case SKILL_CHOPOFF:
		koeff = 14;
		break;
	case SKILL_REPAIR:
		koeff = 50;
		break;
	case SKILL_REST:
		koeff = 50;
		break;
	case SKILL_UPGRADE:
		koeff = 30;
		break;
	case SKILL_COURAGE:
		koeff = 9;
		break;
	default:
		koeff = 4;
		break;
	
	}
		 /* Success - multy by 2 */
     prob  = success ? 16000 : 12000;

	 koeff *= GET_REAL_INT(ch) + MAX(0,GET_REAL_INT(ch) - 15) + MAX(0,2*(GET_REAL_INT(ch)-20));
	 koeff *= SERVER_RATE;
     koeff *= MAX(130, 10 * (GET_LEVEL(ch) + 5)) / (skill_info[skill_no].k_improove[GET_CLASS(ch)]+5); 
	 
	 prob += 200 *( 5 + how_many - (wis_app[GET_REAL_WIS(ch)].max_skills + GET_LEVEL(ch)/3));     
	 prob    += number(1, GET_SKILL(ch,skill_no) * 50);
 
	 skill_is = number(1, MAX(1, prob));
 
     // if (!IS_NPC(ch))
//        log("Player %s skill '%d' - need to improove %d(%d-%d)",
//            GET_NAME(ch), skill_no, skill_is, div, prob);
   if (((GET_LEVEL(ch) + wis_app[GET_REAL_WIS(ch)].max_skills -	5) * 3) > GET_SKILL(ch,skill_no))
     if ( ( victim  && (100 * skill_is <= koeff * (GET_LEVEL(victim) + 2) / (GET_LEVEL(ch)) + 2)) ||
          ( !victim && (100 * skill_is <= koeff)))
      if (!GET_PRACTICES(ch,skill_no)) 
	 { if (success)
           sprintf(buf,"%sВам необходимо посетить гильдию в области \"%s\".%s\r\n",
                   CCCYN(ch,C_NRM),skill_name(skill_no),CCNRM(ch,C_NRM));
        else
           sprintf(buf,"%sВам пришло время посетить гильдию в области \"%s\".%s\r\n",
                   CCCYN(ch,C_NRM),skill_name(skill_no),CCNRM(ch,C_NRM));
        send_to_char(buf,ch);
        GET_PRACTICES(ch,skill_no) = 1;
      }
	}
//end Natan
}

int train_skill (struct char_data * ch, int skill_no, int max_value,
                struct char_data * vict)
{int    percent = 0;

percent = calculate_skill(ch,skill_no,max_value,vict);
if (!IS_NPC(ch))
   {if (skill_no != SKILL_SECOND_ATTACK && GET_SKILL(ch, skill_no) > 0 &&
        (!vict || (IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_PROTECT) &&
          !AFF_FLAGGED(vict, AFF_CHARM)) && !IS_HORSE(vict)
         
        )
       )
     improove_skill(ch, skill_no, percent >= max_value, vict);
	}
else
  if (!AFF_FLAGGED(ch,AFF_CHARM))
      if (GET_SKILL(ch, skill_no) > 0  &&
      GET_REAL_INT(ch) * 3 >= number(0, 1000 - 2 * GET_REAL_WIS(ch)) &&
      GET_SKILL(ch, skill_no) < skill_info[skill_no].max_percent
     )
   // как раз прокачка тут скиллов для мобов!!!!
   GET_SKILL(ch,skill_no)++;

return (percent);

}

/**** This function return chance of skill */
int calculate_skill (CHAR_DATA * ch, int skill_no, int max_value, CHAR_DATA * vict)
{int    skill_is, percent=0, victim_sav = SAVING_REFLEX, victim_modi = 0;
//SAVING_REFLEX реакция

if (skill_no < 1 || skill_no > MAX_SKILLS)
   {// log("ERROR: ATTEMPT USING UNKNOWN SKILL <%d>", skill_no);
    return 0;
   }
if ((skill_is = GET_SKILL(ch,skill_no)) <= 0)
   {return 0;
   }

skill_is += int_app[GET_REAL_INT(ch)].to_skilluse;

switch (skill_no)
  {
case SKILL_BACKSTAB:   // заколоть
     percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction + 10;

     if (awake_others(ch))
        percent -= 50;

	percent += (GET_SKILL(ch, SKILL_HIDE) - 40)/5;
    percent += (GET_SKILL(ch, SKILL_SNEAK) - 40)/5;

     if (vict)
        {if (!CAN_SEE(vict, ch))
            percent += 30;
         if (GET_POS(vict) < POS_FIGHTING)
            percent +=  (20 * (POS_FIGHTING - GET_POS(vict)));
         else
         if (AFF_FLAGGED(vict, AFF_AWARNESS))
            victim_modi -= 20;
         victim_modi +=  size_app[GET_REAL_SIZE(vict)].ac;
         victim_modi -=  dex_app_skill[GET_REAL_DEX(vict)].traps;
        }
     break;
case SKILL_BLADE_VORTEX:   // bl vortex
     percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction;

     if (awake_others(ch))
        percent -= 50;

     if (vict)
        {if (GET_POS(vict) < POS_FIGHTING)
            percent +=  (20 * (POS_FIGHTING - GET_POS(vict)));
         else
         if (AFF_FLAGGED(vict, AFF_AWARNESS))
            victim_modi -= 30;
         victim_modi +=  size_app[GET_REAL_SIZE(vict)].ac;
         victim_modi -=  dex_app_skill[GET_REAL_DEX(vict)].traps;
        }
     break;
 case SKILL_BASH:// сбить
    percent =  skill_is + 10 + size_app[GET_REAL_SIZE(ch)].interpolate +
               dex_app[GET_REAL_DEX(ch)].reaction +
			   dex_app[GET_REAL_STR(ch)].reaction +
			   (GET_EQ(ch, WEAR_SHIELD) ?
		     weapon_app[MIN(50, MAX(0, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_SHIELD))))].
		     bashing : 0);
               
    	if (vict)
		{ victim_modi -= size_app[GET_REAL_SIZE(vict)].interpolate;
          if (GET_AF_BATTLE(ch, EAF_AWAKE)) 
        victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
	
		  if (GET_POS(vict) < POS_FIGHTING && GET_POS(vict) > POS_SLEEPING)
              victim_modi -= 30;

	     victim_modi -=dex_app[GET_REAL_DEX(vict)].reaction;
        }
     break;
case SKILL_HIDE:       // спрятаться
     percent = skill_is +
               dex_app_skill[GET_REAL_DEX(ch)].hide -
               size_app[GET_REAL_SIZE(ch)].ac;

     if (awake_others(ch))//шумит ли предмет и железный ли., если в санке то тоже плохо
        percent -= 50;

     if (IS_DARK(IN_ROOM(ch)))
        percent += 25;

     if (SECT(IN_ROOM(ch)) == SECT_CITY)
        percent -= 15;
     else
     if (SECT(IN_ROOM(ch)) == SECT_FOREST)
        percent += 20;
     else
     if (SECT(IN_ROOM(ch)) == SECT_HILLS ||
         SECT(IN_ROOM(ch)) == SECT_MOUNTAIN)
        percent += 15;
     if (equip_in_metall(ch))//считает вес экипировки железной и можешь ли нести.
        percent -= 50;

     if (vict)
        {if (AWAKE(vict))
            victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        }
     break;
case SKILL_KICK:       // пнуть
	 victim_sav = SAVING_STABILITY;
	 percent = skill_is + 
			dex_app[GET_REAL_DEX(ch)].reaction +
			dex_app[GET_REAL_STR(ch)].reaction;
		if (vict) {//GET_REAL_SIZE
			victim_modi += size_app[GET_POS_SIZE(vict)].interpolate;
			victim_modi += dex_app[GET_REAL_CON(vict)].reaction;
			if (GET_AF_BATTLE(vict, EAF_AWAKE))
				victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
		}

     break;
case SKILL_PICK_LOCK:  // взломать.
     percent = skill_is +
                dex_app_skill[GET_REAL_DEX(ch)].p_locks;
     break;
case SKILL_PUNCH:      // рукопашный бой.
     percent = skill_is;
     break;
case SKILL_RESCUE:     // спасти
     percent     = skill_is +
                   dex_app[GET_REAL_DEX(ch)].reaction;
     victim_modi = 100;
     break;
case SKILL_SNEAK:      // sneak
     percent = skill_is +
               dex_app_skill[GET_REAL_DEX(ch)].sneak;

     if (awake_others(ch))
        percent -= 50;

     if (SECT(IN_ROOM(ch)) == SECT_CITY)
        percent -= 10;
     if (IS_DARK(IN_ROOM(ch)))
        percent += 20;
     if (equip_in_metall(ch))
        percent -= 50;

     if (vict)
        {if (!CAN_SEE(vict, ch))
                victim_modi += 25;
         if (AWAKE(vict))
            victim_modi -= int_app[GET_REAL_INT(vict)].observation;
            
        }
     break;
case SKILL_STEAL:      // украсть.
     percent = skill_is +
               dex_app_skill[GET_REAL_DEX(ch)].p_pocket;

     if (awake_others(ch))
        percent -= 50;

     if (IS_DARK(IN_ROOM(ch)))
        percent += 20;

     if (vict)
        {if (!CAN_SEE(vict, ch))
            victim_modi += 25;
         if (AWAKE(vict))
            {victim_modi -= int_app[GET_REAL_INT(vict)].observation;
             if (AFF_FLAGGED(vict, AFF_AWARNESS))
                victim_modi -= 30;
            }
        }
     break;
case SKILL_TRACK:      // выследить
      percent = skill_is +
                int_app[GET_REAL_INT(ch)].observation;

      if (SECT(IN_ROOM(ch)) == SECT_FOREST ||
          SECT(IN_ROOM(ch)) == SECT_FIELD)
         percent += 10;

      percent = complex_skill_modifier(ch,SKILL_THAC0,GAPPLY_SKILL_SUCCESS,percent);

      if (SECT(IN_ROOM(ch)) == SECT_WATER_SWIM ||
          SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM ||
          SECT(IN_ROOM(ch)) == SECT_FLYING ||
          SECT(IN_ROOM(ch)) == SECT_UNDERWATER ||
          SECT(IN_ROOM(ch)) == SECT_SECRET ||
          ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
         percent = 0;


      if (vict)
         {victim_modi += con_app[GET_REAL_CON(vict)].hitp;
          if (AFF_FLAGGED(vict, AFF_NOTRACK) ||
              ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
             victim_modi = -100;
         }
      break;

case SKILL_SENSE:
     percent = skill_is +
               int_app[GET_REAL_INT(ch)].observation;

     percent = complex_skill_modifier(ch,SKILL_THAC0,GAPPLY_SKILL_SUCCESS,percent);

     if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
        percent = 0;

     if (vict)
         {victim_modi += con_app[GET_REAL_CON(vict)].hitp;
          if (AFF_FLAGGED(vict, AFF_NOTRACK) ||
              ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTRACK))
             victim_modi = -100;
         }
     break;
case SKILL_MULTYPARRY:
case SKILL_PARRY:      // парировать
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction;
     if (GET_AF_BATTLE(ch, EAF_AWAKE))
        percent += GET_SKILL(ch, SKILL_AWAKE);

     if (GET_EQ(ch,WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch,WEAR_HOLD)) == ITEM_WEAPON)
        {percent += weapon_app[MAX(0,MIN(50,GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_HOLD))))].parrying;
        }
     victim_modi = 100;
     break;

case SKILL_BLOCK:      // закрыться щитом
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction;
     if (GET_AF_BATTLE(ch, EAF_AWAKE))
        percent += GET_SKILL(ch, SKILL_AWAKE);

     break;

case SKILL_TOUCH:      // захватить противника
    percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction +
               size_app[GET_REAL_SIZE(vict)].interpolate;

     if (vict)
        {victim_modi -= dex_app[GET_REAL_DEX(vict)].reaction;
         victim_modi -= size_app[GET_REAL_SIZE(vict)].interpolate;
        }
     break;

case SKILL_PROTECT:    // прикрыть грудью
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction +
               size_app[GET_REAL_SIZE(ch)].interpolate;

     victim_modi = 100;
     break;

case SKILL_BOWS:       // луки
    percent = skill_is + dex_app[GET_REAL_DEX(ch)].miss_att;
     break;
  case SKILL_BOTHHANDS:  // двуручники
  case SKILL_LONGS:      // длинные лезвия
  case SKILL_SPADES:     // копья и пики
  case SKILL_SHORTS:     // короткие лезвия
  case SKILL_CLUBS:      // палицы и дубины
  case SKILL_PICK:       // проникающее
  case SKILL_NONSTANDART:// разнообразное оружие
  case SKILL_AXES:       // секиры
  case SKILL_SECOND_ATTACK: // атака второй рукой
    percent = skill_is;
     break;
case SKILL_LOOKING:    // приглядеться
     percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
     break;
case SKILL_HEARING:    // прислушаться
     percent = skill_is + int_app[GET_REAL_INT(ch)].observation;
     break;
case SKILL_DISARM:
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction +
			   dex_app[GET_REAL_STR(ch)].reaction;
     if (vict)
        {victim_modi -= dex_app[GET_REAL_DEX(vict)].reaction;
         if (GET_EQ(vict,WEAR_BOTHS))
            victim_modi -= 10;
	 if (GET_AF_BATTLE(vict, EAF_AWAKE))
	    victim_modi -= (GET_SKILL(vict, SKILL_AWAKE) / 2);
        }
     break;
case SKILL_HEAL:
     percent = skill_is;
     break;
case SKILL_TURN:
     percent = skill_is;
     break;
case SKILL_ADDSHOT:
     percent = skill_is + dex_app[GET_REAL_DEX(ch)].miss_att  +
		                  wis_app[GET_REAL_WIS(ch)].addshot;
     if (!equip_in_metall(ch))
        percent += 50;
     break;
case SKILL_CAMOUFLAGE:
     percent = skill_is +
               dex_app_skill[GET_REAL_DEX(ch)].hide -
               size_app[GET_REAL_SIZE(ch)].ac;

     if (awake_others(ch))
        percent -= 100;

     if (IS_DARK(IN_ROOM(ch)))
        percent += 15;

     if (SECT(IN_ROOM(ch)) == SECT_CITY)
        percent -= 15;
     else
     if (SECT(IN_ROOM(ch)) == SECT_FOREST)
        percent += 10;
     else
     if (SECT(IN_ROOM(ch)) == SECT_HILLS ||
         SECT(IN_ROOM(ch)) == SECT_MOUNTAIN)
        percent += 5;
     if (equip_in_metall(ch))
        percent -= 30;

     if (vict)
        if (AWAKE(vict))
            victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        
     break;
case SKILL_DEVIATE:
     percent = skill_is -
               size_app[GET_REAL_SIZE(ch)].ac +
               dex_app[GET_REAL_DEX(ch)].reaction;

     if (equip_in_metall(ch))
        percent -= 30;

     if (vict)
        victim_modi -= dex_app[GET_REAL_DEX(vict)].miss_att;
        
     break;
case SKILL_CHOPOFF:
     percent = skill_is +
               dex_app[GET_REAL_DEX(ch)].reaction +
               size_app[GET_REAL_SIZE(ch)].ac +10;

     if (equip_in_metall(ch))
        percent -= 1;

     if (vict)
        {if (!CAN_SEE(vict,ch))
            percent += 20;
         if (GET_POS(vict) < POS_SITTING)
            percent -= 30;
         if (AWAKE(vict) && AFF_FLAGGED(vict, AFF_AWARNESS))
            victim_modi -= 20;
	 if (GET_AF_BATTLE(vict, EAF_AWAKE))
	    victim_modi -= GET_SKILL(ch, SKILL_AWAKE);
         victim_modi -= dex_app[GET_REAL_DEX(vict)].reaction;
         victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        }
     break;
case SKILL_REPAIR:
     percent = skill_is;
     break;
case SKILL_UPGRADE:
     percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction +
		          dex_app[GET_REAL_STR(ch)].reaction;
case SKILL_COURAGE:
     percent = skill_is * 130 / 100;
     break;
case SKILL_SHIT:
     percent = skill_is;
     break;
case SKILL_MIGHTHIT:
	 victim_sav = SAVING_STABILITY;
     percent = skill_is +
               size_app[GET_REAL_SIZE(ch)].shocking +
               str_app[GET_REAL_STR(ch)].todam * 2 ;

     if (vict)
         victim_modi -=  size_app[GET_REAL_SIZE(vict)].shocking;
        
     break;
case SKILL_STUPOR:
	 victim_sav = SAVING_STABILITY;
     percent = skill_is +
               str_app[GET_REAL_STR(ch)].tohit;
     if (GET_EQ(ch,WEAR_WIELD))
        percent += weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_WIELD))].shocking;
     else
     if (GET_EQ(ch,WEAR_BOTHS))
        percent += weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_BOTHS))].shocking;

     if (vict)
         victim_modi -=  con_app[GET_REAL_CON(vict)].critic_saving;
        
     break;
case SKILL_POISONED:
     percent = skill_is + int_app[GET_REAL_INT(ch)].improove -10;
     break;
case SKILL_LEADERSHIP:
     percent = skill_is + cha_app[GET_REAL_CHA(ch)].leadership;
     break;
case SKILL_PUNCTUAL:
	 victim_sav = SAVING_CRITICAL;
     percent = skill_is +
               int_app[GET_REAL_INT(ch)].observation;

     if (vict)
        victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        
     break;
case SKILL_AWAKE:
     percent = skill_is +
               int_app[GET_REAL_INT(ch)].observation;

     if (vict)
        victim_modi -= int_app[GET_REAL_INT(vict)].observation;
        
     break;

case SKILL_IDENTIFY:
     percent = skill_is +
               int_app[GET_REAL_INT(ch)].observation;
     break;

case SKILL_CREATE_POTION:
case SKILL_CREATE_SCROLL:
case SKILL_CREATE_WAND:
     percent = skill_is;
     break;
case SKILL_LOOK_HIDE:
     percent   = skill_is +
                 cha_app[GET_REAL_CHA(ch)].illusive;
     if (vict)
        {if (!CAN_SEE(vict,ch))
            percent += 50;
         else
         if (AWAKE(vict))
            victim_modi -= int_app[GET_REAL_INT(ch)].observation;
        }
     break;
case SKILL_ARMORED:
     percent = skill_is;
     break;
case SKILL_DRUNKOFF:
     percent = skill_is - con_app[GET_REAL_CON(ch)].hitp;
     break;
case SKILL_AID:
     percent = skill_is;
     break;
case SKILL_FIRE:
     percent = skill_is;
     if (get_room_sky(IN_ROOM(ch)) == SKY_RAINING)
        percent -= 50;
     else
     if (get_room_sky(IN_ROOM(ch))  != SKY_LIGHTNING)
        percent -= number(10,25);
	break;
case SKILL_RIDING:
     percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction;
	break;

case SKILL_HORSE:
    percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction;
    break;
	
case SKILL_SINGLE_WEAPON:
	percent = skill_is + dex_app[GET_REAL_DEX(ch)].reaction;
	break;
default:
     percent = skill_is;
     break;
  }

  //здесь протестить и перенести в линух 25.02.2007 заремил виктим_модифи, ниже переписал обработку, где морале встречается
  //просмотреть какой будет виктим_модифи в сумме, и проверить как будет действовать к бонусам.
//victim_modi += percent / 2;

	percent = complex_skill_modifier(ch,skill_no, GAPPLY_SKILL_SUCCESS,percent);
	
	int morale = cha_app[GET_REAL_CHA(ch)].morale + GET_MORALE(ch);

	if (vict && percent > skill_info[skill_no].max_percent)
		victim_modi += percent - skill_info[skill_no].max_percent -
		(MAX (0, morale - 50) * 2);

	if (vict!=ch && vict && general_savingthrow(vict, victim_sav, victim_modi))
		percent = 0;
	else
	if (number(0,99) <= morale)
		percent = skill_info[skill_no].max_percent;


	if (IS_IMMORTAL(ch)               ||      // бессмертный
		GET_GOD_FLAG(ch, GF_GODSLIKE)         // спецфлаг
	)
		percent = MAX(percent, skill_info[skill_no].max_percent);
	else
	if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
		(vict && GET_GOD_FLAG(vict, GF_GODSLIKE)))
		percent = 0;
	else
	if (vict && GET_GOD_FLAG(vict, GF_GODSCURSE))
		percent = MAX(percent,skill_info[skill_no].max_percent);
	else
		percent = MIN(MAX(0,percent), max_value);


return (percent);
}

void perform_wan(void)
{
  struct char_data *ch;

	for (ch = combat_list; ch; ch = next_combat_list)
		{ next_combat_list = ch->next_fighting;
       // раунд в секунду, что бы можно было во время сбегания
		// из боя кем бы то ни было бежать с меньшей задержкой
       if (FIGHTING(ch) == NULL ||
           IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)) ||
           IN_ROOM(ch) == NOWHERE)
          stop_fighting(ch, TRUE);
		}
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{ struct char_data *ch, *vict, *caster=NULL, *damager=NULL;
  int    i, do_this, initiative, max_init = 0, min_init = 100,
         sk_use = 0, sk_num = 0;
  struct helper_data_type *helpee;

  // Step
  for (ch = combat_list; ch; ch = next_combat_list)
      {next_combat_list = ch->next_fighting;
       
/*         if (FIGHTING (ch) == NULL || IN_ROOM (ch) != IN_ROOM (FIGHTING (ch)) || IN_ROOM (ch) == NOWHERE)
	{ stop_fighting (ch, TRUE);
	  continue;
	}
*/
	   if (GET_MOB_HOLD(ch)           	||
            AFF_FLAGGED(ch, AFF_HOLDALL)  	||
		   !IS_NPC(ch)            	||
           GET_WAIT(ch) > 0               	||
           GET_POS(ch) < POS_FIGHTING     	||
           AFF_FLAGGED(ch, AFF_CHARM)     	||
	       AFF_FLAGGED(ch, AFF_STOPFIGHT) 	||
           AFF_FLAGGED(ch, AFF_SIELENCE))
          continue;

       if (!PRF_FLAGGED(FIGHTING(ch),PRF_NOHASSLE))		
          for (sk_use = 0, helpee = GET_HELPER(ch); helpee;
               helpee=helpee->next_helper)
              for (vict = character_list; vict; vict = vict->next)
                  {if (!IS_NPC(vict)                          		||
                       GET_MOB_VNUM(vict) != helpee->mob_vnum 		||
                       AFF_FLAGGED(vict, AFF_HOLD)            		||
                       AFF_FLAGGED(vict, AFF_HOLDALL)		  	||	
					   AFF_FLAGGED(vict, AFF_CHARM) ||
                       AFF_FLAGGED(vict, AFF_BLIND)           		||
                       GET_WAIT(vict) > 0                     		||
                       GET_POS(vict) < POS_STANDING           		||
                       IN_ROOM(vict) == NOWHERE               		||
                       FIGHTING(vict)
                      )

                      continue;
                  if (!sk_use && !IS_ANIMALS(ch))
 act("$n закричал$y : \"На помощь! На помощь! На помощь!\"",FALSE,ch,0,0,TO_ROOM);
                  if (IN_ROOM(vict) != IN_ROOM(ch)) {
				      char_from_room(vict);
                      char_to_room(vict, IN_ROOM(ch));
                      act("На крик $R появил$u $n и вступил$y в бой на $S стороне.",FALSE,vict,0,ch,TO_ROOM);
                     }
                  else
                     act("$n вступил$y в бой на стороне $R.",FALSE,vict,0,ch,TO_ROOM);
                  set_fighting(vict, FIGHTING(ch));
               };
      }


  // Step 1. Define initiative, mob casting and mob flag skills
  for (ch = combat_list; ch; ch = next_combat_list)
      {next_combat_list = ch->next_fighting;
       // Initialize initiative
       INITIATIVE(ch) = 0;
       BATTLECNTR(ch) = 0;
       SET_AF_BATTLE(ch,EAF_STAND);
       if (affected_by_spell(ch,SPELL_SLEEP))
          SET_AF_BATTLE(ch,EAF_SLEEP);
       if (GET_MOB_HOLD(ch)               ||
		   AFF_FLAGGED(ch, AFF_HOLDALL)   || 
           AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
           IN_ROOM(ch) == NOWHERE)
          continue;
	
       // Mobs stand up	
       if (IS_NPC(ch)                 &&
           GET_POS(ch) < POS_FIGHTING &&
           GET_POS(ch) > POS_STUNNED  &&
           GET_WAIT(ch) <= 0          &&
           !GET_MOB_HOLD(ch)          &&
		   !AFF_FLAGGED(ch, AFF_HOLDALL)&&
           !AFF_FLAGGED(ch, AFF_SLEEP)
          )
          {GET_POS(ch) = POS_FIGHTING;
            if (NPC_FLAGGED(ch, NPC_MOVEJUMP))
			    act("$n поднял$u на лапы.",FALSE,ch,0,0,TO_ROOM);
			else
			if (NPC_FLAGGED(ch, NPC_MOVERUN))
			    act("$n встал$y на лапы.",FALSE,ch,0,0,TO_ROOM);
			else
			if (AFF_FLAGGED(ch, AFF_FLY) || NPC_FLAGGED(ch, NPC_MOVEFLY))
				act("$n взлетел$y снова над землей.",FALSE,ch,0,0,TO_ROOM);
			else
			if (NPC_FLAGGED(ch, NPC_MOVESWIM))
			    act("$n всплыл$y над водой.",FALSE,ch,0,0,TO_ROOM);
			else 
                        if (NPC_FLAGGED(ch, NPC_MOVECREEP))
				act("$n приш$i в себя.",FALSE,ch,0,0,TO_ROOM);
			else
				act("$n встал$y на ноги.",FALSE,ch,0,0,TO_ROOM);
			
			//act("$n встал$y на ноги.", TRUE, ch, 0, 0, TO_ROOM);
          }	

       // For NPC without lags and charms make it likes
       if (IS_NPC(ch) && MAY_LIKES(ch))
          {// Если есть, берем в комнате оружие и одежду и экипируемся.
           if (!AFF_FLAGGED(ch, AFF_CHARM) && npc_battle_scavenge(ch))
              {npc_wield(ch);
               npc_armor(ch);
              }
           // Set some flag-skills
           // 1) parry
           do_this = number(0,100);
           sk_use  = FALSE;
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_PARRY) )
              {SET_AF_BATTLE(ch,EAF_PARRY);
               sk_use = TRUE;
              }

           // 2) blocking
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_BLOCK))
              {SET_AF_BATTLE(ch,EAF_BLOCK);
               sk_use = TRUE;
              }

           // 3) multyparry
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_MULTYPARRY) )
              {SET_AF_BATTLE(ch,EAF_MULTYPARRY);
               sk_use = TRUE;
              }


           // 4) deviate
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_DEVIATE))
              {SET_AF_BATTLE(ch,EAF_DEVIATE);
               sk_use = TRUE;
              }

           // 5) stupor
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_STUPOR))
              {SET_AF_BATTLE(ch,EAF_STUPOR);
               sk_use = TRUE;
              }

           // 6) mighthit
           do_this = number(0,100);
           if (!sk_use && do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_STUPOR))
              {SET_AF_BATTLE(ch,EAF_MIGHTHIT);
               sk_use = TRUE;
              }

           // 7) styles
           do_this = number(0,100);
           if (do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_AWAKE) > number(1,101))
              SET_AF_BATTLE(ch,EAF_AWAKE);
           else
              CLR_AF_BATTLE(ch,EAF_AWAKE);
           do_this = number(0,100);
           if (do_this <= GET_LIKES(ch) &&
               GET_SKILL(ch,SKILL_PUNCTUAL) > number(1,101))
              SET_AF_BATTLE(ch,EAF_PUNCTUAL);
           else
              CLR_AF_BATTLE(ch,EAF_PUNCTUAL);
          }

       initiative = size_app[GET_REAL_SIZE(ch)].initiative;
       if ((i = number(1,10)) == 10)

          initiative -= 1;
       else
          initiative += i;

       initiative    += GET_INITIATIVE(ch);
              
      if (!IS_NPC(ch))
          switch (IS_CARRYING_W(ch) * 10 / MAX(1,CAN_CARRY_W(ch)))
          {case 10: case 9: case 8:
            initiative -= 2;
            break;
           case 7: case 6: case 5:
            initiative -= 1;
            break;
          }

       if (GET_AF_BATTLE(ch,EAF_AWAKE))
          initiative -= 2;
       if (GET_AF_BATTLE(ch,EAF_PUNCTUAL))
          initiative -= 1;
       if (AFF_FLAGGED(ch,AFF_SLOW))
          initiative -= 10;
       if (AFF_FLAGGED(ch,AFF_HASTE))
          initiative += 10;
       if (GET_WAIT(ch) > 0)
          initiative -= 1;
       if (calc_leadership(ch))
          initiative += 5;
       if (GET_AF_BATTLE(ch, EAF_SLOW))
          initiative  = 1;

       initiative = MAX(initiative, 1);
       INITIATIVE(ch) = initiative;
       SET_AF_BATTLE(ch, EAF_FIRST);
       max_init = MAX(max_init, initiative);
       min_init = MIN(min_init, initiative);
      }

  /* Process fighting           */
  for (initiative = max_init; initiative >= min_init; initiative--)
      for (ch = combat_list; ch; ch = next_combat_list)
          {next_combat_list = ch->next_fighting;
           if (INITIATIVE(ch) != initiative ||
               IN_ROOM(ch) == NOWHERE)
              continue;
           // If mob cast 'hold' when initiative setted
           if (AFF_FLAGGED(ch, AFF_HOLD)      ||
               AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
			   AFF_FLAGGED(ch, AFF_HOLDALL)   ||
	       !AWAKE(ch)
	      )
              continue;
           // If mob cast 'fear', 'teleport', 'recall', etc when initiative setted
           if (!FIGHTING(ch) ||
               IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))
              )
              continue;

           if (IS_NPC(ch))
              {// Select extra_attack type
              
			   fight_mtrigger(ch); //из hit убрать эту функцию.

			   /*// переключение
               if ( MAY_LIKES(ch) &&
                    !AFF_FLAGGED(ch,AFF_CHARM) &&
                    GET_REAL_INT(ch) > number(15,25)
                  )
                 perform_mob_switch(ch);
			   */
               // Cast spells разобраться в пункте себялечения (доделать)
               if (MAY_LIKES(ch))
                  mob_casting(ch);
               
			   if (!FIGHTING(ch)		 	||
                   	IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))  	||
		   	AFF_FLAGGED(ch, AFF_HOLD)             	|| // mob_casting мог от зеркала отразиться
                   	AFF_FLAGGED(ch, AFF_STOPFIGHT)        	||
	                !AWAKE(ch)			 	||
			AFF_FLAGGED(ch, AFF_HOLDALL)
                  )
                  continue;

               if (AFF_FLAGGED(ch, AFF_CHARM)         &&
                   AFF_FLAGGED(ch, AFF_HELPER)        &&
                   ch->master && !IS_NPC(ch->master)  &&
				   CAN_SEE(ch, ch->master)            &&
                   IN_ROOM(ch) == IN_ROOM(ch->master) &&
					AWAKE(ch)
                  )
                  {for (vict = world[IN_ROOM(ch)].people; vict;
                        vict = vict->next_in_room)
                       if (FIGHTING(vict) == ch->master &&
                           vict           != ch)
                          break;
                   if (vict && GET_SKILL(ch, SKILL_RESCUE))
                      go_rescue(ch,ch->master,vict);
                   else
                   if (vict && GET_SKILL(ch, SKILL_PROTECT))
                      go_protect(ch,ch->master);
                  }
               else
	       if (!AFF_FLAGGED(ch, AFF_CHARM))
               for (sk_num = 0, sk_use = GET_REAL_INT(ch)
	            /*,sprintf(buf,"{%d}-{%d}\r\n",sk_use,GET_WAIT(ch))*/
		    /*,send_to_char(buf,FIGHTING(ch))*/;
                    MAY_LIKES(ch) && sk_use > 0;
                    sk_use--)
                   {do_this = number(0,100);
                    if (do_this > GET_LIKES(ch))
                       continue;
                    do_this = number(0,100);
		    //sprintf(buf,"<%d>\r\n",do_this);
		    //send_to_char(buf,FIGHTING(ch));
                    if (do_this < 10)
                       sk_num = SKILL_BASH;
                    else
                    if (do_this < 20)
                       sk_num = SKILL_DISARM;
                    else
                    if (do_this < 30)
                       sk_num = SKILL_KICK;
                    else
                    if (do_this < 40)
                       sk_num = SKILL_PROTECT;
                    else
                    if (do_this < 50)
                       sk_num = SKILL_RESCUE;
                    else
                    if (do_this < 60 && !TOUCHING(ch))
                       sk_num = SKILL_TOUCH;
                    else
                    if (do_this < 70)
                       sk_num = SKILL_CHOPOFF;
                    else
                       sk_num = SKILL_BASH;
                    if (GET_SKILL(ch,sk_num) <= 0)
                       sk_num = 0;
                    if (!sk_num)
                       continue;
                    //else
                    //   act("Victim prepare to skill '$F'.",FALSE,FIGHTING(ch),0,skill_name(sk_num),TO_CHAR);
                    /* Если умеет метать и вооружен метательным, то должен метнуть */
                    if (GET_EQ(ch, WEAR_WIELD))
		      if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD), ITEM_THROWING))
		        if (GET_REAL_INT(ch) > number(1,36))
                          sk_num = SKILL_THROW;

                    if (sk_num == SKILL_TOUCH)
                       {sk_use = 0;
                        go_touch(ch,FIGHTING(ch));
                       }
		       
                    if (sk_num == SKILL_THROW)
                       {sk_use = 0;
		        /* Цель выбираем по рандому */
                        for(vict = world[ch->in_room].people, i = 0; vict;
                            vict = vict->next_in_room) {
                              if (!IS_NPC(vict))
			        i++;
                        }
			if (i > 0) {
			 caster = NULL;
			 i = number(1,i);
                         for(vict = world[ch->in_room].people; i; 
			     vict = vict->next_in_room) {
                              if (!IS_NPC(vict)) {
			        i--;
				caster = vict;
			      }
                          }
			}
			/* Метаем */
                        if (caster)
                         go_throw(ch,caster);
                       }
		    
                    if (sk_num == SKILL_RESCUE || sk_num == SKILL_PROTECT)
                       { CHAR_DATA *attacker;
						   int dumb_mob;
						   caster = NULL;
						   damager = NULL;
						dumb_mob = (int) (GET_REAL_INT(ch) < number(5, 20));
							for (attacker = world[IN_ROOM(ch)].people;
							     attacker; attacker = attacker->next_in_room) {
								vict = FIGHTING(attacker);	// выяснение жертвы
								if (!vict ||	// жертвы нет
								    (!IS_NPC(vict) || AFF_FLAGGED(vict, AFF_CHARM) || AFF_FLAGGED(vict, AFF_HELPER)) ||	// жертва - не моб
								    (IS_NPC(attacker) &&
								     !(AFF_FLAGGED(attacker, AFF_CHARM)
								       && attacker->master && !IS_NPC(attacker->master))
								    /* && !(MOB_FLAGGED(attacker, MOB_ANGEL)
									  && attacker->master
									  && !IS_NPC(attacker->master))*/
								     //не совсем понятно, зачем это было && !AFF_FLAGGED(attacker,AFF_HELPER)
								    ) ||	// свои атакуют (мобы)
								    !CAN_SEE(ch, vict) ||	// не видно, кого нужно спасать
								    ch == vict	// себя спасать не нужно
								    )
									continue;

								// Буду спасать vict от attacker
								if (!caster ||	// еще пока никого не спасаю
								    (GET_HIT(vict) < GET_HIT(caster))	// этому мобу хуже
								    ) {
									caster = vict;
									damager = attacker;
									if (dumb_mob)
								  	    break;	// тупой моб спасает первого
								}
							}
			
                        if (sk_num == SKILL_RESCUE && caster && damager && CAN_SEE(ch, caster))
                           {sk_use = 0;
                            go_rescue(ch,caster,damager);
                           }
                        if (sk_num == SKILL_PROTECT && caster && CAN_SEE(ch, caster))
                           {sk_use = 0;
                            go_protect(ch, caster);
                           }
                       }

                    if (sk_num == SKILL_BASH || sk_num == SKILL_CHOPOFF || sk_num == SKILL_DISARM)
                       {caster = NULL;
                        damager= NULL;
                        if (GET_REAL_INT(ch) < number(15,25))
                           {caster = FIGHTING(ch);
                            damager= FIGHTING(ch);
                           }
                        else
                           {for(vict = world[ch->in_room].people; vict;
                                vict = vict->next_in_room)
                               {if ((IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_CHARM)) ||
                                    !FIGHTING(vict))
                                   continue;
                                if (((AFF_FLAGGED(vict, AFF_HOLD) || AFF_FLAGGED(vict, AFF_HOLDALL)) && GET_POS(vict) < POS_FIGHTING) ||
                                    (IS_CASTER(vict) &&
                                     (AFF_FLAGGED(vict, AFF_HOLD) || AFF_FLAGGED(vict, AFF_HOLDALL) || AFF_FLAGGED(vict, AFF_SIELENCE) || GET_WAIT(vict) > 0)
                                    )
                                   )
                                  continue;
                               if (!caster  ||
                                   (IS_CASTER(vict) && GET_CASTER(vict) > GET_CASTER(caster))
                                  )
                                  caster  = vict;
                               if (!damager || GET_DAMAGE(vict) > GET_DAMAGE(damager))
                                  damager = vict;
                              }
                           }
                        if (caster &&
			    (CAN_SEE(ch, caster) || FIGHTING(ch) == caster) &&
			    GET_CASTER(caster) > POOR_CASTER &&
                            (sk_num == SKILL_BASH || sk_num == SKILL_CHOPOFF)
                           )
                           {if (sk_num == SKILL_BASH)
                               {if (GET_POS(caster) >= POS_FIGHTING ||
                                    calculate_skill(ch,SKILL_BASH,200,caster) > number(50, 80)
                                   )
                                   {sk_use = 0;
                                    go_bash(ch, caster);
                                   }
                               }
                            else
                               {if (GET_POS(caster) >= POS_FIGHTING ||
                                    calculate_skill(ch,SKILL_CHOPOFF,200,caster) > number(50,80)
                                   )
                                   {sk_use = 0;
                                    go_chopoff(ch, caster);
                                   }
                               }
                           }
               if (sk_use &&  damager &&  (CAN_SEE(ch, damager) ||
					FIGHTING(ch) == damager))
                       {if (sk_num == SKILL_BASH)
                               {if (on_horse(damager))
                                    {sk_use = 0;
                                     go_bash(ch, get_horse(damager));
                                    }
                                else
                                if (GET_POS(damager) >= POS_FIGHTING ||
                                    calculate_skill(ch,SKILL_BASH,200,damager) > number(50,80)
                                   )
                                   {sk_use = 0;
                                    go_bash(ch, damager);
                                   }
								}
							else
							if (sk_num == SKILL_CHOPOFF)
                               {if (on_horse(damager))
                                   {sk_use = 0;
                                    go_chopoff(ch, get_horse(damager));
                                   }
                                else
                                if (GET_POS(damager) >= POS_FIGHTING ||
                                    calculate_skill(ch,SKILL_CHOPOFF,200,damager) > number(50,80)
                                   )
                                   {sk_use = 0;
                                    go_chopoff(ch, damager);
                                   }
                               }
                            else
                            if (sk_num == SKILL_DISARM &&
                                (GET_EQ(damager,WEAR_WIELD) ||
                                 GET_EQ(damager,WEAR_BOTHS) ||
                                 GET_EQ(damager,WEAR_HOLD)))
                                {sk_use = 0;
                                 go_disarm(ch, damager);
                                }
                           }
                       }

                    if (sk_num == SKILL_KICK && !on_horse(FIGHTING(ch)))
                       {sk_use = 0;
                        go_kick(ch,FIGHTING(ch));
                       }
                   }

               if (!FIGHTING(ch) ||
                   IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))
                  )
                  continue;

               /***** удар основным оружием или рукой */
               if (!AFF_FLAGGED(ch, AFF_STOPRIGHT))
                  exthit(ch, TYPE_UNDEFINED, 1);

               /***** экстраатаки */
               for(i = 1; i <= ch->mob_specials.extra_attack; i++)
                  {if (AFF_FLAGGED(ch, AFF_STOPFIGHT) ||
                       (i == 1 && AFF_FLAGGED(ch, AFF_STOPLEFT)))
                      continue;
                   exthit(ch, TYPE_UNDEFINED, i+1);
                  }
              }
           else /* PLAYERS - only one hit per round */
              {if (GET_POS(ch) > POS_STUNNED  &&
                   GET_POS(ch) < POS_FIGHTING &&
                   GET_AF_BATTLE(ch, EAF_STAND))
                  {sprintf(buf,"%sВам лучше встать на ноги!%s\r\n",
                               CCWHT(ch,C_NRM),CCNRM(ch,C_NRM));
                   send_to_char(buf, ch);
                   CLR_AF_BATTLE(ch, EAF_STAND);
                  }

               if (GET_CAST_SPELL(ch) && GET_WAIT(ch) <= 0)
                  {if (AFF_FLAGGED(ch, AFF_SIELENCE))
                      send_to_char("Вы не смогли произнести и слова.\r\n",ch);
                   else
                      { cast_spell(ch, GET_CAST_CHAR(ch), GET_CAST_OBJ(ch), GET_CAST_SPELL(ch));
                       	if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) || CHECK_WAIT(ch)))
			  {  WAIT_STATE(ch, PULSE_VIOLENCE);
                     	     SET_CAST(ch, 0, NULL, NULL);
         	          }
		      }
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
                  continue;

               if (GET_EXTRA_SKILL(ch) == SKILL_THROW &&
                   GET_EXTRA_VICTIM(ch) &&
                   GET_WAIT(ch) <= 0)
                  {go_throw(ch,GET_EXTRA_VICTIM(ch));
                    SET_EXTRA(ch,0,NULL);
                    if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }


               if (GET_EXTRA_SKILL(ch) == SKILL_BASH &&
                   GET_EXTRA_VICTIM(ch) &&
                   GET_WAIT(ch) <= 0)
                  {go_bash(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                     if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               if (GET_EXTRA_SKILL(ch) == SKILL_KICK &&
                   GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0)
                  {go_kick(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               if (GET_EXTRA_SKILL(ch) == SKILL_CHOPOFF &&
                   GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0)
                  {go_chopoff(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                    if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               if (GET_EXTRA_SKILL(ch) == SKILL_DISARM &&
                   GET_EXTRA_VICTIM(ch) && GET_WAIT(ch) <= 0)
                  {go_disarm(ch,GET_EXTRA_VICTIM(ch));
                   SET_EXTRA(ch,0,NULL);
                    if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }
		
               if (!FIGHTING(ch) ||
                   IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))
                  )
                  continue;
               /***** удар основным оружием или рукой */
               if (GET_AF_BATTLE(ch, EAF_FIRST))
                  {if (!AFF_FLAGGED(ch, AFF_STOPRIGHT) &&
                       (IS_IMMORTAL(ch) ||
                        GET_GOD_FLAG(ch, GF_GODSLIKE) ||
                        !GET_AF_BATTLE(ch, EAF_USEDRIGHT)
                       )
                      )
                      exthit(ch, TYPE_UNDEFINED, 1);
                   CLR_AF_BATTLE(ch, EAF_FIRST);
                   SET_AF_BATTLE(ch, EAF_SECOND);
                   if (INITIATIVE(ch) > min_init)
                      {INITIATIVE(ch)--;
                       continue;
                      }
                  }

               /***** удар вторым оружием если оно есть и умение позволяет */
               if ( GET_EQ(ch,WEAR_HOLD)                               &&
                    GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_WEAPON &&
                    GET_AF_BATTLE(ch, EAF_SECOND)                      &&
                    !AFF_FLAGGED(ch, AFF_STOPLEFT)                     &&
                    (IS_IMMORTAL(ch) ||
                     GET_GOD_FLAG(ch,GF_GODSLIKE) ||
                     GET_SKILL(ch,SKILL_SECOND_ATTACK) > number(1,101)
                    )
                  )
                  {if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE) ||
                       !GET_AF_BATTLE(ch, EAF_USEDLEFT))
                      exthit(ch, TYPE_UNDEFINED, 2);
                   CLR_AF_BATTLE(ch, EAF_SECOND);
                  }
               else
               /***** удар второй рукой если она свободна и умение позволяет */
               if ( !GET_EQ(ch,WEAR_HOLD)   && !GET_EQ(ch,WEAR_LIGHT)        &&
                    !GET_EQ(ch,WEAR_SHIELD) && !GET_EQ(ch,WEAR_BOTHS)        &&
                    !AFF_FLAGGED(ch,AFF_STOPLEFT)                            &&
                    GET_AF_BATTLE(ch, EAF_SECOND)                            &&
                    GET_SKILL(ch,SKILL_SHIT)
                  )
                  {if (IS_IMMORTAL(ch) || !GET_AF_BATTLE(ch, EAF_USEDLEFT))
                      exthit(ch, TYPE_UNDEFINED, 2);
                   CLR_AF_BATTLE(ch, EAF_SECOND);
                  }
              }
          }

  /* Decrement mobs lag */
  for (ch = combat_list; ch; ch = ch->next_fighting)
      {if (IN_ROOM(ch) == NOWHERE)
          continue;

       CLR_AF_BATTLE(ch, EAF_FIRST);
       CLR_AF_BATTLE(ch, EAF_SECOND);
       CLR_AF_BATTLE(ch, EAF_USEDLEFT);
       CLR_AF_BATTLE(ch, EAF_USEDRIGHT);
       CLR_AF_BATTLE(ch, EAF_MULTYPARRY);
       if (GET_AF_BATTLE(ch, EAF_SLEEP))
          affect_from_char(ch, SPELL_SLEEP);
     	  /*  if (GET_AF_BATTLE(ch, EAF_BLOCK))
          {CLR_AF_BATTLE(ch, EAF_BLOCK);
           if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
            WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
          }
       if (GET_AF_BATTLE(ch, EAF_DEVIATE))
          {CLR_AF_BATTLE(ch, EAF_DEVIATE);
	   if (!WAITLESS(ch) && GET_WAIT(ch) < PULSE_VIOLENCE)
	      WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
          }	*/	
       battle_affect_update(ch);
      }
}

// returns 1 if only ch was outcasted  перенести в линух 21.03.2007 г. проверка и наведение чармистов.
// returns 2 if only victim was outcasted
// returns 4 if both were outcasted
// returns 0 if none was outcasted  
int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim)
{
	CHAR_DATA *cleader, *vleader;
	int return_value = 0;
	if (ch == victim)
		return return_value;
// translating pointers from charimces to their leaders
	if (IS_NPC(ch) && ch->master && (AFF_FLAGGED(ch, AFF_CHARM) || IS_HORSE(ch)))
		ch = ch->master;
	if (IS_NPC(victim) && victim->master &&
	    (AFF_FLAGGED(victim, AFF_CHARM) || IS_HORSE(victim)))
		victim = victim->master;
	cleader = ch;
	vleader = victim;
// finding leaders
	while (cleader->master) {
		if (IS_NPC(cleader) &&
		    !AFF_FLAGGED(cleader, AFF_CHARM) && !IS_HORSE(cleader))
			break;
		cleader = cleader->master;
	}
	while (vleader->master) {
		if (IS_NPC(vleader) &&
		    !AFF_FLAGGED(vleader, AFF_CHARM) && !IS_HORSE(vleader))
			break;
		vleader = vleader->master;
	}
	if (cleader != vleader)
		return return_value;


// finding closest to the leader nongrouped agressor
// it cannot be a charmice
	while (ch->master && ch->master->master) {
		if (!AFF_FLAGGED(ch->master, AFF_GROUP) && !IS_NPC(ch->master)) {
			ch = ch->master;
			continue;
		} else if (IS_NPC(ch->master)
			   && !AFF_FLAGGED(ch->master->master, AFF_GROUP)
			   && !IS_NPC(ch->master->master) && ch->master->master->master) {
			ch = ch->master->master;
			continue;
		} else
			break;
	}

// finding closest to the leader nongrouped victim
// it cannot be a charmice
	while (victim->master && victim->master->master) {
		if (!AFF_FLAGGED(victim->master, AFF_GROUP)
		    && !IS_NPC(victim->master)) {
			victim = victim->master;
			continue;
		} else if (IS_NPC(victim->master)
			   && !AFF_FLAGGED(victim->master->master, AFF_GROUP)
			   && !IS_NPC(victim->master->master)
			   && victim->master->master->master) {
			victim = victim->master->master;
			continue;
		} else
			break;
	}
	if (!AFF_FLAGGED(ch, AFF_GROUP) || cleader == victim) {
		stop_follower(ch, SF_EMPTY);
		return_value |= 1;
	}
	if (!AFF_FLAGGED(victim, AFF_GROUP) || vleader == ch) {
		stop_follower(victim, SF_EMPTY);
		return_value |= 2;
	}
	return return_value;
}
