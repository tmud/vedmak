struct PK_Memory_type
{
  long     unique;        // unique ������
  long     kill_num;      // ���������� ������ �������� ��������� �� unique
  long     kill_at;       // ����� ��������� ���������� ����� (��� spamm)
  long     revenge_num;   // ���������� ������� ���������� ����� �� ������� unique
  long     battle_exp;    // ����� ��������� ��������
  long     thief_exp;     // ����� ��������� ����� ���������
  long     clan_exp;      // ����� ��������� ����-�����
  long     temp_time;     // ����� ��������� ����� ���� � ����� -+3 ������.
  long     temp_at;       // ��������� ��������� ����� ? ����� � ����� �� ���� ��� ��������.
  struct   PK_Memory_type *next;
};

#define		PK_ACTION_NO		1		// ������� ����������
#define		PK_ACTION_FIGHT     2		// �������� � �������� ��������
#define		PK_ACTION_REVENGE	4		// ������� ����������� �����
#define		PK_ACTION_KILL		8		// ���������
#define		PK_TEMP_REVENGE		16		// ��������� �����
//#define		PK_ACTION_GROUP		32		// �������� � ������
// agressor ��������� ������ victim 
// ��������� - ��� �������� (��. ����)
int pk_action_type( struct char_data * agressor, struct char_data * victim );

// �������� ����� �� ch ������ ����������� �������� ������ victim
// TRUE - �����
// FALSE - �� �����
int may_kill_here(
  struct char_data *ch, 
  struct char_data *victim
);

// ����������� ��������� �� ��������
int check_pkill(
  struct char_data *ch,
  struct char_data *opponent,
  char *arg
);

// agressor �������� �������� ������ victim
void pk_agro_action( 
  struct char_data * agressor,
  struct char_data * victim
);

// thief �������� �������� ������ victim, ���������� � �������� �����
void pk_thiefs_action(
  struct char_data * thief,
  struct char_data * victim
);

// killer ������� victim
void pk_revenge_action(
  struct char_data * killer,
  struct char_data * victim
);

// ������� ������ ��
void pk_free_list(
  struct char_data * ch
);

//*************************************************************************
// �������������� ������� ����������� ������� ��

void aura(struct char_data *ch, int lvl, struct char_data *victim, char *s);
char *CCPK(struct char_data *ch, int lvl, struct char_data *victim);
void pk_list_sprintf(struct char_data *ch, char *buff);


//*************************************************************************
// ��������� ������� ����������/�������� �� ������

void load_pkills(struct char_data * ch);
void save_pkills(struct char_data * ch);
void new_save_pkills(struct char_data * ch);
void pk_temp_timer(struct char_data * ch);
