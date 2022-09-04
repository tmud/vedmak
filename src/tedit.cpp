/*
 * Originally written by: Michael Scott -- Manx.
 * Last known e-mail address: scottm@workcomm.net
 *
 * XXX: This needs Oasis-ifying.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "improved-edit.h"
#include "tedit.h"

extern char *credits;
extern char *news;
extern char *motd;
extern char *imotd;
extern char *help;
extern char *info;
extern char *background;
extern char *handbook;
extern char *policies;
extern char *zony;


/*
 * NOTE: This changes the buffer passed in.
 */
void strip_cr(char *buffer)
{
  int rpos, wpos;

  if (buffer == NULL)
    return;

  for (rpos = 0, wpos = 0; buffer[rpos]; rpos++) {
    buffer[wpos] = buffer[rpos];
    wpos += (buffer[rpos] != '\r');
  }
  buffer[wpos] = '\0';
}


void tedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  FILE *fl;
  char *storage = (char *)d->olc;

  if (!storage)
    terminator = STRINGADD_ABORT;

  switch (terminator) {
  case STRINGADD_SAVE:
    if (!(fl = fopen(storage, "w"))) {
      sprintf(buf, "SYSERR: Can't write file '%s'.", storage);
      mudlog(buf, CMP, LVL_IMPL, TRUE);
    } else {
      if (*d->str) {
        strip_cr(*d->str);
        fputs(*d->str, fl);
      }
      fclose(fl);
      sprintf(buf, "OLC: %s saves '%s'.", GET_NAME(d->character), storage);
      mudlog(buf, CMP, LVL_GOD, TRUE);
      SEND_TO_Q("Записано.\r\n", d);
    }
    break;
  case STRINGADD_ABORT:
    SEND_TO_Q("Редактирование прервано.\r\n", d);
    act("$n прервал$y редактирование текста.", TRUE, d->character, 0, 0, TO_ROOM);
    break;
  default:
    log("SYSERR: tedit_string_cleanup: Unknown terminator status.");
    break;
  }

  /* Common cleanup code. */
  if (d->olc) {
    free(d->olc);
    d->olc = NULL;
  }
  STATE(d) = CON_PLAYING;
}

ACMD(do_tedit)
{
  int l, i;
  char field[MAX_INPUT_LENGTH];
  char *backstr = NULL;
   
 edit_files fields[] = {
        { "создатели",	LVL_IMPL,	&credits,		2400,	CREDITS_FILE},
	{ "новости",	LVL_GOD,	&news,		8192,	NEWS_FILE},
	{ "смертные",	LVL_GOD,	&motd,		2400,	MOTD_FILE},
	{ "имморталы",	LVL_GOD,	&imotd,		2400,	IMOTD_FILE},
	{ "справка",    LVL_GRGOD,	&help,		8192,	HELP_PAGE_FILE},
	{ "новичок",	LVL_GOD,	&info,		8192,	INFO_FILE},
	{ "предистория",LVL_GOD,	&background,	8192,	BACKGROUND_FILE},
	{ "руководство",LVL_IMPL,	&handbook,		8192,   HANDBOOK_FILE},
	{ "правила",	LVL_IMPL,	&policies,		8192,	POLICIES_FILE},
        { "зоны",	LVL_IMPL,	&zony,		8192,	ZONY_FILE},
	{ "\n",		0,		NULL,			0,	NULL }
  };

  if (ch->desc == NULL)
    return;
   
  half_chop(argument, field, buf);

  if (!*field) {
    strcpy(buf, "Файл для редактирования:\r\n");
    i = 1;
    for (l = 0; *fields[l].cmd != '\n'; l++) {
      if (GET_LEVEL(ch) >= fields[l].level) {
	sprintf(buf, "%s%-12.11s", buf, fields[l].cmd);
	if (!(i % 7))
	  strcat(buf, "\r\n");
	i++;
      }
    }
    if (--i % 7)
      strcat(buf, "\r\n");
    if (i == 0)
      strcat(buf, "Нет.\r\n");
    send_to_char(buf, ch);
    return;
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;
   
  if (*fields[l].cmd == '\n') {
    send_to_char("Недопустимая опция редактора.\r\n", ch);
    return;
  }
   
  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("Вы не имеете для этих действий полномочий!\r\n", ch);
    return;
  }

  /* set up editor stats */
 // clear_screen(ch->desc);
  send_editor_help(ch->desc);
  send_to_char("Редактор файлов ниже:\r\n\r\n", ch);

  if (*fields[l].buffer) {
    send_to_char(*fields[l].buffer, ch);
    backstr = str_dup(*fields[l].buffer);
  }

  ch->desc->olc = str_dup(fields[l].filename);
  string_write(ch->desc, (char **)fields[l].buffer, fields[l].size, 0, backstr);

  act("$n закончил$y редактирование.", TRUE, ch, 0, 0, TO_ROOM);
  STATE(ch->desc) = CON_TEDIT;
}
