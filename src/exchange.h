/**************************************************************************
*   File: exchange.h                                 Part of Bylins       *
*  Usage: Exhange headers functions used by the MUD                       *
*  Modified for Gold Dragon mud                                           *
************************************************************************ */

#ifndef _EXCHANGE_HPP_
#define _EXCHANGE_HPP_

struct exchange_item_data
{
    exchange_item_data() : seller_id(-1), obj_cost(0), obj(NULL) {}
    ~exchange_item_data() {}
    void clear() 
    {   seller_id = -1;
        obj_cost = 0;
        comment.clear();
        if (obj)
            extract_obj(obj);
        obj = NULL; 
    }
	int seller_id;		        //����� ��������
	int obj_cost;		        //���� ����
    std::string comment;		//����������
	OBJ_DATA *obj;		        //���������� �������
};
typedef struct exchange_item_data
			EXCHANGE_ITEM_DATA;

void load_exchange_db();
void update_exchange_bank(CHAR_DATA *ch);
void exchange_tick_timer_objects();

#define EXCHANGE_BANK_FILE "plrobjs/exchange.bank"
#define EXCHANGE_TIMERS_FILE "plrobjs/exchange.timers"
#define EX_END_CHAR '$'
#define FILTER_LENGTH 32
#define EXCHANGE_EXHIBIT_PAY_COEFF 0.05	// ����������� ������ � ����������� �� ���� ������
#define EXCHANGE_IDENT_PAY 100	        // ���� �� ���������
#define EXCHANGE_MIN_CHAR_LEV 5	        // ����������� ������� ��� ������� � ������
#define EXCHANGE_MAX_EXHIBIT_PER_CHAR 20// ������������ ���-�� ������������ �������� ����� �����

#endif
