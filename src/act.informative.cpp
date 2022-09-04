/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
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
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "pk.h"
#include "clan.h"

// extern variables 
extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern struct time_info_data time_info;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct zone_data *zone_table;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern int top_of_socialk;
extern struct char_data char_player_data; 
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern char w;
extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *zony;
extern const char *pc_class_types[];
extern struct char_data *mob_proto;
extern vector < ClanRec * > clan;


// extern functions 
long	find_class_bitvector(char arg);
int	level_exp(int chclass, int level);
struct	time_info_data *real_time_passed(time_t t2, time_t t1);
int	compute_armor_class(struct char_data *ch);
char	*str_str(char *cs, char *ct);
void	do_auto_exits(struct char_data * ch);

// local functions 
int  low_charm(struct char_data *ch);
void print_object_location(int num, struct obj_data * obj, struct char_data * ch, int recur);
void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode, int show);
void show_obj_to_char(struct obj_data * object, struct char_data * ch, int mode, int show_state, int how);
void perform_mortal_where(struct char_data * ch, char *arg);
void perform_immort_where(struct char_data * ch, char *arg);

int max_char_to_boot = 0;//количество игроков с момента перезагрузки.

ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_gen_tog);
ACMD(do_score);
ACMD(do_exits);
ACMD(do_scan);
ACMD(do_sides);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_looking);
ACMD(do_consider);
ACMD(do_commands);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
ACMD(do_affects);
ACMD(do_wimpy);
ACMD(do_razd);
ACMD(do_qwest);

void sort_commands(void);
void diag_char_to_char(struct char_data * i, struct char_data * ch);
void look_at_char(struct char_data * i, struct char_data * ch);
void list_one_char(struct char_data * i, struct char_data * ch, int skill_mode);
void list_char_to_char(struct char_data * list, struct char_data * ch);

void look_in_direction(struct char_data * ch, int dir, int info_is);
void look_in_obj(struct char_data * ch, char *arg);
char *find_exdesc(char *word, struct extra_descr_data * list);
void look_at_target(struct char_data * ch, char *arg, int subcmd);

#define EXIT_SHOW_WALL    (1 << 0)
#define EXIT_SHOW_LOOKING (1 << 1)

char *diag_obj_to_char(struct char_data * i, struct obj_data * obj, int mode)
{
   static  const char  *ObjState[8][2] =
   { {"рассыпается",  "рассыпается"},
   {"ужасное",      "в ужасном состоянии"},
   {"очень плохое", "в очень плохом состоянии"},
   {"плохое",        "в плохом состоянии"},
   {"среднее",      "в среднем состоянии"},
   {"хорошее",       "в хорошем состоянии"},
   {"очень хорошее", "в очень хорошем состоянии"},
   {"великолепное",  "в великолепном состоянии"}
   };

   static char out_str[128];
   out_str[0] = 0;

  char const   *color;
  int  percent;

  if (GET_OBJ_MAX(obj) > 0)
    percent = 100 * GET_OBJ_CUR(obj) / GET_OBJ_MAX(obj);
  else
    percent = -1;

  if (percent >= 100)
     {percent = 7;
      color   = CCCYN(i, C_NRM);
     }
  else
  if (percent >= 90)
     {percent = 6;
      color   = CCCYN(i, C_NRM);
     }
  else
  if (percent >= 75)
     {percent = 5;
      color   = CCGRN(i, C_NRM);
     }
  else
  if (percent >= 50)
     {percent = 4;
      color   = CCGRN(i, C_NRM);
     }
  else
  if (percent >= 30)
     {percent = 3;
      color   = CCYEL(i, C_NRM);
     }
  else
  if (percent >= 15)
     {percent = 2;
      color   = CCRED(i, C_NRM);
     }
  else
  if (percent >  0)
     {percent = 1;
      color   = CCRED(i, C_NRM);
     }
  else
     {percent = 0;
      color   = CCNRM(i, C_NRM);
     }

  if (mode == 1)
     sprintf(out_str,"&n[%s%s&n]%s",color, ObjState[percent][0], CCNRM(i, C_NRM));
  else
  if (mode == 2)
     strcpy(out_str, ObjState[percent][1]);
  else 
  if (mode == 3)
     sprintf(out_str,"%s%s%s", color, ObjState[percent][0], CCNRM(i, C_NRM));
  return out_str;
}

char *diag_timer_to_char(struct obj_data *obj)
{static char out_str[MAX_INPUT_LENGTH];

 *out_str = 0;
 if (GET_OBJ_RNUM(obj) != NOTHING)
    {int tm = (GET_OBJ_TIMER(obj) * 100 / GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(obj)));
     if (tm < 20)
        sprintf(out_str,"%s выглядит &rнепригодн$G&n.", obj->short_description);
     else
     if (tm < 40)
        sprintf(out_str,"%s выглядит &Rсильно изношенн$G&n.", obj->short_description);
     else
     if (tm < 60)
        sprintf(out_str,"%s выглядит &Yдовольно изношенн$G&n.", obj->short_description);
     else
     if (tm < 80)
        sprintf(out_str,"%s выглядит &Gнемного подержанн$G&n.", obj->short_description);
     else
        sprintf(out_str,"%s выглядит &Cсовсем нов$G&n.", obj->short_description);
    }          	
 return (CAP(out_str));
}

const char *weapon_class[] =
{ "Луки",
  "Короткие лезвия",
  "Длинные лезвия",
  "Топоры",
  "Посохи и дубины",
  "Разнообразное",
  "Двуручники",
  "Проникающее оружие",
  "Копья",
  "Рукопашный бой"
};

char *diag_weapon_to_char(struct obj_data *obj, int show_wear)
{static char out_str[MAX_INPUT_LENGTH];
 int    skill = 0;

 *out_str = '\0';
 switch(GET_OBJ_TYPE(obj))
 {case ITEM_WEAPON :
       switch (GET_OBJ_SKILL(obj))
       {case SKILL_BOWS        : skill = 1; break;
        case SKILL_SHORTS      : skill = 2; break;
        case SKILL_LONGS       : skill = 3; break;
        case SKILL_AXES        : skill = 4; break;
        case SKILL_CLUBS       : skill = 5; break;
        case SKILL_NONSTANDART : skill = 6; break;
        case SKILL_BOTHHANDS   : skill = 7; break;
        case SKILL_PICK        : skill = 8; break;
        case SKILL_SPADES      : skill = 9; break;
        case SKILL_PUNCH       : skill = 10; break;
        default:
           sprintf(out_str,"Не принадлежит к известным типам оружия - сообщите Богам!\r\n");
       }
       if (skill)
         sprintf(out_str,"Оружейное умение: \"%s\".\r\n",weapon_class[skill-1]);
  default:
       if (show_wear)
          {if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
              sprintf(out_str+strlen(out_str), "Можно одеть на палец.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_NECK))
              sprintf(out_str+strlen(out_str), "Можно одеть на шею.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_BODY))
              sprintf(out_str+strlen(out_str), "Можно одеть на туловище.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
              sprintf(out_str+strlen(out_str), "Можно одеть на голову.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
              sprintf(out_str+strlen(out_str), "Можно одеть на ноги.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_EYES))
			  sprintf(out_str+strlen(out_str), "Можно одеть на глаза. \r\n");
		   if (CAN_WEAR(obj, ITEM_WEAR_EAR))
              sprintf(out_str+strlen(out_str), "Можно вдеть в уши.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_BACKPACK))
              sprintf(out_str+strlen(out_str), "Можно носить за спиной.\r\n");
		   if (CAN_WEAR(obj, ITEM_WEAR_FEET))
              sprintf(out_str+strlen(out_str), "Можно обуть.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
              sprintf(out_str+strlen(out_str), "Можно одеть на руки.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
              sprintf(out_str+strlen(out_str), "Можно одеть на кисти.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
              sprintf(out_str+strlen(out_str), "Можно использовать как щит.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
              sprintf(out_str+strlen(out_str), "Можно одеть на плечи.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
              sprintf(out_str+strlen(out_str), "Можно одеть на пояс.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
              sprintf(out_str+strlen(out_str), "Можно одеть на запястья.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
              sprintf(out_str+strlen(out_str), "Можно взять в правую руку.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
              sprintf(out_str+strlen(out_str), "Можно взять в левую руку.\r\n");
           if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
              sprintf(out_str+strlen(out_str), "Можно взять в обе руки.\r\n");
          }
 }
 return (out_str);
}

void show_obj_to_char(struct obj_data * object, struct char_data * ch,
                      int mode,int show_state, int how)
{
  *buf = '\0';
  if ((mode < 5) && PRF_FLAGGED(ch, PRF_ROOMFLAGS) && (mode == 7))
     sprintf(buf, "[%5d] ", GET_OBJ_VNUM(object));    

  if ((mode == 0) && object->description)
     strcat(buf, object->description);
  else
  if (object->short_description &&
      ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4) || (mode == 7)))
     if (how > 1)
		 strcat(buf, object->short_description);
     else 
	     sprintf(buf,"%s", object->short_description);

  else
  if (mode == 5)
     {if (GET_OBJ_TYPE(object) == ITEM_NOTE)
         {if (object->action_description)
             {strcpy(buf, "Вы прочитали следующее :\r\n\r\n");
	      strcat(buf, object->action_description);
	      page_string(ch->desc, buf, 1);
             }
          else
	     send_to_char("Ничего не написано.\r\n", ch);
          return;
         }
      else
     if (GET_OBJ_TYPE(object) != ITEM_DRINKCON)
         {sprintf(buf, "Вы видите %s", object->short_vdescription);
		 if (OBJ_FLAGGED(object, ITEM_SHARPEN))
			 strcat(buf, " (заточен$Y)");
		     strcat(buf, "\r\n");
		}      
     else 
         strcpy(buf, "Это емкость для жидкости.\r\n");
     }

  if (show_state)
     { *buf2 = '\0';
       if (mode == 1 && how <= 1)
          {if (GET_OBJ_TYPE(object) == ITEM_LIGHT)
              {if (GET_OBJ_VAL(object,0) == -1) // изменяю на первую позицию - это 0 в редакторе
                  strcpy(buf2," (вечный свет)");
               else
               if (GET_OBJ_VAL(object,0) == 0) //  так же изменяю
                  sprintf(buf2," (погас%s)",
                          GET_OBJ_SUF_4(object));
               else
                  sprintf(buf2," (%d %s)",
                          GET_OBJ_VAL(object,0),desc_count(GET_OBJ_VAL(object,0),WHAT_HOUR));
              }
	   else
            if (GET_OBJ_CUR(object) <= GET_OBJ_MAX(object))
              sprintf(buf2,"  %s", diag_obj_to_char(ch,object,1));
	   if (GET_OBJ_TYPE(object) == ITEM_CONTAINER)
	      {if (object->contains)
	          strcat(buf2," (есть содержимое)");
	       else
	          sprintf(buf2+strlen(buf2)," (пуст%s)",GET_OBJ_SUF_1(object));
	      }
          }
       else
       if (mode >= 2 && how <= 1 && mode != 7) // изменяю что бы был свет с двойки на ноль
          {if(GET_OBJ_TYPE(object) == ITEM_LIGHT)
             {if (GET_OBJ_VAL(object,0) == -1)
                  sprintf(buf2,"%s - вечный свет.\r\n",
                          CAP((char *)OBJN(object,ch)));
               else
               if (GET_OBJ_VAL(object,0) == 0)
                  sprintf(buf2,"%s погас%s.",
                          CAP((char *)OBJN(object,ch)), GET_OBJ_SUF_4(object));
               else
                  sprintf(buf2,"%s будет светить %d %s.",
                          CAP((char *)OBJN(object,ch)),GET_OBJ_VAL(object,0),desc_count(GET_OBJ_VAL(object,0),WHAT_HOUR));
             }
           else
           if (GET_OBJ_CUR(object) <= GET_OBJ_MAX(object))
              sprintf(buf2,"%s %s.",
                      CAP((char *)OBJS(object,ch)), diag_obj_to_char(ch,object,2));
          }
       strcat(buf,buf2);
     }
  if (how > 1)
     sprintf(buf+strlen(buf)," [%d]",how);
  if (mode != 3 && how <= 1)
     {if (IS_OBJ_STAT(object, ITEM_INVISIBLE))
         {sprintf(buf2, " (невидим%s)", GET_OBJ_SUF_3(object));
          strcat(buf, buf2);
         }
      if (IS_OBJ_STAT(object, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
         strcat(buf, " ...голубая аура");
      if (IS_OBJ_STAT(object, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
         strcat(buf, " ...желтая аура");
      if (IS_OBJ_STAT(object, ITEM_POISONED) && AFF_FLAGGED(ch, AFF_DETECT_POISON))
         {sprintf(buf2, "...отравлен%s", GET_OBJ_SUF_6(object));
          strcat(buf, buf2);
         }
      if (IS_OBJ_STAT(object, ITEM_GLOW))
         strcat(buf, " ...блестит");
      if (IS_OBJ_STAT(object, ITEM_HUM))
         strcat(buf, " ...шумит");
      if (IS_OBJ_STAT(object, ITEM_FIRE))
         strcat(buf, " ...горит");
          
	}
   
  if (mode >= 5 && mode !=7)
     {strcat(buf, "\r\n");
	  strcat(buf,diag_weapon_to_char(object, FALSE));
      strcat(buf,diag_timer_to_char(object));
     }
 act(buf, FALSE, ch, object, 0,TO_CHAR);
}




/*void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
		           int show)
{
  struct obj_data *i;
  bool found = FALSE;
  int kol = 1;
  *buf1 = '\0';
  *buf = '\0';
  *buf2 = '\0';
  for (i = list; i; i = i->next_content) {
    if ((CAN_SEE_OBJ(ch, i) || GET_OBJ_TYPE(i)) == ITEM_LIGHT) {
           		found = TRUE;
		   if (mode == 7) {
			   if (show) sprintf(buf2, i->short_vdescription);
	     	   else sprintf(buf2, i->description);
		 if (!(i->next_content)) {
				if (kol > 1)
					send_to_char(buf, ch);
					 else { 
					strcat(buf2, "\r\n");
				send_to_char(buf2, ch);
				}
				 break;
			}
		
		 if (strcmp(i->short_vdescription, i->next_content->short_vdescription)) {
			 if (kol > 1) sprintf(buf1, buf);
		     else sprintf(buf1,"%s\r\n", buf2);
			  kol = 1;
		 } else {
			kol ++;
       sprintf(buf, "%s [%d]\r\n", buf2, kol);
       		continue;
		}		
				
		}  else show_obj_to_char(i, ch, mode);
		
	}
				if ((kol == 1) && mode == 7) 
				send_to_char(buf1, ch); 
			    if ((kol > 1) && mode == 7) 
				send_to_char(buf, ch);   
  }

  if (!found && show)
    send_to_char(" Ничего.\r\n", ch);

}*/

void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
                      int show)
{
  struct obj_data *i, *push=NULL;
  bool   found      = FALSE;
  int    push_count = 0;

  for (i = list; i; i = i->next_content)
      {if (CAN_SEE_OBJ(ch, i))
          {if (!push)
              {push       = i;
               push_count = 1;
              }
           else
           if (GET_OBJ_VNUM(i)    != GET_OBJ_VNUM(push) ||
               (GET_OBJ_TYPE(i)  == ITEM_DRINKCON &&
                GET_OBJ_VAL(i,2) != GET_OBJ_VAL(push,2)
               )                                        ||
	       (GET_OBJ_TYPE(i)  == ITEM_CONTAINER &&
	        i->contains &&
		!push->contains)                        ||
            (GET_OBJ_VNUM(push) == -1 && strcmp(push->short_description, "письмо" )) // mini hack for grouping mails
              )
			{  
				if (GET_OBJ_RLVL(push)>0)
				      send_to_char ("(У)", ch);					  
			   show_obj_to_char(push, ch, mode, show, push_count);
               push       = i;
               push_count = 1;
			}
           else
              push_count++;
           found = TRUE;
          }
      }
  if (push && push_count){
		if (GET_OBJ_RLVL(push)>0)
		      send_to_char ("(У)", ch);	
		show_obj_to_char(push, ch, mode, show, push_count);
   }
  if (!found && show)
     {if (show == 1)
         send_to_char("Внутри пусто.\r\n", ch);
      else
         send_to_char(" Ничего.\r\n", ch);
     }
}

void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int percent;

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_REAL_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  strcpy(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100)
    strcat(buf, " находится в великолепном состоянии");
  else if (percent >= 90)
    strcat(buf, " находится в очень хорошем состоянии"); 
  else if (percent >= 75)
    strcat(buf, " находится в хорошем состоянии");
  else if (percent >= 50)
    strcat(buf, " находится в среднем состоянии");
  else if (percent >= 30)
    strcat(buf, " находится в плохом состоянии");
  else if (percent >= 15)
    strcat(buf, " находится в очень плохом состоянии");
  else if (percent >= 0)
    strcat(buf, " находится в ужасном состоянии");
  else
    strcat(buf, " находится в бессознательном состоянии, умирает");

if (AFF_FLAGGED(ch, AFF_DETECT_POISON))
     if (AFF_FLAGGED(i, AFF_POISON))
        sprintf(buf + strlen(buf)," и отравлен%s", GET_CH_SUF_6(i));
       strcat(buf, ".\r\n");  
	 send_to_char(buf, ch);
}

void look_at_char(struct char_data * i, struct char_data * ch)
{
  int    j, found, push_count = 0;
  struct obj_data *tmp_obj, *push = NULL;

/*  if (!ch->desc)
     return;*/

  if (i->player.description && *i->player.description)
       send_to_char(i->player.description, ch);
  else  if (!IS_NPC(i))
		{ strcpy(buf,"На вид это");
			if (GET_SEX(i) == SEX_FEMALE)
			{ if (GET_HEIGHT(i) <= 151)
			{ if (GET_WEIGHT(i) >= 140)
                 strcat(buf," просто помпушка.\r\n");
              else
              if (GET_WEIGHT(i) >= 125)
                 strcat(buf," маленькая женщина.\r\n");
              else
                 strcat(buf," миниатюрная дамочка.\r\n");
             }
			  else
			  if (GET_HEIGHT(i) <= 159)
			  { if (GET_WEIGHT(i) >= 145)
                 strcat(buf," невысокая и в меру упитанная женщина.\r\n");
              else
              if (GET_WEIGHT(i) >= 130)
                 strcat(buf," невысокая женщина.\r\n");
              else
                 strcat(buf," стройная красавица.\r\n");
             }
          else
          if (GET_HEIGHT(i) <= 165)
				{ if (GET_WEIGHT(i) >= 145)
                 strcat(buf," среднего роста женщина.\r\n");
				else
                 strcat(buf," изящная женщина среднего роста.\r\n");
				}
			else
			if (GET_HEIGHT(i) <= 175)
				{ if (GET_WEIGHT(i) >= 150)
                 strcat(buf," могучая женщина.\r\n");
				else
				if (GET_WEIGHT(i) >= 135)
                 strcat(buf," высокая стройная женщина.\r\n");
				else
                 strcat(buf," высокая и импозантная женщина.\r\n");
             }
				else
				{ if (GET_WEIGHT(i) >= 155)
                 strcat(buf," крупная как лось женщина.\r\n");
				else
				if (GET_WEIGHT(i) >= 140)
                 strcat(buf," высокая и стройная женщина.\r\n");
				else
                 strcat(buf," худющая как жердь женщина.\r\n");
				}
			}
		 else
			{ if (GET_HEIGHT(i) <= 165)
			{ if (GET_WEIGHT(i) >= 170)
                 strcat(buf," пухленький и кругленький мужчинка.\r\n");
              else
              if (GET_WEIGHT(i) >= 150)
                 strcat(buf," низкорослый и плотный мужчина.\r\n");
              else
                 strcat(buf," замученный семейной жизнью мужчина.\r\n");
             }
          else
          if (GET_HEIGHT(i) <= 175)
             {if (GET_WEIGHT(i) >= 175)
                 strcat(buf," невыского роста крепкий мужчина.\r\n");
              else
              if (GET_WEIGHT(i) >= 160)
                 strcat(buf," невысокий крепкий мужчина.\r\n");
              else
                 strcat(buf," невысокий худощавый мужчина.\r\n");
             }
          else
          if (GET_HEIGHT(i) <= 185)
             {if (GET_WEIGHT(i) >= 180)
                 strcat(buf," среднего роста коренастный мужчина.\r\n");
              else
              if (GET_WEIGHT(i) >= 165)
                 strcat(buf," среднего роста крепкий мужчина.\r\n");
              else
                 strcat(buf," среднего роста худощавый мужчина.\r\n");
             }
          else
          if (GET_HEIGHT(i) <= 195)
             {if (GET_WEIGHT(i) >= 185)
                 strcat(buf," крупный как медведь мужчина.\r\n");
              else
              if (GET_WEIGHT(i) >= 170)
                 strcat(buf," высокий и симпатичный мужчина.\r\n");
              else
                 strcat(buf," длинный как жердь и худощавый мужчина.\r\n");
             }
          else
             {if (GET_WEIGHT(i) >= 190)
                 strcat(buf," просто какой-то уволень, а не мужчина!\r\n");
              else
              if (GET_WEIGHT(i) >= 180)
                 strcat(buf," очень высокий и крупный мужчина.");
              else
                 strcat(buf," длиннющий, похожий на жердь мужчина.\r\n");
             }
         }
      send_to_char(buf,ch);
     }
  else
     act("Ничего необычного в $p нет.", FALSE, i, 0, ch, TO_VICT);

 
  if (IS_HORSE(i) && i->master == ch)
     {strcpy(buf,"\r\nЭто Ваш скакун. Он ");
      if (GET_HORSESTATE(i) <= 0)
         strcat(buf,"загнан.\r\n");
      else
      if (GET_HORSESTATE(i) <= 20)
         strcat(buf,"в мыле.\r\n");
      else
      if (GET_HORSESTATE(i) <= 80)
         strcat(buf,"в хорошем состоянии.\r\n");
      else
         strcat(buf,"выглядит полным сил.\r\n");
      send_to_char(buf,ch);
     };

  diag_char_to_char(i, ch);

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
         found = TRUE;

  if (found)
     {send_to_char("\r\n",ch);
      act("$n использу$x:", FALSE, i, 0, ch, TO_VICT);
      for (j = 0; j < NUM_WEARS; j++)
          if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
             {send_to_char(where[j], ch);
	          show_obj_to_char(GET_EQ(i, j), ch, 1, ch == i, 1);
             }
     }

  if (ch != i && (IS_THIEF(ch) || IS_IMMORTAL(ch)))
     {found = FALSE;
      act("\r\nВы попытались разглядеть, что $n несет в инвентаре:", FALSE, i, 0, ch, TO_VICT);
      for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content)
          {if (CAN_SEE_OBJ(ch, tmp_obj) &&
               (number(0, 30) < GET_LEVEL(ch)))
              {if (!push)
                  {push       = tmp_obj;
                   push_count = 1;
                  }
               else
               if (GET_OBJ_VNUM(push) != GET_OBJ_VNUM(tmp_obj) || GET_OBJ_VNUM(push) == -1)
                  {show_obj_to_char(push, ch, 1, ch == i, push_count);
                   push       = tmp_obj;
                   push_count = 1;
                  }
               else
                  push_count++;
	           found = TRUE;
              }
          }
      if (push && push_count)
         show_obj_to_char(push, ch, 1, ch == i, push_count);
      if (!found)
         send_to_char("Вы ничего не смогли разглядеть.\r\n", ch);
     }
}


void list_one_char(struct char_data * i, struct char_data * ch, int skill_mode)
{ struct char_data *vict = NULL;
  int   sector = SECT_CITY;

  static const char *positions[] = {
    "- мертвое тело лежит здесь",
    "находится здесь в смертельно раненном состоянии",
    "находится здесь в тяжело раненном состоянии",
    "находится здесь в оглушенном состоянии",
    "спит здесь.",
    "отдыхает здесь.",
    "сидит здесь.",
    "сражается!",
    "стоит здесь."
	 }; 

  if (IS_HORSE(i) && on_horse(i->master))
     {if ( ch == i->master)
         {act("$N везет Вас на своей спине.",FALSE,ch,0,i,TO_CHAR);
         }
      return;
     }

  if (skill_mode == SKILL_LOOKING)
     {if (HERE(i) && INVIS_OK(ch,i) &&
          GET_REAL_LEVEL(ch) >= (IS_NPC(i) ? 0 : GET_INVIS_LEV(i))
         )
         {sprintf(buf,"Вы смогли разглядеть %s.\r\n",GET_VNAME(i));
          send_to_char(buf,ch);
         }
      return;
     }

     if (!CAN_SEE(ch,i) && (skill_mode != DARK))
	 {skill_mode = check_awake(i, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING |
                                  ACHECK_GLOWING | ACHECK_WEIGHT);
      *buf = 0;
      if (IS_SET(skill_mode, ACHECK_AFFECTS))
         {REMOVE_BIT(skill_mode, ACHECK_AFFECTS);
          sprintf(buf + strlen(buf),"магический ореол%s",skill_mode ? ", " : " ");
         }
      if (IS_SET(skill_mode, ACHECK_LIGHT))
         {REMOVE_BIT(skill_mode, ACHECK_LIGHT);
          sprintf(buf + strlen(buf),"яркий свет%s",skill_mode ? ", " : " ");
         }
      if (IS_SET(skill_mode, ACHECK_GLOWING) && IS_SET(skill_mode, ACHECK_HUMMING))
         {REMOVE_BIT(skill_mode, ACHECK_GLOWING);
          REMOVE_BIT(skill_mode, ACHECK_HUMMING);
          sprintf(buf + strlen(buf),"шум и блеск экипировки%s",skill_mode ? ", " : " ");
         }
      if (IS_SET(skill_mode, ACHECK_GLOWING))
         {REMOVE_BIT(skill_mode, ACHECK_GLOWING);
          sprintf(buf + strlen(buf),"блеск экипировки%s",skill_mode ? ", " : " ");
         }
      if (IS_SET(skill_mode, ACHECK_HUMMING))
         {REMOVE_BIT(skill_mode, ACHECK_HUMMING);
          sprintf(buf + strlen(buf),"шум экипировки%s",skill_mode ? ", " : " ");
         }
      if (IS_SET(skill_mode, ACHECK_WEIGHT))
         {REMOVE_BIT(skill_mode, ACHECK_WEIGHT);
          sprintf(buf + strlen(buf),"звон металла%s",skill_mode ? ", " : " "); 
         }
      
	 // sprintf(buf + strlen(buf),"и свет выдает присутствие %s.\r\n", GET_RNAME(i));
	  strcat(buf,"выдает чье-то присутствие.\r\n");
      send_to_char(CAP(buf),ch);
      return;
     }
       
	  if (!CAN_SEE(ch,i))
		return;
	 
	 if (IS_NPC(i) &&
      i->player.long_descr &&
      GET_POS(i) == GET_DEFAULT_POS(i) &&
      IN_ROOM(ch) == IN_ROOM(i) &&
      !AFF_FLAGGED(i,AFF_CHARM) &&
      !IS_HORSE(i)
     )
     {*buf = '\0';
      if (PRF_FLAGGED(ch, PRF_ROOMFLAGS))
       sprintf(buf, "[%5d] ", GET_MOB_VNUM(i));
      if (GET_ID(i) == GET_QUESTMOB(ch))
	  sprintf(buf + strlen(buf), "&G(задание) &R");

    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
         {if (IS_EVIL(i))
	         strcat(buf, "(аура тьмы) ");
          else
          if (IS_GOOD(i))
	         strcat(buf, "(аура света) ");
         }
	if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC) && IS_NPC(i))
	  { if (NPC_FLAGGED(i, NPC_AIRCREATURE))
		sprintf(buf + strlen(buf), "%s(аура воздуха)%s ", CCBLU(ch, C_CMP), CCRED(ch, C_CMP));
	    else if (NPC_FLAGGED(i, NPC_WATERCREATURE))
		sprintf(buf + strlen(buf), "%s(аура воды)%s ", CCCYN(ch, C_CMP), CCRED(ch, C_CMP));
	    else if (NPC_FLAGGED(i, NPC_FIRECREATURE))
		sprintf(buf + strlen(buf), "%s(аура огня)%s ", CCMAG(ch, C_CMP), CCRED(ch, C_CMP));
	    else if (NPC_FLAGGED(i, NPC_EARTHCREATURE))
		sprintf(buf + strlen(buf), "%s(аура земли)%s ", CCGRN(ch, C_CMP), CCRED(ch, C_CMP));
	  }       


       if (AFF_FLAGGED(i, AFF_MENTALLS))
         strcat(buf, "(в ментальном поле) ");	
      if (AFF_FLAGGED(i, AFF_INVISIBLE))
         sprintf(buf+strlen(buf), "(невидим%s) ", GET_CH_SUF_3(i));
      if (AFF_FLAGGED(i, AFF_HIDE))
         sprintf(buf+strlen(buf), "(спрятал%s) ", GET_CH_SUF_2(i));
      if (AFF_FLAGGED(i, AFF_CAMOUFLAGE))
         sprintf(buf+strlen(buf), "(замаскировал%s) ", GET_CH_SUF_2(i));
      if (AFF_FLAGGED(i, AFF_FLY))
         strcat(buf, "(летит) ");
      if (AFF_FLAGGED(i, AFF_HORSE))
         strcat(buf, "(под седлом) ");

	  if (GET_POS(i) == POS_SLEEPING)
             sprintf(buf + strlen(buf), "%s %s\r\n", CAP(GET_NAME(i)), positions[(int) GET_POS(i)]);
          else
	     strcat(buf, i->player.long_descr);
             send_to_char(buf, ch);
	  if (AFF_FLAGGED(i, AFF_HOLYLIGHT))
	     act("...вокруг летает маленький светящийся шарик.", FALSE, i, 0, ch, TO_VICT);
          if (AFF_FLAGGED(i, AFF_SANCTUARY))
             act("...светится ярким сиянием", FALSE, i, 0, ch, TO_VICT);
          if (AFF_FLAGGED (ch, AFF_DETECT_MAGIC))
		{  if (AFF_FLAGGED (i, AFF_HOLD))
	               act("...парализован$y", FALSE, i, 0, ch, TO_VICT);
                   if (AFF_FLAGGED(i, AFF_HOLDALL))
		       act("...не может передвигаться", FALSE, i, 0, ch, TO_VICT);
	           if (AFF_FLAGGED (i, AFF_SIELENCE))
	               act("...не может разговаривать", FALSE, i, 0, ch, TO_VICT);
		}
      if (AFF_FLAGGED(i, AFF_BLIND))
         act("...слеп$y", FALSE, i, 0, ch, TO_VICT);
      return; 
     }

  if (IS_NPC(i))
     {*buf1 = '\0';
	  if (GET_ID(i) == GET_QUESTMOB(ch))
	  sprintf(buf1, "&G(задание) &R");
	  strcpy(buf1 + strlen(buf1), CAP(i->player.short_descr));
      strcat(buf1, " ");
      if (AFF_FLAGGED(i, AFF_HORSE))
         strcat(buf1, "(под седлом) ");
      CAP(buf1);
     }
   else
     {
	  sprintf(buf1, "%s%s ",PRF_FLAGGED(i, PRF_AFK) ? "&C[AFK]&R ": "",
          race_or_title(i));
     }

  sprintf(buf, "%s%s",AFF_FLAGGED(i, AFF_CHARM) ? "(чарм) " : "", buf1);
   if (AFF_FLAGGED(i, AFF_MENTALLS))
     strcat(buf, "(в ментальном поле) ");
  if (AFF_FLAGGED(i, AFF_INVISIBLE))
     sprintf(buf+strlen(buf), "(невидим%s) ", GET_CH_SUF_3(i));
  if (AFF_FLAGGED(i, AFF_HIDE))
     sprintf(buf+strlen(buf), "(спрятал%s) ", GET_CH_SUF_2(i));
  if (AFF_FLAGGED(i, AFF_CAMOUFLAGE))
     sprintf(buf+strlen(buf), "(замаскировал%s) ", GET_CH_SUF_2(i));
  if (!IS_NPC(i) && !i->desc)
     sprintf(buf+strlen(buf), "(потерял%s связь) ", GET_CH_SUF_1(i));
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
     strcat(buf, "(пишет) ");

  if (GET_POS(i) != POS_FIGHTING)
     {if (on_horse(i)
         )
         sprintf(buf+strlen(buf),"сидит здесь верхом на %s ",
                 PPERS(get_horse(i),ch));
      else
      if (IS_HORSE(i) &&
          AFF_FLAGGED(i,AFF_TETHERED)
         )
         sprintf(buf + strlen(buf),"привязан%s здесь. ",GET_CH_SUF_6(i));
      else
      if ((sector = real_sector(IN_ROOM(i))) == SECT_FLYING)
         strcat(buf, "летает здесь.");
      else
      if (sector == SECT_UNDERWATER)
         strcat(buf, "плавает здесь.");
      else
      if (GET_POS(i) > POS_SLEEPING &&
          AFF_FLAGGED(i, AFF_FLY)
         )
         strcat(buf, "летает здесь.");
      else
      if (sector == SECT_WATER_SWIM || sector == SECT_WATER_NOSWIM)
         strcat(buf, "плавает здесь.");
      else
      if  (IS_NPC(i) && GET_POS(i) > POS_SLEEPING && GET_CLASS(i) == CLASS_SNEAK)
         strcat(buf, "ползает здесь.");
      else	 
      strcat(buf, positions[(int) GET_POS(i)]);
     }
  else
     {if (FIGHTING(i))
         {strcat(buf, "сражается c ");
          if (i->in_room != FIGHTING(i)->in_room)
             strcat(buf, "призрачной тенью");
          else
          if (FIGHTING(i) == ch)
	         strcat(buf, "ВАМИ");
          else {
	         strcat(buf, TPERS(FIGHTING(i), ch));
	         strcat(buf, "");
	        }
	  strcat(buf, "!");
         }
      else			/* NIL fighting pointer */
         strcat(buf, "сражаеться с ветряной мельницей.");
     }

  if (AFF_FLAGGED(ch, AFF_DETECT_POISON))
     if (AFF_FLAGGED(i, AFF_POISON))
        sprintf(buf+strlen(buf)," (отравлен%s) ", GET_CH_SUF_6(i));

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
     {if (IS_EVIL(i))
         strcat(buf, " (темная аура) ");
      else
      if (IS_GOOD(i))
         strcat(buf, " (светлая аура) ");
     }
   sprintf(buf + strlen(buf), " %s", PLR_FLAGGED(i, PLR_KILLER) ? "&W(УБИЙЦА)&R" : "");
   if (AFF_FLAGGED(i, AFF_HOLYLIGHT))
	strcat(buf, "...освещая все вокруг");
        strcat(buf, "&R\r\n");
        send_to_char(buf, ch);

  if (AFF_FLAGGED(i, AFF_SANCTUARY))
     act("...светится ярким сиянием ", FALSE, i, 0, ch, TO_VICT);
  if (AFF_FLAGGED (ch, AFF_DETECT_MAGIC))
		{  if (AFF_FLAGGED (i, AFF_HOLD))
	               act("...парализован$y", FALSE, i, 0, ch, TO_VICT);
                   if (AFF_FLAGGED(i, AFF_HOLDALL))
		       act("...не может передвигаться", FALSE, i, 0, ch, TO_VICT);
	           if (AFF_FLAGGED (i, AFF_SIELENCE))
	               act("...не может разговаривать", FALSE, i, 0, ch, TO_VICT);
		}
  if (AFF_FLAGGED(i, AFF_BLIND))
     act("...слеп$y", FALSE, i, 0, ch, TO_VICT);
  }
	

void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
      if (ch != i && IS_NPC(i))
         {if (CAN_SEE(ch, i) || awaking(i,AWAKE_HIDE | AWAKE_INVIS | AWAKE_CAMOUFLAGE))
              list_one_char(i, ch, 0);
          else
          if (IS_DARK(i->in_room)       &&
              IN_ROOM(i) == IN_ROOM(ch) &&
              !CAN_SEE_IN_DARK(ch)      &&
	      AFF_FLAGGED(i, AFF_INFRAVISION)
             )
	     send_to_char("Вы видите светящиеся красным глаза.\r\n", ch);
         }

	  for (i = list; i; i = i->next_in_room)
      if (ch != i && !IS_NPC(i))
         {if (HERE(i) && CAN_SEE(ch, i) || awaking(i,AWAKE_HIDE | AWAKE_INVIS | AWAKE_CAMOUFLAGE))
              list_one_char(i, ch, 0);
          else
          if (IS_DARK(i->in_room)       &&
              IN_ROOM(i) == IN_ROOM(ch) &&
              !CAN_SEE_IN_DARK(ch)      &&
	      AFF_FLAGGED(i, AFF_INFRAVISION)
             )
	     send_to_char("Вы видите светящиеся красным глаза.\r\n", ch);
         }
  }


const char *dirs_L[] =
{
  "N",
  "E",
  "S",
  "W",
  "U",
  "D",
  "\n"
};

void do_auto_exits(CHAR_DATA * ch)
{
	int door, slen = 0;

	buf[0] = 0;

	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE) {
			if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
				slen += sprintf(buf + slen, "(%c) ", LOWER(*dirs_L[door]));
			else if (!EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN))
				slen += sprintf(buf + slen, "%c ", LOWER(*dirs_L[door]));
		}
	sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCGRN(ch, C_NRM), *buf ? buf : "None! ", CCNRM(ch, C_NRM));

	send_to_char(buf2, ch);
}

ACMD(do_exits)
{
  int door;

  *buf = '\0';

  if (AFF_FLAGGED(ch, AFF_BLIND))
     {send_to_char("Вы слепы, как сурок!\r\n", ch);
      return;
     }
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) &&
        EXIT(ch, door)->to_room != NOWHERE &&
  	!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)
       )
       {if (IS_GOD(ch))
           sprintf(buf2, "%-9s - [%5d] %s\r\n", dirs2[door],
                   GET_ROOM_VNUM(EXIT(ch, door)->to_room),
                   world[EXIT(ch, door)->to_room].name);
        else
           {sprintf(buf2, "%-9s - ", dirs2[door]);
            if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
               strcat(buf2, "&Kслишком темно\r\n");
            else
                {sprintf(buf2 + strlen(buf2),"&C%s&w", world[EXIT(ch,
				door)->to_room].name);
                strcat(buf2, "\r\n");
               }
           }
        strcat(buf, CAP(buf2));
       }
  send_to_char("Доступные выходы:\r\n", ch);
  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char("Видимых выходов нет!\r\n", ch);
}


ACMD(do_looking)
{
  int  i;

  if (!ch->desc)
     return;

  if (GET_POS(ch) < POS_SLEEPING)
     send_to_char("Вам приглядеться только не хватало, пора подумать о душе!\r\n", ch);
  if (GET_POS(ch) == POS_SLEEPING)
     send_to_char("Вам стоит сначала проснуться!\r\n", ch);
  else
  if (AFF_FLAGGED(ch, AFF_BLIND))
     send_to_char("Вы слепы!\r\n", ch);
  else
  if (GET_SKILL(ch,SKILL_LOOKING))
     {if (check_moves(ch, LOOKING_MOVES))
         {send_to_char("Вы начали присматриваться по сторонам и сумели разглядеть.\r\n", ch);
          for (i=0; i < NUM_OF_DIRS; i++)
              look_in_direction(ch, i, EXIT_SHOW_LOOKING);
          if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE)))
               WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
         }
     }
  else
     send_to_char("Вы так напрягли зрение, что глаза чуть не лопнули.\r\n", ch);
}


#define MAX_FIRES 6
const char *Fires[MAX_FIRES] =
{
 "тлеет небольшая кучка угольков",
 "тлеет большая кучка угольков",
 "теплится небольной огонек",
 "горит небольшой костер",
 "весело трещит большой костер",
 "ярко пылает огромный костер"
};

#define TAG_NIGHT       "<night>"
#define TAG_DAY         "<day>"
#define TAG_WINTERNIGHT "<winternight>"
#define TAG_WINTERDAY   "<winterday>"
#define TAG_SPRINGNIGHT "<springnight>"
#define TAG_SPRINGDAY   "<springday>"
#define TAG_SUMMERNIGHT "<summernight>"
#define TAG_SUMMERDAY   "<summerday>"
#define TAG_AUTUMNNIGHT "<autumnnight>"
#define TAG_AUTUMNDAY   "<autumnday>"

int  paste_description(char *string, const char *tag, int need)
{
   char *pos;
   if (!*string || !*tag)
    return (FALSE);
    
   char btag[256];
   strcpy( btag, tag);

   if ((pos = str_str(string,btag)))
    {if (need)
        {for (;*pos && *pos != '>'; pos++);
         if  (*pos)
             pos++;
         if  (*pos == 'R')
             {pos++;
              buf[0] = '\0';
             }
         
		
			 if (!buf)   
		     strcpy(buf,pos);
         else 
	    	 sprintf (buf + strlen(buf), "%s\r\n", pos);
			 // strcat(buf, pos);
		 if ((pos = str_str(buf,btag)))
		 {  *pos = '\0';
           strcat(buf, "\r\n");
         }
		   return (TRUE);
        }
     else
        {*pos = '\0';
         if ((pos = str_str(string,btag)))
            strcat(string,pos+strlen(btag));
        }
    }
 return (FALSE);
}


void show_extend_room(char *description, struct char_data *ch)
{
   int  found = FALSE,i = 0;
   char string[MAX_STRING_LENGTH], *pos;

   if (!description || !*description)
      return;

 strcpy(string,description);
 if ((pos = strchr(description,'<')))
    *pos = '\0';
 strcpy(buf,description);
 if (pos)
    *pos = '<';

 found = found || paste_description(string,TAG_WINTERNIGHT,
        (weather_info.season == SEASON_WINTER &&
         (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
 found = found || paste_description(string,TAG_WINTERDAY,
        (weather_info.season == SEASON_WINTER &&
         (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
 found = found || paste_description(string,TAG_SPRINGNIGHT,
        (weather_info.season == SEASON_SPRING &&
         (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
 found = found || paste_description(string,TAG_SPRINGDAY,
        (weather_info.season == SEASON_SPRING &&
         (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
 found = found || paste_description(string,TAG_SUMMERNIGHT,
        (weather_info.season == SEASON_SUMMER &&
         (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
 found = found || paste_description(string,TAG_SUMMERDAY,
        (weather_info.season == SEASON_SUMMER &&
         (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
 found = found || paste_description(string,TAG_AUTUMNNIGHT,
        (weather_info.season == SEASON_AUTUMN &&
         (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)));
 found = found || paste_description(string,TAG_AUTUMNDAY,
        (weather_info.season == SEASON_AUTUMN &&
         (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)));
 found = found || paste_description(string,TAG_NIGHT,
         (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK));
 found = found || paste_description(string,TAG_DAY,
         (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT));
 for (i = strlen(buf); i > 0 && *(buf+i) == '\n'; i--)
     {*(buf+i) = '\0';
      if (i > 0 && *(buf+i) == '\r')
         *(buf+ --i) = '\0';
     }

   send_to_char(buf,ch);
// send_to_char("\r\n",ch);

}


/*ACMD(do_affects)
{
struct affected_type* aff;
char   sp_name[MAX_STRING_LENGTH];
	
	send_to_char("\\c09Аффекты:\\c00\r\n", ch);

  
  if (ch->affected)
     {for (aff = ch->affected; aff; aff = aff->next)
          {*buf2 = '\0';
           strcpy(sp_name,spell_name(aff->type));
              sprintf(buf, "\\c06%s\\c00", sp_name);
		   if (!IS_IMMORTAL(ch))
              {if (aff->next && aff->type == aff->next->type)
                  continue;
              }
           else
              {if (aff->modifier)
                  {sprintf(buf2, " %+d к %s", aff->modifier, apply_types[(int) aff->location]);
	           strcat(buf, buf2);
                  }
               if (aff->bitvector)
                  {if (*buf2)
	              strcat(buf, ", устанавливает ");
	           else
	              strcat(buf, " устанавливает ");
	           strcat(buf,CCRED(ch,C_NRM));
                   sprintbit(aff->bitvector, affected_bits, buf2);
   	           strcat(buf, buf2);
                   strcat(buf,CCNRM(ch,C_NRM));	
                  }
              }
           send_to_char(strcat(buf, "\r\n"), ch);
          }
     }
}
 */
int hiding[] = { AFF_SNEAK,
	AFF_HIDE,
	AFF_CAMOUFLAGE,
	0
};

ACMD(do_affects)
{ 	AFFECT_DATA *aff;
	FLAG_DATA saved;
	int i, j;
	char sp_name[MAX_STRING_LENGTH];

	saved = ch->char_specials.saved.affected_by;

	for (i = 0; (j = hiding[i]); i++)
	{ if (IS_SET(GET_FLAG(saved, j), j))
	  	  SET_BIT(GET_FLAG(saved, j), j);
		
	    REMOVE_BIT(AFF_FLAGS(ch, j), j);
	}

	sprintbits(ch->char_specials.saved.affected_by, affected_bits, buf2, ", ");
	sprintf(buf, "Аффекты: %s%s%s\r\n", CCRED(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	for (i = 0; (j = hiding[i]); i++)
		if (IS_SET(GET_FLAG(saved, j), j))
			SET_BIT(AFF_FLAGS(ch, j), j);

  if (ch->affected)
     {for (aff = ch->affected; aff; aff = aff->next)
          {*buf2 = '\0';
           strcpy(sp_name, spell_name(aff->type));
		   (aff->duration+1)/SECS_PER_MUD_HOUR ? sprintf(buf2, "%d %s", (aff->duration+1)/SECS_PER_MUD_HOUR + 1,
			desc_count((aff->duration+1)/SECS_PER_MUD_HOUR + 1, WHAT_HOUR)) : sprintf(buf2, "менее часа");
         		sprintf(buf, "%s%s%-21s - %s%s",
				*sp_name == '!' ? "Состояние  : " : "Заклинание : ",
				CCCYN(ch, C_NRM), sp_name, buf2, CCNRM(ch, C_NRM));

		   if (!IS_IMMORTAL(ch))
              {if (aff->next && aff->type == aff->next->type)
                  continue;
              }
           else
              {if (aff->modifier)
                  {sprintf(buf2, " %+ld к %s", aff->modifier, apply_types[(int) aff->location]);
	           strcat(buf, buf2);
                  }
               if (aff->bitvector)
                  {if (*buf2)
	              strcat(buf, ", устанавливает ");
	           else
	              strcat(buf, " устанавливает ");
	           strcat(buf,CCRED(ch,C_NRM));
                   sprintbit(aff->bitvector, affected_bits, buf2);
   	           strcat(buf, buf2);
                   strcat(buf,CCNRM(ch,C_NRM));	
                  }
              }
           send_to_char(strcat(buf, "\r\n"), ch);
          }
     }
}





// Смотрим в комнату
void look_at_room(struct char_data * ch, int ignore_brief)
{  
  if (!ch->desc)
    return;

  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char("Вы ничего не видите, кроме летающих в глазах зайчиков..\r\n", ch); 
    return;
  }
  			   send_to_char(CCCYN(ch, C_NRM), ch);
			  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
			    sprintbits(world[ch->in_room].room_flags, room_bits, buf, ", ");
		      sprintf(buf2, "[%5d] %s [ %s]", GET_ROOM_VNUM(IN_ROOM(ch)),
	       world[ch->in_room].name, buf);
       send_to_char(buf2, ch);
			} else
				send_to_char(world[ch->in_room].name, ch);

		send_to_char(CCNRM(ch, C_NRM), ch);
		send_to_char("\r\n", ch);
 
		if (IS_DARK(IN_ROOM(ch)) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
     send_to_char("Здесь темно...\r\n",ch);
        else
            if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
		      ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
	        //send_to_char(world[ch->in_room].description, ch);
			{show_extend_room(world[ch->in_room].description, ch);
			}
   
 /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
     do_auto_exits(ch);
			
		if (world[IN_ROOM(ch)].fires)
			{sprintf(buf,"%sВ центре %s.%s\r\n",
              CCRED(ch,C_NRM),
              Fires[MIN(world[IN_ROOM(ch)].fires, MAX_FIRES - 1)],
              CCNRM(ch,C_NRM));

      send_to_char(buf,ch);
     }

  if (world[IN_ROOM(ch)].portal_time)
     {sprintf(buf,"%sМагический портал переливаясь цветами радуги, сверкает здесь.%s\r\n",
              CCYEL(ch,C_NRM),
              CCNRM(ch,C_NRM));
      send_to_char(buf,ch);
     }

  if (IN_ROOM(ch) != NOWHERE && !ROOM_FLAGGED(IN_ROOM(ch),ROOM_NOWEATHER))
    {*buf = '\0';
     switch(real_sector(IN_ROOM(ch)))
     { case SECT_FIELD_SNOW:
       case SECT_FOREST_SNOW:
       case SECT_HILLS_SNOW:
       case SECT_MOUNTAIN_SNOW:
       sprintf(buf,"&WСнежный ковер лежит у Вас под ногами.&n\r\n");
               
       break;
       case SECT_FIELD_RAIN:
       case SECT_FOREST_RAIN:
       case SECT_HILLS_RAIN:
       sprintf(buf,"%sВы увязаете в грязи.%s\r\n",
               CCWHT(ch,C_NRM),CCNRM(ch,C_NRM));
       break;
       case SECT_THICK_ICE:
       sprintf(buf,"%sУ Вас под ногами толстый лед.%s\r\n",
               CCBLU(ch,C_NRM),CCNRM(ch,C_NRM));
       break;
       case SECT_NORMAL_ICE:
       sprintf(buf,"%sУ Вас под ногами достаточно толстый лед.%s\r\n",
               CCBLU(ch,C_NRM),CCNRM(ch,C_NRM));
       break;
       case SECT_THIN_ICE:
       sprintf(buf,"%sТонкий лед вот-вот проломится под Вами.%s\r\n",
               CCCYN(ch,C_NRM),CCNRM(ch,C_NRM));
       break;
      };
      if (*buf)
         send_to_char(buf,ch);
    }

		/* now list characters & objects */
		send_to_char("&Y", ch);
			list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
					send_to_char(CCRED(ch, C_NRM), ch);
				list_char_to_char(world[ch->in_room].people, ch);
			send_to_char(CCNRM(ch, C_NRM), ch);
            
 }


/*void look_in_direction(struct char_data * ch, int dir)
{
	int    count = 0, probe, percent;
    struct room_direction_data * rdata=NULL;
    struct char_data * tch;

	if (EXIT(ch, dir)&&!IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
	  if (world[EXIT(ch, dir)->to_room].people) {
		sprintf(buf, "Вы посмотрели %s и увидели: \r\n", dirs2[dir]);
		  send_to_char(buf, ch);	
			if(IS_DARK(EXIT(ch, dir)->to_room) && GET_LEVEL(ch) <= LVL_IMMORT){ 
		  strcpy(buf, "\\c06слишком темно, что бы что-нибудь увидеть\\c00\r\n");
	     send_to_char(buf, ch);
		}	
 }
    else
      send_to_char("Вы не видите здесь ничего необычного.\r\n", ch); 

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && EXIT(ch, dir)->keyword) {
		   sprintf(buf, "%s - закрыто.\r\n", fname(EXIT(ch, dir)->keyword)); 
             send_to_char(buf, ch);
	} else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword) {
           sprintf(buf, "%s - открыто.\r\n", fname(EXIT(ch, dir)->keyword));
          send_to_char(buf, ch);
       list_char_to_char(world[EXIT(ch, dir)->to_room].people, ch);
    }
  } else
    send_to_char("Нет ничего необычного.\r\n", ch);
}

*/
void look_in_direction(struct char_data * ch, int dir, int info_is)
{int    count = 0, probe, percent;
 struct room_direction_data * rdata=NULL;
 struct char_data * tch;

const char *dirs_3[] = {
	  "На севере: ",
      "На востоке: ",
	  "На юге: ",
	  "На западе: ",
	  "Вверху: ",
	  "Внизу: "
  };
 
 if (CAN_GO(ch, dir) || (EXIT(ch,dir) && EXIT(ch,dir)->to_room != NOWHERE))
    {rdata  = EXIT(ch, dir);
     count += sprintf(buf,"%14s",dirs_3[dir]);
     if (EXIT_FLAGGED(rdata, EX_CLOSED))
        {if (rdata->keyword)
            count += sprintf(buf+count, " закрыто (%s).\r\n",
                             fname(rdata->keyword));
         else
            count += sprintf(buf+count, " закрыто.\r\n");
         send_to_char(buf, ch);
         return;
        }
     if (IS_TIMEDARK(rdata->to_room))
        {count += sprintf(buf+count, "&Kслишком темно&n\r\n");
         send_to_char(buf, ch);
         if (info_is & EXIT_SHOW_LOOKING)
            {send_to_char("&R", ch);
             for (count = 0, tch = world[rdata->to_room].people; tch; tch = tch->next_in_room)
                 {percent = number (1,skill_info[SKILL_LOOKING].max_percent);
                  probe   = train_skill(ch,SKILL_LOOKING,skill_info[SKILL_LOOKING].max_percent,NULL);
                  if (HERE(tch)        &&
                      INVIS_OK(ch,tch) &&
                      probe >= percent &&
                      (percent < 100 || IS_IMMORTAL(ch))
                     )
                     {list_one_char(tch,ch,SKILL_LOOKING);
                      count++;
                     }
                 }
             if (!count)
                send_to_char("Вы ничего не смогли разглядеть!\r\n",ch);
             send_to_char(CCNRM(ch, C_NRM), ch);
            }
       }
    else
	{if (rdata->general_description)
           count += sprintf(buf+count, "&C%s&w\r\n",rdata->general_description);
        else
           count += sprintf(buf+count, "&K%s&w\r\n",world[rdata->to_room].name);
        send_to_char(buf,ch);
        send_to_char("&R", ch);
        list_char_to_char(world[rdata->to_room].people, ch);
        send_to_char(CCNRM(ch, C_NRM), ch);
       }
   }
	/*{   count += sprintf(buf + count, "&Kничего особенного&n\r\n");
		    send_to_char(buf,ch);
        send_to_char(CCRED(ch, C_NRM), ch);
        list_char_to_char(world[rdata->to_room].people, ch);
        send_to_char(CCNRM(ch, C_NRM), ch);
       }
   }*/
 else
    if (info_is & EXIT_SHOW_WALL)
       send_to_char("Что Вы там собираетесь увидеть?\r\n", ch);
}

void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char("Смотреть во что?\r\n", ch);
    else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				                      FIND_OBJ_EQUIP, ch, &dummy, &obj))) 
	{ sprintf(buf, "У Вас этого нет.\r\n"); 
      send_to_char(buf, ch);
	} else
	  if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	      (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	      (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
    act("Вы не можете смотреть в $3.", FALSE, ch, obj, 0, TO_CHAR);
	else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
	     act("Закрыт$Y.",FALSE,ch,obj,0,TO_CHAR);
	  else { send_to_char((obj->short_description), ch);
	
		switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(" (в инвентаре): \r\n", ch); 
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(" (на земле): \r\n", ch);
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(" (в экипировке): \r\n", ch); 
	  break;
	}
				if (!obj->contains)
                 send_to_char(" Внутри пусто.\r\n", ch);
				else	
				list_obj_to_char(obj->contains, ch, 1, bits != FIND_OBJ_ROOM);
      }
    } 
	else 
	{ if (GET_OBJ_VAL(obj, 1) <= 0)
	  {  sprintf(buf, "Внутри %s пусто!\r\n", obj->short_rdescription);
		send_to_char(buf, ch);
	  
      } else {
	if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0)) {
	  sprintf(buf, "Содержимое невидно.\r\n");
	} else {
	  amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
	  sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
	  sprintf(buf, "%s %sзаполнен$Y %s жидкостью.",CAP(obj->short_description), fullness[amt], buf2);
	}
    act(buf,FALSE,ch,obj,0,TO_CHAR);
	//	send_to_char(buf, ch);
      }
    }
  }
}



char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */


void look_at_target(struct char_data * ch, char *arg, int subcmd)
{
  int bits, found = FALSE, fnum, i = 0, in_eq = FALSE;
  struct char_data *found_char = NULL;
  struct obj_data *found_obj = NULL;
  char   *desc, *what, whatp[MAX_INPUT_LENGTH], where[MAX_INPUT_LENGTH];
  int    where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM;


  if (!*arg)
     {send_to_char("Что Вы хотите увидеть?\r\n", ch);
      return;
     }

  half_chop(arg, whatp, where);
  what = whatp;

  if (isname(where, "земля комната room ground"))
     where_bits = FIND_OBJ_ROOM | FIND_CHAR_ROOM;
  else
  if (isname(where, "инвентарь inventory"))
     where_bits = FIND_OBJ_INV  | FIND_CHAR_ROOM;
  else
  if (isname(where, "экипировка equipment"))
     where_bits = FIND_OBJ_EQUIP | FIND_CHAR_ROOM;

  bits = generic_find(what, where_bits, ch, &found_char, &found_obj);

   // Проверяем цель персонаж? 
   if (found_char != NULL)
      {if (subcmd == SCMD_LOOK_HIDE &&
           !check_moves(ch, LOOKHIDE_MOVES)
          )
          return;
       look_at_char(found_char, ch);
       if (ch != found_char)
          {if (subcmd == SCMD_LOOK_HIDE &&
               GET_SKILL(ch, SKILL_LOOK_HIDE) > 0
              )
              {fnum = number(1,skill_info[SKILL_LOOK_HIDE].max_percent);
               found= train_skill(ch,SKILL_LOOK_HIDE,skill_info[SKILL_LOOK_HIDE].max_percent,found_char);
               if (!WAITLESS(ch))
	          WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
               if (found >= fnum                   &&
                   (fnum < 100 || IS_IMMORTAL(ch)) &&
                   !IS_IMMORTAL(found_char))
                  return;
              }
           if (CAN_SEE(found_char, ch))
              act("$n посмотрел$y на Вас.", TRUE, ch, 0, found_char, TO_VICT);
           act("$n посмотрел$y на $V.", TRUE, ch, 0, found_char, TO_NOTVICT);
          }
       return;
      }

   // Смотрим "number." (2.хлеб). 
  if (!(fnum = get_number(&what)))
     {send_to_char("Что Вы хотите посмотреть?\r\n", ch);
      return;
     }

  // Соответствует ли аргумент дополнительному ключевому слову в описании комнаты? 
  if ((desc = find_exdesc(what, world[ch->in_room].ex_description)) != NULL &&
      ++i == fnum)
     {page_string(ch->desc, desc, FALSE);
      return;
     }

if (bits && (found_obj != NULL))
    { if (!(desc = find_exdesc (what, found_obj->ex_description)))
    	show_obj_to_char (found_obj, ch, 5, TRUE, 1);	// Показываем без описания 
      else 
	{ send_to_char (desc, ch);
	  show_obj_to_char (found_obj, ch, 6, TRUE, 1);	// Ищем шум,блеск и т.д.
	}
	}
  else
     send_to_char("Здесь ничего нет!\r\n", ch);

}


ACMD(do_look)
{
  char arg2[MAX_INPUT_LENGTH];
  int look_type;

 // if (!ch->desc)
    //return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("Вы ничего не видите кроме звезд!\r\n", ch);
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char("Вы ничего не видите, Вы слепы как крот!\r\n", ch);
  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !arg) {
    send_to_char("Здесь темно...\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char("Читать что?\r\n", ch);
      else
	look_at_target(ch, arg, subcmd);
      return;
    }
    if (!*arg)			
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "в")||is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type, EXIT_SHOW_WALL);
    else if (is_abbrev(arg, "на")||is_abbrev(arg, "at"))
      look_at_target(ch, arg2,  subcmd);
    else
      look_at_target(ch, arg, subcmd);
  }
}



ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char   obj_name[MAX_STRING_LENGTH];
  
	if (GET_POS(ch) < POS_SLEEPING)
     {send_to_char("Вы не можете этого сделать во сне!\r\n", ch);
      return;
     }
	else
	if (AFF_FLAGGED(ch, AFF_BLIND))
     {send_to_char("Вы слепы!\r\n", ch);
      return;
     }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("посмотреть что?\r\n", ch); 
    return;
  }
    strcpy(obj_name, arg);
  look_at_target(ch, arg, subcmd);

  generic_find(obj_name, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		                 FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) 
//	act("Вы посмотрели в $3 и увидели:", FALSE, ch, tmp_object,0 , TO_CHAR); 
  	      look_in_obj(ch, arg);
    }
		
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char("Вы голы ака соколы!\r\n", ch);
  else if (GET_GOLD(ch) == 1)
    send_to_char("Вы имеете одну монету и поверьте, это не много!\r\n", ch);
  else {
    sprintf(buf, "Вы имеете %d %s.\r\n", GET_GOLD(ch), desc_count(GET_GOLD(ch),WHAT_MONEYa));
    send_to_char(buf, ch);
  }
}

ACMD(do_qwest)
{ struct char_data *vict = NULL;
    
  if (IS_NPC(ch))
    return;
    vict = get_mob_by_id(GET_QUESTMOB(ch));
	
	if (GET_QUESTMOB(ch) > 0 && vict != NULL)
		{ sprintf(buf, "Активные задания:\r\n\r\n &gВы должны убить &W%s&g.&n\r\n", GET_VNAME(vict));
	      sprintf(buf + strlen(buf), "&g На выполнение задания у вас &Wосталось %d %s&g.&n\r\n", 
		  GET_COUNTDOWN(ch), desc_count(GET_COUNTDOWN(ch),WHAT_MINa));	  
		  sprintf(buf + strlen(buf), " &gМестонахождение: &W%s&n\r\n", 
		  world[vict->in_room].name != NULL ? world[vict->in_room].name : "неизвестно");
		  send_to_char(buf, ch);
		}
	else 
	  if (!GET_COUNTDOWN(ch))	
            send_to_char("Активных заданий нет.\r\n", ch);
     if (GET_COUNTDOWN(ch) && !GET_QUESTMOB(ch))
	{ sprintf(buf, "Активные задания:\r\n\r\n &G(Выполнено) &gДоложить о выполнении &Wосталось %d %s&g.&n\r\n",
	  GET_COUNTDOWN(ch), desc_count(GET_COUNTDOWN(ch),WHAT_MINu));
	  send_to_char(buf, ch); 	
	} 
     if (GET_NEXTQUEST(ch))
	{ sprintf(buf, " &gСледующее задание Вы можете &Wполучить через %d %s&g.&n\r\n",
	  GET_NEXTQUEST(ch), desc_count(GET_NEXTQUEST(ch),WHAT_MINu));
	  send_to_char(buf, ch); 	
	}
}

const char *MessagAlign(int percent )
{
if (percent <= -1000)
    return "&KГений зла";
  if (percent <= -900)
    return "&KЛютый";
  if (percent <= -800)
    return "&wИзувер";
  if (percent <= -700)
    return "&wЗлой";
  if (percent <= -600)
    return "&wКоварный";
  if (percent <= -500)
    return "&bПодлый";
  if (percent <= -400)
    return "&bЖестокий";
  if (percent <= -300)
    return "&bГадкий";
  if (percent <= -200)
    return "&bЗверский";
  if (percent <= -100)
    return "&BТень зла";
  if (percent <= 0)
    return "&BНейтрал";
  if (percent <= 100)
    return "&BТень добра";
  if (percent <= 200)
    return "&BНежный";
  if (percent <= 300)
    return "&cЛасковый";
  if (percent <= 400)
    return "&cГуманный";
  if (percent <= 500)
    return "&cДобрый";
  if (percent <= 600)
    return "&cМилый";
  if (percent <= 700)
    return "&CДобро";
  if (percent <= 800)
    return "&CПраведный";
  if (percent <= 900)
    return "&WБлагой";
  if (percent <= 1000)
    return "&WСамо добро";
    
  return "ОШИБКА";
}


char buf3[15];//stats - просто статы, Stats - модифицированые (REAL) статы.
char* Stats_char(char_data* ch, int har)
{
  int value = 0;
  int real_value = 0;  
  switch (har)
  {
  case STR:
      value = GET_STR(ch);
      real_value = GET_REAL_STR(ch);
  break;
  case DEX:
      value = GET_DEX(ch);
      real_value = GET_REAL_DEX(ch);
  break;
  case CON:
      value = GET_CON(ch);
      real_value = GET_REAL_CON(ch);
  break;
  case INT:
      value = GET_INT(ch);
      real_value = GET_REAL_INT(ch);
  break;
  case WIS:
      value = GET_WIS(ch);
      real_value = GET_REAL_WIS(ch);
  break;
  case CHA:
      value = GET_CHA(ch);
      real_value = GET_REAL_CHA(ch);
  break;
  default:
      strcpy(buf3, "");
      return buf3;
  }
  
  int trens = GET_POINT_STAT(ch, har); // trening points
  int born_value = value - trens;
  int race = GET_RACE(ch);
  static const char *data_roll_char[14] = {"&r", "&R", "&m", "&M","&y", "&Y", "&g", "&G", "&b", "&B", "&c", "&C", "&W", "&W"};

  sprintf(buf3, "%s%2d%s(%d+%d)", 
      data_roll_char [MAX(0, MIN(13, real_value - ClasStat[race][har]))], real_value,
      data_roll_char [MAX(0, MIN(13, value - ClasStat[race][har]))], born_value, trens
		                        );
  return buf3;
}

extern int has_mail(CHAR_DATA* ch);
ACMD(do_score)
{ 
  struct time_info_data playing_time;
  static const char *Posic[] = 
  {"Вы умерли",
   "Вы умираете",
   "Без сознания",
   "Вы ранены",
   "Вы спите",
   "Отдыхаете",
   "Вы сидите",
   "Вы в бою",
   "Вы стоите"
  }; 

  if (IS_NPC(ch))
      return;


  playing_time = *real_time_passed((time(0) - ch->player_specials->time.logon) +
				  ch->player_specials->time.played, 0);


sprinttype(GET_SEX(ch), genders, buf1);

char mail[16];
int letters = has_mail(ch);
if (!letters) { strcpy(mail, "&cНет"); }
else
{ 
   const char *m = "писем";
   int x = letters % 10;
   if (letters < 10 || letters > 20)
   {   
        if (x == 1)
           m = "письмо";
        if (x >= 2 && x <= 4)
           m = "письма";
   }
   sprintf(mail, "&C%d %s", letters, m); 
}

char dsu[16];
if (!IS_IMMORTAL(ch))
{
    if (!GET_GOD_FLAG(ch, GF_REMORT))
    {
        int exp = level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1) - GET_EXP(ch);
        sprintf(dsu, "%d", exp);
    }
    else
    {
        strcpy(dsu, "Реморт");
    }
}
else
{
    strcpy(dsu, "0");
}

sprintf(buf,"  &Y                    .---.                          .---.\r\n"
			"                    /V    )\\            .           /(    V\\\r\n"
			"                  /V )     )\\          /\\          /(     ( V\\\r\n"
			"                /V )        )\\  .  \".'\\  /'.\"  .  /(        ( V\\\r\n"
			"              /V )            . )\\ /\\\"\\  /\"/\\ /( .            ( V\\\r\n"
			"            /V )             . )~.\\&W(&r|&Y\\ \\/ /&r|&W)&Y/.~( .             ( V\\\r\n"
			"          /V )              . )-. '.&W'&Y/ )( \\&W'&Y.' .~( .              ( V\\\r\n"
			"   &W______&Y/&W_&Y)&W___          ____&Y~.&W__&Y'/ /_(/\\)_\\ \\ &W__&Y.~&W____          ___&Y(&W_&Y\\&W_______\r\n"
			"  &W/ __)&c***&W(___ \\&Y.~~~~~~.&W/ (&c*&W\\/&c*&W) &Y( (( )\"\"( )) ) &W(&c*&W\\/&c*&W) \\&Y.~~~~~~.&W/ ___)&c***&W(___ \\\r\n"
			" / ____)&c*&W(_____&Y/  ^  ^  \\&W___)(____&Y\\\\\\_._._.///&W____)(___&Y/  ^  ^  \\&W_____)&c*&W(_____ \\\r\n"
			"(&c*&W/           &Y(__)(__)(__)         \\ &W\\/vv\\/&Y /         (__)(__)(__)            &W\\&c*&W)\r\n"
			"((&c*           &w \\/  \\/  \\/ \\_/      &Y \\&W/\\^^/\\&Y/       &w\\_/ \\/  \\/  \\/             &c*&W))\r\n");
sprintf(buf + strlen(buf), "&c*&W)) Интеллект : %-13s&w-<_&Y)        &Y'~~~~'        &Y(&w_>-  &WУбитых мобов  : &c%-5d&W((&c*\r\n", Stats_char(ch, INT), ch->player_specials->saved.spare19);
sprintf(buf + strlen(buf), "&W((&c* &WМудрость  : %-14s&W/ \\                      / \\   Убитых игроков: &c%-5d*&W))\r\n", Stats_char(ch, WIS),ch->player_specials->saved.spare9);
sprintf(buf + strlen(buf), "&c*&W)) Ловкость  : %-14s&W)   Класс     : &c%-11s&W(   Ваших смертей : &c%-5d&W((&c*\r\n", 	Stats_char(ch, DEX), CLASS_ABBR(ch), ch->player_specials->saved.spare12);
sprintf(buf + strlen(buf), "&W((&c* &WСила      : %-13s&W(    Пол       : &c%-12s&W)  Голод         : %-7s&c*&W))\r\n", Stats_char(ch, STR), CAP(buf1), !GET_COND(ch, FULL) ? "&RДа ": "&cНет");
sprintf(buf + strlen(buf), "&c*&W)) Тело      : %-14s&W)   Уровень   : &c%-11d&W(   Жажда         : %-7s&W((&c*\r\n", Stats_char(ch, CON), GET_LEVEL(ch), 	!GET_COND(ch, THIRST) ? "&RДа ": "&cНет");
sprintf(buf + strlen(buf), "&W((&c* &WУдача     : %-13s&W(    ОИМ       : &c%-12d&W)  Опьянение     : %-7s&c*&W))\r\n", Stats_char(ch, CHA), IND_POWER_CHAR(ch),(GET_COND(ch, DRUNK) > 10) ? "&RДа&n ":"&cНет");
sprintf(buf + strlen(buf), "&c*&W)) Склонность: %-12s&W)   ПИМ       : &c%-11d&W(   Поглощение    : &c%-5d&W((&c*\r\n"
			"&W((&c*&W----------------------(----------------------------)-----------------------&c*&W))\r\n"
			"&c*&W)) Жизни     : &c%4d/%-5d&W)   &WОпыт      : &c%-11d&W(   Возраст       : &c%-5d&W((&c*\r\n"
			"&W((&c* &WЭнергии   : &c%3d/%-5d&W(    &WДСУ       : &c%-12s&W)  Вес           : &c%-5d*&W))\r\n"
			"&c*&W)) Наличные  : &c%-10d&W)   &WРеморты   : &c%-11d&W(   Рост          : &c%-5d&W((&c*\r\n"
			"&W((&c* &WБанк      : &c%-9d&W/    &WПочта     : %-14s&W\\  Размер        : &c%-5d*&W))\r\n"
			"&c*&W)) Слава     : &c%-8d&W/  &wВ игре%4d %-4s и %2d %-7s&W\\ Хитролл       : &c%-5d&W((&c*\r\n"
            "&W(&c*&W\\ Тренировок: &c%-5d&W  /________________________________\\Дамролл       : &c%-5d&W/&c*&W)\r\n"
			" \\ \\__/&c*&W)) _//_\\\\____ /%18s %-13s  \\  ___/(\\_'/|| \\___// /\r\n"
			"  \\___\\&c*&W))_\\\\&c*&W_&c*&W//___/____________________________________\\____\\(&c*&W))_||_/_____/\r\n"
			"               &Y (__)(__)(__)&w\\/                      &w\\/&Y(__)(__)(__) \r\n"
			"                &w \\/  \\/  \\/                            &w\\/  \\/  \\/ \r\n",

			MessagAlign(GET_ALIGNMENT(ch)),
			IND_SHOP_POWER(ch), GET_ABSORBE(ch),
			GET_HIT(ch), GET_REAL_MAX_HIT(ch),
			GET_EXP(ch), GET_REAL_AGE(ch),
			GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
			dsu,
			GET_WEIGHT(ch),
			GET_GOLD(ch), GET_REMORT(ch), GET_HEIGHT(ch),
			GET_BANK_GOLD(ch), mail ,GET_REAL_SIZE(ch),
			GET_GLORY(ch), playing_time.day, desc_count(playing_time.day, WHAT_DAY),
			playing_time.hours, desc_count(playing_time.hours, WHAT_HOUR), GET_REAL_HR(ch), GET_CRBONUS(ch),  GET_REAL_DR(ch),
			pc_race_types[GET_RACE(ch)][GET_SEX(ch)], GET_NAME(ch));

act(buf, FALSE, ch, 0, 0, TO_CHAR);

*buf = '\0';

    if (GET_GOD_FLAG(ch, GF_GODSCURSE) &&  GODS_DURATION(ch))
     {int hrs  = (GODS_DURATION(ch) - time(NULL)) / 3600;
      int mins = ((GODS_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
      sprintf(buf + strlen(buf),"Вы прокляты Богами на %d %s %d %s.\r\n",
              hrs,desc_count(hrs,WHAT_HOUR),
              mins,desc_count(mins,WHAT_MINu));
     }	   
			if (RENTABLE(ch)) 
            {
                int t = ((RENTABLE(ch) - time(NULL)) % 3600 + 59) / 60;                
                sprintf(buf + strlen(buf),"%sВ настоящее время Вы не сможете уйти на постой, Вы в состоянии войны. Время БД: %d%s\r\n",
			    CCRED(ch, C_NRM), t, CCNRM(ch, C_NRM));
            }
	    
	if (PLR_FLAGGED(ch, PLR_KILLER) && GET_TIME_KILL(ch))
	{  int mins = GET_TIME_KILL(ch);	
		sprintf(buf + strlen(buf),"&CУстановлен флаг убийцы на время %d %s.&n\r\n", 
				mins, desc_count(mins, WHAT_MINu));
	}
     if (PLR_FLAGGED(ch, PLR_HELLED) &&
        HELL_DURATION(ch)            &&
        HELL_DURATION(ch) > time(NULL)
      )
     {int hrs  = (HELL_DURATION(ch) - time(NULL)) / 3600;
      int mins = ((HELL_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
      sprintf(buf + strlen(buf),"Вам предстоит провести в тюрьме еще %d %s, %d %s.\r\nКто и основание:\r\n%s.\r\n",
              hrs, desc_count(hrs,WHAT_HOUR), 
              mins, desc_count(mins,WHAT_MINu), HELL_REASON(ch));
     }
    if (PLR_FLAGGED(ch, PLR_MUTE) &&
      MUTE_DURATION(ch) != 0 &&
      MUTE_DURATION(ch) > time(NULL)
     )
     {int hrs  = (MUTE_DURATION(ch) - time(NULL)) / 3600;
      int mins = ((MUTE_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
      sprintf(buf + strlen(buf),"Вы не сможете кричать еще %d %s %d %s.\r\n",
              hrs,desc_count(hrs,WHAT_HOUR),
              mins,desc_count(mins,WHAT_MINu));
         }	
    if (PLR_FLAGGED(ch, PLR_DUMB) &&
      DUMB_DURATION(ch) != 0 &&
      DUMB_DURATION(ch) > time(NULL)
     )
     {int hrs  = (DUMB_DURATION(ch) - time(NULL)) / 3600;
      int mins = ((DUMB_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
      sprintf(buf + strlen(buf),"Вы будете молчать еще %d %s %d %s.\r\n",
              hrs,desc_count(hrs,WHAT_HOUR),
              mins,desc_count(mins,WHAT_MINu));
        }	
    if (PLR_FLAGGED(ch, PLR_NAMED) &&
	  NAME_DURATION(ch) !=0		&&
	  NAME_DURATION(ch) > time(NULL)
	  )
	{ int hrs  = (NAME_DURATION(ch) - time(NULL)) / 3600;
      int mins = ((NAME_DURATION(ch) - time(NULL)) % 3600 + 59) / 60;
              sprintf(buf + strlen(buf),"Вы будете находится в комнате имени %d %s %d %s.\r\n",
              hrs,desc_count(hrs,WHAT_HOUR),
              mins,desc_count(mins,WHAT_MINu));
	}
 
    if (has_horse(ch,FALSE))
     {if (on_horse(ch))
         sprintf(buf + strlen(buf),"Вы верхом на %s.\r\n",GET_PNAME(get_horse(ch)));
      else
         sprintf(buf + strlen(buf),"У Вас есть %s.\r\n",GET_NAME(get_horse(ch)));
     }
    strcat(buf, CCNRM(ch, C_NRM));
  act(buf, FALSE, ch, 0, 0, TO_CHAR);
}



ACMD(do_inventory)
{
  send_to_char("Вы несете:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 7, 2);
}

ACMD(do_equipment)
{ int i, found = 1;
 
 send_to_char("Вы используете:\r\n", ch);
   for (i = 0; i < NUM_WEARS; i++)
   { if (GET_EQ(ch, WEAR_SHIELD) && (i == WEAR_HOLD   || i == WEAR_BOTHS)			||
	 GET_EQ(ch, WEAR_HOLD)   && (i == WEAR_SHIELD || i == WEAR_BOTHS || i == WEAR_LIGHT) 	||
	 GET_EQ(ch, WEAR_BOTHS)  && (i == WEAR_HOLD   || i == WEAR_SHIELD||                   	
				     i == WEAR_WIELD  || i == WEAR_LIGHT )	 		||
	 GET_EQ(ch, WEAR_LIGHT)  && (i == WEAR_HOLD   || i == WEAR_BOTHS)
		 )
              continue;
	  	 if (GET_EQ(ch, i)) 
		 { send_to_char(where[i], ch);
			 if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
			{   if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR)
				{ if (GET_OBJ_VAL(GET_EQ(ch, i),1) < 15)
				      send_to_char ("&n", ch);
		          else
                  if (GET_OBJ_VAL(GET_EQ(ch, i),1) < 25)
					  send_to_char ("&c", ch);
				  else
				  if (GET_OBJ_VAL(GET_EQ(ch, i),1) < 50)
				      send_to_char ("&C", ch);
		          else
                      send_to_char ("&W", ch);
          	   	}
				
				if (GET_OBJ_RLVL(GET_EQ(ch, i))>0)
				      send_to_char ("(У)", ch);
				  
			 show_obj_to_char(GET_EQ(ch, i), ch, 1, TRUE, 1);
			 }
		    else  
		   send_to_char("Что-то.\r\n", ch);		
		} 
		else
		{ send_to_char ("&K", ch);
		  send_to_char(where[i], ch);
		  send_to_char("<ничего нет>&n\r\n", ch);
		  found = TRUE;
		}
   }
  if (!found) 
    send_to_char("На Вас ничего не одето.\r\n", ch);
}

ACMD(do_time)
{
  int day, month = 0, days_go = 0;
  if (IS_NPC(ch))
     return;
  
  sprintf(buf, "Сейчас ");
  switch(time_info.hours % 24)
     {
  case 0:
       sprintf(buf+strlen(buf), "полночь, ");
       break;
  case 1:
       sprintf(buf+strlen(buf), "1 час ночи, ");
       break;
  case 2:
  case 3:
  case 4:
       sprintf(buf+strlen(buf), "%d часа ночи, ", time_info.hours);
       break;
  case 5: case 6: case 7: case 8: case 9: case 10: case 11:
       sprintf(buf+strlen(buf), "%d часов утра, ", time_info.hours);
       break;
  case 12:
       sprintf(buf+strlen(buf), "полдень, ");
       break;
  case 13:
       sprintf(buf+strlen(buf), "1 час пополудни, ");
       break;
  case 14:
  case 15:
  case 16:
       sprintf(buf+strlen(buf), "%d часа пополудни, ", time_info.hours-12);
       break;
  case 17: case 18: case 19: case 20: case 21: case 22: case 23:
       sprintf(buf+strlen(buf), "%d часов вечера, ", time_info.hours-12);
       break;
     }

 /* if (GET_RELIGION(ch) == RELIGION_POLY)
     strcat(buf, weekdays_poly[weather_info.week_day_poly]);
  else*/
     strcat(buf, weekdays[weather_info.week_day_mono]);
  switch (weather_info.sunlight)
  {case SUN_DARK : strcat(buf,", темное время суток"); break;
   case SUN_SET  : strcat(buf,", сумерки"); break;
   case SUN_LIGHT: strcat(buf,", светлое время суток"); break;
   case SUN_RISE : strcat(buf,", заря"); break;
  }
  strcat(buf, ".\r\n");
  send_to_char(buf, ch);
   day = time_info.day + 1;	/* day in [1..35] */
  *buf = '\0';
 /* if (GET_RELIGION(ch) == RELIGION_POLY || IS_IMMORTAL(ch))
     {days_go = time_info.month * DAYS_PER_MONTH + time_info.day;
      month   = days_go / 40;
      days_go = (days_go % 40) + 1;
      sprintf(buf + strlen(buf), "%s, %dй День, Год %d%s",
              month_name_poly[month], days_go, time_info.year,IS_IMMORTAL(ch) ? ".\r\n" : "");
     }*/
 // if (GET_RELIGION(ch) == RELIGION_MONO || IS_IMMORTAL(ch))
     sprintf(buf + strlen(buf), "%s, День %dй, Год %d",
             month_name[(int) time_info.month], day, time_info.year);
  switch (weather_info.season)
  {case SEASON_WINTER : strcat(buf, ", зима"); break;
   case SEASON_SPRING : strcat(buf, ", весна"); break;
   case SEASON_SUMMER : strcat(buf, ", лето"); break;
   case SEASON_AUTUMN : strcat(buf, ", осень"); break;
  }
  strcat(buf, ".\r\n");
  send_to_char(buf, ch);
 // gods_day_now(ch);
}


int get_moon(int sky, int mode)
{ if (!mode && (weather_info.sunlight == SUN_RISE  ||
      weather_info.sunlight == SUN_LIGHT ||
      sky                   == SKY_RAINING))
     return (0);
  else
  if (weather_info.moon_day <= NEWMOONSTOP || weather_info.moon_day >= NEWMOONSTART)
     return (1);
  else
  if (weather_info.moon_day <  HALFMOONSTART)
     return (2);
  else
  if (weather_info.moon_day <  FULLMOONSTART)
     return (3);
  else
  if (weather_info.moon_day <= FULLMOONSTOP)
     return (4);
  else
  if (weather_info.moon_day < LASTHALFMOONSTART)
     return (5);
  else
     return (6);
  return (0);
}


ACMD(do_weather)
{ int   sky=weather_info.sky, weather_type=weather_info.weather_type;
  const char *sky_look[] =
        {"облачное", //0
         "пасмурное",//1
         "покрыто тяжелыми тучами", //2
         "ясное" //3
        };
  const char *moon_look[] =
        {"Новолуние.",
         "Растущий серп луны.",
         "Растущая луна.",
         "Полнолуние.",
         "Убывающая луна.",
         "Убывающий серп луны."
        };

  if (OUTSIDE(ch))
     { *buf = '\0';
       if (world[IN_ROOM(ch)].weather.duration > 0)
          {sky          = world[IN_ROOM(ch)].weather.sky;
           weather_type = world[IN_ROOM(ch)].weather.weather_type;
          }
       sprintf(buf + strlen(buf),
               "Небо %s. %s\r\n%s\r\n", sky_look[sky],
               get_moon(sky, 0) ? moon_look[get_moon(sky, 0)-1] : "",
	           (weather_info.change >= 0 ? "Атмосферное давление повышается." :
 	                                       "Атмосферное давление понижается."));
       sprintf(buf+strlen(buf),"На улице %d %s.\r\n",weather_info.temperature, desc_count(weather_info.temperature, WHAT_DEGREE));

       if (IS_SET(weather_info.weather_type,WEATHER_BIGWIND))
          strcat(buf,"Порывистый ветер сдувает с ног.\r\n");
       else
       if (IS_SET(weather_info.weather_type,WEATHER_MEDIUMWIND))
          strcat(buf,"Дует умеренный ветер.\r\n");
       else
       if (IS_SET(weather_info.weather_type,WEATHER_LIGHTWIND))
          strcat(buf,"Дует легкий ветерок.\r\n");

       if (IS_SET(weather_type,WEATHER_BIGSNOW))
          strcat(buf,"Повалил снег.\r\n");
       else
       if (IS_SET(weather_type,WEATHER_MEDIUMSNOW))
          strcat(buf,"Снегопад.\r\n");
       else
       if (IS_SET(weather_type,WEATHER_LIGHTSNOW))
          strcat(buf,"Начался слабый снег.\r\n");

       if (IS_SET(weather_type,WEATHER_GRAD))
          strcat(buf,"Пошел дождь с градом.\r\n");
       else
       if (IS_SET(weather_type,WEATHER_BIGRAIN))
          strcat(buf,"Начался сильный ливень.\r\n");
       else
       if (IS_SET(weather_type,WEATHER_MEDIUMRAIN))
          strcat(buf,"Идет дождь.\r\n");
       else
       if (IS_SET(weather_type,WEATHER_LIGHTRAIN))
          strcat(buf,"Моросит дождик.\r\n");
 	
       send_to_char(buf, ch);
     }
  else
     send_to_char("Вы ничего не можете сказать о погоде сегодня.\r\n", ch);
  if (IS_GOD(ch))
     {sprintf(buf,"День: %d Месяц: %s Час: %d Такт = %d\r\n"
              "Температура =%-5d, за день = %-8d, за неделю = %-8d\r\n"
              "Давление    =%-5d, за день = %-8d, за неделю = %-8d\r\n"
              "Выпало дождя = %d(%d), снега = %d(%d). Лед = %d(%d). Погода = %08x(%08x).\r\n",
              time_info.day, month_name[time_info.month], time_info.hours, weather_info.hours_go,
              weather_info.temperature, weather_info.temp_last_day, weather_info.temp_last_week,
              weather_info.pressure, weather_info.press_last_day, weather_info.press_last_week,
              weather_info.rainlevel,   world[IN_ROOM(ch)].weather.rainlevel,
              weather_info.snowlevel,   world[IN_ROOM(ch)].weather.snowlevel,
              weather_info.icelevel,    world[IN_ROOM(ch)].weather.icelevel,
              weather_info.weather_type,world[IN_ROOM(ch)].weather.weather_type);
      send_to_char(buf,ch);
     }
}

ACMD(do_help)
{
  int chk, bot, top, mid, minlen, compare = 0;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
	  send_to_char(help, ch);//нафига нужна здесь страничка? в прочем есжели че.. разремить -)
    //page_string(ch->desc, help, 1);
    return;
  }
  if (!help_table) {
    send_to_char("Помощь невозможна.\r\n", ch);
    return;
  }
    
  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;
    if (bot > top)
	{ send_to_char("Нет справки по заданной теме.\r\nПопробуйте изменить запрос или набрать без пробелов.\r\nМожете спросить используя канал &YОФФТОПИК&n.\r\n", ch);
          if (!strstr(argument, "%"))
           { sprintf(buf,"HELP: %s - %s",argument, GET_NAME(ch));
             mudlog(buf, CMP, LVL_IMMORT, TRUE);
           }
          return;
        } 
	else
    {  compare = ((int)strlen(help_table[mid].keyword));
		if(!(chk = strn_cmp(argument, help_table[mid].keyword, minlen)))
		{  while ((mid > 0) && ((chk = !is_abbrev(argument, help_table[mid].keyword))))
               mid--;
		     if (compare != minlen)
		        while (mid < top &&
	                 !strn_cmp(argument, help_table[mid+1].keyword, minlen) &&
	                 *(help_table[mid].keyword+minlen))
	                mid++;
             page_string(ch->desc, help_table[mid].entry, 0);
             return;
		}
		else
		{
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
		}	
    }
  }
}

#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-c classlist] [-s] [-o] [-q] [-r] [-z]\r\n"

ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH];
 // char morts[MAX_STRING_LENGTH];
  char impl[MAX_INPUT_LENGTH];
  char glgod[MAX_INPUT_LENGTH];
  char grgod[MAX_INPUT_LENGTH];

  char god[MAX_INPUT_LENGTH];
  char immort[MAX_INPUT_LENGTH];
//  char mode;
  size_t i=0;
  int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0, num_can_all = 0;
  int who_room = 0, clan_num=0;  

  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';
  impl[0]   = '\0';
  glgod[0]  = '\0';
  grgod[0]  = '\0';
  god[0]    = '\0';
  immort[0] = '\0';


 /* while (*buf) {
    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else if (*arg == '-') {
      mode = *(arg + 1);       // just in case; we destroy arg in the switch 
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = 1;
	strcpy(buf, buf1);
	break;
      case 'c':
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      default:
	send_to_char(WHO_FORMAT, ch);
	return;
      }				/* end of switch */

   // } else {			/* endif */
  /*    send_to_char(WHO_FORMAT, ch);
      return;
    }
  }		*/		/* end while (parser) */
   
	
    *buf = '\0';	
  
  for (d = descriptor_list; d; d = d->next)
  { if (STATE(d) != CON_PLAYING)
            continue;

    if (d->original)
       tch = d->original;
    else
	if (!(tch = d->character))
       continue;

    if (GET_LEVEL(tch) < LVL_IMMORT)
 	num_can_all++; 
    

	if (!CAN_SEE(ch, tch))
	  continue;

	
    any_one_arg(argument, arg);
	if (!isname(arg, GET_NAME(tch)) && *arg)
	{   who_room = 1;  
		continue;
	}

   	if (GET_LEVEL(tch) == LVL_IMPL)
	{   sprintf(impl + strlen(impl),"\r\n%29s%s%s&n",!GET_PTITLE(tch)?"&W":GET_PTITLE(tch),GET_NAME(tch),
	    !GET_TITLE(tch)?"":GET_TITLE(tch));
       if (GET_INVIS_LEV(tch))
	sprintf(impl + strlen(impl), " (i%d)", GET_INVIS_LEV(tch));
			if (PRF_FLAGGED(tch, PRF_AFK))
    strcat(impl, " \\c14[\\c05A\\c08F\\c11K\\c14]\\c00");
	low++;	
	}
  
	if(GET_LEVEL(tch) == LVL_GLGOD)
	{ sprintf(glgod + strlen(glgod),"\r\n%29s%s%s&n",!GET_PTITLE(tch)?"&W":GET_PTITLE(tch),GET_NAME(tch),
	!GET_TITLE(tch)?"":GET_TITLE(tch));
     if (GET_INVIS_LEV(tch))
	 sprintf(glgod + strlen(glgod), " (i%d)", GET_INVIS_LEV(tch));
			if (PRF_FLAGGED(tch, PRF_AFK))
     strcat(glgod, " \\c14[\\c05A\\c08F\\c11K\\c14]\\c00");
		low++;
	}
 
	if(GET_LEVEL(tch) == LVL_GRGOD)
	{ sprintf(grgod + strlen(grgod),"\r\n%29s%s%s&n",!GET_PTITLE(tch)?"&W":GET_PTITLE(tch),GET_NAME(tch),
	!GET_TITLE(tch)?"":GET_TITLE(tch));
    if (GET_INVIS_LEV(tch))
	  sprintf(grgod + strlen(grgod), " (i%d)", GET_INVIS_LEV(tch));
			if (PRF_FLAGGED(tch, PRF_AFK))
       strcat(grgod, " \\c14[\\c05A\\c08F\\c11K\\c14]\\c00");
		low++;
	}

	if(GET_LEVEL(tch) == LVL_GOD)
	{ sprintf(god + strlen(god),"\r\n%29s%s%s&n",!GET_PTITLE(tch)?"&W":GET_PTITLE(tch),GET_NAME(tch),
	!GET_TITLE(tch)?"":GET_TITLE(tch));
      if (GET_INVIS_LEV(tch))
	     sprintf(god + strlen(god), " (i%d)", GET_INVIS_LEV(tch));
			if (PRF_FLAGGED(tch, PRF_AFK))
    strcat(god, " \\c14[\\c05A\\c08F\\c11K\\c14]\\c00");
		low++;
	}

	if(GET_LEVEL(tch) == LVL_IMMORT)
	{ sprintf(immort + strlen(immort),"\r\n%29s%s%s&n",!GET_PTITLE(tch)?"&W":GET_PTITLE(tch),GET_NAME(tch),
	!GET_TITLE(tch)?"":GET_TITLE(tch));
             if (GET_INVIS_LEV(tch))
	   sprintf(immort + strlen(immort), " (i%d)", GET_INVIS_LEV(tch));
			if (PRF_FLAGGED(tch, PRF_AFK))
        strcat(immort, " \\c14[\\c05A\\c08F\\c11K\\c14]\\c00");
		low++;
		}
 /*   
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[ch->in_room].zone != world[tch->in_room].zone)
      continue;
    if (who_room && (tch->in_room != ch->in_room))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;	
	*/	
		
	 
	
		if(GET_LEVEL(tch) < LVL_IMMORT)
		{ if (!num_can_see)
			{ sprintf(buf, "\r\n\r\n   %29sИгроки%s\r\n",CCGRN(ch, C_CMP),CCNRM(ch, C_CMP));
			sprintf(buf+strlen(buf),"  %29s--------%s\r\n",CCGRN(ch, C_CMP),CCNRM(ch, C_CMP));
			}
    	num_can_see++;
     *buf1 ='\0';
	if ((clan_num=find_clan_by_id(GET_CLAN(tch)))>=0&&clan_num<num_of_clans)
		{
		if(GET_CLAN_RANK(tch)>0)
        sprintf(buf1,"[%s]  ", clan[clan_num]->name);
	//	sprintf(buf1, "%22s", buf1);
      // else
       // sprintf(buf + strlen(buf), "    &W[%s]    ", clan[clan_num]->name);
		}
		
		
	sprintf(buf + strlen(buf), "&C%25s",buf1);
      
  
	     /* if (GET_PTITLE(tch))
       sprintf(buf + strlen(buf), "&c%s",GET_PTITLE(tch)); */
       sprintf(buf + strlen(buf), "&c%s", race_or_title(tch));

	  if (AFF_FLAGGED(tch, AFF_INVISIBLE))
	sprintf(buf+strlen(buf), " (невидим%s)", GET_CH_SUF_6(tch)); 
	  if (PLR_FLAGGED(tch, PLR_MAILING))
	strcat(buf, " (отправляет почту)");
      else
	  if (PLR_FLAGGED(tch, PLR_WRITING))
	strcat(buf, " (пишет)");
      if (PRF_FLAGGED(tch, PRF_DEAF))
	sprintf(buf+strlen(buf), " (глух%s)", GET_CH_SUF_1(tch));
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	sprintf(buf+strlen(buf), " (занят%s)", GET_CH_SUF_6(tch));
      if (PRF_FLAGGED(tch, PRF_QUEST))
	strcat(buf, " (на задании)");
	  if (PRF_FLAGGED(tch, PRF_AFK))
    strcat(buf, " [AFK]");
      if (PLR_FLAGGED(tch, PLR_THIEF))
	strcat(buf, " (ВОР)");
      if (PLR_FLAGGED(tch, PLR_KILLER))
	strcat(buf, " (УБИЙЦА)");
      if (AFF_FLAGGED(tch, AFF_CAMOUFLAGE))
    strcat(buf, " (в маскировке)");
     if (AFF_FLAGGED(tch, AFF_HIDE))
    strcat(buf, " (прячется)");
     
    strcat(buf, CCNRM(ch, C_SPR));
      strcat(buf, "\r\n");
     }				/* endif shortlist */
   }				/* end of for */
  
   if (low)
	{sprintf(buf1,"\r\n&C%36s&n\r\n", "Бессмертные");
	sprintf(buf1+strlen(buf1),"%29s-------------%s",CCGRN(ch, C_CMP),CCNRM(ch, C_CMP));
    send_to_char(buf1, ch);
	}

send_to_char(impl, ch); 
send_to_char(glgod, ch);
send_to_char(grgod, ch);
send_to_char(god, ch);
send_to_char(immort, ch);
send_to_char("\r\n", ch);


if (!who_room)
	{ if (short_list && (num_can_see % 4))
		  strcat(buf, "\r\n");
  
  if (!low)
	 strcat(buf, "\r\n\r\nВы не знаете, наблюдают ли за Вами Боги, но воззвать никто не запрещает!\r\n");
  else
	 sprintf(buf + strlen(buf),"\r\n\r\nБогов в Мире сейчас - %d\r\n", low);
	 
  if (num_can_see == 0)
     strcat(buf, "В Мире видимых жителей нет!\r\n");
  else if (num_can_see == 1)
     strcat(buf, "Сейчас в Мире находится один житель.\r\n"); 
  else
  sprintf(buf + strlen(buf), "Сейчас в Мире находятся %d видимых жител%s.\r\n", num_can_see,(num_can_see < 5 ? "я":"ей"));
  	}

    if (num_can_all > max_char_to_boot)
	   max_char_to_boot = num_can_all;
    if (!who_room)
    sprintf(buf + strlen(buf), "Максимальное количество жителей с момента перезагрузки %d.\r\n", max_char_to_boot);

   page_string(ch->desc,buf,1);

	//if((num_can_see && who_room) || low)	
	//send_to_char("\r\n", ch);
	if (!num_can_see && who_room && !low)
    send_to_char("Нет никого с таким именем!\r\n", ch);
}

#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p] [-d]\r\n"
#define MAX_LIST_LEN 200

ACMD(do_users)
{
  char  mode;
  char buf[MAX_INPUT_LENGTH];  
  char name_search[MAX_INPUT_LENGTH]="\0", host_search[MAX_INPUT_LENGTH]="\0";

  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = LVL_IMPL;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

  strcpy(buf, argument);
  while (*buf)
    {half_chop(buf, arg, buf1);
     if (*arg == '-')
        {mode = *(arg + 1);  
    
   switch (mode) {
    case 'o':
	    outlaws = 1;
	    playing = 1;
	    strcpy(buf, buf1);
	break;
    case 'p':
	    playing = 1;
	    strcpy(buf, buf1);
	break;
    case 'd':
	    deadweight = 1;
	    strcpy(buf, buf1);
	break;
    case 'l':
        if (!IS_GOD(ch)) 
 	        return;
	    playing = 1;
	    half_chop(buf1, arg, buf);
	    sscanf(arg, "%d-%d", &low, &high);
	break;
    case 'n':
	    playing = 1;
	    half_chop(buf1, name_search, buf);
	break;
    case 'h':
	    playing = 1;
	    half_chop(buf1, host_search, buf);
	break;
    case 'c':
	    playing = 1;
	    half_chop(buf1, arg, buf);
	    for (int i = 0; i < (int)strlen(arg); i++)
	        showclass |= find_class_bitvector(arg[i]);
	break;       

    default:
    	send_to_char(USERS_FORMAT, ch);
	return;
           }				// end of switch

     }
     else
     {  /* endif */
         strcpy(name_search, arg);
         strcpy(buf, buf1);
        }
  }				/* end while (parser) */
  
  int count = 0;

  TempBuffer tb;
  tb.append(" Num  LVL  Класс       Имя       Положение  Idl Логин    IP-адрес      E-mail\r\n");
  tb.append("----- --- -------- ------------ ----------- --- ----- --------------- ----------------\r\n"); 
  
  for (d = descriptor_list; d; d = d->next)
  {
       if (STATE(d) != CON_PLAYING && playing)
          continue;
       if (STATE(d) == CON_PLAYING && deadweight)
          continue;

       if (d->original)
          tch = d->original;
       else
          tch = d->character;

       bool playing = (tch && STATE(d) == CON_PLAYING);
       if (playing)
       {
          if (*host_search && !strstr(d->host, host_search))
             continue;
          if (*name_search && !isname(name_search, GET_NAME(tch)))
             continue;
          if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
             continue;
          if (outlaws && !PLR_FLAGGED((ch), PLR_KILLER))
             continue;
          if (showclass && !(showclass & (1 << GET_CLASS(tch))))
             continue;
          if (GET_INVIS_LEV(tch) > GET_LEVEL(ch))
             continue;
       }

       int sd = STATE(d);
       bool gen_pers = (!playing && tch && !GET_LEVEL(tch) && sd>CON_GET_NAME ) ? true : false;

       std::string state;
       if (playing)
       {
           sprintf(buf, "Room:%d",  GET_ROOM_VNUM(IN_ROOM(tch)));
           state.assign(buf);
       }
       else {
           if (gen_pers)
               state.assign("Новый перс");
           else
           {
               if (!tch || sd == CON_GET_NAME || sd == CON_PASSWORD)
                 state.assign("Авторизация");
               else
                 state.assign("Меню");
           }
       }

       std::string idletime;
       if (d->character && STATE(d) == CON_PLAYING && !IS_GOD(d->character))
       {
           char tmp[32];
           sprintf(tmp, "%d", d->character->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
           idletime.assign(tmp);
       }

       std::string logintime(asctime(localtime(&d->login_time)));
       logintime = logintime.substr(11, 5);

       std::string host;
       if (d->host && *d->host)
           host.assign(d->host);
       
       if (tch && sd != CON_GET_NAME) 
       {
           const char* c = CCGRN(ch, C_SPR);
           if (gen_pers)
               c = CCYEL(ch, C_SPR);
           else if (sd == CON_PASSWORD)
               c = CCCYN(ch, C_SPR);

           sprintf(buf, "%s%5d %3d %-8s %-12s %-11s %-3s %-5s %-15s %-15s%s\r\n", 
           (!playing)?c:"", d->desc_num, GET_LEVEL(tch), (!gen_pers) ? CLASS_ABBR(tch) : "",
           GET_NAME(tch), state.c_str(), idletime.c_str(), logintime.c_str(), host.c_str(), GET_EMAIL(tch), 
           (!playing)?CCNRM(ch, C_SPR):"");
       }
       else
       {
           sprintf(buf, "%s%5d %25s %-11s %-3s %-5s %-15s%s\r\n", 
           CCCYN(ch, C_SPR), d->desc_num, "", state.c_str(), idletime.c_str(), logintime.c_str(), host.c_str(),
           CCNRM(ch, C_SPR));
       }

       tb.append(buf);
       count++;
  }

  sprintf(buf, "\r\n%d видимых соединений.\r\n", count);
  tb.append(buf);
  page_string(ch->desc, tb, 1);  
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_ZONY:
    page_string(ch->desc, zony, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char("\033[H\033[J", ch);
    break;
  case SCMD_VERSION:
    send_to_char(strcat(strcpy(buf, circlemud_version), "\r\n"), ch);
    break;
  case SCMD_WHOAMI:
    sprintf(buf,
    "Падежи Вашего персонажа :\r\nИменительный - &C&1%s&n\r\nРодительный  - &C&1%s&n\r\nДательный    - &C&1%s&n\r\nВинительный  - &C&1%s&n\r\nТворительный - &C&1%s&n\r\nПредложный   - &C&1%s&n\r\n",
    GET_NAME(ch), GET_RNAME(ch), GET_DNAME(ch), GET_VNAME(ch), GET_TNAME(ch), GET_PNAME(ch));
    send_to_char(buf, ch);
    break;
  default:
    log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    return;
  }
}


void perform_mortal_where(struct char_data * ch, char *arg)
{

  sprintf(buf, "Вы находитесь в зоне: &C%s&n.\r\n", zone_table[world[ch->in_room].zone].name);
  send_to_char(buf, ch);
  
/*
 register struct char_data *i;
  register struct descriptor_data *d;

  if (!*arg) {
    send_to_char("Игроки в Вашей зоне\r\n--------------------\r\n", ch);
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (i->in_room == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[ch->in_room].zone != world[i->in_room].zone)
	continue;
      sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
      send_to_char(buf, ch);
    }
  } else {			// print only FIRST char, not all. 
    for (i = character_list; i; i = i->next) {
      if (i->in_room == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[i->in_room].zone != world[ch->in_room].zone)
	continue;
      if (!isname(arg, i->player.name))
	continue;
      sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
      send_to_char(buf, ch);
      return;
    }
    send_to_char("Здесь нет никого с таким именем.\r\n", ch); 
  }
  */
}


void print_object_location(int num, struct obj_data * obj, struct char_data * ch,
			        int recur)
{
  if (num > 0)
    sprintf(buf, "O%3d. %-25s - ", num, obj->short_description);
  else
    sprintf(buf, "%33s", " - ");

  if (obj->in_room > NOWHERE) {
    sprintf(buf + strlen(buf), "[%5d] %s\r\n",
	    GET_ROOM_VNUM(IN_ROOM(obj)), world[obj->in_room].name);
    send_to_char(buf, ch);
  } else if (obj->carried_by) {
    sprintf(buf + strlen(buf), "несет %s\r\n",
	    PERS(obj->carried_by, ch));
    send_to_char(buf, ch);
  } else if (obj->worn_by) {
    sprintf(buf + strlen(buf), "в экипировке у %s\r\n",
	    RPERS(obj->worn_by, ch));/*worn by */
    send_to_char(buf, ch);
  } else if (obj->in_obj) {
    sprintf(buf + strlen(buf), "лежит в %s%s\r\n",
	    obj->in_obj->short_description, (recur ? ", который находится" : " "));
    send_to_char(buf, ch);
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else {
    sprintf(buf + strlen(buf), "в неизвестном месте\r\n");
    send_to_char(buf, ch);
  }
}



void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {	  
    send_to_char("Игроки\r\n-------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (STATE(d) == CON_PLAYING) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
	  if (d->original)
	    sprintf(buf, "%-20s - [%5d] %s (in %s)\r\n",
		    GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
		 world[d->character->in_room].name, GET_NAME(d->character));
	  else
	    sprintf(buf, "%-20s - [%5d] %s\r\n", GET_NAME(i),
		    GET_ROOM_VNUM(IN_ROOM(i)), world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, i->player.name)) {
	found = 1;
	sprintf(buf, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
		GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
	send_to_char(buf, ch);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char("Не могу найти такую цель.\r\n", ch); 
  }
}



ACMD(do_where)
{
  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
  int i;

  if (IS_NPC(ch)) {
    send_to_char("Мобы не имеют уровней.\r\n", ch);
    return;
  }
  *buf = '\0';

  for (i = 1; i < LVL_IMMORT; i++) {
    sprintf(buf + strlen(buf), "[%2d] %8d-%-8d", i,
	    level_exp(GET_CLASS(ch), i), level_exp(GET_CLASS(ch), i+1) - 1);
       strcat(buf, "\r\n");
  }
  sprintf(buf + strlen(buf), "[%2d] %8d          : БЕССМЕРТНЫЙ\r\n",
	  LVL_IMMORT, level_exp(GET_CLASS(ch), LVL_IMMORT));
  page_string(ch->desc, buf, 1);
}



ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
    send_to_char("Сравнить с кем?\r\n", ch); 
    return;
  }
  if (victim == ch) {
    send_to_char("Простой, очень простой действительно!\r\n", ch); 
    return;
  }
  if (!IS_NPC(victim)) {
    send_to_char("Может Вам еще ключ от квартиры дать!\r\n", ch); 
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char("Где этот цыпленок?\r\n", ch); 
  else if (diff <= -5)
    send_to_char("Вы можете это сделать без усилий!\r\n", ch); 
  else if (diff <= -2)
    send_to_char("Легко.\r\n", ch); 
  else if (diff <= -1)
    send_to_char("Довольно легко.\r\n", ch); 
  else if (diff == 0)
    send_to_char("Примерное равенство.\r\n", ch);
  else if (diff <= 1)
    send_to_char("Вам должно повезти.\r\n", ch); 
  else if (diff <= 2)
    send_to_char("Вам необходимо много удачи!\r\n", ch); 
  else if (diff <= 3)
    send_to_char("Вам необходимо много удачи и хорошая экипировка тоже б не помешала!\r\n", ch);
  else if (diff <= 5)
    send_to_char("Пожалуй, это Вам не по зубам.\r\n", ch); 
  else if (diff <= 10)
    send_to_char("Вы СОШЛИ с ума!?\r\n", ch); 
  else if (diff <= 100)
   send_to_char("ВЫ СОШЛИ С УМА!\r\n", ch);

}



ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
      send_to_char(NOPERSON, ch);
    else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Диагностировать кого?\r\n", ch);
  }
}


const char *ctypes[] = {
  "выключен", "средний", "нормальный", "полный", "\n"
};

ACMD(do_color)
{
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "Ваш текущий цветовой уровень %s.\r\n", ctypes[COLOR_LEV(ch)]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char("Usage: Цвет { Выключен | Нормальный | Полный }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1);
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_2);

  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)));
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_2 * (tp & 2) >> 1));
  sprintf(buf, "Теперь у Вас %sцвет%s %s.\r\n", CCRED(ch, C_SPR),
	  CCNRM(ch, C_OFF), ctypes[tp]);
  send_to_char(buf, ch);
}

const char *gen_tog_type[] =
{"автовыходы",//   "autoexits",
 "краткий",//      "brief",
 "компактный",//   "compact",
 "цвет",//         "color",
 "повтор",//       "norepeat",
 "сказать",//    "notell",
 "болтать",//      "nogossip",
 "кричать",//      "noshout",
 "орать",//        "noholler",
 "оффтопик",
 "аукцион",// "noauction",
 "квестер",//      "quest",
 "автозаучивание",//"automem",
 "сжатие",        //compress  
 "автосжатие",    //autocompresss
 "агро",		//режим агро пк
 "направление", 
 "мирный",//    "nohassle",
 "призвать", //    "nosummon",
 "частный", //     "nowiz",
 "флаги",  //      "roomflags",
 "замедление", //  "slowns",
 "выслеживание",// "trackthru",
 "всевидение"  , //"holylight",
 "кодер",        //"coder",
 "ГА",//"goahead",
 "бдисплей",
 "следование",
 "noexp",
 "журналы",
 "\n"
};

struct gen_tog_param_type
{ int level;
  int subcmd;
} gen_tog_param[] =
{ {0,   SCMD_AUTOEXIT},
  {0,   SCMD_BRIEF},
  {0,   SCMD_COMPACT},
  {0,   SCMD_COLOR},
  {0,   SCMD_NOREPEAT},
  {0,   SCMD_NOTELL},
  {0,   SCMD_NOGOSSIP},
  {0,   SCMD_NOSHOUT},
  {0,   SCMD_NOHOLLER},
  {0,   SCMD_NOGRATZ}, //оффтопик
  {0,   SCMD_NOAUCTION},
  {0,   SCMD_QUEST},
  {0,   SCMD_AUTOMEM},
  {0,   SCMD_COMPRESS},
  {0,   SCMD_AUTOZLIB},
  {0,   SCMD_AGRO},
  {0,   SCMD_DIRECTION},	
  {LVL_IMMORT, SCMD_NOHASSLE},
  {LVL_GRGOD, SCMD_NOSUMMON},
  {LVL_GOD,   SCMD_NOWIZ},
  {LVL_IMMORT,SCMD_ROOMFLAGS},
  {LVL_IMPL,  SCMD_SLOWNS},
  {LVL_GOD,   SCMD_TRACK},
  {LVL_GOD,   SCMD_HOLYLIGHT},
  {LVL_GOD,   SCMD_CODERINFO},
  {0,         SCMD_GOAHEAD},
  {0,	      SCMD_DISPHP}, //боевой дисплей
  {0,	      SCMD_NOFOLLOW},
  {0,	      SCMD_NOEXP},
  {0,	      SCMD_JOURNAL}
};

ACMD(do_toggle)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];	
  int i;
  *arg1 = '\0';

	 if (IS_NPC(ch))
    return;
 
  half_chop(argument, arg, arg1);
  
   if (!str_cmp(arg, "трус"))
		{ do_wimpy(ch, arg1, 0, 0);
			return;
		}
	
    if (!str_cmp(arg, "знак"))
	{ do_razd(ch, arg1, 0, 0);
		    return;
	}

  if (*arg)
	{ if ((i = search_block(arg, gen_tog_type, FALSE)) < 0 ||
	       GET_LEVEL(ch) < gen_tog_param[i].level   /*       ||
		!GET_COMMSTATE(ch)*/)
		{ send_to_char("Я не знаю такого режима!\r\n", ch);
			return;
		}
	  do_gen_tog(ch, arg1, 0, gen_tog_param[i].subcmd);
	  return;
	}
  
	
	
	if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "&RНЕТ&n");
  else
    sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

  if (GET_LEVEL(ch) >= LVL_IMMORT)
     {sprintf(buf,
	  " Мирный         : %-3s    "
	  " Всевидение     : %-3s    "
	  " Флаги комнат   : %-3s\r\n",
	ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
	ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
	ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS))
    );
      send_to_char(buf, ch);
     }

  sprintf(buf,
	  " Сказать        : %-3s    "
	  " Краткий режим  : %-3s    "
	  " Орать          : %-3s\r\n"

	  " Компакт. режим : %-3s    "
	  " Болтать        : %-3s    "
	  " Повтор команд  : %-3s\r\n"

	  " Аукцион        : %-3s    "
	  " Автовыходы     : %-3s    "
	  " Кричать        : %-3s\r\n"
	  	  
	  " Квестер        : %-3s    "
	  " Автозаучивание : %-3s    "
	  " Оффтопик       : %-3s\r\n"
         
      " Сжатие         : %-3s    "
      " Автосжатие     : %-3s    "
	  " Магический знак: %-3s\r\n"
	  
	  " Трусость       : %-3s    "
      " Режим ГА       : %-3s    "
      " Бдисплей       : %-3s\r\n"
      		  
      " Агро           : %-3s    "
      " Направление    : %-3s    "    
      " Цвет           : %-3s\r\n"

      " Не следовать   : %-3s    "
	  " Режим noexp    : %-3s    "
      " Журналы        : %s\r\n",
      

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOTELL)),
	  ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_DEAF)),

	  ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	  YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
      ONOFF(!PRF_FLAGGED(ch, PRF_NOSHOUT)),

      YESNO(PRF_FLAGGED(ch, PRF_QUEST)),
      ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)),
      ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),
#if defined(HAVE_ZLIB)
	  ch->desc->deflate == NULL ? "&RНЕТ&n   " : (ch->desc->mccp_version == 2 ? "&CMCCPv2&n" : "&CMCCPv1&n"),
	  YESNO(PRF_FLAGGED(ch, PRF_AUTOZLIB)),
#else
	  "N/A","N/A",
#endif
	  ch->player_specials->saved.spare0,
	  buf2,
	  ONOFF(PLR_FLAGGED(ch, PLR_GOAHEAD)),
      ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
	  ONOFF(PRF_FLAGGED(ch, PRF_AGRO)),
      ONOFF(PRF_FLAGGED(ch, PRF_DIRECTION)),
	  ctypes[COLOR_LEV(ch)], 
      ONOFF(PLR_FLAGGED(ch, PLR_NOFOLLOW)),
	  ONOFF(PLR_FLAGGED(ch, PLR_NOEXP)),
      (PRF_FLAGGED(ch, PRF_JOURNAL)) ? "в обратном порядке" : "по порядку"
      );

  send_to_char(buf, ch);
}

struct sort_struct {
  int sort_pos;
  byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
  int a, b, tmp;

  num_of_cmds = 0;

  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;

  // create data array
  CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

  // initialize it
  for (a = 1; a < num_of_cmds; a++) {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = FALSE;// (cmd_info[a].command_pointer == do_action);
  }

  // the infernal special case 
  cmd_sort_info[find_command("insult")].is_social = TRUE;

  // Sort.  'a' starts at 1, not 0, to remove 'RESERVED'
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
	tmp = cmd_sort_info[a].sort_pos;
	cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
	cmd_sort_info[b].sort_pos = tmp;
      }
}



ACMD(do_commands)
{
  int no, i, cmd_num, num_of;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)) || IS_NPC(vict)) {
      send_to_char("Кто это?\r\n", ch);
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char("Вы не можете использовать команды игроков выше Вашего уровня.\r\n", ch);
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  sprintf(buf, "Следующие %s%s доступны %s:\r\n",
	  wizhelp ? "привелегированные " : "",
	  socials ? "социалы" : "комманды",
	  vict == ch ? "Вам" : GET_NAME(vict));

    if (socials)
     num_of = top_of_socialk+1;
  else
     num_of = num_of_cmds - 1;

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = socials ? 0 : 1; cmd_num < num_of; cmd_num++)
      if (socials)
         {sprintf(buf + strlen(buf), "%-19s", soc_keys_list[cmd_num].keyword);
          if (!(no % 5))
	         strcat(buf, "\r\n");
          no++;
         }
      else
         {i = cmd_sort_info[cmd_num].sort_pos;
          if (cmd_info[i].minimum_level >= 0 &&
	 GET_LEVEL(vict) >= cmd_info[i].minimum_level &&
	(cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp &&
	(wizhelp || socials == cmd_sort_info[i].is_social)) {
		if (*cmd_info[i].command <= 0 && SCMD_COMMANDS == subcmd) { 
      sprintf(buf + strlen(buf), "%-13s", cmd_info[i].command);
      if (!(no % 7))
		  strcat(buf, "\r\n");
      no++;}
if (*cmd_info[i].command > 1 && SCMD_COMMANDS1 == subcmd) { 
      sprintf(buf + strlen(buf), "%-13s", cmd_info[i].command);
      if (!(no % 7))
		  strcat(buf, "\r\n");
      no++;}
if (wizhelp || socials) { 
      sprintf(buf + strlen(buf), "%-13s", cmd_info[i].command);
      if (!(no % 7))
		  strcat(buf, "\r\n");
      no++;}
    }
  
  } 

  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}

ACMD(do_sides)
{
  int  i;

  if (!ch->desc)
     return;

  if (GET_POS(ch) <= POS_SLEEPING)
     send_to_char("Вы не можете этого делать во сне!\r\n", ch);
  else
  if (AFF_FLAGGED(ch, AFF_BLIND))
     send_to_char("Вы слепы и ничего не можете видеть!\r\n", ch);
  else
	{ send_to_char("Вы посмотрели по сторонам.\r\n", ch);
		for (i=0; i < NUM_OF_DIRS; i++)
			look_in_direction(ch, i, 0);
     }
}

ACMD(do_scan)
{
	do_sides(ch,argument,cmd,subcmd);
  /*
  int door;
  const char *dirs_3[] = {
       "На севере: ",
      "На востоке: ",
	  "На юге: ",
       "На западе: ",
	  "Вверху: ",
	   "Внизу: "
  };

  *buf = '\0';
  
  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("Вы слепы и ничего не можете видеть!\r\n", ch);
    return;
  }
  // Оглянувшись, можно увидеть кто находится в соседних комнатах
  send_to_char("Вы быстро оглянулись и увидели:\r\n", ch);
    for (door = 0; door < NUM_OF_DIRS; door++)
	{ if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	  !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
        if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) &&
			GET_LEVEL(ch) <= LVL_IMMORT)  
		    continue;
			sprintf(buf,"&n%14s", dirs_3[door]);
		  if(IS_DARK(EXIT(ch, door)->to_room) && GET_LEVEL(ch) <= LVL_IMMORT){
         strcat(buf, "&Kслишком темно&n\r\n");
       send_to_char(buf, ch);
	 continue;
	}
	 else {strcat(buf, "&Kничего особенного&n\r\n"); //world[EXIT(ch, door)->to_room].name
		//	buf+count, "&C%s&w\r\n",world[rdata->to_room].name
	 }
       send_to_char(buf, ch);
       send_to_char("&R", ch);
       // смотрим кто в соседних комнатах.
	   list_char_to_char(world[EXIT(ch, door)->to_room].people, ch);
        send_to_char("&n", ch);
      } 
    }       
*/	
}

