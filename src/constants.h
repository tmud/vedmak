extern const char *circlemud_version;
extern const char *dirs1[];
extern const char *dirs2[];
extern const char *dirs[];
extern const char *room_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *genders[];
extern const char *position_types[];
extern const char *player_bits[];
extern const char *action_bits[];
extern const char *special_bits[];
extern const char *preference_bits[];
extern const char *affected_bits[];
extern const char *where[];
extern const char *equipment_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *apply_negative[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *drinks[];
extern const char *drinknames[];
extern const char *color_liquid[];
extern const char *fullness[];
extern const char *spell_wear_off_msg[];
extern const char *pc_class_types[];
extern const char *weekdays[];
extern const char *month_name[];
extern const char *weekdays_poly[];
extern const char *month_name_poly[];
extern const char *npc_race_types[];
extern const char *pc_race_types[][3];
extern const struct str_app_type str_app[];
extern const struct dex_skill_type dex_app_skill[];
extern const struct dex_app_type dex_app[];
extern const struct con_app_type con_app[];
extern const struct int_app_type int_app[];
extern const struct wis_app_type wis_app[];
extern const struct weapon_affect_types weapon_affect[];
extern const struct class_app_type class_app[];
extern const struct race_app_type race_app[];
extern const char *ingradient_bits[];
extern int rev_dir[];
extern int movement_loss[];
extern int drink_aff[][3];
extern const struct size_app_type size_app[];
extern const struct weapon_app_type weapon_app[];
extern const struct cha_app_type cha_app[];
extern const char *anti_bits[];
extern const char *no_bits[];

#define STRANGE_ROOM     1
#define FIRST_ROOM       0
#define DG_CASTER_PROXY  1

#define FIRE_MOVES       20
#define LOOKING_MOVES    5
#define HEARING_MOVES    2
#define LOOKHIDE_MOVES   3
#define SNEAK_MOVES      1
#define CAMOUFLAGE_MOVES 1
#define PICKLOCK_MOVES   10
#define TRACK_MOVES      2
#define SENSE_MOVES      3
#define HIDETRACK_MOVES  10

extern int HORSE_VNUM;
extern int HORSE_COST;
extern int START_SWORD;
extern int START_CLUB;
extern int START_KNIFE;
extern int START_BREAD;
extern int START_SCROLL;
extern int CREATE_LIGHT;
extern int START_LIGHT;
extern int START_ARMOR;
extern int START_BOTTLE;
extern int START_BOW;
extern int START_WRUNE;
extern int START_ARUNE;
extern int START_ERUNE;
extern int START_FRUNE;
