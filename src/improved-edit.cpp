/*

improved-edit.c		Routines specific to the improved editor.

*/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "improved-edit.h"

void send_editor_help(struct descriptor_data *d)
{
  if (using_improved_editor)
    SEND_TO_Q("??????????: /s ??? @ ??? ??????????, /h ??? ?????? ??????.\r\n", d);
  else
    SEND_TO_Q("??????????: ??????????? @ ? ??????? ????.\r\n", d);
}

#if CONFIG_IMPROVED_EDITOR

int improved_editor_execute(struct descriptor_data *d, char *str)
{
  char actions[MAX_INPUT_LENGTH];

  if (*str != '/')
    return STRINGADD_OK;

  strncpy(actions, str + 2, sizeof(actions) - 1);
  actions[sizeof(actions) - 1] = '\0';
  *str = '\0';

  switch (str[1]) {
  case 'a':
    return STRINGADD_ABORT;
  case 'c':
    if (*(d->str)) {
      free(*d->str);
      *(d->str) = NULL;
      SEND_TO_Q("??????? ????? ??????.\r\n", d);
    } else
      SEND_TO_Q("??????? ????? ??????.\r\n", d);
    break;
  case 'd':
    parse_action(PARSE_DELETE, actions, d);
    break;
  case 'e':
    parse_action(PARSE_EDIT, actions, d);
    break;
  case 'f':
    if (*(d->str))
      parse_action(PARSE_FORMAT, actions, d);
    else
      SEND_TO_Q("??????? ????? ??????.\r\n", d);
    break;
  case 'i':
    if (*(d->str))
      parse_action(PARSE_INSERT, actions, d);
    else
      SEND_TO_Q("??????? ????? ??????.\r\n", d);
    break;
  case 'h':
    parse_action(PARSE_HELP, actions, d);
    break;
  case 'l':
    if (*d->str)
      parse_action(PARSE_LIST_NORM, actions, d);
    else
      SEND_TO_Q("??????? ????? ??????.\r\n", d);
    break;
  case 'n':
    if (*d->str)
      parse_action(PARSE_LIST_NUM, actions, d);
    else
      SEND_TO_Q("??????? ????? ??????.\r\n", d);
    break;
  case 'r':
    parse_action(PARSE_REPLACE, actions, d);
    break;
  case 's':
    return STRINGADD_SAVE;
  default:
    SEND_TO_Q("???????????? ?????.\r\n", d);
    break;
  }
  return STRINGADD_ACTION;
}

/*
 * Handle some editor commands.
 */
void parse_action(int command, char *string, struct descriptor_data *d)
{
  int indent = 0, rep_all = 0, flags = 0, replaced, i, line_low, line_high, j = 0;
  unsigned int total_len;
  char *s, *t, temp;

  switch (command) {
  case PARSE_HELP:
    sprintf(buf,
	    "?????? ?????? ?????????: \"/<?????>\"\r\n\r\n"
	    "/a         -  ????? ?? ?????????\r\n"
	    "/c         -  ??????? ??????\r\n"
	    "/d#        -  ??????? ?????? #\r\n"
	    "/e# <text> -  ???????? ?????? # ?? <?????>\r\n"
	    "/f         -  ?????? ??????\r\n"
	    "/fi        -  indented formatting of text\r\n"
	    "/h         -  ??????? ????????? ???????\r\n"
	    "/i# <text> -  ???????? <?????> ????? ??????? #\r\n"
	    "/l         -  ??????? ??????\r\n"
	    "/n         -  ??????? ?????? ? ???????? ?????\r\n"
	    "/r 'a' 'b' -  ???????? ?????? ????????? <a> ? ????? ?? ????? <b>\r\n"
	    "/ra 'a' 'b'-  ???????? ??? ????????? ?????? <a> ? ?????? ?? ????? <b>\r\n"
	    "              ??????: /r[a] '??????' '?? ??? ????????'\r\n"
	    "/s         -  ?????????? ??????\r\n");
    SEND_TO_Q(buf, d);
    break;
  case PARSE_FORMAT:
    while (isalpha((unsigned char)string[j]) && j < 2)
      if (string[j++] == 'i' && !indent) {
	indent = TRUE;
	flags += FORMAT_INDENT;
      }
    format_text(d->str, flags, d, d->max_str);
    sprintf(buf, "Text formatted with%s indent.\r\n", (indent ? "" : "out"));
    SEND_TO_Q(buf, d);
    break;
  case PARSE_REPLACE:
    while (isalpha((unsigned char)string[j]) && j < 2)
      if (string[j++] == 'a' && !indent)
	rep_all = 1;

    if ((s = strtok(string, "'")) == NULL) {
      SEND_TO_Q("???????????? ??????.\r\n", d);
      return;
    } else if ((s = strtok(NULL, "'")) == NULL) {
      SEND_TO_Q("?????? ?????? ???? ???????? ? ?????????.\r\n", d);
      return;
    } else if ((t = strtok(NULL, "'")) == NULL) {
      SEND_TO_Q("??? ?????? ??????.\r\n", d);
      return;
    } else if ((t = strtok(NULL, "'")) == NULL) {
      SEND_TO_Q("?????????? ?????? ?????? ???? ????????? ? ?????????.\r\n", d);
      return;
    } else if ((total_len = ((strlen(t) - strlen(s)) + strlen(*d->str))) <= d->max_str) {
      if ((replaced = replace_str(d->str, s, t, rep_all, d->max_str)) > 0) {
      	sprintf(buf, "???????? ????????? '%s' ?? '%s'.\r\n", replaced, ((replaced != 1) ? "s " : " ") ); //, s, t);
	      SEND_TO_Q(buf, d);
    } else if (replaced == 0) {
	sprintf(buf, "?????? '%s' ?? ??????.\r\n", s);
	SEND_TO_Q(buf, d);
      } else
	SEND_TO_Q("??????: ??? ??????? ?????? ????? ?????????? - ????????.\r\n", d);
    } else
      SEND_TO_Q("??? ?????????? ????? ??? ?????????? ???????.\r\n", d);
    break;
  case PARSE_DELETE:
    switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
    case 0:
      SEND_TO_Q("?? ?????? ?????????? ????? ?????? ??? ????????, ??? ?? ???????.\r\n", d);
      return;
    case 1:
      line_high = line_low;
      break;
    case 2:
      if (line_high < line_low) {
	SEND_TO_Q("???? ???????? ??????????.\r\n", d);
	return;
      }
      break;
    }

    i = 1;
    total_len = 1;
    if ((s = *d->str) == NULL) {
      SEND_TO_Q("????? ????.\r\n", d);
      return;
    } else if (line_low > 0) {
      while (s && i < line_low)
	if ((s = strchr(s, '\n')) != NULL) {
	  i++;
	  s++;
	}
      if (s == NULL || i < line_low) {
	SEND_TO_Q("??????(?) ??? ????????? - ???????????????.\r\n", d);
	return;
      }
      t = s;
      while (s && i < line_high)
	if ((s = strchr(s, '\n')) != NULL) {
	  i++;
	  total_len++;
	  s++;
	}
      if (s && (s = strchr(s, '\n')) != NULL) {
	while (*(++s))
	  *(t++) = *s;
      } else
	total_len--;
      *t = '\0';
      RECREATE(*d->str, char, strlen(*d->str) + 3);

      sprintf(buf, "%d line%sdeleted.\r\n", total_len, (total_len != 1 ? "s " : " "));
      SEND_TO_Q(buf, d);
    } else {
      SEND_TO_Q("????????????? ??? ??????? ????? ?????? ??? ????????.\r\n", d);
      return;
    }
    break;
  case PARSE_LIST_NORM:
    /*
     * Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so
     * they are probly ok for what to do here. 
     */
    *buf = '\0';
    if (*string)
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
      case 0:
	line_low = 1;
	line_high = 999999;
	break;
      case 1:
	line_high = line_low;
	break;
    } else {
      line_low = 1;
      line_high = 999999;
    }

    if (line_low < 1) {
      SEND_TO_Q("????? ?????? ?????? ???? ?????? ??? 0.\r\n", d);
      return;
    } else if (line_high < line_low) {
      SEND_TO_Q("???? ???????? ??????????.\r\n", d);
      return;
    }
    *buf = '\0';
    if (line_high < 999999 || line_low > 1)
      sprintf(buf, "Current buffer range [%d - %d]:\r\n", line_low, line_high);
    i = 1;
    total_len = 0;
    s = *d->str;
    while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL) {
	i++;
	s++;
      }
    if (i < line_low || s == NULL) {
      SEND_TO_Q("Line(s) out of range; no buffer listing.\r\n", d);
      return;
    }
    t = s;
    while (s && i <= line_high)
      if ((s = strchr(s, '\n')) != NULL) {
	i++;
	total_len++;
	s++;
      }
    if (s) {
      temp = *s;
      *s = '\0';
      strcat(buf, t);
      *s = temp;
    } else
      strcat(buf, t);
    /*
     * This is kind of annoying...but some people like it.
     */
    //sprintf(buf + strlen(buf), "\r\n%d line%sshown.\r\n", total_len, (total_len != 1) ? "s " : " "); 
    //send_to_char(buf, d->character);
	page_string(d, buf, TRUE);
    break;
  case PARSE_LIST_NUM:
    /*
     * Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so
     * they are probly ok for what to do here. 
     */
    *buf = '\0';
    if (*string)
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
      case 0:
	line_low = 1;
	line_high = 999999;
	break;
      case 1:
	line_high = line_low;
	break;
    } else {
      line_low = 1;
      line_high = 999999;
    }

    if (line_low < 1) {
      SEND_TO_Q("????? ?????? ?????? ???? ????? 0.\r\n", d);
      return;
    }
    if (line_high < line_low) {
      SEND_TO_Q("???? ???????? ??????????.\r\n", d);
      return;
    }
    *buf = '\0';
    i = 1;
    total_len = 0;
    s = *d->str;
    while (s && i < line_low)
      if ((s = strchr(s, '\n')) != NULL) {
	i++;
	s++;
      }
    if (i < line_low || s == NULL) {
      SEND_TO_Q("??????(?) ??? ????????? - ???????????????.\r\n", d);
      return;
    }
    t = s;
    while (s && i <= line_high)
      if ((s = strchr(s, '\n')) != NULL) {
	i++;
	total_len++;
	s++;
	temp = *s;
	*s = '\0';
	sprintf(buf, "%s%4d:\r\n", buf, (i - 1));
	strcat(buf, t);
	*s = temp;
	t = s;
      }
    if (s && t) {
      temp = *s;
      *s = '\0';
      strcat(buf, t);
      *s = temp;
    } else if (t)
      strcat(buf, t);

    //send_to_char(buf, d->character);
	page_string(d, buf, TRUE);
    break;

  case PARSE_INSERT:
    half_chop(string, buf, buf2);
    if (*buf == '\0') {
      SEND_TO_Q("?? ?????? ??????? ????? ??????, ????? ??????? ???????? ?????.\r\n", d);
      return;
    }
    line_low = atoi(buf);
    strcat(buf2, "\r\n");

    i = 1;
    *buf = '\0';
    if ((s = *d->str) == NULL) {
      SEND_TO_Q("????? ???? - ?????? ?? ?????????.\r\n", d);
      return;
    }
    if (line_low > 0) {
      while (s && (i < line_low))
	if ((s = strchr(s, '\n')) != NULL) {
	  i++;
	  s++;
	}
      if (i < line_low || s == NULL) {
	SEND_TO_Q("????? ?????? ??? ????????? - ????????.\r\n", d);
	return;
      }
      temp = *s;
      *s = '\0';
      if ((strlen(*d->str) + strlen(buf2) + strlen(s + 1) + 3) > d->max_str) {
	*s = temp;
	SEND_TO_Q("??????????? ????? ????????? ???????? ?????? - ????????.\r\n", d);
	return;
      }
      if (*d->str && **d->str)
	strcat(buf, *d->str);
      *s = temp;
      strcat(buf, buf2);
      if (s && *s)
	strcat(buf, s);
      RECREATE(*d->str, char, strlen(buf) + 3);

      strcpy(*d->str, buf);
      SEND_TO_Q("?????? ?????????.\r\n", d);
    } else {
      SEND_TO_Q("????? ?????? ?????? ???? ?????? 0.\r\n", d);
      return;
    }
    break;

  case PARSE_EDIT:
    half_chop(string, buf, buf2);
    if (*buf == '\0') {
      SEND_TO_Q("?? ?????? ??????? ????? ?????? ??????????? ??????.\r\n", d);
      return;
    }
    line_low = atoi(buf);
    strcat(buf2, "\r\n");

    i = 1;
    *buf = '\0';
    if ((s = *d->str) == NULL) {
      SEND_TO_Q("????? ?????, ???????? ?? ???????.\r\n", d);
      return;
    }
    if (line_low > 0) {
      /*
       * Loop through the text counting \n characters until we get to the line.
       */
      while (s && i < line_low)
	if ((s = strchr(s, '\n')) != NULL) {
	  i++;
	  s++;
	}
      /*
       * Make sure that there was a THAT line in the text.
       */
      if (s == NULL || i < line_low) {
	SEND_TO_Q("?????? ??? ????????? - ????????.\r\n", d);
	return;
      }
      /*
       * If s is the same as *d->str that means I'm at the beginning of the
       * message text and I don't need to put that into the changed buffer.
       */
      if (s != *d->str) {
	/*
	 * First things first .. we get this part into the buffer.
	 */
	temp = *s;
	*s = '\0';
	/*
	 * Put the first 'good' half of the text into storage.
	 */
	strcat(buf, *d->str);
	*s = temp;
      }
      /*
       * Put the new 'good' line into place.
       */
      strcat(buf, buf2);
      if ((s = strchr(s, '\n')) != NULL) {
	/*
	 * This means that we are at the END of the line, we want out of
	 * there, but we want s to point to the beginning of the line
	 * AFTER the line we want edited 
	 */
	s++;
	/*
	 * Now put the last 'good' half of buffer into storage.
	 */
	strcat(buf, s);
      }
      /*
       * Check for buffer overflow.
       */
      if (strlen(buf) > d->max_str) {
	SEND_TO_Q("?????????? ????????????? ??????? ?????? - ????????.\r\n", d);
	return;
      }
      /*
       * Change the size of the REAL buffer to fit the new text.
       */
      RECREATE(*d->str, char, strlen(buf) + 3);
      strcpy(*d->str, buf);
      SEND_TO_Q("?????? ????????.\r\n", d);
    } else {
      SEND_TO_Q("????? ?????? ?????? ???? ?????? 0.\r\n", d);
      return;
    }
    break;
  default:
    SEND_TO_Q("???????? ????????.\r\n", d);
    mudlog("SYSERR: invalid command passed to parse_action", BRF, LVL_IMPL, TRUE);
    return;
  }
}


/*
 * Re-formats message type formatted char *.
 * (for strings edited with d->str) (mostly olc and mail)
 */
void format_text(char **ptr_string, int mode, struct descriptor_data *d, unsigned int maxlen)
{
  int line_chars, cap_next = TRUE, cap_next_next = FALSE;
  char *flow, *start = NULL, temp;
  char formatted[MAX_STRING_LENGTH];
   
  /* Fix memory overrun. */
  if (d->max_str > MAX_STRING_LENGTH) {
    log("SYSERR: format_text: max_str is greater than buffer size.");
    return;
  }

  /* XXX: Want to make sure the string doesn't grow either... */

  if ((flow = *ptr_string) == NULL)
    return;

  if (IS_SET(mode, FORMAT_INDENT)) {
    strcpy(formatted, "   ");
    line_chars = 3;
  } else {
    *formatted = '\0';
    line_chars = 0;
  } 

  while (*flow) {
    while (*flow && strchr("\n\r\f\t\v ", *flow))
      flow++;

    if (*flow) {
      start = flow++;
      while (*flow && !strchr("\n\r\f\t\v .?!", *flow))
	flow++;

      if (cap_next_next) {
        cap_next_next = FALSE;
        cap_next = TRUE;
      }

      /*
       * This is so that if we stopped on a sentence .. we move off the
       * sentence delimiter.
       */
      while (strchr(".!?", *flow)) {
	cap_next_next = TRUE;
	flow++;
      }
	 
      temp = *flow;
      *flow = '\0';

      if (line_chars + strlen(start) + 1 > 79) {
	strcat(formatted, "\r\n");
	line_chars = 0;
      }

      if (!cap_next) {
	if (line_chars > 0) {
	  strcat(formatted, " ");
	  line_chars++;
	}
      } else {
	cap_next = FALSE;
	*start = UPPER(*start);
      }

      line_chars += strlen(start);
      strcat(formatted, start);

      *flow = temp;
    }

    if (cap_next_next) {
      if (line_chars + 3 > 79) {
	strcat(formatted, "\r\n");
	line_chars = 0;
      } else {
	strcat(formatted, "  ");
	line_chars += 2;
      }
    }
  }
  strcat(formatted, "\r\n");

  if (strlen(formatted) + 1 > maxlen)
    formatted[maxlen - 1] = '\0';
  RECREATE(*ptr_string, char, MIN(maxlen, strlen(formatted) + 1));
  strcpy(*ptr_string, formatted);
}

int replace_str(char **string, char *pattern, char *replacement, int rep_all, unsigned int max_size)
{
  char *replace_buffer = NULL;
  char *flow, *jetsam, temp;
  int len, i;

  if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
    return -1;

  CREATE(replace_buffer, char, max_size);
  i = 0;
  jetsam = *string;
  flow = *string;
  *replace_buffer = '\0';

  if (rep_all) {
    while ((flow = (char *)strstr(flow, pattern)) != NULL) {
      i++;
      temp = *flow;
      *flow = '\0';
      if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size) {
        i = -1;
        break;
      }
      strcat(replace_buffer, jetsam);
      strcat(replace_buffer, replacement);
      *flow = temp;
      flow += strlen(pattern);
      jetsam = flow;
    }
    strcat(replace_buffer, jetsam);
  } else {
    if ((flow = (char *)strstr(*string, pattern)) != NULL) {
      i++;
      flow += strlen(pattern);
      len = ((char *)flow - (char *)*string) - strlen(pattern);
      strncpy(replace_buffer, *string, len);
      strcat(replace_buffer, replacement);
      strcat(replace_buffer, flow);
    }
  }

  if (i <= 0)
    return 0;
  else {
    RECREATE(*string, char, strlen(replace_buffer) + 3);
    strcpy(*string, replace_buffer);
  }
  free(replace_buffer);
  return i;
}

#endif
