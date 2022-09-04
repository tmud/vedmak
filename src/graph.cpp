/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"


extern struct char_data *character_list;
//extern const char *dirs1[];
extern const char *dirs2[];
extern struct room_data *world;
extern int track_through_doors;
//extern int through_doors;
extern int top_of_p_table;
extern struct player_index_element *player_table;

/* local functions */
int  attack_best(struct char_data *ch, struct char_data *victim);
void bfs_enqueue(room_rnum room, int dir);
void bfs_dequeue(void);
void bfs_clear_queue(void);
int find_first_step(room_rnum src, room_rnum target, struct char_data *ch);

/* Externals */
ACMD(do_say);
ACMD(do_track);
ACMD(do_sense);
void hunt_victim(struct char_data * ch);

extern struct char_data *mob_proto;

struct bfs_queue_struct {
  room_rnum room;
  char dir;
  struct bfs_queue_struct *next;
};

static struct bfs_queue_struct *queue_head = NULL,
                               *queue_tail = NULL,
			       *room_queue = NULL;


#define EDGE_ZONE   1
#define EDGE_WORLD  2

/* Utility macros */
#define MARK(room)	   (SET_BIT(ROOM_FLAGS(room, ROOM_BFS_MARK), ROOM_BFS_MARK))
#define UNMARK(room)	(REMOVE_BIT(ROOM_FLAGS(room, ROOM_BFS_MARK), ROOM_BFS_MARK))
#define IS_MARKED(room)	(ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y)	(world[(x)].dir_option[(y)]->to_room)
#define IS_CLOSED(x, y)	(EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED))

int VALID_EDGE(room_rnum x, int y, int edge_range, int through_doors)
{
  if (world[x].dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
    return 0;

  if (edge_range == EDGE_ZONE && (world[x].zone != world[TOROOM (x, y)].zone))
    return 0;

  if (through_doors == FALSE && IS_CLOSED(x, y))
    return 0;

  if (ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK) || IS_MARKED(TOROOM(x, y))) 
    return 0;

  return 1;
}

void bfs_enqueue(room_rnum room, int dir)
{
  struct bfs_queue_struct *curr;

  CREATE(curr, struct bfs_queue_struct, 1);
  curr->room = room;
  curr->dir = dir;
  curr->next = 0;

  if (queue_tail) {
    queue_tail->next = curr;
    queue_tail = curr;
  } else
    queue_head = queue_tail = curr;
}


void bfs_dequeue(void)
{
  struct bfs_queue_struct *curr;

  curr = queue_head;

  if (!(queue_head = queue_head->next))
    queue_tail = 0;
  free(curr);
}


void bfs_clear_queue(void)
{
  while (queue_head)
    bfs_dequeue();
}

void room_enqueue(room_rnum room)
{
   struct bfs_queue_struct *curr;

   CREATE( curr, struct bfs_queue_struct, 1 );
   curr->room = room;
   curr->next = room_queue;
   room_queue = curr;
}

void clean_room_queue(void) 
{
   struct bfs_queue_struct *curr, *curr_next;

   for (curr = room_queue; curr; curr = curr_next )
   {
      UNMARK( curr->room );
      curr_next = curr->next;
      free( curr );
   }
   room_queue = NULL;
}


/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */

int real_zone (int number);
int find_first_step(room_rnum src, room_rnum target, struct char_data *ch)
{  int curr_dir, edge, through_doors;//, zone_nr;
   room_rnum curr_room = 0, rnum_start = FIRST_ROOM, rnum_stop = top_of_world;//тут рнум_старт поставил 1, в родном случае 0, тест

  if (src < FIRST_ROOM || src > top_of_world || target < FIRST_ROOM
      || target > top_of_world)
    {
      log ("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)",
	   src, target, __FILE__);
      return (BFS_ERROR);
    }



  if (src == target)
     return (BFS_ALREADY_THERE);
 

   if (IS_NPC (ch))
    {  
      if (world[src].zone != world[target].zone)// может и стоить разрешить искать в других
		                                        //зонах если моб не стоит в своей && MOB_FLAGGED(ch, MOB_STAY_ZONE))
	return (BFS_ERROR);

     // Запрещаем мобам искать через двери ...
      through_doors = FALSE;
      edge = EDGE_ZONE;
    }
  else
    {
      // Игроки полноценно ищут в мире.
      through_doors = TRUE;
      edge = EDGE_WORLD;
    }
 
  room_enqueue( src );
  MARK( src );
	  
  // first, enqueue the first steps, saving which direction we're going.
  for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)//изменил
      if (VALID_EDGE (src, curr_dir, edge, through_doors))
		{ MARK(TOROOM(src, curr_dir));
		  room_enqueue(TOROOM(src, curr_dir));
          bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
        }

  // now, do the classic BFS.
  while (queue_head) 
		{ if (queue_head->room == target) 
            	   { curr_dir = queue_head->dir;
                     bfs_clear_queue();
		     clean_room_queue();//добавил
                     return (curr_dir);
            	   } 
         	else 
            	{ for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
	            if (VALID_EDGE (queue_head->room, curr_dir, edge, through_doors))
			{ MARK(TOROOM(queue_head->room, curr_dir));
			  room_enqueue(TOROOM(queue_head->room, curr_dir));
			  bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
	            	}
             	bfs_dequeue();
            	}
          }
	clean_room_queue();

  return (BFS_NO_PATH);
}


/********************************************************
* Functions and Commands which use the above functions. *
********************************************************/
int go_track(struct char_data *ch, struct char_data *victim, int skill_no)
{int percent, dir;

 if (AFF_FLAGGED(victim, AFF_NOTRACK)) 
    {return BFS_ERROR;
    }

  /* 101 is a complete failure, no matter what the proficiency. */
  percent = number(0,skill_info[skill_no].max_percent);
  
  if (percent > calculate_skill(ch, skill_no, skill_info[skill_no].max_percent, victim)) 
     {int tries = 10;
      /* Find a random direction. :) */
      do {dir = number(0, NUM_OF_DIRS - 1);
         } while (!CAN_GO(ch, dir) && --tries);
      return dir;   
     }

  /* They passed the skill check. */
  return find_first_step(ch->in_room, victim->in_room, ch);
}

/********************************************************
* Functions and Commands which use the above functions. *
********************************************************/
ACMD(do_sense)
{
  struct char_data *vict;
  int dir;

  /* The character must have the track skill. */
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SENSE)) {
    send_to_char("Вы и понятия не имеете как это делать.\r\n", ch);
    return;
  }
 
  
   if (!check_moves(ch, SENSE_MOVES))
     return;
  
  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Кого Вы хотите найти?\r\n", ch);
    return;
  }
  /* The person can't see the victim. */
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) {
    send_to_char("Нет никого с таким именем.\r\n", ch);
    return;
  }
  /* We can't track the victim. */
  if (AFF_FLAGGED(vict, AFF_NOTRACK)) {
    send_to_char("Вы не можете отсюда выследить.\r\n", ch);
    return;
  }
   
  dir = go_track(ch, vict, SKILL_SENSE);
  switch (dir) {
  case BFS_ERROR:
    send_to_char("Хмм... Кажется что-то не так.\r\n", ch); 
    break;
  case BFS_ALREADY_THERE:
    send_to_char("Вы уже находитесь в одном месте!!!\r\n", ch); 
    break;
  case BFS_NO_PATH:
    sprintf(buf, "Вы не можете выследить %s от сюда.\r\n", HSHR(vict));
    send_to_char(buf, ch);
    break;
  default:	/* Success! */
    improove_skill(ch,SKILL_SENSE,TRUE,vict);
	sprintf(buf, "Вы видите след, который ведет %s!\r\n", dirs2[dir]);
    send_to_char(buf, ch);
    break;
  }
}
const char *track_when[] =
{"совсем свежие",
 "свежие",
 "менее полудневной давности",
 "примерно полудневной давности",
 "почти дневной давности",
 "примерно дневной давности",
 "совсем старые"
}; 

#define CALC_TRACK(ch,vict) (calculate_skill(ch,SKILL_TRACK,skill_info[SKILL_TRACK].max_percent,0))

int age_track(struct char_data *ch, int time, int calc_track)
{int when = 0;

 if (calc_track >= number(1,50))
    {if (time & 0x03)   /* 2 */
        when = 0;
     else
     if (time & 0x1F)   /* 5 */
        when = 1;
     else
     if (time & 0x3FF)  /* 10 */
        when = 2;
     else
     if (time & 0x7FFF)  /* 15 */
        when = 3;
     else
     if (time & 0xFFFFF) /* 20 */
        when = 4;
     else
     if (time & 0x3FFFFFF) /* 26 */
        when = 5;
     else
        when = 6;   
    }
 else
    when = number(0,6);   
 return (when);
}    
 

ACMD(do_track)
{ struct char_data  *vict = NULL;
  struct track_data *track;
  int    found = FALSE, c, calc_track = 0,track_t,i;
  char   name[MAX_INPUT_LENGTH];
  *name = '\0';

  /* The character must have the track skill. */
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRACK)) 
     {send_to_char("Но Вы не знаете как.\r\n", ch);
      return;
     }
  
   if (AFF_FLAGGED(ch, AFF_BLIND))
     {send_to_char("Вам еще и видеть не мешало бы!\r\n", ch);
      return;
     }
  
  if (!check_moves(ch, TRACK_MOVES))
     return;
  
  calc_track = CALC_TRACK(ch,NULL);
  act("Похоже, $n кого-то выслеживает.",FALSE,ch,0,0,TO_ROOM);
  one_argument(argument, arg);
  
  /* No argument - show all */
  if (!*arg) 
     {for (track = world[IN_ROOM(ch)].track; track; track = track->next)
          {*name = '\0';
           if (IS_SET(track->track_info, TRACK_NPC))
              strcpy(name,GET_NAME(mob_proto + track->who));
           else
              for (c = 0; c <= top_of_p_table; c++)
                  {if (player_table[c].id == track->who)
                      {strcpy(name,player_table[c].name);
                       break;
                      }
                  }    
                     
           if (*name && calc_track > number(1,40))
              {CAP(name);
               for (track_t = i = 0; i < NUM_OF_DIRS; i++)
                   {track_t |= track->time_outgone[i];
                    track_t |= track->time_income[i];
                   }
               sprintf(buf,"%s : следы %s.\r\n",name,track_when[age_track(ch,track_t,calc_track)]);//CAP(name) добавил 15.06.2002г.
               send_to_char(buf,ch);
               found = TRUE;
              } 
          } 
    
    if (found)
        improove_skill(ch, SKILL_TRACK, TRUE, 0);
	
    if (!found)
         send_to_char("Вы не видите ничьих следов.\r\n", ch);
      return;
     }
             
  if (vict = get_char_vis(ch,arg,FIND_CHAR_ROOM))
     {if (ch != vict)
	      act("Вы же в одной комнате с $T!",FALSE,ch,0,vict,TO_CHAR);
      else  
		  act("Себя выслеживать? А зачем?",FALSE,ch,0,vict,TO_CHAR);
  return;
     }
  
  /* found victim */
  for (track = world[IN_ROOM(ch)].track; track; track = track->next)
      {*name = '\0';
       if (IS_SET(track->track_info, TRACK_NPC))
          strcpy(name,GET_NAME(mob_proto + track->who));
       else
          for (c = 0; c <= top_of_p_table; c++)
              if (player_table[c].id == track->who)
                 {strcpy(name,player_table[c].name);
                  break;
                 }
       if (*name && isname(arg,name))
          break;
       else
          *name = '\0';
      }
          
 if (calc_track < number(1,40) || !*name || ROOM_FLAGGED(IN_ROOM(ch),ROOM_NOTRACK))
    {send_to_char("Вы не видите похожих следов.\r\n", ch);
     return;
    }
     
 ch->track_dirs       = 0; 
 CAP(name);
 sprintf(buf,"%s:\r\n",name);
 
 for (c = 0; c < NUM_OF_DIRS; c++)
     {if ((track  && track->time_income[c] && calc_track >= number(0,skill_info[SKILL_TRACK].max_percent)) ||
          (!track && calc_track  < number(0,skill_info[SKILL_TRACK].max_percent))
         )
         {found = TRUE;
          sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n", 
	          track_when[age_track(ch,track ? track->time_income[c] : (1 << number(0,25)),calc_track)],
		  dirs1[Reverse[c]]
		 );
         }
      if ((track  && track->time_outgone[c] && calc_track >= number(0,skill_info[SKILL_TRACK].max_percent)) ||
          (!track && calc_track  < number(0,skill_info[SKILL_TRACK].max_percent))
         )
         {found = TRUE;
          SET_BIT(ch->track_dirs, 1 << c);
          sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n", 
	          track_when[age_track(ch,track ? track->time_outgone[c] : (1 << number(0,25)),calc_track)],
		  dirs2[c]
		 );
         }
     }
 
 if (found)
     improove_skill(ch, SKILL_TRACK, TRUE, 0);
 if (!found)
    {sprintf(buf,"След неожиданно оборвался.\r\n");
    }
 send_to_char(buf, ch);   
 return;    
}



void hunt_victim(struct char_data * ch)
{
  int dir;
  bool found;
  struct char_data *tmp;

  if (!ch || !HUNTING(ch) || FIGHTING(ch))
     return;

  /* make sure the char still exists */
  for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
      if (HUNTING(ch) == tmp && IN_ROOM(tmp) != NOWHERE)
         found = TRUE;

  if (!found) 
     {do_say(ch, "О, Боже! Моя добыча ускользнула!", 0, 0);
      HUNTING(ch) = NULL;
      return;
     }
     
  if ((dir = find_first_step(ch->in_room, HUNTING(ch)->in_room, ch)) < 0) 
     {sprintf(buf, "Проклятье, %s сбежал%s!", 
                   HMHR(HUNTING(ch)), GET_CH_SUF_1(HUNTING(ch)));
      do_say(ch, buf, 0, 0);
      HUNTING(ch) = NULL;
     } 
  else 
     {perform_move(ch, dir, 1, FALSE);
      if (ch->in_room == HUNTING(ch)->in_room)
         attack_best(ch, HUNTING(ch));
     }
}

ACMD(do_hidetrack)
{struct track_data *track[NUM_OF_DIRS+1], *temp;
 int    percent, prob, i, croom, found = FALSE, dir, rdir;

 if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDETRACK))
    {send_to_char("Но Вы не знаете как.\r\n", ch);
     return;
    }
    
 croom = IN_ROOM(ch);

 for (dir = 0; dir < NUM_OF_DIRS; dir++)
     {track[dir] = NULL;
      rdir       = Reverse[dir];
      if (EXITDATA(croom,dir)                         && 
          EXITDATA(EXITDATA(croom,dir)->to_room,rdir) &&
          EXITDATA(EXITDATA(croom,dir)->to_room,rdir)->to_room == croom)
         {for (temp = world[EXITDATA(croom,dir)->to_room].track; temp; temp = temp->next)
              if (!IS_SET(temp->track_info, TRACK_NPC) && 
                  GET_IDNUM(ch) == temp->who           &&
                  !IS_SET(temp->track_info, TRACK_HIDE)&&
                  IS_SET(temp->time_outgone[rdir], 3)
                 )
                 {found      = TRUE;
                  track[dir] = temp;
                  break;
                 }
         }
     }

 track[NUM_OF_DIRS] = NULL;
 for (temp = world[IN_ROOM(ch)].track; temp; temp = temp->next)
     if (!IS_SET(temp->track_info, TRACK_NPC) && 
         GET_IDNUM(ch) == temp->who           &&
         !IS_SET(temp->track_info, TRACK_HIDE)        
        )
        {found              = TRUE;
         track[NUM_OF_DIRS] = temp;
         break;
        }
 
 if (!found)
    {send_to_char("Вы не видите своих следов.\r\n",ch);
     return;
    }
 if (!check_moves(ch,HIDETRACK_MOVES))
    return;
 percent = number(1,skill_info[SKILL_HIDETRACK].max_percent);
 prob    = calculate_skill(ch,SKILL_HIDETRACK,skill_info[SKILL_HIDETRACK].max_percent,0);
 if (percent > prob)
    {send_to_char("Вы безуспешно попытались замести свои следы.\r\n",ch);
     if (!number(0,25-timed_by_skill(ch,SKILL_HIDETRACK) ? 0 : 15))
        improove_skill(ch,SKILL_HIDETRACK,FALSE,0);
    }
 else
    {send_to_char("Вы успешно замели свои следы.\r\n",ch);
     if (!number(0,25-timed_by_skill(ch,SKILL_HIDETRACK) ? 0 : 15))
        improove_skill(ch,SKILL_HIDETRACK,TRUE,0);
     prob -= percent;
     for (i = 0; i <= NUM_OF_DIRS; i++)
         if (track[i])
            {if (i < NUM_OF_DIRS)
               track[i]->time_outgone[Reverse[i]] <<= MIN(31,prob);
             else
                for (rdir = 0; rdir < NUM_OF_DIRS; rdir++)
                    {track[i]->time_income[rdir]  <<= MIN(31,prob);
                     track[i]->time_outgone[rdir] <<= MIN(31,prob);
                    }
	   
            }
    }

 for (i = 0; i <= NUM_OF_DIRS; i++)
     if (track[i])
        SET_BIT(track[i]->track_info, TRACK_HIDE);
}


