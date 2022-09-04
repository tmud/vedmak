/***********************************************************************
*  File: corpse.c Version 1.1                                          *
*  Usage: Corpse Saving over reboots/crashes/copyovers                 *
*                                                                      *
*  By: Michael Cunningham (Romulus) IMP of Legends of The Phoenix MUD  *
*  Permission is granted to use and modify this software as long as    *
*  credits are given in the credits command in the game.               *
*  Built for circle30bpl15                                             *
*  Bunch of code reused from the XAP object patch by Patrick Dughi     *
*                                                  <dughi@IMAXX.NET>   *
*  Thankyou Patrick!                                                   *
*  Some functions have been renamed to protect the innocent            *
*  All Rights Reserved, Copyright (C) 1999                             *
***********************************************************************/

/* The standard includes */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

// установим путь сохранения трупов 
#define CORPSE_FILE "misc/corpse.save"

/* External Structures / Variables */
extern struct obj_data *object_list;
extern struct room_data *world;
extern struct index_data *obj_index;   /* index table for object file   */
extern room_vnum frozen_start_room;

/* External Functions */
void obj_from_room(struct obj_data * obj);
void obj_to_room(struct obj_data * object, room_rnum room);
void obj_to_obj(struct obj_data * obj, struct obj_data * obj_to);
void tascii(int *pointer, int num_planes, char *ascii); //перенести в линух 03.03.2007 экстроаффекты
void asciiflag_conv1(char *flag, void *value);

/* Local Function Declerations */
void save_corpses(void);
void load_corpses(void);
int corpse_save(struct obj_data * obj, FILE * fp, int location, bool recurse_this_tree);
int write_corpse_to_disk(FILE *fp, struct obj_data *obj, int locate);
void clean_string(char *buffer);

/* Tada! THE FUNCTIONS ! Yaaa! */

void clean_string(char *buffer)
{
  register char *ptr, *str;

   ptr = buffer;
   str = ptr;

   while ((*str = *ptr)) {
     str++;
     ptr++;
     if (*ptr == '\r')
       ptr++;
    }
}

int corpse_save(struct obj_data * obj, FILE * fp, int location, bool recurse_this_tree)
{
  /* This function basically is responsible for taking the    */
  /* supplied obj and figuring out if it has any contents. If */
  /* it does then we write those to disk.. Ad Nasum.          */

  int result;

  if (obj) { /* a little recursion (can be a dangerous thing:) */

    /* recurse_this_tree causes the recursion to branch only
       down the corpses content's tree and not the contents of the
       room. obj->next_content points to the rooms contents
       the first time this function is called from save_corpses
       hence we avoid going down there otherwise we will save
       the rooms contents as well as the corpses contents in the
       corpse.save file.
    */

    if (recurse_this_tree != FALSE)
    {
      corpse_save(obj->next_content, fp, location, recurse_this_tree);
    }
    recurse_this_tree = TRUE;
    corpse_save(obj->contains, fp, MIN(0, location) - 1, recurse_this_tree);
    result = write_corpse_to_disk(fp, obj, location);

    // корректируем вес
   // for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
   //   GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return (0);
  }
  return (TRUE);
}


int write_corpse_to_disk(FILE *fp, struct obj_data *obj, int locate)
{
  /* This is basically Patrick's my_obj_save_to_disk function with    */
  /* a few minor tweaks to make it work for corpses. Basically it     */
  /* writes one object out to the corpse file every time it is called.*/
  /* It can handle regular obj's and XAP objects.                     */

  int counter;
  struct extra_descr_data *ex_desc;
  char buf[MAX_INPUT_LENGTH +1];//перенести в линух 03.03.2007 и везде оставить только buf, а не буф1
  char f [MAX_INPUT_LENGTH +1];
  char f1[MAX_INPUT_LENGTH +1];

      if (obj->action_description) {
        strcpy(buf, obj->action_description);
        clean_string(buf);
      } else
        *buf = 0;

	  *f  = '\0';
	  *f1 = '\0';

	  	tascii(&GET_OBJ_EXTRA(obj, 0), 4, f);
        fprintf(fp,
             "#%d\n"
             "%d %d %d %d %d %s %d %d\n",
              GET_OBJ_VNUM(obj),
              locate,
              GET_OBJ_VAL(obj, 0),
              GET_OBJ_VAL(obj, 1),
              GET_OBJ_VAL(obj, 2),
              GET_OBJ_VAL(obj, 3),
			  f,
              GET_OBJ_VROOM(obj),    //Виртуальная комната для трупа
              GET_OBJ_TIMER(obj));   // был создан в. смотри make_corpse 


       fprintf(fp,
             "XAP\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%d %d %d %d %d %d %d\n",
              obj->name ? obj->name : "undefined",
              obj->short_rdescription ? obj->short_rdescription : "undefined",
              obj->short_ddescription ? obj->short_ddescription : "undefined",
              obj->short_vdescription ? obj->short_vdescription : "undefined",
              obj->short_tdescription ? obj->short_tdescription : "undefined",
              obj->short_pdescription ? obj->short_pdescription : "undefined",
              obj->short_description  ? obj->short_description  : "undefined",
              obj->description ? obj->description : "undefined",
              buf,
              GET_OBJ_TYPE(obj),
              GET_OBJ_WEAR(obj),
              GET_OBJ_WEIGHT(obj),
              GET_OBJ_COST(obj),
              GET_OBJ_RENT(obj),
			  GET_OBJ_CUR(obj),
              GET_OBJ_MAX(obj)
        );
      /* Do we have affects? */
      for (counter = 0; counter < MAX_OBJ_AFFECT; counter++)
        if (obj->affected[counter].modifier)
          fprintf(fp, "A\n"
                  "%d %d\n",
                  obj->affected[counter].location,
                  obj->affected[counter].modifier
            );

      // Имеется ли расширенное описание? 
      if (obj->ex_description) {        // Да, есть описание дополнительное 
        for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
          // Чек здравомыслия, чтобы предотвратить противные ошибки защиты 
          if (!ex_desc->keyword || !ex_desc->description) {
            continue;
          }
          strcpy(buf, ex_desc->description);
          clean_string(buf);
          fprintf(fp, "E\n"
                  "%s~\n"
                  "%s~\n",
                  ex_desc->keyword,
                  buf
            );
        }
      }
  return 1;
}

void save_corpses(void)
{
   /* This is basically the mother of all the save corpse functions */
   /* You can call it from anywhere in the game with no arguments */
   /* Basically any time a corpse is manipulated in any way..either */
   /* directly or indirectly you need to call this function */

   FILE *fp;
   struct obj_data *i, *next;
   int location = 0;

   // Открываем файл сохранения трупов
   if (!(fp = fopen(CORPSE_FILE, "wb"))){
     if (errno != ENOENT)   // Если получилась ошибка, то не из за файла
     log("SYSERR: checking for corpse file %s : %s", CORPSE_FILE, strerror(errno));
     return;
   }

   // Просмотрим список обьектов
   for (i = object_list; i; i = next)
	{ next = i->next;
     // Проверяем, труп ли это игрока? 
	  if (IS_OBJ_STAT(i, ITEM_PC_CORPSE))
        { // Это записать в файл для сохранения
         if (!corpse_save(i, fp, location, FALSE))
			{ log("SYSERR: A corpse didnt save for some reason");
              fclose(fp);
              return;
			}
		}
   }
   // закрываем файл сохранения трупов 
   fclose(fp);
}

void load_corpses(void)
{
  
  FILE *fp;
  char line[256], f[256];
  int t[10],danger,zwei=0;
  int locate=0, j, k, nr, num_objs=0;
  struct obj_data *temp = NULL, *obj = NULL, *next_obj = NULL;
  struct extra_descr_data *new_descr;

  if (!(fp = fopen(CORPSE_FILE, "r+b"))) {
    if (errno != ENOENT) {
      sprintf(buf1, "SYSERR: READING CORPSE FILE %s in load_corpses", CORPSE_FILE);
      perror(buf1);
    }
  }

  if(!feof(fp))
    get_line(fp, line);

  if (*line == '\0') return;

  while (!feof(fp)) {
        temp = NULL;
        // Сначала мы получаем номер
    if(*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        continue;
      }
      // Получили число, проверили, загружаем. 
      if (nr == NOTHING) {   // Это уже уникально
        temp = create_obj();
        temp->item_number=NOTHING;
      } else if (nr < 0) {
        continue;
      } else {
        if(nr >= 99999)
          continue;
        temp = read_object(nr,VIRTUAL);
        if (!temp) {
          continue;
        }
      }

	  *f = '\0';
      get_line(fp,line);
      sscanf(line,"%d %d %d %d %d %s %d %d",t, t + 1, t+2, t + 3 ,t + 4, f,t + 6, t + 7);
      locate=t[0];
      GET_OBJ_VAL(temp,0) = t[1];
      GET_OBJ_VAL(temp,1) = t[2];
      GET_OBJ_VAL(temp,2) = t[3];
      GET_OBJ_VAL(temp,3) = t[4];
	  asciiflag_conv1(f, &GET_OBJ_EXTRA(temp, 0));
   	  GET_OBJ_VROOM(temp) = t[6];
      GET_OBJ_TIMER(temp) = t[7];
     

	  get_line(fp,line);
       /* read line check for xap. */
      if(!strcmp("XAP",line)) {  /* then this is a Xap Obj, requires
                                       special care */
        if ((temp->name = fread_string(fp, buf2)) == NULL) {
          temp->name = "undefined";
        }
        if ((temp->short_rdescription = fread_string(fp, buf2)) == NULL) {
          temp->short_rdescription = "undefined";
        }
        if ((temp->short_ddescription = fread_string(fp, buf2)) == NULL) {
          temp->short_ddescription = "undefined";
        }
        if ((temp->short_vdescription = fread_string(fp, buf2)) == NULL) {
          temp->short_vdescription = "undefined";
        }
        if ((temp->short_tdescription = fread_string(fp, buf2)) == NULL) {
          temp->short_tdescription = "undefined";
        }
        if ((temp->short_pdescription = fread_string(fp, buf2)) == NULL) {
          temp->short_pdescription = "undefined";
        }

        if ((temp->short_description = fread_string(fp, buf2)) == NULL) {
          temp->short_description = "undefined";
        }

        if ((temp->description = fread_string(fp, buf2)) == NULL) {
          temp->description = "undefined";
        }

        if ((temp->action_description = fread_string(fp, buf2)) == NULL) {
          temp->action_description=0;
        }
        if (!get_line(fp, line) ||
           (sscanf(line, "%d %d %d %d %d %d %d", t,t+1,t+2,t+3,t+4, t+5, t+6) != 7)) {
           log("Format error in first numeric line (expecting 6 args)");
        }
        temp->obj_flags.type_flag		= t[0];
        temp->obj_flags.wear_flags		= t[1];
        temp->obj_flags.weight			= t[2];
        temp->obj_flags.cost			= t[3];
        temp->obj_flags.cost_per_day	= t[4];
		temp->obj_flags.ostalos			= t[5];
        GET_OBJ_MAX(temp)				= t[6];
        obj_index[real_object(nr)].stored ++;

        // очищаем аффекты, что бы не было накладок
        for (j = 0; j < MAX_OBJ_AFFECT; j++) {
          temp->affected[j].location = APPLY_NONE;
          temp->affected[j].modifier = 0;
        }

        // Мы имеем пустое расширинное описание, когда парсим обьект.
        // Это сделано прежде чем прочитано это расширенное описание. 

        if (temp->ex_description) {
           temp->ex_description = NULL;
           }

        get_line(fp,line);
        for (k=j=zwei=0;!zwei && !feof(fp);) {
          switch (*line) {
            case 'E':
              CREATE(new_descr, struct extra_descr_data, 1);
              new_descr->keyword = fread_string(fp, buf2);
              new_descr->description = fread_string(fp, buf2);
              new_descr->next = temp->ex_description;
              temp->ex_description = new_descr;
              get_line(fp,line);
              break;
            case 'A':
              if (j >= MAX_OBJ_AFFECT) {
                log("SYSERR: Превышение допустимого количества аффектов на предмете");
                danger=1;
              }
              get_line(fp, line);
              sscanf(line, "%d %d", t, t + 1);

              temp->affected[j].location = t[0];
              temp->affected[j].modifier = t[1];
              j++;
              get_line(fp,line);
              break;

            case '$':
            case '#':
              zwei=1;
              break;
            default:
              zwei=1;
              break;
          }
        }      // выход из цикла 
      }   /* выход из цикла хар */
      if(temp != NULL)
	  { num_objs++;
      // Проверяем, является объект трупом. 
        if (IS_OBJ_STAT(temp, ITEM_PC_CORPSE))
       {    /* scan our temp room for objects */
            for (obj = world[real_room(frozen_start_room)].contents; obj ; obj = next_obj)
            {
            next_obj = obj->next_content;
            if (obj)
             {
              obj_from_room(obj);     // получаем обьекты из этой комнаты
              obj_to_obj(obj, temp);  // и помещаем их в труп
             }
            } // выходим из цикла просматривая список
           if (temp)
            { // поместим труп в нужную комнату
             obj_to_room(temp, real_room(GET_OBJ_VROOM(temp)));
            }
       } else {
        //временно помещаем труп в данную комнату, пока не загрузим труп комната с номером 1202.
        obj_to_room(temp, real_room(frozen_start_room));
       }
      }
    }
  else
	break;
 }
  fclose(fp);
}

