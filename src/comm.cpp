/* ************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __COMM_C__

#include "conf.h"
#include "sysdep.h"

#ifdef CIRCLE_MACINTOSH		// Includes for the Macintosh 
# define SIGPIPE 13
# define SIGALRM 14
   GUSI headers 
# include <sys/ioctl.h>
  // Codewarrior dependant 
# include <SIOUX.h>
# include <console.h>
#endif

#ifdef CIRCLE_WINDOWS		// Includes for Win32
# ifdef __BORLANDC__
#  include <dir.h>
# else // MSVC 
#  include <direct.h>
# endif
# include <mmsystem.h>
#endif // CIRCLE_WINDOWS

#ifdef CIRCLE_AMIGA		// Includes for the Amiga 
# include <sys/ioctl.h>
# include <clib/socket_protos.h>
#endif // CIRCLE_AMIGA 

#ifdef CIRCLE_ACORN		// Includes for the Acorn (RiscOS) 
# include <socklib.h>
# include <inetlib.h>
# include <sys/ioctl.h>
#endif


/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

#include <sys/types.h>

#if defined (CIRCLE_UNIX)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#endif

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "dg_scripts.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif


/* externs */
void Crash_ldsave(struct char_data *ch);
int level_exp(int chclass, int level);
extern const char *dirs[];
extern struct ban_list_element *ban_list;
extern int num_invalid;

extern const char *circlemud_version;
extern int circle_restrict;
extern int mini_mud;
extern int no_rent_check;
extern FILE *player_fl;
extern FILE *pkiller_fl;
extern ush_int DFLT_PORT;
extern const char *DFLT_DIR;
extern const char *DFLT_IP;
extern const char *LOGNAME;
extern int max_playing;
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern int autosave_time;	/* see config.c */
extern struct room_data *world;	/* In db.c */
extern struct time_info_data time_info;		/* In db.c */
extern char *help;
extern struct char_data *character_list;
extern struct char_data *combat_list;
unsigned long int number_of_bytes_read = 0;
unsigned long int number_of_bytes_written = 0;
extern void proc_color(char *inbuf, int color);

/* local globals */
long last_rent_check = 0;   /* at what time checked rented time */
int dg_act_check;			/* toggle for act_trigger */
struct descriptor_data *descriptor_list = NULL;		/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
int scheck = 0;			/* for syntax checking mode */
struct timeval null_time;	/* zero-valued time structure */
FILE *logfile = NULL;		/* Where to send the log messages. */
unsigned long dg_global_pulse = 0; /* number of pulses since game start */

#if defined (CIRCLE_WINDOWS)
BOOL CtrlHandler(DWORD fdwCtrlType)
{
    bool res = FALSE;
    switch (fdwCtrlType)
    {
    case CTRL_CLOSE_EVENT:
    case CTRL_C_EVENT:    
    case CTRL_BREAK_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        res = TRUE;
        circle_shutdown = 1;
        log("STOP: Ctrl+C,Close or System shutdown event.");
        break;
    }
    return res;
}
void startup() { ::SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE); }
void shutdown() { 
    ::SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, FALSE);
}
#else
void startup() {}
void shutdown() {}
#endif

/* functions in this file */
RETSIGTYPE reread_wizlists(int sig);
RETSIGTYPE unrestrict_game(int sig);
RETSIGTYPE reap(int sig);
RETSIGTYPE checkpointing(int sig);
RETSIGTYPE hupsig(int sig);
size_t perform_socket_read(socket_t desc, char *read_point,size_t space_left);
size_t perform_socket_write(socket_t desc, const char *txt,size_t length);
/*void set_binary(struct descriptor_data *d);*/
void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);
void sanity_check(void);
void circle_sleep(struct timeval *timeout);
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(ush_int port);
void signal_setup(void);
void game_loop(socket_t mother_desc);
socket_t init_socket(ush_int port);
int new_descriptor(socket_t s);
int get_max_players(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void save_corpses(void);
void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);
void timeadd(struct timeval *sum, struct timeval *a, struct timeval *b);
void flush_queues(struct descriptor_data *d);
void nonblock(socket_t s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
char *make_prompt(struct descriptor_data *point);
void check_idle_passwords(void);
void heartbeat(int pulse);
struct in_addr *get_bind_addr(void);
int parse_ip(const char *addr, struct in_addr *inaddr);
int set_sendbuf(socket_t s);
void setup_log(const char *filename, int fd);
int open_logfile(const char *filename, FILE *stderr_fp);
char *disp_color(int color);
#if defined(POSIX)
sigfunc *my_signal(int signo, sigfunc * func);
#endif

#if defined(HAVE_ZLIB)
void *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
void zlib_free(void *opaque, void *address);
#endif

/* extern fcnts */
int  level_exp(int chclass, int chlevel);
void mobile_affect_update(void);
void hour_update();
void dt_activity(void);
void beat_points_update(int pulse);
void Crash_rent_time(int dectime);
void Crash_frac_rent_time(int frac_part);
void Crash_frac_save_all(int frac_part);
void dupe_player_index(void);
void flush_player_index(void);
void reboot_wizlists(void);
void boot_world(void);
void player_affect_update(void);	/* In spells.c */
void mobile_activity(int activity_level);
void perform_violence(void);
void perform_wan(void);
void show_string(struct descriptor_data *d, char *input);
int isbanned(char *hostname);
void weather_and_time(int mode);
void save_clans(void);//сохраняем кланы

/*int init_lua(void);*/

/* For Timed Rebooting */
extern int reboot_time;
extern int reboot_state;

#ifdef __CXREF__
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#undef FD_CLR
#define FD_ZERO((unsigned) x)
#define FD_SET(x, y) 0
#define FD_ISSET(x, y) 0
#define FD_CLR(x, y)
#endif

#if defined(HAVE_ZLIB)
/*
 * MUD Client for Linux and mcclient compression support.
 * "The COMPRESS option (unofficial and completely arbitary) is
 * option 85." -- mcclient documentation as of Dec '98.
 *
 * [ Compression protocol documentation below, from Compress.c ]
 *
 * Server sends  IAC WILL COMPRESS
 * We reply with IAC DO COMPRESS
 *
 * Later the server sends IAC SB COMPRESS WILL SE, and immediately following
 * that, begins compressing
 *
 * Compression ends on a Z_STREAM_END, no other marker is used
 */
int mccp_start( struct descriptor_data *t, int ver );
int mccp_end( struct descriptor_data *t, int ver );

#define TELOPT_COMPRESS        85
#define TELOPT_COMPRESS2       86
const char compress_will[]     = { IAC, WILL, TELOPT_COMPRESS2,
                                   IAC, WILL, TELOPT_COMPRESS, '\0' };
const char compress_start_v1[] = { IAC, SB, TELOPT_COMPRESS, WILL, SE, '\0' };
const char compress_start_v2[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, '\0' };
static char end_line[] = { IAC, GA };
#endif
const unsigned char go_ahead_str[] = { IAC, GA, '\0' };



/*#define TELOPT_COMPRESS        85
#define TELOPT_COMPRESS2       86
static char will_sig[] = { IAC, WILL, TELOPT_COMPRESS, 0 };
static char do_sig[] =   { IAC, DO, TELOPT_COMPRESS, 0 };
static char dont_sig[] = { IAC, DONT, TELOPT_COMPRESS, 0 };
#define DO_SIG_LEN 3
static char on_sig[] =   { IAC, SB, TELOPT_COMPRESS, WILL, SE, 0 };
static char will_sig2[]= { IAC, WILL, TELOPT_COMPRESS2, 0 };
static char do_sig2[]  = { IAC, DO, TELOPT_COMPRESS2, 0 };
static char dont_sig2[]= { IAC, DONT, TELOPT_COMPRESS2, 0 };
static char on_sig2[]  = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, 0 };
static char end_line[] = { IAC, GA };
#endif
const  char go_ahead_str[] = { IAC, GA, 0 };   */
/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

#if defined(CIRCLE_WINDOWS) || defined(CIRCLE_MACINTOSH)

/*
 * Windows doesn't have gettimeofday, so we'll simulate it.
 * The Mac doesn't have gettimeofday either.
 * Borland C++ warns: "Undefined structure 'timezone'"
 */
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
#if defined(CIRCLE_WINDOWS)
  DWORD millisec = GetTickCount();
#elif defined(CIRCLE_MACINTOSH)
  unsigned long int millisec;
  millisec = (int)((float)TickCount() * 1000.0 / 60.0);
#endif

  t->tv_sec = (int) (millisec / 1000);
  t->tv_usec = (millisec % 1000) * 1000;
}

#endif	/* CIRCLE_WINDOWS || CIRCLE_MACINTOSH */


#define plant_magic(x)	do { (x)[sizeof(x) - 1] = MAGIC_NUMBER; } while (0)
#define test_magic(x)	((x)[sizeof(x) - 1])

/*#ifdef CIRCLE_WINDOWS
#include "..\msvc\windump.h"
#endif*/

//#define VLD

#ifdef CIRCLE_WINDOWS
#ifdef _DEBUG
#ifdef VLD
#include "..\vld\vld.h"
#pragma comment(lib, "vld.lib")
#endif
#endif
#endif

int main(int argc, char **argv)
{
#ifdef CIRCLE_WINDOWS
#ifdef _DEBUG
#ifndef VLD
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#endif
#endif

  ush_int port;
  int pos = 1;
  const char *dir;

  /* Initialize these to check for overruns later. */
  plant_magic(buf);
  plant_magic(buf1);
  plant_magic(buf2);
  plant_magic(arg);

#ifdef CIRCLE_MACINTOSH
  /*
   * ccommand() calls the command line/io redirection dialog box from
   * Codewarriors's SIOUX library
   */
  argc = ccommand(&argv);
  /* Initialize the GUSI library calls.  */
  GUSIDefaultSetup();
#endif

  port = DFLT_PORT;
  dir = DFLT_DIR;

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'o':
      if (*(argv[pos] + 2))
	LOGNAME = argv[pos] + 2;
      else if (++pos < argc)
	LOGNAME = argv[pos];
      else {
	puts("SYSERR: File name to log to expected after option -o.");
	exit(1);
      }
      break;
    case 'd':
      if (*(argv[pos] + 2))
	dir = argv[pos] + 2;
      else if (++pos < argc)
	dir = argv[pos];
      else {
	puts("SYSERR: Directory arg expected after option -d.");
	exit(1);
      }
      break;
    case 'm':
      mini_mud = 1;
      no_rent_check = 1;
      puts("Running in minimized mode & with no rent check.");
      break;
    case 'c':
      scheck = 1;
      puts("Syntax check mode enabled.");
      break;
    case 'q':
      no_rent_check = 1;
      puts("Quick boot mode -- rent check supressed.");
      break;
    case 'r':
      circle_restrict = 1;
      puts("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      puts("Suppressing assignment of special routines.");
      break;
    case 'h':
      /* From: Anil Mahajan <amahajan@proxicom.com> */
      printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n"
              "  -c             Enable syntax check mode.\n"
              "  -d <directory> Specify library directory (defaults to 'lib').\n"
              "  -h             Print this command line argument help.\n"
              "  -m             Start in mini-MUD mode.\n"
	      "  -o <file>      Write log to <file> instead of stderr.\n"
              "  -q             Quick boot (doesn't scan rent for object limits)\n"
              "  -r             Restrict MUD -- no new players allowed.\n"
              "  -s             Suppress special procedure assignments.\n",
		 argv[0]
      );
      exit(0);
    default:
      printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
      break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit((unsigned char)*argv[pos])) {
      printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      printf("SYSERR: Illegal port number %d.\n", port);
      exit(1);
    }
  }

  /* All arguments have been parsed, try to open log file. */
  setup_log(LOGNAME, STDERR_FILENO);

  /*
   * Moved here to distinguish command line options and to show up
   * in the log if stderr is redirected to a file.
   */
  log(circlemud_version);

  startup();

  if (chdir(dir) < 0) {
    perror("SYSERR: Fatal error changing to data directory");
     exit(1);
  }
  log("Using %s as data directory.", dir);

  if (scheck) {
    boot_world();
    log("Done.");
  } else {
    log("Running game on port %d.", port);
    init_game(port);
  }
  shutdown();
 
  return (0);
}



/* Init sockets, run game, and cleanup sockets */
void init_game(ush_int port)
{
  socket_t mother_desc;

  /* We don't want to restart if we crash before we get up. */
  touch(KILLSCRIPT_FILE);
    
  circle_srandom(time(0));

  max_players = get_max_players();
  log("Players limit: %d ", max_players);

  log("Opening mother connection.");
  mother_desc = init_socket(port);

  boot_db();

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)
  log("Signal trapping.");
  signal_setup();
#endif

  /* If we made it this far, we will be able to restart without problem. */
  remove(KILLSCRIPT_FILE);

  log("Entering game loop.");

  game_loop(mother_desc);

//  Crash_save_all();
  
  log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list, TRUE);

  CLOSE_SOCKET(mother_desc);
  if (!USE_SINGLE_PLAYER)
  { fclose(player_fl);
    fclose(pkiller_fl);
  }
  
  unload_db();

  if (circle_reboot) {
    log("Rebooting.");
    exit(52);			/* what's so great about HHGTTG, anyhow? */
  }
  log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
socket_t init_socket(ush_int port)
{
  socket_t s;
  struct sockaddr_in sa;
  int opt;

#ifdef CIRCLE_WINDOWS
  {
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);

    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
      log("SYSERR: WinSock not available!");
      exit(1);
    }
    if ((wsaData.iMaxSockets - 4) < max_players) {
      max_players = wsaData.iMaxSockets - 4;
    }
    log("Max players set to %d", max_players);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      log("SYSERR: Error opening network connection: Winsock error #%d",
	  WSAGetLastError());
      exit(1);
    }
  }
#else
  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so the point is (hopefully) moot.
   */

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("SYSERR: Error creating socket");
    exit(1);
  }
#endif				/* CIRCLE_WINDOWS */

#if defined(SO_REUSEADDR) && !defined(CIRCLE_MACINTOSH)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0){
    perror("SYSERR: setsockopt REUSEADDR");
    exit(1);
  }
#endif

  set_sendbuf(s);

/*
 * The GUSI sockets library is derived from BSD, so it defines
 * SO_LINGER, even though setsockopt() is unimplimented.
 *	(from Dean Takemori <dean@UHHEPH.PHYS.HAWAII.EDU>)
 */
#if defined(SO_LINGER) && !defined(CIRCLE_MACINTOSH)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0)
      perror("SYSERR: setsockopt SO_LINGER");	/* Not fatal I suppose. */
  }
#endif

  /* Clear the structure */
  memset((char *)&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr = *(get_bind_addr());

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("SYSERR: bind");
    CLOSE_SOCKET(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return (s);
}

int get_max_players(void)
{
  return (max_playing);
}


int get_max_players_off(void) // old
{
#ifndef CIRCLE_UNIX
  return (max_playing);
#else

  int max_descs = 0;
  const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
#ifdef HAS_RLIMIT
  {
    struct rlimit limit;

    /* find the limit of file descs */
    method = "rlimit";
    if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling getrlimit");
      exit(1);
    }

    /* set the current to the maximum */
    limit.rlim_cur = limit.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling setrlimit");
      exit(1);
    }
#ifdef RLIM_INFINITY
    if (limit.rlim_max == RLIM_INFINITY)
      max_descs = max_playing + NUM_RESERVED_DESCS;
    else
      max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#else
    max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#endif
  }

#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
  method = "OPEN_MAX";
  max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
				 * OPEN_MAX */
#elif defined (_SC_OPEN_MAX)
  /*
   * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
   * try the POSIX sysconf() function.  (See Stevens' _Advanced Programming
   * in the UNIX Environment_).
   */
  method = "POSIX sysconf";
  errno = 0;
  if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
    if (errno == 0)
      max_descs = max_playing + NUM_RESERVED_DESCS;
    else {
      perror("SYSERR: Error calling sysconf");
      exit(1);
    }
  }
#else
  /* if everything has failed, we'll just take a guess */
  method = "random guess";
  max_descs = max_playing + NUM_RESERVED_DESCS;
#endif

  /* now calculate max _players_ based on max descs */
  max_descs = MIN(max_playing, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    log("SYSERR: Non-positive max player limit!  (Set at %d using %s).",
	    max_descs, method);
    exit(1);
  }
  log("   Setting player limit to %d using %s.", max_descs, method);
  return (max_descs);
#endif /* CIRCLE_UNIX */
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(socket_t mother_desc)
{
  fd_set input_set, output_set, exc_set, null_set;
  struct timeval last_time, opt_time, process_time, temp_time;
  struct timeval before_sleep, now, timeout;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int pulse = 0, missed_pulses, maxdesc, aliased;

  /* initialize various time values */
  null_time.tv_sec		= 0;
  null_time.tv_usec		= 0;
  opt_time.tv_usec		= OPT_USEC;
  opt_time.tv_sec		= 0;
  FD_ZERO(&null_set);
  last_rent_check       = time(NULL);
  
  gettimeofday(&last_time, (struct timezone *) 0);

       
  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

        /* Sleep if we don't have any connections */
    if (descriptor_list == NULL) {
      log("No connections.  Going to sleep.");
      FD_ZERO(&input_set);
      FD_SET(mother_desc, &input_set);
      if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
	if (errno == EINTR)
	  log("Waking up to process signal.");
	else
	  perror("SYSERR: Select coma");
      } else
	log("New connection.  Waking up.");
      gettimeofday(&last_time, (struct timezone *) 0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);

    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next) {
#ifndef CIRCLE_WINDOWS
      if (d->descriptor > maxdesc)
	maxdesc = d->descriptor;
#endif
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    /*
     * At this point, we have completed all input, output and heartbeat
     * activity from the previous iteration, so we have to put ourselves
     * to sleep until the next 0.1 second tick.  The first step is to
     * calculate how long we took processing the previous iteration.
     */
    
    gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
    timediff(&process_time, &before_sleep, &last_time);

    /*
     * If we were asleep for more than one pass, count missed pulses and sleep
     * until we're resynchronized with the next upcoming pulse.
     */
    if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
      missed_pulses = 0;
    } else {
      missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
      missed_pulses += process_time.tv_usec / OPT_USEC;
      process_time.tv_sec = 0;
      process_time.tv_usec = process_time.tv_usec % OPT_USEC;
    }

    /* Calculate the time we should wake up */
    timediff(&temp_time, &opt_time, &process_time);
    timeadd(&last_time, &before_sleep, &temp_time);

    /* Now keep sleeping until that time has come */
    gettimeofday(&now, (struct timezone *) 0);
    timediff(&timeout, &last_time, &now);

    /* Go to sleep */
    do {
      circle_sleep(&timeout);
      gettimeofday(&now, (struct timezone *) 0);
      timediff(&timeout, &last_time, &now);
    } while (timeout.tv_usec || timeout.tv_sec);

    /* Poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("SYSERR: Select poll");
      return;
    }
    /* If there are new connections waiting, accept them. */
    if (FD_ISSET(mother_desc, &input_set))
      new_descriptor(mother_desc);

    /* Kick out the freaky folks in the exception set and marked for close */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) {
	FD_CLR(d->descriptor, &input_set);
	FD_CLR(d->descriptor, &output_set);
	close_socket(d, TRUE);
      }
    }

    /* Process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &input_set))
	if (process_input(d) < 0)
	  close_socket(d, FALSE);
    }

    /* Process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

      /*
       * Not combined to retain --(d->wait) behavior. -gg 2/20/98
       * If no wait state, no subtraction.  If there is a wait
       * state then 1 is subtracted. Therefore we don't go less
       * than 0 ever and don't require an 'if' bracket. -gg 2/27/99
       */

	   if (d->character)
            { GET_WAIT_STATE(d->character) -= (GET_WAIT_STATE(d->character) > 0 ? 1 : 0);
              SET_COMMSTATE(d->character,0);
              if (WAITLESS(d->character) || GET_WAIT_STATE(d->character) < 0)
                 GET_WAIT_STATE(d->character) = 0;
              if (GET_WAIT_STATE(d->character))
                 continue;
            }

         // Если сидим больше 5 минут в меню, то вылетаем!!!
         if  (!get_from_q(&d->input, comm, &aliased))
 	     {if (STATE(d) != CON_PLAYING          &&
	          STATE(d) != CON_DISCONNECT       &&
	          time(NULL) - d->input_time > 300 &&
                  d->character                     &&
	          !IS_GOD(d->character)
		 )
                 close_socket(d,TRUE);
              continue;
	     }

         d->input_time = time(NULL);
	   
	   /*if (d->character) {
        GET_WAIT_STATE(d->character) -= (GET_WAIT_STATE(d->character) > 0);

        if (GET_WAIT_STATE(d->character))
          continue;
      }*/

     /* if (!get_from_q(&d->input, comm, &aliased))
        continue;*/

      if (d->character) {
	/* Reset the idle timer & pull char back from void if necessary */
	d->character->char_specials.timer = 0;
	if (STATE(d) == CON_PLAYING && GET_WAS_IN(d->character) != NOWHERE) {
	  if (d->character->in_room != NOWHERE)
	    char_from_room(d->character);
	  char_to_room(d->character, GET_WAS_IN(d->character));
	  GET_WAS_IN(d->character) = NOWHERE;
	  act("$n возвратил$u.", TRUE, d->character, 0, 0, TO_ROOM); /*has returned*/
         GET_WAIT_STATE(d->character) = 1;
         }
      }
      d->has_prompt = 0;

      if (d->str)		/* Writing boards, mail, etc. */
	string_add(d, comm);
      else if (d->showstr_count) /* Reading something w/ pager */
	show_string(d, comm);
      else if (STATE(d) != CON_PLAYING) /* In menus, etc. */
		  nanny(d, comm);
      else {			/* else: we're playing normally. */
	if (aliased)		/* To prevent recursive aliases. */
	  d->has_prompt = 1;	/* To get newline before next cmd output. */
	else if (perform_alias(d, comm))
	  get_from_q(&d->input, comm, &aliased);
 	command_interpreter(d->character, comm); /* Send it to interpreter */
      }
    }

    for (d = descriptor_list; d; d = next_d)
    {
      next_d = d->next;
      if ( (!d->has_prompt || *(d->output)) &&
           FD_ISSET(d->descriptor, &output_set)
         )
      {
        if ( process_output(d) < 0 )
          close_socket(d,FALSE); 
        else
          d->has_prompt = 1;     
      }
    }
#if 0
    /* Send queued output out to the operating system (ultimately to user). */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {
	/* Output for this player is ready */
	if (process_output(d) < 0)
	  close_socket(d, FALSE);
	else
	  d->has_prompt = 1;
      }
    }

    /* Print prompts for other descriptors who had no other output */
    for (d = descriptor_list; d; d = d->next) {
      if (!d->has_prompt) {
             sprintf(buf,"%s",make_prompt(d));

#if defined(HAVE_ZLIB)
	 	SEND_TO_Q(make_prompt(d), d);
                //оттестить на линухе
		SEND_TO_Q(end_line,d);
#else
	  from_koi(buf, d->codepage);
	  SEND_TO_SOCKET(buf, d->descriptor);
#endif
	  d->has_prompt = 2; //???? 1
      }
    }
#endif
    
    /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    for (d = descriptor_list; d; d = next_d)
		{ next_d = d->next;
		  if ((STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT))
		  close_socket(d, FALSE);
	    }
    /*
     * Now, we execute as many pulses as necessary--just one if we haven't
     * missed any pulses, or make up for lost time if we missed a few
     * pulses by sleeping for too long.
     */
    missed_pulses++;

    if (missed_pulses <= 0) {
      log("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE (%d), TIME GOING BACKWARDS!!", missed_pulses);
      missed_pulses = 1;
    }

    /* If we missed more than 30 seconds worth of pulses, just do 30 secs */
    if (missed_pulses > (30 * PASSES_PER_SEC)) {
      log("SYSERR: Missed %d seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
      missed_pulses = 30 * PASSES_PER_SEC;
    }

    /* Now execute the heartbeat functions */
    while (missed_pulses--)
      heartbeat(++pulse);

    /* Roll pulse over after 10 hours */
    if (pulse >= (600 * 60 * PASSES_PER_SEC))
      pulse = 0;

#ifdef CIRCLE_UNIX
    /* Update tics for deadlock protection (UNIX only) */
    tics++;
#endif
  }
}

#define FRAC_SAVE TRUE
void beat_points_update(int pulse);


void heartbeat(int pulse)
{
  struct char_data *ch=NULL, *k, *next;

  long   check_at = 0, save_start = 0;
  static int mins_since_crashsave = 0;

  void process_events(void);

   dg_global_pulse++;

   process_events();

 if (!(pulse % PULSE_DG_SCRIPT))
      script_trigger_check();
    
    

  if (!(pulse % (PASSES_PER_SEC/5))) // 0.2 секунды обработчик
       perform_wan();	//  событий в комнате во время прекращения боя.
  

 if (!(pulse % (30 * PASSES_PER_SEC)))
    sanity_check();

 

  if (!(pulse % (40 * PASSES_PER_SEC)))		//40 seconds
    check_idle_passwords();//здесь обработчик времени сидения в меню

 
  
  if (!(pulse % PASSES_PER_SEC))  // 1 second Timed Reboot  
     { if (reboot_state) 
         if (reboot_time <= 0) 
           { touch(FASTBOOT_FILE);
   	     circle_shutdown = circle_reboot = 1;
	   }
     }

  
    mobile_activity(pulse);

    if (!(pulse % (2 * PASSES_PER_SEC)))
        dt_activity();
     
     	
    if (!(pulse % PULSE_VIOLENCE)) 
	  perform_violence();

    if (!(pulse % (AUCTION_PULSES * PASSES_PER_SEC)))
          tact_auction();
      
    if (!(pulse % (SECS_PER_PLAYER_AFFECT * PASSES_PER_SEC)))
          player_affect_update();
         
    if (!(pulse % (TIME_KOEFF * SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
     {
      weather_and_time(1);
      paste_mobiles(-1);
     }

  if (!(pulse % PULSE_ZONE))
       zone_update();

  if (!(pulse % (3 * 60 * PASSES_PER_SEC)))	// 3 minutes
      save_clans();

// 1 минута
   if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)))
   {
      mobile_affect_update();
	  point_update();
	  save_corpses();	
	   
      flush_player_index();

			if (reboot_state) {
				reboot_time--;
				if (reboot_time > 0) {
			sprintf(buf, "&C                            [ВНИМАНИЕ... ДО НАЧАЛА ПЕРЕЗАГРУЗКИ - %d МИНУТ%s!]&n\r\n", reboot_time, reboot_time >= 5 ? "": reboot_time == 1 ? "А!!" : "Ы");
			send_to_all(buf);
				}
			else  {
			send_to_all("&W\r\n                                       ******** ПЕРЕЗАГРУЗКА ********&n\r\n");
	  reboot_time --;
	  }
	}
  }
  
  if (pulse == 720)
     dupe_player_index();

  if (!(pulse % PASSES_PER_SEC))
     beat_points_update(pulse / PASSES_PER_SEC);

	
    if (FRAC_SAVE && auto_save && !(pulse % PASSES_PER_SEC))
     {// 1 game secunde
      Crash_frac_save_all((pulse / PASSES_PER_SEC) % PLAYER_SAVE_ACTIVITY);
      Crash_frac_rent_time((pulse / PASSES_PER_SEC) % OBJECT_SAVE_ACTIVITY);
     }

  if (!FRAC_SAVE && auto_save && !(pulse % (60 * PASSES_PER_SEC)))
     {// 1 minute
      if (++mins_since_crashsave >= autosave_time)
         {mins_since_crashsave = 0;
          //log("Crash save all...");
          Crash_save_all();
          //log("Stop it...");
          check_at = time(NULL);
          if (last_rent_check > check_at)
             last_rent_check = check_at;
          if (((check_at - last_rent_check) / 60))
             {//log("Crash rent time...");
              save_start = time(NULL);
              Crash_rent_time((check_at - last_rent_check) / 60);
              //log("Saving rent timer time = %ld(s)",time(NULL) - save_start);
              last_rent_check = time(NULL) - (check_at - last_rent_check) % 60;
	      //log("Stop it...");
             }
         }
     }
	
	
  
  if (!(pulse % (5 * 60 * PASSES_PER_SEC)))	// 5 minutes
    record_usage();

 //log ("Real extract char");
  for (k = character_list; k; k = next)
      {next = k->next;
       if (IN_ROOM(k) == NOWHERE || MOB_FLAGGED(k, MOB_DELETE))
          {//log("[Heartbeat] Remove from list char %s", GET_NAME(k));
           if (k == character_list)
              character_list = next;
           else
              ch->next = next;
           k->next = NULL;
           if (MOB_FLAGGED(k,MOB_FREE))
              {
               free_char(k);
              }
          }
       else
          ch = k;
      }
}

/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
void timediff(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  if (a->tv_sec < b->tv_sec)
    *rslt = null_time;
  else if (a->tv_sec == b->tv_sec) {
    if (a->tv_usec < b->tv_usec)
      *rslt = null_time;
    else {
      rslt->tv_sec = 0;
      rslt->tv_usec = a->tv_usec - b->tv_usec;
    }
  } else {			/* a->tv_sec > b->tv_sec */
    rslt->tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_usec < b->tv_usec) {
      rslt->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
      rslt->tv_sec--;
    } else
      rslt->tv_usec = a->tv_usec - b->tv_usec;
  }
}

/*
 * Add 2 time values.
 *
 * Patch sent by "d. hall" <dhall@OOI.NET> to fix 'static' usage.
 */
void timeadd(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  rslt->tv_sec = a->tv_sec + b->tv_sec;
  rslt->tv_usec = a->tv_usec + b->tv_usec;

  while (rslt->tv_usec >= 1000000) {
    rslt->tv_usec -= 1000000;
    rslt->tv_sec++;
  }
}


void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (STATE(d) == CON_PLAYING)
      sockets_playing++;
  }

  log("nusage: %-3d sockets connected, %-3d sockets playing",
	  sockets_connected, sockets_playing);

#ifdef RUSAGE	/* Not RUSAGE_SELF because it doesn't guarantee prototype. */
  {
    struct rusage ru;

    getrusage(RUSAGE_SELF, &ru);
    log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
	    ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
  }
#endif

}

/*
 * Binary option (specific to telnet client)
 */
void set_binary(struct descriptor_data *d)
{
  char binary_string[] =
  {
    (char) IAC,
    (char) DO,
    (char) TELOPT_BINARY,
    (char) 0,
  };
//  write(d->descriptor, binary_string,3);
 
} 


/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0
  };

  SEND_TO_Q(off_string, d);
}

/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) IAC,
    (char) WONT,
    (char) TELOPT_NAOFFD,
    (char) IAC,
    (char) WONT,
    (char) TELOPT_NAOCRD,
    (char) 0,
  };

  SEND_TO_Q(on_string, d);
}

int  posi_value(int real, int max)
{ if (real < 0)
     return (-1);
  else
  if (real >= max)
     return (10);

  return (real * 10 / MAX(max,1));
}

char *color_value(struct char_data *ch, int real, int max)
{static char color[256];
 switch (posi_value(real,max))
 {case -1 : case 0 : case 1 :
      sprintf(color,"&R"); break;
  case 2 : case 3 :
      sprintf(color,"&r"); break;
  case 4 : case 5 :
      sprintf(color,"&Y"); break;
  case 6 : case 7 : case 8 :
      sprintf(color,"&G"); break;
  default:
      sprintf(color,"&g"); break;
 }
 return (color);
}

char *show_state(struct char_data *ch, struct char_data *victim)
{ int ch_hp = 11;
  static char *WORD_STATE[12] =
              {"Умирает",
               "Ужасное",
               "О.Плохое",
               "Плохое",
               "Плохое",
               "Среднее",
               "Среднее",
               "Хорошее",
               "Хорошее",
               "Хорошее",
               "О.Хорошее",
               "Превосходное"};

  ch_hp = posi_value(GET_HIT(victim),GET_REAL_MAX_HIT(victim)) + 1;
  sprintf(buf, "[%s:%s%s%s] ",
          PERS(victim,ch),
	      color_value(ch, GET_HIT(victim), GET_REAL_MAX_HIT(victim)),
          WORD_STATE[ch_hp],//GET_CH_SUF_6(victim),
          CCNRM(ch, C_NRM));
  return buf;
}


char *any_arg(char *argument, char *first_arg)
{
	
 // skip_spaces(&argument);
	
  while (*argument && *argument != '%') {
    *(first_arg++) = *argument;
    argument++;
  }
 
  *first_arg = '\0';

  return (argument);
}
//перенести линух
char *make_prompt(struct descriptor_data *d)
{    
    static char DEFAULT_PROMPT[] = "%hж/%HЖ %vэ/%VЭ %Oо %gм %wm %f %#C%e%%>%#n ";
    static char DEFAULT_PROMPT_FIGHTING[] = "%f %hж %vэ %wm %#C%e%%>%#n ";
    static char *dirs[] = {"С","В","Ю","З","^","v"};
    char *str;
    int door, ch_hp, imm=0; // sec_hp,
    *buf2 = '\0';

 if (d->showstr_count)
	{   sprintf(buf2,
	    "\r&K[ Для продолжения нажмите (к)онец, (п)овтор, (н)азад или номер страницы (%d/%d) ]&n",
	    d->showstr_page, d->showstr_count);
	}  else
		if (d->str)
    strcpy(buf2, "] ");
    else
 if (STATE(d) == CON_PLAYING)
	{ 
 
	if (GET_INVIS_LEV(d->character))
        sprintf(buf2 + strlen(buf2), "i%d ", GET_INVIS_LEV(d->character));
        if (PRF_FLAGGED(d->character, PRF_AFK))
        sprintf(buf2 + strlen(buf2),"%s[AFK]%s ",CCCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));



       if (PRF_FLAGGED(d->character, PRF_DISPHP) && FIGHTING(d->character))
	   str = d->character->fprompt;
       else
 	   str = d->character->prompt;
	
	if (IS_NULLSTR(str)&& !FIGHTING(d->character))
	   str = DEFAULT_PROMPT;
	if (IS_NULLSTR(str)&& FIGHTING(d->character))
	   str = DEFAULT_PROMPT_FIGHTING;
	

	if (IS_IMMORTAL(d->character))
           imm = 1;

	while (*str != '\0')
	{
		if (*str != '%')
		{
			*str++;
			continue;
		}

		switch(*++str)
		{
		  default:
	        sprintf(buf2 + strlen(buf2), "");
			break;

	    case 't':
          *(++str);
		   any_arg(str, buf);
		   sprintf(buf2 + strlen(buf2), "%d%s", time_info.hours, buf);
			break;

		case 'e':
	       for (door = 0; door < NUM_OF_DIRS; door++)
			{ if (EXIT(d->character, door) &&
                       EXIT(d->character, door)->to_room != NOWHERE)
				{ if (EXIT_FLAGGED(EXIT(d->character, door), EX_HIDDEN))
					{  if (IS_IMPL(d->character))
					      sprintf(buf2 + strlen(buf2), "[%s]", dirs[door]);
						continue;
					}
					    EXIT_FLAGGED(EXIT(d->character, door), EX_CLOSED) ?
      	                  sprintf(buf2 + strlen(buf2), "(%s)", dirs[door]) : 
						  sprintf(buf2 + strlen(buf2), "%s", dirs[door]);
				} 
				
			}
		break;

		case 'n':
			*(++str);
		   any_arg(str, buf);
		   sprintf(buf2 + strlen(buf2), "%s%s",d->character->player.name,buf);
		    break;	
		case 's':
			sprintf(buf2 + strlen(buf2), "%s",d->character->player.sex == SEX_MALE ? "Мужчина" :
			    d->character->player.sex == SEX_FEMALE ? "Женщина" :
			    "Бесполое");
			break;

		case 'h':
		    *(++str);
		    any_arg(str, buf);
		    sprintf(buf2 + strlen(buf2), "%s%d%s%s",
			color_value(d->character,GET_HIT(d->character),GET_REAL_MAX_HIT(d->character)),
			GET_HIT(d->character), buf, CCNRM(d->character, C_NRM));
		    break;	

		case 'H':
			*(++str);
		    any_arg(str, buf);
		    sprintf(buf2 + strlen(buf2), "%d%s", GET_REAL_MAX_HIT(d->character), buf);
		   	break;

		case 'w':
			*(++str);
		   any_arg(str, buf);
			if (IS_WEDMAK(d->character))
				sprintf(buf2 + strlen(buf2), "%d%s",GET_MANA(d->character), buf);
		  if ((ch_hp = GET_MANA_NEED(d->character)))
          {
              door = mana_gain(d->character);
              //int spell_id = d->character->Memory.get_meming_spell();
              if (door) // &&!FIGHTING(d->character) && spell_id != -1)
			  { 
                  int seconds = d->character->Memory.calc_time(d->character, -1);
                  int minutes = seconds / 60;
                  seconds = seconds - minutes * 60;
                  sprintf(buf2 + strlen(buf2),"Зап:%d:%s%d ", minutes, seconds > 9 ? "" : "0" ,seconds);

                 //ch_hp = d->character->Memory.get_mana_cost(d->character, spell_id);
                 //ch_hp = MAX(0, 1+ch_hp-GET_MANA_STORED(d->character));
		         //sec_hp = ch_hp * 60;
                 //ch_hp = ch_hp / door;
		         //sec_hp = MAX(0, sec_hp/door - ch_hp*60);
                 //sprintf(buf2 + strlen(buf2),"Зап:%d:%s%d ",ch_hp,
		         //sec_hp > 9 ? "" : "0" ,sec_hp);
              }   
			   else
                 sprintf(buf2 + strlen(buf2),"Зап:- ");
             }
            break;
		case 'W':
            if (IS_WEDMAK(d->character))
		{ *(++str);
		  any_arg(str, buf); 
                  sprintf(buf2 + strlen(buf2), "%d%s", d->character->points.max_mana,buf);
		}
                 break;

        case 'f':
			*(++str);
			if (FIGHTING(d->character) && IN_ROOM(d->character) == IN_ROOM(FIGHTING(d->character))) 
		 { sprintf(buf2 + strlen(buf2), "%s", show_state(d->character, d->character));
          if (FIGHTING(FIGHTING(d->character)) && FIGHTING(FIGHTING(d->character)) != d->character)
            sprintf(buf2 + strlen(buf2), "%s", show_state(d->character, FIGHTING(FIGHTING(d->character))));
          sprintf(buf2 + strlen(buf2), "%s", show_state(d->character, FIGHTING(d->character)));
		 }
		 break;
		case 'v':
		    *(++str);
		    any_arg(str, buf);
		    sprintf(buf2 + strlen(buf2), "%s%d%s%s",color_value(d->character,GET_MOVE(d->character),GET_REAL_MAX_MOVE(d->character)),
			GET_MOVE(d->character), buf, CCNRM(d->character, C_NRM)); 
			break;

		case 'V':
		    *(++str);
		    any_arg(str, buf);
		    sprintf(buf2 + strlen(buf2), "%d%s",GET_REAL_MAX_MOVE(d->character), buf);
			break;

		case 'X':
			*(++str);
		    any_arg(str, buf);
			sprintf(buf2 + strlen(buf2), "%ld%s", GET_EXP(d->character), buf);
		   break;

		case 'O':
			if (!IS_NPC(d->character)&& !imm)
			{ *(++str);
		       any_arg(str, buf);
		     sprintf(buf2 + strlen(buf2), "%ld%s",
            level_exp(GET_CLASS(d->character), GET_LEVEL(d->character) + 1) - 
	              GET_EXP(d->character), buf);
			}
			break;
        case 'K': 
			 if(!IS_NPC(d->character) && !imm) 
			 {	*(++str);
		         any_arg(str, buf);
			   sprintf(buf2 + strlen(buf2), "%ld%s",
               (level_exp(GET_CLASS(d->character), GET_LEVEL(d->character) + 1) - 
	              GET_EXP(d->character))/1000, buf);
			}
			 break;
		case 'M':
			 if(!IS_NPC(d->character)&& !imm) 
			 {   	*(++str);
		   any_arg(str, buf);
				 sprintf(buf2 + strlen(buf2), "%ld%s",
            (level_exp(GET_CLASS(d->character), GET_LEVEL(d->character) + 1) - 
	              GET_EXP(d->character))/1000000, buf);					 
			 }		
			break;

		case 'g':
				*(++str);
		    any_arg(str, buf);
			sprintf(buf2 + strlen(buf2), "%d%s", GET_GOLD(d->character), buf);
		
			break;
	

		case 'a':
			sprintf(buf2 + strlen(buf2), "%s ",IS_GOOD(d->character) ? " светлый " :
			    IS_EVIL(d->character) ? " темный " :
			    " нейтральный ");
			break;

		case '#':
           *(++str);
		   any_arg(str, buf);
		   sprintf(buf2 + strlen(buf2), "&%s",buf);
			break;
		case '%':
			*(++str);
			any_arg(str, buf);
			sprintf(buf2 + strlen(buf2), "%s",buf);
			break;
		}
	while((++str) == " ");
}

strcat(buf2, "&n" );
}

else
 *buf2 = '\0';
return (buf2);
} 


/*
void *make_prompt(struct descriptor_data *d)
{ 
int door, ch_hp, sec_hp;
static char prompt[MAX_INPUT_LENGTH];
int color_hits = 0; 
int hits = 0;
  
  
  if (d->showstr_count) {
    sprintf(prompt,
	    "\r&K[ Для продолжения нажмите, (к)онец, (п)овтор, (н)азад, или номер страницы (%d/%d) ]&n",
	    d->showstr_page, d->showstr_count);
  }  else if (d->str)
    strcpy(prompt, "] ");
    
   else if (STATE(d) == CON_PLAYING) {
    int count = 0;
    *prompt = '\0';
    
          if (GET_INVIS_LEV(d->character))
 count += sprintf(prompt + count, "i%d ", GET_INVIS_LEV(d->character));
		  if (PRF_FLAGGED(d->character, PRF_AFK))
 count += sprintf(prompt + count,"%s[AFK]%s ",CCCYN(d->character, C_NRM),
											 CCNRM(d->character, C_NRM));
          if (GET_HIT(d->character) > 0)
            color_hits = (100 * GET_HIT(d->character)) / GET_MAX_HIT(d->character);
           else 
			   color_hits = GET_HIT(d->character);
	      
		   if (PRF_FLAGGED(d->character, PRF_DISPHP)) {
			  if (color_hits >= 75) 
				  count +=sprintf(prompt + count, "%s%dж %s",
						CCGRN(d->character, C_NRM),GET_HIT(d->character),
						CCNRM(d->character, C_NRM));
		       else if (color_hits >= 30)  
				  count +=sprintf(prompt + count, "%s%dж %s",
						CCYEL(d->character, C_NRM),GET_HIT(d->character),
						CCNRM(d->character, C_NRM));
							
               else if (color_hits >= -11) 
	  count +=sprintf(prompt + count, "%s%dж %s",CCRED(d->character, C_NRM),GET_HIT(d->character),
		  CCNRM(d->character, C_NRM));
      							
	 	}
	      
		  if (PRF_FLAGGED(d->character, PRF_DISPMOVE)){
			  if (GET_MOVE(d->character) >= 60)
      count += sprintf(prompt + count, "%s%dэ%s ",CCGRN(d->character, C_NRM), GET_MOVE(d->character), CCNRM(d->character, C_NRM));
	else if (GET_MOVE(d->character) >= 15)
      count += sprintf(prompt + count, "%s%dэ%s ",CCYEL(d->character, C_NRM), GET_MOVE(d->character), CCNRM(d->character, C_NRM));
	else if (GET_MOVE(d->character) >= 0)
      count += sprintf(prompt + count, "%s%dэ%s ",CCRED(d->character, C_NRM), GET_MOVE(d->character), CCNRM(d->character, C_NRM));
		  }

	
 	 if (GET_LEVEL(d->character) < LVL_IMMORT) {
        if (PRF_FLAGGED(d->character,  PRF_DISPEXP))
        if(!d->character->player_specials->saved.spare3) 
		 count += sprintf(prompt + count, "%dо ",
            level_exp(GET_CLASS(d->character), GET_LEVEL(d->character) + 1) - 
	              GET_EXP(d->character));
         else 
			 if(d->character->player_specials->saved.spare3==1) 
		 count += sprintf(prompt + count, "%dк ",
            (level_exp(GET_CLASS(d->character), GET_LEVEL(d->character) + 1) - 
	              GET_EXP(d->character))/1000);
		else 
			 if(d->character->player_specials->saved.spare3==2) 
		 count += sprintf(prompt + count, "%dм ",
            (level_exp(GET_CLASS(d->character), GET_LEVEL(d->character) + 1) - 
	              GET_EXP(d->character))/1000000);

		if (PRF_FLAGGED(d->character, PRF_DISPGOLD))
       	  count += sprintf(prompt + count, "%dд ",GET_GOLD(d->character));
	}
	
     // Mem Info
      if (PRF_FLAGGED(d->character, PRF_DISPMANA))
      {   if (IS_WEDMAK(d->character))
                  count += sprintf(prompt + count, "%dm ",GET_MANA(d->character));
	         if ((ch_hp = GET_MANA_NEED(d->character)))
             {door = mana_gain(d->character);
              if (door &&!FIGHTING(d->character))
			  {  ch_hp = MAX(0, 1+ch_hp-GET_MANA_STORED(d->character));
		         sec_hp = ch_hp * 60;
                 ch_hp = ch_hp / door;
		         sec_hp = MAX(0, sec_hp/door - ch_hp*60);
                 count += sprintf(prompt+count,"Зап:%d:%s%d ",ch_hp,
		         sec_hp > 9 ? "" : "0" ,sec_hp);
                  }   
	      else
                 count += sprintf(prompt+count,"Зап:- ");
             }
       }  
		 
         if (FIGHTING(d->character) && IN_ROOM(d->character) == IN_ROOM(FIGHTING(d->character))) 
		 { count += sprintf(prompt + count, "%s", show_state(d->character, d->character));
          if (FIGHTING(FIGHTING(d->character)) && FIGHTING(FIGHTING(d->character)) != d->character)
             count += sprintf(prompt + count, "%s", show_state(d->character, FIGHTING(FIGHTING(d->character))));
          count += sprintf(prompt + count, "%s", show_state(d->character, FIGHTING(d->character)));
		 
		 
		 
}
  	if (PRF_FLAGGED(d->character, PRF_DISPEXITS))
				{ count +=sprintf(prompt + count, "%s", CCCYN(d->character, C_NRM));
	            for (door = 0; door < NUM_OF_DIRS; door++)
					{ if (EXIT(d->character, door) &&
                       EXIT(d->character, door)->to_room != NOWHERE)
				{if (EXIT_FLAGGED(EXIT(d->character, door), EX_HIDDEN))
					{  if (IS_IMPL(d->character))
					count +=sprintf(prompt + count, "[%s]", dirs[door]);
						continue;
					}
					count += EXIT_FLAGGED(EXIT(d->character, door), EX_CLOSED) ?
      	                  sprintf(prompt + count, "(%s)", dirs[door]) : 
						  sprintf(prompt + count, "%s", dirs[door]);
					} 
				}
				strcat(prompt, CCNRM(d->character, C_NRM));
				}
	   strcat(prompt, "> " );
  } else if (STATE(d) == CON_PLAYING && IS_NPC(d->character))
    sprintf(prompt, "%s>", GET_NAME(d->character));
    else 
	*prompt = '\0';
  return (prompt);
}
 */

char *disp_color(int color)
{
	if (color >=100)
		return "Превосходное";
	if (color >=90)
		return "О.Хорошее";
	if (color >=75)
		return "Хорошее";
	if (color >=50)
		return "Среднее";
	if (color >=30)
		return "Плохое";
	if (color >=15)
		return "О.Плохое";
	if (color >=1)
		return "Ужасное";
	return "Умирает";


}


void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
  struct txt_block *newt;

  CREATE(newt, struct txt_block, 1);
  newt->text = str_dup(txt);
  newt->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    newt->next = NULL;
    queue->head = queue->tail = newt;
  } else {
    queue->tail->next = newt;
    queue->tail = newt;
    newt->next = NULL;
  }
}



int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return (0);

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return (1);
}



/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy));
}

/* Add a new string to a player's output queue */
void write_to_output(const char *txt, struct descriptor_data *t)
{
  int size;

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

   if ((ubyte)*txt == 255) 
         return;

  size = strlen(txt);

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size) {
    strcpy(t->output + t->bufptr, txt);
 	t->bufspace -= size;
    t->bufptr += size;
    return;
  }
  /*
   * If the text is too big to fit into even a large buffer, chuck the
   * new text and switch to the overflow state.
   */
   
if (t->large_outbuf || ((size + strlen(t->output)) > LARGE_BUFSIZE)) {
    t->bufptr = -1;
    buf_overflows++;
    return;
  }

  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL) {
    t->large_outbuf = bufpool;
    bufpool = bufpool->next;
  } else {			/* else create a new one */
    CREATE(t->large_outbuf, struct txt_block, 1);
    CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
  t->output = t->large_outbuf->text;	/* make big buffer primary */
  strcat(t->output, txt);	/* now add new text */


  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;

}



/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

struct in_addr *get_bind_addr()
{
  static struct in_addr bind_addr;

  /* Clear the structure */
  memset((char *) &bind_addr, 0, sizeof(bind_addr));

  /* If DLFT_IP is unspecified, use INADDR_ANY */
  if (DFLT_IP == NULL) {
    bind_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /* If the parsing fails, use INADDR_ANY */
    if (!parse_ip(DFLT_IP, &bind_addr)) {
      log("SYSERR: DFLT_IP of %s appears to be an invalid IP address",DFLT_IP);
      bind_addr.s_addr = htonl(INADDR_ANY);
    }
  }

  /* Put the address that we've finally decided on into the logs */
  if (bind_addr.s_addr == htonl(INADDR_ANY))
    log("Binding to all IP interfaces on this host.");
  else
    log("Binding only to IP address %s", inet_ntoa(bind_addr));

  return (&bind_addr);
}

#ifdef HAVE_INET_ATON

/*
 * inet_aton's interface is the same as parse_ip's: 0 on failure, non-0 if
 * successful
 */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  return (inet_aton(addr, inaddr));
}

#elif HAVE_INET_ADDR

/* inet_addr has a different interface, so we emulate inet_aton's */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  long ip;

  if ((ip = inet_addr(addr)) == -1) {
    return (0);
  } else {
    inaddr->s_addr = (unsigned long) ip;
    return (1);
  }
}

#else

/* If you have neither function - sorry, you can't do specific binding. */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  log("SYSERR: warning: you're trying to set DFLT_IP but your system has no\n"
      "functions to parse IP addresses (how bizarre!)");
  return (0);
}

#endif /* INET_ATON and INET_ADDR */

unsigned long get_ip(const char *addr)
{ 
 static struct in_addr ip; 
 parse_ip(addr, &ip);
  return (ip.s_addr);
}


/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socket_t s)
{
#if defined(SO_SNDBUF) && !defined(CIRCLE_MACINTOSH)
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt SNDBUF");
    return (-1);
  }

#if 0
  if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt RCVBUF");
    return (-1);
  }
#endif

#endif

  return (0);
}

int new_descriptor(socket_t s)
{
  socket_t desc;
/*unsigned long addr;*/
  int sockets_connected = 0;
  int i;
  static int last_desc = 0;	/* last descriptor number */
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from;
 extern char *GREETINGS;

  /* accept the new connection */
#ifdef CIRCLE_UNIX
  socklen_t sl = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) &peer, &sl)) == INVALID_SOCKET)
  {
    perror("SYSERR: accept");
    return (-1);
  }
#else
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET)
  {
    perror("SYSERR: accept");
    return (-1);
  }
#endif

  /* keep it from blocking */
  nonblock(desc);

  /* set the send buffer size */
  if (set_sendbuf(desc) < 0) {
    CLOSE_SOCKET(desc);
    return (0);
  }

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= max_players) {
    SEND_TO_SOCKET("Sorry, server is full!\r\n", desc);
    CLOSE_SOCKET(desc);
    return (0);
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *) newd, 0, sizeof(struct descriptor_data));

  /* find the sitename */
  if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
				      sizeof(peer.sin_addr), AF_INET))) {

    /* resolution failed */
    if (!nameserver_is_slow)
      perror("SYSERR: gethostbyaddr");

    /* find the numeric site address */
    strncpy(newd->host, (char *)inet_ntoa(peer.sin_addr), HOST_LENGTH);
    *(newd->host + HOST_LENGTH) = '\0';
  } else {
    strncpy(newd->host, from->h_name, HOST_LENGTH);
    *(newd->host + HOST_LENGTH) = '\0';
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    CLOSE_SOCKET(desc);
    sprintf(buf2, "Попытка соединения с хоста [%s]", newd->host);
    mudlog(buf2, CMP, LVL_GOD, TRUE);
    free(newd);
    return (0);
  }
#if 0
  /*
   * Log new connections - probably unnecessary, but you may want it.
   * Note that your immortals may wonder if they see a connection from
   * your site, but you are wizinvis upon login.
   */
  sprintf(buf2, "New connect from [%s]", newd->host);
  mudlog(buf2, CMP, LVL_GOD, FALSE);
#endif

  /* initialize descriptor data */

  newd->descriptor = desc;
  newd->idle_tics = 0;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->input_time = time(0);
  newd->login_time = time(0);
  *newd->output = '\0';
  newd->bufptr = 0;
  newd->has_prompt = 1;  /* prompt is part of greetings */
  newd->connected = CON_CODEPAGE;



 /* STATE(newd) = CON_GET_NAME;*/

  /*
   * This isn't exactly optimal but allows us to make a design choice.
   * Do we embed the history in descriptor_data or keep it dynamically
   * allocated and allow a user defined history size?
   */
  CREATE(newd->history, char *, HISTORY_SIZE);

  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  /* prepend to list */
  newd->next = descriptor_list;
  descriptor_list = newd;

SEND_TO_Q(GREETINGS,newd);

 SEND_TO_Q("\r\n\r\n\r\n\r\n\r\n    &RRule of The Witcher&n\r\n"
          "Please, select your charset:\r\n"
		  " 1. WIN    (Tortilla, JMC)\r\n"
		  " 2. DOS    (for dos)\r\n"  
		  " 3. KOI8   (for unix)\r\n"
		  " 4. MAC    (macintosh)\r\n"
		  " 5. WIN    (zMUD client)\r\n\r\n"
		  "Enter Charset:",newd);

#if defined(HAVE_ZLIB)
  //write_to_descriptor(newd->descriptor, will_sig, strlen(will_sig));
  write_to_descriptor(newd->descriptor, compress_will, strlen(compress_will));

#endif


 return (0);
}


/*int toggle_compression(struct descriptor_data *t)
{
#if defined(HAVE_ZLIB)
  int derr, turn_on = (t->deflate == NULL);

  if (turn_on && !DESC_FLAGGED(t, DESC_CANZLIB))
    return 0;

  if (turn_on) {

    write_to_descriptor(t->descriptor, on_sig, strlen(on_sig));

    // Set up zlib structures. 
    CREATE(t->deflate, z_stream, 1);
    t->deflate->zalloc = zlib_alloc;
    t->deflate->zfree = zlib_free;
    t->deflate->opaque = NULL;
    t->deflate->next_in = t->small_outbuf;
    t->deflate->next_out = t->small_outbuf;
    t->deflate->avail_out = SMALL_BUFSIZE;
    t->deflate->avail_in = 0;

    // Initialize. 
    if ((derr = deflateInit(t->deflate, Z_DEFAULT_COMPRESSION)) != 0)
      log("SYSERR: deflateEnd returned %d.", derr);

    return 1;
  }

  if (!turn_on) { // off 
    int pending = t->deflate->avail_out;

    t->deflate->next_out = t->small_outbuf;

    if ((derr = deflate(t->deflate, Z_FINISH)) != Z_STREAM_END)
      log("SYSERR: deflate returned %d upon Z_FINISH. (in: %d, out: %d)", derr, t->deflate->avail_in, t->deflate->avail_out);

    pending -= t->deflate->avail_out;
    if (pending)
      write_to_descriptor(t->descriptor, t->small_outbuf, 6);

    if ((derr = deflateEnd(t->deflate)) != Z_OK)
      log("SYSERR: deflateEnd returned %d. (in: %d, out: %d)", derr, t->deflate->avail_in, t->deflate->avail_out);

    free(t->deflate);
    t->deflate = NULL;

    return 0;
  }
#endif
  return 0;
} 
  */
/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 * FIXME - This will be rewritten before 3.1, this code is dumb.
 */
int process_output(struct descriptor_data *t)
{
  char i[MAX_SOCK_BUF*2], o[MAX_SOCK_BUF*2], *pi, *po;
  int result, offset=0, written = 0; //, c;
  pi = i;
  po = o;
  /* we may need this \r\n for later -- see below */

	strcpy(i, "\r\n");

	/* now, append the 'real' output */
      strcpy(i + 2, t->output);



  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
    strcat(i, "**ПЕРЕПОЛНЕНИЕ**\r\n");

  /* add the extra CRLF if the person isn't in compact mode */
  if (STATE(t) == CON_PLAYING &&
	  t->character            &&
	  !IS_NPC(t->character)   &&
	  !PRF_FLAGGED(t->character, PRF_COMPACT))
 	  strcat(i, "\r\n");

  /* add a prompt */
   strncat(i, make_prompt(t), MAX_PROMPT_LENGTH);



  switch(t->codepage)
  {case KT_ALT :  for(po = i; *po; po++)
	          *(pi++) =KtoA(*po);
               
        break;
   case 0 :       for(po = i; *po; po++)
	          *(pi++) =KtoW(*po);
               
        break;
   case KT_WINZ: for(po = i; *po; po++)
	         *(pi++) =KtoW2(*po);
              
        break;
   case KT_WINZ6: for(po = i; *po; po++)
	          *(pi++) =KtoW2(*po);
        break;
   default     :  for(po = i; *po; po++)
	          *(pi++) =*po;
        break;
  }
  *po = '\0';
  *pi = '\0';

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
     if (t->has_prompt){
	 if(t->character)
      proc_color(i, (clr(t->character, C_NRM)));
	 else proc_color(i, 1);
	 offset = 0;
	}
	  else {
      if(t->character)
      proc_color(i, (clr(t->character, C_NRM)));
        else proc_color(i, 1);
	  offset = 2;
	 }

   if ( t->character && PLR_FLAGGED(t->character, PLR_GOAHEAD))
        strncat(i, (char*)go_ahead_str, MAX_PROMPT_LENGTH);

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by) {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }
    
  #if defined(HAVE_ZLIB)
  if (t->deflate) {    /* Complex case, compression, write it out. */
    /* Keep compiler happy, and MUD, just in case we don't write anything. */
    result = 1;

    /* First we set up our input data. */
    t->deflate->avail_in = strlen(i + offset);
    t->deflate->next_in = (Bytef *)(i + offset);

    do {
      int df, prevsize = SMALL_BUFSIZE - t->deflate->avail_out;

      /* If there is input or the output has reset from being previously full, run compression again. */
      if (t->deflate->avail_in || t->deflate->avail_out == SMALL_BUFSIZE)
        if ((df = deflate(t->deflate, Z_SYNC_FLUSH)) != 0)
          log("SYSERR: process_output: deflate() returned %d.", df);

      /* There should always be something new to write out. */
      result = write_to_descriptor(t->descriptor, t->small_outbuf + prevsize,
		       SMALL_BUFSIZE - t->deflate->avail_out - prevsize);

      /* Wrap the buffer when we've run out of buffer space for the output. */
      if (t->deflate->avail_out == 0) {
        t->deflate->avail_out = SMALL_BUFSIZE;
        t->deflate->next_out = (Bytef *)(t->small_outbuf);
      }

      /* Oops. This shouldn't happen, I hope. -gg 2/19/99 */
      if (result <= 0)
        return -1;

    /* Need to loop while we still have input or when the output buffer was previously full. */
    } while (t->deflate->avail_out == SMALL_BUFSIZE || t->deflate->avail_in);
  } else
#endif
    result = write_to_descriptor(t->descriptor, i + offset, strlen(i + offset));

  written = result >= 0 ? result : -result;
  
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';
 
// Error, cut off. 
  if (result == 0)
     return (-1);


  /* Normal case, wrote ok. */
  if (result > 0)
     return (1);
 /*
   * We blocked, restore the unwritten output. Known
   * bug in that the snooping immortal will see it twice
   * but details...
   */
  write_to_output(i + written + offset, t);
  return (0); 
}


/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

#if defined(CIRCLE_WINDOWS)

size_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
  //size_t result;

  int result = send(desc, txt, length, 0);

  if (result > 0) {
    /* Write was sucessful */
     number_of_bytes_written += result;
     return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /* result < 0: An error was encountered. */

  /* Transient error? */
  if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
    return (0);

  /* Must be a fatal error. */
  return (-1);
}

#else

#if defined(CIRCLE_ACORN)
#define write	socketwrite
#endif

/* perform_socket_write for all Non-Windows platforms */
size_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
  size_t result;

  result = write(desc, txt, length);

  if (result > 0) {
    /* Write was successful. */
    number_of_bytes_written += result; 
    return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /*
   * result < 0, so an error was encountered - is it transient?
   * Unfortunately, different systems use different constants to
   * indicate this.
   */

#ifdef EAGAIN		/* POSIX */
  if (errno == EAGAIN)
    return (0);
#endif

#ifdef EWOULDBLOCK	/* BSD */
  if (errno == EWOULDBLOCK)
    return (0);
#endif

#ifdef EDEADLK		/* Macintosh */
  if (errno == EDEADLK)
    return (0);
#endif

  /* Looks like the error was fatal.  Too bad. */
  return (-1);
}

#endif /* CIRCLE_WINDOWS */

    
/* write_to_descriptor takes a descriptor, and text to write to the
  * descriptor.  It keeps calling the system-level write() until all
  * the text has been delivered to the OS, or until an error is
  * encountered. 'written' is updated to add how many bytes were sent
  * over the socket successfully prior to the return. It is not zero'd.
  *
  * Returns:
  *  +  All is well and good.
  *  0  A fatal or unexpected error was encountered.
  *  -  The socket write would block.
  */
int write_to_descriptor(socket_t desc, const char *txt, size_t total)
{
  size_t bytes_written, total_written = 0;

  if (total == 0)
    { log("write_to_descriptor: write nothing?!");
      return 0;
    }
  
    while (total > 0) {
    bytes_written = perform_socket_write(desc, txt, total);

    if (bytes_written == (-1)) {
      /* Fatal error.  Disconnect the player. */
      perror("SYSERR: write_to_descriptor");
      return (0);
    } else if (bytes_written == 0) {
      /*
       * Temporary failure -- socket buffer full.  For now we'll just
       * cut off the player, but eventually we'll stuff the unsent
       * text into a buffer and retry the write later.  JE 30 June 98.
       */
      log("WARNING: write_to_descriptor: socket write would block, about to close");
      
      int total_written_result = total_written;
      return -total_written_result;
    } else {
      txt += bytes_written;
      total -= bytes_written;
      total_written += bytes_written;
    }
  }

  return (total_written);
}


/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
size_t perform_socket_read(socket_t desc, char *read_point, size_t space_left)
{
  size_t ret;

#if defined(CIRCLE_ACORN)
  ret = recv(desc, read_point, space_left, MSG_DONTWAIT);
#elif defined(CIRCLE_WINDOWS)
  ret = recv(desc, read_point, space_left, 0);
#else
  ret = read(desc, read_point, space_left);
#endif

  /* Read was successful. */
  if (ret > 0)
   {  number_of_bytes_read += ret;
     return (ret);
   }
  /* read() returned 0, meaning we got an EOF. */
  if (ret == 0) {
   // log("WARNING: EOF on socket read (connection broken by peer)");
    return (-1);
  }

  /*
   * read returned a value < 0: there was an error
   */

#if defined(CIRCLE_WINDOWS)	/* Windows */
  if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
    return (0);
#else

#ifdef EINTR		/* Interrupted system call - various platforms */
  if (errno == EINTR)
    return (0);
#endif

#ifdef EAGAIN		/* POSIX */
  if (errno == EAGAIN)
    return (0);
#endif

#ifdef EWOULDBLOCK	/* BSD */
  if (errno == EWOULDBLOCK)
    return (0);
#endif /* EWOULDBLOCK */

#ifdef EDEADLK		/* Macintosh */
  if (errno == EDEADLK)
    return (0);
#endif

#endif /* CIRCLE_WINDOWS */

  /*
   * We don't know what happened, cut them off. This qualifies for
   * a SYSERR because we have no idea what happened at this point.
   */
  perror("SYSERR: perform_socket_read: about to lose connection");
  return (-1);
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 *
 * Ever wonder why 'tmp' had '+8' on it?  The crusty old code could write
 * MAX_INPUT_LENGTH+1 bytes to 'tmp' if there was a '$' as the final
 * character in the input buffer.  This would also cause 'space_left' to
 * drop to -1, which wasn't very happy in an unsigned variable.  Argh.
 * So to fix the above, 'tmp' lost the '+8' since it doesn't need it
 * and the code has been changed to reserve space by accepting one less
 * character. (Do you really need 256 characters on a line?)
 * -gg 1/21/2000
 */
int process_input(struct descriptor_data *t)
{
  int buf_length, failed_subst;
  size_t bytes_read;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos;// = NULL;
  char tmp[MAX_INPUT_LENGTH];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do {
    if (space_left <= 0) {
      log("WARNING: process_input: about to close connection: input overflow");
      return (-1);
    }

    bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

	if (bytes_read < 0 || bytes_read == 0xffffffff)	/* Error, disconnect them. */
      return (-1);
    else if (bytes_read == 0)	/* Just blocking, no problems. */
      return (0);

    /* at this point, we know we got some data from the read */

    read_point[bytes_read] = '\0';	/* terminate the string */


#if defined(HAVE_ZLIB)
     /* Search for an "Interpret As Command" marker.*/
  /*  for (ptr = read_point; *ptr; ptr++)
      while (!memcmp(ptr, do_sig, DO_SIG_LEN))
            {SET_BIT(DESC_FLAGS(t), DESC_CANZLIB);
             //hack
             memmove(ptr, ptr+DO_SIG_LEN, bytes_read-(ptr-read_point)-DO_SIG_LEN+1);
             bytes_read-=DO_SIG_LEN;
            }*/
	    for (ptr = read_point; *ptr; ptr++)
    {
      if ( ptr[0] != (char)IAC ) continue;
      if ( ptr[1] == (char)IAC )
      {
        // ОНЯКЕДНБЮРЕКЭМНЯРЭ IAC IAC
        // ЯКЕДСЕР ГЮЛЕМХРЭ ОПНЯРН МЮ НДХМ IAC, МН
        // ДКЪ ПЮЯЙКЮДНЙ KT_WIN/KT_WINZ6 ЩРН ОПНХГНИДЕР МХФЕ.
        // оНВЕЛС РЮЙ ЯДЕКЮМН - МЕ ГМЮЧ, МН ГЮЛЕМЪРЭ МЕ АСДС.
        ++ptr;
      }
      else
      if ( ptr[1] == (char)DO )
      {
		if( ptr[2] == (char)TELOPT_COMPRESS ) mccp_start( t, 1 );
		else if( ptr[2] == (char)TELOPT_COMPRESS2 ) mccp_start( t, 2 );
        else continue;
        memmove(ptr, ptr+3, bytes_read-(ptr-read_point)-3+1);
        bytes_read-=3;
		--ptr;
      }
      else
      if ( ptr[1] == (char)DONT )
      {
		if( ptr[2] == (char)TELOPT_COMPRESS ) mccp_end( t, 1 );
		else if( ptr[2] == (char)TELOPT_COMPRESS2 ) mccp_end( t, 2 );
        else continue;
        memmove(ptr, ptr+3, bytes_read-(ptr-read_point)-3+1);
        bytes_read-=3;
		--ptr;
      }
    }
#endif

    /* search for a newline in the data we just read */
    for (ptr = read_point, nl_pos = NULL; *ptr && !nl_pos;)
    {  if (ISNEWL(*ptr))
	nl_pos = ptr;
      ptr++;
    }
read_point += bytes_read;
    space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
#if !defined(POSIX_NONBLOCK_BROKEN)
  } while (nl_pos == NULL);
#else
  } while (0);

  if (nl_pos == NULL)
    return (0);
#endif /* POSIX_NONBLOCK_BROKEN */

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

    /* The '> 1' reserves room for a '$ => $$' expansion. */

    for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++) {
               if (*ptr == ';' && (STATE(t) == CON_PLAYING || (STATE(t) == CON_EXDESC))) {
               if (GET_LEVEL(t->character) < LVL_GOD)
	        *ptr = ',';
	     }
	     if (*ptr == '&' && (STATE(t) == CON_PLAYING || (STATE(t) == CON_EXDESC))) {
               if (GET_LEVEL(t->character) < LVL_IMMORT)
	        *ptr = '8';
	     }
	     if (*ptr == '\\' && (STATE(t) == CON_PLAYING || (STATE(t) == CON_EXDESC))) {
               if (GET_LEVEL(t->character) < LVL_IMMORT)
	        *ptr = '/';
         }
		 if (*ptr == '\b' || *ptr == 127)
		 { /* handle backspacing or delete key */
	      if (write_point > tmp) {
	        if (*(--write_point) == '$')
			{ write_point--;
			  space_left += 2;
			}
			else
			 space_left++;
			}
		}
	  else
		 if  (isascii(*ptr) && isprint(*ptr))
			{  if ((*(write_point++) = *ptr) == '$')
				{	/* copy one character */
					*(write_point++) = '$';	/* if it's a $, double it */
					space_left -= 2;
				}
		        else
			space_left--;
		        }
             else
			if ((ubyte) *ptr > 127)
			{
			switch (t->codepage)
	               {default:
 	                 t->codepage = 0;
					 break;
	                case 2:
	                 *(write_point++) = *ptr; //KOI-8
	      	         break;
   	                case KT_ALT:
 	      	         *(write_point++) = AtoK(*ptr);
  	      	         break;
	                 case /*KT_WIN*/0:
	                 case KT_WINZ6:
	                 *(write_point++) = WtoK(*ptr);
	                 if (*ptr == (char) 255 && *(ptr+1) == (char) 255 && ptr+1 < nl_pos)
	                    ptr++;
	                 break;
	                case KT_WINZ:
	      	         *(write_point++) = WtoK(*ptr);
	      	         break;
	      	       }
      	space_left--;
      	}
    if (STATE(t) == CON_PLAYING || (STATE(t) == CON_EXDESC))
	{ if (t->codepage == KT_WINZ6 || t->codepage == KT_WINZ)
		{ if (*(write_point-1) == 'z')
			*(write_point-1) = 'я';
		}
	}
    }

   *write_point = '\0';


    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      sprintf(buffer, "Слишком длинная строка.  Усечена до:\r\n%s\r\n", tmp);
      SEND_TO_Q(buffer, t);
	//return (-1);
    }
    if (t->snoop_by) {
      SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(tmp, t->snoop_by);
      SEND_TO_Q("\r\n", t->snoop_by);
    }
    failed_subst = 0;

    if ( ((tmp[0] == '~') && (tmp[1] == 0)) || (!strn_cmp(tmp, "очистить", 4)) )
    {
        // очистка входной очереди
	    int dummy;
   	    while (get_from_q(&t->input, buf2, &dummy));
  	    tmp[0] = 0;
        send_to_char(t->character, "Очередь команд очищена.\r\n");
	}
   else
      if (*tmp == '!' && !(*(tmp + 1)))	/* Redo last command. */
          strcpy(tmp, t->last_input);
      else
      if (*tmp == '!' && *(tmp + 1))
	 { char *commandln = (tmp + 1);
           int starting_pos = t->history_pos,
	   cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

      skip_spaces(&commandln);
      for (; cnt != starting_pos; cnt--) {
	if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
	  strcpy(tmp, t->history[cnt]);
	  strcpy(t->last_input, tmp);
          SEND_TO_Q(tmp, t); SEND_TO_Q("\r\n", t);
	  break;
	}
        if (cnt == 0)	/* At top, loop to bottom. */
	  cnt = HISTORY_SIZE;
      }
    } else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	strcpy(t->last_input, tmp);
    } else {
      strcpy(t->last_input, tmp);
      if (t->history[t->history_pos])
	free(t->history[t->history_pos]);	/* Clear the old line. */
      t->history[t->history_pos] = str_dup(tmp);	/* Save the new. */
      if (++t->history_pos >= HISTORY_SIZE)	/* Wrap to top. */
	t->history_pos = 0;
    }

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return (1);
}



/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char newsub[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Недопустимая замена.\r\n", t); /*Invalid substitution*/
    return (1);
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Недопустимая замена.\r\n", t);/*Invalid substitution*/
    return (1);
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(newsub, orig, (strpos - orig));
  newsub[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(newsub, second, (MAX_INPUT_LENGTH - strlen(newsub) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(newsub, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(newsub) - 1));

  /* terminate the string in case of an overflow from strncat */
  newsub[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, newsub);

  return (0);
}


#define IS_QUESTOR(ch)     (PLR_FLAGGED(ch, PLR_QUESTOR))
void close_socket(struct descriptor_data *d, int direct)
{
  char buf[128];
  struct descriptor_data *temp;
 
  if (!direct && d->character && RENTABLE(d->character))
     return;

  REMOVE_FROM_LIST(d, descriptor_list, next);
  CLOSE_SOCKET(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    SEND_TO_Q("Ваша цель слежения покинула связь.\r\n", d->snoop_by);
    d->snoop_by->snooping = NULL;
  }

/* switch(d->connected)
  { case CON_OEDIT:
    case CON_REDIT:
    case CON_ZEDIT:
    case CON_MEDIT:
    case CON_SEDIT:
    case CON_TRIGEDIT:
      cleanup_olc(d, CLEANUP_ALL);
    default:
      break;
  }*/


  if (d->character) {
    /*
     * Plug memory leak, from Eric Green.
     */
   
  if (!IS_NPC(d->character) && PLR_FLAGGED(d->character, PLR_MAILING) && d->str) {
      if (*(d->str))
        free(*(d->str));
      free(d->str);
    }
    if (STATE(d) == CON_PLAYING || STATE(d) == CON_DISCONNECT) {
      act("$n потерял$y связь.", TRUE, d->character, 0, 0, TO_ROOM);
      if (!IS_NPC(d->character)) {
	save_char(d->character, NOWHERE);
        check_light(d->character,LIGHT_NO,LIGHT_NO,LIGHT_NO,LIGHT_NO,-1); 
        Crash_ldsave(d->character); //запись в рентфайл вещей
	sprintf(buf, "Потеря связи %s.", GET_TNAME(d->character)); 
	mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
        strncpy(d->character->player_specials->temphost, d->character->desc->host, HOST_LENGTH);
      }
      d->character->desc = NULL;
    } else {
      sprintf(buf, "Отключение соединения %s.",
	      GET_TNAME(d->character) ? GET_TNAME(d->character) : "<неопределенным пользователем>");
      mudlog(buf, CMP, LVL_IMMORT, TRUE);
      free_char(d->character);
    }
  }
   // else
   // mudlog("Losing descriptor without char.", CMP, LVL_IMMORT, TRUE);

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

  /* Clear the command history. */
  if (d->history) {
    int cnt;
    for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
      if (d->history[cnt])
	free(d->history[cnt]);
    free(d->history);
  }

  if (d->showstr_head)
    free(d->showstr_head);
  if (d->showstr_count)
    free(d->showstr_vector);


#if defined(HAVE_ZLIB)
  if (d->deflate) {
    deflateEnd(d->deflate);
    free(d->deflate);
  }
#endif
  free(d);
}



void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME
     && STATE(d) != CON_CODEPAGE && STATE(d) != CON_CREAT_NAME)
		continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      continue;
    } else {
      echo_on(d);
      SEND_TO_Q("\r\nВремя ожидания вышло... До свидания.\r\n", d);
      STATE(d) = CON_CLOSE;
    }
  }
}



/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#if defined(CIRCLE_WINDOWS)

void nonblock(socket_t s)
{
  unsigned long val = 1;
  ioctlsocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_AMIGA)

void nonblock(socket_t s)
{
  long val = 1;
  IoctlSocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_ACORN)

void nonblock(socket_t s)
{
  int val = 1;
  socket_ioctl(s, FIONBIO, &val);
}

#elif defined(CIRCLE_VMS)

void nonblock(socket_t s)
{
  int val = 1;

  if (ioctl(s, FIONBIO, &val) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

#elif defined(CIRCLE_UNIX) || defined(CIRCLE_OS2) || defined(CIRCLE_MACINTOSH)

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s)
{
  int flags;

  flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

#endif  /* CIRCLE_UNIX || CIRCLE_OS2 || CIRCLE_MACINTOSH */


/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)

RETSIGTYPE reread_wizlists(int sig)
{
  mudlog("Signal received - rereading wizlists.", CMP, LVL_IMMORT, TRUE);
  reboot_wizlists();
}


RETSIGTYPE unrestrict_game(int sig)
{
  mudlog("Received SIGUSR2 - completely unrestricting game (emergent)",
	 BRF, LVL_IMMORT, TRUE);
  ban_list = NULL;
  circle_restrict = 0;
  num_invalid = 0;
}

#ifdef CIRCLE_UNIX

/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);

  my_signal(SIGCHLD, reap);
}

RETSIGTYPE checkpointing(int sig)
{
  if (!tics) {
    log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
    abort();
  } else
    tics = 0;
}

RETSIGTYPE hupsig(int sig)
{
  log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  exit(1);			/* perhaps something more elegant should
				 * substituted */
}

#endif	/* CIRCLE_UNIX */

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef POSIX      //NE WCLUCHAETSIA posix!!! OTKLUCHIL VREMENNO
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;	// SunOS
#endif

  if (sigaction(signo, &act, &oact) < 0)
    return (SIG_ERR);

  return (oact.sa_handler);
}
#endif				/* POSIX */


void signal_setup(void)
{
#ifndef CIRCLE_MACINTOSH
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  my_signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.
   */
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGCHLD, reap);
#endif /* CIRCLE_MACINTOSH */
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
}

#endif	/* CIRCLE_UNIX || CIRCLE_MACINTOSH */

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */
void send_to_char(CHAR_DATA * ch, const char *messg, ...)
{
	if (!ch)
		return;
	va_list args;
	char tmpbuf[MAX_STRING_LENGTH];

	va_start(args, messg);
	vsprintf(tmpbuf, messg, args);
	va_end(args);

	if (ch->desc && messg)
		SEND_TO_Q(tmpbuf, ch->desc);
}


//Вывод строк средствами С++, разобратся перенести в линух, в дальнейшем перенести в этом формате все сообщения. 25.12.2006г.
void send_to_char(const std::string & buffer, CHAR_DATA * ch)
{
	if (ch->desc && !buffer.empty())
		SEND_TO_Q(buffer.c_str(), ch->desc);
}



void send_to_char(const char *messg, struct char_data *ch)
{
  if (ch && ch->desc && messg)
    SEND_TO_Q(messg, ch->desc);
}


void send_to_all(const char *messg)
{
  struct descriptor_data *i;

  if (messg == NULL)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (STATE(i) == CON_PLAYING)
      SEND_TO_Q(messg, i);
}


void send_to_outdoor(const char *messg, int control)
{ int    room;
  struct descriptor_data *i;

  if (!messg || !*messg)
     return;

  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) != CON_PLAYING || i->character == NULL)
          continue;
       if (!AWAKE(i->character)    || !OUTSIDE(i->character))
          continue;
       room = IN_ROOM(i->character);
       if (!control                        ||
           (IS_SET(control, SUN_CONTROL)  &&
	    room       != NOWHERE         &&
	    SECT(room) != SECT_UNDERWATER &&
	    !AFF_FLAGGED(i->character, AFF_BLIND)
	   )                               ||
	   (IS_SET(control, WEATHER_CONTROL)    &&
	    room       != NOWHERE               &&
	    SECT(room) != SECT_UNDERWATER       &&
	    !ROOM_FLAGGED(room, ROOM_NOWEATHER) &&
            world[IN_ROOM(i->character)].weather.duration <= 0
	   )
	  )
          SEND_TO_Q(messg, i);
      }
}


void send_to_gods(const char *messg)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
     return;

  for (i = descriptor_list; i; i = i->next)
      {if (STATE(i) != CON_PLAYING || i->character == NULL)
          continue;
       if (!IS_GOD(i->character))
          continue;
       SEND_TO_Q(messg, i);
      }
}

void send_to_room(const char *messg, room_rnum room, int to_awake)
{
  struct char_data *i;

  if (messg == NULL && room != NOWHERE)
    return;

  for (i = world[room].people; i; i = i->next_in_room)
    if (i->desc && !IS_NPC(i) && (!to_awake || AWAKE(i)))
      SEND_TO_Q(messg, i->desc);
}



const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);
#define GET_VSEX  GET_SEX((const struct char_data *) vict_obj)

/* higher-level communication: the act() function */
void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj,
		const void *vict_obj, struct char_data *to)
{
  const char *i = NULL;
  char lbuf[MAX_STRING_LENGTH], *buf;
  struct char_data *dg_victim = NULL;
  struct obj_data *dg_target = NULL;
  char  *dg_arg = NULL;

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'c':
    i = GET_NAME(ch);
      break;
      case 'n':
	i = PERS(ch, to);
	  break;
	  case 'r':
	i = RPERS(ch, to);
	break;
      case 'v':
	i = VPERS(ch, to);
	break;
      case 'd':
	i = DPERS(ch, to);
	break;
      case 't':
	i = TPERS(ch, to);
	break;
     case 'p':
	i = PPERS(ch, to);
	break;
      case 'N':
	CHECK_NULL(vict_obj, PERS((const struct char_data *) vict_obj, to));
	dg_victim = (struct char_data *) vict_obj;
	break;
      case 'R':
   CHECK_NULL(vict_obj, RPERS((const struct char_data *) vict_obj, to));
   dg_victim = (struct char_data *) vict_obj;
   break;
      case 'D':
   CHECK_NULL(vict_obj, DPERS((const struct char_data *) vict_obj, to));
    dg_victim = (struct char_data *) vict_obj;
   break;
      case 'V':
   CHECK_NULL(vict_obj, VPERS((const struct char_data *) vict_obj, to));
    dg_victim = (struct char_data *) vict_obj;
   break;
      case 'T':
   CHECK_NULL(vict_obj, TPERS((const struct char_data *) vict_obj, to));
    dg_victim = (struct char_data *) vict_obj;
   break;
      case 'P':
   CHECK_NULL(vict_obj, PPERS((const struct char_data *) vict_obj, to));
    dg_victim = (struct char_data *) vict_obj;
   break;
      case 'm':
	i = HMHR(ch);
	break;
      case 'M':
           if (vict_obj)
           i = HMHR((const struct char_data *) vict_obj);
           else
           CHECK_NULL(obj,OMHR(obj));
           dg_victim = (struct char_data *) vict_obj;
			break;
      case 's':
			i = HSHR(ch);
			break;
      case 'S':
            if (vict_obj)
            i = HSHR((const struct char_data *) vict_obj);
            else
            CHECK_NULL(obj,OSHR(obj));
            dg_victim = (struct char_data *) vict_obj;	
			break;
      case 'e':
			i = HSSH(ch);
			break;
      case 'E':
			if (vict_obj)
            		i = HSSH((const struct char_data *) vict_obj);
            		else
            		CHECK_NULL(obj,OSSH(obj));
            		dg_victim = (struct char_data *) vict_obj;	
			break;
      case 'b':
			CHECK_NULL(obj, (const char *) obj);
			break;
      case 'B':
			CHECK_NULL(vict_obj, (const char *) vict_obj);
			break;
      case 'a':
			CHECK_NULL(obj, OBJN(obj, to));
			break;
      case 'A':
			CHECK_NULL(vict_obj, OBJN((const struct obj_data *) vict_obj, to));
			dg_victim = (struct char_data *) vict_obj;
			break;
      case 'o':
			CHECK_NULL(obj, OBJS(obj, to));
			break;
      case '1':
			CHECK_NULL(obj, ROBJS(obj, to));
			break;
      case '2':
			CHECK_NULL(obj, DOBJS(obj, to));
			break;
      case '3':
			CHECK_NULL(obj, VOBJS(obj, to));
			break;
      case '4':
			CHECK_NULL(obj, TOBJS(obj, to));
			break;
      case '5':
			CHECK_NULL(obj, POBJS(obj, to));
			break;
      case 'O':
		       CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to));
			dg_victim = (struct char_data *) vict_obj;
			break;
      case '6':
			CHECK_NULL(vict_obj, ROBJS((const struct obj_data *) vict_obj, to));
			dg_victim = (struct char_data *) vict_obj;
			break;
      case '7':
 			CHECK_NULL(vict_obj, DOBJS((const struct obj_data *) vict_obj, to));
			dg_victim = (struct char_data *) vict_obj;
			break;
     case '8':
			CHECK_NULL(vict_obj, VOBJS((const struct obj_data *) vict_obj, to));
			dg_victim = (struct char_data *) vict_obj;
			break;
     case '9':
			CHECK_NULL(vict_obj, TOBJS((const struct obj_data *) vict_obj, to));
			dg_victim = (struct char_data *) vict_obj;
			break;
     case '0':
			CHECK_NULL(vict_obj, POBJS((const struct obj_data *) vict_obj, to));
			dg_victim = (struct char_data *) vict_obj;
			break;
     case 'F':
			CHECK_NULL(vict_obj, fname((const char *) vict_obj));
			break;
     case 'y':
			i = GET_CH_VIS_SUF_1(ch, to);
			break;
    case 'Y':
			if (vict_obj)
        			i = GET_CH_VIS_SUF_1((const struct char_data *) vict_obj,to);
			else CHECK_NULL(obj,GET_OBJ_VIS_SUF_1(obj, to));
        		dg_victim = (struct char_data *) vict_obj;
    			break;
    case 'u':
			i = GET_CH_VIS_SUF_2(ch, to);
	break;
	case 'U':
        if (vict_obj)
        i = GET_CH_VIS_SUF_2((const struct char_data *) vict_obj,to);
            else CHECK_NULL(obj,GET_OBJ_VIS_SUF_2(obj,to));
        dg_victim = (struct char_data *) vict_obj;
    break;
	case 'q':
		i = GET_CH_VIS_SUF_4(ch, to);
	break;
	case 'Q':
        if (vict_obj)
        i = GET_CH_VIS_SUF_4((const struct char_data *) vict_obj,to);
             else CHECK_NULL(obj,GET_OBJ_VIS_SUF_4(obj,to));
        dg_victim = (struct char_data *) vict_obj;
    break;
	case 'w':
		i = GET_CH_VIS_SUF_3(ch, to);
	break;
	case 'W':    
	    if (vict_obj)
        i = GET_CH_VIS_SUF_3((const struct char_data *) vict_obj,to);
			else CHECK_NULL(obj,GET_OBJ_VIS_SUF_3(obj,to));
        dg_victim = (struct char_data *) vict_obj;
    break;
    case 'g':
		i = GET_CH_VIS_SUF_6(ch, to);
	break;
	case 'G':
       if (vict_obj)
        i = GET_CH_VIS_SUF_6((const struct char_data *) vict_obj, to);
			else CHECK_NULL(obj,GET_OBJ_VIS_SUF_6(obj, to));
        dg_victim = (struct char_data *) vict_obj;
			
    break;
	case 'i':
		i = GET_CH_VIS_SUF_5(ch, to);
	break;
	case 'I':
        if (vict_obj)
        i = GET_CH_VIS_SUF_5((const struct char_data *) vict_obj,to);
			else CHECK_NULL(obj,GET_OBJ_VIS_SUF_5(obj,to));
        dg_victim = (struct char_data *) vict_obj;
   break;
        case 'k':
		i = GET_CH_VIS_SUF_7(ch, to);
	break;
	case 'K':
        if (vict_obj)
        i = GET_CH_VIS_SUF_7((const struct char_data *) vict_obj,to);
			else CHECK_NULL(obj,GET_OBJ_VIS_SUF_7(obj,to));
        dg_victim = (struct char_data *) vict_obj;
   break;
	case 'z':
		i = (obj->obj_flags.pol == SEX_MALE ? "ым": obj->obj_flags.pol == SEX_NEUTRAL ? "ое":"ой");
	break;
        case 'x':
			i = GET_CH_VIS_SUF_8(ch, to);
		break;
    case 'X':
          if (vict_obj)
        i = GET_CH_VIS_SUF_8((const struct char_data *) vict_obj,to);
			else CHECK_NULL(obj,GET_OBJ_VIS_SUF_8(obj,to));
        dg_victim = (struct char_data *) vict_obj;
		break;

	case '$':
	i = "$";
	break;
      default:
	log("SYSERR: Illegal $-code to act(): %c", *orig);
	log("SYSERR: %s", orig);
	i = "";
	break;
      }
      while ((*buf = *(i++)))
	buf++;
      orig++;
    } else if (!(*(buf++) = *(orig++)))
      break;
  }

  char *x = buf; x-=2;
  if (*x != 0xd && *x != 0xa)
  {
      *(--buf) = '\r';
      *(++buf) = '\n';
      *(++buf) = '\0';
  }

 // SEND_TO_Q(CAP(lbuf), to->desc);
   if (to->desc) {
     /* Делаем первый символ большим, учитывая &X */
     if (lbuf[0] == '&')
       CAP(lbuf+2);
     SEND_TO_Q(CAP(lbuf), to->desc);
  }


if ((IS_NPC(to) && dg_act_check) && (to != ch))
     act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);
}




void act(const char *str, int hide_invisible, struct char_data *ch,
	 struct obj_data *obj, const void *vict_obj, int type)
{
  struct char_data *to;
  int to_sleeping, check_deaf, stopcount;

  if (!str || !*str)
    return;

  if (!(dg_act_check = !(type & DG_NO_TRIG)))
     type &= ~DG_NO_TRIG;
	/*
   * Warning: the following TO_SLEEP code is a hack.
   * 
   * I wanted to be able to tell act to deliver a message regardless of sleep
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
   * command.  It's not legal to combine TO_x's with each other otherwise.
   * TO_SLEEP only works because its value "happens to be" a single bit;
   * do not change it to something else.  In short, it is a hack.
   */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((to_sleeping = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;
  
  if ((check_deaf = (type & CHECK_DEAF)))
     type &= ~CHECK_DEAF;

  if (type == TO_CHAR) {
    if (ch && SENDOK(ch))
      perform_act(str, ch, obj, vict_obj, ch);
    return;
  }

  if (type == TO_VICT) {
    if ((to = (struct char_data *) vict_obj) != NULL && SENDOK(to))
      perform_act(str, ch, obj, vict_obj, to);
    return;
  }
  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (ch && ch->in_room != NOWHERE)
    to = world[ch->in_room].people;
  else if (obj && obj->in_room != NOWHERE)
    to = world[obj->in_room].people;
  else {
    log("SYSERR: no valid target to act()!");
    return;
  }


 for (stopcount = 0; to && stopcount < 1000; to = to->next_in_room, stopcount++)
      {if (!SENDOK(to) || (to == ch))
          continue;
       if (hide_invisible && ch && !CAN_SEE(to, ch))
          continue;
       if ((type != TO_ROOM && type != TO_ROOM_HIDE) && to == vict_obj)
          continue;
//надо отдельно PRF_DEAF
/*       if (!IS_NPC(to) && check_deaf && PRF_FLAGGED(to, PRF_NOTELL))
          continue; */
       if (type == TO_ROOM_HIDE && !AFF_FLAGGED(to, AFF_SENSE_LIFE))
          continue;
       perform_act(str, ch, obj, vict_obj, to);
      }
} 
  /*  for (; to; to = to->next_in_room) {
    if (!SENDOK(to) || (to == ch))
      continue;
    if (hide_invisible && ch && !CAN_SEE(to, ch))
      continue;
    if (type != TO_ROOM && to == vict_obj)
      continue;
    perform_act(str, ch, obj, vict_obj, to);
  }
}*/

/*
 * This function is called every 30 seconds from heartbeat().  It checks
 * the four global buffers in CircleMUD to ensure that no one has written
 * past their bounds.  If our check digit is not there (and the position
 * doesn't have a NUL which may result from snprintf) then we gripe that
 * someone has overwritten our buffer.  This could cause a false positive
 * if someone uses the buffer as a non-terminated character array but that
 * is not likely. -gg
 */
void sanity_check(void)
{
  int ok = TRUE;

  /*
   * If any line is false, 'ok' will become false also.
   */
  ok &= (test_magic(buf)  == MAGIC_NUMBER || test_magic(buf)  == '\0');
  ok &= (test_magic(buf1) == MAGIC_NUMBER || test_magic(buf1) == '\0');
  ok &= (test_magic(buf2) == MAGIC_NUMBER || test_magic(buf2) == '\0');
  ok &= (test_magic(arg)  == MAGIC_NUMBER || test_magic(arg)  == '\0');

  /*
   * This isn't exactly the safest thing to do (referencing known bad memory)
   * but we're doomed to crash eventually, might as well try to get something
   * useful before we go down. -gg
   * However, lets fix the problem so we don't spam the logs. -gg 11/24/98
   */
  if (!ok) {
    log("SYSERR: *** Buffer overflow! ***\n"
	"buf: %s\nbuf1: %s\nbuf2: %s\narg: %s", buf, buf1, buf2, arg);

    plant_magic(buf);
    plant_magic(buf1);
    plant_magic(buf2);
    plant_magic(arg);
  }

#if 0
  log("Statistics: buf=%d buf1=%d buf2=%d arg=%d",
	strlen(buf), strlen(buf1), strlen(buf2), strlen(arg));
#endif
}

/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd)
{
  FILE *s_fp;

#if defined(__MWERKS__) || defined(__GNUC__)
  s_fp = stderr;
#else
  if ((s_fp = fdopen(STDERR_FILENO, "w")) == NULL) {
    puts("SYSERR: Error opening stderr, trying stdout.");

    if ((s_fp = fdopen(STDOUT_FILENO, "w")) == NULL) {
      puts("SYSERR: Error opening stdout, trying a file.");

      /* If we don't have a file, try a default. */
      if (filename == NULL || *filename == '\0')
        filename = "log/syslog";
    }
  }
#endif

  if (filename == NULL || *filename == '\0') {
    /* No filename, set us up with the descriptor we just opened. */
    logfile = s_fp;
    puts("Using file descriptor for logging.");
    return;
  }

  /* We honor the default filename first. */
  if (open_logfile(filename, s_fp))
    return;

  /* Well, that failed but we want it logged to a file so try a default. */
  if (open_logfile("log/syslog", s_fp))
    return;

  /* Ok, one last shot at a file. */
  if (open_logfile("syslog", s_fp))
    return;

  /* Erp, that didn't work either, just die. */
  puts("SYSERR: Couldn't open anything to log to, giving up.");
  exit(1);
}

int open_logfile(const char *filename, FILE *stderr_fp)
{
  if (stderr_fp)	/* freopen() the descriptor. */
    logfile = freopen(filename, "w", stderr_fp);
  else
    logfile = fopen(filename, "w");

  if (logfile) {
    printf("Using log file '%s'%s.\n",
		filename, stderr_fp ? " with redirection" : "");
    return (TRUE);
  }

  printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
  return (FALSE);
}

/*
 * This may not be pretty but it keeps game_loop() neater than if it was inline.
 */
#if defined(CIRCLE_WINDOWS)

void circle_sleep(struct timeval *timeout)
{
  Sleep(timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
}

#else

void circle_sleep(struct timeval *timeout)
{
  if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
    if (errno != EINTR) {
      perror("SYSERR: Select sleep");
      exit(1);
    }
  }
}

#endif /* CIRCLE_WINDOWS */

#if defined(HAVE_ZLIB)

/* Compression stuff. */

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
    return calloc(items, size);
}

void zlib_free(void *opaque, void *address)
{
    free(address);
}

#endif

#if defined(HAVE_ZLIB)

int mccp_start( struct descriptor_data *t, int ver )
{
  int derr;

  if ( t->deflate ) 
    return 1; // компрессия уже включена

  /* Set up zlib structures. */
  CREATE(t->deflate, z_stream, 1);
  t->deflate->zalloc    = zlib_alloc;
  t->deflate->zfree     = zlib_free;
  t->deflate->opaque    = NULL;
  t->deflate->next_in   = (Bytef *)(t->small_outbuf);
  t->deflate->next_out  = (Bytef *)t->small_outbuf;
  t->deflate->avail_out = SMALL_BUFSIZE;
  t->deflate->avail_in  = 0;

  /* Initialize. */
  if ((derr = deflateInit(t->deflate, Z_DEFAULT_COMPRESSION)) != 0)
  {
    log("SYSERR: deflateEnd returned %d.", derr);
    free( t->deflate );
    t->deflate = NULL;
    return 0; 
  }

  if( ver != 2 )
    write_to_descriptor( t->descriptor, 
                         compress_start_v1,
			             strlen( compress_start_v1 ) 
                       );
  else
    write_to_descriptor( t->descriptor, 
                         compress_start_v2,
			             strlen( compress_start_v2 ) 
                       );

  t->mccp_version = ver;
  return 1;
}


int mccp_end( struct descriptor_data *t, int ver )
{
  int derr;
  int prevsize, pending;
  unsigned char tmp[1];

  if ( t->deflate == NULL ) 
    return 1;

  if ( t->mccp_version != ver )
    return 0;

  t->deflate->avail_in = 0;
  t->deflate->next_in = tmp;
  prevsize = SMALL_BUFSIZE - t->deflate->avail_out;

  log("SYSERR: about to deflate Z_FINISH." );

  if ((derr = deflate(t->deflate, Z_FINISH)) != Z_STREAM_END)
  {
    log("SYSERR: deflate returned %d upon Z_FINISH. (in: %d, out: %d)", derr, t->deflate->avail_in, t->deflate->avail_out);
    return 0;
  }

  pending = SMALL_BUFSIZE - t->deflate->avail_out - prevsize;

  if( !write_to_descriptor( t->descriptor, 
                            t->small_outbuf+prevsize,
			                pending )
    )
    return 0;

  if ((derr = deflateEnd(t->deflate)) != Z_OK)
    log("SYSERR: deflateEnd returned %d. (in: %d, out: %d)", derr, t->deflate->avail_in, t->deflate->avail_out);

  free(t->deflate);
  t->deflate = NULL;

  return 1;  
}
#endif

int toggle_compression(struct descriptor_data *t)
{
#if defined(HAVE_ZLIB)
  if ( t->mccp_version == 0 ) return 0;
  if ( t->deflate == NULL ) 
  {
    return mccp_start( t, t->mccp_version ) ? 1 : 0;
  }
  else
  {
    return mccp_end( t, t->mccp_version ) ? 0 : 1;
  }
#endif
  return 0;
}
