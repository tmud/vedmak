/**************************************************************************
 * File: clan.h									   Part of Golden Dragon  *
 * Usage: This is the code for clans                                      *
 * By Andrey Gorbenko										              *
 * Golden Dragon Copyright (C) 2004										  *
 * Version 3.1.6 Date 02 March 2006										  *
 **************************************************************************/
// ������ ����_������� ������������� ��� ������������ � ���� Golden Dgagon.
// ������������� � ��������� ���� ��������� ��������������� � ���������� ������.
// ����� ���������� ��������� ������ � �������� ������.
// ������� ������������ ����� ���������� � ����������� �������������� ������������
// �������. 


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
void save_clans (void);					//������ ����� �� ����
void init_clans (void);					//������������� ������
void IndexClans (void);					//���������� ������� ������
//void do_clan_PKset(struct char_data *ch, char *arg, int mode);

sh_int find_clan_by_id (int clan_id);	//��������� �� ������ �����



const int MAX_CLANS				= 10;		//������������ ���������� ����������� ������
const int LVL_CLAN_GOD			= 49;		//������� �������� ����� (��� 49 �����)
const int DEFAULT_APP_LVL		= 8;		//������� ���������� �� ���������
const int LIDER_RANKS			= 7;		//������������ ���� �����

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




#define HOUSE_ID(clan_num) (clan[clan_num]->id)	//����� �����
#define MAX_MEMBERS			300					// ������������ ���������� ���������
#define MAX_PKLIST			100					// ������������ ���������� � ��-�����
#define SPELL_CLANS			10					// �������� ������.
#define SKILL_CLANS			10					// �������� ������.
#define CLAN_WARS			10					// �������� �����


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
  
 

  

   	static sh_int find_clan (char *name);//��������� �������� �����
 

	
	char		alignment;			//����������� �����.
	int		id;					//������� �����
	int		members;			//���������� �����������
	int		app_fee;			//��������������� ����
	char		app_level;			//������� ������ � ����.
	int		at_war[CLAN_WARS];	//�����.�������� �������� ������, ����������� � ����� ��� �� � �����.(�� �� ��� � ��� ��)
	char		rank_name[LIDER_RANKS][20];			//��� �����
	char		name[32];				//��� �����
	char		*description;		//�������� �����
	ubyte		ranks;				//���� �����
	ubyte		privilege[20];		//����������������� ������� ��� �����������
	long		glory;				//����� �����
	long		treasure;			//���������� ����� � �����
	long		power;				//�������� �����
	long		dues;				//����������� �����
	long		PkList[MAX_PKLIST];	//�������� ��_����� �����.(������� ���� �������, ���� ��������).
	time_t		built_on;			//����� �������� �����.
	int			zone;				//������� �                                                                                      ���� ��� �����.
	long        guests[MAX_MEMBERS];  			


    Journal     journal;

    struct		Pk_Date *CharPk;		//��������� �������� ��_�����.
    
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

