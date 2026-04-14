/***************************************************************************
*                           STAR WARS REALITY 1.0                          *
*--------------------------------------------------------------------------*
* Star Wars Reality Code Additions and changes from the Smaug Code         *
* copyright (c) 1997 by Sean Cooper                                        *
* -------------------------------------------------------------------------*
* Starwars and Starwars Names copyright(c) Lucas Film Ltd.                 *
*--------------------------------------------------------------------------*
* SMAUG 1.0 (C) 1994, 1995, 1996 by Derek Snider                           *
* SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,                    *
* Scryn, Rennard, Swordbearer, Gorog, Grishnakh and Tricops                *
* ------------------------------------------------------------------------ *
* Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
* Chastain, Michael Quan, and Mitchell Tse.                                *
* Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
* Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
* ------------------------------------------------------------------------ *
*			 Low-level communication module			   *
****************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#include "mud.h"
#ifdef DNS_SLAVE /* DNS Slave stuff */
#include "dns_slave.h"
#endif

#ifndef isascii
#define isascii(c) (((unsigned char)(c)) <= 127)
#endif

#ifndef TELOPT_MAX
#define TELOPT_MAX 256
#endif
#define TELOPT_GMCP 201
#define TELOPT_MSSP 70

typedef enum
{
    TELPOLICY_IGNORE = 0,  /* Do nothing */
    TELPOLICY_ACCEPT,      /* Accept and proceed */
    TELPOLICY_REJECT       /* Explicitly refuse */
} telnet_policy_t;

#define LOGTELNET

/*
 * Socket and TCP/IP stuff.
 */

#ifndef _WIN32
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <netdb.h>
#endif

#define PAGEPOINT_NULL ((size_t)-1) 

const	unsigned char	echo_off_str	[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const	unsigned char	echo_on_str	[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const	unsigned char 	go_ahead_str	[] = { IAC, GA, '\0' };

#ifdef MCCP
#define COMPRESS_BUF_SIZE 8192

#define TELOPT_COMPRESS 85
#define TELOPT_COMPRESS2 86
const   unsigned char    eor_on_str      [] = { IAC, WILL, TELOPT_EOR, '\0' };
const   unsigned char    compress_on_str [] = { IAC, WILL, TELOPT_COMPRESS, '\0' };
const   unsigned char    compress2_on_str [] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };

bool    compressStart   args( ( DESCRIPTOR_DATA *d, unsigned char telopt ) );
bool    compressEnd     args( ( DESCRIPTOR_DATA *d ) );
#endif

bool bootup = FALSE;

void    send_auth args( ( struct descriptor_data *d ) );
void    read_auth args( ( struct descriptor_data *d ) );
void    start_auth args( ( struct descriptor_data *d ) );
void    save_sysdata args( ( SYSTEM_DATA sys ) );

/*  from act_info?  */
void    show_condition( CHAR_DATA *ch, CHAR_DATA *victim );

/*
 * Global variables.
 */
int player_count = 0;
DESCRIPTOR_DATA *   first_descriptor;	/* First descriptor		*/
DESCRIPTOR_DATA *   last_descriptor;	/* Last descriptor		*/
DESCRIPTOR_DATA *   d_next;		/* Next descriptor in loop	*/
int		    num_descriptors;
FILE *		    fpReserve;		/* Reserved file handle		*/
bool		    mud_down;		/* Shutdown			*/
bool		    wizlock;		/* Game is wizlocked		*/
time_t              boot_time;
HOUR_MIN_SEC  	    set_boot_time_struct;
HOUR_MIN_SEC *      set_boot_time;
struct tm *         new_boot_time;
struct tm           new_boot_struct;
char		    str_boot_time[MAX_INPUT_LENGTH];
char		    lastplayercmd[MAX_INPUT_LENGTH*2];
time_t		    current_time;	/* Time of this pulse		*/
int		    control;		/* Controlling descriptor	*/
int		    control2;		/* Controlling descriptor #2	*/
int		    conclient;		/* MUDClient controlling desc	*/
int		    conjava;		/* JavaMUD controlling desc	*/
int		    newdesc;		/* New descriptor		*/
fd_set		    in_set;		/* Set of desc's for reading	*/
fd_set		    out_set;		/* Set of desc's for writing	*/
fd_set		    exc_set;		/* Set of desc's with errors	*/
int 		    maxdesc;
#ifdef DNS_SLAVE
pid_t slave_pid;
static int slave_socket = -1;
#endif
static volatile sig_atomic_t g_alarm_fired = 0;
static volatile sig_atomic_t g_pending_accept_fd = -1;


/*
 * OS-dependent local functions.
 */
void	game_loop		args( (GameContext *game) );
int	init_socket		args( ( int port ) );
void	new_descriptor		args( ( GameContext *game, int new_desc ) );
bool	read_from_descriptor	args( ( DESCRIPTOR_DATA *d ) );


/*
 * Other local functions (OS-independent).
 */
void send_mssp(DESCRIPTOR_DATA *d);
ssize_t net_write(DESCRIPTOR_DATA *d, const unsigned char *data, size_t len);
void send_telnet(DESCRIPTOR_DATA *d, const unsigned char *data, size_t len);
void output_to_descriptor(DESCRIPTOR_DATA *d, const std::string& txt);
void output_to_descriptor_char(DESCRIPTOR_DATA *d, const char *txt);
void write_to_buffer_str(DESCRIPTOR_DATA *d, const char *txt);
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length );
std::string wrap_text_smart( const std::string& txt, int width );
size_t visible_length(const char *txt);
size_t visible_length(const std::string&txt);
std::string wrap_text_ex( const std::string& txt, int width, int flags, int indent );
bool process_compressed(DESCRIPTOR_DATA *d);
bool	check_parse_name	args( ( const std::string& name ) );
short	check_reconnect		args( ( DESCRIPTOR_DATA *d, const std::string& name,
				    bool fConn ) );
short	check_playing		args( ( DESCRIPTOR_DATA *d, const std::string& name, bool kick ) );
bool	check_multi		args( ( DESCRIPTOR_DATA *d, const std::string& name ) );
int	main			args( ( int argc, char **argv ) );
void	nanny			args( ( DESCRIPTOR_DATA *d, const std::string& argument ) );
bool	flush_buffer		args( ( DESCRIPTOR_DATA *d, bool fPrompt ) );
void	read_from_buffer	args( ( DESCRIPTOR_DATA *d ) );
void	stop_idling		args( ( CHAR_DATA *ch ) );
void	free_desc		args( ( DESCRIPTOR_DATA *d ) );
void	display_prompt		args( ( DESCRIPTOR_DATA *d ) );
int make_color_sequence(const char *col, char *buf, DESCRIPTOR_DATA *d,  int *consumed);
void	set_pager_input		args( ( DESCRIPTOR_DATA *d, const std::string& argument ) );
bool	pager_output		args( ( DESCRIPTOR_DATA *d ) );

/* Nanny forward declarations */
static void nanny_get_name( DESCRIPTOR_DATA *d, const std::string& argumentin );
static void nanny_get_old_password( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_confirm_new_name( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_get_new_password( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_confirm_new_password( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_get_new_sex( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_get_new_race( DESCRIPTOR_DATA *d, const std::string& argumentin );
static void nanny_get_new_class( DESCRIPTOR_DATA *d, const std::string& argumentin );
static void nanny_roll_stats( DESCRIPTOR_DATA *d );
static void nanny_stats_ok( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_get_want_ripansi( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_get_msp( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_press_enter( DESCRIPTOR_DATA *d, const std::string& argument );
static void nanny_read_motd( DESCRIPTOR_DATA *d, const std::string& argument );

void	mail_count		args( ( CHAR_DATA *ch ) );
void    cron();

int main( int argc, char **argv )
{
    struct timeval now_time;
    extern time_t new_boot_time_t;
    
    int port;

    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug( 2 );
#endif
    // Init the game!
    GameContext *game = new GameContext();
    num_descriptors		= 0;
    first_descriptor		= NULL;
    last_descriptor		= NULL;

    /*
     * Init time.
     */
    gettimeofday( &now_time, NULL );
    current_time = (time_t) now_time.tv_sec;
/*  gettimeofday( &boot_time, NULL);   okay, so it's kludgy, sue me :) */
    boot_time = time(0);         /*  <-- I think this is what you wanted */
    SPRINTF( str_boot_time, "%s", ctime( &current_time ) );

    /*
     * Init boot time.
     */
    set_boot_time = &set_boot_time_struct;
/*  set_boot_time->hour   = 6;
    set_boot_time->min    = 0;
    set_boot_time->sec    = 0;*/
    set_boot_time->manual = 1;
    
    new_boot_time = update_time(localtime(&current_time));
    /* Copies *new_boot_time to new_boot_struct, and then points
       new_boot_time to new_boot_struct again. -- Alty */
    new_boot_struct = *new_boot_time;
    new_boot_time = &new_boot_struct;
    new_boot_time->tm_mday += 1;
    if(new_boot_time->tm_hour > 12)
      new_boot_time->tm_mday += 1;
    new_boot_time->tm_sec = 0;
    new_boot_time->tm_min = 0;
    new_boot_time->tm_hour = 6;

    /* Update new_boot_time (due to day increment) */
    new_boot_time = update_time(new_boot_time);
    new_boot_struct = *new_boot_time;
    new_boot_time = &new_boot_struct;
    reboot_check(game, mktime(new_boot_time));
    new_boot_time_t = mktime(new_boot_time);

    /* Set reboot time string for do_time */
    get_reboot_string(game);

    /*
     * Reserve two channels for our use.
     */
    if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
	perror( NULL_FILE );
	exit( 1 );
    }
    if ( ( fpLOG = fopen( NULL_FILE, "r" ) ) == NULL )
    {
	perror( NULL_FILE );
	exit( 1 );
    }

    /*
     * Get the port number.
     */
    port = 4000;
    if ( argc > 1 )
    {
	if ( !is_number( argv[1] ) )
	{
	    fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
	    exit( 1 );
	}
	else if ( ( port = atoi( argv[1] ) ) <= 1024 )
	{
	    fprintf( stderr, "Port number must be above 1024.\n" );
	    exit( 1 );
	}
    }

    /*
     * Run the game.
     */
    log_printf("PID: %d",getpid());
    bootup = TRUE;
    log_string("Booting Database");

    game->get_sysdata()->no_name_resolving	= TRUE;
    game->get_sysdata()->wait_for_auth	= TRUE;

    boot_db( game );
    log_string("Initializing socket");
    control  = init_socket( port   );
    control2 = init_socket( port+1 );
    conclient= init_socket( port+10);
    conjava  = init_socket( port+20);
    log_printf( "Rise in Power ready on port %d.", port );
    bootup = FALSE;
    game_loop(game);
    close( control  );
    close( control2 );
    close( conclient);
    close( conjava  );
    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    exit( 0 );
    return 0;
}

int closed( int d )
{
  return close(d);
}

int init_socket( int port )
{
    char hostname[64];
    struct sockaddr_in	 sa;
//    struct hostent	*hp;
//    struct servent	*sp;
    int x = 1;
    int fd;

    gethostname(hostname, sizeof(hostname));
    

    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
      perror( "Init_socket: socket" );
      exit( 1 );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
		    (void *) &x, sizeof(x) ) < 0 )
    {
      perror( "Init_socket: SO_REUSEADDR" );
      close( fd );
      exit( 1 );
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
	struct	linger	ld;

	ld.l_onoff  = 1;
	ld.l_linger = 1000;

	if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
			(void *) &ld, sizeof(ld) ) < 0 )
	{
	    perror( "Init_socket: SO_DONTLINGER" );
	    close( fd );
	    exit( 1 );
	}
    }
#endif

//    hp = gethostbyname( hostname );
//    sp = getservbyname( "service", "mud" );
    memset(&sa, '\0', sizeof(sa));
    sa.sin_family   = AF_INET; /* hp->h_addrtype; */
    sa.sin_port	    = htons( port );

    if ( bind( fd, (struct sockaddr *) &sa, sizeof(sa) ) == -1 )
    {
	perror( "Init_socket: bind" );
	close( fd );
	exit( 1 );
    }

    if ( listen( fd, 50 ) < 0 )
    {
	perror( "Init_socket: listen" );
	close( fd );
	exit( 1 );
    }

    return fd;
}

/*
static void SegVio()
{
  CHAR_DATA *ch;
  char buf[MAX_STRING_LENGTH];

  log_string( "SEGMENTATION VIOLATION" );
  log_string( lastplayercmd );
  for ( ch = first_char; ch; ch = ch->next )
  {
    log_printf( "%cPC: %-20s room: %d", IS_NPC(ch) ? 'N' : ' ',
    		ch->name, ch->in_room->vnum );
  }
  exit(0);
}
*/

/*
 * LAG alarm!							-Thoric
 */
/*
 * Real SIGALRM handler.
 *
 * IMPORTANT:
 * This must stay minimal.  It does only two things:
 *   1) records that the alarm fired
 *   2) force-closes g_pending_accept_fd immediately if one is in progress
 *
 * close() is async-signal-safe.
 * bug(), echo_to_all(), FD_CLR(), SPRINTF(), and game_loop() are not.
 */
static void caught_alarm(int)
{
    g_alarm_fired = 1;

    if (g_pending_accept_fd >= 0)
    {
        close(g_pending_accept_fd); 
        g_pending_accept_fd = -1;
    }
}

/*
 * Normal post-alarm recovery logic.
 *
 * This runs back in ordinary game context, where it is safe to log,
 * echo messages, manipulate fd_sets, and reset bookkeeping.
 */
static void handle_alarm_recovery(GameContext *game)
{
    (void)game;

    bug("ALARM CLOCK!");
    echo_to_all(AT_IMMORT, ("Alas, the hideous mandalorian entity known only as 'Lag' rises once more!\n"), ECHOTAR_ALL);

    log_string("alarm recovery complete");
    g_alarm_fired = 0;
}

bool check_bad_desc( int desc )
{
    if ( FD_ISSET( desc, &exc_set ) )
    {
	FD_CLR( desc, &in_set );
	FD_CLR( desc, &out_set );
	log_string( "Bad FD caught and disposed." );
	return TRUE;
    }
    return FALSE;
}


void accept_new( GameContext *game, int ctrl )
{
    static struct timeval null_time;
    DESCRIPTOR_DATA *d;

#if defined(MALLOC_DEBUG)
    if ( malloc_verify( ) != 1 )
        abort( );
#endif

    FD_ZERO( &in_set  );
    FD_ZERO( &out_set );
    FD_ZERO( &exc_set );
    FD_SET( ctrl, &in_set );
    maxdesc = ctrl;
    newdesc = 0;

    for ( d = first_descriptor; d; d = d_next )
    {
        d_next = d->next;
        if ( d->descriptor < 0 )
            continue;

        maxdesc = UMAX( maxdesc, d->descriptor );
        FD_SET( d->descriptor, &in_set  );
        FD_SET( d->descriptor, &out_set );
        FD_SET( d->descriptor, &exc_set );

        if ( d->auth_fd != -1 )
        {
            maxdesc = UMAX( maxdesc, d->auth_fd );
            FD_SET( d->auth_fd, &in_set );
            if ( IS_SET( d->auth_state, FLAG_WRAUTH ) )
                FD_SET( d->auth_fd, &out_set );
        }

        if ( d == last_descriptor )
            break;
    }

    if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
    {
        if ( errno == EINTR )
        {
            if ( g_alarm_fired )
            {
                log_string( "accept_new: select interrupted by alarm" );
                return;
            }

            return;
        }

        perror( "accept_new: select: poll" );
        exit( 1 );
    }

    if ( FD_ISSET( ctrl, &exc_set ) )
    {
        bug( "Exception raise on controlling descriptor %d", ctrl );
        FD_CLR( ctrl, &in_set );
        FD_CLR( ctrl, &out_set );
    }
    else if ( FD_ISSET( ctrl, &in_set ) )
    {
        newdesc = ctrl;
        new_descriptor( game, newdesc );
    }
}

void game_loop(GameContext *game)
{
    struct timeval	  last_time;
    std::string cmdline;
    DESCRIPTOR_DATA *d;
//  AREA_DATA *pArea;

/*  time_t	last_check = 0;  */

    signal( SIGPIPE, SIG_IGN );
    //signal( SIGALRM, (void (*) (int) )caught_alarm );
    {
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = caught_alarm;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0; /* do NOT use SA_RESTART; we want SIGALRM to interrupt stalls */

        sigaction(SIGALRM, &sa, NULL);
    }    
    /* signal( SIGSEGV, SegVio ); */
    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;
    

    /* Main loop */
    while ( !mud_down )
    {
        if (g_alarm_fired)
            handle_alarm_recovery(game);

	accept_new( game, control  );
	accept_new( game, control2 );
	accept_new( game, conclient);
	accept_new( game, conjava  );
        cron();
	/*
	 * Kick out descriptors with raised exceptions
	 * or have been idle, then check for input.
	 */
	for ( d = first_descriptor; d; d = d_next )
	{
        // Hack in game into this, just to make sure nothing is missing GameContext - DV 4-7-26
        {
            if (d->game == nullptr)
                d->game = game;
            if (d->character && d->character->game == nullptr)
                d->character->game = game;
        }

	    if ( d == d->next )
	    {
	      bug( "descriptor_loop: loop found & fixed" );
	      d->next = NULL;
	    }
 	    d_next = d->next;   

        if (d->mccp_pending == 1 && d->connected == CON_PLAYING)
        {
            compressStart(d, TELOPT_COMPRESS2);
            d->mccp_pending = false;
        }      

	    d->idle++;	/* make it so a descriptor can idle out */
	    if ( FD_ISSET( d->descriptor, &exc_set ) )
	    {
            FD_CLR( d->descriptor, &in_set  );
            FD_CLR( d->descriptor, &out_set );
            if ( d->character
            && ( d->connected == CON_PLAYING
            ||   d->connected == CON_EDITING ) )
                save_char_obj( d->character );
            d->outtop	= 0;
            close_socket( d, TRUE );
            continue;
	    }
	    else 
	    if (( d->character ? d->character->top_level <= LEVEL_IMMORTAL : FALSE) &&
	    ( d->idle > 7200 ) && !BV_IS_SET(d->character->act, PLR_AFK))		  /* 30 minutes  */
	    {
            if( (d->character && d->character->in_room) ? d->character->top_level <= LEVEL_IMMORTAL : FALSE)
            {
                output_to_descriptor( d,
                "Idle 30 Minutes. Activating AFK Flag\n");
                BV_SET_BIT(d->character->act, PLR_AFK);
                act(AT_GREY,"$n is now afk due to idle time.", d->character, NULL, NULL, TO_ROOM);
                continue;
            }
	    }
	    else 
	    if (( d->character ? d->character->top_level <= LEVEL_IMMORTAL : TRUE) &&
	    ( (!d->character && d->idle > 360)		  /* 2 mins */
            ||   ( d->connected != CON_PLAYING && d->idle > 1200) /* 5 mins */
	    ||     d->idle > 28800 ))				  /* 2 hrs  */
	    {
            if( d->character ? d->character->top_level <= LEVEL_IMMORTAL : TRUE)
            {
                output_to_descriptor( d,
                "Idle timeout... disconnecting.\n");
                d->outtop	= 0;
                close_socket( d, TRUE );
                continue;
            }
	    }
	    else
	    {
            d->fcommand	= FALSE;

            if ( FD_ISSET( d->descriptor, &in_set ) )
            {
                d->idle = 0;
                if ( d->character )
                d->character->timer = 0;
                if ( !read_from_descriptor( d ) )
                {
                    FD_CLR( d->descriptor, &out_set );
                    if ( d->character
                    && ( d->connected == CON_PLAYING
                    ||   d->connected == CON_EDITING ) )
                    save_char_obj( d->character );
                    d->outtop	= 0;
                    close_socket( d, FALSE );
                    continue;
                }
            }

            /* IDENT authentication */
                if ( ( d->auth_fd == -1 ) && ( d->atimes < 20 ) 
            && !str_cmp( d->user, "unknown" ) )
            start_auth( d );

            if ( d->auth_fd != -1)
            {
            if ( FD_ISSET( d->auth_fd, &in_set ) )
            {
                read_auth( d );
                /* if ( !d->auth_state ) 
                    check_ban( d );*/
            }
            else
            if ( FD_ISSET( d->auth_fd, &out_set )
            && IS_SET( d->auth_state, FLAG_WRAUTH) )
            {
                send_auth( d );
                /* if ( !d->auth_state )
                check_ban( d );*/
            }
            }
            if ( d->character && d->character->wait > 0 )
            {
                --d->character->wait;
                continue;
            }
            /*
            * ============================================================
            * MULTI-COMMAND PROCESSING LOOP 
            * ============================================================
            */
            int cmd_count = 0;

            while ( d->inbuf_len > 0 || !d->intext_str.empty() )
            {
                /* Stop if a command is already pending */
                if ( !d->incomm_str.empty() )
                    break;

                read_from_buffer( d );

                /* No full command ready */
                if ( d->incomm_str.empty() )
                    break;

                d->fcommand = TRUE;
                stop_idling( d->character );

                cmdline = d->incomm_str;
                d->incomm_str.clear();

                if ( d->character )
                    set_cur_char( d->character );

                /*
                    * 🔥 EXECUTE immediately (instead of delaying a tick)
                    */
                if ( d->pagepoint != PAGEPOINT_NULL)
                    set_pager_input(d, cmdline);
                else
                {
                    switch( d->connected )
                    {
                        default:
                            nanny( d, cmdline );
                            break;

                        case CON_PLAYING:
                            d->character->cmd_recurse = 0;
                            interpret( d->character, cmdline );
                            break;

                        case CON_EDITING:
                            edit_buffer( d->character, cmdline );
                            break;
                            
                    }
                }
                if ( d->descriptor < 0 )   /* CLOSED inside interpret/nanny */
                {
                    break;
                }
                /*
                    * 🔥 SAFETY: prevent CPU abuse
                    */
                if ( ++cmd_count >= MAX_COMMANDS_PER_TICK )
                {
                    output_to_descriptor(d,
                        "\n*** Command limit per tick reached ***\n");
                    break;
                }
            }
            /*
            * ================= END MULTI-COMMAND BLOCK ==================
            */
        }

        gmcp_flush(d); 

        if ( d->descriptor < 0 )   /* CLOSED inside interpret/nanny */
        {
            DISPOSE(d);
            break;    
        }
	   if ( d == last_descriptor )
	        break;
	}

	/*
	 * Autonomous game motion.
	 */
	update_handler( game );

	/*
	 * Check REQUESTS pipe
	 */
        check_requests( game );

	/*
	 * Output.
	 */
	for ( d = first_descriptor; d; d = d_next )
	{
	    d_next = d->next;   

        if ( d->descriptor < 0 )
        {
            DISPOSE(d);
            continue;
        }

       if ( ( d->fcommand || d->outtop > 0 )
	    &&   FD_ISSET(d->descriptor, &out_set) )
	    {
	        if ( d->pagepoint != PAGEPOINT_NULL )
	        {
	          if ( !pager_output(d) )
	          {
	            if ( d->character
	            && ( d->connected == CON_PLAYING
	            ||   d->connected == CON_EDITING ) )
	                save_char_obj( d->character );
	            d->outtop = 0;
	            close_socket(d, FALSE);
	          }
	        }
		else if ( !flush_buffer( d, TRUE ) )
		{
		    if ( d->character
		    && ( d->connected == CON_PLAYING
		    ||   d->connected == CON_EDITING ) )
			save_char_obj( d->character );
		    d->outtop	= 0;
		    close_socket( d, FALSE );
		}
	    }
	    if ( d == last_descriptor )
	      break;
	}

	/*
	 * Synchronize to a clock.
	 * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
	 * Careful here of signed versus unsigned arithmetic.
	 */
	{
	    struct timeval now_time;
	    long secDelta;
	    long usecDelta;

	    gettimeofday( &now_time, NULL );
	    usecDelta	= ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
			+ 1000000 / PULSE_PER_SECOND;
	    secDelta	= ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
	    while ( usecDelta < 0 )
	    {
		usecDelta += 1000000;
		secDelta  -= 1;
	    }

	    while ( usecDelta >= 1000000 )
	    {
		usecDelta -= 1000000;
		secDelta  += 1;
	    }

	    if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
	    {
		struct timeval stall_time;

		stall_time.tv_usec = usecDelta;
		stall_time.tv_sec  = secDelta;
        if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 )
        {
            if ( errno == EINTR )
            {
                if (g_alarm_fired)
                    handle_alarm_recovery(game);
            }
            else
            {
                perror( "game_loop: select: stall" );
                exit( 1 );
            }
        }
	    }
	}

	gettimeofday( &last_time, NULL );
	current_time = (time_t) last_time.tv_sec;

        /* Check every 5 seconds...  (don't need it right now)
	if ( last_check+5 < current_time )
	{
	  CHECK_LINKS(first_descriptor, last_descriptor, next, prev,
	      DESCRIPTOR_DATA);
	  last_check = current_time;
	}
	*/
    }
    
/*  for ( pArea = first_area; pArea; pArea = pArea->next )
    {
      fold_area( pArea, pArea->filename, FALSE );
    }
*/
    
    return;
}


void new_descriptor( GameContext *game, int new_desc )
{
    std::string buf;
    DESCRIPTOR_DATA *dnew;
    BAN_DATA *pban;
    struct hostent  *from;
    char *hostname;
    struct sockaddr_in sock;
    int desc;
    int size;

    set_alarm( 20 );
    size = sizeof(sock);
    if ( check_bad_desc( new_desc ) )
    {
      set_alarm( 0 );
      return;
    }
    set_alarm( 20 );
    if ( ( desc = accept( new_desc, (struct sockaddr *) &sock, (socklen_t *)&size) ) < 0 )
    {
        perror( "New_descriptor: accept");
    /*	log_printf("[*****] BUG: New_descriptor: accept"); */
        set_alarm( 0 );
        return;
    }
    g_pending_accept_fd = desc;

    if ( check_bad_desc( new_desc ) )
    {
      set_alarm( 0 );
      g_pending_accept_fd = -1;
      return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    set_alarm( 20 );
    if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
    {
        perror( "New_descriptor: fcntl: FNDELAY" );
        if ( g_pending_accept_fd >= 0 )
            close( desc );
        g_pending_accept_fd = -1;                    
        set_alarm( 0 );
        return;
    }
    if ( check_bad_desc( new_desc ) )
    {
        if ( g_pending_accept_fd >= 0 )
            close( desc );

        g_pending_accept_fd = -1;
        set_alarm( 0 );
        return;
    }

    if ( g_alarm_fired )
    {
        log_string( "new_descriptor: aborted due to alarm (post-accept/post-fcntl)" );
        g_pending_accept_fd = -1;
        set_alarm( 0 );
        return;
    }

    CREATE( dnew, DESCRIPTOR_DATA, 1 );
    dnew->game = game;
    dnew->next		= NULL;
    dnew->descriptor	= desc;
    dnew->connected	= CON_GET_NAME;
    dnew->outsize	= 2000;
    dnew->idle		= 0;
    dnew->lines		= 0;
    dnew->scrlen	= 24;
    dnew->port		= ntohs( sock.sin_port );
    dnew->user 		= STRALLOC("unknown");
    dnew->auth_fd	= -1;
    dnew->auth_inc	= 0;
    dnew->auth_state	= 0;
    dnew->newstate	= 0;
    dnew->rendercolor	=   0x07;
    dnew->has_rendercolor = false;
    dnew->last_sent_color = 0xFF;    
    dnew->color_initialized = FALSE;        
    dnew->original      = NULL;
    dnew->character     = NULL;
    dnew->telnet_pos = 0;
    dnew->pagepoint = PAGEPOINT_NULL;
    dnew->sga_enabled = false;
    dnew->eor_enabled = false;
    dnew->ttype_count = 0;
    dnew->supports_256color = false;
    dnew->terminal_type[0] = '\0';    
    dnew->echo_enabled = false;
    dnew->intext_len = 0;
    dnew->intext[0] = '\0';    
    dnew->gmcp_enabled = false;
    dnew->client_name[0]   = '\0';
    dnew->terminal_type[0] = '\0';
    dnew->last_ttype[0]    = '\0';
    dnew->client_type = CLIENT_UNKNOWN;
    dnew->supports_utf8 = false;
    dnew->supports_ansi = false;
    dnew->ttype_count = 0;    

#ifdef LOGTELNET
    dnew->debug_telnet = true;
#else
    dnew->debug_telnet = false;
#endif
    memset(dnew->telopt_us,  0, sizeof(dnew->telopt_us));
    memset(dnew->telopt_him, 0, sizeof(dnew->telopt_him));
    CREATE_ARRAY( dnew->outbuf, char, dnew->outsize );

    buf = inet_ntoa( sock.sin_addr );

    dnew->host = STRALLOC( buf );

    //Watchdog code -- Kre
    /*
    if (strcmp(dnew->host,"127.0.0.1")==0) {
      output_to_descriptor( dnew,"@" );
      free_desc(dnew);
      set_alarm( 0 );
      return;
    }
    */

    if ( g_alarm_fired )
    {
        log_string( "new_descriptor: aborted due to alarm (pre-DNS)" );
        free_desc( dnew );
        g_pending_accept_fd = -1;
        set_alarm( 0 );
        return;
    }

 /* Noresolve now does something useful - DV - Stuff for dontresolve. - Ulysses */
     if ( !game->get_sysdata()->no_name_resolving && !check_dont_resolve(buf) )
         from = gethostbyaddr( (char *) &sock.sin_addr,
     	  	sizeof(sock.sin_addr), AF_INET );
     else
         from = NULL;
    hostname = STRALLOC( (char *)( from ? from->h_name : "") );

    if ( g_alarm_fired )
    {
        log_string( "new_descriptor: aborted due to alarm (post-DNS)" );
        free_desc( dnew );
        g_pending_accept_fd = -1;
        set_alarm( 0 );
        return;
    }

/*	
    if ( !str_prefix( dnew->host, "172.1" ) )
    {
//    log_string_plus( "AOL Ping!", LOG_COMM, sysdata.log_level );
      buf = inet_ntoa( sock.sin_addr );
      log_printf_plus( LOG_COMM, sysdata.log_level, "Sock.sinaddr: AOL Ping! %s, port %hd.",
		buf.c_str(), dnew->port );
      
      output_to_descriptor( dnew,
      "AOL users have been banned from this site.  We apologize for the inconvenience.\n");
      flush_buffer(d, TRUE);
      free_desc( dnew );
      set_alarm( 0 );
      return;
    }
*/

    for ( pban = first_ban; pban; pban = pban->next )
    {
	if ( 
	  ( 
	    !str_prefix( pban->name, dnew->host ) 
	    || !str_suffix ( pban->name , hostname ) 
	  )
	  &&  pban->level >= LEVEL_SUPREME 
	)
	{
	    output_to_descriptor( dnew,
		    "Your site has been banned from this Mud.\n" );
        flush_buffer(dnew, FALSE);
	    free_desc( dnew );
        g_pending_accept_fd = -1;          
	    set_alarm( 0 );
      
	    return;
	}
    }

    buf = inet_ntoa( sock.sin_addr );
    log_printf_plus( LOG_COMM, game->get_sysdata()->log_level, "Sock.sinaddr:  %s, port %d.",	buf.c_str(), dnew->port );

    if ( !game->get_sysdata()->no_name_resolving )
    {
       STRFREE ( dnew->host);
       dnew->host = STRALLOC( ( from ? from->h_name : buf) );
    }

    /*
     * Init descriptor data.
     */

    if ( !last_descriptor && first_descriptor )
    {
      DESCRIPTOR_DATA *d;

      bug( "New_descriptor: last_desc is NULL, but first_desc is not! ...fixing" );
      for ( d = first_descriptor; d; d = d->next )
	    if ( !d->next )
		    last_descriptor = d;
    }
    g_pending_accept_fd = -1;
    LINK( dnew, first_descriptor, last_descriptor, next, prev );

// Start Telnet Initiation

#ifdef MCCP
    send_telnet(dnew, (const unsigned char *)eor_on_str, sizeof(eor_on_str));
    send_telnet(dnew, (const unsigned char *)compress2_on_str, sizeof(compress2_on_str));
//  output_to_descriptor(dnew, (const char *)compress_on_str);
#endif

    const unsigned char will_naws[] = { IAC, DO, TELOPT_NAWS };
    send_telnet(dnew,(const unsigned char *)(will_naws), 3);

    /* SGA negotiation */
    const unsigned char will_sga[] = { IAC, WILL, TELOPT_SGA };
    send_telnet(dnew, will_sga, 3);

    /* EOR negotiation */
    const unsigned char will_eor[] = { IAC, WILL, TELOPT_EOR };
    send_telnet(dnew, will_eor, 3);

    /* TTYPE negotiation */
    const unsigned char do_ttype[] = { IAC, DO, TELOPT_TTYPE };
    send_telnet(dnew, do_ttype, 3);

    /* LINEMODE WONT */
    const unsigned char do_linemode[] = { IAC, WONT, TELOPT_LINEMODE };
    send_telnet(dnew, do_linemode, 3);

    /* ECHO WONT */
//    const unsigned char will_echo[] = { IAC, WONT, TELOPT_ECHO };
//    send_telnet(dnew, will_echo, 3);    
    
    // GMCP
    const unsigned char will_gmcp[] = { IAC, WILL, TELOPT_GMCP };
    send_telnet(dnew, will_gmcp, 3);    

// Temporary for testing MSSP    
    const unsigned char will_mssp[] = { IAC, WILL, TELOPT_MSSP };
    send_telnet(dnew, will_mssp, 3);    


// End Telnet Initiation

    /*
     * Send the greeting.
     */
    {
    extern char * help_greeting;
    if ( help_greeting[0] == '.' )
	    output_to_descriptor( dnew, help_greeting+1);
	else
	    output_to_descriptor( dnew, help_greeting );
    }
    start_auth( dnew ); /* Start username authorization */

    if ( ++num_descriptors > game->get_sysdata()->maxplayers )
    	game->get_sysdata()->maxplayers = num_descriptors;
    if ( game->get_sysdata()->maxplayers > game->get_sysdata()->alltimemax )
    {
      buf = str_printf("%24.24s", ctime(&current_time));
      game->get_sysdata()->set_time_of_max (buf);
      game->get_sysdata()->alltimemax = game->get_sysdata()->maxplayers;
      buf = str_printf("Broke all-time maximum player record: %d", game->get_sysdata()->alltimemax);
      log_string_plus( buf.c_str(), LOG_COMM, game->get_sysdata()->log_level );
      to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
      save_sysdata( *game->get_sysdata() );
    }
    set_alarm(0);

//    fprintf(stderr, "COLDB: NEW DESC INIT: init=%d color=%02x\n", dnew->color_initialized, dnew->last_sent_color);

    return;
}

void free_desc( DESCRIPTOR_DATA *d )
{
    close( d->descriptor );
    d->descriptor = -1;
    STRFREE( d->host );
    DISPOSE_ARRAY( d->outbuf );
    STRFREE( d->user );    /* identd */


#ifdef MCCP
    if (d->compressing)
    {
        compressEnd(d);
    }
#endif

    --num_descriptors;
    return;
}

void close_socket( DESCRIPTOR_DATA *dclose, bool force )
{
    CHAR_DATA *ch;
    DESCRIPTOR_DATA *d;
    bool DoNotUnlink = FALSE;

    GameContext *game = dclose->game;

    /* flush outbuf */
    if ( !force && dclose->outtop > 0 )
	    flush_buffer( dclose, FALSE );

    /* say bye to whoever's snooping this descriptor */
    if ( dclose->snoop_by )
	output_to_descriptor( dclose->snoop_by,
	    "Your victim has left the game.\n" );

    /* stop snooping everyone else */
    for ( d = first_descriptor; d; d = d->next )
	if ( d->snoop_by == dclose )
	  d->snoop_by = NULL;

    /* Check for switched people who go link-dead. -- Altrag */
    if ( dclose->original )
    {
	if ( ( ch = dclose->character ) != NULL )
	  do_return(ch, "");
	else
	{
	  bug( "Close_socket: dclose->original without character %s",
		(dclose->original->name ? dclose->original->name : "unknown") );
	  dclose->character = dclose->original;
	  dclose->original = NULL;
	}
    }
    
    ch = dclose->character;

    /* sanity check :( */
    if ( !dclose->prev && dclose != first_descriptor )
    {
	DESCRIPTOR_DATA *dp, *dn;
	bug( "Close_socket: %s desc:%p != first_desc:%p and desc->prev = NULL!",
		ch ? ch->name : d->host, dclose, first_descriptor );
	dp = NULL;
	for ( d = first_descriptor; d; d = dn )
	{
	   dn = d->next;
	   if ( d == dclose )
	   {
		bug( "Close_socket: %s desc:%p found, prev should be:%p, fixing.",
		    ch ? ch->name : d->host, dclose, dp );
		dclose->prev = dp;
		break;
	   }
	   dp = d;
	}
	if ( !dclose->prev )
	{
	    bug( "Close_socket: %s desc:%p could not be found!.",
		    ch ? ch->name : dclose->host, dclose );
	    DoNotUnlink = TRUE;
	}
    }
    if ( !dclose->next && dclose != last_descriptor )
    {
	DESCRIPTOR_DATA *dp, *dn;
	bug( "Close_socket: %s desc:%p != last_desc:%p and desc->next = NULL!",
		ch ? ch->name : d->host, dclose, last_descriptor );
	dn = NULL;
	for ( d = last_descriptor; d; d = dp )
	{
	   dp = d->prev;
	   if ( d == dclose )
	   {
		bug( "Close_socket: %s desc:%p found, next should be:%p, fixing.",
		    ch ? ch->name : d->host, dclose, dn );
		dclose->next = dn;
		break;
	   }
	   dn = d;
	}
	if ( !dclose->next )
	{
	    bug( "Close_socket: %s desc:%p could not be found!.",
		    ch ? ch->name : dclose->host, dclose );
	    DoNotUnlink = TRUE;
	}
    }

    if ( dclose->character )
    {
	log_printf_plus( LOG_COMM, UMAX( game->get_sysdata()->log_level, ch->top_level ), "Closing link to %s.", ch->name );
	if ( dclose->connected == CON_PLAYING
	||   dclose->connected == CON_EDITING )
	{
	    act( AT_ACTION, "$n has lost $s link.", ch, NULL, NULL, TO_ROOM );
	    ch->desc = NULL;
	}
	else
	{
	    /* clear descriptor pointer to get rid of bug message in log */
	    dclose->character->desc = NULL;
	    free_char( dclose->character );
	}
    }


    if ( !DoNotUnlink )
    {
	/* make sure loop doesn't get messed up */
	if ( d_next == dclose )
	  d_next = d_next->next;
	UNLINK( dclose, first_descriptor, last_descriptor, next, prev );
    }

#ifdef MCCP
    compressEnd(dclose);
#endif

    if ( dclose->descriptor == maxdesc )
      --maxdesc;
    if ( dclose->auth_fd != -1 ) 
      close( dclose->auth_fd );

    free_desc( dclose );
    return;
}

int readd( int handle, char *buffer, int length )
{
  return read( handle, buffer, length );
}

const char *telcmd(unsigned char c)
{
    switch (c)
    {
        case WILL: return "WILL";
        case WONT: return "WONT";
        case DO:   return "DO";
        case DONT: return "DONT";
        case SB:   return "SB";
        case SE:   return "SE";
        default:   return "UNK";
    }
}

const char *telopt(unsigned char c)
{
    switch (c)
    {
        case TELOPT_COMPRESS:  return "COMPRESS";
        case TELOPT_COMPRESS2: return "COMPRESS2";
        case TELOPT_NAWS:      return "NAWS";
        case TELOPT_TTYPE:     return "TTYPE";
        case TELOPT_ECHO:      return "ECHO";
        case TELOPT_SGA:        return "SGA";        
        case TELOPT_EOR:        return "EOR";  
        case TELOPT_LINEMODE:   return "LINEMODE"; 
        case TELOPT_GMCP:       return "GMCP";
        default:
        {
            static char buf[16];
            snprintf(buf, sizeof(buf), "OPT_%d", c);
            return buf;
        }
    }
}

#define TELLOG(d, fmt, ...) \
    do { if ((d) && (d)->debug_telnet) \
        fprintf(stderr, "[TELNET] " fmt, ##__VA_ARGS__); } while(0)

#define TELLOG_APPEND(d, fmt, ...) \
    do { \
        if ((d)->debug_telnet) { \
            (d)->telnet_logpos += snprintf( \
                (d)->telnet_logbuf + (d)->telnet_logpos, \
                sizeof((d)->telnet_logbuf) - (d)->telnet_logpos, \
                fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define TELLOG_FLUSH(d, prefix) \
    do { \
        if ((d)->debug_telnet && (d)->telnet_logpos > 0) { \
            fprintf(stderr,"[TELNET] %s%s\r\n", prefix, (d)->telnet_logbuf); \
            (d)->telnet_logpos = 0; \
            (d)->telnet_logbuf[0] = '\0'; \
        } \
    } while(0)

bool is_known_telopt(unsigned char opt)
{
    switch (opt)
    {
        case TELOPT_ECHO:
        case TELOPT_SGA:
        case TELOPT_NAWS:
        case TELOPT_EOR:
        case TELOPT_TTYPE:
        case TELOPT_LINEMODE:
        case TELOPT_GMCP:
#ifdef MCCP
        case TELOPT_COMPRESS2:
#endif
            return true;

        default:
            return false;
    }
}

telnet_policy_t telnet_policy(DESCRIPTOR_DATA *d, int cmd, int opt)
{
    switch (opt)
    {
        case TELOPT_COMPRESS2:
            if (cmd == DO)
                return TELPOLICY_ACCEPT;
            if (cmd == DONT)
                return TELPOLICY_ACCEPT;
            return TELPOLICY_IGNORE;

        case TELOPT_NAWS:
            if (cmd == WILL)
                return TELPOLICY_ACCEPT;
            return TELPOLICY_IGNORE;

        case TELOPT_ECHO:
            return TELPOLICY_REJECT;
        // TTYPE Support
        case TELOPT_TTYPE:
            if (cmd == DO || cmd == WILL)
                return TELPOLICY_ACCEPT;
            return TELPOLICY_IGNORE;            
        /* SGA support */
        case TELOPT_SGA:
            if (cmd == DO || cmd == WILL)
                return TELPOLICY_ACCEPT;
            return TELPOLICY_IGNORE;
        /* Common safe defaults */
        case TELOPT_EOR:
            if (cmd == DO || cmd == WILL)
                return TELPOLICY_ACCEPT;
            return TELPOLICY_IGNORE;
        case TELOPT_LINEMODE:
            return TELPOLICY_REJECT;
        case TELOPT_GMCP:
            if (cmd == DO || cmd == WILL)
                return TELPOLICY_ACCEPT;
            return TELPOLICY_IGNORE;    
        case TELOPT_MSSP:
            if (cmd == DO)
                return TELPOLICY_ACCEPT;
            return TELPOLICY_IGNORE;                    
        default:
            return TELPOLICY_REJECT;
    }
}

int telnet_process( DESCRIPTOR_DATA *d, const unsigned char *in, int in_len, unsigned char *out, int out_max )
{
    int i, out_len = 0;
    bool last_was_cr = false;    

    for (i = 0; i < in_len; i++)
    {
        unsigned char c = in[i];
        bool process_char = true;

        if (d->telstate == TS_IAC && c == IAC)
        {
            d->telstate = TS_DATA;

            if (out_len < out_max - 1)
                out[out_len++] = IAC;

            continue;
        }

        switch (d->telstate)
        {
            case TS_DATA:
            {
                if (c == IAC)
                {
                    if (d->debug_telnet)
                    {
                        d->telnet_logpos = 0;
                        d->telnet_logbuf[0] = '\0';
                        TELLOG_APPEND(d, "IAC ");
                    }

                    d->telstate = TS_IAC;
                    process_char = false;
                }
                break;
            }
            case TS_IAC:
            {
                TELLOG_APPEND(d, "%s ", telcmd(c));
                switch (c)
                {
                    case IAC:
                        /* Escaped 255 -> treat as data */
                        d->telstate = TS_DATA;
                        process_char = true;
                        c = IAC;
                        break;

                    case WILL:
                        d->telstate = TS_WILL;
                        process_char = false;
                        break;

                    case WONT:
                        d->telstate = TS_WONT;
                        process_char = false;
                        break;

                    case DO:
                        d->telstate = TS_DO;
                        process_char = false;
                        break;

                    case DONT:
                        d->telstate = TS_DONT;
                        process_char = false;
                        break;

                    case SB:
                        /* Only allow SB if NAWS enabled */
                        d->telstate = TS_SB;
                        process_char = false;
                        break;

                    default:
                        d->telstate = TS_DATA;
                        process_char = false;
                        break;
                }
                break;
            }
            case TS_WILL:
            {
                /* ignore invalid telopts */
                if (c >= 240)
                {
                    d->telstate = TS_DATA;
                    process_char = false;
                    break;
                }                
                TELLOG_APPEND(d, "%s", telopt(c));
                TELLOG_FLUSH(d, "RCV: ");   

                if (d->telopt_him[c])
                {
                    d->telstate = TS_DATA;
                    break;
                }

                telnet_policy_t policy = telnet_policy(d, WILL, c);

                if (policy == TELPOLICY_REJECT)
                {
                    /* CHANGE: only reply to known options */
                    if (is_known_telopt(c))
                    {
                        const unsigned char dont[] = { IAC, DONT, c };
                        send_telnet(d, dont, 3);
                    }
                    d->telstate = TS_DATA;
                    process_char = false;                     
                    break;
                }
                /* SGA */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_SGA)
                {
                    const unsigned char do_sga[] = { IAC, DO, TELOPT_SGA };
                    send_telnet(d, do_sga, 3);
                }                
                /* EOR */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_EOR)
                {
                    const unsigned char do_eor[] = { IAC, DO, TELOPT_EOR };
                    send_telnet(d, do_eor, 3);
                }                
                /* TTYPE */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_TTYPE)
                {
                    const unsigned char do_ttype[] = { IAC, DO, TELOPT_TTYPE };
                    send_telnet(d, do_ttype, 3);

                    TELLOG(d, "TTYPE: DO sent\r\n");

                    /* Request terminal type */
                    const unsigned char ttype_send[] = {
                        IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE
                    };
                    send_telnet(d, ttype_send, sizeof(ttype_send));

                    TELLOG(d, "TTYPE: SEND request\r\n");
                }
                /* === POLICY: gated NAWS === */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_NAWS)
                {
                    TELLOG(d, "NAWS: negotiation start\r\n");

                    const unsigned char do_naws[] = { IAC, DO, TELOPT_NAWS };
                    send_telnet(d, (const unsigned char *)do_naws, 3);

                    d->naws_enabled = true;

                    TELLOG(d, "NAWS: enabled\r\n");
                }
                // GMCP
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_GMCP)
                {
                    const unsigned char do_gmcp[] = { IAC, DO, TELOPT_GMCP };
                    send_telnet(d, do_gmcp, 3);

                    d->telopt_him[c] = 1;
                    d->gmcp_enabled = true;
                    gmcp_init_subscriptions(d);

                    TELLOG(d, "GMCP: enabled\r\n");
                }                
                if (policy == TELPOLICY_REJECT && c == TELOPT_LINEMODE)
                {
                    const unsigned char dont[] = { IAC, DONT, TELOPT_LINEMODE };
                    send_telnet(d, dont, 3);

                    TELLOG(d, "LINEMODE: rejected\r\n");
                    break;
                }
                d->telopt_him[c] = 1;                
                d->telstate = TS_DATA;
                process_char = false;
                break;
            }
            case TS_WONT:
            {
                /* ignore invalid telopts */
                if (c >= 240)
                {
                    d->telstate = TS_DATA;
                    process_char = false;
                    break;
                }                
                TELLOG_APPEND(d, "%s", telopt(c));
                TELLOG_FLUSH(d, "RCV: ");   
                
                if (!d->telopt_him[c])
                {
                    d->telstate = TS_DATA;
                    process_char = false; 
                    break;
                }

                d->telopt_him[c] = 0;
                d->telstate = TS_DATA;
                process_char = false;
                break;
            }
            case TS_DO:
            {
                /* ignore invalid telopts */
                if (c >= 240)
                {
                    d->telstate = TS_DATA;
                    process_char = false;
                    break;
                }                
                TELLOG_APPEND(d, "%s", telopt(c));
                TELLOG_FLUSH(d, "RCV: ");   
                /* dedupe */
                if (d->telopt_us[c])
                {
                    /* NEW: ensure GMCP state is correct even if already negotiated */
                    if (c == TELOPT_GMCP)
                    {
                        d->gmcp_enabled = true;
                        gmcp_init_subscriptions(d);   /* NEW */
                        TELLOG(d, "GMCP: enabled (dedupe path)\r\n"); /* NEW */
                    }
                    process_char = false; // This was not there before!
                    d->telstate = TS_DATA;
                    break;
                }
                /* === POLICY ADD === */
                telnet_policy_t policy = telnet_policy(d, DO, c);
                /* GMCP SUPPORT (symmetric safety) */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_GMCP)
                {
                    const unsigned char will_gmcp[] = { IAC, WILL, TELOPT_GMCP };
                    send_telnet(d, will_gmcp, 3);

                    d->telopt_us[c] = 1;
                    d->gmcp_enabled = true;

                    gmcp_init_subscriptions(d);   /* NEW */

                    TELLOG(d, "GMCP: enabled (DO path)\r\n");
                }             
                /* EOR */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_EOR)
                {
                    d->telopt_us[c] = 1;

                    const unsigned char will_eor[] = { IAC, WILL, TELOPT_EOR };
                    send_telnet(d, will_eor, 3);

                    d->eor_enabled = true;
                }
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_ECHO)
                {
                    d->telopt_us[c] = 1;

                    const unsigned char will_echo[] = { IAC, WILL, TELOPT_ECHO };
                    send_telnet(d, will_echo, 3);

                    d->echo_enabled = true;

                    TELLOG(d, "ECHO: enabled\r\n");
                }                
                /* MSSP */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_MSSP)
                {
                    TELLOG(d, "MSSP: client requested (DO)\r\n");

                    d->telopt_us[c] = 1;

                    /* MSSP does NOT require WILL response in practice */
                    send_mssp(d);

                    TELLOG(d, "MSSP: sent\r\n");

                    d->telstate = TS_DATA;
                    process_char = false;
                    break;
                }                
                if (policy == TELPOLICY_REJECT)
                {
                    /* only reply to known options */
                    if (is_known_telopt(c))
                    {
                        const unsigned char dont[] = { IAC, DONT, c };
                        send_telnet(d, dont, 3);
                    }        
                    d->telstate = TS_DATA;
                    process_char = false;                                 
                    break;
                }

#ifdef MCCP
                /* === POLICY CHANGE: gated MCCP === */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_COMPRESS2)
                {
                    TELLOG(d, "MCCP: client requested COMPRESS2 (DO)\r\n");

                    const unsigned char will_comp[] = { IAC, WILL, TELOPT_COMPRESS2 };
                    send_telnet(d, (const unsigned char *)will_comp, 3);
                    d->telopt_us[c] = 1;
                    d->mccp_pending = 1;

                    TELLOG(d, "MCCP: pending set → waiting for CON_PLAYING\r\n");
                }
#endif
                /* SGA */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_SGA)
                {
                    d->telopt_us[c] = 1;
                    const unsigned char will_sga[] = { IAC, WILL, TELOPT_SGA };
                    send_telnet(d, will_sga, 3);

                    d->sga_enabled = true;
                }
                /* === POLICY CHANGE: gated NAWS === */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_NAWS)
                {
                    const unsigned char will_naws[] = { IAC, WILL, TELOPT_NAWS };
                    send_telnet(d, (const unsigned char *)will_naws, 3);

                    d->naws_enabled = true;
                }

                d->telstate = TS_DATA;
                process_char = false;
                break;
            }
            case TS_DONT:
            {
                /* ignore invalid telopts */
                if (c >= 240)
                {
                    d->telstate = TS_DATA;
                    process_char = false;
                    break;
                }                
                TELLOG_APPEND(d, "%s", telopt(c));
                TELLOG_FLUSH(d, "RCV: ");               
                /* === POLICY ADD === */
                /* dedupe */
                if (!d->telopt_us[c])
                {
                    d->telstate = TS_DATA;
                    process_char = false; 
                    break;
                }
                d->telopt_us[c] = 0;                
                telnet_policy_t policy = telnet_policy(d, DONT, c);
                if (c == TELOPT_EOR)
                {
                    d->eor_enabled = false;
                    TELLOG(d, "EOR: disabled\r\n");
                }

                if (c == TELOPT_SGA)
                {
                    d->sga_enabled = false;
                    TELLOG(d, "SGA: disabled\r\n");
                }
#ifdef MCCP
                /* === POLICY CHANGE: gated compression shutdown === */
                if (policy == TELPOLICY_ACCEPT && c == TELOPT_COMPRESS2 &&
                    d->compressing == TELOPT_COMPRESS2)
                {
                    compressEnd(d);
                }
#endif

                d->telstate = TS_DATA;
                process_char = false;
                break;
            }
            case TS_SB:
            {
                /* FIX: validate telopt before trusting */
                if (!is_known_telopt(c))
                {
                    d->telstate = TS_DATA;
                    process_char = false;
                    break;
                }

                d->sb_option = c;
                d->sb_len = 0;

                TELLOG_APPEND(d, "%s ", telopt(c)); 

                d->telstate = TS_SB_DATA;
                process_char = false;
                break;
            }
            case TS_SB_DATA:
            {
                TELLOG_APPEND(d, "%d ", c);
                if (c == IAC)
                {
                    d->telstate = TS_SB_IAC;
                    process_char = false;
                    break;
                }

                if (d->sb_len >= (int)sizeof(d->sb_buf))
                {
                    process_char = false;
                    break;
                }
                d->sb_buf[d->sb_len++] = c;
                process_char = false;
                break;
            }
            case TS_SB_IAC:
            {
                if (c == SE)
                {            
                    TELLOG_APPEND(d, "IAC SE");
                    TELLOG_FLUSH(d, "RCV: ");                        
                    if (d->sb_option == TELOPT_NAWS && d->sb_len == 4)
                    {
                        int width  = (d->sb_buf[0] << 8) | d->sb_buf[1];
                        int height = (d->sb_buf[2] << 8) | d->sb_buf[3];

                        if (width < 20 || width > 500)
                            width = 80;

                        d->term_width  = width;
                        d->term_height = height;
                    }
                    else if (d->sb_option == TELOPT_TTYPE && d->sb_len >= 1)
                    {
                        if (d->sb_buf[0] == TELQUAL_IS)
                        {
                            char tmp[128];

                            int len = d->sb_len - 1;
                            if (len > (int)sizeof(tmp) - 1)
                                len = sizeof(tmp) - 1;

                            memcpy(tmp, d->sb_buf + 1, len);
                            tmp[len] = '\0';

                            TELLOG(d, "TTYPE IS[%d]: %s\r\n", d->ttype_count, tmp);

                            char norm[128];
                            int i;
                            for (i = 0; i < len && i < (int)sizeof(norm) - 1; i++)
                                norm[i] = tolower((unsigned char)tmp[i]);
                            norm[i] = '\0';

                            if (d->ttype_count == 0)
                            {
                                strncpy(d->client_name, tmp, sizeof(d->client_name) - 1);
                                d->client_name[sizeof(d->client_name) - 1] = '\0';

                                /* NEW: detect client */
                                if (strstr(norm, "mudlet"))
                                    d->client_type = CLIENT_MUDLET;
                                else if (strstr(norm, "cmud"))
                                    d->client_type = CLIENT_CMUD;
                                else if (strstr(norm, "zmud"))
                                    d->client_type = CLIENT_ZMUD;
                                else if (strstr(norm, "tintin"))
                                    d->client_type = CLIENT_TINTIN;
                                else if (strstr(norm, "mushclient"))
                                    d->client_type = CLIENT_MUSHCLIENT;
                                else if (strstr(norm, "putty"))
                                    d->client_type = CLIENT_PUTTY;
                                else if (strstr(norm, "kitty"))
                                    d->client_type = CLIENT_KITTY;
                                else
                                    d->client_type = CLIENT_UNKNOWN;
                            }

                            else if (d->ttype_count == 1)
                            {
                                strncpy(d->terminal_type, tmp, sizeof(d->terminal_type) - 1);
                                d->terminal_type[sizeof(d->terminal_type) - 1] = '\0';
                            }

                            /* Detect capabilities */
                            if (strstr(norm, "256"))
                            {
                                d->supports_256color = true;
                            }
                            /* Detect capabilities */
                            if (strstr(norm, "truecolor") || strstr(norm, "24bit"))
                            {
                                d->supports_truecolor = true;
                            }                            
                            if (strstr(norm, "utf-8") || strstr(norm, "utf8"))
                                d->supports_utf8 = true;
                            if (strstr(norm, "xterm") || strstr(norm, "ansi"))
                                d->supports_ansi = true;                                
                            /* loop protection */
                            strncpy(d->last_ttype, tmp, sizeof(d->last_ttype) - 1);
                            d->last_ttype[sizeof(d->last_ttype) - 1] = '\0';                                

                            d->ttype_count++;

                            /* Request next TTYPE (LIMITED) */
                            if (d->ttype_count < 5)
                            {
                                const unsigned char ttype_send[] = {
                                    IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE
                                };
                                send_telnet(d, ttype_send, sizeof(ttype_send));
                            }
                        }
                    }    
                    /* GMCP HOOK */
                    else if (d->sb_option == TELOPT_GMCP)
                    {
                        handle_gmcp(d, d->sb_buf, d->sb_len);
                    }                                
                    else
                    {
                        /* hook for future protocols (GMCP, MSSP, etc.) */
                        /* Example:
                        if (d->sb_option == TELOPT_GMCP)
                            handle_gmcp(d, d->sb_buf, d->sb_len);
                        */
                    }

                    d->telstate = TS_DATA;
                    d->sb_len = 0;
                }
                else if (c == IAC)
                {
                    if (d->sb_len < (int)sizeof(d->sb_buf) - 1)
                        d->sb_buf[d->sb_len++] = IAC;

                    d->telstate = TS_SB_DATA;
                }
                else
                {
                    /* PARTIAL SB — stay in SB mode, wait for more data */
                    d->telstate = TS_SB_DATA;

                    if (d->sb_len < (int)sizeof(d->sb_buf) - 1)
                        d->sb_buf[d->sb_len++] = c;

                    process_char = false;
                }

                process_char = false;
                break;
            }
        }

        /* Emit only real data bytes */
        if (process_char && d->telstate == TS_DATA)
        {
            if (c < 32 && c != '\n' && c != '\r' && c != '\b' && c != 127)
                continue;            
            if (c == '\r')
            {
                last_was_cr = true;
                c = '\n'; 
            }
            else if (c == '\n')
            {
                if (last_was_cr)
                {
                    last_was_cr = false;
                    continue; /* skip LF in CRLF */
                }

                /* lone LF → treat as newline */
                c = '\n';
            }
            else if (c == '\0' && last_was_cr)
            {
                last_was_cr = false;
                continue; /* skip NUL after CR */
            }
            else
            {
                last_was_cr = false;
            }
            if (out_len < out_max - 1)
            {
                out[out_len++] = c;
            }            
        }
    }
    if (out_len < out_max)
        out[out_len] = '\0';
    return out_len;
}

bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    unsigned int iStart;

    /* Hold horses if pending command already. */
    if ( !d->incomm_str.empty() )
        return TRUE;

    /* Check for overflow. */
    iStart = d->inbuf_len;
    if ( iStart >= sizeof(d->inbuf) - 10 )
        {
        log_printf( "%s input overflow!", d->host );
        output_to_descriptor( d,
            "\n*** PUT A LID ON IT!!! ***\n");
        flush_buffer(d, TRUE);
        return FALSE;
    }

    /*
     * Per-second flood window reset
     *
     * WHY:
     * - We track bytes/sec instead of strlen()
     * - Reset counters once per second
     */
    time_t now = current_time;

    if ( now != d->in_time )
    {
        d->in_time = now;
        d->in_bytes = 0;
        d->in_commands = 0;
    }

    for ( ; ; )
    {
        int nRead;

        nRead = read( d->descriptor, d->inbuf + iStart,
            sizeof(d->inbuf) - 10 - iStart );
// Debug
/*
        if (nRead <= 0)
            break;
        for (int x = 0; x < nRead; x++)
        {
            unsigned char c = (unsigned char)d->inbuf[iStart + x];
            log_printf("RAW: %03d (0x%02X) %c",
                c, c, isprint(c) ? c : '.');
        }
*/
        if ( nRead > 0 )
        {
            /*
             * Count RAW bytes (pre-telnet, pre-filter)
             * This is the ONLY correct place to measure input volume
             */
            d->in_bytes += nRead;

            if ( d->in_bytes > 4096 )  /* tune as needed */
            {
                bug("FLOOD: %s (%d bytes/sec)",
                    d->character ? d->character->name : d->host,
                    d->in_bytes);

                output_to_descriptor( d,
                    "\n*** DISCONNECT: Input flood detected ***\n");

                return FALSE;
            }

            for (int x = 0; x < nRead; x++)
            {
//              unsigned char c = (unsigned char)d->inbuf[iStart + x];
//              bug("RAW BYTE (pre-anything): %d", c);
            }            
            iStart += nRead;
            d->inbuf_len += nRead;
//            if ( d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r' )
//                break;
        }
        else if ( nRead == 0 )
        {
            log_string_plus( "EOF encountered on read.", LOG_COMM, d->game->get_sysdata()->log_level );
            return FALSE;
        }
        else if ( errno == EWOULDBLOCK )
            break;
        else
        {
            perror( "Read_from_descriptor" );
            return FALSE;
        }
    }

//  bug("---- INBUF DUMP START ----");
    for (unsigned int x = 0; x < iStart; x++)
    {
//      bug("INBUF[%d]=%d", x, (unsigned char)d->inbuf[x]);
    }
//  bug("---- INBUF DUMP END ----");

    d->inbuf[d->inbuf_len] = '\0';  // safe terminator AFTER binary data

    return TRUE;
}

void dump_bytes(const char *label, const unsigned char *buf, int len)
{
    printf("%s [%d]: ", label, len);

    for (int i = 0; i < len; i++)
        printf("%02X ", buf[i]);

    printf(" | ");

    for (int i = 0; i < len; i++)
    {
        unsigned char c = buf[i];
        if (c >= 32 && c <= 126)
            printf("%c", c);
        else
            printf(".");
    }

    printf("\n");
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    int i;

    if ( !d->incomm_str.empty() )
        return;

    unsigned char clean[MAX_INBUF_SIZE];

    /*
     * ============================================================
     * Process ENTIRE buffer as a continuous stream
     * ============================================================
     */
    int clean_len = 0;

    if ( d->inbuf_len > 0 )
    {
        memset(clean, 0, sizeof(clean));

        clean_len = telnet_process(
            d,
            (unsigned char *)d->inbuf,
            d->inbuf_len,
            clean,
            sizeof(clean)
        );
    }

        /*
         * ============================================================
         * APPEND CLEAN DATA INTO INTEXT BUFFER
         * ============================================================
         */
        if ( clean_len > 0 )
        {
            size_t space = MAX_INBUF_SIZE - d->intext_str.size() - 1;

            if ( (size_t)clean_len > space )
                clean_len = (int)space;

            d->intext_str.append(reinterpret_cast<const char*>(clean), clean_len);
        }

     /*
     * ============================================================
     * Fully consume buffer after processing
     * ============================================================
     */
    d->inbuf_len = 0;
    d->inbuf[0] = '\0';

    /*
     * ============================================================
     * FIND NEWLINE IN INTEXT
     * ============================================================
     */
    size_t line_end = d->intext_str.find('\n');

    if ( line_end == std::string::npos )
        return;

    /*
     * ============================================================
     * BUILD COMMAND FROM INTEXT
     * ============================================================
     */
    d->incomm_str.clear();

    for ( i = 0; i < (int)line_end; i++ )
    {
        unsigned char c = (unsigned char)d->intext_str[i];

        if ( c == '\r' )
            continue;

        if ( d->incomm_str.size() >= 254 )
        {
            output_to_descriptor( d, "Line too long.\n" );
            break;
        }

        if ( (c == '\b' || c == 127) && !d->incomm_str.empty() )
        {
            size_t k = d->incomm_str.size();
            int back = utf8_prev_len(d->incomm_str.c_str(), d->incomm_str.c_str() + k);
            if (back > 0 && back <= (int)k)
                d->incomm_str.erase(k - back);
        }
        else if (c >= 32 || (c & 0x80))
        {
            int char_len = utf8_char_len_safe(&d->intext_str[i]);

            if ( d->incomm_str.size() + char_len >= 254 )
            {
                output_to_descriptor( d, "Line too long.\n" );
                break;
            }

            for (int j = 0; j < char_len; j++)
            {
                if ((size_t)(i + j) >= line_end)
                    break;

                d->incomm_str.push_back(d->intext_str[i + j]);
            }

            i += char_len - 1;
        }
    }

    if ( d->incomm_str.empty() )
        d->incomm_str = " ";

    /*
     * ============================================================
     * RATE LIMIT
     * ============================================================
     */
    d->in_commands++;

    if ( d->in_commands > 20 )
    {
        output_to_descriptor( d,
            "\n*** SLOW DOWN ***\n" );
        return;
    }

    /*
     * ============================================================
     * HISTORY
     * ============================================================
     */
    if ( !d->incomm_str.empty() && ( d->incomm_str.size() > 1 || d->incomm_str[0] == '!' ) )
    {
        if ( d->incomm_str[0] != '!' && d->incomm_str != d->inlast_str )
            d->repeat = 0;
        else if ( ++d->repeat >= 100 )
            output_to_descriptor( d,
                "\n*** PUT A LID ON IT!!! ***\n" );
    }

    if ( !d->incomm_str.empty() && d->incomm_str[0] == '!' )
        d->incomm_str = d->inlast_str;
    else
        d->inlast_str = d->incomm_str;

    /*
     * ============================================================
     * REMOVE ONLY ONE LINE FROM INTEXT
     * ============================================================
     */
    d->intext_str.erase(0, line_end + 1);

    return;
}

/*
 * Low level output function.
 */
bool flush_buffer( DESCRIPTOR_DATA *d, bool fPrompt )
{
    char buf[MAX_INPUT_LENGTH];
    extern bool mud_down;
    CHAR_DATA *ch;
    bool flushed_all = true;    
    
    ch = d->original ? d->original : d->character;
    if( ch && ch->fighting && ch->fighting->who )
         show_condition( ch, ch->fighting->who );


    /*
     * Bust a prompt.
     */
    if ( fPrompt && !mud_down && d->connected == CON_PLAYING )
    {
	ch = d->original ? d->original : d->character;
	if ( BV_IS_SET(ch->act, PLR_BLANK) )
	    output_to_descriptor( d, "\n" );

	if ( BV_IS_SET(ch->act, PLR_PROMPT) )
	    display_prompt(d);
	if ( BV_IS_SET(ch->act, PLR_TELNET_GA) )
	    output_to_descriptor( d, (const char *) go_ahead_str );
    }

    bool had_output = (d->outtop > 0);    

//    TELLOG(d, "flush start: had_output=%d outtop=%ld fPrompt=%d\r\n",
//       had_output, d->outtop, fPrompt);

    /*
     * Short-circuit if nothing to write.
     */
    if ( d->outtop == 0 )
    	return TRUE;

    /*
     * Snoop-o-rama.
     */
    if ( d->snoop_by )
    {
        /* without check, 'force mortal quit' while snooped caused crash, -h */
        if ( d->character && d->character->name )
        {
            /* Show original snooped names. -- Altrag */
            if ( d->original && d->original->name )
            SPRINTF( buf, "%s (%s)", d->character->name, d->original->name );
            else
            SPRINTF( buf, "%s", d->character->name);
            output_to_descriptor( d->snoop_by, buf );
        }
        output_to_descriptor( d->snoop_by, "% " );
        if (d->snoop_by != d)
            output_to_descriptor(d->snoop_by, d->outbuf);
    }   
    #ifdef MCCP
    /*
     * === COMPRESSED PATH ===
     */
    if (d->out_compress)
    {
        unsigned char tmp[COMPRESS_BUF_SIZE];

        size_t remaining = d->outtop;
        size_t offset = 0;

        while (remaining > 0)
        {
            size_t chunk = remaining;

            if (chunk > sizeof(tmp))
                chunk = sizeof(tmp);

            memcpy(tmp, d->outbuf + offset, chunk);

            d->out_compress->next_in  = tmp;
            d->out_compress->avail_in = chunk;

            /* Track input */
            d->mccp_bytes_in += chunk;

            while (d->out_compress->avail_in > 0)
            {
                d->out_compress->avail_out =
                    COMPRESS_BUF_SIZE -
                    (d->out_compress->next_out - d->out_compress_buf);

                if (d->out_compress->avail_out == 0)
                {
                    if (!process_compressed(d))
                        return FALSE;
                    continue;
                }

                if (deflate(d->out_compress, Z_SYNC_FLUSH) != Z_OK)
                    return FALSE;
            }

            offset += chunk;
            remaining -= chunk;
        }
        // Track Input
        d->mccp_bytes_in += d->outtop;        

        d->outtop = 0;
        if (d->out_compress->next_out > d->out_compress_buf)
        {
            if (!process_compressed(d))
                return FALSE;
        }
        if (d->eor_enabled && had_output)
        {
            //TELLOG(d, "flush (MCCP): sending EOR\r\n");

            static const unsigned char eor[] = { IAC, EOR };

            d->out_compress->next_in  = (unsigned char *)eor;
            d->out_compress->avail_in = 2;

            while (d->out_compress->avail_in > 0)
            {
                d->out_compress->avail_out =
                    COMPRESS_BUF_SIZE -
                    (d->out_compress->next_out - d->out_compress_buf);

                if (d->out_compress->avail_out == 0)
                {
                    if (!process_compressed(d))
                        return FALSE;
                    continue;
                }

                if (deflate(d->out_compress, Z_SYNC_FLUSH) != Z_OK)
                    return FALSE;
            }

            /* ensure it actually goes out */
            if (d->out_compress->next_out > d->out_compress_buf)
            {
                if (!process_compressed(d))
                    return FALSE;
            }
        }
        return TRUE;
    }
#endif

    /*
     * === NORMAL NON-BLOCKING WRITE ===
     */
    size_t max_chunk = 32768;

    size_t total_written = 0;

    while (d->outtop > 0)
    {
        size_t chunk = d->outtop > max_chunk ? max_chunk : d->outtop;

        ssize_t n = write(d->descriptor, d->outbuf, chunk);

        if (n > 0)
        {
            total_written += n;

            if ((size_t)n < d->outtop)
            {
                memmove(d->outbuf, d->outbuf + n, d->outtop - n);
            }

            d->outtop -= n;
        }
        else if (n == 0)
        {
            return FALSE;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                flushed_all = false;
                break;
            }

            return FALSE;
        }
    }

    /*
     * Slow client protection
     */
    if (d->outtop > 32000)
    {
        bug("flush_buffer: slow client overflow (%s)",
            d->character ? d->character->name : d->host);
        return FALSE;
    }
    if (d->eor_enabled && flushed_all && had_output)
    {
        TELLOG(d, "flush: had_output=%d outtop=%ld", had_output, d->outtop);        
        const unsigned char eor[] = { IAC, EOR };
        send_telnet(d, eor, 2);
    }
    return TRUE;
}

static size_t utf8_safe_len(const char *buf, size_t len)
{
    size_t i = 0;

    while (i < len)
    {
        unsigned char c = (unsigned char)buf[i];
        size_t char_len = 1;

        if (c < 0x80)
            char_len = 1;
        else if ((c & 0xE0) == 0xC0)
            char_len = 2;
        else if ((c & 0xF0) == 0xE0)
            char_len = 3;
        else if ((c & 0xF8) == 0xF0)
            char_len = 4;
        else
            break; /* invalid start byte */

        /* Not enough bytes left → stop BEFORE partial char */
        if (i + char_len > len)
            break;

        /* Validate continuation bytes */
        for (size_t j = 1; j < char_len; j++)
        {
            if ((buf[i + j] & 0xC0) != 0x80)
                return i; /* invalid sequence */
        }

        i += char_len;
    }

    return i;
}


void write_to_buffer_str(DESCRIPTOR_DATA *d,  const std::string& txt)
{
    write_to_buffer(d, txt.data(), (int)txt.size());
}

// Output_to_descriptor with smart wrapping and UTF-8 safety
void output_to_descriptor(DESCRIPTOR_DATA *d, const std::string& txt)
{
    std::string wrapped;

    if (!d || txt.empty())
        return;

    wrapped = wrap_text_smart((txt), d->term_width);

    if (wrapped.empty())
        return;

    write_to_buffer_str(d, wrapped);

    return;
}

void write_to_buffer_str(DESCRIPTOR_DATA *d, const char *txt)
{
    size_t len = strnlen(txt, MAX_STRING_LENGTH - 1);

    if (len == MAX_STRING_LENGTH)
    {
        bug("write_to_buffer_str: string truncated");
    }

    write_to_buffer(d, txt, len);
}


void write_to_buffer_raw(DESCRIPTOR_DATA *d, const unsigned char *data, int length)
{
    if (!d)
    {
        bug("write_to_buffer_raw: NULL descriptor");
        return;
    }

     if (!d->outbuf || !data || length == 0)
        return;

    /*
     * HARD CAP
     */
    if (d->outtop + length > d->outsize * 2)
    {
            fprintf(stderr, "write_to_buffer_raw: overflow (%s) [%ld bytes queued]\r\n",
                d->character ? d->character->name : d->host,
                d->outtop);
            close_socket(d, TRUE);
        return;
    }

    /* early flush */
    if (d->outtop + length > d->outsize)
    {
        if (!flush_buffer(d, FALSE))
            return;
    }

    /*
     * Expand buffer as needed (same logic)
     */
    while (d->outtop + (size_t)length + 1 >= d->outsize)
    {
        if (d->outsize > 262144)
        {
            d->outtop = 0;
            bug("write_to_buffer_raw: max buffer exceeded (%s)",
                d->character ? d->character->name : "???");
            close_socket(d, TRUE);
            return;
        }

        d->outsize *= 2;
        char *newbuf = new char[d->outsize](); // value-initialize

        if (d->outbuf)
        {
            memcpy(newbuf, d->outbuf, d->outtop);
            delete[] d->outbuf;
        }

        d->outbuf = newbuf;
    }

    /*
     * RAW COPY (no processing)
     */
    memcpy(d->outbuf + d->outtop, data, length);
    d->outtop += length;

    /*
     * DO NOT null-terminate binary stream
     * (but harmless if you want for debugging)
     */
    d->outbuf[d->outtop] = '\0';

    return;
}

void write_to_buffer_raw(DESCRIPTOR_DATA *d, const std::string& txt)
{
    write_to_buffer_raw(d, reinterpret_cast<const unsigned char*>(txt.data()), static_cast<int>(txt.size()));
}

int build_ansi_from_state( unsigned char prev, unsigned char cl, std::string &buf )
{
    buf.clear();

    /* =========================
     * Detect if anything changed
     * ========================= */
    int changed = 0;

    if ( ( cl & 0x88 ) != ( prev & 0x88 ) ) /* intensity + blink */
        changed = 1;

    if ( ( cl & 0x07 ) != ( prev & 0x07 ) ) /* foreground */
        changed = 1;

    if ( ( cl & 0x70 ) != ( prev & 0x70 ) ) /* background */
        changed = 1;

    if ( !changed )
        return 0;

    /* =========================
     * Build ANSI sequence
     * ========================= */

    buf += '\033';
    buf += '[';

    int added = 0;

    /* intensity (bold/normal) */
    if ( ( cl & 0x08 ) != ( prev & 0x08 ) )
    {
        if ( cl & 0x08 )
            buf += "1;";
        else
            buf += "22;";

        added = 1;
    }

    /* blink */
    if ( ( cl & 0x80 ) != ( prev & 0x80 ) )
    {
        if ( cl & 0x80 )
            buf += "5;";
        else
            buf += "25;";

        added = 1;
    }

    /* foreground */
    if ( ( cl & 0x07 ) != ( prev & 0x07 ) )
    {
        buf += '3';
        buf += static_cast<char>( '0' + ( cl & 0x07 ) );
        buf += ';';

        added = 1;
    }

    /* background */
    if ( ( cl & 0x70 ) != ( prev & 0x70 ) )
    {
        buf += '4';
        buf += static_cast<char>( '0' + ( ( cl & 0x70 ) >> 4 ) );
        buf += ';';

        added = 1;
    }

    /* =========================
     * Finalize or discard
     * ========================= */

    if ( !added )
    {
        buf.clear();
        return 0;
    }

    if ( !buf.empty() && buf.back() == ';' )
        buf.back() = 'm';
    else
        buf += 'm';

    return static_cast<int>( buf.size() );
}

static void flush_color( DESCRIPTOR_DATA *d )
{
    if ( !d->has_rendercolor )
        return;

    unsigned char prev   = d->last_sent_color;
    unsigned char target = d->rendercolor;

    if ( prev > 0x8F )
        prev = 0x07;
    if ( target > 0x8F )
        target = 0x07;

    if ( target != prev )
    {
        std::string colbuf;
        int ln = build_ansi_from_state( prev, target, colbuf );

        if ( ln > 0 )
        {
            while ( d->outtop + static_cast<size_t>( ln ) >= d->outsize )
            {
                d->outsize *= 2;
                char *newbuf = new char[d->outsize];

                if ( d->outbuf )
                {
                    memcpy( newbuf, d->outbuf, d->outtop ); // copy valid data
                    delete[] d->outbuf;
                }

                d->outbuf = newbuf;
            }

            memcpy( d->outbuf + d->outtop, colbuf.data(), static_cast<size_t>( ln ) );
            d->outtop += static_cast<size_t>( ln );
        }

        d->last_sent_color = target;
    }

    d->has_rendercolor = false;
}

unsigned char parse_color_code(unsigned char current, char type, char code)
{
    unsigned char newcolor = current;

    /*
     * type:
     *   '&' = foreground
     *   '^' = background
     */

    if (type == '&')
    {
        /* reset */
        if (code == 'x' || code == 'X' || code == '-')
            return 0x07;

        /* clear foreground bits, preserve blink/background */
        newcolor &= 0xF8;

        switch (code)
        {
            case 'r': newcolor &= ~0x08; newcolor |= 1; break;
            case 'g': newcolor &= ~0x08; newcolor |= 2; break;
            case 'y': newcolor &= ~0x08; newcolor |= 3; break;
            case 'b': newcolor &= ~0x08; newcolor |= 4; break;
            case 'm': newcolor &= ~0x08; newcolor |= 5; break;
            case 'c': newcolor &= ~0x08; newcolor |= 6; break;
            case 'w': newcolor &= ~0x08; newcolor |= 7; break;

            case 'R': newcolor |= 1; newcolor |= 0x08; break;
            case 'G': newcolor |= 2; newcolor |= 0x08; break;
            case 'Y': newcolor |= 3; newcolor |= 0x08; break;
            case 'B': newcolor |= 4; newcolor |= 0x08; break;
            case 'M': newcolor |= 5; newcolor |= 0x08; break;
            case 'C': newcolor |= 6; newcolor |= 0x08; break;
            case 'W': newcolor |= 7; newcolor |= 0x08; break;

            case 'D': newcolor &= ~0x07; break; /* dark/black */

            default:
                return current; /* unknown → no change */
        }

        return newcolor;
    }

    if (type == '^')
    {
        /* clear background bits (upper nibble or flag-based) */
        newcolor &= 0x8F;

        switch (code)
        {
            case 'r': newcolor |= (1 << 4); break;
            case 'g': newcolor |= (2 << 4); break;
            case 'y': newcolor |= (3 << 4); break;
            case 'b': newcolor |= (4 << 4); break;
            case 'm': newcolor |= (5 << 4); break;
            case 'c': newcolor |= (6 << 4); break;
            case 'w': newcolor |= (7 << 4); break;
            case 'D': newcolor |= (0 << 4); break;

            case 'x':
            case 'X':
                newcolor &= 0x0F; /* reset background */
                break;

            default:
                return current;
        }

        return newcolor;
    }

    return current;
}

int make_color_sequence( const std::string &col, std::string &buf,
                         DESCRIPTOR_DATA *d, int *consumed )
{
    int ln = 0;
    CHAR_DATA *och;
    bool ansi;

    buf.clear();

    if ( consumed )
        *consumed = 2;

    if ( col.empty() )
    {
        if ( consumed )
            *consumed = 0;
        return -1;
    }

    char ctype = col[0];

    och = ( d->original ? d->original : d->character );
    ansi = ( !IS_NPC( och ) && BV_IS_SET( och->act, PLR_ANSI ) );

    if ( col.size() < 2 )
        return -1;

    if ( ctype != '&' && ctype != '^' )
    {
        bug( "Make_color_sequence: command '%c' not '&' or '^'.", ctype );
        if ( consumed )
            *consumed = 1;
        return -1;
    }

    char code = col[1];

    /* literal && or ^^ */
    if ( code == ctype )
    {
        buf += code;
        if ( consumed )
            *consumed = 2;
        return 1;
    }

    if ( !ansi )
        return 0;

    /* Use rendercolor as source of truth */
    unsigned char current = d->rendercolor;
    unsigned char target  = parse_color_code( current, ctype, code );

    if ( target == current )
        return 0;

    if ( target > 0x8F )
        target = 0x07;

    /* Update deferred state ONLY */
    d->rendercolor = target;
    d->has_rendercolor = true;

    return ln;
}


static void copy_with_newlines(DESCRIPTOR_DATA *d,
                               const char *src,
                               size_t len)
{
    const char *p   = src;
    const char *end = src + len;

    while (p < end)
    {
        /* precise reserve instead of magic 64 */
        size_t needed = 2 /* CRLF */ + 32 /* ANSI worst case */;
        if (d->outtop + needed >= d->outsize)
        {
            d->outsize *= 2;
            char *newbuf = new char[d->outsize];

            if (d->outbuf)
            {
                memcpy(newbuf, d->outbuf, d->outtop); // copy valid data
                delete[] d->outbuf;
            }

            d->outbuf = newbuf;            
        }

        /* unified CR/LF handling */
        if (*p == '\r' || *p == '\n')
        {
            if (*p == '\r' && (p + 1 < end) && p[1] == '\n')
                p++;
            else if (*p == '\n' && (p + 1 < end) && p[1] == '\r')
                p++;

            d->outbuf[d->outtop++] = '\r';
            d->outbuf[d->outtop++] = '\n';

            p++;
            continue;
        }

        /* normal character */
        size_t char_len = utf8_char_len((unsigned char)*p);

        /* Clamp if malformed or truncated */
        if (p + char_len > end)
            char_len = 1;

        /* Ensure buffer space */
        if (d->outtop + char_len >= d->outsize)
        {
            d->outsize *= 2;
            char *newbuf = new char[d->outsize];

            if (d->outbuf)
            {
                memcpy(newbuf, d->outbuf, d->outtop); // copy valid data
                delete[] d->outbuf;
            }

            d->outbuf = newbuf;
        }

        /* Copy full character */
        memcpy(d->outbuf + d->outtop, p, char_len);
        d->outtop += char_len;
        p += char_len;
    }
}

static void copy_with_newlines( DESCRIPTOR_DATA *d, const std::string &src )
{
    copy_with_newlines( d, src.data(), src.size() );
}

/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{
    static int count = 0;

    if ( !d )
    {
        bug( "Write_to_buffer: NULL descriptor" );
        return;
    }

    if ( d->has_rendercolor )
        flush_color( d );

    if ( !d->outbuf )
        return;

    /* Find length in case caller didn't. */
    if ( length <= 0 )
    {
        if ( !txt )
            return;
        length = strlen( txt );  /* fallback only */
    }

    if ( length <= 0 )
        return;

    /* Initial \n\r if needed. */
    if ( d->outtop == 0 && !d->fcommand )
    {
        d->outbuf[0] = '\r';
        d->outbuf[1] = '\n';
        d->outtop = 2;
    }

    /*
     * HARD CAP (slow client protection)
     * WHY:
     * - Prevent infinite growth if client isn't reading
     * - Protects server from output flooding
     */
    if ( d->outtop + length + 4 > 50000 )
    {
        if ( count == 0 )
        {
            bug( "write_to_buffer: overflow (%s) [%d bytes queued]",
                d->character ? d->character->name : d->host,
                d->outtop );
            count++;
            close_socket( d, TRUE );
        }
        count = 0;
        return;
    }

    /* Expand the buffer as needed. */
    while ( d->outtop + (unsigned long)length + 4 >= d->outsize )
    {
        if ( d->outsize > 65536 )
        {
            /* empty buffer */
            d->outtop = 0;
            bug( "write_to_buffer: max buffer exceeded (%s)",
                d->character ? d->character->name : "???" );
            close_socket( d, TRUE );
            return;
        }

        d->outsize *= 2;
        char *newbuf = new char[d->outsize];

        if ( d->outbuf )
        {
            memcpy( newbuf, d->outbuf, d->outtop ); /* copy valid data */
            delete[] d->outbuf;
        }

        d->outbuf = newbuf;
    }

    /* COLOR-AWARE COPY */
    const char *ptr = txt;
    const char *end = txt + length;
    std::string colbuf;

    while ( ptr < end )
    {
        const char *colstr = NULL;

        /* find next color code */
        for ( const char *p = ptr; p < end; ++p )
        {
            if ( *p == '&' || *p == '^' )
            {
                colstr = p;
                break;
            }
        }

        if ( !colstr )
            break;

        /* copy text before color */
        size_t seglen = (size_t)( colstr - ptr );

        if ( seglen > 0 )
        {
            flush_color( d );
            size_t safe = utf8_safe_len( ptr, seglen );
            copy_with_newlines( d, ptr, safe );
        }

        if ( colstr + 1 >= end )
        {
            ptr = end;
            break;
        }

        /* ========================= */
        /* track consumed            */
        /* ========================= */
        int consumed = 0;

        colbuf.clear();
        int ln = make_color_sequence( std::string( colstr, 2 ), colbuf, d, &consumed );

        if ( ln > 0 )
        {
            flush_color( d );
            copy_with_newlines( d, colbuf );
        }

        /* advance past color code */
        if ( consumed <= 0 )
            consumed = 2;

        ptr = colstr + consumed;

        /* ========================================= */
        /* IMMEDIATELY copy following text           */
        /* until next color code                     */
        /* ========================================= */
        const char *next = ptr;

        while ( next < end && *next != '&' && *next != '^' )
            next++;

        size_t textlen = (size_t)( next - ptr );

        if ( textlen > 0 )
        {
            flush_color( d );
            size_t safe = utf8_safe_len( ptr, textlen );
            copy_with_newlines( d, ptr, safe );
        }

        ptr = next;
    }

    /* =========================
     * remainder copy (length-safe)
     * ========================= */
    if ( ptr < end )
    {
        size_t rem = (size_t)( end - ptr );
        flush_color( d );
        size_t safe = utf8_safe_len( ptr, rem );
        copy_with_newlines( d, ptr, safe );
    }

    flush_color( d );
    d->outbuf[d->outtop] = '\0';
}


bool write_to_descriptor_depreciated(int desc, char *txt, int length)
{
    bug("write_to_descriptor: deprecated path used!");
    return FALSE;
}


void show_title( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch;

    ch = d->character;

    if ( !BV_IS_SET( ch->pcdata->flags, PCFLAG_NOINTRO ) )
    {
	if (BV_IS_SET(ch->act, PLR_ANSI))
	  send_ansi_title(ch);
	else
	  send_ascii_title(ch);
    }
    else
    {
      output_to_descriptor( d, "Press enter...\n" );
    }
    d->connected = CON_PRESS_ENTER;
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, const std::string& argumentin )
{
    if ( !d )
        return;
    std::string argument = argumentin.empty() ? "" : argumentin;

    const char *p = argument.c_str();

    while (*p && isspace_utf8(p))
        UTF8_NEXT(p);
    argument = p;

    switch ( d->connected )
    {
        default:
            bug( "Nanny: bad d->connected %d.", d->connected );
            close_socket( d, TRUE );
            return;

        case CON_GET_NAME:
            nanny_get_name( d, argument );
            return;

        case CON_GET_OLD_PASSWORD:
            nanny_get_old_password( d, argument );
            return;

        case CON_CONFIRM_NEW_NAME:
            nanny_confirm_new_name( d, argument );
            return;

        case CON_GET_NEW_PASSWORD:
            nanny_get_new_password( d, argument );
            return;

        case CON_CONFIRM_NEW_PASSWORD:
            nanny_confirm_new_password( d, argument );
            return;

        case CON_GET_NEW_SEX:
            nanny_get_new_sex( d, argument );
            return;

        case CON_GET_NEW_RACE:
            nanny_get_new_race( d, argument );
            return;

        case CON_GET_NEW_CLASS:
            nanny_get_new_class( d, argument );
            return;

        case CON_ROLL_STATS:
            nanny_roll_stats( d );
            return;

        case CON_STATS_OK:
            nanny_stats_ok( d, argument );
            return;

        case CON_GET_WANT_RIPANSI:
            nanny_get_want_ripansi( d, argument );
            return;

        case CON_GET_MSP:
            nanny_get_msp( d, argument );
            return;

        case CON_PRESS_ENTER:
            nanny_press_enter( d, argument );
            return;

        case CON_READ_MOTD:
            nanny_read_motd( d, argument );
            return;
    }
}

static void nanny_get_name( DESCRIPTOR_DATA *d, const std::string& argumentin )
{
    CHAR_DATA *ch;
    BAN_DATA *pban;
    bool fOld;
    short chk;

    if ( argumentin.empty() )
    {
        /* close_socket( d, FALSE ); */
        return;
    }

    std::string argument = argumentin;
    argument[0] = UPPER( argument[0] );
    if ( !check_parse_name( argument ) )
    {
        output_to_descriptor( d, "Illegal name, try another.\nName: " );
        return;
    }

    if ( !str_cmp( argument, "New" ) )
    {
        if ( d->newstate == 0 )
        {
            /* New player */
            /* Don't allow new players if DENY_NEW_PLAYERS is true */
            if ( d->game->get_sysdata()->deny_new_players == TRUE )
            {
                output_to_descriptor( d, "The mud is currently preparing for a reboot.\n" );
                output_to_descriptor( d, "New players are not accepted during this time.\n" );
                output_to_descriptor( d, "Please try again in a few minutes.\n" );
                close_socket( d, FALSE );
                return;
            }
                
            output_to_descriptor( d,
                "\nChoosing a name is one of the most important parts of this game...\n"
                "Make sure to pick a name appropriate to the character you are going\n"
                "to role play, and be sure that it suits our Star Wars theme.\n"
                "If the name you select is not acceptable, you will be asked to choose\n"
                "another one.\n\nPlease choose a name for your character: " );
            d->newstate++;
            d->connected = CON_GET_NAME;
            return;
        }
        else
        {
            output_to_descriptor( d, "Illegal name, try another.\nName: " );
            return;
        }
    }

    if ( check_playing( d, argument, FALSE ) == BERR )
    {
        output_to_descriptor( d, "Name: " );
        return;
    }

    fOld = load_char_obj( d, argument, TRUE );
    if ( !d->character )
    {
        log_printf( "Bad player file %s@%s.", argument.c_str(), d->host );
        output_to_descriptor( d, "Your playerfile is corrupt...Please notify the admins.\n" );
        close_socket( d, FALSE );
        return;
    }

    ch = d->character;

    for ( pban = first_ban; pban; pban = pban->next )
    {
        if ( ( !str_prefix( pban->name, d->host ) || !str_suffix( pban->name, d->host ) )
        && pban->level >= ch->top_level )
        {
            output_to_descriptor( d, "Your site has been banned from this Mud.\n" );
            close_socket( d, FALSE );
            return;
        }
    }

    if ( BV_IS_SET( ch->act, PLR_DENY ) )
    {
        log_printf_plus( LOG_COMM, d->game->get_sysdata()->log_level,
            "Denying access to %s@%s.", argument.c_str(), d->host );
        if ( d->newstate != 0 )
        {
            output_to_descriptor( d, "That name is already taken.  Please choose another: " );
            d->connected = CON_GET_NAME;
            return;
        }
        output_to_descriptor( d, "You are denied access.\n" );
        close_socket( d, FALSE );
        return;
    }

    chk = check_reconnect( d, argument, FALSE );
    if ( chk == BERR )
        return;

    if ( chk )
    {
        fOld = TRUE;
    }
    else
    {
        if ( wizlock && !IS_IMMORTAL( ch ) )
        {
            output_to_descriptor( d, "The game is wizlocked.  Only immortals can connect now.\n" );
            output_to_descriptor( d, "Please try back later.\n" );
            close_socket( d, FALSE );
            return;
        }
    }

    if ( fOld )
    {
        if ( d->newstate != 0 )
        {
            output_to_descriptor( d, "That name is already taken.  Please choose another: " );
            d->connected = CON_GET_NAME;
            return;
        }

        /* Old player */
        output_to_descriptor( d, "Password: " );
        output_to_descriptor( d, (const char *)echo_off_str );
        d->connected = CON_GET_OLD_PASSWORD;
        return;
    }
    else
    {
        if ( check_bad_name( ch->name ) )
        {
            output_to_descriptor( d, "\nThat name is unacceptable, please choose another.\n" );
            output_to_descriptor( d, "Name: " );
            d->connected = CON_GET_NAME;
            return;
        }

        output_to_descriptor( d, "\nI don't recognize your name, you must be new here.\n\n" );
        output_to_descriptor( d,
            str_printf("Did I get that right, %s (Y/N)? ", argument.c_str()) );
        d->connected = CON_CONFIRM_NEW_NAME;
        return;
    }
}

static void nanny_get_old_password( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch;
    std::string name;
    bool fOld;
    short chk;
    char pwdhash[65];

    ch = d->character;

    output_to_descriptor( d, "\n" );

    /* New SHA-256 password checking - AI/DV 3-12-26 */
    sha256_hash( argument.c_str(), pwdhash );

    if ( strcmp( pwdhash, ch->pcdata->pwd ) )
    {
        output_to_descriptor( d, "Wrong password.\n" );
        /* clear descriptor pointer to get rid of bug message in log */
        d->character->desc = NULL;
        close_socket( d, FALSE );
        return;
    }

    output_to_descriptor( d, (const char*) echo_on_str );

    if ( check_playing( d, ch->name, TRUE ) )
        return;

    chk = check_reconnect( d, ch->name, TRUE );
    if ( chk == BERR )
    {
        if ( d->character && d->character->desc )
            d->character->desc = NULL;
        close_socket( d, FALSE );
        return;
    }
    if ( chk == TRUE )
        return;

    if ( check_multi( d, ch->name ) )
    {
        close_socket( d, FALSE );
        return;
    }

    name = ch->name;
    d->character->desc = NULL;
    free_char( d->character );
    fOld = load_char_obj( d, str_printf("%s", name.c_str()), FALSE );
   if( !fOld )
      bug( "%s: failed to load_char_obj for %s.", __func__, name.c_str() );

    ch = d->character;

    if ( ch->top_level < LEVEL_DEMI )
        log_printf_plus( LOG_COMM, d->game->get_sysdata()->log_level,
            "%s@%s(%s) has connected.", ch->name, d->host, d->user );
    else
        log_printf_plus( LOG_COMM, ch->top_level,
            "%s@%s(%s) has connected.", ch->name, d->host, d->user );

    show_title( d );
    if ( ch->pcdata->area )
        do_loadarea( ch, "" );
}

static void nanny_confirm_new_name( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch;

    ch = d->character;

    switch ( argument[0] )
    {
        case 'y':
        case 'Y':
            output_to_descriptor( d, 
                    str_printf( "\nMake sure to use a password that won't be easily guessed by someone else."          
                        "\nPick a good password for %s: %s",
                        ch->name, echo_off_str ));
            d->connected = CON_GET_NEW_PASSWORD;
            break;

        case 'n':
        case 'N':
            output_to_descriptor( d, "Ok, what IS it, then? " );
            /* clear descriptor pointer to get rid of bug message in log */
            d->character->desc = NULL;
            free_char( d->character );
            d->character = NULL;
            d->connected = CON_GET_NAME;
            break;

        default:
            output_to_descriptor( d, "Please type Yes or No. " );
            break;
    }
}

static void nanny_get_new_password( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    char pwdnewhash[65];

    ch = d->character;

    output_to_descriptor( d, "\n" );

    if ( visible_length( argument ) < 5 )
    {
        output_to_descriptor( d,
            "Password must be at least five characters long.\nPassword: " );
        return;
    }

    /* New SHA-256 password hashing - AI/DV 3-12-26 */
    sha256_hash( argument.c_str(), pwdnewhash );
    pwdnew = pwdnewhash;

    for ( p = pwdnew; *p != '\0'; p++ )
    {
        if ( *p == '~' )
        {
            output_to_descriptor( d,
                "New password not acceptable, try again.\nPassword: " );
            return;
        }
    }

    STR_DISPOSE( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );
    output_to_descriptor( d, "\nPlease retype the password to confirm: " );
    d->connected = CON_CONFIRM_NEW_PASSWORD;
}

static void nanny_confirm_new_password( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch;
    char *p;
    char pwdnewhash[65];

    ch = d->character;

    output_to_descriptor( d, "\n" );

    sha256_hash( argument.c_str(), pwdnewhash );

    for ( p = pwdnewhash; *p != '\0'; p++ )
    {
        if ( *p == '~' )
        {
            output_to_descriptor( d,
                "New password not acceptable, try again.\nPassword: " );
            d->connected = CON_GET_NEW_PASSWORD;
            return;
        }
    }

    if ( strcmp( pwdnewhash, ch->pcdata->pwd ) != 0 )
    {
        output_to_descriptor( d,
            "Passwords don't match.\nRetype password: " );
        d->connected = CON_GET_NEW_PASSWORD;
        return;
    }

    output_to_descriptor( d, (const char*) echo_on_str );
    output_to_descriptor( d, "\nWhat is your sex (M/F/N)? " );
    d->connected = CON_GET_NEW_SEX;
}

static void nanny_get_new_sex( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch;
    int iRace, halfmax;

    ch = d->character;

    switch ( argument[0] )
    {
        case 'm':
        case 'M':
            ch->sex = SEX_MALE;
            break;

        case 'f':
        case 'F':
            ch->sex = SEX_FEMALE;
            break;

        case 'n':
        case 'N':
            ch->sex = SEX_NEUTRAL;
            break;

        default:
            output_to_descriptor( d, "That's not a sex.\nWhat IS your sex? " );
            return;
    }

    output_to_descriptor( d,
        "\nYou may choose from the following races, or type showstat [race] to learn more:\n" );

    halfmax = ( MAX_RACE / 3 ) + 1;

    for ( iRace = 0; iRace < halfmax; iRace++ )
    {
        if ( iRace == RACE_GOD )
            continue;

        if ( race_table[iRace].race_name[0] != '\0' )
        {
            std::string line;

            line += str_printf( "%-20s", race_table[iRace].race_name );
            line += str_printf( "%-20s", race_table[iRace + halfmax].race_name );

            if ( iRace + ( halfmax * 2 ) < MAX_RACE )
                line += str_printf( "%s", race_table[iRace + ( halfmax * 2 )].race_name );

            line += "\n";
            output_to_descriptor( d, line );
        }
    }

    output_to_descriptor( d, ": " );
    d->connected = CON_GET_NEW_RACE;
}

static void nanny_get_new_race( DESCRIPTOR_DATA *d, const std::string& argumentin )
{
    std::string arg;
    CHAR_DATA *ch;
    int iRace;
    int iClass, halfmax;

    ch = d->character;

    std::string argument = argumentin;
    argument = one_argument( argument, arg );
    if ( !str_cmp( arg, "help" ) )
    {
        if (argument.empty())
            do_help( ch, "race" );
        do_help( ch, (char*)argument.c_str() );
        output_to_descriptor( d, "Please choose a race: " );
        return;
    }
    if ( !str_cmp( arg, "showstat" ) )
    {
        do_showstatistic( ch, (char*)argument.c_str() );
        output_to_descriptor( d, "Please choose a race: " );
        return;
    }

    for ( iRace = 0; iRace < MAX_RACE; iRace++ )
    {
        if ( toupper( arg[0] ) == toupper( race_table[iRace].race_name[0] )
        && !str_prefix( arg, race_table[iRace].race_name ) )
        {
            ch->race = iRace;
            break;
        }
    }

    if ( iRace == RACE_ASSASSIN_DROID || iRace == RACE_YEVETHA || iRace == RACE_TOGARIAN )
    {
        output_to_descriptor( d,
            "Do to too many people choosing this race, it now must be applied for.\nWhat IS your race? " );
        return;
    }

    if ( iRace == MAX_RACE || iRace == RACE_GOD
    || race_table[iRace].race_name[0] == '\0' )
    {
        output_to_descriptor( d, "That's not a race.\nWhat IS your race? " );
        return;
    }

    output_to_descriptor( d,
        "\nPlease choose a main ability from the folowing classes:\n" );
    halfmax = ( MAX_ABILITY / 2 ) + 1;

    for ( iClass = 0; iClass < halfmax; iClass++ )
    {
        if ( ability_name[iClass] && ability_name[iClass][0] != '\0' )
        {
            std::string line;

            line += str_printf( "%-20s", ability_name[iClass] );

            if ( iClass + halfmax < MAX_ABILITY
            && ability_name[iClass + halfmax]
            && ability_name[iClass + halfmax][0] != '\0' )
            {
                line += str_printf( "%s", ability_name[iClass + halfmax] );
            }

            line += "\n";
            output_to_descriptor( d, line );
        }
    }

    output_to_descriptor( d, ": " );
    d->connected = CON_GET_NEW_CLASS;
}

static void nanny_get_new_class( DESCRIPTOR_DATA *d, const std::string& argumentin )
{
    std::string arg;
    CHAR_DATA *ch;
    int iClass;

    ch = d->character;

    std::string argument = argumentin;
    argument = one_argument( argument, arg );
    if ( !str_cmp( arg, "help" ) )
    {
        if ( argument.empty())
            do_help( ch, "classes" );
        do_help( ch, (char*)argument.c_str() );
        output_to_descriptor( d, "Please choose an ability class: " );
        return;
    }

    for ( iClass = 0; iClass < MAX_ABILITY; iClass++ )
    {
        if ( toupper( arg[0] ) == toupper( ability_name[iClass][0] )
        && !str_prefix( arg, ability_name[iClass] ) )
        {
            ch->main_ability = iClass;
            break;
        }
    }

    if ( iClass == MAX_ABILITY || iClass == 7
    || !ability_name[iClass] || ability_name[iClass][0] == '\0' )
    {
        output_to_descriptor( d,
            "That's not a skill class.\nWhat IS it going to be? " );
        return;
    }

    output_to_descriptor( d, "\nRolling stats....\n" );

    /* Original code fell through to CON_ROLL_STATS here. */
    nanny_roll_stats( d );
}

static void roll_new_character_stats( CHAR_DATA *ch )
{
    ch->perm_str = number_range( 1, 6 ) + number_range( 1, 6 ) + number_range( 1, 6 );
    ch->perm_int = number_range( 3, 6 ) + number_range( 1, 6 ) + number_range( 1, 6 );
    ch->perm_wis = number_range( 3, 6 ) + number_range( 1, 6 ) + number_range( 1, 6 );
    ch->perm_dex = number_range( 3, 6 ) + number_range( 1, 6 ) + number_range( 1, 6 );
    ch->perm_con = number_range( 3, 6 ) + number_range( 1, 6 ) + number_range( 1, 6 );
    ch->perm_cha = number_range( 3, 6 ) + number_range( 1, 6 ) + number_range( 1, 6 );

    ch->perm_str += race_table[ch->race].str_plus;
    ch->perm_int += race_table[ch->race].int_plus;
    ch->perm_wis += race_table[ch->race].wis_plus;
    ch->perm_dex += race_table[ch->race].dex_plus;
    ch->perm_con += race_table[ch->race].con_plus;
    ch->perm_cha += race_table[ch->race].cha_plus;
}

static void show_new_character_stats( DESCRIPTOR_DATA *d, CHAR_DATA *ch, const std::string& prompt )
{
    output_to_descriptor( d,
        str_printf(
            "\nSTR: %d  INT: %d  WIS: %d  DEX: %d  CON: %d  CHA: %d\n",
            ch->perm_str, ch->perm_int, ch->perm_wis,
            ch->perm_dex, ch->perm_con, ch->perm_cha
        ) );

    output_to_descriptor( d, prompt );
}

static void nanny_roll_stats( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch = d->character;

    roll_new_character_stats( ch );
    show_new_character_stats( d, ch, "\nAre these stats OK?. " );
    d->connected = CON_STATS_OK;
}

static void nanny_stats_ok( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch = d->character;

    switch ( argument[0] )
    {
        case 'y':
        case 'Y':
            break;

        case 'n':
        case 'N':
            roll_new_character_stats( ch );
            show_new_character_stats( d, ch, "\nOK?. " );
            return;

        default:
            output_to_descriptor( d, "Invalid selection.\nYES or NO? " );
            return;
    }

    output_to_descriptor( d,
        "\nWould you like ANSI or no graphic/color support, (A/N)? " );
    d->connected = CON_GET_WANT_RIPANSI;
}

static void nanny_get_want_ripansi( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch;

    ch = d->character;

    switch ( argument[0] )
    {
        case 'a':
        case 'A':
            BV_SET_BIT( ch->act, PLR_ANSI );
            break;

        case 'n':
        case 'N':
            break;

        default:
            output_to_descriptor( d, "Invalid selection.\nANSI or NONE? " );
            return;
    }

    d->connected = CON_GET_MSP;
//    output_to_descriptor( d, "Does your mud client have the Mud Sound Protocol? " );
    nanny_get_msp( d, "n" ); /* default to no MSP, because who uses it? DV 4-8-26*/
}

static void nanny_get_msp( DESCRIPTOR_DATA *d, const std::string& argument )
{
    std::string buf;
    CHAR_DATA *ch;
    int ability;

    ch = d->character;

    switch ( argument[0] )
    {
        case 'y':
        case 'Y':
            BV_SET_BIT( ch->act, PLR_SOUND );
            break;

        case 'n':
        case 'N':
            break;

        default:
            output_to_descriptor( d, "Invalid selection.\nYES or NO? " );
            return;
    }

    buf = str_printf( "%s@%s new %s.", ch->name, d->host,
        race_table[ch->race].race_name );
    log_string_plus( buf.c_str(), LOG_COMM, d->game->get_sysdata()->log_level );
    to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
    output_to_descriptor( d, "Press [ENTER] " );
    show_title( d );

    for ( ability = 0; ability < MAX_ABILITY; ability++ )
        ch->skill_level[ability] = 0;

    ch->top_level = 0;
    ch->position = POS_STANDING;
    d->connected = CON_PRESS_ENTER;
    return;
}

static void nanny_press_enter( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch;

    (void)argument;
    ch = d->character;

    if ( BV_IS_SET( ch->act, PLR_ANSI ) )
        send_to_pager( "\033[2J", ch );
    else
        send_to_pager( "\014", ch );

    if ( IS_IMMORTAL( ch ) )
    {
        send_to_pager( "&WImmortal Message of the Day&w\n", ch );
        do_help( ch, "imotd" );
    }
    if ( ch->top_level > 0 )
    {
        send_to_pager( "\n&WMessage of the Day&w\n", ch );
        do_help( ch, "motd" );
    }
    if ( ch->top_level >= 100 )
    {
        send_to_pager( "\n&WAvatar Message of the Day&w\n", ch );
        do_help( ch, "amotd" );
    }
    if ( ch->top_level == 0 )
        do_help( ch, "nmotd" );

    send_to_pager( "\n&WPress [ENTER] &Y", ch );
    d->connected = CON_READ_MOTD;
}

static void nanny_enter_playing_state( DESCRIPTOR_DATA *d, CHAR_DATA *ch )
{
    output_to_descriptor( d, "\nWelcome to Rise in Power...\n\n" );
    add_char( ch );
    d->connected = CON_PLAYING;

    if ( !IS_NPC( ch ) && BV_IS_SET( ch->act, PLR_SOUND ) )
        send_to_char( "!!MUSIC(starwars.mid V=100)", ch );
}

static void nanny_initialize_new_character_language( CHAR_DATA *ch )
{
    int lang;
    int sn;

    lang = race_table[ch->race].language;

    if ( !VALID_LANG( lang ) )
    {
        bug( "Nanny: invalid racial language." );
        return;
    }

    sn = lang_sn[lang];
    if ( sn < 0 )
    {
        bug( "Nanny: cannot find racial language skill." );
        return;
    }

    ch->pcdata->learned[sn] = 100;
    ch->speaking = lang;

    /* Quarren special case */
    if ( ch->race == RACE_QUARREN )
    {
        int qlang = LANG_QUARREN;
        int qsn = lang_sn[qlang];

        if ( qsn >= 0 )
        {
            ch->pcdata->learned[qsn] = 100;
            BV_SET_BIT( ch->speaks, qlang );
        }
    }

    /* Mon Calamari special case */
    if ( ch->race == RACE_MON_CALAMARI )
    {
        int clang = LANG_COMMON;
        int csn = lang_sn[clang];

        if ( csn >= 0 )
            ch->pcdata->learned[csn] = 100;
    }
}

static void nanny_initialize_new_character_stats( CHAR_DATA *ch )
{
    int ability;

    ch->pcdata->clan_name = STRALLOC( "" );
    ch->pcdata->clan = NULL;

    ch->perm_lck = number_range( 6, 20 );
    ch->perm_frc = number_range( -800, 20 );
    ch->affected_by = race_table[ch->race].affected;
    ch->perm_lck += race_table[ch->race].lck_plus;
    ch->perm_frc += race_table[ch->race].frc_plus;

    if ( ch->main_ability == FORCE_ABILITY )
        ch->perm_frc = 5;
    else
        ch->perm_frc = URANGE( 0, ch->perm_frc, 20 );

    /* Hunters do not recieve force */
    if ( ch->main_ability == HUNTING_ABILITY )
        ch->perm_frc = 0;

    /* Droids do not recieve force */
    if ( is_droid( ch ) )
        ch->perm_frc = 0;

    ch->resistant = race_table[ch->race].resist;
    ch->susceptible = race_table[ch->race].suscept;

    name_stamp_stats( ch );

    for ( ability = 0; ability < MAX_ABILITY; ability++ )
    {
        ch->skill_level[ability] = 1;
        ch->experience[ability] = 0;
    }

    ch->top_level = 1;
    ch->hit = ch->max_hit;
    ch->hit += race_table[ch->race].hit;
    ch->max_hit += race_table[ch->race].hit;
    ch->move = ch->max_move;
    ch->gold = 5000;

    if ( ch->perm_frc > 0 )
        ch->max_mana = 100 + 100 * ch->perm_frc;
    else
        ch->max_mana = 0;

    ch->max_mana += race_table[ch->race].mana;
    ch->mana = ch->max_mana;
}

static void nanny_give_new_character_equipment( CHAR_DATA *ch )
{
    OBJ_DATA *obj;

    /* Added by Narn. Start new characters with autoexit and autgold on */
    BV_SET_BIT( ch->act, PLR_AUTOGOLD );
    BV_SET_BIT( ch->act, PLR_AUTOEXIT );

    /* New players don't have to earn some eq */
    obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_BANNER ), 0 );
    obj_to_char( obj, ch );
    equip_char( ch, obj, WEAR_LIGHT );

    obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_DAGGER ), 0 );
    obj_to_char( obj, ch );
    equip_char( ch, obj, WEAR_WIELD );

    /* comlink */
    {
        OBJ_INDEX_DATA *obj_ind = get_obj_index( 10424 );
        if ( obj_ind != NULL )
        {
            obj = create_object( obj_ind, 0 );
            obj_to_char( obj, ch );
        }
    }
}

static void nanny_initialize_new_character( CHAR_DATA *ch )
{
    std::string buf;

    nanny_initialize_new_character_language( ch );
    nanny_initialize_new_character_stats( ch );

    buf = str_printf("%s the %s", ch->name, race_table[ch->race].race_name );
    set_title( ch, buf );

    nanny_give_new_character_equipment( ch );

    if ( !ch->game->get_sysdata()->wait_for_auth )
    {
        char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
        ch->pcdata->auth_state = 3;
    }
    else
    {
        char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
        ch->pcdata->auth_state = 1;
        BV_SET_BIT( ch->pcdata->flags, PCFLAG_UNAUTHED );
    }
}

static bool nanny_try_place_in_ship_room( CHAR_DATA *ch )
{
    SHIP_DATA *ship;

    if ( !ch->in_room )
        return FALSE;

    for ( ship = first_ship; ship; ship = ship->next )
    {
        if ( ch->in_room->vnum >= ship->firstroom
        &&   ch->in_room->vnum <= ship->lastroom )
        {
            if ( ship->shipclass != SHIP_PLATFORM || ship->spaceobject )
            {
                char_to_room( ch, ch->in_room );
                return TRUE;
            }
        }
    }

    return FALSE;
}

static void nanny_place_character( CHAR_DATA *ch )
{
    if ( ch->top_level == 0 )
    {
        nanny_initialize_new_character( ch );
        return;
    }

    if ( !IS_IMMORTAL( ch ) && ch->pcdata->release_date > current_time )
    {
        if ( ch->pcdata->jail_vnum )
            char_to_room( ch, get_room_index( ch->pcdata->jail_vnum ) );
        else
            char_to_room( ch, get_room_index( 6 ) );
        return;
    }

    if ( ch->in_room && !IS_IMMORTAL( ch )
    && !BV_IS_SET( ch->in_room->room_flags, ROOM_SPACECRAFT )
    && ch->in_room != get_room_index( 6 ) )
    {
        char_to_room( ch, ch->in_room );
        return;
    }

    if ( ch->in_room && !IS_IMMORTAL( ch )
    && BV_IS_SET( ch->in_room->room_flags, ROOM_SPACECRAFT )
    && ch->in_room != get_room_index( 6 ) )
    {
        if ( nanny_try_place_in_ship_room( ch ) )
            return;
    }

    char_to_room( ch, get_room_index( wherehome( ch ) ) );
}

static void nanny_clear_login_flags_and_timers( CHAR_DATA *ch )
{
    if ( BV_IS_SET( ch->act, ACT_POLYMORPHED ) )
        BV_REMOVE_BIT( ch->act, ACT_POLYMORPHED );

    if ( BV_IS_SET( ch->act, PLR_QUESTOR ) )
        BV_REMOVE_BIT( ch->act, PLR_QUESTOR );

    if ( get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
        remove_timer( ch, TIMER_SHOVEDRAG );

    if ( get_timer( ch, TIMER_PKILLED ) > 0 )
        remove_timer( ch, TIMER_PKILLED );
}

static void nanny_load_player_home_storage( DESCRIPTOR_DATA *d, CHAR_DATA *ch )
{
    std::string buf;
    FILE *fph;
    ROOM_INDEX_DATA *storeroom;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    if ( ch->plr_home == NULL )
        return;

    storeroom = ch->plr_home;

    for ( obj = storeroom->first_content; obj; obj = obj_next )
    {
        obj_next = obj->next_content;
        extract_obj( obj );
    }

    buf = str_printf("%s%c/%s.home", PLAYER_DIR, tolower( ch->name[0] ),
        capitalize( ch->name ).c_str() );

    if ( ( fph = fopen( buf.c_str(), "r" ) ) == NULL )
        return;

    rset_supermob( storeroom );

    for ( ; ; )
    {
        char letter;
        char *word;

        letter = fread_letter( fph );
        if ( letter == '*' )
        {
            fread_to_eol( fph );
            continue;
        }

        if ( letter != '#' )
        {
            bug( "Load_plr_home: # not found.", 0 );
            bug( ch->name, 0 );
            break;
        }

        word = fread_word( fph );
        if ( !str_cmp( word, "OBJECT" ) )
            fread_obj( d->game, supermob, fph, OS_CARRY );
        else if ( !str_cmp( word, "END" ) )
            break;
        else
        {
            bug( "Load_plr_home: bad section.", 0 );
            bug( ch->name, 0 );
            break;
        }
    }

    FCLOSE( fph );

    {
        OBJ_DATA *tobj, *tobj_next;

        for ( tobj = supermob->first_carrying; tobj; tobj = tobj_next )
        {
            tobj_next = tobj->next_content;
            obj_from_char( tobj );
            if ( tobj->item_type != ITEM_MONEY )
                obj_to_room( tobj, storeroom );
        }
    }

    release_supermob( d->game );
}

static void nanny_restore_pet( CHAR_DATA *ch )
{
    if ( !ch->pcdata->pet )
        return;

    act( AT_ACTION, "$n returns with $s master.",
        ch->pcdata->pet, NULL, ch, TO_NOTVICT );
    act( AT_ACTION, "$N returns with you.",
        ch, NULL, ch->pcdata->pet, TO_CHAR );
}

static void nanny_finish_login( CHAR_DATA *ch )
{
    ch->pcdata->logon = current_time;

    act( AT_ACTION, "$n has entered the game.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );
    mail_count( ch );
    gmcp_force_resync( ch );
}

static void nanny_read_motd( DESCRIPTOR_DATA *d, const std::string& argument )
{
    CHAR_DATA *ch;

    (void)argument;
    ch = d->character;

    nanny_enter_playing_state( d, ch );
    nanny_place_character( ch );
    nanny_clear_login_flags_and_timers( ch );
    nanny_load_player_home_storage( d, ch );
    nanny_restore_pet( ch );
    nanny_finish_login( ch );
}


/*
 * Parse a name for acceptability.
 */
bool check_parse_name( const std::string& name )
{
//    log_printf("Check_parse_name: %s",name);  
    /*
     * Reserved words.
     */
    if ( is_name( name, "all auto someone immortal self god supreme demigod dog guard cityguard cat cornholio spock hicaine hithoric death ass fuck shit piss crap quit" ) )
	return FALSE;

    /*
     * Length restrictions.
     */
    if ( visible_length(name) <  3 )
	    return FALSE;

    if ( visible_length(name) > 20 )
	    return FALSE;

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
	const char *pc;
	bool fIll;

	fIll = TRUE;
	for ( pc = name.c_str(); *pc != '\0'; pc++ )
	{
	    if ( !isalpha(*pc) )
		    return FALSE;
	    if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
		fIll = FALSE;
	}

	if ( fIll )
	    return FALSE;
    }

    /*
     * Code that followed here used to prevent players from naming
     * themselves after mobs... this caused much havoc when new areas
     * would go in...
     */

    return TRUE;
}



/*
 * Look for link-dead player to reconnect.
 */
short check_reconnect( DESCRIPTOR_DATA *d, const std::string& name, bool fConn )
{
    CHAR_DATA *ch;

    for ( ch = first_char; ch; ch = ch->next )
    {
	if ( !IS_NPC(ch)
	&& ( !fConn || !ch->desc )
	&&    ch->name
	&&   !str_cmp( name, ch->name ) )
	{
	    if ( fConn && ch->switched )
	    {
	      output_to_descriptor( d, "Already playing.\nName: " );
	      d->connected = CON_GET_NAME;
	      if ( d->character )
	      {
		 /* clear descriptor pointer to get rid of bug message in log */
		 d->character->desc = NULL;
		 free_char( d->character );
		 d->character = NULL;
	      }
	      return BERR;
	    }
	    if ( fConn == FALSE )
	    {
		STR_DISPOSE( d->character->pcdata->pwd );
		d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
	    }
	    else
	    {
		/* clear descriptor pointer to get rid of bug message in log */
		d->character->desc = NULL;
		free_char( d->character );
		d->character = ch;
		ch->desc	 = d;
		ch->timer	 = 0;
		send_to_char( "Reconnecting.\n", ch );
		act( AT_ACTION, "$n has reconnected.", ch, NULL, NULL, TO_ROOM );
		log_printf_plus( LOG_COMM, UMAX( d->game->get_sysdata()->log_level, ch->top_level ), "%s@%s(%s) reconnected.", ch->name, d->host, d->user );

		d->connected = CON_PLAYING;
	    }
	    return TRUE;
	}
    }

    return FALSE;
}



/*
 * Check if already playing.
 */
 
bool check_multi( DESCRIPTOR_DATA *d , const std::string& name )
{
        DESCRIPTOR_DATA *dold;
        
    for ( dold = first_descriptor; dold; dold = dold->next )
    {
	if ( dold != d
	&& (  dold->character || dold->original )
	&&   str_cmp( name, dold->original
		 ? dold->original->name : dold->character->name ) 
        && !str_cmp(dold->host , d->host ) )
	{
/*	        const char *ok = "";
	        const char *ok2 = "";
	        int iloop;
*/
	        
	        if ( d->character->top_level >= LEVEL_LESSER 
		|| ( dold->original ? dold->original : dold->character )->top_level >= LEVEL_LESSER )
		    return FALSE;
/*		for ( iloop = 0 ; iloop < 11 ; iloop++ )
	        {
	            if ( ok[iloop] != d->host[iloop] )
	              break;
	        }
	        if ( iloop >= 10 )
	           return FALSE; 
		for ( iloop = 0 ; iloop < 11 ; iloop++ )
	        {
	            if ( ok2[iloop] != d->host[iloop] )
	              break;
	        }
	        if ( iloop >= 10 )
	           return FALSE;
*/
		output_to_descriptor( d, "Sorry multi-playing is not allowed ... have you other character quit first.\n" );
		log_printf_plus( LOG_COMM, d->game->get_sysdata()->log_level, "%s attempting to multiplay with %s.", dold->original ? dold->original->name : dold->character->name , d->character->name );

	        d->character = NULL;
	        free_char( d->character );
	        return TRUE;
	}
    }

    return FALSE;

}                

short check_playing( DESCRIPTOR_DATA *d, const std::string& name, bool kick )
{
    CHAR_DATA *ch;

    DESCRIPTOR_DATA *dold;
    int	cstate;

    for ( dold = first_descriptor; dold; dold = dold->next )
    {
	if ( dold != d
	&& (  dold->character || dold->original )
	&&   !str_cmp( name, dold->original
		 ? dold->original->name : dold->character->name ) )
	{
	    cstate = dold->connected;
	    ch = dold->original ? dold->original : dold->character;
	    if ( !ch->name
	    || ( cstate != CON_PLAYING && cstate != CON_EDITING ) )
	    {
        output_to_descriptor( d, "Already connected - try again.\n" );
        log_printf_plus( LOG_COMM, d->game->get_sysdata()->log_level, "%s already connected.", ch->name );
        return BERR;
	    }
	    if ( !kick )
	      return TRUE;
	    output_to_descriptor( d, "Already playing... Kicking off old connection.\n" );
	    output_to_descriptor( dold, "Kicking off old connection... bye!\n" );
	    close_socket( dold, FALSE );
	    /* clear descriptor pointer to get rid of bug message in log */
	    d->character->desc = NULL;
	    free_char( d->character );
	    d->character = ch;
	    ch->desc	 = d;
	    ch->timer	 = 0;
	    if ( ch->switched )
	      do_return( ch->switched, "" );
	    ch->switched = NULL;
	    send_to_char( "Reconnecting.\n", ch );
	    act( AT_ACTION, "$n has reconnected, kicking off old link.",
	         ch, NULL, NULL, TO_ROOM );
	    log_printf_plus( LOG_COMM, UMAX( d->game->get_sysdata()->log_level, ch->top_level ), "%s@%s reconnected, kicking off old link.",
	             ch->name, d->host );
	    d->connected = cstate;
	    return TRUE;
	}
    }

    return FALSE;
}



void stop_idling( CHAR_DATA *ch )
{
    if ( !ch
    ||   !ch->desc
    ||    ch->desc->connected != CON_PLAYING
    ||   !ch->was_in_room
    ||    ch->in_room != get_room_index( ROOM_VNUM_LIMBO ) )
	return;

    ch->timer = 0;
    char_from_room( ch );
    char_to_room( ch, ch->was_in_room );
    ch->was_in_room	= NULL;
    act( AT_ACTION, "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
    return;
}

void send_to_char( const std::string& txt, CHAR_DATA *ch )
{
    if ( !ch )
    {
      bug( "Send_to_char: NULL *ch" );
      return;
    }
    if ( !txt.empty() && ch->desc )
	    output_to_descriptor( ch->desc, txt );
    return;
}

inline void write_to_pager( DESCRIPTOR_DATA *d, const std::string& txt )
{
    write_to_pager( d, txt.data(), static_cast<int>( txt.size() ) );
}

void write_to_pager( DESCRIPTOR_DATA *d, const char *txt, int length )
{
    if ( !d || !txt )
        return;

    if ( length <= 0 )
        length = static_cast<int>( strlen( txt ) );

    if ( length <= 0 )
        return;

    if ( d->pagepoint == PAGEPOINT_NULL )
    {
        d->pagepoint = 0;
        d->pagecmd = '\0';
        d->pagebuf.clear();
    }

    if ( d->pagebuf.empty() && !d->fcommand )
        d->pagebuf.push_back( '\n' );

    /*
     * HARD CAP
     */
    if ( d->pagebuf.size() + static_cast<size_t>( length ) + 1 >= 32000 )
    {
        bug( "Pager overflow. Ignoring.\n" );
        d->pagebuf.clear();
        d->pagepoint = PAGEPOINT_NULL;
        return;
    }

    d->pagebuf.append( txt, static_cast<size_t>( length ) );
}

int build_ansi_from_atype(sh_int AType, char *buf)
{
    if ( AType == 7 )
        return mud_snprintf_runtime(buf, 64, "\033[m");
    return mud_snprintf_runtime(buf, 64, "\033[0;%d;%s%dm",
        (AType & 8) == 8,
        (AType > 15 ? "5;" : ""),
        (AType & 7) + 30);
}

void send_to_pager( const char *txt, CHAR_DATA *ch )
{
  if ( !ch )
  {
    bug( "Send_to_pager: NULL *ch" );
    return;
  }
  if ( txt && ch->desc )
  {
    DESCRIPTOR_DATA *d = ch->desc;
    
    ch = d->original ? d->original : d->character;
    if ( IS_NPC(ch) || !BV_IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
    {
	    send_to_char(std::string(txt), d->character);
	    return;
    }
    if ( d->pagecolor_dirty )
    {
        char buf[64];
        int len;

        len = build_ansi_from_atype(d->pagecolor, buf);

        write_to_pager(d, buf, len);

        d->pagecolor_dirty = FALSE;
        d->pagecolor_pending = d->pagecolor;
    }    
    write_to_pager(d, txt, 0);
  }
  return;
}

void send_to_pager( const std::string& txt, CHAR_DATA *ch )
{
  if ( !ch )
  {
    bug( "Send_to_pager: NULL *ch" );
    return;
  }
  if ( !txt.empty() && ch->desc )
  {
    DESCRIPTOR_DATA *d = ch->desc;
    
    ch = d->original ? d->original : d->character;
    if ( IS_NPC(ch) || !BV_IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
    {
	    send_to_char(std::string(txt), d->character);
	    return;
    }
    if ( d->pagecolor_dirty )
    {
        char buf[64];
        int len;

        len = build_ansi_from_atype(d->pagecolor, buf);

        write_to_pager(d, buf, len);

        d->pagecolor_dirty = FALSE;
        d->pagecolor_pending = d->pagecolor;
    }    
    write_to_pager(d, txt);
  }
  return;
}

void set_char_color(sh_int AType, CHAR_DATA *ch)
{
    CHAR_DATA *och;
    DESCRIPTOR_DATA *d;    
    
    if (!ch || !ch->desc)
        return;
    
    d = ch->desc;
    och = (d->original ? d->original : ch);

    if (!IS_NPC(och) && BV_IS_SET(och->act, PLR_ANSI))
    {
        unsigned char newcolor;

        if (AType == 7)
            newcolor = 0x07; /* reset */
        else
        {
            newcolor = d->rendercolor & 0x70;

            if (AType & 8)
                newcolor |= 0x08;

            if (AType > 15)
                newcolor |= 0x80;

            newcolor |= (AType & 7);
        }

        /* ✅ ONLY update state — DO NOT EMIT ANSI */
        d->rendercolor = newcolor;
        d->has_rendercolor = true;
        //d->last_sent_color = newcolor;
    }
}

void set_pager_color( sh_int AType, CHAR_DATA *ch )
{
//  char buf[16];
    CHAR_DATA *och;
    
    if ( !ch || !ch->desc )
      return;
    
    och = (ch->desc->original ? ch->desc->original : ch);
    if ( IS_NPC(och) || !BV_IS_SET(och->act, PLR_ANSI) )
        return;

    /* ✅ Only store pager color state */
    ch->desc->pagecolor = AType;
    ch->desc->pagecolor_dirty = TRUE;

    return;
}


/* source: EOD, by John Booth <???> */
void ch_printf_( CHAR_DATA *ch, const char *fmt, ... )
{
    va_list args;
    va_start( args, fmt );

    std::string out = vstr_printf( fmt, args );

    va_end( args );

    send_to_char( out, ch );
}

void pager_printf(CHAR_DATA *ch, const char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH * 2];
    va_list args;

    if (!ch)
        return;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    send_to_pager(buf, ch);
}



char *obj_short( OBJ_DATA *obj )
{
    static char buf[MAX_STRING_LENGTH];

    if ( obj->count > 1 )
    {
      SPRINTF( buf, "%s (%d)", obj->short_descr, obj->count );
      return buf;
    }
    return obj->short_descr;
}
//Added stripping the 'a' and 'an' from object short descriptions for act() DV 4-12-26
std::string strip_aoran( const std::string &input )
{
    if ( input.size() >= 2 )
    {
        // Check for "a "
        if ( (input[0] == 'a' || input[0] == 'A') &&
             input[1] == ' ' )
        {
            return input.substr( 2 );
        }
    }

    if ( input.size() >= 3 )
    {
        // Check for "an "
        if ( (input[0] == 'a' || input[0] == 'A') &&
             (input[1] == 'n' || input[1] == 'N') &&
             input[2] == ' ' )
        {
            return input.substr( 3 );
        }
    }

    return input;
}

/*
 * The primary output interface for formatted output.
 */
#define NAME(ch) (IS_NPC(ch) ? ch->short_descr : ch->name)

/*
 * act_string()
 *
 * Expands the $ codes used by act() into readable text.
 *
 * Example format string:
 *
 *   "$n gives $p to $N."
 *
 * Which might become:
 *
 *   "Bob gives a sword to Alice."
 *
 * Parameters:
 *
 *   format  : original format string containing $ tokens
 *   to      : character receiving the message
 *   ch      : acting character
 *   arg1    : usually object or string ($p or $t)
 *   arg2    : usually victim or string ($N or $T)
 *
 * Returns:
 *
 *   std::string containing the formatted message
 *
 * Safety improvements:
 *
 *   • Eliminates static buffer usage
 *   • Prevents buffer overflow via std::string growth
 *   • Protects against NULL pointers
 *   • Handles missing arg1/arg2 safely
 *   • Preserves original behavior and formatting
 */

static const char *he_she[]  = { "it",  "he",  "she" };
static const char *him_her[] = { "it",  "him", "her" };
static const char *his_her[] = { "its", "his", "her" };

std::string act_string( const char *format, CHAR_DATA *to, CHAR_DATA *ch,
                        const void *arg1, const void *arg2 )
{
    /* Temporary buffer used by some tokens (ex: $d) */
    std::string fname;

    /* Pointer to current position in format string */
    const char *str = format;

    /* Pointer to the string we will append */
    const char *i = nullptr;

    /* Output string (replaces static buffer) */
    std::string out;

    /* Temporary string for language token */
    std::string lang_buf;

    /*
     * Interpret arg1 and arg2 in common ways.
     *
     * arg1 -> often object
     * arg2 -> often character
     */
    CHAR_DATA *vch = (CHAR_DATA *)arg2;
    OBJ_DATA  *obj1 = (OBJ_DATA *)arg1;
    OBJ_DATA  *obj2 = (OBJ_DATA *)arg2;

    /* Defensive checks */
    if ( !format || !ch )
        return std::string();

    /*
     * Walk through the format string one character at a time
     */
    while ( *str )
    {
        /* If this is not a '$' token, just copy it directly */
        if ( *str != '$' )
        {
            out.push_back( *str++ );
            continue;
        }

        /* Skip the '$' */
        ++str;

        /* If format string ended after '$', stop safely */
        if ( !*str )
            break;

        /*
         * If a token requires arg2 but it was not provided,
         * log a bug and insert a placeholder.
         */
        if ( !arg2 && *str >= 'A' && *str <= 'Z' )
        {
            bug( "act_string: missing arg2 for code %c", *str );
            i = "<@@@>";
        }
        else
        {
            /*
             * Interpret token following '$'
             */
            switch ( *str )
            {
                default:
                    bug( "act_string: bad code %c", *str );
                    i = "<@@@>";
                    break;

                /* $t = string from arg1 */
                case 't':
                    i = arg1 ? (const char *)arg1 : "";
                    break;

                /* $T = string from arg2 */
                case 'T':
                    i = arg2 ? (const char *)arg2 : "";
                    break;

                /* $n = actor name (seen by 'to') */
                case 'n':
                    i = ( to ? PERS( ch, to ) : NAME( ch ) );
                    break;

                /* $N = victim name */
                case 'N':
                    i = vch ? ( to ? PERS( vch, to ) : NAME( vch ) ) : "someone";
                    break;

                /* $e / $E = he/she/it - CHAR */
                case 'e':
                    i = he_she[URANGE( 0, ch->sex, 2 )];
                    break;

                /* $E = he/she/it - VICT */
                case 'E':
                    i = vch ? he_she[URANGE( 0, vch->sex, 2 )] : "it";
                    break;

                /* $m / $M = him/her/it - CHAR */
                case 'm':
                    i = him_her[URANGE( 0, ch->sex, 2 )];
                    break;

                /* $M = him/her/it - VICT */
                case 'M':
                    i = vch ? him_her[URANGE( 0, vch->sex, 2 )] : "it";
                    break;

                /* $s / $S = his/her/its - CHAR */
                case 's':
                    i = his_her[URANGE( 0, ch->sex, 2 )];
                    break;

                /* $S = his/her/its - VICT */
                case 'S':
                    i = vch ? his_her[URANGE( 0, vch->sex, 2 )] : "its";
                    break;

                /*
                 * $q = plural suffix
                 */
                case 'q':
                    i = ( to == ch ) ? "" : "s";
                    break;

                /*
                 * $Q = possessive pronoun
                 */
                case 'Q':
                    i = ( to == ch ) ? "your"
                                     : his_her[URANGE( 0, ch->sex, 2 )];
                    break;

                /* $p = object short description */
                case 'p':
                    i = ( !obj1 ? "something"
                         : ( !to || can_see_obj( to, obj1 )
                             ? obj_short( obj1 )
                             : "something" ) );
                    break;

                /* $P = second object */
                case 'P':
                    i = ( !obj2 ? "something"
                         : ( !to || can_see_obj( to, obj2 )
                             ? obj_short( obj2 )
                             : "something" ) );
                    break;

                /*
                 * $d = door direction
                 * Extracts first word from arg2.
                 */
                case 'd':
                    if ( !arg2 || ((const char *)arg2)[0] == '\0' )
                        i = "door";
                    else
                    {
                        one_argument( std::string((char *)arg2), fname );
                        i = fname.c_str();
                    }
                    break;

                /* $l = language string */
                case 'l':
                    lang_buf = lang_string( ch, to );
                    i = lang_buf.c_str();
                    break;
            }
        }

        /* Move past token */
        ++str;

        if ( !i )
            i = "";

        /* Append token result safely */
        out += i;
    }

    /* Terminate message with newline */
    out += '\n';

    /*
     * Capitalize first visible character
     * (skip leading color codes)
     */
    size_t p = 0;

    while ( p + 1 < out.size() && ( out[p] == '&' || out[p] == '^' ) )
        p += 2;

    if ( p < out.size() )
        out[p] = UPPER( out[p] );

    return out;
}
#undef NAME

std::string act_string( const std::string &format, CHAR_DATA *to, CHAR_DATA *ch,
                        const void *arg1, const void *arg2 )
{
    return act_string( format.c_str(), to, ch, arg1, arg2 );
}

/*
 * act() - Cleaned up Act - DV 3-15-26
 *
 * Sends formatted action messages to characters depending on the
 * message target type (TO_CHAR, TO_ROOM, TO_VICT, etc).
 *
 * The format string can contain special tokens like:
 *   $n - acting character name
 *   $N - victim name
 *   $p - object name
 *   $t - string argument
 *
 * These tokens are expanded by act_string().
 *
 * Parameters:
 *   AType  - color code for the message
 *   format - format string containing tokens
 *   ch     - character performing the action
 *   arg1   - optional object or string argument
 *   arg2   - optional victim character
 *   type   - target audience (TO_CHAR, TO_ROOM, etc)
 */
void act_dir( sh_int AType, const std::string &format, CHAR_DATA *ch,
          const void *arg1, const void *arg2, int type )
{
    std::string txt;              /* Final expanded message string */
    CHAR_DATA *to;                /* Character receiving the message */
    CHAR_DATA *vch = (CHAR_DATA *)arg2; /* Victim character (if applicable) */

    /*
     * Ignore null or empty messages.
     */
    if ( format.empty() )
        return;

    /*
     * Acting character must exist.
     */
    if ( !ch )
    {
        bug( "Act: null ch. (%s)", format.c_str() );
        return;
    }

    /*
     * Determine who the initial recipient should be.
     */
    if ( !ch->in_room )
        to = NULL;
    else if ( type == TO_CHAR )
        to = ch;
    else
        to = ch->in_room->first_person;

    /*
     * ACT_SECRETIVE mobs do not broadcast actions to the room.
     */
    if ( IS_NPC(ch) && BV_IS_SET(ch->act, ACT_SECRETIVE) && type != TO_CHAR )
        return;

    /*
     * Handle victim-targeted messages.
     */
    if ( type == TO_VICT )
    {
        if ( !vch )
        {
            bug( "Act: null vch with TO_VICT." );
            bug( "%s (%s)", ch->name, format.c_str() );
            return;
        }

        if ( !vch->in_room )
        {
            bug( "Act: vch in NULL room!" );
            bug( "%s -> %s (%s)", ch->name, vch->name, format.c_str() );
            return;
        }

        to = vch;
    }

    /*
     * Trigger room and object ACT_PROG scripts if applicable.
     */
    if ( MOBtrigger && type != TO_CHAR && type != TO_VICT && to )
    {
        OBJ_DATA *to_obj;

        /* Generate message once (no recipient-specific context) */
        txt = act_string( format, NULL, ch, arg1, arg2 );

        if ( IS_SET(to->in_room->progtypes, ACT_PROG) )
            rprog_act_trigger( txt, to->in_room, ch,
                               (OBJ_DATA *)arg1, (void *)arg2 );

        for ( to_obj = to->in_room->first_content;
              to_obj;
              to_obj = to_obj->next_content )
        {
            if ( IS_SET(to_obj->pIndexData->progtypes, ACT_PROG) )
                oprog_act_trigger( txt, to_obj, ch,
                                   (OBJ_DATA *)arg1, (void *)arg2 );
        }
    }

    /*
     * Send the message to appropriate characters.
     */
    for ( ; to; to = (type == TO_CHAR || type == TO_VICT)
                     ? NULL : to->next_in_room )
    {
        if ( !to )
            continue;

        /*
         * Skip invalid recipients
         */
        if ( ((!to->desc)
             && (IS_NPC(to) && !IS_SET(to->pIndexData->progtypes, ACT_PROG)))
            || !IS_AWAKE(to) )
            continue;

        if ( !can_see(to, ch) && type != TO_VICT )
            continue;

        if ( type == TO_CHAR && to != ch )
            continue;

        if ( type == TO_VICT && ( to != vch || to == ch ) )
            continue;

        if ( type == TO_ROOM && to == ch )
            continue;

        if ( type == TO_NOTVICT && ( to == ch || to == vch ) )
            continue;

        /*
         * Expand message per recipient
         */
        txt = act_string( format, to, ch, arg1, arg2 );

        /*
         * Send message to player
         */
        if ( to->desc )
        {
            set_char_color( AType, to );
            flush_color( to->desc );
            send_to_char( txt, to );
        }

        /*
         * Mob ACT trigger
         */
        if ( MOBtrigger )
        {
            mprog_act_trigger( txt, to, ch,
                               (OBJ_DATA *)arg1, (void *)arg2 );
        }
    }

    /*
     * Ensure mob triggers remain enabled.
     */
    MOBtrigger = TRUE;
}

void act( sh_int AType, const std::string &format, CHAR_DATA *ch,
          const void *arg1, const void *arg2, int type )
{
    act_dir( AType, format, ch, arg1, arg2, type );
}
void act( sh_int AType, const char *format, CHAR_DATA *ch,
          const void *arg1, const void *arg2, int type )
{
    act_dir( AType, std::string( format ? format : "" ), ch, arg1, arg2, type );
}



//Added lots of overloads, because I know I or someone will screw this up eventually.  So $T and $t can be std::string or const char*.
void act( sh_int AType, const std::string &format, CHAR_DATA *ch,
          const std::string *arg1, const void *arg2, int type )
{
    act_dir( AType, format.c_str(), ch, arg1 ? arg1->c_str() : nullptr, arg2, type );
}

void act( sh_int AType, const std::string &format, CHAR_DATA *ch,
          const void *arg1, const std::string *arg2, int type )
{
    act_dir( AType, format.c_str(), ch, arg1, arg2 ? arg2->c_str() : nullptr, type );
}

void act( sh_int AType, const char *format, CHAR_DATA *ch,
          const std::string &arg1, const void *arg2, int type )
{
    act_dir( AType, std::string( format ? format : "" ), ch, arg1.c_str(), arg2, type );
}

void act( sh_int AType, const std::string &format, CHAR_DATA *ch,
          const void *arg1, const std::string &arg2, int type )
{
    act( AType, format.c_str(), ch, arg1, arg2.c_str(), type );
}

void act( sh_int AType, const std::string &format, CHAR_DATA *ch,
          const std::string &arg1, const void *arg2, int type )
{
    act_dir( AType, format, ch, arg1.c_str(), arg2, type );
}

void act( sh_int AType, const std::string &format, CHAR_DATA *ch,
          const std::string *arg1, const std::string *arg2, int type )
{
    act_dir( AType, format.c_str(), ch,
         arg1 ? arg1->c_str() : nullptr,
         arg2 ? arg2->c_str() : nullptr,
         type );
}


void do_name( CHAR_DATA *ch, char *argument )
{
  char fname[1024];
  struct stat fst;
  CHAR_DATA *tmp;
  std::string buf;

  if ( !NOT_AUTHED(ch) || ch->pcdata->auth_state != 2)
  {
    send_to_char("Huh?\n", ch);
    return;
  }

  argument[0] = UPPER(argument[0]);

  if (!check_parse_name(argument))
  {
    send_to_char("Illegal name, try another.\n", ch);
    return;
  }

  if (!str_cmp(ch->name, argument))
  {
    send_to_char("That's already your name!\n", ch);
    return;
  }

  for ( tmp = first_char; tmp; tmp = tmp->next )
  {
    if (!str_cmp(argument, tmp->name))
    break;
  }

  if ( tmp )
  {
    send_to_char("That name is already taken.  Please choose another.\n", ch);
    return;
  }

  SPRINTF( fname, "%s%c/%s", PLAYER_DIR, tolower(argument[0]),
                        capitalize( argument ).c_str() );
  if ( stat( fname, &fst ) != -1 )
  {
    send_to_char("That name is already taken.  Please choose another.\n", ch);
    return;
  }

  STRFREE( ch->name );
  ch->name = STRALLOC( argument );
  buf = str_printf("%s the %s",ch->name,
    race_table[ch->race].race_name );
  set_title( ch, buf );
  
  send_to_char("Your name has been changed.  Please apply again.\n", ch);
  ch->pcdata->auth_state = 1;
  return;
}
  
char *default_prompt( CHAR_DATA *ch )
{
  static char buf[MAX_STRING_LENGTH];
  buf[0] = '\0';
  if (ch->skill_level[FORCE_ABILITY] > 1 || get_trust(ch) >= LEVEL_IMMORTAL )
    STRAPP_RAW(buf, "&pForce:&P%m/&p%M  &pAlign:&P%a\n");
  STRAPP_RAW(buf, "&BHealth:&C%h&B/%H  &BMovement:&C%v&B/%V");
  STRAPP_RAW(buf, "&C >&w");
  return buf;
}

int getcolor(char clr)
{
  static const char colors[17] = "xrgObpcwzRGYBPCW";
  int r;
  
  for ( r = 0; r < 16; r++ )
    if ( clr == colors[r] )
      return r;
  return -1;
}

void send_eor(DESCRIPTOR_DATA *d)
{
    if (!d->eor_enabled)
        return;

    const unsigned char eor[] = { IAC, EOR };
    send_telnet(d, eor, 2);
}

void display_prompt( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch = d->character;
    CHAR_DATA *och = ( d->original ? d->original : d->character );
    bool ansi = ( !IS_NPC( och ) && BV_IS_SET( och->act, PLR_ANSI ) );
    const char *prompt;
    int stat;

    if ( !ch )
    {
        bug( "display_prompt: NULL ch" );
        return;
    }

    if ( !IS_NPC( ch ) && ch->substate != SUB_NONE
    &&   ch->pcdata->subprompt
    &&   ch->pcdata->subprompt[0] != '\0' )
        prompt = ch->pcdata->subprompt;
    else if ( IS_NPC( ch ) || !ch->pcdata->prompt || !*ch->pcdata->prompt )
        prompt = default_prompt( ch );
    else
        prompt = ch->pcdata->prompt;

    if ( ansi )
        set_char_color( 7, ch );  /* reset to default */

    std::string out;

    for ( ; *prompt; ++prompt )
    {
        /*
         * '&' = foreground color/intensity bit
         * '^' = background color/blink bit
         * '%' = prompt commands
         * Note: foreground changes will revert background to 0 (black)
         */
        if ( *prompt != '%' )
        {
            out.push_back( *prompt );
            continue;
        }

        ++prompt;
        if ( !*prompt )
            break;

        if ( *prompt == *( prompt - 1 ) )
        {
            out.push_back( *prompt );
            continue;
        }

        switch ( *( prompt - 1 ) )
        {
            default:
                bug( "Display_prompt: bad command char '%c'.", *( prompt - 1 ) );
                break;

            case '&':
            case '^':
                break;

            case '%':
                stat = 0x80000000;

                switch ( *prompt )
                {
                    case '%':
                        out.push_back( '%' );
                        break;

                    case 'a':
                        if ( ch->top_level >= 10 )
                            stat = ch->alignment;
                        else if ( IS_GOOD( ch ) )
                            out += "good";
                        else if ( IS_EVIL( ch ) )
                            out += "evil";
                        else
                            out += "neutral";
                        break;

                    case 'h':
                        stat = ch->hit;
                        break;

                    case 'H':
                        stat = ch->max_hit;
                        break;

                    case 'm':
                        if ( IS_IMMORTAL( ch ) || ch->skill_level[FORCE_ABILITY] > 1 )
                            stat = ch->mana;
                        else
                            stat = 0;
                        break;

                    case 'M':
                        if ( IS_IMMORTAL( ch ) || ch->skill_level[FORCE_ABILITY] > 1 )
                            stat = ch->max_mana;
                        else
                            stat = 0;
                        break;
                    case 'n':
                        out.push_back('\n');
                        break;
                    case 'p':
                        if ( ch->position == POS_RESTING )
                            out += "resting";
                        else if ( ch->position == POS_SLEEPING )
                            out += "sleeping";
                        else if ( ch->position == POS_SITTING )
                            out += "sitting";
                        break;

                    case 'u':
                        stat = num_descriptors;
                        break;

                    case 'U':
                        stat = d->game->get_sysdata()->maxplayers;
                        break;

                    case 'v':
                        stat = ch->move;
                        break;

                    case 'V':
                        stat = ch->max_move;
                        break;

                    case 'g':
                        stat = ch->gold;
                        break;

                    case 'r':
                        if ( IS_IMMORTAL( och ) && ch->in_room )
                            stat = ch->in_room->vnum;
                        break;

                    case 'R':
                        if ( BV_IS_SET( och->act, PLR_ROOMVNUM ) && ch->in_room )
                            out += str_printf( "<#%d> ", ch->in_room->vnum );
                        break;

                    case 'i':
                        if ( ( !IS_NPC( ch ) && BV_IS_SET( ch->act, PLR_WIZINVIS ) )
                        ||   ( IS_NPC( ch ) && BV_IS_SET( ch->act, ACT_MOBINVIS ) ) )
                        {
                            out += str_printf(
                                "(Invis %d) ",
                                ( IS_NPC( ch ) ? ch->mobinvis : ch->pcdata->wizinvis ) );
                        }
                        else if ( IS_AFFECTED( ch, AFF_INVISIBLE ) )
                            out += "(Invis) ";
                        break;

                    case 'I':
                        stat = ( IS_NPC( ch )
                            ? ( BV_IS_SET( ch->act, ACT_MOBINVIS ) ? ch->mobinvis : 0 )
                            : ( BV_IS_SET( ch->act, PLR_WIZINVIS ) ? ch->pcdata->wizinvis : 0 ) );
                        break;
                }

                if ( stat != (int)0x80000000 )
                    out += str_printf( "%d", stat );
                break;
        }
    }

    gmcp_flush( d );
    output_to_descriptor( d, out );
}

void set_pager_input( DESCRIPTOR_DATA *d, const std::string &argument )
{
    const char *p = argument.c_str();
    while ( *p && isspace_utf8( p ) )
        UTF8_NEXT( p );
    d->pagecmd = *p;
}

bool pager_output( DESCRIPTOR_DATA *d )
{
    size_t last;
    CHAR_DATA *ch;
    int pclines;
    int lines;

    if ( !d || d->pagepoint == PAGEPOINT_NULL || d->pagecmd == -1 )
        return TRUE;

    ch = d->original ? d->original : d->character;

    int term_lines = 0;

    if ( d->term_height > 0 )
        term_lines = d->term_height - 2;  /* leave room for prompt */
    else
        term_lines = 24;                  /* sane fallback */

    pclines = UMAX( ch->pcdata->pagerlen, 5 );

    /* Use the smaller of user setting vs terminal size */
    if ( term_lines > 0 )
        pclines = UMIN( pclines, term_lines );

    pclines = UMAX( pclines - 1, 3 );

    switch ( LOWER( d->pagecmd ) )
    {
        default:
            lines = 0;
            break;

        case 'b':
            lines = -1 - ( pclines * 2 );
            break;

        case 'r':
            lines = -1 - pclines;
            break;

        case 'q':
            d->pagebuf.clear();
            d->pagepoint = PAGEPOINT_NULL;
            flush_buffer( d, TRUE );
            return TRUE;
    }

    /* =========================
     * BACKWARD SCAN
     * ========================= */
    while ( lines < 0 && d->pagepoint > 0 )
    {
        d->pagepoint--;

        if ( d->pagebuf[d->pagepoint] == '\n' )
            ++lines;
    }

    /* =========================
     * HANDLE \n\r
     * ========================= */
    while ( d->pagepoint < d->pagebuf.size()
        && ( d->pagebuf[d->pagepoint] == '\n'
          || d->pagebuf[d->pagepoint] == '\r' ) )
    {
        d->pagepoint++;
    }

    /* =========================
     * FORWARD SCAN
     * ========================= */
    size_t line_end = d->pagepoint;

    for ( lines = 0, last = d->pagepoint;
          last < d->pagebuf.size() && lines < pclines;
          ++last )
    {
        if ( d->pagebuf[last] == '\n' )
        {
            line_end = last + 1;
            ++lines;
        }
    }

    /* =========================
     * OUTPUT CHUNK
     * ========================= */
    if ( line_end == d->pagepoint )
        line_end = last;  /* fallback if no newline found */

    if ( line_end != d->pagepoint )
    {
        output_to_descriptor(
            d,
            d->pagebuf.substr( d->pagepoint, line_end - d->pagepoint ) );

        d->pagepoint = line_end;
    }

    /* =========================
     * END OF BUFFER
     * ========================= */
    last = d->pagepoint;

    if ( last >= d->pagebuf.size() )
    {
        d->pagebuf.clear();
        d->pagepoint = PAGEPOINT_NULL;
        flush_buffer( d, TRUE );
        return TRUE;
    }

    d->pagecmd = -1;
    d->pagecolor = d->rendercolor;

    if ( BV_IS_SET( ch->act, PLR_ANSI ) )
        set_char_color( AT_LBLUE, ch );

    send_to_char( "(C)ontinue, (R)efresh, (B)ack, (Q)uit: [C] ", ch );

    if ( BV_IS_SET( ch->act, PLR_ANSI ) )
    {
        d->rendercolor = d->pagecolor;
        d->has_rendercolor = true;
    }

    flush_color( d );
    flush_buffer( d, FALSE );
    return TRUE;
}

void whocount(struct tm tmdata)
{
  char datebuf[32];
  char timebuf[32];
  char countbuf[64];
  int imm_count = 0;
  int count = 0;
  DESCRIPTOR_DATA *d;

  count = 0;
  SPRINTF(datebuf,"%d/%d/%d",
	      tmdata.tm_mon+1, tmdata.tm_mday, 1900+tmdata.tm_year);
  SPRINTF(timebuf,"%d:%d",tmdata.tm_hour, tmdata.tm_min);
  for ( d = last_descriptor; d; d = d->prev ) {
    count++;
    if (d->character) {
      if (d->character->top_level > 100)
        imm_count++;
      else
        player_count++;
    }
  }
  snprintf(countbuf, sizeof(countbuf),"%d %d %d",player_count,imm_count,count);
  if (fork() == 0) { 
    execl("../bin/scripts/whocount.pl","../bin/scripts/whocount.pl",datebuf,timebuf,countbuf,NULL);
    exit(0);
  }
}

void cron()
{
  struct tm lastTM;
  struct tm thisTM;
  static time_t lastCheck = 0;
  time_t thisCheck;
  int status;
  if (lastCheck == 0) {
    lastCheck = time(NULL);
    return;
  }
  thisCheck = time(NULL);
  //If it is only 60 seconds difference leave
  
  if (thisCheck-lastCheck< 60) 
    return;

  localtime_r(&thisCheck,&thisTM);
  localtime_r(&lastCheck,&lastTM);
  //Check the minute, range 0-59
  //Enters once during the minute
  if (thisTM.tm_min != lastTM.tm_min) {

#ifndef _WIN32    
    //clean up dead children every 5 minutes
    if (thisTM.tm_min % 5 == 0) {
      while(waitpid(-1,&status,WNOHANG) >= 0);
    }
#endif  
  }
  //Check the hour, range 0-23 
  //Enters once during the hour
  if (thisTM.tm_hour != lastTM.tm_hour) {
    
    //Do this stuff every hour
    whocount(thisTM);

  }
  //Check the day, range 0-31
  //Enters once during the day
  if (thisTM.tm_mday != lastTM.tm_mday) {
  
    //Do this stuff every day
    if (fork() == 0) {
      execl("../bin/scripts/daily.sh","../bin/scripts/daily.sh",NULL);
      exit(0);
    }

  }
  lastCheck = thisCheck;
}


#ifdef MCCP
/*
 * Ported to SMAUG by Garil of DOTDII Mud
 * aka Jesse DeFer <dotd@dotd.com>  http://www.dotd.com
 *
 * revision 1: MCCP v1 support
 * revision 2: MCCP v2 support
 * revision 3: Correct MMCP v2 support
 * revision 4: clean up of write_to_descriptor() suggested by Noplex@CB
 *
 * See the web site below for more info.
 */

/*
 * mccp.c - support functions for mccp (the Mud Client Compression Protocol)
 *
 * see http://homepages.ihug.co.nz/~icecube/compress/ and README.Rom24-mccp
 *
 * Copyright (c) 1999, Oliver Jowett <icecube@ihug.co.nz>.
 *
 * This code may be freely distributed and used if this copyright notice is
 * retained intact.
 */

void do_mccpstat(CHAR_DATA *ch, char *argument)
{
    if (!ch->desc || !ch->desc->out_compress)
    {
        send_to_char("MCCP is not active.\n\r", ch);
        return;
    }

    DESCRIPTOR_DATA *d = ch->desc;

    if (d->mccp_bytes_in == 0)
    {
        send_to_char("No compression data yet.\n\r", ch);
        return;
    }

    double ratio = 100.0 *
        (1.0 - ((double)d->mccp_bytes_out / (double)d->mccp_bytes_in));

    ch_printf(ch,
        "MCCP stats:\n\r"
        "  Raw: %zu bytes\n\r"
        "  Compressed: %zu bytes\n\r"
        "  Savings: %.2f%%\n\r",
        d->mccp_bytes_in,
        d->mccp_bytes_out,
        ratio);
}

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
    return calloc(items, size);
}

void zlib_free(void *opaque, void *address)
{
    STR_DISPOSE(address);
}

bool process_compressed(DESCRIPTOR_DATA *d)
{
    int iStart = 0, nBlock, nWrite, len;

    if (!d->out_compress)
        return TRUE;

    // Try to write out some data..
    len = d->out_compress->next_out - d->out_compress_buf;

    if (len > 0)
    {
        // we have some data to write
        for (iStart = 0; iStart < len; iStart += nWrite)
        {
            nBlock = len - iStart;
            if ((nWrite = write(d->descriptor, d->out_compress_buf + iStart, nBlock)) < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOSR)
                    break;

                return FALSE;
            }
            if (nWrite > 0)
            {
                d->mccp_bytes_out += nWrite;
            }                
            if (!nWrite)
                break;
        }

        if (iStart)
        {
            // We wrote "iStart" bytes
            if (iStart < len)
                memmove(d->out_compress_buf, d->out_compress_buf+iStart, len - iStart);

            d->out_compress->next_out = d->out_compress_buf + len - iStart;
        }
    }

    return TRUE;
}

char enable_compress[] =
{
    (char)IAC, (char)SB, (char)TELOPT_COMPRESS, (char)WILL, (char)SE, 0
};
char enable_compress2[] =
{
    (char)IAC, (char)SB, (char)TELOPT_COMPRESS2, (char)IAC, (char)SE, 0
};

bool compressStart(DESCRIPTOR_DATA *d, unsigned char telopt)
{
    z_stream *s;

    if (d->out_compress)
        return TRUE;

    bug("Starting compression for descriptor %d", d->descriptor);
    TELLOG(d, "MCCP: START (COMPRESS%d)", telopt == TELOPT_COMPRESS2 ? 2 : 1);  

    CREATE(s, z_stream, 1);
    CREATE_ARRAY(d->out_compress_buf, unsigned char, COMPRESS_BUF_SIZE);

    s->next_in = NULL;
    s->avail_in = 0;

    s->next_out = d->out_compress_buf;
    s->avail_out = COMPRESS_BUF_SIZE;

    s->zalloc = zlib_alloc;
    s->zfree  = zlib_free;
    s->opaque = NULL;

    d->mccp_bytes_in  = 0;
    d->mccp_bytes_out = 0;

    if (deflateInit(s, 9) != Z_OK)
    {
        STR_DISPOSE(d->out_compress_buf);
        DISPOSE(s);
        return FALSE;
    }

    {
        static const unsigned char start[] =
        {
            IAC, SB, TELOPT_COMPRESS2, IAC, SE
        };

        send_telnet(d, start, sizeof(start));
    }

    d->compressing = telopt;
    d->out_compress = s;

    return TRUE;
}

bool compressEnd(DESCRIPTOR_DATA *d)
{
    unsigned char dummy[1];

    if (!d->out_compress)
        return TRUE;

    bug("Stopping compression for descriptor %d", d->descriptor);
    TELLOG(d, "MCCP: END");
    if (d->mccp_bytes_in > 0)
    {
        double ratio = 100.0 *
            (1.0 - ((double)d->mccp_bytes_out / (double)d->mccp_bytes_in));

        bug("MCCP stats for descriptor %d: in=%zu, out=%zu, saved=%.2f%%",
            d->descriptor,
            d->mccp_bytes_in,
            d->mccp_bytes_out,
            ratio);
    }

    d->out_compress->avail_in = 0;
    d->out_compress->next_in = dummy;

    if (deflate(d->out_compress, Z_FINISH) == Z_STREAM_END)
      process_compressed(d); /* try to send any residual data */

    deflateEnd(d->out_compress);
    DISPOSE_ARRAY(d->out_compress_buf);
    DISPOSE(d->out_compress);
    d->compressing = 0;

    return TRUE;
}

void do_compress( CHAR_DATA *ch, char *argument )
{
    if (!ch->desc) {
        send_to_char("What descriptor?!\n", ch);
        return;
    }

    if (!ch->desc->out_compress) {
        send_to_char("Initiating compression.\n", ch);
        output_to_descriptor( ch->desc, (const char *)compress2_on_str );
        output_to_descriptor( ch->desc, (const char *)compress_on_str );
    } else {
        send_to_char("Terminating compression.\n", ch);
        compressEnd(ch->desc);
    }

}
#endif

static bool is_divider_line(const char *s)
{
    int count = 0;

    for (; *s; s++)
    {
        if (*s == '-' || *s == '=' || *s == '_')
            count++;
        else if (!isspace(*s) && *s != '&') /* allow color codes */
            return FALSE;
    }

    return count >= 10; /* long enough to be a divider */
}

static std::string format_divider_line( const std::string& src, int width )
{
    char fill = '-';

    /* detect fill character */
    for ( char c : src )
    {
        if ( c == '-' || c == '=' || c == '_' )
        {
            fill = c;
            break;
        }
    }

    /* preserve leading color codes like &C */
    std::string prefix;
    size_t p = 0;

    while ( p + 1 < src.size() && src[p] == '&' )
    {
        prefix.push_back( src[p++] );
        prefix.push_back( src[p++] );
    }

    std::string out;
    out.reserve( prefix.size() + width + 1 );
    out += prefix;
    out.append( width, fill );
    out.push_back( '\n' );

    return out;
}

std::string wrap_text_ex( const std::string& txt, int width, int flags, int indent )
{
    if ( txt.empty() )
        return std::string();

    if ( width <= 0 || ( flags & WRAP_NO_WRAP ) )
        return txt;

    if ( indent >= width )
        indent = ( width > 0 ) ? width - 1 : 0;

    const int len = static_cast<int>( txt.size() );

    std::string out;
    out.reserve( txt.size() * 4 + 1 );

    int col = 0;
    int line = 0;
    int wrapped_line = 0;

#define WORD_MAX 4096
#define VIS_MAX 4096

    char word[WORD_MAX];
    int wlen = 0;
    int wcol = 0; /* visible width */
    int vis_to_raw[VIS_MAX];
    memset( vis_to_raw, 0, sizeof( vis_to_raw ) );

    auto append_char = [&]( char c )
    {
        out.push_back( c );
    };

    auto append_bytes = [&]( const char *src, int n )
    {
        if ( n > 0 )
            out.append( src, static_cast<size_t>( n ) );
    };

    auto apply_indent = [&]()
    {
        if ( ( flags & WRAP_INDENT )
        || ( ( flags & WRAP_HANGING_INDENT ) && wrapped_line ) )
        {
            for ( int k = 0; k < indent; ++k )
            {
                out.push_back( ' ' );
                col++;
            }
        }
    };

    auto flush_word = [&]()
    {
        if ( wlen > 0 )
        {
            append_bytes( word, wlen );
            col += wcol;

            wlen = 0;
            wcol = 0;
            memset( vis_to_raw, 0, sizeof( vis_to_raw ) );
        }
    };

    apply_indent();

    for ( int i = 0; i < len; i++ )
    {
        char c = txt[i];

        /* =========== PRESERVE LINES =========== */
        if ( flags & WRAP_PRESERVE_LINES )
        {
            if ( c == '\r' )
                continue;

            if ( c == '\n' )
            {
                append_char( '\n' );
                col = 0;
                line++;
                wrapped_line = 0;
                apply_indent();
                continue;
            }

            /* otherwise fall through and allow wrapping */
        }

        /* ================= ANSI ================= */
        if ( !( flags & WRAP_NO_COLOR ) && c == '\x1b' )
        {
            if ( wlen >= WORD_MAX - 2 )
                goto flush_word_label;

            word[wlen++] = c;
            i++;

            int ansi_len = 0;
            while ( i < len && ansi_len < 32 )
            {
                if ( wlen < WORD_MAX - 1 )
                    word[wlen++] = txt[i];

                if ( txt[i++] == 'm' )
                    break;

                ansi_len++;
            }

            i--;
            continue;
        }

        /* ================= SMAUG COLOR ================= */
        if ( !( flags & WRAP_NO_COLOR ) && c == '&' && i + 1 < len )
        {
            if ( txt[i + 1] == '&' )
            {
                if ( wlen < WORD_MAX - 2 && wcol < VIS_MAX )
                {
                    /* store BOTH characters */
                    word[wlen++] = '&';
                    word[wlen++] = '&';

                    /* but count as ONE visible char */
                    vis_to_raw[wcol] = wlen - 1;
                    wcol++;
                }
                else
                    goto flush_word_label;

                i++; /* skip second '&' */
                continue;
            }

            if ( wlen < WORD_MAX - 2 )
            {
                word[wlen++] = txt[i++];
                word[wlen++] = txt[i];
            }
            continue;
        }

        /* ================= NEWLINE ================= */
        if ( c == '\n' || c == '\r' )
        {
flush_word_label:
            flush_word();

            if ( c == '\n' )
            {
                append_char( '\n' );
                col = 0;
                line++;
                wrapped_line = 0;
                apply_indent();
            }
            continue;
        }

        /* ================= TAB ================= */
        if ( c == '\t' )
        {
            int spaces = 4 - ( col % 4 );

            for ( int s = 0; s < spaces; s++ )
            {
                if ( wlen < WORD_MAX - 1 && wcol < VIS_MAX )
                {
                    word[wlen++] = ' ';
                    vis_to_raw[wcol] = wlen - 1;
                    wcol++;
                }
                else
                    goto flush_word_label;
            }
            continue;
        }

        /* ================= SPACE ================= */
        if ( c == ' ' )
        {
            /* flush word first */
            if ( wlen > 0 )
            {
                if ( col + wcol > width )
                {
                    append_char( '\n' );
                    col = 0;
                    line++;
                    wrapped_line = 1;
                    apply_indent();
                }

                flush_word();
            }

            /* then add space */
            if ( col + 1 > width )
            {
                append_char( '\n' );
                col = 0;
                line++;
                wrapped_line = 1;
                apply_indent();
            }
            else
            {
                append_char( ' ' );
                col++;
            }

            continue;
        }

        /* ================= NORMAL CHAR ================= */
        if ( wlen >= WORD_MAX - 4 || wcol >= VIS_MAX )
            goto flush_word_label;

        unsigned char uc = (unsigned char)c;
        int char_len = utf8_char_len( uc );

        if ( i + char_len > len )
            char_len = 1;

        for ( int j = 0; j < char_len; j++ )
            word[wlen++] = txt[i + j];

        vis_to_raw[wcol] = wlen - 1;   /* map visible → last byte */
        wcol++;

        i += char_len - 1;

        /* ============= LONG WORD HANDLING ============== */
        while ( wcol > width )
        {
            int split_vis = ( col == 0 ) ? width : ( width - col );

            if ( split_vis <= 0 )
            {
                append_char( '\n' );
                col = 0;
                line++;
                wrapped_line = 1;
                apply_indent();
                split_vis = width;
            }

            int split_raw;
            if ( wcol == 0 || split_vis - 1 < 0 )
            {
                split_raw = wlen;
            }
            else
            {
                split_raw = vis_to_raw[split_vis - 1] + 1;

                /* prevent splitting inside ANSI */
                int check = split_raw - 1;

                while ( check >= 0 )
                {
                    if ( word[check] == 'm' )
                        break;

                    if ( word[check] == '\x1b' )
                    {
                        int fwd = split_raw;
                        while ( fwd < wlen && word[fwd] != 'm' )
                            fwd++;

                        if ( fwd < wlen )
                            split_raw = fwd + 1;
                        else
                            split_raw = wlen;

                        break;
                    }

                    check--;
                }
            }

            append_bytes( word, split_raw );

            memmove( word, word + split_raw, static_cast<size_t>( wlen - split_raw ) );

            int new_wlen = wlen - split_raw;
            int new_wcol = wcol - split_vis;

            for ( int k = 0; k < new_wcol; k++ )
                vis_to_raw[k] = vis_to_raw[k + split_vis] - split_raw;

            wlen = new_wlen;
            wcol = new_wcol;

            col += split_vis;

            if ( col >= width )
            {
                append_char( '\n' );
                col = 0;
                line++;
                wrapped_line = 1;
                apply_indent();
            }
        }
    }

    /* final word flush */
    if ( wlen > 0 )
    {
        if ( col + wcol > width )
        {
            append_char( '\n' );
            col = 0;
            line++;
            wrapped_line = 1;
            apply_indent();
        }

        append_bytes( word, wlen );
    }

    return out;
}

static inline bool is_smaug_color(char c)
{
    return isalnum((unsigned char)c);
}

size_t visible_length(const std::string&txt)
{
    return visible_length(txt.c_str());
}

size_t visible_length(const char *txt)
{
    if (!txt)
        return 0;

    size_t len = 0;

    for (int i = 0; txt[i] != '\0';)
    {
        /* =========================
         * ANSI escape sequence
         * ========================= */
        if (txt[i] == '\x1b')
        {
            i++; /* skip ESC */

            /* Skip until 'm' (end of SGR sequence) */
            while (txt[i] && txt[i] != 'm')
                i++;

            if (txt[i] == 'm')
                i++;

            /* reached end of string safely */
            break;
        }
        /* =========================
         * SMAUG color codes (&x, ^x)
         * ========================= */
        if ((txt[i] == '&' || txt[i] == '^'))
        {
            if (txt[i + 1] != '\0' && is_smaug_color(txt[i + 1]))
            {
                i += 2;  // skip color code
                continue;
            }
        }

        /* =========================
         * UTF-8 handling
         * ========================= */
        unsigned char c = (unsigned char)txt[i];
        int char_len = utf8_char_len(c);

        /* Prevent overrunning string on malformed UTF-8 */
        for (int j = 1; j < char_len; j++)
        {
            if (txt[i + j] == '\0')
            {
                char_len = 1;
                break;
            }
        }

        i += char_len;   /* advance full UTF-8 char */
        len++;           /* visible width = 1 */
    }

    return len;
}

size_t visible_length_range(const char *start, const char *end, int flags)
{
    size_t len = 0;
    const char *p = start;

    while (p < end && *p)
    {
        /* =========================
         * ANSI escape sequence
         * ========================= */
        if (!(flags & WRAP_NO_COLOR) && *p == '\x1b')
        {
            p++; /* skip ESC */

            while (p < end && *p != 'm')
                p++;

            if (p < end)
                p++; /* consume 'm' */

            continue;
        }

        /* =========================
         * SMAUG color codes
         * ========================= */
        if (!(flags & WRAP_NO_COLOR) && (*p == '&' || *p == '^'))
        {
            if ((p + 1) < end && is_smaug_color(*(p + 1)))
            {
                p += 2;
                continue;
            }
        }

        /* =========================
         * UTF-8 handling
         * ========================= */
        unsigned char c = (unsigned char)*p;
        int char_len = utf8_char_len(c);

        /* Clamp to end to prevent overflow */
        for (int j = 1; j < char_len; j++)
        {
            if ((p + j) >= end || *(p + j) == '\0')
            {
                char_len = 1;
                break;
            }
        }

        p += char_len;
        len++;
    }

    return len;
}

bool looks_preformatted( const std::string& txt )
{
    if ( txt.empty() )
        return FALSE;

    if ( visible_length( txt ) == 0 )
        return TRUE;

    int lines = 0;
    int short_lines = 0;
    int long_lines = 0;
    int indented_lines = 0;

    const char *start = txt.c_str();

    while ( *start == '\n' )
        ++start;

    const char *p = start;
    const char *line_start = p;

    while ( *p )
    {
        if ( *p == '\n' )
        {
            size_t len = visible_length_range( line_start, p, 0 );

            /* Skip empty lines entirely */
            if ( len == 0 )
            {
                line_start = p + 1;
                ++p;
                continue;
            }

            lines++;

            if ( len < 60 )
                short_lines++;

            if ( len > 90 )
                long_lines++;

            /* Detect indentation (spaces at start) */
            if ( *line_start == ' ' || *line_start == '\t' )
                indented_lines++;

            line_start = p + 1;
        }

        ++p;
    }

    /* Handle last line (no trailing newline) */
    if ( line_start != p )
    {
        size_t len = visible_length_range( line_start, p, 0 );
        lines++;

        if ( len < 60 )
            short_lines++;

        if ( len > 90 )
            long_lines++;

        if ( *line_start == ' ' || *line_start == '\t' )
            indented_lines++;
    }

    /*
     * ============================================================
     * Heuristics
     * ============================================================
     */

    if ( lines <= 1 )
        return FALSE;

    /*
     * Strong signal: lots of short lines
     */
    if ( lines > 3 && short_lines > lines / 2 )
        return TRUE;

    /*
     * Strong signal: indentation present (helps, lists)
     */
    if ( indented_lines > 0 && lines > 3 )
        return TRUE;

    /*
     * Mixed lengths (manual formatting)
     */
    if ( short_lines > 0 && long_lines > 0 )
        return TRUE;

    return FALSE;
}

bool is_structured_line( const std::string& line )
{
    if ( line.empty() )
        return FALSE;

    /* Indentation */
    if ( line[0] == ' ' || line[0] == '\t' )
        return TRUE;

    /* Bullets */
    if ( line[0] == '-' || line[0] == '*' || line[0] == '+' )
        return TRUE;

    /* Table-ish */
    if ( line.find( '|' ) != std::string::npos )
        return TRUE;

    return FALSE;
}

std::string wrap_text_smart( const std::string& txt, int width )
{
    if ( txt.empty() )
        return std::string();

    if ( visible_length( txt ) == 0 )
        return txt;

    std::string out;
    out.reserve( txt.size() * 2 + 32 );

    const char *p = txt.c_str();
    const char *txt_end = p + txt.size();

    while ( *p )
    {
        /*
         * ============================================
         * STEP 1: Find paragraph boundary safely
         * Supports:
         *   "\n\n"
         *   "\n\r\n\r"
         * ============================================
         */
        const char *start = p;
        const char *end = nullptr;

        const char *scan = p;
        while ( *scan )
        {
            if ( scan[0] == '\n' && scan[1] == '\n' )
            {
                end = scan;
                break;
            }
            if ( scan[0] == '\n' && scan[1] == '\r'
              && scan[2] == '\n' && scan[3] == '\r' )
            {
                end = scan;
                break;
            }
            ++scan;
        }

        if ( !end )
            end = txt_end;

        size_t plen = static_cast<size_t>( end - start );

        /*
         * Skip empty paragraphs cleanly
         */
        if ( plen == 0 )
        {
            if ( end[0] == '\n' && end[1] == '\n' )
                p = end + 2;
            else
                p = end;  /* fallback safety */

            continue;
        }

        /*
         * ============================================
         * STEP 2: Copy paragraph into std::string
         * ============================================
         */
        std::string para( start, plen );

        /*
         * ============================================
         * STEP 3: Decide formatting strategy
         * ============================================
         */
        std::string processed;

        if ( is_divider_line( para.c_str() ) )
        {
            processed = format_divider_line( para, width );
        }
        else if ( is_structured_line( para ) ) // || looks_preformatted(para)
        {
            /*
             * Pass through unchanged
             */
            processed = para;
        }
        else
        {
            /*
             * Main wrapping path
             *
             * IMPORTANT:
             * wrap_text_ex MUST be ANSI-aware:
             *   - skip \033 sequences
             *   - do NOT count them toward width
             *   - do NOT split them
             */
            std::string wrapped = wrap_text_ex( para, width, WRAP_NONE, 0 );
            processed = wrapped;
        }

        /*
         * ============================================
         * STEP 4: Append to output buffer
         * ============================================
         */
        out += processed;

        /*
         * ============================================
         * STEP 5: Preserve paragraph spacing
         * ============================================
         */
        if ( *end )
        {
            out += "\n\n";

            /* Advance pointer past delimiter */
            if ( end[0] == '\r' )
                p = end + 4;  // \r\n\r\n
            else
                p = end + 2;  // \n\n
        }
        else
        {
            break;
        }
    }

    return out;
}

void send_telnet(DESCRIPTOR_DATA *d, const unsigned char *data, size_t len)
{
    if (!d || d->descriptor < 0 || !data || len == 0)
        return;

    if (d->debug_telnet && len >= 2 && data[0] == IAC)
    {
        char buf[256];
        int pos = 0;

        pos += snprintf(buf + pos, sizeof(buf) - pos, "IAC ");

        for (size_t i = 1; i < len && pos < (int)sizeof(buf) - 10; i++)
        {
            unsigned char c = data[i];
            if (c == IAC)
                pos += snprintf(buf + pos, sizeof(buf) - pos, "IAC ");
            else if (c == SB || c == SE || c == DO || c == WILL || c == WONT || c == DONT)
                pos += snprintf(buf + pos, sizeof(buf) - pos, "%s ", telcmd(c));
            else
                pos += snprintf(buf + pos, sizeof(buf) - pos, "%s ", telopt(c));
        }

        fprintf(stderr, "[TELNET] SEND: %s\r\n", buf);
    }

    /* 🔥 Single call is enough */
    net_write(d, data, len);
}

ssize_t net_write_raw(DESCRIPTOR_DATA *d, const unsigned char *data, size_t len)
{
    ssize_t total = 0;

    while (total < (ssize_t)len)
    {
        ssize_t n = write(d->descriptor, data + total, len - total);

        if (n <= 0)
        {
            if (errno == EINTR)
                continue;

            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;

            perror("net_write_raw");
            close_socket(d, TRUE);
            return -1;
        }

        total += n;
    }

    return total;
}

ssize_t net_write(DESCRIPTOR_DATA *d, const unsigned char *data, size_t len)
{
#ifdef MCCP
    if (d->compressing == TELOPT_COMPRESS2)
    {
        z_stream *s = d->out_compress;

        s->next_in  = (unsigned char *)data;
        s->avail_in = len;

        size_t total_out = 0;

        while (s->avail_in > 0)
        {
            /* === FIX: use persistent buffer instead of stack === */
            s->next_out  = d->out_compress_buf;        // ✅ WAS: stack buffer
            s->avail_out = COMPRESS_BUF_SIZE;

            if (deflate(s, Z_SYNC_FLUSH) != Z_OK)
                return -1;

            size_t have = COMPRESS_BUF_SIZE - s->avail_out;

            if (have > 0)
            {
                if (net_write_raw(d, d->out_compress_buf, have) < 0)
                    return -1;

                total_out += have;
            }
        }

        return total_out; /* approximate */
    }
#endif

    return net_write_raw(d, data, len);
}

void send_gmcp(DESCRIPTOR_DATA *d, const char *msg)
{
    unsigned char buf[2048];
    int len = 0;

    int msg_len = strlen(msg);

    /* NEW: bounds check */
    if (msg_len > (int)sizeof(buf) - 6)
        msg_len = sizeof(buf) - 6;

    buf[len++] = IAC;
    buf[len++] = SB;
    buf[len++] = TELOPT_GMCP;

    /* SAFE COPY WITH IAC ESCAPING */
    for (int i = 0; i < msg_len && len < (int)sizeof(buf) - 2; i++)
    {
        unsigned char c = (unsigned char)msg[i];

        if (c == IAC)
        {
            /* Duplicate IAC */
            if (len < (int)sizeof(buf) - 3)
            {
                buf[len++] = IAC;
                buf[len++] = IAC;
            }
            else
            {
                break; /* avoid overflow */
            }
        }
        else
        {
            buf[len++] = c;
        }
    }
//    len += msg_len;

    buf[len++] = IAC;
    buf[len++] = SE;

    write_to_buffer_raw(d, buf, len);

//    send_telnet(d, buf, len);
}

#define MSSP_VAR 1
#define MSSP_VAL 2

extern int			top_area;
extern int			top_help;
extern int			top_mob_index;
extern int			top_obj_index;
extern int			top_room;


void send_mssp(DESCRIPTOR_DATA *d)
{
    unsigned char buf[2048];
    int len = 0;
    if (!d)
        return;

    buf[len++] = IAC;
    buf[len++] = SB;
    buf[len++] = TELOPT_MSSP;

#define ADD(name, value) \
    do { \
        buf[len++] = MSSP_VAR; \
        len += snprintf((char *)buf + len, sizeof(buf) - len, "%s", name); \
        buf[len++] = MSSP_VAL; \
        len += snprintf((char *)buf + len, sizeof(buf) - len, "%s", value); \
    } while (0)

    char players[16], uptime[32], port[16];
    char areas[16], rooms[16], mobs[16], objs[16];
    char levels[16], classes[16], races[16], skills[16];
    char helps[16];

    snprintf(players, sizeof(players), "%d", player_count);
    snprintf(uptime, sizeof(uptime), "%ld", (long)boot_time);
    snprintf(port, sizeof(uptime), "%d", control);
    snprintf(areas, sizeof(areas), "%d", top_area);
    snprintf(rooms, sizeof(rooms), "%d", top_room);
    snprintf(mobs, sizeof(mobs), "%d", top_mob_index);
    snprintf(objs, sizeof(objs), "%d", top_obj_index);
    snprintf(helps, sizeof(helps), "%d", top_help);
    snprintf(levels, sizeof(levels), "%d", MAX_LEVEL);
    snprintf(classes, sizeof(classes), "%d", MAX_RL_ABILITY);
    snprintf(races, sizeof(races), "%d", MAX_RACE);
    snprintf(skills, sizeof(skills), "%d", top_sn);


    ADD("NAME", "SWRIP2");
    ADD("PLAYERS", players);
    ADD("UPTIME", uptime);
    ADD("HOSTNAME", "swrip.net");    
    ADD("PORT", port);

    /* Classification */
    ADD("CODEBASE", "SMAUG");    
    ADD("CODEBASE", "SWR");
    ADD("CODEBASE", "SWRIP2");    
    ADD("FAMILY", "DikuMUD");
    ADD("GENRE", "Science Fiction");
    ADD("GAMEPLAY", "Roleplaying");
    ADD("STATUS", "Closed Beta");

    /* World */
    ADD("AREAS", areas);
    ADD("ROOMS", rooms);
    ADD("MOBILES", mobs);
    ADD("OBJECTS", objs);    

    /* Player system */
    ADD("LEVELS", levels);
    ADD("CLASSES", classes);
    ADD("RACES", races);
    ADD("SKILLS", skills);

    /* Capabilities */
    ADD("ANSI", "1");
    ADD("GMCP", "1");
    ADD("MCCP", "1");

    /* Optional but useful */
    ADD("CREATED", "2001?");
    ADD("LANGUAGE", "English");
    ADD("LOCATION", "USA");
    ADD("WEBSITE", "https://swrip.com");    
    ADD("CONTACT", "darrik@swrip.net");    

    ADD("CRAWL DELAY", "-1");

#undef ADD

    buf[len++] = IAC;
    buf[len++] = SE;

    send_telnet(d, buf, len);
}