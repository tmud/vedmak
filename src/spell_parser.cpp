/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
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
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "pk.h"
#include "screen.h"
#include "constants.h"
#include "clan.h"

struct spell_info_type   spell_info[TOP_SPELL_DEFINE + 1];
struct skill_info_type   skill_info[MAX_SKILLS + 2];
/*struct char_spell_info char_spell[TOP_SPELL_DEFINE + 1];*/
extern int what_sky;
#define SINFO spell_info[spellnum]
#define SpINFO  SINFO
#define SkINFO skill_info[skillnum]
char   cast_argument[MAX_INPUT_LENGTH];
extern struct room_data *world;
extern struct char_data *character_list;
struct spell_create_type spell_create[TOP_SPELL_DEFINE + 1];
char SpelsInSlot[13];

// extern function
int  attack_best(struct char_data *ch, struct char_data *victim);
int  check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int extract);
int  check_recipe_values(struct char_data * ch, int spellnum, int spelltype, int showrecipe);
int  invalid_no_class(struct char_data *ch, struct obj_data *obj);
void pk_translate_pair(struct char_data * * pkiller, struct char_data * * pvictim);

// local functions 
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch, struct obj_data * tobj);
void spello(int spl, const char *name, const char *syn, int max_mana, int min_mana, int mana_change, 
			int minpos, int targets, int violent, int routines, int danger);
void skillo(int spl, const char *name, int max_percent);
int mag_manacost(struct char_data * ch, int spellnum);
int  equip_in_metall(struct char_data *ch);
int get_moon(int sky, int mode);
ACMD(do_cast);
ACMD(do_ident);
ACMD(do_razd);
ACMD(do_remember);
ACMD(do_forget);
void list_spells(CHAR_DATA * ch, CHAR_DATA * vict);
void unused_spell(int spl);
void unused_skill(int spl);
void mag_assign_spells(void);
int check_pkill(struct char_data *ch, struct char_data *victim, char *arg);
/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

const char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */

// ������� ��� ������� ����������
const int TablKrugSlot [MAX_KRUG][16] = {
	{	1,	2,	3,	4,	5,	6,	8,	15,	25,	36,	48,	70, 100,140,1400,1600	},
	{	2,	4,	6,	8,	10,	12,	15,	27,	40,	60,	80, 100,120,160,1500,1700	},	
	{	3,	6,	9,	12,	15,	19,	24,	30,	57,	69, 85,	110,130,180,1600,1800	},		
	{	4,	8,	12,	16,	20,	25,	31,	38,	66, 77,	87,	120,140,200,1700,1900	},			
	{	5,	10,	15,	20,	26,	33,	41,	50,	70, 88,	95,	130,150,210,1800,2000	}, 			
	{	6,	12,	18,	24,	31,	39,	48,	58, 89,	99,	101,144,163,220,1890,2130	},				
	{	7,	14,	21,	29,	38,	48,	59,	71,	94,	108,113,159,176,230,1940,2160	},					
	{	8,	16,	24,	33,	43,	54,	66,	79,	103,118,124,161,189,240,1980,2190	},						
	{	9,	18,	28,	39,	51,	64,	78,	93,	119,126,144,173,193,250,2260,2490	}							
};


#define KASTER_CRUG(ch)    (i * 10/int_app[POSI(GET_REAL_INT(ch))].maxkrug)

int slot_for_circle(struct char_data * ch, int point_slot, int GET_CASTER_KRUG)
{   int mage_max_krug=0, i, KRUG;
   
	
		switch (GET_CLASS(ch))
		{ case CLASS_BATTLEMAGE :
	           KRUG = GET_CASTER_KRUG(ch);
		   break;
	      case CLASS_CLERIC :
          case CLASS_DRUID :
			  KRUG = GET_CASTER_KRUG(ch) < 7 ? GET_CASTER_KRUG(ch) :7;
		   break;
          
		  case CLASS_ASSASINE :
              KRUG = GET_CASTER_KRUG(ch) < 4 ? GET_CASTER_KRUG(ch) :4; 
           break;

		  case CLASS_TAMPLIER :
              KRUG = GET_CASTER_KRUG(ch) < 3 ? GET_CASTER_KRUG(ch) :3; 
           break;

          case CLASS_MONAH :
          case CLASS_SLEDOPYT :  
              KRUG = GET_CASTER_KRUG(ch) < 2 ? GET_CASTER_KRUG(ch) :2;  
           break;
		  default: 
			  KRUG =0;
			  break;
		break;
	}
       for (i=0; i <= KRUG; i++)
	  {     
	  mage_max_krug=GET_CASTER_KRUG;
            if (mage_max_krug >= 8)
			{ if (point_slot>TablKrugSlot[8][SpelsInSlot[8]])
				{point_slot -=TablKrugSlot[8][SpelsInSlot[8]];
				 SpelsInSlot[8] ++;
               	}
				mage_max_krug--;
			}
	  
	  
	        if (mage_max_krug >= 7)
			{ if (point_slot>TablKrugSlot[7][SpelsInSlot[7]])
				{point_slot -=TablKrugSlot[7][SpelsInSlot[7]];
				 SpelsInSlot[7] ++;
               	}
				mage_max_krug--;
			}
					   
		    if (mage_max_krug >= 6)
			{ if (point_slot>TablKrugSlot[6][SpelsInSlot[6]])
				{point_slot -=TablKrugSlot[6][SpelsInSlot[6]];
			     SpelsInSlot[6] ++;
				}
				 mage_max_krug--;
			}

		    if (mage_max_krug >= 5)
			{ if (point_slot>TablKrugSlot[5][SpelsInSlot[5]])
				{point_slot -=TablKrugSlot[5][SpelsInSlot[5]];
				 SpelsInSlot[5] ++;
				}
				mage_max_krug--;
			}
			
			if (mage_max_krug >= 4)
			{ if (point_slot>TablKrugSlot[4][SpelsInSlot[4]])
				{point_slot -=TablKrugSlot[4][SpelsInSlot[4]];
				 SpelsInSlot[4] ++;
				}
				mage_max_krug--;
			}
			
			if (mage_max_krug >= 3)
			{ if (point_slot>TablKrugSlot[3][SpelsInSlot[3]])
				{point_slot -=TablKrugSlot[3][SpelsInSlot[3]];
				 SpelsInSlot[3] ++;
				}
				 mage_max_krug--;
			}

            if (mage_max_krug >= 2)
			{ if (point_slot>TablKrugSlot[2][SpelsInSlot[2]])
				{point_slot -=TablKrugSlot[2][SpelsInSlot[2]];
				 SpelsInSlot[2] ++;
				}
				 mage_max_krug--;
				
			}

            if (mage_max_krug >= 1)
			{ if (point_slot>TablKrugSlot[1][SpelsInSlot[1]])
				{point_slot -=TablKrugSlot[1][SpelsInSlot[1]];
				 SpelsInSlot[1] ++;
				}
				 mage_max_krug--;
			}

			 if (mage_max_krug >= 0)
			{ if (point_slot>TablKrugSlot[0][SpelsInSlot[0]])
				{point_slot -=TablKrugSlot[0][SpelsInSlot[0]];
				 SpelsInSlot[0] ++;
				}
			}
	  }
return point_slot;
}

int slot_for_char1(struct char_data * ch, int slot_num)
{  int  i, mage_max_krug=0, point_slot=0;
  
   if ( GET_LEVEL(ch) < 1 || IS_NPC(ch))
         return -1;
 
 if (IS_IMMORTAL(ch))
    return 16;

    slot_num--;
		
	for (i=0; i <= 12; i++)
	SpelsInSlot[i] = 0;

	switch (GET_CLASS(ch))
		{ case CLASS_BATTLEMAGE :
	            for (i = 1; i <= GET_LEVEL(ch); i++)		
				{ point_slot = GET_LEVEL(ch) + point_slot +
					           wis_app[GET_REAL_WIS(ch)].slot_bonus;
				  point_slot = slot_for_circle (ch, point_slot, KASTER_CRUG(ch));
				}
		   break;
	      case CLASS_CLERIC :
          case CLASS_DRUID :
				for (i = 1; i <= GET_LEVEL(ch); i++)		
				{ point_slot = GET_LEVEL(ch) + point_slot +
					           wis_app[GET_REAL_WIS(ch)].slot_bonus;
				  point_slot = slot_for_circle (ch, point_slot, (KASTER_CRUG(ch) < 7) ? KASTER_CRUG(ch): 7);
				}
		   break;
         
		  case CLASS_ASSASINE :
                 for (i = 1; i <= GET_LEVEL(ch); i++)		
				{ point_slot = GET_LEVEL(ch) + point_slot +
					           wis_app[GET_REAL_WIS(ch)].slot_bonus;
				  point_slot /=3;
				  point_slot = slot_for_circle (ch, point_slot, (KASTER_CRUG(ch)-1 <5) ? KASTER_CRUG(ch)-1: 4);
				}
           break;

		  case CLASS_SLEDOPYT :
		  case CLASS_TAMPLIER :
                 for (i = 1; i <= GET_LEVEL(ch); i++)		
				{ point_slot = GET_LEVEL(ch) + point_slot +
					           wis_app[GET_REAL_WIS(ch)].slot_bonus;
				  point_slot /=3;
				  point_slot = slot_for_circle (ch, point_slot, (KASTER_CRUG(ch)-2 <4) ? KASTER_CRUG(ch)-2: 3);
				}
           break;

          case CLASS_MONAH :
                 for (i = 1; i <= GET_LEVEL(ch); i++)		
				{ point_slot = GET_LEVEL(ch) + point_slot +
					           wis_app[GET_REAL_WIS(ch)].slot_bonus;
			        point_slot /=3;
				 point_slot = slot_for_circle (ch, point_slot, (KASTER_CRUG(ch)-3 <3) ? KASTER_CRUG(ch)-3: 2);
				}
           break;
		  default: break;
		break;
	}
          
  int slots = (SpelsInSlot[slot_num] ? MIN(18, SpelsInSlot[slot_num] + GET_SLOT(ch,slot_num) + GET_REMORT(ch)) : 0);
  switch (GET_CLASS(ch))
  {
     case CLASS_SLEDOPYT :
     case CLASS_MONAH :
         {
             if (GET_LEVEL(ch) >= 16 && (slot_num == 1 || slot_num == 2))
             {
                 if (slots == 0)
                     slots = 1;
             }
         }
     break;
  } 
  return slots;
}
		

int mag_manacost(struct char_data * ch, int spellnum)
{ 
return MAX(SINFO.mana_max - (SINFO.mana_change *
	   (1 + GET_CASTER_KRUG(ch) + GET_LEVEL(ch)/4 -
	   SINFO.slot_forc[(int) GET_CLASS(ch)])),
	   SINFO.mana_min);
}

int znak_manacost(struct char_data * ch, int spellnum)
{
  if (GET_LEVEL(ch) <= SpINFO.slot_forc[(int) GET_CLASS(ch)])
     return SpINFO.mana_max;

  return MAX(SpINFO.mana_max - (SpINFO.mana_change *
                     (GET_LEVEL(ch) - SpINFO.slot_forc[(int) GET_CLASS(ch)])),
                 SpINFO.mana_min);
}

/* say_spell erodes buf, buf1, buf2 */

void say_spell(struct char_data * ch, int spellnum, struct char_data * tch,
	            struct obj_data * tobj)
{
        char *say_to_self, *say_to_other, *say_to_obj_vis, *say_to_something,
             *helpee_vict, *damagee_vict;
  const char *format;

  struct char_data *i;

  *buf = '\0';
  strcpy(buf, SpINFO.syn);

  if (!IS_NPC(ch) || GET_CLASS(ch)==CLASS_HUMAN)
  {     say_to_self      = "$n ������$y ���� ����� � ���������$y �����: \'%s\'"; 
        say_to_other     = "$n ���������� ���������$y �� $V � ���������$y �����: \'%s\'";
        say_to_obj_vis   = "$n ����� �������� ������������$u �� $5 � ��������$q: \'%s\'";
     	  say_to_something = "$n ���������$y �����: \'%s\'"; 
        damagee_vict     = "$n ��������$y �� ��� �� ������ � ���������$y: \'%s\'";
        helpee_vict      = "����������� ���, $n ��������$q: \'%s\'";
  }
  else
  {     say_to_self      = "$n ��������$q ����� ������� �� ���������� \'%s\'"; 
        say_to_other     = "$n ���������� ���������$y �� $V � ��������$q ����� ������� �� ���������� \'%s\'";
        say_to_obj_vis   = "$n ���������$y �� $3 � ��������$q ����� ������� �� ���������� \'%s\'";
       	say_to_something = "$n ��������$q ����� ������� ��: \'%s\'"; 
        damagee_vict     = "$n ��������$y �� ��� �� ������ � ��������$q ����� ������� �� ���������� \'%s\'";
        helpee_vict      = "� ��������� ���������� ����, $n ��������$q ����� ������� �� ���������� \'%s\'";
 }   
 
 
  if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch))
  {  if (tch == ch)
      format = say_to_self;
     else
	 if (AFF_FLAGGED(ch, AFF_BLIND))
      format = say_to_something;
     else 
	  format = say_to_other;
  } 
  else
	  if (tobj != NULL &&
	     ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
        format = say_to_obj_vis;
      else
        format = say_to_something; 

  sprintf(buf1, format, spell_name(spellnum));
  sprintf(buf2, format, buf);

//�� ���� �� �������?
  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
    if (i == ch || i == tch || !i->desc || !AWAKE(i))
      continue;
   	
    if (IS_SET(GET_SPELL_TYPE(i, spellnum), SPELL_KNOW)) 
		perform_act(buf1, ch, tobj, tch, i);
    else
		perform_act(buf2, ch, tobj, tch, i);
  }

  if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch))
  {   if (SpINFO.violent)
	 { if (AFF_FLAGGED(ch, AFF_BLIND))
	       sprintf(buf1, say_to_something,
		   IS_SET(GET_SPELL_TYPE(tch,spellnum),SPELL_KNOW) ? spell_name(spellnum) : buf);
	  else
	       sprintf(buf1, damagee_vict,
	       IS_SET(GET_SPELL_TYPE(tch,spellnum),SPELL_KNOW) ? spell_name(spellnum) : buf);
	}
  else 
	  sprintf(buf1, helpee_vict,
	       IS_SET(GET_SPELL_TYPE(tch,spellnum),SPELL_KNOW) ? spell_name(spellnum) : buf);
	  act(buf1, FALSE, ch, NULL, tch, TO_VICT);
  }
}




/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */
const char *skill_name(int num)
{
  if (num > 0 && num <= MAX_SKILLS)
    return (skill_info[num].name);
  else if (num == -1)
    return ("UNUSED");
  else
    return ("UNDEFINED");
}

	 
const char *spell_name(int num)
{
  if (num > 0 && num <= TOP_SPELL_DEFINE)
      return (spell_info[num].name);
  else
  if (num == -1)
    return "UNUSED";
  else
    return "UNDEFINED";
}


int find_skill_num(const char *name)
{
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256];
  char buffer[256];

  for (index = 1; index <= MAX_SKILLS; index++) {
	  if (skill_info[index].name == unused_spellname) continue;
    if (is_abbrev(name, skill_info[index].name))
      return (index);

    ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    strcpy(buffer, skill_info[index].name);
    temp = any_one_arg(buffer, first);

    strcpy(buffer, name);
    temp2 = any_one_arg(buffer, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
	ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return (index);
  }

  return (-1);
}


int find_spell_num(const char *name)
{
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256], *realname;
  char buffer[256];

  for (index = 1; index <= TOP_SPELL_DEFINE; index++)
      {
       realname = ((ubyte)*name <= (ubyte)'z') ? (char *) spell_info[index].syn : (char *) spell_info[index].name;

       if (!realname || !*realname)
          continue;
       if (is_abbrev(name, realname))
          return (index);
       ok = TRUE;
       /* It won't be changed, but other uses of this function elsewhere may. */
       temp = any_one_arg((char *)realname, first);

       strcpy(buffer, name);
       temp2 = any_one_arg(buffer, first2);
       while (*first && *first2 && ok)
             {if (!is_abbrev(first2, first))
                     ok = FALSE;
              temp = any_one_arg(temp, first);
              temp2 = any_one_arg(temp2, first2);
             }
       if (ok && !*first2)
          return (index);

       }
  return (-1);
}


int may_cast_in_nomagic(struct char_data *caster, struct char_data *victim,
                        int spellnum)
{
  // More than 33 level - may cast always
  if (IS_GRGOD(caster))
     return (TRUE);


   if (IS_NPC(caster) && !AFF_FLAGGED(caster,AFF_CHARM))
       return TRUE;


  return (FALSE);
}


// �����-�� ������ �������� ����������? 
int may_cast_here( 
  struct char_data *caster, 
  struct char_data *victim,
  int spellnum
)
{
  int ignore;
  struct char_data *   ch_vict;

  //��������� 48 ������� ����� ��� �������
  if ( IS_GRGOD(caster) ) 
    return TRUE;

  // �� � ����� ����� ��������� ��� ��� ������
  if ( !ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) )
    return TRUE;

  // ��������, ��� ���� ����� ���� �� ���������� ���������� ����������
  ignore = IS_SET(SpINFO.targets, TAR_IGNORE)  ||
           IS_SET(SpINFO.routines, MAG_MASSES) ||
           IS_SET(SpINFO.routines, MAG_GROUPS);

  // ���� ���
  if ( !ignore && !victim )
    return TRUE;

  if ( ignore && 
       !IS_SET(SpINFO.routines, MAG_MASSES) &&
       !IS_SET(SpINFO.routines, MAG_GROUPS)
     ) 
  {
    if ( SpINFO.violent ) return FALSE; // ������ ���� ���������
    // ���� ������������ ����, �� ������ ���� GROUP ��� MASS
    // � ��������� ������ �� ���� ��������� ������   
    return victim == 0 ? TRUE: FALSE;
  }

  // ��������� ���������� �� ��������

  // �������������� ����
  ignore = victim&&
           (IS_SET(SpINFO.targets,TAR_CHAR_ROOM)||
            IS_SET(SpINFO.targets,TAR_CHAR_WORLD))&&
           !IS_SET(SpINFO.routines, MAG_AREAS);

  // ������� ��������� ������� �����
  // ��� ������ ���������� - �������� �� ���������� ����
  // ��� ���� ���������� - �������� �� ����
  for ( ch_vict = world[caster->in_room].people; ch_vict; ch_vict = ch_vict->next_in_room )
  { // if ( IS_IMMORTAL( ch_vict ) ) continue; // ��������� �� ���� ������� �� ������
    if ( !HERE( ch_vict ) )
         continue; // �� �� ���������
   // if ( SpINFO.violent && same_group(caster,ch_vict) ) 
       //  continue; // �� ���������� ���������� �� �����
    if ( ignore && ch_vict != victim ) 
         continue; // ������ �� 1 ����, � �� ��������
    // ch_vict ����� ����� ������������� �������
    if ( SpINFO.violent )
    {
      if ( !may_kill_here( caster, ch_vict ) ) return false;
    }
    else
    {
      if ( !may_kill_here( caster, FIGHTING(ch_vict) ) ) return false;
    }
  }

  // check_pkill ������� �� ����, �.�. ��� ������ ������������ �������

  // ���! �����
  return (TRUE);
}


int check_mobile_list(struct char_data *ch)
{struct char_data *vict;

 for (vict = character_list; vict; vict = vict->next)
     if (vict == ch)
        return (TRUE);

 return (FALSE);
}

void cast_reaction(struct char_data *victim, struct char_data *caster, int spellnum)
{  struct char_data *temp = NULL;   

 if (!check_mobile_list(victim))// || !SpINFO.violent)
    return;

 if (AFF_FLAGGED(victim,AFF_CHARM)     ||
     AFF_FLAGGED(victim,AFF_SLEEP)     ||
     AFF_FLAGGED(victim,AFF_BLIND)     ||
     AFF_FLAGGED(victim,AFF_STOPFIGHT) ||
     AFF_FLAGGED(victim,AFF_HOLD)      ||
     AFF_FLAGGED(victim,AFF_HOLDALL)   ||
	 IS_HORSE(victim)
    )
    return;

 if (IS_NPC(caster) && GET_MOB_RNUM(caster) == real_mobile(DG_CASTER_PROXY))
    return;

   if (!SpINFO.violent)
	{ for (temp = world[IN_ROOM(victim)].people; temp; temp = temp->next_in_room)
	  {    if (!IS_NPC(temp))
             continue;
	     
        if (FIGHTING(temp) == victim 	   && 
	      !AFF_FLAGGED(temp,AFF_CHARM)     && 
          !IS_HORSE(temp)                  && 
		  !AFF_FLAGGED(temp,AFF_STOPFIGHT) && 
		  !AFF_FLAGGED(temp,AFF_HOLD)      && 
		  !MOB_FLAGGED(temp,MOB_NOFIGHT)   &&
                  !PLR_FLAGGED(temp, PLR_IMMKILL)  && 
		  !AFF_FLAGGED(temp,AFF_HOLDALL)   &&
		  GET_WAIT(temp) <= 0              && 
		  GET_POS(temp) >= POS_RESTING     &&
		  CAN_SEE(temp,caster)             &&
                  IN_ROOM(temp) == IN_ROOM(caster)
		  )
			{  attack_best(temp, caster);	
		   	   if (MOB_FLAGGED(temp, MOB_MEMORY))
				remember(temp, caster);
		     	         return;
			}	
	
		}
 }

    if (!SpINFO.violent)
          return;
 

 if (CAN_SEE(victim,caster)              &&
     MAY_ATTACK(victim)                  &&
     IN_ROOM(victim) == IN_ROOM(caster)
    )
    {if (IS_NPC(victim))
        attack_best(victim, caster);
     else
        hit(victim, caster, TYPE_UNDEFINED, 1);
    }
 else
 if (CAN_SEE(victim,caster) &&
     !IS_NPC(caster)        &&
     IS_NPC(victim)         &&
     MOB_FLAGGED(victim, MOB_MEMORY)
    )
    remember(victim, caster);
}


/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(struct char_data * caster,
               struct char_data * cvict,
               struct obj_data  * ovict,
               int spellnum,
               int level,
               int casttype,
               int use_glory)
{
  int savetype;

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
     return (0);

       if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) &&
          !may_cast_in_nomagic(caster, cvict, spellnum))
         {send_to_char("���� ����� ��������� ������� � ���������� �� �������.\r\n", caster);
          act("����� $r ��������� ������� � ���������� �� �������.", FALSE, caster, 0, 0, TO_ROOM);
          return (0);
         }
     
        if (cvict && (world[caster->in_room].zone == 4 || world[caster->in_room].zone == 5) &&
	   !IS_NPC(cvict) &&
	   !may_cast_here(caster, cvict, spellnum))
	   { send_to_char("�� �� ������ ��������� ������������ � �����!\r\n", caster);
         return (FALSE);
       } 
   
   if (!may_cast_here(caster, cvict, spellnum))
         {send_to_char("���� ����� ���������� ����� ���� � ����� �������!\r\n", caster);
          act("����� ������� �� ��� �������� ������� � ��� �� �������.", FALSE, caster, 0, 0, TO_ROOM);
          return (0);
         }
      

  // determine the type of saving throw 
  switch (casttype)
	{ case CAST_STAFF:
	  case CAST_SCROLL:
	  case CAST_POTION:
	  case CAST_WAND:
	  case CAST_ITEMS:
	  case CAST_RUNES:
	  case CAST_SPELL:
		savetype = SAVING_STABILITY;
		break;
	  default:
		savetype = SAVING_CRITICAL;
		break;
	}

  if (IS_SET(SpINFO.routines, MAG_DAMAGE))
     if (mag_damage(level, caster, cvict, spellnum, savetype, use_glory) == -1)
        return (-1);    /* Successful and target died, don't cast again. */

  if (IS_SET(SpINFO.routines, MAG_AFFECTS))
     mag_affects(level, caster, cvict, spellnum, savetype, use_glory);

  if (IS_SET(SpINFO.routines, MAG_UNAFFECTS))
     mag_unaffects(level, caster, cvict, spellnum, savetype, use_glory);

  if (IS_SET(SpINFO.routines, MAG_POINTS))
      mag_points(level, caster, cvict, spellnum, savetype, use_glory);

  if (IS_SET(SpINFO.routines, MAG_ALTER_OBJS))
     mag_alter_objs(level, caster, ovict, spellnum, savetype, use_glory);

  if (IS_SET(SpINFO.routines, MAG_GROUPS))
     mag_groups(level, caster, spellnum, savetype, use_glory);

  if (IS_SET(SpINFO.routines, MAG_MASSES))
     mag_masses(level, caster, spellnum, savetype, use_glory);

  if (IS_SET(SpINFO.routines, MAG_AREAS))
     mag_areas(level, caster, spellnum, savetype, use_glory);

  if (IS_SET(SpINFO.routines, MAG_SUMMONS))
     mag_summons(level, caster, ovict, spellnum, savetype, use_glory);

  if (IS_SET(SpINFO.routines, MAG_CREATIONS))
     mag_creations(level, caster, spellnum, use_glory);

  if (IS_SET(SINFO.routines, MAG_MANUAL))
     switch (spellnum)
     {case SPELL_CHARM:             MANUAL_SPELL(spell_charm); break;
      case SPELL_CREATE_WATER:      MANUAL_SPELL(spell_create_water); break;
      case SPELL_DETECT_POISON:     MANUAL_SPELL(spell_detect_poison); break;
      case SPELL_ENCHANT_WEAPON:    MANUAL_SPELL(spell_enchant_weapon); break;
      case SPELL_IDENTIFY:          MANUAL_SPELL(spell_identify); break;
      case SPELL_LOCATE_OBJECT:     MANUAL_SPELL(spell_locate_object); break;
      case SPELL_SUMMON:            MANUAL_SPELL(spell_summon); break;
      case SPELL_GROUP_RECALL:      MANUAL_SPELL(spell_group_recall); break;
      case SPELL_WORD_OF_RECALL:    MANUAL_SPELL(spell_recall); break;
      case SPELL_TELEPORT:          MANUAL_SPELL(spell_teleport); break;
      case SPELL_PORTAL:            MANUAL_SPELL(spell_portal); break;
      case SPELL_RELOCATE:          MANUAL_SPELL(spell_relocate); break;
      case SPELL_CHAIN_LIGHTNING:   MANUAL_SPELL(spell_chain_lightning); break;
      case SPELL_CONTROL_WEATHER:   MANUAL_SPELL(spell_control_weather); break;
      case SPELL_CREATE_WEAPON:     MANUAL_SPELL(spell_create_weapon); break;
      case SPELL_EVILESS:           MANUAL_SPELL(spell_eviless); break;
      case SPELL_RESTORE_MAGIC:     MANUAL_SPELL(spell_restore_magic); break;
     }
  cast_reaction(cvict,caster,spellnum);
  return (1);
}


ACMD(do_ident)
{ struct char_data *cvict = NULL, *caster = ch;
  struct obj_data  *ovict = NULL;
  struct timed_type timed;
  int    k, level = 0;

  if (IS_NPC(ch) || GET_SKILL(ch, SKILL_IDENTIFY) <= 0)
     {send_to_char("��� ����� ������� ����� ���������.\r\n", ch);
      return;
     }

  one_argument(argument, arg);

  if (timed_by_skill(ch, SKILL_IDENTIFY))
     {send_to_char("�� ���� �� ������ ��������� ��� ������.\r\n", ch);
      return;
     }

  k = generic_find(arg, FIND_CHAR_ROOM |
                        FIND_OBJ_INV   |
                        FIND_OBJ_ROOM  |
                        FIND_OBJ_EQUIP, caster, &cvict, &ovict);
  if (!k)
     {send_to_char("������, ����� ����� ���.\r\n", ch);
      return;
     }
  if (!IS_IMMORTAL(ch))
     {timed.skill    = SKILL_IDENTIFY;
      timed.time     = 1;
      timed_to_char(ch, &timed);
     }
  MANUAL_SPELL(skill_identify)
}


/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */
const char * what_sky_type[] =
 { "��������",
   "cloudless",
   "�������",
   "cloudy",
   "�����",
   "raining",
   "����",
   "lightning",
   "\n"
};

const char *what_weapon[] =
{ "�����",
  "whip",
  "������",
  "club",
  "\n"
};

 

 int find_cast_target(int spellnum, char *t, struct char_data *ch, struct char_data **tch, struct obj_data **tobj)
{
struct char_data *i;
struct obj_data *o;	 
 *tch  = NULL;
 *tobj = NULL;

 
 i = get_char_vis(ch, t, FIND_CHAR_ROOM);

  if (spellnum == SPELL_CONTROL_WEATHER)
     {if (!t || (what_sky = search_block(t, what_sky_type, FALSE)) < 0)
         {send_to_char("�� ������ ��� ������.\r\n",ch);
          return FALSE;
         }
      else
         what_sky >>= 1;
     }	

  if (spellnum == SPELL_CREATE_WEAPON)
     {if (!t || (what_sky = search_block(t, what_weapon, FALSE)) < 0)
         {send_to_char("�� ������ ��� ������.\r\n",ch);
          return FALSE;
         }
      else
         what_sky = 5 + (what_sky >> 1);
     }	

  *cast_argument = '\0';

  if (t)
     strcat(cast_argument,t);


 if (IS_SET(SpINFO.targets, TAR_IGNORE))
    return  TRUE;
 else
 if (t != NULL && *t) //�����������	 
    {if (IS_SET(SINFO.targets, TAR_CHAR_ROOM))
        {if ((*tch = get_char_vis(ch, t, FIND_CHAR_ROOM)) != NULL)
            {if (SpINFO.violent && check_pkill(ch,*tch,t))
	        return FALSE;
            if (IS_SET(SINFO.targets, TAR_OBJ_INV))
            for (o = i->carrying; o; o = o->next_content)
			{	if (IS_OBJ_STAT(o, ITEM_NODROP))
                obj_from_char(o);//*tobj = o;
            }
				return TRUE;
	//return TRUE;
            }
         }


      if (IS_SET(SINFO.targets, TAR_CHAR_WORLD))
         {if ((*tch = get_char_vis(ch, t, FIND_CHAR_WORLD)) != NULL)
	     {if (SpINFO.violent && check_pkill(ch,*tch,t))
	        return FALSE;
              return TRUE;
	     }
         }	

      if (IS_SET(SINFO.targets, TAR_OBJ_INV))
         if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != NULL)
            return TRUE;

      if (IS_SET(SINFO.targets, TAR_OBJ_EQUIP))
         {int i;
	  for (i = 0; i < NUM_WEARS; i++)
              if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name))
                 {*tobj = GET_EQ(ch, i);
                  return TRUE;
                 }
         }

      if (IS_SET(SpINFO.targets, TAR_OBJ_ROOM))
         if ((*tobj = get_obj_in_list_vis(ch, t, world[IN_ROOM(ch)].contents)) != NULL)
            return TRUE;

      if (IS_SET(SpINFO.targets, TAR_OBJ_WORLD))
         if ((*tobj = get_obj_vis(ch, t)) != NULL)
            return TRUE;
     }
  else
     {if (IS_SET(SpINFO.targets, TAR_FIGHT_SELF))
         if (FIGHTING(ch) != NULL)
            {*tch    = ch;
             return  TRUE;
            }
      if (IS_SET(SpINFO.targets, TAR_FIGHT_VICT))
         if (FIGHTING(ch) != NULL)
            {*tch    = FIGHTING(ch);
             return TRUE;
            }
      if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM) && !SpINFO.violent)
         {*tch    = ch;
          return TRUE;
         }
     }
  sprintf(buf, "�� %s �� ������ %s?\r\n",
          IS_SET(SpINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "���" : "����",
		  !IS_WEDMAK(ch) ? "��������� ��� �����": "������������ ���� ����");
  send_to_char(buf, ch);
  return FALSE;
}


void mag_objectmagic(struct char_data * ch, struct obj_data * obj,
		          char *argument)
{
  int i, k, spellnum;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		   FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_STAFF:
    act("�� ������� $4 � �����.", FALSE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
    else
      act("$n ������ $4 � �����.", FALSE, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      send_to_char("��� ���� ��� ��� ��� ����������!\r\n", ch); 
      act("������, �������, �� ���������.", FALSE, ch, obj, 0, TO_ROOM);
    } else {
      GET_OBJ_VAL(obj, 2)--;
      WAIT_STATE(ch, PULSE_VIOLENCE);
      /* Level to cast spell at. */
      k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

      /*
       * Problem : Area/mass spells on staves can cause crashes.
       * Solution: Remove the special nature of area/mass spells on staves.
       * Problem : People like that behavior.
       * Solution: We special case the area/mass spells here.
       */
      if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS)) {
        for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
	  i++;
	while (i-- > 0)
	  call_magic(ch, NULL, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
      } else {
	for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
	  next_tch = tch->next_in_room;
	  if (ch != tch)
	    call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
		}
      }
    }
    break;
  case ITEM_WAND:
   spellnum = GET_OBJ_VAL(obj, 3);
   if (!*arg)
       tch = ch;
    else
    if (!find_cast_target(spellnum,argument,ch,&tch,&tobj))
       return;

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      send_to_char("�� ��� ����� ��� ������������.\r\n", ch); 
      act("������ �� ���������.", FALSE, ch, obj, 0, TO_ROOM);
      return;
    }

    if (tch) {
      if (tch == ch) {
	act("�� ������� $4 �� ����.", FALSE, ch, obj, 0, TO_CHAR); 
	act("$n ������$y $4 �� ����.", FALSE, ch, obj, 0, TO_ROOM);  
      } else {
	act("�� ������� $4 �� $V.", FALSE, ch, obj, tch, TO_CHAR); 
	if (obj->action_description)
	  act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
	else
	  act("$n ������$y $4 �� $V.", TRUE, ch, obj, tch, TO_ROOM);
      }
    } else if (tobj != NULL) {
      act("�� ������� $4 �� $8.", FALSE, ch, obj, tobj, TO_CHAR);
      if (obj->action_description)
	act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
      else
	act("$n ������$y $4 �� $8.", TRUE, ch, obj, tobj, TO_ROOM);
    } else if (IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES)) {
      /* Wands with area spells don't need to be pointed. */
      act("�� ��������� $4 � �������.", FALSE, ch, obj, NULL, TO_CHAR); 
      act("$n ��������$y $4 � �������.", TRUE, ch, obj, NULL, TO_ROOM);
    } else {
      act("� ���� �� ������ ������������ $4?", FALSE, ch, obj, NULL, TO_CHAR);
      return;
    }

    
    GET_OBJ_VAL(obj, 2)--;
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (GET_OBJ_VAL(obj, 0))
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 GET_OBJ_VAL(obj, 0), CAST_WAND);
    else
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 DEFAULT_WAND_LVL, CAST_WAND);
    break;
  case ITEM_SCROLL:
    	if (AFF_FLAGGED(ch, AFF_SIELENCE))
     	{send_to_char(SIELENCE, ch);
     	 return;
     	}
	if (AFF_FLAGGED(ch, AFF_BLIND))
       {send_to_char("�� �� ������ �������, �� �����.\r\n", ch);
        return;
       }  
    	if (AFF_FLAGGED(ch,AFF_COURAGE))
	{ send_to_char("��� �� �� ���� �������!\r\n", ch);
	 return;
	}
    spellnum = GET_OBJ_VAL(obj, 1);
    if (!*argument)
       tch = ch;
    else
    if (!find_cast_target(spellnum,argument,ch,&tch,&tobj))
       return;

    act("�� �������� $3, ������� ���������.", TRUE, ch, obj, 0, TO_CHAR); 
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n �������$y $3.", FALSE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  case ITEM_POTION:
    tch = ch;
    act("�� ������� $3.", FALSE, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n ������$y $3.", TRUE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_POTION) <= 0)
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  default:
    log("SYSERR: Unknown object_type %d in mag_objectmagic.",
	GET_OBJ_TYPE(obj));
    break;
  }
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */

#define REMEMBER_SPELL(ch,spellnum) 	(ch)->Memory.add(spellnum);\
					(ch)->ManaMemNeeded += mag_manacost(ch,spellnum);

int cast_spell(struct char_data * ch, struct char_data * tch,
	           struct obj_data * tobj, int spellnum, int use_glory)
{
  if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE) {
    log("SYSERR: cast_spell trying to call spellnum %d/%d.\n", spellnum,
	TOP_SPELL_DEFINE);
    return (0);
  }
    
  if (GET_POS(ch) < SINFO.min_position) {
    switch (GET_POS(ch)) {
      case POS_SLEEPING:
      send_to_char("�� �������� � ������� ���������� �����.\r\n", ch);
      break;
    case POS_RESTING:
      send_to_char("�� �� ������ ������������������, �� ������� �����������.\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("�� �� ������ ����� ������� ����!\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("����������! �� �� ������ ������������������!\r\n", ch);
      break;
    default:
      send_to_char("�� �� ������ ������� ������ ��� ���!\r\n", ch);
      break;
    }
    return (0);
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
    send_to_char("�� �� ������ ������� ���� ������ �������!\r\n", ch);
    return (0);
  }
  if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY)) {
    if (IS_WEDMAK(ch))
	  send_to_char("�� ������ ������������ ���� ���� ������ �� ����.\r\n", ch);
    else
	  send_to_char("�� ������ ��������� ��� ���������� ������ �� ����.\r\n", ch);
    return (0);
  }
  if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
    if (IS_WEDMAK(ch))
      send_to_char("�� �� ������ ������������ ���� ���� �� ����.\r\n", ch);
    else  
	  send_to_char("�� �� ������ ��������� ��� ���������� �� ����.\r\n", ch); 
    return (0);
  }
  
if ((!tch || IN_ROOM(tch) == NOWHERE) && !tobj &&
      IS_SET(SpINFO.targets,
             TAR_CHAR_ROOM | TAR_CHAR_WORLD | TAR_FIGHT_SELF | TAR_FIGHT_VICT |
             TAR_OBJ_INV   | TAR_OBJ_ROOM   | TAR_OBJ_WORLD  | TAR_OBJ_EQUIP)
     )
     {send_to_char("�� �� ������ ����� ���� ��� ������ ����������!\r\n",ch);
      return (0);
	}
  
 if (use_glory)
    {
        if (spellnum != SPELL_SUMMON_ANIMAL)
        {
            send_to_char("��� ���������� �� �������� �� �����.\r\n", ch);
            return 0;
        }
    }
  
if (IS_SET(SINFO.routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("�� �� ������ ��������� ��� ����������, ���� �� ���������� � ������!\r\n",ch);
    return (0);
  }

 if (!IS_WEDMAK(ch))
 { if (!IS_NPC(ch))
     {if (PRF_FLAGGED(ch,PRF_NOREPEAT))
         send_to_char(OK, ch);
      else
         {sprintf(buf,"�� ���������� ���������� \'%s%s%s\'\r\n",
                  CCCYN(ch,C_NRM),SpINFO.name,CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
         }
     }	

 say_spell(ch, spellnum, tch, tobj);
 
 if (GET_SPELL_MEM(ch,spellnum) > 0)
     GET_SPELL_MEM(ch,spellnum)--;
 else
     GET_SPELL_MEM(ch,spellnum) = 0;
  
          if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
        	{REMEMBER_SPELL(ch,spellnum);
		}
	  
	  if (!IS_NPC(ch))
     	   { if (!GET_SPELL_MEM(ch,spellnum))
         	REMOVE_BIT(GET_SPELL_TYPE(ch,spellnum),SPELL_TEMP);
         	 affect_total(ch);
      	   }
  	 else
     		GET_CASTER(ch)  -= (IS_SET(spell_info[spellnum].routines,NPC_CALCULATE) ? 1 : 0);
     
 }
    return (call_magic(ch, tch, tobj, spellnum, GET_LEVEL(ch), CAST_SPELL, use_glory));
}


int spell_use_success(struct char_data *ch, struct char_data *victim, int casting_type, int spellnum)
{int prob = 100;
 if  (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
     return (TRUE);

 switch (casting_type)
 {case SAVING_SPELL:
  case SAVING_ROD:	//����������� �� ������ ������ ����������
                   //�������� �� �������, � �� ����� ����....
 
             //����� ������ ������� 30.05.2004�
                      prob =   int_app[GET_REAL_INT(ch)].spell_aknowlege		   - 
				(spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)] * 2)  +
			        GET_CAST_SUCCESS(ch)/2					   +
				wis_app[GET_REAL_WIS(ch)].spell_success	  		   - 
				GET_USTALOST(ch)- (victim ? (GET_LEVEL(victim)-GET_LEVEL(ch)): 0);

      if (spellnum == SPELL_HEAL	||
		  spellnum == SPELL_CURE_CRITIC ||
		  spellnum == SPELL_CURE_SERIOUS)
	  {
		 
	      if (GET_AF_BATTLE(victim, EAF_AWAKE))
			  prob -= GET_SKILL(victim, SKILL_AWAKE)/10;
	  }
  




       if ((IS_MAGIC_USER(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_MAGE)) ||
           (IS_CLERIC(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_CLERIC)) ||
           (IS_TAMPLIER(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_TAMPLIER)) ||
           (IS_DRUID(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch),ROOM_DRUID)))
          prob += 10;
       if (IS_MAGE(ch) && equip_in_metall(ch))
          prob -= 50;
       break;
 }

 prob = complex_spell_modifier(ch,spellnum,GAPPLY_SPELL_SUCCESS,prob);
 if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
     (SpINFO.violent  && victim && GET_GOD_FLAG(victim, GF_GODSLIKE)) ||
     (!SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSCURSE))
    )
    prob -= 50;
 if ((SpINFO.violent  && victim && GET_GOD_FLAG(victim, GF_GODSCURSE)) ||
     (!SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSLIKE))
    )
    prob += 50;
    
 return (prob > number(0,100));
}

class tokenizer
{
public:
    tokenizer(const char* str)
    {        
        const char *b = str;
        const char *e = b + strlen(str);
        if (b == e)
            return;
        bool word = sym(*b) ? false : true; 
        const char *p = b + 1;
        for (;p != e; ++p)
        {
            if (sym(*p) && word)
            {
                std::string new_part(b, p-b);
                tokens.push_back(new_part);
                word = false;
            }
            if (!sym(*p) && !word)
            {
                b = p;
                word = true;
            }
        }
        if (word)
        {
            std::string new_part(b, e-b);
            tokens.push_back(new_part);
        }
    }
    const char* operator[](int index) { return tokens[index].c_str(); }
    bool sym(char t) { return (t == ' ' || t == '.') ? true : false; }

    std::vector<std::string> tokens;
};

char* find_spell_ex(char* s, int* spell_num)
{
    int spellnum = -1; char *t = NULL;
    tokenizer tk(s);
    int count = tk.tokens.size();
    if (count > 0)
    {
       int p = (!strn_cmp(tk[0], "�����", 3)) ? 1 : 0;
       count -= p;          
       if (count > 1)
       {
           std::string tmp(tk[p]); tmp.append(" "); tmp.append(tk[p+1]);
           strcpy(buf, tmp.c_str());
           spellnum = find_spell_num(buf);
           if (spellnum != -1)
               p = p + 2;
       }
       if (spellnum == -1 && count > 0)
       {
           strcpy(buf, tk[p]);
           spellnum = find_spell_num(buf);
           if (spellnum != -1)              
               p = p + 1;
       }

       if (spellnum != -1)
       {
           count = tk.tokens.size();
           if (p < count)
           {
               const char* x = strstr(s, tk[p]);
               strcpy(buf, x);
               t = buf;
           }
       }
   }
   *spell_num = spellnum;
   return t;
}


ACMD(do_cast)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char  *s, *t;
  int    spellnum, target = 0, mod = 0;
  const char *W;


    if (PLR_FLAGGED(ch, PLR_IMMCAST))
	{ send_to_char("� ��� ��� ���������� ���������!\r\n", ch);
	  return;
	}

   if (IS_NPC (ch) && AFF_FLAGGED (ch, AFF_CHARM))
         return;


    if (!IS_NPC(ch))
    { if (!IS_CASTER(ch) && !IS_IMMORTAL(ch))
	{ send_to_char("� ����� ��� ������� ��� �����?\r\n", ch);
	  return;
	}
    }

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char("�� ���������� ���������� ����������, �� ���� ����� ���-�� �������.\r\n",ch);
      return;
     }

  if (!*ch->player_specials->saved.spare0)
	  W = "'";
  else 
	  W = ch->player_specials->saved.spare0;

  // get: blank, spell name, target name
  s = strtok(argument, W);
  if (s == NULL)
     {send_to_char("��� �� ������ ���������?\r\n", ch);
      return;
     }
  
  int use_glory = 0;
  strcpy(buf, argument);
  char *x = &buf[0];
  skip_spaces(&x);
  if (!strn_cmp(x, "�����", 3))
  {
      use_glory = 1;
  }

  char *s2 = strtok(NULL, W);
  if (s2 != NULL)
  {
      s = s2;
      t = strtok(NULL, "\0");
      spellnum = find_spell_num(s);
  }
  else
  {
      size_t pos = strspn(s, " ");
      s += pos;
      t = find_spell_ex(s, &spellnum);
  }
  /* s = strtok(NULL, W);
  if (s == NULL) {
      sprintf(buf, "�������� ���������� ������ ���� �������� � ���������� ��������: %s \r\n", W);
      send_to_char(buf ,ch); 
      return;
  }
  spellnum = find_spell_num(s);
  */  
  
  // ����������� ���������� 
  if (spellnum < 1 || spellnum > MAX_SPELLS)
     {send_to_char("�� �� ������ ������ ����������.\r\n", ch);
      return;
     }
 
  // Caster is lower than spell level
if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_TEMP | SPELL_KNOW) &&
      !IS_IMMORTAL(ch) && !IS_NPC(ch))//��������
     { if (1+GET_CASTER_KRUG(ch) < spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)] ||
		  SpINFO.min_level[(int) GET_CLASS(ch)] > GET_REMORT(ch))
  		{ send_to_char("�� ���� ��� �� �������� ���� �����������.\r\n", ch);
          return;
        }
      else
         {send_to_char("��� ���������� ������� ���� �������.\r\n", ch);
          return;
         }
     }

 if (SpINFO.aligment[(int) GET_CLASS(ch)]) //��������
		{ if (IS_SET(SpINFO.aligment[(int) GET_CLASS(ch)], SKILL_GOOD) && IS_GOOD(ch))
			  target=1; 
		  else
          if (IS_SET(SpINFO.aligment[(int) GET_CLASS(ch)], SKILL_NEUTRAL) && IS_NEUTRAL(ch))
	          target=1; 
          else
          if (IS_SET(SpINFO.aligment[(int) GET_CLASS(ch)], SKILL_EVIL) && IS_EVIL(ch))
			  target=1;
		 if (!target)
		{ send_to_char("�� �� ������ ������������ ����� ������ �����������!\r\n", ch);
			return;	 
		}
	   }

  // ������ �� ����� ��������� ����������
  if (!GET_SPELL_MEM(ch,spellnum) && !IS_IMMORTAL(ch))
     {send_to_char("�� ���������� �� �������, ��� ������������ ��� ����������!\r\n", ch);
      return;
     }

  // ����� ����
  if (t != NULL)
     {one_argument(strcpy(arg, t), t);
      skip_spaces(&t);
     }

  target = find_cast_target(spellnum,t,ch,&tch,&tobj);

  if (target && (tch == ch) && SpINFO.violent)
     {send_to_char("������ ����������, ��� � ���� ������� ����������!\r\n", ch);
      return;
     }

  if (!target)
     {send_to_char("�� ���� ����� ���� ������ ����������!\r\n", ch);
      return;
     }

  // ����� � ��� �� ������� � ��� ���������� ����������� !!!
  
   SET_CAST(ch,0,NULL,NULL);
    
  if (!use_glory && !spell_use_success(ch, tch, SAVING_SPELL, spellnum))
     { if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
          WAIT_STATE(ch, PULSE_VIOLENCE);
       if (GET_SPELL_MEM(ch, spellnum))
          GET_SPELL_MEM(ch,spellnum)--;
       if (!GET_SPELL_MEM(ch,spellnum))
          REMOVE_BIT(GET_SPELL_TYPE(ch,spellnum), SPELL_TEMP);
       if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
	     { REMEMBER_SPELL(ch,spellnum); //������� ������ 
	     }
	  
		//log("[DO_CAST->AFFECT_TOTAL] Start");
       affect_total(ch);
       //log("[DO_CAST->AFFECT_TOTAL] Stop");
      // if (!tch || !skill_message(0, ch, tch, spellnum))
       act("$n ������$y ������� � �������$u ���������� ����������, �� ������ �� ����������",FALSE,ch,tobj,tch,TO_ROOM); 
	   send_to_char("�� �� ������ ���������������� ���� ��������!\r\n", ch);
     }
  else
     {    
       if (cast_spell(ch, tch, tobj, spellnum, use_glory) >= 0)
       {
           /*switch (spellnum) // ���������
	   {case SPELL_PORTAL:
	      mod = 8;
	    break;
	    case SPELL_LOCATE_OBJECT:
	      mod = 10;
	    break;
		case SPELL_ARMAGEDDON:
	      mod = 3;
	    break;
		case SPELL_COLOR_SPRAY:
	      mod = 3;
	    break;
		case SPELL_ENCHANT_WEAPON:
	      mod = 20;
	    break;
		case SPELL_EARTHQUAKE:
	      mod = 2;
	    break;
		case SPELL_CHAIN_LIGHTNING:
	      mod = 2;
	    break;

	    default:
		break;
	       }*/
           if (!(WAITLESS(ch)))//|| CHECK_WAIT(ch)))
             { WAIT_STATE(ch, PULSE_VIOLENCE);
               //ustalost_timer(ch, mod);
	     } 
          }
     }
}

/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */

ACMD(do_znak)
{ int znak=0, spellnum, mana =0, irgen=0, aksi=0, gelio=0, kven=0;
  struct char_data *vict = NULL;
  struct obj_data *tobj = NULL;

const char *wedZnak1[] =
{	
	"����",
	"�����",
	"�����",
    "����",
	"���������",
	"����",
	"\n"
};
  
  const char *wedZnak[] =
{	
	"����",
	"�����",
	"�����",
    "����",
	"���������",
	"����",
	"\n"
};
   

 if (PLR_FLAGGED(ch, PLR_IMMKILL))
	{ send_to_char("� ��� ������� ����, ��� ����� ���� -(\r\n", ch);
      return;
	}

	if (!IS_WEDMAK(ch) && !IS_IMPL(ch))
	{ send_to_char("���������� �� ����, ������ �� �� �� ��������?\r\n", ch);
	  return; //�������� 23.04.03 �.
	}
	*buf = '\0';

   argument = one_argument(argument, arg);
              skip_spaces(&argument);

 
		if (!arg || !*arg)
		{ 	for (znak = 0; znak <= 5; znak ++)
			{ spellnum = find_spell_num(wedZnak1[znak]);
			  if (GET_LEVEL(ch) >= spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)] &&
				spell_info[spellnum].min_level[(int) GET_CLASS(ch)] <= GET_REMORT(ch))
				{ mana = 1; 
				  sprintf(buf + strlen(buf), " &W%s&n\r\n", SpINFO.name);
				}
			}	

		   if (mana)
			{ send_to_char("� ��������� ����� ��� �������� ��������� �����: \r\n", ch);
			  send_to_char(buf, ch);
			}
			else
			  send_to_char("� ��������� �����, ��� ����� ����������!\r\n", ch);  
			return;
		}

   if ((znak = search_block(arg, wedZnak, FALSE)) < 0)
		{ send_to_char("� ��� �� ����� ���� �� ���������?\r\n", ch);
	      return; 	
		}
    if (GET_EQ(ch,WEAR_HOLD) || GET_EQ(ch,WEAR_BOTHS) || GET_EQ(ch,WEAR_LIGHT))
    {    send_to_char("�� �� ������ ������� ����, � ��� ������ ����!\r\n", ch);
         return;
	}
		 spellnum = find_spell_num(arg);
	  
      if (GET_LEVEL(ch) < spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)] ||
				spell_info[spellnum].min_level[(int) GET_CLASS(ch)] > GET_REMORT(ch))
				{ send_to_char("�� ��� �� �������� ���� ������!\r\n", ch);
			      return;
				}

   if (!find_cast_target(spellnum,argument,ch,&vict,&tobj))       
         return;
	if (vict && check_pkill(ch, vict,argument))  
        return;
    

	 if (affected_by_spell(ch, SPELL_ZNAK_IRGEN))
	    { affect_from_char(ch, SPELL_ZNAK_IRGEN);
		  send_to_char("�� ���������� ������������ ���� '&W�����&n'\r\n",ch);	
		  irgen =1;

		}
	 if (affected_by_spell(ch, SPELL_ZNAK_AKSY))
	    { affect_from_char(ch, SPELL_ZNAK_AKSY);
	      send_to_char("�� ���������� ������������ ���� '&W�����&n'\r\n",ch);
	      aksi =1;
		}

     if (affected_by_spell(ch, SPELL_ZNAK_GELIOTROP))
	    { affect_from_char(ch, SPELL_ZNAK_GELIOTROP);
	      send_to_char("�� ���������� ������������ ���� '&W���������&n'\r\n",ch);
	      gelio =1;
		}
		
	if (affected_by_spell(ch, SPELL_ZNAK_KVEN))
	    { affect_from_char(ch, SPELL_ZNAK_KVEN);
	      send_to_char("�� ���������� ������������ ���� '&W����&n'\r\n",ch);
	      kven =1;
		}
		
	 switch (znak)
	{case 0:
    if (!IS_IMMORTAL(ch))
			{  mana = znak_manacost(ch, spellnum);
				if ((mana > 0) && (GET_MANA(ch) < mana))
				{ send_to_char("� ��� �� ������� ������� ��� ����� �����!\r\n", ch);
				return;
				}
				if (mana > 0)
				GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			}
	sprintf(buf, "�� �������� ���� � ������� $R � ������� ������ ������ \'%s\'", wedZnak[znak]);
	act(buf,FALSE,ch,0,vict,TO_CHAR);
	sprintf(buf, "$n �������$y ���� � ���� ������� � ������$y ������ ������ \'%s\'", wedZnak[znak]);
	act(buf,FALSE,ch,0,vict,TO_VICT);
    sprintf(buf, "$n �������$y ���� � ������� $R � ������$y ������ ������ \'%s\'", wedZnak[znak]);
	act(buf,TRUE,ch,0,vict,TO_NOTVICT);
	cast_spell(ch, vict, NULL, SPELL_ZNAK_IGNI);
	break;
      case 1:
      if (!irgen)
	{ if (!IS_IMMORTAL(ch))
			{  mana = znak_manacost(ch, spellnum);
				if ((mana > 0) && (GET_MANA(ch) < mana))
				{ send_to_char("� ��� �� ������� ������� ��� ����� �����!\r\n", ch);
				return;
				}
				if (mana > 0)
				GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			}
		  cast_spell(ch, vict, NULL, SPELL_ZNAK_IRGEN);
    
	} 
	 break;
      case 2:
	 if(!aksi)
	{ if (!IS_IMMORTAL(ch))
			{  mana = znak_manacost(ch, spellnum);
				if ((mana > 0) && (GET_MANA(ch) < mana))
				{ send_to_char("� ��� �� ������� ������� ��� ����� �����!\r\n", ch);
				return;
				}
				if (mana > 0)
				GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			}
		  cast_spell(ch, vict, NULL, SPELL_ZNAK_AKSY);
	}
	break;	  
	  case 3:
    if (!IS_IMMORTAL(ch))
			{  mana = znak_manacost(ch, spellnum);
				if ((mana > 0) && (GET_MANA(ch) < mana))
				{ send_to_char("� ��� �� ������� ������� ��� ����� �����!\r\n", ch);
				return;
				}
				if (mana > 0)
				GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			}
	 sprintf(buf, "�� �������� ���� � ������� $R � ������� ������ ������ \'%s\'", wedZnak[znak]);
	act(buf,FALSE,ch,0,vict,TO_CHAR);
	sprintf(buf, "$n �������$y ���� � ���� ������� � ������$y ������ ������ \'%s\'", wedZnak[znak]);
	act(buf,FALSE,ch,0,vict,TO_VICT);
    sprintf(buf, "$n �������$y ���� � ������� $R � ������$y ������ ������ \'%s\'", wedZnak[znak]);
	act(buf,TRUE,ch,0,vict,TO_NOTVICT);
	cast_spell(ch, vict, NULL, SPELL_ZNAK_AARD);
	break;
	  case 4:
		  if (!gelio)
	{ if (!IS_IMMORTAL(ch))
			{  mana = znak_manacost(ch, spellnum);
				if ((mana > 0) && (GET_MANA(ch) < mana))
				{ send_to_char("� ��� �� ������� ������� ��� ����� �����!\r\n", ch);
				return;
				}
				if (mana > 0)
				GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
			}
		  cast_spell(ch, vict, NULL, SPELL_ZNAK_GELIOTROP);
    
	} 
   break;
      case 5:
	  if(!kven)
		{ if (!IS_IMMORTAL(ch))
				{  mana = znak_manacost(ch, spellnum);
					if ((mana > 0) && (GET_MANA(ch) < mana))
					{ send_to_char("� ��� �� ������� ������� ��� ����� �����!\r\n", ch);
					return;
					}
					if (mana > 0)
					GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
				}
			  cast_spell(ch, vict, NULL, SPELL_ZNAK_KVEN);
		}
	break;
	}
if (!(WAITLESS(ch)))//|| CHECK_WAIT(ch)))
              WAIT_STATE(ch, PULSE_VIOLENCE);
}


ACMD(do_zackl)
{if (IS_NPC(ch))
     return;
if (!IS_CASTER(ch) && !IS_IMMORTAL(ch))
		{ send_to_char("� ����� ��� ������� ��� �����?\r\n", ch);
		return;
		}
if (!slot_for_char1(ch,1))
		{ send_to_char("� ��������� �����, ��� ����� ����������!\r\n", ch);
			return;
		} 
    list_spells(ch, ch);
}


ACMD(do_razd)
{
	
	skip_spaces(&argument);
    
	
/*	if (GET_LEVEL(ch) < LVL_IMMORT)
    if (GET_CLASS(ch) == CLASS_THIEF || GET_CLASS(ch) == CLASS_WARRIOR ||
       GET_CLASS(ch) == CLASS_VARVAR) {
    send_to_char("� ����� ��� ��� �����? -)\r\n", ch);
	return;
}*/
if (slot_for_char1(ch,1) <= 0)
     {send_to_char("� ����� ��� ��� �����? =)\r\n", ch);
      return;
     }
	if (!*argument) {
	sprintf(buf, "�� ���������, ��� ���������� ������ ��� ����������  %s\r\n", ch->player_specials->saved.spare0);
    	send_to_char(buf, ch);
	return;
	}
	  
   
//LOWER
     // begin = *argument;
     // argument++;
       
		if (strlen(argument) > 1) 
        { send_to_char("���������� ������ ������ ���� ������!\r\n", ch);
          send_to_char("�� ��������� ��������������� ���������� ������ \"*\"!\r\n", ch);
          strcpy(ch->player_specials->saved.spare0,"*");
		  return;
		}
		strcpy(ch->player_specials->saved.spare0, argument);
		sprintf(buf, "������ ��� ���������� ������ ��� ����������  %s\r\n", ch->player_specials->saved.spare0);
		send_to_char(buf, ch);

}  

int chance_learn(struct char_data *ch, struct obj_data *obj)
{
	int addchance=10, count =0, i;

     switch(GET_CLASS(ch))
	{ case CLASS_CLERIC:
	  case CLASS_TAMPLIER:
		if (weather_info.sunlight == SUN_LIGHT) //����
			{ addchance +=3;
			  count++;	
			}
		if (weather_info.sky == SKY_LIGHTNING)// ����
			{ addchance +=5;
			  count++;
			}
        if (get_moon(0, 1) != 4 || get_moon(0, 1) != 1)//���� �� ���
			{ addchance +=2;
			  count++;
			}
		  break;

	  case CLASS_MAGIC_USER:
	  case CLASS_ASSASINE:
		if (weather_info.sunlight == SUN_DARK) //����
			{ addchance +=3;
			  count++;	
			}
		if (weather_info.sky == SKY_RAINING)//�����
			{ addchance +=7;
			  count++;
			}
		if (get_moon(0, 1) == 4) //����������
			{ addchance +=5;
			  count++;
			}
		
		 break;

      case CLASS_MONAH:
      case CLASS_SLEDOPYT:
      case CLASS_DRUID:
		if (weather_info.sunlight == SUN_RISE || //�������, �����
			weather_info.sunlight == SUN_SET)
			{ addchance +=3;
			  count++;	
			}
		if (weather_info.sky == SKY_CLOUDLESS || //������� � ��������
			weather_info.sky == SKY_CLOUDY)
			{ addchance +=1;
			  count++;
			}
        if (get_moon(0, 1) == 1) //���������
			{ addchance +=4;
			  count++;
			}
		  break;
	}
    
		if(!count)
		  count =1;
 addchance *=count;
 addchance += (IS_CLERIC(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_CLERIC)) ||
              (IS_MAGIC_USER(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_MAGE))   ||
              (IS_DRUID(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_DRUID))		||
              (IS_SLEDOPYT(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_SLEDOPYT)) ||
              (IS_TAMPLIER(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_TAMPLIER)) ||
		  (IS_MONAH(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_MONAH))	||
              (IS_WEDMAK(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_WEDMAK))
			  ? 30 : 0;

    i = int_app[POSI(GET_REAL_INT(ch))].spell_aknowlege +
	(GET_REAL_INT(ch) - 18)*3 + addchance -
	spell_info[GET_OBJ_VAL(obj,1)].slot_forc[GET_CLASS(ch)]*13 +
	GET_LEVEL(ch) *2  + (GET_REAL_CHA(ch)-12);

	if (GET_COND(ch, FULL) < 14) 
			i -=3;
    if (GET_COND(ch, THIRST) < 14)
		    i -=3;
	if (GET_COND(ch, FULL) > 22)
		    i -=3;
    if (GET_COND(ch, THIRST) > 22)
			i -=3;
	if (GET_COND(ch, DRUNK) <= 12 && GET_COND(ch, DRUNK) >= 6)
		     i += 10;

//��������� ���������� ��������!!! (� ��������� - ������ �������, ������ ��� ����!!!!!
// ��� ���� ��������, ��� ������� ���������!!!!!!!!!! 
   i += GET_OBJ_LEVEL(obj); 
   i -= GET_MAX_MOVE(ch) - GET_MOVE(ch);
   i -= GET_MAX_HIT(ch)  - GET_HIT(ch);
  
		if (number(80,200) <= i)// ��������, ������ ��?
			return TRUE;
return FALSE;
} 

ACMD(do_learn)
{ struct obj_data *obj;
  int   spellnum, i=0;//, addchance;

  if (IS_NPC(ch))
     return;

  /* get: blank, spell name, target name */
  one_argument(argument, arg);

  if (!arg || !*arg)
     {send_to_char("�� ��������� ����������� ������� ���� �����. ��, ���� �� � ���������.\r\n", ch);
      act("$n ��������� �������$u �� ���� �����. �������� �� �� ���-������ $m.", FALSE, ch, 0, 0, TO_ROOM);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {send_to_char("� � ��� ����� ���.\r\n", ch);
      return;
     }

  if (GET_OBJ_TYPE(obj) != ITEM_BOOK)
     {act("�� ���������� �� $3, ��� ����� �� ����� ������.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n �����$y ����������� ������� ������� $1.", FALSE, ch, obj, 0, TO_ROOM);
      return;
     }

  if (slot_for_char1(ch,1) <= 0)
     {send_to_char("��������� ���-������ ����� ����������!\r\n", ch);
      return;
     }

  if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) <= LAST_USED_SPELL)
     spellnum = GET_OBJ_VAL(obj,1);
  else
     {send_to_char("����� �� ���������� - �������� �����!\r\n",ch);
      return;
     }

    if (SpINFO.aligment[(int) GET_CLASS(ch)])
		{ if (IS_SET(SpINFO.aligment[(int) GET_CLASS(ch)], SKILL_GOOD) && IS_GOOD(ch))
			  i=1; 
		  else
          if (IS_SET(SpINFO.aligment[(int) GET_CLASS(ch)], SKILL_NEUTRAL) && IS_NEUTRAL(ch))
	          i=1; 
          else
          if (IS_SET(SpINFO.aligment[(int) GET_CLASS(ch)], SKILL_EVIL) && IS_EVIL(ch))
			  i=1;
		 if (!i)
		{ send_to_char("�� �� ������ ������� ����� ������ �����������!\r\n", ch);
			return;	 
		}
	   }

  if (1+GET_CASTER_KRUG(ch) < SpINFO.slot_forc[(int) GET_CLASS(ch)] ||
	  	SpINFO.min_level[(int) GET_CLASS(ch)] > GET_REMORT(ch)		||
	    invalid_no_class(ch, obj))
	{ sprintf(buf,"- \"�������, � �� ������, ��� ������ �������� �� �����.\r\n"
			  "�� ���������� ��� ���, ��� ���������� ���-�� �������� � ������,\r\n"
			  "�������� %s, ������������ %s\".\r\n"
              "������� ������ ��� ��������� �����, �� � ��������������\r\n"
              "������ %s.\r\n"
			  "\"������ ��� ���������� �� ��� ����\", �������� ��.\r\n",
              number(0,1) ? "� ������ ����" : number(0,1) ? "��� ���" : "� ������ ������ ������",
              number(0,1) ? "��"   : number(0,1) ? "���" : "�����",
              obj->short_vdescription);
      send_to_char(buf,ch);
      act("$n � ��������� ���������$y � $3,\r\n"
		  "� ������ ��������, �������$y �������.",
          FALSE, ch, obj, 0, TO_ROOM);
      return;
     }



  if (GET_SPELL_TYPE(ch,spellnum) & SPELL_LTEMP)
	{ sprintf(buf, "��� ���������� �� ������� ������� �� ��������� ������!\r\n");
	  send_to_char(buf,ch);
        return;
	}
	 
  if (GET_SPELL_TYPE(ch,spellnum) & SPELL_KNOW)
     {sprintf(buf,"   �� ������� \\c14%s\\c00\r\n"
                  "� ��������� � �������� ����������� � ������������ ������.\r\n"
				  "   ������ �� ���� ���� �������������, ����� �� ������� %s,\r\n"
                  "� ������, ��� ���������� \\c14\"%s\"\\c00 �� ��� ������.\r\n",
                  obj->short_vdescription,
                  number(0,1) ? "��� �����" :
                  number(0,1) ? "��������� �������" : "����������� �� �����",
                  SINFO.name);
      send_to_char (buf, ch);
      act("$n � ��������� ������$u ������� $3.\r\n"
          "���������� $s ������� ����� ������� � $e, ������$y $3.",
          FALSE, ch, obj, 0, TO_ROOM);
      return;
     }

   

 
  if (chance_learn(ch, obj))   //����� �������, �� ����...
  {sprintf(buf,"�� ����� � ���� \\c14%s\\c00 � ������ �������. ����������,\r\n"
                  "���������� �����, ����� ������������ � �������� �����.\r\n"
                  "�� �������������, ��� �������� ����� ����������� - \\c14\'%s\'\\c00.\r\n",
                  obj->short_vdescription, SINFO.name);
      			  send_to_char(buf,ch);
      GET_SPELL_TYPE(ch,spellnum) |= SPELL_KNOW;
     }
  else
     {sprintf(buf,"�� ����� � ���� %s � ������ �������.\r\n"
                  "���������� ����� ����� �������� ������ �����.\r\n"
                  "��� ��������, ��� ���-��� ��������� ����������� �����,\r\n"
                  "��, ���-�� ��� ��������, %s \r\n"
			"�����%s �� ��� � �����%s.\r\n�� �� ������ ������� %s.\r\n",
                  obj->short_vdescription, obj->short_description, GET_OBJ_SUF_1(obj),
			GET_OBJ_SUF_4(obj), obj->short_vdescription);
                  send_to_char(buf,ch);
      SET_BIT(GET_SPELL_TYPE(ch, spellnum), SPELL_LTEMP);
    //GET_SPELL_TYPE(ch,spellnum) |= SPELL_LTEMP;
     }
  obj_from_char(obj);
  extract_obj(obj); //������� 11 ������� 2002�. ��� �� ����� ��������...
}


void show_wizdom(struct char_data * ch)
{char names[MAX_SLOT][MAX_STRING_LENGTH];
 int  slots[MAX_SLOT], i, max_slot, count, slot_num, is_full, gcount=0, p=0,
      imax_slot = 0;
 for (i = 1; i <= MAX_SLOT; i++)
     {*names[i-1] = '\0';
       slots[i-1] = 0;
      if (slot_for_char1(ch,i))// if (slot_for_char(ch,i)) //���������� ������� ����� ���� ������
          imax_slot = i;		// � ������ (���� ���� ��������,
     }							// ������ ���� ����)
  
   // if (bitset & 0x01)
   { 
    is_full = 0;
    for (i = 1, max_slot = 0; i <= MAX_SPELLS; i++)
        {if (!GET_SPELL_TYPE(ch,i))
            continue;
         if (!spell_info[i].name || *spell_info[i].name == '!')
            continue;

         count    = GET_SPELL_MEM(ch,i);
         if (IS_IMMORTAL(ch))
             count = 10;
         if (!count)
            continue;
         slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)] - 1;
         max_slot = MAX(slot_num, max_slot);
			
		 slots[slot_num] += sprintf(names[slot_num]+slots[slot_num],
                                    "%s[%d] %-27s",
                                   slots[slot_num] % 87 < 32 ? "\r\n" : "",
                                    count,
                                    spell_info[i].name);
				/*	if (p == 3)
					{gcount += sprintf(buf2+gcount, "\r\n");
					p = 0;
					}*/
				is_full++;
        };
     
    if (is_full)
       {gcount += sprintf(buf2+gcount,"%s�� ������� ��������� ���������� :%s",
                                  CCCYN(ch, C_NRM), CCNRM(ch, C_NRM)  );
		for (i = 0; i < max_slot+1; i++)
            {if (slots[i])
                {	
			        gcount += sprintf(buf2+gcount,"\r\n&g���� %d:&n",i+1);
                    gcount += sprintf(buf2+gcount,"%s\r\n",names[i]);
                
				}
            }
         gcount += sprintf(buf2+gcount,"\r\n");
       }
    else
      {
          gcount += sprintf(buf2+gcount,"&c������ � ��� ��� ��������� ����������.&n\r\n");
      }     
   }
 
 //if (bitset & 0x02)
 {
    int e=ch->Memory.size();
    if (e > 0)
    {
        gcount += sprintf(buf2+gcount,"%s�� ����������� ��������� ���������� :%s\r\n",
                                  CCGRN(ch, C_NRM), CCNRM(ch,C_NRM));
        for (i = 0,e=ch->Memory.size(); i<e; ++i)
        {
            //slot_num = spell_info[spell_id].slot_forc[(int) GET_CLASS(ch)];

            int spell_id = ch->Memory.getid(i);
            int seconds = ch->Memory.calc_time(ch, i);
            int minutes = seconds / 60;
            seconds = seconds - minutes * 60;           
            gcount += sprintf(buf2+gcount,"%d. %s %d:%s%d\r\n", i+1, spell_info[spell_id].name, minutes, seconds > 9 ? "" : "0" ,seconds);            
        }
        gcount += sprintf(buf2+gcount,"\r\n");
    }

    /* is_full = 0;
    for (i = 0; i < MAX_SLOT; i++)
        {*names[i]='\0'; slots[i] = 0;}

    for (i = 1; i <= MAX_SPELLS; i++)
    {
         count = ch->Memory.count(i);
         if (count == 0)
            continue;

	     slot_num         = spell_info[i].slot_forc[(int) GET_CLASS(ch)] - 1;
         slots[slot_num] += sprintf(names[slot_num]+slots[slot_num],
                                    "%2s[%d] %-31s",
                                    slots[slot_num] % 80 < 10 ? "\r\n" : "  ",
                                    count,
									spell_info[i].name);
         is_full++;
    };
    
    if (is_full)
       {gcount += sprintf(buf2+gcount,"%s�� ����������� ��������� ���������� :%s",
                                  CCGRN(ch, C_NRM), CCNRM(ch,C_NRM) );
		for (i = 0; i < imax_slot; i++)
            {if (slots[i])
                {gcount += sprintf(buf2+gcount,"\r\n&g���� %d&n:",i+1);
                 gcount += sprintf(buf2+gcount,"   %s",names[i]);
                }
            }
        gcount += sprintf(buf2+gcount,"\r\n\r\n");
    }
    else
    {
       // gcount += sprintf(buf2+gcount,"\r\n&c  �� ������ �� �����������.&n");
       //gcount += sprintf(buf2+gcount,"\r\n");
    }*/
   }

 //if (bitset & 0x04)
 {
    is_full = 0;
    for (i = 0; i < MAX_SLOT; i++)
        slots[i] = 0;

    for (i = 0; i < MAX_SPELLS; i++)
    {
        if (!IS_SET(GET_SPELL_TYPE(ch,i),SPELL_KNOW))
            continue;
         slot_num         = spell_info[i].slot_forc[(int) GET_CLASS(ch)] - 1;
         slots[slot_num] += ch->Memory.count(i) + GET_SPELL_MEM(ch,i);
    }

    if (imax_slot)
       {gcount += sprintf(buf2+gcount,"%s��������� ������:%s\r\n",
                          CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
        for (i = 0; i < imax_slot; i++)
            {slot_num = MAX(0,slot_for_char1(ch,i+1) - slots[i]);
             gcount += sprintf(buf2+gcount,"%s%2d-%2d%s  ",
                               slot_num ? CCCYN(ch,C_NRM) : "",
			       i+1,slot_num,
			       slot_num ? CCNRM(ch,C_NRM) : "");
            };
       }
   strcat(buf2, "\r\n");
	}
// page_string(ch->desc, buf2, 1);
 send_to_char (buf2, ch);
}


ACMD(do_remember)
{ char *s;
  int spellnum, i, slotnum, slotcnt;
const char *W;

  /* get: blank, spell name, target name */
  if (!IS_CASTER(ch))
		{ send_to_char("�� ������ �� �������� �����������.\r\n", ch);
		return;
		} 
  if (!slot_for_char1(ch,1))
		{ send_to_char("� ��������� �����, ��� ����� ����������.\r\n", ch);
			return;
		} 
  if (!argument || !(*argument))
     {show_wizdom(ch);
      return;
     }
  if (IS_IMMORTAL(ch))
     {send_to_char("� �����, ��� ��� �� �����!\r\n", ch);
      return;
     }
   
  if (!ch->player_specials->saved.spare0)
	  W = "'";
  else 
	  W = ch->player_specials->saved.spare0;

  s = strtok(argument, W);
  if (s == NULL)
     {send_to_char("����� ���������� �� ������ �������?\r\n", ch);
      return;
     }

  char *s2 = strtok(NULL, W);
  if (s2 != NULL)
  {
      s = s2;
      spellnum = find_spell_num(s);
  }
  else
  {
      size_t pos = strspn(s, " ");
      s += pos;
      find_spell_ex(s, &spellnum);
  }

  /*if (s == NULL)
     { sprintf(buf, "�������� ���������� ������ ���� �������� � ���������� ��������: %s \r\n", W);
    send_to_char(buf ,ch); 
      return;
     }
  */

  if (spellnum < 1 || spellnum > MAX_SPELLS)
     {send_to_char("� �� ���� �� ����� ��������� �����������?\r\n", ch);
      return;
     }
  /* Caster is lower than spell level */
  //��������
 /* if (GET_LEVEL(ch) < SpINFO.min_level[(int) GET_CLASS(ch)] ||
      slot_for_char1(ch,SpINFO.slot_forc[(int) GET_CLASS(ch)]) <= 0)*/
   if (1+GET_CASTER_KRUG(ch) < spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)])  
  {send_to_char("�� ���� ��� �� �������� ����� �����������!\r\n", ch);
      return;
     };
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW))
     {send_to_char("�� �� �������� ���� �����������!\r\n", ch);
      return;
     }
  slotnum = SpINFO.slot_forc[(int) GET_CLASS(ch)];
  slotcnt = slot_for_char1(ch,slotnum);
  for (i = 1; i <= MAX_SPELLS; i++)
  {
      if (slotnum != spell_info[i].slot_forc[(int) GET_CLASS(ch)])
          continue;

       if ((slotcnt -= (GET_SPELL_MEM(ch,i) + ch->Memory.count(i) )) <= 0)
          {send_to_char("� ��� ��� ������ ��������� ������.\r\n", ch);
           return;
          }
  };
  REMEMBER_SPELL(ch,spellnum);
           
  //if (GET_RELIGION(ch) == RELIGION_MONO)
    if ((GET_CLASS(ch) == CLASS_CLERIC) || (GET_CLASS(ch) == CLASS_SLEDOPYT))
		   sprintf(buf,"�� �������� ���������� '&c%s%s%s&n' � ���� ����������.\r\n",
             CCMAG(ch,C_NRM),SpINFO.name,CCNRM(ch,C_NRM));
  else
	  if ((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_TAMPLIER) || (GET_CLASS(ch) == CLASS_ASSASINE))
     sprintf(buf,"�� �������� ���������� '&c%s%s%s&n' � ���� ����� ����������.\r\n",
             CCMAG(ch,C_NRM),SpINFO.name,CCNRM(ch,C_NRM));
  else
	  if ((GET_CLASS(ch) == CLASS_DRUID) || (GET_CLASS(ch) == CLASS_MONAH))
	  sprintf(buf,"�� ������ ���������� '&c%s%s%s&n' � ���� ��������.\r\n",
             CCMAG(ch,C_NRM),SpINFO.name,CCNRM(ch,C_NRM));
		  send_to_char(buf,ch);
  return;
}

ACMD(do_forget)
{ char *s, *t;
  int spellnum, in_mem;
const char *W;//[] = {'\0'};

  /* get: blank, spell name, target name */
  if (IS_IMMORTAL(ch))
     {send_to_char("������� \"skillset\" ����� ������ ���� �� ��� ���!!!\r\n", ch);
      return;
     }
  if (!IS_CASTER(ch))
		{ send_to_char("�� � �������� ������ �� �����!\r\n", ch);
		return;
		}
if (!slot_for_char1(ch,1))
		{ send_to_char("� ��������� �����, ��� ����� ����������.\r\n", ch);
			return;
		}

  strcpy(buf, argument);
  char *x = &buf[0];
  skip_spaces(&x);
  if (!str_cmp(x, "���"))
  {  
     for (int i = 1; i <= MAX_SPELLS; i++)
     {
        GET_SPELL_MEM(ch, i) = 0;
     }
     send_to_char("�� ������ ��� ��������� ����������.\r\n",ch);
     return;
  }

  if (!ch->player_specials->saved.spare0)
	  W = "'";
  else 
	  W = ch->player_specials->saved.spare0;

  s = strtok(argument, W);
  if (s == NULL)
     {send_to_char("����� ���������� �� ������ ������?\r\n", ch);
      return;
     }

  char *s2 = strtok(NULL, W);
  if (s2 != NULL)
  {
      s = s2;
      spellnum = find_spell_num(s);
  }
  else
  {
      size_t pos = strspn(s, " ");
      s += pos;
      find_spell_ex(s, &spellnum);
  }

  /*s = strtok(NULL, W);
  if (s == NULL)
     { sprintf(buf, "�������� ���������� ������ ���� �������� � ���������� ��������: %s \r\n", W);
    send_to_char(buf ,ch); 
	  return;
     }
  spellnum = find_spell_num(s);*/

  /* Unknown spell */
  if (spellnum < 1 || spellnum > MAX_SPELLS)
     {send_to_char("������ ���������� ���.\r\n", ch);
      return;
     }
  if (!IS_SET(GET_SPELL_TYPE(ch, spellnum), SPELL_KNOW | SPELL_TEMP))
     {send_to_char("� �� ����� ���������� � �� �����!\r\n", ch);
      return;
     }
  t      = strtok(NULL, "\0");
  in_mem = 0;
  if (t != NULL)
     {one_argument(strcpy(arg, t), t);
      skip_spaces(&t);
      in_mem = (//!strn_cmp("��������",t,strlen(t)) ||
                //!strn_cmp("����",t,strlen(t)) ||
                !strn_cmp("����",t,strlen(t)));
     }
  if (!in_mem)
     if (!GET_SPELL_MEM(ch,spellnum))
        {send_to_char("��� �� ������ �����-������ ����������, ��� ���� ������� �������.\r\n",ch);
         return;
        }
     else
        {GET_SPELL_MEM(ch,spellnum)--;
         if (!GET_SPELL_MEM(ch,spellnum))
            REMOVE_BIT(GET_SPELL_TYPE(ch,spellnum), SPELL_TEMP);
         //log("[DO_FORGET->AFFECT_TOTAL] Start");
         affect_total(ch);
         //log("[DO_FORGET->AFFECT_TOTAL] Stop");
         sprintf(buf,"�� ������� ���������� %s'%s'%s �� %s.\r\n",
                 CCCYN(ch,C_NRM),
                 SpINFO.name,
                 CCNRM(ch,C_NRM),
                 (GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_TAMPLIER) ? "����� �����" :
		 (GET_CLASS(ch) == CLASS_CLERIC) || (GET_CLASS(ch) == CLASS_SLEDOPYT) ?  "������ �����������" : "����� ��������");
         send_to_char(buf,ch);
        }
  else
     if (ch->Memory.count(spellnum))
     {
         ch->Memory.dec(spellnum);
         GET_MANA_NEED(ch) -= MIN(GET_MANA_NEED(ch), mag_manacost(ch,spellnum));
         if (!GET_MANA_NEED(ch))
            GET_MANA_STORED(ch) = 0;
         sprintf(buf,"�� �������� ���������� ���������� &C'%s'&n.\r\n",
                 SpINFO.name);
         send_to_char(buf,ch);
     }
     else
        send_to_char("� �� � �� ��������� ��� ����������.\r\n",ch);
}


void skill_level(int spell, int chclass, int level)
{
  int bad = 0;

  if (spell < 0 || spell > MAX_SKILLS + 1) {
    log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass < 0 || chclass >= NUM_CLASSES) {
    log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
		chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 1 || level > LVL_IMPL) {
    log("SYSERR: assigning '%s' to illegal level %d/%d.", skill_name(spell),
		level, LVL_IMPL);
    bad = 1;
  }

  if (!bad)    
    skill_info[spell].k_improove[chclass] = level;
}


/*********************************************************************************/
//������� ��� ��������
void mspell_level(int spell, int chclass, int level)
{
  int bad = 0, i;

  if (spell < 0 || spell > TOP_SPELL_DEFINE)
  { log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass <= 0 || chclass >= 1 << NUM_CLASSES)
  { log("SYSERR: assigning '%s' to illegal class %d/%d.", spell_name(spell),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 0 || level > LVL_IMPL)
     { log("SYSERR: assigning '%s' to illegal level %d/%d.", spell_name(spell),
           level, LVL_IMPL);
       bad = 1;
     }

  if (!bad)
     for (i = 0; i < NUM_CLASSES; i++)
         if (chclass & (1 << i))
            spell_info[spell].min_level[i] = level;
}


void mspell_slot(int spell, int chclass, int slot)
{
  int bad = 0, i;

  if (spell < 0 || spell > TOP_SPELL_DEFINE)
  { log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass <= 0 || chclass >= 1 << NUM_CLASSES)
  { log("SYSERR: assigning '%s' to illegal class %d/%d.", spell_name(spell),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (slot < 1 || slot > LVL_IMPL)
     { log("SYSERR: assigning '%s' to illegal slot %d/%d.", spell_name(spell),
           slot, LVL_IMPL);
       bad = 1;
     }

  if (!bad)
     for (i = 0; i < NUM_CLASSES; i++) //���������� ��� ������ ����� (slot)
         if (chclass & (1 << i))
            spell_info[spell].slot_forc[i] = slot;
}

void mskill_level(int spell, int chclass, int level)//������� � ����� ������ ��� �������
{
  int bad = 0, i;

  if (spell < 0 || spell > MAX_SKILLS + 1)
  { log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, MAX_SKILLS + 1);
    return;
  }

  if (chclass <= 0 || chclass >= 1 << NUM_CLASSES)
  { log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 0 || level > LVL_IMPL)
     { log("SYSERR: assigning '%s' to illegal level %d/%d.", skill_name(spell),
           level, LVL_IMPL);
       bad = 1;
     }

  if (!bad)
     for (i = 0; i < NUM_CLASSES; i++)
         if (chclass & (1 << i))
            skill_info[spell].k_improove[i] = level;
}

#define MAX_REMORT 15
void mskill_remort(int spell, int chclass, int remort)//������� � ����� ������ ��� �������.
{
  int bad = 0, i;

  if (spell < 0 || spell > MAX_SKILLS + 1)
  { log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, MAX_SKILLS + 1);
    return;
  }

  if (chclass <= 0 || chclass >= 1 << NUM_CLASSES)
  { log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (remort < 0 || remort > MAX_REMORT)
     { log("SYSERR: assigning '%s' to illegal remort %d/%d.", skill_name(spell),
           remort, LVL_IMPL);
       bad = 1;
     }

  if (!bad)
     for (i = 0; i < NUM_CLASSES; i++) 
         if (chclass & (1 << i))
            skill_info[spell].n_remort[i] = remort;
}


void asciiflag_conv1(char *flag, void *value);

void aligment_spell(int spell, int chclass, char *slot)
{
  int bad = 0, i;

  if (spell < 0 || spell > TOP_SPELL_DEFINE)
  { log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass <= 0 || chclass >= 1 << NUM_CLASSES)
  { log("SYSERR: assigning '%s' to illegal class %d/%d.", spell_name(spell),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

 
  if (!bad)
     for (i = 0; i < NUM_CLASSES; i++) //���������� ��� ������ �������� ���������� (slot)
         if (chclass & (1 << i))
            asciiflag_conv1(slot, &spell_info[spell].aligment[i]);
}

void aligment_skill(int spell, int chclass, char *slot)
{
  int bad = 0, i;

  if (spell < 0 || spell > MAX_SKILLS + 1)
  { log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, MAX_SKILLS + 1);
    return;
  }

  if (chclass <= 0 || chclass >= 1 << NUM_CLASSES)
  { log("SYSERR: assigning '%s' to illegal class %d/%d.", spell_name(spell),
                chclass, NUM_CLASSES - 1);
    bad = 1;
  }

 /* if (slot < 0 || slot > 7)
     { log("SYSERR: assigning '%s' to illegal aligment %d/%d.", skill_name(spell),
           slot, LVL_IMPL);
       bad = 1;
     }*/

  if (!bad)
     for (i = 0; i < NUM_CLASSES; i++) //���������� ��� ������ �������� ������ (slot)
         if (chclass & (1 << i))
            asciiflag_conv1(slot, &skill_info[spell].a_flags[i]);
}
/***********************************************************************************************/




/* Assign the spells on boot up */
void spello(int spl, const char *name, const char *syn, int max_mana, int min_mana,
	int mana_change, int minpos, int targets, int violent, int routines, int danger)
{
  int i;
//�������� ���� ���� ��������
  for (i = 0; i < NUM_CLASSES; i++) {
      spell_info[spl].min_level[i] =0;// LVL_IMMORT;
      spell_info[spl].slot_forc[i] = MAX_SLOT;
	}

	  spell_info[spl].mana_max		= max_mana;
	  spell_info[spl].mana_min	    	= min_mana;
	  spell_info[spl].mana_change		= mana_change;
	  spell_info[spl].min_position		= minpos;
	  spell_info[spl].danger		= danger;
	  spell_info[spl].targets		= targets;
	  spell_info[spl].violent		= violent;
	  spell_info[spl].routines		= routines;
	  spell_info[spl].name			= name;
      	  spell_info[spl].syn			= syn;
}


void unused_spell(int spl)
{
  int i;
//�������� �������� ��������
  for (i = 0; i < NUM_CLASSES; i++) {
    spell_info[spl].min_level[i] =0;// LVL_IMPL + 1;
    spell_info[spl].slot_forc[i] = MAX_SLOT;
  }
  
  for (i = 0; i < 3; i++)
  {spell_create[spl].wand.items[i]   = -1;
  spell_create[spl].scroll.items[i] = -1;
  spell_create[spl].potion.items[i] = -1;
  spell_create[spl].items.items[i]  = -1;
  spell_create[spl].runes.items[i]  = -1;
   }

  spell_create[spl].wand.rnumber    = -1;
  spell_create[spl].scroll.rnumber  = -1;
  spell_create[spl].potion.rnumber  = -1;
  spell_create[spl].items.rnumber   = -1;
  spell_create[spl].runes.rnumber   = -1;

  spell_info[spl].mana_max		= 0;
  spell_info[spl].mana_min		= 0;
  spell_info[spl].mana_change   = 0;
  spell_info[spl].min_position  = 0;
  spell_info[spl].danger        = 0;
  spell_info[spl].targets		= 0;
  spell_info[spl].violent		= 0;
  spell_info[spl].routines		= 0;
  spell_info[spl].name			= unused_spellname;
  spell_info[spl].syn			= unused_spellname;
}

//#define skillo(skill, name) spello(skill, name, 0, 0, 0, 0, 0, 0, 0);

void skillo(int spl, const char *name, int max_percent)
{
  int i;
  for (i = 0; i < NUM_CLASSES; i++)
  skill_info[spl].k_improove[i] = LVL_IMMORT;
  skill_info[spl].min_position    = 0;
  skill_info[spl].name            = name;
  skill_info[spl].max_percent     = max_percent;
}

void unused_skill(int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
      skill_info[spl].k_improove[i] = 0;
  skill_info[spl].min_position      = 0;
  skill_info[spl].max_percent       = 100;
  skill_info[spl].name              = unused_spellname;
}
/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * spellname: The name of the spell.
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells(void)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i <= TOP_SPELL_DEFINE; i++)
    unused_spell(i);
  for (i = 0; i <= MAX_SKILLS+1; i++)
    unused_skill(i);
  /* Do not change the loop above. */

//1
  spello(SPELL_ARMOR, "������", "Gratadh",
         65, 30, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//2
  spello(SPELL_TELEPORT, "��������", "Ire Lokke",
         240, 120, 10, POS_STANDING,
         TAR_CHAR_ROOM, FALSE,
         MAG_MANUAL | NPC_DAMAGE_PC, 1);
//3
  spello(SPELL_BLESS, "�����������", "Seifreisse",
         70, 20, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV, FALSE,
         MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_NPC, 0);
//4
  spello(SPELL_BLINDNESS, "�������", "Baeliend",
         70, 20, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//5
  spello(SPELL_BURNING_HANDS, "������� ����", "Hoel Eveith",
         80, 20, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC,1);
//6
  spello(SPELL_CALL_LIGHTNING, "������� ������", "Muiarenn",
         70, 20, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AIR,
         MAG_DAMAGE | NPC_DAMAGE_PC, 2);
//7
  spello(SPELL_CHARM, "����������", "En'leass",
         70, 37, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, MTYPE_NEUTRAL, MAG_MANUAL, 1);
//8
  spello(SPELL_CHILL_TOUCH, "������� �������������", "Chill Touch",
         55, 45, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_WATER,
         MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC,1);
//9
  spello(SPELL_CLONE, "������������", "Clone",
         190, 100, 10, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_SUMMONS, 0);
//10
  spello(SPELL_COLOR_SPRAY, "�������� ������", "Feain Dairenn",
         120, 90, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AIR,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 3);
//11
  spello(SPELL_CONTROL_WEATHER, "�������� ������", "Tedd Vaen",
         300, 150, 15, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_MANUAL, 0);
//12
  spello(SPELL_CREATE_FOOD, "������� ����", "Saie Leam",
         45, 20, 3, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_CREATIONS, 0);
//13
  spello(SPELL_CREATE_WATER, "������� ����", "Saie Muire",
         45, 20, 3, POS_STANDING,
         TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_CHAR_ROOM, FALSE, MAG_MANUAL, 0);
//14
  spello(SPELL_CURE_BLIND, "�������� �������", "Laend Civian",
         85, 35, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 0);
//15
  spello(SPELL_CURE_CRITIC, "����������� ���������", "Cuain aeshedh",
         120, 30, 8, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | NPC_DUMMY, 3);
//16
  spello(SPELL_CURE_LIGHT, "������ ���������", "Cuain Hleith",
         80, 20, 5, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | NPC_DUMMY,1);
//17
  spello(SPELL_CURSE, "���������", "Geas",
         65, 35, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_INV,  MTYPE_NEUTRAL,
         MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_PC, 1);
//18
  spello(SPELL_DETECT_ALIGN, "���������� �����������", "Naen Dice'nte",
         45, 20, 2, POS_STANDING,
        TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//19
  spello(SPELL_DETECT_INVIS, "������ ���������", "Va Caelm",
         110, 25, 6, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//20
  spello(SPELL_DETECT_MAGIC, "���������� �����", "Detuct Migev",
         110, 25, 6, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//21
  spello(SPELL_DETECT_POISON, "���������� ��", "Saeve Lie'nn",
         100, 25, 7, POS_STANDING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL, 0);
//22
  spello(SPELL_DISPEL_EVIL, "�������� ���", "Dispel Evil",
         100, 40, 8, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL, MAG_DAMAGE, 1);
//23
  spello(SPELL_EARTHQUAKE, "�������������", "Beidgereith",
         240, 110, 10, POS_FIGHTING,
         TAR_IGNORE, MTYPE_EARTH,
         MAG_AREAS | NPC_DAMAGE_PC, 2);
//24
  spello(SPELL_ENCHANT_WEAPON, "����������� ������", "Enchant Weapon",
         280, 180, 10, POS_STANDING,
         TAR_OBJ_INV, FALSE, MAG_MANUAL, 0);
//25
  spello(SPELL_ENERGY_DRAIN, "�������� �������", "Esorit Puener",
         150, 140, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | MAG_AFFECTS | NPC_DAMAGE_PC, 1);
//26
  spello(SPELL_FIREBALL, "�������� ���", "Aenyell Hael",
         110, 55, 6, POS_FIGHTING,
         TAR_IGNORE, MTYPE_FIRE, MAG_AREAS | NPC_DAMAGE_PC, 5);
 //      MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2);
//27
  spello(SPELL_HARM, "����", "Mestro Auter",
         200, 130, 8, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC, 5);
//28
  spello(SPELL_HEAL, "���������", "Fiolie",
         180, 40, 10, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | MAG_UNAFFECTS | NPC_DUMMY, 10);
//29
  spello(SPELL_INVISIBLE, "�����������", "Sqaess",
         80, 40, 4, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM,
         FALSE, MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_NPC, 0);
//30
  spello(SPELL_LIGHTNING_BOLT, "������", "Feain'rennt",
         90, 45, 5, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC, 1);
//31
  spello(SPELL_LOCATE_OBJECT, "����� �������", "Ain Saevhernet",
         250, 150, 10, POS_STANDING,
         TAR_OBJ_WORLD, FALSE, MAG_MANUAL, 0);
//32
  spello(SPELL_MAGIC_MISSILE, "���������� �������", "Meveli Maeth",
         90, 10, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC, 1);
//33
  spello(SPELL_POISON, "��", "Lie'n",
         120, 55, 8, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | MAG_ALTER_OBJS | NPC_AFFECT_PC, 2);
//34
  spello(SPELL_PROT_FROM_EVIL, "������������� ����", "Strusa Temur",
         60, 45, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//35
  spello(SPELL_REMOVE_CURSE, "����� ���������", "Ean Nyell",
         65, 30, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
         MAG_UNAFFECTS | MAG_ALTER_OBJS | NPC_UNAFFECT_NPC, 0);
//36
  spello(SPELL_SANCTUARY, "���������", "Vessekheal",
         95, 40, 5, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS   | NPC_AFFECT_NPC, 1);
//37
  spello(SPELL_SHOCKING_GRASP, "���������� ������", "Shel Aleite",
         50, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC, 1);
//38
  spello(SPELL_SLEEP, "���", "Dearme",
         70, 45, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
         MAG_AFFECTS | NPC_AFFECT_PC, 0);
//39
  spello(SPELL_STRENGTH, "����", "Veelde",
         75, 40, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//40
  spello(SPELL_SUMMON, "��������", "Terutto Autirro",
         140, 120, 2, POS_STANDING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL, 0);
//41-not used now
//42
  spello(SPELL_WORD_OF_RECALL, "����� ��������", "Weni Vutores",
         140, 70, 8, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_MANUAL | NPC_DAMAGE_PC, 0);
//43
  spello(SPELL_REMOVE_POISON, "�������������� ��", "Ness Lie'n",
         60, 35, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE,
         MAG_UNAFFECTS | MAG_ALTER_OBJS | NPC_UNAFFECT_NPC, 0);
//44
  spello(SPELL_SENSE_LIFE, "����������� �����", "Dearg Leith",
         85, 45, 4, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//45
  spello(SPELL_ANIMATE_DEAD, "������� ����", "Truspo Exo",
         50, 35, 3, POS_STANDING,
         TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 0);
//46
  spello(SPELL_DISPEL_GOOD, "�������� ����", "Jurio Plex",
         100, 90, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL, MAG_DAMAGE, 1);
//47
  spello(SPELL_GROUP_ARMOR, "��������� ������", "Freved Gratadh",
         230, 150, 10, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_AFFECT_NPC, 0);
//48
  spello(SPELL_GROUP_HEAL, "��������� ���������", "Wstor's Utospe",
         210, 150, 50, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_DUMMY, 30);
//49
  spello(SPELL_GROUP_RECALL, "��������� �������", "Group Recall",
         125, 120, 2, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, 0);
//50
  spello(SPELL_INFRAVISION, "�����������", "Isferlian",
         60, 30, 3, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//51
  spello(SPELL_WATERWALK, "������������", "Waterwalk",
         70, 55, 1, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//52
  spello(SPELL_CURE_SERIOUS, "��������� ���������", "Cuain Voerle",
         75, 25, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | NPC_DUMMY, 2);
//53
  spello(SPELL_GROUP_STRENGTH, "��������� ����", "Freved Veelde",
         160, 100, 7, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_AFFECT_NPC, 0);
//54 
  spello(SPELL_HOLD, "�������", "Beit'ness",//hold
         100, 40, 6, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 3);
//55
  spello(SPELL_POWER_HOLD, "���������� �������", "T'en Pavien",
         120, 50, 6, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 4);
//56
  spello(SPELL_MASS_HOLD, "�������� ����������", "Cepos Zhutx",
         150, 130, 5, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 5);
//57
  spello(SPELL_FLY, "�����", "Thuates",
         60, 30, 3, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//58
  spello(SPELL_UNDEAD_HOLD, "���������� ������", "Ten Unauf",
         100, 40, 6, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 4);
//59
  spello(SPELL_NOFLEE, "����������", "Trouf'ex",
         100, 90, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 0);
//60
  spello(SPELL_CREATE_LIGHT, "������� ����", "Aine Tynos",
         40, 20, 3, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_CREATIONS, 0);
//61
  spello(SPELL_DARKNESS, "����", "Darkness",
         100, 70, 3, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,0);
//62
  spello(SPELL_STONESKIN, "����������", "Stoneskin",
         65, 30, 4, POS_FIGHTING,
         TAR_CHAR_ROOM , FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//63
  spello(SPELL_CLOUDLY, "�������������", "Delaith",
         75, 30, 4, POS_FIGHTING,
         TAR_CHAR_ROOM , FALSE, MAG_AFFECTS, 0);
	
//64
  spello(SPELL_SIELENCE, "��������", "Thaes Aep",
         100, 40, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 1);
//65
  spello(SPELL_LIGHT, "����", "Aine Verseos",
         130, 70, 5, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//66
  spello(SPELL_CHAIN_LIGHTNING, "����� ������", "Chain Lightning",
         170, 75, 10, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_AIR,
         MAG_MANUAL |  NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 1);       
//67
  spello(SPELL_FIREBLAST, "��������� ���", "Cluter Ducas",
         150, 90, 20, POS_FIGHTING,
         TAR_IGNORE , MTYPE_AIR,
         MAG_AREAS | NPC_DAMAGE_PC, 5);
//68
  spello(SPELL_IMPLOSION, "���� �����", "Naton's Powers",
         220, 150, 10, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_EARTH,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,15);
//69
  spello(SPELL_WEAKNESS, "��������", "Loenles",
         75, 45, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 0);
//70
  spello(SPELL_GROUP_INVISIBLE, "��������� �����������", "Aell Squaess",
         150, 130, 5, POS_STANDING, TAR_IGNORE,
         FALSE, MAG_GROUPS | NPC_AFFECT_NPC, 0);
//71
  spello(SPELL_PROTECT_MAGIC, "������������� �����", "Veisemag",
         95, 40, 5, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS   | NPC_AFFECT_NPC, 1);

//72
  spello(SPELL_ACID, "��������� ������", "Alinde Maeth",
         90, 45, 5, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | MAG_ALTER_OBJS | NPC_DAMAGE_PC,2);
//73
  spello(SPELL_REPAIR, "���������� ������", "Restor",
         110, 100, 1, POS_STANDING,
         TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_ALTER_OBJS, 0);
//74
  spello(SPELL_ENLARGE, "����������", "Sefierne",//enlarge
         65, 40, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
	
//75
  spello(SPELL_FEAR, "�����", "Gar'ean",//fear
         80, 35, 5, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE,
         MAG_DAMAGE | NPC_DAMAGE_PC,1);
//76
  spello(SPELL_SACRIFICE, "�������� �����", "Sa'crofine",
         180, 125, 7, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_EARTH,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,10);
//77
  spello(SPELL_WEB, "����", "Weruns",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//78
  spello(SPELL_BLINK, "�������", "Exuit",
         70, 55, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_SELF_ONLY, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//79
  spello(SPELL_REMOVE_HOLD, "����� ����������", "Liened Beith",
         110, 90, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 1);
//80
  spello(SPELL_CAMOUFLAGE, "!����������!", "!set by skill!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//81
  spello(SPELL_POWER_BLINDNESS, "������ �������", "Putow'r Bial",
         110, 100, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 2);
//82
  spello(SPELL_MASS_BLINDNESS, "�������� �������", "Va'ts Glasur",
         140, 120, 2, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 2);
//83
  spello(SPELL_POWER_SIELENCE, "���������� ��������", "Kachumaj'lox",
         120, 90, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 2);
//84
  spello(SPELL_EXTRA_HITS, "��������� �����", "Sefier Leith",
         130, 70, 7, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_POINTS | NPC_DUMMY,1);
//85
  spello(SPELL_RESSURECTION, "������� ����", "Ressurection",
         120, 100, 2, POS_STANDING,
         TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 0);
//86-not used now

//87
  spello(SPELL_FORBIDDEN, "���������� �������", "Tychan Windu",
         125, 110, 2, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_AREAS, 0);
//88
  spello(SPELL_MASS_SIELENCE, "�������� ��������", "Aluc'ero Kachum",
         140, 120, 2, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 2);
//89
  spello(SPELL_REMOVE_SIELENCE, "����� ��������", "Sh'aente",
         70, 55, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_UNAFFECTS | NPC_UNAFFECT_NPC | NPC_UNAFFECT_NPC_CASTER, 1);
//90
  spello(SPELL_DAMAGE_LIGHT, "������ ����", "Bloede Ars",
        50, 20, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC,1);
//91
  spello(SPELL_DAMAGE_SERIOUS, "��������� ����", "Bloede Caerme",
         80, 45, 6, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC, 2);
//92
  spello(SPELL_DAMAGE_CRITIC, "����������� ����", "N'eiss Caerme",
         95, 55, 8, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC, 3);
//93
  spello(SPELL_MASS_CURSE, "�������� ���������", "Proclus All",
         140, 120, 2, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 2);
//94
  spello(SPELL_ARMAGEDDON, "���������", "Deireadh",
         190, 130, 8, POS_FIGHTING,
         TAR_IGNORE, MTYPE_EARTH,
         MAG_AREAS | NPC_DAMAGE_PC, 10);
//95
  spello(SPELL_GROUP_FLY, "�������� �����", "Aell Thuates",
         160, 100, 4, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_MASSES | MAG_ALL, 2);
//96
  spello(SPELL_GROUP_BLESS, "��������� �����������", "Aell Seifreisse",
         120, 70, 4, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_AFFECT_NPC, 1);
//97
  spello(SPELL_REFRESH, "��������������", "Straede",
         90, 50, 4, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS,0);
//98
  spello(SPELL_STUNNING, "�������� ���������", "Steine Deas",
         150, 140, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL, MAG_DAMAGE,15);

//99
  spello(SPELL_HIDE, "���������", "!set by skill!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//100
  spello(SPELL_SNEAK, "��������", "!set by skill!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//101
  spello(SPELL_DRUNKED, "���������", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//102
  spello(SPELL_ABSTINENT, "�����������", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//103
  spello(SPELL_FULL, "���������", "Ferkeas",
         80, 35, 4, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS, 10);
//104
  spello(SPELL_CONE_OF_COLD, "����� ������", "Cold Wind",
         140, 60, 5, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_WATER,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,15);
//105
  spello(SPELL_BATTLE, "������� � ���", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//106
  spello(SPELL_HAEMORRAGIA, "������������", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//107
  spello(SPELL_COURAGE, "�������", "!set by programm!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//108
  spello(SPELL_WATERBREATH, "������ �����", "Utosy Dutarony",
         100, 90, 1, POS_STANDING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 0);
//109
  spello(SPELL_SLOW, "����������", "Vailn'ess",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//110
  spello(SPELL_HASTE, "���������", "Ques Evall",
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//111
  spello(SPELL_MASS_SLOW, "�������� ��������������", "Aell Vailn'ess",
         150, 120, 2, POS_FIGHTING,
         TAR_IGNORE,  MTYPE_NEUTRAL,
         MAG_MASSES | NPC_AFFECT_PC, 2);
//112
  spello(SPELL_GROUP_HASTE, "��������� ���������", "Ques Mnugos",
         110, 100, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_AFFECT_NPC, 1);
//113
  spello(SPELL_SHIELD, "������ �����", "Gratha Oide",
         150, 140, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 2);
//114
  spello(SPELL_PLAQUE, "���������", "Pecencu'sa",
         70, 55, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 2);
//115
  spello(SPELL_CURE_PLAQUE, "�������� ���������", "Cur Pecencu'sa",
         110, 90, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_UNAFFECTS | NPC_UNAFFECT_NPC, 0);
//116
  spello(SPELL_AWARNESS, "��������������", "Visneite",
         100, 90, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//117
  spello(SPELL_RELIGION, "!������� ��� ������!", "!pray or donate!",
         0, 0, 0, 255,
         0, FALSE, MAG_MANUAL, 0);

//118
  spello(SPELL_AIR_SHIELD, "���������� ���", "Mutedas To'tuons",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//119
  spello(SPELL_PORTAL, "������� ������", "Ard Caeth",
         180, 120, 5, POS_STANDING,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL, 0);
//120
  spello(SPELL_DISPELL_MAGIC, "�������� �����", "Scertus Mutedas",
         120, 40, 9, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE, MAG_UNAFFECTS, 0);
//121
  spello(SPELL_SUMMON_KEEPER, "��������", "Presgie",
         100, 80, 2, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS, 0);
//122
  spello(SPELL_FAST_REGENERATION, "�����������", "Nudestah",//����������� �� ������ ����� � ��������� � ����� 31.07.2007�.
         220, 90, 25, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS| MAG_UNAFFECTS  | NPC_AFFECT_NPC, 0);
//123
  spello(SPELL_CREATE_WEAPON, "������� ������", "Gasurt Nerwal",
         130, 110, 2, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_MANUAL, 0);
//124
  spello(SPELL_FIRE_SHIELD, "�������� ���", "Af'ew Fdaqers",
         150, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//125
  spello(SPELL_RELOCATE, "�������������", "Hervien",
         140, 120, 2, POS_STANDING,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL, 0);
//126
  spello(SPELL_SUMMON_FIREKEEPER, "�������� ��������", "Af'ew Tipus",
         150, 140, 1, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_SUMMONS, 0);
//127
  spello(SPELL_ICE_SHIELD, "������� ���", "Dfaseq Stug",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//128
  spello(SPELL_ICESTORM, "������� �����", "Dfaseq Burens",
         155, 110, 3, POS_FIGHTING,
         TAR_IGNORE, MTYPE_WATER,
         MAG_AREAS | NPC_DAMAGE_PC, 5);
//129
  spello(SPELL_ENLESS, "����������", "Sefimine",// ��������� � ����� 01.04.2007�.
         55, 40, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS, 0);
//130
  spello(SPELL_SHINEFLASH, "����� ����", "Vesol Poqas",
         60, 45, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_FIRE,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP, 2);
//131
  spello(SPELL_MADNESS, "�������", "Delleion",
         130, 110, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//132
  spello(SPELL_MAGICGLASS, "���������� �������", "Mine Veide",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//133
  spello(SPELL_STAIRS, "��������", "Stairs",
         85, 70, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//134
  spello(SPELL_VACUUM, "���� �������", "Goawe Tree",
         150, 140, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_DAMAGE_PC | NPC_DAMAGE_PC_MINHP,15);
//135
  spello(SPELL_METEORSTORM, "���� ������", "Die...",
         125, 110, 2, POS_FIGHTING,
         TAR_IGNORE, MTYPE_EARTH,
         MAG_MASSES | MAG_ALL | NPC_DAMAGE_PC, 5);
//136
  spello(SPELL_STONEHAND, "�������� ����", "Bugas Postew",
         50, 30, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
		
//137
  spello(SPELL_MINDLESS, "����������� ������", "Mutos Axits",
         120, 110, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC | NPC_AFFECT_PC_CASTER, 0);
//138
  spello(SPELL_PRISMATICAURA, "���������", "Vampiric Transformation",
         85, 70, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 1);
//139
  spello(SPELL_EVILESS, "���� ���", "Storew Loxeq",
         150, 130, 5, POS_STANDING, TAR_IGNORE, FALSE,
	 MAG_MANUAL, 0);
//140
  spello(SPELL_AIR_AURA, "������������� �������", "Vlagos Vewas",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//141
  spello(SPELL_FIRE_AURA, "������������� ����", "Vlagos Tresq",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//142
  spello(SPELL_ICE_AURA, "������������� ������", "Vlagos Brelah",
         140, 120, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_SELF, FALSE,
         MAG_AFFECTS | NPC_AFFECT_NPC, 0);
//143
  spello(SPELL_ZNAK_AARD, "����", "Aard",
         170, 120, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_AFFECT_PC, 1);
//144
  spello(SPELL_ZNAK_IGNI, "����", "Igni",
         70, 40, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
         MAG_DAMAGE | NPC_AFFECT_PC | NPC_DAMAGE_PC, 1);
//145  
   spello(SPELL_ZNAK_IRGEN, "�����", "Irgen",
         110, 55, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//146
   spello(SPELL_ZNAK_AKSY, "�����", "Aksy",
         110, 55, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//147 
    spello(SPELL_ZNAK_GELIOTROP, "���������", "Geliotrop",
         120, 70, 10, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
//148
    spello(SPELL_ZNAK_KVEN, "����", "Kven",
         140, 110, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);	
//149
spello(SPELL_OWER_HOLD, "����� ��������", "Ten Pavien",
         140, 90, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT,  MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 4);
	
//150
spello(SPELL_UNLUCK, "�������", "Bahuit",
         70, 20, 4, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, MTYPE_NEUTRAL,
         MAG_AFFECTS | NPC_AFFECT_PC, 1);
//151
spello (SPELL_GROUP_REFRESH, "��������� ��������������", "Freved Straede",
	  160, 140, 1, POS_STANDING,
	  TAR_IGNORE, FALSE, MAG_GROUPS | MAG_POINTS | NPC_DUMMY, 30);

//152
spello(SPELL_MENTALLS, "���������� ����", "Menues Poues",
         140, 100, 4, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | NPC_AFFECT_NPC, 0);

//153
spello(SPELL_DETECT_MENTALLS, "������ ������������", "Va Caelm",
         120, 45, 6, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, 0);
// 154
spello(SPELL_SUMMON_ANIMAL, "�������� ��������", "Vocare Animalis",
         420, 220, 10, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_SUMMONS, 0);
  
//155
spello(SPELL_GROUP_CURE_CRITIC, "������� ��������", "Cuain aeshedh maxima",
         210, 150, 50, POS_FIGHTING,
         TAR_IGNORE, FALSE,
         MAG_GROUPS | NPC_DUMMY, 30);
//156
spello(SPELL_RESTORE_MAGIC, "�������������� �����", "Retora bedaas magicus",
       50, 80, 10, POS_STANDING,
       TAR_CHAR_ROOM, FALSE,
       MAG_MANUAL, 0);


  /* NON-castable spells should appear below here. */
//351
  spello(SPELL_IDENTIFY, "��������", "identify", 0, 0, 0, 0,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL,0);

  /*
   * These spells are currently not used, not implemented, and not castable.
   * Values for the 'breath' spells are filled in assuming a dragon's breath.
   */
  spello(SPELL_FIRE_BREATH, "�������� �������", "fire breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);

  spello(SPELL_GAS_BREATH, "��������� �������", "gas breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);

  spello(SPELL_FROST_BREATH, "������� �������", "frost breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);

  spello(SPELL_ACID_BREATH, "��������� �������", "acid breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);

  spello(SPELL_LIGHTNING_BREATH, "��������� �������", "lightning breath",
         0, 0, 0, POS_SITTING,
             TAR_IGNORE, TRUE, 0, 0);


  /*
   * Declaration of skills - this actually doesn't do anything except
   * set it up so that immortals can use these skills by default.  The
   * min level to use the skill for other classes is set up in class.c.
   */	
  skillo(SKILL_AWAKE,			"���������� ���", 100);
  skillo(SKILL_CLUBS,			"������ � ������", 100);
  skillo(SKILL_PICK,			"����������� ������", 100);
  skillo(SKILL_PARRY,			"����������", 120);
  skillo(SKILL_BLOCK,			"����������� �����", 120);
  skillo(SKILL_BACKSTAB,		"��������", 100); 
  skillo(SKILL_BASH,			"�����", 100);
  skillo(SKILL_HIDE,			"����������", 100);
  skillo(SKILL_KICK,			"�����", 100);
  skillo(SKILL_PICK_LOCK,		"��������", 100);
  skillo(SKILL_PUNCH,			"���������� ���", 100);
  skillo(SKILL_RESCUE,			"������", 100);
  skillo(SKILL_SNEAK,			"�����������",100); 
  skillo(SKILL_STEAL,			"�������", 100); 
  skillo(SKILL_TRACK,			"���������", 100);
  skillo(SKILL_BOTHHANDS,		"����������", 120);
  skillo(SKILL_SECOND_ATTACK,	"������ ����", 100); 
  skillo(SKILL_AXES,			"������", 100);
  skillo(SKILL_LONGS,			"������� ������", 120);
  skillo(SKILL_SHORTS,			"�������� ������", 120);
  skillo(SKILL_TWO_WEAPON,		"��� ������", 100);
  skillo(SKILL_SHIELD_MASTERY,	"�������� �����", 100);   
  skillo(SKILL_BOTH_WEAPON,		"��������� ������", 100); 
  skillo(SKILL_DISARM,			"�����������", 100);
  skillo(SKILL_NONSTANDART,		"������������� ������", 100);
  skillo(SKILL_COURAGE,			"�������", 100);
  skillo(SKILL_PROTECT,         "��������", 120);
  skillo(SKILL_BOWS,			"����", 100);
  skillo(SKILL_SPADES,			"�����", 100);
  skillo(SKILL_LOOKING,         "������������", 100);
  skillo(SKILL_TURN,            "turn", 100);
  skillo(SKILL_ADDSHOT,         "�������������� �������", 100);
  skillo(SKILL_CAMOUFLAGE,      "����������", 100);
  skillo(SKILL_DEVIATE,         "����������", 100);
  skillo(SKILL_CHOPOFF,         "��������", 100);
  skillo(SKILL_REPAIR,          "��������", 100);
  skillo(SKILL_IDENTIFY,        "��������", 100);
  skillo(SKILL_LOOK_HIDE,       "�����������", 100);
  skillo(SKILL_UPGRADE,         "��������", 100);
  skillo(SKILL_ARMORED,         "�������", 100);
  skillo(SKILL_AID,				"����������", 100);
  skillo(SKILL_FIRE,			"�������� ������", 100);
  skillo(SKILL_SHIT,			"���� ����� �����", 100);
  skillo(SKILL_MIGHTHIT,		"����������", 100);
  skillo(SKILL_STUPOR,			"��������", 100);
  skillo(SKILL_POISONED,		"������� ��", 100);
  skillo(SKILL_LEADERSHIP,		"���������", 100);
  skillo(SKILL_PUNCTUAL,		"����� ������", 100);
  skillo(SKILL_SENSE,			"�����", 100);
  skillo(SKILL_RIDING,			"�������� ����", 100);
  skillo(SKILL_HORSE,			"�������� ���", 100);
  skillo(SKILL_HIDETRACK,		"������� �����", 120);
  skillo(SKILL_RELIGION,		"������� ��� ������", 100);
  skillo(SKILL_MAKEFOOD,		"����������", 120);
  skillo(SKILL_TEAR,			"�������", 120);
  skillo(SKILL_MULTYPARRY,      "������� ������", 140);
  skillo(SKILL_SINGLE_WEAPON,   "������ � ����� ����",100);
//  skillo(SKILL_TRANSFORMWEAPON, "����������", 140);
  skillo(SKILL_THROW,           "�������", 150);
 // skillo(SKILL_CREATEBOW,		"���������� ���", 140);
  skillo(SKILL_TOCHNY,			"������ ����", 100);
   skillo(SKILL_BASH_DOOR,		"������ �����", 100);
   skillo(SKILL_BLADE_VORTEX,		"����� �������", 100);
	  
}

