/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "screen.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

#define LOC_INVENTORY	0
#define MAX_BAG_ROWS	5
#define ITEM_DESTROYED  100

//extern struct room_data *world;
extern INDEX_DATA *mob_index;
extern INDEX_DATA *obj_index;
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern struct obj_data *obj_proto;
extern int top_of_p_table;
extern int rent_file_timeout, crash_file_timeout;
extern int free_crashrent_period;
extern int free_rent;
extern long last_rent_check;
extern int max_obj_save;	/* change in config.c */
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;

/* Extern functions */

ACMD(do_tell);

SPECIAL(receptionist);
SPECIAL(cryogenicist);

void    name_from_drinkcon(struct obj_data * obj);
void    name_to_drinkcon(struct obj_data * obj, int type);
int	invalid_unique(CHAR_DATA *ch, struct obj_data *obj);
int	Crash_read_timer(int index, int temp);
long    get_ptable_by_name(char *name);
int	Crash_delete_files(int index);
void   	asciiflag_conv1(char *flag, void *value);
bitvector_t   	asciiflag_conv(char *flag);
int  	invalid_anti_class(CHAR_DATA *ch, struct obj_data *obj);
int    	min_rent_cost(CHAR_DATA *ch);
int    	invalid_no_class(CHAR_DATA *ch, struct obj_data *obj);

// local functions 
int     delete_char(char *name);
int	Crash_offer_rent(CHAR_DATA * ch, CHAR_DATA * receptionist, int display, int factor, int *totalcost, int Serw);
int	Crash_report_unrentables(CHAR_DATA * ch, CHAR_DATA * recep, struct obj_data * obj);
int	Crash_is_unrentable(struct obj_data * obj);
int	gen_receptionist(CHAR_DATA * ch, CHAR_DATA * recep, int cmd, char *arg, int mode);
void	Crash_extract_norent_eq(CHAR_DATA *ch);
void	auto_equip(CHAR_DATA *ch, struct obj_data *obj, int location);
void	Crash_report_rent(CHAR_DATA * ch, CHAR_DATA * recep, struct obj_data * obj, int *cost, long *nitems, int display, int factor, int equip, int recursive);
void	update_obj_file(void);
void	Crash_save(int iplayer, struct obj_data * obj, int location);
void	Crash_rent_deadline(CHAR_DATA * ch, CHAR_DATA * recep, long cost, int Serw);
void	Crash_restore_weight(struct obj_data * obj);
void	Crash_extract_objs(struct obj_data * obj);
void	Crash_extract_norents(struct obj_data * obj);
void	Crash_extract_expensive(struct obj_data * obj);
void    tascii(int *pointer, int num_planes, char *ascii);
void    tag_argument(char *argument, char *tag);

#define MAKESIZE(number) (sizeof(struct save_info) + sizeof(struct save_time_info) * number)
#define MAKEINFO(pointer, number) CREATE(pointer, struct save_info, MAKESIZE(number))
#define SAVEINFO(number) ((player_table+number)->timer)
#define RENTCODE(number) ((player_table+number)->timer->rent.rentcode)
#define SAVESIZE(number) (sizeof(struct save_info) +\
                          sizeof(struct save_time_info) * (player_table+number)->timer->rent.nitems)
#define GET_INDEX(ch) (get_ptable_by_name(GET_NAME(ch)))
#define DIV_CHAR  '#'
#define END_CHAR  '$'
#define END_LINE  '\n'
#define END_RLINE  '\r'
#define END_LINES '~'
#define COM_CHAR  '*'



int get_buf_line(char **source, char *target)
{char *otarget = target;
 int  empty = TRUE;

 *target = '\0';
 for (;
      **source && **source != DIV_CHAR && **source != END_CHAR;
      (*source)++
     )
     {
        if (**source == END_RLINE)
            (*source)++;

        if (**source == END_LINE)
         {if (empty || *otarget == COM_CHAR)
             {target  = otarget;
              *target = '\0';
              continue;
             }
          (*source)++;
          return (TRUE);
         }
      *target = **source;
      if (!is_space(*target++))
         empty = FALSE;
      *target = '\0';
     }
 return (FALSE);
}

int get_buf_lines(char **source, char *target)
{
 *target = '\0';

  for (; **source && **source != DIV_CHAR && **source != END_CHAR; (*source)++ )
      {if (**source == END_LINES)
          {(*source)++;
           if (**source == END_RLINE)
	      (*source)++;
	   if (**source == END_LINE)
              (*source)++;

           return (TRUE);
          }
       *(target++) = **source;
       *target = '\0';
      }
 return (FALSE);
}


// Данная процедура выбирает предмет из буфера.
// с поддержкой нового формата вещей игроков [от 10.12.04].
OBJ_DATA *read_one_object_new(char **data, int *error)
{
	char buffer[MAX_STRING_LENGTH+1000];
	char read_line[MAX_STRING_LENGTH+1000];
	int t[1+100];
	int vnum;
	OBJ_DATA *object = NULL;
	EXTRA_DESCR_DATA *new_descr;

	*error = 1;
	// Станем на начало предмета
	for (; **data != DIV_CHAR; (*data)++)
		if (!**data || **data == END_CHAR)
			return (object);
	*error = 2;
	// Пропустим #
	(*data)++;
	// Считаем vnum предмета
	if (!get_buf_line(data, buffer))
		return (object);
	*error = 3;
	if (!(vnum = atoi(buffer)))
		return (object);

	// Если без прототипа, создаем новый. Иначе читаем прототип и возвращаем NULL,
	// если прототип не существует.
	if (vnum < 0) {
		object = create_obj();
		GET_OBJ_RNUM(object) = NOTHING;
		object->ex_description = NULL;
	} else {
		object = read_object(vnum, VIRTUAL);//тут срочно надо избавиться от рандомного создания предмета, ибо он в инвентареже.
		if (!object) {
			*error = 4;
			return NULL;
		}
	}
	// Далее найденные параметры присваиваем к прототипу
	while (get_buf_lines(data, buffer)) {
		tag_argument(buffer, read_line);

		if (read_line != NULL) {
			// Чтобы не портился прототип вещи некоторые параметры изменяются лишь у вещей с vnum < 0
			if (!strcmp(read_line, "Alia")) {
				*error = 6;
				GET_OBJ_ALIAS(object) = str_dup(buffer);
				object->name = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad0")) {
				*error = 7;
				object->short_description = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad1")) {
				*error = 8;
				object->short_rdescription = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad2")) {
				*error = 9;
				object->short_ddescription = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad3")) {
				*error = 10;
				object->short_vdescription = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad4")) {
				*error = 11;
				object->short_tdescription = str_dup(buffer);
			} else if (!strcmp(read_line, "Pad5")) {
				*error = 12;
				object->short_pdescription = str_dup(buffer);
			} else if (!strcmp(read_line, "Desc")) {
				*error = 13;
				GET_OBJ_DESC(object) = str_dup(buffer);
			} else if (!strcmp(read_line, "ADsc")) {
				*error = 14;
				GET_OBJ_ACT(object) = str_dup(buffer);
			} else if (!strcmp(read_line, "Lctn")) {
				*error = 5;
				object->worn_on = atoi(buffer);
                        } else if (!strcmp(read_line, "Chst")) {
				*error = 48;
				object->chest = atoi(buffer) ? true: false;
			} else if (!strcmp(read_line, "Skil")) {
				*error = 15;
				GET_OBJ_SKILL(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Maxx")) {
				*error = 16;
				GET_OBJ_MAX(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Curr")) {
				*error = 17;
				GET_OBJ_CUR(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Mter")) {
				*error = 18;
				GET_OBJ_MATER(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Sexx")) {
				*error = 19;
				GET_OBJ_SEX(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Tmer")) {
				*error = 20;
				GET_OBJ_TIMER(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Spll")) {
				*error = 21;
				GET_OBJ_SPELL(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Levl")) {
				*error = 22;
				GET_OBJ_LEVEL(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Affs")) {
				*error = 23;
				asciiflag_conv1(buffer, &GET_OBJ_AFFECTS(object));
			} else if (!strcmp(read_line, "Anti")) {
				*error = 24;
				asciiflag_conv1(buffer, &GET_OBJ_ANTI(object));
			} else if (!strcmp(read_line, "Nofl")) {
				*error = 25;
				asciiflag_conv1(buffer, &GET_OBJ_NO(object));
			} else if (!strcmp(read_line, "Extr")) {
				*error = 26;
				// обнуляем старое extra_flags - т.к. asciiflag_conv только добавляет флаги 
				object->obj_flags.extra_flags.flags[0] = 0;
				object->obj_flags.extra_flags.flags[1] = 0;
				object->obj_flags.extra_flags.flags[2] = 0;
				object->obj_flags.extra_flags.flags[3] = 0;
				asciiflag_conv1(buffer, &GET_OBJ_EXTRA(object, 0));
			} else if (!strcmp(read_line, "Wear")) {
				*error = 27;
				// обнуляем старое wear_flags - т.к. asciiflag_conv только добавляет флаги 
				GET_OBJ_WEAR(object) = 0;
				asciiflag_conv1(buffer, &GET_OBJ_WEAR(object));
			} else if (!strcmp(read_line, "Type")) {
				*error = 28;
				GET_OBJ_TYPE(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Val0")) {
				*error = 29;
				GET_OBJ_VAL(object, 0) = atoi(buffer);
			} else if (!strcmp(read_line, "Val1")) {
				*error = 30;
				GET_OBJ_VAL(object, 1) = atoi(buffer);
			} else if (!strcmp(read_line, "Val2")) {
				*error = 31;
				GET_OBJ_VAL(object, 2) = atoi(buffer);
			} else if (!strcmp(read_line, "Val3")) {
				*error = 32;
				GET_OBJ_VAL(object, 3) = atoi(buffer);
			} else if (!strcmp(read_line, "Weig")) {
				*error = 33;
				GET_OBJ_WEIGHT(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Cost")) {
				*error = 34;
				GET_OBJ_COST(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Rent")) {
				*error = 35;
				GET_OBJ_RENT(object) = atoi(buffer);
			} else if (!strcmp(read_line, "RntQ")) {
				*error = 36;
				GET_OBJ_RENTE(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Ownr")) {
				*error = 37;
				GET_OBJ_OWNER(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Mker")) {
				*error = 38;
				GET_OBJ_MAKER(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Prnt")) {
				*error = 39;
				GET_OBJ_PARENT(object) = atoi(buffer);
			} else if (!strcmp(read_line, "Afc0")) {
				*error = 40;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[0].location = t[0];
				object->affected[0].modifier = t[1];
			} else if (!strcmp(read_line, "Afc1")) {
				*error = 41;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[1].location = t[0];
				object->affected[1].modifier = t[1];
			} else if (!strcmp(read_line, "Afc2")) {
				*error = 42;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[2].location = t[0];
				object->affected[2].modifier = t[1];
			} else if (!strcmp(read_line, "Afc3")) {
				*error = 43;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[3].location = t[0];
				object->affected[3].modifier = t[1];
			} else if (!strcmp(read_line, "Afc4")) {
				*error = 44;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[4].location = t[0];
				object->affected[4].modifier = t[1];
			} else if (!strcmp(read_line, "Afc5")) {
				*error = 45;
				sscanf(buffer, "%d %d", t, t + 1);
				object->affected[5].location = t[0];
				object->affected[5].modifier = t[1];
			} else if (!strcmp(read_line, "Edes")) {
				*error = 46;
				CREATE(new_descr, EXTRA_DESCR_DATA, 1);
				new_descr->keyword = str_dup(buffer);
				if (!get_buf_lines(data, buffer)) {
					free(new_descr->keyword);
					free(new_descr);
					*error = 47;
					return (object);
                }
				new_descr->description = str_dup(buffer);
				new_descr->next = object->ex_description;
				object->ex_description = new_descr;
			} 
            else if (!strcmp(read_line, "Clin"))
            {
                unsigned int clink = 0;
				sscanf(buffer, "%u", &clink);
                object->clin = clink;
            }
            else if (!strcmp(read_line, "Cont"))
            {
                unsigned int cont = 0;
                sscanf(buffer, "%u", &cont);
                object->cont = cont;
    		}
            else if (!strcmp(read_line, "Rlev"))
            {
                int cont = 0;
                sscanf(buffer, "%d", &cont);
                if (cont > 0)
                    GET_OBJ_RLVL(object) = cont;
    		}
            else {
				sprintf(buf, "WARNING: \"%s\" is not valid key for character items! [value=\"%s\"]",
					read_line, buffer);
				mudlog(buf, NRM, LVL_GRGOD, TRUE);
			}
		}
	}
	*error = 0;

	// Проверить вес фляг и т.п.
	if (GET_OBJ_TYPE(object) == ITEM_DRINKCON || GET_OBJ_TYPE(object) == ITEM_FOUNTAIN) {
		if (GET_OBJ_WEIGHT(object) < GET_OBJ_VAL(object, 1))
			GET_OBJ_WEIGHT(object) = GET_OBJ_VAL(object, 1) + 5;
	}
	// Проверка на ингры
/*	if (GET_OBJ_TYPE(object) == ITEM_MING) {
		int err = im_assign_power(object);
		if (err)
			*error = 100 + err;
	}*/
	return (object);
}

// разобраться - для чего эта фича
// Данная процедура выбирает предмет из буфера
struct obj_data *read_one_object(char **data, int *error)
{ char   buffer[MAX_STRING_LENGTH], f0[MAX_STRING_LENGTH],
         f1[MAX_STRING_LENGTH], f2[MAX_STRING_LENGTH];
  int    vnum, i=0, j, t[5];
  struct obj_data *object = NULL;
  struct extra_descr_data *new_descr;

  *error = 1;
  // Станем на начало предмета
  for(;**data != DIV_CHAR; (*data)++)
     if (!**data || **data == END_CHAR)
        return (object);
  *error = 2;
  // Пропустим #
  (*data)++;
  // Считаем vnum предмета
  if (!get_buf_line(data,buffer))
     return (object);
  *error = 3;
  if (!(vnum = atoi(buffer)))
     return (object);

  if (vnum < 0)
     {// Предмет не имеет прототипа
      object = create_obj();
      GET_OBJ_RNUM(object) = NOTHING;
      *error = 4;
      if (!get_buf_lines(data,buffer))
         return (object);
      // Алиасы
    //  GET_OBJ_ALIAS(object) = str_dup(buffer);
      object->name          = str_dup(buffer);
      // Падежи
      *error = 5;
          if (!get_buf_lines(data,buffer))
              return (object);
           object->short_description = str_dup(buffer);
          if (!get_buf_lines(data,buffer))
              return (object);
           object->short_rdescription = str_dup(buffer);
	 	  if (!get_buf_lines(data,buffer))
              return (object);
           object->short_ddescription = str_dup(buffer);
          if (!get_buf_lines(data,buffer))
              return (object);
           object->short_vdescription = str_dup(buffer);
		   if (!get_buf_lines(data,buffer))
              return (object);
           object->short_tdescription = str_dup(buffer);
          if (!get_buf_lines(data,buffer))
              return (object);
           object->short_pdescription = str_dup(buffer);
		   // Описание когда на земле
      *error = 6;
      if (!get_buf_lines(data,buffer))
         return (object);
      GET_OBJ_DESC(object) = str_dup(buffer);
      // Описание при действии
      *error = 7;
      if (!get_buf_lines(data,buffer))
         return (object);
      GET_OBJ_ACT(object) = str_dup(buffer);
     }
  else
  if (!(object = read_object(vnum, VIRTUAL)))
  {  *error = 8;
     return (object);
  }

  *error = 9;
  if (!get_buf_line(data, buffer) ||
      sscanf(buffer, " %s %d %d %d", f0, t+1, t+2, t+3) != 4
     )
     return (object);
  asciiflag_conv1(f0, &GET_OBJ_SKILL(object));
  GET_OBJ_MAX(object)   = t[1];
  GET_OBJ_CUR(object)   = t[2];
  GET_OBJ_MATER(object) = t[3];

  *error = 10;
  if (!get_buf_line(data, buffer) ||
       sscanf(buffer, " %d %d %d %d", t, t+1, t+2, t+3) != 4
     )
     return (object);
  GET_OBJ_SEX(object)   = t[0];
  GET_OBJ_TIMER(object) = t[1];
  GET_OBJ_SPELL(object) = t[2];
  GET_OBJ_LEVEL(object) = t[3];

  *error = 11;
  if (!get_buf_line(data, buffer) ||
      sscanf(buffer, " %s %s %s", f0, f1, f2) != 3
     )
     return (object);
  asciiflag_conv1(f0, &GET_OBJ_AFFECTS(object));//ПЕРЕНЕСЕНО 01.09.2007 экстроаффекты
  asciiflag_conv1(f1, &GET_OBJ_ANTI(object));
  asciiflag_conv1(f2, &GET_OBJ_NO(object));

  *error = 12;
  if (!get_buf_line(data,buffer) ||
      sscanf(buffer, " %d %s %s", t, f1, f2) != 3
     )
     return (object);
  GET_OBJ_TYPE(object) = t[0];
  asciiflag_conv1(f1, &GET_OBJ_EXTRA(object,0));//ПЕРЕНЕСЕНО01.09.2007 экстроаффекты
  asciiflag_conv1(f2, &GET_OBJ_WEAR(object));

  *error = 13; 
  if (!get_buf_line(data,buffer) ||
      sscanf(buffer, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4
     )
     return (object);
  GET_OBJ_VAL(object,1) = t[0];
  GET_OBJ_VAL(object,1) = t[1];
  GET_OBJ_VAL(object,2) = t[2];
  GET_OBJ_VAL(object,3) = t[3];

  *error = 14;
  if (!get_buf_line(data,buffer) ||
      sscanf(buffer, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4
     )
     return (object);
  GET_OBJ_WEIGHT(object)   = t[0];
  GET_OBJ_COST(object)     = t[1];
  GET_OBJ_RENT(object)     = t[2];
  GET_OBJ_RENTE(object)    = t[3];

  *error = 15;
  if (!get_buf_line(data,buffer) ||
      sscanf(buffer, "%d %d", t, t + 1) != 2
     )
     return (object);
  object->worn_on       = t[0];
  GET_OBJ_OWNER(object) = t[1];

  // Проверить вес фляг и т.п.
  if (GET_OBJ_TYPE(object) == ITEM_DRINKCON ||
      GET_OBJ_TYPE(object) == ITEM_FOUNTAIN
     )
    {if (GET_OBJ_WEIGHT(object) < GET_OBJ_VAL(object,1))
        GET_OBJ_WEIGHT(object) = GET_OBJ_VAL(object,1) + 5;
    }

  object->ex_description = NULL; // Exlude doubling ex_description !!!
  j                      = 0;

  for (;;)
     {if (!get_buf_line(data, buffer))
      {
         *error = 0;
         return (object);
      }
      switch (*buffer)
         { case 'E':
                    CREATE(new_descr, struct extra_descr_data, 1);
                    if (!get_buf_lines(data, buffer))
                       {free(new_descr);
                        *error = 16;
                        return (object);
                       }
                    new_descr->keyword = str_dup(buffer);
                    if (!get_buf_lines(data, buffer))
                       {free(new_descr->keyword);
                        free(new_descr);
                        *error = 17;
                        return (object);
                       }
                    new_descr->description = str_dup(buffer);
                    new_descr->next = object->ex_description;
                    object->ex_description = new_descr;
                    break;
            case 'A':
                    if (j >= MAX_OBJ_AFFECT)
                    { *error = 18;
                      return (object);
                    }
                    if (!get_buf_line(data,buffer))
                    { *error = 19;
                       return (object);
                    }
                    if (sscanf(buffer, " %d %d ", t, t + 1) == 2)
                       {object->affected[j].location = t[0];
                        object->affected[j].modifier = t[1];
                        j++;
                       }
                    break;
             default:
                    break;
         }
     }
  *error = 20;
  return (object);
}


// shapirus: функция проверки наличия доп. описания в прототипе
inline bool proto_has_descr(EXTRA_DESCR_DATA * odesc, EXTRA_DESCR_DATA * pdesc)
{
	EXTRA_DESCR_DATA *desc;

	for (desc = pdesc; desc; desc = desc->next)
		if (!str_cmp(odesc->keyword, desc->keyword) && !str_cmp(odesc->description, desc->description))
			return TRUE;

	return FALSE;
}

unsigned int p2u(void *p)
{
    // convert ptr to value (its will be id)
    unsigned int v = 0;
    v = (unsigned int)p;
    return v;
}

// Данная процедура помещает предмет в буфер
// [ ИСПОЛЬЗУЕТСЯ В НОВОМ ФОРМАТЕ ВЕЩЕЙ ПЕРСОНАЖА ОТ 10.12.04 ]
void write_one_object(char **data, OBJ_DATA * object, int location)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	EXTRA_DESCR_DATA *descr;
	int count = 0, i, j;
	OBJ_DATA *proto;

	// vnum
	count += sprintf(*data + count, "#%d\n", GET_OBJ_VNUM(object));
	// Положение в экипировке (сохраняем только если > 0)
	if (location)
	    count += sprintf(*data + count, "Lctn: %d~\n", location);

        if (object->chest)
            count += sprintf(*data + count, "Chst: %d~\n", object->chest);

	// Если у шмотки есть прототип то будем сохранять по обрезанной схеме, иначе
	// придется сохранять все статсы шмотки.
	if (GET_OBJ_VNUM(object) >= 0 && (proto = read_object(GET_OBJ_VNUM(object), VIRTUAL))) {
		// Алиасы
		if (str_cmp(GET_OBJ_ALIAS(object), GET_OBJ_ALIAS(proto)))
			count += sprintf(*data + count, "Alia: %s~\n", GET_OBJ_ALIAS(object));
		// Падежи
	   count += sprintf(*data+count,"Pad0: %s~\n",object->short_description);
           count += sprintf(*data+count,"Pad1: %s~\n",object->short_rdescription);
           count += sprintf(*data+count,"Pad2: %s~\n",object->short_ddescription);
           count += sprintf(*data+count,"Pad3: %s~\n",object->short_vdescription);
	   count += sprintf(*data+count,"Pad4: %s~\n",object->short_tdescription);
           count += sprintf(*data+count,"Pad5: %s~\n",object->short_pdescription);
		// Описание когда на земле
		if (str_cmp(GET_OBJ_DESC(object), GET_OBJ_DESC(proto)))
			count +=
			    sprintf(*data + count, "Desc: %s~\n", GET_OBJ_DESC(object) ? GET_OBJ_DESC(object) : "");
		// Описание при действии
		if (GET_OBJ_ACT(object) != NULL && GET_OBJ_ACT(proto) != NULL) {
			if (str_cmp(GET_OBJ_ACT(object), GET_OBJ_ACT(proto)))
				count += sprintf(*data + count, "ADsc: %s~\n", GET_OBJ_ACT(object) ? GET_OBJ_ACT(object) : "");
		}
		// Тренируемый скилл
		if (GET_OBJ_SKILL(object) != GET_OBJ_SKILL(proto))
			count += sprintf(*data + count, "Skil: %d~\n", GET_OBJ_SKILL(object));
		// Макс. прочность
		if (GET_OBJ_MAX(object) != GET_OBJ_MAX(proto))
			count += sprintf(*data + count, "Maxx: %d~\n", GET_OBJ_MAX(object));
		// Текущая прочность
		if (GET_OBJ_CUR(object) != GET_OBJ_CUR(proto))
			count += sprintf(*data + count, "Curr: %d~\n", GET_OBJ_CUR(object));
		// Материал
		if (GET_OBJ_MATER(object) != GET_OBJ_MATER(proto))
			count += sprintf(*data + count, "Mter: %d~\n", GET_OBJ_MATER(object));
		// Пол
		if (GET_OBJ_SEX(object) != GET_OBJ_SEX(proto))
			count += sprintf(*data + count, "Sexx: %d~\n", GET_OBJ_SEX(object));
		// Таймер
		if (GET_OBJ_TIMER(object) != GET_OBJ_TIMER(proto))
			count += sprintf(*data + count, "Tmer: %d~\n", GET_OBJ_TIMER(object));
		// Вызываемое заклинание
		if (GET_OBJ_SPELL(object) != GET_OBJ_SPELL(proto))
			count += sprintf(*data + count, "Spll: %d~\n", GET_OBJ_SPELL(object));
		// Уровень заклинания
		if (GET_OBJ_LEVEL(object) != GET_OBJ_LEVEL(proto))
			count += sprintf(*data + count, "Levl: %d~\n", GET_OBJ_LEVEL(object));
		// Наводимые аффекты
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_AFFECTS(object), 4, buf);
		tascii((int *) &GET_OBJ_AFFECTS(proto), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Affs: %s~\n", buf);
		// Анти флаги
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_ANTI(object), 4, buf);
		tascii((int *) &GET_OBJ_ANTI(proto), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Anti: %s~\n", buf);
		// Запрещающие флаги
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_NO(object), 4, buf);
		tascii((int *) &GET_OBJ_NO(proto), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Nofl: %s~\n", buf);
		// Экстра флаги
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_EXTRA(object, 0), 4, buf);
		tascii((int *) &GET_OBJ_EXTRA(proto, 0), 4, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Extr: %s~\n", buf);
		// Флаги слотов экипировки
		*buf = '\0';
		*buf2 = '\0';
		tascii((int *) &GET_OBJ_WEAR(object), 1, buf);
		tascii((int *) &GET_OBJ_WEAR(proto), 1, buf2);
		if (str_cmp(buf, buf2))
			count += sprintf(*data + count, "Wear: %s~\n", buf);
		// Тип предмета
		if (GET_OBJ_TYPE(object) != GET_OBJ_TYPE(proto))
			count += sprintf(*data + count, "Type: %d~\n", GET_OBJ_TYPE(object));
		// Значение 0, Значение 1, Значение 2, Значение 3.
		for (i = 0; i < 4; i++)
			if (GET_OBJ_VAL(object, i) != GET_OBJ_VAL(proto, i))
				count += sprintf(*data + count, "Val%d: %d~\n", i, GET_OBJ_VAL(object, i));
		// Вес
		if (GET_OBJ_WEIGHT(object) != GET_OBJ_WEIGHT(proto))
			count += sprintf(*data + count, "Weig: %d~\n", GET_OBJ_WEIGHT(object));
		// Цена
		if (GET_OBJ_COST(object) != GET_OBJ_COST(proto))
			count += sprintf(*data + count, "Cost: %d~\n", GET_OBJ_COST(object));
		// Рента (снято)
		if (GET_OBJ_RENT(object) != GET_OBJ_RENT(proto))
			count += sprintf(*data + count, "Rent: %d~\n", GET_OBJ_RENT(object));
		// Рента (одето)
		if (GET_OBJ_RENTE(object) != GET_OBJ_RENTE(proto))
			count += sprintf(*data + count, "RntQ: %d~\n", GET_OBJ_RENTE(object));
		// Владелец
		if (GET_OBJ_OWNER(object) && GET_OBJ_OWNER(object) != GET_OBJ_OWNER(proto))
			count += sprintf(*data + count, "Ownr: %d~\n", GET_OBJ_OWNER(object));
		// Создатель
		if (GET_OBJ_MAKER(object) && GET_OBJ_MAKER(object) != GET_OBJ_MAKER(proto))
			count += sprintf(*data + count, "Mker: %d~\n", GET_OBJ_MAKER(object));
		// Родитель
		if (GET_OBJ_PARENT(object) && GET_OBJ_PARENT(object) != GET_OBJ_PARENT(proto))
			count += sprintf(*data + count, "Prnt: %d~\n", GET_OBJ_PARENT(object));
        // Container link
        if (object->in_obj)
            count += sprintf(*data + count, "Clin: %u~\n", p2u(object->in_obj));
        if (object->contains)
            count += sprintf(*data + count, "Cont: %u~\n", p2u(object));
        if (GET_OBJ_RLVL(object))
            count += sprintf(*data + count, "Rlev: %d~\n", GET_OBJ_RLVL(object));


        
		// Аффекты
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			if (object->affected[j].location
			    //&& object->affected[j].location != proto->affected[j].location // problem with some upgraded affects
			    && object->affected[j].modifier != proto->affected[j].modifier)
				count += sprintf(*data + count, "Afc%d: %d %d~\n", j,
						 object->affected[j].location, object->affected[j].modifier);

		// Доп описания
		// shapirus: исправлена ошибка с несохранением, например, меток
		// на крафтеных луках
		for (descr = object->ex_description; descr; descr = descr->next) {
			if (proto_has_descr(descr, proto->ex_description))
				continue;
			count += sprintf(*data + count, "Edes: %s~\n%s~\n",
					 descr->keyword ? descr->keyword : "",
					 descr->description ? descr->description : "");
		}
		extract_obj(proto);
	} else			// Если у шмотки нет прототипа - придется сохранять ее целиком.
	{
		// Алиасы
		if (GET_OBJ_ALIAS(object))
			count += sprintf(*data + count, "Alia: %s~\n", GET_OBJ_ALIAS(object));
		// Падежи
	//	for (i = 0; i < NUM_PADS; i++)
		//	if (GET_OBJ_PNAME(object, i))
			//	count += sprintf(*data + count, "Pad%d: %s~\n", i, GET_OBJ_PNAME(object, i));
		   count += sprintf(*data+count,"Pad0: %s~\n",object->short_description);
           count += sprintf(*data+count,"Pad1: %s~\n",object->short_rdescription);
           count += sprintf(*data+count,"Pad2: %s~\n",object->short_ddescription);
           count += sprintf(*data+count,"Pad3: %s~\n",object->short_vdescription);
		   count += sprintf(*data+count,"Pad4: %s~\n",object->short_tdescription);
           count += sprintf(*data+count,"Pad5: %s~\n",object->short_pdescription);


		// Описание когда на земле
		if (GET_OBJ_DESC(object))
			count +=
			    sprintf(*data + count, "Desc: %s~\n", GET_OBJ_DESC(object) ? GET_OBJ_DESC(object) : "");
		// Описание при действии
		if (GET_OBJ_ACT(object))
			count += sprintf(*data + count, "ADsc: %s~\n", GET_OBJ_ACT(object) ? GET_OBJ_ACT(object) : "");
		// Тренируемый скилл
		if (GET_OBJ_SKILL(object))
			count += sprintf(*data + count, "Skil: %d~\n", GET_OBJ_SKILL(object));
		// Макс. прочность
		if (GET_OBJ_MAX(object))
			count += sprintf(*data + count, "Maxx: %d~\n", GET_OBJ_MAX(object));
		// Текущая прочность
		if (GET_OBJ_CUR(object))
			count += sprintf(*data + count, "Curr: %d~\n", GET_OBJ_CUR(object));
		// Материал
		if (GET_OBJ_MATER(object))
			count += sprintf(*data + count, "Mter: %d~\n", GET_OBJ_MATER(object));
		// Пол
		if (GET_OBJ_SEX(object))
			count += sprintf(*data + count, "Sexx: %d~\n", GET_OBJ_SEX(object));
		// Таймер
		if (GET_OBJ_TIMER(object))
			count += sprintf(*data + count, "Tmer: %d~\n", GET_OBJ_TIMER(object));
		// Вызываемое заклинание
		if (GET_OBJ_SPELL(object))
			count += sprintf(*data + count, "Spll: %d~\n", GET_OBJ_SPELL(object));
		// Уровень заклинания
		if (GET_OBJ_LEVEL(object))
			count += sprintf(*data + count, "Levl: %d~\n", GET_OBJ_LEVEL(object));
		// Наводимые аффекты
		*buf = '\0';
		tascii((int *) &GET_OBJ_AFFECTS(object), 4, buf);
		count += sprintf(*data + count, "Affs: %s~\n", buf);
		// Анти флаги
		*buf = '\0';
		tascii((int *) &GET_OBJ_ANTI(object), 4, buf);
		count += sprintf(*data + count, "Anti: %s~\n", buf);
		// Запрещающие флаги
		*buf = '\0';
		tascii((int *) &GET_OBJ_NO(object), 4, buf);
		count += sprintf(*data + count, "Nofl: %s~\n", buf);
		// Экстра флаги
		*buf = '\0';
		tascii((int *) &GET_OBJ_EXTRA(object, 0), 4, buf);
		count += sprintf(*data + count, "Extr: %s~\n", buf);
		// Флаги слотов экипировки
		*buf = '\0';
		tascii((int *) &GET_OBJ_WEAR(object), 1, buf);
		count += sprintf(*data + count, "Wear: %s~\n", buf);
		// Тип предмета
		count += sprintf(*data + count, "Type: %d~\n", GET_OBJ_TYPE(object));
		// Значение 0, Значение 1, Значение 2, Значение 3.
		for (i = 0; i < 4; i++)
			if (GET_OBJ_VAL(object, i))
				count += sprintf(*data + count, "Val%d: %d~\n", i, GET_OBJ_VAL(object, i));
		// Вес
		if (GET_OBJ_WEIGHT(object))
			count += sprintf(*data + count, "Weig: %d~\n", GET_OBJ_WEIGHT(object));
		// Цена
		if (GET_OBJ_COST(object))
			count += sprintf(*data + count, "Cost: %d~\n", GET_OBJ_COST(object));
		// Рента (снято)
		if (GET_OBJ_RENT(object))
			count += sprintf(*data + count, "Rent: %d~\n", GET_OBJ_RENT(object));
		// Рента (одето)
		if (GET_OBJ_RENTE(object))
			count += sprintf(*data + count, "RntQ: %d~\n", GET_OBJ_RENTE(object));
		// Владелец
		if (GET_OBJ_OWNER(object))
			count += sprintf(*data + count, "Ownr: %d~\n", GET_OBJ_OWNER(object));
		// Создатель
		if (GET_OBJ_MAKER(object))
			count += sprintf(*data + count, "Mker: %d~\n", GET_OBJ_MAKER(object));
		// Родитель
		if (GET_OBJ_PARENT(object))
			count += sprintf(*data + count, "Prnt: %d~\n", GET_OBJ_PARENT(object));
        // Container link
        if (object->in_obj)        
            count += sprintf(*data + count, "Clin: %u~\n", p2u(object->in_obj));
        if (object->contains)
            count += sprintf(*data + count, "Cont: %u~\n", p2u(object));
        if (GET_OBJ_RLVL(object))
            count += sprintf(*data + count, "Rlev: %d~\n", GET_OBJ_RLVL(object));

		// Аффекты
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			if (object->affected[j].location)
				count += sprintf(*data + count, "Afc%d: %d %d~\n", j,
						 object->affected[j].location, object->affected[j].modifier);

		// Доп описания
		for (descr = object->ex_description; descr; descr = descr->next)
			count += sprintf(*data + count, "Edes: %s~\n%s~\n",
					 descr->keyword ? descr->keyword : "",
					 descr->description ? descr->description : "");
	}
	*data += count;
	**data = '\0';
}


void tascii(int *pointer, int num_planes, char *ascii)
{int    i, c, found;

 for (i = 0, found = FALSE; i < num_planes; i++)
     {for (c = 0; c < 31; c++)
          if (*(pointer+i) & (1 << c))
             {found = TRUE;
              sprintf(ascii+strlen(ascii),"%c%d", c < 26 ? c+'a' : c-26+'A', i);
             }
     }
 if (!found)
    strcat(ascii,"0 ");
 else
    strcat(ascii," ");
}


void auto_equip(struct char_data *ch, struct obj_data *obj, int location)
{
  int j;

  /* Lots of checks... */
  if (location > 0) {	/* Was wearing it. */
    switch (j = (location - 1)) {
    case WEAR_LIGHT:
      break;
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
        location = LOC_INVENTORY;
      break;
    case WEAR_EAR_R:
    case WEAR_EAR_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_EAR))
        location = LOC_INVENTORY;
      break;
    case WEAR_EYES:
      if (!CAN_WEAR(obj, ITEM_WEAR_EYES))
        location = LOC_INVENTORY;
      break;
    case WEAR_BACKPACK:
      if (!CAN_WEAR(obj, ITEM_WEAR_BACKPACK))
        location = LOC_INVENTORY;
      break;
    case WEAR_NECK_1:
    case WEAR_NECK_2:
      if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
        location = LOC_INVENTORY;
      break;
	case WEAR_BODY:
      if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
        location = LOC_INVENTORY;
      break;
    case WEAR_HEAD:
      if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
        location = LOC_INVENTORY;
      break;
    case WEAR_LEGS:
      if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
        location = LOC_INVENTORY;
      break;
    case WEAR_FEET:
      if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
        location = LOC_INVENTORY;
      break;
    case WEAR_HANDS:
      if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
        location = LOC_INVENTORY;
      break;
    case WEAR_ARMS:
      if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
        location = LOC_INVENTORY;
      break;
    case WEAR_SHIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_ABOUT:
      if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
        location = LOC_INVENTORY;
      break;
    case WEAR_WAIST:
      if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_HOLD:
      if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
	break;
        location = LOC_INVENTORY;
      break;
    case WEAR_BOTHS:
      if (CAN_WEAR(obj,ITEM_WEAR_BOTHS))
	  break;
    default:
      location = LOC_INVENTORY;
    }

    if (location > 0) {	    // экипирован
      if (!GET_EQ(ch,j)) {
	/*
	 * Check the characters's alignment to prevent them from being
	 * zapped through the auto-equipping.
         */
         if (invalid_align(ch, obj)      ||
             invalid_anti_class(ch, obj) ||
              invalid_no_class(ch,obj))
          location = LOC_INVENTORY;
        else
          equip_char(ch, obj, j|0x80|0x40);
      } else {	/* Oops, saved a player with double equipment? */
        char aeq[128];
        sprintf(aeq, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
        mudlog(aeq, BRF, LVL_IMMORT, TRUE);
        location = LOC_INVENTORY;
      }
    }
  }
  if (location <= 0)	// в инвентаре
    obj_to_char(obj, ch);
}


int Crash_delete_crashfile(struct char_data * ch)
{int index;
 
 index = GET_INDEX(ch);
 if (index<0)
    return FALSE;
 if (!SAVEINFO(index))
    Crash_delete_files(index);
 return TRUE;
}

int Crash_delete_files(int index)
{ char  filename[MAX_STRING_LENGTH], name[MAX_NAME_LENGTH];
  FILE *fl;
  int retcode = FALSE;

  if (index<0)
     return retcode;

  strcpy(name, player_table[index].name);

  /*удаляем файл описания объектов*/
  if (!get_filename(name, filename, TEXT_CRASH_FILE))
     {log("SYSERR: Error deleting objects file for %s - unable to resolve file name.", name);
      retcode = FALSE;
     }
  else
     {if (!(fl = fopen(filename, "rb")))
         {if (errno != ENOENT)	/* if it fails but NOT because of no file */
             log("SYSERR: Error deleting objects file %s (1): %s", filename, strerror(errno));
          retcode = FALSE;
         }
      else
         {fclose(fl);
          /* if it fails, NOT because of no file */
          if (remove(filename) < 0 && errno != ENOENT)
             {log("SYSERR: Error deleting objects file %s (2): %s", filename, strerror(errno));
              retcode = FALSE;
             }
         }
     }

  /*удаляем файл таймеров*/
  if (!get_filename(name, filename, TIME_CRASH_FILE))
     {log("SYSERR: Error deleting timer file for %s - unable to resolve file name.", name);
      retcode = FALSE;
     }
  else
     {if (!(fl = fopen(filename, "rb")))
         {if (errno != ENOENT)	/* if it fails but NOT because of no file */
             log("SYSERR: deleting timer file %s (1): %s", filename, strerror(errno));
          retcode = FALSE;
         }
      else
         {fclose(fl);
          /* if it fails, NOT because of no file */
          if (remove(filename) < 0 && errno != ENOENT)
             {log("SYSERR: deleting timer file %s (2): %s", filename, strerror(errno));
              retcode = FALSE;
             }
         }
     }
  return (retcode);
}

/********* Timer utils: create, read, write, list, timer_objects *********/

void Crash_clear_objects(int index)
{
  int i=0, rnum;
  Crash_delete_files(index);
  if (SAVEINFO(index))
     {for (;i<SAVEINFO(index)->rent.nitems; i++)
          if (SAVEINFO(index)->time[i].timer >= 0    &&
              (rnum = real_object(SAVEINFO(index)->time[i].vnum)) >= 0
             )
             obj_index[rnum].stored--;
      free(SAVEINFO(index));
      SAVEINFO(index)=NULL;
     }
}

void Crash_reload_timer(int index)
{
  int i=0, rnum;

   if (SAVEINFO(index))
     {for (;i<SAVEINFO(index)->rent.nitems; i++)
          if (SAVEINFO(index)->time[i].timer >= 0    &&
              (rnum = real_object(SAVEINFO(index)->time[i].vnum)) >= 0
             )
             obj_index[rnum].stored--;
      free(SAVEINFO(index));
      SAVEINFO(index)=NULL;
     }

  if (!Crash_read_timer(index, FALSE))
     {sprintf(buf, "SYSERR: Невозможно прочитать файл таймера для %s..",
              player_table[index].name);
      mudlog(buf, BRF, MAX(LVL_IMMORT, LVL_GOD), TRUE);
     }

}

void Crash_create_timer(int index, int num)
{if (SAVEINFO(index))
    free(SAVEINFO(index));
 
if (((sizeof(struct save_info) + sizeof(struct save_time_info) * num)) * sizeof(char) <= 0)
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);
	
if (!((((player_table+index)->timer)) = (save_info *) calloc (((sizeof(struct save_info)
	+ sizeof(struct save_time_info) * num)), sizeof(char)))) {
		perror("SYSERR: malloc failure");
		abort();
	}


 memset(SAVEINFO(index),0,MAKESIZE(num));
}

int Crash_write_timer(int index)
{
  FILE   *fl;
  char   fname[MAX_STRING_LENGTH];
  char   name[MAX_NAME_LENGTH];

  strcpy(name,player_table[index].name);
  if (!SAVEINFO(index))
     {log("SYSERR: Error writing %s timer file - no data.", name);
      return FALSE;
     }
  if (!get_filename(name, fname, TIME_CRASH_FILE))
     {log("SYSERR: Error writing %s timer file - unable to resolve file name.", name);
      return FALSE;
     }
  if (!(fl = fopen(fname,"wb")))
     {log("[WriteTimer] Error writing %s timer file - unable to open file %s.", name, fname);
      return FALSE;
     }
  fwrite(SAVEINFO(index), SAVESIZE(index), 1, fl);
  fclose(fl);
  return TRUE;
}


int Crash_read_timer(int index, int temp)
{
  FILE   *fl;
  char   fname[MAX_INPUT_LENGTH];
  char   name[MAX_NAME_LENGTH];
  int    size = 0, count = 0, rnum, num = 0;
  struct save_rent_info rent;
  struct save_time_info info;

  strcpy(name,player_table[index].name);
  if (!get_filename(name, fname, TIME_CRASH_FILE))
     {log("[ReadTimer] Error reading %s timer file - unable to resolve file name.", name);
      return FALSE;
     }
  if (!(fl = fopen(fname,"rb")))
     {if (errno != ENOENT)
         {log("SYSERR: fatal error opening timer file %s", fname);
          return FALSE;
         }
      else
         {log("[ReadTimer] %s has no timer file.", name);
          return TRUE;
         }
     }

  fseek(fl, 0L, SEEK_END);
  size = ftell(fl);
  rewind(fl);
  if ((size = size - sizeof(struct save_rent_info)) < 0 ||
      size % sizeof(struct save_time_info)
     )
     {log("WARNING:  Файл таймера %s поврежден!", fname);
      return FALSE;
     }

  sprintf (buf,"[ReadTimer] Чтение файл-таймера %s для %s :", fname, name);
  fread(&rent, sizeof(struct save_rent_info), 1, fl);
  switch (rent.rentcode)
     {case RENT_RENTED:
           strcat(buf, " Рента ");
           break;
      case RENT_CRASH:
           if (rent.time<1001651000L) //креш-сейв до Sep 28 00:26:20 2001
              rent.net_cost_per_diem = 0; //бесплатно!
           strcat(buf, " Разрушение ");
           break;
      case RENT_CRYO:
           strcat(buf, " Крио ");
           break;
      case RENT_TIMEDOUT:
           strcat(buf, " Просроченно ");
           break;
      case RENT_FORCED:
           strcat(buf, " Принуждение ");
           break;
	  case RENT_CHEST:
           strcat(buf, " Склад ");
           break;
      default:
           log("[ReadTimer] Ошибка считывания %s таймер-файла - неизвестный код ренты.", name);
           return FALSE;
           //strcat(buf, " Undef ");
           //rent.rentcode = RENT_CRASH;
           break;
     }
  strcat(buf, "код ренты.");
  log(buf);
  Crash_create_timer(index, rent.nitems);
  player_table[index].timer->rent = rent;
  
  for(; count < rent.nitems && !feof(fl); count++)
  { fread(&info, sizeof(struct save_time_info), 1, fl);
      if (ferror(fl))
	  { log("SYSERR: I/O Ошибка чтения %s - файл-таймера .", name);
          fclose(fl);
          free(SAVEINFO(index));
          SAVEINFO(index)=NULL;
          return FALSE;
       }
      if (info.vnum && info.timer >= -1)
         player_table[index].timer->time[num++] = info;
      else
         log("[ReadTimer] Warning: incorrect vnum (%d) or timer (%d) while reading %s timer file.",
              info.vnum, info.timer, name);

      if (info.timer >= 0 && (rnum = real_object(info.vnum)) >= 0 && !temp)
            obj_index[rnum].stored++;
  }
  fclose(fl);
  if (rent.nitems != num)
     {log("[ReadTimer] Ошибка чтения %s файл таймера - файл испорчен.", fname);
      free(SAVEINFO(index));
      SAVEINFO(index)=NULL;
      return FALSE;
     }
  else
     return TRUE;
}

void Crash_timer_obj(int index, long time)
{
  char   name[MAX_NAME_LENGTH];
  int    nitems = 0, idelete = 0, ideleted = 0, rnum, timer, i;
  int    rentcode, timer_dec;


#ifndef USE_AUTOEQ
        return;
#endif

  strcpy(name, player_table[index].name);

  if (!player_table[index].timer)
     {log("[TO] %s - данные не сохранены.", name);
      return;
     }
  rentcode  = SAVEINFO(index)->rent.rentcode;
  timer_dec = time - SAVEINFO(index)->rent.time;
  timer_dec = ((timer_dec * 10)/SAVEINFO(index)->rent.m_nServis)/10;

  // удаляем просроченные файлы ренты
  if (rentcode == RENT_RENTED &&
      timer_dec > rent_file_timeout * SECS_PER_REAL_DAY)
     {Crash_clear_objects(index);
      log("[TO] Удаление %s сохраненная информация вышла за пределы отведенного времени.", name);
      return;
     }
  else
  if (rentcode != RENT_CRYO &&
      timer_dec > crash_file_timeout * SECS_PER_REAL_DAY)
     {Crash_clear_objects(index);
      strcpy(buf, "");
      switch (rentcode)
          {case RENT_CRASH:
                log("[TO] Удаление разрушенно-сохраненной информации для %s  - вышло за пределы отведенного времени.", name);
                break;
           case RENT_FORCED:
                log("[TO] Удаление принудительно-сохраненной информации для %s  - вышло за пределы отведенного времени.", name);
                break;
           case RENT_TIMEDOUT:
                log("[TO] Удаление автосохраненной информации для %s  - вышло за пределы отведенного времени.", name);
                break;
           default:
                log("[TO] Удаление неопределенной информации для %s  - вышло за пределы отведенного времени.", name);
                break;
         }
      return;
     }

  timer_dec = (timer_dec/SECS_PER_MUD_HOUR) + (timer_dec%SECS_PER_MUD_HOUR ? 1 : 0);

  // уменьшаем таймеры
  nitems = player_table[index].timer->rent.nitems;
  //log("[TO] Checking items for %s (%d items, rented time %dmin):",
//      name, nitems, timer_dec);
  //sprintf (buf,"[TO] Checking items for %s (%d items) :", name, nitems);
  //mudlog(buf, BRF, LVL_IMMORT, TRUE);
  for (i = 0; i < nitems; i++)
      if (player_table[index].timer->time[i].timer >= 0)
         {rnum = real_object(player_table[index].timer->time[i].vnum);
          timer = player_table[index].timer->time[i].timer;
          if (timer<timer_dec)
             {player_table[index].timer->time[i].timer = -1;
              idelete++;
              if (rnum >= 0)
                 {obj_index[rnum].stored--;
                  log("[TO] Игрок %s : предмет %s удален - истечение времени, отведенного для сохранения", name, (obj_proto+rnum)->short_description);
                 }
             }
         }
      else
         ideleted++;

 // log("Objects (%d), Deleted (%d)+(%d).", nitems, ideleted, idelete);

  // если появились новые просроченные объекты, обновляем файл таймеров
  if (idelete)
     if (!Crash_write_timer(index))
        {sprintf(buf, "SYSERR: [TO] Ошибка сохранения таймера в файл для %s.", name);
         mudlog(buf, CMP, MAX(LVL_IMMORT, LVL_GOD), TRUE);
        }
}

void Crash_list_objects(struct char_data * ch, int index)
{
  int    i=0, rnum;
  struct save_time_info data;
  long timer_dec;
  float num_of_days;

  if (!SAVEINFO(index))
     return;

  timer_dec = time(0) - SAVEINFO(index)->rent.time;
  timer_dec = ((timer_dec * 10)/SAVEINFO(index)->rent.m_nServis)/10;
  num_of_days = (float) timer_dec / SECS_PER_REAL_DAY;
  timer_dec = (timer_dec/SECS_PER_MUD_HOUR) + (timer_dec%SECS_PER_MUD_HOUR ? 1 : 0);

  strcpy(buf, "Код ренты - ");
  switch (SAVEINFO(index)->rent.rentcode)
     {
      case RENT_RENTED:
           strcat(buf, "Рента.\r\n");
           break;
      case RENT_CRASH:
           strcat(buf, "Разрушение.\r\n");
           break;
      case RENT_CRYO:
           strcat(buf, "Заморозка.\r\n");
           break;
      case RENT_TIMEDOUT:
           strcat(buf, "Выход времени ожидания.\r\n");
           break;
      case RENT_FORCED:
           strcat(buf, "Принудительно.\r\n");
           break;
	  case RENT_CHEST:
           strcat(buf, " Склад ");
           break;
      default:
           strcat(buf, "UNDEF!\r\n");
           break;
     }
  for(; i<SAVEINFO(index)->rent.nitems; i++)
     {data = SAVEINFO(index)->time[i];
      if ((rnum=real_object(data.vnum)) > -1)
         {sprintf(buf + strlen(buf), " [%5d] (%5dau) <%6d> %-20s\r\n",
          data.vnum, GET_OBJ_RENT(obj_proto+rnum),
          MAX(-1, data.timer - timer_dec), (obj_proto+rnum)->short_description);
         }
      else
         {sprintf(buf + strlen(buf), " [%5d] (?????au) <%2d> %-20s\r\n",
          data.vnum, MAX(-1, data.timer - timer_dec), "БЕЗ ПРОТОТИПА");
         }
      if (strlen(buf) > MAX_STRING_LENGTH - 80)
         {strcat(buf, "** Excessive rent listing. **\r\n");
          break;
         }
     }
  send_to_char(buf, ch);
  sprintf(buf, "Время в ренте: %ld тиков.\r\n",
          timer_dec);
  send_to_char(buf, ch);
  sprintf(buf, "Предметов: %d. Стоимость: (%d в день) * (%1.2f дней) = %d.\r\n", 
          SAVEINFO(index)->rent.nitems,
          SAVEINFO(index)->rent.net_cost_per_diem,
          num_of_days,
          (int) (num_of_days * SAVEINFO(index)->rent.net_cost_per_diem));
  send_to_char(buf, ch);
}

void Crash_listrent(struct char_data * ch, char *name)
{ 
  int index;

  index=get_ptable_by_name(name);

  if (index<0)
     {send_to_char("Нет такого игрока.\r\n", ch);
      return;
     }

  if (!SAVEINFO(index))
     if (!Crash_read_timer(index, TRUE))
        {sprintf(buf, "Невозможно прочитать файл таймера с именем %s.\r\n", name);
         send_to_char(buf, ch);
        }
     else
        if (!SAVEINFO(index))
          {sprintf(buf, "%s не имеет файла ренты.\r\n", CAP(name));
           send_to_char(buf, ch);
          }
        else
          {sprintf(buf, "%s находится в игре. Содержимое файла ренты:\r\n",
                   CAP(name));
           send_to_char(buf, ch);
           Crash_list_objects(ch, index);
           free(SAVEINFO(index));
           SAVEINFO(index)=NULL;
          }
  else
   {sprintf(buf, "%s находится в ренте. Содержимое файла ренты:\r\n",
            CAP(name));
    send_to_char(buf, ch);
    Crash_list_objects(ch, index);
   }
}



struct container_list_type {
 struct obj_data            *tank;
 struct container_list_type *next;
 int    location;
};


// разобраться с возвращаемым значением
/*******************  load_char_objects ********************/
int Crash_load(CHAR_DATA * ch)
{
  FILE  *fl;
  char   fname[MAX_STRING_LENGTH], *data, *readdata;
  int    cost, num_objs = 0, reccount, fsize, error, index, CHst;
  float  num_of_days;
  struct obj_data *obj, *obj2, *obj_list=NULL;
  int    location, rnum;
  struct container_list_type *tank_list = NULL, *tank, *tank_to;
  long   timer_dec;
  bool   need_convert_character_objects = 0;//проверка на лоад вещей по новому формату.

  if ((index=GET_INDEX(ch)) < 0)
     return (1);

  Crash_reload_timer(index);

  Whouse &c = GET_CHEST(ch).m_chest;
  if (!c.empty())
     c.clear();


  if (!SAVEINFO(index))
     {sprintf(buf, "Вхождение в игру %s без экипировки.", GET_TNAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      return (1);
     }

  switch (RENTCODE(index))
  {
   case RENT_RENTED:
     sprintf(buf, "%s разренчивается и входит в Мир.", GET_NAME(ch));
     break;
   case RENT_CRASH:
     sprintf(buf, "%s восстановливается после крэша и входит в Мир.", GET_NAME(ch));
     break;
   case RENT_CRYO:
     sprintf(buf, "%s выходит из криосохранения и входит в Мир.", GET_NAME(ch));
     break;
   case RENT_FORCED:
     sprintf(buf, "%s восстанавливается после принудительного автосохранения и входит в Мир.", GET_NAME(ch));
     break;
   case RENT_TIMEDOUT:
     sprintf(buf, "%s восстанавливается после автосохранения и входит в Мир.", GET_NAME(ch));
     break;
	case RENT_CHEST:
      sprintf(buf, "%s восстанавливается после автосохранения склада и входит в Мир.", GET_NAME(ch));
    break;
   default:
     sprintf(buf, "SYSERR: %s входит в игру с неопределенным ренткодом %d.", GET_NAME(ch), RENTCODE(index));
     mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
     send_to_char("\r\n** Неизвестный код ренты **\r\n"
                  "Проблемы с восстановлением Ваших вещей из файла.\r\n"
                  "Обращайтесь за помощью к Богам.\r\n", ch);
     Crash_clear_objects(index);
     return(1);
     break;
  }
  mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

  //Деньги за постой
  num_of_days = (float) (time(0) - SAVEINFO(index)->rent.time) / SECS_PER_REAL_DAY;
  sprintf(buf,"%s находил%s %1.2f %s в ренте.",GET_NAME(ch), GET_CH_SUF_2(ch),
	      num_of_days, desc_count((int)num_of_days, WHAT_DAY));
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
  cost = (int) (SAVEINFO(index)->rent.net_cost_per_diem * num_of_days);
  CHst = (int)(SAVEINFO(index)->rent.m_nCostChest * num_of_days);//цена за использование склада
  cost = MAX(0, cost);
  CHst = MAX(0, CHst);
 
  if ((RENTCODE(index) == RENT_CRASH || 
       RENTCODE(index) == RENT_FORCED) &&
       SAVEINFO(index)->rent.time + 
             free_crashrent_period*SECS_PER_REAL_HOUR > time(0)
     )
     {//Бесплатная рента, если выйти в течение 2 часов после ребута или креша
      sprintf(buf,"           &G*** В этот раз постой для Вас оказался бесплатным ***&n\r\n");
      send_to_char(buf,ch);
      sprintf(buf, "%s entering game, free crashrent.", GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
     }
  else
  if ((cost + CHst) > GET_GOLD(ch) + GET_BANK_GOLD(ch))
     {sprintf(buf,"&WВы находились на постое %1.2f дней.\n\r"
                  "Вам предъявили счет на %d %s за постой (%d %s в день).\r\n"
				  "Так же Вам предъявили счет за использование склада на %d %s в день.\r\n"
                  "Но все, что у Вас было - %ld %s... Увы. Все Ваши вещи переданы монстрам.&n\n\r",
              num_of_days, cost, desc_count(cost, WHAT_MONEYu), SAVEINFO(index)->rent.net_cost_per_diem,
              desc_count(SAVEINFO(index)->rent.net_cost_per_diem, WHAT_MONEYa), CHst, desc_count(CHst, WHAT_MONEYu),
              GET_GOLD(ch) + GET_BANK_GOLD(ch), desc_count(GET_GOLD(ch) + GET_BANK_GOLD(ch),WHAT_MONEYa));
      send_to_char(buf,ch);
      sprintf(buf, "%s: rented equipment lost (no $).", GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      GET_GOLD(ch) = GET_BANK_GOLD(ch) = 0;
      Crash_clear_objects(index);
      return (2);
     }
  else
     {if (cost || CHst)
		{ sprintf(buf,"&WВы были на постое %1.2f дней.\n\r"
                      "При этом Вам пришлось заплатить %d %s за постой (%d %s за полный день).\r\n"
					  "При этом Вам пришлось заплатить %d %s за склад  (%d %s за полный день)\r\n&n",
                  num_of_days, cost, desc_count(cost, WHAT_MONEYu), SAVEINFO(index)->rent.net_cost_per_diem,
                  desc_count(SAVEINFO(index)->rent.net_cost_per_diem, WHAT_MONEYa), CHst, desc_count(CHst, WHAT_MONEYa),
				  SAVEINFO(index)->rent.m_nCostChest, desc_count(SAVEINFO(index)->rent.m_nCostChest, WHAT_MONEYu));

                 send_to_char(buf, ch);
         
				GET_GOLD(ch) -= MAX((cost + CHst) - GET_BANK_GOLD(ch), 0);
				GET_BANK_GOLD(ch) = MAX(GET_BANK_GOLD(ch) - (cost + CHst), 0);
		}
     }

  // Чтение описаний объектов в буфер
  if (!get_filename(GET_NAME(ch), fname, TEXT_CRASH_FILE) ||
      !(fl = fopen(fname, "r+b"))
     )
     {send_to_char("\r\n** Нет файла рены для вещей **\r\n"
		   "Проблемы с восстановлением Ваших вещей из файла ренты.\r\n"
		   "Обращайтесь за помощью к Богам.\r\n", ch);
      Crash_clear_objects(index);
      return(1);
     }
  fseek(fl,0L,SEEK_END);
  fsize = ftell(fl);
  if (!fsize)
     {fclose(fl);
      send_to_char("\r\n** Файл описания вещей в ренте пустой **\r\n"
		   "Проблемы с восстановлением Ваших вещей из файла ренты.\r\n"
		   "Обращайтесь за помощью к Богам.\r\n", ch);
      Crash_clear_objects(index);
      return(1);
     }

  CREATE(readdata, char, fsize+1);
  fseek(fl,0L,SEEK_SET);
  if (!fread(readdata,fsize,1,fl) || ferror(fl) || !readdata)
     {fclose(fl);
      send_to_char("\r\n** Ошибка чтения файла описания вещей для ренты **\r\n"
		   "Проблемы с восстановлением Ваших вещей из файла ренты.\r\n"
		   "Обращайтесь за помощью к Богам.\r\n", ch);
      log("Ошибка памяти или не может считать %s(%d)...",fname,fsize);
      free(readdata);
      Crash_clear_objects(index);
      return(1);
     };
  fclose(fl);

  data = readdata;
  *(data+fsize) = '\0';


  // Проверка в каком формате записана информация о персонаже.
	if (!strn_cmp(readdata, "*", 1))//@- стоит этот знак для переопределения, в дальнейшем проверить и вернуть назад
		need_convert_character_objects = 1;//но в старом формате у меня стоит для записи звезджочка, и тут буду ее юзать

  // Создание объектов
  timer_dec = time(0) - SAVEINFO(index)->rent.time;
  timer_dec = (timer_dec/SECS_PER_MUD_HOUR) + (timer_dec%SECS_PER_MUD_HOUR ? 1 : 0);

  std::map<int, obj_data *> containers;
  std::map<obj_data *, int> cobjects;

  for (fsize = 0, reccount = SAVEINFO(index)->rent.nitems;
       reccount > 0 && *data && *data != END_CHAR; reccount--, fsize++)
	   { if (need_convert_character_objects) {
			// Формат новый => используем новую функцию
			if ((obj = read_one_object_new(&data, &error)) == NULL)
			{ send_to_char("Ошибка при чтении файла объектов.\r\n", ch);
				sprintf(buf, "SYSERR: Чтение объектов потерпело неудачу для %s, ошибка %d, остановка чтения.",
					GET_NAME(ch), error);
				mudlog(buf, BRF, LVL_IMMORT, TRUE);
				continue;
			}
		}
	   else 
	   {   if ((obj = read_one_object(&data, &error)) == NULL)
          {send_to_char("Ошибка при чтении файла обьектов, остановка чтения.\r\n", ch);
           sprintf(buf, "SYSERR: Objects reading fail for %s error %d, stop reading.",
                   GET_NAME(ch), error);
           mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
		   continue;
           //break;
          }
	   }

       if (error)
          {sprintf(buf, "WARNING: Error #%d reading item #%d from %s.", error, num_objs,fname);
           mudlog(buf, CMP, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE);
          }

       if (GET_OBJ_VNUM(obj) != SAVEINFO(index)->time[fsize].vnum)
          {//send_to_char("Нет соответствия заголовков - чтение предметов прервано.\r\n", ch);
           sprintf(buf, "SYSERR: Objects reading fail for %s (2).", GET_NAME(ch));
           mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
           extract_obj(obj);	
           continue;
          }

       // проверяем таймер
       if (SAVEINFO(index)->time[fsize].timer > 0 &&
           (rnum=real_object(SAVEINFO(index)->time[fsize].vnum)) >= 0)
           obj_index[rnum].stored--;

          
	        GET_OBJ_TIMER(obj) = MAX(-1, (SAVEINFO(index)->time[fsize].timer * 10 -
				                 (timer_dec * 10)/ SAVEINFO(index)->rent.m_nServis)/10);


         if (GET_OBJ_TIMER(obj) <= 0)             //дописал 21.10.2003г.
            {sprintf(buf,"%s%s рассыпал%s от длительного использования.\r\n",
             CCWHT(ch,C_NRM),CAP(obj->short_description),GET_OBJ_SUF_2(obj));
             send_to_char(buf,ch);
             extract_obj(obj);
             continue;
            }


		 // очищаем ZoneDecay объедки 
		if (OBJ_FLAGGED(obj, ITEM_ZONEDECAY)) {
			sprintf(buf, "%s рассыпал%s в прах.\r\n", CAP(obj->short_description), GET_OBJ_SUF_2(obj));
			send_to_char(buf, ch);
			extract_obj(obj);
			continue;
		}


       // Недопустимый класс предмета и непредназначенный для постоя.
       if (invalid_anti_class(ch,obj) || invalid_unique(ch,obj) || Crash_is_unrentable(obj))
          {sprintf(buf,"%s рассыпал%s, как запрещенн%s для Вас.\r\n",
                   CAP(obj->short_description),GET_OBJ_SUF_2(obj),GET_OBJ_SUF_3(obj));
           send_to_char(buf,ch);
           extract_obj(obj);
           continue;
          }

	   // Обработка предметов, которые лежат на складе
		if (obj->chest)
		{ GET_CHEST(ch).m_chest.push_back( obj );
			continue;
		}

        // 
        if (obj->clin)
            cobjects[obj] = obj->clin;
        if (obj->cont)
            containers[obj->cont] = obj;            

       obj->next_content = obj_list;
       obj_list          = obj;
      }

  free(readdata);

  for (obj = obj_list; obj; obj = obj2)
      {obj2 = obj->next_content;
       obj->next_content = NULL;
       if (obj->worn_on >= 0)
          {// Equipped or in inventory
           if (obj2 && obj2->worn_on < 0 && GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
              {// This is container and it is not free
               CREATE(tank, struct container_list_type, 1);
               tank->next     = tank_list;
               tank->tank     = obj;
               tank->location = 0;
               tank_list      = tank;
              }
           else
              {while (tank_list)
                     {// Clear tanks list
                      tank      = tank_list;
                      tank_list = tank->next;
                      free(tank);
                     }
              }
           location = obj->worn_on;
           obj->worn_on = 0;

           auto_equip(ch, obj, location);
          }
       else
          {if (obj2 && obj2->worn_on < obj->worn_on && GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
              {// This is container and it is not free
               tank_to        = tank_list;
               CREATE(tank, struct container_list_type, 1);
               tank->next     = tank_list;
               tank->tank     = obj;
               tank->location = obj->worn_on;
               tank_list      = tank;
              }
           else
              {while ((tank_to = tank_list))
                    // Clear all tank than less or equal this object location
                     if (tank_list->location > obj->worn_on)
                        break;
                     else
                        {tank      = tank_list;
                         tank_list = tank->next;
                         free(tank);
                        }
              }
           obj->worn_on = 0;
           if (tank_to)
              obj_to_obj(obj,tank_to->tank);
           else
              obj_to_char(obj,ch);
          }
      }
  while (tank_list)
        {//Clear tanks list
         tank      = tank_list;
         tank_list = tank->next;
         free(tank);
        }
  affect_total(ch);
  free(SAVEINFO(index));
  SAVEINFO(index)=NULL;

  // now put object in containers etc
  std::map<obj_data *, int>::iterator it = cobjects.begin(), it_end = cobjects.end();
  for (; it != it_end; ++it)
  {
      int container = it->second;
      std::map<int, obj_data *>::iterator ct = containers.find(container);
      if (ct != containers.end())
      {
          // put object in container
          obj_data *obj = it->first;
          obj_data *cont = ct->second;
          obj_from_obj(obj);
          obj_from_char(obj);
          obj_to_obj(obj, cont);
      }     
  }
  return (0);
}


void Crash_restore_weight(OBJ_DATA * obj)
{
  
		for (; obj; obj=obj->next_content)
		{  Crash_restore_weight(obj->contains);
			if (obj->in_obj)
			 GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
		}
}

void Crash_extract_objs(OBJ_DATA * obj)
{
  int rnum;
  struct obj_data * next;
  for (; obj; obj=next)
     {next=obj->next_content;
      Crash_extract_objs(obj->contains);
      if ((rnum = real_object(GET_OBJ_VNUM(obj))) >= 0 && GET_OBJ_TIMER(obj)>=0)
             obj_index[rnum].stored++;
      extract_obj(obj);
     }
}

void CrashChestItems(CHAR_DATA * ch)
{ int rnum;

   const Whouse &chest = GET_CHEST(ch).m_chest;

	for (Whouse::const_iterator p = chest.begin(), pe=chest.end(); p != pe; ++p)
	  {	if ((rnum = real_object(GET_OBJ_VNUM(*p))) >= 0 && GET_OBJ_TIMER(*p)>=0)
             obj_index[rnum].stored++;
	           extract_obj(*p);
	  }
	GET_CHEST(ch).m_chest.clear();
}

int Crash_is_unrentable(OBJ_DATA * obj)
{
  if (!obj)
     return FALSE;

  if (IS_OBJ_STAT(obj, ITEM_NORENT) ||
      GET_OBJ_RENT(obj) < 0         ||
      GET_OBJ_RNUM(obj) <= NOTHING  ||
      GET_OBJ_TYPE(obj) == ITEM_KEY ||
      IS_OBJ_STAT(obj, ITEM_NODROP) //что бы с проклятыми на постой не ходил
	  )
     return TRUE;

  return FALSE;
}

/*
 * Get !RENT items from equipment to inventory and
 * extract !RENT out of worn containers.
 */
void Crash_extract_norents(OBJ_DATA * obj)
{ struct obj_data *next;
  for (; obj; obj=next)
      {next=obj->next_content;
       Crash_extract_norents(obj->contains);
       if (Crash_is_unrentable(obj))
          extract_obj(obj);
      }
}


void Crash_extract_norent_eq(CHAR_DATA * ch)
{
  int j;

  for (j = 0; j < NUM_WEARS; j++) {
    if (GET_EQ(ch, j) == NULL)
      continue;

    if (Crash_is_unrentable(GET_EQ(ch, j)))
      obj_to_char(unequip_char(ch, j), ch);
    else
      Crash_extract_norents(GET_EQ(ch, j));
  }
}


int Crash_calculate_rent(OBJ_DATA * obj)
{ int cost = 0;
  for (; obj; obj=obj->next_content)
	 { if (Crash_is_unrentable(obj)) 
		continue;
	  cost +=Crash_calculate_rent(obj->contains);
       cost += MAX(0, GET_OBJ_RENT(obj));
      }
  return (cost);
}

int Crash_calculate_rent_eq(OBJ_DATA * obj)
{
	int cost = 0;
	for (; obj; obj = obj->next_content) {
		cost += Crash_calculate_rent(obj->contains);
		cost += MAX(0, GET_OBJ_RENTE(obj));
	}
	return (cost);
}


int Crash_calcitems(OBJ_DATA * obj)
{ int i = 0;
    
    for (; obj; obj=obj->next_content)
    {    if (Crash_is_unrentable(obj)) //дописано 
			continue;	
	    i ++;
        i += Crash_calcitems(obj->contains);
	}
	return (i);
}

#define CRASH_LENGTH   0x40000
#define CRASH_DEPTH    0x1000

int Crashitems;
char  *Crashbufferdata = NULL;
char  *Crashbufferpos;

void Crash_save(int iplayer, OBJ_DATA * obj, int location)
{
  for (; obj; obj=obj->next_content)
	{  //порядок менять не следует, нужно что бы вычитался вес, иначе каждый тик вес добавляется
	  //потому что не учитывается.	  
		if (obj->in_obj)
          GET_OBJ_WEIGHT(obj->in_obj) -= GET_OBJ_WEIGHT(obj);
		
		if (Crash_is_unrentable(obj))
			continue; //дописано

	       Crash_save(iplayer, obj->contains, MIN(0, location) - 1);
       if (iplayer >= 0 &&
           Crashitems < MAX_SAVED_ITEMS &&
           Crashbufferpos - Crashbufferdata + CRASH_DEPTH < CRASH_LENGTH
          )
          {write_one_object(&Crashbufferpos,obj,location);
           SAVEINFO(iplayer)->time[Crashitems].vnum  = GET_OBJ_VNUM(obj);
           SAVEINFO(iplayer)->time[Crashitems].timer = GET_OBJ_TIMER(obj);
           Crashitems++;

          }
      }
}

/********************* Сохранение предметов игроков на складе ********************************/

void CalcChestItems(CHAR_DATA *ch)
{ 
	Whouse &chest = GET_CHEST(ch).m_chest;
	Whouse tmp;
	tmp.resize(chest.size());
	for (int i = 0, e = chest.size(); i < e; ++i)
		tmp[i] = chest[i];
	
	//for (Whouse::const_iterator p = chest.begin(); p != chest.end(); ++p)
	//	Crash_save(GET_INDEX(ch), *p, 0);
	for (int j = 0, je = tmp.size(); j < je; ++j)
		Crash_save(GET_INDEX(ch), tmp[j], 0);
	
}

int save_char_objects(CHAR_DATA *ch, int savetype, int rentcost, int Num)
{ FILE *fl;
  char fname[MAX_TITLE_LENGTH];//не может длинна быть больше имени чара (макс 80)
  struct save_rent_info rent;
  int j, num=0, iplayer=-1, cost = 0;

  if (IS_NPC(ch))
     return FALSE;

  if ((iplayer = GET_INDEX(ch)) < 0)
     {sprintf(buf,"[SYSERR] Store file '%s' - INVALID ID %d",GET_NAME(ch), iplayer);
      mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_IMMORT), TRUE);
      return FALSE;
     }


    if (savetype != RENT_CRASH)
     {// не crash и не ld
      Crash_extract_norent_eq(ch);
      Crash_extract_norents(ch->carrying);
     }
	 
	cost = Crash_calculate_rent(ch->carrying);
	num += Crash_calcitems(ch->carrying);

	  for (j = 0; j < NUM_WEARS; j++)
	  {	if (GET_EQ(ch, j))
		{ num  += Crash_calcitems(GET_EQ(ch, j)); 
		  cost += Crash_calculate_rent(GET_EQ(ch, j));
		}
	  } 		

	num += GET_CHEST(ch).m_chest.size();

  if (!num)
     {Crash_delete_files(iplayer);
      return FALSE;
     }
 
//тут бы в структуру добавить цену складскую с модификаторами (срок и цена, от разрушения зависимость)
  rent.rentcode				 = savetype;
  rent.net_cost_per_diem	 = cost;//если рентился не постоем, то цена за шмутки списывается по полной.
  rent.time					 = time(0);
  rent.nitems				 = num;//количество предметов ренты

  //CRYO-rent надо дорабатывать либо выкидывать нафиг
  if (savetype == RENT_CRYO)
     {rent.net_cost_per_diem = 0;
      GET_GOLD(ch)           = MAX(0, GET_GOLD(ch) - cost);
     }
  if (savetype == RENT_RENTED)
     rent.net_cost_per_diem  = rentcost;
 // rent.gold					 = GET_GOLD(ch); за ненадобностью в настоящее время.
 // rent.account				 = GET_BANK_GOLD(ch);

  


  if (Crashbufferdata)
   free(Crashbufferdata);//?

  CREATE(Crashbufferdata, char, CRASH_LENGTH);

  Crashitems	  = 0;
  Crashbufferpos  = Crashbufferdata;
  *Crashbufferpos = '\0';


  switch (Num)
	{ case 1:
		rent.m_nCostChest = 550;
		break;
	  case 2:
		rent.m_nCostChest = 1200;
		break;
	  case 3:
		rent.m_nCostChest = 3500;
		break;
	  case 4:
		rent.m_nCostChest = 10500;
		break;
	  case 5:
		rent.m_nCostChest = 20000;
		break;
	  default:
		rent.m_nCostChest = 550;//по умолчанию (это когда концом уходят)
	   break;
  }
  
  rent.m_nServis = MAX(1, Num);

  if(!GET_CHEST(ch).m_chest.size())
	  rent.m_nCostChest = 0;
 
  
  Crash_create_timer(iplayer, num);
  SAVEINFO(iplayer)->rent = rent;
  
  CalcChestItems(ch);


      for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j))
	  { Crash_save(iplayer, GET_EQ(ch, j), j + 1);
        Crash_restore_weight(GET_EQ(ch, j));
	  }
	  	Crash_save(iplayer, ch->carrying, 0);
		Crash_restore_weight(ch->carrying);

  

  if (savetype != RENT_CRASH)
  {  for (j = 0; j < NUM_WEARS; j++)
        if (GET_EQ(ch, j))
      Crash_extract_objs(GET_EQ(ch, j));
      Crash_extract_objs(ch->carrying);
	//}
   CrashChestItems(ch);
   }

  if (get_filename(GET_NAME(ch), fname, TEXT_CRASH_FILE))
     {if (!(fl = fopen(fname,"w")) )
         {sprintf(buf,"[SYSERR] Store objects file '%s'- MAY BE LOCKED.",fname);
          mudlog(buf, NRM, MAX(LVL_IMMORT, LVL_GOD), TRUE);
          free(Crashbufferdata);
          Crashbufferdata = NULL;
          Crash_delete_files(iplayer);
          return FALSE;
         }
      fprintf(fl,"* Items file\n%s\n$\n$\n",Crashbufferdata);//* является меткой для перехода на новый формат сохранения предметов
      fclose(fl);
     }
  else
     {free(Crashbufferdata);
      Crashbufferdata = NULL;
      Crash_delete_files(iplayer);
      return FALSE;
     }

  free(Crashbufferdata);
  Crashbufferdata = NULL;
  if (!Crash_write_timer(iplayer))
     {Crash_delete_files(iplayer);
      return FALSE;
     }

  
  if (savetype == RENT_CRASH)
    { free(SAVEINFO(iplayer));
      SAVEINFO(iplayer) = NULL;
    }

  return TRUE;
}


// some dummy functions

void Crash_crashsave(struct char_data * ch)
{
  save_char_objects(ch, RENT_CRASH, 0, 0);
}

void Crash_ldsave(struct char_data * ch)
{
  save_char_objects(ch, RENT_CRASH, 0, 0);
}

void Crash_idlesave(struct char_data * ch)
{
  save_char_objects(ch, RENT_TIMEDOUT, 0, 0);
}

int Crash_rentsave(struct char_data * ch, int cost, int num)
{
  return save_char_objects(ch, RENT_RENTED, cost, num);
}

int Crash_cryosave(struct char_data * ch, int cost)
{
  return save_char_objects(ch, RENT_TIMEDOUT, cost, 0);
}

/* ************************************************************************
* Routines used for the receptionist					  *
************************************************************************* */
void Crash_rent_deadline(struct char_data * ch, struct char_data * recep, long cost, int Serw)
{
  long rent_deadline;

  if (!cost && !Serw)
     {send_to_char("Вы можете жить здесь сколько душе угодно!\r\n",ch);
      return;
     }

  rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) /(cost + (Serw == 0 ? 1 : Serw)));
  sprintf(buf, "$n сказал$y Вам: \"Ваших денег для проживания хватит на %ld %s.\"",
  rent_deadline, desc_count(rent_deadline, WHAT_DAY));
    
  act(buf, FALSE, recep, 0, ch, TO_VICT);
}

int Crash_report_unrentables(struct char_data * ch, struct char_data * recep,
			         struct obj_data * obj)
{
  char buf[128];
  int has_norents = 0;

  if (obj)
     {if (Crash_is_unrentable(obj))
         {has_norents = 1;
          sprintf(buf, "$n сказал$y Вам: \"Я не могу взять на хранение %s.\"", VOBJS(obj, ch));
          act(buf, FALSE, recep, 0, ch, TO_VICT);
         }
      has_norents += Crash_report_unrentables(ch, recep, obj->contains);
      has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
     }
  return (has_norents);
}



void Crash_report_rent(struct char_data * ch, struct char_data * recep,
                       struct obj_data * obj, int *cost, long *nitems, int display, int factor, int equip, int recursive)
{
  static char buf[256];
  char bf[80];

  if (obj)
     {if (!Crash_is_unrentable(obj))
         {(*nitems)++;
          *cost += MAX(0, ((equip ? GET_OBJ_RENTE(obj) : GET_OBJ_RENT(obj)) * factor));
          if (display)
             {if (*nitems == 1)
                 {if (!recursive)
                     act("$n сказал$y Вам: \"Если экипировку не снимать, постой будет дешевле!\"",
                         FALSE, recep, 0, ch, TO_VICT);
                  else 
                     act("$n сказал$y Вам: \"Это я в кладовку уберу:\"", FALSE, recep, 0, ch, TO_VICT);
                 }
              if (CAN_WEAR_ANY(obj))
                 {if (equip) 
                    sprintf(bf," (%d если снять)",GET_OBJ_RENT(obj) * factor);
                  else 
                    sprintf(bf," (%d если надеть)",GET_OBJ_RENTE(obj) * factor);
                 } 
              else
                 *bf='\0';
              sprintf(buf, "%s - %d %s%-18s за %s %s",
                      recursive ? "" : "&W",
                      (equip ? GET_OBJ_RENTE(obj) : GET_OBJ_RENT(obj)) * factor,
                      desc_count((equip ? GET_OBJ_RENTE(obj) : GET_OBJ_RENT(obj)) * factor, WHAT_MONEYa),
                      bf, VOBJS(obj, ch), recursive ? "" : "&n");
              act(buf, FALSE, recep, 0, ch, TO_VICT);
             }
         }
      if (recursive)
         {Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor, FALSE, TRUE);
          Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor, FALSE, TRUE);
         }
     }
}
 
int Crash_offer_rent(CHAR_DATA * ch, CHAR_DATA * receptionist,
		             int display, int factor, int *totalcost, int Serw)
{
  char buf[MAX_INPUT_LENGTH];
  int  i, divide = 1;
  long numitems = 0, norent;
  long numitems_weared = 0;
  
  *totalcost = 0;
  norent     = Crash_report_unrentables(ch, receptionist, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
      norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));

  if (norent)
     return (FALSE);

  for (i = 0; i < NUM_WEARS; i++)
      Crash_report_rent(ch, receptionist, GET_EQ(ch, i), totalcost, &numitems, display, factor, TRUE, FALSE);

  numitems_weared = numitems; numitems = 0; 

  Crash_report_rent(ch, receptionist, ch->carrying, totalcost, &numitems, display, factor, FALSE, TRUE);

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
	  Crash_report_rent(ch, receptionist, (GET_EQ(ch, i))->contains, totalcost, &numitems, display, factor, FALSE, TRUE);
     

  numitems += numitems_weared;

  if (!numitems && !GET_CHEST(ch).m_chest.size())
     {act("$n сказал$y Вам: \"Но у Вас ничего нет! Вы можете выйти набрав \'&cКОНЕЦ&n\'!\"",
	      FALSE, receptionist, 0, ch, TO_VICT);
      return (FALSE);
     }

  if (numitems > max_obj_save)
     {sprintf(buf, "$n сказал$y Вам: \"Извините, но я не могу хранить больше %d предметов.\"",
	          max_obj_save);
      act(buf, FALSE, receptionist, 0, ch, TO_VICT);
      return (FALSE);
     }

  divide    = GET_LEVEL(ch) <= 15 ? 2 : 1;

  if (display)
     { sprintf(buf, "$n сказал$y Вам: \"Всего постой Вам обойдется &Y%d %s&n%s \r\n"
	  "                       а за хранение на складе - &Y%d %s&n за полный день\"",
              *totalcost, desc_count(*totalcost, WHAT_MONEYu), (factor == RENT_FACTOR ? " в день," : ","),
			  Serw, desc_count(Serw, WHAT_MONEYu));
      act(buf, FALSE, receptionist, 0, ch, TO_VICT);
      if (MAX(0, *totalcost/divide + Serw) > GET_GOLD(ch) + GET_BANK_GOLD(ch))
         {act("\"Вы не заработали таких денег для своей ренты.\"",
	          FALSE, receptionist, 0, ch, TO_VICT);
          return (FALSE);
         };

      *totalcost = MAX(0, *totalcost/divide);
      if (divide > 1 && *totalcost > 0)
         act("\"Для Вас будет скидка 50%.\"",
             FALSE, receptionist, 0, ch, TO_VICT);
      if (factor == RENT_FACTOR)
         Crash_rent_deadline(ch, receptionist, *totalcost, Serw);
     }
  else
     *totalcost = MAX(0, *totalcost/divide);
  return (TRUE);
}


#define IS_QUESTOR(ch)     (PLR_FLAGGED(ch, PLR_QUESTOR))

int gen_receptionist(struct char_data * ch, struct char_data * recep,
		         int cmd, char *arg, int mode)
{
  room_rnum save_room;
  int cost, rentshow = TRUE, Serw = 0, Koeff =0;
 
  if (!ch->desc || IS_NPC(ch))
     return (FALSE);


  if (!CMD_IS("offer")  && !CMD_IS("rent")		&&
      !CMD_IS("постой") && !CMD_IS("стоимость") &&
      !CMD_IS("конец")	&& !CMD_IS("quit"))	
     return (FALSE);

  save_room = ch->in_room;

  if ((CMD_IS("конец") || CMD_IS("quit")))
     {if (save_room != r_helled_start_room &&
          save_room != r_named_start_room  &&
          save_room != r_unreg_start_room
         )
         GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
    
     if (IS_QUESTOR(ch)) 
	  { GET_NEXTQUEST(ch) = number(4, 10);
	    GET_COUNTDOWN(ch) = 0; 
	  }
   
   return (FALSE);
     }

  if (!AWAKE(recep))
     {sprintf(buf, "%s пока что не может говорить с Вами!\r\n", HSSH(recep));
      send_to_char(buf, ch);
      return (TRUE);
     }
  if (!CAN_SEE(recep, ch) && !IS_IMMORTAL(ch))
     {act("$n сказал$y: \"Вы бы для приличия хотя бы появились!\"", FALSE, recep, 0, 0, TO_ROOM);
      return (TRUE);
     }
 /* if (!IS_IMPL(ch) && GetClanzone(ch))
	{ act("$n сказал$y: \"Куда в чужие номера прешься? А ну Вали от сель!\"", FALSE, recep, 0, 0, TO_ROOM);
      return (TRUE);
	}*/
  if (RENTABLE(ch) && !IS_IMMORTAL(ch))
     {send_to_char("У Вас еще масса незаконченных дел, подождите немного.\r\n",ch);
      return (TRUE);
     }
  if (FIGHTING(ch))
      return (FALSE);
     
  if (free_rent)
     {act("$n сказал$y Вам: \"Сегодня праздник, рента свободная, наберите &Y\'конец\'.&n\"",
	     FALSE, recep, 0, ch, TO_VICT);
      return (TRUE);
     }
  
  if(GET_CHEST(ch).m_chest.size())
  {  if (!*arg)
	{ send_to_char("У Вас на складе находятся предметы на сохранении.\r\n",ch);
	  send_to_char("Укажите уровень хранения вещей по таймеру (от 1 до 5).\r\n",ch);
	  send_to_char("&CУРОВНИ ХРАНЕНИЯ:&n\r\n",ch);
	  send_to_char("       1 - Обычный уровень хранения        (цена 550   монет за сутки).\r\n",ch);
	  send_to_char("       2 - на &C200%&n износ вещей замедляется (цена 1200  монет за сутки).\r\n       3 - на &C300%&n износ вещей замедляется (цена 3500  монет за сутки).\r\n",ch);
	  send_to_char("       4 - на &C400%&n износ вещей замедляется (цена 10500 монет за сутки).\r\n       5 - на &C500%&n износ вещей замедляется (цена 20000 монет за сутки).\r\n",ch);
		return (TRUE);
	}
  
  if (atoi(arg) > 5 || atoi(arg) < 1)
  {	send_to_char("Диапазон значений должен быть от 1 до 5.\r\n",ch);
    return (TRUE);
  }
  Koeff = MAX(1, MIN(5, atoi(arg)));
	switch (Koeff)//что бы за пределы не выходило 1 - 5
	{ case 1:
	   Serw = 550;
	    break;
	  case 2:
	   Serw = 1200; 
	    break;
	  case 3:
	   Serw = 3500;
	    break;
	  case 4:
	   Serw = 10500;
	    break;
	  case 5:
	   Serw = 20000;
	    break;
	  default:
	   Serw = 550;
		  break;
	
	}
  }
  if (CMD_IS("rent") || CMD_IS("постой"))
     { if (!Crash_offer_rent(ch, recep, rentshow, mode, &cost, Serw))
         return (TRUE);

      if (!rentshow)
         {if (mode == RENT_FACTOR)
             sprintf(buf, "$n сказал$y Вам: \"Постой Вам обойдется %d %s.\"",
                     cost, desc_count(cost, WHAT_MONEYu));
          else
          if (mode == CRYO_FACTOR)
             sprintf(buf, "$n сказал$y Вам: \"Заморозка для консервации Вам обойдется %d %s\"",
                     cost, desc_count(cost, WHAT_MONEYu));
          act(buf, FALSE, recep, 0, ch, TO_VICT);

          if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch))
             {act("$n сказал$y Вам: \"..Вам это не по карману.\"",
	              FALSE, recep, 0, ch, TO_VICT);
              return (TRUE);
             }
          if (cost && (mode == RENT_FACTOR))
             Crash_rent_deadline(ch, recep, cost, Serw);
         }

      if (mode == RENT_FACTOR)
         {act("&C$n закрыл$y Ваши вещи в сейф и проводил$y Вас в комнату.&n",
	          FALSE, recep, 0, ch, TO_VICT);
          Crash_rentsave(ch, cost, Koeff);
          sprintf(buf, "%s сохраняется (%d %s в день, в наличии %ld всего монет.)",
                  GET_NAME(ch), cost + Serw, desc_count(cost + Serw, WHAT_MONEYu), GET_GOLD(ch) + GET_BANK_GOLD(ch));
         }
      else
         {// cryo 
          act("$n запер$q Ваши вещи в сундук и пове$q в тесную каморку.\r\n"
	          "Белый призрак появился в комнате, обдав Вас холодом...\r\n"
	          "Вы потеряли связь с окружающими Вас...",
	          FALSE, recep, 0, ch, TO_VICT);
          Crash_cryosave(ch, cost);
          sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
		  SET_BIT(PLR_FLAGS(ch, PLR_CRYO), PLR_CRYO);
         }

      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

      if (save_room == r_named_start_room)
         act("$n проводил$y $V мощным пинком на свободную лавку.", FALSE, recep, 0, ch, TO_ROOM);
      else
      if (save_room == r_helled_start_room || save_room == r_unreg_start_room)
         act("$n проводил$y $V мощным пинком на свободные нары.", FALSE, recep, 0, ch, TO_ROOM);
      else
         act("$n проводил$y $V в $S комнату.", FALSE, recep, 0, ch, TO_NOTVICT);
   
        if (IS_QUESTOR(ch))
	  { GET_NEXTQUEST(ch) = number(4, 10);
	    GET_COUNTDOWN(ch) = 0; 
	  }  

    extract_char(ch,FALSE);
      if (save_room != r_helled_start_room &&
          save_room != r_named_start_room  &&
	  save_room != r_unreg_start_room
         )
         GET_LOADROOM(ch) = GET_ROOM_VNUM(save_room);
      save_char(ch, save_room);
     }
  else
     {Crash_offer_rent(ch, recep, TRUE, mode, &cost, Serw);
      act("$N предложил$Y $d остановиться у н$S.", FALSE, ch, 0, recep, TO_ROOM);
     }
  return (TRUE);
}



SPECIAL(receptionist)
{
  return (gen_receptionist(ch, (CHAR_DATA *) me, cmd, argument, RENT_FACTOR));
}


SPECIAL(cryogenicist)
{
  return (gen_receptionist(ch, (CHAR_DATA *) me, cmd, argument, CRYO_FACTOR));
}

void Crash_frac_save_all(int frac_part)
{struct descriptor_data *d;

 for (d = descriptor_list; d; d = d->next)
     {if ((STATE(d) == CON_PLAYING)               &&
          !IS_NPC(d->character)                   &&
	  GET_ACTIVITY(d->character) == frac_part
	 )	
		{ Crash_crashsave(d->character);
          save_char(d->character, NOWHERE);
	      REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRASH), PLR_CRASH);
         }
	}  
}


void Crash_save_all(void)
{
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character)) {
      if (PLR_FLAGGED(d->character, PLR_CRASH)) {
	Crash_crashsave(d->character);
	save_char(d->character, NOWHERE);
	REMOVE_BIT(PLR_FLAGS(d->character, PLR_CRASH), PLR_CRASH);
      }
    }
  }
}


void Crash_frac_rent_time(int frac_part)
{int c;
 for (c = 0; c <= top_of_p_table; c++)
     if (player_table[c].activity == frac_part &&
         player_table[c].unique   != -1        &&
         SAVEINFO(c)
        )
        Crash_timer_obj(c, time(0));
}

void Crash_rent_time(int dectime)
{int c;
 for (c = 0; c <= top_of_p_table; c++)
     if (player_table[c].unique != -1)
        Crash_timer_obj(c, time(0));
}

