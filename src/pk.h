struct PK_Memory_type
{
  long     unique;        // unique игрока
  long     kill_num;      // количество флагов носителя структуры на unique
  long     kill_at;       // время получения последнего флага (для spamm)
  long     revenge_num;   // количетсво попыток реализации мести со стороны unique
  long     battle_exp;    // время истечения поединка
  long     thief_exp;     // время истечения флага воровства
  long     clan_exp;      // время истечения клан-флага
  long     temp_time;     // время временной мести если в ранге -+3 левела.
  long     temp_at;       // продление временной мести ? может и нафиг не надо щас посмотрю.
  struct   PK_Memory_type *next;
};

#define		PK_ACTION_NO		1		// никаких конфликтов
#define		PK_ACTION_FIGHT     2		// действия в процессе поединка
#define		PK_ACTION_REVENGE	4		// попытка реализовать месть
#define		PK_ACTION_KILL		8		// нападение
#define		PK_TEMP_REVENGE		16		// временная месть
//#define		PK_ACTION_GROUP		32		// агрессор в группе
// agressor действует против victim 
// Результат - тип действий (см. выше)
int pk_action_type( struct char_data * agressor, struct char_data * victim );

// Проверка может ли ch начать аргессивные действия против victim
// TRUE - может
// FALSE - не может
int may_kill_here(
  struct char_data *ch, 
  struct char_data *victim
);

// Определение возможных ПК действий
int check_pkill(
  struct char_data *ch,
  struct char_data *opponent,
  char *arg
);

// agressor проводит действия против victim
void pk_agro_action( 
  struct char_data * agressor,
  struct char_data * victim
);

// thief проводит действия против victim, приводящие к скрытому флагу
void pk_thiefs_action(
  struct char_data * thief,
  struct char_data * victim
);

// killer убивает victim
void pk_revenge_action(
  struct char_data * killer,
  struct char_data * victim
);

// удалить список ПК
void pk_free_list(
  struct char_data * ch
);

//*************************************************************************
// Информационные функции отображения статуса ПК

void aura(struct char_data *ch, int lvl, struct char_data *victim, char *s);
char *CCPK(struct char_data *ch, int lvl, struct char_data *victim);
void pk_list_sprintf(struct char_data *ch, char *buff);


//*************************************************************************
// Системные функции сохранения/загрузки ПК флагов

void load_pkills(struct char_data * ch);
void save_pkills(struct char_data * ch);
void new_save_pkills(struct char_data * ch);
void pk_temp_timer(struct char_data * ch);
