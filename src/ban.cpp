/* ************************************************************************
*   File: ban.c                                         Part of CircleMUD *
*  Usage: banning/unbanning/checking sites and player names               *
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

struct ban_list_element *ban_list = NULL;
struct proxi_list_element *proxi_list = NULL;
extern struct descriptor_data *descriptor_list;

/* local functions */
void load_banned(void);
int isbanned(char *hostname);
void _write_one_node(FILE * fp, struct ban_list_element * node);
void write_ban_list(void);
void load_proxi(void);
int isproxi(char *hostname);
void _write_one_proxi(FILE * fp, struct proxi_list_element * node);
void write_proxi_list(void);
int Valid_Name(char *newname);
void Read_Invalid_List(void);

ACMD(do_ban);
ACMD(do_unban);
ACMD(do_bedname);
ACMD(do_proxi);
ACMD(do_unproxi);
ACMD(do_bedname);

const char *ban_types[] = {
  "no",
  "новый",
  "выбор",/*select*/
  "все",
  "ERROR"
};


void load_banned(void)
{
  FILE *fl;
  int i, date;
  char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
  char name[MAX_NAME_LENGTH + 1];
  struct ban_list_element *next_node;

  ban_list = 0;

  if (!(fl = fopen(BAN_FILE, "r"))) {
    if (errno != ENOENT) {
      log("SYSERR: Unable to open banfile '%s': %s", BAN_FILE, strerror(errno));
    } else
      log("   Ban file '%s' doesn't exist.", BAN_FILE);
    return;
  }
  while (fscanf(fl, " %s %s %d %s ", ban_type, site_name, &date, name) == 4) {
    CREATE(next_node, struct ban_list_element, 1);
    strncpy(next_node->site, site_name, BANNED_SITE_LENGTH);
    next_node->site[BANNED_SITE_LENGTH] = '\0';
    strncpy(next_node->name, name, MAX_NAME_LENGTH);
    next_node->name[MAX_NAME_LENGTH] = '\0';
    next_node->date = date;

    for (i = BAN_NOT; i <= BAN_ALL; i++)
      if (!strcmp(ban_type, ban_types[i]))
	next_node->type = i;

    next_node->next = ban_list;
    ban_list = next_node;
  }

  fclose(fl);
}


int isbanned(char *hostname)
{
  int i;
  struct ban_list_element *banned_node;
  char *nextchar;

  if (!hostname || !*hostname)
    return (0);

  i = 0;
  for (nextchar = hostname; *nextchar; nextchar++)
    *nextchar = LOWER((unsigned) *nextchar);

  for (banned_node = ban_list; banned_node; banned_node = banned_node->next)
    if (strstr(hostname, banned_node->site))	/* if hostname is a substring */
      i = MAX(i, banned_node->type);

  return (i);
}


void _write_one_node(FILE * fp, struct ban_list_element * node)
{
  if (node) {
    _write_one_node(fp, node->next);
    fprintf(fp, "%s %s %ld %s\n", ban_types[node->type],
	    node->site, (long) node->date, node->name);
  }
}



void write_ban_list(void)
{
  FILE *fl;

  if (!(fl = fopen(BAN_FILE, "w"))) {
    perror("SYSERR: Unable to open '" BAN_FILE "' for writing");
    return;
  }
  _write_one_node(fl, ban_list);/* recursively write from end to start */
  fclose(fl);
  return;
}


ACMD(do_ban)
{
  char flag[MAX_INPUT_LENGTH], site[MAX_INPUT_LENGTH],
	format[MAX_INPUT_LENGTH], *nextchar, *timestr;
  int i;
  struct ban_list_element *ban_node;

  *buf = '\0';

  if (!*argument) {
    if (!ban_list) {
      send_to_char("Ни один сайт не забанен!\r\n", ch);
      return;
    }
    strcpy(format, "%-25.25s  %-8.8s  %-10.10s  %-16.16s\r\n");
    sprintf(buf, format,
	    "Бан для сайта",
	    "Тип Бана",
	    "Запрет на",
	    "Запрет");
    send_to_char(buf, ch); /*Banned Site Name", "Ban Type",Banned By,Banned on*/
    sprintf(buf, format,
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------");
    send_to_char(buf, ch);

    for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
      if (ban_node->date) {
	timestr = asctime(localtime(&(ban_node->date)));
	*(timestr + 10) = 0;
	strcpy(site, timestr);
      } else
	strcpy(site, "Неизвестный");/*Unknown*/
      sprintf(buf, format, ban_node->site, ban_types[ban_node->type], site,
	      ban_node->name);
      send_to_char(buf, ch);
    }
    return;
  }
  two_arguments(argument, flag, site);
  if (!*site || !*flag) {
    send_to_char("Usage: ban {все | выбор | новый} site_name\r\n", ch);
    return;
  }
  if (!(!str_cmp(flag, "выбор") || !str_cmp(flag, "все") || !str_cmp(flag, "новый"))) {
    send_to_char("Flag must be ALL, SELECT, or NEW.\r\n", ch);
    return;
  }
  for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
    if (!str_cmp(ban_node->site, site)) {
     send_to_char("Этот сайт уже забанен -- разбанте и измените тип запрета для этого сайта.\r\n", ch);
      return;
    }
  }

  CREATE(ban_node, struct ban_list_element, 1);
  strncpy(ban_node->site, site, BANNED_SITE_LENGTH);
  for (nextchar = ban_node->site; *nextchar; nextchar++)
    *nextchar = LOWER(*nextchar);
  ban_node->site[BANNED_SITE_LENGTH] = '\0';
  strncpy(ban_node->name, GET_NAME(ch), MAX_NAME_LENGTH);
  ban_node->name[MAX_NAME_LENGTH] = '\0';
  ban_node->date = time(0);

  for (i = BAN_NEW; i <= BAN_ALL; i++)
    if (!str_cmp(flag, ban_types[i]))
      ban_node->type = i;

  ban_node->next = ban_list;
  ban_list = ban_node;

  sprintf(buf, "%s запрещает %s для игроков %s.", GET_NAME(ch), site,
	  ban_types[ban_node->type]); /*has banned %s for %s players*/
  mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
  send_to_char(".Сайт забанен\r\n", ch); /*Site banned*/
  write_ban_list();
}


ACMD(do_unban)
{
  char site[MAX_INPUT_LENGTH];
  struct ban_list_element *ban_node, *temp;
  int found = 0;

  one_argument(argument, site);
  if (!*site) {
    send_to_char("Укажите, какой сайт Вы хотите разбанить.\r\n", ch); 
    return;
  }
  ban_node = ban_list;
  while (ban_node && !found) {
    if (!str_cmp(ban_node->site, site))
      found = 1;
    else
      ban_node = ban_node->next;
  }

  if (!found) {
    send_to_char("Сайт в настоящее время не забанен.\r\n", ch);
    return;
  }
  REMOVE_FROM_LIST(ban_node, ban_list, next);
  send_to_char("Сайт разбанен.\r\n", ch);
  sprintf(buf, "%s removed the %s-player ban on %s.",
	  GET_NAME(ch), ban_types[ban_node->type], ban_node->site);
  mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

  free(ban_node);
  write_ban_list();
}

/**************************************************************************
*         Code to chek for registered sites of clubs
*                Written by Natan
**************************************************************************/

const char *proxi_types[] = {
  "нет",
  "квартира",
  "клуб",
  "ERROR"
};


void load_proxi(void)
{
  FILE *fl;
  int i, date;
  char site_name[BANNED_SITE_LENGTH + 1], proxi_type[100];
  char name[MAX_NAME_LENGTH + 1];
  struct proxi_list_element *next_node;

  proxi_list = 0;

  if (!(fl = fopen(PROXI_FILE, "r"))) {
    if (errno != ENOENT) {
      log("SYSERR: Unable to open proxifile '%s': %s", BAN_FILE, strerror(errno));
    } else
      log("   Proxi file '%s' doesn't exist.", BAN_FILE);
    return;
  }
  while (fscanf(fl, " %s %s %d %s ", proxi_type, site_name, &date, name) == 4) {
    CREATE(next_node, struct proxi_list_element, 1);
    strncpy(next_node->site, site_name, BANNED_SITE_LENGTH);
    next_node->site[BANNED_SITE_LENGTH] = '\0';
    strncpy(next_node->name, name, MAX_NAME_LENGTH);
    next_node->name[MAX_NAME_LENGTH] = '\0';
    next_node->date = date;

    for (i = BAN_NOT; i < BAN_ALL; i++)
      if (!strcmp(proxi_type, proxi_types[i]))
	next_node->type = i;

    next_node->next = proxi_list;
    proxi_list = next_node;
  }

  fclose(fl);
}


int isproxi(char *hostname)
{
  int i=0;
  struct proxi_list_element *proxi_node;
  char *nextchar;

  if (!hostname || !*hostname)
    return (0);

  
  for (nextchar = hostname; *nextchar; nextchar++)
    *nextchar = LOWER((unsigned) *nextchar);

  for (proxi_node = proxi_list; proxi_node; proxi_node = proxi_node->next)
    if (strstr(hostname, proxi_node->site))	/* if hostname is a substring */
      i = MAX(i, proxi_node->type);

  return (i);
}


void _write_one_proxi(FILE * fp, struct proxi_list_element * node)
{
  if (node) {
    _write_one_proxi(fp, node->next);
    fprintf(fp, "%s %s %ld %s\n", proxi_types[node->type],
	    node->site, (long) node->date, node->name);
  }
}



void write_proxi_list(void)
{
  FILE *fl;

  if (!(fl = fopen(PROXI_FILE, "w"))) {
    perror("SYSERR: Unable to open '" PROXI_FILE "' for writing");
    return;
  }
  _write_one_proxi(fl, proxi_list);/* recursively write from end to start */
  fclose(fl);
  return;
}


ACMD(do_proxi)
{
  char flag[MAX_INPUT_LENGTH], site[MAX_INPUT_LENGTH],
	format[MAX_INPUT_LENGTH], *nextchar, *timestr;
  int i;
  struct proxi_list_element *proxi_node;

  *buf = '\0';

  if (!*argument) {
    if (!proxi_list) {
      send_to_char("Ни один сайт не зарегистрирован!\r\n", ch);
      return;
    }
    strcpy(format, "%-25.25s  %-8.8s  %-10.10s  %-16.16s\r\n");
    sprintf(buf, format,
	    "Имя сайта",
	    "Тип рег.",
	    "Время",
	    "Регистрировал");
    send_to_char(buf, ch);
    sprintf(buf, format,
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------");
    send_to_char(buf, ch);

    for (proxi_node = proxi_list; proxi_node; proxi_node = proxi_node->next) {
      if (proxi_node->date) {
	timestr = asctime(localtime(&(proxi_node->date)));
	*(timestr + 10) = 0;
	strcpy(site, timestr);
      } else
	strcpy(site, "Неизвестный");/*Unknown*/
      sprintf(buf, format, proxi_node->site, proxi_types[proxi_node->type], site,
	      proxi_node->name);
      send_to_char(buf, ch);
    }
    return;
  }
  two_arguments(argument, flag, site);
  if (!*site || !*flag) {
    send_to_char("Формат: зарегить {квартира | клуб} имя сайта\r\n", ch);
    return;
  }
  if (!(!str_cmp(flag, "квартира") || !str_cmp(flag, "клуб"))) {
    send_to_char("Можно выбрать опции: квартира или клуб.\r\n", ch);
    return;
  }
  for (proxi_node = proxi_list; proxi_node; proxi_node = proxi_node->next) {
    if (!str_cmp(proxi_node->site, site)) {
      send_to_char("Этот сайт уже зарегистрирован.\r\n", ch);
      return;
    }
  }

  CREATE(proxi_node, struct proxi_list_element, 1);
  strncpy(proxi_node->site, site, BANNED_SITE_LENGTH);
  for (nextchar = proxi_node->site; *nextchar; nextchar++)
    *nextchar = LOWER(*nextchar);
  proxi_node->site[BANNED_SITE_LENGTH] = '\0';
  strncpy(proxi_node->name, GET_NAME(ch), MAX_NAME_LENGTH);
  proxi_node->name[MAX_NAME_LENGTH] = '\0';
  proxi_node->date = time(0);

  for (i = BAN_NEW; i < BAN_ALL; i++)
    if (!str_cmp(flag, proxi_types[i]))
      proxi_node->type = i;

  proxi_node->next = proxi_list;
  proxi_list = proxi_node;

  sprintf(buf, "%s регистрирует сайт %s для игроков из %s%s.", GET_NAME(ch), site,
	  proxi_types[proxi_node->type], proxi_node->type ==2 ? "а" : "ы" );
  mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
  send_to_char(".Сайт зарегестрирован\r\n", ch);
  write_proxi_list();
}


ACMD(do_unproxi)
{
  char site[MAX_INPUT_LENGTH];
  struct proxi_list_element *proxi_node, *temp;
  int found = 0;

  one_argument(argument, site);
  if (!*site) {
    send_to_char("Укажите, с какого сайта Вы хотите снять регистрацию.\r\n", ch); 
    return;
  }
  proxi_node = proxi_list;
  while (proxi_node && !found) {
    if (!str_cmp(proxi_node->site, site))
      found = 1;
    else
      proxi_node = proxi_node->next;
  }

  if (!found) {
    send_to_char("Сайт в настоящее не зарегистрирован.\r\n", ch);
    return;
  }
  REMOVE_FROM_LIST(proxi_node, proxi_list, next);
  send_to_char("Регистрация с сайта снята.\r\n", ch);
  sprintf(buf, "%s removed the %s-player registered on %s.",
	  GET_NAME(ch), proxi_types[proxi_node->type], proxi_node->site);
  mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

  free(proxi_node);
  write_proxi_list();
}


/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

#define MAX_INVALID_NAMES	2000

char *invalid_list[MAX_INVALID_NAMES];int num_invalid = 0;
int Is_Valid_Name(char *newname)
{ int i;
  char tempname[MAX_INPUT_LENGTH];

  if (!invalid_list || num_invalid < 1)
     return (1);

  /* change to lowercase */
  strcpy(tempname, newname);
  for (i = 0; tempname[i]; i++)
      tempname[i] = LOWER(tempname[i]);

  /* Does the desired name contain a string in the invalid list? */
  for (i = 0; i < num_invalid; i++)
      if (!strncmp(tempname, invalid_list[i], 4))
	         return (0);

  return (1);
}

int Is_Valid_Dc(char *newname)
{ struct descriptor_data *dt;

  for (dt = descriptor_list; dt; dt = dt->next)
      if (dt->character && GET_NAME(dt->character) && 
         !str_cmp(GET_NAME(dt->character), newname)&&
         !IS_IMMORTAL(dt->character))
     return (STATE(dt) == CON_PLAYING);
       /* switch(STATE(dt))
	  { case CON_PLAYING:
	    return (0);
		break;
		case CON_MENU:
	    return (0);
	  break;
	  }  */
	 return (0);
}

int Valid_Name(char *newname)
{  
  return (Is_Valid_Name(newname) && !Is_Valid_Dc(newname));
}

void Read_Invalid_List(void)
{  FILE *fp;  char temp[256];
    
  if (!(fp = fopen(XNAME_FILE, "r")))
    { perror("SYSERR: Unable to open '" XNAME_FILE "' for reading");
      return;
    } 
    num_invalid = 0;
  while (get_line(fp, temp) && num_invalid < MAX_INVALID_NAMES) 
   invalid_list[num_invalid++] = str_dup(temp);     
    if (num_invalid >= MAX_INVALID_NAMES)
 {  
  log("SYSERR: Too many invalid names; change MAX_INVALID_NAMES in ban.c");
    exit(1); 
 }
  fclose(fp);
}

ACMD(do_bedname)
{ FILE *fp;
 int i=0, count=0;
  
 one_argument(argument, arg);

       if(!*arg)
		{ send_to_char("Напечатайте имя, которое будет запрещено или удалено из запрета.\r\n", ch);
		  return;
		}	  
switch (subcmd)
{  default:
		 break;
  case 0:
     
	  if (!Is_Valid_Name(arg))
	  {  send_to_char("Такое имя уже существует в файле badname.\r\n", ch);
		  break;
	  }
	  
	  if (num_invalid >= (MAX_INVALID_NAMES - 2))
		{  send_to_char("Файл badname переполнен.\r\n", ch);
		   break;
		}

          if (!(fp = fopen(XNAME_FILE, "a+"))) 
			{ sprintf(buf, "Ошибка: файл badname поврежден или не существует.\r\n");
			  mudlog(buf, NRM, LVL_GOD, TRUE);
			  break;
			}
		    num_invalid++;
            sprintf(buf, "Имен в файле badname: [%d], MAX количество [%d].\r\nИмя: &C%s - &Rдобавлено&n.\r\n", num_invalid, MAX_INVALID_NAMES, arg);
			send_to_char(buf, ch);
			fprintf(fp, "%s\n", arg);
 		    fclose(fp);
          Read_Invalid_List();
     break;
  case 1:
	  if (Is_Valid_Name(arg))
	  {   send_to_char("Такого имени нет в файле badname.\r\n", ch);
		  break;
	  }

   

	  if (!(fp = fopen(XNAME_FILE, "w"))) {
          perror("SYSERR: Unable to open '" XNAME_FILE "' for writing");
           return;
  }
  for (i = 0; i < num_invalid; i++)
  { if (!strncmp(invalid_list[i], arg, 4))
	{ count=i;
	  continue;
	}
  fprintf(fp, "%s\n", invalid_list[i]);
   
  }
	   fclose(fp);

	   if (count)
	   { num_invalid --;
	     sprintf(buf, "Имен в файле badname: [%d], MAX количество [%d].\r\nИмя: &C%s - &Rудалено&n. \r\n", num_invalid, MAX_INVALID_NAMES, invalid_list[count]);
		 send_to_char(buf, ch);
	   }
	   break;
 }
}
