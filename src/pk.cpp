/* ************************************************************************
*   File: pk.c                                          Part of CircleMUD *
*  Usage: �� �������                                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991, 2004          *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "constants.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "clan.h"
#include "pk.h"

extern vector < ClanRec * > clan;

struct pkiller_file_u
{int  unique;
 int  pkills;
};
FILE *pkiller_fl = NULL;
ACMD(do_revenge);

extern struct char_data *character_list;
extern struct room_data *world;

#define FirstPK  1
#define SecondPK 5
#define ThirdPK	 10
#define FourthPK 20
#define FifthPK  30
#define KillerPK 50

// ��������� ��������� � ������� ��������� �������
#define KILLER_UNRENTABLE  30
#define REVENGE_UNRENTABLE 10
#define CLAN_REVENGE       10
#define THIEF_UNRENTABLE   30
#define BATTLE_DURATION    3
#define SPAM_PK_TIME       30

// ��������� ��������� � �������� ��������� �������
#define TIME_PK_GROUP      5

// ��������� ��������� � ����� ��������� �������
#define TIME_GODS_CURSE    192

#define MAX_PKILL_FOR_PERIOD 3

#define MAX_PKILLER_MEM  100
#define MAX_REVENGE      3


int pk_count(struct char_data *ch) {
 struct PK_Memory_type *pk; 
 int i;
 for (i = 0, pk = ch->pk_list; pk; pk = pk->next) i += pk->kill_num;
 return i;
}

int pk_calc_spamm( struct char_data *ch )
{
  struct PK_Memory_type *pk, *pkg;
  int count = 0, grou_fl;

  for ( pk = ch->pk_list; pk; pk = pk->next ) 
  {
    if ( time(NULL) - pk->kill_at <= SPAM_PK_TIME*60 ) 
    {
      grou_fl = 1;
      for (pkg = pk->next; pkg; pkg = pkg->next) 
	    if ( MAX(pk->kill_at, pkg->kill_at) - 
	         MIN(pk->kill_at, pkg->kill_at) <= TIME_PK_GROUP
		   ) 
		{
          grou_fl = 0;
          break;
        }
      if (grou_fl) 
	    count++;
    }
  }
  return (count);
}

void pk_check_spamm( struct char_data * ch )
{
  if ( pk_calc_spamm(ch) > MAX_PKILL_FOR_PERIOD )
  {
    SET_GOD_FLAG(ch,GF_GODSCURSE);
    GODS_DURATION(ch) = time(0)+TIME_GODS_CURSE*60*60;
    act("�� ���� ������, �� ������ �������!", FALSE, ch, 0, 0, TO_CHAR);
  }
  if ( pk_count(ch) >= KillerPK ) 
  SET_BIT(PLR_FLAGS(ch, PLR_KILLER), PLR_KILLER);
 }

// ������� ��������� ���������� *pkiller � *pvictim �� ������, ���� ��� �����
void pk_translate_pair( 
  struct char_data * * pkiller,
  struct char_data * * pvictim
)
{
 /* if ( pkiller != NULL && pkiller[0] != NULL)
    if ( IS_NPC(pkiller[0])                 && 
         AFF_FLAGGED(pkiller[0],AFF_CHARM) 
       )
      pkiller[0] = pkiller[0]->master;

  if ( pvictim != NULL && pvictim[0] != NULL)
  {
    if ( IS_NPC(pvictim[0])                                      && 
         (AFF_FLAGGED(pvictim[0],AFF_CHARM)||IS_HORSE(pvictim[0])) &&
         IN_ROOM(pvictim[0]) == IN_ROOM(pvictim[0]->master)
       )
      pvictim[0] = pvictim[0]->master;
    if ( !HERE(pvictim[0]) ) pvictim[0] = NULL;
  }*/

 if (pkiller != NULL && pkiller[0] != NULL)
    if (IS_NPC (pkiller[0]) && pkiller[0]->master &&
	AFF_FLAGGED (pkiller[0], AFF_CHARM) &&
	IN_ROOM (pkiller[0]) == IN_ROOM (pkiller[0]->master))
      pkiller[0] = pkiller[0]->master;

  if (pvictim != NULL && pvictim[0] != NULL)
    {
      if (IS_NPC (pvictim[0]) && pvictim[0]->master &&
	  (AFF_FLAGGED (pvictim[0], AFF_CHARM) || IS_HORSE (pvictim[0])))
	if  (IN_ROOM (pvictim[0]) == IN_ROOM (pvictim[0]->master))
 	pvictim[0] = pvictim[0]->master;
      if (!HERE (pvictim[0]))
	pvictim[0] = NULL;
    }

}

// agressor �������� �������������� �������� ������ victim
// ������/�������� ����-����
void pk_update_clanflag( struct char_data * agressor, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  for ( pk = agressor->pk_list; pk; pk = pk->next ) 
  {
    // ��� ������ ��� ���� � ������ ������ ?
    if ( pk->unique == GET_UNIQUE(victim) ) break;
  }
  if ( !pk )
  {
    CREATE( pk, struct PK_Memory_type, 1 );
    pk->unique        = GET_UNIQUE(victim);
    pk->next          = agressor->pk_list;
    agressor->pk_list = pk;
  }
  if ( victim->desc )
  {
    if ( pk->clan_exp > time(NULL) )
    {
      act("�� �������� ����� �������� ����� $D!", FALSE, victim, 0, agressor, TO_CHAR);
      act("$N �������$Y ����� ��� ��� ��������� ���!", FALSE, agressor, 0, victim, TO_CHAR);
    }
    else
    {
      act("�� �������� ����� �������� ����� $D!", FALSE, victim, 0, agressor, TO_CHAR);
      act("$N �������$Y ����� �������� ����� �� ���!", FALSE, agressor, 0, victim, TO_CHAR);
    }
  }
  pk->clan_exp = time(NULL) + CLAN_REVENGE*60;

  save_char(agressor, NOWHERE);

  return;
}

// victim ���� agressor (��� � ������) 
// ����� ����-���� � agressor
void pk_clear_clanflag( struct char_data * agressor, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  for ( pk = agressor->pk_list; pk; pk = pk->next ) 
  {
    // ��� ������ ��� ���� � ������ ������ ?
    if ( pk->unique == GET_UNIQUE(victim) ) break;
  }
  if ( !pk ) return; // � �����-�� � �� ���� :(

  if ( pk->clan_exp>time(NULL) )
  {
    act("�� ������������ ����� �������� ����� $D.", FALSE, victim, 0, agressor, TO_CHAR);
  }
  pk->clan_exp = 0;
 
  return;
}

// ������������ ����� �������� � ��
void pk_update_revenge( 
  struct char_data * agressor, 
  struct char_data * victim, 
  int attime, int renttime 
)
{
  struct PK_Memory_type *pk;

  for ( pk = agressor->pk_list; pk; pk = pk->next ) 
    if ( pk->unique == GET_UNIQUE(victim) ) break;
  if ( !pk && !attime && !renttime ) 
    return;
  if ( !pk ) 
  {
    CREATE( pk, struct PK_Memory_type, 1 );
    pk->unique        = GET_UNIQUE(victim);
    pk->next          = agressor->pk_list;
    agressor->pk_list = pk;
  }
  pk->battle_exp     = time(NULL)+attime*60;
  if ( !same_group( agressor, victim ) )
    RENTABLE(agressor) = MAX(RENTABLE(agressor),time(NULL)+renttime*60);
  return;
}

//��������� ���� ��������� ����� ��� ������� ��� � ����� -+3 ������
void inc_pk_temp(struct char_data *agressor, struct char_data *victim)
{struct PK_Memory_type *pk;
 int    count=0;


 for ( pk = agressor->pk_list; pk; pk = pk->next ) 
    {
         if (pk->unique == GET_UNIQUE(victim))
			 break;
    }
    if ( !pk )
    { 
      CREATE( pk, struct PK_Memory_type, 1 );
      pk->unique        = GET_UNIQUE(victim);
      pk->next          = agressor->pk_list;
      agressor->pk_list = pk;
    }
 
	if (pk->temp_at > 0)
	{act("$N �������$Y ����� ��������� ����� ��� ���!", FALSE, agressor, 0, victim, TO_CHAR);
     act("�� �������� ����� ��������� ����� $D!", FALSE, victim, 0, agressor, TO_CHAR);
	}	
	else
	{ act("$N �������$Y ����� ��������� ����� �� ���!", FALSE, agressor, 0, victim, TO_CHAR);
	  act("�� �������� ����� ��������� ����� �� $V!", FALSE, victim, 0, agressor, TO_CHAR);
	}

	++pk->temp_at;//���������� ��������� �� ����� ��������� �����.
    pk->temp_time = time(NULL);
	pk->battle_exp     = time(NULL)+BATTLE_DURATION*60; //��������� 3 ������ ����_����
    RENTABLE(agressor)    = MAX(RENTABLE(agressor), time(NULL) + MAX_PTHIEF_LAG);
    sprintf(buf,"��������� ����� ��������� ����� : %s->%s",GET_NAME(agressor),GET_NAME(victim));
    mudlog(buf, CMP, LVL_GOD, FALSE);

    pk_check_spamm( agressor );
  save_char(agressor, NOWHERE);
}


// agressor �������� �������������� �������� ������ victim
// 1. ������ ����
// 2. ������ ��������
// 3. ���� �����, ������ ��
// 4. ��������� � ����� ���� � ���� ������������ ��������� �����, ������ ��������� �����
//    ���� �� � ����� ���� � � ������, ������ ��������� ����� �� ���� ���������� ��������.
//    ���� � ����� ����, ���� ������ � ����������� �� ������� �������, ���� ������ �����, ���� ���������.
void pk_increment_kill( struct char_data * agressor, struct char_data * victim, int rent )
{ int a, clan_num;
  struct PK_Memory_type *pk;
  struct Pk_Date *Pk;


 	if ((clan_num = find_clan_by_id(GET_CLAN(agressor))) >= 0)
		{ for (Pk = clan[clan_num]->CharPk; Pk; Pk = Pk->next)
		      if (Pk->PkUid == GET_UNIQUE(victim))
	                { RENTABLE(agressor)     = MAX(RENTABLE(agressor), time(NULL) + KILLER_UNRENTABLE * 60);
		          return;
			}	  
	        }

 if (GET_LEVEL(agressor) > GET_LEVEL(victim))
    a = GET_LEVEL(agressor) - GET_LEVEL(victim);
 else
    a = GET_LEVEL(victim)-GET_LEVEL(agressor);


 if (a <= 3) 
 {  inc_pk_temp(agressor, victim);//��������� ��� ������� ���� ��������� �����.
   return ;
 }

 //����������� � ���������� ����_�� ��������� ����� �� ����� �� ������, ������ �� ����������.
 if ( GET_CLAN_RANK(agressor) && GET_CLAN_RANK(victim) && (GET_CLAN(agressor) !=  GET_CLAN(victim)) )
  {
    // ����������� �������� 
    pk_update_clanflag(agressor,victim);
  }
  else
  {
    // �� ��� ��������� ���������
    for ( pk = agressor->pk_list; pk; pk = pk->next ) 
    {
      // ��� ������ ��� ���� � ������ ������ ?
      if ( pk->unique == GET_UNIQUE(victim) ) break;
    }
    if ( !pk )
    {
      CREATE( pk, struct PK_Memory_type, 1 );
      pk->unique        = GET_UNIQUE(victim);
      pk->next          = agressor->pk_list;
      agressor->pk_list = pk;
    }
    if ( victim->desc )
    {  if (IN_ROOM(agressor)!=IN_ROOM(victim))
	   {  if (world[IN_ROOM(agressor)].zone == world[IN_ROOM(victim)].zone)
		  {if (pk_action_type(agressor, victim)>PK_ACTION_FIGHT )
				{  inc_pk_temp(agressor, victim);   
		            return;
				}
		  }
		else
			return;
	   }

		if (IN_ROOM(agressor)==IN_ROOM(victim))
	  if ( pk->kill_num > 0 )
      {
        act("�� �������� ����� ��� ��� ��������� $D!", FALSE, victim, 0, agressor, TO_CHAR);
        act("$N �������$Y ����� ��� ��� ��������� ���!", FALSE, agressor, 0, victim, TO_CHAR);
      }
      else
      {
        act("�� �������� ����� ��������� $D!", FALSE, victim, 0, agressor, TO_CHAR);
        act("$N �������$Y ����� ��������� ���", FALSE, agressor, 0, victim, TO_CHAR);
      }
    }

    pk->kill_num++;
    pk->kill_at = time(NULL);

    pk_check_spamm( agressor );
  }
  save_char(agressor, NOWHERE);

  pk_update_revenge( agressor, victim, BATTLE_DURATION, rent ? KILLER_UNRENTABLE  : 0 );
  pk_update_revenge( victim, agressor, BATTLE_DURATION, rent ? REVENGE_UNRENTABLE : 0 );

  return;
}

// victim ���������� ����� �� agressor
void pk_decrement_kill( struct char_data * agressor, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  // ����������� ��������
  if ( GET_CLAN_RANK(agressor) && GET_CLAN_RANK(victim) )
  {
    // ������ ����-���� � ����� (agressor)
    pk_clear_clanflag( agressor, victim );
    return;
  }
 
  for ( pk = agressor->pk_list; pk; pk = pk->next ) 
  {
    // ��� ������ ��� ���� � ������ ������ ?
    if ( pk->unique == GET_UNIQUE(victim) ) break;
  }
  if ( !pk ) return; // � �����-�� � �� ���� :(

  if ( pk->thief_exp>time(NULL) )
  {
    act("�� ��������� $D �� ���������.", FALSE, victim, 0, agressor, TO_CHAR);
    pk->thief_exp = 0;
    return;
  }

  if ( pk->kill_num )
  {
    if ( --(pk->kill_num) == 0 )
      act("�� ������ �� ������ ������ $D.", FALSE, victim, 0, agressor, TO_CHAR);
    pk->revenge_num = 0;
  }
  return;
}

//������� ���������� ����� ��������� ����� �� ������� agressor

void pk_temp_revenge (struct char_data * agressor, struct char_data * victim)
{ struct PK_Memory_type *pk;

 for ( pk = victim->pk_list; pk; pk = pk->next ) 
    if ( pk->unique == GET_UNIQUE(agressor) ) break;
  if ( !pk ) 
  {
    mudlog("��������� ���������� ��� ����� �����!", CMP, LVL_GOD, TRUE);
    return;
  }

    if ( !pk->temp_at ) 
  {
    // ������� �����. ����� �� ����� ���� ����� � ���,
    // �� ��� ������ ���������
    return;
  }
  act("�� ������������ ����� ��������� ����� $D.", FALSE, agressor, 0, victim, TO_CHAR);
  act("$N ����������� ��������� �����.", FALSE, victim, 0, agressor, TO_CHAR);
 
    pk->battle_exp     = time(NULL)+BATTLE_DURATION*60;

 --(pk->temp_at);
  return;
}

// ��������� ������� ����������� ����� �� ������� agressor
void pk_increment_revenge( struct char_data * agressor, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  for ( pk = victim->pk_list; pk; pk = pk->next ) 
    if ( pk->unique == GET_UNIQUE(agressor) ) break;
  if ( !pk ) 
  {
    mudlog("��������� ���������� ��� ����� �����!", CMP, LVL_GOD, TRUE);
    return;
  }
  if ( GET_CLAN_RANK(agressor) && GET_CLAN_RANK(victim) )
  {
    // ����������� �������� (����� ����� ����� ��� - ��������� ����-����)
    pk_update_clanflag(agressor,victim);
    return;
  }
  if ( !pk->kill_num ) 
  {
    // ������� �����. ����� �� ����� ���� ����� � ���,
    // �� ��� ������ ���������
    return;
  }
  act("�� ������������ ����� ����� $D.", FALSE, agressor, 0, victim, TO_CHAR);
  act("$N ��������$Y ���.", FALSE, victim, 0, agressor, TO_CHAR);
  if ( pk->revenge_num == MAX_REVENGE ) 
  {
    mudlog("������������ revenge_num!", CMP, LVL_GOD, TRUE);
  }
  ++(pk->revenge_num);
  return;
}

void pk_increment_agkill( struct char_data * agressor, struct char_data * victim )
{

 //��������, ��� ������� �� ����������, ��������� �����.
    struct char_data * leader;
    struct follow_type * f;

   leader = agressor->master?agressor->master:agressor;
		
		if ( AFF_FLAGGED(leader,AFF_GROUP))
			{ if ( pk_action_type(leader, victim)>PK_ACTION_FIGHT )
                   pk_increment_kill( leader, victim , TRUE );
	  		}

        for ( f = leader->followers; f; f = f->next )
           if ( AFF_FLAGGED(f->follower,AFF_GROUP))
			{ if ( pk_action_type(f->follower,victim)>PK_ACTION_FIGHT )
              pk_increment_kill( f->follower, victim , TRUE );
	        }
}


void pk_increment_gkill( struct char_data * agressor, struct char_data * victim )
{ 
  if ( !AFF_FLAGGED(victim,AFF_GROUP) && !AFF_FLAGGED(agressor,AFF_GROUP)) 
  {
   // ��������� �� ��������
    pk_increment_kill( agressor, victim, TRUE );

  }
  else
  {
    // ��������� �� ����� ������
    struct char_data * leader;
    struct follow_type * f;

    leader = victim->master?victim->master:victim;

    if ( AFF_FLAGGED(leader,AFF_GROUP))
	{ if (world[IN_ROOM(agressor)].zone == world[IN_ROOM(victim)].zone &&
          pk_action_type(agressor,leader)>PK_ACTION_FIGHT)
			{ pk_increment_kill( agressor, leader, leader == victim );
			  pk_increment_agkill( agressor, leader );
			}
	   
	 }
      else
	    pk_increment_agkill( agressor, leader );
	   
	  
    for ( f = leader->followers; f; f = f->next )
	 if (AFF_FLAGGED(f->follower,AFF_GROUP))
		{    if (world[IN_ROOM(f->follower)].zone == world[IN_ROOM(victim)].zone               &&
				pk_action_type(agressor,f->follower)>PK_ACTION_FIGHT)
				{ pk_increment_kill( agressor, f->follower, f->follower == victim );
                  pk_increment_agkill( agressor, f->follower );
				}
		}
	  else
	  	  pk_increment_agkill( agressor, f->follower );
 
  }
	
}

void pk_agro_action( struct char_data * agressor, struct char_data * victim )
{

  pk_translate_pair( &agressor, &victim );
  switch ( pk_action_type( agressor, victim ) )
  {
    case PK_ACTION_NO:      // ��� ���������� ������ �����
      break;

    case PK_ACTION_REVENGE: // agressor �������� ����������� ����� �� victim
      pk_increment_revenge( agressor, victim );
      // ��� break �� �����, �.�. ����� ������ ������� � ��

    case PK_ACTION_FIGHT:   // ��� ������� ���������� ����������� � ��������
      // �������� ����� �������� � ��
      pk_update_revenge( agressor, victim, BATTLE_DURATION, REVENGE_UNRENTABLE );
      pk_update_revenge( victim, agressor, BATTLE_DURATION, REVENGE_UNRENTABLE );
      break;

    case PK_ACTION_KILL:    // agressor ����� ������ �������� ������ victim
      // ������� ������ ���� ������
      // �������� �� ���� �������
      // �� ������ ����� ����������������� �����������
	 //  ���� � ������ � �������, ����� ������� �� ������, ���������� ������ (�� ��������)
		if (same_group(agressor, victim ))
          break;
       pk_increment_gkill(agressor, victim);
	      break;
    case PK_TEMP_REVENGE:
	  pk_temp_revenge (agressor, victim );
	  break;
   }

}

void pk_thiefs_action( struct char_data * thief, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  pk_translate_pair( &thief, &victim );
  switch ( pk_action_type( thief, victim ) )
  {
    case PK_ACTION_NO:
      return;

    case PK_ACTION_FIGHT:
    case PK_ACTION_REVENGE:
    case PK_ACTION_KILL:
      // ��������/���������� ���� ���������
      for ( pk = thief->pk_list; pk; pk = pk->next ) 
        if ( pk->unique == GET_UNIQUE(victim) ) break;
      if ( !pk )
      {
        CREATE( pk, struct PK_Memory_type, 1 );
        pk->unique     = GET_UNIQUE(victim);
        pk->next       = thief->pk_list;
        thief->pk_list = pk;
      }
      if ( pk->thief_exp == 0 )
        act("$N �������$Y ����� ����� �� ���!", FALSE, thief, 0, victim, TO_CHAR);
      else
        act("$N �������$Y ����� ����� �� ���!", FALSE, thief, 0, victim, TO_CHAR);
      pk->thief_exp   = time(NULL)+THIEF_UNRENTABLE*60;
      RENTABLE(thief) = MAX(RENTABLE(thief),time(NULL)+THIEF_UNRENTABLE*60);
      break;
  }
  return;
}

void pk_revenge_action( struct char_data * killer, struct char_data * victim )
{

  if ( killer )
  {
    pk_translate_pair( &killer, NULL );
  
    if ( !IS_NPC( killer ) && !IS_NPC( victim ) ) 
    {
      // ���� ���� ���������
      pk_decrement_kill( victim, killer );
      // ���������� ��������, ������ ���������� ������� ��
      pk_update_revenge( killer, victim, 0, REVENGE_UNRENTABLE );
    }
  }

  // ��������� ��� ��������, � ������� ���������� victim
  for ( killer = character_list; killer; killer = killer->next )
  {
    if ( IS_NPC(killer) ) continue;
    pk_update_revenge( victim, killer, 0, 0 );  
    pk_update_revenge( killer, victim, 0, 0 );  
  }
}

int pk_action_type(CHAR_DATA * agressor, CHAR_DATA * victim)
{ struct PK_Memory_type *pk;
  pk_translate_pair( &agressor, &victim );
 
  if ( !agressor                                  ||
       !victim                                    ||
       agressor == victim                         ||
       ROOM_FLAGGED(IN_ROOM(agressor),ROOM_ARENA) ||
       ROOM_FLAGGED(IN_ROOM(victim),ROOM_ARENA)   || // ������������� ���� � ��������� � ������
       IS_NPC(agressor)                           ||
       IS_NPC(victim)
     )
    return PK_ACTION_NO;

     if (!PRF_FLAGGED(victim, PRF_AGRO))
	 {   SET_BIT(PRF_FLAGS(victim), PRF_AGRO);
         SET_BIT(PRF_FLAGS(victim), PRF_AGRO_AUTO);
	     send_to_char("\r\n&K����� &R����&K ������� �������������!&n\r\n\r\n",victim);
	 }
  // ����� ����� ���� ����� ������ � ���� ������
  // �������� �������������� ��� �� ������
  if ( PLR_FLAGGED(victim, PLR_KILLER) )
    return PK_ACTION_FIGHT;

  for ( pk = agressor->pk_list; pk; pk = pk->next )
  { if (pk->unique != GET_UNIQUE(victim))
		continue;
    if (pk->battle_exp>time(NULL))
		return PK_ACTION_FIGHT;
    if (pk->revenge_num >= MAX_REVENGE)
		pk_decrement_kill(agressor,victim);
  }

  for ( pk = victim->pk_list; pk; pk = pk->next )
  {
    if (pk->unique != GET_UNIQUE(agressor))
		continue;
    if (pk->battle_exp>time(NULL))
		return PK_ACTION_FIGHT;
    if (pk->revenge_num >= MAX_REVENGE)
		pk_decrement_kill(victim,agressor);
    if ( GET_CLAN_RANK(agressor) && // ��������� ������ ���� � �����
//       GET_CLAN_RANK(victim)   && // ��������� ����� ���� � �� � �����
                                    // ��� ������, ��� ��� ��������� �� ����� 
                                    // �������� ����-�����
         pk->clan_exp > time(NULL)) 
		 return PK_ACTION_REVENGE;  // ����� �� ����-�����
    if (pk->kill_num             &&
        !(GET_CLAN_RANK(agressor) && GET_CLAN_RANK(victim)))
		return PK_ACTION_REVENGE;  // ������� �����
    if (pk->thief_exp>time(NULL))
		return PK_ACTION_REVENGE;  // ����� ����
	if (pk ->temp_at && pk->temp_time + MAX_PTHIEF_LAG > time(NULL))
		return PK_TEMP_REVENGE; //��������� �����
  }
  return PK_ACTION_KILL; 
}

char *CCPK(struct char_data *ch, int lvl, struct char_data *victim) 
{
 int i;
 
 i = pk_count(victim);
 if (i >= FifthPK) 
    return (char* )CCRED(ch, lvl);
 else	       
 if (i >= FourthPK) 
    return (char* )CCMAG(ch, lvl);
 else	       
 if (i >= ThirdPK) 
    return (char* )CCYEL(ch, lvl);
 else	       
 if (i >= SecondPK) 
    return (char* )CCCYN(ch, lvl);
 else   
 if (i >= FirstPK) 
    return (char* )CCGRN(ch, lvl);
 else	       
    return (char* )CCNRM(ch, lvl);
}

void aura(struct char_data *ch, int lvl, struct char_data *victim, char *s)
{
 int i;
 
 i = pk_count(victim);
 if (i >= FifthPK) {
    sprintf(s, "%s(�������� ����)%s",CCRED(ch, lvl),CCRED(ch, lvl));
    return;
 } else	       
 if (i >= FourthPK) {
    sprintf(s, "%s(��������� ����)%s",CCMAG(ch, lvl),CCRED(ch, lvl));
    return;
 } else	       
 if (i >= ThirdPK) {
    sprintf(s, "%s(������ ����)%s",CCYEL(ch, lvl),CCRED(ch, lvl));
    return;
 } else	       
 if (i >= SecondPK) {
    sprintf(s, "%s(������� ����)%s",CCCYN(ch, lvl),CCRED(ch, lvl));
    return;
 } else   
 if (i >= FirstPK) {
    sprintf(s, "%s(������� ����)%s",CCGRN(ch, lvl),CCRED(ch, lvl));
    return;
 } else	{
    sprintf(s, "%s(������ ����)%s",CCNRM(ch, lvl),CCRED(ch, lvl));
    return;
 }
}

/* ������ ������ �� */
void pk_list_sprintf(struct char_data *ch, char *buff)
{
  struct PK_Memory_type *pk;
  char *temp;

  *buff = '\0';
  strcat(buff,"�� ������:\r\n");
  strcat(buff,"              ���    Kill Rvng Clan Batl Thif\r\n");
  for (pk = ch->pk_list; pk; pk = pk->next) 
  {
    temp = get_name_by_unique(pk->unique);
    sprintf(buff+strlen(buff),
            "%20s %4ld %4ld", 
            temp?CAP(temp):"<��� � ����>",pk->kill_num,pk->revenge_num);
    if ( pk->clan_exp>time(NULL) )
      sprintf(buff+strlen(buff)," %4ld",pk->clan_exp-time(NULL));
    else
      strcat(buff,"    -");
    if ( pk->battle_exp>time(NULL) )
      sprintf(buff+strlen(buff)," %4ld",pk->battle_exp-time(NULL));
    else
      strcat(buff,"    -");
    if ( pk->thief_exp>time(NULL) )
      sprintf(buff+strlen(buff)," %4ld",pk->thief_exp-time(NULL));
    else
      strcat(buff,"    -");
    strcat(buff,"\r\n");
  }
}


ACMD(do_revenge)
{ 
  struct char_data      *tch;
  struct PK_Memory_type *pk;
  int    found = FALSE;

  if (IS_NPC(ch))
     return;

  one_argument(argument, arg);

  *buf = '\0';
  for ( tch = character_list; tch; tch = tch->next )
  {
    if (IS_NPC(tch)) continue;
    if ( *arg && !isname(GET_NAME(tch), arg) ) continue;

    for ( pk = tch->pk_list; pk; pk = pk->next )
      if (pk->unique == GET_UNIQUE(ch)) 
      {
        if (pk->revenge_num >= MAX_REVENGE &&
            pk->battle_exp <= time(NULL) 
           ) 
          pk_decrement_kill(tch,ch);
// ������� �������� ���� �����
        if ( GET_CLAN_RANK(ch) && pk->clan_exp>time(NULL) )
        {
           sprintf(buf,"%-40s <� ����� � ����>\r\n",
                   GET_DNAME(tch));
        }
        else
        if ( pk->kill_num + pk->revenge_num > 0 ) 
	    {
       
			 sprintf(buf,"%-12s - ��������� �� ���: %ld, ������� ����� ���������: %ld\r\n",
					  GET_DNAME(tch), pk->kill_num, pk->revenge_num);
	    }
        else
		 if (pk->temp_at && pk->temp_time + MAX_PTHIEF_LAG > time(NULL))
		   { sprintf(buf,"%-12s - ��������� �����, ���������� ������� �����: %ld\r\n", GET_DNAME(tch), pk->temp_at);
		                   			   
			}
	   else
          continue;
        if (!found) send_to_char("�� ������ ����� ��������� :\r\n",ch);
        send_to_char(buf,ch);
        found = TRUE;                   			   
        break;
      }
  }

  if (!found)
     send_to_char("��� ������ ������.\r\n",ch);
}


ACMD(do_forgive)
{ 
  struct char_data      *tch;
  struct PK_Memory_type *pk;
  int    found = FALSE;

  if (IS_NPC(ch))
     return;

/*  if (RENTABLE(ch))
  { send_to_char("�� �� ������ ����������� ��� ����� ��� ������ ���������.\r\n",ch);
     return;
  }*/

  one_argument(argument, arg);
  if ( !*arg )
  {
    send_to_char("���� �� ������ ��������?\r\n",ch);
    return;
  }

  *buf = '\0';

  for ( tch = character_list; tch; tch = tch->next )
  {
    if (IS_NPC(ch)) continue;
    if (!CAN_SEE_CHAR(ch,tch)) continue;
    if (!isname(GET_NAME(tch),arg)) continue;

   
    for ( pk = tch->pk_list; pk; pk = pk->next )
      if (pk->unique == GET_UNIQUE(ch) && pk->kill_num + pk->revenge_num + pk->temp_at > 0) 
      // if(pk->kill_num + pk->revenge_num + pk->temp_at > 0)
	  {  found = TRUE;
        pk->kill_at     = 0;
        pk->battle_exp  = 0;
        pk->thief_exp   = 0;
        pk->clan_exp    = 0;
		pk->temp_time   = 0;
        if (pk->kill_num)
		    pk->kill_num --;
        if (pk->revenge_num)
			pk->revenge_num --;
		if (pk->temp_at)
		    pk->temp_at --;
        
	       break;
      
	 } 
    break;
  }

  if ( found ) 
  {  act("�� �������� $V.", FALSE, ch, 0, tch, TO_CHAR );
     sprintf(buf,"��� $N �������$Y ���������, �������� ����������� -%ld, ��������� - %ld",
                    pk->kill_num, pk->temp_at);
					act(buf, FALSE, tch, 0, ch, TO_CHAR);
  }  
  else
	   send_to_char("��� ������ �������� �����.\r\n",ch);
   //  send_to_char("������, ����� ������ ��� � ����.\r\n",ch);
}

void pk_free_list( struct char_data * ch )
{
  struct PK_Memory_type *pk, *pk_next;

  for ( pk=ch->pk_list; pk; pk=pk_next )
  {
    pk_next = pk->next;
    free(pk);
  }
}


void new_load_pkills(struct char_data * ch)
{FILE *loaded;
 struct pkiller_file_u pk_data;
 struct PK_Memory_type *pk_one = NULL;
 char   filename[MAX_STRING_LENGTH];

 /**** read pkiller list for char */
 log("Load pkill %s", GET_NAME(ch));
 if (get_filename(GET_NAME(ch),filename,PKILLERS_FILE) &&
     (loaded = fopen(filename,"r+b")))
    {while (!feof(loaded))
          {fread(&pk_data, sizeof(struct pkiller_file_u), 1, loaded);
           if (!feof(loaded))
              {if (pk_data.unique < 0 || !correct_unique(pk_data.unique))
                  continue;
               for (pk_one = ch->pk_list; pk_one; pk_one = pk_one->next)
                   if (pk_one->unique == pk_data.unique)
                      break;
               if (pk_one)
                  {log("SYSERROR : duplicate entry pkillers data for %s", GET_NAME(ch));
                   continue;
                  }
               // log("SYSINFO : load pkill one for %s", GET_NAME(ch));
              CREATE(pk_one, struct PK_Memory_type, 1);
              pk_one->unique       = pk_data.unique;
              pk_one->kill_num     = pk_data.pkills;
              pk_one->next         = ch->pk_list;
              ch->pk_list          = pk_one;
             };
          }
     fclose(loaded);
    }
}

/* Load a char, TRUE if loaded, FALSE if not */
void load_pkills(struct char_data * ch)
{
 if (USE_SINGLE_PLAYER)
     {new_load_pkills(ch);
      return;
     }
}

void save_pkills(struct char_data * ch)
{ 
  if (USE_SINGLE_PLAYER)
     {new_save_pkills(ch);
      return;
    }
}

void new_save_pkills(struct char_data * ch)
{ FILE   *saved;
  struct PK_Memory_type *pk, *temp, *tpk;
  struct char_data *tch = NULL;
  struct pkiller_file_u pk_data;
  char   filename[MAX_STRING_LENGTH];

  /**** store pkiller list for char */
 // log("Save pkill %s", GET_NAME(ch));
  if (get_filename(GET_NAME(ch),filename,PKILLERS_FILE) &&
      (saved = fopen(filename,"w+b"))
     )
  {
    for (pk=ch->pk_list; pk && !PLR_FLAGGED(ch,PLR_DELETED);)
    {
      if (pk->kill_num > 0 && correct_unique(pk->unique))
      {
        if (pk->revenge_num >= MAX_REVENGE &&
            pk->battle_exp <= time(NULL) 
           ) 
        { 
          for (tch = character_list; tch; tch = tch->next)
            if (!IS_NPC(tch) && GET_UNIQUE(tch) == pk->unique)
              break;
          if ( --(pk->kill_num) == 0 && tch )
            act("�� ������ �� ������ ������ $D.", FALSE, tch, 0, ch, TO_CHAR);
        }  
        if (pk->kill_num <= 0)
        {
          tpk = pk->next;
          REMOVE_FROM_LIST(pk, ch->pk_list, next);
          free(pk);
          pk = tpk;
          continue;
        }
        pk_data.unique = pk->unique;
        pk_data.pkills = pk->kill_num;
        fwrite(&pk_data, sizeof(struct pkiller_file_u), 1, saved);
      }
      pk = pk->next;
    }
    fclose(saved);
  }
}

// �������� ����� �� ch ������ ����������� �������� ������ victim
int may_kill_here(CHAR_DATA *ch, CHAR_DATA *victim)
{
  if ( !victim ) 
   return true;

  if (PLR_FLAGGED(ch, PLR_IMMKILL))
    { send_to_char("��� ��������� ������ ��������!\r\n", ch);
      return false;
    } 
 
  
  if (PLR_FLAGGED(victim, PLR_IMMKILL))
    { send_to_char("��� ��������� ������ ��������!\r\n", ch);
      return false;
    } 
 

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
	  return false;
  
  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOFIGHT))
  {
    act("�� �� ������ ��������� ���� $D.",FALSE,ch,0,victim,TO_CHAR);
    return false;
  }


  if ((world[ch->in_room].zone == 4 || world[ch->in_room].zone == 5) && !IS_NPC(victim))
	   { send_to_char("�� �� ������ ��������� ������������ � �����!\r\n", ch);
         return false;
       }

    // �� ����� ������ <=10 �� ����� >=20
/*	if (!IS_NPC(victim) && !IS_NPC(ch) && GET_LEVEL(ch)<=10 && GET_LEVEL(victim)>=20 && 
   !(pk_action_type(ch,victim)&(PK_ACTION_REVENGE|PK_ACTION_FIGHT)) )
  {
    act("�� ��� ������� �����, ����� ������� �� $V.",FALSE,ch,0,victim,TO_CHAR);
    return (FALSE);
  }*/

  if ((FIGHTING(ch)     && FIGHTING(ch) == victim) ||
      (FIGHTING(victim) && FIGHTING(victim) == ch)
     )
    return true;

  if (ch != victim && 
      (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)    ||
       ROOM_FLAGGED(victim->in_room, ROOM_PEACEFUL)
      )
     )
  {
    // ���� �� ���������� � ������ �������
    if (MOB_FLAGGED(victim,MOB_HORDE))
    {
      return true;
    }
    if (IS_GOD(ch)                                                      ||
        (IS_NPC(ch) && ch->nr == real_mobile(DG_CASTER_PROXY))          ||
        (pk_action_type(ch,victim)&(PK_ACTION_REVENGE|PK_ACTION_FIGHT|PK_TEMP_REVENGE)))
             return true;
    else
    { send_to_char("����� ������� �����, ����� ������ �����!\r\n", ch);
           return false;
    }
  }
  return true;
}

// ����������� ��������� �� ��������
// ��� ������ ����� ��������������� ������ �� ������� ������
// ����������� ������������� ����������� ��,
// �.�. !NPC ������ ��� !NPC ������ ������
// TRUE - �������� �� ��������, FALSE - ����� �� ��
int check_pkill(struct char_data *ch,struct char_data *opponent,char *arg)
{
  char opp_name_part[200] = "\0",opp_name[200] = "\0",*opp_name_remain;


  if (PRF_FLAGGED(ch, PRF_AGRO))
            return FALSE;

  // ������������� ������ �������� � ��?
  if ( IS_NPC(opponent) &&
       (!opponent->master || IS_NPC(opponent->master) || opponent->master == ch)
     ) return FALSE;

  // ��� ����?
  if ( FIGHTING(ch) == opponent || FIGHTING(opponent) == ch ) return FALSE;

  // ��� �� �������? 
  if (!arg) return TRUE;
  if (!*arg) return FALSE;

  while (*arg && (*arg == '.' || (*arg >= '0' && *arg <= '9'))) ++arg;

  strcpy(opp_name,GET_NAME(opponent));
  for ( opp_name_remain = opp_name; *opp_name_remain; )
  {
    opp_name_remain = one_argument(opp_name_remain, opp_name_part);
    if (!str_cmp(arg, opp_name_part)) return FALSE;
  }

  // ���������� �� �����
  send_to_char("��� ���������� ����������������� �������� ������� ��� ������ ���������.\r\n",ch);
  return TRUE;
}
 void pk_temp_timer(struct char_data * agressor)
 {  struct PK_Memory_type *pk;

 	 for (pk = agressor->pk_list; pk; pk = pk->next)
            if (pk->temp_time + MAX_PTHIEF_LAG < time(NULL))
		{ pk->temp_at   = 0;
	          pk->temp_time = 0;
		}
	 
 }
