/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern int dts_are_dumps;
extern int mini_mud;
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;

SPECIAL(dump);
SPECIAL(pet_shops);
SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(guild_guard);
SPECIAL(guild);
SPECIAL(guild1);
SPECIAL(horse_keeper);
SPECIAL(horse_shops);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(kladovchik);
SPECIAL(janitor);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(bank);
SPECIAL(gen_board);
SPECIAL(questmaster);
SPECIAL(power_stat);
SPECIAL(cleric);
SPECIAL(SaleBonus);
SPECIAL(WHouse);

void assign_kings_castle(void);
char *str_str(char *cs, char *ct);
/* local functions */
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
  mob_rnum rnum;

  if ((rnum = real_mobile(mob)) >= 0)
    mob_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) >= 0)
    obj_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
  room_rnum rnum;

  if ((rnum = real_room(room)) >= 0)
    world[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
//  assign_kings_castle();

  ASSIGNMOB(1, puff);

   //Мад-школа, кладовщик
  ASSIGNMOB(1014, kladovchik); 

  // Immortal Zone 
  ASSIGNMOB(1200, receptionist);
  ASSIGNMOB(1201, postmaster);
  ASSIGNMOB(1203, receptionist);
  ASSIGNMOB(1204, receptionist);
//  ASSIGNMOB(1202, janitor);

// Наследники
// ASSIGNMOB(37000, receptionist);
// ASSIGNMOB(37007, bank);
// ASSIGNMOB(37008, postmaster);

   //Клан Орден Света
 ASSIGNMOB(53012, receptionist);
 ASSIGNMOB(53016, postmaster);
 ASSIGNMOB(53013, bank);
//Работник слкада по выдаче предметов, которые хранятся в хранилище.
  ASSIGNMOB(53017, WHouse);

//Клан ЗАМОК Черного Братства
 ASSIGNMOB(54009, receptionist);
 ASSIGNMOB(54005, postmaster);
 ASSIGNMOB(54011, bank);
//Работник слкада по выдаче предметов, которые хранятся в хранилище.
  ASSIGNMOB(54015, WHouse);

// Клан ТЕмный легион
// ASSIGNMOB(54009, receptionist);
// ASSIGNMOB(54010, receptionist);
// ASSIGNMOB(54005, postmaster);
// ASSIGNMOB(54011, bank);

  
// Клан Порождения хаоса
//  ASSIGNMOB(55000, receptionist);
  
// Клан Хранители Леса  
//  ASSIGNMOB(56014, receptionist);
//  ASSIGNMOB(56013, postmaster);
//  ASSIGNMOB(56011, bank);
//  ASSIGNMOB(56012, horse_shops);


// Клан Рыцари Рассвета
   ASSIGNMOB(57000, receptionist);
   ASSIGNMOB(57006, bank);
   ASSIGNMOB(57007, postmaster);
   ASSIGNMOB(57013, WHouse);
  

// Наемники рента
// ASSIGNMOB(21507, receptionist);
// ASSIGNMOB(21505, bank);

 // Новиград
  ASSIGNMOB(36610, receptionist);

  // Шаэрраведд 
  ASSIGNMOB(3600, receptionist);
  ASSIGNMOB(3614, guild);
  ASSIGNMOB(3615, guild);
  ASSIGNMOB(3616, guild);
  ASSIGNMOB(3617, guild);
  ASSIGNMOB(3601, guild);
  ASSIGNMOB(3602, guild);
  ASSIGNMOB(3603, guild);
  ASSIGNMOB(3604, guild);
  ASSIGNMOB(3605, guild);
  ASSIGNMOB(3606, guild);
  ASSIGNMOB(3636, guild);
  ASSIGNMOB(3642, cleric);
  ASSIGNMOB(3608, kladovchik);
  ASSIGNMOB(3645, SaleBonus);
  //Работник слкада по выдаче предметов, которые хранятся в хранилище.
  ASSIGNMOB(3646, WHouse);

// Крейден 
  ASSIGNMOB(6211, receptionist);
  ASSIGNMOB(6209, guild1);
  ASSIGNMOB(6210, guild1);
  ASSIGNMOB(6203, guild1);
  ASSIGNMOB(6202, guild1);
  ASSIGNMOB(6201, guild1);
  ASSIGNMOB(6205, guild1);
  ASSIGNMOB(6208, guild1);
  ASSIGNMOB(6204, guild1);
  ASSIGNMOB(6207, guild1);
  ASSIGNMOB(6206, guild1);
  ASSIGNMOB(6200, guild1);
  ASSIGNMOB(6228, cleric);
  ASSIGNMOB(6212, bank);
  ASSIGNMOB(6229, SaleBonus);
  //Работник слкада по выдаче предметов, которые хранятся в хранилище.
  ASSIGNMOB(6213, WHouse);
  ASSIGNMOB(6227, power_stat);
  ASSIGNMOB(6230, questmaster);



  /* Гильдмастера Гелибола*/
  ASSIGNMOB(9004, guild1);
  ASSIGNMOB(9005, guild1);
  ASSIGNMOB(9006, guild1);
  ASSIGNMOB(9007, guild1);
  ASSIGNMOB(9008, guild1);
  ASSIGNMOB(9009, guild1);
  ASSIGNMOB(9010, guild1);
  ASSIGNMOB(9011, guild1);
  ASSIGNMOB(9012, guild1);
  ASSIGNMOB(9013, guild1);
  ASSIGNMOB(9017, guild1);

/*ASSIGNMOB(3607, guild_guard);*/
/*  ASSIGNMOB(3025, guild_guard);
  ASSIGNMOB(3026, guild_guard);
  ASSIGNMOB(3027, guild_guard);
  ASSIGNMOB(3059, cityguard);
  ASSIGNMOB(3060, cityguard);
  ASSIGNMOB(3061, janitor);
  ASSIGNMOB(3062, fido);
  ASSIGNMOB(3066, fido);
  ASSIGNMOB(3067, cityguard);
  ASSIGNMOB(3068, janitor);
  ASSIGNMOB(3095, cryogenicist);*/
 
  // Квестор
  ASSIGNMOB(3629, questmaster);

  /* BANK */
  ASSIGNMOB(3618, bank);
  ASSIGNMOB(36605, bank);
 //postmaster
 ASSIGNMOB(3643, postmaster);
  
  /* HORSEKEEPER */
 // ASSIGNMOB(3123, horse_keeper);
  ASSIGNMOB(8006, horse_shops);
  ASSIGNMOB(3631, power_stat);
  
  
  
  
  
  /*ASSIGNMOB(3105, mayor);

  /* MORIA */
  /*ASSIGNMOB(4000, snake);
  ASSIGNMOB(4001, snake);
  ASSIGNMOB(4053, snake);
  ASSIGNMOB(4100, magic_user);
  ASSIGNMOB(4102, snake);
  ASSIGNMOB(4103, thief);

  /* Redferne's */
  /*ASSIGNMOB(7900, cityguard);

  /* PYRAMID */
  /*ASSIGNMOB(5300, snake);
  ASSIGNMOB(5301, snake);
  ASSIGNMOB(5304, thief);
  ASSIGNMOB(5305, thief);
  ASSIGNMOB(5309, magic_user); /* should breath fire */
  /*ASSIGNMOB(5311, magic_user);
  ASSIGNMOB(5313, magic_user); /* should be a cleric */
  /*ASSIGNMOB(5314, magic_user); /* should be a cleric */
  /*ASSIGNMOB(5315, magic_user); /* should be a cleric */
  /*ASSIGNMOB(5316, magic_user); /* should be a cleric */
  /*ASSIGNMOB(5317, magic_user);

  /* High Tower Of Sorcery */
  /*ASSIGNMOB(2501, magic_user); /* should likely be cleric */
  /*ASSIGNMOB(2504, magic_user);
  ASSIGNMOB(2507, magic_user);
  ASSIGNMOB(2508, magic_user);
  ASSIGNMOB(2510, magic_user);
  ASSIGNMOB(2511, thief);
  ASSIGNMOB(2514, magic_user);
  ASSIGNMOB(2515, magic_user);
  ASSIGNMOB(2516, magic_user);
  ASSIGNMOB(2517, magic_user);
  ASSIGNMOB(2518, magic_user);
  ASSIGNMOB(2520, magic_user);
  ASSIGNMOB(2521, magic_user);
  ASSIGNMOB(2522, magic_user);
  ASSIGNMOB(2523, magic_user);
  ASSIGNMOB(2524, magic_user);
  ASSIGNMOB(2525, magic_user);
  ASSIGNMOB(2526, magic_user);
  ASSIGNMOB(2527, magic_user);
  ASSIGNMOB(2528, magic_user);
  ASSIGNMOB(2529, magic_user);
  ASSIGNMOB(2530, magic_user);
  ASSIGNMOB(2531, magic_user);
  ASSIGNMOB(2532, magic_user);
  ASSIGNMOB(2533, magic_user);
  ASSIGNMOB(2534, magic_user);
  ASSIGNMOB(2536, magic_user);
  ASSIGNMOB(2537, magic_user);
  ASSIGNMOB(2538, magic_user);
  ASSIGNMOB(2540, magic_user);
  ASSIGNMOB(2541, magic_user);
  ASSIGNMOB(2548, magic_user);
  ASSIGNMOB(2549, magic_user);
  ASSIGNMOB(2552, magic_user);
  ASSIGNMOB(2553, magic_user);
  ASSIGNMOB(2554, magic_user);
  ASSIGNMOB(2556, magic_user);
  ASSIGNMOB(2557, magic_user);
  ASSIGNMOB(2559, magic_user);
  ASSIGNMOB(2560, magic_user);
  ASSIGNMOB(2562, magic_user);
  ASSIGNMOB(2564, magic_user);

  /* SEWERS */
  /*ASSIGNMOB(7006, snake);
  ASSIGNMOB(7009, magic_user);
  ASSIGNMOB(7200, magic_user);
  ASSIGNMOB(7201, magic_user);
  ASSIGNMOB(7202, magic_user);

  /* FOREST */
  /*ASSIGNMOB(6112, magic_user);
  ASSIGNMOB(6113, snake);
  ASSIGNMOB(6114, magic_user);
  ASSIGNMOB(6115, magic_user);
  ASSIGNMOB(6116, magic_user); /* should be a cleric */
  /*ASSIGNMOB(6117, magic_user);

  /* ARACHNOS */
  /*ASSIGNMOB(6302, magic_user);
  ASSIGNMOB(6309, magic_user);
  ASSIGNMOB(6312, magic_user);
  ASSIGNMOB(6314, magic_user);
  ASSIGNMOB(6315, magic_user);

  /* Desert */
  /*ASSIGNMOB(5004, magic_user);
  ASSIGNMOB(5005, guild_guard); /* brass dragon */
  /*ASSIGNMOB(5010, magic_user);
  ASSIGNMOB(5014, magic_user);

  /* Drow City */
  /*ASSIGNMOB(5103, magic_user);
  ASSIGNMOB(5104, magic_user);
  ASSIGNMOB(5107, magic_user);
  ASSIGNMOB(5108, magic_user);

  /* Old Thalos */
  /*ASSIGNMOB(5200, magic_user);
  ASSIGNMOB(5201, magic_user);
  ASSIGNMOB(5209, magic_user);

  /* New Thalos */
/* 5481 - Cleric (or Mage... but he IS a high priest... *shrug*) */
  /*ASSIGNMOB(5404, receptionist);
  ASSIGNMOB(5421, magic_user);
  ASSIGNMOB(5422, magic_user);
  ASSIGNMOB(5423, magic_user);
  ASSIGNMOB(5424, magic_user);
  ASSIGNMOB(5425, magic_user);
  ASSIGNMOB(5426, magic_user);
  ASSIGNMOB(5427, magic_user);
  ASSIGNMOB(5428, magic_user);
  ASSIGNMOB(5434, cityguard);
  ASSIGNMOB(5440, magic_user);
  ASSIGNMOB(5455, magic_user);
  ASSIGNMOB(5461, cityguard);
  ASSIGNMOB(5462, cityguard);
  ASSIGNMOB(5463, cityguard);
  ASSIGNMOB(5482, cityguard);
/*
5400 - Guildmaster (Mage)
5401 - Guildmaster (Cleric)
5402 - Guildmaster (Warrior)
5403 - Guildmaster (Thief)
5456 - Guildguard (Mage)
5457 - Guildguard (Cleric)
5458 - Guildguard (Warrior)
5459 - Guildguard (Thief)
*/

  /* ROME */
  /*ASSIGNMOB(12009, magic_user);
  ASSIGNMOB(12018, cityguard);
  ASSIGNMOB(12020, magic_user);
  ASSIGNMOB(12021, cityguard);
  ASSIGNMOB(12025, magic_user);
  ASSIGNMOB(12030, magic_user);
  ASSIGNMOB(12031, magic_user);
  ASSIGNMOB(12032, magic_user);

  /* King Welmar's Castle (not covered in castle.c) */
  /*ASSIGNMOB(15015, thief);      /* Ergan... have a better idea? */
  /*ASSIGNMOB(15032, magic_user); /* Pit Fiend, have something better?  Use it */

  /* DWARVEN KINGDOM */
/*  ASSIGNMOB(6500, cityguard);
  ASSIGNMOB(6502, magic_user);
  ASSIGNMOB(6509, magic_user);
  ASSIGNMOB(6516, magic_user);*/
}



/* assign special procedures to objects */
void assign_objects(void)
{
  ASSIGNOBJ(3004, gen_board);	/* social board */
  ASSIGNOBJ(3048, gen_board);	/* freeze board */
  ASSIGNOBJ(3078, gen_board);	/* immortal board */
  ASSIGNOBJ(3084, gen_board);	/* mortal board */

//  ASSIGNOBJ(3086, bank);	/* atm */
 // ASSIGNOBJ(3036, bank);	/* cashcard */
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;
  int FIRST_ROOM = 0;

/*  ASSIGNROOM(3030, dump);
  ASSIGNROOM(3031, pet_shops);*/

  if (dts_are_dumps)
    for (i = FIRST_ROOM; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	world[i].func = dump;
}
