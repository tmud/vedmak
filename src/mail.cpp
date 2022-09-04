/* ************************************************************************
*   File: mail.c                                        Part of CircleMUD *
*  Usage: Internal funcs and player spec-procs of mud-mail system         *
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
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "support.h"
#include "mail.h"

#define MAIL_BUFFER_LEN 64

std::map<int, int> mail_data;

SPECIAL(postmaster);
void postmaster_send_mail(struct char_data * ch, struct char_data *mailman, int cmd, char *arg);
void postmaster_check_mail(struct char_data * ch, struct char_data *mailman, int cmd, char *arg);
void postmaster_receive_mail(struct char_data * ch, struct char_data *mailman, int cmd, char *arg);

extern long	get_id_by_name(char *name);
extern char *get_name_by_id(long id);
extern int get_buf_line(char **source, char *target);


void get_cur_date(char *buf)
{
    time_t mytime = time(0);
    tm* td = localtime(&mytime);
    sprintf(buf, "%d %s %d %d:%02d", 
       td->tm_mday, get_month(td), td->tm_year+1900, td->tm_hour, td->tm_min);
}

void del_mail_id(int id)
{
   std::map<int, int>::iterator it = mail_data.find(id);
   if (it != mail_data.end())
        mail_data.erase(it);
}

void send_mail(long to_id, CHAR_DATA* from, char *message)
{
    char buf[MAIL_BUFFER_LEN];
    char* name = get_name_by_id(to_id);
    if (!name)
    {
         sprintf(buf, "[SYSERR] get name by player id=%ld", to_id);
         log(buf);
         return;
    }
    char fname[MAX_TITLE_LENGTH];
    if (!get_filename(name, fname, PLAYER_MAIL))
    {
        sprintf(buf, "[SYSERR] Cant get filename by name '%s'", name);
        log(buf);
        return;
    }

    get_cur_date(buf);
    MemoryBuffer letter(MAX_MAIL_SIZE + MAX_TITLE_LENGTH + 128);
    char* p = letter.getData();

    std::string from_name("Системы");
    if (from)
        from_name.assign(GET_RNAME(from));
    sprintf(p, "%s\r\n%s\r\n%s<end>\r\n", from_name.c_str(), buf, message);

    int len = strlen(p);
    bool saved = false;
    FILE *fl;
    if (fl = fopen(fname, "a+b"))
    {
        if (fwrite(p, 1, len, fl) == len)
              saved = true;
        fclose(fl);
    }
    del_mail_id(to_id);

    if (!saved)
    {
        sprintf(buf, "[SYSERR] Failed save in mail file: %s", fname);
        mudlog(buf, BRF, LVL_IMMORT, TRUE);
    }    
    else
    {        
        CHAR_DATA* to_char = get_char_by_id(to_id);
        if (to_char)    // player online!
        {
            if (has_mail(to_char))
                send_to_char("&GВам пришла почта!&n\r\n", to_char);
        }
    }
}


bool read_mail_file(CHAR_DATA *ch, MemoryBuffer *buffer)
{ 
    buffer->alloc(0);

    char buf[MAIL_BUFFER_LEN];
    char fname[MAX_TITLE_LENGTH];
    if (!get_filename(GET_NAME(ch), fname, PLAYER_MAIL))
    {
        sprintf(buf, "[SYSERR] Cant get filename by name '%s'", GET_NAME(ch));
        log(buf);
        return false;
    }

    FILE *fl = fopen(fname, "rb");
    if (!fl)
        return true;

    fseek(fl, 0L, SEEK_END);
	int fsize = ftell(fl);
    fseek(fl, 0L, SEEK_SET);

    buffer->alloc(fsize+1);
    if (fread(buffer->getData(), 1, fsize, fl) != fsize || ferror(fl))
    {
        fclose(fl);
        sprintf(buf, "[SYSERR] Cant read mail file '%s'", fname);
        log(buf);
        return false;
    }
    fclose(fl);
    return true;
}

int has_mail(CHAR_DATA *ch)
{
    int id = GET_IDNUM(ch);
    std::map<int, int>::iterator it = mail_data.find(id);
    if (it != mail_data.end())
        return (it->second) ? it->second : 0;
    
    MemoryBuffer buffer;
    if (!read_mail_file(ch, &buffer))
    {
        // error in mail system
        send_to_char("Проблемы с почтой. Обратитесь к Богам.\r\n", ch);
        mail_data[id] = 0;
        return 0;
    }

    int fsize = buffer.getSize();
    if (fsize == 0)
    {
        // empty mail file
        mail_data[id] = 0;
        return 0;
    }

    MemoryBuffer text(MAX_INPUT_LENGTH);
    char *data = buffer.getData();
    char *end_data = data + (fsize - 1);
    *end_data = 0; // set end marker

    int count = 0;
    while (data != end_data)
    {
        get_buf_line(&data, text.getData());
        if (!strcmp(text.getData(), "<end>"))
            count ++;
    }

    if (!count)
    {
        char fname[MAX_TITLE_LENGTH];
        if (get_filename(GET_NAME(ch), fname, PLAYER_MAIL))
            remove(fname);
        mail_data[id] = 0;
        return 0;
    }

    mail_data[id] = count;
    return count;
}

SPECIAL(postmaster)
{
  if (!ch->desc || IS_NPC(ch))
    return (0);			/* so mobs don't get caught here */

  if (!(CMD_IS("отправить") || CMD_IS("проверить") || CMD_IS("получить")))
    return (0);

  if (CMD_IS("отправить")) {
    postmaster_send_mail(ch, (struct char_data *)me, cmd, argument);
    return (1);
  } else if (CMD_IS("проверить")) {
    postmaster_check_mail(ch, (struct char_data *)me, cmd, argument);
    return (1);
  } else if (CMD_IS("получить")) {
    postmaster_receive_mail(ch, (struct char_data *)me, cmd, argument);
    return (1);
  } else
    return (0);
}

void postmaster_send_mail(struct char_data * ch, struct char_data *mailman, int cmd, char *arg)
{
  long recipient;
  char buf[MAX_INPUT_LENGTH], **write;

  if (GET_LEVEL(ch) < MIN_MAIL_LEVEL && !GET_REMORT(ch)) 
  {
    sprintf(buf, "$n сказал$y Вам: \"Что бы посылать почту, Вы должны быть %d уровня!\"", MIN_MAIL_LEVEL);
    act(buf, FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  one_argument(arg, buf);

  if (!*buf) {			// you'll get no argument from me! 
    act("$n сказал$y Вам: \"Вы должны указать адресата!\"",
	FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  if (GET_GOLD(ch) < STAMP_PRICE)
  {
    sprintf(buf, "$n сказал$y Вам: \"Отправка стоит %d монет.\"\r\n"
	    "$n сказал$y Вам: \"Я вижу, Вы не можете это себе позволить.\"", STAMP_PRICE);
    act(buf, FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  if ((recipient = get_id_by_name(buf)) < 0)
  {
    act("$n сказал$y Вам: \"Персонаж с таким именем не найден.\"", FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  act("$n начал$y отправлять почту.", TRUE, ch, 0, 0, TO_ROOM);
  sprintf(buf, "$n сказал$y Вам:  \"Это будет стоить %d монет.\"\r\n"
       "Пишите письмо (/s сохранить и отправить, /h справка):\"",
	  STAMP_PRICE);

  act(buf, FALSE, mailman, 0, ch, TO_VICT);
  GET_GOLD(ch) -= STAMP_PRICE;
  SET_BIT(PLR_FLAGS(ch, PLR_MAILING), PLR_MAILING);

  // Start writing! 
  CREATE(write, char*, 1);
  string_write(ch->desc, write, MAX_MAIL_SIZE, recipient, NULL);
}

void postmaster_check_mail(struct char_data * ch, struct char_data *mailman,
			  int cmd, char *arg)
{
  char buf[MAX_INPUT_LENGTH];
  if (has_mail(ch))
    sprintf(buf, "$n сказал$y Вам: \"Для Вас есть почта.\"");
  else
    sprintf(buf, "$n сказал$y Вам: \"Извините, но для Вас нет писем.\"");
  act(buf, FALSE, mailman, 0, ch, TO_VICT);
}


void postmaster_receive_mail(struct char_data * ch, struct char_data *mailman,
			  int cmd, char *arg)
{  
  int count = has_mail(ch);
  if (!count) 
  {
    sprintf(buf, "$n сказал$y Вам: \"На Ваше имя почты не приходило!\"");
    act(buf, FALSE, mailman, 0, ch, TO_VICT);
    return;
  }

  std::vector<std::string> letters;
  {
      MemoryBuffer buffer;
      if (!read_mail_file(ch, &buffer))
      {
         // error in mail system
         send_to_char("Проблемы с почтой. Обратитесь к богам.\r\n", ch);
         return;
      }

      int fsize = buffer.getSize();
      char *data = buffer.getData();
      char *end_data = data + (fsize - 1);
      *end_data = 0; // set end marker

      MemoryBuffer tmp_buffer(MAX_INPUT_LENGTH);
      char* tmp = tmp_buffer.getData();
      
      std::string letter;
      int c = 0;
      while (data != end_data)
      {
          get_buf_line(&data, tmp);
          if (!strcmp(tmp, "<end>"))
          {
              c = 0;
              if (!letter.empty())
                letters.push_back(letter);
              letter.clear();
              continue;
          }
          if (c == 0)
          {
              std::string line(tmp);
              sprintf(tmp, "От: &K%s&n\r\nДля: &K%s&n\r\n", line.c_str(), GET_RNAME(ch));
              letter.append(tmp);
          }
          else if (c == 1)
          {
              std::string line(tmp);
              sprintf(tmp, "Дата: &C%s&n\r\n", line.c_str());
              letter.append(tmp);
          }      
          else 
          {
            letter.append(tmp);
            letter.append("\r\n");
          }
          c++;
      }  
  } // end read of letters
  
  char fname[MAX_TITLE_LENGTH];
  if (get_filename(GET_NAME(ch), fname, PLAYER_MAIL))
      remove(fname); 
  else
  {
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf, "[SYSERR] Cant delete mail file: %s", fname);
      log(buf);
  }
  del_mail_id(GET_IDNUM(ch));

  for (int i=letters.size()-1; i>=0; --i)
  {
    OBJ_DATA* obj = create_obj();
    obj->item_number = NOTHING;
    obj->name = str_dup("почта письмо mail paper letter");
    obj->short_description  = str_dup("письмо");
    obj->short_rdescription = str_dup("письма");
    obj->short_ddescription = str_dup("письму");
    obj->short_vdescription = str_dup("письмо");
    obj->short_tdescription = str_dup("письмом");
    obj->short_pdescription = str_dup("письме");
    obj->description = str_dup("Почтовое сообщение лежит здесь, кем-то брошенное.");

    GET_OBJ_EXTRA(obj, ITEM_DECAY)|= ITEM_DECAY;
    GET_OBJ_TYPE(obj)  = ITEM_NOTE;
    GET_OBJ_WEAR(obj)  = ITEM_WEAR_TAKE | ITEM_WEAR_HOLD;
    GET_OBJ_WEIGHT(obj)= 1;
    GET_OBJ_COST(obj)  = 30;
    GET_OBJ_RENT(obj)  = 10;
    GET_OBJ_TIMER(obj) = 24*60;

    obj->action_description = str_dup(letters[i].c_str());
    obj_to_char(obj, ch);
  }
  
  act("$n дал$y Вам почту.", FALSE, mailman, 0, ch, TO_VICT);
  act("$N дал$Y $d почту.", FALSE, ch, 0, mailman, TO_ROOM);
}
