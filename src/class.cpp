/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"

extern int siteok_everyone;
//extern const int ClasStat[][6];
int  level_exp(int chclass, int chlevel);
int  slot_for_char1(struct char_data *ch, int slotnum);
/* local functions */
int parse_class(int arg);
long find_class_bitvector(char arg);
int thaco(int class_num, int level);
void roll_real_abils(struct char_data * ch, int rollstat);
void do_start(struct char_data * ch, int newbie);
int backstab_mult(int level);
int  invalid_no_class(struct char_data *ch, struct obj_data *obj);
//void obj_to_char(tobj, ch);
void equip_char(struct char_data *ch, struct obj_data *obj, int pos);
byte extend_saving_throws(int class_num, int type, int level);
//extern struct spell_info_type spell_info[];
/* Names first */
int  invalid_anti_class(struct char_data *ch, struct obj_data *obj);
extern struct  spell_create_type spell_create[];
/*const char *class_abbrevs[] = {
  "Клерик",
  "Маг",
  "Вор",
  "Воин",
  "Ассасин",
  "Паладин", // Тамплиер
  "Следопыт",
  "Друид",
  "Монах",
  "Варвар",
  "Ведьмак",
  "\n"
};
  */

const char *pc_class_types[] = {
  "Клерик",
  "Маг",
  "Вор",
  "Воин",
  "Ассасин",
  "Паладин", //ТАмплиер
  "Следопыт",
  "Друид",
  "Монах",
  "Варвар",
  "Ведьмак",
  "\n"
};

const char *pc_race_types[][3] = {
	{"Человек",				"Человек",			"Человек"},
	{"Эльф долины цветов",	"Эльф долины цветов","Эльфийка долины цветов"},	
	{"Гном",				"Гном",				"Гнома"},
	{"Краснолюд",			"Краснолюд",		"Краснолюд"},
	{"Низушек",				"Низушек",			"Низушек"},
	{"Полуэльф",			"Полуэльф",			"Полуэльфийка"},
	{"Зерриканин",			"Зерриканин",		"Зерриканка"},
	{"Лесной эльф",			"Лесной эльф",		"Лесная эльфийка"},
	{"Вампир",				"Вампир",			"Вампирша"}
  };

/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n""   Выбор класса, определяет стиль игры, характер действий, которые\r\n"
"будет предпринимать ваш персонаж в тех или иных ситуациях. Причем стили\r\n"
"различаются настолько, что для вас может быть совершенно неприемлем один,\r\n"
"в то время как другой вы найдете более подходящим.\r\n"
"   Каждая профессия обладает своим уникальным набором в достаточной степени\r\n"
"отличающихся умений и особенностей, требуя к себе различных подходов.\r\n"
"   Тем не менее, если вы начинающий игрок, то познакомиться с миром мы\r\n"
"Вам советуем, выбрав воина-зерриканина.\r\n"
"&W  [0]  Клерик   &n- целитель, зачаровывающий монстров и открывающий порталы.\r\n"
"&W  [1]  Вор      &n- одиночка, идеален для скрытного проникновения и нападения.\r\n"
"&W  [2]  Воин     &n- мастер щита, боевая машина, обученная соотвествующим образом.\r\n"
"&W  [3]  Маг      &n- властелин сфер магии, повелитель разума и загадочный странник.\r\n"
"&W  [4]  Варвар   &n- яростный боец, боевая машина, наносящая большой урон.\r\n"
"&W  [5]  Друид    &n- владыка природы, наделенный целебной и разрушительной магией.\r\n"
"&W  [6]  Следопыт &n- мастер стрельбы, неуловимый путешественник и искатель.\r\n"
"&W  [7]  Паладин  &n- искуссный мечник, полный благородства и знающий магию.\r\n"
"&W  [8]  Ассасин  &n- мастер тихого убийства, познавший это искусство в совершенстве.\r\n"
"&W  [9]  Монах    &n- обладатель каменных рук, все его тело великолепное оружие.\r\n"
"&W  [10] Ведьмак  &n- уникальный боец, обладающий экстраординарными навыками.\r\n" 
"&KЕсли Вы не решили кого выбрать, в игре есть подробная информация о всех классах.\r\n";


/*"\r\n"
"Выберите класс, каким будете играть:\r\n"
"  [0]  Клерик\r\n"
"  [1]  Вор\r\n"
"  [2]  Воин\r\n"
"  [3]  Маг\r\n"
"  [4]  Варвар\r\n"
"  [5]  Друид\r\n"
"  [6]  Следопыт\r\n"
"  [7]  Паладин\r\n"  //Тамплиер
"  [8]  Ассасин\r\n"
"  [9]  Монах\r\n"
"  [10] Ведьмак\r\n"
;
*/

/* The menu for choosing a race in interpreter.c: */
const char *race_menu =
"\r\n"
"   Выбор расы также очень важен. Представители тех или иных рас обладают\r\n"
"большей предрасположенностью к овладению одними профессиями и меньшей к другим.\r\n"
"Это выражается прежде всего в различных стартовых характеристиках, которые\r\n"
"получают представители рас при рождении. Также это может выражаться в различном\r\n"
"количестве &Wтренировочных единициц&n (это характерно для людей). Некоторые\r\n"
"расы обладают врожденной способностью видеть и в инфракрасном спектре, что\r\n"
"позволяет им ориентироваться в темноте.\r\n"
"                            &GСила   Ловкость  Тело   Интеллект Мудрость  Удача&n\r\n"
"&W  [1] Человек            &C 12 - 14  12 - 14  12 - 14  12 - 14  12 - 14  12 - 14&n\r\n"
"&W  [2] Эльф долины цветов &C 10 - 12  10 - 12  10 - 12  14 - 16  16 - 18  13 - 15&n\r\n" 
"&W  [3] Гном               &C 14 - 16  14 - 16  14 - 16  11 - 13   9 - 11  11 - 13&n\r\n"
"&W  [4] Краснолюд          &C 16 - 18   9 - 11  16 - 18  11 - 13  11 - 13  10 - 12&n\r\n"
"&W  [5] Низушек            &C 13 - 15  16 - 18  10 - 12  12 - 14   9 - 11  13 - 15&n\r\n"
"&W  [6] Полуэльф           &C 11 - 13  10 - 12   9 - 11  16 - 18  14 - 16  13 - 15&n\r\n"
"&W  [7] Зерриканин         &C 16 - 18  14 - 16  13 - 15  10 - 12   9 - 11  11 - 13&n\r\n"
"&W  [8] Лесной эльф        &C 13 - 15  13 - 15  11 - 13   9 - 11  14 - 16  13 - 15&n\r\n"
"&KБолее подробную информацию читайте в разделах &WРАСА, СОЗДАНИЕ&n\r\n";




/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(int arg)
{
  
  switch (arg) {
  case 0: return CLASS_CLERIC;
  case 1: return CLASS_THIEF;
  case 2: return CLASS_WARRIOR;
  case 3: return CLASS_MAGIC_USER;
  case 4: return CLASS_VARVAR;
  case 5: return CLASS_DRUID;
  case 6: return CLASS_SLEDOPYT;
  case 7: return CLASS_TAMPLIER;
  case 8: return CLASS_ASSASINE;
  case 9: return CLASS_MONAH;
  case 10:return CLASS_WEDMAK;
  default:return CLASS_UNDEFINED;
  }
}

/* Код для выбора рассы*/

int const parse_race(int arg)
{
  switch (arg) {
  case '1':
    return RACE_HUMAN;
    break;
  case '2':
    return RACE_ELF;
    break;
  case '3':
    return RACE_GNOME;
    break;
  case '4':
    return RACE_DWARF;
    break;
  case '5':
    return RACE_HOBBIT;
    break;
  case '6':
    return RACE_POLUELF;
    break;
  case '7':
    return RACE_OGR;
    break;
  case '8':
    return RACE_LESELF;
    break;
  default:
    return RACE_UNDEFINED;
    break;
  }
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long find_class_bitvector(char arg)
{
	arg = LOWER(arg);

  switch (arg) {
    case 'к': return (1 << CLASS_CLERIC);
    case 'в': return (1 << CLASS_THIEF);
    case 'о': return (1 << CLASS_WARRIOR);
    case 'м': return (1 << CLASS_MAGIC_USER);
    case 'р': return (1 << CLASS_VARVAR);
    case 'д': return (1 << CLASS_DRUID);
    case 'с': return (1 << CLASS_SLEDOPYT);
    case 'п': return (1 << CLASS_TAMPLIER);
    case 'а': return (1 << CLASS_ASSASINE);
    case 'н': return (1 << CLASS_MONAH);
    case 'е': return (1 << CLASS_WEDMAK);
    default:  return 0;
  }
}

/*Выбор битвектора рассы, для того что бы добавить рассы необходимо 
добавить (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5), и так далее*/

long find_race_bitvector(int arg)
{

  switch (arg) {
    case '1':
      return (1 << 0);
      break;
    case '2':
      return (1 << 1);
      break;
    case '3':
      return (1 << 2);
      break;
    case '4':
      return (1 << 3);
      break;
    case '5':
      return (1 << 4);
      break;
    case '6':
      return (1 << 5);
      break;
    case '7':
      return (1 << 6);
      break;
  	default:
      return 0;
      break;
  }
}

/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

void init_spell_levels(void)
{ FILE * magic;
  char line1[256], line2[256], line3[256], line4[256], name[256];
  int  i[10], c, j, sp_num;
  if (!(magic = fopen(LIB_MISC"magic.lst","r")))
     {log("Cann't open magic list file...");
      _exit(1);
  }
  while (get_line(magic,name))
        {if (!name[0] || name[0] == ';')
            continue;
         if (sscanf(name, "%s %s %s %d %d %s",
                      line1, line2, line3,
                      i+1, i+2, line4) != 6)
            {log("Bad format for magic string !\r\n"
                 "Format : <spell name (%%s %%s)> <classes (%%s)> <slot (%%d)> <level (%%d)>");
             _exit(1);
            }

         name[0] = '\0';
         strcat(name,line1);
         if (*line2 != '*')
            {*(name+strlen(name)+1) = '\0';
             *(name+strlen(name)+0) = ' ';
             strcat(name,line2);
            }

         if ((sp_num = find_spell_num(name)) < 0)
            {log("Spell '%s' not found...", name);
             _exit(1);
            }

         for (j = 0; line3[j] && j < NUM_CLASSES; j++)
             {if (!strchr("1xX!",line3[j]))
					  continue;
              if (i[2]>=0)
                 {mspell_level(sp_num, 1 << j, i[2]);//При каком уровне реморта можно использовать заклинашку. 
                 }
              if (i[1])
                 {mspell_slot(sp_num, 1 << j, i[1]);//При каком круге можно использовать заклинашку.
                 }
			  if (*line4>=0) //При како алигменте будут срабатывать заклинашки.
				{ aligment_spell(sp_num, 1 << j, line4);
                 log("ALIGMENT set '%s' classes %x value %s",name,j,line4);
              	}
             }
        }
  fclose(magic);
  if (!(magic = fopen(LIB_MISC"items.lst","r")))
     {log("Cann't open items list file...");
      _exit(1);
     }
  while (get_line(magic,name))
        {if (!name[0] || name[0] == ';')
            continue;
         if (sscanf(name, "%s %s %s %d %d %d %d",
                      line1, line2, line3,
                      i, i+1, i+2, i+3) != 7)
            {log("Bad format for magic string !\r\n"
                 "Format : <spell name (%%s %%s)> <type (%%s)> <items_vnum (%%d %%d %%d %%d)>");
             _exit(1);
            }

         name[0] = '\0';
         strcat(name,line1);
         if (*line2 != '*')
            {*(name+strlen(name)+1) = '\0';
             *(name+strlen(name)+0) = ' ';
             strcat(name,line2);
            }
         if ((sp_num = find_spell_num(name)) < 0)
            {log("Spell '%s' not found...", name);
             _exit(1);
            }
         c = strlen(line3);
         if (!strn_cmp(line3,"potion",c))
            {spell_create[sp_num].potion.items[0] = i[0];
             spell_create[sp_num].potion.items[1] = i[1];
             spell_create[sp_num].potion.items[2] = i[2];
             spell_create[sp_num].potion.rnumber  = i[3];
             log("CREATE potion FOR MAGIC '%s'", spell_name(sp_num));
				}
         else
         if (!strn_cmp(line3,"wand",c))
            {spell_create[sp_num].wand.items[0] = i[0];
             spell_create[sp_num].wand.items[1] = i[1];
             spell_create[sp_num].wand.items[2] = i[2];
             spell_create[sp_num].wand.rnumber  = i[3];
             log("CREATE wand FOR MAGIC '%s'", spell_name(sp_num));
            }
         else
         if (!strn_cmp(line3,"scroll",c))
            {spell_create[sp_num].scroll.items[0] = i[0];
             spell_create[sp_num].scroll.items[1] = i[1];
             spell_create[sp_num].scroll.items[2] = i[2];
             spell_create[sp_num].scroll.rnumber  = i[3];
             log("CREATE scroll FOR MAGIC '%s'", spell_name(sp_num));
            }
         else
         if (!strn_cmp(line3,"items",c))
            {spell_create[sp_num].items.items[0] = i[0];
             spell_create[sp_num].items.items[1] = i[1];
             spell_create[sp_num].items.items[2] = i[2];
             spell_create[sp_num].items.rnumber  = i[3];
             log("CREATE items FOR MAGIC '%s'", spell_name(sp_num));
            }
         else
         if (!strn_cmp(line3,"runes",c))
            {spell_create[sp_num].runes.items[0] = i[0];
             spell_create[sp_num].runes.items[1] = i[1];
             spell_create[sp_num].runes.items[2] = i[2];
             spell_create[sp_num].runes.rnumber  = i[3];
             log("CREATE runes FOR MAGIC '%s'", spell_name(sp_num));
            }
         else
            {log("Unknown items option : %s", line3);
             _exit(1);
            }
        }
  fclose(magic);
  if (!(magic = fopen(LIB_MISC"skills.lst","r")))
     {log("Cann't open skills list file...");
      _exit(1);
     }
  while (get_line(magic,name)) 
        {if (!name[0] || name[0] == ';')
				continue;
         if (sscanf(name, "%s %s %s %d %d %s",
                      line1, line2, line3,
                      i+1, i+2, line4) != 6)
            {log("Bad format for skill string !\r\n"
                 "Format : <skill name (%%s %%s)>  <class (%%d)> <improove (%%d)> !");
             _exit(1);
            }
         name[0] = '\0';
         strcat(name,line1);
         if (*line2 != '*')
            {*(name+strlen(name)+1) = '\0';
             *(name+strlen(name)+0) = ' ';
             strcat(name,line2);
            }
         if ((sp_num = find_skill_num(name)) < 0)
            {log("Skill '%s' not found...", name);
             _exit(1);
            }
         
		 for (j = 0; line3[j] && j < NUM_CLASSES; j++)
             {if (!strchr("1xX!",line3[j]))
					  continue;
			     
			  if (i[1])
				{ mskill_level(sp_num, 1 << j, i[1]);//При каком уровне можно использовать умение.
                }
		      if (i[2]>=0)
				{ mskill_remort(sp_num, 1 << j, i[2]);//При каком уровне реморта можно использовать умение. 
                }
              
			  if (*line4>=0) //алигмент для скиллов
				{ aligment_skill(sp_num, 1 << j, line4);
              	}
             }
         }
  fclose(magic);
  return;
}



#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % считается как изученный полностью "learned" */
/* #define MAX_PER_PRAC		1 Максимальный процент изучения за 1 практику */
/* #define MIN_PER_PRAC		2 Минимальный процент изучения за 1 практику */
/* #define PRAC_TYPE		3 Указывает, заклинание это или скиллы	*/

int prac_params[4][NUM_CLASSES] = {			  //Тамплиер
/*КЛЕРИК МАГ ВОР   ВОИН   ВАРВАР ДРУИД СЛЕДОПЫТ ПАЛАДИН  АССАСИН МОНАХ  ВЕДЬМАК*/
  {55,	 55, 55,	55,     55,   55,      55,     55,      55,    55,	  55},/* learned level */
  {100,	100, 12,	12,     12,  100,      12,     12,      12,    12,	  12},/* max per prac */
  {25,	 25,  0,	0,      0,    25,       0,      0,       0,     0,	  0 },/* min per pac */
  {SPELL,SPELL,SKILL,SKILL,SKILL,SPELL, SKILL,  SKILL,   SKILL, SKILL, SKILL} /* prac name */
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */

int guild_info[][3] = {

/* Шаэрраведд */
  {CLASS_CLERIC,	3683},
  {CLASS_MAGIC_USER,3680},
  {CLASS_THIEF,		3678},
  {CLASS_WARRIOR,	3682},
  {CLASS_ASSASINE,	3670},
  {CLASS_TAMPLIER,	3658},
  {CLASS_SLEDOPYT,	3657},
  {CLASS_DRUID,		3684},
  {CLASS_MONAH,		3659},
  {CLASS_VARVAR,	3685},
  {CLASS_WEDMAK,	3665},

  /* Мад - школа*/
  {CLASS_CLERIC,	1071},
  {CLASS_MAGIC_USER,1065},
  {CLASS_THIEF,		1066},
  {CLASS_WARRIOR,	1068},
  {CLASS_ASSASINE,	1067},
  {CLASS_TAMPLIER,	1070},
  {CLASS_SLEDOPYT,	1069},
  {CLASS_DRUID,		1073},
  {CLASS_MONAH,		1074},
  {CLASS_VARVAR,	1072},
  {CLASS_WEDMAK,	1075},
  /* Гелибол*/
  {CLASS_CLERIC,	9018},
  {CLASS_MAGIC_USER,9017},
  {CLASS_THIEF,		9005},
  {CLASS_WARRIOR,	9006},
  {CLASS_ASSASINE,	9008},
  {CLASS_TAMPLIER,	9014},
  {CLASS_SLEDOPYT,	9012},
  {CLASS_DRUID,		9015},
  {CLASS_MONAH,		9011},
  {CLASS_VARVAR,	9009},
  {CLASS_WEDMAK,	9026},
  /* Крейден*/
  {CLASS_CLERIC,	6225},
  {CLASS_MAGIC_USER,6226},
  {CLASS_THIEF,		6217},
  {CLASS_WARRIOR,	6211},
  {CLASS_ASSASINE,	6212},
  {CLASS_TAMPLIER,	6215},
  {CLASS_SLEDOPYT,	6224},
  {CLASS_DRUID,		6216},
  {CLASS_MONAH,		6220},
  {CLASS_VARVAR,	6222},
  {CLASS_WEDMAK,	6209},
  /* Brass Dragon */
  {-999 /* all */ ,	5065},

/* this must go last -- add new guards above! */
  {-1, -1}
};



/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 *
 * Do not forget to change extern declaration in magic.c if you add to this.
 */

/*byte saving_throws(int class_num, int type, int level)
{ return extend_saving_throws(class_num,type,level);
}
*/
byte extend_saving_throws(int class_num, int type, int level)
{
  switch (class_num)
  {case CLASS_MAGIC_USER:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  100;
		 case  1: return  95;
		 case  2: return  94;
		 case  3: return  93;
		 case  4: return  92;
		 case  5: return  91;
		 case  6: return  90;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  68;
		 case 23: return  66;
		 case 24: return  64;
		 case 25: return  62;
		 case 26: return  60;
		 case 27: return  57;
		 case 28: return  54;
		 case 29: return  50;
		 case 30: return  45;
	     case 31: return  40;
		 case 32: return  35;
		 case 33: return  30;
		 case 34: return  25;
		 case 35: return  20;
		 case 36: return  15;
		 case 37: return  10;
		 case 38: return  5;
		 case 39: return  0;
		 case 40: return  -5;
		 case 41: return  -10;
		 case 42: return  -15;
		 case 43: return  -20;
		 case 44: return  -25;
		 case 45: return  -30;
		 default: return 	0;
		break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  100;
		 case  1: return  95;
		 case  2: return  94;
		 case  3: return  93;
		 case  4: return  92;
		 case  5: return  91;
		 case  6: return  90;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  68;
		 case 23: return  66;
		 case 24: return  64;
		 case 25: return  62;
		 case 26: return  60;
		 case 27: return  57;
		 case 28: return  54;
		 case 29: return  50;
		 case 30: return  45;
		 case 31: return  40;
		 case 32: return  35;
		 case 33: return  30;
		 case 34: return  25;
		 case 35: return  20;
		 case 36: return  15;
		 case 37: return  10;
		 case 38: return  5;
		 case 39: return  0;
		 case 40: return  -5;
		 case 41: return  -10;
		 case 42: return  -15;
		 case 43: return  -20;
		 case 44: return  -25;
		 case 45: return  -30;
		 default: return 	0;
		break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  100;
		 case  1: return  95;
		 case  2: return  94;
		 case  3: return  93;
		 case  4: return  92;
		 case  5: return  91;
		 case  6: return  90;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  68;
		 case 23: return  66;
		 case 24: return  64;
		 case 25: return  62;
		 case 26: return  60;
		 case 27: return  57;
		 case 28: return  54;
		 case 29: return  50;
		 case 30: return  45;
		 case 31: return  40;
		 case 32: return  35;
		 case 33: return  30;
		 case 34: return  25;
		 case 35: return  20;
		 case 36: return  15;
		 case 37: return  10;
		 case 38: return  5;
		 case 39: return  0;
		 case 40: return  -5;
		 case 41: return  -10;
		 case 42: return  -15;
		 case 43: return  -20;
		 case 44: return  -25;
		 case 45: return  -30;
		 default: return 	0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 100;
		 case  1: return 99;
		 case  2: return 99;
		 case  3: return 98;
		 case  4: return 98;
		 case  5: return 97;
		 case  6: return 97;
		 case  7: return 96;
		 case  8: return 96;
		 case  9: return 95;
		 case 10: return 95;
		 case 11: return 94;
		 case 12: return 94;
		 case 13: return 93;
		 case 14: return 93;
		 case 15: return 92;
		 case 16: return 92;
		 case 17: return 91;
		 case 18: return 91;
		 case 19: return 90;
		 case 20: return 90;
		 case 21: return 89;
		 case 22: return 89;
         case 23: return 88;
         case 24: return 88;
         case 25: return 87;
         case 26: return 87;
         case 27: return 86;
         case 28: return 86;
		 case 29: return 85;
		 case 30: return 85;
	     case 31: return 84;
		 case 32: return 83;
		 case 33: return 82;
		 case 34: return 81;
		 case 35: return 80;
		 case 36: return 78;
		 case 37: return 76;
		 case 38: return 74;
		 case 39: return 72;
		 case 40: return 70;
		 case 41: return 66;
		 case 42: return 63;
		 case 43: return 60;
		 case 44: return 57;
		 case 45: return 54;
		 default: return 0;
	     break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 case 31: return  13;
		 case 32: return  8;
		 case 33: return  1;
		 case 34: return  -6;
		 case 35: return  -13;
		 case 36: return  -20;
		 case 37: return  -27;
		 case 38: return  -35;
		 case 39: return  -42;
		 case 40: return  -49;
		 case 41: return  -56;
		 case 42: return  -63;
		 case 43: return  -70;
		 case 44: return  -80;
		 case 45: return  -90;
		 default: return 	0;
		break;
		}
	 case SAVING_BASIC:  // Protect from skills
		switch (level)
      {case  0: return  100;
	   case  1: return  100;
       case  2: return  100;
	   case  3: return  100;
       case  4: return  99;
       case  5: return  99;
       case  6: return  99;
       case  7: return  99;
       case  8: return  99;
       case  9: return  98;
       case 10: return  98;
	   case 11: return  98;
       case 12: return  98;
       case 13: return  97;
       case 14: return  97;
       case 15: return  97;
       case 16: return  96;
       case 17: return  96;
       case 18: return  96;
       case 19: return  95;
       case 20: return  95;
       case 21: return  94;
       case 22: return  94;
       case 23: return  93;
       case 24: return  93;
       case 25: return  92;
       case 26: return  92;
       case 27: return  91;
	   case 28: return  91;
       case 29: return  90;
       case 30: return  90;
       case 31: return  89;
		 case 32: return  88;
		 case 33: return  87;
		 case 34: return  86;
		 case 35: return  85;
		 case 36: return  84;
		 case 37: return  83;
		 case 38: return  82;
		 case 39: return  81;
		 case 40: return  80;
		 case 41: return  79;
		 case 42: return  78;
		 case 43: return  77;
		 case 44: return  76;
		 case 45: return  75;
		 default: return 	0;
 	   break;
      }
	 default:
		log("SYSERR: Invalid saving throw type.");
		break;
						}
	 break;

  case CLASS_CLERIC:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  79;
		 case 16: return  77;
		 case 17: return  75;
		 case 18: return  73;
		 case 19: return  71;
		 case 20: return  68;
		 case 21: return  65;
		 case 22: return  62;
		 case 23: return  59;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  35;
		 case 30: return  30;
		 case 31: return  25;
		 case 32: return  20;
		 case 33: return  15;
		 case 34: return  10;
		 case 35: return  5;
		 case 36: return  0;
		 case 37: return  -5;
		 case 38: return  -10;
		 case 39: return  -15;
		 case 40: return  -20;
		 case 41: return  -25;
		 case 42: return  -30;
		 case 43: return  -35;
		 case 44: return  -40;
		 case 45: return  -45;
		 default: return 	0;
		break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  100;
		 case  1: return  95;
		 case  2: return  94;
		 case  3: return  93;
		 case  4: return  92;
		 case  5: return  91;
		 case  6: return  90;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  68;
		 case 23: return  66;
		 case 24: return  64;
		 case 25: return  62;
		 case 26: return  60;
		 case 27: return  57;
		 case 28: return  54;
		 case 29: return  50;
		 case 30: return  45;
	     case 31: return  40;
		 case 32: return  35;
		 case 33: return  30;
		 case 34: return  25;
		 case 35: return  20;
		 case 36: return  15;
		 case 37: return  10;
		 case 38: return  5;
		 case 39: return  0;
		 case 40: return  -5;
		 case 41: return  -10;
		 case 42: return  -15;
		 case 43: return  -20;
		 case 44: return  -25;
		 case 45: return  -30;
		 default: return 	0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
	     case 31: return  40;
		 case 32: return  35;
		 case 33: return  30;
		 case 34: return  25;
		 case 35: return  20;
		 case 36: return  15;
		 case 37: return  10;
		 case 38: return  5;
		 case 39: return  0;
		 case 40: return  -5;
		 case 41: return  -10;
		 case 42: return  -15;
		 case 43: return  -20;
		 case 44: return  -25;
		 case 45: return  -30;
		 default: return  0;
		 break;
      }
    case SAVING_BREATH:	// Breath weapons
      switch (level)
      {case  0: return 100;
		 case  1: return 99;
		 case  2: return 99;
		 case  3: return 98;
		 case  4: return 98;
		 case  5: return 97;
		 case  6: return 97;
		 case  7: return 96;
		 case  8: return 96;
		 case  9: return 95;
		 case 10: return 95;
		 case 11: return 94;
		 case 12: return 94;
		 case 13: return 93;
		 case 14: return 93;
		 case 15: return 92;
		 case 16: return 92;
		 case 17: return 91;
		 case 18: return 91;
		 case 19: return 90;
		 case 20: return 90;
		 case 21: return 89;
		 case 22: return 89;
         case 23: return 88;
         case 24: return 88;
         case 25: return 87;
         case 26: return 87;
         case 27: return 86;
         case 28: return 86;
		 case 29: return 85;
		 case 30: return 85;
	     case 31: return 84;
		 case 32: return 83;
		 case 33: return 82;
		 case 34: return 81;
		 case 35: return 80;
		 case 36: return 78;
		 case 37: return 76;
		 case 38: return 74;
		 case 39: return 72;
		 case 40: return 70;
		 case 41: return 66;
		 case 42: return 63;
		 case 43: return 60;
		 case 44: return 57;
		 case 45: return 54;
		 default: return 	0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  71;
		 case 21: return  68;
		 case 22: return  64;
		 case 23: return  60;
		 case 24: return  56;
		 case 25: return  52;
		 case 26: return  48;
		 case 27: return  44;
		 case 28: return  40;
		 case 29: return  36;
		 case 30: return  32;
		 case 31: return  28;
		 case 32: return  24;
		 case 33: return  20;
		 case 34: return  15;
		 case 35: return  10;
		 case 36: return  5;
		 case 37: return  0;
		 case 38: return  -5;
		 case 39: return  -10;
		 case 40: return  -15;
		 case 41: return  -20;
		 case 42: return  -25;
		 case 43: return  -30;
		 case 44: return  -35;
		 case 45: return  -40;
		 default: return  0;
		}
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {case  0: return  100;
	   case  1: return  100;
       case  2: return  100;
	   case  3: return  100;
       case  4: return  99;
       case  5: return  99;
       case  6: return  99;
       case  7: return  98;
       case  8: return  98;
       case  9: return  98;
       case 10: return  97;
	   case 11: return  97;
       case 12: return  97;
       case 13: return  96;
       case 14: return  96;
       case 15: return  95;
       case 16: return  95;
       case 17: return  94;
       case 18: return  94;
       case 19: return  93;
       case 20: return  93;
       case 21: return  92;
       case 22: return  92;
       case 23: return  91;
       case 24: return  91;
       case 25: return  90;
       case 26: return  90;
       case 27: return  89;
	   case 28: return  88;
       case 29: return  87;
       case 30: return  86;
		 case 31: return  85;
		 case 32: return  84;
		 case 33: return  83;
		 case 34: return  82;
		 case 35: return  81;
		 case 36: return  80;
		 case 37: return  79;
		 case 38: return  78;
		 case 39: return  77;
		 case 40: return  76;
		 case 41: return  75;
		 case 42: return  74;
		 case 43: return  73;
		 case 44: return  72;
		 case 45: return  70;
		 default: return 	0;
 	   break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
						}
	 break;
  case CLASS_DRUID:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  83;
		 case 13: return  82;
		 case 14: return  80;
		 case 15: return  79;
		 case 16: return  76;
		 case 17: return  74;
		 case 18: return  72;
		 case 19: return  69;
		 case 20: return  66;
		 case 21: return  63;
		 case 22: return  60;
		 case 23: return  57;
		 case 24: return  53;
		 case 25: return  49;
		 case 26: return  45;
		 case 27: return  40;
		 case 28: return  35;
		 case 29: return  30;
		 case 30: return  25;
		 case 31: return  20;
		 case 32: return  15;
		 case 33: return  10;
		 case 34: return  5;
		 case 35: return  0;
		 case 36: return  -5;
		 case 37: return  -10;
		 case 38: return  -15;
		 case 39: return  -20;
		 case 40: return  -25;
		 case 41: return  -30;
		 case 42: return  -35;
		 case 43: return  -40;
		 case 44: return  -45;
		 case 45: return  -50;
		 default: return 	0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  100;
		 case  1: return  95;
		 case  2: return  94;
		 case  3: return  93;
		 case  4: return  92;
		 case  5: return  91;
		 case  6: return  90;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  68;
		 case 23: return  66;
		 case 24: return  64;
		 case 25: return  62;
		 case 26: return  60;
		 case 27: return  57;
		 case 28: return  54;
		 case 29: return  50;
		 case 30: return  45;
	     case 31: return  40;
		 case 32: return  35;
		 case 33: return  30;
		 case 34: return  25;
		 case 35: return  20;
		 case 36: return  15;
		 case 37: return  10;
		 case 38: return  5;
		 case 39: return  0;
		 case 40: return  -5;
		 case 41: return  -10;
		 case 42: return  -15;
		 case 43: return  -20;
		 case 44: return  -25;
		 case 45: return  -30;
		 
		 default: return  0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  83;
		 case 13: return  82;
		 case 14: return  80;
		 case 15: return  79;
		 case 16: return  76;
		 case 17: return  74;
		 case 18: return  72;
		 case 19: return  69;
		 case 20: return  66;
		 case 21: return  63;
		 case 22: return  60;
		 case 23: return  57;
		 case 24: return  53;
		 case 25: return  49;
		 case 26: return  45;
		 case 27: return  40;
		 case 28: return  35;
		 case 29: return  30;
		 case 30: return  25;
		 case 31: return  40;
		 case 32: return  35;
		 case 33: return  30;
		 case 34: return  25;
		 case 35: return  20;
		 case 36: return  15;
		 case 37: return  10;
		 case 38: return  5;
		 case 39: return  0;
		 case 40: return  -5;
		 case 41: return  -10;
		 case 42: return  -15;
		 case 43: return  -20;
		 case 44: return  -25;
		 case 45: return  -30;
		 default: return 	0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 100;
		 case  1: return 99;
		 case  2: return 99;
		 case  3: return 98;
		 case  4: return 98;
		 case  5: return 97;
		 case  6: return 97;
		 case  7: return 96;
		 case  8: return 96;
		 case  9: return 95;
		 case 10: return 95;
		 case 11: return 94;
		 case 12: return 94;
		 case 13: return 93;
		 case 14: return 93;
		 case 15: return 92;
		 case 16: return 92;
		 case 17: return 91;
		 case 18: return 91;
		 case 19: return 90;
		 case 20: return 90;
		 case 21: return 89;
		 case 22: return 89;
         case 23: return 88;
         case 24: return 88;
         case 25: return 87;
         case 26: return 87;
         case 27: return 86;
         case 28: return 86;
		 case 29: return 85;
		 case 30: return 85;
	     case 31: return 84;
		 case 32: return 83;
		 case 33: return 82;
		 case 34: return 81;
		 case 35: return 80;
		 case 36: return 78;
		 case 37: return 76;
		 case 38: return 74;
		 case 39: return 72;
		 case 40: return 70;
		 case 41: return 66;
		 case 42: return 63;
		 case 43: return 60;
		 case 44: return 57;
		 case 45: return 54;
		 default: return 	0;
	   
       break;
      }
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  83;
		 case 13: return  82;
		 case 14: return  80;
		 case 15: return  79;
		 case 16: return  76;
		 case 17: return  74;
		 case 18: return  72;
		 case 19: return  69;
		 case 20: return  66;
		 case 21: return  63;
		 case 22: return  60;
		 case 23: return  57;
		 case 24: return  53;
		 case 25: return  49;
		 case 26: return  45;
		 case 27: return  40;
		 case 28: return  35;
		 case 29: return  30;
		 case 30: return  25;
		 case 31: return  20;
		 case 32: return  15;
		 case 33: return  10;
		 case 34: return  5;
		 case 35: return  0;
		 case 36: return  -5;
		 case 37: return  -10;
		 case 38: return  -15;
		 case 39: return  -20;
		 case 40: return  -25;
		 case 41: return  -30;
		 case 42: return  -35;
		 case 43: return  -40;
		 case 44: return  -45;
		 case 45: return  -50;
		 default: return 	0;
      }
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {case  0: return  100;
	   case  1: return  100;
       case  2: return  100;
	   case  3: return  100;
       case  4: return  99;
       case  5: return  99;
       case  6: return  99;
       case  7: return  98;
       case  8: return  98;
       case  9: return  98;
       case 10: return  97;
	   case 11: return  97;
       case 12: return  97;
       case 13: return  96;
       case 14: return  96;
       case 15: return  95;
       case 16: return  95;
       case 17: return  94;
       case 18: return  94;
       case 19: return  93;
       case 20: return  93;
       case 21: return  92;
       case 22: return  92;
       case 23: return  91;
       case 24: return  91;
       case 25: return  90;
       case 26: return  90;
       case 27: return  89;
	   case 28: return  88;
       case 29: return  87;
       case 30: return  86;
       	 case 31: return  85;
		 case 32: return  84;
		 case 33: return  83;
		 case 34: return  82;
		 case 35: return  81;
		 case 36: return  80;
		 case 37: return  79;
		 case 38: return  78;
		 case 39: return  77;
		 case 40: return  76;
		 case 41: return  75;
		 case 42: return  74;
		 case 43: return  73;
		 case 44: return  72;
		 case 45: return  70;
		 default: return 	0;
 	   break;
		}
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
                  }
    break;
  case CLASS_THIEF:
  case CLASS_ASSASINE:
  //case CLASS_MERCHANT:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 case 31: return  36;
		 case 32: return  32;
		 case 33: return  28;
		 case 34: return  24;
		 case 35: return  20;
		 case 36: return  16;
		 case 37: return  12;
		 case 38: return  8;
		 case 39: return  4;
		 case 40: return  0;
		 case 41: return  -4;
		 case 42: return  -8;
		 case 43: return  -12;
		 case 44: return  -16;
		 case 45: return  -20;
		 default: return 	0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  89;
		 case  4: return  88;
		 case  5: return  87;
		 case  6: return  86;
		 case  7: return  85;
		 case  8: return  84;
		 case  9: return  83;
		 case 10: return  82;
		 case 11: return  81;
		 case 12: return  80;
		 case 13: return  79;
		 case 14: return  77;
		 case 15: return  74;
		 case 16: return  71;
		 case 17: return  68;
		 case 18: return  64;
		 case 19: return  60;
		 case 20: return  56;
		 case 21: return  52;
		 case 22: return  48;
		 case 23: return  44;
		 case 24: return  40;
		 case 25: return  36;
		 case 26: return  32;
		 case 27: return  28;
		 case 28: return  24;
		 case 29: return  20;
		 case 30: return  16;
	     case 31: return  10;
		 case 32: return  4;
		 case 33: return  -2;
		 case 34: return  -8;
		 case 35: return  -14;
		 case 36: return  -20;
		 case 37: return  -27;
		 case 38: return  -34;
		 case 39: return  -31;
		 case 40: return  -28;
		 case 41: return  -35;
		 case 42: return  -42;
		 case 43: return  -49;
		 case 44: return  -56;
		 case 45: return  -63;
		 default: return 	0;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
		 case 31: return  40;
		 case 32: return  35;
		 case 33: return  30;
		 case 34: return  25;
		 case 35: return  20;
		 case 36: return  15;
		 case 37: return  10;
		 case 38: return  5;
		 case 39: return  0;
		 case 40: return  -5;
		 case 41: return  -10;
		 case 42: return  -15;
		 case 43: return  -20;
		 case 44: return  -25;
		 case 45: return  -30;
		 default: return  0;
       break;
      }
    case SAVING_BREATH:	// Breath weapons
      switch (level)
      {  case  0: return 99;
		 case  1: return 90;
         case  2: return 89;
         case  3: return 88;
		 case  4: return 87;
		 case  5: return 86;
		 case  6: return 85;
		 case  7: return 84;
		 case  8: return 83;
		 case  9: return 82;
		 case 10: return 81;
		 case 11: return 80;
		 case 12: return 79;
		 case 13: return 78;
		 case 14: return 77;
		 case 15: return 76;
		 case 16: return 75;
		 case 17: return 74;
		 case 18: return 73;
		 case 19: return 72;
		 case 20: return 71;
		 case 21: return 70;
		 case 22: return 69;
		 case 23: return 68;
		 case 24: return 67;
		 case 25: return 66;
		 case 26: return 65;
		 case 27: return 64;
		 case 28: return 63;
		 case 29: return 62;
		 case 30: return 61;
		 case 31: return 60;
		 case 32: return 59;
		 case 33: return 58;
		 case 34: return 57;
		 case 35: return 56;
		 case 36: return 54;
		 case 37: return 52;
		 case 38: return 50;
		 case 39: return 48;
		 case 40: return 46;
		 case 41: return 44;
		 case 42: return 42;
		 case 43: return 40;
		 case 44: return 38;
		 case 45: return 35;
		 default: return  0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  86;
		 case 12: return  85;
		 case 13: return  84;
		 case 14: return  83;
		 case 15: return  81;
		 case 16: return  80;
		 case 17: return  78;
		 case 18: return  76;
		 case 19: return  74;
		 case 20: return  72;
		 case 21: return  70;
		 case 22: return  67;
		 case 23: return  64;
		 case 24: return  61;
		 case 25: return  58;
		 case 26: return  55;
		 case 27: return  52;
		 case 28: return  48;
		 case 29: return  44;
		 case 30: return  40;
	     case 31: return  35;
		 case 32: return  30;
		 case 33: return  25;
		 case 34: return  20;
		 case 35: return  15;
		 case 36: return  10;
		 case 37: return  5;
		 case 38: return  0;
		 case 39: return  -5;
		 case 40: return  -10;
		 case 41: return  -15;
		 case 42: return  -20;
		 case 43: return  -25;
		 case 44: return  -30;
		 case 45: return  -35;
		 
		 default: return  0;
		 break;
		}
	 case SAVING_BASIC:  // Protect from skills
		switch (level)
		{case  0: return  100;
		 case  1: return  99;
		 case  2: return  98;
		 case  3: return  97;
		 case  4: return  96;
		 case  5: return  95;
		 case  6: return  94;
		 case  7: return  93;
		 case  8: return  92;
		 case  9: return  91;
		 case 10: return  90;
		 case 11: return  89;
		 case 12: return  88;
		 case 13: return  87;
		 case 14: return  86;
		 case 15: return  85;
		 case 16: return  84;
		 case 17: return  83;
		 case 18: return  82;
		 case 19: return  81;
		 case 20: return  80;
		 case 21: return  79;
		 case 22: return  78;
		 case 23: return  77;
		 case 24: return  76;
		 case 25: return  75;
		 case 26: return  74;
		 case 27: return  73;
		 case 28: return  72;
		 case 29: return  71;
		 case 30: return  70;
		 case 31: return  74;
		 case 32: return  73;
		 case 33: return  72;
		 case 34: return  71;
		 case 35: return  70;
		 case 36: return  69;
		 case 37: return  68;
		 case 38: return  67;
		 case 39: return  66;
		 case 40: return  65;
		 case 41: return  64;
		 case 42: return  63;
		 case 43: return  62;
		 case 44: return  61;
		 case 45: return  60;
		 default : return 0;
		break;
		}
	 default:
		log("SYSERR: Invalid saving throw type.");
		break;
						}
	 break;
  case CLASS_WARRIOR:
  case CLASS_VARVAR:
  case CLASS_WEDMAK:
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  100;
		 case  1: return  99;
		 case  2: return  98;
		 case  3: return  97;
		 case  4: return  96;
		 case  5: return  95;
		 case  6: return  94;
		 case  7: return  93;
		 case  8: return  92;
		 case  9: return  90;
		 case 10: return  88;
		 case 11: return  86;
		 case 12: return  84;
		 case 13: return  82;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  72;
		 case 18: return  69;
		 case 19: return  66;
		 case 20: return  63;
		 case 21: return  60;
		 case 22: return  56;
		 case 23: return  52;
		 case 24: return  48;
		 case 25: return  44;
		 case 26: return  40;
		 case 27: return  36;
		 case 28: return  32;
		 case 29: return  28;
		 case 30: return  24;
		 case 31: return  20;
		 case 32: return  15;
		 case 33: return  10;
		 case 34: return  5;
		 case 35: return  0;
		 case 36: return  -5;
		 case 37: return  -10;
		 case 38: return  -15;
		 case 39: return  -20;
		 case 40: return  -25;
		 case 41: return  -30;	// Some mobiles
		 case 42: return  -35;
		 case 43: return  -40;
		 case 44: return  -45;
		 case 45: return  -50;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  90;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  89;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  87;
		 case 12: return  86;
		 case 13: return  85;
		 case 14: return  84;
		 case 15: return  83;
		 case 16: return  82;
		 case 17: return  80;
		 case 18: return  79;
		 case 19: return  77;
		 case 20: return  75;
		 case 21: return  74;
		 case 22: return  72;
		 case 23: return  69;
		 case 24: return  67;
		 case 25: return  65;
		 case 26: return  62;
		 case 27: return  59;
		 case 28: return  56;
		 case 29: return  53;
		 case 30: return  38;
		 case 31: return  33;
		 case 32: return  27;
		 case 33: return  21;
		 case 34: return  15;
		 case 35: return  10;
		 case 36: return  5;
		 case 37: return  0;
		 case 38: return  -5;
		 case 39: return  -10;
		 case 40: return  -15;
		 case 41: return  -20;	// Some mobiles
		 case 42: return  -25;
		 case 43: return  -30;
		 case 44: return  -35;
		 case 45: return  -40;
		 case 46: return  -45;
		 case 47: return  -50;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  90;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  89;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  87;
		 case 12: return  86;
		 case 13: return  85;
		 case 14: return  84;
		 case 15: return  83;
		 case 16: return  82;
		 case 17: return  80;
		 case 18: return  79;
		 case 19: return  77;
		 case 20: return  75;
		 case 21: return  74;
		 case 22: return  72;
		 case 23: return  69;
		 case 24: return  67;
		 case 25: return  65;
		 case 26: return  62;
		 case 27: return  59;
		 case 28: return  56;
		 case 29: return  53;
		 case 30: return  50;
		 case 31: return  45;
		 case 32: return  40;
		 case 33: return  35;
		 case 34: return  30;
		 case 35: return  25;
		 case 36: return  20;
		 case 37: return  15;
		 case 38: return  10;
		 case 39: return   5;
		 case 40: return   4;
		 case 41: return   3;	// Some mobiles
		 case 42: return   2;
		 case 43: return   1;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 100;
		 case  1: return 99;
		 case  2: return 98;
		 case  3: return 97;
		 case  4: return 96;
		 case  5: return 95;
		 case  6: return 94;
		 case  7: return 93;
		 case  8: return 92;
		 case  9: return 91;
		 case 10: return 90;
		 case 11: return 88;
		 case 12: return 87;
		 case 13: return 85;
		 case 14: return 84;
		 case 15: return 82;
		 case 16: return 81;
		 case 17: return 79;
		 case 18: return 78;
		 case 19: return 76;
		 case 20: return 74;
		 case 21: return 72;
		 case 22: return 70;
		 case 23: return 68;
		 case 24: return 66;
		 case 25: return 64;
		 case 26: return 61;
		 case 27: return 58;
		 case 28: return 55;
		 case 29: return 52;
		 case 30: return 49;
		 case 31: return 46;
		 case 32: return 43;
		 case 33: return 40;
		 case 34: return 37;
		 case 35: return 34;
		 case 36: return 31;
		 case 37: return 28;
		 case 38: return 25;
		 case 39: return 22;
		 case 40: return 16;
		 case 41: return 14;
		 case 42: return 10;
		 case 43: return  6;
		 case 44: return  2;
		 case 45: return  0;
		 default: return  0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  90;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  89;
		 case  9: return  88;
		 case 10: return  87;
		 case 11: return  87;
		 case 12: return  86;
		 case 13: return  85;
		 case 14: return  84;
		 case 15: return  83;
		 case 16: return  82;
		 case 17: return  80;
		 case 18: return  79;
		 case 19: return  77;
		 case 20: return  75;
		 case 21: return  74;
		 case 22: return  72;
		 case 23: return  69;
		 case 24: return  67;
		 case 25: return  65;
		 case 26: return  62;
		 case 27: return  59;
		 case 28: return  56;
		 case 29: return  53;
		 case 30: return  50;
		 case 31: return  47;
		 case 32: return  44;
		 case 33: return  41;
		 case 34: return  38;
		 case 35: return  35;
		 case 36: return  32;
		 case 37: return  28;
		 case 38: return  24;
		 case 39: return  20;
		 case 40: return  16;
		 case 41: return  12;	// Some mobiles
		 case 42: return   8;
		 case 43: return   4;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
      }
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {  case  0: return  100;
		 case  1: return  99;
         case  2: return  97;
         case  3: return  96;
         case  4: return  95;
         case  5: return  94;
         case  6: return  92;
         case  7: return  91;
         case  8: return  89;
         case  9: return  88;
         case 10: return  86;
         case 11: return  85;
         case 12: return  84;
         case 13: return  83;
         case 14: return  81;
		 case 15: return  80;
         case 16: return  79;
	  	 case 17: return  77;
         case 18: return  76;
         case 19: return  75;
         case 20: return  74;
         case 21: return  72;
         case 22: return  71;
         case 23: return  70;
         case 24: return  68;
		 case 25: return  65;
         case 26: return  64;
		 case 27: return  63;
         case 28: return  62;
	 	 case 29: return  61;
         case 30: return  60;
         case 31: return  58;
         case 32: return  57;
         case 33: return  56;
         case 34: return  54;
         case 35: return  52;
         case 36: return  51;
         case 37: return  49;
		 case 38: return  47;
         case 39: return  46;
		 case 40: return  45;
		 case 41: return  43;
		 case 42: return  42;
		 case 43: return  41;
		 case 44: return  39;
		 case 45: return  37;
		 case 46: return  35;
		 case 47: return  34;
		 case 48: return  32;
		 case 49: return  31;
		 default: return  30;
		break;
		}
	 default:
		log("SYSERR: Invalid saving throw type.");
		break;
	 }
  case CLASS_SLEDOPYT:
  case CLASS_TAMPLIER: // -> Паладин
  case CLASS_MONAH:	// дружинники
	 switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  85;
		 case 11: return  83;
		 case 12: return  81;
		 case 13: return  79;
		 case 14: return  76;
		 case 15: return  73;
		 case 16: return  70;
		 case 17: return  67;
		 case 18: return  64;
		 case 19: return  61;
		 case 20: return  58;
		 case 21: return  55;
		 case 22: return  52;
		 case 23: return  49;
		 case 24: return  46;
		 case 25: return  43;
		 case 26: return  40;
		 case 27: return  37;
		 case 28: return  33;
		 case 29: return  29;
		 case 30: return  25;
	     case 31: return  21;
		 case 32: return  27;
		 case 33: return  23;
		 case 34: return  19;
		 case 35: return  15;
		 case 36: return  11;
		 case 37: return  7;
		 case 38: return  3;
		 case 39: return  -1;
		 case 40: return  -5;
		 case 41: return  -9;
		 case 42: return  -13;
		 case 43: return  -17;
		 case 44: return  -21;
		 case 45: return  -25;
		 default: return 	0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  86;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  80;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  75;
		 case 19: return  72;
		 case 20: return  70;
		 case 21: return  67;
		 case 22: return  65;
		 case 23: return  62;
		 case 24: return  59;
		 case 25: return  55;
		 case 26: return  52;
		 case 27: return  48;
		 case 28: return  44;
		 case 29: return  39;
		 case 30: return  35;
	     case 31: return  25;
		 case 32: return  20;
		 case 33: return  15;
		 case 34: return  10;
		 case 35: return  5;
		 case 36: return  0;
		 case 37: return  -5;
		 case 38: return  -10;
		 case 39: return  -15;
		 case 40: return  -20;
		 case 41: return  -25;
		 case 42: return  -30;
		 case 43: return  -35;
		 case 44: return  -40;
		 case 45: return  -45;
		 default: return 	0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  86;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  80;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  75;
		 case 19: return  72;
		 case 20: return  70;
		 case 21: return  67;
		 case 22: return  65;
		 case 23: return  62;
		 case 24: return  59;
		 case 25: return  55;
		 case 26: return  52;
		 case 27: return  48;
		 case 28: return  44;
		 case 29: return  39;
		 case 30: return  35;
	     case 31: return  35;
		 case 32: return  30;
		 case 33: return  25;
		 case 34: return  20;
		 case 35: return  15;
		 case 36: return  10;
		 case 37: return  5;
		 case 38: return  0;
		 case 39: return  -5;
		 case 40: return  -10;
		 case 41: return  -15;
		 case 42: return  -20;
		 case 43: return  -25;
		 case 44: return  -30;
		 case 45: return  -35;
		 default: return  0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 100;
		 case  1: return 99;
       	 case  2: return 98;
       	 case  3: return 97;
       	 case  4: return 96;
       	 case  5: return 95;
       	 case  6: return 94;
       	 case  7: return 93;
       	 case  8: return 92;
       	 case  9: return 91;
       	 case 10: return 90;
       	 case 11: return 89;
       	 case 12: return 88;
       	 case 13: return 87;
       	 case 14: return 86;
       	 case 15: return 85;
       	 case 16: return 84;
       	 case 17: return 83;
       	 case 18: return 82;
	   	 case 19: return 80;
       	 case 20: return 78;
       	 case 21: return 76;
       	 case 22: return 74;
       	 case 23: return 72;
       	 case 24: return 70;
       	 case 25: return 68;
	   	 case 26: return 66;
       	 case 27: return 64;
       	 case 28: return 62;
       	 case 29: return 60;
	   	 case 30: return 58;
	     case 31: return 55;
		 case 32: return 52;
		 case 33: return 49;
		 case 34: return 46;
		 case 35: return 43;
		 case 36: return 40;
		 case 37: return 37;
		 case 38: return 34;
		 case 39: return 31;
		 case 40: return 28;
		 case 41: return 25;
		 case 42: return 22;
		 case 43: return 18;
		 case 44: return 14;
		 case 45: return 10;
       default: return 0;
       break;
      }
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  89;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  86;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  80;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  75;
		 case 19: return  72;
		 case 20: return  70;
		 case 21: return  67;
		 case 22: return  65;
		 case 23: return  62;
		 case 24: return  59;
		 case 25: return  55;
		 case 26: return  52;
		 case 27: return  48;
		 case 28: return  44;
		 case 29: return  39;
		 case 30: return  35;
	     case 31: return  30;
		 case 32: return  25;
		 case 33: return  20;
		 case 34: return  15;
		 case 35: return  10;
		 case 36: return  5;
		 case 37: return  0;
		 case 38: return  5;
		 case 39: return  -10;
		 case 40: return  -15;
		 case 41: return  -20;
		 case 42: return  -25;
		 case 43: return  -30;
		 case 44: return  -35;
		 case 45: return  -40;
		 default: return 	0;
		 break;
		}
	 case SAVING_BASIC:  // Protect from skills
		switch (level)
		{case  0: return  100;
		 case  1: return  99;
		 case  2: return  97;
		 case  3: return  96;
		 case  4: return  95;
		 case  5: return  94;
         case  6: return  92;
		 case  7: return  91;
         case  8: return  89;
         case  9: return  88;
         case 10: return  86;
         case 11: return  84;
         case 12: return  82;
         case 13: return  80;
         case 14: return  78;
         case 15: return  76;
         case 16: return  74;
		 case 17: return  72;
         case 18: return  70;
         case 19: return  68;
         case 20: return  64;
         case 21: return  62;
         case 22: return  60;
         case 23: return  58;
         case 24: return  56;
         case 25: return  54;
         case 26: return  52;
         case 27: return  50;
         case 28: return  49;
         case 29: return  47;
         case 30: return  45;
 	     case 31: return  42;
		 case 32: return  39;
		 case 33: return  36;
		 case 34: return  33;
		 case 35: return  30;
		 case 36: return  27;
		 case 37: return  24;
		 case 38: return  21;
		 case 39: return  18;
		 case 40: return  15;
		 case 41: return  12;
		 case 42: return  8;
		 case 43: return  4;
		 case 44: return  0;
		 case 45: return  0;
         default: return  0;
       break;
      }
    default:
		log("SYSERR: Invalid saving throw type.");
      break;
    }

default: // All NPC's
    switch (type) {
	 case SAVING_PARA:	// Paralyzation
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  88;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  85;
		 case 12: return  84;
		 case 13: return  83;
		 case 14: return  82;
		 case 15: return  81;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  75;
		 case 19: return  73;
		 case 20: return  71;
		 case 21: return  68;
		 case 22: return  65;
		 case 23: return  61;
		 case 24: return  56;
		 case 25: return  50;
		 case 26: return  43;
		 case 27: return  36;
		 case 28: return  29;
		 case 29: return  22;
		 case 30: return  15;
		 case 31: return 	14;
		 case 32: return 	13;
		 case 33: return   12;
		 case 34: return   11;
		 case 35: return   10;
		 case 36: return   6;
		 case 37: return   5;
		 case 38: return   4;
		 case 39: return   3;
		 case 40: return   2;
		 case 41: return   1;	// Some mobiles
		 case 42: return   0;
		 case 43: return   0;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_ROD:	// Rods
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 case 31: return 	15;
		 case 32: return 	10;
		 case 33: return   9;
		 case 34: return   8;
		 case 35: return   7;
		 case 36: return   6;
		 case 37: return   5;
		 case 38: return   4;
		 case 39: return   3;
		 case 40: return   2;
		 case 41: return   1;	// Some mobiles
		 case 42: return   0;
		 case 43: return   0;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_PETRI:	// Petrification
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 case 31: return 	15;
		 case 32: return 	10;
		 case 33: return   9;
		 case 34: return   8;
		 case 35: return   7;
		 case 36: return   6;
		 case 37: return   5;
		 case 38: return   4;
		 case 39: return   3;
		 case 40: return   2;
		 case 41: return   1;	// Some mobiles
		 case 42: return   0;
		 case 43: return   0;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
	 case SAVING_BREATH:	// Breath weapons
		switch (level)
		{case  0: return 100;
	 	 case  1: return 99;
         case  2: return 98;
         case  3: return 97;
         case  4: return 96;
         case  5: return 95;
         case  6: return 94;
         case  7: return 93;
         case  8: return 92;
         case  9: return 91;
         case 10: return 90;
         case 11: return 89;
         case 12: return 88;
         case 13: return 87;
		 case 14: return 86;
         case 15: return 85;
         case 16: return 84;
         case 17: return 82;
         case 18: return 80;
		 case 19: return 78;
         case 20: return 76;
         case 21: return 74;
	 	 case 22: return 72;
         case 23: return 70;
         case 24: return 68;
         case 25: return 66;
         case 26: return 64;
         case 27: return 62;
         case 28: return 60;
		 case 29: return 58;
         case 30: return 56;
         case 31: return 54;
         case 32: return 52;
         case 33: return 50;
         case 34: return 48;
         case 35: return 46;
         case 36: return 44;
		 case 37: return 42;
		 case 38: return 40;
		 case 39: return 37;
		 case 40: return 34;
		 case 41: return 31;
		 case 42: return 28;
		 case 43: return 25;
		 case 44: return 22;
		 case 45: return 19;
		 case 46: return 16;
		 case 47: return 13;
		 case 48: return 10;
		 case 49: return  5;
		 default: return  0;
		 break;
		}
	 case SAVING_SPELL:	// Generic spells
		switch (level)
		{case  0: return  90;
		 case  1: return  90;
		 case  2: return  90;
		 case  3: return  90;
		 case  4: return  90;
		 case  5: return  89;
		 case  6: return  89;
		 case  7: return  88;
		 case  8: return  87;
		 case  9: return  87;
		 case 10: return  86;
		 case 11: return  84;
		 case 12: return  83;
		 case 13: return  81;
		 case 14: return  80;
		 case 15: return  78;
		 case 16: return  75;
		 case 17: return  73;
		 case 18: return  70;
		 case 19: return  68;
		 case 20: return  65;
		 case 21: return  61;
		 case 22: return  58;
		 case 23: return  54;
		 case 24: return  50;
		 case 25: return  46;
		 case 26: return  41;
		 case 27: return  36;
		 case 28: return  31;
		 case 29: return  26;
		 case 30: return  20;
		 case 31: return  15;
		 case 32: return  10;
		 case 33: return   9;
		 case 34: return   8;
		 case 35: return   7;
		 case 36: return   6;
		 case 37: return   5;
		 case 38: return   4;
		 case 39: return   3;
		 case 40: return   2;
		 case 41: return   1;	// Some mobiles
		 case 42: return   0;
		 case 43: return   0;
		 case 44: return   0;
		 case 45: return   0;
		 case 46: return   0;
		 case 47: return   0;
		 case 48: return   0;
		 case 49: return   0;
		 default: return   0;
		 break;
		}
    case SAVING_BASIC:  // Protect from skills
      switch (level)
      {  case  0: return  100;
		 case  1: return  99;
         case  2: return  97;
         case  3: return  96;
         case  4: return  95;
         case  5: return  94;
         case  6: return  92;
		 case  7: return  91;
         case  8: return  89;
         case  9: return  88;
         case 10: return  86;
         case 11: return  85;
         case 12: return  84;
         case 13: return  83;
         case 14: return  81;
		 case 15: return  80;
		 case 16: return  79;
		 case 17: return  77;
		 case 18: return  76;
		 case 19: return  75;
		 case 20: return  74;
		 case 21: return  72;
	     case 22: return  71;
		 case 23: return  70;
		 case 24: return  68;
		 case 25: return  65;
		 case 26: return  64;
		 case 27: return  63;
		 case 28: return  62;
		 case 29: return  61;
		 case 30: return  60;
		 case 31: return  58;
		 case 32: return  57;
		 case 33: return  56;
		 case 34: return  54;
		 case 35: return  52;
		 case 36: return  51;
		 case 37: return  49;
		 case 38: return  47;
		 case 39: return  46;
		 case 40: return  45;
		 case 41: return  43;
		 case 42: return  42;
		 case 43: return  41;
		 case 44: return  39;
		 case 45: return  37;
		 case 46: return  35;
		 case 47: return  34;
		 case 48: return  32;
		 case 49: return  31;
         default: return  30;
		 break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
  }
  return 100;
}

/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(int class_num, int level) 
{
  switch (class_num) {
  case CLASS_MAGIC_USER:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  20;
    case  5: return  19;
    case  6: return  19;
    case  7: return  19;
    case  8: return  19;
    case  9: return  18;
    case 10: return  18;
    case 11: return  18;
    case 12: return  18;
    case 13: return  17;
    case 14: return  17;
    case 15: return  17;
    case 16: return  17;
    case 17: return  16;
    case 18: return  16;
    case 19: return  16;
    case 20: return  16;
    case 21: return  15;
    case 22: return  15;
    case 23: return  15;
    case 24: return  15;
    case 25: return  14;
    case 26: return  14;
    case 27: return  14;
    case 28: return  14;
    case 29: return  13;
    case 30: return  13;
    case 31: return  13;
    case 32: return  13;
    case 33: return  12;
    case 34: return  12;
    case 35: return  12;
    case 36: return  12;
    case 37: return  11;
    case 38: return  11;
    case 39: return  11;
    case 40: return  11;
    case 41: return  12;
    case 42: return  12;
    case 43: return  12;
    case 44: return  13;
    case 45: return  13;

    default:
      return  -30;//log("SYSERR: Missing level for mage thac0.");
    }
  case CLASS_CLERIC:
  case CLASS_DRUID:
    switch (level) {
      case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  20;
    case  5: return  19;
    case  6: return  19;
    case  7: return  19;
    case  8: return  19;
    case  9: return  18;
    case 10: return  18;
    case 11: return  18;
    case 12: return  18;
    case 13: return  17;
    case 14: return  17;
    case 15: return  17;
    case 16: return  17;
    case 17: return  16;
    case 18: return  16;
    case 19: return  16;
    case 20: return  16;
    case 21: return  15;
    case 22: return  15;
    case 23: return  15;
    case 24: return  15;
    case 25: return  14;
    case 26: return  14;
    case 27: return  14;
    case 28: return  14;
    case 29: return  13;
    case 30: return  13;
    case 31: return  13;
    case 32: return  13;
    case 33: return  12;
    case 34: return  12;
    case 35: return  12;
    case 36: return  12;
    case 37: return  11;
    case 38: return  11;
    case 39: return  11;
    case 40: return  11;
    case 41: return  12;
    case 42: return  12;
    case 43: return  12;
    case 44: return  13;
    case 45: return  13;

    default:
      return  -30;//log("SYSERR: Missing level for mage thac0.");
    }
	case CLASS_THIEF:    case CLASS_SLEDOPYT:
    case CLASS_ASSASINE: case CLASS_MONAH:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  19;
    case  3: return  19;
    case  4: return  19;
    case  5: return  18;
    case  6: return  18;
    case  7: return  18;
    case  8: return  17;
    case  9: return  17;
    case 10: return  17;
    case 11: return  16;
    case 12: return  16;
    case 13: return  16;
    case 14: return  15;
    case 15: return  15;
    case 16: return  15;
    case 17: return  14;
    case 18: return  14;
    case 19: return  14;
    case 20: return  13;
    case 21: return  13;
    case 22: return  13;
    case 23: return  12;
    case 24: return  12;
    case 25: return  12;
    case 26: return  11;
    case 27: return  11;
    case 28: return  11;
    case 29: return  10;
    case 30: return  10;
    case 31: return  10;
    case 32: return  9;
    case 33: return  9;
    case 34: return  9;
    case 35: return  8;
    case 36: return  8;
    case 37: return  8;
    case 38: return  7;
    case 39: return  7;
    case 40: return  7;
    case 41: return  6;
    case 42: return  6;
    case 43: return  6;
    case 44: return  5;
    case 45: return  5;

    default:
      return  -30;//log("SYSERR: Missing level for mage thac0.");
    }
  case CLASS_WARRIOR:  case CLASS_VARVAR:
  case CLASS_TAMPLIER: case CLASS_WEDMAK:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  19;
    case  3: return  19;
    case  4: return  18;
    case  5: return  18;
    case  6: return  17;
    case  7: return  17;
    case  8: return  16;
    case  9: return  16;
    case 10: return  15;
    case 11: return  15;
    case 12: return  14;
    case 13: return  14;
    case 14: return  13;
    case 15: return  13;
    case 16: return  12;
    case 17: return  12;
    case 18: return  11;
    case 19: return  11;
    case 20: return  10;
    case 21: return  10;
    case 22: return  9;
    case 23: return  9;
    case 24: return  8;
    case 25: return  8;
    case 26: return  7;
    case 27: return  7;
    case 28: return  6;
    case 29: return  6;
    case 30: return  5;
    case 31: return  5;
    case 32: return  4;
    case 33: return  4;
    case 34: return  3;
    case 35: return  3;
    case 36: return  2;
    case 37: return  2;
    case 38: return  1;
    case 39: return  1;
    case 40: return  0;
    case 41: return  0;
    case 42: return -1;
    case 43: return -1;
    case 44: return -2;
    case 45: return -2;

    default:
      return  -30;//log("SYSERR: Missing level for mage thac0.");
    }
  default:
    log("SYSERR: Unknown class in thac0 chart.");
  }

  /* Will not get there unless something is wrong. */
  return 100;
}


/* AC0 for classes and levels. */
int extra_aco(int class_num, int level) 
{
  switch (class_num) {
  case CLASS_MAGIC_USER:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return 0;
    case 10: return 0;
    case 11: return 0;
    case 12: return 0;
    case 13: return 0;
    case 14: return 0;
    case 15: return 0;
    case 16: return 0;
	case 17: return 0;
    case 18: return 0;
    case 19: return 0;
    case 20: return 0;
    case 21: return 0;
    case 22: return 0;
    case 23: return 0;
    case 24: return 0;
    case 25: return 0;
    case 26: return 0;
    case 27: return 0;
    case 28: return 0;
    case 29: return 0;
    case 30: return 0;
    case 31: return 0;
    case 32: return 0;
    case 33: return 0;
    case 34: return 0;
    case 35: return 0;
    case 36: return 0;
    case 37: return 0;
    case 38: return 0;
    case 39: return 0;
    case 40: return 0;
    case 41: return 0;
    case 42: return 0;
    case 43: return 0;
    case 44: return 0;
    case 45: return 0;
	
	default: return -15;
	 }
  case CLASS_CLERIC:
  case CLASS_DRUID:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return 0;
    case 10: return 0;
    case 11: return 0;
    case 12: return 0;
    case 13: return 0;
    case 14: return 0;
    case 15: return 0;
    case 16: return 0;
    case 17: return 0;
    case 18: return 0;
    case 19: return 0;
    case 20: return 0;
	case 21: return 0;
    case 22: return 0;
    case 23: return 0;
    case 24: return 0;
    case 25: return 0;
    case 26: return 0;
    case 27: return 0;
    case 28: return 0;
    case 29: return 0;
    case 30: return 0;
    case 31: return 0;
    case 32: return 0;
    case 33: return 0;
    case 34: return 0;
    case 35: return 0;
    case 36: return 0;
    case 37: return 0;
    case 38: return 0;
    case 39: return 0;
    case 40: return 0;
    case 41: return 0;
    case 42: return 0;
    case 43: return 0;
    case 44: return 0;
    case 45: return 0;
	
	default: return -15;
	 }
  case CLASS_ASSASINE:
  case CLASS_THIEF:
//  case CLASS_MERCHANT:
    switch (level) {
	 case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return -1;
    case 10: return -1;
    case 11: return -1;
    case 12: return -2;
    case 13: return -2;
    case 14: return -2;
    case 15: return -3;
    case 16: return -3;
    case 17: return -3;
    case 18: return -4;
    case 19: return -4;
    case 20: return -4;
    case 21: return -5;
    case 22: return -5;
    case 23: return -5;
    case 24: return -6;
    case 25: return -6;
    case 26: return -6;
    case 27: return -7;
    case 28: return -7;
    case 29: return -7;
    case 30: return -8;
    case 31: return -8;
    case 32: return -8;
    case 33: return -9;
    case 34: return -9;
    case 35: return -9;
    case 36: return -10;
    case 37: return -10;
    case 38: return -10;
    case 39: return -11;
    case 40: return -11;
    case 41: return -11;
    case 42: return -12;
    case 43: return -12;
    case 44: return -12;
    case 45: return -13;
	
	default: return -15;
	 }
  case CLASS_SLEDOPYT:
  case CLASS_MONAH:
  case CLASS_VARVAR: case CLASS_WEDMAK:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
	case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return 0;
    case 10: return 0;
    case 11: return 0;
    case 12: return 0;
    case 13: return 0;
    case 14: return 0;
    case 15: return 0;
    case 16: return -1;
    case 17: return -1;
    case 18: return -1;
    case 19: return -1;
    case 20: return -2;
    case 21: return -2;
    case 22: return -2;
    case 23: return -2;
    case 24: return -3;
    case 25: return -3;
    case 26: return -3;
    case 27: return -3;
    case 28: return -4;
    case 29: return -4;
    case 30: return -4;
    case 31: return -4;
    case 32: return -5;
    case 33: return -5;
    case 34: return -5;
    case 35: return -5;
    case 36: return -6;
    case 37: return -6;
    case 38: return -6;
    case 39: return -7;
    case 40: return -7;
    case 41: return -7;
    case 42: return -7;
    case 43: return -8;
    case 44: return -8;
    case 45: return -8;
	
	default: return -15;
	 }
  case CLASS_WARRIOR:
  case CLASS_TAMPLIER:
     switch (level) {
     case  0: return 0;
     case  1: return 0;
     case  2: return 0;
     case  3: return 0;
     case  4: return 0;
     case  5: return 0;
     case  6: return 0;
     case  7: return 0;
     case  8: return 0;
	 case  9: return 0;
	 case 10: return 0;
	 case 11: return 0;
	 case 12: return 0;
	 case 13: return 0;
	 case 14: return 0;
	 case 15: return 0;
	 case 16: return 0;
	 case 17: return 0;
	 case 18: return 0;
	 case 19: return 0;
	 case 20: return 0;
	 case 21: return -1;
	 case 22: return -1;
	 case 23: return -1;
	 case 24: return -1;
	 case 25: return -1;
	 case 26: return -2;
	 case 27: return -2;
	 case 28: return -2;
	 case 29: return -2;
	 case 30: return -2;
	 case 31: return -3;
	 case 32: return -3;
	 case 33: return -3;
	 case 34: return -3;
	 case 35: return -3;
	 case 36: return -4;
	 case 37: return -4;
	 case 38: return -4;
	 case 39: return -4;
	 case 40: return -4;
	 case 41: return -5;
	 case 42: return -5;
	 case 43: return -5;
	 case 44: return -5;
	 case 45: return -5;
	
	default: return -15;
	 }
  }
  return 0;
}
/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 * Здесь функция формирования характеристик для каждого из классов*/
//при ручном ролле сумма 79

void roll_real_abils(struct char_data * ch, int rollstat)
{
    int i;

const int RasaHeight0[][4] = 
{ {0, 160, 150, 0},//человек RACE_HUMAN
  {0, 160, 160, 0},//эльф
  {0, 110, 100, 0},//гном
  {0, 120, 110, 0},//дварф
  {0, 130, 120, 0},//низушек
  {0, 170, 170, 0},//полуэльф
  {0, 180, 190, 0},//варвар
  {0, 160, 160, 0} //лесной эльф

};
const int RasaHeight1[][4] = 
{ {0, 210, 190, 0},
  {0, 220, 190, 0},
  {0, 140, 150, 0},
  {0, 150, 160, 0},
  {0, 180, 160, 0},
  {0, 230, 200, 0},
  {0, 230, 240, 0},
  {0, 220, 190, 0}

};

const int RasaWeight0[][4] = 
{ {0, 80,  50,  0},
  {0, 60,  60,  0},
  {0, 110, 80,  0},
  {0, 100, 75,  0},
  {0, 40,  35,  0},
  {0, 60,  50,  0},
  {0, 130, 140, 0},
  {0, 60,  60,  0}

};
const int RasaWeight1[][4] = 
{ {0, 120, 110, 0},
  {0, 100, 80,  0},
  {0, 125, 100, 0},
  {0, 110, 90,  0},
  {0, 70,  65,  0},
  {0, 100, 90,  0},
  {0, 150, 160, 0},
  {0, 100, 80,  0}

};

	ch->add_abils.str_add = 0;
  
 	
	 i = MIN(85, rollstat);

	  do{     GET_INT_ROLL(ch) = ch->real_abils.intel=ClasStat[GET_RACE(ch)][INT] + number(0, 2);
		  GET_WIS_ROLL(ch) = ch->real_abils.wis  =ClasStat[GET_RACE(ch)][WIS] + number(0, 2);
		  GET_DEX_ROLL(ch) = ch->real_abils.dex  =ClasStat[GET_RACE(ch)][DEX] + number(0, 2);
		  GET_STR_ROLL(ch) = ch->real_abils.str  =ClasStat[GET_RACE(ch)][STR] + number(0, 2);
		  GET_CON_ROLL(ch) = ch->real_abils.con  =ClasStat[GET_RACE(ch)][CON] + number(0, 2);
		  GET_CHA_ROLL(ch) = ch->real_abils.cha  =ClasStat[GET_RACE(ch)][CHA] + number(0, 2);
		  GET_HEIGHT(ch)   = number(RasaHeight0[GET_RACE(ch)][GET_SEX(ch)], RasaHeight1[GET_RACE(ch)][GET_SEX(ch)]);
		  GET_WEIGHT(ch)   = number(RasaWeight0[GET_RACE(ch)][GET_SEX(ch)], RasaWeight1[GET_RACE(ch)][GET_SEX(ch)]);
		} 
	  while (ch->real_abils.con+ch->real_abils.wis+ch->real_abils.intel+ch->real_abils.dex+ch->real_abils.str+ch->real_abils.cha != i - (GET_RACE(ch)== RACE_HUMAN)?1 :0 );

 ch->real_abils.size= (((6 * ch->player.weight)/ (ch->player.height/12)))/2 + 1;


}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch, int newbie)
{
   int spellnum;

  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;
    if (newbie)
  roll_real_abils(ch, 0);
  
  ch->points.max_hit = 150;
  GET_MAX_MANA(ch)   = 100;
//изменить перенести в гильдии и здесь поставить присвоение для спелла 1
   for (spellnum = 0; spellnum <= MAX_SKILLS; spellnum++) {
	   if (1 == spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)])
	 	   GET_SPELL_TYPE(ch, spellnum) = SPELL_KNOW;
  }
   
   GET_SKILL(ch, SKILL_TEAR) = 30;
  
   GET_CRBONUS(ch)	= 8;
      
   if (IS_HUMAN(ch))
	  GET_CRBONUS(ch)	= 11;
   
   switch (GET_CLASS(ch))
   {  case CLASS_MAGIC_USER:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 15 + GET_REMORT(ch)*3;
	break;  
	case CLASS_CLERIC:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 25 + GET_REMORT(ch)*2; 
	break; 
	case CLASS_DRUID:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 35 + GET_REMORT(ch)*2;
	break;  
	case CLASS_WARRIOR:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 80 + GET_REMORT(ch);
	break; 
	case CLASS_WEDMAK:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 50;
		GET_CRBONUS(ch)=15;
    break;
	case CLASS_SLEDOPYT:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 80 + GET_REMORT(ch);
	case CLASS_TAMPLIER:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 70 + GET_REMORT(ch)*2/3;
		    if (IS_HUMAN(ch))
		ch->real_abils.cha += 1;
    break;
  case CLASS_THIEF:
		 GET_SKILL(ch, SKILL_SECOND_ATTACK) = 70 + GET_REMORT(ch)*2/3;
    break;
  case CLASS_ASSASINE:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 70 + GET_REMORT(ch)*2/3;
	break;
  case CLASS_MONAH:
		GET_SKILL(ch, SKILL_SECOND_ATTACK) = 90;
		GET_CRBONUS(ch)=13;
  break;

  default:
	GET_SKILL(ch, SKILL_SECOND_ATTACK) = 80 + GET_REMORT(ch);
  break;  }
  
  advance_level(ch);
  sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

  GET_HIT(ch)  = GET_REAL_MAX_HIT(ch);
  GET_MOVE(ch) = GET_REAL_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL)   = 24;
  GET_COND(ch, DRUNK)  = 0;

 
  if (siteok_everyone)
  SET_BIT(PLR_FLAGS(ch, PLR_SITEOK), PLR_SITEOK);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data * ch)
{
  int add_hp, con=0, add_move = 0, add_mana=0, i, count;

  add_hp = con_app[GET_CON(ch)].addhit;
  GET_COUNT_SKILL(ch) = 0; 
  
 /* if (GET_LEVEL(ch) == 5)
	  ch->player.bonus = 0;
  */

  

  for (count = 1; count <= MAX_SPELLS; count++)
  {  if (IS_SET(GET_SPELL_TYPE(ch, count), SPELL_LTEMP))
      REMOVE_BIT(GET_SPELL_TYPE(ch, count), SPELL_LTEMP);
      if (GET_SKILL(ch, count))
	   GET_COUNT_SKILL(ch)++; 
  }

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER: 
    i = number(1, 2);
	add_hp += i;
	ch->player_specials->saved.spare1 = i;
    add_move = 2;
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    i = number(1, 3);
	add_hp += i;
	ch->player_specials->saved.spare1 = i;
    add_move = 2;
    break;

  case CLASS_THIEF:    case CLASS_SLEDOPYT:
  case CLASS_ASSASINE: case CLASS_MONAH:
  case CLASS_VARVAR:   case CLASS_TAMPLIER:
    i = number(2, 3);
	add_hp += i;
	ch->player_specials->saved.spare1 = i;
    i = GET_DEX(ch)/5+1;
    ch->player_specials->saved.spare2 = i;
	add_move += i; 
    break;

  case CLASS_WARRIOR:
    i = number(1, 3);
    add_hp += i;
	ch->player_specials->saved.spare1 = i;
    i = GET_DEX(ch)/5+1;
    ch->player_specials->saved.spare2 = i;
	add_move += i; 
   	break;
   case CLASS_WEDMAK:
	i = number(2, 3);
    add_hp += i;
	ch->player_specials->saved.spare1 = i;
    i = GET_DEX(ch)/5+1;
    ch->player_specials->saved.spare2 = i;
	add_move += i; 
    add_mana = MAX(wis_app[GET_WIS(ch)+2].spell_success, 10);
	break;
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

 // if (GET_LEVEL(ch) >= 1)
    ch->points.max_mana += add_mana;

  /* ЗДесь надо будет вводить изменения по поводу учебы скиллов*/
 
  for (i = 0; i <= MAX_SKILLS; i++)//со временем перенести в первый фор,который выше.
  {		   if (SKILL_SECOND_ATTACK == i)
			continue;
		   if (GET_LEVEL(ch) == skill_info[i].k_improove[(int) GET_CLASS(ch)] && !GET_SKILL(ch, i)) 
            if(!skill_info[i].n_remort[(int) GET_CLASS(ch)])
			 GET_PRACTICES(ch, i) = 1;
  }

  
   if (GET_LEVEL(ch) >= LVL_IMMORT) {
      ch->real_abils.intel = GET_INT_ROLL(ch) = 25;
      ch->real_abils.wis   = GET_WIS_ROLL(ch) = 25;
      ch->real_abils.dex   = GET_DEX_ROLL(ch) = 25;
	  ch->real_abils.str   = GET_STR_ROLL(ch) = 25;
      ch->real_abils.con   = GET_CON_ROLL(ch) = 25;
	  ch->real_abils.cha   = GET_CHA_ROLL(ch) = 25;
	  
	  for (i = 1; i <= MAX_SKILLS; i++)
		SET_SKILL(ch, i, 100);
      for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  save_char(ch, NOWHERE);
}

/* Function to return the exp required for each class/level */

void decrease_level(struct char_data * ch)
{  //struct obj_data *obj=NULL;
   int con=0, add_hp, add_move=0, add_mana=0, i/*, j*/;
   add_hp = con_app[GET_CON(ch)].addhit;
   

   for (i = 0; i <= MAX_SKILLS; i++)//со временем 
  {		   if (SKILL_SECOND_ATTACK == i)
		 	      continue;
   if (GET_LEVEL(ch) >= skill_info[i].k_improove[(int) GET_CLASS(ch)]) 
        if (GET_SKILL(ch, i) > 2)
            GET_SKILL(ch, i)-= 2; 
	
        if (GET_LEVEL(ch) < skill_info[i].k_improove[(int) GET_CLASS(ch)]) 
           { SET_SKILL(ch, i, 0);
   	     GET_PRACTICES(ch, i) = 0;
		   }
   }



  switch (GET_CLASS(ch))
         {
  case CLASS_MAGIC_USER:
 /* case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:*/
    add_move = 2;
   add_hp +=ch->player_specials->saved.spare1;
  	break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
    add_move = 2;
add_hp +=ch->player_specials->saved.spare1;
    break;

  case CLASS_THIEF:
  case CLASS_ASSASINE:
//case CLASS_MERCHANT:
  case CLASS_WARRIOR:
  case CLASS_MONAH:
  case CLASS_SLEDOPYT:
  case CLASS_VARVAR:
  case CLASS_TAMPLIER:
    add_move+= ch->player_specials->saved.spare2;
    add_hp +=ch->player_specials->saved.spare1;
    break;
         
   case CLASS_WEDMAK:
  add_mana = MAX(wis_app[GET_WIS(ch)+2].spell_success, 10);
  add_move+= ch->player_specials->saved.spare2;
  add_hp +=ch->player_specials->saved.spare1;
    break;
 }
   if (GET_LEVEL(ch) > 1)
    ch->points.max_mana -= MIN(ch->points.max_mana, MAX(1, add_mana));
  if(!ch->points.max_mana)
     ch->points.max_mana =1;
  
  
  log("Dec hp for %s set ot %d", GET_NAME(ch), add_hp);
  ch->points.max_hit  -= MIN(ch->points.max_hit, MAX(1, add_hp));
   if(!ch->points.max_hit)
     ch->points.max_hit++;

  ch->points.max_move -= MIN(ch->points.max_move,MAX(1, add_move));
   if(!ch->points.max_move)
     ch->points.max_move++;
  GET_WIMP_LEV(ch)     = MAX(0,MIN(GET_WIMP_LEV(ch),GET_REAL_MAX_HIT(ch)/2));
   if (!IS_IMMORTAL(ch))
     REMOVE_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);

	if (GET_LEVEL(ch) > 14 && GET_LEVEL(ch) < 16)
		set_title(ch, "");
	if (GET_LEVEL(ch) > 23 && GET_LEVEL(ch) < 25)
		set_ptitle(ch, "");

  save_char(ch, NOWHERE);
}
/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
  if (level <= 0)
    return 1;	  /* level 0 */
  else if (level <= 7)
    return 2;	  /* level 1 - 7 */
  else if (level <= 13)
    return 3;	  /* level 8 - 13 */
  else if (level <= 20)
    return 4;	  /* level 14 - 20 */
  else if (level <= 28)
    return 5;	  /* level 21 - 28 */
  else if (level < LVL_IMMORT)
    return 6;	  /* all remaining mortal levels */
  else
    return 20;	  /* immortals */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_unique(struct char_data *ch, struct obj_data *obj)
{
 struct obj_data *object;
 for (object = obj->contains; object; object = object->next_content)
  if (invalid_unique(ch, object))
   return (TRUE);
 if (!ch             ||
     !obj            ||
     IS_NPC(ch)      ||
     IS_IMMORTAL(ch) ||
     obj->obj_flags.Obj_owner == 0 ||
     obj->obj_flags.Obj_owner == GET_UNIQUE(ch))
    return (FALSE);
 return (TRUE);
}


int invalid_anti_class(struct char_data *ch, struct obj_data *obj)
{
 struct obj_data *object;
 for (object = obj->contains; object; object = object->next_content)
  if (invalid_anti_class(ch, object))
   return (TRUE);
 if (IS_NPC(ch) || WAITLESS(ch))
     return (FALSE);
  if ((IS_OBJ_ANTI(obj, ITEM_AN_GOOD)       && IS_GOOD(ch))		  ||
      (IS_OBJ_ANTI(obj, ITEM_AN_EVIL)       && IS_EVIL(ch))	      ||
      (IS_OBJ_ANTI(obj, ITEM_AN_NEUTRAL)    && IS_NEUTRAL(ch))    ||
      (IS_OBJ_ANTI(obj, ITEM_AN_MAGIC)      && IS_MAGIC_USER(ch)) ||
      (IS_OBJ_ANTI(obj, ITEM_AN_CLERIC)     && IS_CLERIC(ch))     ||
      (IS_OBJ_ANTI(obj, ITEM_AN_THIEF)      && IS_THIEF(ch))      ||
	  (IS_OBJ_ANTI(obj, ITEM_AN_WARRIOR)    && IS_WARRIOR(ch))    ||
      (IS_OBJ_ANTI(obj, ITEM_AN_TAMPLIER)   && IS_TAMPLIER(ch))   ||
	  (IS_OBJ_ANTI(obj, ITEM_AN_SLEDOPYT)   && IS_SLEDOPYT(ch))   ||
	  (IS_OBJ_ANTI(obj, ITEM_AN_DRUID)      && IS_DRUID(ch))      ||
	  (IS_OBJ_ANTI(obj, ITEM_AN_VARVAR)     && IS_VARVAR(ch))     ||
      (IS_OBJ_ANTI(obj, ITEM_AN_ASSASINE)   && IS_ASSASINE(ch))   ||
      (IS_OBJ_ANTI(obj, ITEM_AN_MONAH)      && IS_MONAH(ch))	  ||
	  (IS_OBJ_ANTI(obj, ITEM_AN_WEDMAK)     && IS_WEDMAK(ch))
	  )      
     
     return (TRUE);
  return (FALSE);
}
// разобраться с заточкой для варвара
int invalid_no_class(struct char_data *ch, struct obj_data *obj)
{ if (IS_NPC(ch) || WAITLESS(ch))
     return (FALSE);

  if ((IS_OBJ_NO(obj, ITEM_NO_GOOD)       && IS_GOOD(ch))	   ||
      (IS_OBJ_NO(obj, ITEM_NO_EVIL)       && IS_EVIL(ch))	   ||
      (IS_OBJ_NO(obj, ITEM_NO_NEUTRAL)    && IS_NEUTRAL(ch))   ||
      (IS_OBJ_NO(obj, ITEM_NO_MAGIC)      && IS_MAGIC_USER(ch))||
      (IS_OBJ_NO(obj, ITEM_NO_CLERIC)     && IS_CLERIC(ch))    ||
      (IS_OBJ_NO(obj, ITEM_NO_THIEF)      && IS_THIEF(ch))     ||
	  (IS_OBJ_NO(obj, ITEM_NO_WARRIOR)    && IS_WARRIOR(ch))   ||
      (IS_OBJ_NO(obj, ITEM_NO_TAMPLIER)   && IS_TAMPLIER(ch))  ||
	  (IS_OBJ_NO(obj, ITEM_AN_SLEDOPYT)   && IS_SLEDOPYT(ch))  ||
	  (IS_OBJ_NO(obj, ITEM_NO_DRUID)      && IS_DRUID(ch))     ||
	  (IS_OBJ_NO(obj, ITEM_NO_VARVAR)     && IS_VARVAR(ch))    ||
      (IS_OBJ_NO(obj, ITEM_NO_ASSASINE)   && IS_ASSASINE(ch))  ||
      (IS_OBJ_NO(obj,ITEM_NO_WEDMAK)      && IS_WEDMAK(ch))	   ||
	  (IS_OBJ_NO(obj, ITEM_NO_MONAH)      && IS_MONAH(ch))     ||
      ((OBJ_FLAGGED(obj, ITEM_ARMORED) || OBJ_FLAGGED(obj, ITEM_SHARPEN)) &&   
      !IS_VARVAR(ch))
      )
      return (TRUE);
  return (FALSE);
}



/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX  700000000

/* Function to return the exp required for each class/level */
int level_exp(int chclass, int level)
{
  if (level > LVL_IMPL || level < 0) {
    log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /*
   * Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist.
   */
   if (level > LVL_IMMORT) {
     return EXP_MAX - ((LVL_IMPL - level) * 100000);
   }

  /* Exp required for normal mortals is below */

  switch (chclass) {

  case CLASS_MAGIC_USER: case CLASS_DRUID:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 60000;
      case  8: return 90000;
      case  9: return 140000;
      case 10: return 200000;
      case 11: return 315000;
      case 12: return 445000;
      case 13: return 580000;
      case 14: return 780000;
      case 15: return 1090000;
      case 16: return 1360000;
      case 17: return 1750000;
      case 18: return 2200000;
	  case 19: return 2700000;
      case 20: return 3250000;
      case 21: return 4330000;
      case 22: return 5500000;
      case 23: return 7100000;
      case 24: return 8800000;
      case 25: return 10800000;
      case 26: return 14700000;
      case 27: return 22000000;
      case 28: return 33000000;
      case 29: return 49000000;
      case 30: return 70000000;
      case 31: return 91000000;
      case 32: return 112000000;
      case 33: return 133000000;
      case 34: return 154000000;
	  case 35: return 175000000;
      case 36: return 196000000;
      case 37: return 217000000;
      case 38: return 238000000;
      case 39: return 259000000;
      case 40: return 285000000;
      case 41: return 315000000;
      case 42: return 345000000;
      case 43: return 375000000;
      case 44: return 405000000;
      case 45: return 455000000;
    
	  default: return 500000000;
    }
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 60000;
      case  8: return 90000;
      case  9: return 140000;
      case 10: return 200000;
      case 11: return 315000;
      case 12: return 445000;
      case 13: return 580000;
      case 14: return 780000;
      case 15: return 1090000;
      case 16: return 1360000;
      case 17: return 1750000;
      case 18: return 2200000;
	  case 19: return 2700000;
      case 20: return 3250000;
      case 21: return 4330000;
      case 22: return 5500000;
      case 23: return 7100000;
      case 24: return 8800000;
      case 25: return 10800000;
      case 26: return 14700000;
      case 27: return 22000000;
      case 28: return 33000000;
      case 29: return 49000000;
      case 30: return 70000000;
      case 31: return 91000000;
      case 32: return 112000000;
      case 33: return 133000000;
      case 34: return 154000000;
	  case 35: return 175000000;
      case 36: return 196000000;
      case 37: return 217000000;
      case 38: return 238000000;
      case 39: return 259000000;
      case 40: return 285000000;
      case 41: return 315000000;
      case 42: return 345000000;
      case 43: return 375000000;
      case 44: return 405000000;
      case 45: return 455000000;
    
	   /* add new levels here */
      default: return 500000000;
    }
    break;

    case CLASS_THIEF: case CLASS_ASSASINE: 
	case CLASS_SLEDOPYT: case CLASS_MONAH:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 60000;
      case  8: return 90000;
      case  9: return 140000;
      case 10: return 200000;
      case 11: return 315000;
      case 12: return 445000;
      case 13: return 580000;
      case 14: return 780000;
      case 15: return 1090000;
      case 16: return 1360000;
      case 17: return 1750000;
      case 18: return 2200000;
	  case 19: return 2700000;
      case 20: return 3250000;
      case 21: return 4330000;
      case 22: return 5500000;
      case 23: return 7100000;
      case 24: return 8800000;
      case 25: return 10800000;
      case 26: return 14700000;
      case 27: return 22000000;
      case 28: return 33000000;
      case 29: return 49000000;
      case 30: return 70000000;
      case 31: return 91000000;
      case 32: return 112000000;
      case 33: return 133000000;
      case 34: return 154000000;
	  case 35: return 175000000;
      case 36: return 196000000;
      case 37: return 217000000;
      case 38: return 238000000;
      case 39: return 259000000;
      case 40: return 285000000;
      case 41: return 315000000;
      case 42: return 345000000;
      case 43: return 375000000;
      case 44: return 405000000;
      case 45: return 455000000;
		  /* add new levels here */
      default: return 500000000;
    }
    break;

    case CLASS_WARRIOR:case CLASS_TAMPLIER:
    case CLASS_VARVAR: case CLASS_WEDMAK:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 60000;
      case  8: return 90000;
      case  9: return 120000;
      case 10: return 200000;
      case 11: return 315000;
      case 12: return 445000;
      case 13: return 580000;
      case 14: return 780000;
      case 15: return 1090000;
      case 16: return 1360000;
      case 17: return 1750000;
      case 18: return 2200000;
	  case 19: return 2700000;
      case 20: return 3250000;
      case 21: return 4330000;
      case 22: return 5500000;
      case 23: return 7100000;
      case 24: return 8800000;
      case 25: return 10800000;
      case 26: return 14700000;
      case 27: return 22000000;
      case 28: return 33000000;
      case 29: return 49000000;
      case 30: return 70000000;
      case 31: return 91000000;
      case 32: return 112000000;
      case 33: return 133000000;
      case 34: return 154000000;
	  case 35: return 175000000;
      case 36: return 196000000;
      case 37: return 217000000;
      case 38: return 238000000;
      case 39: return 259000000;
      case 40: return 285000000;
      case 41: return 315000000;
      case 42: return 345000000;
      case 43: return 375000000;
      case 44: return 405000000;
      case 45: return 455000000;

      /* add new levels here */
      default: return 500000000;
    }
    break;
  }
log("SYSERR: XP tables not set up correctly in class.c!");
  return 123456;
}

int extra_damroll(int class_num, int level)//перевод
{
  switch (class_num) {
  case CLASS_MAGIC_USER:
 /* case CLASS_DEFENDERMAGE:
  case CLASS_CHARMMAGE:
  case CLASS_NECROMANCER:*/
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
	case  7: return 0;
    case  8: return 0;
    case  9: return 0;
    case 10: return 0;
    case 11: return 1;
    case 12: return 1;
    case 13: return 1;
    case 14: return 1;
    case 15: return 1;
    case 16: return 1;
    case 17: return 1;
    case 18: return 1;
    case 19: return 1;
    case 20: return 1;
    case 21: return 2;
    case 22: return 2;
    case 23: return 2;
    case 24: return 2;
    case 25: return 2;
    case 26: return 2;
    case 27: return 2;
    case 28: return 2;
    case 29: return 3;
    case 30: return 3;
	case 31: return 3;
	case 32: return 3;
	case 33: return 3;
	case 34: return 4;
	case 35: return 4; 
	case 36: return 4;
    case 37: return 4;
    case 38: return 5;
    case 39: return 5;
    case 40: return 5;
	case 41: return 5;
	case 42: return 6;
	case 43: return 6;
	case 44: return 6;
	case 45: return 7; 
	default: return 20;
    }
  case CLASS_CLERIC:
  case CLASS_DRUID:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 0;
    case  9: return 1;
    case 10: return 1;
    case 11: return 1;
    case 12: return 1;
    case 13: return 1;
    case 14: return 1;
    case 15: return 1;
    case 16: return 1;
    case 17: return 2;
    case 18: return 2;
    case 19: return 2;
    case 20: return 2;
    case 21: return 2;
    case 22: return 2;
    case 23: return 2;
    case 24: return 2;
    case 25: return 3;
    case 26: return 3;
    case 27: return 3;
    case 28: return 3;
    case 29: return 3;
    case 30: return 3;
    case 31: return 4;
    case 32: return 4;
    case 33: return 4;
    case 34: return 5;
    case 35: return 5;
	case 36: return 5;
    case 37: return 6;
    case 38: return 6;
    case 39: return 6;
    case 40: return 7;
	case 41: return 7;
	case 42: return 8;
	case 43: return 8;
	case 44: return 9;
	case 45: return 10; 
    default: return 20;
    }
  case CLASS_ASSASINE:
  case CLASS_THIEF:
 // case CLASS_MERCHANT:
	 switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
    case  4: return 0;
    case  5: return 0;
    case  6: return 0;
    case  7: return 0;
    case  8: return 1;
    case  9: return 1;
    case 10: return 1;
    case 11: return 1;
    case 12: return 1;
    case 13: return 1;
    case 14: return 1;
    case 15: return 2;
    case 16: return 2;
    case 17: return 2;
    case 18: return 2;
    case 19: return 2;
    case 20: return 2;
    case 21: return 2;
    case 22: return 3;
    case 23: return 3;
    case 24: return 3;
    case 25: return 3;
    case 26: return 3;
    case 27: return 3;
    case 28: return 3;
    case 29: return 4;
    case 30: return 4;
    case 31: return 5;
    case 32: return 5;
    case 33: return 6;
    case 34: return 6;
    case 35: return 7;
	case 36: return 7;
    case 37: return 8;
    case 38: return 8;
    case 39: return 9;
    case 40: return 9;
	case 41: return 10;
	case 42: return 10;
	case 43: return 11;
	case 44: return 12;
	case 45: return 13; 
	default: return 20;
    }
  case CLASS_WARRIOR:case CLASS_WEDMAK:
  case CLASS_VARVAR:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
	case  4: return 0;
    case  5: return 0;
    case  6: return 1;
    case  7: return 1;
    case  8: return 1;
    case  9: return 1;
    case 10: return 1;
    case 11: return 2;
    case 12: return 2;
    case 13: return 2;
    case 14: return 2;
    case 15: return 2;
    case 16: return 3;
    case 17: return 3;
    case 18: return 3;
    case 19: return 3;
    case 20: return 3;
    case 21: return 4;
    case 22: return 4;
    case 23: return 4;
    case 24: return 4;
    case 25: return 4;
    case 26: return 5;
    case 27: return 5;
    case 28: return 5;
    case 29: return 5;
    case 30: return 6;
    case 31: return 6;
    case 32: return 7;
    case 33: return 7;
    case 34: return 8;
    case 35: return 9;
	case 36: return 10;
    case 37: return 11;
    case 38: return 12;
    case 39: return 13;
    case 40: return 14;
	case 41: return 15;
	case 42: return 16;
	case 43: return 17;
	case 44: return 18;
	case 45: return 20; 
    default: return 40;
    }
  case CLASS_TAMPLIER:
  case CLASS_SLEDOPYT:
  case CLASS_MONAH:
    switch (level) {
    case  0: return 0;
    case  1: return 0;
    case  2: return 0;
    case  3: return 0;
	case  4: return 0;
    case  5: return 0;
    case  6: return 1;
    case  7: return 1;
    case  8: return 1;
    case  9: return 1;
    case 10: return 1;
    case 11: return 2;
    case 12: return 2;
    case 13: return 2;
    case 14: return 2;
    case 15: return 2;
    case 16: return 3;
    case 17: return 3;
    case 18: return 3;
    case 19: return 3;
    case 20: return 3;
    case 21: return 4;
    case 22: return 4;
    case 23: return 4;
    case 24: return 4;
    case 25: return 4;
    case 26: return 5;
    case 27: return 5;
    case 28: return 5;
    case 29: return 5;
    case 30: return 6;
    case 31: return 6;
    case 32: return 7;
    case 33: return 7;
    case 34: return 8;
    case 35: return 9;
	case 36: return 10;
    case 37: return 11;
    case 38: return 12;
    case 39: return 13;
    case 40: return 14;
	case 41: return 15;
	case 42: return 16;
	case 43: return 17;
	case 44: return 18;
	case 45: return 20; 
    default: return 40;
    }
  }
  return 0;
}
 
  /*
   * This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete -- so, complete them!
   */
