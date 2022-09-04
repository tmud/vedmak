/* ************************************************************************
*   File: pk.c                                          Part of CircleMUD *
*  Usage: ПК система                                                      *
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

// Временные константы в минутах реального времени
#define KILLER_UNRENTABLE  30
#define REVENGE_UNRENTABLE 10
#define CLAN_REVENGE       10
#define THIEF_UNRENTABLE   30
#define BATTLE_DURATION    3
#define SPAM_PK_TIME       30

// Временные константы в секундах реального времени
#define TIME_PK_GROUP      5

// Временные константы в часах реального времени
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
    act("За свои деяния, ты предан АНАФЕМЕ!", FALSE, ch, 0, 0, TO_CHAR);
  }
  if ( pk_count(ch) >= KillerPK ) 
  SET_BIT(PLR_FLAGS(ch, PLR_KILLER), PLR_KILLER);
 }

// функция переводит переменные *pkiller и *pvictim на хозяев, если это чармы
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

// agressor совершил противоправные действия против victim
// выдать/обновить клан-флаг
void pk_update_clanflag( struct char_data * agressor, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  for ( pk = agressor->pk_list; pk; pk = pk->next ) 
  {
    // эта жертва уже есть в списке игрока ?
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
      act("Вы продлили право клановой мести $D!", FALSE, victim, 0, agressor, TO_CHAR);
      act("$N продлил$Y право еще раз отомстить Вам!", FALSE, agressor, 0, victim, TO_CHAR);
    }
    else
    {
      act("Вы получили право клановой мести $D!", FALSE, victim, 0, agressor, TO_CHAR);
      act("$N получил$Y право клановой мести на Вас!", FALSE, agressor, 0, victim, TO_CHAR);
    }
  }
  pk->clan_exp = time(NULL) + CLAN_REVENGE*60;

  save_char(agressor, NOWHERE);

  return;
}

// victim убил agressor (оба в кланах) 
// снять клан-флаг у agressor
void pk_clear_clanflag( struct char_data * agressor, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  for ( pk = agressor->pk_list; pk; pk = pk->next ) 
  {
    // эта жертва уже есть в списке игрока ?
    if ( pk->unique == GET_UNIQUE(victim) ) break;
  }
  if ( !pk ) return; // а мести-то и не было :(

  if ( pk->clan_exp>time(NULL) )
  {
    act("Вы использовали право клановой мести $D.", FALSE, victim, 0, agressor, TO_CHAR);
  }
  pk->clan_exp = 0;
 
  return;
}

// Продлевается время поединка и БД
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

//установим флаг временной мести или продлим его в ранге -+3 левела
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
	{act("$N продлил$Y право временной мести для Вас!", FALSE, agressor, 0, victim, TO_CHAR);
     act("Вы продлили право временной мести $D!", FALSE, victim, 0, agressor, TO_CHAR);
	}	
	else
	{ act("$N получил$Y право временной мести на Вас!", FALSE, agressor, 0, victim, TO_CHAR);
	  act("Вы получили право временной мести на $V!", FALSE, victim, 0, agressor, TO_CHAR);
	}

	++pk->temp_at;//количество нападений по флагу временная месть.
    pk->temp_time = time(NULL);
	pk->battle_exp     = time(NULL)+BATTLE_DURATION*60; //установим 3 минуты батл_флаг
    RENTABLE(agressor)    = MAX(RENTABLE(agressor), time(NULL) + MAX_PTHIEF_LAG);
    sprintf(buf,"Установка флага временной мести : %s->%s",GET_NAME(agressor),GET_NAME(victim));
    mudlog(buf, CMP, LVL_GOD, FALSE);

    pk_check_spamm( agressor );
  save_char(agressor, NOWHERE);
}


// agressor совершил противоправные действия против victim
// 1. выдать флаг
// 2. начать поединок
// 3. если нужно, начать БД
// 4. Проверяем в одной руме и ранг обеспечивает временную месть, ставим временную месть
//    если не в одной руме и в группе, ставим временную месть на всех участников агрессии.
//    если в одной руме, всем выдаем в зависимости от разницы уровней, либо вечную месть, либо временную.
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
 {  inc_pk_temp(agressor, victim);//установим или продлим флаг временной мести.
   return ;
 }

 //разобраться и переписать клан_пк временную месть на кланы не вешаем, только на участников.
 if ( GET_CLAN_RANK(agressor) && GET_CLAN_RANK(victim) && (GET_CLAN(agressor) !=  GET_CLAN(victim)) )
  {
    // межклановая разборка 
    pk_update_clanflag(agressor,victim);
  }
  else
  {
    // не все участники приписаны
    for ( pk = agressor->pk_list; pk; pk = pk->next ) 
    {
      // эта жертва уже есть в списке игрока ?
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
        act("Вы получили право еще раз отомстить $D!", FALSE, victim, 0, agressor, TO_CHAR);
        act("$N получил$Y право еще раз отомстить Вам!", FALSE, agressor, 0, victim, TO_CHAR);
      }
      else
      {
        act("Вы получили право отомстить $D!", FALSE, victim, 0, agressor, TO_CHAR);
        act("$N получил$Y право отомстить Вам", FALSE, agressor, 0, victim, TO_CHAR);
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

// victim реализовал месть на agressor
void pk_decrement_kill( struct char_data * agressor, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  // межклановые разборки
  if ( GET_CLAN_RANK(agressor) && GET_CLAN_RANK(victim) )
  {
    // Снимаю клан-флаг у трупа (agressor)
    pk_clear_clanflag( agressor, victim );
    return;
  }
 
  for ( pk = agressor->pk_list; pk; pk = pk->next ) 
  {
    // эта жертва уже есть в списке игрока ?
    if ( pk->unique == GET_UNIQUE(victim) ) break;
  }
  if ( !pk ) return; // а мести-то и не было :(

  if ( pk->thief_exp>time(NULL) )
  {
    act("Вы отомстили $D за воровство.", FALSE, victim, 0, agressor, TO_CHAR);
    pk->thief_exp = 0;
    return;
  }

  if ( pk->kill_num )
  {
    if ( --(pk->kill_num) == 0 )
      act("Вы больше не можете мстить $D.", FALSE, victim, 0, agressor, TO_CHAR);
    pk->revenge_num = 0;
  }
  return;
}

//попытка реализации флага временной мести со стороны agressor

void pk_temp_revenge (struct char_data * agressor, struct char_data * victim)
{ struct PK_Memory_type *pk;

 for ( pk = victim->pk_list; pk; pk = pk->next ) 
    if ( pk->unique == GET_UNIQUE(agressor) ) break;
  if ( !pk ) 
  {
    mudlog("Инкремент реализации без флага мести!", CMP, LVL_GOD, TRUE);
    return;
  }

    if ( !pk->temp_at ) 
  {
    // Попытка мести. Может на самом деле мести и нет,
    // но это ничего страшного
    return;
  }
  act("Вы использовали право временной мести $D.", FALSE, agressor, 0, victim, TO_CHAR);
  act("$N использовал временную месть.", FALSE, victim, 0, agressor, TO_CHAR);
 
    pk->battle_exp     = time(NULL)+BATTLE_DURATION*60;

 --(pk->temp_at);
  return;
}

// очередная попытка реализовать месть со стороны agressor
void pk_increment_revenge( struct char_data * agressor, struct char_data * victim )
{
  struct PK_Memory_type *pk;

  for ( pk = victim->pk_list; pk; pk = pk->next ) 
    if ( pk->unique == GET_UNIQUE(agressor) ) break;
  if ( !pk ) 
  {
    mudlog("Инкремент реализации без флага мести!", CMP, LVL_GOD, TRUE);
    return;
  }
  if ( GET_CLAN_RANK(agressor) && GET_CLAN_RANK(victim) )
  {
    // межклановая разборка (любая атака после боя - взводится клан-флаг)
    pk_update_clanflag(agressor,victim);
    return;
  }
  if ( !pk->kill_num ) 
  {
    // Попытка мести. Может на самом деле мести и нет,
    // но это ничего страшного
    return;
  }
  act("Вы использовали право мести $D.", FALSE, agressor, 0, victim, TO_CHAR);
  act("$N отомстил$Y Вам.", FALSE, victim, 0, agressor, TO_CHAR);
  if ( pk->revenge_num == MAX_REVENGE ) 
  {
    mudlog("Переполнение revenge_num!", CMP, LVL_GOD, TRUE);
  }
  ++(pk->revenge_num);
  return;
}

void pk_increment_agkill( struct char_data * agressor, struct char_data * victim )
{

 //проверим, кто следует за агрессором, установим флаги.
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
   // нападение на одиночку
    pk_increment_kill( agressor, victim, TRUE );

  }
  else
  {
    // нападение на члена группы
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
    case PK_ACTION_NO:      // без конфликтов просто выход
      break;

    case PK_ACTION_REVENGE: // agressor пытается реализовать месть на victim
      pk_increment_revenge( agressor, victim );
      // тут break не нужен, т.к. нужно начать пединок и БД

    case PK_ACTION_FIGHT:   // обе стороны продолжают участвовать в поединке
      // обновить время поединка и БД
      pk_update_revenge( agressor, victim, BATTLE_DURATION, REVENGE_UNRENTABLE );
      pk_update_revenge( victim, agressor, BATTLE_DURATION, REVENGE_UNRENTABLE );
      break;

    case PK_ACTION_KILL:    // agressor начал боевые действия против victim
      // раздача флагов всей группе
      // поединок со всей группой
      // БД только между непосредственными участниками
	 //  если в группе и сагрили, флаги никакие не ставим, распускаем группу (из файтинга)
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
      // продлить/установить флаг воровства
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
        act("$N получил$Y право мести на Вас!", FALSE, thief, 0, victim, TO_CHAR);
      else
        act("$N продлил$Y право мести на Вас!", FALSE, thief, 0, victim, TO_CHAR);
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
      // один флаг отработал
      pk_decrement_kill( victim, killer );
      // остановить разборку, убийце установить признак БД
      pk_update_revenge( killer, victim, 0, REVENGE_UNRENTABLE );
    }
  }

  // завершить все поединки, в которых участвовал victim
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
       ROOM_FLAGGED(IN_ROOM(victim),ROOM_ARENA)   || // предотвращаем баги с чармисами и ареной
       IS_NPC(agressor)                           ||
       IS_NPC(victim)
     )
    return PK_ACTION_NO;

     if (!PRF_FLAGGED(victim, PRF_AGRO))
	 {   SET_BIT(PRF_FLAGS(victim), PRF_AGRO);
         SET_BIT(PRF_FLAGS(victim), PRF_AGRO_AUTO);
	     send_to_char("\r\n&KРЕЖИМ &RАГРО&K ВКЛЮЧЕН АВТОМАТИЧЕСКИ!&n\r\n\r\n",victim);
	 }
  // Убийц можно бить когда угодно и кому угодно
  // Клановая принадлежность тут не причем
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
    if ( GET_CLAN_RANK(agressor) && // атакующий должен быть в клане
//       GET_CLAN_RANK(victim)   && // атакуемый может быть и не в клане
                                    // это значит, что его исключили на время 
                                    // действия клан-флага
         pk->clan_exp > time(NULL)) 
		 return PK_ACTION_REVENGE;  // месть по клан-флагу
    if (pk->kill_num             &&
        !(GET_CLAN_RANK(agressor) && GET_CLAN_RANK(victim)))
		return PK_ACTION_REVENGE;  // обычная месть
    if (pk->thief_exp>time(NULL))
		return PK_ACTION_REVENGE;  // месть вору
	if (pk ->temp_at && pk->temp_time + MAX_PTHIEF_LAG > time(NULL))
		return PK_TEMP_REVENGE; //временная месть
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
    sprintf(s, "%s(кровавая аура)%s",CCRED(ch, lvl),CCRED(ch, lvl));
    return;
 } else	       
 if (i >= FourthPK) {
    sprintf(s, "%s(пурпурная аура)%s",CCMAG(ch, lvl),CCRED(ch, lvl));
    return;
 } else	       
 if (i >= ThirdPK) {
    sprintf(s, "%s(желтая аура)%s",CCYEL(ch, lvl),CCRED(ch, lvl));
    return;
 } else	       
 if (i >= SecondPK) {
    sprintf(s, "%s(голубая аура)%s",CCCYN(ch, lvl),CCRED(ch, lvl));
    return;
 } else   
 if (i >= FirstPK) {
    sprintf(s, "%s(зеленая аура)%s",CCGRN(ch, lvl),CCRED(ch, lvl));
    return;
 } else	{
    sprintf(s, "%s(чистая аура)%s",CCNRM(ch, lvl),CCRED(ch, lvl));
    return;
 }
}

/* Печать списка пк */
void pk_list_sprintf(struct char_data *ch, char *buff)
{
  struct PK_Memory_type *pk;
  char *temp;

  *buff = '\0';
  strcat(buff,"ПК список:\r\n");
  strcat(buff,"              Имя    Kill Rvng Clan Batl Thif\r\n");
  for (pk = ch->pk_list; pk; pk = pk->next) 
  {
    temp = get_name_by_unique(pk->unique);
    sprintf(buff+strlen(buff),
            "%20s %4ld %4ld", 
            temp?CAP(temp):"<Нет в базе>",pk->kill_num,pk->revenge_num);
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
// Сначала проверка клан флага
        if ( GET_CLAN_RANK(ch) && pk->clan_exp>time(NULL) )
        {
           sprintf(buf,"%-40s <В войне с Вами>\r\n",
                   GET_DNAME(tch));
        }
        else
        if ( pk->kill_num + pk->revenge_num > 0 ) 
	    {
       
			 sprintf(buf,"%-12s - нападений на Вас: %ld, попыток мести агрессору: %ld\r\n",
					  GET_DNAME(tch), pk->kill_num, pk->revenge_num);
	    }
        else
		 if (pk->temp_at && pk->temp_time + MAX_PTHIEF_LAG > time(NULL))
		   { sprintf(buf,"%-12s - временная месть, количество попыток мести: %ld\r\n", GET_DNAME(tch), pk->temp_at);
		                   			   
			}
	   else
          continue;
        if (!found) send_to_char("Вы имеете право отомстить :\r\n",ch);
        send_to_char(buf,ch);
        found = TRUE;                   			   
        break;
      }
  }

  if (!found)
     send_to_char("Вам некому мстить.\r\n",ch);
}


ACMD(do_forgive)
{ 
  struct char_data      *tch;
  struct PK_Memory_type *pk;
  int    found = FALSE;

  if (IS_NPC(ch))
     return;

/*  if (RENTABLE(ch))
  { send_to_char("Вы не можете реализовать это право при боевых действиях.\r\n",ch);
     return;
  }*/

  one_argument(argument, arg);
  if ( !*arg )
  {
    send_to_char("Кого Вы хотите простить?\r\n",ch);
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
  {  act("Вы простили $V.", FALSE, ch, 0, tch, TO_CHAR );
     sprintf(buf,"Вам $N простил$Y нападение, осталось непрощенных -%ld, временной - %ld",
                    pk->kill_num, pk->temp_at);
					act(buf, FALSE, tch, 0, ch, TO_CHAR);
  }  
  else
	   send_to_char("Вам некому простить месть.\r\n",ch);
   //  send_to_char("Похоже, этого игрока нет в игре.\r\n",ch);
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
            act("Вы больше не можете мстить $D.", FALSE, tch, 0, ch, TO_CHAR);
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

// Проверка может ли ch начать аргессивные действия против victim
int may_kill_here(CHAR_DATA *ch, CHAR_DATA *victim)
{
  if ( !victim ) 
   return true;

  if (PLR_FLAGGED(ch, PLR_IMMKILL))
    { send_to_char("Вам запрещены боевые действия!\r\n", ch);
      return false;
    } 
 
  
  if (PLR_FLAGGED(victim, PLR_IMMKILL))
    { send_to_char("Вам запрещены боевые действия!\r\n", ch);
      return false;
    } 
 

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
	  return false;
  
  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOFIGHT))
  {
    act("Вы не можете причинить вред $D.",FALSE,ch,0,victim,TO_CHAR);
    return false;
  }


  if ((world[ch->in_room].zone == 4 || world[ch->in_room].zone == 5) && !IS_NPC(victim))
	   { send_to_char("Вы не можете совершать агродействия в школе!\r\n", ch);
         return false;
       }

    // не агрим чарами <=10 на чаров >=20
/*	if (!IS_NPC(victim) && !IS_NPC(ch) && GET_LEVEL(ch)<=10 && GET_LEVEL(victim)>=20 && 
   !(pk_action_type(ch,victim)&(PK_ACTION_REVENGE|PK_ACTION_FIGHT)) )
  {
    act("Вы еще слишком слабы, чтобы напасть на $V.",FALSE,ch,0,victim,TO_CHAR);
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
    // Один из участников в мирной комнате
    if (MOB_FLAGGED(victim,MOB_HORDE))
    {
      return true;
    }
    if (IS_GOD(ch)                                                      ||
        (IS_NPC(ch) && ch->nr == real_mobile(DG_CASTER_PROXY))          ||
        (pk_action_type(ch,victim)&(PK_ACTION_REVENGE|PK_ACTION_FIGHT|PK_TEMP_REVENGE)))
             return true;
    else
    { send_to_char("Здесь слишком мирно, чтобы начать драку!\r\n", ch);
           return false;
    }
  }
  return true;
}

// Определение возможных ПК действий
// Имя должно ТОЧНО соответствовать одному из алиасов жертвы
// Проверяется потенциальная возможность ПК,
// т.е. !NPC жертва или !NPC хозяин жертвы
// TRUE - возможно ПК действия, FALSE - точно не ПК
int check_pkill(struct char_data *ch,struct char_data *opponent,char *arg)
{
  char opp_name_part[200] = "\0",opp_name[200] = "\0",*opp_name_remain;


  if (PRF_FLAGGED(ch, PRF_AGRO))
            return FALSE;

  // Потенциальная жертва приведет к ПК?
  if ( IS_NPC(opponent) &&
       (!opponent->master || IS_NPC(opponent->master) || opponent->master == ch)
     ) return FALSE;

  // Уже воюю?
  if ( FIGHTING(ch) == opponent || FIGHTING(opponent) == ch ) return FALSE;

  // Имя не указано? 
  if (!arg) return TRUE;
  if (!*arg) return FALSE;

  while (*arg && (*arg == '.' || (*arg >= '0' && *arg <= '9'))) ++arg;

  strcpy(opp_name,GET_NAME(opponent));
  for ( opp_name_remain = opp_name; *opp_name_remain; )
  {
    opp_name_remain = one_argument(opp_name_remain, opp_name_part);
    if (!str_cmp(arg, opp_name_part)) return FALSE;
  }

  // Совпадений не нашел
  send_to_char("Для исключения незапланированной агрессии введите имя жертвы полностью.\r\n",ch);
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
