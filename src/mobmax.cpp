/* ************************************************************************
*   File: mobmax.c                                      Part of CircleMUD *
*  Usage: ������ ������� �� �����                                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"

#include "mobmax.h"

#define MAX_MOB_LEVEL 45       /* ������������ ������� ����� */
#define MIN_MOB_IN_MOBKILL 2   /* ����������� ���������� ����� ������ ������ */
                               /*  � ����� ������� */
#define MAX_MOB_IN_MOBKILL 100 /* ������������ ���������� ����� ������ ������ */
                               /*  � ����� ������� */
#define MOBKILL_KOEFF 3        /* �� ������� ��� ���� ����� ����� ������ */
                               /* ���  �� ���� � ����, ����� ������ */
			       /*  ������������*/

/* external vars */
extern mob_rnum top_of_mobt;
extern struct char_data *mob_proto;

/* local global */
int num_mob_lev[MAX_MOB_LEVEL+1];

/* ������� ��� ����� ������ level */
void drop_old_mobs(struct char_data *ch, int level) 
{
  int i, lev_count, r_num;

  if (level > MAX_MOB_LEVEL || level < 0)
   return;

  i = ch->MobKill.count - 1;
  lev_count = 0;
  while (i >= 0) {
      r_num = real_mobile(ch->MobKill.vnum[i]);
      if (r_num < 0 || (ch->MobKill.howmany[i]) < 1) {
	  clear_kill_vnum(ch, (ch->MobKill.vnum[i]));
      } else
      if (GET_LEVEL(mob_proto + r_num) == level) {
        if (lev_count > num_mob_lev[level]) {
	  clear_kill_vnum(ch, (ch->MobKill.vnum[i]));
	} else {
	  lev_count++;
	}
      }
      i--;
  }
}

/* �������� ����� ������� */
void new_load_mkill(struct char_data *ch)
{FILE *loaded;
 char filename[MAX_STRING_LENGTH];
 int  intload = 0,i,c;

 if (ch->MobKill.howmany)
    {log("MOBKILL ERROR for %s - attempt load when not empty - call to Coder",
         GET_NAME(ch));
     free(ch->MobKill.howmany);
    }
 if (ch->MobKill.vnum)
    {log("MOBKILL ERROR for %s - attempt load when not empty - call to Coder",
         GET_NAME(ch));
     free(ch->MobKill.vnum);
    }
 ch->MobKill.count   = 0;
 ch->MobKill.howmany = NULL;
 ch->MobKill.vnum    = NULL;

 log("Load mkill %s", GET_NAME(ch));
 if (get_filename(GET_NAME(ch),filename,PMKILL_FILE) &&
     (loaded = fopen(filename,"r+b")))
    {intload = fread(&ch->MobKill.count, sizeof(int), 1, loaded);
     if (ch->MobKill.count && intload)
        {CREATE(ch->MobKill.howmany, int, (ch->MobKill.count / 10L + 1) * 10L);
         CREATE(ch->MobKill.vnum,    int, (ch->MobKill.count / 10L + 1) * 10L);
         intload = fread(ch->MobKill.howmany, sizeof(int), ch->MobKill.count, loaded);
         intload = fread(ch->MobKill.vnum,    sizeof(int), ch->MobKill.count, loaded);
         if (intload < ch->MobKill.count)
            ch->MobKill.count = intload;
         for (i = c = 0; c < ch->MobKill.count; c++)
             if (*(ch->MobKill.vnum+c) > 0 && *(ch->MobKill.vnum+c) < 100000)
                {*(ch->MobKill.vnum+i)    = *(ch->MobKill.vnum+c);
                 *(ch->MobKill.howmany+i) = *(ch->MobKill.howmany+c);
                 i++;
                }
         ch->MobKill.count = i;
        }
     else
        {
         ch->MobKill.count = 0;
         CREATE(ch->MobKill.howmany, int, 10);
         CREATE(ch->MobKill.vnum,    int, 10);
        }
     fclose(loaded);
    }
 else
    {
     CREATE(ch->MobKill.howmany, int, 10);
     CREATE(ch->MobKill.vnum,    int, 10);
    }
    /* ������� ������ �� ������ �����, ���������� �� ����� ���������� 
       ��� ����� ���������� */
   // for (i = 0; i<=MAX_MOB_LEVEL; i++) - �������� ������, ����� 3.11.02!!!
   //   drop_old_mobs(ch, i);
}

/* ������� ������ �� ���� vnum. ���������� true ���� ����� ������ � false 
   ���� ��� � �� ���� */
int clear_kill_vnum(struct char_data *vict, int vnum)
{
  int i, j;  

  for (i = j = 0; j < vict->MobKill.count; i++, j++) {
    if (vict->MobKill.vnum[i] == vnum)
      j++;
    vict->MobKill.vnum[i]    = vict->MobKill.vnum[j];
    vict->MobKill.howmany[i] = vict->MobKill.howmany[j];
  }
  if (j > i) {
    vict->MobKill.count--;
    return (TRUE);
  } else {
    return (FALSE);
  }
}

/* ����������� ���������� ������ vnum ����� �� incvalue */
void inc_kill_vnum(struct char_data *ch, int vnum, int incvalue)
{ int i;
  if (IS_NPC(ch) || IS_IMMORTAL(ch) || vnum < 0)
     return;

  if (ch->MobKill.vnum)
     {for (i = 0; i < ch->MobKill.count; i++)
          if (*(ch->MobKill.vnum+i) == vnum)
             {*(ch->MobKill.howmany+i) += incvalue;
              return;
             }
      if (!(ch->MobKill.count % 10L))
         {RECREATE(ch->MobKill.howmany, int, (ch->MobKill.count / 10L + 1) * 10L);
          RECREATE(ch->MobKill.vnum,    int, (ch->MobKill.count / 10L + 1) * 10L);
         }
     }
  else
     {ch->MobKill.count  = 0;
      CREATE(ch->MobKill.vnum,    int, 10);
      CREATE(ch->MobKill.howmany, int, 10);
     }

  *(ch->MobKill.vnum+ch->MobKill.count)       = vnum;
  *(ch->MobKill.howmany+ch->MobKill.count++)  = incvalue;

  /* �������� �������  �������� ���� ��� 3.11.02*/
 /* i = real_mobile(vnum);
  if (i >= 0) {
    drop_old_mobs(ch, GET_LEVEL(mob_proto + i));
  }*/
}

/* ��������� ���������� ������ vnum ����� */
int get_kill_vnum(struct char_data *ch, int vnum)
{ int i;
  if (IS_NPC(ch) || IS_IMMORTAL(ch) || vnum < 0)
     return (0);
  if (ch->MobKill.vnum)
     {for (i = 0; i < ch->MobKill.count; i++)
          if (*(ch->MobKill.vnum+i) == vnum)
             return (*(ch->MobKill.howmany+i));
     }
  return (0);
}


void save_mkill(struct char_data *ch, FILE *saved) 
{
  int i;
  mob_rnum r_num;
  
  if (IS_NPC(ch) || IS_IMMORTAL(ch))
     return;
  if (ch->MobKill.vnum) {
     for (i = 0; i < ch->MobKill.count; i++)
         if ((r_num = real_mobile(*(ch->MobKill.vnum+i))) > -1) {
           fprintf(saved,"Mob : %d %d %s\n",*(ch->MobKill.vnum+i),
	        *(ch->MobKill.howmany+i), 
		mob_proto[r_num].player.short_descr);
         }
  }
}


/* ��������� �� ����� ���� ������� */
void new_save_mkill(struct char_data *ch)
{ FILE *saved;
  char filename[MAX_STRING_LENGTH];

  if (!ch->MobKill.count || !ch->MobKill.vnum || !ch->MobKill.howmany)
     {//if (!IS_IMMORTAL(ch))
      //   {sprintf(buf,"SYSERR: MobKill list for %s empty...",GET_NAME(ch));
      //    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      //   }
     }

//  log("Save mkill %s", GET_NAME(ch));
  if (get_filename(GET_NAME(ch),filename,PMKILL_FILE) &&
      (saved = fopen(filename,"w+b")))
      {fwrite(&ch->MobKill.count, sizeof(int), 1, saved);
       fwrite(ch->MobKill.howmany, sizeof(int), ch->MobKill.count, saved);
       fwrite(ch->MobKill.vnum,    sizeof(int), ch->MobKill.count, saved);
       fclose(saved);
      }
}

/* ������� ������ ��� ���������� ������ ��� ���� */
void free_mkill(struct char_data *ch) 
{
      if (ch->MobKill.howmany)
         free(ch->MobKill.howmany);

      if (ch->MobKill.vnum)
         free(ch->MobKill.vnum);

      ch->MobKill.count  = 0;
      ch->MobKill.vnum  = NULL;
      ch->MobKill.howmany  = NULL;
}

/* ������� ���� ������� */
void delete_mkill_file(char *name)
{
 char   filename[MAX_INPUT_LENGTH];
 
 get_filename(name, filename, PMKILL_FILE);
 remove(filename);
}

/* �������� ���������� ����� ����� ������� ������ � ���� � ����������
   ������������� �� ���������� � ����� ������� */
void mob_lev_count(void)
{
  int nr;
  
  for (nr = 0; nr<=MAX_MOB_LEVEL ; nr++)
     num_mob_lev[nr] = 0;
  
  for (nr = 0; nr <= top_of_mobt; nr++) {
    if (GET_LEVEL(mob_proto + nr) > MAX_MOB_LEVEL) 
      log("Warning! Mob >MAXIMUN lev!");
    else if (GET_LEVEL(mob_proto + nr) < 0) 
      log("Warning! Mob <0 lev!");
    else
      num_mob_lev[(int) GET_LEVEL(mob_proto + nr)]++;
  }
     
  for (nr = 0; nr<=MAX_MOB_LEVEL ; nr++) {
     log("Mob lev %d. Num of mobs %d", nr, num_mob_lev[nr]);
     num_mob_lev[nr] = num_mob_lev[nr] / MOBKILL_KOEFF;
     if (num_mob_lev[nr] < MIN_MOB_IN_MOBKILL)
         num_mob_lev[nr] = MIN_MOB_IN_MOBKILL;
     if (num_mob_lev[nr] > MAX_MOB_IN_MOBKILL)
         num_mob_lev[nr] = MAX_MOB_IN_MOBKILL;
     log("Mob lev %d. Max in MobKill file %d", nr, num_mob_lev[nr]);
     
  }
}
