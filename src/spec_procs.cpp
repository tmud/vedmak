/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
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
#include "screen.h"

//   external vars  
extern struct room_data *world;
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern struct char_data *mob_proto;
extern int guild_info[][3];
extern int top_of_mobt;

// extern functions 
int  go_track(struct char_data *ch, struct char_data *victim, int skill_no);
int  check_recipe_items(struct char_data * ch, int spellnum, int spelltype, int extract);
int  find_first_step(room_rnum src, room_rnum target, struct char_data *ch);
int  has_key(struct char_data *ch, obj_vnum key);
int  slot_for_char1(struct char_data *ch, int slotnum);
void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
void add_follower(struct char_data * ch, struct char_data * leader);
void extract_char(struct char_data *ch, int clear_objs);
int  get_kill_vnum(struct char_data *ch, int vnum);

ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);
ACMD(do_umenia);
ACMD(do_weapon_umenia);
ACMD(do_zackl);
// local functions 
void sort_spells(void);
void quest_update(struct char_data * ch);
int compare_spells(const void *x, const void *y);
const char *how_good(int percent);
void list_skills(CHAR_DATA * ch, CHAR_DATA * vict, bool);
void generate_quest(struct char_data *ch, struct char_data *questman);

SPECIAL(guild);
SPECIAL(guild1);
SPECIAL(dump);
SPECIAL(mayor);
void do_npc_steal(struct char_data * ch, struct char_data * victim);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
//SPECIAL(kladovchik);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(shop_keeper);
SPECIAL(horse_shops);
SPECIAL(questmaster);
SPECIAL(power_stat);

#define IS_SHOPKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == shop_keeper)
#define IS_QUESTOR(ch)     (PLR_FLAGGED(ch, PLR_QUESTOR))

/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

int spell_sort_info[MAX_SKILLS + 1];

int compare_spells(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a].name, spell_info[b].name);
}

void sort_spells(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a <= MAX_SKILLS; a++)
    spell_sort_info[a] = a;

  qsort(&spell_sort_info[1], MAX_SKILLS, sizeof(int), compare_spells);
}

/*const char *how_good(int percent)
{
if (percent < 0) 
    return " error)";
  if (percent == 0) 
    return " (\\c14�� �������\\c00)"; 
  if (percent <= 7) 
    return " (\\c01������\\c00)";
  if (percent <= 14)  
    return " (\\c08����� �����\\c00)"; 
  if (percent <= 21)
    return " (\\c08�����\\c00)";
  if (percent <= 28) 
    return " (\\c03���� ��������\\c00)";
  if (percent <= 35) 
    return " (\\c10�������\\c00)";
  if (percent <= 42) 
    return " (\\c10���� ��������\\c00)"; 
  if (percent <= 49)  
    return " (\\c06������\\c00)";
  if (percent <= 56) 
   return " (\\c06����� ������\\c00)";
  if (percent <= 63)
    return " (\\c06�������\\c00)";
  if (percent <= 70)
    return " (\\c13�����������\\c00)";
  if (percent <= 77)
    return " (\\c13�����������\\c00)";
  if (percent <= 84)
    return " (\\c13��������\\c00)";
  if (percent <= 91)
    return " (\\c13����������\\c00)";
  if (percent <= 100)
    return " (\\c14����������\\c00)";
  if (percent <= 110)
    return " (\\c14����������\\c00)";
  if (percent <= 135)
    return " (\\c14�����������\\c00)";
  if (percent <= 170)
    return " (\\c14�����������\\c00)";
    
  return " (\\c14���������!!!\\c00)"; 
} */


const char *how_good(int percent)
{
  if (percent < 0) 
   return " ������";  
if (percent == 0)
    return " &n�� �������&n";  
if (percent <= 7)  
  return " &R������$w&n";  
if (percent <= 14) 
   return " &R�������$w&n";  
if (percent <= 21)
    return " &r���������$w&n";
  if (percent <= 28)
    return " &r�������$w&n";  
if (percent <= 35)
    return " &y���������$w&n";  
if (percent <= 42)
    return " &y������$w&n";  
if (percent <= 49)
    return " &Y�����$w&n";  
if (percent <= 56) 
   return " &Y��������������$w&n";
  if (percent <= 63)
    return " &g���������&n";
  if (percent <= 70)
    return " &g��������$w&n";
  if (percent <= 76)
    return " &G�������&n";
  if (percent <= 83)
    return " &G��������&n";
  if (percent <= 91)
    return " &W����������&n";
  if (percent <= 100)
    return " &W����������&n";
  if (percent <= 110)
    return " &W����������&n";
  if (percent <= 135)
    return " \\c14����������\\c00";
  if (percent <= 170)
    return " \\c14�����������\\c00";
    
  return " \\c14���������!!!\\c00";}



const char *prac_types[] = {
  "����������",
  "������"
};

#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */

/* actual prac_params are in class.c */
extern int prac_params[4][NUM_CLASSES];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

void list_skills(CHAR_DATA * ch, CHAR_DATA * vict, bool check, int mode)
{
  int i=0, sortpos, count=0;

  const char *teg = "���������� ��������/�������";
  if (mode == 1)
      teg = "���������� ��������";
  if (mode == 2)
      teg = "��������� �������";

  sprintf(buf, "&n%s �������%s %s:&n\r\n", check ? GET_NAME(ch): "��", check ? "": "�", teg);
  strcpy(buf2, buf);

  for (sortpos = 1; sortpos <= MAX_SKILLS; sortpos++)
  {
      bool weapon = false;
      switch(sortpos)
      {
          case SKILL_CLUBS:
          case SKILL_AXES:
          case SKILL_LONGS:
          case SKILL_SHORTS:
          case SKILL_NONSTANDART:
          case SKILL_BOTHHANDS:
          case SKILL_PICK:
          case SKILL_SPADES:
          case SKILL_BOWS:
          case SKILL_PUNCH:
              weapon = true;
          break;
      }
      if (mode == 1 && weapon)
          continue;
      if (mode == 2 && !weapon)
          continue;
      
       if (strlen(buf2) >= MAX_STRING_LENGTH - 60)
          {strcat(buf2, "**OVERFLOW**\r\n");
           break;
          }
       if (GET_SKILL(ch, sortpos))
          {if (!skill_info[sortpos].name ||
               *skill_info[sortpos].name == '!')
              continue;
       GET_COUNT_SKILL(ch) = ++count;
           switch (sortpos)
           {case SKILL_AID:
            case SKILL_DRUNKOFF:
            case SKILL_IDENTIFY:
            case SKILL_POISONED:
            case SKILL_COURAGE:
            if (timed_by_skill(ch, sortpos))
               sprintf(buf," &R[&c%2d&R]&n ", timed_by_skill(ch, sortpos));
            else
               sprintf(buf,"      ");
            break;
            default:
            sprintf(buf,"      ");
            }
           

		   if (check)
				sprintf(buf+strlen(buf), "&C%s&n %-22s [%3d%%] %s\r\n",GET_PRACTICES(ch, sortpos)==1 ? "*": " ",
				skill_info[sortpos].name, GET_SKILL(ch, sortpos), how_good(GET_SKILL(ch, sortpos)));
		   else
				sprintf(buf+strlen(buf), "&C%s&n %-22s [%3d%%] %s\r\n",GET_PRACTICES(ch, sortpos)==1 ? "*": " ",
				skill_info[sortpos].name, GET_SKILL(ch, sortpos), how_good(GET_SKILL(ch, sortpos)));
           strcat(buf2, buf);	
           i++;
          }
      }
      if (!i)
        sprintf(buf2 + strlen(buf2),"��� ������.\r\n");
      act(buf2, TRUE, vict, 0, ch, TO_CHAR);
}


void list_spells(CHAR_DATA * ch, CHAR_DATA * vict)
{ char names[LVL_IMPL][MAX_STRING_LENGTH];
 int  slots[LVL_IMPL],i,max_slot=0, slot_num, is_full, gcount=0;

 is_full = 0;
 max_slot= 0;
 for (i = 0; i < LVL_IMPL; i++)
     {*names[i] = '\0';
       slots[i] = 0;
     }
 for (i = 1; i <= MAX_SPELLS; i++)
     {if (!GET_SPELL_TYPE(ch,i) || GET_SPELL_TYPE(ch,i) == SPELL_LTEMP)
         continue;
      if (!spell_info[i].name || *spell_info[i].name == '!')
         continue;
      if ((GET_SPELL_TYPE(ch,i) & 0xFF) == SPELL_RUNES &&
          !check_recipe_items(ch,i,SPELL_RUNES,FALSE))
         continue;
      slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)] - 1;
      max_slot = MAX(slot_num+1,max_slot);
      slots[slot_num] += sprintf(names[slot_num]+slots[slot_num], "%s %-25s ",
                                slots[slot_num] % 87 < 27 ? "\r\n" : "  ",
                                spell_info[i].name);
      is_full++;
     };
 
 if (is_full)
    {gcount = sprintf(buf2+gcount,"&n�� �������� ��������� ������:&n");
	 for (i = 0; i < max_slot; i++)
         {if (slots[i] != 0) gcount += sprintf(buf2+gcount,"\r\n&g���� %d&n:",i+1);
          if (slots[i])
             gcount += sprintf(buf2+gcount,"%s",names[i]);
         }
    }
 else
    gcount += sprintf(buf2+gcount,"� ��������� ����� ����� ��� ����������!");
    gcount += sprintf(buf2+gcount,"\r\n");
send_to_char(buf2, vict);
}

ACMD(do_umenia)
{
 if (IS_NPC(ch))
     return;
   list_skills(ch, ch, false, 1);
}

ACMD(do_weapon_umenia)
{
 if (IS_NPC(ch))
     return;
   list_skills(ch, ch, false, 2);
}

SPECIAL(guild)
{	int skill_num=0, percent, i;

	for (i = 0; guild_info[i][0] != -1; i++) {
		if (GET_ROOM_VNUM(IN_ROOM(ch)) == guild_info[i][1]) 
			break;
	}
      if (GET_CLASS(ch) != guild_info[i][0]) 
		  return(0);
	  
	  

  if (IS_NPC(ch) || !CMD_IS("�����"))  {
  if (IS_NPC(ch) || !CMD_IS("�����������"))    return (0);   else {
          skip_spaces(&argument);
             if (!*argument) {
                   send_to_char("��� �� ������ �����������?\r\n", ch);
                   return (1);
			 }
          
			 if(!GET_CRBONUS(ch)) {
			send_to_char("� ��� ��� ������������� ������.\r\n", ch);	 
				 return (1);
			 }
     		if(isname(argument, "����")) {
				 if ((GET_STR_ROLL(ch) + 5) <= GET_STR(ch))
					{ send_to_char("�� ������ �� ������ ����������� ����!\r\n", ch);
					  return (1);
					}
				  GET_STR(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, STR)++;
		          send_to_char("�� �������� ���� ���� �� �������.\r\n", ch);
			 }
			if(isname(argument, "����")) {
				if ((GET_CON_ROLL(ch) + 5) <= GET_CON(ch))
					{ send_to_char("�� ������ �� ������ ����������� ����!\r\n", ch);
                      return (1);
					}
				  GET_MAX_HIT(ch)++;
				  GET_CON(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, CON)++;
                  send_to_char("�� �������� ���� ���� �� �������.\r\n", ch);
			 }
			if(isname(argument, "���������")) {
				  if ((GET_INT_ROLL(ch) + 5) <= GET_INT(ch))
					{ send_to_char("�� ������ �� ������ ����������� ���������!\r\n", ch);
                      return (1);
				  }
                  		  GET_INT(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, INT)++;
             	  send_to_char("�� �������� ���� ��������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "��������")) {
				  if ((GET_WIS_ROLL(ch) + 5) <= GET_WIS(ch))
					{ send_to_char("�� ������ �� ������ ����������� ��������!\r\n", ch);
                      return (1);
					}
                  		  GET_WIS(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, WIS)++;
              send_to_char("�� �������� ���� �������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "��������")) {
				  if ((GET_DEX_ROLL(ch) + 5) <= GET_DEX(ch))
					{ send_to_char("�� ������ �� ������ ����������� ��������!\r\n", ch);
                      return (1);
					}
				  GET_DEX(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, DEX)++;
			  send_to_char("�� �������� ���� �������� �� �������.\r\n", ch);
			 }
			if(isname(argument,"�����")) {
				  if ((GET_CHA_ROLL(ch) + 5) <= GET_CHA(ch))
					{ send_to_char("�� ������ �� ������ ����������� �����!\r\n", ch);
                      return (1);
					}
                  		  GET_CHA(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, CHA)++;
              send_to_char("�� �������� ���� ����� �� �������.\r\n", ch);
			 }
			
		save_char(ch, NOWHERE);
	
   }       
    return (1);
  }  skip_spaces(&argument);  if (!*argument) {   *buf = '\0';
       for (i = 0; i <= MAX_SKILLS; i++)
	   {    if (!skill_info[i].name || *skill_info[i].name == '!')
                 continue;
		if (GET_PRACTICES(ch, i)) 	
		{   skill_num = 1;
		 if (GET_LEVEL(ch) >= skill_info[i].k_improove[(int) GET_CLASS(ch)])
	         sprintf(buf+strlen(buf), "                 &c%s&n\r\n", skill_info[i].name);
		}
	   }	   
	   if (skill_num)  
		{ send_to_char("�� ������ ��������� ��������� �������:\r\n", ch);
		  send_to_char(buf, ch);
		
		}
	   else
         send_to_char("�� �� ������ ������� ������.\r\n", ch);	   
	   return (1);  } 
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		{ send_to_char("�� � ��� ��� ������ ��� ��� ����!\r\n", ch);
			return (1);
		}
	
	if (!strcmp(argument, "���")) {
		i = 0;
	  	  for (skill_num = 0; skill_num <= MAX_SKILLS; skill_num++)
			  			  
	if (GET_LEVEL(ch) >= skill_info[skill_num].k_improove[(int) GET_CLASS(ch)] && skill_info[skill_num].name != "!UNUSED!") 
		{ if (!skill_info[skill_num].name || *skill_info[skill_num].name == '!')
                 continue;
		 if (GET_PRACTICES(ch, skill_num)) 
			{	i = 1; 
		   if (GET_SKILL(ch, skill_num) > LEARNED(ch))
			{ sprintf(buf, "� �� ���� ��� ����� � ������� '&W%s&n', ����� ������� ������������!\r\n", skill_info[skill_num].name);	 
				send_to_char(buf, ch);
					continue;
			}
			sprintf(buf, "�� ������� ��������� � �������: '&c%s&n'\r\n", 
					  skill_info[skill_num].name);
						send_to_char(buf, ch);
					percent = GET_SKILL(ch, skill_num);
				percent += 7;
			SET_SKILL(ch, skill_num, MIN(100, percent));
		GET_PRACTICES(ch, skill_num) = 0;

			}
		}
 if (!i)
   send_to_char("��� ��� ����� �� �������!\r\n", ch);
return(1);
	}

  /* ����� ����� ����������� ������������ ��� ��� ����������*/
	
	skill_num = find_skill_num(argument);
    if (skill_num < 1 ||      GET_LEVEL(ch) < skill_info[skill_num].k_improove[(int) GET_CLASS(ch)])
 {    sprintf(buf, "�� �� ������ ����� %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return (1); 
 }	
         if (GET_SKILL(ch, skill_num) > LEARNED(ch))
		{ sprintf(buf, "� �� ���� ��� ����� � ������� &W%s&n, ����� ������� ������������.\r\n", skill_info[skill_num].name);	 
			send_to_char(buf, ch);
				return (1);
		}
    if (GET_PRACTICES(ch, skill_num))
{    	percent = GET_SKILL(ch, skill_num);
		percent += 7;
		sprintf(buf, "�� ������� ��������� � �������: %s\r\n", skill_info[skill_num].name);
		SET_SKILL(ch, skill_num, MIN(100, percent));
        GET_PRACTICES(ch, skill_num) = 0;
	send_to_char(buf, ch);
  }
	else 
   send_to_char("�� ������ �� ������ �������.\r\n", ch);

	if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("�� ������� � ���� �������.\r\n", ch);
  return (1);
}

SPECIAL(guild1)
{
 struct char_file_u;
	int skill_num=0, percent, i;


	for (i = 0; guild_info[i][0] != -1; i++) {
		if (GET_ROOM_VNUM(IN_ROOM(ch)) == guild_info[i][1]) 
			break;
	}
    

	if (GET_CLASS(ch) != guild_info[i][0]) 
		  return(0);
	  
	  

  if (IS_NPC(ch) || !CMD_IS("�����"))  {
  if (IS_NPC(ch) || !CMD_IS("�����������"))
    return (0);
   else {
          skip_spaces(&argument);
             if (!*argument) {
                   send_to_char("��� �� ������ �����������?\r\n", ch);
                   return (1);
			 }
          	 if(!GET_CRBONUS(ch)) {
			send_to_char("� ��� ��� ������������� ������.\r\n", ch);	 
				 return (1);
			 }
     		if(isname(argument, "����")) {
				 if ((GET_STR_ROLL(ch) + 5) <= GET_STR(ch))
					{ send_to_char("�� ������ �� ������ ����������� ����!\r\n", ch);
					  return (1);
					}
				  GET_STR(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, STR)++;
		          send_to_char("�� �������� ���� ���� �� �������.\r\n", ch);
			 }
			if(isname(argument, "����")) {
				if ((GET_CON_ROLL(ch) + 5) <= GET_CON(ch))
					{ send_to_char("�� ������ �� ������ ����������� ����!\r\n", ch);
                      return (1);
					}
				  GET_MAX_HIT(ch)++;
				  GET_CON(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, CON)++;
                  send_to_char("�� �������� ���� ���� �� �������.\r\n", ch);
			 }
			if(isname(argument, "���������")) {
				  if ((GET_INT_ROLL(ch) + 5) <= GET_INT(ch))
					{ send_to_char("�� ������ �� ������ ����������� ���������!\r\n", ch);
                      return (1);
				  }
                  		  GET_INT(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, INT)++;
             	  send_to_char("�� �������� ���� ��������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "��������")) {
				  if ((GET_WIS_ROLL(ch) + 5) <= GET_WIS(ch))
					{ send_to_char("�� ������ �� ������ ����������� ��������!\r\n", ch);
                      return (1);
					}
                  		  GET_WIS(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, WIS)++;
              send_to_char("�� �������� ���� �������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "��������")) {
				  if ((GET_DEX_ROLL(ch) + 5) <= GET_DEX(ch))
					{ send_to_char("�� ������ �� ������ ����������� ��������!\r\n", ch);
                      return (1);
					}
				  GET_DEX(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, DEX)++;
			  send_to_char("�� �������� ���� �������� �� �������.\r\n", ch);
			 }
			if(isname(argument,"�����")) {
				  if ((GET_CHA_ROLL(ch) + 5) <= GET_CHA(ch))
					{ send_to_char("�� ������ �� ������ ����������� �����!\r\n", ch);
                      return (1);
					}
                  		  GET_CHA(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, CHA)++;
              send_to_char("�� �������� ���� ����� �� �������.\r\n", ch);
			 }

	save_char(ch, NOWHERE);	
   }       
    return (1);
  }
  skip_spaces(&argument);

 
  if (!*argument) 
  { *buf = '\0';
       for (i = 0; i <= MAX_SKILLS; i++)
	{    if (!skill_info[i].name || *skill_info[i].name == '!')
                 continue;
		  if (GET_PRACTICES(ch, i)) 	
		{   skill_num = 1;
		 if (GET_LEVEL(ch) >= skill_info[i].k_improove[(int) GET_CLASS(ch)]) //&&
	         sprintf(buf+strlen(buf), "                 &c%s&n\r\n", skill_info[i].name);
		}
	}	   
	   if (skill_num)  
		{ send_to_char("�� ������ ��������� ��������� �������:\r\n", ch);
		  send_to_char(buf, ch);
		
		}
	   else
         send_to_char("�� �� ������ ������� ������.\r\n", ch);
    return (1);
  }
   
 
 
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		{ send_to_char("�� � ��� ��� ������ ��� ��� ����!\r\n", ch);
			return (1);
		}
	
	if (!strcmp(argument, "���")) {
		i = 0;
	  	  for (skill_num = 0; skill_num <= MAX_SKILLS; skill_num++)
			  		  
	if (GET_LEVEL(ch) >= skill_info[skill_num].k_improove[(int) GET_CLASS(ch)] && skill_info[skill_num].name != "!UNUSED!") 
		{     if (!skill_info[skill_num].name || *skill_info[skill_num].name == '!')
                 continue;
		    if (GET_PRACTICES(ch, skill_num)) 
			{	i = 1; 
		   if (GET_SKILL(ch, skill_num) <= LEARNED(ch))
			{ sprintf(buf, "� �� ���� ��� ����� � ������� &W%s&n, �� �� ���������� ��� ����� ������������!.\r\n", skill_info[skill_num].name);	 
				send_to_char(buf, ch);
					continue;
			}
			if (GET_SKILL(ch, skill_num) > 100)
		{ sprintf(buf, "� �� ���� ��� ����� � ������� &W%s&n, ����� ������� ������������.\r\n", skill_info[skill_num].name);	 
			send_to_char(buf, ch);
				continue;
		}
		   sprintf(buf, "�� ������� ��������� � �������: &c%s&n\r\n", 
					  skill_info[skill_num].name);
						send_to_char(buf, ch);
					percent = GET_SKILL(ch, skill_num);
				percent += 3;
			SET_SKILL(ch, skill_num, MIN(100, percent));
		GET_PRACTICES(ch, skill_num) = 0;

			}
		}
 if (!i)
   send_to_char("��� ��� ����� �� �������!\r\n", ch);
return(1);
	}

  /* ����� ����� ����������� ������������ ��� ��� ����������*/

	
	skill_num = find_skill_num(argument);

    if (skill_num < 1 ||
      GET_LEVEL(ch) < skill_info[skill_num].k_improove[(int) GET_CLASS(ch)]) {
    sprintf(buf, "�� �� ������ ����� %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return (1);
  }
	if (GET_SKILL(ch, skill_num) <= LEARNED(ch))
		{ sprintf(buf, "� �� ���� ��� ����� � ������� &W%s&n, �� �� ���������� ������������ ��� �����!.\r\n", skill_info[skill_num].name);	 
			send_to_char(buf, ch);
				return (1);
		}
  if (GET_SKILL(ch, skill_num) > 100)
		{ sprintf(buf, "� �� ���� ��� ����� � ������� &W%s&n, ����� ������� ������������.\r\n", skill_info[skill_num].name);	 
			send_to_char(buf, ch);
				return (1);
		}
  if (GET_PRACTICES(ch, skill_num)){
    	percent = GET_SKILL(ch, skill_num);
		percent += 7;
		sprintf(buf, "�� ������� ��������� � �������: %s\r\n", skill_info[skill_num].name);
		SET_SKILL(ch, skill_num, MIN(100, percent));
        GET_PRACTICES(ch, skill_num) = 0;
	send_to_char(buf, ch);
  }
	else send_to_char("�� ������ �� ������ �������.\r\n", ch);

//	if (GET_SKILL(ch, skill_num) >= 100	);//LEARNED(ch))
 //   send_to_char("�� ������� � ���� �������.\r\n", ch);

  return (1);
}


SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$o �������� � ������ ����!", FALSE, 0, k, 0, TO_ROOM); /*vanishes in a puff of smoke!*/
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (0);

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$o �������� � ������ ����!", FALSE, 0, k, 0, TO_ROOM); /*vanishes in a puff of smoke*/
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    send_to_char("You are awarded for outstanding performance.\r\n", ch);/*You are awarded for outstanding performance*/
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM); /*has been awarded for being a good citizen*/

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      GET_GOLD(ch) += value;
  }
  return (1);
}



/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */

void do_npc_steal(struct char_data * ch, struct char_data * victim)
{ struct obj_data *obj, *best = NULL;
  int    gold;

  if (IS_NPC(victim) || IS_SHOPKEEPER(ch) || FIGHTING(victim))
     return;

  if (GET_LEVEL(victim) >= LVL_IMMORT)
     return;
     
  if (!CAN_SEE(ch,victim))
     return;
     
  if (AWAKE(victim) &&
      (number(0, MAX(0,GET_LEVEL(ch) - int_app[GET_REAL_INT(victim)].observation)) == 0))
     {act("�� ���������� ���� $r � ����� �������.", FALSE, ch, 0, victim, TO_VICT);
      act("$n �����$u ��������� $V.", TRUE, ch, 0, victim, TO_NOTVICT);
     }
  else
     {/* Steal some gold coins          */
      gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
      if (gold > 0)
         {GET_GOLD(ch)     += gold;
          GET_GOLD(victim) -= gold;
         }
      /* Steal something from equipment */
      if (calculate_skill(ch,SKILL_STEAL,100,victim) >=
          number(1,100) - (AWAKE(victim) ? 100 : 0))
         {for (obj = victim->carrying; obj; obj = obj->next_content)
              if (CAN_SEE_OBJ(ch,obj) &&
	          (!best || GET_OBJ_COST(obj) > GET_OBJ_COST(best))
		 )
                 best = obj;
          if (best)
             {obj_from_char(best);
              obj_to_char(best,ch);
             }
         }
     }
}

void npc_steal(struct char_data *ch)
{
  struct char_data *cons;

  if (GET_POS(ch) != POS_STANDING || IS_SHOPKEEPER(ch) || FIGHTING(ch))
     return;

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
      if (!IS_NPC(cons) && !IS_IMMORTAL(cons) && (number(0, GET_REAL_INT(ch)) > 10))
         {do_npc_steal(ch, cons);
          return;
         }
  return;
}


SPECIAL(snake)
{
  if (cmd)
    return (FALSE);

  if (GET_POS(ch) != POS_FIGHTING)
    return (FALSE);

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 42 - GET_LEVEL(ch)) == 0)) {
    act("$n ������$y $V!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n ������$y ���!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
    return (TRUE);
  }
  return (FALSE);
}


SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return (FALSE);

  if (GET_POS(ch) != POS_STANDING)
    return (FALSE);

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4))) {
      do_npc_steal(ch, cons);
      return (TRUE);
    }
  return (FALSE);
}


SPECIAL(magic_user)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return (FALSE);

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
    return (TRUE);

  if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
    cast_spell(ch, vict, NULL, SPELL_SLEEP);

  if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
    cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

  if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
  }
  if (number(0, 4))
    return (TRUE);

  switch (GET_LEVEL(ch)) {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL);
    break;
  }
  return (TRUE);

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard)
{
  int i;
  struct char_data *guard = (struct char_data *) me;
  const char *buf = "�������� ��������� ���, ���������� ��� ����.\r\n";
  const char *buf2 = "�������� ���������$y $d, ���������� $s ����.\r\n";

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
    return (FALSE);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (FALSE);

  for (i = 0; guild_info[i][0] != -1; i++) {
    if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
	GET_ROOM_VNUM(IN_ROOM(ch)) == guild_info[i][1] &&
	cmd == guild_info[i][2]) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return (TRUE);
    }
  }

  return (FALSE);


}


SPECIAL(puff)
{
  if (cmd)
    return (0);

  switch (number(0, 60)) {
  case 0:
    do_say(ch, "��� ���! ����� ����� �����!", 0, 0); 
    return (1);
  case 1:
    do_say(ch, "�� ���� ������ ��� ���� � ������ �����?", 0, 0); 
    return (1);
  case 2:
    do_say(ch, "� ����� ������� ������.", 0, 0); 
    return (1);
  case 3:
    do_say(ch, "� ����� ������ ������.", 0, 0);
    return (1);
  default:
    return (0);
  }
}



SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (IS_CORPSE(i)) {
      act("$n ������� ������� ����.", FALSE, ch, 0, 0, TO_ROOM); /*savagely devours a corpse*/
      for (temp = i->contains; temp; temp = next_obj) {
	next_obj = temp->next_content;
	obj_from_obj(temp);
	obj_to_room(temp, ch->in_room);
      }
      extract_obj(i);
      return (TRUE);
    }
  }
  return (FALSE);
}



SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    if (FIGHTING(ch))
		continue;
	act("$n ������� ����� �����.", FALSE, ch, 0, 0, TO_ROOM); /*picks up some trash*/
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }

  return (FALSE);
}

int has_curse(struct obj_data *obj)
{ int i;
	for (i = 0; weapon_affect[i].aff_bitvector >= 0; i++) {
		// ������ ��������� �� ������
		if (weapon_affect[i].aff_spell <= 0 || !IS_OBJ_AFF(obj, weapon_affect[i].aff_pos))
			continue;
		if (IS_SET(spell_info[weapon_affect[i].aff_spell].routines, NPC_AFFECT_PC | NPC_DAMAGE_PC))
			return (TRUE);
	}
	return (FALSE);
}


int calculate_weapon_class(struct char_data *ch, struct obj_data *weapon)
{ int damage=0, hits=0, i;

  if (!weapon || GET_OBJ_TYPE(weapon) != ITEM_WEAPON)
     return (0);

  hits   = calculate_skill(ch,GET_OBJ_SKILL(weapon),101,0);
  damage = (GET_OBJ_VAL(weapon,1) + 1) * (GET_OBJ_VAL(weapon,2)) / 2;
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      {if (weapon->affected[i].location == APPLY_DAMROLL)
          damage += weapon->affected[i].modifier;
       if (weapon->affected[i].location == APPLY_HITROLL)
          hits   += weapon->affected[i].modifier * 10;
      }

  if (has_curse(weapon))
     return (0);

  return (damage + (hits > 200 ? 10 : hits/20));
}


void npc_armor(struct char_data *ch)
{struct obj_data *obj, *next;
 int    where=0;

 if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
    return;

 for (obj = ch->carrying; obj; obj = next)
     {next = obj->next_content;
      if (GET_OBJ_TYPE(obj) != ITEM_ARMOR)
          continue;
      if (CAN_WEAR(obj, ITEM_WEAR_FINGER))      where = WEAR_FINGER_R;
      if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK_1;
      if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
      if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
      if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
      if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
      if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
      if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
      if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))      where = WEAR_SHIELD;
      if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
      if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
      if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;
	  if (CAN_WEAR(obj, ITEM_WEAR_EAR))			where = WEAR_EAR_R;
	  if (CAN_WEAR(obj, ITEM_WEAR_EYES))		where = WEAR_EYES;
	  if (CAN_WEAR(obj, ITEM_WEAR_BACKPACK))	where = WEAR_BACKPACK; 

      if (!where)
         continue;

      if ((where == WEAR_FINGER_R) ||
          (where == WEAR_NECK_1)   ||
          (where == WEAR_WRIST_R)  ||
		  (where == WEAR_EAR_R))
         if (GET_EQ(ch, where))
            where++;
      if (where == WEAR_SHIELD &&
          (GET_EQ(ch,WEAR_BOTHS) || GET_EQ(ch,WEAR_HOLD)))
         continue;
      if (GET_EQ(ch, where))
         {if (GET_REAL_INT(ch) < 15)
             continue;
          if (GET_OBJ_VAL(obj,0) + GET_OBJ_VAL(obj,1) * 3 <=
              GET_OBJ_VAL(GET_EQ(ch,where),0) + GET_OBJ_VAL(GET_EQ(ch,where),1) * 3 ||
              has_curse(obj))
             continue;
          act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,where),0,TO_ROOM);
          obj_to_char(unequip_char(ch,where),ch);
         }
      act("$n ����$y $3.",FALSE,ch,obj,0,TO_ROOM);
      obj_from_char(obj);
      equip_char(ch,obj,where);
      break;
     }
}

int npc_battle_scavenge(struct char_data *ch)
{bool    max = FALSE;
 struct obj_data *obj, *next_obj=NULL;

 if (IS_SHOPKEEPER(ch))
    return (FALSE);

 if ( world[IN_ROOM(ch)].contents && number(0, GET_REAL_INT(ch)) > 10 )
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj)
        {next_obj = obj->next_content;
         if (CAN_GET_OBJ(ch, obj) &&
             !has_curse(obj) && //- ���������� ������, ���� ����� ����������
             (GET_OBJ_TYPE(obj) == ITEM_ARMOR ||
              GET_OBJ_TYPE(obj) == ITEM_WEAPON))
            {obj_from_room(obj);
             obj_to_char(obj, ch);
             act("$n ������$y $3.", FALSE, ch, obj, 0, TO_ROOM);
             max = TRUE;
            }
        }
 return (max);
}

int npc_walk(struct char_data *ch)
{ int  rnum, door=BFS_ERROR;


  if (IN_ROOM (ch) == NOWHERE)
      return (BFS_ERROR);


  if (GET_DEST(ch) == NOWHERE || (rnum = real_room(GET_DEST(ch))) == NOWHERE)
     return (BFS_ERROR);

   if (world[IN_ROOM (ch)].zone != world[rnum].zone)
    return (BFS_NO_PATH);

  if (IN_ROOM(ch) == rnum)
     {if (ch->mob_specials.dest_count == 1)
         return (BFS_ALREADY_THERE);
      if (ch->mob_specials.dest_pos == ch->mob_specials.dest_count - 1 &&
          ch->mob_specials.dest_dir >= 0)
         ch->mob_specials.dest_dir = -1;
      if (!ch->mob_specials.dest_pos && ch->mob_specials.dest_dir < 0)
         ch->mob_specials.dest_dir = 0;
      ch->mob_specials.dest_pos += ch->mob_specials.dest_dir >= 0 ? 1 : -1;
      if ((rnum = real_room(GET_DEST(ch))) == NOWHERE || rnum == IN_ROOM(ch))
         return (BFS_ERROR);
      else
         return (npc_walk(ch));
     }

  door = find_first_step(ch->in_room,real_room(GET_DEST(ch)),ch);
  // log("MOB %s walk to room %d at dir %d", GET_NAME(ch), GET_DEST(ch), door);

  return (door);
}

void best_weapon(struct char_data *ch, struct obj_data *sweapon, struct obj_data **dweapon)
{ if (*dweapon == NULL)
     {if (calculate_weapon_class(ch,sweapon) > 0)
         *dweapon = sweapon;
     }
  else
  if (calculate_weapon_class(ch,sweapon) > calculate_weapon_class(ch,*dweapon))
     *dweapon = sweapon;
}


void npc_wield(struct char_data *ch)
{struct obj_data *obj, *next, *right=NULL, *left=NULL, *both=NULL;

 if (GET_REAL_INT(ch) < 12 || IS_SHOPKEEPER(ch))
    return;

 if (GET_EQ(ch,WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch,WEAR_HOLD)) == ITEM_WEAPON)
    left  = GET_EQ(ch,WEAR_HOLD);
 if (GET_EQ(ch,WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch,WEAR_WIELD)) == ITEM_WEAPON)
    right = GET_EQ(ch,WEAR_WIELD);
 if (GET_EQ(ch,WEAR_BOTHS) && GET_OBJ_TYPE(GET_EQ(ch,WEAR_BOTHS)) == ITEM_WEAPON)
    both = GET_EQ(ch,WEAR_BOTHS);

 if (GET_REAL_INT(ch) < 15 &&
     ((left && right) || (both)))
    return;

 for (obj = ch->carrying; obj; obj = next)
     {next = obj->next_content;
      if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
         continue;
      if (CAN_WEAR(obj, ITEM_WEAR_HOLD) && OK_HELD(ch,obj))
         best_weapon(ch,obj,&left);
      else
      if (CAN_WEAR(obj, ITEM_WEAR_WIELD) && OK_WIELD(ch,obj))
         best_weapon(ch,obj,&right);
      else
      if (CAN_WEAR(obj, ITEM_WEAR_BOTHS) && OK_BOTH(ch,obj))
         best_weapon(ch,obj,&both);
     }

  if (both && calculate_weapon_class(ch,both) > calculate_weapon_class(ch,left) +
                                               calculate_weapon_class(ch,right))
    {if (both == GET_EQ(ch,WEAR_BOTHS))
        return;
     if (GET_EQ(ch,WEAR_BOTHS))
        {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
        }
     if (GET_EQ(ch,WEAR_WIELD))
        {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_WIELD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_WIELD), ch);
        }
     if (GET_EQ(ch,WEAR_SHIELD))
        {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_SHIELD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_SHIELD),ch);
        }
     if (GET_EQ(ch,WEAR_HOLD))
        {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_HOLD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_HOLD),ch);
        }
     act("$n ����$y $3 � ��� ����.",FALSE,ch,both,0,TO_ROOM);
     obj_from_char(both);
     equip_char(ch,both,WEAR_BOTHS);
    }
 else
    {if (left && GET_EQ(ch,WEAR_HOLD) != left)
        {if (GET_EQ(ch,WEAR_BOTHS))
            {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
            }
         if (GET_EQ(ch,WEAR_SHIELD))
            {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_SHIELD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_SHIELD),ch);
            }
         if (GET_EQ(ch,WEAR_HOLD))
            {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_HOLD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_HOLD),ch);
            }
         act("$n ����$y $3 � ����� ����.",FALSE,ch,left,0,TO_ROOM);
         obj_from_char(left);
         equip_char(ch,left,WEAR_HOLD);
        }
     if (right && GET_EQ(ch,WEAR_WIELD) != right)
        {if (GET_EQ(ch,WEAR_BOTHS))
            {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
            }
         if (GET_EQ(ch,WEAR_WIELD))
            {act("$n ���������$y ������������ $3.",FALSE,ch,GET_EQ(ch,WEAR_WIELD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_WIELD), ch);
            }
         act("$n ����$y $3 � ������ ����.",FALSE,ch,right,0,TO_ROOM);
         obj_from_char(right);
         equip_char(ch,right,WEAR_WIELD);
        }
    }
}

void npc_light(struct char_data *ch)
{struct obj_data *obj, *next;

 if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
    return;

 if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    return;

 if ((obj=GET_EQ(ch,WEAR_LIGHT)) &&
     (GET_OBJ_VAL(obj,0) == 0 || !IS_DARK(IN_ROOM(ch))))//���� 2, ������� �� 0 - ������� ����� � ����� ���������
    {act("$n ���������$y ������������ $3.",FALSE,ch,obj,0,TO_ROOM);
     obj_to_char(unequip_char(ch,WEAR_LIGHT),ch);
    }

 if (!GET_EQ(ch,WEAR_LIGHT) && IS_DARK(IN_ROOM(ch)))
    for (obj = ch->carrying; obj; obj = next)
        {next = obj->next_content;
         if (GET_OBJ_TYPE(obj) != ITEM_LIGHT)
             continue;
         if (GET_OBJ_VAL(obj,0) == 0)// ��� �� �������
            {act("$n ��������$y $3.",FALSE,ch,obj,0,TO_ROOM);
             obj_from_char(obj);
             obj_to_room(obj,IN_ROOM(ch));
             continue;
            }
         act("$n �����$y ������������ $3.",FALSE,ch,obj,0,TO_ROOM);
         obj_from_char(obj);
         equip_char(ch,obj,WEAR_LIGHT);
         return;
        }
}


SPECIAL(cityguard)
{
  struct char_data *tch, *evil;
  int max_evil;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  max_evil = 1000;
  evil = 0;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, (PLR_KILLER | PLR_THIEF) )) {
      act("$n ������$y...'�� $n!!! �� �������$y �����!! �� ������ ����$y!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED,1);
      return (TRUE);
    }
  }

  
/*  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
	  (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
	max_evil = GET_ALIGNMENT(tch);
	evil = tch;
      }
    }
  }*/

/*  if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0)) {
    act("$n ������$y '�� ������ �����!  ������!  ������!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED); 
    return (TRUE);
  }*/
  return (FALSE);
}

#define HORSE_PRICE(pet) (GET_LEVEL(pet) * 30) 
#define HORSE_ROOM		  8098 // �������, ��� ������ ������ �������

SPECIAL(horse_shops)
{ int i= 1;
  char buf[MAX_STRING_LENGTH], pet_name[256];
  room_rnum pet_room;
  struct char_data *victim = (struct char_data *) me, *pet = NULL;

  if (IS_NPC(ch))
     return (0);

  pet_room = real_room(HORSE_ROOM);

  if (CMD_IS("������") || CMD_IS("list")) {
    act("&C\"� ���� � ������� ���� ��������� �������\", ������$Y ��� $N&n", FALSE, ch, 0, victim, TO_CHAR);
	send_to_char("+----------------------------------------------------------------+\r\n", ch);
	send_to_char("| ����     ���                                                   |\r\n", ch);
	send_to_char("+----------------------------------------------------------------+\r\n", ch);
	for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
    sprintf(buf, "%-4d     %s\r\n", HORSE_PRICE(pet), GET_NAME(pet));
      send_to_char(buf, ch);
        
	}
    return (TRUE);
  } else 
	  if (CMD_IS("������")|| CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      act("&Y\"� ���� ��� ������ �������\"&n, ������$Y ��� $N", FALSE, ch, 0, victim, TO_CHAR);
      return (TRUE);
    }
   
    if (has_horse(ch,FALSE))
		{act("$N ��������$Y : \"$n, ����� ���� ������ ������?.\"",
            FALSE,ch,0,victim,TO_CHAR);
       return (TRUE);
		}
	
	if (GET_GOLD(ch) < HORSE_PRICE(pet)) {
     act("&Y\"� ��� ��� ������� �����\"&n, ������$Y ��� $N",FALSE,ch,0,victim,TO_CHAR);
	    return (TRUE);
    }
    
    if(!(pet = read_mobile(GET_MOB_RNUM(pet), REAL)))
		{	act("\"� ��������� ����� � ���� ��� ������� ��� ���.\"- ������� �����, ��������$Q $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      make_horse(pet,ch);
      char_to_room(pet,IN_ROOM(ch));
      sprintf(buf,"$N �������$Y %s � �������$Y %s ���.",GET_VNAME(pet),HSHR(pet));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N �������$Y %s � �������$Y %s $d.",GET_VNAME(pet),HSHR(pet));
      act(buf,FALSE,ch,0,victim,TO_ROOM);
      GET_GOLD(ch) -= HORSE_PRICE(pet);
      SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
      return (TRUE);
     }
	else 
	  if (CMD_IS("�������")|| CMD_IS("sell")) 
	  {if (!has_horse(ch,TRUE) && !*argument)
         {act("$N ������$U : \"$n, ��� �� ����� � ������� ��������, ���������!\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (on_horse(ch))
         {act("\"�� ��� ������ ���� ������� ������ � ��������?.\"- ������ �������$U $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (!(pet = get_horse(ch)))
         {act("\"��� ����� ������� � ����� �� �����!.\"- ������$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (IN_ROOM(pet) != IN_ROOM(victim))
         {act("\"�� ������� ������� �������, ������ ��� ��� ���������.\"- ������$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      extract_char(pet,FALSE);
      sprintf(buf,"$N ����$Y ����� � %s � �����$Y %s � �������.",GET_VNAME(pet) ,HSHR(pet));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N ����$Y ����� � %s � �����$Y %s � �������.",GET_VNAME(pet), HSHR(pet));
      act(buf,FALSE,ch,0,victim,TO_ROOM);
      GET_GOLD(ch) += (HORSE_PRICE(pet) >> 1);
      SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
      return (TRUE);
     }
 return (0);
}
   


SPECIAL(horse_keeper)
{ struct char_data *victim = (struct char_data *) me, *horse = NULL;

  if (IS_NPC(ch))
     return (0);

  if (!CMD_IS("������") && !CMD_IS("horse"))
     return (0);


  skip_spaces(&argument);

  if (!*argument)
     {if (has_horse(ch,FALSE))
         {act("$N �������������$U : \"$n,� ����� ��� ������ ������?\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      sprintf(buf,"$N ������$Y : \"������ ��� ��������� %d %s.\"",
              HORSE_COST, desc_count(HORSE_COST,WHAT_MONEYa));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      return (TRUE);
     }

  if (!strn_cmp(argument, "������", strlen(argument)) ||
      !strn_cmp(argument, "buy", strlen(argument)))
     {if (has_horse(ch,FALSE))
         {act("$N �������$U : \"$n, �� ����� �� ���� �������� �������� ������?\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (GET_GOLD(ch) < HORSE_COST)
         {act("\"� ��� ��� ����� �����!\"- ��������$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (!(horse = read_mobile(HORSE_VNUM,VIRTUAL)))
         {act("\"��������, �� � ������ ������ � ���� ��� ��������.\"- ������$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      make_horse(horse,ch);
      char_to_room(horse,IN_ROOM(ch));
      sprintf(buf,"$N �������$Y %s � �������$Y %s ���.",GET_VNAME(horse),HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N �������$Y %s � �������$Y %s $d.",GET_VNAME(horse),HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_ROOM);
      GET_GOLD(ch) -= HORSE_COST;
      SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
      return (TRUE);
     }


  if (!strn_cmp(argument, "�������", strlen(argument)) ||
      !strn_cmp(argument, "sell", strlen(argument)))
     {if (!has_horse(ch,TRUE))
         {act("$N ������$U : \"$n, ��� �� ����� ��� ��������, ���������!\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (on_horse(ch))
         {act("\"�� ��� ������ ���� ����� ������ � ��������?.\"- ������ �������$U $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (!(horse = get_horse(ch)) || GET_MOB_VNUM(horse) != HORSE_VNUM )
         {act("\"��� ����� ������� � ����� �� �����!.\"- ������$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (IN_ROOM(horse) != IN_ROOM(victim))
         {act("\"�� ������� ������� �������, ������ ��� ��� �����.\"- ������$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      extract_char(horse,FALSE);
      sprintf(buf,"$N ����$Y ����� � %s � �����$Y %s � �������.",GET_VNAME(horse) ,HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N ����$Y ����� � %s � �����$Y %s � �������.",GET_VNAME(horse), HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_ROOM);
      GET_GOLD(ch) += (HORSE_COST >> 1);
      SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
      return (TRUE);
     }

   return (0);
}



int npc_track(struct char_data *ch)
{ struct char_data  *vict;
  memory_rec        *names;
  int               door=BFS_ERROR, msg = FALSE, i;
  struct track_data *track;

  if (GET_REAL_INT(ch) < number(15,20))
     {for (vict = character_list; vict && door == BFS_ERROR; vict = vict->next)
          {if (CAN_SEE(ch,vict) && IN_ROOM(vict) != NOWHERE)
              for (names = MEMORY(ch); names && door == BFS_ERROR; names = names->next)
                  if (GET_IDNUM(vict) == names->id &&
                      (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
                       world[IN_ROOM(ch)].zone == world[IN_ROOM(vict)].zone
                      )
                     )
                    {for (track = world[IN_ROOM(ch)].track; track && door == BFS_ERROR; track=track->next)
                         if (track->who == GET_IDNUM(vict))
                            for  (i = 0; i < NUM_OF_DIRS; i++)
                                 if (IS_SET(track->time_outgone[i],7))
                                    {door = i;
                                     // log("MOB %s track %s at dir %d", GET_NAME(ch), GET_NAME(vict), door);
                                     break;
                                    }
                     if (!msg)
                        {msg = TRUE;
                         act("$n �����$y ����������� ���������� ���-�� �����.",FALSE,ch,0,0,TO_ROOM);
                        }
                    }
          }
     }
  else
     {for (vict = character_list; vict && door == BFS_ERROR; vict = vict->next)
          if (CAN_SEE(ch,vict) && IN_ROOM(vict) != NOWHERE)
             {for (names = MEMORY(ch); names && door == BFS_ERROR; names = names->next)
                  if (GET_IDNUM(vict) == names->id &&
                      (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
                       world[IN_ROOM(ch)].zone == world[IN_ROOM(vict)].zone
                      )
                     )
                    {if (!msg)
                        {msg = TRUE;
                         act("$n �����$y ����������� ������ ���-�� �����.",FALSE,ch,0,0,TO_ROOM);
                        }
                     door = go_track(ch, vict, SKILL_TRACK);
                     // log("MOB %s sense %s at dir %d", GET_NAME(ch), GET_NAME(vict), door);
                    }
             }
     }
  return (door);
}

int item_nouse(struct obj_data *obj)
{switch (GET_OBJ_TYPE(obj))
 {
 case ITEM_LIGHT  : if (GET_OBJ_VAL(obj,0) == 0) // ������� � ������ �� 0 - ��� �
                       return (TRUE);			// � ��������� ���������� ������� �����
                    break;
 case ITEM_SCROLL :
 case ITEM_POTION :
                    if (!GET_OBJ_VAL(obj,1) &&
                        !GET_OBJ_VAL(obj,2) &&
                        !GET_OBJ_VAL(obj,3))
                       return (TRUE);
                    break;
 case ITEM_STAFF:
 case ITEM_WAND:    if (!GET_OBJ_VAL(obj,2))
                       return (TRUE);
                    break;
 case ITEM_OTHER:
 case ITEM_TRASH:
 case ITEM_TRAP:
 case ITEM_CONTAINER:
 case ITEM_NOTE:
 case ITEM_DRINKCON:
 case ITEM_FOOD:
 case ITEM_PEN:
 case ITEM_BOAT:
 case ITEM_FOUNTAIN:
                    return (TRUE);
                    break;
 }
 return (FALSE);
}

void npc_dropunuse(struct char_data *ch)
{struct obj_data *obj, *nobj;
 for (obj = ch->carrying; obj; obj = nobj)
     {nobj = obj->next_content;
      if (item_nouse(obj))
         {act("$n ��������$y $3.",FALSE,ch,obj,0,FALSE);
          obj_from_char(obj);
          obj_to_room(obj,IN_ROOM(ch));
         }
     }
}



int npc_scavenge(struct char_data *ch)
{int    max = 1;
 struct obj_data *obj, *best_obj, *cont, *best_cont, *cobj;

 if (IS_SHOPKEEPER(ch))
    return (FALSE);
 npc_dropunuse(ch);
 if ( world[ch->in_room].contents && number(0, GET_REAL_INT(ch)) > 10 )
    {max      = 1;
     best_obj = NULL;
     cont     = NULL;
     best_cont= NULL;
     for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
         if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
            {if (IS_CORPSE(obj) || OBJVAL_FLAGGED(obj,CONT_LOCKED))
                continue;
             for (cobj = obj->contains; cobj; cobj = cobj->next_content)
                 if (CAN_GET_OBJ(ch, cobj) &&
                     !item_nouse(cobj)     &&
                     GET_OBJ_COST(cobj) > max)
                    {cont      = obj;
                     best_cont = best_obj = cobj;
                     max       = GET_OBJ_COST(cobj);
                    }
            }
         else
         if (!IS_CORPSE(obj)         &&
             CAN_GET_OBJ(ch, obj)    &&
             GET_OBJ_COST(obj) > max &&
             !item_nouse(obj))
            {best_obj = obj;
             max = GET_OBJ_COST(obj);
            }
     if (best_obj != NULL)
        {if (best_obj != best_cont)
            {obj_from_room(best_obj);
             obj_to_char(best_obj, ch);
             act("$n ������$y $3.", FALSE, ch, best_obj, 0, TO_ROOM);
            }
         else
            {obj_from_obj(best_obj);
             obj_to_char(best_obj, ch);
             sprintf(buf,"$n ������$y $3 �� %s.", cont->short_rdescription);
             act(buf,FALSE,ch,best_obj,0,TO_ROOM);
            }
        }
    }
 return (max > 1);
}

int npc_loot(struct char_data *ch)
{int    max = FALSE;
 struct obj_data *obj, *loot_obj, *next_loot, *cobj, *cnext_obj;

 if (IS_SHOPKEEPER(ch))
    return (FALSE);
 npc_dropunuse(ch);
 if (world[ch->in_room].contents && number(0, GET_REAL_INT(ch)) > 10)
    {for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
         if (CAN_SEE_OBJ(ch, obj) && IS_CORPSE(obj))
            for(loot_obj = obj->contains; loot_obj; loot_obj = next_loot)
               {next_loot = loot_obj->next_content;
                if (GET_OBJ_TYPE(loot_obj) == ITEM_CONTAINER)
                   {if (IS_CORPSE(loot_obj) || OBJVAL_FLAGGED(loot_obj,CONT_LOCKED))
                       continue;
                    for (cobj = loot_obj->contains; cobj; cobj = cnext_obj)
                        {cnext_obj = cobj->next_content;
                         if (CAN_GET_OBJ(ch, cobj) &&
                             !item_nouse(cobj))
                            {sprintf(buf,"$n �������$y $3 �� %s.",obj->short_rdescription);
                             act(buf,FALSE,ch,cobj,0,TO_ROOM);
                             obj_from_obj(cobj);
                             obj_to_char(cobj,ch);
                             max++;
                            }
                        }
                   }
                else
                if (CAN_GET_OBJ(ch, loot_obj) && !item_nouse(loot_obj))
		{ sprintf(buf,"$n �������$y $3 �� %s.",obj->short_rdescription);
                    act(buf,FALSE,ch,loot_obj,0,TO_ROOM);
                   	if (GET_OBJ_TYPE(loot_obj) == ITEM_MONEY)
			   { GET_GOLD(ch) += GET_OBJ_VAL(loot_obj, 0);
			     extract_obj(loot_obj);
			   }
			else
			   { obj_from_obj(loot_obj);
			     obj_to_char(loot_obj, ch);
			   }
                    max++;
                   }
               }
    }
 return (max);
}

int npc_move(struct char_data *ch, int dir, int need_specials_check)
{ int       need_close=FALSE, need_lock=FALSE;
  int       rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
  struct    room_direction_data *rdata = NULL;
  int       retval = FALSE;

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
     return (FALSE);
  else
  if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE)
     return (FALSE);
  else
  if (ch->master && IN_ROOM(ch) == IN_ROOM(ch->master))
      return (FALSE);
  else
  if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
     {if (!EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR))
         return (FALSE);
      rdata     = EXIT(ch,dir);
      if (EXIT_FLAGGED(rdata, EX_LOCKED))
         { if (has_key(ch,rdata->key) ||
              (!EXIT_FLAGGED(rdata, EX_PICKPROOF) &&
               calculate_skill(ch,SKILL_PICK,100,0) >= number(0,100)))
             {do_doorcmd(ch,0,dir,SCMD_UNLOCK);
                need_lock = TRUE;
             }
          else
              return (FALSE);
         }
      if (EXIT_FLAGGED (rdata, EX_CLOSED))
        if (GET_REAL_INT (ch) >= 15  ||
  	    GET_DEST (ch) != NOWHERE ||
	    MOB_FLAGGED (ch, MOB_OPENDOOR))
	 { do_doorcmd (ch, 0, dir, SCMD_OPEN);
	   need_close = TRUE;
	 }
     }

  retval = perform_move(ch, dir, 1, FALSE);
  
  if (need_close)
     {if (retval)
         do_doorcmd(ch,0,rev_dir[dir],SCMD_CLOSE);
      else
         do_doorcmd(ch,0,dir,SCMD_CLOSE);
     }

  if (need_lock)
     {if (retval)
         do_doorcmd(ch,0,rev_dir[dir],SCMD_LOCK);
      else
         do_doorcmd(ch,0,dir,SCMD_LOCK);
     }

  return (retval);
}

#define ZONE(ch)  (GET_MOB_VNUM(ch) / 100)
#define GROUP(ch) ((GET_MOB_VNUM(ch) % 100) / 10)

void npc_group(struct char_data *ch)
{struct char_data *vict, *leader=NULL;
 int    zone = ZONE(ch), group = GROUP(ch), members = 0;

 if (GET_DEST(ch) == NOWHERE || IN_ROOM(ch) == NOWHERE)
    return;

 if (ch->master &&
     IN_ROOM(ch) == IN_ROOM(ch->master)
    )
    leader = ch->master;

 if (!ch->master)
    leader = ch;

 if (leader &&
     (AFF_FLAGGED(leader, AFF_CHARM) ||
      GET_POS(leader) < POS_SLEEPING
     )
    )
    leader = NULL;

 // Find leader
 for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict)                  ||
          GET_DEST(vict) != GET_DEST(ch) ||
          zone != ZONE(vict)             ||
	  group != GROUP(vict)           ||
          AFF_FLAGGED(vict, AFF_CHARM)   ||
	  GET_POS(vict) < POS_SLEEPING
	 )
        continue;
      members++;
      if (!leader || GET_REAL_INT(vict) > GET_REAL_INT(leader))
         leader = vict;
     }

 if (members <= 1)
    {if (ch->master)
        stop_follower(ch, SF_EMPTY);
       return;
    }

if (leader->master)
   stop_follower(leader, SF_EMPTY);
   

 // Assign leader
 for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
     {if (!IS_NPC(vict)                  ||
          GET_DEST(vict) != GET_DEST(ch) ||
          zone != ZONE(vict)             ||
	  group != GROUP(vict)           ||
          AFF_FLAGGED(vict, AFF_CHARM)   ||
	  GET_POS(vict) < POS_SLEEPING
	 )
         continue;
      if (vict == leader)
         { SET_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);
          continue;
         }
      if (!vict->master)
         add_follower(vict,leader);
      else
      if (vict->master != leader)
         {stop_follower(vict,SF_EMPTY);
          add_follower(vict,leader);
         }
       	   SET_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);
     }

}

void npc_groupbattle(struct char_data *ch)
{struct follow_type *k;
 struct char_data   *tch, *helper;

 if (!IS_NPC(ch) ||
     !FIGHTING(ch) ||
     AFF_FLAGGED(ch,AFF_CHARM) ||
     !ch->master ||
     IN_ROOM(ch) == NOWHERE ||
     !ch->followers)
    return;

 k   = ch->master ? ch->master->followers : ch->followers;
 tch = ch->master ? ch->master : ch;
 for (;k; (k = tch ? k : k->next), tch = NULL)
     {helper = tch ? tch : k->follower;
      if (IN_ROOM(ch) == IN_ROOM(helper) &&
          !FIGHTING(helper) &&
          !IS_NPC(helper) &&
          GET_POS(helper) > POS_STUNNED)
         {GET_POS(helper) = POS_STANDING;
          set_fighting(helper, FIGHTING(ch));
          act("$n �������$u �� $V.",FALSE,helper,0,ch,TO_ROOM);
         }
     }
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  room_rnum pet_room;
  struct char_data *pet;

  pet_room = ch->in_room + 1;

  if (CMD_IS("������") || CMD_IS("list")) {
    send_to_char("� �������:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("������")|| CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("������ ��������� ��������� ���!\r\n", ch); 
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char("�� �� ������ ������� ������� �����!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= PET_PRICE(pet);

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT(pet->char_specials.saved.affected_by.flags[0], AFF_CHARM);

    if (*pet_name) {
      sprintf(buf, "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = str_dup(buf);

      sprintf(buf, "%sA small sign on a chain around the neck says '���� ����� %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("�� ������ ������������ ����� ����� ������.\r\n", ch);
    act("$n �����$y $V, ��� �������� ��������.", FALSE, ch, 0, pet, TO_ROOM);

    return (1);
  }
  /* All commands except list and buy */
  return (0);
}

// �����������

SPECIAL(questmaster)
{
  struct char_data *questinfo = NULL;
  struct char_data *questman = (struct char_data*)me;
  int count = 0;
  //GET_QUESTGIVER(ch) = questman->player.short_descr;

	if (CMD_IS("��������"))
	{  if (IS_QUESTOR(ch))
		{ sprintf(buf, "������ ����� ��������: � ��� ��� ���� �������, � ������� �� �� ��������.");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
				return(1);
		}
		if (GET_NEXTQUEST(ch) > 0)
		{  sprintf(buf, "������ ����� ��������: �� ������� �������������, %s, ���������!",GET_NAME(ch));
                act(buf, FALSE, questman, 0, 0, TO_ROOM);
		   sprintf(buf, "������ ����� ��������: ������� �������.");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
			    return(1);
		}
           if (GET_LEVEL(ch) <= 5)
		{ send_to_char("������ ����� ��������: �������� ��� ��� �� ������ ���� �������!\r\n", ch);
	      return (1);
		}
	       generate_quest(ch, questman);
        if (GET_QUESTMOB(ch) > 0 || GET_QUESTOBJ(ch) > 0)
			{ GET_COUNTDOWN(ch) = number(20,40);
			  SET_BIT(PLR_FLAGS(ch, PLR_QUESTOR), PLR_QUESTOR);
		          sprintf(buf, " &g� ��� ���� &W%d %s&g ��� ���������� ����� �������.&n",
			  GET_COUNTDOWN(ch), desc_count(GET_COUNTDOWN(ch),WHAT_MINa));
                          act(buf, FALSE, questman, 0, ch, TO_VICT);
			  sprintf(buf, "&g������� �������, %s, ����� ���� �����!",GET_NAME(ch));
                          act(buf, FALSE, questman, 0, ch, TO_VICT);
			  sprintf(buf, "&g����� ��� � ������!&n");
                          act(buf, FALSE, questman, 0, ch, TO_VICT);
			
			}
return(1);
	}
			if (CMD_IS("��������"))
			{  	if (IS_QUESTOR(ch))
				{ if (GET_QUESTMOB(ch))
				   { questinfo = get_mob_by_id(GET_QUESTMOB(ch));
				      if (questinfo)
					GET_QUESTOR(questinfo) = 0;
				   }
				  else
				   { if (GET_QUESTOK(ch))
					act("������ ����� ��������: �� �� ������ �������� �������, ������� ��� ���������!\r\n ���������� ����� ��������!", FALSE, questman, 0, ch, TO_VICT);
					else 
					act("&K��� ���������, ��� ������� ���-�� ��� ��������.&n", FALSE, questman, 0, ch, TO_VICT);
					return(1);
				   }
				sprintf(buf, "������ ����� ��������: ��, ����� �����, � � �� ���� ��������!&n\r\n ������, � ������� ���� �������.");
                		act(buf, FALSE, questman, 0, ch, TO_VICT);
			        REMOVE_BIT(PLR_FLAGS(ch, PLR_QUESTOR), PLR_QUESTOR);
		      		 // GET_QUESTGIVER(ch) = NULL;
                		GET_COUNTDOWN(ch) = 0;
                		GET_QUESTMOB(ch) = 0;
                		GET_NEXTQUEST(ch) = number(3, 6);//�������� 9.11.02�.
                		sprintf(buf, "&G�� ������ �������� ������ ������� ����� &W%d �����&G.&n", GET_NEXTQUEST(ch));
                		act(buf, FALSE, questman, 0, ch, TO_VICT);
				return(1);
				}
				
				sprintf(buf, "� � ��� � �� ���� �������!");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
				return(1);
			}

			if (CMD_IS("��������"))
			{ if (GET_COUNTDOWN(ch) <=0)
				{act("������ ����� ��������: � ��� �� ������ ��������?", FALSE, questman, 0, ch, TO_VICT);
					return(1);
				}
			 if (!GET_QUESTOK(ch) && !GET_QUESTMOB(ch))	
				{act("������ ����� ��������: �� ������ ����� ������� ���� �����������!", FALSE, questman, 0, ch, TO_VICT);
				return(1);	
				}
			 if (!GET_QUESTOK(ch) && GET_QUESTMOB(ch))
				{act("������ ����� ��������: �� ��� ������� �� ���������, ��� ��� �� ������ �������!", FALSE, questman, 0, ch, TO_VICT);
				return(1);	
				}
			 if (GET_QUESTOK(ch) && !GET_QUESTMOB(ch))	
				{ act("������ ����� ��������: �� ��� ��, � ������������ ���!", FALSE, questman, 0, ch, TO_VICT);
				GET_GLORY(ch) += GET_QUESTMAX(ch);
			   // GET_GLORY(ch) += count;
				sprintf (buf, "&G�� ���������� ����� ������� �� �������� %d %s �����!&n",
                		GET_QUESTMAX(ch), desc_count(GET_QUESTMAX(ch), WHAT_POINT)+10);
                		act(buf, FALSE, questman, 0, ch, TO_VICT);
				GET_QUESTOK(ch) = 0;
				GET_COUNTDOWN(ch) = 0;
				GET_NEXTQUEST(ch) = number(2, 6);
				REMOVE_BIT(PLR_FLAGS(ch, PLR_QUESTOR), PLR_QUESTOR);
				return(1);	
				}
			}
return(0);
}



void generate_quest(struct char_data *ch, struct char_data *questman)
{
    struct char_data *victim = NULL, *rndm = NULL;
    int level_diff, i, count =0;
   

   	for(i = 0; i < top_of_world; i++)
	{ if (!(rndm = world[i].people))
	      continue;
	   if (!IS_NPC(rndm) || AFF_FLAGGED(rndm, AFF_CHARM))
			continue;
	level_diff = GET_LEVEL(rndm) - (GET_LEVEL(ch) + GET_QUESTPOINTS(ch)/5000 + GET_REMORT(ch));
           if (level_diff <  2	&& level_diff >=-3 && !GET_QUESTOR(rndm))
	       if ((NPC_FLAGGED(rndm, NPC_QUEST_GOOD) &&
			 IS_GOOD(ch)) || (NPC_FLAGGED(rndm, NPC_QUEST_EVIL) &&
			 IS_EVIL(ch)) || (IS_NEUTRAL(ch) &&
			 (NPC_FLAGGED(rndm, NPC_QUEST_EVIL) || 
			  NPC_FLAGGED(rndm, NPC_QUEST_GOOD))))
		  {
			 if (!number(0, count))
			       victim = rndm;
			      count++;
		  }
	  }
	
  	if (victim == NULL)
    {
  sprintf(buf, "������ ����� ��������: ��������, �� � ������ ������ � ���� ��� ��� ��� �������.");
  act(buf, FALSE, questman, 0, ch, TO_VICT);
  sprintf(buf, "������� �������.");
  act(buf, FALSE, questman, 0, ch, TO_VICT);
  return;
    }


if (victim->mob_specials.max_factor && victim->mob_specials.max_factor < get_kill_vnum(ch,GET_MOB_VNUM(victim)))
        GET_QUESTMAX(ch) = (GET_LEVEL(ch) + number(GET_LEVEL(ch)/6, GET_LEVEL(ch)/2)) *
        (victim->mob_specials.max_factor + GET_LEVEL(victim) + 5) /
        (get_kill_vnum(ch,GET_MOB_VNUM(victim)) + GET_LEVEL(victim) + 5);
   else
       GET_QUESTMAX(ch) = GET_LEVEL(ch) + number(GET_LEVEL(ch)/6, GET_LEVEL(ch)/2);
       GET_QUESTMAX(ch) = GET_QUESTMAX(ch) * 2 / (GET_REMORT(ch) + 2);

  if (!victim->mob_specials.Questor)
 {   switch(GET_CLASS(victim))
   { 
   case CLASS_HUMAN:
     switch(number(0,1))
    {
	case 0:
        sprintf(buf, "������ ����� ��������:\r\n &g����� ����� ���, %s ���������� ���� �����!&n", GET_NAME(victim));
                act(buf, FALSE, questman, 0, ch, TO_VICT);
        sprintf(buf, " &g�� ����� ����� � ����� ����.&n\r\n&g ������ �� ����������� ���������� ���!&n");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
				break;
	 case 1:
    act("������ ����� ��������:\r\n &g������������ ��������� $n, ������$y �� ������ � �������� �� ���� ������!&n",
					FALSE, victim, 0, ch, TO_VICT);
	sprintf(buf, " &g�� ����� ������, %s ����$y � �������$y %d ���� ������!&n\r\n  &g�������� �� � ��������� ��� �������!&n",
			   GET_NAME(victim), number(2,20));
                act(buf, FALSE, victim, 0, ch, TO_VICT);
				break;
    }
	   break;

  case CLASS_ANIMAL:
     switch(number(0,1))
    {
	case 0:
        sprintf(buf, "������ ����� ��������:\r\n &g� ��������� ����� ���������� ������ �������� ������.&n\r\n"
			         " &g��� ����������� ���������, ���� �� ������������ ����� ��&n\r\n"
					 " &g%s, ���� �� ������ ������������, ������� � ������ $s!&n", GET_PNAME(victim));
                      act(buf, FALSE, victim, 0, ch, TO_VICT);
        break;
    case 1:
        sprintf(buf,"������ ����� ��������:\r\n &g������� ������ ����� ���������� �� ������ ������ ���������.&n\r\n"
					" &g����� ����� ����$u %s.&n\r\n &g����� �� ��������� ���������� ������� ���������,&n "
					"$s ���������� ���������!&n", GET_NAME(victim));
		 act(buf,FALSE, victim, 0, ch, TO_VICT);
		break;
    }
 break;

   case CLASS_DRAGON:
     switch(number(0,1))
    {
	case 0:
        act("������ ����� ��������:\r\n &g������� ����� �� ������� ����$y ��������� $n,&n\r\n"
			" &g$s ���������� ������� �� ����� �����, ����� ������� �����&n\r\n"
			" &g������ � ������ ��� ������ �����! ��������� ������ ���� ���&n\r\n"
			" &g� ������� (���������).&n", FALSE, victim, 0, ch, TO_VICT);
    break;
	 case 1:
    act("������ ����� ��������:\r\n &g������� �������� ����� �������� �����, ������ � ��������. ���� �� ���� ������&n\r\n"
		" &g���������� � ���� ��������� $v, ��� ���� ��������� - �������� ������!&n",
		FALSE, victim, 0, ch, TO_VICT);
	break;
    }
	   break;

   case CLASS_UNDEAD:
     switch(number(0,1))
    {
	case 0:
        act("������ ����� ��������:\r\n &g� ������ ��������, ��� �� �����&n\r\n"
			" &g���-�� ��������� ����� � �� ���� �������� �������� ����� ��������&n\r\n"
			" &g�������� ���. ���� �� ������� ������� $v, ���������� ��� �����&n\r\n"
			" &g������� ������ ��� ��������!&n",
			FALSE, victim, 0, ch, TO_VICT);
        break;
	 case 1:
    act("������ ����� ��������:\r\n &g� ����� ����� �����$u $n � ��������� �������� �����.&n\r\n"
		" &g���������� ��� �������� � ������������� ������!&n", FALSE, victim, 0, ch, TO_VICT);
	break;
    }
	   break;
	 
	 default:  
    switch(number(0,1))
    {
	case 0:
        sprintf(buf, "������ ����� ��������:\r\n &g�� �������, %s, ���������� ����� ����?!&n", GET_NAME(victim));
                act(buf, FALSE, questman, 0, ch, TO_VICT);
        sprintf(buf, " &g��� ������ ������ ���� ����������!&n");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
				break;
	 case 1:
    act("������ ����� ��������:\r\n &g������������ ��������� $n, ������$y �� ������ � �������� �� ���� ������!&n",
					FALSE, victim, 0, ch, TO_VICT);
	sprintf(buf, " &g�� ����� ������, %s ����$y � �������$y %d �������!&n",
			   GET_NAME(victim), number(2,20));
                act(buf, FALSE, victim, 0, ch, TO_VICT);
				break;
    }
   
   }

 }
   else
       send_to_char(victim->mob_specials.Questor, ch); 
       if (world[victim->in_room].name != NULL)
	     { sprintf(buf, "&g�����, ��� ���������� &W%s&g:\r\n &W���� '%s'&g, &W� ������: %s&n",
	       GET_NAME(victim),zone_table[world[victim->in_room].zone].name,
	       world[victim->in_room].name);
               act(buf, FALSE, victim, 0, ch, TO_VICT);
 	     }


		GET_QUESTMOB(ch)     = victim->id;
		GET_QUESTOR(victim)  = 1;
   
    return;
}

// Called from update_handler() by pulse_area 

void quest_update(struct char_data * ch)
{
    struct char_data *vict;

    if (GET_NEXTQUEST(ch) > 0)
	 {  GET_NEXTQUEST(ch)--;
             if (GET_NEXTQUEST(ch) == 0)
		 { send_to_char("&G�� ����� ������ �������� �������!&n\r\n",ch);
                    return;
		 }
	  }
    else 
    if (IS_QUESTOR(ch))
        { if (--GET_COUNTDOWN(ch) <= 0)
			{	GET_NEXTQUEST(ch) = number(3, 6);
				sprintf(buf, "&K�� �� ������ ��������� �������.&n\r\n&G��������� ������� �� ������ �������� ����� %d %s.&n\r\n",
				GET_NEXTQUEST(ch), desc_count(GET_NEXTQUEST(ch),WHAT_MINu));
				send_to_char(buf, ch);
				REMOVE_BIT(PLR_FLAGS(ch, PLR_QUESTOR), PLR_QUESTOR);
		            if (GET_QUESTMOB(ch))
				{ vict = get_mob_by_id(GET_QUESTMOB(ch));
                                  if (vict)
				  GET_QUESTOR(vict) = 0;
				}
			//GET_QUESTGIVER(ch) = NULL;
                                GET_COUNTDOWN(ch) = 0;
                                GET_QUESTMOB(ch) = 0;
			}
		 if (GET_COUNTDOWN(ch) == 4)
			{  send_to_char("&g������������! ��� ���������� ������� �������� ���� �������!&n\r\n",ch);
			return;
			}
	}
     
}




/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */
CHAR_DATA *get_player_of_name(CHAR_DATA * ch, char *name)
{
	CHAR_DATA *i;

	for (i = character_list; i; i = i->next) {
		if (IS_NPC(i))
			continue;
		if (!isname(name, i->player.name))
			continue;
		return (i);
	}

	return (NULL);
}



SPECIAL(bank)
{
  CHAR_DATA *vict;
  int amount, sbor;

  if (CMD_IS("������")||CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "�� ����� ����� &c%ld&n %s.\r\n",
	      GET_BANK_GOLD(ch), desc_count(GET_BANK_GOLD(ch),WHAT_MONEYa)); 
    else
      sprintf(buf, "� ��������� ����� �� ����� ����� ������ ���.\r\n"); 
    send_to_char(buf, ch);
    return (1);
  } else if (CMD_IS("�������")||CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("������� �� ������ ������� �����?\r\n", ch);
      return (1);
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char("� ��� ��� ������� �������� �����!\r\n", ch); 
      return (1);
    }
    GET_GOLD(ch) -= amount;
    sbor = amount/20;
	if (sbor <1)
	    sbor = 1;
   if (GET_LEVEL(ch) <= 10)
         sbor = 0;
    GET_BANK_GOLD(ch) += (amount - sbor);
    sprintf(buf, "�� ������� %d %s.\r\n�� ������, ���� ������� %d %s.\r\n", amount,
		          desc_count(amount,WHAT_MONEYu), sbor, desc_count(sbor,WHAT_MONEYu)); 
    send_to_char(buf, ch);
    act("$n ��������$y ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM); 
    return (1);
  } else if (CMD_IS("�����")||CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("������� �� ������ ����� �����?\r\n", ch);
      return (1);
    }
     if (GET_BANK_GOLD(ch) < amount) {
      send_to_char("��� � �� ������� ������� ����� �� ����� �����!\r\n", ch); 
      return (1);
    }
    GET_BANK_GOLD(ch) -= amount;
    GET_GOLD(ch) += amount;
    sprintf(buf, "�� ����� �� ������ ����� %d %s.\r\n", amount, desc_count(amount,WHAT_MONEYu)); 
    send_to_char(buf, ch);
    act("$n ��������$y ���������� ��������.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (1);
  } else
	  if (CMD_IS("transfer") || CMD_IS("���������"))
	  { argument = one_argument(argument, arg);
		amount = atoi(argument);
		if (IS_GOD(ch) && !IS_IMPL(ch))
		{ send_to_char("�� � ��, ������ �������?\r\n", ch);
			return (1);
		}
		if (!*arg)
		{ send_to_char("���� �� ������ ������� �������� �������?\r\n", ch);
			return (1);
		}
		if (amount <= 0)
		{ send_to_char("����� ���������� ����� �� ������ ���������?\r\n", ch);
			return (1);
		}
		if (amount <= 100)
		{ send_to_char("����� ��������� �������� �� ������ ���� ����� 100 �����.\r\n", ch);
			return (1);
		}

		if (GET_BANK_GOLD(ch) < amount) {
			send_to_char("� ��� ��� ����� ����� ��� ��������.\r\n", ch);
			return (1);
		}
		if (GET_BANK_GOLD(ch) < amount + ((amount * 5) / 100)) {
			send_to_char("��� ����� ��������� �� ��������� ������ ��������!\r\n", ch);
			return (1);
		}
		if ((vict = get_player_of_name(ch, arg))) {
			GET_BANK_GOLD(ch) -= amount + (amount * 5) / 100;
			sprintf(buf, "&W�� �������� %d %s �� ���������� ���� %s.&n\r\n", amount, desc_count(amount,WHAT_MONEYu), GET_DNAME(vict) );
			send_to_char(buf, ch);
			GET_BANK_GOLD(vict) += amount;
			sprintf(buf, "&W�� �������� ���������� ������� �� %d %s �� %s.&n\r\n", amount, desc_count(amount,WHAT_MONEYu), GET_RNAME(ch));
			send_to_char(buf, vict);
			return (1);
		} else {
			CREATE(vict, CHAR_DATA, 1);
			clear_char(vict);
			if (load_char(arg, vict) < 0) {
				send_to_char("������ ��������� �� ���������� � ����.\r\n", ch);
				free(vict);
				return (1);
			}
			GET_BANK_GOLD(ch) -= amount + (amount * 5) / 100;
			sprintf(buf, "&W�� �������� %d %s �� ���������� ���� %s.&n\r\n", amount, desc_count(amount,WHAT_MONEYu), GET_DNAME(vict));
			send_to_char(buf, ch);
			GET_BANK_GOLD(vict) += amount;
			save_char(vict, GET_LOADROOM(vict));
			free_char(vict);
			return (1);
		}
	} else
    return (0);
}


/***************************************************************/
/*    ������� ����������		  		       */
/***************************************************************/

SPECIAL(power_stat)
{
   int coins = 1000;
    
  if  (CMD_IS("������"))
  { 
	  send_to_char ("� �������� ���������� �� ������ �������� ��������� ��������������.\r\n", ch);
  send_to_char("��� �� �������� �������������� �� ����� ���������� �������� &C\"�����������\"&n ��������������.\r\n", ch);
  send_to_char("��� �� �������� �������������� �� ��� ���������� �������� &C\"������\"&n ��������������.\r\n", ch);
  send_to_char("**********************************************************************\r\n", ch);
  send_to_char("| &C��������������                 ���������&n                           |\r\n", ch);
  send_to_char("**********************************************************************\r\n", ch);
   sprintf(buf,"| &W����                        - %5d ����� ����� ��� 35000 ���&n     |\r\n", coins);            
sprintf(buf+strlen(buf),"| &W����                        - %5d ����� ����� ��� 35000 ���&n     |\r\n", coins);
sprintf(buf+strlen(buf),"| &W��������                    - %5d ����� ����� ��� 35000 ���&n     |\r\n", coins);	  
sprintf(buf+strlen(buf),"| &W���������                   - %5d ����� ����� ��� 35000 ���&n     |\r\n", coins);
sprintf(buf+strlen(buf),"| &W��������                    - %5d ����� ����� ��� 35000 ���&n     |\r\n", coins);
sprintf(buf+strlen(buf),"| &W�����                       - %5d ����� ����� ��� 35000 ���&n     |\r\n", coins);
   send_to_char(buf, ch);
   send_to_char("**********************************************************************\r\n", ch);
  return (1);
  }
  
  if (IS_NPC(ch) || !CMD_IS("������"))  {
  if (IS_NPC(ch) || !CMD_IS("�����������"))
    return (0);
    else {
          skip_spaces(&argument);
           
		  if (!*argument)
		  {  send_to_char("����� �������������� �� ������ �����������?\r\n", ch);
                   return (1);
		  }		  
		       if(GET_GLORY(ch) < coins)
				{ send_to_char("� ��� ��� ������������ ���������� ����� ��� ����������!\r\n", ch);	 
				 return (1);
				}

     		if(isname(argument, "����"))
			{	 if ((GET_POINT_STAT(ch, STR) + 5 + GET_STR_ROLL(ch)) <= GET_STR(ch))
					{ send_to_char("�� ������ �� ������ ����������� ����!\r\n", ch);
                       return (1);
					}
				     GET_STR(ch)++;
				     GET_GLORY(ch) -= coins;
				     GET_QUESTPOINTS(ch) +=coins;
				     send_to_char("�� �������� ���� ���� �� �������.\r\n", ch);
			}
			if(isname(argument, "����")) {
				  if ((GET_POINT_STAT(ch, CON) + 5 + GET_CON_ROLL(ch)) <= GET_CON(ch))
					 { send_to_char("�� ������ �� ������ ����������� ����!\r\n", ch);
                       return (1);
					 }
				   GET_CON(ch)++;
				   GET_MAX_HIT(ch) += GET_CON(ch);
				   GET_GLORY(ch) -= coins;
				   GET_QUESTPOINTS(ch) += coins;
			       send_to_char("�� �������� ���� ���� �� �������.\r\n", ch);
				}
			if(isname(argument, "���������")) {
                  if ((GET_POINT_STAT(ch, INT) + 5 + GET_INT_ROLL(ch)) <= GET_INT(ch))
					 { send_to_char("�� ������ �� ������ ����������� ���������!\r\n", ch);
                       return (1);
					 }
				  GET_INT(ch)++;
				  GET_GLORY(ch) -= coins;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("�� �������� ���� ��������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "��������")) {
                 if ((GET_POINT_STAT(ch, WIS) + 5 + GET_WIS_ROLL(ch)) <= GET_WIS(ch))
					 { send_to_char("�� ������ �� ������ ����������� ��������!\r\n", ch);
                       return (1);
					 }
				 GET_WIS(ch)++;
				 GET_GLORY(ch) -= coins;
				 GET_QUESTPOINTS(ch) += coins;
				 if (IS_WEDMAK(ch))
                 GET_MAX_MANA(ch) += GET_WIS(ch);
			  send_to_char("�� �������� ���� �������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "��������")) {
				  if ((GET_POINT_STAT(ch, DEX) + 5 + GET_DEX_ROLL(ch)) <= GET_DEX(ch))
					 { send_to_char("�� ������ �� ������ ����������� ��������!\r\n", ch);
                       return (1);
					 }
				  GET_DEX(ch)++;
				  GET_GLORY(ch) -= coins;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("�� �������� ���� �������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "�����")) {
                  if ((GET_POINT_STAT(ch, CHA) + 5 + GET_CHA_ROLL(ch)) <= GET_CHA(ch))
					 { send_to_char("�� ������ �� ������ ����������� �����!\r\n", ch);
                       return (1);
					 }
				  GET_CHA(ch)++;
				  GET_GLORY(ch) -= coins;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("�� �������� ���� ����� �� �������.\r\n", ch);
			 }
	save_char(ch, NOWHERE);	
          
  
	}
	  return (1);
  }

  skip_spaces(&argument);
   if (!*argument)
		  {  send_to_char("����� �������������� �� ������ �����������?\r\n", ch);
                   return (1);
		  }		  
		       if(IND_POWER_CHAR(ch) < 35000)
				{ send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ����������!\r\n", ch); 
				 return (1);
				}

     		if(isname(argument, "����"))
			{	 if ((GET_POINT_STAT(ch, STR) + 5 + GET_STR_ROLL(ch)) <= GET_STR(ch))
					{ send_to_char("�� ������ �� ������ ����������� ����!\r\n", ch);
                       return (1);
					}
				     GET_STR(ch)++;
				     IND_POWER_CHAR(ch) -= 35000;
					 GET_QUESTPOINTS(ch) += coins;
				     send_to_char("�� �������� ���� ���� �� �������.\r\n", ch);
			}
			if(isname(argument, "����")) {
				  if ((GET_POINT_STAT(ch, CON) + 5 + GET_CON_ROLL(ch)) <= GET_CON(ch))
					 { send_to_char("�� ������ �� ������ ����������� ����!\r\n", ch);
                       return (1);
					 }
				   GET_CON(ch)++;
				   GET_MAX_HIT(ch) += GET_CON(ch);
				   IND_POWER_CHAR(ch) -= 35000;
				   GET_QUESTPOINTS(ch) += coins;
			       send_to_char("�� �������� ���� ���� �� �������.\r\n", ch);
				}
			if(isname(argument, "���������")) {
                  if ((GET_POINT_STAT(ch, INT) + 5 + GET_INT_ROLL(ch)) <= GET_INT(ch))
					 { send_to_char("�� ������ �� ������ ����������� ���������!\r\n", ch);
                       return (1);
					 }
				  GET_INT(ch)++;
				  IND_POWER_CHAR(ch) -= 35000;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("�� �������� ���� ��������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "��������")) {
                 if ((GET_POINT_STAT(ch, WIS) + 5 + GET_WIS_ROLL(ch)) <= GET_WIS(ch))
					 { send_to_char("�� ������ �� ������ ����������� ��������!\r\n", ch);
                       return (1);
					 }
				 GET_WIS(ch)++;
				 IND_POWER_CHAR(ch) -= 35000;
				 GET_QUESTPOINTS(ch) += coins;
				 if (IS_WEDMAK(ch))
                 GET_MAX_MANA(ch) += GET_WIS(ch);
			  send_to_char("�� �������� ���� �������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "��������")) {
				  if ((GET_POINT_STAT(ch, DEX) + 5 + GET_DEX_ROLL(ch)) <= GET_DEX(ch))
					 { send_to_char("�� ������ �� ������ ����������� ��������!\r\n", ch);
                       return (1);
					 }
				  GET_DEX(ch)++;
				  IND_POWER_CHAR(ch) -= 35000;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("�� �������� ���� �������� �� �������.\r\n", ch);
			 }
			if(isname(argument, "�����")) {
                  if ((GET_POINT_STAT(ch, CHA) + 5 + GET_CHA_ROLL(ch)) <= GET_CHA(ch))
					 { send_to_char("�� ������ �� ������ ����������� �����!\r\n", ch);
                       return (1);
					 }
				  GET_CHA(ch)++;
				  IND_POWER_CHAR(ch) -= 35000;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("�� �������� ���� ����� �� �������.\r\n", ch);
			 }
	save_char(ch, NOWHERE);	


  return (1);
}

SPECIAL(cleric)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  struct price_info {
    short int number;
    char name[25];
    short int price;
  } prices[] = {
    // ����� ���������� 	�������� ����������	   ����
    { SPELL_ARMOR, 		"������               ", 30 },
    { SPELL_FLY,		"�����                ", 15 },
    { SPELL_CURE_SERIOUS,	"���أ���� ���������  ", 45 },
	{ SPELL_REMOVE_CURSE,	"����� ���������      ", 150 },
    { SPELL_BLESS, 		"�����������          ", 15 },
    { SPELL_INVISIBLE,		"�����������          ", 300 },
    { SPELL_REMOVE_POISON, 	"�������������� ��    ", 90 },
    { SPELL_CURE_BLIND, 	"�������� �������     ", 150 },
    { SPELL_SANCTUARY, 		"���������            ", 30 },
    { SPELL_HEAL, 		"���������            ", 600 },
    { SPELL_REFRESH,            "��������������       ", 15 },
    { SPELL_LIGHT,              "����                 ", 10 },
    { SPELL_RESTORE_MAGIC,      "����������           ", 10000 },

    // ����� ����� ��������� �������������� ������ 
    { -1, "\r\n", -1 }
  };


  if (CMD_IS("��������")) {
    argument = one_argument(argument, buf);

    if (GET_POS(ch) == POS_FIGHTING) return TRUE;

    if (*buf) {
      for (i=0; prices[i].number > SPELL_RESERVED_DBC; i++) {
	if (is_abbrev(buf, prices[i].name)) 
	  if (GET_GOLD(ch) < prices[i].price) {
	    act("$n ������ ���: \"� ��� ��� ������� �����!\"",
		FALSE, (struct char_data *) me, 0, ch, TO_VICT);
            return TRUE;
          } else {
	    
	    act("$N ���$Y �� ������ ��������� ����� $d.",
		FALSE, (struct char_data *) me, 0, ch, TO_NOTVICT);
	    sprintf(buf, "�� ���� %s %d �����.\r\n", 
		    GET_DNAME((struct char_data *) me), prices[i].price);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) -= prices[i].price;
	     cast_spell((struct char_data *) me, ch, NULL, prices[i].number);
	    return TRUE;
          
	  }
      }
      act("$n ������ ���: \"� �� ������ ������ ������������!\"\r\n"
	  "  ����������� &W��������&n ��� �� ������ ��������� ����������.", FALSE, (struct char_data *) me, 
	  0, ch, TO_VICT);
	  
      return TRUE;
    } else {
      act("$n ������ ���: \"����� ������ � ���� �� ��� ������, �������� �� ������ ���������������.\"",
	  FALSE, (struct char_data *) me, 0, ch, TO_VICT);
      for (i=0; prices[i].number > SPELL_RESERVED_DBC; i++) {
        sprintf(buf, "&W%s&n %-25d\r\n", prices[i].name, prices[i].price);
        send_to_char(buf, ch);
      }
      return TRUE;
    }
  }

  if (cmd) return FALSE;

  // ������������, �������� ���������� � ����. 
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (!number(0, 4))
      break;
  
  // ���������, ���� �� ����, ����� �������, ��� �� �� ���� �� ����������� 
  if (vict == NULL || IS_NPC(vict) || (GET_LEVEL(vict) > 10))
    return FALSE;

  act("������ ������ ����, ������������ �����, ������ ��������� �������������� ������.", TRUE, ch, 0, vict, TO_ROOM);
  
  switch (number(1, GET_LEVEL(vict)))
  { 
      case 1: cast_spell(ch, vict, NULL, SPELL_CURE_LIGHT); break; 
      case 2: cast_spell(ch, vict, NULL, SPELL_BLESS); break; 
      case 3: cast_spell(ch, vict, NULL, SPELL_ARMOR); break;
      case 4: cast_spell(ch, vict, NULL, SPELL_FLY); break; 
      case 5: cast_spell(ch, vict, NULL, SPELL_BLESS); break; 
      case 6: cast_spell(ch, vict, NULL, SPELL_CURE_CRITIC); break; 
      case 7: cast_spell(ch, vict, NULL, SPELL_ARMOR); break;
      case 8: cast_spell(ch, vict, NULL, SPELL_CURE_BLIND); break; 
      case 9: cast_spell(ch, vict, NULL, SPELL_ARMOR); break; 
  
        break; 
  }
  return TRUE; 
}  


//��������� ���������� �� ����������
void LoadToCharObj(CHAR_DATA * ch, CHAR_DATA * vict, int vnum)
{ OBJ_DATA *obj;

 //��������� ������� ��������� �������, ����� NULL
 if ((obj = read_object(vnum, VIRTUAL))) 
	{ obj_to_char(obj, ch);
	  act("$n ���$y ��� $3.", FALSE, vict, obj, ch, TO_VICT);
	}
}

//������� �������� �������� � ��������� � ���������� � � ���������.
//���������� ����������� �� ���������.
//�������� ����� �� ������������ ������ ��������.
//���� �����, ���������� 0, ���� ��� �� 1.

int GetObjInChar(CHAR_DATA * ch, int obj)
{ ubyte i;
  register OBJ_DATA *o;

      for (o = ch->carrying; o; o = o->next_content)
		{ if(CAN_SEE_OBJ (ch, o) &&
		      GET_OBJ_VNUM(o) == obj)
		     return TRUE;
		}
	  
	  for (i = 0; i < NUM_WEARS; i++)
	  { if (!GET_EQ(ch, i))
	     continue;
       if(CAN_SEE_OBJ (ch, GET_EQ(ch, i)) &&
		   obj == GET_OBJ_VNUM(GET_EQ(ch, i))
		   )
             return TRUE;
	  }
  return FALSE;
}

//������� ���������, ���������� ��� ��������� �������.
//������ �������, ����������� ��� ������ �������, � ������ ������������ ������
//�� �������� ���������, �� 22. 
#define CLASSES_OBJ 12
static int ClassObj[][CLASSES_OBJ] = {
{1600, 1620, 1624, 1632, 1634, 1635, 1637, 1641, 1645, 1662, 1671, 1672 },
{1608, 1621, 1624, 1632, 1633, 1635, 1637, 1641, 1645, 1662, 1671, 1672 },
{1604, 1631, 1637, 1642, 1646, 1655, 1662, 1663, 1665, 1672, 1673, 0 },
{1607, 1630, 1638, 1639, 1640, 1644, 1657, 1659, 1661, 1662, 1666, 0 },
{1602, 1631, 1637, 1642, 1646, 1655, 1662, 1663, 1665, 1672, 1673, 0 },
{1610, 1630, 1638, 1639, 1640, 1644, 1657, 1660, 1661, 1662, 1667, 0 },
{1606, 1627, 1631, 1637, 1645, 1650, 1655, 1662, 1663, 1672, 0,	   0 },
{1603, 1617, 1632, 1635, 1637, 1641, 1645, 1648, 1662, 1672, 0,	   0 },
{1605, 1613, 1614, 1636, 1637, 1643, 1672, 0   , 0,    0,    0,	   0 },
{1609, 1615, 1619, 1622, 1623, 1625, 1626, 1647, 1668, 1669, 1670, 1675 },
{1601, 1627, 1631, 1637, 1645, 1659, 1662, 1663, 1672, 1674, 0,	   0 }
};


SPECIAL(kladovchik)
 { 
	 if (CMD_IS("��������"))
	 {	 ubyte i;
		 CHAR_DATA *kladov = (struct char_data *) me;
	     skip_spaces(&argument);        
	     if (FIGHTING(ch))
		    return TRUE;
		if (GET_LEVEL(ch) > 49)
		{ act("&C$n ������ ���: \"�� � ��������� ���� � ���� ������������.\"&n",
	           FALSE, (struct char_data *) me, 0, ch, TO_VICT);
			   return TRUE;
		}
	
	   if (!*argument)
		{ sprintf(buf, "&C$n ������ ���: \"�� ������:&n\r\n �������� �������\r\n �������� ������\r\n �������� ������\r\n �������� ������      (��������, �� ? ������)\r\n �������� ����        (�� ? ����)\r\n �������� ����������� (�� ? �������)\r\n �������� ���         (�����������������)\r\n");
          act(buf, FALSE, (struct char_data *) me, 0, ch, TO_VICT);
           return TRUE;
	 	}
	     
	  act("&G$n ������ ���: \"������ ���������, ����� �� � ��� ��� ���.\"&n", FALSE, kladov, NULL, ch, TO_VICT);
	  act("$n �����$y ������ �� ���������.", FALSE, kladov, NULL, ch, TO_ROOM);

	    if (!str_cmp(argument, "�������") || !str_cmp(argument, "���") && GET_LEVEL(ch) < 17)
		{  if (!GetObjInChar(ch, 3639))
		    { LoadToCharObj(ch, kladov, 3639);
		      LoadToCharObj(ch, kladov, 3639);
		      LoadToCharObj(ch, kladov, 3639);
                      LoadToCharObj(ch, kladov, 1805);
			}
		  else
		    act("&G$n ������ ���: \"������� ������� � ��� ����. ��� ���������� ��������.\"&n", FALSE, kladov, NULL, ch, TO_VICT); 

		   if (!GetObjInChar(ch, 102))
		       LoadToCharObj(ch, kladov, 102);
		   else
		      act("&G$n ������ ���: \"����� � ��� ����, ���������� �� �� ������� (������; ��).\"&n", FALSE, kladov, NULL, ch, TO_VICT);	   
		}
	
	 if (!str_cmp(argument, "������") || !str_cmp(argument, "���"))
		{  for (i = 0; i < CLASSES_OBJ; i++)
			{ if (!ClassObj[GET_CLASS(ch)][i])
				    continue; 
	           if (!GetObjInChar(ch, ClassObj[GET_CLASS(ch)][i]))
			        LoadToCharObj(ch, kladov, ClassObj[GET_CLASS(ch)][i]);
			}
				   
			if(!GetObjInChar(ch, 1611))
			   LoadToCharObj(ch, kladov, 1611);//��� ���� �� ���

			if(!GetObjInChar(ch, 1616))
		    { LoadToCharObj(ch, kladov, 1616);//�������
			  LoadToCharObj(ch, kladov, 1616);//�������
			}		
		}
     
	 if (!str_cmp(argument, "������") || !str_cmp(argument, "���"))
		{ switch (GET_CLASS(ch))
			{ case CLASS_CLERIC:
	           if (!GetObjInChar(ch, 1612))
				   LoadToCharObj(ch, kladov, 1612);
			    break;
              case CLASS_MAGIC_USER:
			  if (!GetObjInChar(ch, 1653))
			      LoadToCharObj(ch, kladov, 1653);
				  break;
			  case CLASS_THIEF:
			  if (!GetObjInChar(ch, 1652))
			      { LoadToCharObj(ch, kladov, 1652);
			        LoadToCharObj(ch, kladov, 1652);
			      }
			    break;
			  case CLASS_WARRIOR:
			  if (!GetObjInChar(ch, 1649))
			 	   LoadToCharObj(ch, kladov, 1649);
			  if (!GetObjInChar(ch, 1656))
				  LoadToCharObj(ch, kladov, 1656);
			    break;
			  case CLASS_ASSASINE:
			  if (!GetObjInChar(ch, 1664))
			      { LoadToCharObj(ch, kladov, 1664);
                                LoadToCharObj(ch, kladov, 1664);
			      }
			    break;
			  case CLASS_TAMPLIER:
			  if (!GetObjInChar(ch, 1649))
			 	  LoadToCharObj(ch, kladov, 1649);
			  if (!GetObjInChar(ch, 1656))
				  LoadToCharObj(ch, kladov, 1656);
			    break;
			  case CLASS_SLEDOPYT:
			  if (!GetObjInChar(ch, 1658))
				   LoadToCharObj(ch, kladov, 1658);
			    break;
			  case CLASS_DRUID:
			  if (!GetObjInChar(ch, 1618))
				  LoadToCharObj(ch, kladov, 1618);
			    break;
			  case CLASS_MONAH:
			    break;
			  case CLASS_VARVAR:
			  if (!GetObjInChar(ch, 1629))
				  LoadToCharObj(ch, kladov, 1629);
			  if (!GetObjInChar(ch, 1651))
			      LoadToCharObj(ch, kladov, 1651);
			    break;
			  case CLASS_WEDMAK:
			  if (!GetObjInChar(ch, 1654))
				  LoadToCharObj(ch, kladov, 1654);
			    break;
			}
		}
	  if (!str_cmp(argument, "�����������") ||
		  !str_cmp(argument, "�������")     ||
		  !str_cmp(argument, "���")
		  )
	  {		if (!GetObjInChar(ch, 138))
		       LoadToCharObj(ch, kladov, 138);
	  
	  }
	  if (!str_cmp(argument, "����") || !str_cmp(argument, "���"))
	  {		if (!GetObjInChar(ch, 103))
		       LoadToCharObj(ch, kladov, 103);
	  
	  }
	  if (!str_cmp(argument, "������") || !str_cmp(argument, "���"))
	  {		if (!GetObjInChar(ch, 104))
		       LoadToCharObj(ch, kladov, 104);
	  
	  }
	return TRUE;
   }
 return FALSE;
}

SPECIAL(SaleBonus)
{

	if (IS_NPC(ch))
	 return  false;

	 if  (CMD_IS("������"))
  { 
	
  
  send_to_char("&W ----------------------------------------------------------------------------------- \r\n", ch);
 sprintf(buf, "/&c  ---&w***&c---                ---<<< &C���� �������:&c  >>>---                 ---&w***&c---&W  \\\r\n");
 send_to_char(buf, ch); 
 
 send_to_char("|  ---&c^^^&W-------------------------------------------------------------------&c^^^&W---  |\r\n", ch);
  
send_to_char( "&W|&c\\&W|                                                                               &W|&c/&W|\r\n"
			  "|&w*&W|&c ���������  : &n20000  &c������     : &n35000  &c�������  : &n25000  &c�������    : &n45000  &W|&w*&W|\r\n"
			  "|&c/&W|&c ���������� : &n5000   &c��������   : &n10000  &c������   : &n65000  &c������     : &n65000  &W|&c\\&W|\r\n"
			  "|&w*&W|&c ���c����   : &n140000 &c���������� : &n85000  &c������   : &n50000  &c���������� : &n80000  &W|&w*&W|\r\n"
			  "|&c\\&W|&c ����       : &n70000  &c���������  : &n65000  &c�������  : &n70000  &c��������   : &n75000  &W|&c/&W|\r\n"
			  "|&w*&W|&c ��������   : &n135000 &c���������  : &n135000&c �������� : &n120000 &c����       : &n120000 &W|&w*&W|\r\n"
		      "|&c\\&W|&c                     ����       : &n125000 &c�����    : &n115000                     &W|&c/&W|\r\n"
			  "|  \\_____________________________________________________________________________/  |\r\n"
              "| / ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^&c___&w***&c___&W^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \\ |\r\n"
			  "\\__________________________________________________________________________________/&n\r\n", ch);
  
  
  return (1);
}
 
	  if (CMD_IS("������"))  
      	{ 
		  skip_spaces(&argument);

		if (!*argument)
		  {  send_to_char("����� ����� �� ������ ������?\r\n", ch);
                   return (1);
		  }	

     		if(isname(argument, "���������"))
			{	 if (IND_SHOP_POWER(ch) < 20000)
					{ send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c���������!&n\r\n", ch);
                       return true;
					}
				 if (PLR_FLAGGED(ch, AFF_CLAN_HITPOINT))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}
					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_HITPOINT), AFF_CLAN_HITPOINT);
				    IND_SHOP_POWER(ch) -= 20000;
				    send_to_char("�� ������ ����� &c���������&n.\r\n", ch);
			}

		   else
			   if(isname(argument, "������"))
			{	  if (IND_SHOP_POWER(ch) < 35000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_MORALE))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_MORALE), AFF_CLAN_MORALE);
				    IND_SHOP_POWER(ch) -= 35000;
				    send_to_char("�� ������ ����� &c������&n.\r\n", ch);	
				}

			else
				if(isname(argument, "�������"))
			{	  if (IND_SHOP_POWER(ch) < 25000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c�������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_DAMROLL))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_DAMROLL), AFF_CLAN_DAMROLL);
				    IND_SHOP_POWER(ch) -= 25000;
				    send_to_char("�� ������ ����� &c�������&n.\r\n", ch);	
			}
			
			else
				if(isname(argument, "�������"))
			{	  if (IND_SHOP_POWER(ch) < 40000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c�������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_HITROLL))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_HITROLL), AFF_CLAN_HITROLL);
				    IND_SHOP_POWER(ch) -= 40000;
				    send_to_char("�� ������ ����� &c�������&n.\r\n", ch);	
			}
			
			else
				if(isname(argument, "����������"))
			{	  if (IND_SHOP_POWER(ch) < 5000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c����������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_INITIATIVE))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_INITIATIVE), AFF_CLAN_INITIATIVE);
				    IND_SHOP_POWER(ch) -= 5000;
				    send_to_char("�� ������ ����� &c����������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (IND_SHOP_POWER(ch) < 10000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c��������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_MOVE))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_MOVE), AFF_CLAN_MOVE);
				    IND_SHOP_POWER(ch) -= 10000;
				    send_to_char("�� ������ ����� &c��������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "������"))
			{	  if (IND_SHOP_POWER(ch) < 65000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_HITREG))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_HITREG), AFF_CLAN_HITREG);
				    IND_SHOP_POWER(ch) -= 65000;
				    send_to_char("�� ������ ����� &c������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "������"))
			{	  if (IND_SHOP_POWER(ch) < 65000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_MOVEREG))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_MOVEREG), AFF_CLAN_MOVEREG);
				    IND_SHOP_POWER(ch) -= 65000;
				    send_to_char("�� ������ ����� &c������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (IND_SHOP_POWER(ch) < 140000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c�������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_CAST_SUCCES))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_CAST_SUCCES), AFF_CLAN_CAST_SUCCES);
				    IND_SHOP_POWER(ch) -= 140000;
				    send_to_char("�� ������ ����� &c�������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "����������"))
			{	  if (IND_SHOP_POWER(ch) < 85000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c����������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_MANAREG))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_MANAREG), AFF_CLAN_MANAREG);
				    IND_SHOP_POWER(ch) -= 85000;
				    send_to_char("�� ������ ����� &c����������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "������"))
			{	  if (IND_SHOP_POWER(ch) < 50000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_AC))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_AC), AFF_CLAN_AC);
				    IND_SHOP_POWER(ch) -= 50000;
				    send_to_char("�� ������ ����� &c������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "����������"))
			{	  if (IND_SHOP_POWER(ch) < 80000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c����������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_ABSORBE))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_ABSORBE), AFF_CLAN_ABSORBE);
				    IND_SHOP_POWER(ch) -= 80000;
				    send_to_char("�� ������ ����� &c����������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "����"))
			{	  if (IND_SHOP_POWER(ch) < 70000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c����!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_SPELL))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_SPELL), AFF_CLAN_SAVE_SPELL);
				    IND_SHOP_POWER(ch) -= 70000;
				    send_to_char("�� ������ ����� &c����&n.\r\n", ch);	
			}

			else
				if(isname(argument, "���������"))
			{	  if (IND_SHOP_POWER(ch) < 65000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c���������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_PARA))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_PARA), AFF_CLAN_SAVE_PARA);
				    IND_SHOP_POWER(ch) -= 65000;
				    send_to_char("�� ������ ����� &c���������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "�������"))
			{	  if (IND_SHOP_POWER(ch) < 70000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c�������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_ROD))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_ROD), AFF_CLAN_SAVE_ROD);
				    IND_SHOP_POWER(ch) -= 70000;
				    send_to_char("�� ������ ����� &c�������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (IND_SHOP_POWER(ch) < 75000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c��������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_BASIC))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_BASIC), AFF_CLAN_SAVE_BASIC);
				    IND_SHOP_POWER(ch) -= 75000;
				    send_to_char("�� ������ ����� &c��������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (IND_SHOP_POWER(ch) < 135000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c��������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_WIS))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_WIS), AFF_CLAN_WIS);
				    IND_SHOP_POWER(ch) -= 135000;
				    send_to_char("�� ������ ����� &c��������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "���������"))
			{	  if (IND_SHOP_POWER(ch) < 135000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c���������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_INT))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_INT), AFF_CLAN_INT);
				    IND_SHOP_POWER(ch) -= 135000;
				    send_to_char("�� ������ ����� &c���������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (IND_SHOP_POWER(ch) < 120000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c��������!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_DEX))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_DEX), AFF_CLAN_DEX);
				    IND_SHOP_POWER(ch) -= 120000;
				    send_to_char("�� ������ ����� &c��������&n.\r\n", ch);	
			}


			else
				if(isname(argument, "����"))
			{	  if (IND_SHOP_POWER(ch) < 120000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c����!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_STR))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_STR), AFF_CLAN_STR);
				    IND_SHOP_POWER(ch) -= 120000;
				    send_to_char("�� ������ ����� &c����&n.\r\n", ch);	
			}


			else
				if(isname(argument, "����"))
			{	  if (IND_SHOP_POWER(ch) < 125000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c����!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_CON))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_CON), AFF_CLAN_CON);
				    IND_SHOP_POWER(ch) -= 125000;
				    send_to_char("�� ������ ����� &c����&n.\r\n", ch);	
			}

			else
				if(isname(argument, "�����"))
			{	  if (IND_SHOP_POWER(ch) < 115000)		  
					 { send_to_char("� ��� ��� ������������ ���������� &c���&n ��� ������� ������ &c�����!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_LCK))
					{ send_to_char("���� ����� � ��� ��� �������!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_LCK), AFF_CLAN_LCK);
				    IND_SHOP_POWER(ch) -= 115000;
				    send_to_char("�� ������ ����� &c�����&n.\r\n", ch);	
			}

	save_char(ch, NOWHERE);	
return true;
  }


	
//����� �� �������, 1 ������� �� ��������, ��� �� �� ����� ���� ����.
	if (CMD_IS("�������"))  
      	{ 
		  skip_spaces(&argument);

		if (!*argument)
		  {  send_to_char("����� ����� �� ������ �������?\r\n", ch);
                   return (1);
		  }	

     			if(isname(argument, "���������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_HITPOINT))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}
					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_HITPOINT), AFF_CLAN_HITPOINT);
				    IND_SHOP_POWER(ch) += 20000;//20000
				    send_to_char("�� ������� ����� &c���������&n.\r\n", ch);
			}

			else
				if(isname(argument, "������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_MORALE))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_MORALE), AFF_CLAN_MORALE);
				    IND_SHOP_POWER(ch) += 35000;//35000
				    send_to_char("�� ������� ����� &c������&n.\r\n", ch);	
				}


			else
				if(isname(argument, "�������"))
			{	   if (!PLR_FLAGGED(ch, AFF_CLAN_DAMROLL))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_DAMROLL), AFF_CLAN_DAMROLL);
				    IND_SHOP_POWER(ch) += 25000;//25000
				    send_to_char("�� ������� ����� &c�������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "�������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_HITROLL))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_HITROLL), AFF_CLAN_HITROLL);
				    IND_SHOP_POWER(ch) += 40000;//40000
				    send_to_char("�� ������� ����� &c�������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "����������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_INITIATIVE))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_INITIATIVE), AFF_CLAN_INITIATIVE);
				    IND_SHOP_POWER(ch) += 5000;//5000
				    send_to_char("�� ������� ����� &c����������&n.\r\n", ch);	
			}
		
			else
				if(isname(argument, "��������"))
			{	   if (!PLR_FLAGGED(ch, AFF_CLAN_MOVE))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_MOVE), AFF_CLAN_MOVE);
				    IND_SHOP_POWER(ch) += 10000;//10000
				    send_to_char("�� ������� ����� &c��������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "������"))
			{	 if (!PLR_FLAGGED(ch, AFF_CLAN_HITREG))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_HITREG), AFF_CLAN_HITREG);
				    IND_SHOP_POWER(ch) += 65000;//65000
				    send_to_char("�� ������� ����� &c������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_MOVEREG))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_MOVEREG), AFF_CLAN_MOVEREG);
				    IND_SHOP_POWER(ch) += 65000;//65000
				    send_to_char("�� ������� ����� &c������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_CAST_SUCCES))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_CAST_SUCCES), AFF_CLAN_CAST_SUCCES);
				    IND_SHOP_POWER(ch) += 140000;//140000
				    send_to_char("�� ������� ����� &c�������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "����������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_MANAREG))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_MANAREG), AFF_CLAN_MANAREG);
				    IND_SHOP_POWER(ch) += 85000;//85000
				    send_to_char("�� ������� ����� &c����������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_AC))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_AC), AFF_CLAN_AC);
				    IND_SHOP_POWER(ch) += 50000;//50000
				    send_to_char("�� ������� ����� &c������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "����������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_ABSORBE))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_ABSORBE), AFF_CLAN_ABSORBE);
				    IND_SHOP_POWER(ch) += 80000;//80000
				    send_to_char("�� ������� ����� &c����������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_SAVE_SPELL))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_SPELL), AFF_CLAN_SAVE_SPELL);
				    IND_SHOP_POWER(ch) += 70000;//70000
				    send_to_char("�� ������� ����� &c��������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_SAVE_PARA))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_PARA), AFF_CLAN_SAVE_PARA);
				    IND_SHOP_POWER(ch) += 65000;//65000
				    send_to_char("�� ������� ����� &c��������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "�������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_SAVE_ROD))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_ROD), AFF_CLAN_SAVE_ROD);
				    IND_SHOP_POWER(ch) += 70000;//70000
				    send_to_char("�� ������� ����� &c�������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "���������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_SAVE_BASIC))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_BASIC), AFF_CLAN_SAVE_BASIC);
				    IND_SHOP_POWER(ch) += 75000;//75000
				    send_to_char("�� ������ ����� &c���������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_WIS))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_WIS), AFF_CLAN_WIS);
				    IND_SHOP_POWER(ch) += 135000;//135000
				    send_to_char("�� ������� ����� &c��������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "���������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_INT))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_INT), AFF_CLAN_INT);
				    IND_SHOP_POWER(ch) += 135000;//135000
				    send_to_char("�� ������� ����� &c���������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "��������"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_DEX))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_DEX), AFF_CLAN_DEX);
				    IND_SHOP_POWER(ch) += 120000;//120000
				    send_to_char("�� ������� ����� &c��������&n.\r\n", ch);	
			}

			else
				if(isname(argument, "����"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_STR))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_STR), AFF_CLAN_STR);
				    IND_SHOP_POWER(ch) += 120000;//120000
				    send_to_char("�� ������� ����� &c����&n.\r\n", ch);	
			}

			else
				if(isname(argument, "����"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_CON))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_CON), AFF_CLAN_CON);
				    IND_SHOP_POWER(ch) += 125000;//(125000)
				    send_to_char("�� ������� ����� &c����&n.\r\n", ch);	
			}

			else
				if(isname(argument, "�����"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_LCK))
					{ send_to_char("� ��� ��� ������ ������!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_LCK), AFF_CLAN_LCK);
				    IND_SHOP_POWER(ch) += 115000;//115000
				    send_to_char("�� ������� ����� &c�����&n.\r\n", ch);	
			}

	save_char(ch, NOWHERE);	
	return true;
  }
return false;
}


//������ ���������� ������ ������ � ������ �++, ��� ������ ����,
//�� �������� ���� ���������, ���� ���� �� ���� �������, ����
//����� �� ��� ��� ���� -(.



//����������� ������� ����������� �������� �� ������� �������� �� ������.
//�� ������� ������, �������� ������ �������� �����, �� ������� �������� ���� ���, ���� ����� ������
//��� �� �� ���� �������� � ���������� ���������. �������� ��� ������� ��������� �������,
//�� ���� �������� �������� 1,5,7,10	 ��� �� �� ������, �������� ����, �����, ���.





int Crash_is_unrentable(struct obj_data * obj);
//class WareHouse;
//��� ������� ����������� �������� ������� �� ������
//�.�. ����� �������� ���������� ����� ��� �������
//������, ����� ������ ����� ����� ��������� ���
//���������, ��� �� ����� �� ������.

void WareHouse::GetObj( CHAR_DATA *ch, char *argument )
{ int rnum = 0, j = 0, number, index =0;
  

		if(m_chest.empty())
		{ send_to_char("��� ����� ����!\r\n", ch);
		  return;
		}

		Whouse::iterator p;

	skip_spaces(&argument);

		if (!str_cmp(argument, "���"))
		{	p = m_chest.begin();
			send_to_char("�� ������� �� ������, ������� ����������� ������, ��������� ��������:\r\n", ch);
			while (p != m_chest.end())
			{  act("$o", FALSE, ch, *p, 0, TO_CHAR);
			    (*p)->chest = false;
				 obj_to_char(*p, ch);
				   p++;
			}
			//�� ����� ���� clear() ���� ����� ����� ��� erase(m_chest.begin(), m_chest.end())
			m_chest.clear();
		    return;
		}

		char tmpname[MAX_INPUT_LENGTH];
        char *tmp = tmpname;
		strcpy(tmp, argument);

		if (is_number(tmp))
        {
            int size = GET_CHEST(ch).m_chest.size();
		    if (size >= (index = atoi(tmp)))
            {
                for ( p = m_chest.begin(); p != m_chest.end(); ++p)
			    { 	if (--index == 0)
				    { act("�� ������� �� ������ $3.", FALSE, ch, *p, 0, TO_CHAR);
		                (*p)->chest = false;
					    obj_to_char(*p, ch);
						    m_chest.erase(p);
						    return;
				    }
		        }
		    }
		    else
		    { send_to_char("��� ������ ������!\r\n", ch);
		         return;
		    }
        }
		
		if (!(number = get_number(&tmp)))
				return;

		for ( p = m_chest.begin(); p != m_chest.end() && (j <= number); ++p)
		{ if (isname(tmp, (*p)->short_description))
				if (++j == number)
				{act("�� ������� �� ������ $3.", FALSE, ch, *p, 0, TO_CHAR);
				   (*p)->chest = false;
				    obj_to_char(*p, ch);
					  m_chest.erase(p);
					return;
				}
		}
        send_to_char("� ��� �� ������ ��� ������ ��������!\r\n", ch);
}

const int MAXCOUNTCHEST = 100; //�����������-���������� ����� �������� ��������� �� ������.

void WareHouse::TransferObj( CHAR_DATA *ch, char *argument )
{ int rnum = 0, j = 0, number, index =0;
  
 CHAR_DATA *vict;

		if(m_chest.empty())
		{ send_to_char("��� ����� ����!\r\n", ch);
		  return;
		}

		Whouse::iterator p;

		argument = one_argument(argument, arg);

		if (!*argument)
		{ send_to_char("��� �� ������ ���������?\r\n", ch);
			return;
		}

		skip_spaces(&argument);

		if ((vict = get_player_of_name(ch, arg))) {

			if (vict==ch) {
				send_to_char("� ����� ����� ����?\r\n", ch);
				return;
			}
			
		  if ( GET_CHEST(vict).m_chest.size() >= MAXCOUNTCHEST )
			{ send_to_char ("�� ������ ���������� ��� ��������� ����.\r\n", ch);
			  return;
			}  

			if (!str_cmp(argument, "���"))
			{	
				if ( GET_CHEST(vict).m_chest.size() + m_chest.size()  >= MAXCOUNTCHEST )
					{ send_to_char ("�� ������ ���������� ��������� ��������� ����.\r\n", ch);
					  return;
					}  

				p = m_chest.begin();
				send_to_char("&W�� ��������� �� ������, ������� ����������� ������, ��������� ��������:&n\r\n", ch);
				send_to_char("&W�� �������� �� �����, ��������� ��������:&n\r\n", vict);
				while (p != m_chest.end())
				{  
					act("$o", FALSE, ch, *p, 0, TO_CHAR);
					act("$o", FALSE, vict, *p, 0, TO_CHAR);
					GET_CHEST(vict).m_chest.push_back(*p);
					p++;
				}
				//�� ����� ���� clear() ���� ����� ����� ��� erase(m_chest.begin(), m_chest.end())
				m_chest.clear();
					return;
			}

			char tmpname[MAX_INPUT_LENGTH];
			char *tmp = tmpname;
			strcpy(tmp, argument);

			if (is_number(tmp))
            {
                int size = GET_CHEST(ch).m_chest.size();
			    if ( size >= (index = atoi(tmp)) )
			     { for ( p = m_chest.begin(); p != m_chest.end(); ++p)
				    { 	if (--index == 0)
					    { 
						    act("&W�� ��������� �������� $3.&n", FALSE, ch, *p, 0, TO_CHAR);
						    act("&W�� �������� �� ����� $3.&n", FALSE, vict, *p, 0, TO_CHAR);
							    GET_CHEST(vict).m_chest.push_back(*p);
							    m_chest.erase(p);
							    return;
					    }
				    }
			    }
			    else
			    { send_to_char("��� ������ ������!\r\n", ch);
			      return;
			    }
            }
			
			if (!(number = get_number(&tmp)))
					return;

			for ( p = m_chest.begin(); p != m_chest.end() && (j <= number); ++p)
			{ if (isname(tmp, (*p)->short_description))
					if (++j == number)
					{act("&W�� ��������� �������� $3.&n", FALSE, ch, *p, 0, TO_CHAR);
					 act("&W�� �������� �� ����� $3.&n", FALSE, vict, *p, 0, TO_CHAR);
						  GET_CHEST(vict).m_chest.push_back(*p);
						  m_chest.erase(p);
						return;
					}
			}
			send_to_char("� ��� �� ������ ��� ������ ��������!\r\n", ch);

		}
}

//����� ����������� ������� ����������� �������� �������� �� ������� (�������),
//��������, �� ����, (�����, ������, ����, ������ � �.�.)
void WareHouse::ListObj(CHAR_DATA *ch)
{	
	if(m_chest.empty())
	{ send_to_char("�� ����� ������ �����!\r\n", ch);
	   return;
	}

	Whouse::iterator p;

	int i = 0;
	*buf = '\0';
	
	send_to_char("� ��� �� ������ �������� ��������� ��������:\r\n", ch);

	//p - ����������� �������������� (*p)-> ����� �� ����� �������. �� ����� ������������ ���.
	for (int i = 0, e = m_chest.size(); i < e; ++i)
		sprintf(buf + strlen(buf), "%2d. %s.\r\n", i+1, m_chest[i]->short_description);


	page_string(ch->desc, buf, TRUE);
}

void WareHouse::PushObj( OBJ_DATA *obj )
{ 
	obj->chest = true;
	m_chest.push_back(obj);
}

int WareHouse::FindObj(const char* name)
{
    int j = 0, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;

    strcpy(tmp, name);
    if (!(number = get_number(&tmp)))
        return -1;

    for (int i = 0,e=m_chest.size(); i<e; ++i)
    {
        OBJ_DATA *obj = m_chest[i];
        if (isname(tmp, obj->name))          
	        if (++j == number)
	            return i;
    }
  return -1;   
}

void WareHouse::RemoveObj(int index)
{
    int size = m_chest.size();
    if (index >= 0 && index < size)
        m_chest.erase(m_chest.begin() + index);
}

//������� �� ������ �� �������
//��� ���� ��������� �� ����� ������� ������ ����������, ���� ����������
//���� �� ���� ��� ��������, ����� �� ����� ������ ���� ���������,
//� �� ���� �� ��� ������, � �� ������ �� �� ��� �������,
//� ������ ������� ��� � �� ����, ��� �������� �� ��� �������?
//����� �������� ����������� ��������� �����?
//����� ������� ������ � ����������� � ����������,
//� ����� ������� ���� ����� ������ ������� � ���� �����.

SPECIAL(WHouse)
{
  OBJ_DATA *obj, *next;
  int  rnum = 0, i = 0;
  bool found = false;

	if (!ch->desc || IS_NPC(ch))
			return false;

    *buf = '\0';

	CHAR_DATA *WHouse = (CHAR_DATA *) me;
	
	skip_spaces(&argument);        

	 if(CMD_IS("������"))
	 { GET_CHEST(ch).ListObj( ch );
		 return true;
	 }
	 else
	 if(CMD_IS("�����"))
	  {	 if (!ch->carrying)
			{ send_to_char("� ��� ������ ���!\r\n", ch);
		      return true; 
			}		
		  if (!*argument)
			{ send_to_char("��� �� ������ �����?\r\n", ch);
				return true;
			}

		  if ( GET_CHEST(ch).m_chest.size() >= MAXCOUNTCHEST )
			{ send_to_char ("�� ����� ������ ��� ��������� ����.\r\n", ch);
			  return true;
			}       

			if (!str_cmp(argument, "���"))
			{ for (obj = ch->carrying; obj;  obj = next)
				{ next = obj->next_content;
					if (obj->contains)
					{ sprintf(buf1, "$n ������$y ��� : \"� �� ����� �� ����� %s � ����������, �������� ��� �������������!\"", VOBJS(obj, ch));
					  act(buf1, FALSE, WHouse, 0, ch, TO_VICT);
						continue;
					}
					if (Crash_is_unrentable(obj))
					{ sprintf(buf1, "$n ������$y ��� : \"� �� ���� ������� �� ����� %s.\"", VOBJS(obj, ch));
					  act(buf1, FALSE, WHouse, 0, ch, TO_VICT);
					  continue;
					}
				 found = true;

				  if ( GET_CHEST(ch).m_chest.size() >= MAXCOUNTCHEST )
					{ send_to_char ("�� ����� ������ ��������� ���� �� ��������.\r\n", ch);
					   break;
					} 

				 obj_from_char(obj);
				 GET_CHEST(ch).PushObj( obj );
				 sprintf(buf + strlen(buf), "%2d. %s.\r\n", GET_CHEST(ch).m_chest.size(), obj->short_vdescription);
				}//end for
			
			if (found)
				{ send_to_char("�� ����� �� ����� ��������� ��������:\r\n", ch);
				  send_to_char(buf, ch);
				}
			  return true;
			}
			if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying)))
				{  send_to_char("� ���� �� ������ ����� �� ��������?\r\n",ch);
	    		   act("$n ����������� ����������$u!", FALSE, WHouse, 0, 0, TO_ROOM);
				   return true;
				}
			if (Crash_is_unrentable(obj))
				{ sprintf(buf, "$n ������$y ���: \"� �� ���� ������� �� ����� %s.\"", VOBJS(obj, ch));
				  act(buf, FALSE, WHouse, 0, ch, TO_VICT);
				  return true;
				}
			if (obj->contains)
				{ sprintf(buf, "$n ������$y ���: \"� �� ����� �� ����� %s � ����������, �������� ��� �������������!\"", VOBJS(obj, ch));
				  act(buf, FALSE, WHouse, 0, ch, TO_VICT);
				  return true;
				}
			
			act("�� ����� $3 �� �����.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n ����$y $3 �� �����.", FALSE, ch, obj, 0, TO_ROOM);
			GET_CHEST(ch).PushObj( obj );
			obj_from_char(obj);
		 return true;
		}
     else
	 if(CMD_IS("��������"))
	  {	  if (!*argument)
			{ send_to_char("��� �� ������ ��������?\r\n", ch);
				return true;
			}

	      GET_CHEST(ch).GetObj( ch, argument );
		  return true;
	  }else
 	 if(CMD_IS("���������"))
	  {	  if (!*argument)
			{ send_to_char("���� �� ������ ���������?\r\n", ch);
				return true;
			}
	      GET_CHEST(ch).TransferObj( ch, argument );
		  return true;
	  }
return false;
}

