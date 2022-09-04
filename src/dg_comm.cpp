#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;

/* same as any_one_arg except that it stops at punctuation */
char *any_one_name(char *argument, char *first_arg)
{
    char* arg;

    /* Find first non blank */
    while(is_space(*argument))
          argument++; 
 
    /* Find length of first word */
    for(arg = first_arg ;
        *argument && !is_space(*argument) &&
          (!ispunct(*argument) || *argument == '#' || *argument == '-') ;
        arg++, argument++)
        *arg = LOWER(*argument);
    *arg = '\0';

    return argument;
}


void sub_write_to_char(char_data *ch, char *tokens[],
		       void *otokens[], char type[])
{
    char sb[MAX_STRING_LENGTH];
    int i;

    strcpy(sb,"");

    for (i = 0; tokens[i + 1]; i++)
    {strcat(sb,tokens[i]);
	 switch (type[i])
	   {
	case '~':
	    if (!otokens[i])
		    strcat(sb,"���-��");
	    else 
	    if ((char_data *)otokens[i] == ch)
		    strcat(sb,"��");
	    else
		    strcat(sb,PERS((char_data *)otokens[i], ch));
	    break;

	case '@':
	    if (!otokens[i])
		   strcat(sb,"���-��");
	    else 
	    if ((char_data *)otokens[i] == ch)
		   strcat(sb,"���");
	    else
	       {strcat(sb,RPERS((char_data *) otokens[i], ch));
	       }
	    break;

	case '^':
	    if (!otokens[i] || !CAN_SEE(ch, (char_data *) otokens[i]))
		   strcat(sb,"���-��");
	    else 
	    if (otokens[i] == ch)
		   strcat(sb,"���");
	    else
		   strcat(sb,HSHR((char_data *) otokens[i]));
	    break;
		
	case '&':
	    if (!otokens[i] || !CAN_SEE(ch, (char_data *) otokens[i]))
		   strcat(sb,"��");
	    else 
	    if (otokens[i] == ch)
		   strcat(sb,"��");
	    else
		   strcat(sb,HSSH((char_data *) otokens[i]));
	    break;
	    
	case '*':
	    if (!otokens[i] || !CAN_SEE(ch, (char_data *) otokens[i]))
		   strcat(sb,"���");
	    else 
	    if (otokens[i] == ch)
		   strcat(sb,"���");
	    else
		   strcat(sb,HMHR((char_data *) otokens[i]));
	    break;

	case '`':
	    if (!otokens[i])
		   strcat(sb,"���-��");
	    else
		   strcat(sb,OBJS(((obj_data *) otokens[i]), ch));
	    break;
	   }
    }

    strcat(sb,tokens[i]);
    strcat(sb,"\n\r");
    sb[0] = toupper(sb[0]);
    send_to_char(sb,ch);
}


void sub_write(char *arg, char_data *ch, byte find_invis, int targets)
{
    }

void send_to_zone(char *messg, int zone_rnum)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
     return;

  for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i->character && AWAKE(i->character) &&
          (IN_ROOM(i->character) != NOWHERE) &&
          (world[IN_ROOM(i->character)].zone == zone_rnum))
         SEND_TO_Q(messg, i);
}
