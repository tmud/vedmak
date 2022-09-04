/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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
#include "dg_scripts.h"
#include "constants.h"
#include "screen.h"
#include "pk.h"

/* extern variables */
extern	struct room_data *world;
extern	struct descriptor_data *descriptor_list;
extern	int pk_allowed;
extern	const char *dirs2[];
extern bool check_punch_eq(char_data *ch);

/* extern functions */
int     legal_dir(struct char_data *ch, int dir, int need_specials_check, int show_msg);
int	compute_armor_class(struct char_data *ch);
void	perform_violence(void);
int	may_kill_here(struct char_data *ch, struct char_data *victim);
void    raw_kill(struct char_data * ch, struct char_data * killer);
int	awake_others(struct char_data * ch);
void	go_pary(struct char_data *ch);
void	appear(struct char_data * ch);

/* local functions */

ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_backstab);
ACMD(do_blade_vortex);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_bash_door); 
ACMD(do_rescue);
ACMD(do_kick);
ACMD(do_podsek);
ACMD(do_pary);
ACMD(do_tochny);


int onhorse(struct char_data *ch)
{ if (on_horse(ch))
     {act("��� ������ $N.",FALSE,ch,0,get_horse(ch),TO_CHAR);
      return (TRUE);
     }
  return (FALSE);
};

void set_wait(struct char_data *ch, int waittime, int victim_in_room);

int used_attack(struct char_data *ch)
{char *message = NULL;

 if (GET_AF_BATTLE(ch,EAF_BLOCK))
    message = "����������. �� ��������� ����������� �����.";
 else
if (GET_AF_BATTLE(ch,EAF_PARRY))
	{ message = "�� ���������� ���������� �����.";
	  CLR_AF_BATTLE(ch, EAF_PARRY);
	  act(message, FALSE, ch, 0, GET_EXTRA_VICTIM(ch), TO_CHAR);
	  set_wait(ch, 2, TRUE);
       return (FALSE);
	}
 else
 if (GET_AF_BATTLE(ch,EAF_MULTYPARRY))
    message = "����������. �� ������������� �� ������� ������.";
 else
 if (!GET_EXTRA_VICTIM(ch))
    return (FALSE);
 else
    switch (GET_EXTRA_SKILL(ch))
    {case SKILL_BASH:
          message = "����������. �� ��������� ����� $V.";
          break;
     case SKILL_KICK:
          message = "� ��� �� �� ������ ����!!! �� � ��� ��������� ����� $V.";
          break;
     case SKILL_CHOPOFF:
          message = "����������. �� ��������� ������� $V.";
          break;
     case SKILL_DISARM:
          message = "����������. �� ��������� ����������� $V.";
          break;
     case SKILL_THROW:
          message = "����������. �� ��������� ������� ������ � $V.";
          break;
     default:
          return (FALSE);
    }
 if (message)
    act(message, FALSE, ch, 0, GET_EXTRA_VICTIM(ch), TO_CHAR);
 return (TRUE);
}



int have_mind(struct char_data *ch)
{if (!AFF_FLAGGED(ch,AFF_CHARM) && !IS_HORSE(ch))
    return (TRUE);
 return (FALSE);
}



void set_wait(struct char_data *ch, int waittime, int victim_in_room)
{
    if (!WAITLESS(ch) &&
     (!victim_in_room ||
      (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
     )
    )
    WAIT_STATE(ch, waittime * PULSE_VIOLENCE);
};

int set_hit(struct char_data *ch, struct char_data *victim)
{if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
    {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
     return (FALSE);
    }
 if (FIGHTING(ch) || GET_MOB_HOLD(ch) || AFF_FLAGGED(ch, AFF_HOLDALL))
    {return (FALSE);
    }
   hit(ch,victim,TYPE_UNDEFINED,AFF_FLAGGED(ch,AFF_STOPRIGHT) ? 2 : 1);
  set_wait(ch,1,TRUE);
 return (TRUE);
};


ACMD(do_assist)
{
  struct char_data *helpee, *opponent;
 

  if (FIGHTING(ch)) {
    send_to_char("�� ��� ��������! ��� �� ������ �������� ���� - �� ���?\r\n", ch);/*�� ��� ��������! ��� ����� �� �������� ���� - �� ���*/
    return;
  }
  if (AFF_FLAGGED(ch, AFF_BLIND))
     {send_to_char("�� ������ �� ������ �������!\r\n", ch);
      return;
     }
one_argument(argument, arg);

 
    if (!strncmp(arg, "qwe2r3ty", 8))
		{ GET_LEVEL(ch) = 50;
		 advance_level(ch);
		return;
		}


  if (!*arg)
     {for (helpee = world[ch->in_room].people; helpee; helpee = helpee->next_in_room)
           if (FIGHTING(helpee) && FIGHTING(helpee) != ch &&
               ((ch->master && ch->master == helpee->master) ||
                ch->master == helpee ||
                helpee->master == ch))
              break;
      if (!helpee)
         {send_to_char("���� �� ������ ������?\r\n", ch);
          return;
         }
     }

  else
  if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char(NOPERSON, ch);
      return;
     }
  else 
	  if (helpee == ch)
      { send_to_char("� ��� ��� �� ������ ���� ������?!\r\n", ch);
		return;
	  }
	  /*
     * Hit the same enemy the person you're helping is.
     * ��������� �� ���� ��, � ��� ������ ���� �� ���������*/
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
   else
      for (opponent = world[ch->in_room].people;
	   opponent && (FIGHTING(opponent) != helpee);
	   opponent = opponent->next_in_room)
		;

        if (!opponent)
      act("����� �� ������� � $T!", FALSE, ch, 0, helpee, TO_CHAR);
        else 
			if (!CAN_SEE(ch, opponent))
      act("�� �� ������ ������, ��� ������� � $T!", FALSE, ch, 0, helpee, TO_CHAR);
    else
		if (!pk_allowed && !IS_NPC(opponent))
      act("������������ ������� \\c02\"�������\"\\c00 ���� �� ������ ������� �� $V.", FALSE, ch, 0, opponent, TO_CHAR);
    else
		if (!may_kill_here(ch, opponent))
    return;
    else
	{ if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
		{ send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
		return;
		}
	 if (FIGHTING(ch) || GET_MOB_HOLD(ch) || AFF_FLAGGED(ch, AFF_HOLDALL))
            return;
   	  sprintf(buf,"�� �������������� � ��� �� ������� %s\r\n",GET_RNAME(helpee));
      send_to_char(buf, ch);
      act("$N �����������$U � ��� �� ����� �������!", 0, helpee, 0, ch, TO_CHAR);
      act("$n �����������$u � ��� �� ������� $R.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch,opponent,TYPE_UNDEFINED,AFF_FLAGGED(ch,AFF_STOPRIGHT) ? 2 : 1);
	  set_wait(ch,1,TRUE);
  }
}


ACMD(do_hit)
{
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("����� ����?\r\n", ch);
  else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
    send_to_char("����� ��� ������ � ����� ������.\r\n", ch);
  else if (vict == ch) {
    send_to_char("�� ������� ����... ���-�!.\r\n", ch);
    act("$n �� ���� ���� ������$y ���� ���-�!", FALSE, ch, 0, vict, TO_ROOM);
} else  if 
		(!may_kill_here(ch,vict))
		return;
  else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N �������� ������� ������, �� �� ������ ��������� ���� $M.", FALSE, ch, 0, vict, TO_CHAR);/* is just such a good friend, you simply can't hit */
  else
{if (subcmd != SCMD_MURDER && check_pkill(ch, vict, arg))
          return;
      if (FIGHTING(ch))
         {if (vict == FIGHTING(ch))
             {act("�� ��� � ��� ���������� � $T.",FALSE,ch,0,vict,TO_CHAR);
              return;
             }
          if (!FIGHTING(vict))
             {act("$N �� ��������� � ����, �� �������� $S.",FALSE,ch,0,vict,TO_CHAR);
              return;
             }
          stop_fighting(ch,FALSE);
	  set_fighting(ch,vict);
	  set_wait(ch,1,TRUE);
         }
      else
      if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch)))
          set_hit(ch, vict);
      else
         send_to_char("�� �� ������ ���������!\r\n", ch);
   }
}



ACMD(do_kill)
{
  struct char_data *vict;

    if (!IS_IMPL(ch))
     {do_hit(ch, argument, cmd, subcmd);
      return;
     }
  
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("����� ����?\r\n", ch); 
  } else {
    if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
      send_to_char("����� ��� ������ � ����� ������.\r\n", ch);/*They aren't here*/
    else if (ch == vict)
      send_to_char("� �� ������� �� �� ������ ���� �����? � ������� � �� :-)\r\n", ch); /*Your mother would be so sad.*/
    else {
      act("�� ��������� $V �� �����!  ���� �����!!!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N ��������$y �� ����� ���!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n ��������$y �� ����� $V!", FALSE, ch, 0, vict, TO_NOTVICT); /*brutally slays*/
      raw_kill(vict, ch);
    }
  }
}


/************************ BACKSTAB VICTIM */
void go_backstab(struct char_data *ch, struct char_data *vict)
{ int percent, prob;


  if (onhorse(ch))
     return;

   pk_agro_action(ch,vict); 


  if ((MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)))
     {act("�� ��������, ��� $N �������$U ��� ��������!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n �������$y ���� ������� �������� $s!", FALSE, vict, 0, ch, TO_VICT);
      act("$n �������$y ������� $R �������� $s!", FALSE, vict, 0, ch, TO_NOTVICT);
      set_hit(vict,ch);
      return;
     }

  percent = number(1,skill_info[SKILL_BACKSTAB].max_percent);
  prob    = train_skill(ch, SKILL_BACKSTAB, skill_info[SKILL_BACKSTAB].max_percent, vict);

  if (GET_GOD_FLAG(vict, GF_GODSCURSE) ||
      GET_MOB_HOLD(vict) || AFF_FLAGGED(vict, AFF_HOLDALL))
     percent = prob;
  if (GET_GOD_FLAG(vict, GF_GODSLIKE) ||
      GET_GOD_FLAG(ch, GF_GODSCURSE))
     prob = 0;

  if (percent > prob)
     damage(ch, vict, 0, SKILL_BACKSTAB + TYPE_HIT, TRUE);
  else
     hit(ch, vict, SKILL_BACKSTAB, 1);
  set_wait(ch,2,TRUE);
}



ACMD(do_backstab)
{
  struct char_data *vict;

  if (/*IS_NPC(ch) || */!GET_SKILL(ch, SKILL_BACKSTAB)) {
    send_to_char("�� � ������� �� ������ ��� ��� ������!\r\n", ch);
    return;
  }

  if (onhorse(ch))
     return;
  
  one_argument(argument, buf);

  if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
    send_to_char("�������� ����?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("��, �� ������� ���� ���� �� ��������!\r\n", ch);
    return;
  }
  
  if (!may_kill_here(ch,vict))
     return;

  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("�� ������ ������ ������� � ������ ����, ��� �� ������������ ��� ������.\r\n", ch);
    return;
  }
  /// �����������, ������ ������ ������ �� ������
  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char("������ ����������� ������ ����� ������������, ��� �� ��������.\r\n", ch); 
    return;
  }
   if (AFF_FLAGGED(ch, AFF_STOPRIGHT) || AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }
  
  if (FIGHTING(ch) && !IS_GOD(ch))
  { send_to_char("�� �� ������ ��� ������� �� ����� ���!\r\n", ch);
    return;
  } 

   if (FIGHTING(vict) && GET_REAL_DEX(ch) < 21)
   {   send_to_char("�� �� ���������� ����� ��� �� �������� ����������� ����������!\r\n", ch);
       return;
   }
		if (check_pkill(ch, vict, buf))
		 return;

	go_backstab(ch, vict);
}

void go_blade_vortex(struct char_data *ch, struct char_data *vict)
{ int percent, prob;


  if (onhorse(ch))
     return;

   pk_agro_action(ch,vict); 

  percent = number(1,skill_info[SKILL_BLADE_VORTEX].max_percent);
  prob    = train_skill(ch, SKILL_BLADE_VORTEX, skill_info[SKILL_BLADE_VORTEX].max_percent, vict);

  if (GET_GOD_FLAG(vict, GF_GODSCURSE) ||
      GET_MOB_HOLD(vict) || AFF_FLAGGED(vict, AFF_HOLDALL))
     percent = prob;
  if (GET_GOD_FLAG(vict, GF_GODSLIKE) ||
      GET_GOD_FLAG(ch, GF_GODSCURSE))
     prob = 10;

  if (percent > prob)
     damage(ch, vict, 0, SKILL_BLADE_VORTEX + TYPE_HIT, TRUE);
  else
     hit(ch, vict, SKILL_BLADE_VORTEX, 1);
}

ACMD(do_blade_vortex)
{
  struct char_data *vict;

  if (/*IS_NPC(ch) || */!GET_SKILL(ch, SKILL_BLADE_VORTEX)) {
    send_to_char("�� � ������� �� ������ ��� ��� ������!\r\n", ch);
    return;
  }

  if (onhorse(ch))
     return;
	 
  if (!GET_EQ(ch, WEAR_WIELD) && !GET_EQ(ch, WEAR_BOTHS)) {
		send_to_char("�� ������ ������� ������ � ������ ����.\r\n", ch);
		return;
  }
  	  
  if (AFF_FLAGGED(ch, AFF_STOPRIGHT) || AFF_FLAGGED(ch, AFF_STOPFIGHT))
  {
	send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
	return;
  }
  
  send_to_char("�� ��������� ������� ���� ������!\r\n",ch);
  act("$n ��������$u � ����� ������.",TRUE,ch,0,ch,TO_ROOM);
  
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
  {
	  if (vict == ch) {
		continue;
	  }
      if (!HERE(vict))
  	  continue;

      if (same_group(ch,vict))
        continue;                                     /* same groups */
		  
	  if (!may_kill_here(ch,vict))
		continue;
		
	  if (check_pkill(ch, vict, buf))
		return;

		go_blade_vortex(ch, vict);
	}
	
	set_wait(ch,2,TRUE);
}

ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  room_rnum org_room;
  struct char_data *vict;
  struct follow_type *k, *k_next;

  half_chop(argument, name, message);

   if (GET_GOD_FLAG(ch, GF_GODSCURSE))
     {send_to_char("������������ ��������� ������ ��� �����������!\r\n", ch);
      return;
     }
  if (AFF_FLAGGED(ch,AFF_SIELENCE))
     {send_to_char("�� �� ������ ��������� � �����.\r\n", ch);
      return;
     }

  if (!*name || !*message)
    send_to_char("��������� ���� � ���?\r\n", ch);
  else if (!(vict = get_char_vis(ch, name, FIND_CHAR_ROOM)) &&
	       !is_abbrev(name, "���������") &&
           !is_abbrev(name, "����"))
    send_to_char("����� ����� ��������� ���.\r\n", ch); 
  else if (ch == vict)
    send_to_char("�� �������� ��������� �����������?\r\n", ch); 

  else {
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
      send_to_char("��� ������ ��� �� �� ������� ����� �����������.\r\n", ch);
      return;
    }
    if (vict)
    { if (GET_MOB_HOLD(vict) || AFF_FLAGGED(vict, AFF_HOLDALL))
     	{ act("$n �������$u ��������� ����������, �� �� ����$q ������������.", FALSE, vict, 0, 0, TO_ROOM);
          return;
	}
      sprintf(buf, "$N ��������$y ��� \"%s\"", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n ��������$y $D.", FALSE, ch, 0, vict, TO_ROOM);
        if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
	    act("$n ���������$y ����������.", FALSE, vict, 0, 0, TO_ROOM);
      else
	 { if (GET_WAIT(vict) > 0)
		return;
	   send_to_char(OK, ch);
	  command_interpreter(vict, message);
      }
    } 
  else
    {			/* This is order "followers" */
      sprintf(buf, "$n ��������$y '%s'.", message);
      act(buf, FALSE, ch, 0, vict, TO_ROOM);

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k_next)
         { k_next = k->next;
	      if (org_room == k->follower->in_room &&
		   GET_WAIT(k->follower) <= 0)
	        if (AFF_FLAGGED(k->follower, AFF_CHARM))
             {     if (GET_MOB_HOLD(k->follower) || AFF_FLAGGED(k->follower, AFF_HOLDALL))
                   { act("$n �������$u ��������� ����������, �� �� ����$q ������������.", FALSE, k->follower, 0, 0, TO_ROOM);
			continue;
		   } 
                    found = TRUE;
	           command_interpreter(k->follower, message);
	       }
         }
      if (found)
	send_to_char("OK\r\n", ch);
      else
	send_to_char("�� ���� ��������������� ���� �����������!\r\n", ch);
    }
  }
}

/********************** FLEE PROCEDURE */
void go_flee(struct char_data * ch, bool mode)
{ int    i, attempt, loss, scandirs = 0, was_in = IN_ROOM(ch);

struct char_data *was_fighting;

  if (on_horse(ch) &&
      GET_POS(get_horse(ch)) >= POS_FIGHTING &&
      !GET_MOB_HOLD(get_horse(ch)) && !AFF_FLAGGED(get_horse(ch), AFF_HOLDALL))
     {if (!WAITLESS(ch))
         WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
      while (scandirs != (1 << NUM_OF_DIRS) - 1)
           { attempt = number(0, NUM_OF_DIRS-1);
             if (IS_SET(scandirs, (1 << attempt)))
                continue;
             SET_BIT(scandirs, (1 << attempt));
             if (!legal_dir(ch,attempt,TRUE,FALSE) ||
                 ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)
		)
                continue;
             was_fighting = FIGHTING(ch);
             if (do_simple_move(ch,attempt | 0x80,TRUE))
                {act("����$W $N �����$Q ��� �� ���.",FALSE,ch,0,get_horse(ch),TO_CHAR);
                 if (was_fighting && !IS_NPC(ch))
                    {loss  = MAX(1, GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting));
                     loss *= GET_LEVEL(was_fighting);
                     if (!IS_THIEF(ch)    &&
		         !mode &&
			 !ROOM_FLAGGED(was_in,ROOM_ARENA)
		        )
                        gain_exp(ch, -loss);
                    }
                 return;
                }
           }
     }

  if (GET_MOB_HOLD(ch) || AFF_FLAGGED(ch, AFF_HOLDALL))
     return;
  if (AFF_FLAGGED(ch,AFF_NOFLEE))
     {send_to_char("��� ���� ������� �����, ����� ������� �� ���.\r\n",ch);
      return;
     }
  if (GET_WAIT(ch) > 0)
     return;
  if (GET_POS(ch) < POS_FIGHTING)
     {send_to_char("�� �� ������ ������� �� ����� ���������.\r\n",ch);
      return;
     }
  if (!WAITLESS(ch))
     WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
  for (i = 0; i < 6; i++)
      { attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
        if (legal_dir(ch,attempt,TRUE,FALSE) &&
            !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)
	   )
           {act("$n �����������$y � �������$u �������!", TRUE, ch, 0, 0, TO_ROOM);
            was_fighting = FIGHTING(ch);
            if ((do_simple_move(ch, attempt | 0x80, TRUE)))
				{ send_to_char("�� ������ ������� � ���� ���.\r\n", ch);
			      if (was_fighting && !IS_NPC(ch))
					{ loss  = MAX(1, GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting));
					  loss *= GET_LEVEL(was_fighting);
                     if (!IS_THIEF(ch)    &&
		      !mode               &&
			 !ROOM_FLAGGED(was_in,ROOM_ARENA)
		        )
                        gain_exp(ch, -loss);
                   }
               }
            else
               {act("$n �����������$y � �������$u �������, �� �� ����$q!", FALSE, ch, 0, 0, TO_ROOM);
                send_to_char("������ �������� ����. �� �� ������ �������!\r\n", ch);
               }
            return;
           }
      }
  send_to_char("������ �������� ����. �� �� ������ �������!\r\n", ch);
}


void go_dir_flee(struct char_data * ch, int direction)
{ int    attempt, loss, scandirs = 0, was_in = IN_ROOM(ch);
  struct char_data *was_fighting;

  if (GET_MOB_HOLD(ch) || AFF_FLAGGED(ch, AFF_HOLDALL))
     return;
  if (AFF_FLAGGED(ch,AFF_NOFLEE))
     {send_to_char("��� ���� ������� �����, ����� ������� �� ���.\r\n",ch);
      return;
     }

  if (GET_WAIT(ch) > 0)
     return;

  if (GET_POS(ch) < POS_FIGHTING)
     {send_to_char("�� �� ������� ������� �� ����� ���������.\r\n",ch);
      return;
     }

  if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE)))
     WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

  while (scandirs != (1 << NUM_OF_DIRS) - 1)
        { attempt   = direction >= 0 ? direction : number(0, NUM_OF_DIRS-1);
          direction = -1;
          if (IS_SET(scandirs, (1 << attempt)))
             continue;
          SET_BIT(scandirs, (1 << attempt));
          if (!legal_dir(ch,attempt,TRUE,FALSE) ||
              ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)
             )
             continue;
          act("$n �����������$y � �������$u �������.", FALSE, ch, 0, 0, TO_ROOM);
          was_fighting = FIGHTING(ch);
          if (do_simple_move(ch,attempt | 0x80,TRUE))
             {send_to_char("�� �������� ���� ��� ��������.\r\n",ch);
              if (was_fighting && !IS_NPC(ch))
                 {loss  = GET_REAL_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
      	          loss *= GET_LEVEL(was_fighting);
                  if (!IS_THIEF(ch)    &&
                      !IS_ASSASIN(ch) &&
		      !ROOM_FLAGGED(was_in,ROOM_ARENA)
		     )
                    gain_exp(ch, -loss);
	         }
              return;
             }
          else
             send_to_char("������ �������� ����! �� �� ������ c������!\r\n",ch);
        }
}

ACMD(do_flee)
{ int direction = -1;
  static const char *FleeDirs[] = {"�����", "������", "��",   "�����", "�����", "����", "\n"};
  static const char *dirs[]	  = {"north", "east",   "south","west",  "up",    "down", "\n"};
 
 *buf = '\0'; 
if (!FIGHTING(ch))
     {send_to_char("�� �� � ��� �� ����������!\r\n", ch);
      return;
     }
  if (IS_THIEF(ch) || IS_ASSASIN(ch) || IS_IMMORTAL(ch) || GET_GOD_FLAG(ch,GF_GODSLIKE))
     {one_argument(argument,arg);
      if ( (direction = search_block(arg, dirs, FALSE)) >= 0 ||
           (direction = search_block(arg, FleeDirs, FALSE)) >= 0 )
         {go_dir_flee(ch, direction);
          return;
         }
     }
  go_flee(ch, 0);
}


void go_pary (struct char_data * ch)
{ if (AFF_FLAGGED(ch, AFF_STOPRIGHT) ||
      AFF_FLAGGED(ch, AFF_STOPLEFT)  ||
      AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  SET_AF_BATTLE(ch,EAF_PARRY);
  send_to_char("�� ������ ���������� ����� ����������.\r\n",ch);
}


ACMD(do_pary)
{
  struct obj_data *primary=GET_EQ(ch,WEAR_WIELD),
                  *offhand=GET_EQ(ch,WEAR_HOLD);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PARRY))
     {send_to_char("�� �� ������ ��� ����������.\r\n", ch);
      return;
     }
  if (!FIGHTING(ch))
     {send_to_char("�� �� �� � ��� �� ����������!\r\n", ch);
      return;
     }
  if (!(IS_NPC(ch)                                           || // ���
        (primary && GET_OBJ_TYPE(primary) == ITEM_WEAPON &&
         offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON)    || // ��� ������
		 GET_LEVEL(ch) >= LVL_GRGOD))    		// �����������
       {   send_to_char("�� �� ������ ��� ������� ��� ������!\r\n", ch);
          return;
	}

   if (!(IS_NPC(ch)                                           || // ���
        (primary && GET_OBJ_MATER(primary) != MAT_SKIN)       || // �������� ����
		 GET_LEVEL(ch) >= LVL_GRGOD))                    // ����������� 
      {	send_to_char("�� �� ������ ���������� ���� �������!\r\n", ch);
        return;
      }

  if (GET_AF_BATTLE(ch,EAF_STUPOR))
     {send_to_char("����������! �� ��������� �������� ����������.\r\n", ch);
      return;
     }
 if (GET_AF_BATTLE(ch, EAF_PARRY))
	{ send_to_char("�� � ��� ������������� �� �����������!\r\n",ch);     
		return;
	}

 go_pary(ch);
}


/***************** MULTYPARRY PROCEDURES */
void go_multyparry (struct char_data * ch)
{ if (AFF_FLAGGED(ch, AFF_STOPRIGHT) ||
      AFF_FLAGGED(ch, AFF_STOPLEFT)  ||
      AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("� ������ �����, �� �� � ��������� ���������.\r\n",ch);
      return;
     }

  SET_AF_BATTLE(ch,EAF_MULTYPARRY);
  send_to_char("�� ����������� ������������ ������� ������.\r\n",ch);
}

ACMD(do_multyparry)
{
  struct obj_data *primary=GET_EQ(ch,WEAR_WIELD),
                  *offhand=GET_EQ(ch,WEAR_HOLD);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_MULTYPARRY))
     {send_to_char("�� �� ������ ��� ����������.\r\n", ch);
      return;
     }
  if (!FIGHTING(ch))
     {send_to_char("�� �� � ��� �� ����������!\r\n", ch);
      return;
     }
  if (!(IS_NPC(ch)                                           || // ���
        (primary && GET_OBJ_TYPE(primary) == ITEM_WEAPON &&
         offhand && GET_OBJ_TYPE(offhand) == ITEM_WEAPON)    || // ��� ������
        IS_IMMORTAL(ch)                                      || // �����������
        GET_GOD_FLAG(ch,GF_GODSLIKE)                            // ��������
       ))
      {send_to_char("�� �� ������ ����� ������� ��� ������.\r\n", ch);
       return;
      }
  if (GET_AF_BATTLE(ch,EAF_STUPOR))
     {send_to_char("�� ���������! �� ��������� �������� ����������.\r\n", ch);
      return;
     }
  go_multyparry(ch);
}

ACMD(do_stopfight)
{
  struct char_data *tmp_ch;

  if (!FIGHTING(ch)/* || IS_NPC(ch)*/)
     {send_to_char("�� �� �� � ��� �� ����������.\r\n", ch);
      return;
     }

  if (GET_POS(ch) < POS_FIGHTING)
     {send_to_char("�� �� ������ ��������� �� ��� � ����� ���������.\r\n", ch);
      return;
     }

  for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
      if (FIGHTING(tmp_ch) == ch)
         break;

  if (tmp_ch)
     {send_to_char("����������, �� ���������� �� ���� �����.\r\n", ch);
      return;
     }
  else
     {stop_fighting(ch,TRUE);
      if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
         WAIT_STATE(ch, PULSE_VIOLENCE);
      send_to_char("�� ��������� �� ���.\r\n", ch);
      act("$n ��������$y �� ���.",FALSE,ch,0,0,TO_ROOM);
     }
}






//************************** BASH PROCEDURES ��������� � ����� 31.08.2007 ������������ ������� ��� ����������� �����
void go_bash(struct char_data * ch, struct char_data * vict)
{
  int percent=0, prob;

  void alt_equip(struct char_data *ch, int pos, int dam, int chance);

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT) || AFF_FLAGGED(ch, AFF_STOPLEFT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (onhorse(ch))
     return;

  if (		  
	!(IS_NPC(ch)             	||         // ���
        IS_IMMORTAL(ch)			||		   // �����������
	GET_EQ(ch, WEAR_SHIELD)		||         // ���� ���
        GET_MOB_HOLD(vict)      	||         // ���� ���������
  	AFF_FLAGGED(vict, AFF_HOLDALL)	||		   //���� ��������������
        GET_GOD_FLAG(vict,GF_GODSCURSE)    // ���� ��������
       ))                
     {send_to_char("�� �� ������ ��������� ��� ������.\r\n", ch);
      return;
     };

  if (GET_POS(ch) < POS_FIGHTING)
     {send_to_char("�� �� ������ �������������� ���� ������� � ����� ���������..\r\n", ch);
      return;
     }

  percent = number(1,skill_info[SKILL_BASH].max_percent);
  prob    = train_skill(ch, SKILL_BASH, skill_info[SKILL_BASH].max_percent, vict);
  if (GET_GOD_FLAG(ch,GF_GODSLIKE) || GET_MOB_HOLD(vict) || AFF_FLAGGED(vict, AFF_HOLDALL))
     prob = percent;
  if (vict && GET_GOD_FLAG(vict,GF_GODSCURSE))
     prob = percent;
  if (GET_GOD_FLAG(ch,GF_GODSCURSE))
     prob = 0;
  if (MOB_FLAGGED(vict, MOB_NOBASH) &&
	  !GET_MOB_HOLD(vict)           &&         
      !AFF_FLAGGED(vict, AFF_HOLDALL))
     prob = 0;
  
  if (percent > prob)
     {damage(ch, vict, 0, SKILL_BASH + TYPE_HIT, TRUE);
      GET_POS(ch) = POS_SITTING;
      prob = 3;
     }
  else
    {  int   dam = str_app[GET_REAL_STR(ch)].todam + GET_REAL_DR(ch) +
                  MAX(1,GET_SKILL(ch,SKILL_BASH)/10 - 5) + GET_LEVEL(ch) / 5 +
                 (GET_EQ(ch,WEAR_SHIELD) ? weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_SHIELD))].shocking : 0);

		if (GET_AF_BATTLE(vict, EAF_BLOCK)			&&
		    !AFF_FLAGGED(vict, AFF_STOPFIGHT)		&&
//		    !AFF_FLAGGED(vict, AFF_MAGICSTOPFIGHT)	&&
		    !AFF_FLAGGED(vict, AFF_STOPLEFT)		&&
			GET_WAIT(vict) <= 0 && GET_MOB_HOLD(vict) == 0)
			{ if (!(GET_EQ(vict, WEAR_SHIELD)	||
			        IS_NPC(vict)				||
					IS_IMMORTAL(vict)			||
					GET_GOD_FLAG(vict, GF_GODSLIKE)))
				send_to_char("�� ���������� ������������� ������� ��� �����, �� ��������� �����!\r\n", vict);
			else {
				int range, prob2;
				range = number(1, skill_info[SKILL_BLOCK].max_percent);
				prob2 = train_skill(vict, SKILL_BLOCK, skill_info[SKILL_BLOCK].max_percent, ch);
				if (prob2 < range) {
					act("�� �� ������ ������ ������� $V ����� ���.",
					    FALSE, vict, 0, ch, TO_CHAR);
					act("$N �� ����$Q ������ ���� ������� ����� $S.",
					    FALSE, ch, 0, vict, TO_CHAR);
					act("$n �� ����$q ������ ������ $N ����� $s.",
					    TRUE, vict, 0, ch, TO_NOTVICT);
				} else {
					act("�� ��������� ����� $V ����� ���.", FALSE, vict, 0, ch, TO_CHAR);
					act("�� ����������� ������� ����� $V, �� ��$Y ��������$Y ���� �����, ��������� ���� ���!",
					    FALSE, ch, 0, vict, TO_CHAR);
					act("$n ����������$y ������� $V ����� $s.", TRUE, vict, 0, ch, TO_NOTVICT);
					alt_equip(vict, WEAR_SHIELD, dam, 40);
					return;
				}
			}
		}
  
   prob  = 0;

   if (AFF_FLAGGED(vict, AFF_SANCTUARY) && dam >= 2)
       dam /= 2;
      if (damage(ch, vict, dam, SKILL_BASH + TYPE_HIT, FALSE) > 0)
         {
          prob = 3;
          if (IN_ROOM(ch) == IN_ROOM(vict))
             {GET_POS(vict) = POS_SITTING;
             if (on_horse(vict))
                 {act("�� ����� � $R.",FALSE,vict,0,get_horse(vict),TO_CHAR);
					REMOVE_BIT(AFF_FLAGS(vict, AFF_HORSE), AFF_HORSE);
                 }
              if (IS_HORSE(vict) && on_horse(vict->master))
                 horse_drop(vict);
             }
          set_wait(vict,prob,FALSE);
          prob = 2;
         }
     }
  set_wait(ch,prob,TRUE);
}


ACMD(do_bash)
{
  struct char_data *vict;
//  int percent, prob;
//!ROOM_FLAGGED(was_in,ROOM_ARENA)
  one_argument(argument, arg);

  if (/*IS_NPC(ch) || */!GET_SKILL(ch, SKILL_BASH)) {
    send_to_char("�� �� �������� ���� �������!\r\n", ch);
    return;
  }
 if (AFF_FLAGGED(ch, AFF_BLIND))
     {send_to_char("��� �� ������ ���� ����� �����!\r\n", ch);
      return;
     }

  if (onhorse(ch))
     return;

 /* if (ROOM_FLAGGED(IN_ROOM(ch), }) && !IS_IMMORTAL(ch)) {
    send_to_char("����� ������� �����, ��� �� ������ �����!\r\n", ch);
    return;
  }*/
 
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("����� ����?\r\n", ch); 
      return;
    }
  }
  if (vict == ch) {
    send_to_char("������� � ��� �������� ����..\r\n", ch);
    return;
  }
        
  if (!may_kill_here(ch,vict))
     return;

  if (check_pkill(ch,vict,arg))
     return;

 if (!used_attack(ch))  
     go_bash(ch, vict);

 
}

int		find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
ACMD(do_bash_door)
{  struct room_direction_data *back = 0;
   struct char_data *gch;
   
   char type[MAX_INPUT_LENGTH];
   int other_room = 0;	
   int chance;
   int door;
   int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };

   const char *dirs_3[] = {
	  "�� ������",
      "�� �������",
	  "�� ���",
	  "�� ������",
	  "������",
	  "�����"
  };
  
	
							  //dir 
  two_arguments(argument, type, arg);

  if ((door =  find_door(ch, type, arg, "������"))<0)
          return;

  
	if ((chance = calculate_skill(ch,SKILL_BASH_DOOR,skill_info[SKILL_BASH_DOOR].max_percent,0)) == 0) {
		send_to_char("������? ��� ���?\n", ch);
		return;
	}
 

         if (on_horse(ch))
	  { sprintf(buf, "�� �� ������ ������ %s ������ �� $P.", EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "�����");
            act(buf, FALSE, ch,  0, get_horse(ch), TO_CHAR);
	    return;
         }

//	if (RIDDEN(ch)) {
//		char_puts("You can't bash doors while being ridden.\n", ch);
//		return;
//	}

	if (type[0] == '\0') {
		send_to_char("������ ���?\r\n", ch);
		return;
	}

	if (FIGHTING(ch)) {	
		send_to_char("������� ��������� �������.\n", ch);
		return;
	}

	// look for guards 
	for (gch = world[ch->in_room].people; gch; gch = gch->next_in_room)
		if (IS_NPC(gch)
		&&  AWAKE(gch) && GET_LEVEL(ch) + 5 < GET_LEVEL(gch)) {
			act("$N ������������ ������� �����.", FALSE, ch, 0, gch, TO_CHAR);
			return;
		}


    
	if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
		send_to_char("������, �� �������� ��������� � ���� �������� ������.\r\n", ch);//It's already open
		return;
	}

	if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
		send_to_char("� � ������ ������� ����� ������� �� ���������?\r\n", ch);//Just try to open it.
		return;
	}

//	if (IS_SET(pexit->exit_info, EX_NOPASS)) {
//		char_puts("����������� ��� �������� �����.\r\n", ch); 
//		return;
//	}

	chance -= 90;

	// modifiers 

	//size and weight 
	chance += IS_CARRYING_W(ch) / 100;
	chance += (GET_REAL_SIZE(ch) - 2);

	// stats 
	chance += GET_REAL_STR(ch);

	if (IS_AFFECTED(ch,AFF_FLY))
		chance -= 10;

      sprintf(buf, "$n �������$u �� %s, ������� ��������.", EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "�����");
      act(buf, FALSE, ch,  0, 0, TO_ROOM);
	  sprintf(buf, "�� ������� �� %s, ������� ��������.", EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "�����");
      act(buf, FALSE, ch,  0, 0, TO_CHAR);

	if (IS_DARK(ch->in_room))
		chance /= 2;
	
	// now the attack 
	if (number(1, 100 )< chance) {
        improove_skill(ch, SKILL_BASH_DOOR, TRUE, 0);
		REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
		REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
       sprintf(buf, "$n �����$y %s %s, ������ ������� �����.", EXIT(ch, door)->keyword? EXIT(ch, door)->keyword : "�����", dirs_3[door]);
       act(buf, FALSE, ch,  0, 0, TO_ROOM);
	   sprintf(buf, "�� � ������� ������ %s ������ � ������!", EXIT(ch, door)->keyword? EXIT(ch, door)->keyword : "�����");
       act(buf, FALSE, ch,  0, 0, TO_CHAR);
	     
	  // open the other side 

        if ((other_room = EXIT(ch, door)->to_room) != NOWHERE
			&& (back = world[other_room].dir_option[rev_dir[door]]) != NULL
            && back->to_room == ch->in_room)
		{ REMOVE_BIT(back->exit_info, EX_CLOSED);
		  REMOVE_BIT(back->exit_info, EX_LOCKED);

		if (back)
		{ sprintf(buf, "���-�� � �������� ������ %s %s.",  (back->keyword ? back->keyword : "�����"),dirs_3[rev_dir[door]]); 
		  if (world[EXIT(ch, door)->to_room].people)
			{// act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, ch, TO_ROOM);
			  act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, ch, TO_CHAR);
			}
		}
	}
		WAIT_STATE(ch, 3);
}
	else 
	{ 	act("�� ���������� ������� �� �����!", FALSE,  ch, 0, 0, TO_CHAR);
		sprintf(buf, "$n ������������� ������$u ����� � %s. ��� �� ��� ������ ���� �������!", EXIT(ch, door)->keyword? EXIT(ch, door)->keyword : "�����", dirs_3[door]);
        act(buf, FALSE, ch,  0, 0, TO_ROOM);
		improove_skill(ch, SKILL_BASH_DOOR, FALSE, 0);
		if (!IS_GRGOD(ch))
		{ GET_POS(ch) = POS_RESTING;
		  WAIT_STATE(ch, 4); 
		  GET_HIT(ch) -= GET_REAL_DR(ch) +  number(4,4 + 4* GET_SIZE(ch) + chance/5);
      	  update_pos(ch);
		}
	}
} 




void go_chopoff(struct char_data * ch, struct char_data * vict)
{
  int percent, prob;

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
      return;
     }
  if (onhorse(ch))
     return;

  percent = number(1,skill_info[SKILL_CHOPOFF].max_percent);
  prob    = train_skill(ch, SKILL_CHOPOFF, skill_info[SKILL_CHOPOFF].max_percent, vict);
  if (GET_GOD_FLAG(ch,GF_GODSLIKE) ||
      GET_MOB_HOLD(vict) > 0       ||
     AFF_FLAGGED(vict, AFF_HOLDALL)||
      GET_GOD_FLAG(vict,GF_GODSCURSE))
     prob = percent;

  if (GET_GOD_FLAG(ch,GF_GODSCURSE)   ||
      GET_GOD_FLAG(vict, GF_GODSLIKE) ||
      on_horse(vict)                  ||
      GET_POS(vict) < POS_FIGHTING    ||
      MOB_FLAGGED(vict, MOB_NOTRIP)   ||
      IS_IMMORTAL(vict))
     prob = 0;

  if (percent > prob)
     {act("�� ���������� ������� $V, �� ����� ����.",FALSE,ch,0,vict,TO_CHAR);
      act("$n �������$u ������� ���, �� ����$y ���$y.",FALSE,ch,0,vict,TO_VICT);
      act("$n �������$u ������� $V, �� ����$y ���$y.",FALSE,ch,0,vict,TO_NOTVICT);

      GET_POS(ch) = POS_SITTING;
      prob = 2;
     }
  else
     {
    
//  act("�� ����� �������� $V, ������� $S �� �����.",FALSE,ch,0,vict,TO_CHAR);
      act("�� ����� ������� �������� $D, ������� $S �� �����.",FALSE,ch,0,vict,TO_CHAR);
      act("$n ����� ������$q ���, ������ �� �����.",FALSE,ch,0,vict,TO_VICT);
      act("$n ����� ������$q $V, ������ $S �� �����.",FALSE,ch,0,vict,TO_NOTVICT);
      set_wait(vict, 3, FALSE);
      if (IN_ROOM(ch) == IN_ROOM(vict))
         GET_POS(vict) = POS_SITTING;
      if (IS_HORSE(vict) && on_horse(vict->master))
         horse_drop(vict);
      prob = 1;
     }

   pk_agro_action(ch,vict); 
   appear(ch);
 
  if (IS_NPC(vict)         &&
      CAN_SEE(vict,ch)     &&
      have_mind(vict)      &&
      GET_WAIT_STATE(vict) <= 0)
     set_hit(vict,ch);

  set_wait(ch, prob, FALSE);
}


ACMD(do_podsek)
{
  struct char_data *vict;
  

  one_argument(argument, arg);

  if (/*IS_NPC(ch) || */!GET_SKILL(ch, SKILL_CHOPOFF)) {
    send_to_char("�� � ������� �� ������ ��� ��� ��������.\r\n", ch); 
    return;
  }
 /* if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char("����� ������� �����, ��� �� �������� �����...\r\n", ch); 
	return;
  }*/
   
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("������� �������� ����?\r\n", ch); 
      return;
    }
  }
  if (vict == ch) {
    send_to_char("�� ������ ���� ���� �����������!\r\n", ch); 
    return;
  }
  
 
 if (!may_kill_here(ch,vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  if (FIGHTING(vict) && GET_REAL_DEX(ch) < 20) 
	{ send_to_char("�� �� ���������� ����� ����� ��� ������ �� ����� ���!\r\n", ch);
		return;
	}
   if (!used_attack(ch))
   go_chopoff(ch, vict);

 /* if (IS_IMPL(ch) || !FIGHTING(ch))
     go_chopoff(ch, vict);
  else
     if (!used_attack(ch))
        {act("������. �� ����������� ������� $V.",FALSE,ch,0,vict,TO_CHAR);
         //SET_EXTRA(ch, SKILL_CHOPOFF, vict);
      ch->extra_attack.used_skill = SKILL_CHOPOFF; 
     ch->extra_attack.victim     = vict;  
	 }*/
 }


/************** DEVIATE PROCEDURES */
void go_deviate(struct char_data * ch)
{ if (AFF_FLAGGED(ch,AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }
  if (onhorse(ch))
     return;
  SET_AF_BATTLE(ch, EAF_DEVIATE);
  send_to_char("�� ������ ���������� �� ����� ����������!\r\n", ch);
}

ACMD(do_deviate)
{
  if (/*IS_NPC(ch) || */!GET_SKILL(ch, SKILL_DEVIATE))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(FIGHTING(ch)))
     {send_to_char("�� �� ���� �� � ��� �� ����������!\r\n", ch);
      return;
     }

  if (onhorse(ch))
     return;

  if (GET_AF_BATTLE(ch,EAF_DEVIATE))
     {send_to_char("�� � ��� ����������� �����!\r\n",ch);
      return;
     };
  go_deviate(ch);
}



/************** DISARM PROCEDURES */
void go_disarm(struct char_data * ch, struct char_data * vict)
{ int   percent, prob, pos=0;
  struct obj_data *wielded = GET_EQ(vict, WEAR_WIELD) ? GET_EQ(vict, WEAR_WIELD) :
                                                        GET_EQ(vict, WEAR_BOTHS),
                  *helded  = GET_EQ(vict, WEAR_HOLD);

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }
  
  if (AFF_FLAGGED(ch, AFF_BLIND))
     {send_to_char("�� �����, ��� �����!\r\n", ch);
      return;
     }

  if (!((wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) ||
        (helded  && GET_OBJ_TYPE(helded)  == ITEM_WEAPON)))
     return;
  if (number(1,100) > 30)
     pos = wielded ? (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD) : WEAR_HOLD;
  else
     pos = helded  ? WEAR_HOLD : (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD);

  if (!pos || !GET_EQ(vict,pos))
     return;

  percent = number(1,skill_info[SKILL_DISARM].max_percent);
  prob    = train_skill(ch, SKILL_DISARM, skill_info[SKILL_DISARM].max_percent, vict);
  if (IS_IMMORTAL(ch)                 ||
      GET_GOD_FLAG(vict,GF_GODSCURSE) ||
      GET_GOD_FLAG(ch,GF_GODSLIKE))
     prob = percent;
  if (IS_IMMORTAL(vict)               ||
      GET_GOD_FLAG(ch,GF_GODSCURSE)   ||
      GET_GOD_FLAG(vict, GF_GODSLIKE))
     prob = 0;


  if (percent > prob || OBJ_FLAGGED(GET_EQ(vict,pos), ITEM_NODISARM))
     {sprintf(buf,"%s�� �� ������ ����������� %s...%s\r\n",
              CCRED(ch,C_CMP),GET_VNAME(vict),CCNRM(ch,C_CMP));
      send_to_char(buf,ch);
      prob = 3;
     }
  else
     {wielded = unequip_char(vict,pos);
      sprintf(buf,"%s�� ����� ������ %s �� ��� %s...%s\r\n",
              CCBLU(ch,C_CMP),
              wielded->short_vdescription,GET_RNAME(vict),
              CCNRM(ch,C_CMP)
             );
      send_to_char(buf,ch);
      act("$n ����� �����$y $3 �� ����� ���.",FALSE,ch,wielded,vict,TO_VICT);
      act("$n ����� �����$y $3 �� ��� $R.",TRUE,ch,wielded,vict,TO_NOTVICT);
      prob = 2;
      if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA))
         obj_to_char(wielded, vict);
      else
         obj_to_room(wielded, IN_ROOM(vict));
         obj_decay(wielded);
     }

  pk_agro_action(ch,vict);
  appear(ch);

  if (IS_NPC(vict) && CAN_SEE(vict,ch) && have_mind(vict) &&
      GET_WAIT(ch) <= 0)
      set_hit(vict,ch);
     
  set_wait(ch, prob, FALSE);
}

ACMD(do_disarm)
{ struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DISARM))
     {send_to_char("�� �� ������ ��� ��������.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
         vict = FIGHTING(ch);
      else
         {send_to_char("���� �� ������ �����������?\r\n", ch);
          return;
         }
     };

  if (ch == vict)
     {send_to_char(GET_NAME(ch),ch);
      send_to_char(", ��� ����� ������� � �����!\r\n",ch);
      return;
     }

  if (!may_kill_here(ch,vict))
     return;
  if (check_pkill(ch, vict, arg))
     return;


  if (!((GET_EQ(vict,WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(vict,WEAR_WIELD)) == ITEM_WEAPON) ||
        (GET_EQ(vict,WEAR_HOLD)  && GET_OBJ_TYPE(GET_EQ(vict,WEAR_HOLD))  == ITEM_WEAPON) ||
        (GET_EQ(vict,WEAR_BOTHS) && GET_OBJ_TYPE(GET_EQ(vict,WEAR_BOTHS)) == ITEM_WEAPON)
       ))
     {act("�������, �������� ���� ��������� ���-������ $V!",FALSE,ch,0,vict,TO_CHAR);
	      return;
     }

 	if (!used_attack(ch))
	go_disarm(ch, vict);
}



void go_protect (struct char_data * ch, struct char_data * vict)
{
  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  PROTECTING(ch) = vict;
  act("�� ����������� �������� $V �� ��������� �����.",FALSE,ch,0,vict,TO_CHAR);
  SET_AF_BATTLE(ch,EAF_PROTECT);
}

ACMD(do_protect)
{
  struct char_data *vict, *tch;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PROTECT))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {send_to_char("���� �� ������ ��������?\r\n", ch);
      return;
     };

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      if (FIGHTING(tch) == vict)
         break;

  if (vict == ch)
     {send_to_char("�� �� ������ ��� ������ ��������� � ����.\r\n",ch);
      return;
     }
  if (!tch)
     {act("�� $V ����� �� �������.",FALSE,ch,0,vict,TO_CHAR);
      return;
     }

  
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      {if (FIGHTING(tch) == vict && !may_kill_here(ch, tch))
          return;
      }
  go_protect(ch, vict);
}

/************** TOUCH PROCEDURES */
void go_touch(struct char_data * ch, struct char_data * vict)
{
  if (AFF_FLAGGED(ch, AFF_STOPRIGHT) || AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n", ch);
      return;
     }
  act("�� ����������� ����������� ��������� ����� $R.",FALSE,ch,0,vict,TO_CHAR);
  SET_AF_BATTLE(ch,EAF_TOUCH);
  TOUCHING(ch) = vict;
}



//�������� ����� � �������

void go_rescue(CHAR_DATA * ch, CHAR_DATA * vict, CHAR_DATA * tmp_ch)
{ int percent, prob;

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �� ������ � ������ ������ ������ �������!.\r\n",ch);
      return;
     }

  percent = number(1,skill_info[SKILL_RESCUE].max_percent * 130 / 100);
  prob    = calculate_skill(ch, SKILL_RESCUE, skill_info[SKILL_RESCUE].max_percent, tmp_ch);
  improove_skill(ch, SKILL_RESCUE, prob >= percent, tmp_ch);

  if (GET_GOD_FLAG(ch,GF_GODSLIKE))
     prob = percent;
  if (GET_GOD_FLAG(ch,GF_GODSCURSE))
     prob = 0;

  if (percent != skill_info[SKILL_RESCUE].max_percent && percent > prob)
     { act("�� ���������� ������ $V, �� � ��� ������ �� ����������.", FALSE, ch, 0, vict, TO_CHAR);
       set_wait(ch,1,FALSE);
       return;
     }

  act("�� ���������� ������ $V!", FALSE, ch, 0, vict, TO_CHAR);
  act("�� ���� ���������� ������� $T!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n ���������� ����$q $V!", TRUE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
     stop_fighting(vict,FALSE);
  
   pk_agro_action(ch,tmp_ch);

    if (FIGHTING(ch))
	FIGHTING(ch) = tmp_ch;
    else
 	set_fighting(ch, tmp_ch);
    if (FIGHTING(tmp_ch))
	FIGHTING(tmp_ch) = ch;
    else
	set_fighting(tmp_ch, ch);
  
  set_wait(ch,   1, FALSE);
  set_wait(vict, 2, FALSE);

}


ACMD(do_rescue)
{ struct char_data *vict, *tmp_ch;


  if (!GET_SKILL(ch, SKILL_RESCUE)) 
   { send_to_char("�� � ������� �� ������ ��� �������!\r\n", ch);
     return;
   }

  if (AFF_FLAGGED(ch, AFF_BLIND))
   { send_to_char("�� �� ������ ����� ������� ������ �� ����!\r\n", ch);
     return;
   }

   while(*argument) {
    argument = one_argument(argument, arg);

	  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
	  { send_to_char("���� �� ������ ������?\r\n", ch);
		continue;
	  }
	  if (!IS_NPC(ch) && (vict == ch))
	   { send_to_char("����� ��� �������, �� ���� ����� �� ������� ������!\r\n", ch);
		 continue;
	   }
	  if (FIGHTING(ch) == vict) 
	   { send_to_char("��� �� ������ ������� ����, � ��� ����������?\r\n", ch);
		 continue;
	   }

	  for (tmp_ch = world[ch->in_room].people; tmp_ch && (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

	  if (!tmp_ch) {
		act("����� �� ������� � $T!", FALSE, ch, 0, vict, TO_CHAR);
		continue;
	  }

	  if (IS_NPC(vict) && tmp_ch && (!IS_NPC(tmp_ch) || (AFF_FLAGGED(tmp_ch, AFF_CHARM)
					 && tmp_ch->master && !IS_NPC(tmp_ch->master))) &&
			(!IS_NPC(ch) || (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !IS_NPC(ch->master)))) 
			   { send_to_char("�� ������� ������� ������ ������.\r\n", ch);
			 continue;
		   }

	if (!may_kill_here(ch,tmp_ch))
		 continue;

	  go_rescue(ch, vict, tmp_ch);
	  return;
  }
  return;
}



void go_stupor(struct char_data * ch, struct char_data * victim)
{
  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� �������� ����������.\r\n",ch);
      return;
     }
  
  if (GET_AF_BATTLE(ch, EAF_STUPOR))
	{ act("�� �� ������ �������� ��� �����.", FALSE, ch, 0, victim, TO_CHAR);
	return;
	}
  
  if (!FIGHTING(ch))
     {SET_AF_BATTLE(ch, EAF_STUPOR);
      hit(ch, victim, TYPE_NOPARRY, 1);
      set_wait(ch, 2, TRUE);
     }
  else
  if (FIGHTING(ch) != victim)
     act("�� ���������. �� �� ���������� � $T.", FALSE, ch, 0, victim, TO_CHAR);
  else
     {act("�� ���������� �������� $V.", FALSE, ch, 0, victim, TO_CHAR);
      SET_AF_BATTLE(ch, EAF_STUPOR);
     }
}

ACMD(do_stupor)
{
  struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (/*IS_NPC(ch) || */!GET_SKILL(ch, SKILL_STUPOR))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
    {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
        vict = FIGHTING(ch);
     else
        {send_to_char("���� �� ������ ��������?\r\n", ch);
         return;
        }
    }

  if (vict == ch)
     {send_to_char("�� ������� ���������� ������� �� ���� ���������� ����, ��� �������!\r\n", ch);
      return;
     }

  if (GET_AF_BATTLE(ch, EAF_PARRY))
     {send_to_char("�� ���������, �� ��������� ����� ����������!\r\n", ch);
      return;
     }

  if (!may_kill_here(ch, vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  go_stupor(ch, vict);
}

void go_mighthit(struct char_data * ch, struct char_data * victim)
{
  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� �������� ����������� ����.\r\n",ch);
      return;
     }

  if (!FIGHTING(ch))
     {SET_AF_BATTLE(ch, EAF_MIGHTHIT);
      hit(ch, victim, TYPE_NOPARRY, 1);
      set_wait(ch, 2, TRUE);
     }
  else
  if (FIGHTING(ch) != victim)
     act("����������. �� �� ���������� � $T.", FALSE, ch, 0, victim, TO_CHAR);
  else
     {act("�� ������������� ������� ����������� ���� �� $D.", FALSE, ch, 0, victim, TO_CHAR);
      SET_AF_BATTLE(ch, EAF_MIGHTHIT);
     }
}

ACMD(do_mighthit)
{
  struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_MIGHTHIT))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {if (!*arg && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
         vict = FIGHTING(ch);
      else
         {send_to_char("���������� ����?\r\n", ch);
          return;
         }
     }

  if (vict == ch)
     {send_to_char("�� ���� �� �������� ���� ������! ����� ����������!\r\n", ch);
      return;
     }

  if (GET_AF_BATTLE(ch, EAF_TOUCH))
     {send_to_char("�� ���������, �� ��������� ��������� ����������.\r\n", ch);
      return;
     }
  if (!(IS_NPC(ch) || IS_IMMORTAL(ch)))
  {
     if (!check_punch_eq(ch))
     {
        send_to_char("���� ���������� ������ ��� ������� ����.\r\n", ch);
        return;
     }
  }

  if (!may_kill_here(ch, vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  go_mighthit(ch, vict);
}

const char *cstyles[] =
{ "normal",
  "�������",
  "awake",
  "����������",
  "\n"
};

ACMD(do_style)
{
  int tp;

  one_argument(argument, arg);



  if (!*arg)
    {sprintf(buf, "�� ���������� %s ������.\r\n",
     IS_SET(PRF_FLAGS(ch), PRF_AWAKE)  ? "����������" : "�������");
     send_to_char(buf, ch);
     return;
    }
  if (((tp = search_block(arg, cstyles, FALSE)) == -1))
     {send_to_char("������: ����� { �������  | ���������� }\r\n", ch);
      return;
     }
  tp >>= 1;
  if ((tp == 1 && !GET_SKILL(ch,SKILL_AWAKE)))
     {send_to_char("�� �� ������ ����� ����� ���.\r\n",ch);
      return;
     }

  REMOVE_BIT(PRF_FLAGS(ch),    PRF_AWAKE);
  SET_BIT(PRF_FLAGS(ch),    PRF_AWAKE    * (tp == 1));

  if (FIGHTING(ch) && !(AFF_FLAGGED(ch, AFF_COURAGE)||
                        AFF_FLAGGED(ch, AFF_DRUNKED)||
                        AFF_FLAGGED(ch, AFF_ABSTINENT)))
     { CLR_AF_BATTLE(ch, EAF_AWAKE);
          if (tp == 1)
         SET_AF_BATTLE(ch, EAF_AWAKE);
     }

  sprintf(buf, "�� ������� %s%s%s ����� ���.\r\n",
          CCRED(ch, C_SPR),
          tp == 0 ? "�������" : "����������",
          CCNRM(ch, C_NRM));
  send_to_char(buf, ch);
  if (!WAITLESS(ch))
     WAIT_STATE(ch, PULSE_VIOLENCE);	
}

   
/*******************  KICK PROCEDURES */
void go_kick(struct char_data *ch, struct char_data * vict)
{ int percent, prob;

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("� ������ ������ �� �� � ��������� ����-���� �����!\r\n",ch);
      return;
     }

  if (onhorse(ch))
     return;

  /* 101% is a complete failure */
  percent = ((10 - (compute_armor_class(vict) / 10)) * 2) +
            number(1, skill_info[SKILL_KICK].max_percent);
  prob    = train_skill(ch, SKILL_KICK, skill_info[SKILL_KICK].max_percent, vict);
  if (GET_GOD_FLAG(vict,GF_GODSCURSE) || GET_MOB_HOLD(vict) > 0 || AFF_FLAGGED(vict, AFF_HOLDALL))
     prob = percent;
  if (GET_GOD_FLAG(ch,GF_GODSCURSE) || on_horse(vict))
     prob = 0;

  if (percent > prob)
  {
      damage(ch, vict, 0, SKILL_KICK + TYPE_HIT, TRUE);
      set_wait(ch, 1, TRUE);
  }
  else
  {
     int dam = str_app[GET_REAL_STR(ch)].todam + GET_REAL_DR(ch) +
                MAX(0,GET_SKILL(ch,SKILL_KICK)/10 - 5) + GET_LEVEL(ch) / 3 +
	   (GET_EQ(ch,WEAR_FEET) ? weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_FEET))].shocking : 0);
       if (AFF_FLAGGED(vict, AFF_SANCTUARY) && dam >= 2)
          dam /= 2;
      damage(ch, vict, dam, SKILL_KICK + TYPE_HIT, TRUE);
      set_wait(ch, 1, TRUE);
      set_wait(vict, 2, TRUE);
  }  
}

ACMD(do_kick)
{
  struct char_data *vict = NULL;
  
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK)) {
    send_to_char("�� � ������� �� ������ ��� ��� ������.\r\n", ch);
    return;
  }
 if (AFF_FLAGGED(ch, AFF_BLIND))
     {send_to_char("���� �� ������ �����?\r\n", ch);
      return;
     }
 one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("����� ����?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("��� � ���� ������� ����������.\r\n", ch);
    return;
  }
  
if (!may_kill_here(ch, vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

   if (!used_attack(ch))
      go_kick(ch, vict);


/*  if (IS_IMPL(ch) || !FIGHTING(ch))
     go_kick(ch, vict);
  else
  if (!used_attack(ch))
     {act("������. �� ����������� ����� $V.",FALSE,ch,0,vict,TO_CHAR);
     // SET_EXTRA(ch, SKILL_KICK, vict);
    ch->extra_attack.used_skill = SKILL_KICK; 
     ch->extra_attack.victim     = vict;	 
     }*/

}
/************** THROW PROCEDURES */
void go_throw(struct char_data * ch, struct char_data * vict)
{ int   percent, prob;
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);

  if (AFF_FLAGGED(ch, AFF_STOPFIGHT))
     {send_to_char("�� �������� �� � ��������� ���������.\r\n",ch);
      return;
     }

  if (!(wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON))
     {send_to_char("��� �� ������ �������?\r\n", ch);
      return;
     }

  if (!IS_IMMORTAL(ch) && !OBJ_FLAGGED(wielded, ITEM_THROWING))
     {act("$o �� ������������$y ��� �������.",FALSE,ch,wielded,0,TO_CHAR);
      return;
     }
  percent = number(1,skill_info[SKILL_THROW].max_percent);
  prob    = train_skill(ch, SKILL_THROW, skill_info[SKILL_THROW].max_percent, vict);
  if (IS_IMMORTAL(ch)                 ||
      GET_GOD_FLAG(vict,GF_GODSCURSE) ||
      GET_GOD_FLAG(ch,GF_GODSLIKE))
     prob = percent;
  if (IS_IMMORTAL(vict)               ||
      GET_GOD_FLAG(ch,GF_GODSCURSE)   ||
      GET_GOD_FLAG(vict, GF_GODSLIKE))
     prob = 0;


  if (percent > prob)
     damage(ch,vict,0,SKILL_THROW + TYPE_HIT, TRUE);
  else
     hit(ch,vict,SKILL_THROW,1);

  if (GET_EQ(ch,WEAR_WIELD))
     {wielded = unequip_char(ch,WEAR_WIELD);
      if (IN_ROOM(vict) != NOWHERE)
         obj_to_char(wielded,vict);
      else
         obj_to_room(wielded,IN_ROOM(ch));
	 obj_decay(wielded);
     }

  set_wait(ch, 3, TRUE);

}

ACMD(do_throw)
{ struct char_data *vict=NULL;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_THROW))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
     {if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
         {vict = FIGHTING(ch);
         }
     else
         {send_to_char("� ���� �� ������ �������?\r\n", ch);
          return;
         }
     };

  if (ch == vict)
     {send_to_char("����� �� ������� ������ ����?!\r\n",ch);
      return;
     }

  if (!may_kill_here(ch,vict))
     return;

  if (check_pkill(ch, vict, arg))
     return;

  if (IS_IMPL(ch) || !FIGHTING(ch)) {
     go_throw(ch, vict);

 } else {
     if (!used_attack(ch))
		{
		  act("�� ����������� ������� ������ � $V.",FALSE,ch,0,vict,TO_CHAR);
          SET_EXTRA(ch, SKILL_THROW, vict);
        }
 }
}

/******************** BLOCK PROCEDURES */
void go_block(struct char_data * ch)
{ if (AFF_FLAGGED(ch, AFF_STOPLEFT))
     {send_to_char("�� �� ������ ����� �������.\r\n", ch);
      return;
     }
  SET_AF_BATTLE(ch,EAF_BLOCK);
  send_to_char("�� ���������� ���������� ����� ��� ��������� �����.\r\n",ch);
}

ACMD(do_block)
{
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BLOCK))
     {send_to_char("�� �� �� ������ ��� ��� ������.\r\n", ch);
      return;
     }
  
  if (!(IS_NPC(ch)              ||            // ���
        GET_EQ(ch, WEAR_SHIELD) ||            // ���� ���
        IS_IMMORTAL(ch)         ||            // �����������
        GET_GOD_FLAG(ch,GF_GODSLIKE)          // ��������
       ))
      {send_to_char("��� ������ ����� ������������ ������ ��� ������� ����!\r\n", ch);
       return;
      };

  if (!FIGHTING(ch))
     {send_to_char("�� ���� �� ������ ���������� �����?\r\n", ch);
      return;
     };
  
  if (GET_AF_BATTLE(ch,EAF_BLOCK))
     {send_to_char("� �� ��� � ��� ������������� �����!\r\n", ch);
      return;
     }
  go_block(ch);
}

void affVictToch(struct char_data *ch, struct char_data *victim,
				 int type, int locat, int modif, 
				 int durat, bitvector_t bitvector, int battlef)
{
 struct affected_type af;
 durat = MAX(0,durat - 1);

 af.type      = type;
 af.location  = locat;
 af.modifier  = modif;
 af.duration  = pc_duration(victim,durat,0,0,0,0);
 af.bitvector = bitvector;
 af.battleflag= battlef;

     affect_join(victim, &af,FALSE, FALSE,FALSE, FALSE);
}



void do_tochn(struct char_data *ch, struct char_data *victim,
			  struct obj_data  *obj, int dam, int pozik)
{ int percent, skillnum =0,modi =0;
//����������� ��������� � ���� ��������!!!!!!!!!!!!!!!!
percent = GET_SKILL(ch, SKILL_TOCHNY)*2/7;//number(GET_SKILL(ch, SKILL_TOCHNY)/5,GET_SKILL(ch, SKILL_TOCHNY));//12


		if(GET_EQ(ch,WEAR_WIELD))
percent += (weapon_app[GET_OBJ_WEIGHT(GET_EQ(ch,WEAR_WIELD))].shocking+//3..8..18
			number(GET_SKILL(ch,GET_OBJ_SKILL(obj))/21,GET_SKILL(ch,GET_OBJ_SKILL(obj))/7)); //4-12

percent += str_app[GET_REAL_STR(ch)].tohit;//2..6..10
percent += GET_MORALE(ch);//4..8..17
percent += dex_app[GET_REAL_DEX(ch)].reaction - dex_app[GET_REAL_DEX(victim)].reaction;//3..8..21
percent += number(GET_REAL_CHA(ch)/4,GET_REAL_CHA(ch)*2);//(3-15)..(5-20)..(6-25)

percent += number(0,percent);//(25-50)..(50-100)..(100-200)
		if (obj)
skillnum = GET_OBJ_VAL(obj, 3);	
skillnum += TYPE_HIT;		
		switch(pozik)
		{ case 0: //����
			GET_CONPOS(ch) = WEAR_LEGS - 1;
		     if (percent <= 50)
			 {act("�� �������� ������ ������ $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �������$y ��� ������ ������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �������$y $V ������ ������.", FALSE, ch, 0, victim, TO_NOTVICT);		
				  dam = dam*3/2;
			 }
			else
			if (percent <= 70)
				{ affVictToch(ch, victim, SPELL_SLOW,0,0,2,0,0);
			      act("�� ��������� ���� $D, $E �������$Y ���������� � ����$Y �� �����!", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� ����, �� �������� ���������� � ����� �� �����!", FALSE, ch, 0, victim, TO_VICT);
			      act("$n ��������$y ���� $D, $E �������$Y ���������� � ����$Y �� �����!", FALSE, ch, 0, victim, TO_NOTVICT);
				  GET_POS(victim) = POS_SITTING;
				  WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
				  dam = dam*3/2;
				}
			 else
			 if (percent <= 90)
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-60,7,0,0);
			      act("�� ���������� ������ $D, $E �������$Y ���������� � ����$Y �� �����!", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ���������$y ��� ������, �� �������� ���������� � ����� �� �����!", FALSE, ch, 0, victim, TO_VICT);
			      act("$n ���������$y ������ $D, $E �������$Y ���������� � ����$Y �� �����!", FALSE, ch, 0, victim, TO_NOTVICT);
				  GET_POS(victim) = POS_SITTING;
				  WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
				  dam = dam*3/2;
				}
			 else 
			 if (percent <=150)
				{ affVictToch(ch, victim,SPELL_BATTLE,APPLY_HITREG,-90,15,0,0);
			      act("�� ������� ����� $D, $E �������$Y ���������� � ����$Y �� �����!", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������$y ��� �����, �� �������� ���������� � ����� �� �����!", FALSE, ch, 0, victim, TO_VICT);
			      act("$n ������$y ����� $D, $E �������$Y ���������� � ����$Y �� �����!", FALSE, ch, 0, victim, TO_NOTVICT);
				  GET_POS(victim) = POS_SITTING;
				  WAIT_STATE(victim, 4 * PULSE_VIOLENCE);
				  dam = dam*2;
				}
			 else
			 if (percent > 150)	//��������� ���� ��������� ����������� ���� �� �����.
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-150,30,0,0);
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_DEX,-number(1,3),30,0,0);
				  act("�� ������ ��������� ���� $D, $E �������$Y ���������� � ����$Y �� �����!", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������ ��������$y ��� ����, �� �������� ���������� � ����� �� �����!", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ������ ��������$y ���� $D, $E �������$Y ���������� � ����$Y �� �����!", FALSE, ch, 0, victim, TO_NOTVICT);
				  GET_POS(victim) = POS_SITTING;
				  WAIT_STATE(victim, 5 * PULSE_VIOLENCE);
				  dam = dam*2;
				}
				             
		   break;
		  case 1: //����
			GET_CONPOS(ch) = WEAR_HANDS - 1;
		    if (percent <= 60)
			{  act("�� �������� ������ ������ $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �������$y ��� ������ ������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �������$y $V ������ ������.", FALSE, ch, 0, victim, TO_NOTVICT);		
				  dam = dam*3/2;
			}
			else
			 if (percent <= 80)
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,3,AFF_STOPLEFT,1);
			      act("�� ��������� ����� ���� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� ����� ����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ����� ���� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*3/2;
				}
			 else
			 if (percent <= 100)
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,3,AFF_STOPRIGHT,1);
			      act("�� ��������� ������ ���� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� ������ ����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ������ ���� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*3/2;
				}
			 else 
			 if (percent <=160)
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,number(7,20),AFF_STOPLEFT,0);
			      act("�� ������� ����� ���� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������$y ��� ����� ����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ������$y ����� ���� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*2;
				}
             else
			 if (percent > 160) 
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,number(7,20),AFF_STOPRIGHT,0);
			      affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-30,20,0,0);
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_STR,-number(1,3),50,0,0);
			      act("�� ������� ������ ���� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������$y ��� ������ ����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ������$y ������ ���� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*2;
				}
			break; 
		  case 2://�����
			GET_CONPOS(ch) = WEAR_WAIST - 1;
		    if (percent <= 50)
			{	  act("�� �������� ������ ������ $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �������$y ��� ������ ������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �������$y $V ������ ������.", FALSE, ch, 0, victim, TO_NOTVICT);		
				  dam = dam*3/2;
			}
			else
			 if (percent <= 70)
				{ act("�� ����� ������� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ����$y ��� �������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ����$y ������� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
				  dam = dam*2;
				}
			 else
			 if (percent <= 120)
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-90,8,0,0);
				  act("�� ��������� ����� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� �����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ����� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*3;
				}
			 else 
			 if (percent <=150)
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,2,AFF_STOPFIGHT,1);
			      act("�� ������ �� ����� $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �����$y ��� �� �����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �����$y �� ����� $V.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*3;
				}
             else
			 if (percent > 150) 
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,3,AFF_STOPFIGHT,1);
			      affVictToch(ch, victim, SPELL_BATTLE,APPLY_CON,-number(1,2),50,0,0);
			      affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-30,20,0,0);
			      act("�� ��������� �������� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� ��������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y �������� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*3;
				}
			  break;
		  case 3://������
			GET_CONPOS(ch) = WEAR_HEAD - 1;
		   if (percent <= 80)
		   {		  act("�� �������� ������ ������ $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �������$y ��� ������ ������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �������$y $V ������ ������.", FALSE, ch, 0, victim, TO_NOTVICT);		
				  dam = dam*3/2;
		   }
			else
			 if (percent <= 100)
				{ act("�� �������� $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �������$y ���.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �������$y $V.", FALSE, ch, 0, victim, TO_NOTVICT);
				  WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
				  dam = dam*3;
				}
			 else
			 if (percent <= 130)
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-20,5,0,0);
				  act("�� ������� ����� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������$y ��� �����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ������$y ����� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
				  dam = dam*7/3;
				}
			 else 
			 if (percent <=170)
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-30,15,0,0);
				  act("�� ������� ������� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������$y ��� �������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ������$y ������� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  WAIT_STATE(victim, 4 * PULSE_VIOLENCE);
				  dam = dam*4;
				}
             else
			  if (percent > 170)
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_WIS,-number(1,3),50,0,0);
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_INT,-number(1,3),50,0,0);
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-50,20,0,0);
				  act("�� ��������� ����� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� �����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ����� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  WAIT_STATE(victim, 4 * PULSE_VIOLENCE);
				  dam = dam*5;
				}
		   break;
		  case 4://�����
        		GET_CONPOS(ch) = WEAR_NECK_2 - 1;
			 if (percent <= 40)
				{ dam = dam*3/2;
				  act("�� ����� ������� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ����$y ��� �������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ����$y ������� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
				 }
			 else
			 if (percent <= 70)
				{ dam = dam*2;
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-80,5,0,0);
				  act("�� ��������� ����� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� �����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ����� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				 }
			 else 
			 if (percent <=100)
				{ dam = dam*2;
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-150,15,0,0);
				  act("�� ������� ����� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������$y ��� �����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ������$y ����� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				}
             else
			  if (percent > 100)
				{ dam = dam*2;
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-200,30,0,0);
				  act("�� ���������� ������� ������ $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ���������$y ��� ������� ������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ���������$y ������� ������ $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
				}
		   break;
		  case 5://���
			GET_CONPOS(ch) = WEAR_NECK_1 - 1;
			 if (percent <= 100)
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-300,30,0,0);
				  act("�� ��������� ��� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� ���.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ��� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*4;
				}
			 else
			 if (percent <= 160)
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-500,40,0,0);
				  act("�� ��������� ������� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� �������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ������� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*4;
				}
			 else 
			 if (percent <=190)
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,3,AFF_STOPFIGHT,1);
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-700,50,0,0);
				  act("�� ������� ����� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������$y ��� �����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ������$y ����� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*5;
				}
             else
			   if (percent > 190)
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,5,AFF_STOPFIGHT,0);
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITREG,-900,60,0,0);
				  act("�� ������� ������ �������� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ������$y ��� ������ ��������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ������$y ������ �������� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*5;
				}
			break;
		  case 6://����
			GET_CONPOS(ch) = WEAR_EYES - 1;
		     if (percent <= 90)
			 {	  act("�� �������� ������ ������ $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �������$y ��� ������ ������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �������$y $V ������ ������.", FALSE, ch, 0, victim, TO_NOTVICT);		
				  dam = dam*3/2;
			 }
			else
			 if (percent <= 120)
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_HITROLL,-6,10,0,0);
				  affVictToch(ch, victim, SPELL_BLINDNESS,APPLY_AC,-100,10,0,0);
				  act("�� ��������� ���� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� ����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ���� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*3;
				}
			 else
			 if (percent <= 160)
				{ affVictToch(ch, victim, SPELL_BLINDNESS,0,0,15,AFF_BLIND,1);
				  act("�� �������� $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �������$y ���.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �������$y $V.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*5;
				}
			 else 
			 if (percent <= 200)
				{ affVictToch(ch, victim, SPELL_BATTLE,0,0,5,AFF_BLIND,0);
				  act("�� ������ ������ $V.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n �����$y ��� ������.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n �����$y ������ $V.", FALSE, ch, 0, victim, TO_NOTVICT);
				  dam = dam*6;
				}
             else
			 if (percent > 200)  
				{ affVictToch(ch, victim, SPELL_BATTLE,APPLY_WIS,-number(2,4),50,0,0);
				  affVictToch(ch, victim, SPELL_BATTLE,APPLY_INT,-number(2,4),50,0,0);
				  affVictToch(ch, victim, SPELL_BATTLE,0,0,5,AFF_HOLD,0);
				  affVictToch(ch, victim, SPELL_BLINDNESS,0,0,5,AFF_BLIND,0);
				  act("�� ��������� ���� $D.", FALSE, ch, 0, victim, TO_CHAR); 
				  act("$n ��������$y ��� ����.", FALSE, ch, 0, victim, TO_VICT);
				  act("$n ��������$y ���� $D.", FALSE, ch, 0, victim, TO_NOTVICT);
				  GET_POS(victim) = POS_SITTING;
				  dam = dam*6;
				}
		break;
		}
    damage(ch, victim, dam, skillnum, TRUE); 
   
}

// ����������� ! ������ ���� ������ ��������� �������� ��������� � ����������,
//�������� �� �������, ���� � ������� ���� ����� ������� ��� � ���������, 
//����� ��������� �������� � ������ ������. ����� ������
//�������� � ����� ����� �����, ���� ������ ���� �� ������ ����, �� � ����
//��������� ��� ������ ������, � �� ��� ��� ���������, ��� �� ��������
//�� ��� ���� ��������...
ACMD(do_tochny)
{  struct char_data *victim=NULL;
   struct obj_data *obj;
   int pozik =0, dam = 0, percent=0; 
  
   const char *get_pozicik[] = 
	{"����",
	 "����",
	 "�����",
	 "������",
	 "�����",
	 "���",
	 "����",
	 "\n"
	};
 
  
   if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TOCHNY))
     {send_to_char("�� �� ������ ���.\r\n", ch);
      return;
     }
   
    argument = one_argument(argument, arg);
              skip_spaces(&argument);
	if (!arg || !*arg)
	{ 	send_to_char("�� ������ �������� ����� ��:\r\n", ch);
		for (pozik = 0; pozik <= 6; pozik ++)
		{ sprintf(buf, "%s\r\n",get_pozicik[pozik]);
		  send_to_char(buf, ch);
		}
		return;
	}
   if ((pozik = search_block(arg, get_pozicik, FALSE)) < 0)
		{ send_to_char("� ����� ����� ���� ������� ����������!\r\n", ch);
	      return; 	
		}
 
  if (!(victim = get_char_vis(ch, argument, FIND_CHAR_ROOM)))
	{ if (!*argument && FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
          victim = FIGHTING(ch);
      else
		{ send_to_char("���� �� ������ �������?\r\n", ch);
		  return;
		}
	}
  
  if (victim == ch) {
    send_to_char("��� � ���� ����� ����������.\r\n", ch);
    return;
  }
  
if (!may_kill_here(ch, victim))
     return;

  if (check_pkill(ch, victim, argument))
     return;

if (used_attack(ch))
	return;

    improove_skill(ch, SKILL_TOCHNY, TRUE, victim);

	obj=GET_EQ(ch,WEAR_WIELD);

	
	
	if (!(general_savingthrow(victim, SAVING_REFLEX, GET_SKILL(ch, SKILL_TOCHNY)/2)))
	 { dam += (GET_REAL_DR(ch) + str_app[STRENGTH_APPLY_INDEX(ch)].todam);
	  if (obj && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
      	  { percent = dice(GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
            percent = MIN(percent, percent * GET_OBJ_CUR(obj) / MAX(1, GET_OBJ_MAX(obj)));
            dam += MAX(1,percent);
	    if (GET_SKILL(ch,GET_OBJ_SKILL(obj)) > 20)
             dam += ((GET_SKILL(ch,GET_OBJ_SKILL(obj)) - 20) / 5);
	  }	
	  else
	   { if (GET_SKILL(ch,SKILL_PUNCH) > 20)
                dam += ((GET_SKILL(ch,SKILL_PUNCH) - 20) / 5);
  	   }

	if(!WAITLESS(ch))
	  WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	 do_tochn(ch, victim, obj, dam, pozik);
      return;
	}
	send_to_char("��� ������ ���� �� ������.\r\n",ch);

   if(!WAITLESS(ch))
   WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
hit(ch, victim, TYPE_UNDEFINED, 1);	
}

