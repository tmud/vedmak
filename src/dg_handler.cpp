#include "conf.h"
#include "sysdep.h"
    

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "dg_event.h"

extern struct index_data **trig_index;
extern struct trig_data *trigger_list;

void trig_data_free(trig_data *this_data);

/* return memory used by a trigger */
void free_trigger(struct trig_data *trig)
{
  /* threw this in for minor consistance in names with the rest of circle */
  trig_data_free(trig);
}


/* remove a single trigger from a mob/obj/room */
void extract_trigger(struct trig_data *trig)
{
  struct trig_data *temp;

  if (GET_TRIG_WAIT(trig)) 
     {free(GET_TRIG_WAIT(trig)->info);
      remove_event(GET_TRIG_WAIT(trig));
      GET_TRIG_WAIT(trig) = NULL;
     }

  trig_index[trig->nr]->number--; 

  /* walk the trigger list and remove this one */
  REMOVE_FROM_LIST(trig, trigger_list, next_in_world);

  free_trigger(trig);
}

/* remove all triggers from a mob/obj/room */
void extract_script(struct script_data *sc)
{
  struct trig_data *trig, *next_trig;

  for (trig = TRIGGERS(sc); trig; trig = next_trig) 
      {next_trig = trig->next;
       extract_trigger(trig);
      }
  TRIGGERS(sc) = NULL;
}

/* erase the script memory of a mob */
void extract_script_mem(struct script_memory *sc)
{
  struct script_memory *next;
  while (sc) 
        {next = sc->next;
         if (sc->cmd) 
            free(sc->cmd);
         free(sc);
         sc = next;
        }
}

/* perhaps not the best place for this, but I didn't want a new file */
char *skill_percent(struct char_data *ch, char *skill)
{
  static char retval[256];
  int skillnum;

  skillnum = find_skill_num(skill);
  if (skillnum <= 0) return("-1");

  sprintf(retval,"%d",GET_SKILL(ch, skillnum));
  return retval;
}

char *spell_count(struct char_data *ch, char *spell)
{
  static char retval[256];
  int spellnum;

  spellnum = find_spell_num(spell);
  if (spellnum <= 0) return("-1");

  if (GET_SPELL_MEM(ch,spellnum))
     sprintf(retval,"%d",GET_SPELL_MEM(ch, spellnum));
  else
     strcpy(retval, "");   
  return retval;
}

char *spell_knowledge(struct char_data *ch, char *spell)
{
  static char retval[256];
  int spellnum;

  spellnum = find_spell_num(spell);
  if (spellnum <= 0) return("-1");

  if (GET_SPELL_TYPE(ch,spellnum))
     sprintf(retval,"%d",GET_SPELL_TYPE(ch, spellnum));
  else
     strcpy(retval, "");   
  return retval;
}
