/**************************************************************************
 * File: clan.h									   Part of Golden Dragon  *
 * Usage: This is the code for clans                                      *
 * By Andrey Gorbenko										              *
 * Golden Dragon Copyright (C) 2004										  *
 * Version 3.1.6 Date 02 March 2006										  *
 **************************************************************************/
// Данная клан_система предназначена для эксплуотации в МАДе Golden Dgagon.
// Использование и изменение кода разрешено непосредственно с разрешения автора.
// Любое дополнение разрешено только с согласия автора.
// Система представляет собой управление и организацию внутриклановой деятельности
// игроков. 


#ifndef __CLAN_H_
#define __CLAN_H_


//#include <list>
#include <vector>
//#include <string>
//#include <iostream>
//#include <fstream>

using std::vector;
//using std::list;
//using std::string;
//using std::ifstream;
//using std::ofstream;
//using std::endl;

#include "support.h"

void ClanSobytia(struct char_data *ch, struct char_data *vict, int modifer);
void ClansAffect(struct char_data * ch);
void save_clans (void);					//запись клана на диск
void init_clans (void);					//инициализация кланов
void IndexClans (void);					//считывание индекса кланов
//void do_clan_PKset(struct char_data *ch, char *arg, int mode);

sh_int find_clan_by_id (int clan_id);	//получение ИД номера клана



const int MAX_CLANS				= 10;		//максимальное количество создаваемых кланов
const int LVL_CLAN_GOD			= 49;		//уровень создания клана (Имм 49 левел)
const int DEFAULT_APP_LVL		= 8;		//уровень вступления по умолчанию
const int LIDER_RANKS			= 7;		//максимальный ранг клана

#define CP_SET_PLAN   0
#define CP_ENROLL     1
#define CP_EXPEL      2
#define CP_PROMOTE    3
#define CP_DEMOTE     4
#define CP_SET_FEES   5
#define CP_WITHDRAW   6
#define CP_SET_APPLEV 7
#define CP_PKLIST     8
#define CP_NEWS       9
#define NUM_CP        10        /* Number of clan privileges */
 

#define CLAN_PLAN_LENGTH		800

#define CM_DUES   1
#define CM_APPFEE 2
#define CM_GLORY  3

#define CB_DEPOSIT  1
#define CB_WITHDRAW 2




#define HOUSE_ID(clan_num) (clan[clan_num]->id)	//номер клана
#define MAX_MEMBERS			300					// максимальное количество клановцев
#define MAX_PKLIST			100					// максимальное количество в пк-листе
#define SPELL_CLANS			10					// Клановые спеллы.
#define SKILL_CLANS			10					// Клановые скиллы.
#define CLAN_WARS			10					// Клановые воины


extern int num_of_clans;

struct Pk_Date
{
	char	 *Desc;
	int		 PkUid;
	struct Pk_Date *next;
};


class ClanRec
{

public:
	
//	list < Pk_Date * > PkMethod;
  
 

  

   	static sh_int find_clan (char *name);//получение названия клана
 

	
	char		alignment;			//наклонность клана.
	int		id;					//идешник клана
	int		members;			//количество соклановцев
	int		app_fee;			//регистрационный сбор
	char		app_level;			//уровень приема в клан.
	int		at_war[CLAN_WARS];	//война.хранятся идешники кланов, участвующих в войне или не в войне.(то же что и для ПК)
	char		rank_name[LIDER_RANKS][20];			//имя ранга
	char		name[32];				//имя клана
	char		*description;		//описание клана
	ubyte		ranks;				//ранг клана
	ubyte		privilege[20];		//привелегированные команды для соклановцев
	long		glory;				//слава клана
	long		treasure;			//количество денег в банке
	long		power;				//мощность клана
	long		dues;				//ежемесячные сборы
	long		PkList[MAX_PKLIST];	//идешники пк_листа чаров.(сделать либо списком, либо вектором).
	time_t		built_on;			//время создания клана.
	int			zone;				//Родовой г                                                                                      ород для клана.
	long        guests[MAX_MEMBERS];  			


    Journal     journal;

    struct		Pk_Date *CharPk;		//структура хранения ПК_ЛИСТА.
    
ClanRec() : alignment(0), id(0), members(0), app_fee(0), app_level(0), description(0), ranks(0), 
glory(0), treasure(0), power(0), dues(0), built_on(0), zone(0), CharPk(0)
{
    memset(at_war, 0, sizeof(int)*CLAN_WARS);   
    for (int i=0; i<LIDER_RANKS; ++i)
    {
        char *p = rank_name[i];
        memset(p, 0, 20);
    }

    memset(name, 0, 32);
    memset(privilege, 0, 20);
    memset(PkList, 0, MAX_PKLIST*sizeof(long));
    memset(guests, 0, MAX_MEMBERS*sizeof(long));
             
}

virtual ~ClanRec ()
			{}


};

#endif

