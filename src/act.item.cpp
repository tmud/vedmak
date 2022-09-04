/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
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

/* extern variables */
extern room_rnum donation_room_1;
#if 0
extern room_rnum donation_room_2;  /* uncomment if needed! */
extern room_rnum donation_room_3;  /* uncomment if needed! */
#endif
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern const int material_value[];
extern const char *weapon_affects[];
extern struct char_data *mob_proto;


void go_create_weapon(struct char_data *ch, struct obj_data *obj, int obj_type, int skill);
int  invalid_anti_class(struct char_data *ch, struct obj_data *obj);

/* local functions */
int  preequip_char(struct char_data *ch, struct obj_data *obj, int where);
void postequip_char(struct char_data *ch, struct obj_data *obj);
int curbid = 0;				/* current bid on item being auctioned */
struct obj_data *obj_selling = NULL;	/* current object for sale */
struct char_data *ch_selling = NULL;	/* current character selling obj */
struct char_data *ch_buying  = NULL;	/* current character buying the object */
int can_take_obj(struct char_data * ch, struct obj_data * obj);
const char* get_check_money(struct char_data * ch, struct obj_data * obj);
void get_money(struct char_data * ch, struct obj_data * obj);
int perform_get_from_room(struct char_data * ch, struct obj_data * obj);
void get_from_room(struct char_data * ch, char *arg, int amount);
void perform_give_gold(struct char_data * ch, struct char_data * vict, int amount);
void perform_give(struct char_data * ch, struct char_data * vict, struct obj_data * obj);
int perform_drop(struct char_data * ch, struct obj_data * obj, byte mode, const int sname, room_rnum RDR);
void perform_drop_gold(struct char_data * ch, int amount, byte mode, room_rnum RDR);
struct char_data *give_find_vict(struct char_data * ch, char *arg);
void weight_change_object(struct obj_data * obj, int weight);
void perform_put(struct char_data * ch, struct obj_data * obj, struct obj_data * cont);
void name_from_drinkcon(struct obj_data * obj);
void get_from_container(struct char_data * ch, struct obj_data * cont, char *arg, int mode, int amount);
void name_to_drinkcon(struct obj_data * obj, int type);
void wear_message(struct char_data * ch, struct obj_data * obj, int where);
void perform_wear(struct char_data * ch, struct obj_data * obj, int where);
int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
void perform_get_from_container(struct char_data * ch, struct obj_data * obj, struct obj_data * cont, int mode);
void perform_remove(struct char_data * ch, int pos);
void auc_send_to_all(char *messg, bool buyer);

ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_upgrade);

void perform_put(struct char_data * ch, struct obj_data * obj,
		      struct obj_data * cont)
{
      if (!drop_otrigger(obj, ch))
		 return;

	if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0))
    act("$o �� ���������� � $8.", FALSE, ch, obj, cont, TO_CHAR);
  else 
	if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
     act("�� �� ������ �������� ��������� � ���������.",FALSE,ch,0,0,TO_CHAR);
  else
  if (IS_OBJ_STAT(obj, ITEM_NODROP))
     act("��� - �� �� ���� ��� �������� $3 � $8.",FALSE,ch,obj,cont,TO_CHAR);
  else
  {
    obj_from_char(obj);
    obj_to_obj(obj, cont);

    act("$n �������$y $3 � $8.", TRUE, ch, obj, cont, TO_ROOM);

    // Yes, I realize this is strange until we have auto-equip on rent. -gg 
    if (IS_OBJ_STAT(obj, ITEM_NODROP) && !IS_OBJ_STAT(cont, ITEM_NODROP))
    {   SET_BIT(GET_OBJ_EXTRA(cont, ITEM_NODROP), ITEM_NODROP);
        act("�� ������� �����-�� ��������������, ����� �������� $3 � $8.", FALSE,
                ch, obj, cont, TO_CHAR);
    } else
      act("�� �������� $3 � $8.", FALSE, ch, obj, cont, TO_CHAR);
  }
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
  char	 arg1[MAX_INPUT_LENGTH];
  char	 arg2[MAX_INPUT_LENGTH];
  char	 arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj, *cont;
  struct char_data *tmp_char;
  int	 obj_dotmode, cont_dotmode, found = 0, howmany = 1, money_mode=FALSE;
  char	 *theobj, *thecont, *theplace;
  int    where_bits = FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;

  argument = two_arguments(argument, arg1, arg2);
  one_argument(argument, arg3);

  if (*arg3 && is_number(arg1)) {
    howmany = atoi(arg1);
    theobj = arg2;
    thecont = arg3;
    theplace = argument;
  } else {
    theobj = arg1;
    thecont = arg2;
    theplace = arg3;
  }
  if (isname(theplace, "����� ������� room ground"))
     where_bits = FIND_OBJ_ROOM;
  else
  if (isname(theplace, "��������� inventory"))
     where_bits = FIND_OBJ_INV;
  else
  if (isname(theplace, "���������� equipment"))
     where_bits = FIND_OBJ_EQUIP;
  
  if (theobj &&
      (!strn_cmp("�����",theobj,5) ||
       !strn_cmp("�����",theobj,5)))
      { money_mode = TRUE;
        if (howmany <= 0)
           {send_to_char("�� ������ ������� �����.\r\n",ch);
            return;
           }
        if (GET_GOLD(ch) < howmany)
           {send_to_char("� ��� ��� ������� �����.\r\n",ch);
            return;
           }
        obj_dotmode = FIND_INDIV;
      }
  else
    obj_dotmode = find_all_dots(theobj);
  
  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
    send_to_char("�������� ��� � ����?\r\n", ch);
  else if (cont_dotmode != FIND_INDIV)
    send_to_char("�� ������ ������������ ������ ���� ������ � ���� ���������!\r\n", ch);
  else if (!*thecont) {
    sprintf(buf, "��� �� ������ �������� %s���?\r\n",
	    ((obj_dotmode == FIND_INDIV) ? "� " : ""));
    send_to_char(buf, ch);
  } else {
    generic_find(thecont, where_bits, ch, &tmp_char, &cont);
    if (!cont) {
      sprintf(buf, "�� �� ������ ����� %s.\r\n", thecont);
      send_to_char(buf, ch);
    } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
      act("�� ������ �� ������ �������� $3 - ��� �� ���������", FALSE, ch, cont, 0, TO_CHAR);
    else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
      act("�� ����� �� ������� ������� $3!", FALSE, ch, cont, 0, TO_CHAR);
	else {
      if (obj_dotmode == FIND_INDIV) {	/* put <obj> <container> */
	if (!(obj = get_obj_in_list_vis(ch, theobj, ch->carrying))) {
	  sprintf(buf, "�� �� ������ � ����� %s.\r\n", theobj); 
	  send_to_char(buf, ch);
	} else if (obj == cont)
	  send_to_char("���������, ��� ��� � ��� ���������?\r\n", ch);
	else {
	  struct obj_data *next_obj;
	  while(obj && howmany--) {
	    next_obj = obj->next_content;
	    perform_put(ch, obj, cont);
	    obj = get_obj_in_list_vis(ch, theobj, next_obj);
	  }
	}
      } else {
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
	      (obj_dotmode == FIND_ALL || isname(theobj, obj->name))) {
	    found = 1;
	    perform_put(ch, obj, cont);
	  }
	}
	if (!found) {
	  if (obj_dotmode == FIND_ALL)
	    send_to_char("�� ������ �� ������ �������� �������, ��� ��� ��� ������ ���!\r\n", ch); 
	  else {
	    sprintf(buf, "�� ������ �� ������, ��� �� ���������� %s.\r\n", theobj);
	    send_to_char(buf, ch);
			}
		}
      }
    }
  }
}


int can_take_obj(struct char_data * ch, struct obj_data * obj)
{
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    act("$o: �� �� ������ ����� ������� ����� ���������.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
     act("$o: �� �� ������ ����� ���!", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
     act("$o: �� �� ������ ����� ����� �������.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  return (1);
}


const char* get_check_money(struct char_data * ch, struct obj_data * obj)
{
  int value = GET_OBJ_VAL(obj, 0);
  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return "";
  if (value == 1)
    sprintf(buf2, "��� ���� ���� ������.");
  else {
	sprintf(buf2, "� ���������� %d %s.", value, desc_count(value,WHAT_MONEYa));
  }
  return buf2;
}

void get_money(struct char_data * ch, struct obj_data * obj)
{
  int value = GET_OBJ_VAL(obj, 0);
  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return;

  obj_from_char(obj);
  extract_obj(obj);
  GET_GOLD(ch) += value;
}

void perform_get_from_container(struct char_data * ch, struct obj_data * obj,
				     struct obj_data * cont, int mode)
{
  if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      act("$o: �� �� ������ ������� ������ ���������.", FALSE, ch, obj, 0, TO_CHAR);
    else if (get_otrigger(obj, ch))
	{
      obj_from_obj(obj);
      obj_to_char(obj, ch);
	  if (obj->carried_by == ch) 
      {
          sprintf(buf, "�� ����� $3 �� $6. %s",  get_check_money(ch, obj));
          act(buf, FALSE, ch, obj, cont, TO_CHAR);
          act("$n ����$y $3 �� $6.",FALSE, ch, obj, cont, TO_ROOM);
          SET_BIT(GET_OBJ_EXTRA(obj, ITEM_LIFTED), ITEM_LIFTED);
          get_money(ch, obj);
      }
    }
  }
}


void get_from_container(struct char_data * ch, struct obj_data * cont,
			     char *arg, int mode, int howmany)
{
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(arg);

  if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
    act("$o �������.", FALSE, ch, cont, 0, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, arg, cont->contains))) {
      sprintf(buf, "�����, ������� ��� %s � $5.", arg);
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
    } else {
      struct obj_data *obj_next;
      while(obj && howmany--) {
        obj_next = obj->next_content;
        perform_get_from_container(ch, obj, cont, mode);
        obj = get_obj_in_list_vis(ch, arg, obj_next);
      }
    }
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*arg) {
      send_to_char("����� �ӣ ������?\r\n", ch);
      return;
    }
    for (obj = cont->contains; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (obj_dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_container(ch, obj, cont, mode);
      }
    }
    if (!found) {
      if (obj_dotmode == FIND_ALL)
	act("$o, �������, ������.", FALSE, ch, cont, 0, TO_CHAR); 
      else 	
	act("�� �� ������ ����� ����� � $5.", FALSE, ch, cont, 0, TO_CHAR);
     }
  }
}


int perform_get_from_room(struct char_data * ch, struct obj_data * obj)
{
  if (can_take_obj(ch, obj) && get_otrigger(obj, ch)) {
		obj_from_room(obj);
			obj_to_char(obj, ch);
			if (obj->carried_by == ch) 
            {
                sprintf(buf, "�� ����� $3. %s",  get_check_money(ch, obj));
                act(buf, FALSE, ch, obj, 0, TO_CHAR);
                act("$n ����$y $3.",FALSE, ch, obj, 0, TO_ROOM);
                SET_BIT(GET_OBJ_EXTRA(obj, ITEM_LIFTED), ITEM_LIFTED);
                get_money(ch, obj);
                return (1);
			}
  }
  return (0);
}


void get_from_room(struct char_data * ch, char *arg, int howmany)
{
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;

  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents))) {
      sprintf(buf, "�� �� ������ ����� %s.\r\n", arg); 
      send_to_char(buf, ch);
    } else {
      struct obj_data *obj_next;
      while(obj && howmany--) {
	obj_next = obj->next_content;
        perform_get_from_room(ch, obj);
        obj = get_obj_in_list_vis(ch, arg, obj_next);
      }
    }
  } else {
    if (dotmode == FIND_ALLDOT && !*arg) {
      send_to_char("����� ��� ������?\r\n", ch); 
      return;
    }
    for (obj = world[ch->in_room].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_room(ch, obj);
      }
    }
    if (!found) {
      if (dotmode == FIND_ALL)
	send_to_char("�������, ����� ������ ���.\r\n", ch);
      else {
	sprintf(buf, "�� �� ������ ������, ��� �� ���������� %s.\r\n", arg);
	send_to_char(buf, ch);
      }
    }
  }
}

ACMD(do_mark)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  int    cont_dotmode, found = 0, mode;
  struct obj_data  *cont;
  struct char_data *tmp_char;

  argument = two_arguments(argument, arg1, arg2);

  if (!*arg1)
     send_to_char("��� �� ������ ��������?\r\n", ch);
  else
  if (!*arg2 || !is_number(arg2))
     send_to_char("�� ������ ��� �������� ������.\r\n", ch);
  else
     {cont_dotmode = find_all_dots(arg1);
      if (cont_dotmode == FIND_INDIV)
         {mode = generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
          if (!cont)
             {sprintf(buf, "� ��� ��� '%s'.\r\n", arg1);
              send_to_char(buf, ch);
	      return;
             }
          cont->obj_flags.Obj_owner = atoi(arg2);
          act("�� �������� $o3.",FALSE,ch,cont,0,TO_CHAR);
         }
      else
         {if (cont_dotmode == FIND_ALLDOT && !*arg1)
             {send_to_char("�������� ��� \"���\" ?\r\n", ch);
              return;
             }
          for (cont = ch->carrying; cont; cont = cont->next_content)
              if (CAN_SEE_OBJ(ch, cont) &&
                  (cont_dotmode == FIND_ALL || isname(arg1, cont->name))
		 )
	         {cont->obj_flags.Obj_owner = atoi(arg2);
	          act("�� ������� ����� �� $3.",FALSE,ch,cont,0,TO_CHAR);
		  found = TRUE;
	         }
          for (cont = world[ch->in_room].contents; cont; cont = cont->next_content)
              if (CAN_SEE_OBJ(ch, cont) &&
                  (cont_dotmode == FIND_ALL || isname(arg2, cont->name))
		 )
	         {cont->obj_flags.Obj_owner = atoi(arg2);
	          act("�� ������� ����� �� $3.",FALSE,ch,cont,0,TO_CHAR);
		  found = TRUE;
	         }
          if (!found)
             {if (cont_dotmode == FIND_ALL)
                 send_to_char("� ��� ��� ��������, ��� �� ��� ��������.\r\n", ch);
              else
                 {sprintf(buf, "�� �� ������ ����� '%s'.\r\n", arg1);
                  send_to_char(buf, ch);
                 }
             }
         }
     }
}


ACMD(do_get)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char *theplace;
  int cont_dotmode, found = 0, mode;
  int  where_bits = FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP;
  struct obj_data *cont;
  struct char_data *tmp_char;

  argument = two_arguments(argument, arg1, arg2);
  one_argument(argument, arg3);
  theplace = argument;
  
 if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    send_to_char("���� ���� ��� ������!\r\n", ch);
  else if (!*arg1)
    send_to_char("����� ���?\r\n", ch);
  else if (!*arg2)
    get_from_room(ch, arg1, 1);
  else if (is_number(arg1) && !*arg3)
    get_from_room(ch, arg2, atoi(arg1));
  else {
    int amount = 1;
    if (is_number(arg1)) {
      amount = atoi(arg1);
      strcpy(arg1, arg2);
      strcpy(arg2, arg3);
    }
    else
         theplace = arg3;
      if (isname(theplace, "����� ������� room ground"))
         where_bits = FIND_OBJ_ROOM;
      else
      if (isname(theplace, "��������� inventory"))
         where_bits = FIND_OBJ_INV;
      else
      if (isname(theplace, "���������� equipment"))
         where_bits = FIND_OBJ_EQUIP;
	
	cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV) {
      mode = generic_find(arg2, where_bits, ch, &tmp_char, &cont);
      if (!cont) {
	sprintf(buf, "�� �� ������ %s.\r\n", arg2);
	send_to_char(buf, ch);
      } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
	act("$o - ��� �� ���������.", FALSE, ch, cont, 0, TO_CHAR);
      else
	get_from_container(ch, cont, arg1, mode, amount);
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2) {
	send_to_char("����� �ӣ �� ����?\r\n", ch);
	return;
      }
      for (cont = ch->carrying; cont && IS_SET(where_bits, FIND_OBJ_INV); cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    found = 1;
	    get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    found = 1;
	    act("$o - ��� �� ���������.", FALSE, ch, cont, 0, TO_CHAR);
	  }
	}
      for (cont = world[ch->in_room].contents; cont &&
		   IS_SET(where_bits, FIND_OBJ_ROOM); cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
	    found = 1;
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    act("$o - ��� �� ���������.", FALSE, ch, cont, 0, TO_CHAR);
	    found = 1;
	  }
	}
      if (!found) {
	if (cont_dotmode == FIND_ALL)
	  send_to_char("�� �� ������ ����� ����� ���������.\r\n", ch); 
	else {
	  sprintf(buf, "�� ������ �� ������ �����, ��� ���������� %s.\r\n", arg2);
	  send_to_char(buf, ch);
	}
      }
    }
  }
}


void perform_drop_gold(struct char_data * ch, int amount,
		            byte mode, room_rnum RDR)
{
	struct obj_data *obj;


  if (amount <= 0)
    send_to_char("��� ���.. ��� �� ������ �������?\r\n", ch);
  else if (GET_GOLD(ch) < amount)
    send_to_char("� ��� ��� ������� �������� �����!\r\n", ch);
  else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      obj = create_money(amount);
      if (mode == SCMD_DONATE) {
	send_to_char("�� ������� ��������� ����� � ������, ������� ������� � ������ ����!\r\n", ch);
	act("$n ������$y ��������� ����� � ������, ������� ������� � ������ ����!",
	    FALSE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, RDR);
	//act("$o �������� �����$Q � ������ ���������� ����!", ch, 0, obj, 0, TO_ROOM); 
      } else  {
			  if (!drop_wtrigger(obj, ch)) {
			    extract_obj(obj);
                  return;
					 }

	send_to_char("�� ������� ��������� �����.\r\n", ch);
	sprintf(buf, "$n ������$y %s.", money_desc(amount, 3));
	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, ch->in_room);
      }
    } else {
      sprintf(buf, "$n ������$y %s, ������� ������� � ������ ����!",
	      money_desc(amount, 3));
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("�� ������� ��������� ����� � ������, ������� ������� � ������ ����!\r\n", ch);
    }
    GET_GOLD(ch) -= amount;
  }
}


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      ", $E �����$Q � ������ ����!" : ".")

const char *drop_op[3][3] =
{
 {"���������",   "���������",   "��������"},
 {"�������� � ���","�������� � ���","�������$y � ���"},
 {"�������",     "�������",     "������$y"}
};


int perform_drop(struct char_data * ch, struct obj_data * obj,
		     byte mode, const int sname, room_rnum RDR)
{
  int value;
  
	if (!drop_otrigger(obj, ch))
		 return 0;
    if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
         return 0;

 if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
    sprintf(buf, "�� �� ������ %s $3, ��� ��������!", drop_op[sname][0]);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } //����������� �� ����������
     if (IS_OBJ_STAT(obj, ITEM_DECAY)) {
	    act("�� ������� $3, �����$W ��������$U � ����, ������������� � �����.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n ������$y $3, �����$W, ������������� � �����, ��������$U � ����.", TRUE, ch, obj, 0, TO_ROOM);
           extract_obj(obj);
     return (0);
 }
  sprintf(buf, "�� %s $3%s", drop_op[sname][1], VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_CHAR);
  sprintf(buf, "$n %s $3%s", drop_op[sname][2], VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_ROOM);
  SET_BIT(GET_OBJ_EXTRA(obj, ITEM_LIFTED), ITEM_LIFTED);  
  obj_from_char(obj);
  

  if ((mode == SCMD_DONATE) && IS_OBJ_STAT(obj, ITEM_NODONATE))
    mode = SCMD_JUNK;

  switch (mode) {
  case SCMD_DROP:
    obj_to_room(obj, ch->in_room);
    return (0);
  case SCMD_DONATE:
    act("$o �������� ���������$U � ������ ����!", FALSE, 0, obj, 0, TO_ROOM);
    REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODECAY), ITEM_NODECAY);
    obj_to_room(obj, RDR);
	obj_decay(obj);
    return (0);
  case SCMD_JUNK:
    value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
    extract_obj(obj);
    return (value);
  default:
    log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
    break;
  }

  return (0);
} 

ACMD(do_drop)
{
  struct obj_data *obj, *next_obj;
  room_rnum RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode, amount = 0, multi;
  int sname;

  switch (subcmd) {
  case SCMD_JUNK:
    sname = 0;
    mode = SCMD_JUNK;
    break;
  case SCMD_DONATE:
    sname = 1;
    mode = SCMD_DONATE;
    switch (number(0, 2)) {
    case 0:
      mode = SCMD_JUNK;
      break;
    case 1:
    case 2:
      RDR = real_room(donation_room_1);
      break;
/*    case 3: RDR = real_room(donation_room_2); break;
      case 4: RDR = real_room(donation_room_3); break;
*/
    }
    if (RDR == NOWHERE) {
      send_to_char("��������, �� �� ������ ������ �������� � ��� �����.\r\n", ch);
      return;
    }
    break;
  default:
    sname = 2;
    break;
  }

  argument = one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "��� �� ������ %s?\r\n", drop_op[sname][0]);
    send_to_char(buf, ch);
    return;
  } else 
    if (is_number(arg)) {
    multi = atoi(arg);
    one_argument(argument, arg);
    if (!str_cmp(arg, "�����"))
      perform_drop_gold(ch, multi, mode, RDR);

    else if (multi <= 0)
      send_to_char("� ���� �� � ���� �����?\r\n", ch);
    else if (!*arg) {
      sprintf(buf, "��� �� ������ %s %d?\r\n", drop_op[sname][0], multi);
      send_to_char(buf, ch);
    } else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
      sprintf(buf, "�� ������� �� ��������� ��� ����, ������ ��� � ��� ������������!\r\n");
      send_to_char(buf, ch);
    } else {
      do {
        next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
        amount += perform_drop(ch, obj, mode, sname, RDR);
        obj = next_obj;
      } while (obj && --multi);
    }
  } else {
    dotmode = find_all_dots(arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
      if (subcmd == SCMD_JUNK)
	send_to_char("�� ���� ������� �������� ����!\r\n", ch);
      else
	send_to_char("����� � ������� ��� �������������, ���� �� ������ �������� � ��� ��!\r\n", ch);
      return;
    }
    if (dotmode == FIND_ALL) {
      if (!ch->carrying)
	send_to_char("� ��� ������ ���.\r\n", ch); 
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  amount += perform_drop(ch, obj, mode, sname, RDR);
	}
    } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
	sprintf(buf, "��� \"���\" %s?\r\n", drop_op[sname][0]);
	send_to_char(buf, ch);
	return;
      }
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf, "� � ��� ��� ����� ���������.\r\n");
	send_to_char(buf, ch);
      }
      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
	amount += perform_drop(ch, obj, mode, sname, RDR);
	obj = next_obj;
      }
    } else {
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
 	    send_to_char("� ��� ����� ���.\r\n", ch);
      } else
	    amount += perform_drop(ch, obj, mode, sname, RDR);
    }
  }

  if (amount && (subcmd == SCMD_JUNK)) {
    send_to_char("���� ������ �� ���� ������� ������!\r\n", ch);
    act("$d ���� �������� � �������������� �� �����!", TRUE, ch, 0, 0, TO_ROOM);
    //GET_GOLD(ch) += amount;
  }
}


void perform_give(struct char_data * ch, struct char_data * vict,
		       struct obj_data * obj)
{
  if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
    act("�� �� ������ ������ $3! ��� ��������!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
    act("� � $R ������ ����.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
    act("$E �� ����� ������� �����.", FALSE, ch, 0, vict, TO_CHAR); 
    return;
  }
  if (!give_otrigger(obj, ch, vict) || !receive_mtrigger(vict, ch, obj))
    return;
  
  act("�� ���� $3 $D.", FALSE, ch, obj, vict, TO_CHAR); 
  act("$n ���$y ��� $3.", FALSE, ch, obj, vict, TO_VICT);
  act("$n ���$y $3 $D.",FALSE, ch, obj, vict, TO_NOTVICT);
  SET_BIT(GET_OBJ_EXTRA(obj, ITEM_LIFTED), ITEM_LIFTED);
  obj_from_char(obj);
  obj_to_char(obj, vict);
}

/* utility function for give */
struct char_data *give_find_vict(struct char_data * ch, char *arg)
{
  struct char_data *vict;

  if (!*arg) {
    send_to_char("����?\r\n", ch);
    return (NULL);
  } else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    send_to_char(NOPERSON, ch);
    return (NULL);
  } else if (vict == ch) {
    send_to_char("���� ����? �� ����� �������� ���� � ������� � ���������� �������� :)\r\n", ch);
    return (NULL);
  } else
    return (vict);
}


void perform_give_gold(struct char_data * ch, struct char_data * vict,
		            int amount)
{
  if (amount <= 0) {
    send_to_char("��-��-��.. ��� ������! � ��� �� �� ������?\r\n", ch);
    return;
  }
  if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || !IS_IMPL(ch))) {
    send_to_char("�� � ������ �� ������� � �������! ��� �� ������ ������� �����!!!\r\n", ch);
    return;
  }
  send_to_char("OK\r\n", ch);
  sprintf(buf, "$n ���$y ��� %d %s.", amount, desc_count(amount, WHAT_MONEYu));
  act(buf, FALSE, ch, 0, vict, TO_VICT);
  sprintf(buf, "$n ���$y %s $D.", money_desc(amount, 3));
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
    GET_GOLD(ch) -= amount;
  GET_GOLD(vict) += amount;
   
  bribe_mtrigger(vict, ch, amount);
}


ACMD(do_give)
{
  int amount, dotmode, count=0;
  struct char_data *vict;
  struct obj_data *obj, *next_obj;

  argument = one_argument(argument, arg);

  if (!*arg)
    send_to_char("���� ��� � ����?\r\n", ch);
  else if (is_number(arg)) {
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!str_cmp(arg, "�����") || !str_cmp(arg, "�����")) {
      one_argument(argument, arg);

      if ((vict = give_find_vict(ch, arg)) != NULL)
	perform_give_gold(ch, vict, amount);
      return;
    } else if (!*arg) {	/* Give multiple code. */
      sprintf(buf, "��� �� ������ ���� %d ?\r\n", amount);
      send_to_char(buf, ch);
    } else if (!(vict = give_find_vict(ch, argument))) {
      return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
      sprintf(buf, "� ��� ������ ���, ��� �� ���������� %s.\r\n", arg);
      send_to_char(buf, ch);
    } else {
      while (obj && amount--) {
	next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
	perform_give(ch, vict, obj);
	obj = next_obj;
      }
    }
  } else {
    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf, "� ��� ����� ���.\r\n"); 
	send_to_char(buf, ch);
      } else
	perform_give(ch, vict, obj);
    } else {
      if (dotmode == FIND_ALLDOT && !*arg) {
	send_to_char("��� �� ����?\r\n", ch);
	return;
      }
      if (!ch->carrying)
	send_to_char("��, �������, ������ �� �������.\r\n", ch);
      else
	{ for (obj = ch->carrying; obj; obj = next_obj)
	  {    next_obj = obj->next_content;
	    if (CAN_SEE_OBJ(ch, obj) && (dotmode == FIND_ALL || isname(arg, obj->name)))
			{ perform_give(ch, vict, obj);
			  count=1;  
			}
	  }
		if (!count)
        send_to_char("� ���, �������, ����� ���.\r\n", ch);
      }
    }
  }
}



void weight_change_object(struct obj_data * obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (obj->in_room != NOWHERE) {
    GET_OBJ_WEIGHT(obj) += weight;
  } else if ((tmp_ch = obj->carried_by)) {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
  } else if ((tmp_obj = obj->in_obj)) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  } else {
    log("SYSERR: Unknown attempt to subtract weight from an object.");
  }
}



/*void name_from_drinkcon(struct obj_data * obj)
{
  int i;
  char *new_name;

  for (i = 0; (*((obj->name) + i) != ' ') && (*((obj->name) + i) != '\0'); i++);

  if (*((obj->name) + i) == ' ') {
    new_name = str_dup((obj->name) + i + 1);
    if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
      free(obj->name);
    obj->name = new_name;
  }
}



void name_to_drinkcon(struct obj_data * obj, int type)
{
  char *new_name;

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  sprintf(new_name, "%s %s", drinknames[type], obj->name);
  if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);
  obj->name = new_name;
}
   */

void name_from_drinkcon(struct obj_data * obj)
{
  int  i,j=0;
  char new_name[MAX_STRING_LENGTH];

 
/*for (i = 0; (*((obj->name) + i) != ' ') && (*((obj->name) + i) != '\0'); i++);

  if (*((obj->name) + i) == ' ') {
    sprintf(new_name, "%s", str_dup((obj->name) + i + 1));
	 if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
      free(obj->name);
    obj->name = str_dup(new_name);
  }*/

  for (i = 0; *(obj->name + i) && is_space(*(obj->name + i)); i++);
  for (j = 0; *(obj->name + i) && !(is_space(*(obj->name + i)));
       new_name[j] = *(obj->name + i), i++, j++);
       new_name[j] = '\0';
  if (*new_name)
     {if (GET_OBJ_RNUM(obj) < 0 ||
          obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
         free(obj->name);
      obj->name = str_dup(new_name);
     }
       
        //sprintf(new_name, "%s", obj->short_description);
  for (i = 0; *(obj->short_description + i) && is_space(*(obj->short_description + i)); i++);
  for (j = 0; *(obj->short_description + i) && !(is_space(*(obj->short_description + i)));
       new_name[j] = *(obj->short_description + i), i++, j++);
       new_name[j] = '\0';
  
      if (*new_name)
          {if (GET_OBJ_RNUM(obj) < 0 ||
               obj->short_description != obj_proto[GET_OBJ_RNUM(obj)].short_description)
              free(obj->short_description);
              obj->short_description = str_dup(new_name);
          }
      
       
    //	sprintf(new_name, "%s", obj->short_rdescription);
  for (i = 0; *(obj->short_rdescription + i) && is_space(*(obj->short_rdescription + i)); i++);
  for (j = 0; *(obj->short_rdescription + i) && !(is_space(*(obj->short_rdescription + i)));
       new_name[j] = *(obj->short_rdescription + i), i++, j++);
       new_name[j] = '\0';
	    if (*new_name)
          {if (GET_OBJ_RNUM(obj) < 0 ||
               obj->short_rdescription != obj_proto[GET_OBJ_RNUM(obj)].short_rdescription)
              free(obj->short_rdescription);
              obj->short_rdescription = str_dup(new_name);
          }
     

	//		sprintf(new_name, "%s", obj->short_ddescription);
  for (i = 0; *(obj->short_ddescription + i) && is_space(*(obj->short_ddescription + i)); i++);
  for (j = 0; *(obj->short_ddescription + i) && !(is_space(*(obj->short_ddescription + i)));
       new_name[j] = *(obj->short_ddescription + i), i++, j++);
       new_name[j] = '\0';
			if (*new_name)
          {if (GET_OBJ_RNUM(obj) < 0 ||
               obj->short_ddescription != obj_proto[GET_OBJ_RNUM(obj)].short_ddescription)
              free(obj->short_ddescription);
              obj->short_ddescription = str_dup(new_name);
          }

    
	//		sprintf(new_name, "%s", obj->short_tdescription);
  for (i = 0; *(obj->short_tdescription + i) && is_space(*(obj->short_tdescription + i)); i++);
  for (j = 0; *(obj->short_tdescription + i) && !(is_space(*(obj->short_tdescription + i)));
       new_name[j] = *(obj->short_tdescription + i), i++, j++);
       new_name[j] = '\0';
			if (*new_name)
          {if (GET_OBJ_RNUM(obj) < 0 ||
               obj->short_tdescription != obj_proto[GET_OBJ_RNUM(obj)].short_tdescription)
              free(obj->short_tdescription);
              obj->short_tdescription = str_dup(new_name);
          }

	
	//		sprintf(new_name, "%s", obj->short_vdescription);
  for (i = 0; *(obj->short_vdescription + i) &&  is_space(*(obj->short_vdescription + i)); i++);
  for (j = 0; *(obj->short_vdescription + i) &&  !(is_space(*(obj->short_vdescription + i)));
       new_name[j] = *(obj->short_vdescription + i), i++, j++);
       new_name[j] = '\0';
			if (*new_name)
          {if (GET_OBJ_RNUM(obj) < 0 ||
               obj->short_vdescription != obj_proto[GET_OBJ_RNUM(obj)].short_vdescription)
              free(obj->short_vdescription);
              obj->short_vdescription = str_dup(new_name);
          }

    
    //		sprintf(new_name, "%s", obj->short_pdescription);
  for (i = 0; *(obj->short_pdescription + i) && is_space(*(obj->short_pdescription + i)); i++);
  for (j = 0; *(obj->short_pdescription + i) && !(is_space(*(obj->short_pdescription + i)));
       new_name[j] = *(obj->short_pdescription + i), i++, j++);
       new_name[j] = '\0';
			if (*new_name)
          {if (GET_OBJ_RNUM(obj) < 0 ||
               obj->short_pdescription != obj_proto[GET_OBJ_RNUM(obj)].short_pdescription)
              free(obj->short_pdescription);
              obj->short_pdescription = str_dup(new_name);
          }
      
}



void name_to_drinkcon(struct obj_data * obj, int type)
{ 
  char new_name[MAX_INPUT_LENGTH];

  sprintf(new_name, "%s %s", obj->name, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
     free(obj->name);
  obj->name = str_dup(new_name);

  sprintf(new_name, "%s c %s", obj->short_description, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->short_description != obj_proto[GET_OBJ_RNUM(obj)].short_description)
     free(obj->short_description);
  obj->short_description = str_dup(new_name);


  sprintf(new_name, "%s c %s", obj->short_rdescription, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->short_rdescription != obj_proto[GET_OBJ_RNUM(obj)].short_rdescription)
     free(obj->short_rdescription);
  obj->short_rdescription = str_dup(new_name);

  sprintf(new_name, "%s c %s", obj->short_ddescription, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->short_ddescription != obj_proto[GET_OBJ_RNUM(obj)].short_ddescription)
     free(obj->short_ddescription);
  obj->short_ddescription = str_dup(new_name);

  sprintf(new_name, "%s c %s", obj->short_vdescription, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->short_vdescription != obj_proto[GET_OBJ_RNUM(obj)].short_vdescription)
     free(obj->short_vdescription);
  obj->short_vdescription = str_dup(new_name);

  sprintf(new_name, "%s c %s", obj->short_tdescription, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->short_tdescription != obj_proto[GET_OBJ_RNUM(obj)].short_tdescription)
     free(obj->short_tdescription);
  obj->short_tdescription = str_dup(new_name);

  sprintf(new_name, "%s c %s", obj->short_pdescription, drinknames[type]);
  if (GET_OBJ_RNUM(obj) < 0 || obj->short_pdescription != obj_proto[GET_OBJ_RNUM(obj)].short_pdescription)
     free(obj->short_pdescription);
  obj->short_pdescription = str_dup(new_name);
  
 
}



ACMD(do_drink)
{ // ����������� � ���������� �� ������� � ���������� ����������� �������
  struct obj_data *temp;
  struct affected_type af;
  int amount, weight, duration;
  int on_ground = 0;

  one_argument(argument, arg);

  /*if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
   /* return;*/
//��� ����� �� ����� ��� ����!!!
  if (FIGHTING(ch))
  { send_to_char("�� �� ������ ���� �� ����� ���!\r\n", ch);
	return;
  }

  if (!*arg) {
    send_to_char("���� ���?\r\n", ch);
    return;
  }
  if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    if (!(temp = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents))) {
      send_to_char("�� �� ������ ����� �����!\r\n", ch); 
      return;
    } else
      on_ground = 1;
  }
  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
    send_to_char("�� �� ������ ���� �� �����!\r\n", ch); 
    return;
  }
  if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
    send_to_char("��� �� ������, ������ � ����� ���������!\r\n", ch);
    return;
  }
  if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
    /* The pig is drunk */
    send_to_char("�� �� ������ ���������� �� ���!!!\r\n", ch); 
    act("$n �������$u ������, �� ���������$u ���� ���!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
 // if (/*(GET_COND(ch, FULL) > 20) &&*/ (GET_COND(ch, THIRST) == 24)) {
 //   send_to_char("��� ����� �� ����� ��������� ������� ��������!\r\n", ch);
 //   return;
 // }
  if (!GET_OBJ_VAL(temp, 1)) {
    send_to_char("�����.\r\n", ch);
    return;
  }

 if (subcmd == SCMD_DRINK)
     {if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
         amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
      else
         amount = number(3, 10);
     }
  else
     {amount = 1;
     }

  amount = MIN(amount, GET_OBJ_VAL(temp, 1));
  amount = MIN(amount, 24 - GET_COND(ch,THIRST));

  if (amount <= 0)
     {send_to_char("�� ������ �� ������ ����!\r\n", ch);
      return;
     }
  else
  if (subcmd == SCMD_DRINK)
     {sprintf(buf, "$n �����$y %s �� $1.", drinks[GET_OBJ_VAL(temp, 2)]);
    act(buf, TRUE, ch, temp, 0, TO_ROOM);

    sprintf(buf, "�� ������ %s �� $1.", drinks[GET_OBJ_VAL(temp, 2)]); 
    act(buf, TRUE, ch, temp, 0, TO_CHAR);
     }
  else
     {act("$n ������$y ������ �� $1.", TRUE, ch, temp, 0, TO_ROOM);
    sprintf(buf, "�� ����������� %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
    send_to_char(buf, ch);
     }
 

  /* You can't subtract more than the object weighs */
  weight = MIN(amount, GET_OBJ_WEIGHT(temp));

    if (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)
  weight_change_object(temp, -weight);	/* Subtract amount */

  gain_condition(ch, DRUNK,
	 (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount) / 4);

  gain_condition(ch, FULL,
	  (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount) / 4);

  gain_condition(ch, THIRST,
	(int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount) / 4);

  
  if (GET_COND(ch, THIRST) > 20)
    send_to_char("�� �� ���������� ������ �����.\r\n", ch);

  if (GET_COND(ch, FULL) > 20)
    send_to_char("��� �� ������ �� �����, �� �����.\r\n", ch); 

  if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
     {if (GET_COND(ch,DRUNK) >= CHAR_MORTALLY_DRUNKED)
         {send_to_char("�� �����!\r\n", ch);
          duration = 2;
         }
      else
         {send_to_char("�� ������� ������ ���������!\r\n", ch);
          duration = 2 + MAX(0, GET_COND(ch,DRUNK) - CHAR_DRUNKED);
         }
      GET_DRUNK_STATE(ch) = MAX(GET_DRUNK_STATE(ch),GET_COND(ch, DRUNK));
      if (!AFF_FLAGGED(ch, AFF_DRUNKED) && !AFF_FLAGGED(ch, AFF_ABSTINENT))
         {send_to_char("�������� ������ ��� � ������.\r\n", ch);
          /***** Decrease AC ******/
          af.type      = SPELL_DRUNKED;
          af.duration  = pc_duration(ch,duration,0,0,0,0);
          af.modifier  = -20;
          af.location  = APPLY_AC;
          af.bitvector = AFF_DRUNKED;
          af.battleflag= 0;
          affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
          /***** Decrease HR ******/
          af.type      = SPELL_DRUNKED;
          af.duration  = pc_duration(ch,duration,0,0,0,0);
          af.modifier  = -2;
          af.location  = APPLY_HITROLL;
          af.bitvector = AFF_DRUNKED;
          af.battleflag= 0;
          affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
          /***** Increase DR ******/
          af.type      = SPELL_DRUNKED;
          af.duration  = pc_duration(ch,duration,0,0,0,0);
          af.modifier  = (GET_LEVEL(ch) + 4) / 5;
          af.location  = APPLY_DAMROLL;
          af.bitvector = AFF_DRUNKED;
          af.battleflag= 0;
          affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
         }
     }
  
  if (GET_OBJ_VAL(temp, 3) && !IS_GOD(ch)) { 
    send_to_char("���... ��� �� ���� �������� ��������!\r\n", ch); 
    act("$n ��������$q �������� �������� �����.", TRUE, ch, 0, 0, TO_ROOM);
    amount = MIN(amount, GET_OBJ_VAL(temp, 3)); // ������� � 1 �� 3, � ��������� ������� ���
      
	af.type      = SPELL_POISON;
    af.duration  = pc_duration(ch,amount == 1 ? amount : amount * 3,0,0,0,0);
    af.modifier  = -2;
    af.location  = APPLY_STR;
    af.bitvector = AFF_POISON;
    af.battleflag= 0;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
    af.type      = SPELL_POISON;
    af.duration  = pc_duration(ch,amount == 1 ? amount : amount * 2,0,0,0,0);
    af.modifier  = amount * 3;
    af.location  = APPLY_POISON;
    af.bitvector = AFF_POISON;
    af.battleflag= 0;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
    ch->Poisoner = 0;
  }
  /* empty the container, and no longer poison. */
  if (GET_OBJ_TYPE(temp)   != ITEM_FOUNTAIN ||
      GET_OBJ_VAL(temp, 1) != 999)
     GET_OBJ_VAL(temp, 1) -= amount;
  if (!GET_OBJ_VAL(temp, 1))
     {/* The last bit */
      GET_OBJ_VAL(temp, 2) = 0;
      GET_OBJ_VAL(temp, 3) = 0;
      name_from_drinkcon(temp);
     }
  return;
}



ACMD(do_eat)
{
  struct obj_data *food;
  struct affected_type af;
  int amount;

  one_argument(argument, arg);

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
    return;

  //��� ����� �� ����� ��� �����!!!
  if (FIGHTING(ch))
  { send_to_char("�� ���������! �� ���������� �� ���� �����!\r\n", ch);
	return;
  }

  if (!*arg) {
    send_to_char("��� �� ������ ������?\r\n", ch); 
    return;
  }
  if (!(food = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "� ��� ��� %s.\r\n", arg); 
    send_to_char(buf, ch);
    return;
  }
  if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			       (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
    do_drink(ch, argument, 0, SCMD_SIP);
    return;
  }
  if ((GET_OBJ_TYPE(food) != ITEM_FOOD) &&
	  !IS_GOD(ch)						&&
	  GET_OBJ_TYPE(food) != ITEM_NOTE)
		{ send_to_char("�� �� ������ ���� ���!\r\n", ch); 
			return;
		}

  if (GET_COND(ch, FULL) > 20 && GET_OBJ_TYPE(food) != ITEM_NOTE) {
    send_to_char("�� ���������� ����, ����� ��� ����!\r\n", ch); 
    return;
  }
  if (subcmd == SCMD_EAT) {
    act("�� ����� $3.", FALSE, ch, food, 0, TO_CHAR);
    act("$n ����$y $3.", TRUE, ch, food, 0, TO_ROOM);
  } else {
    act("�� ����������� $3.", FALSE, ch, food, 0, TO_CHAR); 
    act("$n ����������$y $3.", TRUE, ch, food, 0, TO_ROOM); 
  }

  amount = ((subcmd == SCMD_EAT && GET_OBJ_TYPE(food) != ITEM_NOTE) ?
								  GET_OBJ_VAL(food, 0) : 1);

  gain_condition(ch, FULL, amount);

  if (GET_COND(ch, FULL) > 20)
    send_to_char("�� ������ �� ������ ����.\r\n", ch); 
 
	// ���� 3, ������� �� 1 - � ��������� ��� ������� ��� ��� ���
  if (GET_OBJ_VAL(food, 1) && !IS_IMMORTAL(ch)) {
    /* The shit was poisoned ! */
      amount = GET_OBJ_VAL(food, 1);
	  send_to_char("���..., ��� �� ���� ���-�� ��������!\r\n", ch);
    act("$n �����$y ���������� � �������� �������� �����.", FALSE, ch, 0, 0, TO_ROOM); 

      af.type      = SPELL_POISON;
      af.duration  = pc_duration(ch,amount == 1 ? amount : amount * 2,0,0,0,0);
      af.modifier  = -1;
      af.location  = APPLY_STR;
      af.bitvector = AFF_POISON;
      af.battleflag= 0;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      af.type      = SPELL_POISON;
      af.duration  = pc_duration(ch,amount == 1 ? amount : amount * 2,0,0,0,0);
      af.modifier  = amount * 3;
      af.location  = APPLY_POISON;
      af.bitvector = AFF_POISON;
      af.battleflag= 0;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      ch->Poisoner = 0;
     }
  if (subcmd == SCMD_EAT)
    extract_obj(food);
  else {
    if (!(--GET_OBJ_VAL(food, 0))) {
      send_to_char("� ��� ��� ������ ������.\r\n", ch);
      extract_obj(food);
    }
  }
}

ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = NULL, *to_obj = NULL;
  int amount;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR) {
    if (!*arg1) {		// No arguments
      send_to_char("�� ���� �� ������ ������?\r\n", ch); 
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      send_to_char("�� �� ������ ����� ���.\r\n", ch); 
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
      send_to_char("�� �� ������ ������ ���.\r\n", ch);
      return;
    }
  }
  if (subcmd == SCMD_FILL) {
    if (!*arg1) {		// no arguments
      send_to_char("��� �� ������ ��������� � �� ����?\r\n", ch); 
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      send_to_char("�� �� ������ ����� ���!\r\n", ch); 
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
      act("�� �� ������ ��������� $3!", FALSE, ch, to_obj, 0, TO_CHAR); 
      return;
    }
    if (!*arg2) {		// no 2nd argument
      act("��� �� ������ ��������� �� $1?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents))) {
      sprintf(buf, "�������, ����� ��� %s.\r\n", arg2);
      send_to_char(buf, ch);
      return;
    }
    
	if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
	{ act("�� �� ������ ��������� �� $1.", FALSE, ch, from_obj, 0, TO_CHAR);
		return;
    }
  }
  
  if (GET_OBJ_VAL(from_obj, 1) == 0)
	{ act("$o - ����$Y.", FALSE, ch, from_obj, 0, TO_CHAR);
		return;
	}

  if (subcmd == SCMD_POUR) {
    if (!*arg2) {
      send_to_char("��� �� ������ ������? ������ � ����?\r\n", ch); 
      return;
    }
    if (!str_cmp(arg2, "�����")) {
      act("$n �����$y ���������� �� $1 �� �����.", TRUE, ch, from_obj, 0, TO_ROOM);
      act("�� ������ ���������� �� $1 �� �����.", FALSE, ch, from_obj, 0, TO_CHAR); 

      weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1));

      GET_OBJ_VAL(from_obj, 1) = 0;
      GET_OBJ_VAL(from_obj, 2) = 0;
      GET_OBJ_VAL(from_obj, 3) = 0;
      name_from_drinkcon(from_obj);

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
      send_to_char("�� �� ������ ����� ���!\r\n", ch);
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
	(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
      send_to_char("�� �� ������ ������ � ���.\r\n", ch);
      return;
    }
  }
  if (to_obj == from_obj) {
    send_to_char("��� ���������� �������.\r\n", ch);
    return;
  }
  if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
      (GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))) {
    send_to_char("����� ��� ��������� ������ ���������!\r\n", ch); 
    return;
  }
  if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0))) {
    send_to_char("����� ��� ����� ��� ���� ������ �ݣ.\r\n", ch);
    return;
  }
  if (subcmd == SCMD_POUR) {
   sprintf(buf, "�� ������ %s �� %s � %s.\r\n",
	    drinks[GET_OBJ_VAL(from_obj, 2)], from_obj->short_rdescription, to_obj->short_vdescription);
          send_to_char(buf, ch);
  }
  if (subcmd == SCMD_FILL) {
    act("�� ����������� ��������� $3 �� $6.", FALSE, ch, to_obj, from_obj, TO_CHAR);
    act("$n ����������� ��������$y $3 �� $6.", TRUE, ch, to_obj, from_obj, TO_ROOM);
  }

  
    // First same type liq.
  GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);
  
  // ����� ��� 
  if (GET_OBJ_VAL(to_obj, 1) == 0)
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));


  // Then how much to pour 
  if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN ||
      GET_OBJ_VAL(from_obj,1) != 999)
     GET_OBJ_VAL(from_obj, 1) -= (amount =
			 (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));
    else
      amount = GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1);

  GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

    // Then the poison boogie 
  GET_OBJ_VAL(to_obj, 3) = (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));


  if (GET_OBJ_VAL(from_obj, 1) < 0) {	// There was too little 
    GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
    amount += GET_OBJ_VAL(from_obj, 1);
    GET_OBJ_VAL(from_obj, 1) = 0;
    GET_OBJ_VAL(from_obj, 2) = 0;
    GET_OBJ_VAL(from_obj, 3) = 0;
    name_from_drinkcon(from_obj);
  }

  // And the weight boogie 
  if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
		weight_change_object(from_obj, -amount);
  
  weight_change_object(to_obj, amount);
}




void wear_message(struct char_data * ch, struct obj_data * obj, int where)
{
 static const char *wear_messages[][2] = {
    {"$n ��������$y $3 � ����$y �� ������ ����.",
    "�� ��������� $3 � ����� �� ������ ����."},           

	{"$n �����$y $3 �� ���� ������.",
    "�� ������ $3 �� ������."}, 
    
	{"$n ����$y $3 �� ���� �����.",
    "�� ����� $3 �� �����."}, 
	
	{"$n �������$y $3 � ���� ������ ���.",
    "�� �������� $3 � ���� ������ ���."},
	
	{"$n �������$y $3 � ���� ����� ���.",
    "�� �������� $3 � ���� ����� ���."}, 

	{"$n �����$y $3 ������ ����� ���.",
    "�� ������ $3 ������ ���."}, 

    {"$n �����$y $3 �� �����.",
    "�� ������ $3 �� �����."},

	{"$n ����$y $3 �� ���� �����.",
    "�� ����� $3 �� �����."}, 

    {"$n �����$y $3 �� ����.",
    "�� ������ $3 �� ����."},

	{"$n �������$y $3 �� �����.",
    "�� �������� $3 �� �����."},

    {"$n �����$y $3 ������ ����� �����.",
    "�� ������ $3 ������ ����� �����."},

    {"$n ����$y $3 �� ���� ����.",
    "�� ����� $3 �� ���� ����."},
	
	{"$n �����$y $3 �� ������ ��������.",
    "�� ������ $3 �� ������ ��������."}, 

    {"$n �����$y $3 �� ����� ��������.",
    "�� ������ $3 �� ����� ��������."},

	{"$n �����$y $3 �� ����� ���.",
    "�� ������ $3 �� ����� ���."},

	{"$n �����$y $3 �� ����� ������ ����.",
    "�� ������ $3 �� ����� ������ ����."},

    {"$n �����$y $3 �� ����� ����� ����.",
    "�� ������ $3 �� ����� ����� ����."}, 
    
    {"$n ����$y $3 �� ���� ����.",
    "�� ����� $3 �� ����."}, 

    {"$n ����$u � $3.",
    "�� ������� � $3."},

    {"$n �����$y ������������ $3 ��� ���.",
    "�� ������ ������������ $3 ��� ���."},

    {"$n ��������$u $4.",
    "�� ����������� $4."},

    {"$n ����$y $3 � ����� ����.",
    "�� ����� $3 � ����� ����."},

	{"$n ����$y $3 � ��� ����.",
	"�� ����� $3 � ��� ����." }
  };

  act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
  act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}



void perform_wear(struct char_data * ch, struct obj_data * obj, int where)
{
  /*
   * ITEM_WEAR_TAKE is used for objects that do not require special bits
   * to be put into that position (e.g. you can hold any object, not just
   * an object with a HOLD bit.)
   */

  int wear_bitvectors[] =
  { ITEM_WEAR_TAKE,  ITEM_WEAR_HEAD,   ITEM_WEAR_EYES,   ITEM_WEAR_EAR,
    ITEM_WEAR_EAR,   ITEM_WEAR_NECK,   ITEM_WEAR_NECK,   ITEM_WEAR_BACKPACK,
	ITEM_WEAR_BODY,  ITEM_WEAR_ABOUT,  ITEM_WEAR_WAIST,  ITEM_WEAR_HANDS,
	ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,  ITEM_WEAR_ARMS,   ITEM_WEAR_FINGER,
	ITEM_WEAR_FINGER,ITEM_WEAR_LEGS,   ITEM_WEAR_FEET,   ITEM_WEAR_SHIELD,
    ITEM_WEAR_WIELD, ITEM_WEAR_TAKE,   ITEM_WEAR_BOTHS
  };

  static const char *already_wearing[] = {
    "�� ��� ����������� ���������.\r\n",             
    "�� ����� ������ ��� ���-�� ������.\r\n",
    "�� ���� ����� ��� ���-�� �����.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "� ������ ��� ��� ���-�� ����.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "� ��� ��� ���-�� ������ �� ���.\r\n",    
    "� ��� ��� ���-�� ��������� �� ������.\r\n",
    "�� ����� ���� ��� ���-�� ������.\r\n",  
    "�� ���� ����� ��� ���-�� ���������.\r\n",  
    "�� ����� ����� ��� ���-�� ������.\r\n", 
    "�� ���� ���� ��� ���-�� ������.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "�� ����� �������� ��� ���-�� ������.\r\n", 
    "�� ������ ����� ��� ��� ���-�� ������.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "�� ����� ������� ��� ���-�� ������.\r\n", 
    "�� ����� ����� ��� ���-�� ������.\r\n", 
    "�� ��� � ��� �� ���-�� �����.\r\n",
    "�� ��� ����������� ���.\r\n",               
    "�� ��� ���-�� ���������.\r\n",             
    "�� ��� ���-�� �������.\r\n",                 
    "�� ��� ���-�� ������� � ���� �����.\r\n"
  };

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where])) {
    act("�� �� ������ ����� $3 �� ��� ����� ����.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  
/* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ( /* �� ����� ������� ���� ���� ���� ��� ��������� */
      (where == WEAR_HOLD && (GET_EQ(ch,WEAR_BOTHS) || GET_EQ(ch,WEAR_LIGHT) ||
                              GET_EQ(ch,WEAR_SHIELD))) ||
       /* �� ����� ����������� ���� ���� ��������� */
      (where == WEAR_WIELD && GET_EQ(ch,WEAR_BOTHS)) ||
       /* �� ����� ������� ��� ���� ���-�� ������ ��� ��������� */
      (where == WEAR_SHIELD && (GET_EQ(ch,WEAR_HOLD) || GET_EQ(ch,WEAR_BOTHS))) ||
      /* �� ����� ��������� ���� ���� ���, ����, �������� ��� ������ */
      (where == WEAR_BOTHS && (GET_EQ(ch,WEAR_HOLD) || GET_EQ(ch,WEAR_LIGHT) ||
                               GET_EQ(ch,WEAR_SHIELD) || GET_EQ(ch,WEAR_WIELD))) ||
      /* �� ����� ������� ���� ���� ��������� ��� ������ */
      (where == WEAR_LIGHT && (GET_EQ(ch,WEAR_HOLD) || GET_EQ(ch,WEAR_BOTHS)))
     )
     {send_to_char("� ��� ������ ����.\r\n",ch);
      return;
     }

    
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) ||
	  (where == WEAR_WRIST_R)  || (where == WEAR_EAR_R))
    if (GET_EQ(ch, where))
      where++;

  if (GET_EQ(ch, where)) {
    send_to_char(already_wearing[where], ch);
    return;
  }
  if (!wear_otrigger(obj, ch, where))
     return;
  
  obj_from_char(obj);
  if (preequip_char(ch,obj,where) && obj->worn_by == ch)
     {wear_message(ch,obj,where);
      postequip_char(ch,obj);
     }
}



int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg)
{
  int where = -1;

  /* \r to prevent explicit wearing. Don't use \n, it's end-of-array marker. */
  static const char *keywords[] = {
    "\r!RESERVED!",//0
    "������",
    "�����",
    "������ ���",
    "����� ���",
    "���",			//5
    "�����",
    "�����",
    "����",
    "�����",
    "�����",
    "����",			//11
    "\r!RESERVED!",
    "��������",
    "�����",    
    "\r!RESERVED!",
    "�����", 
    "����", 
    "�����",
    "���",
    "����������",
    "���������",
    "�������",
    "\n"
  };

  if (!arg || !*arg) {
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
    if (CAN_WEAR(obj, ITEM_WEAR_EAR))		  where = WEAR_EAR_R;
	if (CAN_WEAR(obj, ITEM_WEAR_EYES))		  where = WEAR_EYES;
	if (CAN_WEAR(obj, ITEM_WEAR_BACKPACK))	  where = WEAR_BACKPACK; 
  } else {
    if ((where = search_block(arg, keywords, FALSE)) < 0 ||
         (*arg=='!')) 
	{ sprintf(buf, "'%s'? ��� ��� �� ����� ������ ����?\r\n", arg);
      send_to_char(buf, ch);
         return -1;
    }
  }

  return (where);
}



ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;
  
  if (IS_NPC(ch) && !NPC_FLAGGED(ch, NPC_ARMORING))
	  return;


  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char("����� ���?\r\n", ch);
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV)) {
    send_to_char("�� �� ������ ������ ������ �����!\r\n", ch);
    return;
  }
  if (dotmode == FIND_ALL) {
    for (obj = ch->carrying; obj && !GET_MOB_HOLD(ch) &&!AFF_FLAGGED(ch, AFF_HOLDALL)  && GET_POS(ch) > POS_SLEEPING; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
	items_worn++;
	perform_wear(ch, obj, where);
      }
    }
    if (!items_worn)
      send_to_char("� ��� ������ ���, ��� ����� ���� �� �����.\r\n", ch);
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) {
      send_to_char("����� ��� �� ���?\r\n", ch); 
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      sprintf(buf, "�������, ����� ��� %s.\r\n", arg1);
      send_to_char(buf, ch);
    } else
      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg1, obj->next_content);
	if ((where = find_eq_pos(ch, obj, 0)) >= 0)
	  perform_wear(ch, obj, where);
	else
	  act("�� �� ������ ����������� $4.", FALSE, ch, obj, 0, TO_CHAR);
	obj = next_obj;
      }
  } else {
    if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
         send_to_char("� ��� ����� ���! \r\n", ch);
    } else {
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
	perform_wear(ch, obj, where);
      else if (!*arg2)
	act("�� �� ������ ����� $3.", FALSE, ch, obj, 0, TO_CHAR); /*You can't wear*/
    }
  }
}

ACMD(do_wield)
{
  struct obj_data *obj;
  int    wear;

  if (IS_NPC(ch) && !NPC_FLAGGED(ch, NPC_WIELDING))
     return;

  one_argument(argument, arg);

  if (!*arg)
     send_to_char("����������� ���?\r\n", ch);
  else
     if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
        {sprintf(buf, "� ��� ����� ���!\r\n", arg);
         send_to_char(buf, ch);
        }
     else
        {if (!CAN_WEAR(obj, ITEM_WEAR_WIELD) && !CAN_WEAR(obj, ITEM_WEAR_BOTHS))
           act("�� �� ������ ����������� $4!", FALSE, ch, obj, 0, TO_CHAR);  
		 else
         if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
            act("$o - ��� �� ������!", FALSE, ch, obj, 0, TO_CHAR);
			  else
            { if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
                 wear = WEAR_WIELD;
              else
                 wear = WEAR_BOTHS;
              /* This is too high
              if (GET_OBJ_SKILL(obj) == SKILL_BOTHHANDS ||
                  GET_OBJ_SKILL(obj) == SKILL_BOWS)
                 wear = WEAR_BOTHS;
               */
              if (!(GET_EQ(ch, WEAR_WIELD)||GET_EQ(ch, WEAR_BOTHS)) && wear == WEAR_WIELD && !IS_IMMORTAL(ch) && !OK_WIELD(ch,obj))
                 {act("�� �� ������ �������� $3 � ������ ����.",
                      FALSE,ch,obj,0,TO_CHAR);
                  if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
                     wear = WEAR_BOTHS;
                  else
                     return;
                 }
              if (!(GET_EQ(ch, WEAR_BOTHS)||GET_EQ(ch, WEAR_HOLD)) && wear == WEAR_BOTHS && !IS_IMMORTAL(ch) && !OK_BOTH(ch,obj))
                 {act("�� �� ������ �������� $3 � ���� �����.",
                      FALSE,ch,obj,0,TO_CHAR);
                  return;
                 };
              perform_wear(ch, obj, wear);
             }
        }
}

ACMD(do_grab)
{
	int  where = WEAR_HOLD;
   struct obj_data *obj;

   one_argument(argument, arg);
   
   if (IS_NPC(ch) && !NPC_FLAGGED(ch, NPC_WIELDING))
     return;

    if (!*arg)
    send_to_char("������� ���?\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
      send_to_char("� ��� ��� �����! \r\n", ch);
  } else {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
    {     if (affected_by_spell(ch, SPELL_ZNAK_IRGEN))
	    { affect_from_char(ch, SPELL_ZNAK_IRGEN);
		  send_to_char("�� ���������� ������������ ���� '&W�����&n'\r\n",ch);	
	    }
	 if (affected_by_spell(ch, SPELL_ZNAK_AKSY))
	    { affect_from_char(ch, SPELL_ZNAK_AKSY);
	      send_to_char("�� ���������� ������������ ���� '&W�����&n'\r\n",ch);
	    }
     if (affected_by_spell(ch, SPELL_ZNAK_GELIOTROP))
	    { affect_from_char(ch, SPELL_ZNAK_GELIOTROP);
		  send_to_char("�� ���������� ������������ ���� '&W���������&n'\r\n",ch);	
	    }
	 if (affected_by_spell(ch, SPELL_ZNAK_KVEN))
	    { affect_from_char(ch, SPELL_ZNAK_KVEN);
		  send_to_char("�� ���������� ������������ ���� '&W����&n'\r\n",ch);	
	    }
      perform_wear(ch, obj, WEAR_LIGHT);
    }
    else {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) &&
		  GET_OBJ_TYPE(obj) != ITEM_WAND &&
          GET_OBJ_TYPE(obj) != ITEM_STAFF &&
		  GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
	      GET_OBJ_TYPE(obj) != ITEM_POTION)
      {act("�� �� ������ ������� $3.", FALSE, ch, obj, 0, TO_CHAR);
	  return;
	  }
	  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        {if (GET_OBJ_SKILL(obj) == SKILL_BOTHHANDS ||
             GET_OBJ_SKILL(obj) == SKILL_BOWS
                 )
	  		{ act("�� �� ������ ������� $3 � ����� ����.", FALSE, ch, obj, 0, TO_CHAR);
				return;
			}	
		}
	
         if (affected_by_spell(ch, SPELL_ZNAK_IRGEN))
 	    { affect_from_char(ch, SPELL_ZNAK_IRGEN);
		  send_to_char("�� ���������� ������������ ���� '&W�����&n'\r\n",ch);	
	    }
	 if (affected_by_spell(ch, SPELL_ZNAK_AKSY))
	    { affect_from_char(ch, SPELL_ZNAK_AKSY);
	      send_to_char("�� ���������� ������������ ���� '&W�����&n'\r\n",ch);
	    }
         if (affected_by_spell(ch, SPELL_ZNAK_GELIOTROP))
	    { affect_from_char(ch, SPELL_ZNAK_GELIOTROP);
		  send_to_char("�� ���������� ������������ ���� '&W���������&n'\r\n",ch);	
	    }
	
		if (affected_by_spell(ch, SPELL_ZNAK_KVEN))
	    { affect_from_char(ch, SPELL_ZNAK_KVEN);
		  send_to_char("�� ���������� ������������ ���� '&W����&n'\r\n",ch);	
	    }

          if (!IS_IMMORTAL(ch) && !OK_HELD(ch,obj) && !GET_EQ(ch, WEAR_HOLD))
             {act("� ��� �� ������� ����, ��� �� ������� $3 � ����� ����.",
                   FALSE,ch,obj,0,TO_CHAR);
              if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
                 {if (!OK_BOTH(ch, obj) && !GET_EQ(ch, WEAR_BOTHS))
                     {act("� ��� �� ������� ����, ��� �� ������� $3 ����� ������.",
                          FALSE, ch, obj, 0, TO_CHAR);
                      return;
                     }
                  else
                     where = WEAR_BOTHS;
                 }
              else
                 return;
             }
          perform_wear(ch, obj, where);
		 }
	
	}
}



void perform_remove(struct char_data * ch, int pos)
{
  struct obj_data *obj;

  if (!(obj = GET_EQ(ch, pos)))
    log("SYSERR: perform_remove: bad pos %d passed.", pos);
  else 
  /*if (IS_OBJ_STAT(obj, ITEM_NODROP))
    act("�� �� ������ ������� $3, ��� ������ ���� ��������!", FALSE, ch, obj, 0, TO_CHAR);
  else */
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    act("$3: �� �� ������ ����� ������� ����� �����!", FALSE, ch, obj, 0, TO_CHAR);
  else {
         if (!remove_otrigger(obj, ch))
         return;
    obj_to_char(unequip_char(ch, pos), ch);
    act("�� ���������� ������������ $3.", FALSE, ch, obj, 0, TO_CHAR); 
    act("$n ���������$y ������������ $3.", TRUE, ch, obj, 0, TO_ROOM);
  }
}


ACMD(do_remove)
{
  int i, dotmode, found;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("��� �� ������ �����?\r\n", ch);
    return;
  }
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_ALL) {
    found = 0;
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i)) {
	perform_remove(ch, i);
	found = 1;
      }
    if (!found)
      send_to_char("�� ������ �� �����������!\r\n", ch);
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg)
      send_to_char("����� ��� � ����?\r\n", ch);
    else {
      found = 0;
      for (i = 0; i < NUM_WEARS; i++)
	if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
	    isname(arg, GET_EQ(ch, i)->name)) {
	  perform_remove(ch, i);
	  found = 1;
	}
      if (!found) {
	sprintf(buf, "�� ������ �� �����������, ��� �� ���������� %s.\r\n", arg);
	send_to_char(buf, ch);
      }
    }
  } else {
    /* Returns object pointer but we don't need it, just true/false. */

      if (!get_object_in_equip_vis(ch, arg, ch->equipment, &i)) {
      sprintf(buf, "�� �� ����������� %s.\r\n", arg); 
      send_to_char(buf, ch);
    } else
      perform_remove(ch, i);
  }
}

ACMD(do_fire)
{
 int percent, prob;
 if (!GET_SKILL(ch, SKILL_FIRE))
    {send_to_char("��� ��� �� �� �����.\r\n", ch);
     return;
    }

 if (on_horse(ch))
    {send_to_char("�� �� ����� ������ ������� ������� ��� ������?\r\n",ch);
     return;
    }

 if (AFF_FLAGGED(ch, AFF_BLIND))
    {send_to_char("�� ������ �� ������!\r\n",ch);
     return;
    }


 if (world[IN_ROOM(ch)].fires)
    {send_to_char("����� ��� ���-�� �����.\r\n", ch);
     return;
    }

 if (SECT(IN_ROOM(ch)) == SECT_INSIDE ||
     SECT(IN_ROOM(ch)) == SECT_CITY   ||
     SECT(IN_ROOM(ch)) == SECT_WATER_SWIM   ||
     SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM ||
     SECT(IN_ROOM(ch)) == SECT_FLYING       ||
     SECT(IN_ROOM(ch)) == SECT_UNDERWATER   ||
     SECT(IN_ROOM(ch)) == SECT_SECRET)
    {send_to_char("����� ��������������� ������� ��� �����.\r\n", ch);
     return;
    }

 if (!check_moves(ch, FIRE_MOVES))
    return;

 percent = number(1,skill_info[SKILL_FIRE].max_percent);
 prob = calculate_skill(ch, SKILL_FIRE, skill_info[SKILL_FIRE].max_percent, 0);
 if (percent > prob)
 {
     send_to_char("�� ���������� �������� ������, �� ��� ����� �����!\r\n", ch);
     return;
 }
 else
 {
     world[IN_ROOM(ch)].fires = MAX(0, (prob - percent) / 5) + 1;
     send_to_char("�� ������� ������� � ������� ������.\n\r", ch);
     act("$n ������$y ������.", FALSE, ch, 0, 0, TO_ROOM);
     improove_skill(ch, SKILL_FIRE, TRUE, 0);
 }
}

#define MAX_REMOVE  10
const int RemoveSpell[MAX_REMOVE] =
{SPELL_SLEEP, SPELL_POISON, SPELL_WEAKNESS, SPELL_CURSE, SPELL_PLAQUE,
 SPELL_SIELENCE, SPELL_BLINDNESS, SPELL_HAEMORRAGIA, SPELL_HOLD, SPELL_BATTLE};

ACMD(do_firstaid)
{int    percent, prob, success=FALSE, need=FALSE, count, spellnum = 0;
 struct timed_type timed;
 struct char_data *vict;

 if (!GET_SKILL(ch, SKILL_AID))
    {send_to_char("�� �� �������� ���� �������!\r\n", ch);
     return;
    }
 if (!IS_GOD(ch) && timed_by_skill(ch, SKILL_AID))
    {send_to_char("��������� �������, �� ��� �� ��������� �� ���������� ���������!\r\n", ch);
     return;
    }

 one_argument(argument, arg);

 if (!*arg)
    vict = ch;
 else
 if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
    {send_to_char("���� �� ������ ����������?\r\n", ch);
     return;
    };

 if (FIGHTING(vict))
    {act("$N ������� ������ ���������!",FALSE,ch,0,vict,TO_CHAR);
     return;
    }

 percent = number(1, skill_info[SKILL_AID].max_percent);
 prob    = calculate_skill(ch, SKILL_AID, skill_info[SKILL_AID].max_percent, vict);

 if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) || GET_GOD_FLAG(vict, GF_GODSLIKE))
         percent = 0;

 if (GET_GOD_FLAG(vict, GF_GODSCURSE) || GET_GOD_FLAG(vict, GF_GODSCURSE))
    prob = 0;

 success = (prob >= percent);
 need    = FALSE;
   
   if ((GET_REAL_MAX_HIT(vict) && (GET_HIT(vict) * 100 / GET_REAL_MAX_HIT(vict)) < 31) ||
       (GET_REAL_MAX_HIT(vict) <= 0 && GET_HIT(vict) < GET_REAL_MAX_HIT(vict)))
    {  need = TRUE;
      if (success)
	  { int dif = GET_REAL_MAX_HIT(vict) - GET_HIT(vict);
	    int add = MIN(dif, (dif * (prob - percent) / 100) + 1);
	    GET_HIT(vict) += add;
            update_pos(vict);
          }
    } 
 
 count = MIN(MAX_REMOVE, MAX_REMOVE * prob / 100);

 for (percent = 0, prob = need;
      !need && percent < MAX_REMOVE && RemoveSpell[percent]; percent++
     )
     if (affected_by_spell(vict, RemoveSpell[percent]))
        {need     = TRUE;
         if (percent < count)
            {spellnum = RemoveSpell[percent];
             prob     = TRUE;
            }
        }


 if (!need)
     { if (vict != ch)
	 act("$N � ��������� �� ���������.",FALSE,ch,0,vict,TO_CHAR);
   else
	 act("�� � ��������� �� ����������.",FALSE,ch,0,0,TO_CHAR);
     }
 else
 if (!prob)
    { if (vict != ch)
	 act("�� �� ������ ���������� $V.",FALSE,ch,0,vict,TO_CHAR);
      else
	 act("�� �� ������ ���������� ���� ����.",FALSE,ch,0,0,TO_CHAR);
     }
 else
    {improove_skill(ch, SKILL_AID, TRUE, 0);
     timed.skill = SKILL_AID;
     timed.time  = IS_IMMORTAL(ch) ? 2 : 6;
     timed_to_char(ch, &timed);
     if (vict != ch)
        {if (success)
            {act("�� ���������� ���� $R.", FALSE, ch, 0, vict, TO_CHAR);
             act("$N ������$Y ��� ���������.", FALSE, vict, 0, ch, TO_CHAR);
             act("$n ������$y ��������� $D.", TRUE, ch, 0, vict, TO_NOTVICT);
             if (spellnum)
                affect_from_char(vict,spellnum);
            }
         else
            {act("�� ���������� ������� ��������� $D, �� ���� ������� ������!", FALSE, ch, 0, vict, TO_CHAR);
             act("$N �������$U ������� ��� ���������, �� ������ ������$Y ������.", FALSE, vict, 0, ch, TO_CHAR);
             act("$n �������$u ������� ��������� $D!", TRUE, ch, 0, vict, TO_NOTVICT);
             GET_HIT(vict) -= GET_HIT(vict)/10;
             update_pos(vict);
			}
        }
     else
        {if (success)
            {act("�� ���������� ���� ����.", FALSE, ch, 0, 0, TO_CHAR);
             act("$n ���������$y ���� ����.",FALSE, ch, 0, 0, TO_ROOM);
             if (spellnum)
                affect_from_char(vict,spellnum);
            }
         else
            {act("�� ���������� �� ����, ������� ������� ���� ���������!", FALSE, ch, 0, vict, TO_CHAR);
             act("$n ���������$y �� ����, ������� ������� ���� ���������!", FALSE, ch, 0, vict, TO_ROOM);
             GET_HIT(vict) -= GET_HIT(vict)/10;
             update_pos(vict);
			}
        }
    }
}


#define MAX_POISON_TIME 12

ACMD(do_poisoned)
{
  struct obj_data   *obj;
  struct timed_type timed;
  int    i, apply_pos = MAX_OBJ_AFFECT;

  if (!GET_SKILL(ch, SKILL_POISONED))
     {send_to_char("�� �� ������ �����.\r\n",ch);
      return;
     }

  one_argument(argument, arg);

  if (!*arg)
     {send_to_char("��� �� ������ ��������?\r\n", ch);
      return;
     }

  if (!IS_IMMORTAL(ch) && timed_by_skill(ch, SKILL_POISONED))
     {send_to_char("�� �������� ���������� ����, ��������� �������.\r\n", ch);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "� ��� ����� ���!\r\n");
      send_to_char(buf, ch);
      return;
     };

  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
     {send_to_char("�� ������ �������� �� ������ �� ������.\r\n", ch);
      return;
     }

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location == APPLY_POISON)
         { act("�� $3 ��� ������� ��.", FALSE, ch, obj, 0, TO_CHAR);
           return;
         }
      else
      if (obj->affected[i].location == APPLY_NONE && apply_pos == MAX_OBJ_AFFECT)
         apply_pos = i;

  if (apply_pos >= MAX_OBJ_AFFECT)
     {send_to_char("�� �� ������ ������� �� �� ���� �������.\r\n",ch);
      return;
     }

  obj->affected[apply_pos].location = APPLY_POISON;
  obj->affected[apply_pos].modifier = MAX_POISON_TIME;

  timed.skill     = SKILL_POISONED;
  timed.time      = MAX_POISON_TIME;
  timed_to_char(ch,&timed);

  act("�� ��������� ������� �� �� $3.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n ��������� �����$q �� �� $3.", FALSE, ch, obj, 0, TO_ROOM);
}

ACMD(do_repair)
{
  struct obj_data   *obj;
  int    prob, percent=0, decay;

  if (!GET_SKILL(ch, SKILL_REPAIR))
     {send_to_char("�� �� ������ �����.\r\n",ch);
      return;
     }

  one_argument(argument, arg);

  if (FIGHTING(ch))
     {send_to_char("�� �� ������ ������� ��� � ���!\r\n", ch);
      return;
     }

  if (!*arg)
     {send_to_char("��� �� ������ ��������?\r\n", ch);
      return;
     }
  
  int pos = 0;
  obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos);
  if (!obj)
      obj = get_obj_in_list_vis(ch, arg, ch->carrying);

  if (!obj)
  {
      sprintf(buf, "� ��� ��� \'%s\'.\r\n", arg);
      send_to_char(buf, ch);
      return;
  };

  if (GET_OBJ_MAX(obj) <= GET_OBJ_CUR(obj))
     {act("$o � ������� �� ���������.",FALSE,ch,obj,0,TO_CHAR);
      return;
     }
  prob    = number(1,skill_info[SKILL_REPAIR].max_percent);
  percent = train_skill(ch,SKILL_REPAIR,skill_info[SKILL_REPAIR].max_percent,0);
  if (prob > percent)
     {GET_OBJ_CUR(obj) = MAX(0, GET_OBJ_CUR(obj) * percent / prob);
      //if (obj->obj_flags.ostalos)
	  if (true)
         {act("�� ���������� �������� $3, �� ������� $S ��� ������.",FALSE,ch,obj,0,TO_CHAR);
          act("$n �������$u �������� $3, �� ������$y $S ��� ������.",FALSE,ch,obj,0,TO_ROOM);
          decay   = (GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj)) / 5;
          decay   = MAX(1, MIN(decay,GET_OBJ_MAX(obj) / 10));
          if (GET_OBJ_MAX(obj) > decay)
             GET_OBJ_MAX(obj) -= decay;
          else
             GET_OBJ_MAX(obj) = 1;
         }
      else
         {act("�� ��������� ������� $3.",FALSE,ch,obj,0,TO_CHAR);
          act("$n ��������� ������$y $3.",FALSE,ch,obj,0,TO_ROOM);
          extract_obj(obj);
         }
      }
  else
     {GET_OBJ_MAX(obj) -= MAX(1,(GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj)) / 20);
      GET_OBJ_CUR(obj) = MIN(GET_OBJ_MAX(obj),
                             GET_OBJ_CUR(obj) * percent / prob + 1);
      act("������ $o �������� �����.",FALSE,ch,obj,0,TO_CHAR);
      act("$n ����� �������$y $3.",FALSE,ch,obj,0,TO_ROOM);
     }
     
}

struct obj_data *tear_skin(struct char_data *ch)
{ struct obj_data  *o;
  struct extra_descr_data *new_descr;
  
  
  if (!ch) {
    log("SYSERR: Try to create negative. (%s)", GET_NAME(ch));
    return (NULL);
  }
  o = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);
    
 // o->name = str_dup("�����");
  sprintf(buf2, "����� %s ����� �����.", GET_RNAME(ch));
  o->description  = str_dup(buf2);
  sprintf(buf2, "����� %s", GET_RNAME(ch));
  o->name = str_dup(buf2);
  sprintf(buf2, "����� %s", GET_RNAME(ch));
  o->short_description = str_dup(buf2);
  sprintf(buf2, "����� %s", GET_RNAME(ch));
  o->short_rdescription = str_dup(buf2);
  sprintf(buf2, "����� %s", GET_RNAME(ch));
  o->short_ddescription = str_dup(buf2);
  sprintf(buf2, "����� %s", GET_RNAME(ch));
  o->short_vdescription = str_dup(buf2);
  sprintf(buf2, "������ %s", GET_RNAME(ch));
  o->short_tdescription = str_dup(buf2);
  sprintf(buf2, "����� %s", GET_RNAME(ch));
  o->short_pdescription = str_dup(buf2);
  
    new_descr->keyword = str_dup("�����");
    new_descr->description = str_dup("��� ������������ �����!");
    new_descr->next = NULL;
    o->ex_description = new_descr;
  
  GET_OBJ_COST(o)                  = GET_LEVEL(ch);
  GET_OBJ_TYPE(o)		   = ITEM_SKIN;
  GET_OBJ_WEAR(o)		   = ITEM_WEAR_TAKE;
  GET_OBJ_VAL(o, 0)		   = 3;
  GET_OBJ_MAX(o)                   = 100;
  GET_OBJ_CUR(o)                   = 100;
  GET_OBJ_TIMER(o)                 = 24*60*7;
  GET_OBJ_WEIGHT(o)                = 1;
  GET_OBJ_SEX(o)		   = SEX_FEMALE;
  GET_OBJ_MATER(o)		   = MAT_SKIN;	
  GET_OBJ_RENT(o)		   = 3 * GET_LEVEL(ch);	
  REMOVE_BIT(GET_OBJ_EXTRA(o, ITEM_NOSELL), ITEM_NOSELL);
  
  o->item_number = NOTHING;


 return (o);
}

ACMD(do_tear_skin)
{ struct obj_data  *obj,*tobj = NULL;
  struct char_data *mob;
  
  int    prob=0, percent=0, mobn, wgt=0;

  if (FIGHTING(ch))
  { send_to_char("�� �� ������ ������� ��� � ���!\r\n", ch);
	return;
  }

 if (!GET_SKILL(ch, SKILL_TEAR))
	 {send_to_char("� ��� ��� ������� ������� �����!\r\n",ch);
      return;
     }
  one_argument(argument,arg);
	 if (!*arg)
     {send_to_char("� ���� �� ������ ������� �����?\r\n",ch);
      return;
     }

	if (isname(arg, "�����"))
		strcpy(arg, "����");
	if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)) &&
		    !(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)))
		{ sprintf(buf,"��� �� � ���� ������� �����!\r\n");
		  send_to_char(buf,ch);
		  return;
		};

		if (!IS_CORPSE(obj) || (mobn = GET_OBJ_VAL(obj,2)) < 0)
		{ act("�� �� ������� ������� ����� � $1.",FALSE,ch,obj,0,TO_CHAR);
		  return;
		};

		mob = (mob_proto + real_mobile(mobn));
		wgt = GET_WEIGHT(mob);
		if (!IS_IMMORTAL(ch) && !IS_ANIMALS(mob))
		{ send_to_char("� ����� ����� ������� ����� ����������!\r\n",ch);
		  return;
		};
  
		prob    = number(1,skill_info[SKILL_TEAR].max_percent);
		percent = train_skill(ch,SKILL_TEAR,skill_info[SKILL_TEAR].max_percent,0);
		
	
	if ((prob > percent) ||!(tobj = tear_skin(mob)))
		{ act("�� �� ������ ������� ����� � $1.",FALSE,ch,obj,0,TO_CHAR);
		  act("$n �������$u ������� ����� � $1, �� ������ ��������$y ��!",FALSE,ch,obj,0,TO_ROOM);
		  extract_obj(obj);
		  return;
		}
  else
     { act("$n ����� ������$y ����� � $1.",FALSE,ch,obj,0,TO_ROOM);
       act("�� ����� ������� ����� � $1." ,FALSE,ch,obj,0,TO_CHAR);
      if (obj->carried_by == ch)
         {if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
             {send_to_char("�� �� ������ ����� ������� ���������.", ch);
              obj_to_room(tobj,IN_ROOM(ch));
	      obj_decay(tobj);
             }
          else
          if (GET_OBJ_WEIGHT(tobj) + IS_CARRYING_W(ch) > CAN_CARRY_W(ch))
             {sprintf(buf,"��� ������� ������ ����� ��� � %s.",tobj->short_vdescription);
              send_to_char(buf,ch);
              obj_to_room(tobj,IN_ROOM(ch));
//	      obj_decay(tobj);
             }
          else
             obj_to_char(tobj,ch);
         }
      else
         obj_to_room(tobj,IN_ROOM(ch));
//	 obj_decay(tobj);
     }
  if (obj->carried_by)
     {obj_from_char(obj);
      obj_to_room(obj, IN_ROOM(ch));
     }
  extract_obj(obj);

}

const int meet_vnum[] = {1203,1202,1201,1200};

ACMD(do_makefood)
{ struct obj_data  *obj,*tobj;
  struct char_data *mob;
  int    prob, percent=0, mobn, wgt=0;

  if (!GET_SKILL(ch,SKILL_MAKEFOOD))
     {send_to_char("�� �� �������� ���� �������.\r\n",ch);
      return;
     }

  one_argument(argument,arg);
  if (!*arg)
     {send_to_char("���������� ���?\r\n",ch);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)) &&
      !(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)))
     {sprintf(buf,"�� �� ������ ����� �����.\r\n");
      send_to_char(buf,ch);
      return;
     }

  if (!IS_CORPSE(obj) || (mobn = GET_OBJ_VAL(obj,2)) < 0)
     {act("�� �� ������� ���������� $3.",FALSE,ch,obj,0,TO_CHAR);
      return;
     }
  mob = (mob_proto + real_mobile(mobn));
  if (!IS_IMMORTAL(ch) && 
      !IS_ANIMALS(mob) &&
      (wgt = GET_WEIGHT(mob)) < 50)
     {send_to_char("���� ���� ���������� ����������.\r\n",ch);
      return;
  }
  prob    = number(1,skill_info[SKILL_MAKEFOOD].max_percent);
  percent = train_skill(ch,SKILL_MAKEFOOD,skill_info[SKILL_MAKEFOOD].max_percent,0);
  if (prob > percent ||
      !(tobj = read_object(meet_vnum[number(0,MIN(3,MAX(0,(wgt-50)/5)))],VIRTUAL)))
     {act("�� �� ������ ���������� $3.",FALSE,ch,obj,0,TO_CHAR);
      act("$n �������$u ���������� $3, �� ������ �� ����������.",FALSE,ch,obj,0,TO_ROOM);
     }
  else
     {sprintf(buf,"$n ����� �������$y %s �� $1.",tobj->short_vdescription);
      act(buf,FALSE,ch,obj,0,TO_ROOM);
      sprintf(buf,"�� ����� �������� %s �� $1.",tobj->short_vdescription);
      act(buf,FALSE,ch,obj,0,TO_CHAR);
      if (obj->carried_by == ch)
         {if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
             {send_to_char("�� �� ������ ����� ������� ���������.", ch);
              obj_to_room(tobj,IN_ROOM(ch));
	      obj_decay(tobj);
             }
          else
          if (GET_OBJ_WEIGHT(tobj) + IS_CARRYING_W(ch) > CAN_CARRY_W(ch))
             {sprintf(buf,"��� ������� ������ ����� ��� � %s.",tobj->short_vdescription);
              send_to_char(buf,ch);
              obj_to_room(tobj,IN_ROOM(ch));
//	      obj_decay(tobj);
             }
          else
             obj_to_char(tobj,ch);
         }
      else
         obj_to_room(tobj,IN_ROOM(ch));
//	 obj_decay(tobj);
     }
  if (obj->carried_by)
     {obj_from_char(obj);
      obj_to_room(obj, IN_ROOM(ch));
     }
  extract_obj(obj);
}

#define MAX_ITEMS		9
#define MAX_PROTO		3
#define COAL_PROTO 		311
#define WOOD_PROTO      313
#define TETIVA_PROTO    314
#define ONE_DAY    		24*60
#define SEVEN_DAYS		7*24*60

struct create_item_type
{int obj_vnum;
 int material_bits;
 int min_weight;
 int max_weight;
 int proto[MAX_PROTO];
 int skill;
 int wear;
};

struct create_item_type created_item [] =
{ {300,0x7E,15,40,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {301,0x7E,12,40,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {302,0x7E,8 ,25,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {303,0x7E,5 ,12,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {304,0x7E,10,35,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {305,0   ,8 ,15,{0,0,0},0,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {306,0   ,8 ,20,{0,0,0},0,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS | ITEM_WEAR_WIELD)},
  {307,0x3A,10,20,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_BODY)},
  {308,0x3A,4,10,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_ARMS)},
  {309,0x3A,6,12,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_LEGS)},
  {310,0x3A,4,10,{COAL_PROTO,0,0},SKILL_TRANSFORMWEAPON,(ITEM_WEAR_TAKE | ITEM_WEAR_HEAD)},
  {312,0,4,40,   {WOOD_PROTO,TETIVA_PROTO,0},SKILL_CREATEBOW,(ITEM_WEAR_TAKE | ITEM_WEAR_BOTHS)}
};

const char *create_item_name  [] =
{"��������",
 "���",
 "�����",
 "���",
 "�����",
 "�����",
 "������",
 "��������",
 "������",
 "������",
 "����",
 "���",
 "\n"
};

void go_create_weapon(struct char_data *ch, struct obj_data *obj, int obj_type, int skill)
{struct obj_data *tobj;
 char   *to_char=NULL, *to_room=NULL;
 int    prob,percent,ndice,sdice,weight;

 if (obj_type == 5 ||
     obj_type == 6
    )
    {weight  = number(created_item[obj_type].min_weight, created_item[obj_type].max_weight);
     percent = 100;
     prob    = 100;
    }
 else
    {if (!obj)
        return;
     skill = created_item[obj_type].skill;
     percent= number(1,skill_info[skill].max_percent);
     prob   = train_skill(ch,skill,skill_info[skill].max_percent,0);
     weight = MIN(GET_OBJ_WEIGHT(obj) - 2, GET_OBJ_WEIGHT(obj) * prob / percent);
    }

 if (weight < created_item[obj_type].min_weight)
    send_to_char("� ��� �� ������� ���������.\r\n",ch);
 else
 if (prob * 5 < percent)
    send_to_char("� ��� ������ �� ����������.\r\n",ch);
 else
 if (!(tobj = read_object(created_item[obj_type].obj_vnum,VIRTUAL)))
    send_to_char("������� ��� ������������ ������.\r\n",ch);
 else
    {GET_OBJ_WEIGHT(tobj)      = MIN(weight, created_item[obj_type].max_weight);
     tobj->obj_flags.Obj_owner = GET_UNIQUE(ch);
     switch (obj_type)
     {case 0: /* smith weapon */
      case 1:
      case 2:
      case 3:
      case 4:
      case 11:
        GET_OBJ_TIMER(tobj) = MAX(ONE_DAY, MIN(SEVEN_DAYS, SEVEN_DAYS * prob / percent));
        GET_OBJ_MATER(tobj) = GET_OBJ_MATER(obj);
        GET_OBJ_MAX(tobj)   = MAX(50, MIN(300, 300 * prob / percent));
        GET_OBJ_CUR(tobj)   = GET_OBJ_MAX(tobj);
        percent= number(1,skill_info[skill].max_percent);
        prob   = calculate_skill(ch,skill,skill_info[skill].max_percent,0);
        ndice  = MAX(2,MIN(4,prob/percent));
        ndice += GET_OBJ_WEIGHT(tobj) / 15;
        percent= number(1,skill_info[skill].max_percent);
        prob   = calculate_skill(ch,skill,skill_info[skill].max_percent,0);
        sdice  = MAX(2,MIN(4,prob/percent));
        sdice += GET_OBJ_WEIGHT(tobj) / 15;
        GET_OBJ_VAL(tobj,1) = ndice;
        GET_OBJ_VAL(tobj,2) = sdice;
        SET_BIT(tobj->obj_flags.wear_flags,created_item[obj_type].wear);
        if (skill != SKILL_CREATEBOW)
           {if (GET_OBJ_WEIGHT(tobj) < 14 && percent * 4 > prob)
               SET_BIT(tobj->obj_flags.wear_flags,ITEM_WEAR_HOLD);
            to_room = "$n �������$y $3.";
            to_char = "�� �������� $3.";
           }
        else
           {to_room = "$n ���������$y $3.";
            to_char = "�� ���������� $3.";
           }
        break;
      case 5: /* mages weapon */
      case 6:
        GET_OBJ_TIMER(tobj) = ONE_DAY;
        GET_OBJ_MAX(tobj)   = 50;
        GET_OBJ_CUR(tobj)   = 50;
        ndice  = MAX(2,MIN(4,GET_LEVEL(ch)/number(6,8)));
        ndice += (GET_OBJ_WEIGHT(tobj) / 15);
        sdice  = MAX(2,MIN(5,GET_LEVEL(ch)/number(4,5)));
        sdice += (GET_OBJ_WEIGHT(tobj) / 15);
        GET_OBJ_VAL(tobj,1) = ndice;
        GET_OBJ_VAL(tobj,2) = sdice;
        SET_BIT(GET_OBJ_EXTRA(tobj, ITEM_NORENT), ITEM_NORENT);
        SET_BIT(GET_OBJ_EXTRA(tobj, ITEM_DECAY), ITEM_DECAY);
        SET_BIT(GET_OBJ_EXTRA(tobj, ITEM_NOSELL), ITEM_NOSELL);
        SET_BIT(tobj->obj_flags.wear_flags, created_item[obj_type].wear);
        to_room = "$n ������$y $3.";
        to_char = "�� ������� $3.";
        break;
     case 7: /* smith armor */
     case 8:
     case 9:
     case 10:
        GET_OBJ_TIMER(tobj) = MAX(ONE_DAY, MIN(SEVEN_DAYS, SEVEN_DAYS *  prob / percent));
        GET_OBJ_MATER(tobj) = GET_OBJ_MATER(obj);
        GET_OBJ_MAX(tobj)   = MAX(50, MIN(300, 300 * prob / percent));
        GET_OBJ_CUR(tobj)   = GET_OBJ_MAX(tobj);
        percent= number(1,skill_info[skill].max_percent);
        prob   = calculate_skill(ch,skill,skill_info[skill].max_percent,0);
        ndice  = MAX(2,MIN((105-material_value[GET_OBJ_MATER(tobj)])/10,prob / percent));
        percent= number(1,skill_info[skill].max_percent);
        prob   = calculate_skill(ch,skill,skill_info[skill].max_percent,0);
        sdice  = MAX(1,MIN((105-material_value[GET_OBJ_MATER(tobj)])/15,prob / percent));
        GET_OBJ_VAL(tobj,1) = ndice;
        GET_OBJ_VAL(tobj,2) = sdice;
        SET_BIT(tobj->obj_flags.wear_flags,created_item[obj_type].wear);
        to_room = "$n �������$y $3.";
        to_char = "�� �������� $3.";
        break;
       }
       if (to_char)
          act(to_char,FALSE,ch,tobj,0,TO_CHAR);
       if (to_room)
          act(to_room,FALSE,ch,tobj,0,TO_ROOM);

       if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
          {send_to_char("�� �� ������� ������ ������� ���������.\r\n", ch);
           obj_to_room(tobj, IN_ROOM(ch));
	   obj_decay(tobj);
          }
       else
       if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(tobj) > CAN_CARRY_W(ch))
          {send_to_char("�� �� ������� ������ ����� ���.\r\n", ch);
           obj_to_room(tobj, IN_ROOM(ch));
	   obj_decay(tobj);
          }
       else
           obj_to_char(tobj,ch);
    }
 if (obj)
    {obj_from_char(obj);
     extract_obj(obj);
    }
}


ACMD(do_transform_weapon)
{ char   arg1[MAX_INPUT_LENGTH];
  char   arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj=NULL, *coal, *tmp_list[MAX_ITEMS+1], *proto[MAX_PROTO];
  int    obj_type,i,count=0,found,rnum;

  if (IS_NPC(ch) || !GET_SKILL(ch, subcmd))
     {send_to_char("�� ����� �� �������!\r\n",ch);
      return;
     }

  argument = one_argument(argument, arg1);
  if (!*arg1)
     {switch (subcmd)
      {case SKILL_TRANSFORMWEAPON:
            send_to_char("��� �� ������ ��������?\r\n",ch);
            break;
       case SKILL_CREATEBOW:
            send_to_char("��� �� ������ ����������?\r\n",ch);
            break;
      }
      return;
     }
  if ((obj_type = search_block(arg1, create_item_name, FALSE)) == -1)
     {switch (subcmd)
      {case SKILL_TRANSFORMWEAPON:
            send_to_char("���������� ����� � :\r\n",ch);
            break;
       case SKILL_CREATEBOW:
            send_to_char("���������� ����� :\r\n",ch);
            break;
      }
      for (obj_type = 0; *create_item_name[obj_type] != '\n'; obj_type++)
          if (created_item[obj_type].skill == subcmd)
             {sprintf(buf,"- %s\r\n",create_item_name[obj_type]);
              send_to_char(buf,ch);
             }
      return;
     }
  if (created_item[obj_type].skill != subcmd)
     {switch (subcmd)
      {case SKILL_TRANSFORMWEAPON :
            send_to_char("������ ������� �������� ������.\r\n",ch);
            break;
       case SKILL_CREATEBOW:
            send_to_char("������ ������� ���������� ������.\r\n",ch);
            break;
      }
      return;
     }

  for (i=0;i<MAX_ITEMS;tmp_list[i++] = NULL);
  for (i=0;i<MAX_PROTO;proto[i++] = NULL);

  do {argument = one_argument(argument, arg2);
      if (!*arg2)
         break;
      if (!(obj = get_obj_in_list_vis(ch,arg2,ch->carrying)))
         {sprintf(buf,"� ��� ��� '%s'.\r\n",arg2);
          send_to_char(buf,ch);
         }
      else
      if (obj->contains)
         act("� $5 ���-�� �����.",FALSE,ch,obj,0,TO_CHAR);
      else
      if (GET_OBJ_TYPE(obj) == ITEM_INGRADIENT)
         {for (i = 0, found = FALSE; i < MAX_PROTO; i++)
              if (GET_OBJ_VAL(obj,1) == created_item[obj_type].proto[i])
                 {if (proto[i])
                      found = TRUE;
                  else
                     {proto[i] = obj;
                      found    = FALSE;
                      break;
                     }
                 }
          if (i >= MAX_PROTO)
             {if (found)
                 act("������, �� ��� ���-�� ����������� ������ $1.",FALSE,ch,obj,0,TO_CHAR);
              else
                 act("������, $o �� �������� ��� �����.",FALSE,ch,obj,0,TO_CHAR);
             }
         }
      else
      if (created_item[obj_type].material_bits &&
          !IS_SET(created_item[obj_type].material_bits, (1 << GET_OBJ_MATER(obj))))
         act("$o ������$Y �� ������������� ���������.",FALSE,ch,obj,0,TO_CHAR);
      else
      if (obj->contains)
         act("� $5 ���-�� �����.",FALSE,ch,obj,0,TO_CHAR);
      else
      if (count >= MAX_ITEMS)
         {switch (subcmd)
          {case SKILL_TRANSFORMWEAPON:
                send_to_char("������� ��������� �� �� ������� ����������.\r\n",ch);
                break;
           case SKILL_CREATEBOW:
                send_to_char("������� ����� ���������.\r\n",ch);
                break;
          }
          return;
         }
      else
         {switch (subcmd)
          {case SKILL_TRANSFORMWEAPON:
                if (count && GET_OBJ_MATER(tmp_list[count-1]) != GET_OBJ_MATER(obj))
                   {send_to_char("������ ��������� !\r\n",ch);
                    return;
                   }
                tmp_list[count++] = obj;
                break;
           case SKILL_CREATEBOW:
                act("$o �� ������������$y ��� ���� �����.",FALSE,ch,obj,0,TO_CHAR);
                break;
          }
         }
     } while (TRUE);

  switch (subcmd)
  {case SKILL_TRANSFORMWEAPON:
        if (!count)
           {send_to_char("�� ������ �� ������ ����������.\r\n",ch);
            return;
           }

        if (!IS_IMMORTAL(ch))
           {if (!ROOM_FLAGGED(IN_ROOM(ch),ROOM_SMITH))
               {send_to_char("��� �� ��� ������ ��������, ���������� ����� �������.\r\n",ch);
                return;
               }
            for (coal = ch->carrying; coal; coal = coal->next_content)
                if (GET_OBJ_TYPE(coal) == ITEM_INGRADIENT)
                   {for (i = 0; i < MAX_PROTO; i++)
                        if (proto[i] == coal)
                           break;
                        else
                        if (!proto[i] && GET_OBJ_VAL(coal,1) == created_item[obj_type].proto[i])
                           {proto[i] = coal;
                            break;
                           }
                   }
            for (i = 0, found = TRUE; i < MAX_PROTO; i++)
                if (created_item[obj_type].proto[i] && !proto[i])
                   {if ((rnum = real_object(created_item[obj_type].proto[i])) < 0)
                       act("� ��� ��� ������������ ���������.",FALSE,ch,0,0,TO_CHAR);
                    else
                       act("� ��� �� ������� $1 ��� �����.",FALSE,ch,obj_proto+rnum,0,TO_CHAR);
                    found = FALSE;
                   }
            if (!found)
               return;
           }
        for (i = 1; i < count; i++)
            {GET_OBJ_WEIGHT(tmp_list[0]) += GET_OBJ_WEIGHT(tmp_list[i]);
             extract_obj(tmp_list[i]);
            }
        for (i = 0; i < MAX_PROTO; i++)
            {if (proto[i])
                extract_obj(proto[i]);
            }
        go_create_weapon(ch,tmp_list[0],obj_type,SKILL_TRANSFORMWEAPON);
        break;
    case SKILL_CREATEBOW:
         for (coal = ch->carrying; coal; coal = coal->next_content)
             if (GET_OBJ_TYPE(coal) == ITEM_INGRADIENT)
                {for (i = 0; i < MAX_PROTO; i++)
                      if (proto[i] == coal)
                         break;
                        else
                      if (!proto[i] && GET_OBJ_VAL(coal,1) == created_item[obj_type].proto[i])
                         {proto[i] = coal;
                          break;
                         }
               }
         for (i = 0, found = TRUE; i < MAX_PROTO; i++)
             if (created_item[obj_type].proto[i] && !proto[i])
                {if ((rnum = real_object(created_item[obj_type].proto[i])) < 0)
                       act("� ��� ��� ������� �����������.",FALSE,ch,0,0,TO_CHAR);
                    else
                       act("� ��� �� ������� $1 ��� �����.",FALSE,ch,obj_proto+rnum,0,TO_CHAR);
                    found = FALSE;
                   }
        if (!found)
           return;
        for (i = 1; i < MAX_PROTO; i++)
            if (proto[i])
               {GET_OBJ_WEIGHT(proto[0]) += GET_OBJ_WEIGHT(proto[i]);
                extract_obj(proto[i]);
               }
        go_create_weapon(ch,proto[0],obj_type,SKILL_CREATEBOW);
        break;
 }
}


ACMD(do_upgrade)
{
  struct obj_data *obj;
  int    weight, add_hr, add_dr, prob, percent; //, i;

  if (!GET_SKILL(ch, SKILL_UPGRADE))
     {send_to_char("� ��� ��� ������ ������.\r\n",ch);
      return;
     }

  one_argument(argument, arg);

  if (!*arg)
    { send_to_char("�������� ���?\r\n", ch);
      return;
    }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "� ��� ����� ���.\r\n");
      send_to_char(buf, ch);
      return;
     };

  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
     {send_to_char("�� ������ ���������� ������ ������.\r\n", ch);
      return;
     }

  if (GET_OBJ_SKILL(obj) == SKILL_BOWS)
     {send_to_char("�� �� ������ �������� ���!\r\n", ch);
      return;
     }

 if (OBJ_FLAGGED(obj, ITEM_SHARPEN))
    {act("$o ��� �������$Y.", FALSE, ch, obj, 0, TO_CHAR);
	 return;
    }


  if (OBJ_FLAGGED(obj, ITEM_MAGIC))
    {act("$o �� ������������$y ��� �������.", FALSE, ch, obj, 0, TO_CHAR);
	 return;
    }


  /* Make sure no other affections. */
  /*
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
         {send_to_char("���� ������� �� �� ������ ��������.\r\n", ch);
          return;
         }
*/

  switch (obj->obj_flags.material)
{
case MAT_BRONZE:
case MAT_BULAT:
case MAT_IRON:
case MAT_STEEL:
case MAT_SWORDSSTEEL:
case MAT_COLOR:
case MAT_BONE:
     act("�� ���������� �������� $3.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n �������$u �������� $3.", FALSE, ch, obj, 0, TO_ROOM);
     weight = -1;
     break;
case MAT_WOOD:
case MAT_SUPERWOOD:
     act("�� ������� �������� $3.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n ����$u �������� $3.", FALSE, ch, obj, 0, TO_ROOM);
     weight = -1;
     break;
case MAT_SKIN:
     act("�� ������� ������������ $3.", FALSE, ch, obj, 0, TO_CHAR);
     act("$n ����$u ������������ $3.", FALSE, ch, obj, 0, TO_ROOM);
     weight = +1;
     break;
default:
   act("$o ������$Y �� ������������� ���������.",FALSE,ch,obj,0,TO_CHAR);    
  return;
}

SET_BIT(GET_OBJ_EXTRA(obj, ITEM_SHARPEN), ITEM_SHARPEN);
percent = number(1,skill_info[SKILL_UPGRADE].max_percent);
prob    = train_skill(ch,SKILL_UPGRADE,skill_info[SKILL_UPGRADE].max_percent,0)+20;

add_hr  = IS_IMMORTAL(ch) ? 15 : number(1,(GET_LEVEL(ch) + 3) / 4);
add_dr  = IS_IMMORTAL(ch) ? 8  : number(1,(GET_LEVEL(ch) + 3) / 4);
if (percent > prob || GET_GOD_FLAG(ch, GF_GODSCURSE))
   {act("�� ��� ������ �������� $3.",FALSE,ch,obj,0,TO_CHAR);
    add_hr = -add_hr;
    add_dr = -add_dr;
   }
else
    act("�� ����� �������� $3.", FALSE, ch, obj, 0, TO_CHAR);

	 set_obj_eff (obj, APPLY_HITROLL, add_hr);
	 set_obj_eff (obj, APPLY_DAMROLL, add_dr);
	 /*
obj->affected[0].location = APPLY_HITROLL;
obj->affected[0].modifier = add_hr;

obj->affected[1].location = APPLY_DAMROLL;
obj->affected[1].modifier = add_dr;
*/
obj->obj_flags.weight    += weight;
//obj->obj_flags.Obj_owner  = GET_UNIQUE(ch);
}

int ext_search_block(char *arg, const char **list, int exact)
{
  register int i, l, j, o;

  // Make into lower case, and get length of string
  for (l = 0; *(arg + l); l++)
      *(arg + l) = LOWER(*(arg + l));

  if (exact)
     { for (i = j = 0, o = 1; j != 1; i++)
           if (**(list + i) == '\n')
              {o = 1;
               switch (j)
               {case 0          : j = INT_ONE; break;
                case INT_ONE    : j = INT_TWO; break;
                case INT_TWO    : j = INT_THREE; break;
                default         : j = 1; break;
               }
              }
           else
           if (!str_cmp(arg, *(list + i)))
              return (j | o);
           else
              o <<= 1;
     }
  else
     {if (!l)
         l = 1;	/* Avoid "" to match the first available
				 * string */
      for (i = j = 0, o = 1; j != 1; i++)
           if (**(list + i) == '\n')
              {o = 1;
               switch (j)
               {case 0          : j = INT_ONE; break;
                case INT_ONE    : j = INT_TWO; break;
                case INT_TWO    : j = INT_THREE; break;
                default         : j = 1; break;
               }
              }
           else
           if (!strn_cmp(arg, *(list + i), l))
              return (j | o);
           else
              o <<= 1;
     }

  return (0);
}


ACMD(do_cheat)
{
  struct obj_data *obj;
  int    add_ac, j, i;
  char   field[MAX_INPUT_LENGTH], subfield[MAX_INPUT_LENGTH];

  argument = one_argument(argument, arg);

  if (!GET_COMMSTATE(ch))
     {send_to_char("��� �� ������ �������?\r\n", ch);
      return;
     }

  if (!*arg)
     {send_to_char("���� �����?\r\n", ch);
      return;
     }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
     {sprintf(buf, "� ��� ��� \'%s\'.\r\n", arg);
      send_to_char(buf, ch);
      return;
     };

  *field = *subfield = 0;
  argument = one_argument(argument, field);
  argument = one_argument(argument, subfield);

  if (!*field)
     {send_to_char("�� ������� ����!\r\n",ch);
      return;
     }

  if (!strn_cmp(field,"����0",5))
     {send_to_char("���� 0 : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_VAL(obj,0) = i;
     }
  else
  if (!strn_cmp(field,"����1",5))
     {send_to_char("���� 1 : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_VAL(obj,1) = i;
     }
  else
  if (!strn_cmp(field,"����2",5))
     {send_to_char("���� 2 : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_VAL(obj,2) = i;
     }
  else
  if (!strn_cmp(field,"����3",5))
     {send_to_char("���� 3 : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_VAL(obj,3) = i;
     }
  else
  if (!strn_cmp(field,"���",3))
     {send_to_char("��� : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      obj_from_char(obj);
      GET_OBJ_WEIGHT(obj) = i;
      obj_to_char(obj,ch);
     }
  else
  if (!strn_cmp(field,"������",6))
     {send_to_char("������ : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_TIMER(obj) = i;
     }
  else
  if (!strn_cmp(field,"����",4))
     {send_to_char("������������ ��������� : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_MAX(obj) = i;
     }
  else
  if (!strn_cmp(field,"����",4))
     {send_to_char("��������� : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_CUR(obj) = i;
     }
  else
  if (!strn_cmp(field,"�����",5))
     {send_to_char("�������� : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&i) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      sprintf(buf,"%d Ok !\r\n",i);
      send_to_char(buf,ch);
      GET_OBJ_MATER(obj) = i;
     }
  else
  if ((i = ext_search_block(field, extra_bits, FALSE)))
     {send_to_char("��������� : ", ch);
      if (!*subfield)
         TOGGLE_BIT(GET_OBJ_EXTRA(obj, i),i);
      else
      if (!str_cmp(subfield,"��") || !str_cmp(subfield,"���"))
         SET_BIT(GET_OBJ_EXTRA(obj, i),i);
      else
         REMOVE_BIT(GET_OBJ_EXTRA(obj, i),i);
      sprintf(buf,"%x %s !\r\n", i, IS_SET(GET_OBJ_EXTRA(obj, i),i) ? "On" : "Off");
      send_to_char(buf,ch);
     }
  else
  if ((i = ext_search_block(field, weapon_affects, FALSE)))
     {send_to_char("������: ", ch);
      if (!*subfield)
         TOGGLE_BIT(GET_OBJ_AFF(obj, i),i);
      else
      if (!str_cmp(subfield,"��") || !str_cmp(subfield,"���"))
         SET_BIT(GET_OBJ_AFF(obj, i),i);
      else
         REMOVE_BIT(GET_OBJ_AFF(obj, i),i);
      sprintf(buf,"%x %s !\r\n", i, IS_SET(GET_OBJ_AFF(obj, i),i) ? "On" : "Off");
      send_to_char(buf,ch);
     }
  else
  if ((i = search_block(field, wear_bits, FALSE)) >= 0)
     {send_to_char("����� : ", ch);
      if (!*subfield)
         TOGGLE_BIT(GET_OBJ_WEAR(obj),(1 << i));
      else
      if (!str_cmp(subfield,"��") || !str_cmp(subfield,"���"))
         SET_BIT(GET_OBJ_WEAR(obj),(1 << i));
      else
         REMOVE_BIT(GET_OBJ_WEAR(obj),(1 << i));
      sprintf(buf,"%s %s !\r\n",wear_bits[i],IS_SET(GET_OBJ_WEAR(obj),(1 << i)) ? "On" : "Off");
      send_to_char(buf,ch);
     }
  else
  if ((i = search_block(field, apply_types, FALSE)) >= 0)
     {send_to_char("��������� : ", ch);
      if (!*subfield || sscanf(subfield,"%d",&add_ac) != 1)
         {send_to_char("��������� �������� ��������.\r\n",ch);
          return;
         }
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
          if (obj->affected[j].location == i || !obj->affected[j].location)
             break;
      if (j >= MAX_OBJ_AFFECT)
         {send_to_char("��� ���������� ����� !\r\n",ch);
          return;
         }
      if (!add_ac)
         {i = APPLY_NONE;
          for (; j + 1 < MAX_OBJ_AFFECT; j++)
              {obj->affected[j].location = obj->affected[j+1].location;
               obj->affected[j].modifier = obj->affected[j+1].modifier;
	      }
	 }	
      obj->affected[j].location = i;
      obj->affected[j].modifier = add_ac;
      sprintf(buf,"%s �� %d !\r\n",apply_types[i],add_ac);
      send_to_char(buf,ch);
     }
  else
  if (!strn_cmp(field,"������",6))
     { REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODONATE), ITEM_NODONATE);
       GET_OBJ_OWNER(obj) = 0;
       send_to_char("��������� ������� �����������.\r\n",ch);
       return;
     }
  else
     {sprintf(buf,"������ �� ��������(%s) !\r\n", field);
      send_to_char(buf,ch);
      return;
     }

  SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NODONATE), ITEM_NODONATE);
  GET_OBJ_OWNER(obj) = GET_UNIQUE(ch);
}
