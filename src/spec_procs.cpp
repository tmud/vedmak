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
    return " (\\c14не изучено\\c00)"; 
  if (percent <= 7) 
    return " (\\c01ужасно\\c00)";
  if (percent <= 14)  
    return " (\\c08очень плохо\\c00)"; 
  if (percent <= 21)
    return " (\\c08плохо\\c00)";
  if (percent <= 28) 
    return " (\\c03ниже среднего\\c00)";
  if (percent <= 35) 
    return " (\\c10среднее\\c00)";
  if (percent <= 42) 
    return " (\\c10выше среднего\\c00)"; 
  if (percent <= 49)  
    return " (\\c06хорошо\\c00)";
  if (percent <= 56) 
   return " (\\c06очень хорошо\\c00)";
  if (percent <= 63)
    return " (\\c06отлично\\c00)";
  if (percent <= 70)
    return " (\\c13великолепно\\c00)";
  if (percent <= 77)
    return " (\\c13превосходно\\c00)";
  if (percent <= 84)
    return " (\\c13идеально\\c00)";
  if (percent <= 91)
    return " (\\c13совершенно\\c00)";
  if (percent <= 100)
    return " (\\c14героически\\c00)";
  if (percent <= 110)
    return " (\\c14героически\\c00)";
  if (percent <= 135)
    return " (\\c14божественно\\c00)";
  if (percent <= 170)
    return " (\\c14божественно\\c00)";
    
  return " (\\c14ПРОЧИТЕНО!!!\\c00)"; 
} */


const char *how_good(int percent)
{
  if (percent < 0) 
   return " ОШИБКА";  
if (percent == 0)
    return " &nне изучено&n";  
if (percent <= 7)  
  return " &Rнеумел$w&n";  
if (percent <= 14) 
   return " &Rнеопытн$w&n";  
if (percent <= 21)
    return " &rнеспособн$w&n";
  if (percent <= 28)
    return " &rспособн$w&n";  
if (percent <= 35)
    return " &yпродвинут$w&n";  
if (percent <= 42)
    return " &yискусн$w&n";  
if (percent <= 49)
    return " &Yматер$w&n";  
if (percent <= 56) 
   return " &Yпрофессиональн$w&n";
  if (percent <= 63)
    return " &gмастерски&n";
  if (percent <= 70)
    return " &gвиртуозн$w&n";
  if (percent <= 76)
    return " &Gотлично&n";
  if (percent <= 83)
    return " &Gидеально&n";
  if (percent <= 91)
    return " &Wсовершенно&n";
  if (percent <= 100)
    return " &Wгероически&n";
  if (percent <= 110)
    return " &Wгероически&n";
  if (percent <= 135)
    return " \\c14легендарно\\c00";
  if (percent <= 170)
    return " \\c14божественно\\c00";
    
  return " \\c14ПРОЧИТЕНО!!!\\c00";}



const char *prac_types[] = {
  "заклинания",
  "умения"
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

  const char *teg = "следующими умениями/оружием";
  if (mode == 1)
      teg = "следующими умениями";
  if (mode == 2)
      teg = "следующим оружием";

  sprintf(buf, "&n%s владеет%s %s:&n\r\n", check ? GET_NAME(ch): "Вы", check ? "": "е", teg);
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
        sprintf(buf2 + strlen(buf2),"Нет умений.\r\n");
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
    {gcount = sprintf(buf2+gcount,"&nВы владеете следующей магией:&n");
	 for (i = 0; i < max_slot; i++)
         {if (slots[i] != 0) gcount += sprintf(buf2+gcount,"\r\n&gКруг %d&n:",i+1);
          if (slots[i])
             gcount += sprintf(buf2+gcount,"%s",names[i]);
         }
    }
 else
    gcount += sprintf(buf2+gcount,"В настоящее время магия Вам недоступна!");
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
	  
	  

  if (IS_NPC(ch) || !CMD_IS("учить"))  {
  if (IS_NPC(ch) || !CMD_IS("тренировать"))    return (0);   else {
          skip_spaces(&argument);
             if (!*argument) {
                   send_to_char("Что Вы хотите тренировать?\r\n", ch);
                   return (1);
			 }
          
			 if(!GET_CRBONUS(ch)) {
			send_to_char("У Вас нет тренировочных единиц.\r\n", ch);	 
				 return (1);
			 }
     		if(isname(argument, "сила")) {
				 if ((GET_STR_ROLL(ch) + 5) <= GET_STR(ch))
					{ send_to_char("Вы больше не можете тренировать силу!\r\n", ch);
					  return (1);
					}
				  GET_STR(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, STR)++;
		          send_to_char("Вы повысили свою силу на единицу.\r\n", ch);
			 }
			if(isname(argument, "тело")) {
				if ((GET_CON_ROLL(ch) + 5) <= GET_CON(ch))
					{ send_to_char("Вы больше не можете тренировать тело!\r\n", ch);
                      return (1);
					}
				  GET_MAX_HIT(ch)++;
				  GET_CON(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, CON)++;
                  send_to_char("Вы повысили свое тело на единицу.\r\n", ch);
			 }
			if(isname(argument, "интеллект")) {
				  if ((GET_INT_ROLL(ch) + 5) <= GET_INT(ch))
					{ send_to_char("Вы больше не можете тренировать интеллект!\r\n", ch);
                      return (1);
				  }
                  		  GET_INT(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, INT)++;
             	  send_to_char("Вы повысили свой интеллект на единицу.\r\n", ch);
			 }
			if(isname(argument, "мудрость")) {
				  if ((GET_WIS_ROLL(ch) + 5) <= GET_WIS(ch))
					{ send_to_char("Вы больше не можете тренировать мудрость!\r\n", ch);
                      return (1);
					}
                  		  GET_WIS(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, WIS)++;
              send_to_char("Вы повысили свою мудрость на единицу.\r\n", ch);
			 }
			if(isname(argument, "ловкость")) {
				  if ((GET_DEX_ROLL(ch) + 5) <= GET_DEX(ch))
					{ send_to_char("Вы больше не можете тренировать ловкость!\r\n", ch);
                      return (1);
					}
				  GET_DEX(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, DEX)++;
			  send_to_char("Вы повысили свою ловкость на единицу.\r\n", ch);
			 }
			if(isname(argument,"удача")) {
				  if ((GET_CHA_ROLL(ch) + 5) <= GET_CHA(ch))
					{ send_to_char("Вы больше не можете тренировать удачу!\r\n", ch);
                      return (1);
					}
                  		  GET_CHA(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, CHA)++;
              send_to_char("Вы повысили свою удачу на единицу.\r\n", ch);
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
		{ send_to_char("Вы можете обучиться следующим умениям:\r\n", ch);
		  send_to_char(buf, ch);
		
		}
	   else
         send_to_char("Вы не готовы учиться дальше.\r\n", ch);	   
	   return (1);  } 
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		{ send_to_char("Вы и так все знаете что Вам надо!\r\n", ch);
			return (1);
		}
	
	if (!strcmp(argument, "все")) {
		i = 0;
	  	  for (skill_num = 0; skill_num <= MAX_SKILLS; skill_num++)
			  			  
	if (GET_LEVEL(ch) >= skill_info[skill_num].k_improove[(int) GET_CLASS(ch)] && skill_info[skill_num].name != "!UNUSED!") 
		{ if (!skill_info[skill_num].name || *skill_info[skill_num].name == '!')
                 continue;
		 if (GET_PRACTICES(ch, skill_num)) 
			{	i = 1; 
		   if (GET_SKILL(ch, skill_num) > LEARNED(ch))
			{ sprintf(buf, "Я не могу Вас учить в области '&W%s&n', ищите другого Гильдмастера!\r\n", skill_info[skill_num].name);	 
				send_to_char(buf, ch);
					continue;
			}
			sprintf(buf, "Вы немного поучились в области: '&c%s&n'\r\n", 
					  skill_info[skill_num].name);
						send_to_char(buf, ch);
					percent = GET_SKILL(ch, skill_num);
				percent += 7;
			SET_SKILL(ch, skill_num, MIN(100, percent));
		GET_PRACTICES(ch, skill_num) = 0;

			}
		}
 if (!i)
   send_to_char("Все что можно Вы изучили!\r\n", ch);
return(1);
	}

  /* ЗДесь также потребуется переписывать код для заклинаний*/
	
	skill_num = find_skill_num(argument);
    if (skill_num < 1 ||      GET_LEVEL(ch) < skill_info[skill_num].k_improove[(int) GET_CLASS(ch)])
 {    sprintf(buf, "Вы не знаете этого %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return (1); 
 }	
         if (GET_SKILL(ch, skill_num) > LEARNED(ch))
		{ sprintf(buf, "Я не могу Вас учить в области &W%s&n, ищите другого Гильдмастера.\r\n", skill_info[skill_num].name);	 
			send_to_char(buf, ch);
				return (1);
		}
    if (GET_PRACTICES(ch, skill_num))
{    	percent = GET_SKILL(ch, skill_num);
		percent += 7;
		sprintf(buf, "Вы немного поучились в области: %s\r\n", skill_info[skill_num].name);
		SET_SKILL(ch, skill_num, MIN(100, percent));
        GET_PRACTICES(ch, skill_num) = 0;
	send_to_char(buf, ch);
  }
	else 
   send_to_char("Вы сейчас не можете учиться.\r\n", ch);

	if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("Вы обучены в этой области.\r\n", ch);
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
	  
	  

  if (IS_NPC(ch) || !CMD_IS("учить"))  {
  if (IS_NPC(ch) || !CMD_IS("тренировать"))
    return (0);
   else {
          skip_spaces(&argument);
             if (!*argument) {
                   send_to_char("Что Вы хотите тренировать?\r\n", ch);
                   return (1);
			 }
          	 if(!GET_CRBONUS(ch)) {
			send_to_char("У Вас нет тренировочных единиц.\r\n", ch);	 
				 return (1);
			 }
     		if(isname(argument, "сила")) {
				 if ((GET_STR_ROLL(ch) + 5) <= GET_STR(ch))
					{ send_to_char("Вы больше не можете тренировать силу!\r\n", ch);
					  return (1);
					}
				  GET_STR(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, STR)++;
		          send_to_char("Вы повысили свою силу на единицу.\r\n", ch);
			 }
			if(isname(argument, "тело")) {
				if ((GET_CON_ROLL(ch) + 5) <= GET_CON(ch))
					{ send_to_char("Вы больше не можете тренировать тело!\r\n", ch);
                      return (1);
					}
				  GET_MAX_HIT(ch)++;
				  GET_CON(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, CON)++;
                  send_to_char("Вы повысили свое тело на единицу.\r\n", ch);
			 }
			if(isname(argument, "интеллект")) {
				  if ((GET_INT_ROLL(ch) + 5) <= GET_INT(ch))
					{ send_to_char("Вы больше не можете тренировать интеллект!\r\n", ch);
                      return (1);
				  }
                  		  GET_INT(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, INT)++;
             	  send_to_char("Вы повысили свой интеллект на единицу.\r\n", ch);
			 }
			if(isname(argument, "мудрость")) {
				  if ((GET_WIS_ROLL(ch) + 5) <= GET_WIS(ch))
					{ send_to_char("Вы больше не можете тренировать мудрость!\r\n", ch);
                      return (1);
					}
                  		  GET_WIS(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, WIS)++;
              send_to_char("Вы повысили свою мудрость на единицу.\r\n", ch);
			 }
			if(isname(argument, "ловкость")) {
				  if ((GET_DEX_ROLL(ch) + 5) <= GET_DEX(ch))
					{ send_to_char("Вы больше не можете тренировать ловкость!\r\n", ch);
                      return (1);
					}
				  GET_DEX(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, DEX)++;
			  send_to_char("Вы повысили свою ловкость на единицу.\r\n", ch);
			 }
			if(isname(argument,"удача")) {
				  if ((GET_CHA_ROLL(ch) + 5) <= GET_CHA(ch))
					{ send_to_char("Вы больше не можете тренировать удача!\r\n", ch);
                      return (1);
					}
                  		  GET_CHA(ch)++;
				  GET_CRBONUS(ch)--;
				  GET_POINT_STAT(ch, CHA)++;
              send_to_char("Вы повысили свою удачу на единицу.\r\n", ch);
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
		{ send_to_char("Вы можете обучиться следующим умениям:\r\n", ch);
		  send_to_char(buf, ch);
		
		}
	   else
         send_to_char("Вы не готовы учиться дальше.\r\n", ch);
    return (1);
  }
   
 
 
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		{ send_to_char("Вы и так все знаете что Вам надо!\r\n", ch);
			return (1);
		}
	
	if (!strcmp(argument, "все")) {
		i = 0;
	  	  for (skill_num = 0; skill_num <= MAX_SKILLS; skill_num++)
			  		  
	if (GET_LEVEL(ch) >= skill_info[skill_num].k_improove[(int) GET_CLASS(ch)] && skill_info[skill_num].name != "!UNUSED!") 
		{     if (!skill_info[skill_num].name || *skill_info[skill_num].name == '!')
                 continue;
		    if (GET_PRACTICES(ch, skill_num)) 
			{	i = 1; 
		   if (GET_SKILL(ch, skill_num) <= LEARNED(ch))
			{ sprintf(buf, "Я не могу Вас учить в области &W%s&n, Вы не достаточно для этого подготовлены!.\r\n", skill_info[skill_num].name);	 
				send_to_char(buf, ch);
					continue;
			}
			if (GET_SKILL(ch, skill_num) > 100)
		{ sprintf(buf, "Я не могу Вас учить в области &W%s&n, ищите другого Гильдмастера.\r\n", skill_info[skill_num].name);	 
			send_to_char(buf, ch);
				continue;
		}
		   sprintf(buf, "Вы немного поучились в области: &c%s&n\r\n", 
					  skill_info[skill_num].name);
						send_to_char(buf, ch);
					percent = GET_SKILL(ch, skill_num);
				percent += 3;
			SET_SKILL(ch, skill_num, MIN(100, percent));
		GET_PRACTICES(ch, skill_num) = 0;

			}
		}
 if (!i)
   send_to_char("Все что можно Вы изучили!\r\n", ch);
return(1);
	}

  /* ЗДесь также потребуется переписывать код для заклинаний*/

	
	skill_num = find_skill_num(argument);

    if (skill_num < 1 ||
      GET_LEVEL(ch) < skill_info[skill_num].k_improove[(int) GET_CLASS(ch)]) {
    sprintf(buf, "Вы не знаете этого %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return (1);
  }
	if (GET_SKILL(ch, skill_num) <= LEARNED(ch))
		{ sprintf(buf, "Я не могу Вас учить в области &W%s&n, Вы не достаточно подготовлены для этого!.\r\n", skill_info[skill_num].name);	 
			send_to_char(buf, ch);
				return (1);
		}
  if (GET_SKILL(ch, skill_num) > 100)
		{ sprintf(buf, "Я не могу Вас учить в области &W%s&n, ищите другого Гильдмастера.\r\n", skill_info[skill_num].name);	 
			send_to_char(buf, ch);
				return (1);
		}
  if (GET_PRACTICES(ch, skill_num)){
    	percent = GET_SKILL(ch, skill_num);
		percent += 7;
		sprintf(buf, "Вы немного поучились в области: %s\r\n", skill_info[skill_num].name);
		SET_SKILL(ch, skill_num, MIN(100, percent));
        GET_PRACTICES(ch, skill_num) = 0;
	send_to_char(buf, ch);
  }
	else send_to_char("Вы сейчас не можете учиться.\r\n", ch);

//	if (GET_SKILL(ch, skill_num) >= 100	);//LEARNED(ch))
 //   send_to_char("Вы обучены в этой области.\r\n", ch);

  return (1);
}


SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$o исчезает в клубах дыма!", FALSE, 0, k, 0, TO_ROOM); /*vanishes in a puff of smoke!*/
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (0);

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$o исчезает в клубах дыма!", FALSE, 0, k, 0, TO_ROOM); /*vanishes in a puff of smoke*/
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
     {act("Вы обнаружили руку $r в своем кармане.", FALSE, ch, 0, victim, TO_VICT);
      act("$n пытал$u обокрасть $V.", TRUE, ch, 0, victim, TO_NOTVICT);
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
    act("$n ужалил$y $V!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n ужалил$y Вас!", 1, ch, 0, FIGHTING(ch), TO_VICT);
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
  const char *buf = "Охранник оттолкнул Вас, преграждая Вам путь.\r\n";
  const char *buf2 = "Охранник оттолкнул$y $d, преграждая $s путь.\r\n";

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
    do_say(ch, "Мой Бог! Здесь полно звезд!", 0, 0); 
    return (1);
  case 1:
    do_say(ch, "Вы тоже забыли про игру и стоите здесь?", 0, 0); 
    return (1);
  case 2:
    do_say(ch, "Я очень большой дракон.", 0, 0); 
    return (1);
  case 3:
    do_say(ch, "Я очень добрый дракон.", 0, 0);
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
      act("$n жестоко поедает труп.", FALSE, ch, 0, 0, TO_ROOM); /*savagely devours a corpse*/
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
	act("$n убирает здесь мусор.", FALSE, ch, 0, 0, TO_ROOM); /*picks up some trash*/
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }

  return (FALSE);
}

int has_curse(struct obj_data *obj)
{ int i;
	for (i = 0; weapon_affect[i].aff_bitvector >= 0; i++) {
		// Замена выражения на макрос
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
          act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,where),0,TO_ROOM);
          obj_to_char(unequip_char(ch,where),ch);
         }
      act("$n одел$y $3.",FALSE,ch,obj,0,TO_ROOM);
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
             !has_curse(obj) && //- магическое оружие, надо будет подключить
             (GET_OBJ_TYPE(obj) == ITEM_ARMOR ||
              GET_OBJ_TYPE(obj) == ITEM_WEAPON))
            {obj_from_room(obj);
             obj_to_char(obj, ch);
             act("$n поднял$y $3.", FALSE, ch, obj, 0, TO_ROOM);
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
        {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
        }
     if (GET_EQ(ch,WEAR_WIELD))
        {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_WIELD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_WIELD), ch);
        }
     if (GET_EQ(ch,WEAR_SHIELD))
        {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_SHIELD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_SHIELD),ch);
        }
     if (GET_EQ(ch,WEAR_HOLD))
        {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_HOLD),0,TO_ROOM);
         obj_to_char(unequip_char(ch,WEAR_HOLD),ch);
        }
     act("$n взял$y $3 в обе руки.",FALSE,ch,both,0,TO_ROOM);
     obj_from_char(both);
     equip_char(ch,both,WEAR_BOTHS);
    }
 else
    {if (left && GET_EQ(ch,WEAR_HOLD) != left)
        {if (GET_EQ(ch,WEAR_BOTHS))
            {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
            }
         if (GET_EQ(ch,WEAR_SHIELD))
            {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_SHIELD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_SHIELD),ch);
            }
         if (GET_EQ(ch,WEAR_HOLD))
            {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_HOLD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_HOLD),ch);
            }
         act("$n взял$y $3 в левую руку.",FALSE,ch,left,0,TO_ROOM);
         obj_from_char(left);
         equip_char(ch,left,WEAR_HOLD);
        }
     if (right && GET_EQ(ch,WEAR_WIELD) != right)
        {if (GET_EQ(ch,WEAR_BOTHS))
            {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_BOTHS),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_BOTHS), ch);
            }
         if (GET_EQ(ch,WEAR_WIELD))
            {act("$n прекратил$y использовать $3.",FALSE,ch,GET_EQ(ch,WEAR_WIELD),0,TO_ROOM);
             obj_to_char(unequip_char(ch,WEAR_WIELD), ch);
            }
         act("$n взял$y $3 в правую руку.",FALSE,ch,right,0,TO_ROOM);
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
     (GET_OBJ_VAL(obj,0) == 0 || !IS_DARK(IN_ROOM(ch))))//было 2, заменил на 0 - уровень света в часах редактора
    {act("$n прекратил$y использовать $3.",FALSE,ch,obj,0,TO_ROOM);
     obj_to_char(unequip_char(ch,WEAR_LIGHT),ch);
    }

 if (!GET_EQ(ch,WEAR_LIGHT) && IS_DARK(IN_ROOM(ch)))
    for (obj = ch->carrying; obj; obj = next)
        {next = obj->next_content;
         if (GET_OBJ_TYPE(obj) != ITEM_LIGHT)
             continue;
         if (GET_OBJ_VAL(obj,0) == 0)// так же заменил
            {act("$n выбросил$y $3.",FALSE,ch,obj,0,TO_ROOM);
             obj_from_char(obj);
             obj_to_room(obj,IN_ROOM(ch));
             continue;
            }
         act("$n начал$y использовать $3.",FALSE,ch,obj,0,TO_ROOM);
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
      act("$n заорал$y...'ЭЙ $n!!! ТЫ нарушил$y ЗАКОН!! ТЫ БУДЕШЬ Убит$y!!!'", FALSE, ch, 0, 0, TO_ROOM);
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
    act("$n заорал$y 'ЗА ЗАЩИТУ ДОБРА!  ВПЕРЕД!  СМЕРТЬ!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED); 
    return (TRUE);
  }*/
  return (FALSE);
}

#define HORSE_PRICE(pet) (GET_LEVEL(pet) * 30) 
#define HORSE_ROOM		  8098 // комната, где должны стоять скакуны

SPECIAL(horse_shops)
{ int i= 1;
  char buf[MAX_STRING_LENGTH], pet_name[256];
  room_rnum pet_room;
  struct char_data *victim = (struct char_data *) me, *pet = NULL;

  if (IS_NPC(ch))
     return (0);

  pet_room = real_room(HORSE_ROOM);

  if (CMD_IS("список") || CMD_IS("list")) {
    act("&C\"У меня в наличии есть следующие скакуны\", сказал$Y Вам $N&n", FALSE, ch, 0, victim, TO_CHAR);
	send_to_char("+----------------------------------------------------------------+\r\n", ch);
	send_to_char("| Цена     Имя                                                   |\r\n", ch);
	send_to_char("+----------------------------------------------------------------+\r\n", ch);
	for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
    sprintf(buf, "%-4d     %s\r\n", HORSE_PRICE(pet), GET_NAME(pet));
      send_to_char(buf, ch);
        
	}
    return (TRUE);
  } else 
	  if (CMD_IS("купить")|| CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      act("&Y\"У меня нет такого скакуна\"&n, сказал$Y Вам $N", FALSE, ch, 0, victim, TO_CHAR);
      return (TRUE);
    }
   
    if (has_horse(ch,FALSE))
		{act("$N застонал$Y : \"$n, зачем тебе второй скакун?.\"",
            FALSE,ch,0,victim,TO_CHAR);
       return (TRUE);
		}
	
	if (GET_GOLD(ch) < HORSE_PRICE(pet)) {
     act("&Y\"У Вас нет столько денег\"&n, сказал$Y Вам $N",FALSE,ch,0,victim,TO_CHAR);
	    return (TRUE);
    }
    
    if(!(pet = read_mobile(GET_MOB_RNUM(pet), REAL)))
		{	act("\"В настоящее время у меня нет скакуна для Вас.\"- потупив глаза, произнес$Q $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      make_horse(pet,ch);
      char_to_room(pet,IN_ROOM(ch));
      sprintf(buf,"$N оседлал$Y %s и передал$Y %s Вам.",GET_VNAME(pet),HSHR(pet));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N оседлал$Y %s и передал$Y %s $d.",GET_VNAME(pet),HSHR(pet));
      act(buf,FALSE,ch,0,victim,TO_ROOM);
      GET_GOLD(ch) -= HORSE_PRICE(pet);
      SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
      return (TRUE);
     }
	else 
	  if (CMD_IS("продать")|| CMD_IS("sell")) 
	  {if (!has_horse(ch,TRUE) && !*argument)
         {act("$N сказал$U : \"$n, Вам же будет в конюшне неудобно, подумайте!\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (on_horse(ch))
         {act("\"Вы что решили себя продать вместе с скакуном?.\"- ехидно улыбнул$U $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (!(pet = get_horse(ch)))
         {act("\"Мне такие скакуны и даром не нужны!.\"- заорал$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (IN_ROOM(pet) != IN_ROOM(victim))
         {act("\"Вы сначала найдите скакуна, прежде чем его продавать.\"- сказал$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      extract_char(pet,FALSE);
      sprintf(buf,"$N снял$Y седло с %s и отвел$Y %s в конюшню.",GET_VNAME(pet) ,HSHR(pet));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N снял$Y седло с %s и отвел$Y %s в конюшню.",GET_VNAME(pet), HSHR(pet));
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

  if (!CMD_IS("лошадь") && !CMD_IS("horse"))
     return (0);


  skip_spaces(&argument);

  if (!*argument)
     {if (has_horse(ch,FALSE))
         {act("$N заинтересовал$U : \"$n,и зачем Вам второй скакун?\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      sprintf(buf,"$N сказал$Y : \"Скакун Вам обойдется %d %s.\"",
              HORSE_COST, desc_count(HORSE_COST,WHAT_MONEYa));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      return (TRUE);
     }

  if (!strn_cmp(argument, "купить", strlen(argument)) ||
      !strn_cmp(argument, "buy", strlen(argument)))
     {if (has_horse(ch,FALSE))
         {act("$N улыбнул$U : \"$n, Вы сразу на двух скакунах скакунах будете?\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (GET_GOLD(ch) < HORSE_COST)
         {act("\"У Вас нет таких денег!\"- закричал$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (!(horse = read_mobile(HORSE_VNUM,VIRTUAL)))
         {act("\"Извините, но в данный момент у меня нет скакунов.\"- сказал$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      make_horse(horse,ch);
      char_to_room(horse,IN_ROOM(ch));
      sprintf(buf,"$N оседлал$Y %s и передал$Y %s Вам.",GET_VNAME(horse),HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N оседлал$Y %s и передал$Y %s $d.",GET_VNAME(horse),HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_ROOM);
      GET_GOLD(ch) -= HORSE_COST;
      SET_BIT(PLR_FLAGS(ch, PLR_CRASH), PLR_CRASH);
      return (TRUE);
     }


  if (!strn_cmp(argument, "продать", strlen(argument)) ||
      !strn_cmp(argument, "sell", strlen(argument)))
     {if (!has_horse(ch,TRUE))
         {act("$N сказал$U : \"$n, Вам же будет там неудобно, подумайте!\"",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }
      if (on_horse(ch))
         {act("\"Вы что решили себя сдать вместе с скакуном?.\"- ехидно улыбнул$U $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (!(horse = get_horse(ch)) || GET_MOB_VNUM(horse) != HORSE_VNUM )
         {act("\"Мне такие скакуны и даром не нужны!.\"- заорал$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      if (IN_ROOM(horse) != IN_ROOM(victim))
         {act("\"Вы сначала найдите скакуна, прежде чем его сдать.\"- сказал$Y $N",
              FALSE,ch,0,victim,TO_CHAR);
          return (TRUE);
         }

      extract_char(horse,FALSE);
      sprintf(buf,"$N снял$Y седло с %s и отвел$Y %s в конюшню.",GET_VNAME(horse) ,HSHR(horse));
      act(buf,FALSE,ch,0,victim,TO_CHAR);
      sprintf(buf,"$N снял$Y седло с %s и отвел$Y %s в конюшню.",GET_VNAME(horse), HSHR(horse));
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
                         act("$n начал$y внимательно выискивать чьи-то следы.",FALSE,ch,0,0,TO_ROOM);
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
                         act("$n начал$y внимательно искать чьи-то следы.",FALSE,ch,0,0,TO_ROOM);
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
 case ITEM_LIGHT  : if (GET_OBJ_VAL(obj,0) == 0) // изменяю с двойки на 0 - это в
                       return (TRUE);			// в редакторе количество вермени света
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
         {act("$n выбросил$y $3.",FALSE,ch,obj,0,FALSE);
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
             act("$n поднял$y $3.", FALSE, ch, best_obj, 0, TO_ROOM);
            }
         else
            {obj_from_obj(best_obj);
             obj_to_char(best_obj, ch);
             sprintf(buf,"$n достал$y $3 из %s.", cont->short_rdescription);
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
                            {sprintf(buf,"$n вытащил$y $3 из %s.",obj->short_rdescription);
                             act(buf,FALSE,ch,cobj,0,TO_ROOM);
                             obj_from_obj(cobj);
                             obj_to_char(cobj,ch);
                             max++;
                            }
                        }
                   }
                else
                if (CAN_GET_OBJ(ch, loot_obj) && !item_nouse(loot_obj))
		{ sprintf(buf,"$n вытащил$y $3 из %s.",obj->short_rdescription);
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
          act("$n вступил$u за $V.",FALSE,helper,0,ch,TO_ROOM);
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

  if (CMD_IS("список") || CMD_IS("list")) {
    send_to_char("В наличии:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("купить")|| CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("Такого домашнего животного нет!\r\n", ch); 
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char("Вы не имеете столько золотых монет!\r\n", ch);
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

      sprintf(buf, "%sA small sign on a chain around the neck says 'Меня зовут %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("Вы можете наслаждаться своим новым другом.\r\n", ch);
    act("$n купил$y $V, как домашнее животное.", FALSE, ch, 0, pet, TO_ROOM);

    return (1);
  }
  /* All commands except list and buy */
  return (0);
}

// Автоквестер

SPECIAL(questmaster)
{
  struct char_data *questinfo = NULL;
  struct char_data *questman = (struct char_data*)me;
  int count = 0;
  //GET_QUESTGIVER(ch) = questman->player.short_descr;

	if (CMD_IS("получить"))
	{  if (IS_QUESTOR(ch))
		{ sprintf(buf, "Ровный голос произнес: У вас уже есть задание, о котором вы не доложили.");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
				return(1);
		}
		if (GET_NEXTQUEST(ch) > 0)
		{  sprintf(buf, "Ровный голос произнес: Вы слишком разгорячились, %s, отдохните!",GET_NAME(ch));
                act(buf, FALSE, questman, 0, 0, TO_ROOM);
		   sprintf(buf, "Ровный голос произнес: Зайдите попозже.");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
			    return(1);
		}
           if (GET_LEVEL(ch) <= 5)
		{ send_to_char("Ровный голос произнес: Рановато Вам еще за ратные дела браться!\r\n", ch);
	      return (1);
		}
	       generate_quest(ch, questman);
        if (GET_QUESTMOB(ch) > 0 || GET_QUESTOBJ(ch) > 0)
			{ GET_COUNTDOWN(ch) = number(20,40);
			  SET_BIT(PLR_FLAGS(ch, PLR_QUESTOR), PLR_QUESTOR);
		          sprintf(buf, " &gУ Вас есть &W%d %s&g для выполнения этого задания.&n",
			  GET_COUNTDOWN(ch), desc_count(GET_COUNTDOWN(ch),WHAT_MINa));
                          act(buf, FALSE, questman, 0, ch, TO_VICT);
			  sprintf(buf, "&gБольшое спасибо, %s, желаю тебе удачи!",GET_NAME(ch));
                          act(buf, FALSE, questman, 0, ch, TO_VICT);
			  sprintf(buf, "&gНатан Вам в помощь!&n");
                          act(buf, FALSE, questman, 0, ch, TO_VICT);
			
			}
return(1);
	}
			if (CMD_IS("отменить"))
			{  	if (IS_QUESTOR(ch))
				{ if (GET_QUESTMOB(ch))
				   { questinfo = get_mob_by_id(GET_QUESTMOB(ch));
				      if (questinfo)
					GET_QUESTOR(questinfo) = 0;
				   }
				  else
				   { if (GET_QUESTOK(ch))
					act("Ровный голос произнес: Вы не можете отменить задание, которое уже выполнили!\r\n Попробуйте лучше доложить!", FALSE, questman, 0, ch, TO_VICT);
					else 
					act("&KКак оказалось, это задание кто-то уже выполнил.&n", FALSE, questman, 0, ch, TO_VICT);
					return(1);
				   }
				sprintf(buf, "Ровный голос произнес: Эх, очень плохо, а я на тебя надеялся!&n\r\n Хорошо, я отменяю твое задание.");
                		act(buf, FALSE, questman, 0, ch, TO_VICT);
			        REMOVE_BIT(PLR_FLAGS(ch, PLR_QUESTOR), PLR_QUESTOR);
		      		 // GET_QUESTGIVER(ch) = NULL;
                		GET_COUNTDOWN(ch) = 0;
                		GET_QUESTMOB(ch) = 0;
                		GET_NEXTQUEST(ch) = number(3, 6);//дописано 9.11.02г.
                		sprintf(buf, "&GВы можете получить другое задание через &W%d минут&G.&n", GET_NEXTQUEST(ch));
                		act(buf, FALSE, questman, 0, ch, TO_VICT);
				return(1);
				}
				
				sprintf(buf, "А у Вас и не было задания!");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
				return(1);
			}

			if (CMD_IS("доложить"))
			{ if (GET_COUNTDOWN(ch) <=0)
				{act("Ровный голос произнес: О чем Вы хотите доложить?", FALSE, questman, 0, ch, TO_VICT);
					return(1);
				}
			 if (!GET_QUESTOK(ch) && !GET_QUESTMOB(ch))	
				{act("Ровный голос произнес: Не хорошо чужие заслуги себе присваивать!", FALSE, questman, 0, ch, TO_VICT);
				return(1);	
				}
			 if (!GET_QUESTOK(ch) && GET_QUESTMOB(ch))
				{act("Ровный голос произнес: Вы еще задание не выполнили, как Вам не стыдно хитрить!", FALSE, questman, 0, ch, TO_VICT);
				return(1);	
				}
			 if (GET_QUESTOK(ch) && !GET_QUESTMOB(ch))	
				{ act("Ровный голос произнес: Ну что же, я вознаграждаю Вас!", FALSE, questman, 0, ch, TO_VICT);
				GET_GLORY(ch) += GET_QUESTMAX(ch);
			   // GET_GLORY(ch) += count;
				sprintf (buf, "&GЗа выполнение этого задания Вы получили %d %s славы!&n",
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
  sprintf(buf, "Ровный голос произнес: Извините, но в данный момент у меня для Вас нет задания.");
  act(buf, FALSE, questman, 0, ch, TO_VICT);
  sprintf(buf, "Зайдите попозже.");
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
        sprintf(buf, "Ровный голос произнес:\r\n &gДошли слухи что, %s агрессивно себя ведет!&n", GET_NAME(victim));
                act(buf, FALSE, questman, 0, ch, TO_VICT);
        sprintf(buf, " &gНе место таким в нашем Мире.&n\r\n&g Миссия по уничтожению поручается Вам!&n");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
				break;
	 case 1:
    act("Ровный голос произнес:\r\n &gКриминальный авторитет $n, сбежал$y из тюрьмы и нападает на моих друзей!&n",
					FALSE, victim, 0, ch, TO_VICT);
	sprintf(buf, " &gЗа время побега, %s убил$y и ограбил$y %d моих друзей!&n\r\n  &gСтупайте же и выполните мое задание!&n",
			   GET_NAME(victim), number(2,20));
                act(buf, FALSE, victim, 0, ch, TO_VICT);
				break;
    }
	   break;

  case CLASS_ANIMAL:
     switch(number(0,1))
    {
	case 0:
        sprintf(buf, "Ровный голос произнес:\r\n &gВ последнее время участились случаи грабежей урожая.&n\r\n"
			         " &gКак докладывают странники, вина за происходящее лежит на&n\r\n"
					 " &g%s, если Вы хотите прославиться, пойдите и убейте $s!&n", GET_PNAME(victim));
                      act(buf, FALSE, victim, 0, ch, TO_VICT);
        break;
    case 1:
        sprintf(buf,"Ровный голос произнес:\r\n &gМестные жители стали жаловаться на частые случаи нападения.&n\r\n"
					" &gВиной этому явил$u %s.&n\r\n &gЧтобы не допустить дальнейших случаев нападений,&n "
					"$s необходимо истребить!&n", GET_NAME(victim));
		 act(buf,FALSE, victim, 0, ch, TO_VICT);
		break;
    }
 break;

   case CLASS_DRAGON:
     switch(number(0,1))
    {
	case 0:
        act("Ровный голос произнес:\r\n &gСлишком часто за выкупом стал$y прилетать $n,&n\r\n"
			" &g$s необходимо отучить от такой затеи, возми сколько нужно&n\r\n"
			" &gбойцов и убейте эту наглую тварь! Правители впишут Ваше имя&n\r\n"
			" &gв историю (посмертно).&n", FALSE, victim, 0, ch, TO_VICT);
    break;
	 case 1:
    act("Ровный голос произнес:\r\n &gУжасное чудовище стало воровать детей, женщин и стариков. Один из моих друзей&n\r\n"
		" &gзаподозрил в этом злодеянии $v, ему надо отомстить - отрубить голову!&n",
		FALSE, victim, 0, ch, TO_VICT);
	break;
    }
	   break;

   case CLASS_UNDEAD:
     switch(number(0,1))
    {
	case 0:
        act("Ровный голос произнес:\r\n &gВ городе жалуются, что по ночам&n\r\n"
			" &gкто-то постоянно шумит и не дает спокойно отдыхать после тяжелого&n\r\n"
			" &gрабочего дня. Один из рабочих заметил $v, необходимо как можно&n\r\n"
			" &gбыстрее решить эту проблему!&n",
			FALSE, victim, 0, ch, TO_VICT);
        break;
	 case 1:
    act("Ровный голос произнес:\r\n &gВ наших краях завел$u $n и причиняет огромный ущерб.&n\r\n"
		" &gИзничтожте это существо и возвращайтесь скорей!&n", FALSE, victim, 0, ch, TO_VICT);
	break;
    }
	   break;
	 
	 default:  
    switch(number(0,1))
    {
	case 0:
        sprintf(buf, "Ровный голос произнес:\r\n &gВы слышали, %s, агрессивно ведет себя?!&n", GET_NAME(victim));
                act(buf, FALSE, questman, 0, ch, TO_VICT);
        sprintf(buf, " &gЭта угроза должна быть уничтожена!&n");
                act(buf, FALSE, questman, 0, ch, TO_VICT);
				break;
	 case 1:
    act("Ровный голос произнес:\r\n &gКриминальный авторитет $n, сбежал$y из тюрьмы и нападает на моих друзей!&n",
					FALSE, victim, 0, ch, TO_VICT);
	sprintf(buf, " &gЗа время побега, %s убил$y и ограбил$y %d жителей!&n",
			   GET_NAME(victim), number(2,20));
                act(buf, FALSE, victim, 0, ch, TO_VICT);
				break;
    }
   
   }

 }
   else
       send_to_char(victim->mob_specials.Questor, ch); 
       if (world[victim->in_room].name != NULL)
	     { sprintf(buf, "&gМесто, где скрывается &W%s&g:\r\n &Wзона '%s'&g, &Wв клетке: %s&n",
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
		 { send_to_char("&GВы снова можете получить задание!&n\r\n",ch);
                    return;
		 }
	  }
    else 
    if (IS_QUESTOR(ch))
        { if (--GET_COUNTDOWN(ch) <= 0)
			{	GET_NEXTQUEST(ch) = number(3, 6);
				sprintf(buf, "&KВы не успели выполнить задание.&n\r\n&GСледующее задание Вы можете получить через %d %s.&n\r\n",
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
			{  send_to_char("&gПоторопитесь! Для выполнения задания осталось мало времени!&n\r\n",ch);
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

  if (CMD_IS("баланс")||CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "На Вашем счету &c%ld&n %s.\r\n",
	      GET_BANK_GOLD(ch), desc_count(GET_BANK_GOLD(ch),WHAT_MONEYa)); 
    else
      sprintf(buf, "В настоящее время на Вашем счету ничего нет.\r\n"); 
    send_to_char(buf, ch);
    return (1);
  } else if (CMD_IS("вложить")||CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("Сколько Вы хотите вложить монет?\r\n", ch);
      return (1);
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char("У вас нет столько наличных денег!\r\n", ch); 
      return (1);
    }
    GET_GOLD(ch) -= amount;
    sbor = amount/20;
	if (sbor <1)
	    sbor = 1;
   if (GET_LEVEL(ch) <= 10)
         sbor = 0;
    GET_BANK_GOLD(ch) += (amount - sbor);
    sprintf(buf, "Вы вложили %d %s.\r\nЗа услуги, банк удержал %d %s.\r\n", amount,
		          desc_count(amount,WHAT_MONEYu), sbor, desc_count(sbor,WHAT_MONEYu)); 
    send_to_char(buf, ch);
    act("$n произвел$y финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM); 
    return (1);
  } else if (CMD_IS("снять")||CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("Сколько Вы хотите снять монет?\r\n", ch);
      return (1);
    }
     if (GET_BANK_GOLD(ch) < amount) {
      send_to_char("Вам и не снилось столько монет на Вашем счету!\r\n", ch); 
      return (1);
    }
    GET_BANK_GOLD(ch) -= amount;
    GET_GOLD(ch) += amount;
    sprintf(buf, "Вы сняли со своего счета %d %s.\r\n", amount, desc_count(amount,WHAT_MONEYu)); 
    send_to_char(buf, ch);
    act("$n произвел$y финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (1);
  } else
	  if (CMD_IS("transfer") || CMD_IS("перевести"))
	  { argument = one_argument(argument, arg);
		amount = atoi(argument);
		if (IS_GOD(ch) && !IS_IMPL(ch))
		{ send_to_char("Ай Я ЯЙ, читимс ГОСПОДА?\r\n", ch);
			return (1);
		}
		if (!*arg)
		{ send_to_char("Кому Вы хотите сделать денежный перевод?\r\n", ch);
			return (1);
		}
		if (amount <= 0)
		{ send_to_char("Какое количество денег Вы хотите перевести?\r\n", ch);
			return (1);
		}
		if (amount <= 100)
		{ send_to_char("Сумма денежного перевода не должна быть менее 100 монет.\r\n", ch);
			return (1);
		}

		if (GET_BANK_GOLD(ch) < amount) {
			send_to_char("У Вас нет такой суммы для перевода.\r\n", ch);
			return (1);
		}
		if (GET_BANK_GOLD(ch) < amount + ((amount * 5) / 100)) {
			send_to_char("Вам нечем заплатить за оказанную услугу перевода!\r\n", ch);
			return (1);
		}
		if ((vict = get_player_of_name(ch, arg))) {
			GET_BANK_GOLD(ch) -= amount + (amount * 5) / 100;
			sprintf(buf, "&WВы перевели %d %s на банковский счет %s.&n\r\n", amount, desc_count(amount,WHAT_MONEYu), GET_DNAME(vict) );
			send_to_char(buf, ch);
			GET_BANK_GOLD(vict) += amount;
			sprintf(buf, "&WВы получили банковский перевод на %d %s от %s.&n\r\n", amount, desc_count(amount,WHAT_MONEYu), GET_RNAME(ch));
			send_to_char(buf, vict);
			return (1);
		} else {
			CREATE(vict, CHAR_DATA, 1);
			clear_char(vict);
			if (load_char(arg, vict) < 0) {
				send_to_char("Такого персонажа не существует в Мире.\r\n", ch);
				free(vict);
				return (1);
			}
			GET_BANK_GOLD(ch) -= amount + (amount * 5) / 100;
			sprintf(buf, "&WВы перевели %d %s на банковский счет %s.&n\r\n", amount, desc_count(amount,WHAT_MONEYu), GET_DNAME(vict));
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
/*    МАГАЗИН МОГУЩЕСТВА		  		       */
/***************************************************************/

SPECIAL(power_stat)
{
   int coins = 1000;
    
  if  (CMD_IS("список"))
  { 
	  send_to_char ("В магазине могущества Вы можете улучшить следующие характеристики.\r\n", ch);
  send_to_char("Что бы улучшить характеристики за славу необходимо написать &C\"тренировать\"&n характеристика.\r\n", ch);
  send_to_char("Что бы улучшить характеристики за ОИМ необходимо написать &C\"купить\"&n характеристика.\r\n", ch);
  send_to_char("**********************************************************************\r\n", ch);
  send_to_char("| &Cхарактеристики                 стоимость&n                           |\r\n", ch);
  send_to_char("**********************************************************************\r\n", ch);
   sprintf(buf,"| &WСИЛА                        - %5d очков славы или 35000 ОИМ&n     |\r\n", coins);            
sprintf(buf+strlen(buf),"| &WТЕЛО                        - %5d очков славы или 35000 ОИМ&n     |\r\n", coins);
sprintf(buf+strlen(buf),"| &WЛОВКОСТЬ                    - %5d очков славы или 35000 ОИМ&n     |\r\n", coins);	  
sprintf(buf+strlen(buf),"| &WИНТЕЛЛЕКТ                   - %5d очков славы или 35000 ОИМ&n     |\r\n", coins);
sprintf(buf+strlen(buf),"| &WМУДРОСТЬ                    - %5d очков славы или 35000 ОИМ&n     |\r\n", coins);
sprintf(buf+strlen(buf),"| &WУДАЧА                       - %5d очков славы или 35000 ОИМ&n     |\r\n", coins);
   send_to_char(buf, ch);
   send_to_char("**********************************************************************\r\n", ch);
  return (1);
  }
  
  if (IS_NPC(ch) || !CMD_IS("купить"))  {
  if (IS_NPC(ch) || !CMD_IS("тренировать"))
    return (0);
    else {
          skip_spaces(&argument);
           
		  if (!*argument)
		  {  send_to_char("Какую характеристику Вы хотите тренировать?\r\n", ch);
                   return (1);
		  }		  
		       if(GET_GLORY(ch) < coins)
				{ send_to_char("У Вас нет необходимого количества славы для тренировки!\r\n", ch);	 
				 return (1);
				}

     		if(isname(argument, "сила"))
			{	 if ((GET_POINT_STAT(ch, STR) + 5 + GET_STR_ROLL(ch)) <= GET_STR(ch))
					{ send_to_char("Вы больше не можете тренировать силу!\r\n", ch);
                       return (1);
					}
				     GET_STR(ch)++;
				     GET_GLORY(ch) -= coins;
				     GET_QUESTPOINTS(ch) +=coins;
				     send_to_char("Вы повысили свою силу на единицу.\r\n", ch);
			}
			if(isname(argument, "тело")) {
				  if ((GET_POINT_STAT(ch, CON) + 5 + GET_CON_ROLL(ch)) <= GET_CON(ch))
					 { send_to_char("Вы больше не можете тренировать тело!\r\n", ch);
                       return (1);
					 }
				   GET_CON(ch)++;
				   GET_MAX_HIT(ch) += GET_CON(ch);
				   GET_GLORY(ch) -= coins;
				   GET_QUESTPOINTS(ch) += coins;
			       send_to_char("Вы повысили свое тело на единицу.\r\n", ch);
				}
			if(isname(argument, "интеллект")) {
                  if ((GET_POINT_STAT(ch, INT) + 5 + GET_INT_ROLL(ch)) <= GET_INT(ch))
					 { send_to_char("Вы больше не можете тренировать интеллект!\r\n", ch);
                       return (1);
					 }
				  GET_INT(ch)++;
				  GET_GLORY(ch) -= coins;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("Вы повысили свой интеллект на единицу.\r\n", ch);
			 }
			if(isname(argument, "мудрость")) {
                 if ((GET_POINT_STAT(ch, WIS) + 5 + GET_WIS_ROLL(ch)) <= GET_WIS(ch))
					 { send_to_char("Вы больше не можете тренировать мудрость!\r\n", ch);
                       return (1);
					 }
				 GET_WIS(ch)++;
				 GET_GLORY(ch) -= coins;
				 GET_QUESTPOINTS(ch) += coins;
				 if (IS_WEDMAK(ch))
                 GET_MAX_MANA(ch) += GET_WIS(ch);
			  send_to_char("Вы повысили свою мудрость на единицу.\r\n", ch);
			 }
			if(isname(argument, "ловкость")) {
				  if ((GET_POINT_STAT(ch, DEX) + 5 + GET_DEX_ROLL(ch)) <= GET_DEX(ch))
					 { send_to_char("Вы больше не можете тренировать ловкость!\r\n", ch);
                       return (1);
					 }
				  GET_DEX(ch)++;
				  GET_GLORY(ch) -= coins;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("Вы повысили свою ловкость на единицу.\r\n", ch);
			 }
			if(isname(argument, "удача")) {
                  if ((GET_POINT_STAT(ch, CHA) + 5 + GET_CHA_ROLL(ch)) <= GET_CHA(ch))
					 { send_to_char("Вы больше не можете тренировать удачу!\r\n", ch);
                       return (1);
					 }
				  GET_CHA(ch)++;
				  GET_GLORY(ch) -= coins;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("Вы повысили свою удачу на единицу.\r\n", ch);
			 }
	save_char(ch, NOWHERE);	
          
  
	}
	  return (1);
  }

  skip_spaces(&argument);
   if (!*argument)
		  {  send_to_char("Какую характеристику Вы хотите тренировать?\r\n", ch);
                   return (1);
		  }		  
		       if(IND_POWER_CHAR(ch) < 35000)
				{ send_to_char("У Вас нет необходимого количества &cОИМ&n для тренировок!\r\n", ch); 
				 return (1);
				}

     		if(isname(argument, "сила"))
			{	 if ((GET_POINT_STAT(ch, STR) + 5 + GET_STR_ROLL(ch)) <= GET_STR(ch))
					{ send_to_char("Вы больше не можете тренировать силу!\r\n", ch);
                       return (1);
					}
				     GET_STR(ch)++;
				     IND_POWER_CHAR(ch) -= 35000;
					 GET_QUESTPOINTS(ch) += coins;
				     send_to_char("Вы повысили свою силу на единицу.\r\n", ch);
			}
			if(isname(argument, "тело")) {
				  if ((GET_POINT_STAT(ch, CON) + 5 + GET_CON_ROLL(ch)) <= GET_CON(ch))
					 { send_to_char("Вы больше не можете тренировать тело!\r\n", ch);
                       return (1);
					 }
				   GET_CON(ch)++;
				   GET_MAX_HIT(ch) += GET_CON(ch);
				   IND_POWER_CHAR(ch) -= 35000;
				   GET_QUESTPOINTS(ch) += coins;
			       send_to_char("Вы повысили свое тело на единицу.\r\n", ch);
				}
			if(isname(argument, "интеллект")) {
                  if ((GET_POINT_STAT(ch, INT) + 5 + GET_INT_ROLL(ch)) <= GET_INT(ch))
					 { send_to_char("Вы больше не можете тренировать интеллект!\r\n", ch);
                       return (1);
					 }
				  GET_INT(ch)++;
				  IND_POWER_CHAR(ch) -= 35000;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("Вы повысили свой интеллект на единицу.\r\n", ch);
			 }
			if(isname(argument, "мудрость")) {
                 if ((GET_POINT_STAT(ch, WIS) + 5 + GET_WIS_ROLL(ch)) <= GET_WIS(ch))
					 { send_to_char("Вы больше не можете тренировать мудрость!\r\n", ch);
                       return (1);
					 }
				 GET_WIS(ch)++;
				 IND_POWER_CHAR(ch) -= 35000;
				 GET_QUESTPOINTS(ch) += coins;
				 if (IS_WEDMAK(ch))
                 GET_MAX_MANA(ch) += GET_WIS(ch);
			  send_to_char("Вы повысили свою мудрость на единицу.\r\n", ch);
			 }
			if(isname(argument, "ловкость")) {
				  if ((GET_POINT_STAT(ch, DEX) + 5 + GET_DEX_ROLL(ch)) <= GET_DEX(ch))
					 { send_to_char("Вы больше не можете тренировать ловкость!\r\n", ch);
                       return (1);
					 }
				  GET_DEX(ch)++;
				  IND_POWER_CHAR(ch) -= 35000;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("Вы повысили свою ловкость на единицу.\r\n", ch);
			 }
			if(isname(argument, "удача")) {
                  if ((GET_POINT_STAT(ch, CHA) + 5 + GET_CHA_ROLL(ch)) <= GET_CHA(ch))
					 { send_to_char("Вы больше не можете тренировать удачу!\r\n", ch);
                       return (1);
					 }
				  GET_CHA(ch)++;
				  IND_POWER_CHAR(ch) -= 35000;
				  GET_QUESTPOINTS(ch) += coins;
			  send_to_char("Вы повысили свою удачу на единицу.\r\n", ch);
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
    // Номер заклинания 	Название заклинания	   Цена
    { SPELL_ARMOR, 		"защита               ", 30 },
    { SPELL_FLY,		"полет                ", 15 },
    { SPELL_CURE_SERIOUS,	"серьёзное исцеление  ", 45 },
	{ SPELL_REMOVE_CURSE,	"снять проклятие      ", 150 },
    { SPELL_BLESS, 		"праведность          ", 15 },
    { SPELL_INVISIBLE,		"невидимость          ", 300 },
    { SPELL_REMOVE_POISON, 	"нейтрализовать яд    ", 90 },
    { SPELL_CURE_BLIND, 	"вылечить слепоту     ", 150 },
    { SPELL_SANCTUARY, 		"освящение            ", 30 },
    { SPELL_HEAL, 		"исцеление            ", 600 },
    { SPELL_REFRESH,            "восстановление       ", 15 },
    { SPELL_LIGHT,              "свет                 ", 10 },
    { SPELL_RESTORE_MAGIC,      "волшебство           ", 10000 },

    // Здесь можно добавлять дополнительные спеллы 
    { -1, "\r\n", -1 }
  };


  if (CMD_IS("получить")) {
    argument = one_argument(argument, buf);

    if (GET_POS(ch) == POS_FIGHTING) return TRUE;

    if (*buf) {
      for (i=0; prices[i].number > SPELL_RESERVED_DBC; i++) {
	if (is_abbrev(buf, prices[i].name)) 
	  if (GET_GOLD(ch) < prices[i].price) {
	    act("$n сказал Вам: \"У Вас нет столько денег!\"",
		FALSE, (struct char_data *) me, 0, ch, TO_VICT);
            return TRUE;
          } else {
	    
	    act("$N дал$Y за услуги несколько монет $d.",
		FALSE, (struct char_data *) me, 0, ch, TO_NOTVICT);
	    sprintf(buf, "Вы дали %s %d монет.\r\n", 
		    GET_DNAME((struct char_data *) me), prices[i].price);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) -= prices[i].price;
	     cast_spell((struct char_data *) me, ch, NULL, prices[i].number);
	    return TRUE;
          
	  }
      }
      act("$n сказал Вам: \"Я не владею такими заклинаниями!\"\r\n"
	  "  Напечатайте &Wполучить&n что бы узнать доступные заклинания.", FALSE, (struct char_data *) me, 
	  0, ch, TO_VICT);
	  
      return TRUE;
    } else {
      act("$n сказал Вам: \"Здесь список и цены на мои услуги, которыми Вы можете воспользоваться.\"",
	  FALSE, (struct char_data *) me, 0, ch, TO_VICT);
      for (i=0; prices[i].number > SPELL_RESERVED_DBC; i++) {
        sprintf(buf, "&W%s&n %-25d\r\n", prices[i].name, prices[i].price);
        send_to_char(buf, ch);
      }
      return TRUE;
    }
  }

  if (cmd) return FALSE;

  // Псевдорандом, выбираем персонажей в руме. 
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (!number(0, 4))
      break;
  
  // Проверяем, есть ли цель, какой уровень, что бы на моба не кастовалось 
  if (vict == NULL || IS_NPC(vict) || (GET_LEVEL(vict) > 10))
    return FALSE;

  act("Густое облако дыма, извергнутого чашей, начало принимать фантастические образы.", TRUE, ch, 0, vict, TO_ROOM);
  
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


//Загружаем экипировку от кладовщика
void LoadToCharObj(CHAR_DATA * ch, CHAR_DATA * vict, int vnum)
{ OBJ_DATA *obj;

 //проверяем наличие получения обьекта, иначе NULL
 if ((obj = read_object(vnum, VIRTUAL))) 
	{ obj_to_char(obj, ch);
	  act("$n дал$y Вам $3.", FALSE, vict, obj, ch, TO_VICT);
	}
}

//Функция проверки предмета у персонажа в экипировке и в инвентаре.
//Содержимое контейнеров не проверяем.
//Проверку ведем по виртуальному номеру предмета.
//Если нашли, возвращаем 0, если нет то 1.

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

//Таблица предметов, выдаваемых для различных классов.
//Первоя позиция, выстраиваем для класса обьекты, с учетом максимальных слотов
//по одеванию предметов, их 22. 
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
	 if (CMD_IS("получить"))
	 {	 ubyte i;
		 CHAR_DATA *kladov = (struct char_data *) me;
	     skip_spaces(&argument);        
	     if (FIGHTING(ch))
		    return TRUE;
		if (GET_LEVEL(ch) > 49)
		{ act("&C$n сказал Вам: \"Вы в состоянии сами о себе позаботиться.\"&n",
	           FALSE, (struct char_data *) me, 0, ch, TO_VICT);
			   return TRUE;
		}
	
	   if (!*argument)
		{ sprintf(buf, "&C$n сказал Вам: \"Вы можете:&n\r\n Получить припасы\r\n Получить одежда\r\n Получить оружие\r\n Получить свиток      (возврата, см ? свиток)\r\n Получить свет        (см ? свет)\r\n Получить противоядие (см ? напиток)\r\n Получить все         (вышеперечисленное)\r\n");
          act(buf, FALSE, (struct char_data *) me, 0, ch, TO_VICT);
           return TRUE;
	 	}
	     
	  act("&G$n сказал Вам: \"Сейчас посмотрим, найду ли я что для Вас.\"&n", FALSE, kladov, NULL, ch, TO_VICT);
	  act("$n начал$y шарить по стеллажам.", FALSE, kladov, NULL, ch, TO_ROOM);

	    if (!str_cmp(argument, "припасы") || !str_cmp(argument, "все") && GET_LEVEL(ch) < 17)
		{  if (!GetObjInChar(ch, 3639))
		    { LoadToCharObj(ch, kladov, 3639);
		      LoadToCharObj(ch, kladov, 3639);
		      LoadToCharObj(ch, kladov, 3639);
                      LoadToCharObj(ch, kladov, 1805);
			}
		  else
		    act("&G$n сказал Вам: \"Кусочек лембаса у Вас есть. Как закончится заходите.\"&n", FALSE, kladov, NULL, ch, TO_VICT); 

		   if (!GetObjInChar(ch, 102))
		       LoadToCharObj(ch, kladov, 102);
		   else
		      act("&G$n сказал Вам: \"Фляга у Вас есть, наполняйте ее из фонтана (восток; юг).\"&n", FALSE, kladov, NULL, ch, TO_VICT);	   
		}
	
	 if (!str_cmp(argument, "одежда") || !str_cmp(argument, "все"))
		{  for (i = 0; i < CLASSES_OBJ; i++)
			{ if (!ClassObj[GET_CLASS(ch)][i])
				    continue; 
	           if (!GetObjInChar(ch, ClassObj[GET_CLASS(ch)][i]))
			        LoadToCharObj(ch, kladov, ClassObj[GET_CLASS(ch)][i]);
			}
				   
			if(!GetObjInChar(ch, 1611))
			   LoadToCharObj(ch, kladov, 1611);//для всех на шею

			if(!GetObjInChar(ch, 1616))
		    { LoadToCharObj(ch, kladov, 1616);//браслет
			  LoadToCharObj(ch, kladov, 1616);//браслет
			}		
		}
     
	 if (!str_cmp(argument, "оружие") || !str_cmp(argument, "все"))
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
	  if (!str_cmp(argument, "противоядие") ||
		  !str_cmp(argument, "напиток")     ||
		  !str_cmp(argument, "все")
		  )
	  {		if (!GetObjInChar(ch, 138))
		       LoadToCharObj(ch, kladov, 138);
	  
	  }
	  if (!str_cmp(argument, "свет") || !str_cmp(argument, "все"))
	  {		if (!GetObjInChar(ch, 103))
		       LoadToCharObj(ch, kladov, 103);
	  
	  }
	  if (!str_cmp(argument, "свиток") || !str_cmp(argument, "все"))
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

	 if  (CMD_IS("список"))
  { 
	
  
  send_to_char("&W ----------------------------------------------------------------------------------- \r\n", ch);
 sprintf(buf, "/&c  ---&w***&c---                ---<<< &CЦена бонусов:&c  >>>---                 ---&w***&c---&W  \\\r\n");
 send_to_char(buf, ch); 
 
 send_to_char("|  ---&c^^^&W-------------------------------------------------------------------&c^^^&W---  |\r\n", ch);
  
send_to_char( "&W|&c\\&W|                                                                               &W|&c/&W|\r\n"
			  "|&w*&W|&c Хитпоинты  : &n20000  &cМораль     : &n35000  &cДамролы  : &n25000  &cХитролы    : &n45000  &W|&w*&W|\r\n"
			  "|&c/&W|&c Инициатива : &n5000   &cДвижение   : &n10000  &cХитрег   : &n65000  &cМуврег     : &n65000  &W|&c\\&W|\r\n"
			  "|&w*&W|&c Шанcкаст   : &n140000 &cЗаучивание : &n85000  &cЗащита   : &n50000  &cПоглощение : &n80000  &W|&w*&W|\r\n"
			  "|&c\\&W|&c Воля       : &n70000  &cСтойкость  : &n65000  &cРеакция  : &n70000  &cЗдоровье   : &n75000  &W|&c/&W|\r\n"
			  "|&w*&W|&c Мудрость   : &n135000 &cИнтеллект  : &n135000&c Ловкость : &n120000 &cСила       : &n120000 &W|&w*&W|\r\n"
		      "|&c\\&W|&c                     Тело       : &n125000 &cУдача    : &n115000                     &W|&c/&W|\r\n"
			  "|  \\_____________________________________________________________________________/  |\r\n"
              "| / ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^&c___&w***&c___&W^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \\ |\r\n"
			  "\\__________________________________________________________________________________/&n\r\n", ch);
  
  
  return (1);
}
 
	  if (CMD_IS("купить"))  
      	{ 
		  skip_spaces(&argument);

		if (!*argument)
		  {  send_to_char("Какой бонус Вы хотите купить?\r\n", ch);
                   return (1);
		  }	

     		if(isname(argument, "хитпоинты"))
			{	 if (IND_SHOP_POWER(ch) < 20000)
					{ send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cХитпоинты!&n\r\n", ch);
                       return true;
					}
				 if (PLR_FLAGGED(ch, AFF_CLAN_HITPOINT))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}
					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_HITPOINT), AFF_CLAN_HITPOINT);
				    IND_SHOP_POWER(ch) -= 20000;
				    send_to_char("Вы купили бонус &cХитпоинты&n.\r\n", ch);
			}

		   else
			   if(isname(argument, "мораль"))
			{	  if (IND_SHOP_POWER(ch) < 35000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cМораль!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_MORALE))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_MORALE), AFF_CLAN_MORALE);
				    IND_SHOP_POWER(ch) -= 35000;
				    send_to_char("Вы купили бонус &cМораль&n.\r\n", ch);	
				}

			else
				if(isname(argument, "дамролы"))
			{	  if (IND_SHOP_POWER(ch) < 25000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cДамролы!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_DAMROLL))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_DAMROLL), AFF_CLAN_DAMROLL);
				    IND_SHOP_POWER(ch) -= 25000;
				    send_to_char("Вы купили бонус &cДамролы&n.\r\n", ch);	
			}
			
			else
				if(isname(argument, "хитролы"))
			{	  if (IND_SHOP_POWER(ch) < 40000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cХитролы!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_HITROLL))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_HITROLL), AFF_CLAN_HITROLL);
				    IND_SHOP_POWER(ch) -= 40000;
				    send_to_char("Вы купили бонус &cХитролы&n.\r\n", ch);	
			}
			
			else
				if(isname(argument, "инициатива"))
			{	  if (IND_SHOP_POWER(ch) < 5000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cИнициатива!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_INITIATIVE))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_INITIATIVE), AFF_CLAN_INITIATIVE);
				    IND_SHOP_POWER(ch) -= 5000;
				    send_to_char("Вы купили бонус &cИнициатива&n.\r\n", ch);	
			}

			else
				if(isname(argument, "движение"))
			{	  if (IND_SHOP_POWER(ch) < 10000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cДвижение!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_MOVE))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_MOVE), AFF_CLAN_MOVE);
				    IND_SHOP_POWER(ch) -= 10000;
				    send_to_char("Вы купили бонус &cДвижение&n.\r\n", ch);	
			}

			else
				if(isname(argument, "хитрег"))
			{	  if (IND_SHOP_POWER(ch) < 65000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cХитрег!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_HITREG))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_HITREG), AFF_CLAN_HITREG);
				    IND_SHOP_POWER(ch) -= 65000;
				    send_to_char("Вы купили бонус &cХитрег&n.\r\n", ch);	
			}

			else
				if(isname(argument, "муврег"))
			{	  if (IND_SHOP_POWER(ch) < 65000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cМуврег!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_MOVEREG))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_MOVEREG), AFF_CLAN_MOVEREG);
				    IND_SHOP_POWER(ch) -= 65000;
				    send_to_char("Вы купили бонус &cМуврег&n.\r\n", ch);	
			}

			else
				if(isname(argument, "шанскаст"))
			{	  if (IND_SHOP_POWER(ch) < 140000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cШанкаст!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_CAST_SUCCES))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_CAST_SUCCES), AFF_CLAN_CAST_SUCCES);
				    IND_SHOP_POWER(ch) -= 140000;
				    send_to_char("Вы купили бонус &cШанкаст&n.\r\n", ch);	
			}

			else
				if(isname(argument, "заучиваине"))
			{	  if (IND_SHOP_POWER(ch) < 85000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cЗаучивание!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_MANAREG))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_MANAREG), AFF_CLAN_MANAREG);
				    IND_SHOP_POWER(ch) -= 85000;
				    send_to_char("Вы купили бонус &cЗаучивание&n.\r\n", ch);	
			}

			else
				if(isname(argument, "защита"))
			{	  if (IND_SHOP_POWER(ch) < 50000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cЗащита!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_AC))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_AC), AFF_CLAN_AC);
				    IND_SHOP_POWER(ch) -= 50000;
				    send_to_char("Вы купили бонус &cЗащита&n.\r\n", ch);	
			}

			else
				if(isname(argument, "поглощение"))
			{	  if (IND_SHOP_POWER(ch) < 80000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cПоглощение!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_ABSORBE))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_ABSORBE), AFF_CLAN_ABSORBE);
				    IND_SHOP_POWER(ch) -= 80000;
				    send_to_char("Вы купили бонус &cПоглощение&n.\r\n", ch);	
			}

			else
				if(isname(argument, "воля"))
			{	  if (IND_SHOP_POWER(ch) < 70000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cВоля!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_SPELL))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_SPELL), AFF_CLAN_SAVE_SPELL);
				    IND_SHOP_POWER(ch) -= 70000;
				    send_to_char("Вы купили бонус &cВоля&n.\r\n", ch);	
			}

			else
				if(isname(argument, "стойкость"))
			{	  if (IND_SHOP_POWER(ch) < 65000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cСтойкость!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_PARA))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_PARA), AFF_CLAN_SAVE_PARA);
				    IND_SHOP_POWER(ch) -= 65000;
				    send_to_char("Вы купили бонус &cСтойкость&n.\r\n", ch);	
			}

			else
				if(isname(argument, "реакция"))
			{	  if (IND_SHOP_POWER(ch) < 70000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cРеакция!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_ROD))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_ROD), AFF_CLAN_SAVE_ROD);
				    IND_SHOP_POWER(ch) -= 70000;
				    send_to_char("Вы купили бонус &cРеакция&n.\r\n", ch);	
			}

			else
				if(isname(argument, "здоровье"))
			{	  if (IND_SHOP_POWER(ch) < 75000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cЗдоровье!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_BASIC))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_BASIC), AFF_CLAN_SAVE_BASIC);
				    IND_SHOP_POWER(ch) -= 75000;
				    send_to_char("Вы купили бонус &cЗдоровье&n.\r\n", ch);	
			}

			else
				if(isname(argument, "мудрость"))
			{	  if (IND_SHOP_POWER(ch) < 135000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cМудрость!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_WIS))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_WIS), AFF_CLAN_WIS);
				    IND_SHOP_POWER(ch) -= 135000;
				    send_to_char("Вы купили бонус &cМудрость&n.\r\n", ch);	
			}

			else
				if(isname(argument, "интеллект"))
			{	  if (IND_SHOP_POWER(ch) < 135000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cИнтеллект!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_INT))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_INT), AFF_CLAN_INT);
				    IND_SHOP_POWER(ch) -= 135000;
				    send_to_char("Вы купили бонус &cИнтеллект&n.\r\n", ch);	
			}

			else
				if(isname(argument, "ловкость"))
			{	  if (IND_SHOP_POWER(ch) < 120000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cЛовкость!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_DEX))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_DEX), AFF_CLAN_DEX);
				    IND_SHOP_POWER(ch) -= 120000;
				    send_to_char("Вы купили бонус &cЛовкость&n.\r\n", ch);	
			}


			else
				if(isname(argument, "сила"))
			{	  if (IND_SHOP_POWER(ch) < 120000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cСила!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_STR))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_STR), AFF_CLAN_STR);
				    IND_SHOP_POWER(ch) -= 120000;
				    send_to_char("Вы купили бонус &cСила&n.\r\n", ch);	
			}


			else
				if(isname(argument, "тело"))
			{	  if (IND_SHOP_POWER(ch) < 125000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cТело!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_CON))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_CON), AFF_CLAN_CON);
				    IND_SHOP_POWER(ch) -= 125000;
				    send_to_char("Вы купили бонус &cТело&n.\r\n", ch);	
			}

			else
				if(isname(argument, "удача"))
			{	  if (IND_SHOP_POWER(ch) < 115000)		  
					 { send_to_char("У Вас нет необходимого количества &cПИМ&n для покупки бонуса &cУдача!&n\r\n", ch);
                       return true;
					 }
				  
				  if (PLR_FLAGGED(ch, AFF_CLAN_LCK))
					{ send_to_char("Этот бонус у Вас уже имеется!\r\n", ch);
					   return true;					
					}

					SET_BIT (PLR_FLAGS (ch, AFF_CLAN_LCK), AFF_CLAN_LCK);
				    IND_SHOP_POWER(ch) -= 115000;
				    send_to_char("Вы купили бонус &cУдача&n.\r\n", ch);	
			}

	save_char(ch, NOWHERE);	
return true;
  }


	
//Штраф за продажу, 1 процент от мощности, что бы не юзали туда сюда.
	if (CMD_IS("продать"))  
      	{ 
		  skip_spaces(&argument);

		if (!*argument)
		  {  send_to_char("Какой бонус Вы хотите продать?\r\n", ch);
                   return (1);
		  }	

     			if(isname(argument, "хитпоинты"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_HITPOINT))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}
					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_HITPOINT), AFF_CLAN_HITPOINT);
				    IND_SHOP_POWER(ch) += 20000;//20000
				    send_to_char("Вы продали бонус &cХитпоинты&n.\r\n", ch);
			}

			else
				if(isname(argument, "мораль"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_MORALE))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_MORALE), AFF_CLAN_MORALE);
				    IND_SHOP_POWER(ch) += 35000;//35000
				    send_to_char("Вы продали бонус &cМораль&n.\r\n", ch);	
				}


			else
				if(isname(argument, "дамролы"))
			{	   if (!PLR_FLAGGED(ch, AFF_CLAN_DAMROLL))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_DAMROLL), AFF_CLAN_DAMROLL);
				    IND_SHOP_POWER(ch) += 25000;//25000
				    send_to_char("Вы продали бонус &cДамролы&n.\r\n", ch);	
			}

			else
				if(isname(argument, "хитролы"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_HITROLL))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_HITROLL), AFF_CLAN_HITROLL);
				    IND_SHOP_POWER(ch) += 40000;//40000
				    send_to_char("Вы продали бонус &cХитролы&n.\r\n", ch);	
			}

			else
				if(isname(argument, "инициатива"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_INITIATIVE))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_INITIATIVE), AFF_CLAN_INITIATIVE);
				    IND_SHOP_POWER(ch) += 5000;//5000
				    send_to_char("Вы продали бонус &cИнициатива&n.\r\n", ch);	
			}
		
			else
				if(isname(argument, "движение"))
			{	   if (!PLR_FLAGGED(ch, AFF_CLAN_MOVE))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_MOVE), AFF_CLAN_MOVE);
				    IND_SHOP_POWER(ch) += 10000;//10000
				    send_to_char("Вы продали бонус &cДвижение&n.\r\n", ch);	
			}

			else
				if(isname(argument, "хитрег"))
			{	 if (!PLR_FLAGGED(ch, AFF_CLAN_HITREG))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_HITREG), AFF_CLAN_HITREG);
				    IND_SHOP_POWER(ch) += 65000;//65000
				    send_to_char("Вы продали бонус &cХитрег&n.\r\n", ch);	
			}

			else
				if(isname(argument, "муврег"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_MOVEREG))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_MOVEREG), AFF_CLAN_MOVEREG);
				    IND_SHOP_POWER(ch) += 65000;//65000
				    send_to_char("Вы продали бонус &cМуврег&n.\r\n", ch);	
			}

			else
				if(isname(argument, "шанскаст"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_CAST_SUCCES))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_CAST_SUCCES), AFF_CLAN_CAST_SUCCES);
				    IND_SHOP_POWER(ch) += 140000;//140000
				    send_to_char("Вы продали бонус &cШанкаст&n.\r\n", ch);	
			}

			else
				if(isname(argument, "заучиваине"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_MANAREG))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_MANAREG), AFF_CLAN_MANAREG);
				    IND_SHOP_POWER(ch) += 85000;//85000
				    send_to_char("Вы продали бонус &cЗаучивание&n.\r\n", ch);	
			}

			else
				if(isname(argument, "защита"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_AC))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_AC), AFF_CLAN_AC);
				    IND_SHOP_POWER(ch) += 50000;//50000
				    send_to_char("Вы продали бонус &cЗащита&n.\r\n", ch);	
			}

			else
				if(isname(argument, "поглощение"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_ABSORBE))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_ABSORBE), AFF_CLAN_ABSORBE);
				    IND_SHOP_POWER(ch) += 80000;//80000
				    send_to_char("Вы продали бонус &cПоглощение&n.\r\n", ch);	
			}

			else
				if(isname(argument, "савеспел"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_SAVE_SPELL))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_SPELL), AFF_CLAN_SAVE_SPELL);
				    IND_SHOP_POWER(ch) += 70000;//70000
				    send_to_char("Вы продали бонус &cСавеспел&n.\r\n", ch);	
			}

			else
				if(isname(argument, "савепара"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_SAVE_PARA))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_PARA), AFF_CLAN_SAVE_PARA);
				    IND_SHOP_POWER(ch) += 65000;//65000
				    send_to_char("Вы продали бонус &cСавепара&n.\r\n", ch);	
			}

			else
				if(isname(argument, "саверод"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_SAVE_ROD))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_ROD), AFF_CLAN_SAVE_ROD);
				    IND_SHOP_POWER(ch) += 70000;//70000
				    send_to_char("Вы продали бонус &cСаверод&n.\r\n", ch);	
			}

			else
				if(isname(argument, "савебазик"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_SAVE_BASIC))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_SAVE_BASIC), AFF_CLAN_SAVE_BASIC);
				    IND_SHOP_POWER(ch) += 75000;//75000
				    send_to_char("Вы купили бонус &cСавебазик&n.\r\n", ch);	
			}

			else
				if(isname(argument, "мудрость"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_WIS))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_WIS), AFF_CLAN_WIS);
				    IND_SHOP_POWER(ch) += 135000;//135000
				    send_to_char("Вы продали бонус &cМудрость&n.\r\n", ch);	
			}

			else
				if(isname(argument, "интеллект"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_INT))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_INT), AFF_CLAN_INT);
				    IND_SHOP_POWER(ch) += 135000;//135000
				    send_to_char("Вы продали бонус &cИнтеллект&n.\r\n", ch);	
			}

			else
				if(isname(argument, "ловкость"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_DEX))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_DEX), AFF_CLAN_DEX);
				    IND_SHOP_POWER(ch) += 120000;//120000
				    send_to_char("Вы продали бонус &cЛовкость&n.\r\n", ch);	
			}

			else
				if(isname(argument, "сила"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_STR))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_STR), AFF_CLAN_STR);
				    IND_SHOP_POWER(ch) += 120000;//120000
				    send_to_char("Вы продали бонус &cСила&n.\r\n", ch);	
			}

			else
				if(isname(argument, "тело"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_CON))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_CON), AFF_CLAN_CON);
				    IND_SHOP_POWER(ch) += 125000;//(125000)
				    send_to_char("Вы продали бонус &cТело&n.\r\n", ch);	
			}

			else
				if(isname(argument, "удача"))
			{	  if (!PLR_FLAGGED(ch, AFF_CLAN_LCK))
					{ send_to_char("У Вас нет такого бонуса!\r\n", ch);
					   return true;					
					}

					REMOVE_BIT (PLR_FLAGS (ch, AFF_CLAN_LCK), AFF_CLAN_LCK);
				    IND_SHOP_POWER(ch) += 115000;//115000
				    send_to_char("Вы продали бонус &cУдача&n.\r\n", ch);	
			}

	save_char(ch, NOWHERE);	
	return true;
  }
return false;
}


//Начало реализации работы склада в режиме С++, это первые шаги,
//за качество кода невзыщите, хотя надо за этим следить, ведь
//делаю то все для себя -(.



//Обязательно сделать возможность получать по номерам предметы со склада.
//по команде список, выдавать список хранимых вещей, по команде получить либо имя, либо номер строки
//что бы не было путаницы с получением предметов. возможно еще сделать получение списком,
//то есть написать получить 1,5,7,10	 или же по именам, получить шлем, копье, меч.





int Crash_is_unrentable(struct obj_data * obj);
//class WareHouse;
//тут сделать возможность получать предмет по номеру
//т.е. будет присвоин порядковый номер при команде
//список, через пробел нужно будет принимать имя
//предметов, что бы потом их выдать.

void WareHouse::GetObj( CHAR_DATA *ch, char *argument )
{ int rnum = 0, j = 0, number, index =0;
  

		if(m_chest.empty())
		{ send_to_char("Ваш склад пуст!\r\n", ch);
		  return;
		}

		Whouse::iterator p;

	skip_spaces(&argument);

		if (!str_cmp(argument, "все"))
		{	p = m_chest.begin();
			send_to_char("Вы забрали со склада, нажитые непосильным трудом, следующие предметы:\r\n", ch);
			while (p != m_chest.end())
			{  act("$o", FALSE, ch, *p, 0, TO_CHAR);
			    (*p)->chest = false;
				 obj_to_char(*p, ch);
				   p++;
			}
			//на самом деле clear() есть нечто инное как erase(m_chest.begin(), m_chest.end())
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
				    { act("Вы забрали со склада $3.", FALSE, ch, *p, 0, TO_CHAR);
		                (*p)->chest = false;
					    obj_to_char(*p, ch);
						    m_chest.erase(p);
						    return;
				    }
		        }
		    }
		    else
		    { send_to_char("Нет такого номера!\r\n", ch);
		         return;
		    }
        }
		
		if (!(number = get_number(&tmp)))
				return;

		for ( p = m_chest.begin(); p != m_chest.end() && (j <= number); ++p)
		{ if (isname(tmp, (*p)->short_description))
				if (++j == number)
				{act("Вы забрали со склада $3.", FALSE, ch, *p, 0, TO_CHAR);
				   (*p)->chest = false;
				    obj_to_char(*p, ch);
					  m_chest.erase(p);
					return;
				}
		}
        send_to_char("У Вас на складе нет такого предмета!\r\n", ch);
}

const int MAXCOUNTCHEST = 100; //максимально-допустимое число хранения предметов на складе.

void WareHouse::TransferObj( CHAR_DATA *ch, char *argument )
{ int rnum = 0, j = 0, number, index =0;
  
 CHAR_DATA *vict;

		if(m_chest.empty())
		{ send_to_char("Ваш склад пуст!\r\n", ch);
		  return;
		}

		Whouse::iterator p;

		argument = one_argument(argument, arg);

		if (!*argument)
		{ send_to_char("Что вы хотите перевести?\r\n", ch);
			return;
		}

		skip_spaces(&argument);

		if ((vict = get_player_of_name(ch, arg))) {

			if (vict==ch) {
				send_to_char("У врача давно были?\r\n", ch);
				return;
			}
			
		  if ( GET_CHEST(vict).m_chest.size() >= MAXCOUNTCHEST )
			{ send_to_char ("На складе получателя нет свободных мест.\r\n", ch);
			  return;
			}  

			if (!str_cmp(argument, "все"))
			{	
				if ( GET_CHEST(vict).m_chest.size() + m_chest.size()  >= MAXCOUNTCHEST )
					{ send_to_char ("На складе получателя нехватает свободных мест.\r\n", ch);
					  return;
					}  

				p = m_chest.begin();
				send_to_char("&WВы переслали со склада, нажитые непосильным трудом, следующие предметы:&n\r\n", ch);
				send_to_char("&WВы получили на склад, следующие предметы:&n\r\n", vict);
				while (p != m_chest.end())
				{  
					act("$o", FALSE, ch, *p, 0, TO_CHAR);
					act("$o", FALSE, vict, *p, 0, TO_CHAR);
					GET_CHEST(vict).m_chest.push_back(*p);
					p++;
				}
				//на самом деле clear() есть нечто инное как erase(m_chest.begin(), m_chest.end())
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
						    act("&WВы переслали адресату $3.&n", FALSE, ch, *p, 0, TO_CHAR);
						    act("&WВы получили на склад $3.&n", FALSE, vict, *p, 0, TO_CHAR);
							    GET_CHEST(vict).m_chest.push_back(*p);
							    m_chest.erase(p);
							    return;
					    }
				    }
			    }
			    else
			    { send_to_char("Нет такого номера!\r\n", ch);
			      return;
			    }
            }
			
			if (!(number = get_number(&tmp)))
					return;

			for ( p = m_chest.begin(); p != m_chest.end() && (j <= number); ++p)
			{ if (isname(tmp, (*p)->short_description))
					if (++j == number)
					{act("&WВы переслали адресату $3.&n", FALSE, ch, *p, 0, TO_CHAR);
					 act("&WВы получили на склад $3.&n", FALSE, vict, *p, 0, TO_CHAR);
						  GET_CHEST(vict).m_chest.push_back(*p);
						  m_chest.erase(p);
						return;
					}
			}
			send_to_char("У Вас на складе нет такого предмета!\r\n", ch);

		}
}

//Здесь обязательно сделать возможность смотреть предметы по шаблону (фильтру),
//например, по типу, (книги, оружие, щиты, одежда и т.д.)
void WareHouse::ListObj(CHAR_DATA *ch)
{	
	if(m_chest.empty())
	{ send_to_char("На Вашем складе пусто!\r\n", ch);
	   return;
	}

	Whouse::iterator p;

	int i = 0;
	*buf = '\0';
	
	send_to_char("У Вас на складе хранятся следующие предметы:\r\n", ch);

	//p - обязательно разименовывать (*p)-> иначе не будет доступа. Со всеми контейнерами так.
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

//функция по работе со складом
//тут либо запретить на склад сдавать полные контейнеры, либо обработать
//пока не знаю что логичней, вооще то сдают видные вещи приемщику,
//а то мало ли что сдадут, и он вернет не то что сдавали,
//с другой стороны это ж не реал, для удобства то как сделать?
//будут абьюзить количеством сдаваемых вещей?
//тогда считать вместе с количеством в контейнере,
//в общем принять надо какое нибудь решение в этом плане.

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

	 if(CMD_IS("список"))
	 { GET_CHEST(ch).ListObj( ch );
		 return true;
	 }
	 else
	 if(CMD_IS("сдать"))
	  {	 if (!ch->carrying)
			{ send_to_char("У Вас ничего нет!\r\n", ch);
		      return true; 
			}		
		  if (!*argument)
			{ send_to_char("Что Вы хотите сдать?\r\n", ch);
				return true;
			}

		  if ( GET_CHEST(ch).m_chest.size() >= MAXCOUNTCHEST )
			{ send_to_char ("На Вашем складе нет свободных мест.\r\n", ch);
			  return true;
			}       

			if (!str_cmp(argument, "все"))
			{ for (obj = ch->carrying; obj;  obj = next)
				{ next = obj->next_content;
					if (obj->contains)
					{ sprintf(buf1, "$n сказал$y Вам : \"Я не приму на склад %s с содержимым, сдавайте все поотдельности!\"", VOBJS(obj, ch));
					  act(buf1, FALSE, WHouse, 0, ch, TO_VICT);
						continue;
					}
					if (Crash_is_unrentable(obj))
					{ sprintf(buf1, "$n сказал$y Вам : \"Я не могу принять на склад %s.\"", VOBJS(obj, ch));
					  act(buf1, FALSE, WHouse, 0, ch, TO_VICT);
					  continue;
					}
				 found = true;

				  if ( GET_CHEST(ch).m_chest.size() >= MAXCOUNTCHEST )
					{ send_to_char ("На Вашем складе свободных мест не осталось.\r\n", ch);
					   break;
					} 

				 obj_from_char(obj);
				 GET_CHEST(ch).PushObj( obj );
				 sprintf(buf + strlen(buf), "%2d. %s.\r\n", GET_CHEST(ch).m_chest.size(), obj->short_vdescription);
				}//end for
			
			if (found)
				{ send_to_char("Вы сдали на склад следующие предметы:\r\n", ch);
				  send_to_char(buf, ch);
				}
			  return true;
			}
			if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying)))
				{  send_to_char("А себя не хочешь сдать на хранение?\r\n",ch);
	    		   act("$n громогласно расхохотал$u!", FALSE, WHouse, 0, 0, TO_ROOM);
				   return true;
				}
			if (Crash_is_unrentable(obj))
				{ sprintf(buf, "$n сказал$y Вам: \"Я не могу принять на склад %s.\"", VOBJS(obj, ch));
				  act(buf, FALSE, WHouse, 0, ch, TO_VICT);
				  return true;
				}
			if (obj->contains)
				{ sprintf(buf, "$n сказал$y Вам: \"Я не приму на склад %s с содержимым, сдавайте все поотдельности!\"", VOBJS(obj, ch));
				  act(buf, FALSE, WHouse, 0, ch, TO_VICT);
				  return true;
				}
			
			act("Вы сдали $3 на склад.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n сдал$y $3 на склад.", FALSE, ch, obj, 0, TO_ROOM);
			GET_CHEST(ch).PushObj( obj );
			obj_from_char(obj);
		 return true;
		}
     else
	 if(CMD_IS("получить"))
	  {	  if (!*argument)
			{ send_to_char("Что Вы хотите получить?\r\n", ch);
				return true;
			}

	      GET_CHEST(ch).GetObj( ch, argument );
		  return true;
	  }else
 	 if(CMD_IS("переслать"))
	  {	  if (!*argument)
			{ send_to_char("Кому Вы хотите переслать?\r\n", ch);
				return true;
			}
	      GET_CHEST(ch).TransferObj( ch, argument );
		  return true;
	  }
return false;
}

