/* ************************************************************************
*   File: mobmax.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for явовмхв тр орчво   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

void new_load_mkill(struct char_data *ch);
int  clear_kill_vnum(struct char_data *vict, int vnum);
void inc_kill_vnum(struct char_data *ch, int vnum, int incvalue);
int  get_kill_vnum(struct char_data *ch, int vnum);
void save_mkill(struct char_data *ch, FILE *saved);
void new_save_mkill(struct char_data *ch);
void free_mkill(struct char_data *ch);
void delete_mkill_file(char *name);
void mob_lev_count(void);
