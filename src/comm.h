/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define NUM_RESERVED_DESCS	8

/* comm.c */
void	send_to_all(const char *messg);
void	send_to_char(const char *messg, struct char_data *ch);
void    send_to_char(CHAR_DATA * ch, const char *messg, ...);
void    send_to_char(const std::string & buffer, CHAR_DATA * ch);
void	send_to_room(const char *messg, room_rnum room, int to_awake);
//void	send_to_outdoor(const char *messg);
void	perform_to_all(const char *messg, struct char_data *ch);
void	close_socket(struct descriptor_data *d, int direct);
void	send_to_gods(const char *messg);
void	perform_act(const char *orig, struct char_data *ch,
		struct obj_data *obj, const void *vict_obj, struct char_data *to);
void	send_to_outdoor(const char *messg, int control);
void	act(const char *str, int hide_invisible, struct char_data *ch,
		struct obj_data *obj, const void *vict_obj, int type);

unsigned long get_ip(const char *addr);

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define TO_SLEEP	128	/* to char, even if sleeping */
#define TO_ROOM_HIDE    5 /* В комнату, но только тем, кто чувствует жизнь */
/* I/O functions */
int		write_to_descriptor(socket_t desc, const char *txt, size_t total);
int		toggle_compression(struct descriptor_data *d);
void	write_to_q(const char *txt, struct txt_q *queue, int aliased);
void	write_to_output(const char *txt, struct descriptor_data *d);
void	page_string(struct descriptor_data *d, char *str, int keep_internal);
void	string_add(struct descriptor_data *d, char *str);
void	string_write(struct descriptor_data *d, char **txt, size_t len, long mailto, void *data);

#define SEND_TO_Q(messg, desc)  write_to_output((messg), desc)
#define SEND_TO_SOCKET(messg, desc)	write_to_descriptor((desc), (messg), strlen(messg))

#define USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  ((d)->output == (d)->large_outbuf)

typedef RETSIGTYPE sigfunc(int);
#define CHECK_DEAF      64
#define WEATHER_CONTROL (1 << 1)
#define SUN_CONTROL     (0 << 1)
#define		ERRLOG		1
