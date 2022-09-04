/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
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
#include "screen.h"
#include "dg_scripts.h"
#include "clan.h"
#include "support.h"

/* extern variables */
extern int    level_can_shout;
extern int    holler_move_cost;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct auction_lot_type auction_lots[];
extern struct index_data *mob_index;
extern struct zone_data *zone_table;
extern vector < ClanRec * > clan;


int num_of_clans;

/*extern function */
void imm_show_obj_values(struct obj_data *obj, struct char_data *ch);
void set_wait(struct char_data *ch, int waittime, int victim_in_room);
/* local functions */
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int is_tell_ok(struct char_data *ch, struct char_data *vict);
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_pray_gods);
ACMD(do_gen_comm);
ACMD(do_qcomm);
ACMD(do_remembertell);
ACMD(do_journal);

Journal personal;
Journal _public;
Journal offtopic;
Journal _clan;

void remember_ex(char_data *ch)
{
  bool order = (PRF_FLAGGED(ch, PRF_JOURNAL)) ? true : false;

  int i, j = 0, k = 0;
  for (i = 0; i < MAX_REMEMBER_TELLS; i++) 
  {
    int x = i;
    if (order)    
       x = MAX_REMEMBER_TELLS-(i+1);

    j = GET_LASTTELL(ch) + x;
    if (j >=  MAX_REMEMBER_TELLS)
       j = j - MAX_REMEMBER_TELLS;

    if (GET_TELL(ch, j)[0] != '\0') 
    {
      k = 1;
      send_to_char(GET_TELL(ch, j), ch);
      send_to_char("\r\n", ch);
    }
  }
  
  if (!k) {
      send_to_char("� ��� ����� ������ �� �������.\r\n", ch);
  }
}

ACMD(do_remembertell)
{
  if (IS_NPC(ch))
    return;
  remember_ex(ch);
}

ACMD(do_journal)
{
    strcpy(buf, argument);
    char *x = &buf[0];
    skip_spaces(&x);

    bool order = (PRF_FLAGGED(ch, PRF_JOURNAL)) ? true : false;

    if (cmpstr(x, "������"))         // personal
    {
        if (IS_IMMORTAL(ch))
            personal.sendToChar(ch, order);
        else
           remember_ex(ch);
        return;
    }

    if (cmpstr(x, "���������"))      // public
    {
        _public.sendToChar(ch, order);
        return;
    }

    if (cmpstr(x, "��������"))       // offtopic
    {
        offtopic.sendToChar(ch, order);
        return;
    }

    if (cmpstr(x, "����"))           // clan
    {
        if (IS_IMMORTAL(ch))
            _clan.sendToChar(ch, order);
        else
        {
            int d = -1;
       	    if (GET_CLAN(ch) == 0 || GET_CLAN_RANK(ch) == 0 || (d = find_clan_by_id(GET_CLAN(ch))) == -1)
	        {
               send_to_char ("�� �� ��������� ������ �����.\r\n", ch);
			   return;
            }
            clan[d]->journal.sendToChar(ch, order);
        }
        return;
    }

    send_to_char("����� ������ (������, ���������, ��������, ����) ��������?\r\n", ch);
}

ACMD(do_say)
{
  skip_spaces(&argument);

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }
  
  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
     {send_to_char("��� ��������� �������� � ������� ��������!\r\n", ch);
      return;
     }

  if (!*argument)
    send_to_char("��, �� ��� �� ������ �������?\r\n", ch);
  else {

    /*if (!IS_NPC(ch))
    {
      sprintf(buf, "&W%s ������%s&n: \"%s\"", GET_NAME(ch), GET_SEX(ch) == SEX_FEMALE ? "�" : "", argument);
      _public.addToJournal(buf);
    }*/

    sprintf(buf, "$n ������$y: \"%s\"",argument);
    act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG | CHECK_DEAF);

   if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char("OK\r\n", ch);
    else {
      delete_doubledollar(argument);
      sprintf(buf, "�� �������: \"%s\"\r\n", argument);
      send_to_char(buf, ch);
    }
      speech_mtrigger(ch, argument);
      speech_wtrigger(ch, argument);
  }
}


ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

    if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     } 
  
    skip_spaces(&argument);

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("�� �� �� ���� ������!\r\n", ch);
    return;
  }
  if (!*argument)
    send_to_char("��, �� ��� �� ������ ������� ������?\r\n", ch);
  else {
    if (ch->master)
      k = ch->master;
    else
      k = ch;

    sprintf(buf, "&C$n ������$y ������: \"%s\"&n\r\n",argument);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
	act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char("OK\r\n", ch);
    else {
      sprintf(buf, "&C�� ������� ������: \"%s\"&n\r\n", argument);
      send_to_char(buf, ch);
    }
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{	
  sprintf(buf, "&G$n ������$y ���: \"%s\"&n", arg);
  if (IS_IMMORTAL(ch))
    act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
  else
    act(buf, FALSE, ch, 0, vict, TO_VICT); 
   
  //���������.
  arg[MAX_RAW_INPUT_LENGTH - 35] = 0;
  if (!IS_NPC(vict) && !IS_NPC(ch)) 
  {
  
  strcpy(buf, CurrentTime());
  if (CAN_SEE(vict,ch))
     sprintf(buf + strlen(buf), ": &G%s c�����%s ���: \"%s\"&n", GET_NAME(ch), GET_SEX(ch)==SEX_FEMALE ? "�" : "", arg);
  else
     sprintf(buf + strlen(buf), ": &G���-�� ������ ���: \"%s\"&n", arg);
  
   strcpy(GET_TELL(vict, GET_LASTTELL(vict)), buf);
   GET_LASTTELL(vict)++;
   if (GET_LASTTELL(vict) == MAX_REMEMBER_TELLS)
        GET_LASTTELL(vict) = 0;

   strcpy(buf, CurrentTime());
   sprintf(buf + strlen(buf), ": &G�� ������� %s: \"%s\"&n", GET_DNAME(vict), arg);
   
   strcpy(GET_TELL(ch, GET_LASTTELL(ch)), buf);
   GET_LASTTELL(ch)++;
   if (GET_LASTTELL(ch) == MAX_REMEMBER_TELLS)
        GET_LASTTELL(ch) = 0;
  }
  
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
     send_to_char("OK\r\n", ch);
  else
   { sprintf(buf, "&G�� ������� $D: \"%s\"&n", arg);
     act(buf, FALSE, ch, 0, vict, TO_CHAR);/* | TO_SLEEP);*/
   }

  if (!IS_NPC(vict) && !IS_NPC(ch))
    GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
   { send_to_char("�� �������������� ���� � �����!\r\n", ch);
     return (FALSE);
   }
 else
  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
     {send_to_char("��� ��������� �������� � ������� ��������!\r\n", ch);
      return (FALSE);
     }
   else 
   if (!IS_NPC(vict) && !vict->desc)        
      { act("$E ��� ����� ������.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
        return (FALSE);
      }   
   else 
   if (PLR_FLAGGED(vict, PLR_WRITING))
      { act("$E ����� ��������� ������, ���������� �����.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
        return (FALSE);
      }  
   if (IS_GOD(ch))
       return (TRUE);
 
   if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
      send_to_char("�� �� ������ �������� ����-��, ���� � ��� ����� NOTELL!\r\n", ch);
  else 
   if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
      send_to_char("����� ��������� ���� �����.\r\n", ch); 
  else 
   if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF))
      act("$E �� ����� ��� �������.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
   if (GET_POS(vict) < POS_RESTING)
      send_to_char("�� ����������, � ������ ������ ��� �� �������.\r\n", ch);
   else
   if (GET_POS(ch) < POS_RESTING)
      send_to_char("�� �� ������ �������� � ����� ���������.\r\n", ch);
   else
    return (TRUE);

  return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict = NULL;

  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }
  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
     {send_to_char("��� ��������� �������� � ������� ��������!\r\n", ch);
      return;
     }
  
  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
  {
      send_to_char("��� � ���� �� ������ �������?\r\n", ch);
      return;
  }
  
  vict = get_player_vis(ch, buf, FIND_CHAR_WORLD);
  if (!vict)
  {
      send_to_char(NOPERSON, ch);
      return;
  }
  
  if (PRF_FLAGGED(vict, PRF_AFK) && !IS_IMMORTAL(ch)) {
    sprintf(buf, "%s ��������� � ������ ��� � �� ����� ������� ���� ���������.\r\n",
			GET_NAME(vict));
			send_to_char(buf, ch);
        sprintf(buf, "&c���������: &g%s&n\r\n", GET_AFK(vict));
        send_to_char(buf, ch);
 }

  else if (is_tell_ok(ch, vict))
  { if (PRF_FLAGGED(ch, PRF_NOTELL))
       send_to_char("��� �� ������ ��������!\r\n", ch);       
       if (!IS_NPC(ch) && !IS_NPC(vict))
       {
            sprintf(buf, "&G%s ������%s %s&n: \"%s\"", GET_NAME(ch), GET_SEX(ch) == SEX_FEMALE ? "�" : "", GET_DNAME(vict), buf2);
            personal.addToJournal(buf);
       }       
       perform_tell(ch, vict, buf2);
  }
 }


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;
  
  if (AFF_FLAGGED(ch, AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }

  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
     {send_to_char("��� ��������� �������� � ������� ��������!\r\n", ch);
      return;
     }
  
  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char("��� ������ ��������!\r\n", ch);
  else if (!*argument)
    send_to_char("��� �� ������ ��������?\r\n", ch);
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     */
				     
    /*
     * XXX: A descriptor list based search would be faster although
     *      we could not find link dead people.  Not that they can
     *      hear tells anyway. :) -gg 2/24/98
     */
    while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (tch == NULL)
      send_to_char("� ������ ������ ����� ��� � ����.\r\n", ch);
    else if (PRF_FLAGGED(tch, PRF_AFK) && !IS_IMMORTAL(ch)) {
    sprintf(buf, "%s ��������� � ������ ��� � �� ����� ������� ���� ���������.\r\n",
			GET_NAME(tch));
			send_to_char(buf, ch);
        sprintf(buf, "&c���������: &g%s&n\r\n", GET_AFK(tch));
        send_to_char(buf, ch);
 }

  else if (is_tell_ok(ch, tch))    
    {
        if (!IS_NPC(ch) && !IS_NPC(tch))
        {
            sprintf(buf, "&G%s ������%s %s&n: \"%s\"", GET_NAME(ch), GET_SEX(ch) == SEX_FEMALE ? "�" : "", GET_DNAME(tch), argument);
            personal.addToJournal(buf);
        }
        perform_tell(ch, tch, argument);
    }
  }
}


ACMD(do_spec_comm)
{
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others, *action_nast, *action_wy;

  if (AFF_FLAGGED(ch,AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }
  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
     {send_to_char("��� ��������� �������� � ������� ��������!\r\n", ch);
      return ;
     }

  switch (subcmd) {
  case SCMD_WHISPER:
    action_wy   = "�������";
    action_nast = "�������";
    action_sing = "������$y";
    action_plur = "������$y";
    action_others = "$n ���-�� ������$y $D.";
    break;

  case SCMD_ASK:
    action_wy   = "������ ������";
    action_nast = "��������";
    action_sing = "�����$y ������";
    action_plur = "�����$y ������";
    action_others = "$n �����$y $D ������.";
    break;

  default:
    action_sing = "���";
    action_plur = "����";
    action_others = "$n, ��� �� ������ �������� $D?";
    break;
  }

  half_chop(argument, buf, buf2);

  if (!*buf) {
    sprintf(buf, "��� �� ������ %s?", action_nast);
    act(buf, FALSE, ch, 0, 0, TO_CHAR);
  } else if (!*buf2) {
	  if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
    	  send_to_char(NOPERSON, ch);
			return;
	  }
   else
	   if (subcmd == SCMD_ASK)
		   sprintf(buf, "��� �� ������ %s � $R?", action_nast);
	     else sprintf(buf, "��� �� ������ %s $D?", action_nast);
	   act(buf, FALSE, ch, 0, vict, TO_CHAR);
  } else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
    send_to_char(NOPERSON, ch);
  else if (vict == ch)
    send_to_char("� ����� ��� ��� �����?\r\n", ch);
  else {
    sprintf(buf, "$n %s ���: \"%s\"", action_plur, buf2);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char("OK\r\n", ch);
    else {
      sprintf(buf, "�� %s %s: \"%s\"\r\n", action_wy, GET_DNAME(vict), buf2);
      send_to_char(buf, ch);
    }
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
  }
}



#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
  struct obj_data *paper, *pen = NULL;
  char *papername, *penname;

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* �� �� ��� ������ */
    send_to_char("�� ��� �� ������ ������?\r\n", ch);
    return;
  }
  if (*penname) {		/*��� ������� ��������� */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "� ����� ��������� ��� ��������, �� ������� ����� ���� �� ������� ������.\r\n");
      send_to_char(buf, ch);
      return;
    }					/*����� ������*/
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
      sprintf(buf, "��� ����� ������� �������.\r\n");
      send_to_char(buf, ch);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "� ����� ��������� ��� ��������, �� ������� ����� ���� �� ������� ������.\r\n");
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = NULL;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char("�� ���� �������� ���������� ������.\r\n", ch); 
      return;
    }
    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_HOLD)) {
      sprintf(buf, "�� �� ������ ������ �������.\r\n");
      send_to_char(buf, ch);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
      send_to_char("�� �� ������ ������ ���� ���������!\r\n", ch); 
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_HOLD);
    else
      pen = GET_EQ(ch, WEAR_HOLD);
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$o �� �������� ��� ������.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("�� �� ������ ������ �� $4.", FALSE, ch, paper, 0, TO_CHAR);
  else if (paper->action_description)
    send_to_char("����� ��� ������� ������.\r\n", ch); 
  else {
    /* we can write - hooray! */
    send_to_char("�������� ���� ������.  ������� '@' ��� �� ��������� ������.\r\n", ch);
    act("$n �����$y ������ ������.", TRUE, ch, 0, 0, TO_ROOM);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, NULL);
  }
}


ACMD(do_ctell)
{
  struct descriptor_data *i;
  int zone=0, c=0, d;
  arg[MAX_INPUT_LENGTH];
  skip_spaces (&argument);

    
	  if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
		{ for( d=0; d < num_of_clans; d++)
			{ if(isname(clan[d]->name, argument))
				{ c = clan[d]->id;
				  argument = one_argument(argument, arg);
				  skip_spaces (&argument);
				  d = 0;
              break;
				}
			}	
   			if (d)
			return;

	 while (
		 (*argument != ' ') && (*argument != '\0'))
      argument++;
    while (*argument == ' ') argument++;
		}
  else
    if(!IS_IMMORTAL(ch)			 && 
		((c = GET_CLAN(ch)) == 0 ||
		  GET_CLAN_RANK(ch) == 0))
	{ send_to_char ("�� �� ��������� ������ �����.\r\n", ch);
			return;
    }
  else 
	  c = GET_CLAN(ch);

  skip_spaces (&argument);

  if (!IS_NPC(ch) && (d=find_clan_by_id(GET_CLAN(ch))) == -1)
	  	{ send_to_char ("�� �� �� ���� ������!\r\n", ch);
			return;
		}

  if (!*argument)
  { send_to_char ("��� �� ������ ������� �����?\r\n", ch);
		return;
  }
 
  if (!IS_NPC(ch))
	{ 
    if (PRF_FLAGGED(ch,PRF_NOREPEAT))
		  strcpy (buf, OK);
	  else
          sprintf (buf, "&C[ %s ]: &W��&n: %s&n\r\n",
		  clan[d]->name, argument);
          send_to_char (buf, ch);
	}

  if (!IS_NPC(ch))
  {
     sprintf(buf, "&C[ %s ]: &W%s&n: \"%s\"", clan[d]->name, GET_NAME(ch), argument);
     _clan.addToJournal(buf);
     sprintf(buf, "&C%s&n: \"%s\"", GET_NAME(ch), argument);
     clan[d]->journal.addToJournal(buf);
  }

  for (i = descriptor_list; i; i=i->next)
  { if (i->character								&&
        GET_CLAN(i->character)== c					&&
	    (GET_CLAN_RANK(i->character)                ||
		IS_IMMORTAL(i->character))                  &&
        STATE(i) == CON_PLAYING                     &&
		i != ch->desc                               &&
		i->character != ch)
	{   sprintf (buf, "&C[ %s ]: &W%s&n: %s&n",IS_IMMORTAL(ch) ?
	    "�����������" :!IS_NPC(ch) ? clan[d]->name : "����_���",
        GET_NAME(ch), argument);
	    act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
    }
   }
  return;
}

ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char("Monsters can't page.. go away.\r\n", ch);
  else if (!*arg)
    send_to_char("���� �� ������� ������� ���������?\r\n", ch);
  else {
    sprintf(buf, "\007\007*$n* %s", buf2);
    if (!str_cmp(arg, "���")) {
      if (GET_LEVEL(ch) > LVL_GOD) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char("�� �� ������ ����� �������!\r\n", ch);
      return;
    }
    if ((vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char("OK\r\n", ch);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char("����� �������� � ���� �� ����������!\r\n", ch);
  }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

void showlots(struct char_data *ch)
{int i;
 send_to_char("&W���������� �� ��������:&n \r\n",ch);
for (i = 0; i < MAX_AUCTION_LOT; i++)
     {if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
         {sprintf(buf,"��� #%d: ��������.\r\n",i);
          send_to_char(buf,ch);
          continue;
         }
      if (GET_LOT(i)->prefect && GET_LOT(i)->prefect != ch)
         {sprintf(buf,"��� #%d: ������� %s%s%s (�� �����).\r\n",
                  i,
                  CCYEL(ch,C_NRM),GET_LOT(i)->item->short_description,CCNRM(ch,C_NRM));
          send_to_char(buf,ch);
          continue;
         }

      sprintf(buf,"��� �������� %d: %s%s%s%s %d %s, ������� %d, ������ &c%s&n",
              i, CCYEL(ch,C_NRM),GET_LOT(i)->item->short_description,
		  OBJ_FLAGGED(GET_LOT(i)->item, ITEM_SHARPEN) ? " (��������)":"",CCNRM(ch,C_NRM),
              GET_LOT(i)->cost, desc_count(GET_LOT(i)->cost, WHAT_MONEYa),
              GET_LOT(i)->tact < 0 ? 1 : GET_LOT(i)->tact + 1,
              PERS(GET_LOT(i)->seller, ch));
      if (GET_LOT(i)->prefect && GET_LOT(i)->prefect_unique == GET_UNIQUE(ch))
         {strcat(buf,", (��� ���).\r\n");
         }
      else
         {strcat(buf," (��� ����).\r\n");
         }
      send_to_char(buf,ch);
     }
}

int auction_drive(struct char_data *ch, char *argument)
{int    mode = -1, value = -1, lot = -1;
 struct char_data *tch = NULL;
 struct auction_lot_type *lotis;
 struct obj_data *obj;
 char   operation[MAX_INPUT_LENGTH], whom[MAX_INPUT_LENGTH];
 const char *auction_cmd[] =
{"���������", "set",
 "�����",     "close",
 "������",    "value",
 "�������",   "sell",
 "��������������", "info",
 "\n"
};


 if (!*argument)
    {showlots(ch);
     return (FALSE);
    }
 argument = one_argument(argument, operation);
 if ((mode = search_block(operation, auction_cmd, FALSE)) < 0)
    {send_to_char("������� ��������: ���������, �����, ������, �������, ��������������.\r\n",ch);
     return (FALSE);
    }
 mode >>= 1;
 switch(mode)
 {case 0: // Set lot
          if (!(lotis = free_auction(&lot)))
             {send_to_char("��� ��������� �����.\r\n",ch);
              return (FALSE);
             }
           *operation = '\0';
           *whom      = '\0';
           if (!sscanf(argument,"%s %d %s",operation,&value, whom))
              {send_to_char("������: ������� ��������� ���� [���. ������] [��� ����]\r\n",ch);
               return (FALSE);
              }
           if (!*operation)
             {send_to_char("�� ������ �������, ����� ������� ������ ��������� �� �������!\r\n",ch);
              return (FALSE);
             }
           if (!(obj = get_obj_in_list_vis(ch, operation, ch->carrying)))
              {send_to_char("� ��� ����� ���.\r\n", ch);
               return (FALSE);
              }
           if (OBJ_FLAGGED(obj,ITEM_DECAY)   ||
/*               OBJ_FLAGGED(obj,ITEM_NORENT)  ||*/
	       OBJ_FLAGGED(obj,ITEM_NODROP)  ||
	       OBJ_FLAGGED(obj,ITEM_NOSELL)  ||
               obj->obj_flags.cost      <= 0 ||
               obj->obj_flags.Obj_owner  > 0)
              {send_to_char("���� ������� �� ������������ ��� ��������.\r\n",ch);
               return(FALSE);
              }
           if (obj_on_auction(obj))
              {send_to_char("�� ��� ��������� �� ������� ���� �������.\r\n", ch);
               return (FALSE);
              }
           if (obj->contains)
              {sprintf(buf,"���������� %s ����� ��������.\r\n",obj->short_vdescription);
               send_to_char(buf,ch);
               return (FALSE);
              }
           if (value <= 0)
              {value = MAX(1, GET_OBJ_COST(obj));
              };
           if (*whom)
              {if (!(tch = get_char_vis(ch, whom, FIND_CHAR_WORLD)))
                  {send_to_char("�� �� ������ ����� ������.\r\n", ch);
                   return (FALSE);
                  }
               if (IS_NPC(tch))
                  {send_to_char("� ���� ��������� ����������� ����.\r\n", ch);
                   return (FALSE);
                  }
               if (ch == tch)
                  {send_to_char("�� ��� �� ��!\r\n",ch);
                   return (FALSE);
                  }
              };
           lotis->item_id = GET_ID(obj);
           lotis->item    = obj;
           lotis->cost    = value;
           lotis->tact    = -1;
           lotis->seller_unique = GET_UNIQUE(ch);
           lotis->seller        = ch;
           lotis->buyer_unique  = lotis->prefect_unique = -1;
           lotis->buyer         = lotis->prefect = NULL;
           if (tch)
              {lotis->prefect_unique = GET_UNIQUE(tch);
               lotis->prefect        = tch;
              }
           lotis->cost = value;
           if (tch)
              {sprintf(buf,"�� ��������� �� ������� $8 �� %d %s (��� %s)",
                      value,desc_count(value,WHAT_MONEYu),GET_RNAME(tch));
              }
           else
              {sprintf(buf,"�� ��������� �� ������� $8 �� %d %s",
                       value,desc_count(value,WHAT_MONEYu));
              }
           act(buf,FALSE,ch,0,obj,TO_CHAR);
           sprintf(buf,"&Y�������: ����� ��� #%d '&C%s&Y', ��������� ������ %d %s.&n", lot,
                   obj->short_description, value, desc_count(value,WHAT_MONEYa));
           act(buf,FALSE,ch,0,obj,TO_CHAR);
		   return (TRUE);
           break;
  case 1: // Close
           if (!sscanf(argument,"%d",&lot))
              {send_to_char("�� ������ ����� ����.\r\n",ch);
               return (FALSE);
              }
           if (lot < 0 || lot >= MAX_AUCTION_LOT)
              {send_to_char("�������� ����� ����.\r\n",ch);
               return (FALSE);
              }
           if (GET_LOT(lot)->seller != ch ||
               GET_LOT(lot)->seller_unique != GET_UNIQUE(ch))
              {send_to_char("��� �� ��� ���.\r\n",ch);
               return (FALSE);
              }
           act("�� ����� '$8' � ��������.",FALSE,ch,0,GET_LOT(lot)->item,TO_CHAR);
           sprintf(buf,"&y�������: ��� #%d '%s' ����%s � �������� ����������.&n", lot,
                   GET_LOT(lot)->item->short_description,
                   GET_OBJ_SUF_6(GET_LOT(lot)->item));
           clear_auction(lot);
           return (TRUE);
           break;
  case 2:  // Set
           if (sscanf(argument,"%d %d",&lot,&value) != 2)
              {send_to_char("������: ������� ������ ��� �����.����\r\n",ch);
               return (FALSE);
              }
           if (lot < 0 || lot >= MAX_AUCTION_LOT)
              {send_to_char("�������� ����� ����.\r\n",ch);
               return (FALSE);
              }
           if (!GET_LOT(lot)->item   || GET_LOT(lot)->item_id <= 0 ||
               !GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0)
              {send_to_char("��������� ���.\r\n",ch);
               return (FALSE);
              }
           if (GET_LOT(lot)->seller == ch ||
               GET_LOT(lot)->seller_unique == GET_UNIQUE(ch))
              {send_to_char("�� ��� �� ��� ���!\r\n",ch);
               return (FALSE);
              }
            if (GET_LOT(lot)->prefect && GET_LOT(lot)->prefect_unique > 0 &&
                (GET_LOT(lot)->prefect != ch || GET_LOT(lot)->prefect_unique != GET_UNIQUE(ch)))
               {send_to_char("���� ��� ����� ������� ����������.\r\n",ch);
                return (FALSE);
               }
            if (GET_LOT(lot)->item->carried_by != GET_LOT(lot)->seller)
               {send_to_char("���� ������� ����������.\r\n",ch);
                sprintf(buf, "�������: ��� #%d '%s' ����, ����� ����� ���������.",
                        lot, GET_LOT(lot)->item->short_description);
                clear_auction(lot);
                return (TRUE);
               }
            if (value < GET_LOT(lot)->cost)
               {send_to_char("���� ������ ���� �������.\r\n",ch);
                return (FALSE);
               }
            if (GET_LOT(lot)->buyer &&
                GET_UNIQUE(ch) != GET_LOT(lot)->buyer_unique &&
                value < GET_LOT(lot)->cost + MAX(1,GET_LOT(lot)->cost / 20))
               {send_to_char("����������� ������ ������ ���� �� 5% ���� �������.\r\n",ch);
                return (FALSE);
               }
            if (value > GET_GOLD(ch) + GET_BANK_GOLD(ch))
               {send_to_char("� ��� ��� ����� �����.\r\n",ch);
                return (FALSE);
               }
            GET_LOT(lot)->cost         = value;
            GET_LOT(lot)->tact         = -1;
            GET_LOT(lot)->buyer        = ch;
            GET_LOT(lot)->buyer_unique = GET_UNIQUE(ch);
            sprintf(buf,"������, �� �������� ��������� %d %s �� %s (��� %d).\r\n",
                    value, desc_count(value,WHAT_MONEYu),
                    GET_LOT(lot)->item->short_vdescription, lot);
            send_to_char(buf,ch);
          /*  sprintf(buf,"����� ������ %s �� ��� %d(%s) %d %s.\r\n",
                    GET_RNAME(ch), lot, GET_LOT(lot)->item->short_description,
                    value, desc_count(value, WHAT_MONEYa));
            send_to_char(buf,GET_LOT(lot)->seller);*/
            sprintf(buf, "&Y�������: ��� #%d '%s' - ����� ������ %d %s.&n",lot,
                    GET_LOT(lot)->item->short_description, value, desc_count(value,WHAT_MONEYa));
            return (TRUE);
            break;

  case 3:  // Sell
           if (!sscanf(argument,"%d",&lot))
              {send_to_char("�� ������ ����� ����.\r\n",ch);
               return (FALSE);
              }
           if (lot < 0 || lot >= MAX_AUCTION_LOT)
              {send_to_char("�������� ����� ����.\r\n",ch);
               return (FALSE);
              }
           if (GET_LOT(lot)->seller != ch ||
               GET_LOT(lot)->seller_unique != GET_UNIQUE(ch))
              {send_to_char("��� �� ��� ���.\r\n",ch);
               return (FALSE);
              }
           if (!GET_LOT(lot)->buyer)
              {send_to_char("���������� �� ��� ����� ���� ���.\r\n",ch);
               return (FALSE);
              }

           GET_LOT(lot)->prefect        = GET_LOT(lot)->buyer;
           GET_LOT(lot)->prefect_unique = GET_LOT(lot)->buyer_unique;
           if (GET_LOT(lot)->tact < MAX_AUCTION_TACT_BUY)
              {sprintf(whom,"�������: ��� #%d '%s' ������ � �������� �� %d %s.",lot,GET_LOT(lot)->item->short_description,
                       GET_LOT(lot)->cost, desc_count(GET_LOT(lot)->cost,WHAT_MONEYu));
               GET_LOT(lot)->tact = MAX_AUCTION_TACT_BUY;
              }
           else
              *whom = '\0';
           sell_auction(lot);
           if (*whom)
              {strcpy (buf, whom);
               return (TRUE);
              }
           return (FALSE);
           break;

  case 4: //��������������
           if (!sscanf(argument,"%d",&value))
              { send_to_char("������: ������� �������������� �����_���� \r\n",ch);
                return (FALSE);
              }
	  if (value >= MAX_AUCTION_LOT)
              { send_to_char("���� � ����� ������� �� ����������!\r\n",ch);
                return (FALSE);
              }
	  if (value < 0)
             {send_to_char("������� ����� ��������.\r\n",ch);
              return (FALSE);
             }		
	  if (!GET_LOT(value)->item)
	      { send_to_char("�� ���� ���� ��� ��������!\r\n", ch);
		return (FALSE);
	      }
		   //��������� �������������� 100 �����
	  if (GET_BANK_GOLD(ch) >= 100)
	     { GET_BANK_GOLD(ch) -= 100;
	       send_to_char("� ������ ����������� ����� ����� 100 ����� �� ������ ��������.\r\n", ch);	
	     }
	  else 
	  if (GET_GOLD(ch) >= 100)
	     { GET_GOLD(ch) -= 100; 
	       send_to_char("�� ��� ������ ��������, �� ��������� 100 �����.\r\n", ch);	
	     }
	  else 
	    { send_to_char("��� ������ ����� 100 ����� � � ��� ��� ����� �����!\r\n", ch);
	      return(FALSE);		 
	    }		 
		
	  if (GET_LOT(value)->item)
	     { imm_show_obj_values(GET_LOT(value)->item, ch);
	       return (FALSE);
	     }
	break; 
}
 return (FALSE);
}

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
/*  char color_on[24];*/

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    PRF_DEAF, //�����
    PRF_NOSHOUT,//�������
    PRF_NOGOSS, //�������
    PRF_NOAUCT, //�������
    PRF_NOGRATZ,//��������
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][7] = {
    {"�� �� ������ �����!\r\n",
      "������$y", 
      "��� �� �����, �������� ����� �����",
    "�������",
	"\\c10",
    "������",
    "�������",
    },

    {"�� �� ������ �������!!\r\n",
      "�������$y",
      "��� �� ������� �������� ����� �������!\r\n",
    "��������",
	"&y",
    "�������",
    "��������"
    },

    {"�� �� ������ �������!!\r\n",
      "������$y ����",
      "�� ���� �� �� ������!!\r\n",
    "������� ����",
	"&K",
    "������ ����",
    "������� ����"
    },

    {"�� �� ������ ��������� � ��������!!\r\n",
      "�������",
      "�� �� �� ������ ���������� �������!\r\n",
    "",
	"\\c06",
    "", ""
    },

    {"�� �� �� ������ ���������!\r\n",
      "�������$y ����", //��������
      "�� ���� �� �� ������!\r\n",
    "[��������]",
	"&Y",
    "[��������]",
    "[��������]"
    }
  };

  // to keep pets, etc from being ordered to shout 
   if (AFF_FLAGGED(ch,AFF_SIELENCE))
     {send_to_char(SIELENCE, ch);
      return;
     }
  
  if (!ch->desc)
    return;

  if (PLR_FLAGGED(ch, PLR_DUMB))
    { send_to_char("��� ��������� �������� � ������� ��������!\r\n", ch);
      return;
    }

  if (PLR_FLAGGED(ch, PLR_MUTE)) 
    { send_to_char(com_msgs[subcmd][0], ch);
      return;
    }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  { send_to_char("����� ��������� ���� �����.\r\n", ch); 
       return;
  }
 
  if (subcmd == SCMD_AUCTION)
		{	*buf = '\0';
		//if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_AUCTION)/* ||
		//	ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))*/ && !IS_IMMORTAL(ch))
		//{   send_to_char("�� �� ������ ����� ��������������� �������� ��������.\r\n", ch);
		//		return;
		//}
		if (!auction_drive(ch,argument))
				return;
 		}
 
  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < level_can_shout && subcmd != SCMD_AUCTION) {
    sprintf(buf1, "�� ������ ���� %d ������, ������ ��� ������� ������� ����.\r\n",
	    level_can_shout);
    send_to_char(buf1, ch);
    return;
  }
 
  /* make sure the char is on the channel */ /* �� ���� �� �� ������ */
  if (PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(com_msgs[subcmd][2], ch);
    return;
  }
  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument) {
    sprintf(buf1, "��, %s, ���������, �� ���?",
	    com_msgs[subcmd][1]);
    act(buf1, FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_HOLLER) {
    if (GET_MOVE(ch) < holler_move_cost) {
      send_to_char("� ��� ��� ��� �����.\r\n", ch);
      return;
    } else if (!IS_IMMORTAL(ch))
      GET_MOVE(ch) -= holler_move_cost;
  }
  
   if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char("OK\r\n", ch);
  else if (subcmd != SCMD_AUCTION)
   {  if (subcmd == SCMD_GRATZ)
	  { sprintf(buf1, "%s%s: &W��&n: \"%s\"\\c00",com_msgs[subcmd][4], com_msgs[subcmd][3], argument);
		act(buf1, FALSE, ch, 0, 0, TO_CHAR);
        sprintf(buf, "&Y%s&n: \"%s\"\\c00", GET_NAME(ch), argument);
        offtopic.addToJournal(buf);
        sprintf(buf, "%s[��������]: &W$c&n: \"%s\"\\c00",com_msgs[subcmd][4], argument);
        
	  }
	else
	{   sprintf(buf1, "%s�� %s: \"%s\"\\c00",com_msgs[subcmd][4], com_msgs[subcmd][3], argument);
		act(buf1, FALSE, ch, 0, 0, TO_CHAR);
        if (!IS_NPC(ch))
        {
            int index = GET_SEX(ch) == SEX_FEMALE ? 6 : 5;
            sprintf(buf, "&n%s %s: \"%s\"\\c00", GET_NAME(ch), com_msgs[subcmd][index], argument);
            if (subcmd == SCMD_SHOUT)
                buf[1] = 'y';
            else if (subcmd == SCMD_HOLLER)
                buf[1] = 'Y';
            _public.addToJournal(buf);
        }
        sprintf(buf, "%s$n %s: \"%s\"\\c00",com_msgs[subcmd][4], com_msgs[subcmd][1], argument);        
	 }
  }

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
	!PRF_FLAGGED(i->character, channels[subcmd]) &&
	!PLR_FLAGGED(i->character, PLR_WRITING) &&
	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
	  ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
	   !AWAKE(i->character)))
	continue;
      act(buf, FALSE, ch, 0, i->character, TO_VICT);/* | TO_SLEEP);*/
     }
  }
}

ACMD(do_pray_gods)
{
  char arg1[MAX_INPUT_LENGTH];
  struct descriptor_data *i;
  struct char_data *victim = NULL;
  
  skip_spaces(&argument);

  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB)) {
     send_to_char("��� ��������� ������� � �����.\r\n", ch);
     return;
  }
     
  if (IS_IMMORTAL(ch)) {
    /* �������� ���� ���� �������� ���� */
    argument = one_argument(argument, arg1);
    skip_spaces(&argument);
    if (!*arg1) {
      send_to_char("���� �� ����������� �������� �� ���������?\r\n", ch);
      return;
    }
    victim = get_player_vis(ch, arg1, FIND_CHAR_WORLD);
    if (victim == NULL) {
      send_to_char("������ ��������� ��� � ����.\r\n", ch);
      return;
    }
  }
  
  if (!*argument)
  {
    sprintf(buf, "� ����� �������� �� ������ �������� � �����?\r\n");
    send_to_char(buf, ch);
    return;
  }
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else 
  {
    if (IS_IMMORTAL(ch)) 
    {
      sprintf(buf, "&m�� �������� ���� ���� �� ��������� %s:&n \"%s\"&n", GET_RNAME(victim), argument);
	} 
    else 
    {	 
      sprintf(buf, "&m�� �������� � �����:&n \"%s\"&n", argument);
	  set_wait(ch, 3, FALSE);
	}
    act(buf, FALSE, ch, 0, argument, TO_CHAR | TO_SLEEP);	 
  }  

  if (IS_IMMORTAL(ch)) {
       sprintf(buf, "&m���� �������� ���:&n \"%s\"&n\r\n", argument);
       send_to_char(buf, victim);
       sprintf(buf, "&m$n �������$y �� ��������� %s: \"%s\"&n", 
                     GET_RNAME(victim), argument);
  } else {	 
       sprintf(buf, "&m$n �������$y � ����� � ����������:&n \"%s\"&n",
       argument);
  }
  for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && (IS_IMMORTAL(i->character) 
                                      ))
        if (i->character != ch)
          act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
}



ACMD(do_qcomm)
{
  struct descriptor_data *i;

  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char("� ��� ��� �������!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "%s?  ��, ���������, %s, �� ���?\r\n", CMD_NAME,
	    CMD_NAME);
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char("OK\r\n", ch);
    else {
      if (subcmd == SCMD_QSAY)
	sprintf(buf, "�� ��������: \"%s\"", argument);
      else
	strcpy(buf, argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    if (subcmd == SCMD_QSAY)
      sprintf(buf, "$n �������$y: \"%s\"", argument);
    else
      strcpy(buf, argument);

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i != ch->desc &&
	  PRF_FLAGGED(i->character, PRF_QUEST))
	act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}

