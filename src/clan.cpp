/**************************************************************************
 * File: clan.c					   Part of Golden Dragon  *
 * Usage: This is the code for clans                                      *
 * By Andrey Gorbenko				  	                  *
 * Golden Dragon Copyright (C) 2004					  *
 * Version 3.1.6 Date 02 March 2006					  *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "clan.h"
#include "pk.h"
//#include <list>

using namespace std;
//using std::list;

//общее кол-во кланов.
//int num_of_clans;



//Глобальные структуры
extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int    top_of_p_table;


//функции
struct  char_data *is_playing(char *vict_name);
void    do_clan_news(struct char_data *ch, char *arg);
int     file_to_string(const char *name, char *buf);

//Класс выделяемый вектором
vector < ClanRec * > clan;

/***************************** Инициализация кланов*****************************************************************/

void send_clan_format(struct char_data *ch)
{
int c,r;

send_to_char("Клан-команды доступные для Вас:\n\r"
               "   клан инфо\r\n"
	       "   клан кто                       <имя_клана>\r\n",ch);
if(GET_LEVEL(ch)>=LVL_CLAN_GOD)
  send_to_char("   клан создать                   <имя_лидера> <имя_клана>\r\n"
               "   клан удалить                                <имя_клана>\r\n"
	       "   клан принять                   <имя_игрока> <имя_клана>\r\n"
               "   клан переименовать             <старое_имя> <новое_имя>\r\n"
               "   клан исключить                 <имя_игрока> <имя_клана>\r\n"
               "   клан повысить                  <имя_игрока> <имя_клана>\r\n"
               "   клан понизить                  <имя_игрока> <имя_клана>\r\n"
	       "   клан бонус                     <имя_игрока> <имя_клана>\r\n"
               "   клан снять                     <монет>      <имя_клана>\r\n"
               "   клан вложить                   <монет>      <имя_клана>\r\n"
               "   клан пклист    внести          <имя_игрока>\r\n"
	       "   клан пклист    удалить         <имя_игрока>\r\n"
               "   клан пклист    кто                          <имя клана>\r\n"
	       "   клан присвоить зона            <номер>      <имя_клана>\r\n"
	       "   клан присвоить ранг            <ранг>       <имя_клана>\r\n"
               "   клан присвоить регсбор         <монет>      <имя_клана>\r\n"
               "   клан присвоить слава           <слава>      <имя_клана>\r\n"
	       "   клан присвоить бонус           <бонус>      <имя_клана>\r\n"
	       "   клан присвоить взнос(ежемес)   <монет>      <имя_клана>\r\n"
               "   клан присвоить вступление      <уровень>    <имя_клана>\r\n"
               "   клан присвоить описание                     <имя_клана>\r\n"
               "   клан присвоить привилегия      <привилегия> <ранг> <имя_клана>\r\n"
               "   клан присвоить титул           <номер_клана><ранг> <титул>\r\n"
	       "   клан присвоить наклонность     <уровень>    <имя_клана>\r\n",ch);
else {
  c = find_clan_by_id(GET_CLAN(ch));  
  r = GET_CLAN_RANK(ch);
  if(r <1 )
  send_to_char("   клан обратиться                <имя_клана>|<отказаться>\r\n",ch);
  if(c >= 0) {
  send_to_char("   клан новости\r\n",ch);
  send_to_char("   клан статус\r\n",ch);
  send_to_char("   клан бонусы\r\n",ch);
  send_to_char("   клан инфо                      <имя_клана>\r\n",ch);
  send_to_char("   клан вложить	               <монет>\r\n",ch);
  send_to_char("   клан внести                    <слава>\r\n",ch);
  send_to_char("   клан пклист кто\r\n",ch);
    if(r >= clan[c]->privilege[CP_WITHDRAW])
  send_to_char("   клан снять                     <монет>\r\n" ,ch);
    if(r >= clan[c]->privilege[CP_ENROLL])
  send_to_char("   клан принять                   <имя_игрока>\r\n" ,ch);
    if(r >= clan[c]->privilege[CP_EXPEL])
  send_to_char("   клан исключить                 <имя_игрока>\r\n" ,ch);
    if(r >= clan[c]->privilege[CP_PROMOTE])
  send_to_char("   клан повысить                  <имя_игрока>\r\n",ch);
    if(r >= clan[c]->privilege[CP_DEMOTE])
  send_to_char("   клан понизить                  <имя_игрока>\r\n",ch);
    if(r >= clan[c]->privilege[CP_SET_APPLEV])
  send_to_char("   клан присвоить вступление      <уровень>\r\n",ch);
    if(r >= clan[c]->privilege[CP_SET_FEES])
  send_to_char("   клан присвоить регсбор         <монет>\r\n"
               "   клан присвоить взнос(ежемесяч) <монет>\r\n",ch);
    if(r >= clan[c]->privilege[CP_SET_PLAN])
  send_to_char("   клан присвоить описание\r\n",ch);
    if(r == clan[c]->ranks)
  send_to_char("   клан присвоить титул           <ранг>       <титул>\r\n"
               "   клан присвоить привилегия      <привилегия> <ранг>\r\n"
			   "   клан бонусы                    <имя_игрока>\r\n"
			   "   клан пклист    внести          <имя_игрока>\r\n"
			   "   клан пклист    удалить         <имя_игрока>\r\n", ch);
	}
  }
}




void do_clan_rename(struct char_data *ch, char *arg)
{
char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
int  clan_num=0;

	if (!*arg)
	{	send_clan_format(ch);
		return;
	}

	if (GET_LEVEL(ch) < LVL_CLAN_GOD)
	{	send_to_char("Вы не достаточно могущественны, чтобы переименовать клан!\r\n", ch);
		return;
	}

		half_chop(arg, arg1, arg2);

		if ((clan_num = ClanRec::find_clan(arg1)) < 0)
	{ send_to_char("Прежде чем переименовывать клан, его следовало бы создать!\r\n",ch);
		return;
	}

	if(strlen(arg2) >= 32)
	{ send_to_char("Слишком длинное название клана! (32 - символа максимально)\r\n",ch);
		return;
	}

	strncpy(clan[clan_num]->name, CAP((char *)arg2), (strlen(arg2) + 1));
	send_to_char("Клан успешно переименован!\r\n", ch);
}








/******************************************** ЛОАД И СОХРАНЕНИЕ КЛАН_ИНФЫ ********************************************/
void    Clan_fix(struct char_data *ch)
{
  // перс в клане. Нужно проверить есть ли он в списке соклановцев своего клана или нет
  // если его в списке нет, то добавляем. Это фикс бага с количеством членов в клане clans[]->members
  // чтобы правильно считалась мощь клана и число соклановцев
  sh_int index = find_clan_by_id(GET_CLAN(ch));
  int members = clan[index]->members;
  for (int i =0; i<members; ++i)
  {
      if (clan[index]->guests[i] == GET_IDNUM(ch))
          return;
  }
  clan[index]->guests[members] = GET_IDNUM(ch);
  clan[index]->members++;
  save_clans();
}

//Запись в клан_файл

void save_clans(void)
{
  FILE *fl;
  int i,j;
  char *name = NULL;

 CHAR_DATA *victim, *chdata;
 struct	Pk_Date *Pk;

  if (!(fl = fopen(CLAN_FILE, "w")))
     {perror("SYSERR: Unable to open house control file.");
      return;
     }
  for (i = 0; i < num_of_clans; i++)
  {fprintf(fl,"Name: %s\n", clan[i]->name);				//имя клана
   fprintf(fl,"Alig: %d\n", clan[i]->alignment);		//наклонность клана
   fprintf(fl,"Uniq: %d\n",clan[i]->id);				//Ид клана
   fprintf(fl,"Free: %d\n", clan[i]->app_fee);			//регистрационный сбор клана
   fprintf(fl,"Levl: %d\n", clan[i]->app_level);		//уровень вступления в клан
   fprintf(fl,"Glor: %ld\n", clan[i]->glory);			//слава клана
   fprintf(fl,"Trea: %ld\n", clan[i]->treasure);			//количество денег в клане
   fprintf(fl,"Dues: %ld\n",clan[i]->dues);				//ежемесячные сборы
   fprintf(fl,"BlTm: %ld\n", clan[i]->built_on);		//время создания клана
   fprintf(fl,"Zone: %d\n", clan[i]->zone);				//номер зоны клана

   for (Pk = clan[i]->CharPk; Pk; Pk = Pk->next)
	   fprintf(fl,"Pkud: %d %s\n", Pk->PkUid, Pk->Desc);
   
 /* for (j = 0; j < SPELL_CLANS; j++)
	 	 if (clan[i].spells[j])
   fprintf(fl,"Spel: %d\n", clan[i].spells[j]);	*/
    /* for (j = 0; j < SKILL_CLANS; j++)
	     if (clan[i].skills[j])
   fprintf(fl,"Skil: %d\n", clan[i].skills[j]); */
  	/* for (j = 0; j < CLAN_WARS; j++)
		 if (clan[i].at_war[j])
   fprintf(fl,"Warp: %ld (%s)\n", clan[i].at_war[j], clan[i].name);*/	//Кланы участвующие в боевых действиях
      
   for (j = 0; j < clan[i]->ranks; j++)
     fprintf(fl,"Rnam: %s\n",clan[i]->rank_name[j]);		//имя клан_ранга

   for (j = 0; j < NUM_CP; j++)
     fprintf(fl,"Priv: %d\n",clan[i]->privilege[j]);		//привелигерованные команды

  int Power = 0;
  int members = clan[i]->members;
  for (j = 0; j < members; j++)				                // идешники клановцев
  {
     name = get_name_by_id(clan[i]->guests[j]);
     if (name == NULL)
         continue;

     fprintf(fl,"Gues: %ld (%s)\n", clan[i]->guests[j], name);	 
	 if (victim = is_playing(name))
	    Power += IND_POWER_CHAR(victim);
	 else
     {
        CREATE(chdata, struct char_data, 1);
	    clear_char(chdata);
	    if (load_char(name, chdata) > -1)
          Power += IND_POWER_CHAR(chdata);
		free_char(chdata);
     }
   }

   //средняя клановая мощь
   if (members > 0)
      Power /= members;

   clan[i]->power = Power;
   fprintf(fl,"Powr: %ld\n", clan[i]->power);
   fprintf(fl,"~\n");
  }
   fprintf(fl,"$\n");
  fclose(fl);
}




/* Загрузка данных из клан_файла */
int Clan_load_data(FILE *fl)
{
  int i, a = 0, b = 0, c = 0, d = 0;
  char line[MAX_INPUT_LENGTH + 1], tag[6];
  bool yes = false;
  struct Pk_Date *Pk;
  
  //Чтение файла 
 /* while (get_line(fl, line))
  {	if (line[0] == '~') 
	 {  if (line[1] == '$')
	      return (true);
	   num_of_clans++;
	   return (true);
	 }    */

     
  while (get_line(fl, line))
  {	if (line[0] != '$') 
	 {  if (line[0] == '~')
		{  num_of_clans++;
	       return (true);
		}
	 }
    else 
	  return (true);


  if (!yes)
	{ //Добавляем в контейнер созданный клан
	  clan.push_back(new(ClanRec));
	  yes = true;
	}

 void	tag_argument(char *argument, char *tag);
  tag_argument(line, tag);
  
  for (i = 0; !(line[i] == '\0'); i++);
    line[i] = '\0';
    i++;
    
    if (!strcmp(tag, "Name")) 
	 strcpy(clan[num_of_clans]->name, line);
    else if (!strcmp(tag, "Alig"))
      clan[num_of_clans]->alignment	= atoi(line);
    else if (!strcmp(tag, "Uniq"))
      clan[num_of_clans]->id			= atol(line);
    else if (!strcmp(tag, "BlTm"))
      clan[num_of_clans]->built_on	= atol(line);
    else if (!strcmp(tag, "Free"))
      clan[num_of_clans]->app_fee	= atoi(line);
    else if (!strcmp(tag, "Levl"))
      clan[num_of_clans]->app_level	= atoi(line);
    else if (!strcmp(tag, "Glor"))
       clan[num_of_clans]->glory		= atoi(line);
    else if (!strcmp(tag, "Trea"))
      clan[num_of_clans]->treasure	= atoi(line);
	else if (!strcmp(tag, "Powr"))
      clan[num_of_clans]->power		= atol(line);
    else if (!strcmp(tag, "Dues"))
      clan[num_of_clans]->dues		= atoi(line);
	else if (!strcmp(tag, "Zone"))
      clan[num_of_clans]->zone		= atoi(line);
    else if (!strcmp(tag, "Gues")) 
    {
			  if (clan[num_of_clans]->members < MAX_MEMBERS)
			  { clan[num_of_clans]->guests[clan[num_of_clans]->members] = atol(line);//номера идешников игроков.
				clan[num_of_clans]->members++;
			  }
    }
    else if (!strcmp(tag, "Rnam")) 
			{ if (clan[num_of_clans]->ranks <= LIDER_RANKS)
				{ strcpy(clan[num_of_clans]->rank_name[clan[num_of_clans]->ranks], line);
	              clan[num_of_clans]->ranks++;
				}
			}
	else if (!strcmp(tag, "Priv")) 
			{ if (a < NUM_CP)
			  clan[num_of_clans]->privilege[a++] = atoi(line);
			}
	else if (!strcmp(tag, "Warp")) 
			{ if (b < CLAN_WARS)
			  clan[num_of_clans]->at_war[b++] = atol(line);
			}
	else if (!strcmp(tag, "Pkud"))
			{ char s[40], *s1, *Name;
			s1 = one_argument(line, s);
			skip_spaces(&s1);
			  if ((Name = get_name_by_unique(atoi(s))) != NULL)
				{ 	Pk							= new (struct Pk_Date);
			        Pk->PkUid					= atoi(s);
					Pk->Desc					= new char [strlen(s1 + 1)];
					strcpy(Pk->Desc, s1);
					Pk->next					= clan[num_of_clans]->CharPk;
					clan[num_of_clans]->CharPk	= Pk;
				}
			}
  }  
   
  return (false);
}






void init_clans(void)
{ 
  FILE *fl;
   
	if (!(fl = fopen(CLAN_FILE, "r")))
		{ log("   Clan file does not exist. Will create a new one");
			return;
		}


	while (Clan_load_data(fl));
	  
   

 fclose(fl);

save_clans();
}



/***************************************** КОНЕЦ СОХРАНЕНИЯ И ЛОАДА КЛАН_ИНФЫ ****************************************/


long clan_create_unique(void)
{int  i;
 long j;

 for(;TRUE;)
    {j =  (number(1,255) << 8) + number(1,255);
     for (i=0;i < num_of_clans;i++)
        if (HOUSE_ID(i) == j)
            break;
     if (i >= num_of_clans)
        return (j);
    }
 return (0);
}



void do_clan_create (struct char_data *ch, char *arg)
{

struct char_data *leader = NULL;
char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
int new_id=0;

	if (!*arg)
	{
	    send_clan_format(ch);
		return;
	}

	if (GET_LEVEL(ch) < LVL_CLAN_GOD)
	{ send_to_char("Вы не достаточно могущественны, чтобы создавать новые кланы!\r\n", ch);
		return;
	}

	if(num_of_clans == MAX_CLANS)
	{ send_to_char("Лимит создания кланов исчерпан. ВАУ!\r\n",ch);
		return;
	}

half_chop(arg, arg1, arg2);

	if(!(leader=get_char_vis(ch,arg1, FIND_CHAR_WORLD)))
	{ send_to_char("Лидер создаваемого клана должен присутствовать в игре.\r\n",ch);
		return;
	}

	if(strlen(arg2) > 19)
	{ send_to_char("Слишком длинное название клана! (19 - символа максимально)\r\n",ch);
		return;
	}

	if(GET_LEVEL(leader)>=LVL_IMMORT)
	{
		send_to_char("Бессмертные не могут быть лидерами кланов.\r\n",ch);
		return;
	}

if(GET_CLAN(leader)!=0 && GET_CLAN_RANK(leader)!=0)
	{ send_to_char("Назначаемый игрок принадлежит к одному из кланов!\r\n",ch);
      return;
	}

	if(ClanRec::find_clan(arg2)!=-1)
	{
		send_to_char("Такое название клана уже существует!\r\n",ch);
		return;
	}


clan.push_back (new (ClanRec));//дописать клансистему с использванием ВЕКТОРА или же СПИСКА


//clan[num_of_clans]->name			= new char[strlen(arg2 + 1)];
strcpy(clan[num_of_clans]->name,      CAP((char *)arg2));
clan[num_of_clans]->id				= clan_create_unique();
clan[num_of_clans]->ranks			= LIDER_RANKS;
strcpy(clan[num_of_clans]->rank_name[LIDER_RANKS - 1], "Лидер");
clan[num_of_clans]->members			= 1;
clan[num_of_clans]->app_level		= DEFAULT_APP_LVL ;
clan[num_of_clans]->built_on		= time(0);
clan[num_of_clans]->guests[0]		= GET_IDNUM(leader);

//Иднум этот находится в таблице player_table[i].id = GET_IDNUM(ch)
//имя чара можно получить по функции get_name_by_id

//ЗДЕСЬ ДОПИСАТЬ АВТОМАТИЧЕСКОЕ УДАЛЕНИЕ ИЗ ПК ЛИСТА ЛЮБОГО КЛАНА
//ЗА НЕИМЕНИЕМ ВРЕМЕНИ, ПОКА БУДУ УДАЛЯТЬ ИЗ ПК ЛИСТА ВРУЧНУЮ.


for ( int i = 0; i < NUM_CP; i++ )
  clan[num_of_clans]->privilege[i]=clan[num_of_clans]->ranks;

//for(i=0;i < CLAN_WARS;i++)
 // clan[num_of_clans].at_war[i]=num_of_clans;



	send_to_char("Клан успешно создан!\r\n", ch);

	GET_CLAN(leader)		= clan[num_of_clans]->id;
	GET_CLAN_RANK(leader)	= clan[num_of_clans]->ranks;

	save_char(leader, NOWHERE);

		num_of_clans++;

		save_clans();
return;
}

// ПЕреписать удаление кланов и чаров. 
//ВАЖНО!!!!!!!!! Для удаления, необходимо брать не весь список существующих в мире
//игроков, а только тех членов клана, которые хранятся в этом клане как участники
//это будет оптимизированней и быстрее, да и более правильно.
void do_clan_destroy(struct char_data *ch, char *arg, int destroy)
{ int i,j;
struct char_data *victim=NULL, *chdata;


	if (!*arg)
	{ send_clan_format(ch);
		return;
	}

	if ((i = ClanRec::find_clan(arg)) < 0)
	{ send_to_char("Неизвестный клан.\r\n", ch);
		return;
	}

	if(GET_LEVEL(ch)<LVL_CLAN_GOD && !destroy)
	{ send_to_char("Вы не достаточно могущественны, что бы уничтожить клан!\r\n", ch);
		return;
	}

	int rnum = real_room(3625);//родовой номер шаэрроведа постоя

	
	for (j = 0; j < clan[i]->members; j++)
		{ if (victim = is_playing(get_name_by_id(clan[i]->guests[j])))
			{ GET_CLAN(victim)=0;
			  GET_CLAN_RANK(victim)=0;
		      if (IN_ROOM(victim) == NOWHERE)
     	        char_to_room(victim, rnum);
	          GET_LOADROOM(victim) = rnum;
			  save_char(victim, NOWHERE);
              send_to_char ("\r\n&RВАШ КЛАН БЫЛ РАСПУЩЕН!&n\r\n",victim);
			}
		
		   else
			{ CREATE(chdata, struct char_data, 1);
			  clear_char(chdata);
			  if (load_char(get_name_by_id(clan[i]->guests[j]), chdata) > -1)
			  { GET_CLAN(chdata)		= 0;
                GET_CLAN_RANK(chdata)	= 0;
				GET_LOADROOM(chdata)	= rnum;
			    save_char(chdata, NOWHERE);
              }
			  free(chdata);
			}
		}
	
   
	//Удаление клана.
    delete clan[i];
    clan.erase(clan.begin() + i);

	num_of_clans--;
	send_to_char("Клан удален.\r\n", ch);
	save_clans();
return;

}

//принятие персонажа в клан
void do_clan_enroll(struct char_data *ch, char *arg)
{
struct char_data *vict=NULL;
int clan_num,immcom=0, immort=0;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];



	if (!(*arg))
	{ send_clan_format(ch);
	  return;
	}




	if(GET_LEVEL(ch)<LVL_IMMORT)
	{ if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0)
		{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
			return;
		}
	}
	else
	{ if(GET_LEVEL(ch)<LVL_CLAN_GOD)
		{ send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
			return;
		}
		immort=1;
		half_chop(arg,arg1,arg2);
		strcpy(arg,arg1);
		if ((clan_num = ClanRec::find_clan(arg2)) < 0)
		{ send_to_char("Неизвестный клан.\r\n", ch);
			return;
		}
	}

	if(GET_CLAN_RANK(ch) < clan[clan_num]->privilege[CP_ENROLL] && !immort)
	{ send_to_char("У Вас нет полномочий что бы это делать!\r\n",ch);
		return;
	}

	//что бы приняли в клан, нужно быть в одной комнате.
	if(!(vict=get_char_room_vis(ch,arg)))
		{ send_to_char("Что бы принять в клан, нужно быть в одной комнате с претендентом!\r\n",ch);
			return;
		}
	else
	{ if(GET_CLAN(vict)!=clan[clan_num]->id)
		{ if(GET_CLAN_RANK(vict)>0)
			{ send_to_char("Этот игрок уже состоит в другом клане.\r\n",ch);
				return;
			}
		else
			{ send_to_char("Этот игрок не просился в клан.\r\n",ch);
				return;
			}
		}
	  else
		if(GET_CLAN_RANK(vict)>0)
			{ send_to_char("Этот игрок уже и так в Вашем клане.\r\n",ch);
				return;
			}
 
		if (CALC_ALIGNMENT(vict) != clan[clan_num]->alignment && !immort) 
			{ sprintf(buf, "Вы не можете принять %s в свой клан из-за разной наклонности!\r\n", GET_VNAME(vict));
			  send_to_char(buf, ch);
				return;
			}

	
	if(GET_LEVEL(vict) >= LVL_IMMORT)
		{ send_to_char("Вы не можете принимать Бессмертных в свой клан.\r\n",ch);
		  return;
		}
	}

 
	GET_CLAN_RANK(vict)++;
	save_char(vict, vict->in_room);
	
	clan[clan_num]->guests[clan[clan_num]->members] = GET_IDNUM(vict);
	clan[clan_num]->members++;

	send_to_char("Вы были приняты в клан, который выбрали!\r\n",vict);
	send_to_char("Выполнено.\r\n",ch);

	save_clans();
    return;
}

//Исключение персонажа из клана
void do_clan_expel (struct char_data *ch, char *arg)
{
struct char_data *vict=NULL, *victim = NULL;
int clan_num,immcom=0, rnum;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];

	if (!(*arg))
	{ send_clan_format(ch);
		return;
	}

	if(GET_LEVEL(ch)<LVL_IMMORT)
	{ if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0)
		{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
			return;
		} 
	}
	else
	{ if(GET_LEVEL(ch)<LVL_CLAN_GOD)
		{ send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
			return;
		}
		immcom=1;
		half_chop(arg,arg1,arg2);
		strcpy(arg,arg1);
		if ((clan_num = ClanRec::find_clan(arg2)) < 0)
		{ send_to_char("Неизвестный клан.\r\n", ch);
			return;
		}
	}

	if(GET_CLAN_RANK(ch)<clan[clan_num]->privilege[CP_EXPEL] && !immcom)
		{ send_to_char("У Вас нет полномочий что бы это делать!\r\n",ch);
			return;
		}

	rnum = real_room(3625);//родовой номер шаэрроведа постоя
 
	if(!(vict=get_player_vis(ch, arg, FIND_CHAR_WORLD)))
		{ send_to_char("Такого персонажа нет в онлайн, ищу в оффлайне.\r\n",ch);
		  CREATE(victim, struct char_data, 1);
          clear_char(victim);
         if (load_char(arg, victim) > -1)
		 {  load_pkills(victim);
		    if(GET_CLAN(victim)!=clan[clan_num]->id)
			{ send_to_char("Этот игрок не состоит в Вашем клане.\r\n",ch);
			  free_char(victim);
               return;
			}
			if(GET_CLAN_RANK(victim)>=GET_CLAN_RANK(ch) && !immcom)
			{ send_to_char("Вы не можете исключить этого игрока из клана!\r\n",ch);
			  free_char(victim);
			  return; 
			}

		     if (!immcom)
				{ IND_POWER_CHAR(ch) -= (200 + MAX(0, GET_LEVEL(victim)-15)*20);
					send_to_char("За исключение игрока из клана, Ваша накопительная мощность снизилась.\r\n",ch);
				}

				GET_CLAN(victim)		= 0;
				GET_CLAN_RANK(victim)	= 0;
		    	GET_LOADROOM(victim)	= rnum;
				IND_POWER_CHAR(victim)	= MIN (POWER_STORE_CHAR(victim), IND_POWER_CHAR(victim));

                ClanRec *crec = clan[clan_num];
				for (int j = 0; j < crec->members; j++)				// идешники клановцев
					if (isname(GET_NAME(victim), get_name_by_id(crec->guests[j])))
                    {
                        int last = crec->members-1;
                        for (int k = j; k < last; k++)
                            crec->guests[k] = crec->guests[k+1];
                        crec->guests[last] = 0;
                        crec->members--;
                        break; 
                    }				
				save_char(victim, NOWHERE);	
				free_char(victim);
				send_to_char("Выполнено.\r\n",ch);
                save_clans();
				return;
		 } 
	    else
			{ send_to_char("В базе нет такого персонажа.\r\n", ch);
			  free(victim);
			}
	      return; 
		}
	else
	{ if(GET_CLAN(vict)!=clan[clan_num]->id)
		{ send_to_char("Этот игрок не состоит в Вашем клане.\r\n",ch);
			return; 
		}
	else
		{  if(GET_CLAN_RANK(vict)>=GET_CLAN_RANK(ch) && !immcom)
			{ send_to_char("Вы не можете исключить этого игрока из клана!\r\n",ch);
				return; 
			}
		} 
	}
  	
    GET_CLAN(vict)			= 0;
	GET_CLAN_RANK(vict)		= 0;
	IND_POWER_CHAR(vict)	= MIN (POWER_STORE_CHAR(vict), IND_POWER_CHAR(vict));
    
    ClanRec *crec = clan[clan_num];
	for (int j = 0; j < crec->members; j++)				// идешники клановцев
    if (isname(GET_NAME(vict), get_name_by_id(crec->guests[j])))
    {
        int last = crec->members-1;
        for (int k = j; k < last; k++)
          crec->guests[k] = crec->guests[k+1];
        crec->guests[last] = 0;
        crec->members--;
        break;
    }
	
	if (IN_ROOM(vict) != NOWHERE)
		 char_from_room(vict);
     
	char_to_room(vict, rnum);
	GET_LOADROOM(vict) = rnum;
	save_char(vict, NOWHERE);	
	send_to_char("Вас исключили из клана!\r\n",vict);
	send_to_char("Выполнено.\r\n",ch);
    save_clans();
    return;
}


//понижение ранга соклановца
void do_clan_demote (struct char_data *ch, char *arg)
{
struct char_data *vict=NULL;
int clan_num,immcom=0;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];

	
	if (!(*arg))
	{ send_clan_format(ch);
		return;
	}

	if(GET_LEVEL(ch)<LVL_IMMORT)
	{ if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0)
		{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
			return;
		}
	}
	else
	{ if(GET_LEVEL(ch)<LVL_CLAN_GOD)
		{ send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
			return;
		}

		immcom=1;
		half_chop(arg,arg1,arg2);
		strcpy(arg,arg1);
		if ((clan_num = ClanRec::find_clan(arg2)) < 0)
		{ send_to_char("Неизвестный клан.\r\n", ch);
			return; 
		}
	}

	if(GET_CLAN_RANK(ch) < clan[clan_num]->privilege[CP_DEMOTE] && !immcom)
	{ send_to_char("У Вас нет полномочий что бы это делать!!\r\n",ch);
		return;
	}

	if(!(vict=get_char_vis(ch, arg, FIND_CHAR_WORLD)))
	{ send_to_char("Это кто?\r\n",ch);
		return;
	}
	else
	{ if(GET_CLAN(vict)!=clan[clan_num]->id)
		{ send_to_char("Этот игрок не состоит в Вашем клане.\r\n",ch);
			return;
		}
	else 
	{ if(GET_CLAN_RANK(vict)==1)
		{ send_to_char("Ниже ранга в клане не бывает!\r\n",ch);
			return;
		}
    if(GET_CLAN_RANK(vict)>=GET_CLAN_RANK(ch) && !immcom)
		{ send_to_char("Вы не можете понизить ранг этого игрока!\r\n",ch);
			return;
		}
	}
	}

	GET_CLAN_RANK(vict)--;
	save_char(vict, vict->in_room);
	send_to_char("Вас понизили в ранге клана!\r\n",vict);
	send_to_char("Выполнено.\r\n",ch);
return;
}

//Повышение ранга соклановцу
void do_clan_promote (struct char_data *ch, char *arg)
{
struct char_data *vict=NULL;
int clan_num,immcom=0;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];



	if (!(*arg))
		{ send_clan_format(ch);
			return;
		}

	if(GET_LEVEL(ch) < LVL_CLAN_GOD) 
	{ if((clan_num = find_clan_by_id(GET_CLAN(ch))) < 0)
		{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
			return;
		}
	}
	else
	{	immcom=1;
		half_chop(arg,arg1,arg2);
		strcpy(arg,arg1);
		if ((clan_num = ClanRec::find_clan(arg2)) < 0)
		{ send_to_char("Неизвестный клан.\r\n", ch);
			return;
		}
	}

	if(GET_CLAN_RANK(ch) < clan[clan_num]->privilege[CP_PROMOTE] && !immcom)
		{ send_to_char("У Вас нет полномочий что бы это делать!!\r\n",ch);
			return;
		}

	if(!(vict=get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		{ send_to_char("Это кто?\r\n",ch);
			return;
		}
	else
		{ if(GET_CLAN(vict) != clan[clan_num]->id)
			{ send_to_char("Этот персонаж не состоит в Вашем клане.\r\n",ch);
				return;
			}
			else
		{ if(GET_CLAN_RANK(vict) == 0)
			{ send_to_char("Этот персонаж не принят в клане.\r\n",ch);
				return;
			}
		  if((GET_CLAN_RANK(vict)+1) >= GET_CLAN_RANK(ch) && !immcom)
			{ send_to_char("Вы не можете повысить ранг этому персонажу до своего!\r\n",ch);
				return;
			}
		  if(GET_CLAN_RANK(vict) == clan[clan_num]->ranks)
			{ send_to_char("Персонаж достиг максимального ранга для своего клана!\r\n",ch);
				return;
			}
		}
	}

	GET_CLAN_RANK(vict)++;
	save_char(vict, vict->in_room);
	send_to_char("Вы были повышены в ранге клана!\r\n",vict);
	send_to_char("Выполнено.\r\n",ch);
return;
}


void do_clan_who(struct char_data *ch,  char *arg)
{
 extern struct char_data *character_list;
 extern int top_of_p_table;
 extern struct player_index_element *player_table;
 struct char_data *victim;
  int  j, num = 0, clan_num=0;

if(!GET_CLAN_RANK(ch) && !IS_GRGOD(ch))
{  send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
  return;
}

*buf = '\0';
*buf1= '\0';
*buf2= '\0';

  if (!IS_GRGOD(ch))
  { if (!*arg || !arg)
	{ send_to_char("Укажите статус: &cонлайн&n или &rоффлайн&n.\r\n", ch);
      return;
	}
 for (j = 0; j <= top_of_p_table; j++)
	{  if((victim=is_playing((player_table +j)->name)))
	     {  if((clan_num = find_clan_by_id(GET_CLAN(victim)))<0)
			continue;
                if(!isname(arg, "онлайн"))
		        continue;
		 if(GET_CLAN(victim)==GET_CLAN(ch))
		 {  if (GET_CLAN_RANK(victim))
	          sprintf(buf + strlen(buf),"%-15s %s\r\n",GET_NAME(victim), clan[clan_num]->rank_name[GET_CLAN_RANK(victim)-1]);  
	        else 
		   { if (!IS_IMMORTAL(victim))
                     sprintf(buf1 + strlen(buf1),"%-15s %s\r\n",GET_NAME(victim), clan[clan_num]->rank_name[GET_CLAN_RANK(victim)-1]); 
		   }
		 }
		}
 }
                if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0)
			{  send_to_char("Вы не принадлежите ни к одному из кланов!\r\n", ch);
				return;
			}

	  for (j =0; j < clan[find_clan_by_id(GET_CLAN(ch))]->members; j++)
		  { if(!isname(arg, "оффлайн"))
		        continue;
		    if(!(victim=is_playing(get_name_by_id(clan[clan_num]->guests[j]))))
	         sprintf(buf2 + strlen(buf2),"%-15s\r\n",get_name_by_id(clan[clan_num]->guests[j]));
		  }
	 
	  

    if (*buf != 0)
	{ send_to_char("\r\n&WСписок членов клана: &Conline&n\r\n",ch);
	  send_to_char("---------------------------\r\n",ch);
      send_to_char(buf,ch);
	  
	  
	}
    if (*buf2 != 0)
	{ send_to_char("\r\n&WСписок членов клана: &Roffline&n\r\n",ch);
	  send_to_char("----------------------------\r\n",ch);
      send_to_char(buf2,ch);
	} 

	if (*buf1 != 0)
	{ send_to_char("\r\n&gСписок обращенных к клану&n\r\n",ch);
	  send_to_char("-------------------------\r\n",ch);
      send_to_char(buf1,ch);
	}
return;
  }
 
  if((clan_num = ClanRec::find_clan(arg))<0)
       {send_to_char("Такого клана не существует.\r\n",ch);
        return;
	   }
    
	   for (j = 0; j <= top_of_p_table; j++)
	   if((victim=is_playing((player_table +j)->name)))
		{  if(find_clan_by_id(GET_CLAN(victim))==clan_num)
			 sprintf(buf + strlen(buf),"%-15s %s\r\n",GET_NAME(victim), clan[clan_num]->rank_name[GET_CLAN_RANK(victim)-1]);  
	    }  

 	
          for (j =0; j < clan[clan_num]->members; j++)
		   if(!(victim=is_playing(get_name_by_id(clan[clan_num]->guests[j]))))
	         sprintf(buf2 + strlen(buf2),"%-15s\r\n",get_name_by_id(clan[clan_num]->guests[j]));
		  

    if (*buf != 0)
	{ send_to_char("\r\n&WСписок членов клана &Conline&n\r\n",ch);
	  send_to_char("--------------------------\r\n",ch);
      send_to_char(buf,ch);
	}

    if (*buf2 != 0)
	{ send_to_char("\r\n&WСписок членов клана &Roffline&n\r\n",ch);
	  send_to_char("---------------------------\r\n",ch);
      send_to_char(buf2,ch);
	} 
}



//Статус, обращен не обращен, ранг в клане
void do_clan_status (struct char_data *ch)
{
char line_disp[90];

int clan_num;

	if(GET_LEVEL(ch)>=LVL_IMMORT)
	{ send_to_char("Вы Бессмертны и не можете присоединятся к кланам!\r\n",ch);
		return; 
	}

	if((clan_num=find_clan_by_id(GET_CLAN(ch))) <0 )
	{ send_to_char("Вы не принадлежите к клану!\r\n",ch);
		return;
	}	

	if(GET_CLAN_RANK(ch)==0)
		if(clan_num>=0)
		{ sprintf(line_disp,"Ваш статус: Обращение к клану &c[%s]&n\r\n",clan[clan_num]->name);
		  send_to_char(line_disp,ch);
			return;
		}
	else 
	{ send_to_char("Вы не принадлежите к клану!\r\n",ch);
		return;
	}

	sprintf(line_disp,"Вы %s %d ранга в клане &c[%s]&n [ID %d]\r\n",
    clan[clan_num]->rank_name[GET_CLAN_RANK(ch)-1], GET_CLAN_RANK(ch),
    clan[clan_num]->name, clan[clan_num]->id);
	send_to_char(line_disp,ch);

return;
}

//Обращение в клан или отказ.
void do_clan_apply (struct char_data *ch, char *arg)
{
int clan_num;

	if (!(*arg))
	{ send_clan_format(ch);
		return;
	}

	if(GET_LEVEL(ch) >= LVL_IMMORT)
	{ send_to_char("Бессмертные не могут проситься в кланы.\r\n",ch);
		return;
	}

	if(GET_CLAN_RANK(ch) > 0)
	{ send_to_char("Вы уже принадлежите к клану!\r\n",ch);
		return;
	}
	else
	{  if (!str_cmp(arg, "отказаться") && GET_CLAN(ch))
		{ send_to_char("Вы отказались от вступления в клан!\r\n",ch);
			GET_CLAN(ch) = 0;
			return;
		}
	if ((clan_num = ClanRec::find_clan(arg)) < 0)
		{ send_to_char("Неизвестный клан.\r\n", ch);
			return;
		} 
	}

  if(CALC_ALIGNMENT(ch) != clan[clan_num]->alignment)
	{ send_to_char("Ваша наклонность не позволяет вступить в этот клан.\r\n",ch);
      return;
	}

  if(GET_LEVEL(ch) < clan[clan_num]->app_level) {
     send_to_char("Вы не достигли достаточного уровня, чтобы вступить в этот клан.\r\n",ch);
     return; }

   if (GET_CLAN(ch) == clan[clan_num]->id)
	{ send_to_char("Вы уже обращены в этот клан.\r\n",ch);
      return;
	}

  if(GET_GOLD(ch) < clan[clan_num]->app_fee)
	{ send_to_char("Вы не можете делать регистрационный сбор!\r\n", ch);
	 return;
	}

  GET_GOLD(ch) -= clan[clan_num]->app_fee;
  clan[clan_num]->treasure += clan[clan_num]->app_fee;
  GET_CLAN(ch) = clan[clan_num]->id;
  save_char(ch, ch->in_room);
  send_to_char("Вы обратились к клану.\r\n",ch);

return;
}


//Управление банком клана
void do_clan_bank(struct char_data *ch, char *arg, int action)
{
int clan_num,immcom=0;
long amount=0;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];

if (!(*arg)) {
  send_clan_format(ch);
  return; }

if(GET_LEVEL(ch)<LVL_IMMORT) {
  if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0) {
    send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
    return;
  }
}
else {
  if(GET_LEVEL(ch)<LVL_CLAN_GOD) {
    send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
    return; }
  immcom=1;
  half_chop(arg,arg1,arg2);
  strcpy(arg,arg1);
  if ((clan_num = ClanRec::find_clan(arg2)) < 0) {
    send_to_char("Неизвестный клан.\r\n", ch);
    return;
  }
}

if((GET_CLAN_RANK(ch)<clan[clan_num]->privilege[CP_WITHDRAW] && !immcom && action==CB_WITHDRAW) ||
   (GET_CLAN_RANK(ch) != clan[clan_num]->ranks && !immcom && action==CM_GLORY))
	{  send_to_char("У Вас нет полномочий что бы это делать!!\r\n",ch);
	   return;
	}

if(!(*arg))
  { send_to_char("Отложено сколько?\r\n",ch);
    return;
  }

if(!is_number(arg)) {
  send_to_char("Отложено чего?\r\n",ch);
  return;
  }

amount=atoi(arg);

if(!immcom && action==CB_DEPOSIT && GET_GOLD(ch) < amount) {
  send_to_char("Вы не имеете таких денег!\r\n",ch);
  return;
  }

if(action==CB_WITHDRAW && clan[clan_num]->treasure < amount) {
  send_to_char("У Вашего клана нет таких средств!\r\n",ch);
  return;
  }

switch(action) {
  case CB_WITHDRAW:
    GET_GOLD(ch)			 += amount;
    clan[clan_num]->treasure -= amount;
    sprintf(buf, "Вы сняли со счета клана %ld %s.\r\n", amount, desc_count(amount, WHAT_MONEYu));
	send_to_char(buf,ch);
    break;
  case CB_DEPOSIT:
    if(!immcom) GET_GOLD(ch) -= amount;
    clan[clan_num]->treasure += amount;
    sprintf(buf, "Вы добавили на счет клана %ld %s.\r\n", amount, desc_count(amount, WHAT_MONEYu));
	send_to_char(buf,ch);
    break;
 case CM_GLORY:
    if (!GET_CLAN_RANK(ch))
send_to_char("Прежде чем производить операции со славой, Вас должны принять в клан.\r\n",ch);
	 if (!amount)
	  {	send_to_char("Вы шутник однако!\r\n",ch);
		break;
	  }
	 if (GET_GLORY(ch) >= amount)
	{clan[clan_num]->glory = MAX(0,clan[clan_num]->glory + amount);
     sprintf(buf,"Вы пожертвовали на счет клана %ld %s славы.\r\n",
                 amount, desc_count(amount, WHAT_POINT));
     send_to_char(buf,ch);
	 GET_GLORY(ch) -=amount;
	}
	else
      send_to_char("Вы сначала заслужите столько славы!\r\n",ch);
	break;
  default:
    send_to_char("Проблема с обработкой команды, доложите!\r\n",ch);
    break;
  }
save_char(ch, ch->in_room);
return;
}



void do_clan_money(struct char_data *ch, char *arg, int action)
{
int clan_num,immcom=0;
long amount=0;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];

if (!(*arg)) {
  send_clan_format(ch);
  return; }

if(GET_LEVEL(ch)<LVL_IMMORT) {
  if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0) {
    send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
    return;
  }
}
else {
  if(GET_LEVEL(ch)<LVL_CLAN_GOD) {
    send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
    return; }
  immcom=1;
  half_chop(arg,arg1,arg2);
  strcpy(arg,arg1);
  if ((clan_num = ClanRec::find_clan(arg2)) < 0) {
    send_to_char("Неизвестный клан.\r\n", ch);
    return;
  }
}

if(GET_CLAN_RANK(ch)<clan[clan_num]->privilege[CP_SET_FEES] && !immcom) {
  send_to_char("У Вас нет полномочий что бы это делать!!\r\n",ch);
  return;
  }

if(!(*arg)) {
  send_to_char("Присвоить сколько?\r\n",ch);
  return;
  }

if(!is_number(arg) && !(*arg == '-' || *arg == '+')) {
  send_to_char("Присвоить чего?\r\n",ch);
  return;
  }

amount=atoi(arg);

switch(action) {
  case CM_APPFEE:
    clan[clan_num]->app_fee=amount;
    send_to_char("Вы изменили регистрационный сбор.\r\n",ch);
    break;
  case CM_DUES:
    clan[clan_num]->dues=amount;
    send_to_char("Вы изменили ежемесячные сборы.\r\n",ch);
    break;
  case CM_GLORY:
   	  if (*arg == '-' ||  *arg == '+')
           clan[clan_num]->glory = MAX(0,clan[clan_num]->glory + amount);
        else
           clan[clan_num]->glory = amount;
        sprintf(buf,"Количество славы установлено клану &c[%s]&n в %ld %s.\r\n",
                 clan[clan_num]->name, clan[clan_num]->glory,
				 desc_count(clan[clan_num]->glory, WHAT_POINT));
    
    send_to_char(buf,ch);
	break;
  default:
    send_to_char("Неизвестная команда, пожалуйста сообщите.\r\n",ch);
    break;
  }
return;
}




void do_clan_titles( struct char_data *ch, char *arg)
{ char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int clan_num=0,rank;

if (!(*arg))
	{ send_clan_format(ch);
      return;
	}

if(GET_LEVEL(ch)<LVL_IMMORT) {
  if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0) {
    send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
    return;
  }
  if(GET_CLAN_RANK(ch)!=clan[clan_num]->ranks) {
    send_to_char("У Вас нет достаточных полномочий, что бы это сделать!\r\n",ch);
    return;
  }
}
else {
  if(GET_LEVEL(ch)<LVL_CLAN_GOD) {
    send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
    return; }
  half_chop(arg,arg1,arg2);
  strcpy(arg,arg2);
  if(!is_number(arg1)) {
    send_to_char("Вы должны определить номер клана.\r\n",ch);
    return;
  }
  if((clan_num=atoi(arg1))<0 || clan_num>=num_of_clans) {
    send_to_char("Нет клана с таким номером.\r\n",ch);
    return;
  }
}

half_chop(arg,arg1,arg2);

	if(!is_number(arg1))
	{ send_to_char("Вы должны определить номер ранга.\r\n",ch);
		return;
	}

     rank=atoi(arg1);

	if(rank < 1 || rank > clan[clan_num]->ranks)
	{ send_to_char("Этот клан не имеет такого номера ранга.\r\n",ch);
		return;
	}

	if(strlen(arg2) < 1 || strlen(arg2) > 14)
	{ send_to_char("Название ранга не должно иметь более 14 символов.\r\n",ch);
		return;
	}

	strcpy(clan[clan_num]->rank_name[rank-1],arg2);
	send_to_char("Выполнено.\r\n",ch);

return;
}

void do_clan_application( struct char_data *ch, char *arg, int mode)
{
int clan_num,immcom=0;
int applevel;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];

if (!(*arg)) {
  send_clan_format(ch);
  return; }

if(GET_LEVEL(ch)<LVL_IMMORT) {
  if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0) {
    send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
    return;
  }
}
else {
  if(GET_LEVEL(ch)<LVL_CLAN_GOD) {
    send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
    return; }
  immcom=1;
  half_chop(arg,arg1,arg2);
  strcpy(arg,arg1);
  if ((clan_num = ClanRec::find_clan(arg2)) < 0) {
    send_to_char("Неизвестный клан.\r\n", ch);
    return;
  }
}

if(GET_CLAN_RANK(ch)<clan[clan_num]->privilege[CP_SET_APPLEV] && !immcom && !mode) {
  send_to_char("У Вас нет полномочий что бы это делать!!\r\n",ch);
  return;
  }

if(!(*arg)) {
  send_to_char("Что Вы хотите присвоить?\r\n",ch);
  return;
  }

if(!is_number(arg)) {
  send_to_char("Какой уровень Вы хотите присовить?\r\n",ch);
  return;
  }

applevel=atoi(arg);

if((applevel < 1 || applevel > 46) && !mode) {
  send_to_char("Уровень вступления в клан может составлять от 1 до 45.\r\n",ch);
  return;
  }
if((applevel < 0 || applevel > 2) && mode) {
  send_to_char("Значения могут быть: Светлый - 0, Нейтральный -1, Темный -2.\r\n",ch);
  return;
  }

   if (!mode)
     clan[clan_num]->app_level = applevel;
   else 
     clan[clan_num]->alignment = applevel;
send_to_char("Установлено.\r\n",ch);
return;
}


void do_clan_sp(struct char_data *ch, char *arg, int priv)
{
int clan_num,immcom=0;
int rank;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];

	if (!(*arg))
	{ send_clan_format(ch);
		return;
	}


	if(GET_LEVEL(ch)<LVL_IMMORT)
	{ if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0)
		{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
			return;
		}
	}
	else
	{ if(GET_LEVEL(ch)<LVL_CLAN_GOD)
		{ send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
			return;
		}

	immcom=1;
	half_chop(arg,arg1,arg2);
	strcpy(arg,arg1);

		if ((clan_num = ClanRec::find_clan(arg2)) < 0)
		{ send_to_char("Неизвестный клан.\r\n", ch);
			return;
		}
	}

	if(GET_CLAN_RANK(ch)!=clan[clan_num]->ranks && !immcom)
	{ send_to_char("У Вас нет достаточных полномочий, что бы это сделать!\r\n",ch);
		return;
	}

	if(!(*arg))
	{ send_to_char("Присвоить привилегии к какому рангу?\r\n",ch);
		return;
	}

	if(!is_number(arg))
	{ send_to_char("Сколько Вы хотите присвоить привилегий?\r\n",ch);
		return;
	}

	rank = atoi(arg);

	if(rank < 1 || rank > clan[clan_num]->ranks)
	{ send_to_char("Такого ранга в клане не существует.\r\n",ch);
		return;
	}

	clan[clan_num]->privilege[priv] = rank;
	send_to_char("Присвоено.\r\n",ch);
	return;
}

void do_clan_plan(struct char_data *ch, char *arg)
{
int clan_num;

send_to_char("Команда в стадии доработки.\r\n",ch);
return;

if(GET_LEVEL(ch)<LVL_IMMORT) {
  if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0) {
    send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
    return; }
  if(GET_CLAN_RANK(ch)<clan[clan_num]->privilege[CP_SET_PLAN]) {
    send_to_char("У Вас нет полномочий что бы это делать!\r\n",ch);
    return; }
}
else {
  if(GET_LEVEL(ch)<LVL_CLAN_GOD) {
    send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
    return; }
  if (!(*arg)) {
    send_clan_format(ch);
    return; }
  if ((clan_num = ClanRec::find_clan(arg)) < 0) {
    send_to_char("Неизвестный клан.\r\n", ch);
    return; }
}


if(strlen(clan[clan_num]->description)==0) {
   sprintf(buf, "Введите описание клана <<%s>>.\r\n",clan[clan_num]->name);
   send_to_char(buf, ch);
}
else {
   sprintf(buf, "Старое описание клана <<%s>>:\r\n", clan[clan_num]->name);
   send_to_char(buf, ch);
   send_to_char(clan[clan_num]->description, ch);
   send_to_char("Введине новое описание:\r\n", ch);
}
send_to_char("Чтобы записать напечатайте @ и закончите строку ENTER.\r\n", ch);
//разобраться
*ch->desc->str   = clan[clan_num]->description;
ch->desc->max_str = CLAN_PLAN_LENGTH;
return;
}

void do_clan_zone(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
  int clan_num=0, zone=0;
  
  if(GET_LEVEL(ch) < LVL_CLAN_GOD)
	{ send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
      return;
	}

   if (!(*arg))
   { send_to_char("Укажите номер зоны и имя клана для назначения.\r\n", ch);
     return;
   }
  
   half_chop(arg,arg1,arg2);
   strcpy(arg,arg1);
   
   if((clan_num = ClanRec::find_clan(arg2)) < 0)
	{  send_to_char("Неизвестный клан.\r\n", ch);
      return;
	}
 
   if(!(*arg))
	{ send_to_char("Какую зону Вы хотите присвоить клану?\r\n",ch);
      return;
	}
  
   if(!is_number(arg))
	{ send_to_char("Небходимо указать числовой аргумент.\r\n",ch);
      return;
	}

    zone=atoi(arg); 

	clan[clan_num]->zone = zone;
    send_to_char("Выполнено.\r\n",ch);
	
return;
}


void do_clan_privilege( struct char_data *ch, char *arg)
{
char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
int i;

static const char *clan_privileges[NUM_CP+1] =
{"регсбор","принять","исключить","повысить","понизить","взнос","снять","обратиться", "пклист", "новости"};

half_chop(arg,arg1,arg2);

if (is_abbrev(arg1,"регсбор"  )) { do_clan_sp(ch,arg2,CP_SET_PLAN);   return ;}
if (is_abbrev(arg1,"принять  ")) { do_clan_sp(ch,arg2,CP_ENROLL);     return ;}
if (is_abbrev(arg1,"исключить")) { do_clan_sp(ch,arg2,CP_EXPEL);      return ;}
if (is_abbrev(arg1,"повысить" )) { do_clan_sp(ch,arg2,CP_PROMOTE);    return ;}
if (is_abbrev(arg1,"понизить" )) { do_clan_sp(ch,arg2,CP_DEMOTE);     return ;}
if (is_abbrev(arg1,"снять"    )) { do_clan_sp(ch,arg2,CP_WITHDRAW);   return ;}
if (is_abbrev(arg1,"взнос"    )) { do_clan_sp(ch,arg2,CP_SET_FEES);   return ;}
if (is_abbrev(arg1,"обратиться")){ do_clan_sp(ch,arg2,CP_SET_APPLEV); return ;}
if (is_abbrev(arg1,"пклист"   )) { do_clan_sp(ch,arg2,CP_PKLIST);	  return ;}
if (is_abbrev(arg1,"новости"  )) { do_clan_sp(ch,arg2,CP_NEWS);		  return ;}
send_to_char("Привилегии Клана:\r\n", ch);

	for(i=0;i<NUM_CP;i++)
	{ sprintf(arg1,"\t%s\r\n",clan_privileges[i]);
		send_to_char(arg1,ch);
	}
}



void do_clan_all(struct char_data *ch, char *arg, int mode)
{  struct Pk_Date *Pk;
   int i = 0, clan_num =0, immcom = 0;
   char *nme = NULL;
   *buf = '\0';

  
	if (GET_LEVEL(ch) < LVL_IMMORT)
		{   if ((clan_num = find_clan_by_id(GET_CLAN(ch))) < 0)
			{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
				return;
			}
		}
	else
	{ if (GET_LEVEL(ch) < LVL_CLAN_GOD)
		{ send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
			return;
		}
   
	  if ((clan_num = ClanRec::find_clan(arg)) < 0)
		{ send_to_char("Неизвестный клан.\r\n", ch);
			return;
		}
	}


   switch(mode)
	{ case 2:
	
		for (Pk = clan[clan_num]->CharPk; Pk; Pk = Pk->next)
		{	if ((nme = get_name_by_unique(Pk->PkUid)) != NULL)
				{ sprintf(buf + strlen(buf), "%-10s - %s\r\n", CAP(nme), Pk->Desc);
				  immcom=1;		
				}
		}
		if (immcom)
		{send_to_char("&cИгроки, занесенные в ПКЛИСТ Вашего клана:&n\r\n",ch);
		 send_to_char("&R-----------------------------------------&n\r\n",ch);
		 send_to_char(buf,ch);
		 send_to_char("&R-----------------------------------------&n\r\n",ch);
		}
		else
         send_to_char("&cВ ПКЛИСТе Вашего клана никого нет -(&n\r\n",ch);
		break;
    default:
	 break;
	}
}





//Добавляем в ПК-лист Чара.
void do_clan_PKset(struct char_data *ch, char *arg, int mode)
{

struct char_data *vict = NULL;
struct Pk_Date *Pk;
int clan_num,immcom=0, i=0, ex=0;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];
	
	if (!(*arg))
	{ send_clan_format(ch);
		return;
	}

		half_chop(arg,arg1,arg2);
		   strcpy(arg,arg1);

     if (strlen(arg2) > 28)
		{ send_to_char("Длинна сообщения за что персонаж в ПК листе, не может превышать 28 символов.\r\n",ch);
			return; 
		}

	if(GET_LEVEL(ch)<LVL_IMMORT)
	{ if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0)
		{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
			return;
		}
	}
	else
	{ if(GET_LEVEL(ch)<LVL_CLAN_GOD)
		{ send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
			return;
	}
    

	immcom=1;
	if ((clan_num = ClanRec::find_clan(arg2)) < 0) {
    send_to_char("Неизвестный клан.\r\n", ch);
    return;
  }
}

	if(!immcom && GET_CLAN_RANK(ch)<clan[clan_num]->privilege[CP_PKLIST])
	{ send_to_char("У Вас нет полномочий что бы это делать!\r\n",ch);
      return;
	}
        
	if(!(vict=get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		{  send_to_char("Нет такого игрока.\r\n",ch);
		   return;
		}

	if (IS_NPC(vict))
		{ send_to_char("Вы не можете это сделать по отношению к мобу.\r\n",ch);
		  return;
		}
 
    if (ch == vict)
		{  send_to_char("Подумайте, стоит ли это делать?\r\n",ch);
			return;
		}
	if (IS_IMMORTAL(vict))
		{  send_to_char("За такие шуточки можно и клана лишиться!\r\n",ch);
			return;
		}
	

  switch (mode)
  { case 0:

  if(find_clan_by_id(GET_CLAN(vict)) >= 0 && GET_CLAN_RANK(vict) > 0)
		{ send_to_char("Вы не можете занести в ПКЛИСТ клановца.\r\n",ch);
           return;
		}

        
		for (Pk = clan[clan_num]->CharPk; Pk; Pk = Pk->next)
			if (Pk->PkUid == GET_UNIQUE(vict))
				{ send_to_char("Этот персонаж уже находится в Вашем ПКЛИСТе.\r\n",ch);
					return;
				}
//выделяем память для структуры хранения занесенных в ПК-лист чаров		
	Pk = new (struct Pk_Date);

// Выделяем память для описания, за что внесен в ПК_лист.	
	Pk->PkUid				= GET_UNIQUE(vict);
	Pk->Desc				= new char [strlen(arg2 + 1)];
	strcpy(Pk->Desc, arg2);
	Pk->next				= clan[clan_num]->CharPk;
	clan[clan_num]->CharPk	= Pk;

	
     if (!immcom)
    IND_POWER_CHAR(ch) -= 2000;//штраф за запись в пк лист, что бы не злоупотреблял.

    sprintf(buf, "Вы были занесены в &RПКЛИСТ&n клана &c[%s]\r\n", clan[clan_num]->name);
	send_to_char(buf,vict);
	sprintf(buf, "Вы занесли в &CПКЛИСТ&n клана %s\r\n", GET_VNAME(vict)); 
	send_to_char(buf,ch);
	break;
    
  case 1:
		for (Pk = clan[clan_num]->CharPk; Pk; Pk = Pk->next)
			if (Pk->PkUid == GET_UNIQUE(vict))
			{	sprintf(buf, "Вас удалили из ПКЛИСТа клана &c[%s]\r\n", clan[clan_num]->name);
				send_to_char(buf,vict);
				sprintf(buf, "Вы удалили из ПКЛИСТа клана %s\r\n", GET_VNAME(vict));
				send_to_char(buf,ch);
			    Pk = clan[clan_num]->CharPk->next;
				delete  clan[clan_num]->CharPk;
				clan[clan_num]->CharPk = Pk;
				ex =1;
				break;
			}
  
  
	if(!ex)
	send_to_char("Прежде чем удалять игрока из ПКЛИСТа, его необходимо туда поместить!\r\n",ch);
	
	  break;
	default:
     break;
  }
return;
}







void do_clan_setPklist(struct char_data *ch, char *arg)
{
char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];

half_chop(arg,arg1,arg2);
if (is_abbrev(arg1, "внести"   )) { do_clan_PKset(ch,arg2,0);  return ;
}
if (is_abbrev(arg1, "удалить"  )) { do_clan_PKset(ch,arg2,1);  return ;
}
if (is_abbrev(arg1, "кто"      )) { do_clan_all(ch,arg2,2);    return ;
}


}


void do_clan_set(struct char_data *ch, char *arg)
{
char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];

half_chop(arg,arg1,arg2);

if (is_abbrev(arg1, "описание"   )) { do_clan_plan(ch,arg2);            return ;
}
if (is_abbrev(arg1, "титул"      )) { do_clan_titles(ch,arg2);          return ;
}
if (is_abbrev(arg1, "привилегия" )) { do_clan_privilege(ch,arg2);       return ;
}
if (is_abbrev(arg1, "взнос"      )) { do_clan_money(ch,arg2,CM_DUES);   return ;
}
if (is_abbrev(arg1, "регсбор"    )) { do_clan_money(ch,arg2,CM_APPFEE); return ;
}
if (is_abbrev(arg1, "слава"      )) { do_clan_money(ch,arg2,CM_GLORY);	return ;
}
if (is_abbrev(arg1, "вступление" )) { do_clan_application(ch,arg2,0);   return ;
}
if (is_abbrev(arg1, "наклонность")) { do_clan_application(ch,arg2,1);   return ;
}
if (is_abbrev(arg1, "зона" ))		{ do_clan_zone(ch,arg2);			return ;
}
/*if (is_abbrev(arg1, "война"))		{ do_clan_war(ch,arg2);				return ;
}
if (is_abbrev(arg1, "нейтралитет"   )) { do_clan_neu(ch,arg2);          return ;
}
if (is_abbrev(arg1, "альянс"   ))    { do_clan_group(ch,arg2);          return ;
}*/

send_clan_format(ch);
}


void do_clan_info (struct char_data *ch, char *arg)
{
int i=0;
char blt[50], blt1[50], blt2[50], built[50];
char *timestr;
const char *alignment[] = {"Светлый","Нейтральный","Темный"};


if(num_of_clans == 0) {
  send_to_char("В данное время кланы не созданы.\r\n",ch);
  return; }





if(!(*arg) || (!IS_GOD(ch) && find_clan_by_id(GET_CLAN(ch))<0)) {
  sprintf(buf, "\r");
  for(i=0; i < num_of_clans; i++)
    sprintf(buf, "%s[%-3d]  %-20s Участников: %-3d Мощь: %-9d Наклонность: %s\r\n",buf
 , i, clan[i]->name,clan[i]->members, clan[i]->power, alignment[clan[i]->alignment]);
  page_string(ch->desc,buf, 1);
  return; }

  else
  
   
	  if ((i =  ClanRec::find_clan(arg))< 0)
	  {send_to_char("Неизвестный клан.\r\n", ch);
   	       return;
	  }
	  
   else 
	 if (!IS_GOD(ch))
     {  if (find_clan_by_id(GET_CLAN(ch)) != i || !GET_CLAN_RANK(ch))  
		{ send_to_char("Вы не можете посмотреть информацию другого клана!\r\n", ch);
	      return;
		} 
	      i = find_clan_by_id(GET_CLAN(ch));	
	 }
//j = 1101799462;- 30 ноября
//	 j = 1029799462;20 августа 2002
 if (clan[i]->built_on)
          {timestr = asctime(localtime(&(clan[i]->built_on)));
       		  strcpy(blt1,(timestr + 8));
              *(blt1+2) = '\0';  
			  strcpy(blt2,(timestr + 4));
			  *(blt2+3) = '\0';
			  	if(isname(blt2, "Nov"))
			       strcpy(blt2, "ноября");
				if(isname(blt2, "Dec"))
			       strcpy(blt2, "декабря");
                if(isname(blt2, "Jan"))
			       strcpy(blt2, "января");
				if(isname(blt2, "Feb"))
			       strcpy(blt2, "февраля");
				if(isname(blt2, "Mar"))
			       strcpy(blt2, "марта");
				if(isname(blt2, "Apr"))
			       strcpy(blt2, "апреля");
				if(isname(blt2, "May"))
			       strcpy(blt2, "мая");
                if(isname(blt2, "Jun"))
			       strcpy(blt2, "июня");
				if(isname(blt2, "Jul"))
			       strcpy(blt2, "июля");
				if(isname(blt2, "Aug"))
			       strcpy(blt2, "августа");
				if(isname(blt2, "Sep"))
			       strcpy(blt2, "сентября");
				if(isname(blt2, "Oct"))
			       strcpy(blt2, "октября");
			  strcpy(blt,(timestr + 20));      
              *(blt+4) = '\0';
          sprintf(built, "%s %s %s", blt1, blt2, blt); 
		  }
       else
          strcpy(built, " В стадии");

 sprintf(buf1, "%s - %-13s",GET_NAME(ch), clan[i]->name);
 send_to_char("&W ----------------------------------------------------------------------------------- \r\n", ch);
 sprintf(buf, "/&c  ---&w***&c---         ---<<< &y%28s&c >>>---          ---&w***&c---&W  \\\r\n", buf1);
 send_to_char(buf, ch); 
 
 send_to_char("|  ---&c^^^&W-------------------------------------------------------------------&c^^^&W---  |\r\n", ch);
 sprintf(buf, "|&w*&W| &cВаш ранг   : &w%-10d&W|                    &cПривелегии клана:                &W |&w*&W|\r\n"
			  "|&c/&W|&c Слава      : &w%-10d&W|&c регсбор   :&w %-13s&W|&c взнос     :&w %-13s &W|&c\\&W|\r\n"
		      "|&w*&W|&c Участников : &w%-10d&W|&c принять   :&w %-13s&W|&c снять     :&w %-13s &W|&w*&W|\r\n"
		      "|&c\\&W|&c Мощь       : &w%-10d&W|&c исключить :&w %-13s&W|&c обратиться:&w %-13s &W|&c/&W|\r\n"
		  	  "|&w*&W|&c Капитал    : &w%-10d&W|&c повысить  :&w %-13s&W|&c пклист    :&w %-13s &W|&w*&W|\r\n"
			  "|&c/&W|&c Наклонность: &w%-10s&W|&c понизить  :&w %-13s&W|&c новости   :&w %-13s &W|&c\\&W|\r\n",
 
	   GET_CLAN_RANK(ch),
	   clan[i]->glory,    clan[i]->rank_name[clan[i]->privilege[0]-1],
	                      clan[i]->rank_name[clan[i]->privilege[5]-1],
	   clan[i]->members,clan[i]->rank_name[clan[i]->privilege[1]-1],
						  clan[i]->rank_name[clan[i]->privilege[6]-1],
	   clan[i]->power,    clan[i]->rank_name[clan[i]->privilege[2]-1],
					      clan[i]->rank_name[clan[i]->privilege[7]-1],
	   clan[i]->treasure, clan[i]->rank_name[clan[i]->privilege[3]-1],
					      clan[i]->rank_name[clan[i]->privilege[8]-1],
alignment[clan[i]->alignment],  clan[i]->rank_name[clan[i]->privilege[4]-1],
                          clan[i]->rank_name[clan[i]->privilege[9]-1]
						  );
 send_to_char(buf, ch); 
 send_to_char("|&w*&W|-------------------------------------------------------------------------------|&w*&W|\r\n", ch);
  sprintf(buf,"|&c\\&W|&c Регистрационный сбор   : &w%-10d&W|&c Клан создан    :&w %-24s&W|&c/&W|\r\n"
	           "|&w*&W|&c Уровень для вступления : &w%-10d&W|&c Номер клана    :&w %-24d&W|&w*&W|\r\n"
               "|&c/&W|&c Ежемесячные сборы      : &w%-10d&W|&c Номер замка    :&w %-24d&W|&c\\&W|\r\n"
              "|  \\_____________________________________________________________________________/  |\r\n"
              "| / ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^&c___&w***&c___&W^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \\ |\r\n"
			  "\\__________________________________________________________________________________/&n\r\n",

  clan[i]->app_fee,
  built,
  clan[i]->app_level,
  clan[i]->id,
  clan[i]->dues,
  clan[i]->zone);
  send_to_char(buf, ch);

 
//sprintf(buf, "Description:\r\n%s\r\n\n", clan[i].description);
//send_to_char(buf, ch);

/*if((clan[i].at_war[0] == 0) && (clan[i].at_war[1] == 0) && (clan[i].at_war[2] == 0) && (clan[i].at_war[3] == 0))
  send_to_char("В настоящий момент, этот клан ни с кем не воюет.\r\n", ch);
else
  send_to_char("Этот клан в состоянии войны.\r\n", ch);*/

return;
}


void do_clan_bonus (struct char_data *ch, char *arg)
{ struct char_data *vict, *temp;
int clan_num,immcom=0;
char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];

temp = ch;

	if ((*arg))
{
	if(GET_LEVEL(ch) < LVL_CLAN_GOD) 
	{ if((clan_num = find_clan_by_id(GET_CLAN(ch))) < 0)
		{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
			return;
		}
	}
	else
	{	immcom=1;
		half_chop(arg,arg1,arg2);
		strcpy(arg,arg1);
		if ((clan_num = ClanRec::find_clan(arg2)) < 0)
		{ send_to_char("Неизвестный клан.\r\n", ch);
			return;
		}
	}

	if(GET_CLAN_RANK(ch) < clan[clan_num]->privilege[CP_PROMOTE] && !immcom)
		{ send_to_char("У Вас нет полномочий что бы это делать!!\r\n",ch);
			return;
		}

	if(!(vict=get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		{ send_to_char("Это кто?\r\n",ch);
			return;
		}
	else
		{ if(GET_CLAN(vict) != clan[clan_num]->id)
			{ send_to_char("Этот персонаж не состоит в Вашем клане.\r\n",ch);
				return;
			}
			else
		{ if(GET_CLAN_RANK(vict) == 0)
			{ send_to_char("Его надо сначало в клан принять!\r\n",ch);
				return;
			}
		}
	}
temp = vict;
}
		if(!immcom)
			{ sprintf(buf,
               "\r\n&W /^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^&c___&w***&c___&W^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\\\r\n"
			  "|&c/&W|                                  &GВаши бонусы:                                 &W|&c\\&W|\r\n"
              "|&w*&W|-------------------------------------------------------------------------------|&w*&W|\r\n" 
			  "|&c\\&W|                                                                               &W|&c/&W|\r\n"
			  "|&w*&W|&g   Хитпоинты  :&n %-3s  &g Мораль     : &n%-3s &g  Дамролы    : &n%-3s  &g Хитролы    :&n %-3s   &W|&w*&W|\r\n"
			  "|&c/&W|&g   Инициатива :&n %-3s  &g Движения   : &n%-3s &g  Хитрег     : &n%-3s  &g Муврег     :&n %-3s   &W|&c\\&W|\r\n"
			  "|&w*&W|&g   Шанскаст   : &n%-3s   &gЗаучивание : &n%-3s  &g Защита     : &n%-3s   &gПоглощение : &n%-3s   &W|&w*&W|\r\n"
			  "|&c\\&W|&g   Воля       :&n %-3s  &g Стойкость  :&n %-3s &g  Реакция    :&n %-3s  &g Здоровье   :&n %-3s   &W|&c/&W|\r\n"
			  "|&w*&W|&g   Мудрость   : &n%-3s   &gИнтеллект  : &n%-3s  &g Ловкость   : &n%-3s   &gСила       :&n %-3s   &W|&w*&W|\r\n"
		     "|&c\\&W|&g                      Тело       : &n%-3s &g  Удача      : &n%-3s                      &W|&c/&W|\r\n"
			  "|  \\_____________________________________________________________________________/  |\r\n"
              "| / ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^&c___&w***&c___&W^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \\ |\r\n"
			  "\\__________________________________________________________________________________/&n\r\n",

 
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_HITPOINT)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_MORALE)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_DAMROLL)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_HITROLL)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_INITIATIVE)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_MOVE)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_HITREG)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_MOVEREG)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_CAST_SUCCES)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_MANAREG)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_AC)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_ABSORBE)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_SAVE_SPELL)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_SAVE_PARA)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_SAVE_ROD)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_SAVE_BASIC)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_WIS)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_INT)), 
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_DEX)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_STR)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_CON)),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_LCK))
  );
		}
		else
		{ sprintf(buf,
               "\r\n&W /^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^&c___&w***&c___&W^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\\\r\n"
			  "|&c/&W|                                  &GВаши бонусы:                                 &W|&c\\&W|\r\n"
              "|&w*&W|-------------------------------------------------------------------------------|&w*&W|\r\n" 
			  "|&c\\&W|                                                                               &W|&c/&W|\r\n"
			  "|&w*&W|&g Хитпоинты :&n %-3s &c%-3d &gМораль    : &n%-3s &c%-3d &gДамролы : &n%-3s &c%-3d &gХитролы   :&n %-3s &c%-3d &W|&w*&W|\r\n"
			  "|&c/&W|&g Инициатива:&n %-3s &c%-3d &gДвижения  : &n%-3s &c%-3d &gХитрег  : &n%-3s &c%-3d &gМуврег    :&n %-3s &c%-3d &W|&c\\&W|\r\n"
			  "|&w*&W|&g Шанскаст  : &n%-3s &c%-3d &gЗаучивание: &n%-3s &c%-3d &gЗащита  : &n%-3s &c%-3d &gПоглощение: &n%-3s &c%-3d &W|&w*&W|\r\n"
			  "|&c\\&W|&g Воля      :&n %-3s &c%-3d &gСтойкость :&n %-3s &c%-3d &gРеакция :&n %-3s &c%-3d &gЗдоровье  :&n %-3s &c%-3d &W|&c/&W|\r\n"
			  "|&w*&W|&g Мудрость  : &n%-3s &c%-3d &gИнтеллект : &n%-3s &c%-3d &gЛовкость: &n%-3s &c%-3d &gСила      :&n %-3s &c%-3d &W|&w*&W|\r\n"
		     "|&c\\&W|&g                     Тело      : &n%-3s &c%-3d &gУдача   : &n%-3s &c%-3d                     &W|&c/&W|\r\n"
			  "|  \\_____________________________________________________________________________/  |\r\n"
              "| / ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^&c___&w***&c___&W^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \\ |\r\n"
			  "\\__________________________________________________________________________________/&n\r\n",

 
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_HITPOINT)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/3500, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_MORALE)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/30000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_DAMROLL)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/45000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_HITROLL)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/45000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_INITIATIVE)),MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/25000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_MOVE)),		MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/20000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_HITREG)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/5000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_MOVEREG)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/3000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_CAST_SUCCES)),MAX (MIN (clan[clan_num]->power,IND_POWER_CHAR(temp))/47500, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_MANAREG)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/15000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_AC)),		MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/20000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_ABSORBE)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/60000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_SAVE_SPELL)),MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/40000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_SAVE_PARA)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/40000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_SAVE_ROD)),	MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/40000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_SAVE_BASIC)),MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/40000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_WIS)),		MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/215000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_INT)),		MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/205000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_DEX)),		MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/195000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_STR)),		MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/185000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_CON)),		MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/165000, 0),
  ONOFF(PLR_FLAGGED(temp, AFF_CLAN_LCK)),		MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(temp))/155000, 0)
  );
		}
  send_to_char(buf, ch);
  
  if (temp != ch)
	{ sprintf(buf, "\r\n\r\n                    Параметры личной мощности Вашего соклановца &C%s&n\r\n"
	               "\r\n                         &WОИМ : &C%d         &WПИМ : &C%d&n\r\n",GET_RNAME(temp), 
				     IND_POWER_CHAR(temp), IND_SHOP_POWER(temp));
	  send_to_char(buf, ch);
	}
}

void do_clan_SetID(struct char_data *ch, char *arg)
{

char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
int  clan_num=0;

	if (!*arg)
	{	send_clan_format(ch);
		return;
	}

	if (GET_LEVEL(ch) < LVL_CLAN_GOD)
	{	send_to_char("Вы не достаточно могущественны, чтобы переименовать клан!\r\n", ch);
		return;
	}

		half_chop(arg, arg1, arg2);

		if ((clan_num = ClanRec::find_clan(arg1)) < 0)
	{ send_to_char("Такого клана не существует!\r\n",ch);
		return;
	}

	
	clan[clan_num]->id = atoi(arg2);
	send_to_char("ID - клана изменен!\r\n", ch);
}

ACMD(do_clan)
{
char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

half_chop(argument, arg1, arg2);

if (is_abbrev(arg1, "переименовать")) { do_clan_rename(ch,arg2);return ;}
if (is_abbrev(arg1, "создать"  )) { do_clan_create(ch,arg2);   return ;}
if (is_abbrev(arg1, "удалить"  )) { do_clan_destroy(ch,arg2, 0);return ;}
if (is_abbrev(arg1, "принять"  )) { do_clan_enroll(ch,arg2);   return ;}
if (is_abbrev(arg1, "исключить")) { do_clan_expel(ch,arg2);    return ;}
if (is_abbrev(arg1, "бонусы"   )) { do_clan_bonus(ch, arg2);   return ;}
if (is_abbrev(arg1, "статус"   )) { do_clan_status(ch);        return ;}
if (is_abbrev(arg1, "кто"      )) { do_clan_who(ch, arg2);     return ;}
if (is_abbrev(arg1, "инфо"     )) { do_clan_info(ch,arg2);     return ;}
if (is_abbrev(arg1, "обратиться")){ do_clan_apply(ch,arg2);    return ;}
if (is_abbrev(arg1, "понизить" )) { do_clan_demote(ch,arg2);   return ;}
if (is_abbrev(arg1, "повысить" )) { do_clan_promote(ch,arg2);  return ;}
if (is_abbrev(arg1, "присвоить")) { do_clan_set(ch,arg2);      return ;}
if (is_abbrev(arg1, "снять"    )) { do_clan_bank(ch,arg2,CB_WITHDRAW); return ;}
if (is_abbrev(arg1, "вложить"  )) { do_clan_bank(ch,arg2,CB_DEPOSIT); return ;}
if (is_abbrev(arg1, "внести"   )) { do_clan_bank(ch,arg2,CM_GLORY);  return ;}
if (is_abbrev(arg1, "пклист"   )) { do_clan_setPklist(ch,arg2);return ;}
if (is_abbrev(arg1, "новости"  )) { do_clan_news(ch,arg2);     return ;}
if (is_abbrev(arg1, "установить")){ do_clan_SetID(ch,arg2);     return ;}
send_clan_format(ch);
}

struct char_data *is_playing(char *vict_name)
{
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i, *next_i;

  if (vict_name == NULL)
	  return NULL;

  for (i = descriptor_list; i; i = next_i) {
    next_i = i->next;
   	if(i->connected == CON_PLAYING && !strcmp(i->character->player.name, CAP(vict_name)))
      return i->character;
  }
  return NULL;
}



sh_int find_clan_by_id(int idnum)
{ for (int i=0; i < num_of_clans; i++)
	   if (idnum == clan[i]->id)
       return i;
	return -1;
}


/* Работа с новостями */
void Clan_fname(struct char_data * ch, char *name, int imm)
{
  char id[MAX_INPUT_LENGTH];

  /* Вычисляем имя файла */
  strcpy(name,LIB_HOUSE);
  sprintf(id,"%ld",!imm ? GET_CLAN(ch): imm);
  strcat(name,id);
  strcat(name,".hnews");
  /*  */
}

int Clan_news(struct descriptor_data *d, int imm)
{
  char news[MAX_EXTEND_LENGTH];
  char fname[MAX_INPUT_LENGTH];
  
  Clan_fname(d->character, fname, imm);
  if (!file_to_string(fname,news)) {
	SEND_TO_Q(news,d);
    return (1);
  }
  send_to_char("В настоящее время клановые новости отсутствуют!\r\n",d->character);
  return (0);
}

void delete_clan_news(struct char_data * ch, int imm)
{
  char fname[MAX_INPUT_LENGTH];

  Clan_fname(ch, fname, imm);
  remove(fname);
  send_to_char("Новости удалены!\r\n",ch);
}

void add_clan_news(struct char_data *ch, char *nw, int imm)
{
  char fname[MAX_INPUT_LENGTH];
  int clan_num = 0;
  FILE *fl;
  
  Clan_fname(ch, fname, imm);

  if (!(fl = fopen(fname,"a")))
    return;

  clan_num      = find_clan_by_id(GET_CLAN(ch)); 
 
  if (GET_CLAN_RANK(ch) == clan[clan_num]->privilege[CP_SET_APPLEV] && !imm)
   fprintf(fl, "%s\n", nw);
  else
   fprintf(fl, "[%s] %s\n", GET_NAME(ch), nw);

  fclose(fl);
  send_to_char("Ваша новость добавлена!\r\n",ch);
  
}


void do_clan_news(struct char_data *ch, char *arg)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int clan_num, imm=0;


	if(GET_LEVEL(ch)<LVL_IMMORT)
	{ if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0 || !GET_CLAN_RANK(ch))
		{ send_to_char("Вы не принадлежите ни к одному из кланов!\r\n",ch);
		  return;
		}
	}
	else
	{ if(GET_LEVEL(ch)<LVL_CLAN_GOD)
		{ send_to_char("Вы не имеете для этого привилегий.\r\n", ch);
		  return;
		}
			
			half_chop(arg,arg1,arg2);
			strcpy(arg,arg2);
            imm = atoi(arg1);
		if(!is_number(arg1))
		{ send_to_char("Вы должны определить номер клана.\r\n",ch);
		  return;
		}
	  if((clan_num=atoi(arg1))<0)
		{ send_to_char("Нет клана с таким номером.\r\n",ch);
          return;
		}
	}

  half_chop(arg, arg1, arg2);
  
  if (!*arg)
  {
    Clan_news(ch->desc, imm);
    return;
  }
     
  if (is_abbrev(arg1, "стереть") && GET_CLAN_RANK(ch) >= clan[clan_num]->privilege[CP_SET_APPLEV] || imm)
     delete_clan_news(ch, imm);
  else
  if (is_abbrev(arg1, "добавить") && GET_CLAN_RANK(ch) >= clan[clan_num]->privilege[CP_NEWS]|| imm) 
     add_clan_news(ch, arg2, imm);
  else
    Clan_news(ch->desc, imm);

}


int GetClanzone(struct char_data *ch)
{ int  zone, clan_num;
 
       if (IN_ROOM(ch) == NOWHERE)//чтобы не крешало, если ушли в лд и есть флаг убивица.
             return false;

	if((clan_num = find_clan_by_id(GET_CLAN(ch)))<0)
			return true;	

	zone = zone_table[(world[IN_ROOM(ch)].zone)].number;
 
	if (zone == clan[clan_num]->zone)
			return false;
	
return true;
}

void pk_translate_pair(struct char_data * * pkiller, struct char_data * * pvictim);
//функция по обработке собитый вызываемых действиями клановцев.
void ClanSobytia(struct char_data *ch, struct char_data *vict, int modifer)
{   struct PK_Memory_type *pk;
	int  clan_num, clan_opponent;
    bool found = FALSE;


	//ch   -  киллер, убивающий жертву vict
    //vict -  убитый киллером ch

	// разремить при реализации проверки на месть.
	//переписать обработку удаления кланов, выдернуть десторе из файтинга линуха,
	//да блин ща сделаю тут хули ипатца.
   
          if (IS_NPC(vict))
		return;

//	if (vict)
clan_opponent = find_clan_by_id(GET_CLAN(vict));
pk_translate_pair(&ch, NULL );
clan_num      = find_clan_by_id(GET_CLAN(ch)); 

 if (clan_opponent < 0 && clan_num < 0)
     return;


	 
 /*  if(clan_num >= 0)
		if (clan[clan_num]->power < -1000) 
	{   do_clan_destroy(vict,clan[clan_num]->name, 1);
        save_clans();
		return;
	  
	} 
	
		//проверяем, если умер член клана, делаем проверку на снижение мощности, если мощь ниже -1000 - клан удаляем.
    if(clan_opponent >= 0)
		if (clan[clan_opponent]->power < -1000) 
	{   do_clan_destroy(vict,clan[clan_opponent]->name, 1);
        save_clans();
		return;
	  
	} 
  
*/
//  Пока заремлено, в случае выполнения квестов для набора мощи кланов
//  разремить, здесь происходит снятие мощи за рип клан_игрока.
    if(clan_opponent >=0 && clan_num < 0) 
	 { IND_POWER_CHAR(vict) -= (IND_POWER_CHAR(vict) * 3) / 1000;//при рипе теряем 0.3 процента мощности.
	   IND_SHOP_POWER(vict) -= (IND_POWER_CHAR(vict) * 3) / 1000;
	   return;
     }
//уточинить, обязательно вйона с двух сторон? или при обьявлении войны автоматом всем война присваивается ?
	 //киллер_и_жертва_клановцы находящиеся в войне
	/* if (clan_opponent >= 0 && clan_num >= 0)
		{  if(clan[clan_num].at_war[clan_opponent] ==
	          clan[clan_opponent].at_war[clan_num])
			{ clan[clan_num].power += 500 + (30 * GET_CLAN_RANK(vict));
	          clan[clan_opponent].power -= 200 - (60 * GET_CLAN_RANK(vict));
			}
			else //штраф за убийство если не в войне
			 clan[clan_num].power -= 500;
			return;	 
	 }
*/
     
	 
 //проверяем, если в ПК листе клана, то убиваем с получением мощи клану.    
 /*for (i=0; i < MAX_PKLIST; i++)
 {	 if(clan[clan_num].PkList[i] == GET_UNIQUE(vict))
	{ 	     found =  TRUE;
	      if (GET_LEVEL(vict) > 19)
		   clan[clan_num].power += 200;
	}
 }  
    */
 //здесь 
 if (GET_LEVEL(vict) > 19 && PLR_FLAGGED(vict, PLR_KILLER))
	{ IND_POWER_CHAR(ch) += 500;
      return;
	}
 if (PLR_FLAGGED(vict, PLR_KILLER))
	 return;


 for (pk = vict->pk_list; pk; pk = pk->next)
		    if (pk->unique == GET_UNIQUE(ch))
				 return;
         
		 	 
		 if (!found)
	  IND_POWER_CHAR(ch) -= 500;
	
}




void ClansAffect(struct char_data * ch)
{ int clan_num;
  

	if((clan_num = find_clan_by_id(GET_CLAN(ch))) < 0 ||
		       !GET_CLAN_RANK(ch))
					return;

	if (PLR_FLAGGED(ch, AFF_CLAN_HITPOINT))
		GET_HIT_ADD(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/3500, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_MORALE))
		GET_MORALE(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/30000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_DAMROLL))
		GET_DR_ADD(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/45000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_HITROLL))
		GET_HR_ADD(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/45000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_INITIATIVE))
		GET_INITIATIVE(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/25000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_MOVE))
		GET_MOVE_ADD(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/20000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_HITREG))
		GET_HITREG(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/5000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_MOVEREG))
		GET_MOVEREG(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/3000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_CAST_SUCCES))
		GET_CAST_SUCCESS(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/47500, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_MANAREG))
		GET_MANAREG(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/15000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_AC))
		GET_AC_ADD(ch) -= MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/20000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_ABSORBE))
		GET_ABSORBE(ch) += MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/60000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_SPELL))
		GET_SAVE(ch, SAVING_SPELL) -= MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/40000, 0);
		
	if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_PARA))
		GET_SAVE(ch, SAVING_PARA) -= MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/40000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_ROD))
		GET_SAVE(ch, SAVING_ROD) -= MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/40000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_SAVE_BASIC))
		GET_SAVE(ch, SAVING_BASIC) -= MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/40000, 0);

	if (PLR_FLAGGED(ch, AFF_CLAN_WIS))
	   affect_modify(ch, APPLY_WIS, MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/215000, 0),
								 0, TRUE);

	if (PLR_FLAGGED(ch, AFF_CLAN_INT))
		affect_modify(ch, APPLY_INT, MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/205000, 0),
								 0, TRUE);

	if (PLR_FLAGGED(ch, AFF_CLAN_DEX))
		affect_modify(ch, APPLY_DEX, MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/195000, 0),
								 0, TRUE);

	if (PLR_FLAGGED(ch, AFF_CLAN_STR))
		affect_modify(ch, APPLY_STR, MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/185000, 0),
								 0, TRUE);

	if (PLR_FLAGGED(ch, AFF_CLAN_CON))
		affect_modify(ch, APPLY_CON, MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/165000, 0),
								 0, TRUE);

	if (PLR_FLAGGED(ch, AFF_CLAN_LCK))
		affect_modify(ch, APPLY_CHA, MAX (MIN (clan[clan_num]->power, IND_POWER_CHAR(ch))/155000, 0),
								 0, TRUE);

	
}




sh_int ClanRec :: find_clan (char *name)//получение названия клана
{ 
	for (int i=0; i < num_of_clans; i++)
	 	if (!strcmp(name, clan[i]->name))
			return i;
			   return -1;
}



