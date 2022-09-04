/* ************************************************************************
*   File: mail.h                                        Part of CircleMUD *
*  Usage: header file for mail system                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define MIN_MAIL_LEVEL 5   // minimum level a player must be to send mail
#define STAMP_PRICE 10     // # of gold coins required to send mail
#define MAX_MAIL_SIZE 4096 // Maximum size of mail in bytes (arbitrary)

int	 has_mail(CHAR_DATA *ch);
void send_mail(long to_id, CHAR_DATA* from, char *message);
