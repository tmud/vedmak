/**************************************************************************
*  File: exchange.cpp                                      *
*  Usage: Exchange functions used by the MUD                              *
*  From Bylins. modified for Gold Dragon                    *
************************************************************************ */

#include <stdexcept>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "exchange.h"
#include "constants.h"
#include "spells.h"
#include "support.h"
#include "mail.h"

ACMD(do_exchange);
#define EXCHANGE_ITEM(lot) (&exchange_item_list[lot])
#define EXCHANGE_ITEM_SELLERID(lot)  (exchange_item_list[lot].seller_id)
#define EXCHANGE_ITEM_COST(lot)  (exchange_item_list[lot].obj_cost)
#define EXCHANGE_ITEM_COMMENT(lot)  (exchange_item_list[lot].comment)
#define EXCHANGE_ITEM_OBJ(lot)  (exchange_item_list[lot].obj)
#define EXCHANGE_FILTER(ch) (ch->exchange_filter)
//------------------------------------------------------------------------------------
int exchange_exhibit(CHAR_DATA * ch, char *arg);
int exchange_change_cost(CHAR_DATA * ch, char *arg);
int exchange_withdraw(CHAR_DATA * ch, char *arg);
int exchange_information(CHAR_DATA * ch, char *arg);
int exchange_identify(CHAR_DATA * ch, char *arg);
int exchange_purchase(CHAR_DATA * ch, char *arg);
int exchange_offers(CHAR_DATA * ch, char *arg);
int exchange_setfilter(CHAR_DATA * ch, char *arg);
int get_free_lot(void);
void lot_to_char(int lot, CHAR_DATA *ch);
void delete_lot(int lot);

void message_exchange(char *message, CHAR_DATA * ch, int lot, CHAR_DATA * ch2 = NULL);
int obj_matches_filter(int lot, char *filter_name, char *filter_owner, int *filter_type,
		   int *filter_cost, int *filter_timer, int *filter_wereon, int *filter_weaponclass);
int parse_exch_filter(char *buf, char *filter_name, char *filter_owner, int *filter_type,
					  int *filter_cost, int *filter_timer, int *filter_wereon, int *filter_weaponclass);
void show_lots(char *filter, short int show_type, CHAR_DATA * ch);

//Используемые внешние ф-ии.
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *obj_proto;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern void	page_string(struct descriptor_data *d, char *str, int keep_internal);
extern struct index_data *obj_index;
extern char *diag_weapon_to_char(struct obj_data *obj, int show_wear);
extern char *diag_timer_to_char(struct obj_data *obj);
extern int invalid_align(struct char_data *ch, struct obj_data *obj);
extern int invalid_unique(struct char_data *ch, struct obj_data *obj);
extern int invalid_anti_class(struct char_data *ch, struct obj_data *obj);
extern int invalid_no_class(struct char_data *ch, struct obj_data *obj);
extern void mort_show_obj_values(struct obj_data *obj, struct char_data *ch, int lvl);
extern void imm_show_obj_values(OBJ_DATA * obj, CHAR_DATA * ch);
extern void write_one_object(char **data, OBJ_DATA * object, int location);
extern OBJ_DATA *read_one_object_new(char **data, int *error);
extern int get_filename(char *orig_name, char *filename, int mode);
extern int get_buf_line(char **source, char *target);
extern char *diag_obj_to_char(struct char_data * i, struct obj_data * obj, int mode);

extern int top_of_p_table;
extern struct player_index_element *player_table;
//------------------------------------------------------------------------------------
std::vector<EXCHANGE_ITEM_DATA> exchange_item_list;
std::vector<int> free_slots;
std::map<int, int> exchange_bank;
char osd_buffer[MAX_INPUT_LENGTH];

char* descr(OBJ_DATA *obj, int cap = 0)
{
    const char *name = obj->short_description;    
    std::string tmp;
    if (cap) 
    {
        strcpy(osd_buffer, obj->short_description);
        tmp.assign(CAP(osd_buffer));
        name = tmp.c_str();
    }

    if (GET_OBJ_RLVL(obj))
        sprintf(osd_buffer, "&W%s&K", name);
    else
        sprintf(osd_buffer, "%s", name);
    return osd_buffer;
}

void save_exchange_timers()
{
    TempBuffer tb;
    char buffer[32];
    for (int i=0,e=exchange_item_list.size(); i<e; ++i)
    {
        if (EXCHANGE_ITEM_SELLERID(i) == -1)
            continue;
        OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(i);
        sprintf(buffer, "%d,%d\r\n", i, GET_OBJ_TIMER(obj));
        tb.append(buffer);
    }

    bool saved = false;
    if (FILE *fl =fopen(EXCHANGE_TIMERS_FILE, "wb"))
    {
        int len = tb.getSize();
        if (fwrite(tb, 1, len, fl) == len)
            saved = true;
        fclose(fl);
    }

    if (!saved)
    {
         MemoryBuffer buf(MAX_STRING_LENGTH);
         sprintf(buf.getData(), "[SYSERR] Save TIMERS !!! exchange file '%s' failed!\r\n", EXCHANGE_TIMERS_FILE);
         log(buf.getData());
    }
}

void load_exchange_timers()
{
   MemoryBuffer buf(MAX_STRING_LENGTH);
   int fsize = 0;
   if (FILE *fl = fopen(EXCHANGE_TIMERS_FILE, "rb"))
   {
       fseek(fl, 0L, SEEK_END);
       fsize = ftell(fl);
       fseek(fl, 0L, SEEK_SET);
       if (fsize > (buf.getSize()-1))
            buf.alloc(fsize+1);
       if (fsize > 0)
       {
           if (fread(buf.getData(), 1, fsize, fl) != fsize || ferror(fl))
                 fsize = -1;
       }
       fclose(fl);
   }
   if (fsize == -1)
   {
       sprintf(buf.getData(), "[SYSERR] Load TIMERS !!! exchange file '%s' failed!\r\n", EXCHANGE_TIMERS_FILE);
       log(buf.getData());
       return;
   }    
   if (fsize > 0)
   {
      MemoryBuffer line(MAX_INPUT_LENGTH);  
      char *data = buf.getData();
      char *end_data = data + fsize;
      *end_data = 0;
     
      while (data != end_data)
      {
         get_buf_line(&data, line.getData());
         int lot = -1; int timer = -1;
         sscanf(line.getData(), "%d,%d", &lot, &timer);
         if (lot >= 0 && timer > 0)
         {
             if (EXCHANGE_ITEM_SELLERID(lot) == -1)
                continue;
             OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);
             GET_OBJ_TIMER(obj) = timer;
         }
      }
   }

}

void exchange_tick_timer_objects()
{
    char buf[128];
    for (int i=0,e=exchange_item_list.size(); i<e; ++i)
    {
        if (EXCHANGE_ITEM_SELLERID(i) == -1)
            continue;
        OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(i);
        int timer = --GET_OBJ_TIMER(obj);
        if (timer == 0)
        {
            CHAR_DATA* seller = get_char_by_id(EXCHANGE_ITEM_SELLERID(i));
	        if (seller == NULL) // player offline
            {  // send letter
               sprintf(buf, "%s рассыпал%s от старости.", obj->short_description, GET_OBJ_SUF_2(obj));
               std::string tmp(CAP(buf));

               sprintf(buf,
               "Базар : Лот %d был снят с продажи. %s\r\n", i, tmp.c_str());
               send_mail(EXCHANGE_ITEM_SELLERID(i), NULL, buf);
            }
            else // player online
            {
               sprintf(buf,
               "&GБазар &K: Лот %d был снят с продажи. %s рассыпал%s от старости.&n\r\n", i, descr(obj, 1), GET_OBJ_SUF_2(obj));
               send_to_char(buf, seller);
            }
            sprintf(buf, "лот %d (%s) снят с продажи.", i, descr(obj, 0));
	        message_exchange(buf, seller, i);
            delete_lot(i);
        }
    }
    save_exchange_timers();
}

bool exist_lot(int lot) 
{
   int last_lot = exchange_item_list.size() - 1;
   if (lot >= 0 && lot <= last_lot) 
   {
       return (EXCHANGE_ITEM_SELLERID(lot) < 0) ? false : true;
   }
   return false;
}

bool is_imm(CHAR_DATA *ch)
{
  int level = GET_LEVEL(ch);
  if (level >= LVL_IMMORT && level < LVL_IMPL)
  {
  	  send_to_char("Вам совсем не нужно интересоваться экономикой смертных.\r\n", ch);
	  return true;
  }
  return false;
}

bool is_impl(CHAR_DATA *ch)
{
    return (GET_LEVEL(ch) >= LVL_IMPL) ? true : false;
}

void save_exchange(int id, char* name)
{   
    char sbuf[2048];
    TempBuffer tb;

    for (int i=0,e=exchange_item_list.size(); i<e; ++i)
    {
        if (exchange_item_list[i].seller_id == id)
        {
            EXCHANGE_ITEM_DATA &it = exchange_item_list[i];
            sprintf(sbuf, "%d,%d,%s\n", i, it.obj_cost, (it.comment.empty() ? "empty" : it.comment.c_str()) );
            tb.append(sbuf);
            char *p = &sbuf[0];
            write_one_object(&p, it.obj, 0);
            tb.append(sbuf);
            tb.append("$\n");
        }
    }

    bool saved = false;
    char fname[MAX_TITLE_LENGTH];
    if (get_filename(name, fname, PLAYER_EXCHANGE))
    {
        if (tb.getSize() == 0)
        {
            remove(fname);
            saved = true;
        }
        else
        {
            FILE *fl;
            if (fl = fopen(fname,"w"))
            {          
                int len = tb.getSize();
                if (fwrite(tb, 1, len, fl) == len) 
                        saved = true;
                fclose(fl);
            }
        }
        
        if (!saved)
        {
            sprintf(sbuf, "[SYSERR] Failed save in exchange file: %s", fname);
            mudlog(sbuf, BRF, LVL_IMMORT, TRUE);
        }
    }
    save_exchange_timers();
}

void save_exchange(CHAR_DATA *ch)
{
    save_exchange(GET_IDNUM(ch), GET_NAME(ch));
}

void save_objects(CHAR_DATA *ch)
{
    // save objects in whouse
    Crash_crashsave(ch);
}

void load_bank()
{
   MemoryBuffer buf(MAX_STRING_LENGTH);
   int fsize = 0;
   if (FILE *fl = fopen(EXCHANGE_BANK_FILE, "rb"))
   {
       fseek(fl, 0L, SEEK_END);
       fsize = ftell(fl);
       fseek(fl, 0L, SEEK_SET);
       if (fsize > (buf.getSize()-1))
            buf.alloc(fsize+1);
       if (fsize > 0)
       {
           if (fread(buf.getData(), 1, fsize, fl) != fsize || ferror(fl))
                 fsize = -1;
       }
       fclose(fl);
   }
   if (fsize == -1)
   {
       sprintf(buf.getData(), "[SYSERR] Load BANK !!! exchange file '%s' failed!\r\n", EXCHANGE_BANK_FILE);
       log(buf.getData());
       return;
   }    
   if (fsize > 0)
   {
      MemoryBuffer line(MAX_INPUT_LENGTH);  
      char *data = buf.getData();
      char *end_data = data + fsize;
      *end_data = 0;
     
      while (data != end_data)
      {      
         get_buf_line(&data, line.getData());
         int id = -1; int money = -1;
         sscanf(line.getData(), "%d,%d", &id, &money);
         if (id >= 0 && money > 0)
             exchange_bank[id] = money;
      }
   }
}

void save_bank()
{
    TempBuffer bank;
    char buffer[32];
    std::map<int, int>::iterator it = exchange_bank.begin(), it_end = exchange_bank.end();
    for(; it!=it_end; ++it)
    {
        if (it->second == 0)
            continue;
        sprintf(buffer, "%d,%d\r\n", it->first, it->second);
        bank.append(buffer);
    }

    bool saved = false;
    if (FILE *fl =fopen(EXCHANGE_BANK_FILE, "wb"))
    {
        int len = bank.getSize();
        if (fwrite(bank, 1, len, fl) == len)
            saved = true;
        fclose(fl);
    }

    if (!saved)
    {
         MemoryBuffer buf(MAX_STRING_LENGTH);
         sprintf(buf.getData(), "[SYSERR] Save BANK !!! exchange file '%s' failed!\r\n", EXCHANGE_BANK_FILE);
         log(buf.getData());
    }
}

void save_bank(CHAR_DATA *ch, int money)
{
    // save banks money
    int id = GET_IDNUM(ch);
    std::map<int, int>::iterator st = exchange_bank.find(id);
    if (st != exchange_bank.end())
        st->second += money;
    else
        exchange_bank[id] = money;
    save_bank();
}

void update_exchange_bank(CHAR_DATA *ch)
{
    int id = GET_IDNUM(ch);
    std::map<int, int>::iterator st = exchange_bank.find(id);
    if (st != exchange_bank.end())
    {
        GET_BANK_GOLD(ch) += st->second;
        exchange_bank.erase(st);
        save_bank();
    }    
}

typedef std::map<int, EXCHANGE_ITEM_DATA> EXCHANGE_LOAD_DATA;
bool check_exchange(int id, EXCHANGE_LOAD_DATA& v)
{
    EXCHANGE_LOAD_DATA::iterator it = v.find(id);
    return (it == v.end()) ? false : true;
}

bool load_one_exchange_file(char* data, int len, int id, EXCHANGE_LOAD_DATA& v)
{
   MemoryBuffer buffer(MAX_INPUT_LENGTH);
   MemoryBuffer comment(MAX_INPUT_LENGTH);  
   char *end_data = data + len;
   *end_data = 0;

   while (data != end_data)
   {
       if (*data == EX_END_CHAR) 
           data++;
       get_buf_line(&data, buffer.getData());
       if (data == end_data)
           break;

       int slot = -1; int cost = -1;
       sscanf(buffer.getData(), "%d,%d,%[^\n\r]", &slot, &cost, comment.getData());
       if (slot < 0 || cost < 0 || !strlen(comment.getData()))
           return false;

       int error = 0;
       OBJ_DATA *obj = read_one_object_new(&data, &error);
       if (!obj || error)
           return false;

       EXCHANGE_ITEM_DATA item;
       item.obj = obj;
       item.obj_cost = cost;
       item.seller_id = id;
       std::string c(comment.getData());
       if (!c.empty() && c != "empty")
           item.comment = c;

       while (check_exchange(slot, v))
           slot++;
       v[slot] = item;
   }
   return true;
}

void load_exchange_db()
{
    load_bank();

    EXCHANGE_LOAD_DATA ex_data;
    MemoryBuffer buffer(MAX_STRING_LENGTH);
    char fname[MAX_TITLE_LENGTH];
    for (int i=0; i<=top_of_p_table; ++i)
    {
        const player_index_element &p = player_table[i];
        if (get_filename(p.name, fname, PLAYER_EXCHANGE))
        {
            int fsize = 0;
            if (FILE *fl = fopen(fname,"rb"))
            {
                fseek(fl, 0L, SEEK_END);
	            fsize = ftell(fl);
            	fseek(fl, 0L, SEEK_SET);
                if (fsize > (buffer.getSize()-1))
                    buffer.alloc(fsize+1);
                if (fsize > 0)               
                {                                        
                    if (fread(buffer.getData(), 1, fsize, fl) != fsize || ferror(fl))
                       fsize = -1;
                }
                fclose(fl);
            }

            bool parse_error = true;
            if (fsize > 0)
                parse_error = !load_one_exchange_file(buffer.getData(), fsize, p.id, ex_data);
            else if (fsize == 0) { parse_error = false; }
            
            if (parse_error)
            {
                sprintf(buf, "Error! Loading exchange file '%s' failed!\r\n", fname);
			    log(buf);
            }
        }
    }

    if (ex_data.empty())
        return;

    EXCHANGE_LOAD_DATA::iterator it = ex_data.begin(), it_end = ex_data.end();
    int max_slot = 0;
    for (;it != it_end; ++it)
    {
        int slot = it->first;
        if (slot > max_slot)
            max_slot = slot;
    }
    exchange_item_list.resize(max_slot+1);

    it = ex_data.begin(), it_end = ex_data.end();
    for (;it != it_end; ++it)
    {
        int slot = it->first;
        exchange_item_list[slot] = it->second;        
    }

    for (int i=0; i<=max_slot; ++i)
    {
        if (exchange_item_list[i].seller_id == -1)
            free_slots.push_back(i);
    }

    load_exchange_timers();
}

char arg1[MAX_INPUT_LENGTH];
char arg2[MAX_INPUT_LENGTH];
char arg3[MAX_INPUT_LENGTH];
char arg4[MAX_INPUT_LENGTH];
char info_message[] = ("Команды:\r\n"
                       "базар выставить <предмет> <цена>        - выставить предмет со склада на продажу\r\n"
					   "базар цена <#лот> <цена>                - изменить цену на свой лот\r\n"
					   "базар снять <#лот>                      - снять с продажи свой лот на склад\r\n"
					   "базар информация <#лот>                 - информация о лоте\r\n"
					   "базар характеристики <#лот>             - характеристики лота (цена услуги 100 монет)\r\n"
                       "базар купить <#лот>                     - купить лот, покупка поступит на склад\r\n"
					   "базар предложения все <предмет>         - все предложения\r\n"
					   "базар предложения мои <предмет>         - мои предложения\r\n"
					   //"базар предложения руны <предмет>        - предложения рун\r\n"
					   "базар предложения броня <предмет>       - предложения одежды и брони\r\n"
					   "базар предложения оружие <предмет>      - предложения оружия\r\n"
					   "базар предложения книги <предмет>       - предложения книг\r\n"
					   //"базар предложения ингредиенты <предмет> - предложения ингредиентов\r\n"
					   "базар предложения прочие <предмет>      - прочие предложения\r\n"
                       "базар предложения уникальные <предмет>  - уникальные предметы\r\n"
					   "базар фильтрация <фильтр>               - фильтрация товара на базаре\r\n");


SPECIAL(exchange)
{
    if (IS_NPC(ch))
			return 0;

	if (CMD_IS("exchange") || CMD_IS("базар"))
	{
		if (AFF_FLAGGED(ch, AFF_SIELENCE) | PLR_FLAGGED(ch, PLR_DUMB) | PLR_FLAGGED(ch, PLR_MUTE))
		{
			send_to_char("Вы немы, как рыба об лед.\r\n", ch);
			return 1;
		}
        if (RENTABLE(ch))
		{
			send_to_char("Завершите сначала боевые действия.\r\n", ch);
			return 1;
		}
		
		if (GET_LEVEL(ch) < EXCHANGE_MIN_CHAR_LEV && !GET_REMORT(ch))
		{
			sprintf(buf, "Вам стоит достичь хотя бы %d уровня, чтобы пользоваться базаром.\r\n", EXCHANGE_MIN_CHAR_LEV);
			send_to_char(buf, ch);
			return 1;
		}		

		argument = one_argument(argument, arg1);
		if (is_abbrev(arg1, "выставить") || is_abbrev(arg1, "exhibit"))
			exchange_exhibit(ch, argument);
		else if (is_abbrev(arg1, "цена") || is_abbrev(arg1, "cost"))
			exchange_change_cost(ch, argument);
		else if (is_abbrev(arg1, "снять") || is_abbrev(arg1, "withdraw"))
			exchange_withdraw(ch, argument);
		else if (is_abbrev(arg1, "информация") || is_abbrev(arg1, "information"))
			exchange_information(ch, argument);
		else if (is_abbrev(arg1, "характеристики") || is_abbrev(arg1, "identify"))
			exchange_identify(ch, argument);
		else if (is_abbrev(arg1, "купить") || is_abbrev(arg1, "purchase"))
			exchange_purchase(ch, argument);
		else if (is_abbrev(arg1, "предложения") || is_abbrev(arg1, "offers"))
			exchange_offers(ch, argument);
		else if (is_abbrev(arg1, "фильтрация") || is_abbrev(arg1, "filter"))
			exchange_setfilter(ch, argument);
		else
			send_to_char(info_message, ch);
		return 1;
	}
    return 0;
}

int exchange_exhibit(CHAR_DATA * ch, char *arg)
{
    if (is_imm(ch))
        return false;

    if (!*arg)
    {
        send_to_char("Формат: базар выставить предмет цена комментарий\r\n", ch);
	    return false;
    }
	
    char obj_name[MAX_INPUT_LENGTH];
	char tmpbuf[MAX_INPUT_LENGTH];

	arg = one_argument(arg, obj_name);
	if (!*obj_name)
	{
		send_to_char("Не указан предмет для продажи.\r\n", ch);
		return false;
	}
      
    WareHouse &w = GET_CHEST(ch);
    int index = w.FindObj(obj_name);
	if (index == -1)
	{
		send_to_char("У вас нет на складе этого предмета.\r\n", ch);
		return false;
	}

    OBJ_DATA *obj = w[index];
    int item_cost = 0;
    arg = one_argument(arg, arg2);
	if (!sscanf(arg2, "%d", &item_cost))
		item_cost = 0;

	//if (GET_OBJ_TYPE(obj) != ITEM_BOOK)
	{
		if (OBJ_FLAGGED(obj, ITEM_NORENT)
			|| OBJ_FLAGGED(obj, ITEM_NOSELL)
			|| OBJ_FLAGGED(obj, ITEM_ZONEDECAY)
            || OBJ_FLAGGED(obj, ITEM_DECAY)
            || OBJ_FLAGGED(obj, ITEM_NODROP)
			|| GET_OBJ_RNUM(obj) < 0
            || obj->obj_flags.cost <= 0
            || obj->obj_flags.Obj_owner > 0)
		{
			send_to_char("Этот предмет нельзя продать на базаре.\r\n", ch);
			return false;
		}
	}

	if (obj->contains)
	{
		sprintf(buf, "Уберите все из %s перед продажей.\r\n", obj->short_rdescription);
		send_to_char(buf, ch);
		return false;
	}

	if (item_cost <= 0)
		item_cost = MAX(1, GET_OBJ_COST(obj));

    int tax = 0;	      //налог
    if (GET_OBJ_TYPE(obj) != ITEM_MING)
    {
        tax = (int)(item_cost * EXCHANGE_EXHIBIT_PAY_COEFF);
    }
    else
    {
        tax = (int)(item_cost * EXCHANGE_EXHIBIT_PAY_COEFF); // /2
    }
    
    if (is_impl(ch))
        tax = 0;

	if (GET_BANK_GOLD(ch) < tax)
	{
		send_to_char("На счету в Банке недостаточно средств для оплаты комиссии!\r\n", ch);
		return false;
	}

    // check count of players' lot 
    {
        int counter = 0; int counter_ming = 0;
        for (int i=0,e=exchange_item_list.size(); i<e; ++i)
        {
            if (EXCHANGE_ITEM_SELLERID(i) != GET_IDNUM(ch))
                continue;
            int type = GET_OBJ_TYPE(EXCHANGE_ITEM_OBJ(i));
            if (type == ITEM_MING && type == ITEM_INGRADIENT)
                counter_ming++;
            else
                counter++;
        }
      	if (counter + counter_ming  >= EXCHANGE_MAX_EXHIBIT_PER_CHAR)
	    {
		    send_to_char("Вы уже выставили на базар максимальное количество предметов!\r\n", ch);
		    return false;
	    }
    }

    int lot = get_free_lot();
	if (lot < 0)
	{
		send_to_char("Базар переполнен!\r\n", ch);
		return false;
	}

	EXCHANGE_ITEM_SELLERID(lot) = GET_IDNUM(ch);
	EXCHANGE_ITEM_COST(lot) = item_cost;
	skip_spaces(&arg);
	if (*arg)
		EXCHANGE_ITEM_COMMENT(lot) = arg;
	else
		EXCHANGE_ITEM_COMMENT(lot) = "";

    EXCHANGE_ITEM_OBJ(lot) = obj;
    w.RemoveObj(index);

	sprintf(tmpbuf, "Вы выставили на базар $O (лот %d) за %d %s.\r\n",
			lot, item_cost, desc_count(item_cost, WHAT_MONEYu));
	act(tmpbuf, FALSE, ch, 0, obj, TO_CHAR);
	
    if (tax > 0) {
	    GET_BANK_GOLD(ch) -= tax;
        sprintf(tmpbuf, "Комиссия %d %s.\r\n",
			tax, desc_count(tax, WHAT_MONEYu));
        send_to_char(tmpbuf, ch);
    }

    sprintf(tmpbuf,
			"новый лот %d (%s) цена %d %s.",
            lot, descr(obj), item_cost, desc_count(item_cost, WHAT_MONEYa));
    message_exchange(tmpbuf, ch, lot);

    save_exchange(ch);
    save_objects(ch);
	return (true);
}

int exchange_change_cost(CHAR_DATA * ch, char *arg)
{
    int lot = 0, newcost = 0;
	if (!*arg || sscanf(arg, "%d %d", &lot, &newcost) != 2)
	{
		send_to_char("Формат команды: базар цена <лот> <новая цена>\r\n", ch);
		return false;
	}    
	if (!exist_lot(lot))
	{
		send_to_char("Лота с таким номером не существует.\r\n", ch);
		return false;
	}
	if (EXCHANGE_ITEM_SELLERID(lot) != GET_IDNUM(ch))
	{
		send_to_char("Это не ваш лот.\r\n", ch);
		return false;
	}
    if (newcost == EXCHANGE_ITEM_COST(lot))
	{
		send_to_char("Ваша новая цена совпадает с текущей.\r\n", ch);
		return false;
	}
    if (newcost <= 0)
	{
		send_to_char("Вы указали неправильную цену.\r\n", ch);
		return false;
	}
    
    int pay = newcost - EXCHANGE_ITEM_COST(lot);
    int tax = (pay > 0) ? (pay * EXCHANGE_EXHIBIT_PAY_COEFF) : 0;
    
	if (GET_BANK_GOLD(ch) < tax)
	{
	    send_to_char("На счету в Банке недостаточно средств для оплаты комиссии!\r\n", ch);
		return false;
	}	   

    EXCHANGE_ITEM_COST(lot) = newcost;
    OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);

    char tmpbuf[MAX_INPUT_LENGTH];
    const char* name = descr(obj);
	sprintf(tmpbuf, "Вы назначили цену %d %s за %s (лот %d).\r\n",
			newcost, desc_count(newcost, WHAT_MONEYu), name, lot);
	send_to_char(tmpbuf, ch);
    
    if (tax > 0) {
        GET_BANK_GOLD(ch) -= tax;		
        sprintf(tmpbuf, "Комиссия %d %s.\r\n",
		tax, desc_count(tax, WHAT_MONEYu));
        send_to_char(tmpbuf, ch);
	}

	sprintf(tmpbuf,
			"лот %d (%s) выставлен за новую цену %d %s.",
			lot, name, newcost, desc_count(newcost, WHAT_MONEYa));
	message_exchange(tmpbuf, ch, lot);
    save_exchange(ch);
	return true;
}

int exchange_withdraw(CHAR_DATA * ch, char *arg)
{
    int lot = 0;
	if (!*arg || !sscanf(arg, "%d", &lot))
	{
		send_to_char("Формат команды: базар снять <лот>\r\n", ch);
		return false;
	}    
    if (!exist_lot(lot))
	{
		send_to_char("Лота с таким номером не существует.\r\n", ch);
		return false;
	}
    if ( (EXCHANGE_ITEM_SELLERID(lot) != GET_IDNUM(ch)) && !is_imm(ch) && !is_impl(ch) )
	{
		send_to_char("Это не ваш лот.\r\n", ch);
		return false;
	}
	    
    OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);

    char tmpbuf[MAX_INPUT_LENGTH];
	sprintf(tmpbuf, "Вы сняли предмет %s с базара.\r\n", descr(obj));
	send_to_char(tmpbuf, ch);
    
    int seller_id = EXCHANGE_ITEM_SELLERID(lot);
	if (seller_id == GET_IDNUM(ch))    
    {
        sprintf(tmpbuf,	"лот %d (%s) снят с продажи владельцем.", lot, descr(obj));	
        message_exchange(tmpbuf, ch, lot);
    }
    else
    {
        CHAR_DATA* seller = get_char_by_id(seller_id);
        if (seller == NULL)
        {
            char *seller_name = get_name_by_id(seller_id);
            if (seller_name)
            {
                save_exchange(seller_id, seller_name);
                sprintf(tmpbuf,
                "Базар : Ваш лот %d (%s) снят с продажи Богом %s.\r\n", lot, obj->short_description, GET_TNAME(ch));
                send_mail(seller_id, NULL, tmpbuf);
            }
        }
        else
        {
            save_exchange(seller);
            sprintf(tmpbuf,
            "&GБазар &K: Ваш лот %d (%s) снят с продажи Богом %s.&n\r\n", lot, descr(obj), GET_TNAME(ch));            
            send_to_char(tmpbuf, seller);
        }
        sprintf(tmpbuf,	"лот %d (%s) снят с продажи Богами.", lot, descr(obj));
        message_exchange(tmpbuf, ch, lot, seller);
    }

    lot_to_char(lot, ch);
    save_exchange(ch);
    save_objects(ch);
    return true;
}

int exchange_information(CHAR_DATA * ch, char *arg)
{
    int lot = 0;
	if (!*arg || !sscanf(arg, "%d", &lot))
	{
		send_to_char("Формат команды: базар информация <лот>\r\n", ch);
		return false;
	}
    if (!exist_lot(lot))
	{
		send_to_char("Лота с таким номером не существует.\r\n", ch);
		return false;
	}

    OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);

	sprintf(buf, "Информация о лоте &W#%d&n:\r\n", lot);
	sprintf(buf + strlen(buf), " Предмет: \"%s&n\", Цена: &G%d&n, Таймер: &G%d&n, ", descr(obj), EXCHANGE_ITEM_COST(lot), GET_OBJ_TIMER(obj));

	strcat(buf, "Тип:&g ");
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
	if (*buf2)
            strcat(buf, buf2);
        strcat(buf, " &n\r\n");
	    
    strcat(buf, diag_weapon_to_char(obj, TRUE));
	strcat(buf, diag_timer_to_char(obj));
	strcat(buf, "\n");

	if (invalid_anti_class(ch, obj) || invalid_unique(ch, obj) )
	{
		sprintf(buf2, "Эта вещь вам недоступна!");
		strcat(buf, buf2);
		strcat(buf, "\r\n");
	}

	if (invalid_align(ch, obj) || invalid_no_class(ch, obj))
	{
		sprintf(buf2, "Вы не сможете пользоваться этой вещью.");
		strcat(buf, buf2);
		strcat(buf, "\r\n");
	}

    const char* seller = get_name_by_id(EXCHANGE_ITEM_SELLERID(lot));
    if (seller)
    {
	    sprintf(buf2, "%s", seller);
	    *buf2 = UPPER(*buf2);
	    strcat(buf, " Продавец:&c ");
	    strcat(buf, buf2);
	    strcat(buf, "&n\r\n");
    }

    if (!EXCHANGE_ITEM_COMMENT(lot).empty())
	{
		strcat(buf, "Наклейка на лоте гласит: ");
        sprintf(buf2, "'%s'.", EXCHANGE_ITEM_COMMENT(lot).c_str());
		strcat(buf, buf2);
		strcat(buf, "\r\n");
	}

    act(buf, FALSE, ch, obj, 0, TO_CHAR);
	return true;
}

int exchange_identify(CHAR_DATA * ch, char *arg)
{
    int lot = 0;
    if (!*arg || !sscanf(arg, "%d", &lot))
	{
		send_to_char("Формат команды: базар характеристики <лот>\r\n", ch);
		return false;
	}

    if (!exist_lot(lot))
	{
		send_to_char("Лота с таким номером не существует.\r\n", ch);
		return false;
	}

    int tax = EXCHANGE_IDENT_PAY;
    if (is_imm(ch) || is_impl(ch))
        tax = 0;

    if (GET_BANK_GOLD(ch) < tax)
    {
		send_to_char("На счету в Банке недостаточно средств для оплаты комиссии!\r\n", ch);
		return false;
	}
    if (tax > 0) {
        GET_BANK_GOLD(ch) -= tax;
    }

    OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);
	if (GET_LEVEL(ch) < LVL_IMPL)
		mort_show_obj_values(obj, ch, 200);	//200 - полное опознание
	else
		imm_show_obj_values(obj, ch);

    if (tax > 0) {
	    send_to_char(ch, "\r\n%sКомиссия за услугу %d %s%s.\r\n",
		CCNRM(ch, C_NRM), tax, desc_count(tax, WHAT_MONEYu), CCNRM(ch, C_NRM));
    }
	return true;
}

int exchange_purchase(CHAR_DATA * ch, char *arg)
{
    if (is_imm(ch))
        return false;

    int lot = 0;
	if (!*arg || !sscanf(arg, "%d", &lot))
	{
		send_to_char("Формат команды: базар купить <лот>\r\n", ch);
		return false;
	}

    if (!exist_lot(lot))
	{
		send_to_char("Лота с таким номером не существует.\r\n", ch);
		return false;
	}

	if (EXCHANGE_ITEM_SELLERID(lot) == GET_IDNUM(ch))
	{
		send_to_char("Это же ваш лот. Воспользуйтесь командой 'базар снять <лот>'\r\n", ch);
		return false;
	}

    int item_cost = EXCHANGE_ITEM_COST(lot);
	if ((GET_BANK_GOLD(ch) < item_cost) && (GET_LEVEL(ch) < LVL_IMPL))
	{
		send_to_char("У Вас в Банке не хватает денег на этот лот!\r\n", ch);
		return false;
	}

    OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);	
    CHAR_DATA* seller = get_char_by_id(EXCHANGE_ITEM_SELLERID(lot));   
	if (seller == NULL)
	{
        char tmpbuf[MAX_INPUT_LENGTH];
		char *seller_name = get_name_by_id(EXCHANGE_ITEM_SELLERID(lot));
        seller = new CHAR_DATA; 
        memset(seller, 0, sizeof(struct char_data));
		if ((seller_name == NULL) || (load_char(seller_name, seller) < 0))
		{
            if (!is_impl(ch))
			    GET_BANK_GOLD(ch) -= item_cost;
			act("Вы приобрели на базаре $O. Покупка ждет вас на складе.\r\n", FALSE, ch, 0, obj, TO_CHAR);
			
            OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);
            int cost = EXCHANGE_ITEM_COST(lot);

            sprintf(tmpbuf,
					"лот %d (%s) продан за %d %s.", lot,
					descr(obj),
					cost, desc_count(cost, WHAT_MONEYu));
			message_exchange(tmpbuf, ch, lot);

            lot_to_char(lot, ch);
			save_objects(ch);

            delete seller;
			return true;
		}

        if (!is_impl(ch))
            GET_BANK_GOLD(ch) -= item_cost;

        // напишем письмецо чару, раз уж его нету онлайн
		sprintf(tmpbuf,
            "Базар : лот %d (%s) продан. %d %s переведено на ваш счет.\r\n", lot, descr(obj),
            item_cost, desc_count((item_cost), WHAT_MONEYa) );
        send_mail(EXCHANGE_ITEM_SELLERID(lot), NULL, tmpbuf);

		act("Вы купили на базаре $O.\r\n", FALSE, ch, 0, obj, TO_CHAR);
        
        OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);
        int cost = EXCHANGE_ITEM_COST(lot);

        sprintf(tmpbuf,
				"лот %d (%s) продан за %d %s.", lot,
				descr(obj),
				cost, desc_count(cost, WHAT_MONEYu));
		message_exchange(tmpbuf, ch, lot);

        lot_to_char(lot, ch);

        save_exchange(seller);
        save_bank(seller, item_cost);
		delete seller;

        save_objects(ch);
	}
	else
	{
		GET_BANK_GOLD(seller) += item_cost;
        if (!is_impl(ch))
            GET_BANK_GOLD(ch) -= item_cost;
		act("Вы приобрели на базаре $O. Покупка ждет вас на складе.\r\n", FALSE, ch, 0, obj, TO_CHAR);

        char tmpbuf[MAX_INPUT_LENGTH];
		sprintf(tmpbuf,
				"&GБазар: &Kлот %d (%s) продан. %d %s переведено на ваш счет.&n\r\n", lot,
                descr(obj),
				item_cost, desc_count(item_cost, WHAT_MONEYa));
		act(tmpbuf, FALSE, seller, 0, NULL, TO_CHAR);

        OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);
        int cost = EXCHANGE_ITEM_COST(lot);

        sprintf(tmpbuf,
				"лот %d (%s) продан за %d %s.", lot,
				descr(obj),
				cost, desc_count(cost, WHAT_MONEYu));
		message_exchange(tmpbuf, seller, lot, ch);

        lot_to_char(lot, ch);
        save_exchange(seller);
        save_objects(ch);
	}
    return true;
}

/**
* Проверка длины фильтра.
* \return false - перебор
*         true - корректный
*/
bool correct_filter_length(CHAR_DATA *ch, const char *str)
{
	if (strlen(str) >= FILTER_LENGTH)
	{
		send_to_char(ch, "Слишком длинный фильтр, максимальная длина: %d символа.\r\n", FILTER_LENGTH - 1);
		return false;
	}
	return true;
}

int exchange_offers(CHAR_DATA * ch, char *arg)
{
    //влом байты считать. Если кто хочет оптимизировать, посчитайте точно.
	char filter[MAX_STRING_LENGTH];
	char multifilter[MAX_STRING_LENGTH];
	short int show_type;
	bool ignore_filter;
	
	//show_type
	//0 - все
	//1 - мои
	//2 - руны
	//3 - одежда
	//4 - оружие
	//5 - книги
	//6 - ингры
	//7 - прочие

	memset(filter, 0, FILTER_LENGTH);
	memset(multifilter, 0, FILTER_LENGTH);

	(arg = one_argument(arg, arg1));
	(arg = one_argument(arg, arg2));
	(arg = one_argument(arg, arg3));
	(arg = one_argument(arg, arg4));

	ignore_filter = ((*arg2 == '*') || (*arg3 == '*') || (*arg4 == '*'));

	if (*arg2 == '!')
	{
		strcpy(multifilter, arg2 + 1);
	}
	if (*arg3 == '!')
	{
		strcpy(multifilter, arg3 + 1);
	}
	if (*arg4 == '!')
	{
		strcpy(multifilter, arg4 + 1);
	}

	if (is_abbrev(arg1, "все") || is_abbrev(arg1, "all"))
	{
		show_type = 0;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "И%s", arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " О");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "мои") || is_abbrev(arg1, "mine"))
	{
		show_type = 1;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " О");
			strcat(filter, multifilter);
		}
	}
	/*else if (is_abbrev(arg1, "руны") || is_abbrev(arg1, "runes"))
	{
		show_type = 2;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " С");
			strcat(filter, multifilter);
		}
	}*/
	else if (is_abbrev(arg1, "броня") || is_abbrev(arg1, "armor"))
	{
		show_type = 3;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " О");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "оружие") || is_abbrev(arg1, "weapons"))
	{
		show_type = 4;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " К");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "книги") || is_abbrev(arg1, "books"))
	{
		show_type = 5;
		if ((*arg2) && (*arg2 != '*'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
	}
	/*else if (is_abbrev(arg1, "ингредиенты") || is_abbrev(arg1, "ingradients"))
	{
		show_type = 6;
		if ((*arg2) && (*arg2 != '*'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
	}*/
	else if (is_abbrev(arg1, "прочие") || is_abbrev(arg1, "прочее") || is_abbrev(arg1, "other"))
	{
		show_type = 7;
		if ((*arg2) && (*arg2 != '*'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
	}
    else if (is_abbrev(arg1, "уникальные") || is_abbrev(arg1, "unique"))
	{
		show_type = 8;
		if ((*arg2) && (*arg2 != '*'))
		{
			sprintf(filter, "%s Т%s", filter, arg2);
		}
	}
	else
	{
		send_to_char(info_message, ch);
		return 0;
	}

	if (!ignore_filter && EXCHANGE_FILTER(ch))
		sprintf(multifilter, "%s %s", EXCHANGE_FILTER(ch), filter);
	else
		strcpy(multifilter, filter);

	if (!correct_filter_length(ch, multifilter))
	{
		return 0;
	}

	show_lots(multifilter, show_type, ch);
	return 1;
}

int exchange_setfilter(CHAR_DATA * ch, char *arg)
{
	char tmpbuf[MAX_INPUT_LENGTH];
	char filter_name[FILTER_LENGTH];
	char filter_owner[FILTER_LENGTH];
	int filter_type = 0;
	int filter_cost = 0;
	int filter_timer = 0;
	int filter_wereon = 0;
	int filter_weaponclass = 0;
	char filter[MAX_INPUT_LENGTH];

	if (!*arg)
	{
		if (!EXCHANGE_FILTER(ch))
		{
			send_to_char("Ваш фильтр базара пуст.\r\n", ch);
			return true;
		}
		if (!parse_exch_filter(EXCHANGE_FILTER(ch), filter_name, filter_owner,
							   &filter_type, &filter_cost, &filter_timer, &filter_wereon, &filter_weaponclass))
		{
			free(EXCHANGE_FILTER(ch));
			EXCHANGE_FILTER(ch) = NULL;
			send_to_char("Ваш фильтр базара пуст.\r\n", ch);
			return true;
		}
		else
		{
			sprintf(tmpbuf, "Ваш текущий фильтр базара: %s.\r\n", EXCHANGE_FILTER(ch));
			send_to_char(tmpbuf, ch);
			return true;
		}
	}


	skip_spaces(&arg);
	strcpy(filter, arg);
	if (!correct_filter_length(ch, filter))
	{
		return false;
	}
	if (!strncmp(filter, "нет", 3))
	{
		if (EXCHANGE_FILTER(ch))
		{
			sprintf(tmpbuf, "Ваш старый фильтр: %s. Новый фильтр пуст.\r\n", EXCHANGE_FILTER(ch));
			free(EXCHANGE_FILTER(ch));
			EXCHANGE_FILTER(ch) = NULL;
		}
		else
			sprintf(tmpbuf, "Новый фильтр пуст.\r\n");
		send_to_char(tmpbuf, ch);
		return true;
	}
	if (!parse_exch_filter(filter, filter_name, filter_owner,
						   &filter_type, &filter_cost, &filter_timer, &filter_wereon, &filter_weaponclass))
	{
		send_to_char("Неверный формат фильтра. Прочтите справку.\r\n", ch);
		free(EXCHANGE_FILTER(ch));
		EXCHANGE_FILTER(ch) = NULL;
		return false;
	}
	if (EXCHANGE_FILTER(ch))
		sprintf(tmpbuf, "Ваш старый фильтр: %s. Новый фильтр: %s.\r\n", EXCHANGE_FILTER(ch), filter);
	else
		sprintf(tmpbuf, "Ваш новый фильтр: %s.\r\n", filter);

	send_to_char(tmpbuf, ch);

	if (EXCHANGE_FILTER(ch))
		free(EXCHANGE_FILTER(ch));
	EXCHANGE_FILTER(ch) = str_dup(filter);

	return true;
}

int get_free_lot(void)
{
    int used_slots = exchange_item_list.size() - free_slots.size();
    if (used_slots >= 10000)
        return -1;

    if (!free_slots.empty())
    {
        int id = free_slots[0];
        free_slots.erase(free_slots.begin());
        return id;
    }

    EXCHANGE_ITEM_DATA new_slot;
    exchange_item_list.push_back(new_slot);
    int last = exchange_item_list.size() - 1;
    return last;
}

void lot_to_char(int lot, CHAR_DATA *ch)
{
    OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);
    EXCHANGE_ITEM_OBJ(lot) = NULL;
    delete_lot(lot);

    WareHouse &w = GET_CHEST(ch);
    w.PushObj(obj);
}

void delete_lot(int lot)
{
    exchange_item_list[lot].clear();
    free_slots.push_back(lot);
    std::sort(free_slots.begin(), free_slots.end());
}

void message_exchange(char *message, CHAR_DATA * ch, int lot, CHAR_DATA * ch2)
{
	char filter_name[FILTER_LENGTH];
	char filter_owner[FILTER_LENGTH];
	int filter_type = 0;
	int filter_cost = 0;
	int filter_timer = 0;
	int filter_wereon = 0;
	int filter_weaponclass = 0;

	memset(filter_name, 0, FILTER_LENGTH);
	memset(filter_owner, 0, FILTER_LENGTH);

    DESCRIPTOR_DATA *i = descriptor_list;
	for (; i; i = i->next)
    {
		if (STATE(i) == CON_PLAYING &&
				i->character &&
				!PRF_FLAGGED(i->character, PRF_NOAUCT) &&
				!PLR_FLAGGED(i->character, PLR_WRITING) &&
				!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && 
                GET_POS(i->character) > POS_SLEEPING)
		{
            if (ch && i == ch->desc)
                continue;
            if (ch2 && i == ch2->desc)
                continue;
			if (!EXCHANGE_FILTER(i->character)
					|| ((parse_exch_filter(EXCHANGE_FILTER(i->character),
										   filter_name, filter_owner, &filter_type, &filter_cost, &filter_timer,
										   &filter_wereon, &filter_weaponclass))
						&& (obj_matches_filter(lot, filter_name, filter_owner, &filter_type, &filter_cost, &filter_timer,
						  &filter_wereon, &filter_weaponclass))))
			{
				send_to_char("&GБазар: &K", i->character);
				act(message, FALSE, i->character, 0, 0, TO_CHAR | TO_SLEEP);
				send_to_char("&n", i->character);
			}
		}
	}
}

int obj_matches_filter(int lot, char *filter_name, char *filter_owner, int *filter_type,
					   int *filter_cost, int *filter_timer, int *filter_wereon, int *filter_weaponclass)
{
	int tm;

    OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(lot);
    const char* name = obj->short_description;

    if (*filter_name && !isname(filter_name, name ))
	{
		if ((GET_OBJ_TYPE(obj) != ITEM_MING) &&	(GET_OBJ_TYPE(obj) != ITEM_INGRADIENT))
			return 0;
		//Для ингридиентов, дополнительно проверяем синонимы прототипa
		else
        {
            OBJ_DATA *proto = &obj_proto[GET_OBJ_RNUM(obj)];
            if (!isname(filter_name, GET_OBJ_ALIAS(proto) ))
			    return 0;
        }
	}
	if (*filter_owner && !isname(filter_owner, get_name_by_id(EXCHANGE_ITEM_SELLERID(lot))))
		return 0;
	if (*filter_type && !(GET_OBJ_TYPE(obj) == *filter_type))
		return 0;
	if (*filter_cost)
	{
		if ((*filter_cost > 0) && (EXCHANGE_ITEM_COST(lot) - *filter_cost < 0))
			return 0;
		else if ((*filter_cost < 0) && (EXCHANGE_ITEM_COST(lot) + *filter_cost > 0))
			return 0;
	}
	if (*filter_wereon && (!CAN_WEAR(obj, *filter_wereon)))
		return 0;
	if (*filter_weaponclass
			&& (GET_OBJ_TYPE(obj) != ITEM_WEAPON || GET_OBJ_SKILL(obj) != *filter_weaponclass))
		return 0;
	if (*filter_timer)
	{
		// Вобщем чтобы тут не валилось от всяких писем и прочей ваты на базаре,
		// сейчас туда нельзя ставить вещи с -1 рнумом, но на всякий оставим // Krodo
		try
		{
            OBJ_DATA *proto = &obj_proto[GET_OBJ_RNUM(obj)];
			tm = (GET_OBJ_TIMER(obj) * 100 / GET_OBJ_TIMER(proto));
			if ((tm + 1) < *filter_timer)
				return 0;
		}
		catch (std::out_of_range)
		{
			log("SYSERROR: wrong obj_proto in exchange");
			return 0;
		}
	}

	return 1;		// 1 - Объект подподает под фильтр
}

void show_lots(char *filter, short int show_type, CHAR_DATA * ch)
{
	/*
	show_type
	0 - все
	1 - мои
	2 - руны
	3 - одежда
	4 - оружие
	5 - книги
	6 - ингры
	7 - прочее
	*/
	    
	bool any_item = 0;
	char filter_name[FILTER_LENGTH];
	char filter_owner[FILTER_LENGTH];
	int filter_type = 0;
	int filter_cost = 0;
	int filter_timer = 0;
	int filter_wereon = 0;
	int filter_weaponclass = 0;

	memset(filter_name, 0, FILTER_LENGTH);
	memset(filter_owner, 0, FILTER_LENGTH);

	if (!parse_exch_filter(filter, filter_name, filter_owner, &filter_type, &filter_cost, &filter_timer,
						   &filter_wereon, &filter_weaponclass))
	{
		send_to_char("Неверная строка фильтрации!\r\n", ch);
		log("Exchange: Player uses wrong filter '%s'", filter);
		return;
	}

    TempBuffer buffer;
    buffer.append(" Лот     Предмет                                     Цена  Таймер   Состояние\r\n");
    buffer.append("---------------------------------------------------------------------------------\r\n");

    char tmpbuf[MAX_INPUT_LENGTH];
    for (int i=0,e=exchange_item_list.size(); i<e; ++i)
    {
        if (EXCHANGE_ITEM_SELLERID(i) < 0)
            continue;

        if (!obj_matches_filter(i, filter_name, filter_owner, &filter_type,
			&filter_cost, &filter_timer, &filter_wereon, &filter_weaponclass))
            continue;
        if (show_type == 1 && (!isname(GET_NAME(ch), get_name_by_id(EXCHANGE_ITEM_SELLERID(i)))) )
            continue;

        int obj_type = GET_OBJ_TYPE(EXCHANGE_ITEM_OBJ(i));
        if (show_type == 2 && obj_type != ITEM_MING)
            continue;
        if (show_type == 3 && obj_type != ITEM_ARMOR)
            continue;
        if (show_type == 4 && obj_type != ITEM_WEAPON)
            continue;
        if (show_type == 5 && obj_type != ITEM_BOOK)
            continue;
        if (show_type == 6 && obj_type != ITEM_INGRADIENT)        
            continue;         
        if (show_type == 7)
        {
            //if (obj_type == ITEM_INGRADIENT || obj_type == ITEM_MING ||
            if (obj_type == ITEM_WEAPON || obj_type == ITEM_ARMOR || obj_type == ITEM_BOOK)
                continue;
        }
        OBJ_DATA *obj = EXCHANGE_ITEM_OBJ(i);
        if (show_type == 8)
        {
            if (!GET_OBJ_RLVL(obj))
                continue;
        }

        std::string name(obj->short_description);
        int ml = 38;
        int len = name.length();
        if (len > ml)
            name = name.substr(0, ml);
        else if (len < ml)
        {
            std::string sp(ml-len, ' ');
            name.append(sp);
        }

        if (GET_OBJ_RLVL(obj)) {
            sprintf(tmpbuf, "&W%s&n", name.c_str());
            name.assign(tmpbuf);
        }
           
        sprintf(tmpbuf, "[%4d]   %s %9d   %5d   %s\r\n", i, 
            name.c_str(), EXCHANGE_ITEM_COST(i), GET_OBJ_TIMER(obj), diag_obj_to_char(ch, obj, 3) );

		// Такое вот кино, на выделения для каждой строчки тут уходило до 0.6 секунды при выводе всего базара. стринги рулят -- Krodo
        buffer.append(tmpbuf);
		any_item = 1;
	}

	if (!any_item)
    {
        send_to_char("Базар пуст!\r\n", ch);
        return;
    }

    page_string(ch->desc, buffer, 1);
}

int parse_exch_filter(char *buf, char *filter_name, char *filter_owner, int *filter_type,
					  int *filter_cost, int *filter_timer, int *filter_wereon, int *filter_weaponclass)
{
	char sign[FILTER_LENGTH];
	char tmpbuf[FILTER_LENGTH];

	while (*buf && (*buf != '\r') && (*buf != '\n'))
	{
		switch (*buf)
		{
		case ' ':
			buf++;
			break;
		case 'И':
			buf = one_argument(++buf, filter_name);
			break;
		case 'В':
			buf = one_argument(++buf, filter_owner);
			break;
		case 'Т':
			buf = one_argument(++buf, tmpbuf);
			if (is_abbrev(tmpbuf, "свет") || is_abbrev(tmpbuf, "light"))
				*filter_type = ITEM_LIGHT;
			else if (is_abbrev(tmpbuf, "свиток") || is_abbrev(tmpbuf, "scroll"))
				*filter_type = ITEM_SCROLL;
			else if (is_abbrev(tmpbuf, "палочка") || is_abbrev(tmpbuf, "wand"))
				*filter_type = ITEM_WAND;
			else if (is_abbrev(tmpbuf, "посох") || is_abbrev(tmpbuf, "staff"))
				*filter_type = ITEM_STAFF;
			else if (is_abbrev(tmpbuf, "оружие") || is_abbrev(tmpbuf, "weapon"))
				*filter_type = ITEM_WEAPON;
			else if (is_abbrev(tmpbuf, "броня") || is_abbrev(tmpbuf, "armor"))
				*filter_type = ITEM_ARMOR;
			else if (is_abbrev(tmpbuf, "напиток") || is_abbrev(tmpbuf, "potion"))
				*filter_type = ITEM_POTION;
			else if (is_abbrev(tmpbuf, "прочее") || is_abbrev(tmpbuf, "other"))
				*filter_type = ITEM_OTHER;
			else if (is_abbrev(tmpbuf, "контейнер") || is_abbrev(tmpbuf, "container"))
				*filter_type = ITEM_CONTAINER;
			else if (is_abbrev(tmpbuf, "емкость") || is_abbrev(tmpbuf, "tank"))
				*filter_type = ITEM_DRINKCON;
			else if (is_abbrev(tmpbuf, "книга") || is_abbrev(tmpbuf, "book"))
				*filter_type = ITEM_BOOK;
			else if (is_abbrev(tmpbuf, "руна") || is_abbrev(tmpbuf, "rune"))
				*filter_type = ITEM_INGRADIENT;
			else if (is_abbrev(tmpbuf, "ингредиент") || is_abbrev(tmpbuf, "ingradient"))
				*filter_type = ITEM_MING;
			else
				return 0;
			break;
		case 'Ц':
			buf = one_argument(++buf, tmpbuf);
			if (sscanf(tmpbuf, "%d%[-+]", filter_cost, sign) != 2)
				return 0;
			if (*sign == '-')
				*filter_cost = -(*filter_cost);
			break;
		case 'С':
			buf = one_argument(++buf, tmpbuf);
			if (is_abbrev(tmpbuf, "ужасно"))
				*filter_timer = 1;
			else if (is_abbrev(tmpbuf, "скоро_испортится"))
				*filter_timer = 21;
			else if (is_abbrev(tmpbuf, "плоховато"))
				*filter_timer = 41;
			else if (is_abbrev(tmpbuf, "средне"))
				*filter_timer = 61;
			else if (is_abbrev(tmpbuf, "идеально"))
				*filter_timer = 81;
			else
				return 0;

			break;
		case 'О':
			buf = one_argument(++buf, tmpbuf);
			if (is_abbrev(tmpbuf, "палец"))
				*filter_wereon = ITEM_WEAR_FINGER;
			else if (is_abbrev(tmpbuf, "шея") || is_abbrev(tmpbuf, "грудь"))
				*filter_wereon = ITEM_WEAR_NECK;
			else if (is_abbrev(tmpbuf, "тело"))
				*filter_wereon = ITEM_WEAR_BODY;
			else if (is_abbrev(tmpbuf, "голова"))
				*filter_wereon = ITEM_WEAR_HEAD;
			else if (is_abbrev(tmpbuf, "ноги"))
				*filter_wereon = ITEM_WEAR_LEGS;
			else if (is_abbrev(tmpbuf, "ступни"))
				*filter_wereon = ITEM_WEAR_FEET;
			else if (is_abbrev(tmpbuf, "кисти"))
				*filter_wereon = ITEM_WEAR_HANDS;
			else if (is_abbrev(tmpbuf, "руки"))
				*filter_wereon = ITEM_WEAR_ARMS;
			else if (is_abbrev(tmpbuf, "щит"))
				*filter_wereon = ITEM_WEAR_SHIELD;
			else if (is_abbrev(tmpbuf, "плечи"))
				*filter_wereon = ITEM_WEAR_ABOUT;
			else if (is_abbrev(tmpbuf, "пояс"))
				*filter_wereon = ITEM_WEAR_WAIST;
			else if (is_abbrev(tmpbuf, "запястья"))
				*filter_wereon = ITEM_WEAR_WRIST;
			else if (is_abbrev(tmpbuf, "левая"))
				*filter_wereon = ITEM_WEAR_HOLD;
			else if (is_abbrev(tmpbuf, "правая"))
				*filter_wereon = ITEM_WEAR_WIELD;
			else if (is_abbrev(tmpbuf, "обе"))
				*filter_wereon = ITEM_WEAR_BOTHS;
			else
				return 0;
			break;
		case 'К':
			buf = one_argument(++buf, tmpbuf);
			if (is_abbrev(tmpbuf, "луки"))
				*filter_weaponclass = SKILL_BOWS;
			else if (is_abbrev(tmpbuf, "короткие"))
				*filter_weaponclass = SKILL_SHORTS;
			else if (is_abbrev(tmpbuf, "длинные"))
				*filter_weaponclass = SKILL_LONGS;
			else if (is_abbrev(tmpbuf, "секиры"))
				*filter_weaponclass = SKILL_AXES;
			else if (is_abbrev(tmpbuf, "палицы"))
				*filter_weaponclass = SKILL_CLUBS;
			else if (is_abbrev(tmpbuf, "иное"))
				*filter_weaponclass = SKILL_NONSTANDART;
			else if (is_abbrev(tmpbuf, "двуручники"))
				*filter_weaponclass = SKILL_BOTHHANDS;
			else if (is_abbrev(tmpbuf, "проникающее"))
				*filter_weaponclass = SKILL_PICK;
			else if (is_abbrev(tmpbuf, "копья"))
				*filter_weaponclass = SKILL_SPADES;
			else
				return 0;
			break;
		default:
			return 0;
		}
	}

	return 1;
}

// дублируем кучку кода, чтобы можно было часть команд базара выполнять в любой комнате
ACMD(do_exchange)
{
        if (IS_NPC(ch))
			return;

        char* arg = str_dup(argument);
		argument = one_argument(argument, arg1);	

		/*if (is_abbrev(arg1, "выставить") || is_abbrev(arg1, "exhibit")
		|| is_abbrev(arg1, "цена") || is_abbrev(arg1, "cost")
		|| is_abbrev(arg1, "снять") || is_abbrev(arg1, "withdraw")
		|| is_abbrev(arg1, "купить") || is_abbrev(arg1, "purchase")
		//commented by WorM 2011.05.21 а нафиг чтоб сохранить/загрузить базар быть у базарного торговца?
		/*|| (is_abbrev(arg1, "save") && (GET_LEVEL(ch) >= LVL_IMPL))
		|| (is_abbrev(arg1, "savebackup") && (GET_LEVEL(ch) >= LVL_IMPL))
		|| (is_abbrev(arg1, "reload") && (GET_LEVEL(ch) >= LVL_IMPL))
		|| (is_abbrev(arg1, "reloadbackup") && (GET_LEVEL(ch) >= LVL_IMPL)))
		/*{
			send_to_char("Вам необходимо находиться возле базарного торговца, чтобы воспользоваться этой командой.", ch);
		} else*/

        exchange(ch,NULL,cmd,arg);
		free(arg);
}
